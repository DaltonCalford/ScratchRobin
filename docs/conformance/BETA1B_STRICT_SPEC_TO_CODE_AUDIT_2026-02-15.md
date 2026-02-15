# Beta1b Strict Spec-to-Code Audit (2026-02-15)

Superseded by `docs/conformance/BETA1B_REMEDIATION_CLAUSE_CLOSURE_2026-02-15.md`.

Status: Corrective audit. Supersedes high-level claims in `docs/conformance/BETA1B_IMPLEMENTATION_VS_SPEC_AUDIT_2026-02-15.md` where depth was insufficient.

## Audit Basis

- Specification root: `/home/dcalford/CliWork/local_work/docs/specifications_beta1b`
- Implementation root: `ScratchRobin/src`, `ScratchRobin/tests`
- Conformance inventory: `local_work/docs/specifications_beta1b/10_Execution_Tracks_and_Conformance/CONFORMANCE_VECTOR.csv`
- Current build/tests: `ctest --test-dir build --output-on-failure` => `14/14` pass.

## Core Correction

Passing conformance and checklist status (`96/96 IMPLEMENTED`) does **not** equal full spec realization.  
Clause-level comparison shows major under-implementation, especially for diagramming depth and reporting UX/runtime semantics.

## Section 06 Diagramming: Clause-by-Clause Audit

### Source: `06_Diagramming_and_Visual_Modeling/CANVAS_INTERACTION_AND_COMMANDS.md`

| Spec clause | Required | Code evidence | Status |
|---|---|---|---|
| `:7` | select (single/multi) | single selection only (`selected_node_id_`) in `src/gui/diagram_canvas.cpp:284` | **Partial** |
| `:8` | drag move | implemented drag with validation `src/gui/diagram_canvas.cpp:337`, `src/gui/diagram_canvas.cpp:320` | Implemented |
| `:9` | resize | implemented shift-drag + button resize `src/gui/diagram_canvas.cpp:287`, `src/gui/diagram_canvas.cpp:494` | Implemented |
| `:10` | connector creation | implemented `Ctrl+Click` source/target and edge creation `src/gui/diagram_canvas.cpp:258`, `src/gui/diagram_canvas.cpp:523` | Implemented |
| `:11` | keyboard delete and movement | implemented `Delete` and arrow key nudges `src/gui/diagram_canvas.cpp:376` | Implemented |
| `:12` | zoom in/out/reset | implemented buttons/wheel/reset `src/gui/diagram_canvas.cpp:85`, `src/gui/diagram_canvas.cpp:368`, `src/gui/diagram_canvas.cpp:415` | Implemented |
| `:13` | optional snap-to-grid and grid visibility | implemented toggles and behavior `src/gui/main_frame.cpp:526`, `src/gui/diagram_canvas.cpp:184`, `src/gui/diagram_canvas.cpp:360` | Implemented |
| `:17` | mutating actions command-backed and reversible | no undo/redo stack exists; actions mutate directly in-memory `src/gui/diagram_canvas.cpp:161`, `src/gui/diagram_canvas.cpp:479` | **Missing** |
| `:21` | add/remove node reversible | remove only; no add-node operation, no reversibility `src/gui/diagram_canvas.cpp:143` | **Missing** |
| `:22` | add/remove edge reversible | add edge exists; remove edge only as side-effect of node delete; no reversible command stack `src/gui/diagram_canvas.cpp:162`, `src/gui/diagram_canvas.cpp:549` | **Missing** |
| `:23` | move node reversible | move exists, not reversible `src/gui/diagram_canvas.cpp:465` | **Missing** |
| `:24` | resize node reversible | resize exists, not reversible `src/gui/diagram_canvas.cpp:494` | **Missing** |
| `:25` | containment reparenting reversible | no reparent operation implemented | **Missing** |
| `:29` | prevent invalid parent-child cycles | no containment graph/cycle checks; `target_parent_id` ignored in validator `src/core/beta1b_contracts.cpp:1711` | **Missing** |
| `:31` | invalid drop -> `SRB1-R-6201` | generic operation validation exists, but no drop/reparent feature to enforce | **Partial** |
| `:35` | diagram-only delete mode | implemented `DeleteSelectedNode(false, ...)` `src/gui/diagram_canvas.cpp:380` | Implemented |
| `:36-38` | project-level delete requires explicit confirmation/dependency warning | no UI confirmation/warning flow for destructive mode | **Missing** |
| `:42-46` | emit model updates for object state/dirty/invalidation hooks | only status/log text sink; no object-state or invalidation hooks `src/gui/diagram_canvas.cpp:614`, `src/gui/main_frame.cpp:468` | **Missing** |

### Source: `06_Diagramming_and_Visual_Modeling/DIAGRAM_TYPE_AND_NOTATION_CONTRACTS.md`

