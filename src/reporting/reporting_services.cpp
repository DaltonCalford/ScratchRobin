#include "reporting/reporting_services.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <tuple>

#include "core/reject.h"
#include "core/sha256.h"

namespace scratchrobin::reporting {

namespace {

bool IsRfc3339UtcTimestamp(const std::string& text) {
    static const std::regex pattern(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$)");
    return std::regex_match(text, pattern);
}

std::vector<std::string> SplitTab(const std::string& line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, '\t')) {
        out.push_back(item);
    }
    return out;
}

}  // namespace

ReportingService::ReportingService(connection::BackendAdapterService* adapter)
    : adapter_(adapter) {
    if (adapter_ == nullptr) {
        throw MakeReject("SRB1-R-7001", "reporting adapter missing", "reporting", "constructor");
    }
}

std::string ReportingService::NowUtc() {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return std::string(buffer);
}

std::int64_t ReportingService::NowEpochSeconds() {
    return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count());
}

void ReportingService::InitializeStorage() {
    LoadPersistentState();
}

void ReportingService::ShutdownStorage() {
    FlushPersistentState();
}

void ReportingService::SetPersistenceRoot(const std::string& root_path) {
    if (root_path.empty()) {
        throw MakeReject("SRB1-R-7002", "persistence root cannot be empty", "reporting", "set_persistence_root");
    }
    persistence_root_ = root_path;
    std::filesystem::create_directories(persistence_root_);
}

std::string ReportingService::PersistenceFile(const std::string& leaf) const {
    if (persistence_root_.empty()) {
        return "";
    }
    return (std::filesystem::path(persistence_root_) / leaf).string();
}

void ReportingService::LoadPersistentState() {
    if (persistence_root_.empty()) {
        return;
    }

    storage_.clear();
    cache_payload_by_key_.clear();
    cache_expiry_epoch_by_key_.clear();
    cache_keys_by_connection_.clear();
    cache_keys_by_model_.clear();
    activity_rows_.clear();
    repository_payload_.clear();
    repository_assets_by_id_.clear();

    {
        std::ifstream in(PersistenceFile("results.tsv"), std::ios::binary);
        std::string line;
        while (std::getline(in, line)) {
            const auto cols = SplitTab(line);
            if (cols.size() != 2U) {
                continue;
            }
            if (!cols[0].empty()) {
                storage_[cols[0]] = cols[1];
            }
        }
    }
    {
        std::ifstream in(PersistenceFile("activity.tsv"), std::ios::binary);
        std::string line;
        while (std::getline(in, line)) {
            const auto cols = SplitTab(line);
            if (cols.size() != 3U || !IsRfc3339UtcTimestamp(cols[0]) || cols[1].empty()) {
                continue;
            }
            try {
                activity_rows_.push_back(beta1b::ActivityRow{cols[0], cols[1], std::stod(cols[2])});
            } catch (...) {
                // Ignore malformed persisted row and continue reading.
            }
        }
    }
    {
        std::ifstream in(PersistenceFile("repository.json"), std::ios::binary);
        if (in) {
            std::ostringstream payload;
            payload << in.rdbuf();
            repository_payload_ = payload.str();
            if (!repository_payload_.empty()) {
                try {
                    const auto imported = beta1b::ImportReportingRepository(repository_payload_);
                    for (const auto& asset : imported) {
                        repository_assets_by_id_[asset.id] = asset;
                    }
                } catch (const RejectError&) {
                    repository_assets_by_id_.clear();
                }
            }
        }
    }
}

void ReportingService::PersistResults() const {
    if (persistence_root_.empty()) {
        return;
    }
    std::ofstream out(PersistenceFile("results.tsv"), std::ios::binary | std::ios::trunc);
    for (const auto& [key, payload] : storage_) {
        out << key << '\t' << payload << '\n';
    }
}

void ReportingService::PersistRepository() const {
    if (persistence_root_.empty()) {
        return;
    }
    std::ofstream out(PersistenceFile("repository.json"), std::ios::binary | std::ios::trunc);
    out << repository_payload_;
}

void ReportingService::PersistActivity() const {
    if (persistence_root_.empty()) {
        return;
    }
    std::ofstream out(PersistenceFile("activity.tsv"), std::ios::binary | std::ios::trunc);
    for (const auto& row : activity_rows_) {
        out << row.timestamp_utc << '\t' << row.metric_key << '\t' << row.value << '\n';
    }
}

