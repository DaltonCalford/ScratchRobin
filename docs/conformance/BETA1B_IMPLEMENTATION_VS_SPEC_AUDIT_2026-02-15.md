# Beta1b Implementation vs Specification Audit (2026-02-15)

Superseded by `docs/conformance/BETA1B_REMEDIATION_CLAUSE_CLOSURE_2026-02-15.md`.

Status: Authoritative audit snapshot for `ScratchRobin` implementation parity against `specifications_beta1b`.

## Scope

- Spec corpus audited: `/home/dcalford/CliWork/local_work/docs/specifications_beta1b`
- Implementation audited: `ScratchRobin/src`, `ScratchRobin/tests`, `ScratchRobin/CMakeLists.txt`
- Conformance inventory: `local_work/docs/specifications_beta1b/10_Execution_Tracks_and_Conformance/CONFORMANCE_VECTOR.csv`

## Method

1. Enumerated authoritative conformance cases and phase files.
2. Cross-checked requirements against concrete code paths and UI surfaces.
3. Verified test/conformance status from current build output.
4. Flagged mismatches where implementation behavior does not satisfy normative spec semantics.

## Quantitative Snapshot

- Conformance cases in vector: `96`
- Conformance checks in runtime harness: `96` (1:1 with case ids)
- Checklist status rows marked `IMPLEMENTED`: `96/96`
- Current test status: `14/14` passing (`ctest --test-dir build --output-on-failure`)
- Normative files in spec set (`Status: Normative`): `64`

## Executive Verdict

Conformance harness is green, but full spec parity is **not complete** for QA intended to validate end-user behavior.

Primary blocker: reporting/query-analytics UX and runtime semantics are under-implemented relative to normative specification intent (Metabase-style query -> tuples -> visualization -> dashboard workflows).

## Phase Matrix

| Phase | Spec Intent | Code/Test State | Audit Status |
|---|---|---|---|
| `00` Governance | Reject/audit invariants | Implemented in contracts + tests | Pass |
| `01` Scope/Goals | Product scope baseline | Documentation only (non-runtime) | Informational |
| `02` Runtime | Startup/process/thread model | Implemented + tested | Pass |
| `03` Project/Data model | Binary/schema/specset contracts | Implemented + tested | Pass |
| `04` Backend/connection | Mode routing/capability/enterprise paths | Implemented + tested | Pass |
| `05` UI surfaces/workflows | Full surface topology + workflow UX | Core shell implemented; several required surfaces absent/aliased | **Partial** |
| `06` Diagramming | Canvas + model + export | Interactive canvas now implemented | Pass |
| `07` Reporting/governance | Query/dash runtime + storage + scheduling + artifacts | Runtime contract subset implemented; reporting UX/semantics incomplete | **Partial** |
| `08` Advanced/integrations | CDC/masking/collab/ext/optional surfaces | Service-level contracts present; UI-level surface coverage incomplete | **Partial** |
| `09` Build/test/packaging | Build/profile/package gates | Implemented + tested | Pass |
| `10` Conformance/release gates | Case binding + alpha preservation gates | Implemented + passing | Pass |
| `11` Alpha preservation | Deep mirror continuity | Implemented + gated | Pass |

## High-Severity Gaps (Spec vs Code)

### 1) `ReportingFrame` surface is not implemented as a distinct reporting UI

- Spec requires `ReportingFrame` in mandatory top-level surfaces (baseline partial, required in beta1b): `local_work/docs/specifications_beta1b/05_UI_Surface_and_Workflows/WINDOW_AND_MENU_MODEL.md:25`
- Tools menu contract includes reporting entrypoint: `local_work/docs/specifications_beta1b/05_UI_Surface_and_Workflows/WINDOW_AND_MENU_MODEL.md:91`
- Code routes `open_reporting` to monitoring surface, not reporting frame: `src/gui/main_frame.cpp:148`, `src/gui/main_frame.cpp:149`
- Monitoring tab is metric list only: `src/gui/main_frame.cpp:590`

Impact: reporting UX required by spec is not available to QA users as a dedicated feature surface.

### 2) Reporting query execution output shape is below normative contract

- Spec output requires success flag + `QueryResult` + timing metadata + cache metadata + error details: `local_work/docs/specifications_beta1b/07_Reporting_Governance_and_Analytics/REPORTING_RUNTIME_AND_STORAGE_CONTRACT.md:20`
- Current `RunQuestion` result payload only emits `command_tag` and `rows_affected`: `src/reporting/reporting_services.cpp:141`, `src/reporting/reporting_services.cpp:143`

Impact: insufficient data contract for QA of report result semantics and visualization pipelines.

### 3) Dashboard execution does not resolve widget datasets