| Spec clause | Required | Code evidence | Status |
|---|---|---|---|
| `:7-11` | diagram types (`Erd`, `Silverston`, `Whiteboard`, `MindMap`, `DataFlow`) | implemented enum + parse/toString `src/diagram/diagram_services.h:12`, `src/diagram/diagram_services.cpp:21` | Implemented |
| `:17-20` | notation enum support | implemented via lowercase canonical set only (`crowsfoot`, `idef1x`, `uml`, `chen`) `src/core/beta1b_contracts.cpp:1700` | Partial |
| `:24-33` | node minimum includes name/attributes/notes/tags/trace refs/collapse/pin/ghost/stack_count | node struct only has id/type/parent/geometry/datatype `src/core/beta1b_contracts.h:297` | **Missing** |
| `:34-39` | edge minimum includes label/edge_type/directed/identifying/cardinality | edge struct only has id/from/to/relation_type `src/core/beta1b_contracts.h:308` | **Missing** |
| `:43` | notation renderer factory for all ERD notations | one generic renderer path (no notation-specific renderer) in canvas paint `src/gui/diagram_canvas.cpp:177` | **Missing** |
| `:44` | Silverston-specific semantics preserved | no Silverston-specific rendering semantics in canvas | **Missing** |
| `:45` | mind map collapse/expand and descendant counts | no mind map-specific behavior | **Missing** |
| `:46` | whiteboard typed header/detail body support | no whiteboard-specific rendering contract | **Missing** |
| `:50` | `trace_refs` cross-open navigation | only validation helper exists; no trace-ref fields on nodes and no cross-open UI action `src/diagram/diagram_services.h:43` | **Missing** |
| `:52` | unresolvable trace ref -> `SRB1-R-6101` | validator exists `src/diagram/diagram_services.cpp:127` | Implemented |

### Source: `06_Diagramming_and_Visual_Modeling/REVERSE_FORWARD_ENGINEERING_AND_EXPORT.md`

| Spec clause | Required | Code evidence | Status |
|---|---|---|---|
| `:7-10` | reverse engineering from backend catalog + fixture metadata | no reverse-engineer workflow/service API in diagram module | **Missing** |
| `:14` | normalized node/edge output | partial generic model only | Partial |
| `:18-24` | deterministic forward SQL + explicit unsupported handling | mapping + deterministic emission + reject on unmapped type `src/diagram/diagram_services.cpp:135`, `src/core/beta1b_contracts.cpp:1836` | Implemented |
| `:29-34` | migration diff create/alter/drop + dependency-aware order + unsupported alter reject | stable sort and alter reject exist; dependency-aware planner not implemented `src/diagram/diagram_services.cpp:150` | Partial |
| `:40-42` | mandatory export SVG/PNG | implemented `src/core/beta1b_contracts.cpp:1845` | Implemented |
| `:45-47` | PDF optional by profile with reject on unsupported | implemented `src/core/beta1b_contracts.cpp:1848` | Implemented |
| `:51-57` | persistence must preserve type/notation/geometry/attributes/cardinality/collapse/trace refs | persistence preserves type+notation+geometry and basic edges only; missing attributes/cardinality/collapse/trace refs `src/diagram/diagram_services.cpp:72`, `src/core/beta1b_contracts.cpp:1738` | **Partial** |

### Diagramming Verdict

- Contract smoke behavior exists.
- Full section-06 realization is **not complete**.
- Major missing areas: undo/redo contract, containment/reparenting, richer model schema, notation-specific rendering, reverse engineering, trace-ref navigation, project-level delete workflow.

## Section 05 UI Surface Topology vs Implementation

Source: `05_UI_Surface_and_Workflows/WINDOW_AND_MENU_MODEL.md:13`

| Required surface row | Spec required in beta1b | Current implementation | Status |
|---|---|---|---|
| `MainFrame`, `SqlEditorFrame`, `DiagramFrame`, `MonitoringFrame` | Yes | implemented in `src/gui/main_frame.cpp` | Implemented |
| Object/admin manager frames | Yes | implemented as top-level admin windows `src/gui/main_frame.cpp:1538` | Implemented |
| `ReportingFrame` | Yes (baseline partial) | no dedicated frame; reporting menu routes to monitoring `src/gui/main_frame.cpp:149` | **Missing** |
| `SpecWorkspaceFrame` | Yes | implemented `src/gui/main_frame.cpp:1452` | Implemented |
| `GitIntegrationFrame` | Yes (row present) | no dedicated frame/handler in command map `src/gui/main_frame.h:37` | **Missing** |
| `CdcConfigFrame` | Yes (row present) | no dedicated frame/handler in command map `src/gui/main_frame.h:37` | **Missing** |
| `DataMaskingFrame` | Yes (row present) | menu action routed to diagram frame `src/gui/main_frame.cpp:150` | **Missing** |
| Preview optional frames (`Cluster/Replication/ETL`) | Optional by profile | service-level gating exists, no GUI surfaces `src/advanced/advanced_services.cpp:202` | Partial |
| `DockerManagerPanel` / `TestRunnerPanel` | Spec rows present (ga behavior case ids) | service-level gating/open methods, no GUI panels | Partial |

## Section 07 Reporting: Spec vs Implementation

