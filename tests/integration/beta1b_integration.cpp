#include "core/beta1b_contracts.h"
#include "tests/test_harness.h"

#include <filesystem>
#include <fstream>

using namespace scratchrobin;
using namespace scratchrobin::beta1b;

namespace {

void WriteFile(const std::filesystem::path& p, const std::string& text) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << text;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/specset_discovery_and_load", [] {
                        const auto temp_root = std::filesystem::temp_directory_path() / "scratchrobin_beta1b_it";
                        std::filesystem::remove_all(temp_root);

                        // Required discovery manifests
                        WriteFile(temp_root / "resources/specset_packages/sb_v3_specset_manifest.example.json",
                                  R"({"set_id":"sb_v3","package_root":"sb_v3_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"v3","package_hash_sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"})");
                        WriteFile(temp_root / "resources/specset_packages/sb_vnext_specset_manifest.example.json",
                                  R"({"set_id":"sb_vnext","package_root":"sb_vnext_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"vnext","package_hash_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"})");
                        WriteFile(temp_root / "resources/specset_packages/sb_beta1_specset_manifest.example.json",
                                  R"({"set_id":"sb_beta1","package_root":"sb_beta1_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"beta1","package_hash_sha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"})");

                        // Make one payload resolvable for package load
                        WriteFile(temp_root / "resources/specset_packages/sb_vnext_payload/AUTHORITATIVE_SPEC_INVENTORY.md",
                                  "- `README.md`\n- `contracts/ONE.md`\n");
                        WriteFile(temp_root / "resources/specset_packages/sb_vnext_payload/README.md", "hello");
                        WriteFile(temp_root / "resources/specset_packages/sb_vnext_payload/contracts/ONE.md", "contract\n");

                        const auto manifests = DiscoverSpecsets(temp_root.string());
                        scratchrobin::tests::AssertTrue(manifests.size() == 3U, "expected three manifests");

                        const auto rows = LoadSpecsetPackage((temp_root / "resources/specset_packages/sb_vnext_specset_manifest.example.json").string());
                        scratchrobin::tests::AssertTrue(rows.size() == 2U, "expected two normative files");

                        std::filesystem::remove_all(temp_root);
                    }});

    tests.push_back({"integration/support_completeness", [] {
                        std::vector<SpecFileRow> files = {
                            {"sb_vnext", "A.md", true, "", 1},
                            {"sb_vnext", "B.md", true, "", 1},
                        };

                        std::vector<std::tuple<std::string, std::string, std::string>> links = {
                            {"sb_vnext:A.md", "design", "covered"},
                            {"sb_vnext:B.md", "design", "covered"},
                        };

                        AssertSupportComplete(files, links, "design");
                    }});

    return scratchrobin::tests::RunTests(tests);
}
