#include "packaging/packaging_services.h"

#include <fstream>
#include <filesystem>
#include <sstream>

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

const JsonValue& RequireObjectMember(const JsonValue& object,
                                     const std::string& key,
                                     const std::string& method) {
    if (object.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid json object", "packaging", method);
    }
    const JsonValue* value = FindMember(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "missing/invalid object member", "packaging", method, false, key);
    }
    return *value;
}

void ValidateManifestSchemaContract(const JsonValue& schema) {
    if (schema.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "manifest schema must be object", "packaging", "validate_manifest_schema");
    }
    const JsonValue* id = FindMember(schema, "$id");
    std::string schema_id;
    if (id == nullptr || !GetStringValue(*id, &schema_id) ||
        schema_id != "scratchrobin.package_profile_manifest.schema.json") {
        throw MakeReject("SRB1-R-9002", "unexpected manifest schema id", "packaging", "validate_manifest_schema");
    }
    const JsonValue* props = FindMember(schema, "properties");
    if (props == nullptr || props->type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "manifest schema missing properties", "packaging", "validate_manifest_schema");
    }
    for (const auto& required_key : {"profile_id", "enabled_backends", "surfaces", "security_defaults", "artifacts"}) {
        if (FindMember(*props, required_key) == nullptr) {
            throw MakeReject("SRB1-R-9002", "manifest schema missing required property", "packaging",
                             "validate_manifest_schema", false, required_key);
        }
    }
}

std::vector<std::string> RequireStringArrayMember(const JsonValue& object,
                                                  const std::string& key,
                                                  const std::string& method) {
    if (object.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid json object", "packaging", method);
    }
    const JsonValue* value = FindMember(object, key);
    if (value == nullptr || value->type != JsonValue::Type::Array) {
        throw MakeReject("SRB1-R-9002", "missing/invalid array member", "packaging", method, false, key);
    }
    std::vector<std::string> out;
    out.reserve(value->array_value.size());
    for (const auto& item : value->array_value) {
        std::string text;
        if (!GetStringValue(item, &text)) {
            throw MakeReject("SRB1-R-9002", "non-string array element", "packaging", method, false, key);
        }
        out.push_back(text);
    }
    return out;
}

std::string RequireStringMember(const JsonValue& object,
                                const std::string& key,
                                const std::string& method) {
    if (object.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid json object", "packaging", method);
    }
    const JsonValue* value = FindMember(object, key);
    std::string out;
    if (value == nullptr || !GetStringValue(*value, &out) || out.empty()) {
        throw MakeReject("SRB1-R-9002", "missing/invalid string member", "packaging", method, false, key);
    }
    return out;
}

}  // namespace

std::string PackagingService::CanonicalBuildHash(const std::string& full_commit_id) const {
    return beta1b::CanonicalBuildHash(full_commit_id);
}

std::set<std::string> PackagingService::LoadSurfaceRegistry(const std::string& registry_json_path) const {
    const auto json = ParseJsonManifest(LoadTextFile(registry_json_path));
    const auto values = RequireStringArrayMember(json, "surface_ids", "load_surface_registry");
    std::set<std::string> out(values.begin(), values.end());
    if (out.empty()) {
        throw MakeReject("SRB1-R-9002", "surface registry cannot be empty", "packaging", "load_surface_registry");
    }
    return out;
}

std::set<std::string> PackagingService::LoadBackendEnumFromSchema(const std::string& schema_json_path) const {
    const auto json = ParseJsonManifest(LoadTextFile(schema_json_path));
    const JsonValue& properties = RequireObjectMember(json, "properties", "load_backend_enum");
    const JsonValue& enabled_backends = RequireObjectMember(properties, "enabled_backends", "load_backend_enum");
    const JsonValue& items = RequireObjectMember(enabled_backends, "items", "load_backend_enum");
    const auto enum_values = RequireStringArrayMember(items, "enum", "load_backend_enum");
    std::set<std::string> out(enum_values.begin(), enum_values.end());
    if (out.empty()) {
        throw MakeReject("SRB1-R-9002", "backend enum cannot be empty", "packaging", "load_backend_enum");
    }
    return out;
}

std::string PackagingService::LoadTextFile(const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw MakeReject("SRB1-R-9002", "file read failure", "packaging", "load_text_file", false, path);
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
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

ManifestValidationSummary PackagingService::ValidateManifestFile(const std::string& manifest_path,
                                                                 const std::string& registry_json_path,
                                                                 const std::string& schema_json_path) const {
    const auto manifest_json = LoadTextFile(manifest_path);
    const auto surface_registry = LoadSurfaceRegistry(registry_json_path);
    ValidateManifestSchemaContract(ParseJsonManifest(LoadTextFile(schema_json_path)));
    const auto backend_enum = LoadBackendEnumFromSchema(schema_json_path);
    return ValidateManifestJson(manifest_json, surface_registry, backend_enum);
}

std::set<std::string> PackagingService::CollectManifestArtifactPaths(const std::string& manifest_json) const {
    const auto manifest = ParseJsonManifest(manifest_json);
    const JsonValue& artifacts = RequireObjectMember(manifest, "artifacts", "collect_manifest_artifact_paths");

    std::set<std::string> paths;
    paths.insert(RequireStringMember(artifacts, "license_path", "collect_manifest_artifact_paths"));
    paths.insert(RequireStringMember(artifacts, "attribution_path", "collect_manifest_artifact_paths"));
    paths.insert(RequireStringMember(artifacts, "help_root_path", "collect_manifest_artifact_paths"));
    paths.insert(RequireStringMember(artifacts, "config_template_path", "collect_manifest_artifact_paths"));
    paths.insert(RequireStringMember(artifacts, "connections_template_path", "collect_manifest_artifact_paths"));

    // Mandatory package docs required by PKG-002 remain explicit contracts.
    paths.insert("LICENSE");
    paths.insert("README.md");
    paths.insert("docs/installation_guide/README.md");
    paths.insert("docs/developers_guide/README.md");
    return paths;
}

void PackagingService::ValidateManifestArtifactPathsExist(const std::string& manifest_json,
                                                          const std::string& package_root) const {
    if (package_root.empty()) {
        throw MakeReject("SRB1-R-9003", "package root required", "packaging", "validate_manifest_artifact_paths");
    }
    const auto artifact_paths = CollectManifestArtifactPaths(manifest_json);
    for (const auto& rel_path : artifact_paths) {
        const std::filesystem::path abs = std::filesystem::path(package_root) / rel_path;
        if (!std::filesystem::exists(abs)) {
            throw MakeReject("SRB1-R-9003", "missing mandatory license/documentation artifacts", "packaging",
                             "validate_manifest_artifact_paths", false, rel_path);
        }
    }
    ValidatePackageArtifacts(artifact_paths);
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
