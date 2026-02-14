# Wire Protocol Enablement Tracker (SBWP v1.1)

**Last Updated**: 2026-02-06  
**Owners**: `TBD`

This tracker covers COPY, Cancellation, Prepared Statements, and Streaming enablement in ScratchRobin.

---

## Phase 0 — Alignment + Interfaces

- [x] **(WP-01)** Core COPY types + backend interface  
  Owner: `dcalford` | Status: `Complete` | Depends: `—`

- [x] **(WP-02)** Capability flags for COPY/Prepared/Streaming  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [x] **(WP-03)** UI scaffolding (COPY dialog + Prepared placeholders + Streaming badges)  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

---

## Phase 1 — COPY End-to-End

- [x] **(WP-04)** ScratchBird backend COPY wiring (IN/OUT/BOTH)  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [ ] **(WP-05)** COPY UI workflow (file/clipboard + progress)  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-03`

- [ ] **(WP-06)** COPY performance tuning (window/chunk + metrics)  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-04, WP-05`

- [ ] **(WP-07)** COPY audit + logging hooks  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-05`

---

## Phase 2 — Cancellation

- [x] **(WP-08)** ScratchBird cancel API wiring  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [ ] **(WP-09)** Cancel UX policies (soft/hard, timeouts)  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-08`

- [ ] **(WP-10)** Cancel tests + telemetry  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-08, WP-09`

---

## Phase 3 — Prepared Statements

- [x] **(WP-11)** Prepared statement backend API  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [x] **(WP-12)** Prepared execution UI (parameter grid)  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-11`

- [ ] **(WP-13)** Statement cache + diagnostics  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-11, WP-12`

- [ ] **(WP-14)** Prepared statement tests  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-11, WP-12`

---

## Phase 4 — Streaming

- [x] **(WP-15)** Streaming backend control (window + LOB streaming)  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [ ] **(WP-16)** Streaming UI (incremental grid + pause/resume)  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-15`

- [ ] **(WP-17)** Streaming performance + spill-to-disk  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-15, WP-16`

---

## Phase 5 — Docs + QA Closure

- [ ] **(WP-18)** Documentation updates (COPY/Prepared/Streaming/Cancel)  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-04–WP-17`

- [ ] **(WP-19)** Integration test suite + CI labels  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-04–WP-17`

---

## Phase 6 — Remaining SBWP Message Coverage

- [x] **(WP-20)** Subscribe/Unsubscribe + Notification pipeline  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [x] **(WP-21)** Query progress events + UI surfacing  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [x] **(WP-22)** Status request/response support  
  Owner: `dcalford` | Status: `Complete` | Depends: `WP-01`

- [ ] **(WP-23)** Shutdown messaging support  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-01`

- [ ] **(WP-24)** SBLR bytecode execution path  
  Owner: `TBD` | Status: `Not Started` | Depends: `WP-01`
