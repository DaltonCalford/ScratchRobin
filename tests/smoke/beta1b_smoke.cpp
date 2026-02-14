#include "core/beta1b_contracts.h"
#include "phases/phase_registry.h"
#include "tests/test_harness.h"

using namespace scratchrobin;
using namespace scratchrobin::beta1b;

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"smoke/phase_registry", [] {
                        const auto phases = scratchrobin::phases::BuildPhaseScaffold();
                        scratchrobin::tests::AssertTrue(phases.size() == 11U, "expected 11 phases");
                        scratchrobin::tests::AssertEq(phases.front().phase_id, "00", "phase 00 missing");
                        scratchrobin::tests::AssertEq(phases.back().phase_id, "10", "phase 10 missing");
                    }});

    tests.push_back({"smoke/tools_menu", [] {
                        const auto tools = BuildToolsMenu();
                        bool found = false;
                        for (const auto& item : tools) {
                            if (item.first == "Spec Workspace") {
                                found = true;
                                break;
                            }
                        }
                        scratchrobin::tests::AssertTrue(found, "Spec Workspace menu missing");
                    }});

    tests.push_back({"smoke/profile_gating", [] {
                        auto preview = RegisterOptionalSurfaces("preview");
                        auto ga = RegisterOptionalSurfaces("ga");
                        scratchrobin::tests::AssertTrue(!preview["ClusterManagerFrame"].has_value(),
                                                        "preview should enable cluster manager");
                        scratchrobin::tests::AssertTrue(ga["ClusterManagerFrame"].has_value(),
                                                        "ga should gate cluster manager");
                    }});

    return scratchrobin::tests::RunTests(tests);
}
