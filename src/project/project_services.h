#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "core/beta1b_contracts.h"

namespace scratchrobin::project {

struct ProjectRoundTripResult {
    std::uint64_t bytes_written = 0;
    std::size_t toc_entries = 0;
    std::set<std::string> loaded_chunks;
};

class ProjectBinaryService {
public:
    std::vector<std::uint8_t> BuildBinary(const std::vector<std::uint8_t>& proj_payload,
                                          const std::vector<std::uint8_t>& objs_payload,
                                          const std::map<std::string, std::vector<std::uint8_t>>& optional_chunks = {}) const;

    ProjectRoundTripResult RoundTripFile(const std::string& path,
                                         const std::vector<std::uint8_t>& proj_payload,
                                         const std::vector<std::uint8_t>& objs_payload,
                                         const std::map<std::string, std::vector<std::uint8_t>>& optional_chunks = {}) const;

    beta1b::LoadedProjectBinary LoadFile(const std::string& path) const;
};

void ValidateProjectPayloadWithSchema(const std::string& schema_path, const JsonValue& payload);
void ValidateSpecsetPayloadWithSchema(const std::string& schema_path, const JsonValue& payload);

struct GovernanceInput {
    std::string action;
    std::string actor;
    std::string actor_role;
    std::string environment_id;
    std::string target_id;
    std::string connection_ref;
    bool ai_action = false;
    std::string ai_scope;
    int approval_count = 0;
    bool requires_guaranteed_audit = true;
};

struct GovernancePolicy {
    std::set<std::string> allowed_roles;
    int min_approval_count = 1;
    bool ai_enabled = true;
    bool ai_requires_review = false;
    std::set<std::string> ai_allowed_scopes;
};

struct GovernanceDecision {
    bool allowed = false;
    std::string reason;
};

GovernanceDecision EvaluateGovernance(const GovernanceInput& input, const GovernancePolicy& policy);

void ExecuteGovernedOperation(const GovernanceInput& input,
                              const GovernancePolicy& policy,
                              const std::string& audit_path,
                              const std::function<void()>& operation);

struct SpecSetIndex {
    beta1b::SpecSetManifest manifest;
    std::vector<beta1b::SpecFileRow> files;
    std::string indexed_at_utc;
};

class SpecSetService {
public:
    SpecSetIndex BuildIndex(const std::string& manifest_path, const std::string& indexed_at_utc) const;

    void AssertCoverageComplete(const SpecSetIndex& index,
                                const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
                                const std::string& coverage_class) const;

    void ValidateConformanceBindings(const std::vector<std::string>& binding_case_ids,
                                     const std::set<std::string>& conformance_case_ids) const;

    std::map<std::string, int> CoverageSummary(
        const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const;

    std::string ExportImplementationWorkPackage(
        const std::string& set_id,
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
        const std::string& generated_at_utc) const;
};

}  // namespace scratchrobin::project