void ReportingService::FlushPersistentState() const {
    PersistResults();
    PersistRepository();
    PersistActivity();
}

std::string ReportingService::RunQuestion(bool question_exists, const std::string& normalized_sql) {
    QueryExecutionContext ctx;
    ctx.connection_id = "default";
    ctx.role_id = "default";
    ctx.environment_id = "default";
    ctx.params_json = "{}";
    QueryExecutionOptions options;
    return RunQuestionWithContext("question:" + normalized_sql, question_exists, normalized_sql, ctx, options);
}

std::string ReportingService::RunQuestionWithContext(const std::string& question_id,
                                                     bool question_exists,
                                                     const std::string& normalized_sql,
                                                     const QueryExecutionContext& context,
                                                     const QueryExecutionOptions& options) {
    if (!question_exists) {
        throw MakeReject("SRB1-R-7001", "question not found", "reporting", "run_question_with_context");
    }
    if (context.connection_id.empty() || context.role_id.empty() || context.environment_id.empty()) {
        throw MakeReject("SRB1-R-7001", "execution context incomplete", "reporting", "run_question_with_context");
    }
    if (options.timeout_ms <= 0) {
        throw MakeReject("SRB1-R-7001", "invalid timeout option", "reporting", "run_question_with_context");
    }

    const std::string model_version = "v1";
    const std::string cache_key = BuildCacheKey(normalized_sql, context, options, model_version);
    const auto now_epoch = NowEpochSeconds();
    const auto cache_it = cache_payload_by_key_.find(cache_key);
    if (!options.bypass_cache && cache_it != cache_payload_by_key_.end()) {
        auto expiry_it = cache_expiry_epoch_by_key_.find(cache_key);
        if (expiry_it != cache_expiry_epoch_by_key_.end() && expiry_it->second > now_epoch) {
            std::ostringstream cached;
            cached << "{\"success\":true,\"query_result\":" << cache_it->second
                   << ",\"timing\":{\"elapsed_ms\":0},\"cache\":{\"hit\":true,\"cache_key\":\"" << cache_key
                   << "\",\"ttl_seconds\":" << (expiry_it->second - now_epoch)
                   << "},\"error\":{\"code\":\"\",\"message\":\"\"}}";
            StoreResult(question_id, cached.str());
            return cached.str();
        }
    }

    const std::string result = beta1b::RunQuestion(
        question_exists,
        normalized_sql,
        [&](const std::string& sql) {
            const auto begin = std::chrono::steady_clock::now();
            if (options.validate_only || options.dry_run) {
                std::ostringstream validate_payload;
                validate_payload << "{\"command_tag\":\"VALIDATE\",\"rows_affected\":0,\"messages\":[\""
                                 << (options.validate_only ? "validate-only" : "dry-run")
                                 << "\"],\"execution_context\":{\"connection_id\":\"" << context.connection_id
                                 << "\",\"role_id\":\"" << context.role_id
                                 << "\",\"environment_id\":\"" << context.environment_id << "\"}}";
                return validate_payload.str();
            }
            auto query = adapter_->ExecuteQuery(sql);
            const auto end = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            std::ostringstream query_payload;
            query_payload << "{\"command_tag\":\"" << query.command_tag
                          << "\",\"rows_affected\":" << query.rows_affected
                          << ",\"messages\":[";
            for (std::size_t i = 0; i < query.messages.size(); ++i) {
                if (i > 0U) {
                    query_payload << ",";
                }
                query_payload << "\"" << query.messages[i] << "\"";
            }
            query_payload << "],\"execution_context\":{\"connection_id\":\"" << context.connection_id
                          << "\",\"role_id\":\"" << context.role_id
                          << "\",\"environment_id\":\"" << context.environment_id
                          << "\",\"params\":" << (context.params_json.empty() ? "{}" : context.params_json)
                          << "},\"timing\":{\"adapter_elapsed_ms\":" << elapsed << "}}";
            return query_payload.str();
        },
        [&](const std::string& payload) {
            StoreResult(question_id, payload);
            cache_payload_by_key_[cache_key] = payload;
            cache_expiry_epoch_by_key_[cache_key] = now_epoch + 60;
            cache_keys_by_connection_[context.connection_id].insert(cache_key);
            cache_keys_by_model_[model_version].insert(cache_key);
            return true;
        });
    return result;
}

