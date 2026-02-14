#include "advanced/advanced_services.h"

namespace scratchrobin::advanced {

std::string AdvancedService::RunCdcEvent(const std::string& event_payload,
                                         int max_attempts,
                                         int backoff_ms,
                                         const std::function<bool(const std::string&)>& publish,
                                         const std::function<void(const std::string&)>& dead_letter) const {
    return beta1b::RunCdcEvent(event_payload, max_attempts, backoff_ms, publish, dead_letter);
}

std::vector<std::map<std::string, std::string>> AdvancedService::PreviewMask(
    const std::vector<std::map<std::string, std::string>>& rows,
    const std::map<std::string, std::string>& rules) const {
    return beta1b::PreviewMask(rows, rules);
}

void AdvancedService::EnforceReviewPolicy(int approved_count,
                                          int min_reviewers,
                                          const std::string& action_id,
                                          const std::string& advisory_state) const {
    beta1b::CheckReviewQuorum(approved_count, min_reviewers);
    beta1b::RequireChangeAdvisory(action_id, advisory_state);
}

void AdvancedService::ValidateExtensionRuntime(bool signature_ok,
                                               bool compatibility_ok,
                                               const std::set<std::string>& requested_capabilities,
                                               const std::set<std::string>& allowlist) const {
    beta1b::ValidateExtension(signature_ok, compatibility_ok);
    beta1b::EnforceExtensionAllowlist(requested_capabilities, allowlist);
}

std::pair<std::vector<std::string>, int> AdvancedService::BuildLineage(
    const std::vector<std::string>& node_ids,
    const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const {
    return beta1b::BuildLineage(node_ids, edges);
}

std::map<std::string, std::optional<std::string>> AdvancedService::RegisterOptionalSurfaces(
    const std::string& profile_id) const {
    return beta1b::RegisterOptionalSurfaces(profile_id);
}

void AdvancedService::ValidateAiProviderConfig(const std::string& provider_id,
                                               bool async_enabled,
                                               const std::string& endpoint_or_model,
                                               const std::optional<std::string>& credential) const {
    beta1b::ValidateAiProviderConfig(provider_id, async_enabled, endpoint_or_model, credential);
}

void AdvancedService::ValidateIssueTrackerConfig(const std::string& provider_id,
                                                 const std::string& project_or_repo,
                                                 const std::optional<std::string>& credential) const {
    beta1b::ValidateIssueTrackerConfig(provider_id, project_or_repo, credential);
}

void AdvancedService::ValidateGitSyncState(bool branch_selected,
                                           bool remote_reachable,
                                           bool conflicts_resolved) const {
    beta1b::ValidateGitSyncState(branch_selected, remote_reachable, conflicts_resolved);
}

}  // namespace scratchrobin::advanced
