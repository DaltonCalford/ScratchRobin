#include "release/release_conformance_services.h"
#include "tests/test_harness.h"

#include <filesystem>
#include <fstream>

using namespace scratchrobin;
using namespace scratchrobin::release;

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

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/release_blocker_register_and_gates", [] {
                        namespace fs = std::filesystem;
                        ReleaseConformanceService svc;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_release_gates";
                        fs::remove_all(temp);

                        WriteTextFile(temp / "BLOCKER_REGISTER.csv",
                                      "blocker_id,severity,status,source_type,source_id,opened_at,updated_at,owner,summary\n"
                                      "BLK-0001,P0,open,conformance_case,A0-LNT-001,2026-02-14T00:00:00Z,2026-02-14T00:00:00Z,owner,critical blocker\n"
                                      "BLK-0002,P1,mitigated,reject_code,SRB1-R-9002,2026-02-14T00:00:00Z,2026-02-14T00:00:00Z,owner,high blocker\n"
                                      "BLK-0003,P2,waived,manual,TKT-1,2026-02-14T00:00:00Z,2026-02-14T00:00:00Z,owner,preview-only waiver\n");

                        auto rows = svc.LoadBlockerRegister((temp / "BLOCKER_REGISTER.csv").string());
                        scratchrobin::tests::AssertTrue(rows.size() == 3U, "blocker register row count mismatch");
                        svc.ValidateBlockerRegister(rows);

                        auto phase_gate = svc.EvaluatePhaseAcceptance(rows);
                        scratchrobin::tests::AssertTrue(!phase_gate.pass, "phase gate should fail on P0 open");
                        scratchrobin::tests::AssertTrue(phase_gate.blocking_blocker_ids.size() == 1U,
                                                        "phase gate blocking ids mismatch");

                        auto rc_gate = svc.EvaluateRcEntry(rows);
                        scratchrobin::tests::AssertTrue(!rc_gate.pass, "rc gate should fail on unresolved P0/P1");
                        scratchrobin::tests::AssertTrue(rc_gate.blocking_blocker_ids.size() == 2U,
                                                        "rc gate blocking ids mismatch");

                        rows[0].status = "closed";
                        rows[1].status = "closed";
                        phase_gate = svc.EvaluatePhaseAcceptance(rows);
                        rc_gate = svc.EvaluateRcEntry(rows);
                        scratchrobin::tests::AssertTrue(phase_gate.pass, "phase gate should pass after closure");
                        scratchrobin::tests::AssertTrue(rc_gate.pass, "rc gate should pass after closure");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/release_alpha_preservation_wrappers", [] {
                        namespace fs = std::filesystem;
                        ReleaseConformanceService svc;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_release_alpha";
                        fs::remove_all(temp);

                        WriteTextFile(temp / "alpha/deep/a.txt", "alpha");
                        std::vector<beta1b::AlphaMirrorEntry> entries = {
                            {"alpha/deep/a.txt", 5,
                             "8ed3f6ad685b959ead7022518e1af76cd816f8e8ec7ccdda1ed4018e8f2223f8"},
                        };

                        svc.ValidateAlphaMirrorPresence(temp.string(), entries);
                        svc.ValidateAlphaMirrorHashes(temp.string(), entries);

                        ExpectReject("SRB1-R-5501", [&] {
                            svc.ValidateAlphaMirrorPresence(temp.string(), {{"missing/file.txt", 1, ""}});
                        });
                        ExpectReject("SRB1-R-5502", [&] {
                            svc.ValidateAlphaMirrorHashes(temp.string(), {{"alpha/deep/a.txt", 4, entries[0].expected_sha256}});
                        });

                        svc.ValidateSilverstonContinuity({"silverston/erd_core.md"}, {"silverston/erd_core.md"});
                        ExpectReject("SRB1-R-5503", [&] {
                            svc.ValidateSilverstonContinuity({}, {"silverston/erd_core.md"});
                        });

                        svc.ValidateAlphaInventoryMapping({"EL1"}, {{"a.txt", "EL1"}});
                        ExpectReject("SRB1-R-5504", [&] {
                            svc.ValidateAlphaInventoryMapping({"EL1"}, {{"a.txt", "EL2"}});
                        });

                        svc.ValidateAlphaExtractionGate(true, true, true);
                        ExpectReject("SRB1-R-5505", [&] {
                            svc.ValidateAlphaExtractionGate(true, true, false);
                        });

                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}