std::string ReportingService::RunDashboard(const std::string& dashboard_id,
                                           const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                                           bool cache_hit) {
    QueryExecutionContext ctx;
    ctx.connection_id = "default";
    ctx.role_id = "default";
    ctx.environment_id = "default";
    ctx.params_json = "{}";
    QueryExecutionOptions options;
    options.bypass_cache = !cache_hit;
    std::vector<DashboardWidgetRequest> widgets;
    widgets.reserve(widget_statuses.size());
    for (const auto& row : widget_statuses) {
        DashboardWidgetRequest widget;
        widget.widget_id = row.first;
        widget.dataset_key = "dataset:" + row.first;
        widget.normalized_sql = "select * from " + row.first;
        widgets.push_back(widget);
    }
    return RunDashboardWithQueries(dashboard_id, widgets, ctx, options);
}

std::string ReportingService::RunDashboardWithQueries(const std::string& dashboard_id,
                                                      const std::vector<DashboardWidgetRequest>& widgets,
                                                      const QueryExecutionContext& context,
                                                      const QueryExecutionOptions& options) {
    std::vector<std::pair<std::string, std::string>> statuses;
    statuses.reserve(widgets.size());
    for (const auto& widget : widgets) {
        if (widget.widget_id.empty() || widget.dataset_key.empty()) {
            throw MakeReject("SRB1-R-7001", "invalid dashboard widget contract", "reporting", "run_dashboard_with_queries");
        }
        if (options.validate_only) {
            statuses.push_back({widget.widget_id, "ok:0"});
            continue;
        }
        const auto query = adapter_->ExecuteQuery(widget.normalized_sql.empty() ? "select 1" : widget.normalized_sql);
        statuses.push_back({widget.widget_id, std::string("ok:") + std::to_string(std::max<std::int64_t>(0, query.rows_affected))});
    }
    const auto payload = beta1b::RunDashboardRuntime(dashboard_id, statuses, !options.bypass_cache);
    StoreResult("dashboard:" + dashboard_id, payload);
    cache_payload_by_key_["dash:" + dashboard_id] = payload;
    cache_expiry_epoch_by_key_["dash:" + dashboard_id] = NowEpochSeconds() + 60;
    cache_keys_by_connection_[context.connection_id].insert("dash:" + dashboard_id);
    cache_keys_by_model_["v1"].insert("dash:" + dashboard_id);
    return payload;
}

void ReportingService::StoreResult(const std::string& key, const std::string& payload) {
    beta1b::PersistResult(key, payload, &storage_);
    PersistResults();
}

std::string ReportingService::RetrieveResult(const std::string& key) {
    if (storage_.empty() && !persistence_root_.empty()) {
        LoadPersistentState();
    }
    auto it = storage_.find(key);
    if (it == storage_.end()) {
        throw MakeReject("SRB1-R-7002", "result storage retrieve/metadata incomplete path", "reporting", "retrieve_result",
                         false, key);
    }
    return it->second;
}

ResultMetadata ReportingService::QueryResultMetadata(const std::string& key) {
    if (storage_.empty() && !persistence_root_.empty()) {
        LoadPersistentState();
    }
    auto it = storage_.find(key);
    if (it == storage_.end()) {
        return ResultMetadata{};
    }
    ResultMetadata md;
    md.exists = true;
    md.bytes = it->second.size();
    return md;
}

std::size_t ReportingService::InvalidateCacheByConnection(const std::string& connection_id) {
    auto it = cache_keys_by_connection_.find(connection_id);
    if (it == cache_keys_by_connection_.end()) {
        return 0U;
    }
    std::size_t removed = 0U;
    for (const auto& cache_key : it->second) {
        removed += cache_payload_by_key_.erase(cache_key);
        cache_expiry_epoch_by_key_.erase(cache_key);
    }
    cache_keys_by_connection_.erase(it);
    return removed;
}

std::size_t ReportingService::InvalidateCacheByModel(const std::string& model_id) {
    auto it = cache_keys_by_model_.find(model_id);
    if (it == cache_keys_by_model_.end()) {
        return 0U;
    }
    std::size_t removed = 0U;
    for (const auto& cache_key : it->second) {
        removed += cache_payload_by_key_.erase(cache_key);
        cache_expiry_epoch_by_key_.erase(cache_key);
    }
    cache_keys_by_model_.erase(it);
    return removed;
}

