#include "advanced/advanced_services.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "core/reject.h"

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

void AdvancedService::UpsertMaskingProfile(const std::string& profile_id,
                                           const std::map<std::string, std::string>& rules) {
    if (profile_id.empty()) {
        throw MakeReject("SRB1-R-7005", "masking profile id missing", "advanced", "upsert_masking_profile");
    }
    (void)beta1b::PreviewMask({{}}, rules);
    masking_profiles_[profile_id] = rules;
}

std::vector<std::map<std::string, std::string>> AdvancedService::PreviewMaskWithProfile(
    const std::string& profile_id,
    const std::vector<std::map<std::string, std::string>>& rows) const {
    auto it = masking_profiles_.find(profile_id);
    if (it == masking_profiles_.end()) {
        throw MakeReject("SRB1-R-7005", "masking profile missing", "advanced", "preview_mask_with_profile", false, profile_id);
    }
    return beta1b::PreviewMask(rows, it->second);
}

CdcBatchResult AdvancedService::RunCdcBatch(
    const std::vector<std::string>& events,
    int max_attempts,
    int backoff_ms,
    const std::function<bool(const std::string&)>& publish) {
    CdcBatchResult result;
    for (const auto& event : events) {
        try {
            (void)beta1b::RunCdcEvent(
                event,
                max_attempts,
                backoff_ms,
                publish,
                [&](const std::string& dead_letter) { dead_letter_queue_.push_back(dead_letter); });
            ++result.published;
        } catch (const RejectError&) {
            ++result.dead_lettered;
        }
    }
    return result;
}

std::vector<std::string> AdvancedService::DeadLetterQueue() const {
    return dead_letter_queue_;
}

void AdvancedService::EnforceReviewPolicy(int approved_count,
                                          int min_reviewers,
                                          const std::string& action_id,
                                          const std::string& advisory_state) const {
    beta1b::CheckReviewQuorum(approved_count, min_reviewers);
    beta1b::RequireChangeAdvisory(action_id, advisory_state);
}

void AdvancedService::CreateReviewAction(const std::string& action_id, const std::string& advisory_state) {
    if (action_id.empty()) {
        throw MakeReject("SRB1-R-7301", "review action id missing", "advanced", "create_review_action");
    }
    review_approvals_[action_id] = {};
    review_advisory_state_[action_id] = advisory_state;
}

void AdvancedService::ApproveReviewAction(const std::string& action_id, const std::string& reviewer_id) {
    if (reviewer_id.empty() || review_approvals_.count(action_id) == 0U) {
        throw MakeReject("SRB1-R-7301", "review approval invalid", "advanced", "approve_review_action");
    }
    review_approvals_[action_id].insert(reviewer_id);
}

void AdvancedService::EnforceReviewAction(const std::string& action_id, int min_reviewers) const {
    auto approvals_it = review_approvals_.find(action_id);
    auto advisory_it = review_advisory_state_.find(action_id);
    if (approvals_it == review_approvals_.end() || advisory_it == review_advisory_state_.end()) {
        throw MakeReject("SRB1-R-7301", "review action not registered", "advanced", "enforce_review_action", false, action_id);
    }
    beta1b::CheckReviewQuorum(static_cast<int>(approvals_it->second.size()), min_reviewers);
    beta1b::RequireChangeAdvisory(action_id, advisory_it->second);
}

void AdvancedService::ValidateExtensionRuntime(bool signature_ok,
                                               bool compatibility_ok,
                                               const std::set<std::string>& requested_capabilities,
                                               const std::set<std::string>& allowlist) const {
    beta1b::ValidateExtension(signature_ok, compatibility_ok);
    beta1b::EnforceExtensionAllowlist(requested_capabilities, allowlist);
}

void AdvancedService::RegisterExtensionPackage(const std::string& package_id,
                                               const std::string& signature_sha256,
                                               const std::string& compatibility_tag,
                                               const std::set<std::string>& capabilities) {
    const bool signature_hex = signature_sha256.size() == 64U &&
                               std::all_of(signature_sha256.begin(), signature_sha256.end(), [](char c) {
                                   return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                               });
    if (package_id.empty() || !signature_hex || compatibility_tag.empty() || capabilities.empty()) {
        throw MakeReject("SRB1-R-7303", "extension package registration invalid", "advanced", "register_extension_package");
    }
    ValidateExtensionRuntime(true, true, capabilities, capabilities);
    extension_capabilities_[package_id] = capabilities;
}

void AdvancedService::ExecuteExtensionPackage(const std::string& package_id,
                                              const std::set<std::string>& requested_capabilities,
                                              const std::set<std::string>& sandbox_allowlist) const {
    auto it = extension_capabilities_.find(package_id);
    if (it == extension_capabilities_.end()) {
        throw MakeReject("SRB1-R-7303", "unknown extension package", "advanced", "execute_extension_package", false, package_id);
    }
    ValidateExtensionRuntime(true, true, requested_capabilities, it->second);
    beta1b::EnforceExtensionAllowlist(requested_capabilities, sandbox_allowlist);
}

