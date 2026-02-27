#include "core/beta1b_contracts.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "tests/test_harness.h"

using namespace scratchrobin;
using namespace scratchrobin::beta1b;
using scratchrobin::tests::AssertEq;
using scratchrobin::tests::AssertTrue;

namespace {

void ExpectReject(const std::string& expected_code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        AssertEq(ex.payload().code, expected_code, "reject code mismatch");
        return;
    }
    throw std::runtime_error("expected RejectError not thrown");
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

    // Header placeholder (44 bytes)
    for (int i = 0; i < 44; ++i) {
        bytes.push_back(0);
    }

    // TOC entries
    auto append_toc = [&](const char id[4], std::uint64_t off, std::uint64_t sz, std::uint32_t crc, std::uint32_t ordinal) {
        bytes.push_back(static_cast<std::uint8_t>(id[0]));
        bytes.push_back(static_cast<std::uint8_t>(id[1]));
        bytes.push_back(static_cast<std::uint8_t>(id[2]));
        bytes.push_back(static_cast<std::uint8_t>(id[3]));
        WriteU32(&bytes, 0);      // chunk_flags
        WriteU64(&bytes, off);    // data_offset
        WriteU64(&bytes, sz);     // data_size
        WriteU32(&bytes, crc);    // data_crc
        WriteU16(&bytes, 1);      // payload_version
        WriteU16(&bytes, 0);      // reserved0
        WriteU32(&bytes, ordinal);
        WriteU32(&bytes, 0);      // reserved1
    };

    append_toc("PROJ", proj_off, proj_data.size(), Crc32Local(proj_data.data(), proj_data.size()), 0);
    append_toc("OBJS", objs_off, objs_data.size(), Crc32Local(objs_data.data(), objs_data.size()), 1);

    bytes.insert(bytes.end(), proj_data.begin(), proj_data.end());
    bytes.insert(bytes.end(), objs_data.begin(), objs_data.end());

    // Header fields
    bytes[0] = 'S'; bytes[1] = 'R'; bytes[2] = 'P'; bytes[3] = 'J';
    bytes[4] = 1; bytes[5] = 0;            // major
    bytes[6] = 0; bytes[7] = 0;            // minor
    bytes[8] = 44; bytes[9] = 0;           // header_size
    bytes[10] = 40; bytes[11] = 0;         // toc_entry_size
    // chunk_count = 2
    bytes[12] = 2; bytes[13] = 0; bytes[14] = 0; bytes[15] = 0;
    // toc_offset = 44
    bytes[16] = 44; bytes[17] = 0; bytes[18] = 0; bytes[19] = 0;
    bytes[20] = 0; bytes[21] = 0; bytes[22] = 0; bytes[23] = 0;
    // declared_file_size
    std::uint64_t fs = file_size;
    for (int i = 0; i < 8; ++i) {
        bytes[24 + i] = static_cast<std::uint8_t>((fs >> (i * 8)) & 0xFF);
    }
    // flags/reserved already zero

