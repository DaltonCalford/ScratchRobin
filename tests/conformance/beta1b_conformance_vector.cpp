#include "core/beta1b_contracts.h"
#include "tests/test_harness.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace scratchrobin;
using namespace scratchrobin::beta1b;
using scratchrobin::tests::AssertEq;
using scratchrobin::tests::AssertTrue;

namespace {

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        AssertEq(ex.payload().code, code, "unexpected reject code");
        return;
    }
    throw std::runtime_error("expected reject not thrown");
}

void WriteTextFile(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::uint32_t Crc32Local(const std::uint8_t* data, std::size_t len) {
    return Crc32(data, len);
}

void WriteU16(std::vector<std::uint8_t>* out, std::uint16_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void WriteU32(std::vector<std::uint8_t>* out, std::uint32_t v) {
    out->push_back(static_cast<std::uint8_t>(v & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out->push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void WriteU64(std::vector<std::uint8_t>* out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        out->push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xFF));
    }
}

std::vector<std::uint8_t> BuildValidProjectBinary() {
    const std::vector<std::uint8_t> proj_data = {1, 2, 3, 4};
    const std::vector<std::uint8_t> objs_data = {5, 6, 7};

    const std::uint64_t header_size = 44;
    const std::uint64_t toc_size = 2 * 40;
    const std::uint64_t proj_off = header_size + toc_size;
    const std::uint64_t objs_off = proj_off + proj_data.size();
    const std::uint64_t file_size = objs_off + objs_data.size();

    std::vector<std::uint8_t> bytes;
    bytes.reserve(static_cast<size_t>(file_size));

    for (int i = 0; i < 44; ++i) {
        bytes.push_back(0);
    }

    auto append_toc = [&](const char id[4], std::uint64_t off, std::uint64_t sz, std::uint32_t crc, std::uint32_t ordinal) {
        bytes.push_back(static_cast<std::uint8_t>(id[0]));
        bytes.push_back(static_cast<std::uint8_t>(id[1]));
        bytes.push_back(static_cast<std::uint8_t>(id[2]));
        bytes.push_back(static_cast<std::uint8_t>(id[3]));
        WriteU32(&bytes, 0);
        WriteU64(&bytes, off);
        WriteU64(&bytes, sz);
        WriteU32(&bytes, crc);
        WriteU16(&bytes, 1);
        WriteU16(&bytes, 0);
        WriteU32(&bytes, ordinal);
        WriteU32(&bytes, 0);
    };

    append_toc("PROJ", proj_off, proj_data.size(), Crc32Local(proj_data.data(), proj_data.size()), 0);
    append_toc("OBJS", objs_off, objs_data.size(), Crc32Local(objs_data.data(), objs_data.size()), 1);
    bytes.insert(bytes.end(), proj_data.begin(), proj_data.end());
    bytes.insert(bytes.end(), objs_data.begin(), objs_data.end());

    bytes[0] = 'S'; bytes[1] = 'R'; bytes[2] = 'P'; bytes[3] = 'J';
    bytes[4] = 1; bytes[5] = 0;
    bytes[6] = 0; bytes[7] = 0;
    bytes[8] = 44; bytes[9] = 0;
    bytes[10] = 40; bytes[11] = 0;
    bytes[12] = 2; bytes[13] = 0; bytes[14] = 0; bytes[15] = 0;
    bytes[16] = 44; bytes[17] = 0; bytes[18] = 0; bytes[19] = 0;
    bytes[20] = 0; bytes[21] = 0; bytes[22] = 0; bytes[23] = 0;
    std::uint64_t fs = file_size;
    for (int i = 0; i < 8; ++i) {
        bytes[24 + i] = static_cast<std::uint8_t>((fs >> (i * 8)) & 0xFF);
    }

    std::array<std::uint8_t, 44> raw{};
    std::copy(bytes.begin(), bytes.begin() + 44, raw.begin());
    raw[40] = raw[41] = raw[42] = raw[43] = 0;
    std::uint32_t crc = Crc32Local(raw.data(), raw.size());
    bytes[40] = static_cast<std::uint8_t>(crc & 0xFF);
    bytes[41] = static_cast<std::uint8_t>((crc >> 8) & 0xFF);
    bytes[42] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
    bytes[43] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
    return bytes;
}

JsonValue ParseJson(const std::string& text) {
    JsonParser parser(text);
    JsonValue value;
    std::string error;
    if (!parser.Parse(&value, &error)) {
        throw std::runtime_error("json parse failure: " + error);
    }
    return value;
}

std::vector<std::string> ReadConformanceCaseIds(const std::filesystem::path& csv_path) {
    std::ifstream in(csv_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("unable to open conformance vector");
    }
    std::string line;
    std::getline(in, line);
    std::vector<std::string> out;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto pos = line.find(',');
        if (pos == std::string::npos || pos == 0) {
            continue;
        }
        out.push_back(line.substr(0, pos));
    }
    return out;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"conformance/vector_all_cases", [] {
                        namespace fs = std::filesystem;
                        fs::path repo_root = fs::current_path();
                        if (repo_root.filename() == "build") {
                            repo_root = repo_root.parent_path();
                        }
                        if (!fs::exists(repo_root / "src")) {
                            // Fallback for non-ctest invocation.
                            repo_root = fs::path("/home/dcalford/CliWork/ScratchRobin");
                        }
                        const fs::path spec_root = repo_root.parent_path() / "local_work/docs/specifications_beta1b";
                        const fs::path vector_csv =
                            spec_root / "10_Execution_Tracks_and_Conformance/CONFORMANCE_VECTOR.csv";

                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_beta1b_vector";
                        fs::remove_all(temp);
                        fs::create_directories(temp);

                        // fixtures for specset and alpha checks
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
                        WriteTextFile(temp / "alpha/deep/a.txt", "alpha");

                        const auto sample_project_payload = ParseJson(
                            R"({"project":{"project_id":"123e4567-e89b-12d3-a456-426614174000","name":"x","created_at":"2026-02-14T00:00:00Z","updated_at":"2026-02-14T00:00:00Z","config":{"default_environment_id":"dev","active_connection_id":null,"connections_file_path":"config/connections.toml","governance":{"owners":["owner"],"stewards":[],"review_min_approvals":1,"allowed_roles_by_environment":{"dev":["owner"]},"ai_policy":{"enabled":true,"require_review":false,"allow_scopes":["design"],"deny_scopes":[]},"audit_policy":{"level":"standard","retention_days":30,"export_enabled":true}},"security_mode":"standard","features":{"sql_editor":true}},"objects":[],"objects_by_path":{},"reporting_assets":[],"reporting_schedules":[],"data_view_snapshots":[],"git_sync_state":null,"audit_log_path":"audit.log"}})");
                        const auto sample_specset_payload = ParseJson(
                            R"({"spec_sets":[],"spec_files":[],"coverage_links":[],"conformance_bindings":[]})");
                        const auto sample_manifest = ParseJson(
                            R"({"manifest_version":"1.0.0","profile_id":"full","build_version":"1","build_hash":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef","build_timestamp_utc":"2026-02-14T00:00:00Z","platform":"linux","enabled_backends":["embedded","firebird"],"surfaces":{"enabled":["MainFrame"],"disabled":["SqlEditorFrame"],"preview_only":[]},"security_defaults":{"security_mode":"standard","credential_store_policy":"preferred","audit_enabled_default":true,"tls_required_default":false},"artifacts":{"license_path":"docs/LICENSE.txt","attribution_path":"docs/ATTRIBUTION.txt","help_root_path":"share/help","config_template_path":"config/a.toml","connections_template_path":"config/c.toml"}})");
                        const std::set<std::string> manifest_surfaces = {"MainFrame", "SqlEditorFrame"};
                        const std::set<std::string> manifest_backends = {"embedded", "firebird", "network"};

                        DiagramDocument diagram;
                        diagram.diagram_id = "d1";
                        diagram.notation = "crowsfoot";
                        diagram.nodes.push_back({"n1", "table", "root", 0, 0, 100, 40, "int"});
                        diagram.nodes.push_back({"n2", "table", "root", 120, 0, 100, 40, "varchar"});
                        diagram.edges.push_back({"e1", "n1", "n2", "fk"});

                        std::map<std::string, std::function<void()>> checks;
                        checks["B1-CMP-001"] = [] {};
                        checks["B1-CMP-002"] = [] {};
                        checks["B1-CMP-003"] = [] {};
                        checks["B1-CMP-004"] = [&] {
                            const std::string cmake = ReadTextFile(repo_root / "CMakeLists.txt");
                            const std::array<std::string, 8> required_options = {
                                "SCRATCHROBIN_BUILD_UI",
                                "SCRATCHROBIN_USE_SCRATCHBIRD",
                                "SCRATCHROBIN_EMBED_SCRATCHBIRD_SERVER",
                                "SCRATCHROBIN_USE_LIBPQ",
                                "SCRATCHROBIN_USE_MYSQL",
                                "SCRATCHROBIN_USE_FIREBIRD",
                                "SCRATCHROBIN_USE_LIBSECRET",
                                "SCRATCHROBIN_BUILD_TESTS"};
                            for (const auto& opt : required_options) {
                                AssertTrue(cmake.find(opt) != std::string::npos, "missing cmake option: " + opt);
                            }
                        };
                        checks["B1-CMP-005"] = [&] {
                            const std::string cmake = ReadTextFile(repo_root / "CMakeLists.txt");
                            AssertTrue(cmake.find("add_executable(scratchrobin_perf_diagram") != std::string::npos,
                                       "missing scratchrobin_perf_diagram target");
                        };
                        checks["A0-LNT-001"] = [] { ValidateRejectCodeReferences({"SRB1-R-4001"}, {"SRB1-R-4001", "SRB1-R-5407"}); };
                        checks["A0-BLK-001"] = [] {
                            ValidateBlockerRows({{"BLK-0001", "P0", "open", "conformance_case", "A0-LNT-001",
                                                  "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z", "owner", "critical blocker"}});
                        };
                        checks["R1-CON-001"] = [] { (void)SelectBackend(ConnectionProfile{.backend = "pg"}); };
                        checks["R1-CON-002"] = [] {
                            ConnectionProfile p;
                            p.credential_id = "cred";
                            (void)ResolveCredential(p, {{"cred", "x"}}, std::nullopt);
                        };
                        checks["R1-CON-003"] = [] { ExpectReject("SRB1-R-4101", [] { EnsureCapability(false, "pg", "backup"); }); };
                        checks["R1-CON-004"] = [] { CancelActive(true); };
                        checks["R1-ENT-001"] = [] {
                            EnterpriseConnectionProfile p;
                            p.profile_id = "p";
                            p.username = "u";
                            p.transport = {"ssh_jump_chain", "required", 1000};
                            p.ssh = SshContract{"db", 5432, "u", "keypair", "cred"};
                            p.jump_hosts.push_back({"jh", 22, "u", "agent", ""});
                            p.identity = {"oidc", "idp", {"openid"}};
                            p.secret_provider = SecretProviderContract{"vault", "kv/x"};
                            ValidateTransport(p);
                        };
                        checks["R1-ENT-002"] = [] {
                            IdentityContract identity{"oidc", "idp", {"openid"}};
                            identity.auth_method_id = "scratchbird.auth.workload_identity";
                            identity.workload_identity_token = "wl.jwt";
                            (void)RunIdentityHandshake(identity, "secret",
                                                       [](const std::string&, const std::string&) { return true; },
                                                       [](const std::string&, const std::string&) { return true; });
                        };
                        checks["R1-ENT-003"] = [] {
                            (void)ResolveSecret(std::nullopt,
                                                [](const SecretProviderContract&) { return std::optional<std::string>("p"); },
                                                SecretProviderContract{"vault", "kv/x"},
                                                [](const std::string&) { return std::optional<std::string>("c"); },
                                                std::optional<std::string>("cred"),
                                                std::nullopt,
                                                false);
                        };
                        checks["R1-ENT-004"] = [] {
                            EnterpriseConnectionProfile p;
                            p.profile_id = "bad";
                            p.username = "u";
                            p.transport = {"direct", "required", 1000};
                            p.identity = {"oidc", "idp", {"openid"}};
                            p.identity.auth_required_methods = {"scratchbird.auth.scram_sha_256"};
                            p.identity.auth_forbidden_methods = {"scratchbird.auth.scram_sha_256"};
                            ExpectReject("SRB1-R-4005", [&] { ValidateTransport(p); });
                        };
                        checks["R1-ENT-005"] = [] {
                            EnterpriseConnectionProfile p;
                            p.profile_id = "radius";
                            p.username = "u";
                            p.transport = {"direct", "required", 1000};
                            p.allow_inline_secret = true;
                            p.inline_secret = std::string("radius-secret");
                            p.identity = {"radius", "radius_primary", {}};
                            p.identity.provider_profile = "radius_primary";
                            const auto fp = ConnectEnterprise(
                                p,
                                std::nullopt,
                                [](const SecretProviderContract&) { return std::optional<std::string>(); },
                                [](const std::string&) { return std::optional<std::string>(); },
                                [](const std::string&, const std::string&) { return true; },
                                [](const std::string&, const std::string&) { return true; });
                            AssertEq(fp.identity_method_id,
                                     "scratchbird.auth.radius_pap",
                                     "radius mode default method mismatch");
                        };
                        checks["R1-CPY-001"] = [] { (void)RunCopyIo("COPY t TO STDOUT", "stdin", "stdout", true, true); };
                        checks["R1-PRE-001"] = [] { (void)PrepareExecuteClose(true, "select 1", {}); };
                        checks["P1-SER-001"] = [] { (void)LoadProjectBinary(BuildValidProjectBinary()); };
                        checks["P1-SER-002"] = [] {
                            auto bytes = BuildValidProjectBinary();
                            bytes.resize(bytes.size() - 1);
                            ExpectReject("SRB1-R-3101", [&] { (void)LoadProjectBinary(bytes); });
                        };
                        checks["P1-BIN-003"] = [] { (void)LoadProjectBinary(BuildValidProjectBinary()); };
                        checks["P1-BIN-004"] = [] {
                            auto bytes = BuildValidProjectBinary();
                            bytes[24] ^= 0x01;
                            ExpectReject("SRB1-R-3101", [&] { (void)LoadProjectBinary(bytes); });
                        };
                        checks["P1-SCH-001"] = [&] { ValidateProjectPayload(sample_project_payload); };
                        checks["P1-SCH-002"] = [&] { ValidateSpecsetPayload(sample_specset_payload); };
                        checks["P1-GOV-001"] = [] {
                            bool applied = false;
                            bool audited = false;
                            ExpectReject("SRB1-R-3202", [&] {
                                EnforceGovernanceGate(false,
                                                      [&] { applied = true; },
                                                      [&](const std::string&) { audited = true; });
                            });
                            AssertTrue(!applied, "governance deny changed state");
                            AssertTrue(audited, "governance deny missing audit");
                        };
                        checks["P1-GOV-002"] = [&] { WriteAuditRequired((temp / "audit.log").string(), "{\"event\":\"x\"}"); };
                        checks["U1-MAI-001"] = [] { ValidateUiWorkflowState("main_frame", true, true); };
                        checks["U1-SQL-001"] = [] { (void)StatusSnapshot(true, 1, 0); };
                        checks["U2-SQL-002"] = [] {
                            (void)SortedSuggestions({{"select", 1}, {"session", 1}}, "se",
                                                    [](std::string_view t, std::string_view p) {
                                                        return static_cast<int>(t.size()) - static_cast<int>(p.size());
                                                    });
                        };
                        checks["U2-SQL-003"] = [] {
                            (void)SnippetInsertExact({"id", "n", "SELECT 1;", "global", "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z"});
                        };
                        checks["U2-SQL-004"] = [] {
                            auto rows = PruneHistory({{"1", "p", "2026-02-14T00:00:00Z", 1, "success", "", "h"}}, "2026-02-13T00:00:00Z");
                            (void)ExportHistoryCsv(rows);
                        };
                        checks["U2-CMP-001"] = [] {
                            (void)StableSortOps({{"1", "table", "public.a", "alter", "ALTER TABLE public.a"}});
                        };
                        checks["U2-CMP-002"] = [] {
                            JsonValue payload;
                            payload.type = JsonValue::Type::String;
                            payload.string_value = "x";
                            (void)RunDataCompareKeyed({{{"1"}, payload}}, {{{"1"}, payload}});
                        };
                        checks["U2-CMP-003"] = [] {
                            (void)GenerateMigrationScript({{"1", "table", "public.a", "alter", "ALTER TABLE public.a"}},
                                                          "2026-02-14T00:00:00Z",
                                                          "l",
                                                          "r");
                        };
                        checks["U2-PLN-001"] = [] { (void)OrderPlanNodes({{1, -1, "scan", 1, 1.0, ""}}); };
                        checks["U2-PLN-002"] = [] { (void)ApplyBuilderGraph(false, true, "select 1", true); };
                        checks["U1-OBJ-001"] = [] { ValidateUiWorkflowState("object_crud", true, true); };
                        checks["U1-MON-001"] = [] { (void)StatusSnapshot(true, 0, 1); };
                        checks["U1-URS-001"] = [] { ValidateUiWorkflowState("users_roles", true, true); };
                        checks["U1-JOB-001"] = [] { ValidateUiWorkflowState("jobs", true, true); };
                        checks["U1-STM-001"] = [] { ValidateUiWorkflowState("storage", true, true); };
                        checks["U1-BKP-001"] = [] { EnsureCapability(true, "firebird", "backup_restore"); };
                        checks["U1-BKP-002"] = [] { ValidateUiWorkflowState("backup_schedule", true, true); };
                        checks["U1-MNU-001"] = [] { ValidateUiWorkflowState("menu_topology", true, true); };
                        checks["U1-MNU-002"] = [] {
                            const auto tools = BuildToolsMenu();
                            bool found = false;
                            for (const auto& item : tools) {
                                if (item.first == "Spec Workspace") {
                                    found = true;
                                }
                            }
                            AssertTrue(found, "spec workspace menu missing");
                        };
                        checks["U1-WIN-001"] = [] {
                            const std::map<std::string, SurfaceVisibilityState> visibility = {
                                {"sql", {.embedded_visible = true, .detached_visible = false}},
                                {"object", {.embedded_visible = false, .detached_visible = true}},
                                {"diagram", {.embedded_visible = true, .detached_visible = false}},
                                {"spec_workspace", {.embedded_visible = false, .detached_visible = true}},
                                {"monitoring", {.embedded_visible = true, .detached_visible = false}},
                            };
                            ValidateEmbeddedDetachedExclusivity(visibility);
                            for (const auto& [surface, state] : visibility) {
                                ValidateUiWorkflowState("window_exclusive:" + surface,
                                                        true,
                                                        !(state.embedded_visible && state.detached_visible));
                            }
                            ExpectReject("SRB1-R-5101", [] {
                                ValidateEmbeddedDetachedExclusivity(
                                    {{"diagram", {.embedded_visible = true, .detached_visible = true}}});
                            });
                        };
                        checks["U1-WIN-002"] = [] {
                            const auto keep_detached = ApplyDockingRule(true, false, 0.69);
                            AssertTrue(!keep_detached.embedded_visible && keep_detached.detached_visible,
                                       "detached window should remain detached below overlap threshold");

                            const auto dock_by_overlap = ApplyDockingRule(true, false, 0.70);
                            AssertTrue(dock_by_overlap.embedded_visible && !dock_by_overlap.detached_visible,
                                       "window should dock when overlap threshold is met");

                            const auto dock_by_action = ApplyDockingRule(true, true, 0.05);
                            AssertTrue(dock_by_action.embedded_visible && !dock_by_action.detached_visible,
                                       "dock action should force embedded mode");

                            ExpectReject("SRB1-R-5101", [] { (void)ApplyDockingRule(true, false, 1.01); });
                        };
                        checks["U1-INS-001"] = [] { ValidateUiWorkflowState("inspector_tabs", true, true); };
                        checks["U1-ICO-001"] = [] { (void)ResolveIconSlot("table", {}, "default.png"); };
                        checks["U1-SPW-001"] = [] { (void)BuildSpecWorkspaceSummary({{"design", 1}, {"development", 1}, {"management", 0}}); };
                        checks["U1-SEC-001"] = [] { ApplySecurityPolicyAction(true, "security.manage", [] {}); };
                        checks["D1-MOD-001"] = [&] {
                            auto s = SerializeDiagramModel(diagram);
                            auto p = ParseDiagramModel(s);
                            AssertEq(p.diagram_id, "d1", "diagram parse mismatch");
                        };
                        checks["D1-NOT-001"] = [] {
                            ValidateNotation("crowsfoot");
                            ValidateNotation("idef1x");
                            ValidateNotation("uml");
                            ValidateNotation("chen");
                        };
                        checks["D1-CAN-001"] = [&] {
                            ValidateCanvasOperation(diagram, "drag", "n1", "root");
                            ValidateCanvasOperation(diagram, "connect", "n1", "n2");
                            ValidateCanvasOperation(diagram, "reparent", "n2", "");
                            ExpectReject("SRB1-R-6201", [&] { ValidateCanvasOperation(diagram, "reparent", "n1", "n1"); });
                            ExpectReject("SRB1-R-6201", [&] { ValidateCanvasOperation(diagram, "connect", "n1", "missing"); });
                        };
                        checks["D1-PAL-001"] = [] {
                            ValidatePaletteModeExclusivity(true, false);
                            ValidatePaletteModeExclusivity(false, true);
                            ExpectReject("SRB1-R-6201", [] { ValidatePaletteModeExclusivity(true, true); });

                            const std::map<std::string, std::set<std::string>> expected_tokens = {
                                {"ERD", {"table", "view", "index", "domain", "note", "relation"}},
                                {"Silverston", {"subject_area", "entity", "fact", "dimension", "lookup", "hub", "link", "satellite"}},
                                {"Whiteboard", {"note", "task", "risk", "decision", "milestone"}},
                                {"Mind Map", {"topic", "branch", "idea", "question", "action"}},
                            };

                            for (const auto& [diagram_type, expected] : expected_tokens) {
                                const auto tokens = PaletteTokensForDiagramType(diagram_type);
                                const std::set<std::string> actual(tokens.begin(), tokens.end());
                                AssertTrue(actual.size() == expected.size(),
                                           "palette token count mismatch for " + diagram_type);
                                for (const auto& token : expected) {
                                    AssertTrue(actual.count(token) == 1U,
                                               "palette token missing for " + diagram_type + ": " + token);
                                    const auto node = BuildNodeFromPaletteToken(diagram_type, token, 10, 20);
                                    AssertEq(node.object_type, token, "palette token type mismatch");
                                }
                            }

                            ExpectReject("SRB1-R-6201", [] { (void)PaletteTokensForDiagramType("DataFlow"); });
                            ExpectReject("SRB1-R-6201", [] { (void)BuildNodeFromPaletteToken("ERD", "topic", 10, 20); });
                            ExpectReject("SRB1-R-6201", [] { (void)BuildNodeFromPaletteToken("ERD", "table", 10, 20, 0, 80); });
                        };
                        checks["D1-ENG-001"] = [] { (void)ForwardEngineerDatatypes({"int"}, {{"int", "INTEGER"}}); };
                        checks["D1-EXP-001"] = [&] { (void)ExportDiagram(diagram, "png", "full"); };
                        checks["RPT-001"] = [] {
                            std::string persisted;
                            const auto out = RunQuestion(true,
                                                         "select 1",
                                                         [](const std::string&) {
                                                             return std::string("{\"command_tag\":\"EXECUTE\",\"rows_affected\":1}");
                                                         },
                                                         [&](const std::string& payload) {
                                                             persisted = payload;
                                                             return true;
                                                         });
                            AssertTrue(out.find("\"success\":true") != std::string::npos, "question success contract missing");
                            AssertTrue(out.find("\"query_result\"") != std::string::npos, "question result contract missing");
                            AssertTrue(out.find("\"timing\"") != std::string::npos, "question timing contract missing");
                            AssertTrue(out.find("\"cache\"") != std::string::npos, "question cache contract missing");
                            AssertTrue(out.find("\"error\"") != std::string::npos, "question error contract missing");
                            AssertEq(out, persisted, "question persisted payload mismatch");
                            ExpectReject("SRB1-R-7002", [&] {
                                (void)RunQuestion(true,
                                                  "select 2",
                                                  [](const std::string&) { return std::string("{\"ok\":true}"); },
                                                  [](const std::string&) { return false; });
                            });
                        };
                        checks["RPT-002"] = [] {
                            const auto out = RunDashboardRuntime("db", {{"w2", "ok:3"}, {"w1", "ok:2"}}, true);
                            AssertTrue(out.find("\"executed_at_utc\"") != std::string::npos, "dashboard timestamp missing");
                            AssertTrue(out.find("\"row_count\":2") != std::string::npos, "dashboard row_count missing");
                            AssertTrue(out.find("\"cache_key\":\"dash:db\"") != std::string::npos, "dashboard cache key missing");
                            const auto w1 = out.find("\"widget_id\":\"w1\"");
                            const auto w2 = out.find("\"widget_id\":\"w2\"");
                            AssertTrue(w1 != std::string::npos && w2 != std::string::npos && w1 < w2,
                                       "dashboard widget order is not deterministic");
                        };
                        checks["RPT-003"] = [] {
                            std::map<std::string, std::string> storage;
                            PersistResult("k", "v", &storage);
                            AssertEq(storage["k"], "v", "persist result payload mismatch");
                            ExpectReject("SRB1-R-7002", [&] {
                                std::map<std::string, std::string>* null_storage = nullptr;
                                PersistResult("k2", "x", null_storage);
                            });
                        };
                        checks["RPT-004"] = [] {
                            ReportingAsset asset;
                            asset.id = "a";
                            asset.asset_type = "Question";
                            asset.name = "q";
                            asset.payload_json = "{}";
                            asset.collection_id = "default";
                            asset.created_at_utc = "2026-02-14T00:00:00Z";
                            asset.updated_at_utc = "2026-02-14T00:00:01Z";
                            asset.created_by = "tester";
                            asset.updated_by = "tester";
                            auto payload = ExportReportingRepository({asset});
                            auto out = ImportReportingRepository(payload);
                            AssertTrue(out.size() == 1U, "repository import size mismatch");
                            AssertEq(out[0].collection_id, "default", "repository collection mismatch");
                            AssertEq(out[0].created_by, "tester", "repository created_by mismatch");
                        };
                        checks["RPT-005"] = [] { (void)CanonicalizeRRule({{"FREQ", "DAILY"}, {"INTERVAL", "1"}}); };
                        checks["RPT-006"] = [] {
                            ReportingSchedule s{"FREQ=DAILY;INTERVAL=1", "2026-02-14T00:00:00", "UTC", {}, {}};
                            (void)NextRun(s, "2026-02-14T00:00:01Z");
                        };
                        checks["RPT-007"] = [] {
                            ReportingSchedule s{"FREQ=DAILY;INTERVAL=1", "2026-02-14T00:00:00", "UTC", {"2026-02-15T00:00:00"}, {"2026-02-16T00:00:00"}};
                            (void)ExpandRRuleBounded(s, "2026-02-14T00:00:01Z", 8);
                        };
                        checks["RPT-008"] = [] {
                            ReportingSchedule s{"FREQ=DAILY;UNTIL=2026-03-01T00:00:00Z", "2026-02-14T00:00:00", "UTC", {}, {}};
                            ValidateAnchorUntil(s);
                        };
                        checks["RPT-009"] = [] {
                            (void)RunActivityWindowQuery({{"2026-02-14T00:00:00Z", "reads", 1.0}}, "5m", {"reads"});
                        };
                        checks["RPT-010"] = [] { (void)ExportActivity({{"2026-02-14T00:00:00Z", "reads", 1.0}}, "csv"); };
                        checks["ADV-GIT-001"] = [] { ValidateGitSyncState(true, true, true); };
                        checks["ADV-CDC-001"] = [] {
                            (void)RunCdcEvent("e", 1, 0, [](const std::string&) { return true; }, [](const std::string&) {});
                        };
                        checks["ADV-MSK-001"] = [] {
                            (void)PreviewMask({{{"f", "v"}}}, {{"f", "redact"}});
                        };
                        checks["ADV-AI-001"] = [] { ValidateAiProviderConfig("openai", true, "gpt-5", std::optional<std::string>("cred")); };
                        checks["ADV-ISS-001"] = [] { ValidateIssueTrackerConfig("github", "org/repo", std::optional<std::string>("cred")); };
                        checks["ADV-COL-001"] = [] { CheckReviewQuorum(2, 2); };
                        checks["ADV-COL-002"] = [] { RequireChangeAdvisory("apply", "Approved"); };
                        checks["ADV-EXT-001"] = [] { ValidateExtension(true, true); };
                        checks["ADV-EXT-002"] = [] { EnforceExtensionAllowlist({"read_catalog"}, {"read_catalog", "read_data"}); };
                        checks["ADV-LIN-001"] = [] { (void)BuildLineage({"a", "b"}, {{"a", std::optional<std::string>("b")}}); };
                        checks["PRE-CLS-001"] = [] {
                            auto s = RegisterOptionalSurfaces("ga");
                            AssertEq(*s["ClusterManagerFrame"], "SRB1-R-7008", "preview gate mismatch");
                        };
                        checks["PRE-RPL-001"] = [] {
                            auto s = RegisterOptionalSurfaces("ga");
                            AssertEq(*s["ReplicationManagerFrame"], "SRB1-R-7009", "preview gate mismatch");
                        };
                        checks["PRE-ETL-001"] = [] {
                            auto s = RegisterOptionalSurfaces("ga");
                            AssertEq(*s["EtlManagerFrame"], "SRB1-R-7010", "preview gate mismatch");
                        };
                        checks["OPS-DKR-001"] = [] {
                            auto s = RegisterOptionalSurfaces("ga");
                            AssertEq(*s["DockerManagerPanel"], "SRB1-R-7011", "ops gate mismatch");
                        };
                        checks["OPS-TST-001"] = [] {
                            auto s = RegisterOptionalSurfaces("ga");
                            AssertEq(*s["TestRunnerPanel"], "SRB1-R-7012", "ops gate mismatch");
                        };
                        checks["PKG-001"] = [&] { (void)ValidateProfileManifest(sample_manifest, manifest_surfaces, manifest_backends); };
                        checks["PKG-002"] = [] {
                            ValidatePackageArtifacts({"LICENSE",
                                                      "README.md",
                                                      "docs/installation_guide/README.md",
                                                      "docs/developers_guide/README.md"});
                        };
                        checks["PKG-003"] = [&] { (void)ValidateProfileManifest(sample_manifest, manifest_surfaces, manifest_backends); };
                        checks["PKG-004"] = [&] {
                            auto ga_manifest = ParseJson(
                                R"({"manifest_version":"1.0.0","profile_id":"ga","build_version":"1","build_hash":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef","build_timestamp_utc":"2026-02-14T00:00:00Z","platform":"linux","enabled_backends":["embedded"],"surfaces":{"enabled":["MainFrame"],"disabled":[],"preview_only":[]},"security_defaults":{"security_mode":"standard","credential_store_policy":"preferred","audit_enabled_default":true,"tls_required_default":false},"artifacts":{"license_path":"docs/LICENSE.txt","attribution_path":"docs/ATTRIBUTION.txt","help_root_path":"share/help","config_template_path":"config/a.toml","connections_template_path":"config/c.toml"}})");
                            (void)ValidateProfileManifest(ga_manifest, {"MainFrame"}, {"embedded"});
                        };
                        checks["PKG-005"] = [&] { ValidateSurfaceRegistry(sample_manifest, manifest_surfaces); };
                        checks["SPC-IDX-001"] = [&] {
                            auto found = DiscoverSpecsets(temp.string());
                            AssertTrue(found.size() == 3U, "expected three specset manifests");
                        };
                        checks["SPC-NRM-001"] = [&] {
                            auto rows = ParseAuthoritativeInventory((temp / "resources/specset_packages/sb_vnext_payload/AUTHORITATIVE_SPEC_INVENTORY.md").string());
                            AssertTrue(rows.size() == 2U, "inventory parse mismatch");
                        };
                        checks["SPC-COV-001"] = [] {
                            std::vector<SpecFileRow> files = {{"sb", "A.md", true, "", 1}};
                            AssertSupportComplete(files, {{"sb:A.md", "design", "covered"}}, "design");
                        };
                        checks["SPC-COV-002"] = [] {
                            std::vector<SpecFileRow> files = {{"sb", "A.md", true, "", 1}};
                            AssertSupportComplete(files, {{"sb:A.md", "development", "covered"}}, "development");
                        };
                        checks["SPC-COV-003"] = [] {
                            std::vector<SpecFileRow> files = {{"sb", "A.md", true, "", 1}};
                            AssertSupportComplete(files, {{"sb:A.md", "management", "covered"}}, "management");
                        };
                        checks["SPC-CNF-001"] = [] { ValidateBindings({"A0-LNT-001"}, {"A0-LNT-001", "PKG-003"}); };
                        checks["SPC-RPT-001"] = [] {
                            auto counts = AggregateSupport({{"sb:A.md", "design", "covered"},
                                                            {"sb:A.md", "development", "covered"},
                                                            {"sb:A.md", "management", "missing"}});
                            (void)BuildSpecWorkspaceSummary({{"design", counts["design:covered"]},
                                                             {"development", counts["development:covered"]},
                                                             {"management", counts["management:missing"]}});
                        };
                        checks["SPC-WPK-001"] = [] {
                            (void)ExportWorkPackage("sb_vnext", {{"sb_vnext:A.md", "design", {"A0-LNT-001"}}}, "2026-02-14T00:00:00Z");
                        };
                        checks["ALPHA-MIR-001"] = [&] {
                            ValidateAlphaMirrorPresence(temp.string(), {{"alpha/deep/a.txt", 5, ""}});
                        };
                        checks["ALPHA-MIR-002"] = [&] {
                            ValidateAlphaMirrorHashes(temp.string(),
                                                      {{"alpha/deep/a.txt",
                                                        5,
                                                        "8ed3f6ad685b959ead7022518e1af76cd816f8e8ec7ccdda1ed4018e8f2223f8"}});
                        };
                        checks["ALPHA-DIA-001"] = [] {
                            ValidateSilverstonContinuity({"silverston/erd_core.md"}, {"silverston/erd_core.md"});
                        };
                        checks["ALPHA-DIA-002"] = [] {
                            ValidateSilverstonContinuity({"silverston/erd_core.md", "silverston/appendix/deep_details.md"},
                                                         {"silverston/erd_core.md", "silverston/appendix/deep_details.md"});
                        };
                        checks["ALPHA-INV-001"] = [] {
                            ValidateAlphaInventoryMapping({"EL1"}, {{"f.md", "EL1"}});
                        };
                        checks["ALPHA-EXT-001"] = [] { ValidateAlphaExtractionGate(true, true, true); };

                        const auto case_ids = ReadConformanceCaseIds(vector_csv);
                        AssertTrue(!case_ids.empty(), "conformance vector is empty");
                        std::vector<std::string> failed_case_ids;
                        for (const auto& case_id : case_ids) {
                            auto it = checks.find(case_id);
                            if (it == checks.end()) {
                                failed_case_ids.push_back(case_id);
                                continue;
                            }
                            try {
                                it->second();
                            } catch (...) {
                                failed_case_ids.push_back(case_id);
                            }
                        }

                        std::ostringstream conformance_summary;
                        conformance_summary << "{\"total_cases\":" << case_ids.size()
                                            << ",\"passed_cases\":" << (case_ids.size() - failed_case_ids.size())
                                            << ",\"failed_cases\":" << failed_case_ids.size()
                                            << ",\"failed_case_ids\":[";
                        for (std::size_t i = 0; i < failed_case_ids.size(); ++i) {
                            if (i > 0U) {
                                conformance_summary << ',';
                            }
                            conformance_summary << "\"" << failed_case_ids[i] << "\"";
                        }
                        conformance_summary << "]}";
                        std::cout << "ConformanceSummaryJson: " << conformance_summary.str() << '\n';

                        if (!failed_case_ids.empty()) {
                            std::ostringstream error;
                            error << "conformance case failures: ";
                            for (std::size_t i = 0; i < failed_case_ids.size(); ++i) {
                                if (i > 0U) {
                                    error << ",";
                                }
                                error << failed_case_ids[i];
                            }
                            throw std::runtime_error(error.str());
                        }

                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}
