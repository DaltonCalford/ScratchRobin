#include "advanced/advanced_services.h"
#include "tests/test_harness.h"

using namespace scratchrobin;
using namespace scratchrobin::advanced;

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

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/advanced_cdc_and_masking", [] {
                        AdvancedService svc;

                        int attempts = 0;
                        int dead_letter_count = 0;
                        auto status = svc.RunCdcEvent(
                            "event_a",
                            3,
                            50,
                            [&](const std::string&) {
                                ++attempts;
                                return attempts == 2;
                            },
                            [&](const std::string&) { ++dead_letter_count; });
                        scratchrobin::tests::AssertEq(status, "published", "cdc publish status mismatch");
                        scratchrobin::tests::AssertTrue(dead_letter_count == 0, "dead letter should remain empty");

                        attempts = 0;
                        ExpectReject("SRB1-R-7004", [&] {
                            (void)svc.RunCdcEvent(
                                "event_b",
                                2,
                                50,
                                [&](const std::string&) {
                                    ++attempts;
                                    return false;
                                },
                                [&](const std::string&) { ++dead_letter_count; });
                        });
                        scratchrobin::tests::AssertTrue(dead_letter_count == 1, "dead letter should capture failed event");

                        auto masked = svc.PreviewMask(
                            {{{"email", "person@example.com"}, {"ssn", "123456789"}}},
                            {{"email", "redact"}, {"ssn", "prefix_mask"}});
                        scratchrobin::tests::AssertEq(masked[0].at("email"), "***", "redact rule mismatch");
                        scratchrobin::tests::AssertEq(masked[0].at("ssn"), "12*******", "prefix mask rule mismatch");

                        svc.UpsertMaskingProfile("profile_default", {{"email", "redact"}});
                        auto profile_masked = svc.PreviewMaskWithProfile(
                            "profile_default",
                            {{{"email", "person@example.com"}, {"city", "Austin"}}});
                        scratchrobin::tests::AssertEq(profile_masked[0].at("email"), "***",
                                                      "profile-based mask mismatch");
                        ExpectReject("SRB1-R-7005", [&] {
                            (void)svc.PreviewMaskWithProfile("missing_profile", {{{"email", "a@x"}}});
                        });

                        auto batch = svc.RunCdcBatch(
                            {"evt1", "evt2"},
                            1,
                            10,
                            [&](const std::string& payload) { return payload == "evt1"; });
                        scratchrobin::tests::AssertTrue(batch.published == 1U, "cdc batch publish count mismatch");
                        scratchrobin::tests::AssertTrue(batch.dead_lettered == 1U, "cdc batch dead-letter count mismatch");
                        scratchrobin::tests::AssertTrue(!svc.DeadLetterQueue().empty(), "dead letter queue should not be empty");

                        ExpectReject("SRB1-R-7005", [&] {
                            (void)svc.PreviewMask({{{"v", "x"}}}, {{"v", "unsupported"}});
                        });
                    }});

    tests.push_back({"integration/advanced_review_extension_and_lineage", [] {
                        AdvancedService svc;

                        svc.EnforceReviewPolicy(2, 2, "apply_changes", "Approved");
                        ExpectReject("SRB1-R-7301", [&] {
                            svc.EnforceReviewPolicy(1, 2, "apply_changes", "Approved");
                        });
                        ExpectReject("SRB1-R-7305", [&] {
                            svc.EnforceReviewPolicy(2, 2, "apply_changes", "Draft");
                        });

                        svc.ValidateExtensionRuntime(true, true, {"read_catalog"}, {"read_catalog", "read_data"});
                        ExpectReject("SRB1-R-7303", [&] {
                            svc.ValidateExtensionRuntime(false, true, {"read_catalog"}, {"read_catalog"});
                        });
                        ExpectReject("SRB1-R-7304", [&] {
                            svc.ValidateExtensionRuntime(true, true, {"execute_os"}, {"read_catalog"});
                        });

                        auto lineage = svc.BuildLineage(
                            {"b", "a"},
                            {{"a", std::optional<std::string>("b")}, {"b", std::nullopt}});
                        scratchrobin::tests::AssertEq(lineage.first.front(), "a", "lineage node sort mismatch");
                        scratchrobin::tests::AssertTrue(lineage.second == 1, "lineage unresolved count mismatch");

                        svc.CreateReviewAction("apply_patch", "Approved");
                        svc.ApproveReviewAction("apply_patch", "alice");
                        svc.ApproveReviewAction("apply_patch", "bob");
                        svc.EnforceReviewAction("apply_patch", 2);
                        ExpectReject("SRB1-R-7301", [&] {
                            svc.EnforceReviewAction("missing_action", 1);
                        });

                        svc.RegisterExtensionPackage(
                            "ext.pkg",
                            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
                            "scratchrobin-beta1b",
                            {"read_catalog", "read_data"});
                        svc.ExecuteExtensionPackage("ext.pkg", {"read_catalog"}, {"read_catalog", "read_data"});
                        ExpectReject("SRB1-R-7304", [&] {
                            svc.ExecuteExtensionPackage("ext.pkg", {"read_data"}, {"read_catalog"});
                        });

                        auto depth_rows = svc.BuildLineageDepth(
                            {"root", "child", "orphan"},
                            {{"child", std::optional<std::string>("root")},
                             {"orphan", std::nullopt}});
                        scratchrobin::tests::AssertTrue(!depth_rows.empty(), "lineage depth rows should not be empty");
                    }});

    tests.push_back({"integration/advanced_profile_and_integrations", [] {
                        AdvancedService svc;

                        auto preview_surfaces = svc.RegisterOptionalSurfaces("preview");
                        scratchrobin::tests::AssertTrue(!preview_surfaces["ClusterManagerFrame"].has_value(),
                                                        "preview cluster manager should be enabled");
                        auto cluster_payload = svc.OpenClusterManager("preview", "cluster_main");
                        scratchrobin::tests::AssertTrue(cluster_payload.find("cluster_main") != std::string::npos,
                                                        "cluster manager payload mismatch");
                        auto repl_payload = svc.OpenReplicationManager("preview", "repl_a");
                        scratchrobin::tests::AssertTrue(repl_payload.find("repl_a") != std::string::npos,
                                                        "replication manager payload mismatch");
                        auto etl_payload = svc.OpenEtlManager("preview", "job_a");
                        scratchrobin::tests::AssertTrue(etl_payload.find("job_a") != std::string::npos,
                                                        "etl manager payload mismatch");
                        auto docker_payload = svc.OpenDockerManager("preview", "ps");
                        scratchrobin::tests::AssertTrue(docker_payload.find("\"operation\":\"ps\"") != std::string::npos,
                                                        "docker manager payload mismatch");
                        auto test_payload = svc.OpenTestRunner("preview", "suite_a");
                        scratchrobin::tests::AssertTrue(test_payload.find("suite_a") != std::string::npos,
                                                        "test runner payload mismatch");

                        auto ga_surfaces = svc.RegisterOptionalSurfaces("ga");
                        scratchrobin::tests::AssertEq(*ga_surfaces["ClusterManagerFrame"], "SRB1-R-7008",
                                                      "ga cluster manager reject mismatch");
                        ExpectReject("SRB1-R-7008", [&] {
                            (void)svc.OpenClusterManager("ga", "cluster_main");
                        });

                        svc.ValidateAiProviderConfig("openai", true, "gpt-5", std::optional<std::string>("cred"));
                        ExpectReject("SRB1-R-7006", [&] {
                            svc.ValidateAiProviderConfig("openai", true, "gpt-5", std::nullopt);
                        });

                        svc.ValidateIssueTrackerConfig("github", "org/repo", std::optional<std::string>("cred"));
                        ExpectReject("SRB1-R-7007", [&] {
                            svc.ValidateIssueTrackerConfig("unknown", "org/repo", std::optional<std::string>("cred"));
                        });

                        svc.ValidateGitSyncState(true, true, true);
                        ExpectReject("SRB1-R-8201", [&] {
                            svc.ValidateGitSyncState(true, true, false);
                        });
                    }});

    return scratchrobin::tests::RunTests(tests);
}
