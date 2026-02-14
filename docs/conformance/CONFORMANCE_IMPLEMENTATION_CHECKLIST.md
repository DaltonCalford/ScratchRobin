# Conformance-First Implementation Checklist

Generated from beta1b `CONFORMANCE_VECTOR.csv` and mapped to local phase stubs (`00->10`).

## Summary

- Total cases: `93`
- Priority counts:
  - `Critical`: `24`
  - `High`: `54`
  - `Medium`: `15`
  - `Low`: `0`
- Baseline counts:
  - `Implemented`: `41`
  - `Partial`: `8`
  - `Planned`: `39`
  - `Stub`: `5`
- Phase distribution:
  - `Phase 00` (Governance and Scaffolding): `4`
  - `Phase 03` (Project and Data Model): `5`
  - `Phase 04` (Backend Adapter and Connection): `9`
  - `Phase 05` (UI Surface and Workflows): `23`
  - `Phase 06` (Diagramming and Visual Modeling): `5`
  - `Phase 07` (Reporting and Governance Integration): `10`
  - `Phase 08` (Advanced Features and Integrations): `15`
  - `Phase 09` (Build/Test/Packaging): `8`
  - `Phase 10` (Conformance/Alpha/Release Gates): `14`

## Execution Order

Use `execution_rank` from the CSV for strict conformance-first ordering (priority-first, then phase-order, then case-id).

## First 25 Checklist Rows

| Rank | Case ID | Priority | Phase | Module Stub | Feature |
|---:|---|---|---|---|---|
| 1 | `A0-LNT-001` | `Critical` | `00` | `src/phases/phase00_governance.cpp` | reject code registry and reference integrity |
| 2 | `P1-GOV-001` | `Critical` | `00` | `src/phases/phase00_governance.cpp` | denied action has no side-effect |
| 3 | `P1-SER-001` | `Critical` | `03` | `src/phases/phase03_project_data.cpp` | project binary round-trip |
| 4 | `P1-SER-002` | `Critical` | `03` | `src/phases/phase03_project_data.cpp` | mandatory chunk presence |
| 5 | `R1-CON-001` | `Critical` | `04` | `src/phases/phase04_backend.cpp` | backend selection by mode |
| 6 | `R1-CON-003` | `Critical` | `04` | `src/phases/phase04_backend.cpp` | capability gating reject |
| 7 | `U1-MAI-001` | `Critical` | `05` | `src/phases/phase05_ui.cpp` | main frame tree/inspector refresh |
| 8 | `U1-MNU-001` | `Critical` | `05` | `src/phases/phase05_ui.cpp` | required top-level menu topology and handlers |
| 9 | `U1-OBJ-001` | `Critical` | `05` | `src/phases/phase05_ui.cpp` | object manager CRUD workflow |
| 10 | `U1-SQL-001` | `Critical` | `05` | `src/phases/phase05_ui.cpp` | sql editor async run and export |
| 11 | `D1-MOD-001` | `Critical` | `06` | `src/phases/phase06_diagramming.cpp` | diagram model persistence |
| 12 | `D1-NOT-001` | `Critical` | `06` | `src/phases/phase06_diagramming.cpp` | notation renderer factory coverage |
| 13 | `RPT-001` | `Critical` | `07` | `src/phases/phase07_reporting.cpp` | question execution runtime |
| 14 | `RPT-006` | `Critical` | `07` | `src/phases/phase07_reporting.cpp` | rrule timezone and dst deterministic next-run |
| 15 | `B1-CMP-001` | `Critical` | `09` | `src/phases/phase09_packaging.cpp` | full profile compile |
| 16 | `B1-CMP-002` | `Critical` | `09` | `src/phases/phase09_packaging.cpp` | no_scratchbird compile |
| 17 | `B1-CMP-003` | `Critical` | `09` | `src/phases/phase09_packaging.cpp` | test targets compile |
| 18 | `PKG-003` | `Critical` | `09` | `src/phases/phase09_packaging.cpp` | profile manifest schema validation |
| 19 | `PKG-005` | `Critical` | `09` | `src/phases/phase09_packaging.cpp` | surface id strict registry validation |
| 20 | `ALPHA-DIA-001` | `Critical` | `10` | `src/phases/phase10_conformance_release.cpp` | silverston erd mandatory continuity set retained |
| 21 | `ALPHA-MIR-001` | `Critical` | `10` | `src/phases/phase10_conformance_release.cpp` | required alpha mirror files present |
| 22 | `ALPHA-MIR-002` | `Critical` | `10` | `src/phases/phase10_conformance_release.cpp` | required alpha mirror hashes stable |
| 23 | `SPC-IDX-001` | `Critical` | `10` | `src/phases/phase10_conformance_release.cpp` | index sb_v3 sb_vnext sb_beta1 spec sets |
| 24 | `SPC-NRM-001` | `Critical` | `10` | `src/phases/phase10_conformance_release.cpp` | resolve authoritative inventories and normative file flags |
| 25 | `A0-BLK-001` | `High` | `00` | `src/phases/phase00_governance.cpp` | blocker register schema and lifecycle validity |

## Full Data

Use `docs/conformance/CONFORMANCE_IMPLEMENTATION_CHECKLIST.csv` as the authoritative checklist table for implementation tracking.
