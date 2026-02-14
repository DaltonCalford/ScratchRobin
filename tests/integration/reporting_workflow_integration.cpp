#include "reporting/reporting_services.h"
#include "tests/test_harness.h"

using namespace scratchrobin;
using namespace scratchrobin::reporting;

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

    tests.push_back({"integration/reporting_question_dashboard_storage", [] {
                        auto adapter = BuildConnectedAdapter();
                        ReportingService svc(&adapter);

                        auto question = svc.RunQuestion(true, "select 1");
                        scratchrobin::tests::AssertTrue(question.find("EXECUTE") != std::string::npos, "question result mismatch");
                        auto stored = svc.RetrieveResult("question:select 1");
                        scratchrobin::tests::AssertTrue(stored.find("rows_affected") != std::string::npos, "stored payload missing");
                        auto md = svc.QueryResultMetadata("question:select 1");
                        scratchrobin::tests::AssertTrue(md.exists && md.bytes > 0U, "metadata mismatch");

                        auto dash = svc.RunDashboard("dash_1", {{"w1", "ok"}, {"w2", "ok"}}, false);
                        scratchrobin::tests::AssertTrue(dash.find("\"dashboard_id\":\"dash_1\"") != std::string::npos,
                                                        "dashboard payload mismatch");

                        ExpectReject("SRB1-R-7001", [&] { (void)svc.RunQuestion(false, "select 1"); });
                        ExpectReject("SRB1-R-7002", [&] { (void)svc.RetrieveResult("missing"); });
                    }});

    tests.push_back({"integration/reporting_repository_and_rrule", [] {
                        auto adapter = BuildConnectedAdapter();
                        ReportingService svc(&adapter);

                        std::vector<beta1b::ReportingAsset> assets = {
                            {"b", "dashboard", "db", "{}"},
                            {"a", "question", "q", "{}"},
                        };
                        auto payload = svc.ExportRepository(assets);
                        auto imported = svc.ImportRepository(payload);
                        scratchrobin::tests::AssertTrue(imported.size() == 2U, "imported size mismatch");

                        auto rule = svc.CanonicalizeSchedule({{"FREQ", "DAILY"}, {"INTERVAL", "1"}});
                        scratchrobin::tests::AssertEq(rule, "FREQ=DAILY;INTERVAL=1", "rrule canonical mismatch");

                        beta1b::ReportingSchedule schedule;
                        schedule.schedule_spec = "FREQ=DAILY;INTERVAL=1";
                        schedule.schedule_dtstart_local = "2026-02-14T00:00:00";
                        schedule.timezone = "UTC";
                        auto next = svc.NextRun(schedule, "2026-02-14T00:00:01Z");
                        scratchrobin::tests::AssertEq(next, "2026-02-15T00:00:00Z", "next run mismatch");

                        auto expanded = svc.ExpandSchedule(schedule, "2026-02-14T00:00:01Z", 8);
                        scratchrobin::tests::AssertTrue(!expanded.empty(), "schedule expansion should produce candidates");
                    }});

    tests.push_back({"integration/reporting_activity_dashboard", [] {
                        auto adapter = BuildConnectedAdapter();
                        ReportingService svc(&adapter);

                        std::vector<beta1b::ActivityRow> rows = {
                            {"2026-02-14T00:00:00Z", "reads", 1.0},
                            {"2026-02-14T00:00:05Z", "writes", 2.0},
                            {"2026-02-13T23:59:00Z", "reads", 0.5},
                        };
                        auto filtered = svc.RunActivityQuery(rows, "5m", {"reads", "writes"});
                        scratchrobin::tests::AssertTrue(filtered.size() == 3U, "activity query size mismatch");

                        auto csv = svc.ExportActivity(filtered, "csv");
                        scratchrobin::tests::AssertTrue(csv.find("timestamp_utc") != std::string::npos, "csv export missing header");
                        auto json = svc.ExportActivity(filtered, "json");
                        scratchrobin::tests::AssertTrue(json.find("\"metric_key\":\"reads\"") != std::string::npos,
                                                        "json export missing metrics");

                        auto cleanup = svc.RetentionCleanup(rows, "2026-02-14T00:00:00Z");
                        scratchrobin::tests::AssertTrue(cleanup.first.size() == 2U, "retention keep size mismatch");
                        scratchrobin::tests::AssertTrue(cleanup.second == 1U, "retention dropped mismatch");

                        ExpectReject("SRB1-R-7202", [&] { (void)svc.RunActivityQuery(rows, "2m", {"reads"}); });
                        ExpectReject("SRB1-R-7202", [&] { (void)svc.ExportActivity(filtered, "xml"); });
                    }});

    return scratchrobin::tests::RunTests(tests);
}