### Source: `07_Reporting_Governance_and_Analytics/REPORTING_OBJECT_MODEL.md`

| Spec clause | Required | Code evidence | Status |
|---|---|---|---|
| `:7-15` | artifact types (Question/Dashboard/Model/Metric/Segment/Alert/Subscription/Collection/Timeline) | generic `ReportingAsset` tuple model only `src/core/beta1b_contracts.h:337` | Partial |
| `:27-33` | question contract includes parameters/visualization config/connection/model refs | not represented in typed reporting API; `RunQuestion` takes SQL string only `src/reporting/reporting_services.h:34` | **Missing** |
| `:36` | repository CRUD + list-by-collection for all artifact types | only export/import/save/load helpers; no CRUD/list-by-collection API `src/reporting/reporting_services.h:43` | **Missing** |

### Source: `07_Reporting_Governance_and_Analytics/REPORTING_RUNTIME_AND_STORAGE_CONTRACT.md`

| Spec clause | Required | Code evidence | Status |
|---|---|---|---|
| `:16-19` | input includes execution context/options | `RunQuestion` has `(question_exists, normalized_sql)` only `src/reporting/reporting_services.h:34` | **Missing** |
| `:20-27` | output includes success flag/query result/timing/cache/error details | output payload from query adapter is minimal (`command_tag`, `rows_affected`) `src/reporting/reporting_services.cpp:141` | **Missing** |
| `:34-37` | cache key + TTL + invalidation model | no explicit cache subsystem API in reporting service | **Missing** |
| `:46-53` | storage init/shutdown/store/retrieve/metadata/retention cleanup | retrieve/metadata/store/retention partially present; explicit init/shutdown API absent `src/reporting/reporting_services.h:30` | Partial |
| `:58-63` | scheduler task runtime + channels | RRULE functions exist; no channelized scheduler task engine | Partial |
| `:66-129` | canonical RRULE + deterministic rejects | implemented in contracts `src/core/beta1b_contracts.cpp:2026` | Implemented |

### Source: `07_Reporting_Governance_and_Analytics/REPORTING_EXECUTION_PLAYBOOK.md`

| Case clause | Required | Code evidence | Status |
|---|---|---|---|
| `RPT-001` `:13` | concrete question execution results (non-placeholder) | contract-level pass; payload shape still minimal | Partial |
| `RPT-002` `:14` | dashboard runtime fully resolves widgets/datasets | current runtime returns widget status array only `src/core/beta1b_contracts.cpp:2407` | **Missing** |
| `RPT-003` `:15` | retrieve + metadata complete deterministic | exists for key/payload bytes `src/reporting/reporting_services.cpp:165` | Implemented (narrow) |
| `RPT-004` `:16` | repository export/import fidelity | exists for generic assets, not full artifact type fidelity | Partial |
| `RPT-009` `:21` | activity dashboard metrics/refresh runtime | window/filter/export service exists `src/reporting/reporting_services.cpp:247` | Implemented |
| `RPT-010` `:22` | deterministic export + retention | implemented `src/reporting/reporting_services.cpp:279`, `src/reporting/reporting_services.cpp:283` | Implemented |

### Reporting Verdict

- Reporting backend contracts are partially implemented.
- Metabase-style product surface (question builder, tuple table rendering, chart visualization, dashboard composition) is **not implemented**.
- Current GUI exposes monitoring metrics, not full reporting UX.

## Conformance Adequacy Finding

The conformance harness validates many contract functions directly, but several checks are too shallow for product-level completeness:

- `RPT-001` checks only that `RunQuestion` returns without reject on a stub lambda: `tests/conformance/beta1b_conformance_vector.cpp:369`
- `RPT-002` checks only `RunDashboardRuntime("db", {{"w1","ok"}}, true)`: `tests/conformance/beta1b_conformance_vector.cpp:373`
- `D1-CAN-001` checks only `ValidateCanvasOperation(... "drag" ...)`, not full canvas contract (undo/reparent/delete modes): `tests/conformance/beta1b_conformance_vector.cpp:366`

## Corrected QA Verdict

- Contract-level QA (current tests): **Pass**.
- Full spec-realization QA (end-user capability parity): **Fail**.

## Minimum Required Remediation Before Claiming “Implemented in Full”

1. Implement full `ReportingFrame` workflows (question authoring, execution context, tuple results, visualization config, chart rendering, dashboard composition).
2. Expand reporting runtime outputs to include full query result contract (timing/cache/error metadata).
3. Implement reporting repository CRUD + list-by-collection across all artifact types.
4. Implement missing diagramming contract areas:
- undo/redo command stack
- containment/reparenting + cycle enforcement
- project-level delete confirmation/dependency flow
- richer node/edge schema fields
- notation-specific renderer behavior (including Silverston details)
- reverse engineering workflow
5. Add missing UI surfaces or explicitly disable/profile-gate with matching spec and conformance updates (`ReportingFrame`, `DataMaskingFrame`, `CdcConfigFrame`, `GitIntegrationFrame`).
