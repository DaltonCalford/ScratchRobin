# Conformance-First Implementation Checklist

Generated from beta1b `CONFORMANCE_VECTOR.csv` and mapped to local phase stubs (`00->10`).

## Summary

- Total cases: `96`
- Implementation status:
  - `Implemented`: `96`
  - `Partial`: `0`
  - `Planned`: `0`
  - `Stub`: `0`
- Priority counts:
  - `Critical`: `26`
  - `High`: `55`
  - `Medium`: `15`
  - `Low`: `0`
- Phase distribution:
  - `Phase 00` (Governance and Scaffolding): `4`
  - `Phase 03` (Project and Data Model): `5`
  - `Phase 04` (Backend Adapter and Connection): `9`
  - `Phase 05` (UI Surface and Workflows): `23`
  - `Phase 06` (Diagramming and Visual Modeling): `5`
  - `Phase 07` (Reporting and Governance Integration): `10`
  - `Phase 08` (Advanced Features and Integrations): `15`
  - `Phase 09` (Build/Test/Packaging): `10`
  - `Phase 10` (Conformance/Alpha/Release Gates): `15`

## Evidence

- Full case-ID runtime coverage: `tests/conformance/beta1b_conformance_vector.cpp`
- Contract runtime implementation: `src/core/beta1b_contracts.cpp`
- Unit coverage for contract paths: `tests/unit/beta1b_contracts_unit.cpp`
- Smoke and integration checks: `tests/smoke/beta1b_smoke.cpp`, `tests/integration/beta1b_integration.cpp`

## Test Gate

`ctest --test-dir build --output-on-failure` must pass all of:
- `scratchrobin_tests_unit`
- `scratchrobin_tests_smoke`
- `scratchrobin_tests_integration`
- `scratchrobin_tests_conformance`

## Full Data

Use `docs/conformance/CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv` as the authoritative checklist table for implementation tracking.

## Progress Tracking (Mandatory)

- Queue tracker: `docs/conformance/BETA1B_IMPLEMENTATION_PROGRESS_TRACKER.csv`
- Append-only step log: `docs/conformance/BETA1B_IMPLEMENTATION_STEP_LOG.csv`
- Protocol: `docs/conformance/IMPLEMENTATION_PROGRESS_TRACKING_PROTOCOL.md`

Every implementation step must update tracker + step log using:

`python3 tools/update_progress_tracker.py ...`

Commit-time validation command:

`./tools/check_progress_tracker.sh --staged`

Optional hook installer:

`./tools/install_progress_hook.sh`
