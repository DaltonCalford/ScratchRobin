# ScratchRobin (Beta Implementation)

ScratchRobin is now in Beta implementation.

All Alpha specification goals have been met, and the first Beta specification set now has a contract-complete runtime with case-ID conformance coverage.

Authoritative specification source:
- /home/dcalford/CliWork/local_work/docs/specifications_beta1b/

Legacy implementation snapshot preserved at:
- legacy/2026-02-14_pre_beta1b_reset/

Implementation intent:
1. Remove implementation drift from prior alpha work.
2. Implement deterministically from beta1b contracts and conformance rows.
3. Keep legacy material available for reference only.

Conformance evidence:
- `docs/conformance/CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv`
- `tests/conformance/beta1b_conformance_vector.cpp`

## Clone Resync Required

Repository history was rewritten to remove build/test-build artifacts.

All local clones should re-sync with rewritten history:
- `git fetch origin && git reset --hard origin/main`
- or use a fresh clone.
