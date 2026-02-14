#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/beta1b_contracts.h"

namespace scratchrobin::advanced {

struct CdcBatchResult {
    std::size_t published = 0;
    std::size_t dead_lettered = 0;
};

struct LineageDepthRow {
    std::string node_id;
    int depth = 0;
    bool unresolved_parent = false;
};

class AdvancedService {
public:
    std::string RunCdcEvent(const std::string& event_payload,
                            int max_attempts,
                            int backoff_ms,
                            const std::function<bool(const std::string&)>& publish,
                            const std::function<void(const std::string&)>& dead_letter) const;

    std::vector<std::map<std::string, std::string>> PreviewMask(
        const std::vector<std::map<std::string, std::string>>& rows,
        const std::map<std::string, std::string>& rules) const;
    void UpsertMaskingProfile(const std::string& profile_id, const std::map<std::string, std::string>& rules);
    std::vector<std::map<std::string, std::string>> PreviewMaskWithProfile(
        const std::string& profile_id,
        const std::vector<std::map<std::string, std::string>>& rows) const;
    CdcBatchResult RunCdcBatch(const std::vector<std::string>& events,
                               int max_attempts,
                               int backoff_ms,
                               const std::function<bool(const std::string&)>& publish);
    std::vector<std::string> DeadLetterQueue() const;

    void EnforceReviewPolicy(int approved_count,
                             int min_reviewers,
                             const std::string& action_id,
                             const std::string& advisory_state) const;
    void CreateReviewAction(const std::string& action_id, const std::string& advisory_state);
    void ApproveReviewAction(const std::string& action_id, const std::string& reviewer_id);
    void EnforceReviewAction(const std::string& action_id, int min_reviewers) const;

    void ValidateExtensionRuntime(bool signature_ok,
                                  bool compatibility_ok,
                                  const std::set<std::string>& requested_capabilities,
                                  const std::set<std::string>& allowlist) const;
    void RegisterExtensionPackage(const std::string& package_id,
                                  const std::string& signature_sha256,
                                  const std::string& compatibility_tag,
                                  const std::set<std::string>& capabilities);
    void ExecuteExtensionPackage(const std::string& package_id,
                                 const std::set<std::string>& requested_capabilities,
                                 const std::set<std::string>& sandbox_allowlist) const;

    std::pair<std::vector<std::string>, int> BuildLineage(
        const std::vector<std::string>& node_ids,
        const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const;
    std::vector<LineageDepthRow> BuildLineageDepth(
        const std::vector<std::string>& node_ids,
        const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const;

    std::map<std::string, std::optional<std::string>> RegisterOptionalSurfaces(const std::string& profile_id) const;
    std::string OpenClusterManager(const std::string& profile_id, const std::string& cluster_id) const;
    std::string OpenReplicationManager(const std::string& profile_id, const std::string& replication_id) const;
    std::string OpenEtlManager(const std::string& profile_id, const std::string& job_id) const;
    std::string OpenDockerManager(const std::string& profile_id, const std::string& operation) const;
    std::string OpenTestRunner(const std::string& profile_id, const std::string& suite_id) const;

    void ValidateAiProviderConfig(const std::string& provider_id,
                                  bool async_enabled,
                                  const std::string& endpoint_or_model,
                                  const std::optional<std::string>& credential) const;

    void ValidateIssueTrackerConfig(const std::string& provider_id,
                                    const std::string& project_or_repo,
                                    const std::optional<std::string>& credential) const;

    void ValidateGitSyncState(bool branch_selected,
                              bool remote_reachable,
                              bool conflicts_resolved) const;

private:
    std::map<std::string, std::map<std::string, std::string>> masking_profiles_;
    std::vector<std::string> dead_letter_queue_;
    std::map<std::string, std::set<std::string>> review_approvals_;
    std::map<std::string, std::string> review_advisory_state_;
    std::map<std::string, std::set<std::string>> extension_capabilities_;
};

}  // namespace scratchrobin::advanced