std::size_t ReportingService::InvalidateAllCache() {
    const std::size_t removed = cache_payload_by_key_.size();
    cache_payload_by_key_.clear();
    cache_expiry_epoch_by_key_.clear();
    cache_keys_by_connection_.clear();
    cache_keys_by_model_.clear();
    return removed;
}

std::string ReportingService::ExportRepository(const std::vector<beta1b::ReportingAsset>& assets) const {
    return beta1b::ExportReportingRepository(assets);
}

std::vector<beta1b::ReportingAsset> ReportingService::ImportRepository(const std::string& payload_json) const {
    return beta1b::ImportReportingRepository(payload_json);
}

void ReportingService::SaveRepositoryAssets(const std::vector<beta1b::ReportingAsset>& assets) {
    repository_assets_by_id_.clear();
    for (const auto& asset : assets) {
        repository_assets_by_id_[asset.id] = asset;
    }
    std::vector<beta1b::ReportingAsset> canonical;
    canonical.reserve(repository_assets_by_id_.size());
    for (const auto& [_, asset] : repository_assets_by_id_) {
        canonical.push_back(asset);
    }
    repository_payload_ = beta1b::ExportReportingRepository(canonical);
    PersistRepository();
}

std::vector<beta1b::ReportingAsset> ReportingService::LoadRepositoryAssets() {
    if (repository_assets_by_id_.empty() && repository_payload_.empty() && !persistence_root_.empty()) {
        LoadPersistentState();
    }
    if (!repository_assets_by_id_.empty()) {
        std::vector<beta1b::ReportingAsset> out;
        out.reserve(repository_assets_by_id_.size());
        for (const auto& [_, asset] : repository_assets_by_id_) {
            out.push_back(asset);
        }
        return beta1b::CanonicalArtifactOrder(out);
    }
    if (repository_payload_.empty()) {
        return {};
    }
    return beta1b::ImportReportingRepository(repository_payload_);
}

void ReportingService::UpsertAsset(const beta1b::ReportingAsset& asset) {
    if (asset.id.empty() || asset.asset_type.empty() || asset.name.empty()) {
        throw MakeReject("SRB1-R-7003", "invalid reporting asset for upsert", "reporting", "upsert_asset");
    }
    beta1b::ReportingAsset value = asset;
    if (value.created_at_utc.empty()) {
        value.created_at_utc = NowUtc();
    }
    value.updated_at_utc = NowUtc();
    repository_assets_by_id_[value.id] = value;
    SaveRepositoryAssets(LoadRepositoryAssets());
}