    // header crc with crc bytes zeroed
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
        throw std::runtime_error("json parse failed: " + error);
    }
    return value;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"connection/select_backend", [] {
                        ConnectionProfile p;
                        p.backend = "pg";
                        AssertEq(SelectBackend(p), "postgresql", "backend mapping failed");
                    }});

    tests.push_back({"connection/select_backend_reject", [] {
                        ConnectionProfile p;
                        p.backend = "unknown_engine";
                        ExpectReject("SRB1-R-4001", [&] { (void)SelectBackend(p); });
                    }});

    tests.push_back({"connection/resolve_credential", [] {
                        ConnectionProfile p;
                        p.credential_id = "cred1";
                        const auto secret = ResolveCredential(p, {{"cred1", "pass"}}, std::nullopt);
                        AssertEq(secret, "pass", "credential mismatch");
                    }});

    tests.push_back({"connection/enterprise_connect", [] {
                        EnterpriseConnectionProfile p;
                        p.profile_id = "prod";
                        p.username = "svc";
                        p.transport = TransportContract{"ssh_jump_chain", "required", 15000};
                        p.ssh = SshContract{"db.internal", 5432, "svc", "keypair", "cred_ssh"};
                        p.jump_hosts.push_back(JumpHost{"bastion", 22, "jump", "agent", ""});
                        p.identity = IdentityContract{"oidc", "corp_oidc", {"openid"}};
                        p.secret_provider = SecretProviderContract{"vault", "kv/data/x"};

                        auto fp = ConnectEnterprise(
                            p,
                            std::nullopt,
                            [](const SecretProviderContract&) { return std::optional<std::string>("vault-secret"); },
                            [](const std::string&) { return std::optional<std::string>("credential-secret"); },
                            [](const std::string&, const std::string&) { return true; },
                            [](const std::string&, const std::string&) { return true; });

                        AssertEq(fp.identity_mode, "oidc", "identity mode mismatch");
                        AssertEq(fp.identity_method_id, "scratchbird.auth.jwt_oidc", "identity method mismatch");
                    }});

    tests.push_back({"connection/enterprise_proxy_assertion_profile", [] {
                        EnterpriseConnectionProfile p;
                        p.profile_id = "proxy";
                        p.username = "svc";
                        p.transport = TransportContract{"direct", "required", 5000};
                        p.identity = IdentityContract{"oidc", "idp", {"openid"}};
                        p.identity.auth_method_id = "scratchbird.auth.proxy_assertion";
                        p.identity.proxy_principal_assertion = "proxy.jwt";
                        p.proxy_assertion_only = true;
                        p.no_login_direct = true;

                        auto fp = ConnectEnterprise(
                            p,
                            std::optional<std::string>("runtime_secret"),
                            [](const SecretProviderContract&) { return std::optional<std::string>(); },
                            [](const std::string&) { return std::optional<std::string>(); },
                            [](const std::string&, const std::string&) { return true; },
                            [](const std::string&, const std::string&) { return true; });

                        AssertEq(fp.identity_method_id,
                                 "scratchbird.auth.proxy_assertion",
                                 "proxy assertion method mismatch");
                        AssertTrue(fp.proxy_assertion_only, "proxy_assertion_only flag mismatch");
                        AssertTrue(fp.no_login_direct, "no_login_direct flag mismatch");
                    }});

    tests.push_back({"connection/enterprise_proxy_assertion_rejects_non_proxy_method", [] {
                        EnterpriseConnectionProfile p;
                        p.profile_id = "bad_proxy";
                        p.username = "svc";
                        p.transport = TransportContract{"direct", "required", 5000};
                        p.identity = IdentityContract{"oidc", "idp", {"openid"}};
                        p.identity.auth_method_id = "scratchbird.auth.jwt_oidc";
                        p.proxy_assertion_only = true;
                        p.identity.proxy_principal_assertion = "proxy.jwt";
                        ExpectReject("SRB1-R-4005", [&] {
                            (void)ConnectEnterprise(
                                p,
                                std::optional<std::string>("runtime_secret"),
                                [](const SecretProviderContract&) { return std::optional<std::string>(); },
                                [](const std::string&) { return std::optional<std::string>(); },
                                [](const std::string&, const std::string&) { return true; },
                                [](const std::string&, const std::string&) { return true; });
                        });
                    }});

    tests.push_back({"connection/enterprise_p2_identity_modes", [] {
                        EnterpriseConnectionProfile p;
                        p.profile_id = "ident_profile";
                        p.username = "svc";
                        p.transport = TransportContract{"direct", "required", 5000};
                        p.allow_inline_secret = true;
                        p.inline_secret = std::string("ident-secret");
                        p.identity = IdentityContract{"ident", "ident_local", {}};
                        p.identity.provider_profile = "ident_local_net";

                        auto fp = ConnectEnterprise(
                            p,
                            std::nullopt,
                            [](const SecretProviderContract&) { return std::optional<std::string>(); },
                            [](const std::string&) { return std::optional<std::string>(); },
                            [](const std::string&, const std::string&) { return true; },
                            [](const std::string&, const std::string&) { return true; });

                        AssertEq(fp.identity_method_id,
                                 "scratchbird.auth.ident_rfc1413",
                                 "ident mode default method mismatch");
                        AssertEq(fp.identity_provider_profile, "ident_local_net", "provider profile mismatch");
                    }});

    tests.push_back({"connection/copy_io", [] {
                        auto r = RunCopyIo("COPY t TO STDOUT", "stdin", "stdout", true, true);
                        AssertEq(r, "copy-ok", "copy path mismatch");
                        ExpectReject("SRB1-R-4203", [] { (void)RunCopyIo("COPY t TO STDOUT", "file", "stdout", false, true); });
                    }});

    tests.push_back({"connection/prepared_status", [] {
                        auto p = PrepareExecuteClose(true, "select ? from rdb$database", {"1"});
                        AssertTrue(p.find("prepared-ok") == 0U, "prepared response mismatch");
                        auto s = StatusSnapshot(true, 1, 2);
                        AssertTrue(s.find("running_queries") != std::string::npos, "status payload missing");
                        ExpectReject("SRB1-R-4201", [] { (void)PrepareExecuteClose(false, "select 1", {}); });
                    }});

    tests.push_back({"project/load_binary", [] {
                        auto bytes = BuildValidProjectBinary();
                        auto loaded = LoadProjectBinary(bytes);
                        AssertTrue(loaded.loaded_chunks.count("PROJ") == 1U, "missing PROJ");
                        AssertTrue(loaded.loaded_chunks.count("OBJS") == 1U, "missing OBJS");
                    }});

    tests.push_back({"project/load_binary_size_reject", [] {
                        auto bytes = BuildValidProjectBinary();
                        bytes[24] ^= 0x01;
                        ExpectReject("SRB1-R-3101", [&] { (void)LoadProjectBinary(bytes); });
                    }});

    tests.push_back({"project/validate_payload", [] {
                        auto json = ParseJson(
                            R"({"project":{"project_id":"123e4567-e89b-12d3-a456-426614174000","name":"x","created_at":"2026-02-14T00:00:00Z","updated_at":"2026-02-14T00:00:00Z","config":{"default_environment_id":"dev","active_connection_id":null,"connections_file_path":"config/connections.toml","governance":{"owners":["owner"],"stewards":[],"review_min_approvals":1,"allowed_roles_by_environment":{"dev":["owner"]},"ai_policy":{"enabled":true,"require_review":false,"allow_scopes":["design"],"deny_scopes":[]},"audit_policy":{"level":"standard","retention_days":30,"export_enabled":true}},"security_mode":"standard","features":{"sql_editor":true}},"objects":[],"objects_by_path":{},"reporting_assets":[],"reporting_schedules":[],"data_view_snapshots":[],"git_sync_state":null,"audit_log_path":"audit.log"}})");
                        ValidateProjectPayload(json);
                    }});

    tests.push_back({"governance/blocker_validation", [] {
                        std::vector<BlockerRow> rows = {
                            {"BLK-0001", "P0", "open", "conformance_case", "A0-LNT-001", "2026-02-14T00:00:00Z",
                             "2026-02-14T00:00:00Z", "agent", "critical blocker"},
                            {"BLK-0002", "P2", "waived", "manual", "ticket-1", "2026-02-14T00:00:00Z",
                             "2026-02-14T00:00:00Z", "owner", "preview-only"},
                        };
                        ValidateBlockerRows(rows);
                    }});

    tests.push_back({"governance/deny_no_side_effect", [] {
                        bool applied = false;
                        bool audited = false;
                        ExpectReject("SRB1-R-3202", [&] {
                            EnforceGovernanceGate(false,
                                                  [&] { applied = true; },
                                                  [&](const std::string&) { audited = true; });
                        });
                        AssertTrue(!applied, "denied action had side effect");
                        AssertTrue(audited, "deny path must audit");
                    }});

    tests.push_back({"ui/suggestion_order", [] {
                        std::vector<SuggestionCandidate> c = {{"select", 1}, {"session", 1}, {"self", 0}};
                        auto out = SortedSuggestions(c, "se", [](std::string_view token, std::string_view prefix) {
                            return static_cast<int>(token.size()) - static_cast<int>(prefix.size());
                        });
                        AssertEq(out.front(), "self", "unexpected first suggestion");
                    }});

    tests.push_back({"ui/snippet_insert", [] {
                        Snippet s{"id", "name", "SELECT 1;", "global", "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z"};
                        AssertEq(SnippetInsertExact(s), "SELECT 1;", "snippet body mismatch");
                    }});

    tests.push_back({"ui/prune_export_history", [] {
                        std::vector<QueryHistoryRow> rows = {
                            {"1", "p", "2026-02-13T00:00:00Z", 1, "success", "", "h1"},
                            {"2", "p", "2026-02-14T00:00:00Z", 2, "success", "", "h2"},
                        };
                        auto pruned = PruneHistory(rows, "2026-02-13T12:00:00Z");
                        AssertTrue(pruned.size() == 1U, "expected one row");
                        auto csv = ExportHistoryCsv(pruned);
                        AssertTrue(csv.find("query_id") != std::string::npos, "csv missing header");
                    }});

    tests.push_back({"ui/schema_ops_sort", [] {
                        std::vector<SchemaCompareOperation> ops = {
                            {"2", "table", "public.b", "drop", "DROP TABLE public.b"},
                            {"1", "table", "public.a", "alter", "ALTER TABLE public.a"},
                        };
                        auto sorted = StableSortOps(ops);
                        AssertEq(sorted.front().operation_id, "1", "sort mismatch");
                    }});

    tests.push_back({"ui/builder_guard", [] {
                        ExpectReject("SRB1-R-5108", [] {
                            (void)ApplyBuilderGraph(true, true, "", false);
                        });
                    }});

    tests.push_back({"ui/workflow_and_icons", [] {
                        ValidateUiWorkflowState("main_frame_refresh", true, true);
                        ExpectReject("SRB1-R-5101", [] { ValidateUiWorkflowState("main_frame_refresh", true, false); });
                        auto icon = ResolveIconSlot("table", {{"column", "col.png"}}, "default.png");
                        AssertEq(icon, "default.png", "icon fallback mismatch");
                    }});

    tests.push_back({"ui/spec_workspace_summary", [] {
                        auto s = BuildSpecWorkspaceSummary({{"design", 1}, {"development", 2}, {"management", 3}});
                        AssertTrue(s.find("\"total\":6") != std::string::npos, "summary total mismatch");
                    }});

    tests.push_back({"diagram/contracts", [] {
                        DiagramDocument doc;
                        doc.diagram_id = "d1";
                        doc.notation = "crowsfoot";
                        doc.nodes.push_back({"n1", "table", "root", 0, 0, 100, 50, "INT"});
                        doc.nodes.push_back({"n2", "table", "root", 120, 0, 100, 50, "INT"});
                        doc.edges.push_back({"e1", "n1", "n2", "fk"});
                        ValidateCanvasOperation(doc, "drag", "n1", "");
                        auto model = SerializeDiagramModel(doc);
                        auto parsed = ParseDiagramModel(model);
                        AssertEq(parsed.diagram_id, "d1", "diagram parse mismatch");
                        auto mapped = ForwardEngineerDatatypes({"int", "varchar"}, {{"int", "INTEGER"}, {"varchar", "VARCHAR(50)"}});
                        AssertTrue(mapped.size() == 2U, "forward map mismatch");
                        auto exp = ExportDiagram(doc, "svg", "ga");
                        AssertTrue(exp.find("diagram-export:svg") == 0U, "diagram export mismatch");
                        ExpectReject("SRB1-R-6303", [&] { (void)ExportDiagram(doc, "pdf", "minimal_ui"); });
                    }});

    tests.push_back({"reporting/canonicalize_rrule", [] {
                        std::map<std::string, std::string> kv = {{"INTERVAL", "1"}, {"FREQ", "DAILY"}};
                        auto rule = CanonicalizeRRule(kv);
                        AssertEq(rule, "FREQ=DAILY;INTERVAL=1", "rrule canonical mismatch");
                    }});

    tests.push_back({"reporting/next_run", [] {
                        ReportingSchedule s;
                        s.schedule_spec = "FREQ=DAILY;INTERVAL=1";
                        s.schedule_dtstart_local = "2026-02-14T00:00:00";
                        s.timezone = "UTC";
                        auto next = NextRun(s, "2026-02-14T00:00:01Z");
                        AssertEq(next, "2026-02-15T00:00:00Z", "next_run mismatch");
                    }});

    tests.push_back({"reporting/activity_export", [] {
                        std::vector<ActivityRow> rows = {{"2026-02-14T00:00:00Z", "reads", 1.0}};
                        auto out = ExportActivity(rows, "json");
                        AssertTrue(out.find("reads") != std::string::npos, "json export missing metric");
                    }});

    tests.push_back({"reporting/repository_roundtrip", [] {
                        std::vector<ReportingAsset> assets = {
                            {"b", "dashboard", "db", "{}"},
                            {"a", "question", "q", "{}"},
                        };
                        auto payload = ExportReportingRepository(assets);
                        auto imported = ImportReportingRepository(payload);
                        AssertTrue(imported.size() == 2U, "imported size mismatch");
                        AssertEq(imported.front().id, "b", "canonical ordering mismatch");
                    }});

    tests.push_back({"advanced/cdc_retry_reject", [] {
                        int dead_letter_calls = 0;
                        ExpectReject("SRB1-R-7004", [&] {
                            (void)RunCdcEvent("evt", 2, 1, [](const std::string&) { return false; },
                                              [&](const std::string&) { ++dead_letter_calls; });
                        });
                        AssertTrue(dead_letter_calls == 1, "dead letter not called");
                    }});

    tests.push_back({"advanced/masking_preview", [] {
                        std::vector<std::map<std::string, std::string>> rows = {{{"name", "alice"}}};
                        auto out = PreviewMask(rows, {{"name", "redact"}});
                        AssertEq(out[0]["name"], "***", "mask failed");
                    }});

    tests.push_back({"advanced/extension_allowlist", [] {
                        ExpectReject("SRB1-R-7304", [] {
                            EnforceExtensionAllowlist({"write_fs"}, {"read_catalog"});
                        });
                    }});

    tests.push_back({"advanced/optional_surfaces", [] {
                        auto map = RegisterOptionalSurfaces("ga");
                        AssertTrue(map["ClusterManagerFrame"].has_value(), "ga should reject cluster manager");
                    }});

    tests.push_back({"advanced/config_validators", [] {
                        ValidateAiProviderConfig("openai", true, "gpt-5", std::optional<std::string>("cred"));
                        ValidateIssueTrackerConfig("github", "repo/a", std::optional<std::string>("token"));
                        ValidateGitSyncState(true, true, true);
                        ExpectReject("SRB1-R-7006", [] {
                            ValidateAiProviderConfig("unknown", true, "x", std::optional<std::string>("c"));
                        });
                        ExpectReject("SRB1-R-7007", [] {
                            ValidateIssueTrackerConfig("github", "", std::optional<std::string>("token"));
                        });
                        ExpectReject("SRB1-R-8201", [] {
                            ValidateGitSyncState(true, true, false);
                        });
                    }});

    tests.push_back({"packaging/build_hash", [] {
                        auto hash = CanonicalBuildHash("0123456789abcdef0123456789abcdef01234567");
                        AssertTrue(hash.size() == 64U, "hash length invalid");
                    }});

    tests.push_back({"packaging/manifest_validation", [] {
                        auto manifest = ParseJson(
                            R"({"manifest_version":"1.0.0","profile_id":"full","build_version":"1","build_hash":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef","build_timestamp_utc":"2026-02-14T00:00:00Z","platform":"linux","enabled_backends":["embedded","firebird"],"surfaces":{"enabled":["MainFrame"],"disabled":["SqlEditorFrame"],"preview_only":[]},"security_defaults":{"security_mode":"standard","credential_store_policy":"preferred","audit_enabled_default":true,"tls_required_default":false},"artifacts":{"license_path":"docs/LICENSE.txt","attribution_path":"docs/ATTRIBUTION.txt","help_root_path":"share/help","config_template_path":"config/a.toml","connections_template_path":"config/c.toml"}})");

                        std::set<std::string> surfaces = {"MainFrame", "SqlEditorFrame"};
                        std::set<std::string> backends = {"embedded", "firebird", "network"};
                        auto result = ValidateProfileManifest(manifest, surfaces, backends);
                        AssertTrue(result.ok, "manifest should be valid");
                    }});

    tests.push_back({"packaging/package_artifacts", [] {
                        ValidatePackageArtifacts({"LICENSE",
                                                  "README.md",
                                                  "docs/installation_guide/README.md",
                                                  "docs/developers_guide/README.md"});
                        ExpectReject("SRB1-R-9003", [] {
                            ValidatePackageArtifacts({"LICENSE", "README.md"});
                        });
                    }});

    tests.push_back({"spec_support/validate_bindings", [] {
                        ValidateBindings({"A0-LNT-001", "PKG-003"}, {"A0-LNT-001", "PKG-003", "RPT-001"});
                        ExpectReject("SRB1-R-5404", [] {
                            ValidateBindings({"UNKNOWN-1"}, {"A0-LNT-001"});
                        });
                    }});

    tests.push_back({"spec_support/work_package_export", [] {
                        auto json = ExportWorkPackage("sb_vnext",
                                                      {{"sb_vnext:file.md", "development", {"SPC-COV-002", "SPC-CNF-001"}}},
                                                      "2026-02-14T15:50:00Z");
                        AssertTrue(json.find("sb_vnext") != std::string::npos, "export missing set id");
                    }});

    tests.push_back({"alpha/contracts", [] {
                        namespace fs = std::filesystem;
                        const auto temp = fs::temp_directory_path() / "scratchrobin_alpha_contracts";
                        fs::remove_all(temp);
                        fs::create_directories(temp / "deep");
                        {
                            std::ofstream out(temp / "deep/a.txt", std::ios::binary);
                            out << "alpha";
                        }
                        std::vector<AlphaMirrorEntry> entries = {
                            {"deep/a.txt", 5, "8ed3f6ad685b959ead7022518e1af76cd816f8e8ec7ccdda1ed4018e8f2223f8"},
                        };
                        ValidateAlphaMirrorPresence(temp.string(), entries);
                        ValidateAlphaMirrorHashes(temp.string(), entries);
                        ValidateSilverstonContinuity({"silverston/erd_core.md"}, {"silverston/erd_core.md"});
                        ValidateAlphaInventoryMapping({"EL-1", "EL-2"}, {{"a.md", "EL-1"}, {"b.md", "EL-2"}});
                        ValidateAlphaExtractionGate(true, true, true);
                        ExpectReject("SRB1-R-5501", [&] {
                            ValidateAlphaMirrorPresence(temp.string(), {{"missing.txt", 0, ""}});
                        });
                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}
