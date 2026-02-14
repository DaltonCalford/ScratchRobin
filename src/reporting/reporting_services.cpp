#include "reporting/reporting_services.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <tuple>

#include "core/reject.h"

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
    activity_rows_.clear();
    repository_payload_.clear();

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
    const std::string result = beta1b::RunQuestion(
        question_exists,
        normalized_sql,
        [&](const std::string& sql) {
            auto query = adapter_->ExecuteQuery(sql);
            return std::string("{\"command_tag\":\"") + query.command_tag + "\",\"rows_affected\":" +
                   std::to_string(query.rows_affected) + "}";
        },
        [&](const std::string& payload) {
            StoreResult("question:" + normalized_sql, payload);
            return true;
        });
    return result;
}

std::string ReportingService::RunDashboard(const std::string& dashboard_id,
                                           const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                                           bool cache_hit) {
    const auto payload = beta1b::RunDashboardRuntime(dashboard_id, widget_statuses, cache_hit);
    StoreResult("dashboard:" + dashboard_id, payload);
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

std::string ReportingService::ExportRepository(const std::vector<beta1b::ReportingAsset>& assets) const {
    return beta1b::ExportReportingRepository(assets);
}

std::vector<beta1b::ReportingAsset> ReportingService::ImportRepository(const std::string& payload_json) const {
    return beta1b::ImportReportingRepository(payload_json);
}

void ReportingService::SaveRepositoryAssets(const std::vector<beta1b::ReportingAsset>& assets) {
    repository_payload_ = beta1b::ExportReportingRepository(assets);
    PersistRepository();
}

std::vector<beta1b::ReportingAsset> ReportingService::LoadRepositoryAssets() {
    if (repository_payload_.empty() && !persistence_root_.empty()) {
        LoadPersistentState();
    }
    if (repository_payload_.empty()) {
        return {};
    }
    return beta1b::ImportReportingRepository(repository_payload_);
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