- Spec requires full dashboard runtime resolving widgets and datasets: `local_work/docs/specifications_beta1b/07_Reporting_Governance_and_Analytics/REPORTING_EXECUTION_PLAYBOOK.md:14`
- Current runtime returns sorted widget statuses + cache flag only: `src/core/beta1b_contracts.cpp:2397`, `src/core/beta1b_contracts.cpp:2415`

Impact: behavior is contract-smoke level, not product-level dashboard execution.

### 4) Reporting repository contract not implemented as CRUD + list by collection for all artifact types

- Artifact types include `Question`, `Dashboard`, `Model`, `Metric`, `Segment`, `Alert`, `Subscription`, `Collection`, `Timeline`: `local_work/docs/specifications_beta1b/07_Reporting_Governance_and_Analytics/REPORTING_OBJECT_MODEL.md:7`
- Repository contract requires CRUD + list-by-collection for all artifact types: `local_work/docs/specifications_beta1b/07_Reporting_Governance_and_Analytics/REPORTING_OBJECT_MODEL.md:36`
- Current service exposes export/import/save/load, but no explicit CRUD/list-by-collection API: `src/reporting/reporting_services.h:43`, `src/reporting/reporting_services.h:46`

Impact: repository behavior is not full relative to normative object model.

### 5) Visualization/graph capability implied by reporting object model is not implemented in UI

- Question contract includes visualization config: `local_work/docs/specifications_beta1b/07_Reporting_Governance_and_Analytics/REPORTING_OBJECT_MODEL.md:29`
- No reporting chart/graph rendering surface exists in current GUI implementation; reporting path is aliased to monitoring list (`src/gui/main_frame.cpp:149`, `src/gui/main_frame.cpp:598`)

Impact: no end-user graph/tuple reporting workflow for QA.

## Medium-Severity Gaps

### 6) Data Masking UI entrypoint is misrouted to diagram surface

- Tools menu declares data masking action: `src/core/beta1b_contracts.cpp:1633`
- Code maps it to diagram frame: `src/gui/main_frame.cpp:150`, `src/gui/main_frame.cpp:151`
- Spec expects dedicated `DataMaskingFrame` surface (partial but required): `local_work/docs/specifications_beta1b/05_UI_Surface_and_Workflows/WINDOW_AND_MENU_MODEL.md:29`

### 7) Required/expected UI surfaces missing from GUI layer

- Spec surface matrix includes `GitIntegrationFrame`, `CdcConfigFrame`, `DataMaskingFrame`, `ReportingFrame`: `local_work/docs/specifications_beta1b/05_UI_Surface_and_Workflows/WINDOW_AND_MENU_MODEL.md:25`
- Current GUI command/surface model has no dedicated command IDs/handlers for those frames: `src/gui/main_frame.h:33`
- Service-level logic exists in advanced modules, but no full GUI workflows for these surfaces.

### 8) Conformance depth mismatch (contract checks are shallow for reporting)

- Reporting conformance checks validate minimal contract invocations, not full UX/data semantics: `tests/conformance/beta1b_conformance_vector.cpp:369`, `tests/conformance/beta1b_conformance_vector.cpp:373`, `tests/conformance/beta1b_conformance_vector.cpp:396`

Impact: green conformance does not guarantee user-visible reporting completeness.

## Conformance Inventory Integrity Notes

- `CONFORMANCE_VECTOR.csv` still records many baselines as `Partial|Planned|Stub` for historical origin values: `local_work/docs/specifications_beta1b/10_Execution_Tracks_and_Conformance/CONFORMANCE_VECTOR.csv:54`
- Implementation checklist marks all rows implemented: `docs/conformance/CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv:1`

Interpretation: vector baseline fields are historical provenance, not current completion truth. QA should rely on audited behavior plus implementation evidence, not baseline-origin labels alone.

## QA Readiness Verdict

### For contract-level/engine-service validation

- **Ready** (tests and conformance harness pass).

### For full product QA against beta1b reporting and advanced UI intent

- **Not ready** due to missing/aliased reporting and related surface workflows.

## Required Closure Before "Full Spec Complete" Claim

1. Implement dedicated `ReportingFrame` with query builder/native SQL question workflow.
2. Implement tuple result grid + visualization renderers (at minimum table + graph types) driven by question config.
3. Expand dashboard runtime to execute/query widgets, not status-only payloads.
4. Add reporting repository CRUD/list-by-collection for all artifact types in the object model.
5. Add dedicated GUI surfaces/handlers for `DataMaskingFrame`, `CdcConfigFrame`, and `GitIntegrationFrame` (or explicitly disable with profile-gated policy and update spec/conformance accordingly).
6. Strengthen conformance cases for `RPT-001..RPT-004` to enforce full output semantics and non-placeholder UX paths.
