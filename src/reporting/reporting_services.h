#pragma once

#include <map>
#include <tuple>
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

struct ActivitySummary {
    std::string metric_key;
    double total_value = 0.0;
    std::size_t sample_count = 0;
};

class ReportingService {
public:
    explicit ReportingService(connection::BackendAdapterService* adapter);

    void SetPersistenceRoot(const std::string& root_path);
    void LoadPersistentState();
    void FlushPersistentState() const;

    std::string RunQuestion(bool question_exists, const std::string& normalized_sql);
    std::string RunDashboard(const std::string& dashboard_id,
                             const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                             bool cache_hit);

    void StoreResult(const std::string& key, const std::string& payload);
    std::string RetrieveResult(const std::string& key);
    ResultMetadata QueryResultMetadata(const std::string& key);

    std::string ExportRepository(const std::vector<beta1b::ReportingAsset>& assets) const;
    std::vector<beta1b::ReportingAsset> ImportRepository(const std::string& payload_json) const;
    void SaveRepositoryAssets(const std::vector<beta1b::ReportingAsset>& assets);
    std::vector<beta1b::ReportingAsset> LoadRepositoryAssets();

    std::string CanonicalizeSchedule(const std::map<std::string, std::string>& key_values) const;
    std::string NextRun(const beta1b::ReportingSchedule& schedule, const std::string& now_utc) const;
    std::vector<std::string> ExpandSchedule(const beta1b::ReportingSchedule& schedule,
                                            const std::string& now_utc,
                                            std::size_t max_candidates) const;

    void AppendActivity(const beta1b::ActivityRow& row);
    std::vector<beta1b::ActivityRow> ActivityFeed();
    std::vector<beta1b::ActivityRow> RunActivityQuery(const std::vector<beta1b::ActivityRow>& source,
                                                      const std::string& window,
                                                      const std::set<std::string>& allowed_metrics) const;
    std::vector<beta1b::ActivityRow> RunActivityQueryFromFeed(const std::string& window,
                                                              const std::set<std::string>& allowed_metrics);
    std::vector<ActivitySummary> SummarizeActivity(const std::vector<beta1b::ActivityRow>& rows) const;
    std::string ExportActivity(const std::vector<beta1b::ActivityRow>& rows, const std::string& format) const;

    std::pair<std::vector<beta1b::ActivityRow>, std::size_t> RetentionCleanup(
        const std::vector<beta1b::ActivityRow>& rows,
        const std::string& retain_from_utc) const;

private:
    std::string PersistenceFile(const std::string& leaf) const;
    void PersistResults() const;
    void PersistRepository() const;
    void PersistActivity() const;

    connection::BackendAdapterService* adapter_ = nullptr;
    std::map<std::string, std::string> storage_;
    std::vector<beta1b::ActivityRow> activity_rows_;
    std::string repository_payload_;
    std::string persistence_root_;
};

}  // namespace scratchrobin::reporting