std::pair<std::vector<std::string>, int> AdvancedService::BuildLineage(
    const std::vector<std::string>& node_ids,
    const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const {
    return beta1b::BuildLineage(node_ids, edges);
}

std::vector<LineageDepthRow> AdvancedService::BuildLineageDepth(
    const std::vector<std::string>& node_ids,
    const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const {
    std::map<std::string, std::vector<std::string>> children_by_parent;
    std::set<std::string> has_parent;
    std::set<std::string> unresolved;
    for (const auto& [node_id, parent] : edges) {
        if (!parent.has_value()) {
            unresolved.insert(node_id);
            continue;
        }
        children_by_parent[*parent].push_back(node_id);
        has_parent.insert(node_id);
    }
    for (auto& [_, children] : children_by_parent) {
        std::sort(children.begin(), children.end());
    }

    std::vector<std::string> roots;
    for (const auto& node : node_ids) {
        if (has_parent.count(node) == 0U) {
            roots.push_back(node);
        }
    }
    std::sort(roots.begin(), roots.end());

    std::vector<LineageDepthRow> out;
    std::queue<std::pair<std::string, int>> q;
    for (const auto& root : roots) {
        q.push({root, 0});
    }
    std::set<std::string> seen;
    while (!q.empty()) {
        const auto [node, depth] = q.front();
        q.pop();
        if (!seen.insert(node).second) {
            continue;
        }
        out.push_back({node, depth, unresolved.count(node) > 0U});
        auto children_it = children_by_parent.find(node);
        if (children_it != children_by_parent.end()) {
            for (const auto& child : children_it->second) {
                q.push({child, depth + 1});
            }
        }
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return std::tie(a.depth, a.node_id) < std::tie(b.depth, b.node_id);
    });
    return out;
}

std::map<std::string, std::optional<std::string>> AdvancedService::RegisterOptionalSurfaces(
    const std::string& profile_id) const {
    return beta1b::RegisterOptionalSurfaces(profile_id);
}

std::string AdvancedService::OpenClusterManager(const std::string& profile_id, const std::string& cluster_id) const {
    auto gates = RegisterOptionalSurfaces(profile_id);
    if (gates["ClusterManagerFrame"].has_value()) {
        throw MakeReject(*gates["ClusterManagerFrame"], "cluster manager surface disabled in profile", "advanced",
                         "open_cluster_manager");
    }
    if (cluster_id.empty()) {
        throw MakeReject("SRB1-R-7008", "cluster_id required", "advanced", "open_cluster_manager");
    }
    return std::string("{\"surface\":\"ClusterManagerFrame\",\"cluster_id\":\"") + cluster_id + "\"}";
}

std::string AdvancedService::OpenReplicationManager(const std::string& profile_id, const std::string& replication_id) const {
    auto gates = RegisterOptionalSurfaces(profile_id);
    if (gates["ReplicationManagerFrame"].has_value()) {
        throw MakeReject(*gates["ReplicationManagerFrame"], "replication manager surface disabled in profile", "advanced",
                         "open_replication_manager");
    }
    if (replication_id.empty()) {
        throw MakeReject("SRB1-R-7009", "replication_id required", "advanced", "open_replication_manager");
    }
    return std::string("{\"surface\":\"ReplicationManagerFrame\",\"replication_id\":\"") + replication_id + "\"}";
}

std::string AdvancedService::OpenEtlManager(const std::string& profile_id, const std::string& job_id) const {
    auto gates = RegisterOptionalSurfaces(profile_id);
    if (gates["EtlManagerFrame"].has_value()) {
        throw MakeReject(*gates["EtlManagerFrame"], "etl manager surface disabled in profile", "advanced",
                         "open_etl_manager");
    }
    if (job_id.empty()) {
        throw MakeReject("SRB1-R-7010", "job_id required", "advanced", "open_etl_manager");
    }
    return std::string("{\"surface\":\"EtlManagerFrame\",\"job_id\":\"") + job_id + "\"}";
}

std::string AdvancedService::OpenDockerManager(const std::string& profile_id, const std::string& operation) const {
    auto gates = RegisterOptionalSurfaces(profile_id);
    if (gates["DockerManagerPanel"].has_value()) {
        throw MakeReject(*gates["DockerManagerPanel"], "docker manager surface disabled in profile", "advanced",
                         "open_docker_manager");
    }
    if (operation.empty()) {
        throw MakeReject("SRB1-R-7011", "operation required", "advanced", "open_docker_manager");
    }
    return std::string("{\"surface\":\"DockerManagerPanel\",\"operation\":\"") + operation + "\"}";
}

std::string AdvancedService::OpenTestRunner(const std::string& profile_id, const std::string& suite_id) const {
    auto gates = RegisterOptionalSurfaces(profile_id);
    if (gates["TestRunnerPanel"].has_value()) {
        throw MakeReject(*gates["TestRunnerPanel"], "test runner surface disabled in profile", "advanced",
                         "open_test_runner");
    }
    if (suite_id.empty()) {
        throw MakeReject("SRB1-R-7012", "suite_id required", "advanced", "open_test_runner");
    }
    return std::string("{\"surface\":\"TestRunnerPanel\",\"suite_id\":\"") + suite_id + "\"}";
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