std::optional<beta1b::ReportingAsset> ReportingService::GetAsset(const std::string& asset_id) const {
    auto it = repository_assets_by_id_.find(asset_id);
    if (it == repository_assets_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool ReportingService::DeleteAsset(const std::string& asset_id) {
    auto it = repository_assets_by_id_.find(asset_id);
    if (it == repository_assets_by_id_.end()) {
        return false;
    }
    repository_assets_by_id_.erase(it);
    SaveRepositoryAssets(LoadRepositoryAssets());
    return true;
}

std::vector<beta1b::ReportingAsset> ReportingService::ListAssetsByCollection(const std::string& collection_id) const {
    std::vector<beta1b::ReportingAsset> out;
    for (const auto& [_, asset] : repository_assets_by_id_) {
        if (asset.collection_id == collection_id) {
            out.push_back(asset);
        }
    }
    return beta1b::CanonicalArtifactOrder(out);
}

std::vector<beta1b::ReportingAsset> ReportingService::ListAssetsByType(const std::string& asset_type) const {
    std::vector<beta1b::ReportingAsset> out;
    for (const auto& [_, asset] : repository_assets_by_id_) {
        if (asset.asset_type == asset_type) {
            out.push_back(asset);
        }
    }
    return beta1b::CanonicalArtifactOrder(out);
}

std::string ReportingService::BuildCacheKey(const std::string& normalized_sql,
                                            const QueryExecutionContext& context,
                                            const QueryExecutionOptions& options,
                                            const std::string& model_version) const {
    std::ostringstream key_material;
    key_material << normalized_sql << "|"
                 << context.connection_id << "|"
                 << context.role_id << "|"
                 << context.environment_id << "|"
                 << context.params_json << "|"
                 << options.validate_only << "|"
                 << options.dry_run << "|"
                 << options.timeout_ms << "|"
                 << model_version;
    return "q:" + Sha256Hex(key_material.str());
}

std::string ReportingService::CanonicalizeSchedule(const std::map<std::string, std::string>& key_values) const {
    return beta1b::CanonicalizeRRule(key_values);
}

std::string ReportingService::NextRun(const beta1b::ReportingSchedule& schedule, const std::string& now_utc) const {
    return beta1b::NextRun(schedule, now_utc);
}

std::vector<std::string> ReportingService::ExpandSchedule(const beta1b::ReportingSchedule& schedule,
                                                          const std::string& now_utc,
                                                          std::size_t max_candidates) const {
    return beta1b::ExpandRRuleBounded(schedule, now_utc, max_candidates);
}

void ReportingService::AppendActivity(const beta1b::ActivityRow& row) {
    if (!IsRfc3339UtcTimestamp(row.timestamp_utc) || row.metric_key.empty()) {
        throw MakeReject("SRB1-R-7203", "invalid activity row", "reporting", "append_activity");
    }
    activity_rows_.push_back(row);
    PersistActivity();
}

std::vector<beta1b::ActivityRow> ReportingService::ActivityFeed() {
    if (activity_rows_.empty() && !persistence_root_.empty()) {
        LoadPersistentState();
    }
    auto out = activity_rows_;
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return std::tie(a.timestamp_utc, a.metric_key) < std::tie(b.timestamp_utc, b.metric_key);
    });
    return out;
}

std::vector<beta1b::ActivityRow> ReportingService::RunActivityQuery(
    const std::vector<beta1b::ActivityRow>& source,
    const std::string& window,
    const std::set<std::string>& allowed_metrics) const {
    return beta1b::RunActivityWindowQuery(source, window, allowed_metrics);
}

std::vector<beta1b::ActivityRow> ReportingService::RunActivityQueryFromFeed(
    const std::string& window,
    const std::set<std::string>& allowed_metrics) {
    return beta1b::RunActivityWindowQuery(ActivityFeed(), window, allowed_metrics);
}

std::vector<ActivitySummary> ReportingService::SummarizeActivity(const std::vector<beta1b::ActivityRow>& rows) const {
    std::map<std::string, ActivitySummary> by_metric;
    for (const auto& row : rows) {
        auto& agg = by_metric[row.metric_key];
        agg.metric_key = row.metric_key;
        agg.total_value += row.value;
        agg.sample_count += 1U;
    }
    std::vector<ActivitySummary> out;
    out.reserve(by_metric.size());
    for (const auto& [_, summary] : by_metric) {
        out.push_back(summary);
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.metric_key < b.metric_key;
    });
    return out;
}

std::string ReportingService::ExportActivity(const std::vector<beta1b::ActivityRow>& rows, const std::string& format) const {
    return beta1b::ExportActivity(rows, format);
}

    std::pair<std::vector<beta1b::ActivityRow>, std::size_t> ReportingService::RetentionCleanup(
    const std::vector<beta1b::ActivityRow>& rows,
    const std::string& retain_from_utc) const {
    if (!IsRfc3339UtcTimestamp(retain_from_utc)) {
        throw MakeReject("SRB1-R-7203", "activity dashboard freshness/retention contract violated", "reporting",
                         "retention_cleanup");
    }
    std::vector<beta1b::ActivityRow> kept;
    kept.reserve(rows.size());
    for (const auto& row : rows) {
        if (!IsRfc3339UtcTimestamp(row.timestamp_utc)) {
            throw MakeReject("SRB1-R-7203", "activity dashboard freshness/retention contract violated", "reporting",
                             "retention_cleanup");
        }
        if (row.timestamp_utc >= retain_from_utc) {
            kept.push_back(row);
        }
    }
    const std::size_t dropped = rows.size() - kept.size();
    std::sort(kept.begin(), kept.end(), [](const auto& a, const auto& b) {
        return std::tie(a.timestamp_utc, a.metric_key) < std::tie(b.timestamp_utc, b.metric_key);
    });
    return {kept, dropped};
}

}  // namespace scratchrobin::reporting
