#pragma once

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "core/beta1b_contracts.h"

namespace scratchrobin::packaging {

struct ManifestValidationSummary {
    bool ok = false;
    std::string profile_id;
};

class PackagingService {
public:
    std::string CanonicalBuildHash(const std::string& full_commit_id) const;

    std::set<std::string> LoadSurfaceRegistry(const std::string& registry_json_path) const;
    std::set<std::string> LoadBackendEnumFromSchema(const std::string& schema_json_path) const;
    std::string LoadTextFile(const std::string& path) const;

    ManifestValidationSummary ValidateManifestJson(const std::string& manifest_json,
                                                   const std::set<std::string>& surface_registry,
                                                   const std::set<std::string>& backend_enum) const;
    ManifestValidationSummary ValidateManifestFile(const std::string& manifest_path,
                                                   const std::string& registry_json_path,
                                                   const std::string& schema_json_path) const;
    std::set<std::string> CollectManifestArtifactPaths(const std::string& manifest_json) const;
    void ValidateManifestArtifactPathsExist(const std::string& manifest_json,
                                            const std::string& package_root) const;
    void ValidateSurfaceRegistryJson(const std::string& manifest_json,
                                     const std::set<std::string>& surface_registry) const;
    void ValidatePackageArtifacts(const std::set<std::string>& packaged_paths) const;

    std::vector<std::string> DiscoverSpecsets(const std::string& spec_root) const;
    beta1b::SpecSetManifest LoadSpecsetManifest(const std::string& manifest_path) const;
    std::vector<std::string> ParseAuthoritativeInventory(const std::string& inventory_path) const;
    std::vector<beta1b::SpecFileRow> LoadSpecsetPackage(const std::string& manifest_path) const;

    void AssertCoverageComplete(
        const std::vector<beta1b::SpecFileRow>& spec_files,
        const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
        const std::string& coverage_class) const;
    void ValidateBindings(const std::vector<std::string>& binding_case_ids,
                          const std::set<std::string>& conformance_case_ids) const;
    std::map<std::string, int> AggregateCoverage(
        const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const;
    std::string ExportWorkPackage(
        const std::string& set_id,
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
        const std::string& generated_at_utc) const;
};

}  // namespace scratchrobin::packaging
