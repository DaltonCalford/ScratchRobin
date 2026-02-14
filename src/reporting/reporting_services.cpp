#include "reporting/reporting_services.h"

#include <algorithm>
#include <regex>

#include "core/reject.h"

namespace scratchrobin::reporting {

namespace {

bool IsRfc3339UtcTimestamp(const std::string& text) {
    static const std::regex pattern(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$)");
    return std::regex_match(text, pattern);
}

}  // namespace

ReportingService::ReportingService(connection::BackendAdapterService* adapter)
    : adapter_(adapter) {
    if (adapter_ == nullptr) {
        throw MakeReject("SRB1-R-7001", "reporting adapter missing", "reporting", "constructor");
    }
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
    return beta1b::RunDashboardRuntime(dashboard_id, widget_statuses, cache_hit);
}

void ReportingService::StoreResult(const std::string& key, const std::string& payload) {
    beta1b::PersistResult(key, payload, &storage_);
}

std::string ReportingService::RetrieveResult(const std::string& key) const {
    auto it = storage_.find(key);
    if (it == storage_.end()) {
        throw MakeReject("SRB1-R-7002", "result storage retrieve/metadata incomplete path", "reporting", "retrieve_result",
                         false, key);
    }
    return it->second;
}

ResultMetadata ReportingService::QueryResultMetadata(const std::string& key) const {
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

std::vector<beta1b::ActivityRow> ReportingService::RunActivityQuery(
    const std::vector<beta1b::ActivityRow>& source,
    const std::string& window,
    const std::set<std::string>& allowed_metrics) const {
    return beta1b::RunActivityWindowQuery(source, window, allowed_metrics);
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
