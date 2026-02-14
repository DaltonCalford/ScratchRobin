#include "packaging/packaging_services.h"

#include "core/reject.h"

namespace scratchrobin::packaging {

namespace {

JsonValue ParseJsonManifest(const std::string& manifest_json) {
    JsonParser parser(manifest_json);
    JsonValue root;
    std::string error;
    if (!parser.Parse(&root, &error)) {
        throw MakeReject("SRB1-R-9002", "manifest parse failure", "packaging", "parse_manifest_json", false, error);
    }
    return root;
}

}  // namespace

std::string PackagingService::CanonicalBuildHash(const std::string& full_commit_id) const {
    return beta1b::CanonicalBuildHash(full_commit_id);
}

ManifestValidationSummary PackagingService::ValidateManifestJson(const std::string& manifest_json,
                                                                 const std::set<std::string>& surface_registry,
                                                                 const std::set<std::string>& backend_enum) const {
    const JsonValue manifest = ParseJsonManifest(manifest_json);
    const auto result = beta1b::ValidateProfileManifest(manifest, surface_registry, backend_enum);
    ManifestValidationSummary summary;
    summary.ok = result.ok;
    summary.profile_id = result.profile_id;
    return summary;
}

void PackagingService::ValidateSurfaceRegistryJson(const std::string& manifest_json,
                                                   const std::set<std::string>& surface_registry) const {
    const JsonValue manifest = ParseJsonManifest(manifest_json);
    beta1b::ValidateSurfaceRegistry(manifest, surface_registry);
}

void PackagingService::ValidatePackageArtifacts(const std::set<std::string>& packaged_paths) const {
    beta1b::ValidatePackageArtifacts(packaged_paths);
}

std::vector<std::string> PackagingService::DiscoverSpecsets(const std::string& spec_root) const {
    return beta1b::DiscoverSpecsets(spec_root);
}

beta1b::SpecSetManifest PackagingService::LoadSpecsetManifest(const std::string& manifest_path) const {
    return beta1b::LoadSpecsetManifest(manifest_path);
}

std::vector<std::string> PackagingService::ParseAuthoritativeInventory(const std::string& inventory_path) const {
    return beta1b::ParseAuthoritativeInventory(inventory_path);
}

std::vector<beta1b::SpecFileRow> PackagingService::LoadSpecsetPackage(const std::string& manifest_path) const {
    return beta1b::LoadSpecsetPackage(manifest_path);
}

void PackagingService::AssertCoverageComplete(
    const std::vector<beta1b::SpecFileRow>& spec_files,
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
    const std::string& coverage_class) const {
    beta1b::AssertSupportComplete(spec_files, coverage_links, coverage_class);
}

void PackagingService::ValidateBindings(const std::vector<std::string>& binding_case_ids,
                                        const std::set<std::string>& conformance_case_ids) const {
    beta1b::ValidateBindings(binding_case_ids, conformance_case_ids);
}

std::map<std::string, int> PackagingService::AggregateCoverage(
    const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) const {
    return beta1b::AggregateSupport(coverage_links);
}

std::string PackagingService::ExportWorkPackage(
    const std::string& set_id,
    const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
    const std::string& generated_at_utc) const {
    return beta1b::ExportWorkPackage(set_id, gaps, generated_at_utc);
}

}  // namespace scratchrobin::packaging
