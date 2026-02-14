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
                    }});

    tests.push_back({"integration/advanced_profile_and_integrations", [] {
                        AdvancedService svc;

                        auto preview_surfaces = svc.RegisterOptionalSurfaces("preview");
                        scratchrobin::tests::AssertTrue(!preview_surfaces["ClusterManagerFrame"].has_value(),
                                                        "preview cluster manager should be enabled");

                        auto ga_surfaces = svc.RegisterOptionalSurfaces("ga");
                        scratchrobin::tests::AssertEq(*ga_surfaces["ClusterManagerFrame"], "SRB1-R-7008",
                                                      "ga cluster manager reject mismatch");

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
