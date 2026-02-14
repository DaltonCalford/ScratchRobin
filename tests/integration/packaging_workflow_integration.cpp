#include "packaging/packaging_services.h"
#include "tests/test_harness.h"

#include <filesystem>
#include <fstream>

using namespace scratchrobin;
using namespace scratchrobin::packaging;

namespace {

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        scratchrobin::tests::AssertEq(ex.payload().code, code, "unexpected reject");
        return;
    }
    throw std::runtime_error("expected reject not thrown");
}

void WriteTextFile(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

std::string BuildManifestJson(const std::string& profile_id,
                              const std::string& build_hash,
                              const std::string& enabled_backends_json,
                              const std::string& preview_only_json) {
    return std::string("{")
        + "\"manifest_version\":\"1.0.0\","
        + "\"profile_id\":\"" + profile_id + "\","
        + "\"build_version\":\"1.0.0\","
        + "\"build_hash\":\"" + build_hash + "\","
        + "\"build_timestamp_utc\":\"2026-02-14T00:00:00Z\","
        + "\"platform\":\"linux\","
        + "\"enabled_backends\":" + enabled_backends_json + ","
        + "\"surfaces\":{\"enabled\":[\"MainFrame\"],\"disabled\":[\"SqlEditorFrame\"],\"preview_only\":" + preview_only_json + "},"
        + "\"security_defaults\":{\"security_mode\":\"standard\",\"credential_store_policy\":\"preferred\",\"audit_enabled_default\":true,\"tls_required_default\":false},"
        + "\"artifacts\":{\"license_path\":\"docs/LICENSE.txt\",\"attribution_path\":\"docs/ATTRIBUTION.txt\",\"help_root_path\":\"share/help\",\"config_template_path\":\"config/scratchrobin.toml.example\",\"connections_template_path\":\"config/connections.toml.example\"}"
        + "}";
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/packaging_manifest_and_hash", [] {
                        PackagingService svc;
                        const auto build_hash =
                            svc.CanonicalBuildHash("0123456789abcdef0123456789abcdef01234567");
                        scratchrobin::tests::AssertTrue(build_hash.size() == 64U, "canonical build hash length mismatch");

                        const std::set<std::string> surfaces = {"MainFrame", "SqlEditorFrame", "ClusterManagerFrame"};
                        const std::set<std::string> backends = {"embedded", "firebird", "network"};
                        const auto manifest = BuildManifestJson(
                            "full",
                            build_hash,
                            "[\"embedded\",\"firebird\"]",
                            "[]");

                        auto validation = svc.ValidateManifestJson(manifest, surfaces, backends);
                        scratchrobin::tests::AssertTrue(validation.ok, "manifest validation should pass");
                        scratchrobin::tests::AssertEq(validation.profile_id, "full", "manifest profile mismatch");

                        ExpectReject("SRB1-R-9002", [&] {
                            (void)svc.CanonicalBuildHash("deadbeef");
                        });

                        const auto manifest_bad_backend = BuildManifestJson(
                            "full",
                            build_hash,
                            "[\"embedded\",\"unknown_backend\"]",
                            "[]");
                        ExpectReject("SRB1-R-9002", [&] {
                            (void)svc.ValidateManifestJson(manifest_bad_backend, surfaces, backends);
                        });
                    }});

    tests.push_back({"integration/packaging_registry_and_artifacts", [] {
                        PackagingService svc;
                        const std::set<std::string> surfaces = {"MainFrame", "SqlEditorFrame", "ClusterManagerFrame"};
                        const std::set<std::string> backends = {"embedded", "firebird", "network"};
                        const auto build_hash =
                            svc.CanonicalBuildHash("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

                        svc.ValidatePackageArtifacts({"LICENSE",
                                                      "README.md",
                                                      "docs/installation_guide/README.md",
                                                      "docs/developers_guide/README.md"});
                        ExpectReject("SRB1-R-9003", [&] {
                            svc.ValidatePackageArtifacts({"LICENSE", "README.md"});
                        });

                        const std::string duplicate_surface_manifest =
                            std::string("{")
                            + "\"manifest_version\":\"1.0.0\","
                            + "\"profile_id\":\"full\","
                            + "\"build_version\":\"1\","
                            + "\"build_hash\":\"" + build_hash + "\","
                            + "\"build_timestamp_utc\":\"2026-02-14T00:00:00Z\","
                            + "\"platform\":\"linux\","
                            + "\"enabled_backends\":[\"embedded\"],"
                            + "\"surfaces\":{\"enabled\":[\"MainFrame\"],\"disabled\":[\"MainFrame\"],\"preview_only\":[]},"
                            + "\"security_defaults\":{\"security_mode\":\"standard\",\"credential_store_policy\":\"preferred\",\"audit_enabled_default\":true,\"tls_required_default\":false},"
                            + "\"artifacts\":{\"license_path\":\"docs/LICENSE.txt\",\"attribution_path\":\"docs/ATTRIBUTION.txt\",\"help_root_path\":\"share/help\",\"config_template_path\":\"config/scratchrobin.toml.example\",\"connections_template_path\":\"config/connections.toml.example\"}"
                            + "}";
                        ExpectReject("SRB1-R-9002", [&] {
                            (void)svc.ValidateManifestJson(duplicate_surface_manifest, surfaces, backends);
                        });

                        const auto ga_preview_manifest = BuildManifestJson(
                            "ga",
                            build_hash,
                            "[\"embedded\"]",
                            "[\"ClusterManagerFrame\"]");
                        ExpectReject("SRB1-R-9001", [&] {
                            (void)svc.ValidateManifestJson(ga_preview_manifest, surfaces, backends);
                        });
                    }});

    tests.push_back({"integration/packaging_specset_support", [] {
                        namespace fs = std::filesystem;
                        PackagingService svc;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_packaging_specset";
                        fs::remove_all(temp);

                        WriteTextFile(temp / "resources/specset_packages/sb_v3_specset_manifest.example.json",
                                      R"({"set_id":"sb_v3","package_root":"sb_v3_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"v3","package_hash_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"})");
                        WriteTextFile(temp / "resources/specset_packages/sb_vnext_specset_manifest.example.json",
                                      R"({"set_id":"sb_vnext","package_root":"sb_vnext_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"vnext","package_hash_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"})");
                        WriteTextFile(temp / "resources/specset_packages/sb_beta1_specset_manifest.example.json",
                                      R"({"set_id":"sb_beta1","package_root":"sb_beta1_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"beta1","package_hash_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"})");

                        WriteTextFile(temp / "resources/specset_packages/sb_vnext_payload/AUTHORITATIVE_SPEC_INVENTORY.md",
                                      "- `README.md`\n- `contracts/ONE.md`\n");
                        WriteTextFile(temp / "resources/specset_packages/sb_vnext_payload/README.md", "hello");
                        WriteTextFile(temp / "resources/specset_packages/sb_vnext_payload/contracts/ONE.md", "contract\n");

                        auto manifests = svc.DiscoverSpecsets(temp.string());
                        scratchrobin::tests::AssertTrue(manifests.size() == 3U, "specset manifest discovery mismatch");

                        auto manifest = svc.LoadSpecsetManifest(
                            (temp / "resources/specset_packages/sb_vnext_specset_manifest.example.json").string());
                        scratchrobin::tests::AssertEq(manifest.set_id, "sb_vnext", "specset manifest set id mismatch");

                        auto inventory = svc.ParseAuthoritativeInventory(
                            (temp / "resources/specset_packages/sb_vnext_payload/AUTHORITATIVE_SPEC_INVENTORY.md").string());
                        scratchrobin::tests::AssertTrue(inventory.size() == 2U, "inventory parse size mismatch");

                        auto files = svc.LoadSpecsetPackage(
                            (temp / "resources/specset_packages/sb_vnext_specset_manifest.example.json").string());
                        scratchrobin::tests::AssertTrue(files.size() == 2U, "specset package load size mismatch");

                        svc.AssertCoverageComplete(files,
                                                   {{"sb_vnext:README.md", "design", "covered"},
                                                    {"sb_vnext:contracts/ONE.md", "design", "covered"}},
                                                   "design");
                        ExpectReject("SRB1-R-5403", [&] {
                            svc.AssertCoverageComplete(files,
                                                       {{"sb_vnext:README.md", "design", "covered"}},
                                                       "design");
                        });

                        svc.ValidateBindings({"A0-LNT-001"}, {"A0-LNT-001", "PKG-003"});
                        ExpectReject("SRB1-R-5404", [&] {
                            svc.ValidateBindings({"unknown_case"}, {"A0-LNT-001"});
                        });

                        auto coverage = svc.AggregateCoverage({{"sb:README.md", "design", "covered"},
                                                               {"sb:README.md", "management", "missing"}});
                        scratchrobin::tests::AssertTrue(coverage["design:covered"] == 1, "coverage aggregate mismatch");

                        auto wp = svc.ExportWorkPackage("sb_vnext",
                                                        {{"sb_vnext:README.md", "design", {"A0-LNT-001"}}},
                                                        "2026-02-14T00:00:00Z");
                        scratchrobin::tests::AssertTrue(wp.find("\"set_id\":\"sb_vnext\"") != std::string::npos,
                                                        "work package export mismatch");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/packaging_resource_contract_files", [] {
                        namespace fs = std::filesystem;
                        PackagingService svc;

                        fs::path repo_root = fs::current_path();
                        if (repo_root.filename() == "build") {
                            repo_root = repo_root.parent_path();
                        }
                        if (!fs::exists(repo_root / "resources/schemas/package_profile_manifest.schema.json")) {
                            repo_root = fs::path("/home/dcalford/CliWork/ScratchRobin");
                        }

                        const fs::path schema_path = repo_root / "resources/schemas/package_profile_manifest.schema.json";
                        const fs::path registry_path = repo_root / "resources/schemas/package_surface_id_registry.json";
                        const fs::path template_path = repo_root / "resources/templates/package_profile_manifest.example.json";

                        scratchrobin::tests::AssertTrue(fs::exists(schema_path), "manifest schema file missing");
                        scratchrobin::tests::AssertTrue(fs::exists(registry_path), "surface registry file missing");
                        scratchrobin::tests::AssertTrue(fs::exists(template_path), "manifest template file missing");

                        auto surface_registry = svc.LoadSurfaceRegistry(registry_path.string());
                        auto backend_enum = svc.LoadBackendEnumFromSchema(schema_path.string());
                        scratchrobin::tests::AssertTrue(surface_registry.count("MainFrame") == 1U, "registry missing MainFrame");
                        scratchrobin::tests::AssertTrue(backend_enum.count("embedded") == 1U, "schema missing embedded backend");

                        auto summary =
                            svc.ValidateManifestFile(template_path.string(), registry_path.string(), schema_path.string());
                        scratchrobin::tests::AssertTrue(summary.ok, "template manifest validation should pass");

                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_manifest_file_validation";
                        fs::remove_all(temp);
                        fs::create_directories(temp);
                        auto text = svc.LoadTextFile(template_path.string());
                        const std::string needle = "\"profile_id\": \"full\"";
                        const std::size_t pos = text.find(needle);
                        scratchrobin::tests::AssertTrue(pos != std::string::npos, "template profile_id not found");
                        text.replace(pos, needle.size(), "\"profile_id\": \"ga\"");
                        WriteTextFile(temp / "manifest_ga_invalid.json", text);
                        ExpectReject("SRB1-R-9001", [&] {
                            (void)svc.ValidateManifestFile((temp / "manifest_ga_invalid.json").string(),
                                                           registry_path.string(),
                                                           schema_path.string());
                        });

                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}
