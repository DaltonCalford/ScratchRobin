# Implementation Progress Tracking Protocol (Mandatory)

## Scope

This protocol is mandatory for all beta1b implementation work mapped to the `P0/P1/P2` queue.

## Tracker files

- Queue tracker (current status): `docs/conformance/BETA1B_IMPLEMENTATION_PROGRESS_TRACKER.csv`
- Append-only step log (audit trail): `docs/conformance/BETA1B_IMPLEMENTATION_STEP_LOG.csv`

## Required update points

For every implementation step, update progress using the script before moving to the next step:

1. Step start (`IN_PROGRESS`).
2. Mid-step milestone (`IN_PROGRESS` + increased percent).
3. Step blocked (`BLOCKED` + `--blocked-by` reason).
4. Step completion (`DONE` + `--percent 100`).

## Mandatory command

Run from repository root:

```bash
python3 tools/update_progress_tracker.py \
  --queue-id P0-03 \
  --status IN_PROGRESS \
  --percent 25 \
  --step-title "Implement schema validator for project payload" \
  --step-detail "Replaced shallow key checks with strict nested validation" \
  --files-touched "src/core/beta1b_contracts.cpp|src/project/project_services.cpp" \
  --tests-run "scratchrobin_tests_conformance" \
  --test-result PASS \
  --evidence-ref "tests/conformance/beta1b_conformance_vector.cpp"
```

## Commit gate

Use the check script to verify tracker updates are present for implementation changes:

```bash
./tools/check_progress_tracker.sh --staged
```

Optional pre-commit hook installer:

```bash
./tools/install_progress_hook.sh
```

## Enforcement rules

- Any commit touching implementation surfaces (`src/`, `tests/`, `CMakeLists.txt`) must include tracker updates.
- Progress percent must be monotonic non-decreasing per queue ID, except when status becomes `BLOCKED` and percent remains unchanged.
- `DONE` requires `percent_complete=100`.
- The step log is append-only; do not edit prior rows except to correct clear data-entry mistakes in the immediately previous row.
