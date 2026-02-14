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

    void EnforceReviewPolicy(int approved_count,
                             int min_reviewers,
                             const std::string& action_id,
                             const std::string& advisory_state) const;

    void ValidateExtensionRuntime(bool signature_ok,
                                  bool compatibility_ok,
                                  const std::set<std::string>& requested_capabilities,
                                  const std::set<std::string>& allowlist) const;

    std::pair<std::vector<std::string>, int> BuildLineage(
        const std::vector<std::string>& node_ids,
        const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) const;

    std::map<std::string, std::optional<std::string>> RegisterOptionalSurfaces(const std::string& profile_id) const;

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
};

}  // namespace scratchrobin::advanced
