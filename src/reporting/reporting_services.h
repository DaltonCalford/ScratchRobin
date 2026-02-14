#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "connection/connection_services.h"
#include "core/beta1b_contracts.h"

namespace scratchrobin::reporting {

struct ResultMetadata {
    bool exists = false;
    std::size_t bytes = 0;
};

class ReportingService {
public:
    explicit ReportingService(connection::BackendAdapterService* adapter);

    std::string RunQuestion(bool question_exists, const std::string& normalized_sql);
    std::string RunDashboard(const std::string& dashboard_id,
                             const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                             bool cache_hit);

    void StoreResult(const std::string& key, const std::string& payload);
    std::string RetrieveResult(const std::string& key) const;
    ResultMetadata QueryResultMetadata(const std::string& key) const;

    std::string ExportRepository(const std::vector<beta1b::ReportingAsset>& assets) const;
    std::vector<beta1b::ReportingAsset> ImportRepository(const std::string& payload_json) const;

    std::string CanonicalizeSchedule(const std::map<std::string, std::string>& key_values) const;
    std::string NextRun(const beta1b::ReportingSchedule& schedule, const std::string& now_utc) const;
    std::vector<std::string> ExpandSchedule(const beta1b::ReportingSchedule& schedule,
                                            const std::string& now_utc,
                                            std::size_t max_candidates) const;

    std::vector<beta1b::ActivityRow> RunActivityQuery(const std::vector<beta1b::ActivityRow>& source,
                                                      const std::string& window,
                                                      const std::set<std::string>& allowed_metrics) const;
    std::string ExportActivity(const std::vector<beta1b::ActivityRow>& rows, const std::string& format) const;

    std::pair<std::vector<beta1b::ActivityRow>, std::size_t> RetentionCleanup(
        const std::vector<beta1b::ActivityRow>& rows,
        const std::string& retain_from_utc) const;

private:
    connection::BackendAdapterService* adapter_ = nullptr;
    std::map<std::string, std::string> storage_;
};

}  // namespace scratchrobin::reporting
