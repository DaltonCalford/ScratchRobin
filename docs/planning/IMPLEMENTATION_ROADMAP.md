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

- [ ] **(9)** Persist reporting objects in project state  
  Owner: `TBD` | Status: `Not Started` | Depends: `1,6` | Est: `2–4d`

- [ ] **(10)** Reporting query runner + cache  
  Owner: `TBD` | Status: `Not Started` | Depends: `2,6` | Est: `4–6d`

- [ ] **(11)** Scheduler for refresh/alerts/subscriptions  
  Owner: `TBD` | Status: `Not Started` | Depends: `2,10` | Est: `3–5d`

---

## Phase 4 — Diagram Data Views

- [ ] **(12)** Persist Data View panels  
  Owner: `TBD` | Status: `Not Started` | Depends: `6` | Est: `1–2d`

- [ ] **(13)** Data View refresh + invalidation logic  
  Owner: `TBD` | Status: `Not Started` | Depends: `2,10,12` | Est: `2–3d`

---

## Phase 5 — Governance Enforcement

- [ ] **(14)** Runtime permission enforcement  
  Owner: `TBD` | Status: `Not Started` | Depends: `3,6,10,11` | Est: `3–5d`

- [ ] **(15)** Audit logging integration for reporting  
  Owner: `TBD` | Status: `Not Started` | Depends: `14` | Est: `1–2d`

---

## Phase 6 — Clean‑up + Consistency

- [ ] **(16)** Fix diagram extension contradictions  
  Owner: `TBD` | Status: `Not Started` | Depends: `5` | Est: `0.5–1d`

- [ ] **(17)** Validation + test scaffolding  
  Owner: `TBD` | Status: `Not Started` | Depends: `6–15` | Est: `3–5d`

---

## Notes

- Update `Owner` and `Status` fields as tasks progress.
- Convert `Status` to `In Progress` / `Blocked` / `Complete` as needed.
- Use the numbered task IDs for cross‑references in other planning docs.
