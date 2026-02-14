# Implementation Roadmap (Living Tracker)

**Status**: Draft  
**Last Updated**: 2026-02-05

This tracker reflects the sequenced execution roadmap for reaching a fully implementable ScratchRobin system (ScratchBird-only).

---

## Phase 1 — Spec Closure

- [x] **(1)** Extend project persistence spec for reporting + data views  
  Owner: `TBD` | Status: `Complete` | Depends: `—` | Est: `1–2d`

- [x] **(2)** Reporting runtime execution spec  
  Owner: `TBD` | Status: `Complete` | Depends: `1` | Est: `2–4d`

- [x] **(3)** Governance enforcement spec  
  Owner: `TBD` | Status: `Complete` | Depends: `1` | Est: `2d`

- [x] **(4)** Conflict resolution algorithm spec  
  Owner: `TBD` | Status: `Complete` | Depends: `—` | Est: `2–3d`

- [x] **(5)** Diagram extension standardization spec  
  Owner: `TBD` | Status: `Complete` | Depends: `—` | Est: `0.5d`

---

## Phase 2 — Core Project Persistence Implementation

- [x] **(6)** Implement project Save/Load  
  Owner: `TBD` | Status: `Complete` | Depends: `1` | Est: `4–6d`

- [x] **(7)** Implement DB extraction pipeline  
  Owner: `TBD` | Status: `Complete` | Depends: `6` | Est: `3–5d`

- [x] **(8)** Implement Git sync + conflict resolution  
  Owner: `TBD` | Status: `Complete` | Depends: `4,6` | Est: `4–6d`

---

## Phase 3 — Reporting Runtime

- [x] **(9)** Persist reporting objects in project state  
  Owner: `TBD` | Status: `Complete` | Depends: `1,6` | Est: `2–4d`

- [x] **(10)** Reporting query runner + cache  
  Owner: `TBD` | Status: `Complete` | Depends: `2,6` | Est: `4–6d`

- [x] **(11)** Scheduler for refresh/alerts/subscriptions  
  Owner: `TBD` | Status: `Complete` | Depends: `2,10` | Est: `3–5d`

---

## Phase 4 — Diagram Data Views

- [x] **(12)** Persist Data View panels  
  Owner: `TBD` | Status: `Complete` | Depends: `6` | Est: `1–2d`

- [x] **(13)** Data View refresh + invalidation logic  
  Owner: `TBD` | Status: `Complete` | Depends: `2,10,12` | Est: `2–3d`

---

## Phase 5 — Governance Enforcement

- [x] **(14)** Runtime permission enforcement  
  Owner: `TBD` | Status: `Complete` | Depends: `3,6,10,11` | Est: `3–5d`

- [x] **(15)** Audit logging integration for reporting  
  Owner: `TBD` | Status: `Complete` | Depends: `14` | Est: `1–2d`

---

## Phase 6 — Clean‑up + Consistency

- [x] **(16)** Fix diagram extension contradictions  
  Owner: `TBD` | Status: `Complete` | Depends: `5` | Est: `0.5–1d`

- [x] **(17)** Validation + test scaffolding  
  Owner: `TBD` | Status: `Complete` | Depends: `6–15` | Est: `3–5d`

---

## Phase 7 — Diagram Types Implementation

- [x] **(18)** Silverston diagram renderer + interaction model  
  Owner: `TBD` | Status: `Complete` | Depends: `5` | Est: `4–6d`

- [x] **(19)** Whiteboard diagram engine (domain wizard + freeform attributes)  
  Owner: `TBD` | Status: `Complete` | Depends: `18` | Est: `3–5d`

- [x] **(20)** Mind map model + serialization + round‑trip  
  Owner: `TBD` | Status: `Complete` | Depends: `18` | Est: `3–4d`

- [x] **(21)** Data Flow Diagram (DFD) renderer + ERD traceability  
  Owner: `TBD` | Status: `Complete` | Depends: `18` | Est: `4–6d`

- [x] **(22)** SVG‑first export pipeline for diagram types  
  Owner: `TBD` | Status: `Complete` | Depends: `18–21` | Est: `2–3d`

---

## Phase 8 — Project System & Documentation

- [x] **(23)** Project metadata + governance runtime wiring  
  Owner: `TBD` | Status: `Complete` | Depends: `3,6` | Est: `3–5d`

- [x] **(24)** Project templates + automated documentation generation  
  Owner: `TBD` | Status: `Complete` | Depends: `6` | Est: `3–4d`

- [x] **(25)** Reporting storage persistence implementation  
  Owner: `TBD` | Status: `Complete` | Depends: `1,6,9` | Est: `3–5d`

- [x] **(26)** Documentation indices refresh (README, target features, trackers)  
  Owner: `TBD` | Status: `Complete` | Depends: `—` | Est: `1–2d`

- [x] **(27)** Security policy UI stubs (RLS, audit, password, lockout, role switch, policy epoch, audit log)  
  Owner: `TBD` | Status: `Complete` | Depends: `—` | Est: `1–2d`

---

## Notes

- Update `Owner` and `Status` fields as tasks progress.
- Convert `Status` to `In Progress` / `Blocked` / `Complete` as needed.
- Use the numbered task IDs for cross‑references in other planning docs.
  - Phase 7 deliverables include diagram persistence under `designs/diagrams`, DFD → ERD trace navigation, mind map collapse chevrons + counts, and trace reference parsing tests.
  - Recent progress: Data View renderer + refresh UI + editor dialog + stale indicators; reporting panel UI; query reference parsing hardened for invalidation; `query_refs` caching for Data View invalidation; template discovery API + tests; reporting governance gates expanded for save/create + audit metadata; documentation generator now writes template stubs plus data dictionary/reporting/diagram summaries (schema grouping + kind breakdown + collection breakdown + export hints + metadata details + counts + summary tables + TOCs + totals + collection tables + cross-links + export checks + warning summary + collection links + reporting warnings + design-path + collection-ref + invalid-json + missing-id + id-mismatch + duplicate-id + duplicate-collection + empty-name checks + type-mismatch/unknown flags + diagram warnings section + attribute warnings + orphan-export + extension checks + duplicate-name/path warnings + empty-collection + empty-name warnings + outside-path checks); governance evaluation tests; approval governance hooks added; project validation stub + tests; security policy viewers load catalog-backed data (lockout, role switch, policy epoch, audit log); performance tests now gated via `SCRATCHROBIN_RUN_PERF_TESTS` with threshold scaling by `SCRATCHROBIN_PERF_MS_FACTOR` and a labeled `scratchrobin_perf_diagram` CTest entry (`perf;quarantine`).
