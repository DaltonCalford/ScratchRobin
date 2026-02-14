#include "project/project_services.h"
#include "tests/test_harness.h"

#include <filesystem>
#include <fstream>
#include <map>

using namespace scratchrobin;
using namespace scratchrobin::project;

namespace {

JsonValue ParseJson(const std::string& text) {
    JsonParser parser(text);
    JsonValue value;
    std::string err;
    if (!parser.Parse(&value, &err)) {
        throw std::runtime_error("json parse error: " + err);
    }
    return value;
}

void WriteText(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        scratchrobin::tests::AssertEq(ex.payload().code, code, "reject code mismatch");
        return;
    }
    throw std::runtime_error("expected reject");
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/project_binary_roundtrip", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_project_rt";
                        fs::remove_all(temp);

                        ProjectBinaryService service;
                        const auto result = service.RoundTripFile((temp / "project.srpj").string(),
                                                                  {1, 2, 3, 4},
                                                                  {10, 11},
                                                                  {{"RPTG", {77, 88, 99}}});

                        scratchrobin::tests::AssertTrue(result.bytes_written > 0U, "bytes_written should be > 0");
                        scratchrobin::tests::AssertTrue(result.loaded_chunks.count("PROJ") == 1U, "missing PROJ");
                        scratchrobin::tests::AssertTrue(result.loaded_chunks.count("OBJS") == 1U, "missing OBJS");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/schema_validation_gate", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_schema_gate";
                        fs::remove_all(temp);
                        WriteText(temp / "project_domain.schema.json", "{}");
                        WriteText(temp / "scratchbird_specset.schema.json", "{}");

                        const auto project_payload = ParseJson(
                            R"({"project":{"project_id":"123e4567-e89b-12d3-a456-426614174000","name":"x","created_at":"2026-02-14T00:00:00Z","updated_at":"2026-02-14T00:00:00Z","config":{},"objects":[],"objects_by_path":{},"reporting_assets":[],"reporting_schedules":[],"data_view_snapshots":[],"git_sync_state":null,"audit_log_path":"audit.log"}})");
                        const auto specset_payload = ParseJson(
                            R"({"spec_sets":[],"spec_files":[],"coverage_links":[],"conformance_bindings":[]})");

                        ValidateProjectPayloadWithSchema((temp / "project_domain.schema.json").string(), project_payload);
                        ValidateSpecsetPayloadWithSchema((temp / "scratchbird_specset.schema.json").string(), specset_payload);

                        ExpectReject("SRB1-R-3002", [&] {
                            ValidateProjectPayloadWithSchema((temp / "missing.schema.json").string(), project_payload);
                        });

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/governance_and_audit", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_governance";
                        fs::remove_all(temp);
                        fs::create_directories(temp);

                        GovernanceInput denied_input;
                        denied_input.action = "report.save";
                        denied_input.actor = "alice";
                        denied_input.actor_role = "viewer";
                        denied_input.environment_id = "dev";
                        denied_input.target_id = "rpt-1";
                        denied_input.connection_ref = "local";
                        denied_input.approval_count = 0;
                        denied_input.requires_guaranteed_audit = true;

                        GovernancePolicy policy;
                        policy.allowed_roles = {"owner", "steward"};
                        policy.min_approval_count = 1;

                        bool executed = false;
                        ExpectReject("SRB1-R-3202", [&] {
                            ExecuteGovernedOperation(denied_input,
                                                     policy,
                                                     (temp / "audit.log").string(),
                                                     [&] { executed = true; });
                        });
                        scratchrobin::tests::AssertTrue(!executed, "denied operation should not execute");

                        std::ifstream in(temp / "audit.log", std::ios::binary);
                        std::string line;
                        std::getline(in, line);
                        scratchrobin::tests::AssertTrue(line.find("\"success\":false") != std::string::npos,
                                                        "denied audit event missing");

                        GovernanceInput allow_input = denied_input;
                        allow_input.actor_role = "owner";
                        allow_input.approval_count = 1;
                        ExecuteGovernedOperation(allow_input,
                                                 policy,
                                                 (temp / "audit.log").string(),
                                                 [&] { executed = true; });
                        scratchrobin::tests::AssertTrue(executed, "allowed operation should execute");

                        GovernanceInput failing_audit = allow_input;
                        failing_audit.requires_guaranteed_audit = true;
                        executed = false;
                        ExpectReject("SRB1-R-3201", [&] {
                            ExecuteGovernedOperation(failing_audit,
                                                     policy,
                                                     (temp / "missing_dir/audit.log").string(),
                                                     [&] { executed = true; });
                        });
                        scratchrobin::tests::AssertTrue(!executed, "operation must not execute when required audit fails");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/specset_index_and_coverage", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_specset_index";
                        fs::remove_all(temp);

                        WriteText(temp / "resources/specset_packages/sb_vnext_specset_manifest.example.json",
                                  R"({"set_id":"sb_vnext","package_root":"sb_vnext_payload","authoritative_inventory_relpath":"AUTHORITATIVE_SPEC_INVENTORY.md","version_stamp":"vnext","package_hash_sha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"})");
                        WriteText(temp / "resources/specset_packages/sb_vnext_payload/AUTHORITATIVE_SPEC_INVENTORY.md",
                                  "- `README.md`\n- `contracts/ONE.md`\n");
                        WriteText(temp / "resources/specset_packages/sb_vnext_payload/README.md", "doc\n");
                        WriteText(temp / "resources/specset_packages/sb_vnext_payload/contracts/ONE.md", "contract\n");

                        SpecSetService specset;
                        const auto index = specset.BuildIndex(
                            (temp / "resources/specset_packages/sb_vnext_specset_manifest.example.json").string(),
                            "2026-02-14T00:00:00Z");
                        scratchrobin::tests::AssertTrue(index.files.size() == 2U, "expected two indexed files");

                        specset.AssertCoverageComplete(index,
                                                      {{"sb_vnext:README.md", "design", "covered"},
                                                       {"sb_vnext:contracts/ONE.md", "design", "covered"}},
                                                      "design");
                        ExpectReject("SRB1-R-5403", [&] {
                            specset.AssertCoverageComplete(index,
                                                           {{"sb_vnext:README.md", "development", "covered"}},
                                                           "development");
                        });

                        specset.ValidateConformanceBindings({"A0-LNT-001"}, {"A0-LNT-001", "PKG-003"});
                        auto summary = specset.CoverageSummary({{"a", "design", "covered"}, {"a", "design", "missing"}});
                        scratchrobin::tests::AssertTrue(summary["design:covered"] == 1, "coverage summary mismatch");

                        auto work_pkg = specset.ExportImplementationWorkPackage(
                            "sb_vnext",
                            {{"sb_vnext:contracts/ONE.md", "development", {"SPC-COV-002"}}},
                            "2026-02-14T00:00:00Z");
                        scratchrobin::tests::AssertTrue(work_pkg.find("sb_vnext") != std::string::npos, "work package missing set id");

                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}

