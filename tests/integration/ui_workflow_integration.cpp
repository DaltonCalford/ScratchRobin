#include "ui/ui_workflow_services.h"
#include "tests/test_harness.h"

#include <algorithm>
#include <cctype>

using namespace scratchrobin;
using namespace scratchrobin::ui;

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

connection::BackendAdapterService BuildConnectedAdapter() {
    connection::BackendAdapterService adapter;
    runtime::ConnectionProfile profile;
    profile.name = "local";
    profile.backend = "scratchbird";
    profile.mode = runtime::ConnectionMode::Network;
    profile.host = "127.0.0.1";
    profile.database = "scratchbird";
    profile.username = "sysdba";
    profile.credential_id = "default";
    adapter.Connect(profile);
    return adapter;
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/ui_menu_and_surface", [] {
                        auto adapter = BuildConnectedAdapter();
                        project::SpecSetService specset;
                        UiWorkflowService ui(&adapter, &specset);

                        const auto menu = ui.MainMenuTopology();
                        scratchrobin::tests::AssertTrue(menu.size() == 9U, "menu topology size mismatch");
                        scratchrobin::tests::AssertEq(menu[5], "Tools", "tools menu expected at index 5");

                        ui.EnsureSpecWorkspaceEntrypoint();
                        ui.ValidateSurfaceOpen("main_frame", true, true);
                        ExpectReject("SRB1-R-5101", [&] { ui.ValidateSurfaceOpen("main_frame", true, false); });
                    }});

    tests.push_back({"integration/ui_sql_productivity_history", [] {
                        auto adapter = BuildConnectedAdapter();
                        project::SpecSetService specset;
                        UiWorkflowService ui(&adapter, &specset);

                        auto run = ui.RunSqlEditorQuery("select 1", true, 1, 0);
                        scratchrobin::tests::AssertEq(run.command_tag, "EXECUTE", "command tag mismatch");
                        scratchrobin::tests::AssertTrue(run.status_payload.find("running_queries") != std::string::npos,
                                                        "status payload missing");

                        auto suggestions = ui.SortedSqlSuggestions(
                            {{"select", 1}, {"session", 2}, {"self", 0}},
                            "se",
                            [](std::string_view token, std::string_view prefix) {
                                return static_cast<int>(token.size()) - static_cast<int>(prefix.size());
                            });
                        scratchrobin::tests::AssertEq(suggestions.front(), "self", "suggestion ordering mismatch");

                        auto snippet = ui.InsertSnippetExact(
                            {"id", "name", "SELECT 1;", "global", "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z"});
                        scratchrobin::tests::AssertEq(snippet, "SELECT 1;", "snippet insertion mismatch");
                        ui.UpsertSnippet(true, {"s1", "shared", "SELECT now();", "global",
                                                "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z"});
                        ui.UpsertSnippet(true, {"s2", "session", "SELECT 2;", "connection",
                                                "2026-02-14T00:00:00Z", "2026-02-14T00:00:00Z"});
                        auto global_snippets = ui.ListSnippets(true, "global");
                        scratchrobin::tests::AssertTrue(global_snippets.size() == 1U, "global snippet filter mismatch");
                        ui.RemoveSnippet(true, "s2");
                        ExpectReject("SRB1-R-5103", [&] { ui.RemoveSnippet(true, "missing"); });

                        std::vector<beta1b::QueryHistoryRow> rows = {
                            {"1", "p", "2026-02-13T00:00:00Z", 1, "success", "", "h1"},
                            {"2", "p", "2026-02-14T00:00:00Z", 2, "success", "", "h2"},
                        };
                        auto export_csv = ui.PruneAndExportHistory(rows, "2026-02-13T12:00:00Z", "csv");
                        scratchrobin::tests::AssertTrue(export_csv.retained_rows == 1U, "history prune mismatch");
                        scratchrobin::tests::AssertTrue(export_csv.payload.find("query_id") != std::string::npos, "csv header missing");
                        auto export_json = ui.PruneAndExportHistory(rows, "2026-02-13T12:00:00Z", "json");
                        scratchrobin::tests::AssertTrue(export_json.payload.find("\"query_id\"") != std::string::npos, "json export missing");
                        ui.AppendHistoryRow({"h1", "p", "2026-02-14T00:00:00Z", 10, "success", "", "hash1"});
                        ui.AppendHistoryRow({"h2", "other", "2026-02-14T00:01:00Z", 12, "success", "", "hash2"});
                        auto profile_rows = ui.QueryHistoryByProfile("p");
                        scratchrobin::tests::AssertTrue(profile_rows.size() == 1U, "stored history profile filter mismatch");
                        auto stored_export = ui.PruneAndExportStoredHistory("p", "2026-02-13T00:00:00Z", "csv");
                        scratchrobin::tests::AssertTrue(stored_export.retained_rows == 1U, "stored history export mismatch");
                    }});

    tests.push_back({"integration/ui_compare_migration_plan_builder", [] {
                        auto adapter = BuildConnectedAdapter();
                        project::SpecSetService specset;
                        UiWorkflowService ui(&adapter, &specset);

                        auto sorted_ops = ui.BuildSchemaCompareSet({
                            {"2", "table", "public.b", "drop", "DROP TABLE public.b"},
                            {"1", "table", "public.a", "alter", "ALTER TABLE public.a"},
                        });
                        scratchrobin::tests::AssertEq(sorted_ops.front().operation_id, "1", "schema compare sort mismatch");
                        auto snapshot_ops = ui.BuildSchemaCompareFromSnapshots(
                            {{"table", "public.a", "CREATE TABLE public.a(id INT);"},
                             {"table", "public.b", "CREATE TABLE public.b(id INT);"}},
                            {{"table", "public.a", "CREATE TABLE public.a(id BIGINT);"},
                             {"table", "public.c", "CREATE TABLE public.c(id INT);"}});
                        scratchrobin::tests::AssertTrue(snapshot_ops.size() == 3U, "snapshot compare op count mismatch");

                        JsonValue payload;
                        payload.type = JsonValue::Type::String;
                        payload.string_value = "x";
                        auto data_cmp = ui.RunDataCompare({{{"1"}, payload}}, {{{"1"}, payload}});
                        scratchrobin::tests::AssertTrue(data_cmp.equal.size() == 1U, "data compare equal mismatch");

                        auto script = ui.BuildMigrationScript(
                            sorted_ops,
                            "2026-02-14T00:00:00Z",
                            "left",
                            "right");
                        scratchrobin::tests::AssertTrue(script.find("script_hash_sha256") != std::string::npos,
                                                        "migration script header missing");
                        std::size_t apply_calls = 0;
                        auto apply_summary = ui.ApplyMigrationScript(script, [&](const std::string& statement) {
                            ++apply_calls;
                            return statement.find("TABLE") != std::string::npos;
                        });
                        scratchrobin::tests::AssertTrue(apply_calls == 2U, "migration apply statement count mismatch");
                        scratchrobin::tests::AssertTrue(apply_summary.find("\"status\":\"ok\"") != std::string::npos,
                                                        "migration apply summary mismatch");

                        auto plan = ui.RenderPlan({{1, -1, "scan", 1, 1.0, ""}, {2, 1, "filter", 1, 1.5, ""}});
                        scratchrobin::tests::AssertTrue(plan.node_count == 2U, "plan node count mismatch");
                        auto layout = ui.RenderPlanLayout({{1, -1, "scan", 1, 1.0, ""},
                                                           {2, 1, "filter", 1, 1.5, ""},
                                                           {3, 1, "sort", 1, 2.5, ""}});
                        scratchrobin::tests::AssertTrue(layout.size() == 3U, "plan layout size mismatch");
                        scratchrobin::tests::AssertTrue(layout[0].depth == 0, "plan root depth mismatch");
                        scratchrobin::tests::AssertTrue(layout[1].depth == 1, "plan child depth mismatch");

                        auto builder = ui.ApplyVisualBuilder(false, true, "select 1", true);
                        scratchrobin::tests::AssertEq(builder.mode, "editable", "builder mode mismatch");
                        auto round_trip = ui.ApplyVisualBuilderWithRoundTrip(
                            false,
                            true,
                            "SELECT 1",
                            [](const std::string& sql) {
                                std::string out = sql;
                                std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
                                    return static_cast<char>(std::tolower(c));
                                });
                                return out;
                            },
                            "select 1");
                        scratchrobin::tests::AssertEq(round_trip.mode, "editable", "builder round-trip mode mismatch");
                        ExpectReject("SRB1-R-5108", [&] {
                            (void)ui.ApplyVisualBuilderWithRoundTrip(
                                false,
                                true,
                                "SELECT 2",
                                [](const std::string& sql) { return sql; },
                                "select 1");
                        });
                        ExpectReject("SRB1-R-5108", [&] {
                            (void)ui.ApplyVisualBuilder(true, true, "", false);
                        });
                    }});

    tests.push_back({"integration/ui_spec_workspace_and_security", [] {
                        auto adapter = BuildConnectedAdapter();
                        project::SpecSetService specset;
                        UiWorkflowService ui(&adapter, &specset);

                        auto gap_summary = ui.BuildSpecWorkspaceGapSummary({
                            {"sb_vnext:A.md", "design", "missing"},
                            {"sb_vnext:B.md", "development", "covered"},
                            {"sb_vnext:C.md", "management", "missing"},
                        });
                        scratchrobin::tests::AssertTrue(gap_summary.find("\"design\":1") != std::string::npos,
                                                        "gap summary design mismatch");
                        scratchrobin::tests::AssertTrue(gap_summary.find("\"management\":1") != std::string::npos,
                                                        "gap summary management mismatch");
                        auto dashboard = ui.BuildSpecWorkspaceDashboard({
                            {"sb_vnext:A.md", "design", "missing"},
                            {"sb_vnext:B.md", "development", "covered"},
                            {"sb_vnext:C.md", "management", "partial"},
                        });
                        scratchrobin::tests::AssertTrue(dashboard.find("\"design\"") != std::string::npos,
                                                        "spec workspace dashboard missing design");
                        auto work_package = ui.ExportSpecWorkspaceWorkPackage(
                            "sb_vnext",
                            {{"sb_vnext:A.md", "design", {"A0-LNT-001"}}},
                            "2026-02-14T00:00:00Z");
                        scratchrobin::tests::AssertTrue(work_package.find("\"set_id\":\"sb_vnext\"") != std::string::npos,
                                                        "work package export missing set id");

                        bool applied = false;
                        ui.ExecuteSecurityPolicyAction(true, "security.manage", [&] { applied = true; });
                        scratchrobin::tests::AssertTrue(applied, "security action should execute");

                        ui.UpsertSecurityPolicy(true, "policy_read", R"({"rule":"allow"})");
                        ui.UpsertSecurityPolicy(true, "policy_admin", R"({"rule":"deny"})");
                        auto listed = ui.ListSecurityPolicyIds(true);
                        scratchrobin::tests::AssertTrue(listed.size() == 2U, "security policy count mismatch");
                        scratchrobin::tests::AssertEq(listed[0], "policy_admin", "security policy ordering mismatch");
                        auto policy = ui.GetSecurityPolicy(true, "policy_read");
                        scratchrobin::tests::AssertTrue(policy.find("allow") != std::string::npos,
                                                        "security policy payload mismatch");
                        ExpectReject("SRB1-R-8301", [&] {
                            ui.UpsertSecurityPolicy(false, "blocked", R"({"rule":"allow"})");
                        });
                        ExpectReject("SRB1-R-8301", [&] {
                            (void)ui.ListSecurityPolicyIds(false);
                        });
                        ui.RemoveSecurityPolicy(true, "policy_read");
                        ExpectReject("SRB1-R-8301", [&] {
                            (void)ui.GetSecurityPolicy(true, "policy_read");
                        });

                        ExpectReject("SRB1-R-8301", [&] {
                            ui.ExecuteSecurityPolicyAction(false, "security.manage", [] {});
                        });
                    }});

    return scratchrobin::tests::RunTests(tests);
}
