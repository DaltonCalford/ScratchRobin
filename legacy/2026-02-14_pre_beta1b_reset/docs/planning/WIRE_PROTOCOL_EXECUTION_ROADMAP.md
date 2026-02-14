# ScratchBird Wire Protocol Enablement Roadmap (SBWP v1.1)

**Status**: Draft  
**Last Updated**: 2026-02-06  
**Scope**: COPY, Cancellation, Prepared Statements + Streaming (ScratchBird-only)

This roadmap converts the maximal plan into sequenced phases with dependencies and effort estimates.

---

## Phase 0 — Alignment + Interfaces (2–3d)

1. **Define shared protocol surface in ScratchRobin**  
   Dependencies: —  
   Est: 1d  
   - Copy options/results and backend interface in `core`  
   - Capability flags for COPY, prepared, streaming

2. **UI scaffolding for COPY + Prepared + Streaming**  
   Dependencies: 1  
   Est: 1–2d  
   - COPY dialog + wiring entry point  
   - Prepared parameter placeholder panel  
   - Streaming status placeholders

---

## Phase 1 — COPY End-to-End (6–9d)

1. **ScratchBird backend COPY wiring**  
   Dependencies: Phase 0  
   Est: 1–2d  
   - Map COPY streams to ScratchBird client API  
   - Support IN/OUT/BOTH and binary/text flags

2. **COPY UI workflow + file/clipboard transport**  
   Dependencies: Phase 0  
   Est: 2–3d  
   - File/clipboard inputs and outputs  
   - Progress + summary reporting  
   - Basic error handling + retry

3. **COPY performance + backpressure tuning**  
   Dependencies: 1,2  
   Est: 2–3d  
   - Window/chunk controls  
   - Throughput metrics and adaptive window  
   - Large file smoke tests

4. **COPY logging + audit hooks**  
   Dependencies: 2  
   Est: 1–2d  
   - Audit records for COPY  
   - Operation summary in status bar

---

## Phase 2 — Cancellation (4–6d)

1. **ScratchBird cancel API integration**  
   Dependencies: Phase 0  
   Est: 2–3d  
   - Wire cancel calls through backend  
   - Soft/Hard cancel modes

2. **Cancel UX + timeouts**  
   Dependencies: 1  
   Est: 1–2d  
   - Cancel button state management  
   - Timeout policies + auto-cancel

3. **Cancel tests + observability**  
   Dependencies: 1,2  
   Est: 1d  
   - Unit tests + integration harness  
   - Log + telemetry

---

## Phase 3 — Prepared Statements (8–12d)

1. **Prepared statement core API**  
   Dependencies: Phase 0  
   Est: 2–3d  
   - Prepare/Bind/Execute/Close in backend  
   - Statement cache hooks

2. **Prepared execution UI**  
   Dependencies: 1  
   Est: 3–4d  
   - Parameter grid with type hints  
   - Bind by name or position

3. **Prepared diagnostics + cache management**  
   Dependencies: 1,2  
   Est: 2–3d  
   - Statement cache metrics  
   - Cache invalidation on schema changes

4. **Prepared tests**  
   Dependencies: 1,2  
   Est: 1–2d  
   - Param binding types  
   - Repeated execution + cache behavior

---

## Phase 4 — Streaming (6–9d)

1. **Streaming data flow in backend**  
   Dependencies: Phase 0  
   Est: 2–3d  
   - Stream window control  
   - LOB streaming controls

2. **Streaming UI + grid integration**  
   Dependencies: 1  
   Est: 2–3d  
   - Incremental row append  
   - Stream pause/resume

3. **Performance + spill-to-disk**  
   Dependencies: 1,2  
   Est: 2–3d  
   - Optional disk spill for large results  
   - Memory usage tests

---

## Phase 5 — Documentation + QA Closure (3–5d)

1. **Docs + onboarding updates**  
   Dependencies: Phases 1–4  
   Est: 1–2d  
   - COPY usage  
   - Prepared/streaming workflows  
   - Cancel policy and UX

2. **Integration test suite + CI tags**  
   Dependencies: 1  
   Est: 2–3d  
   - Add SBWP protocol tests  
   - Label/timeout controls

---

## Phase 6 — Remaining SBWP Message Coverage (4–6d)

1. **Subscribe/Unsubscribe + Notification pipeline**  
   Dependencies: Phase 0  
   Est: 1–2d  
   - Add client API for notifications  
   - UI panel for async events

2. **Query progress events**  
   Dependencies: Phase 0  
   Est: 1–2d  
   - Capture QUERY_PROGRESS messages  
   - Surface progress in SQL editor + status bar

3. **Status request/response support** (Complete)  
   Dependencies: Phase 0  
   Est: 0.5–1d  
   - Add status request/response API + backend plumbing  
   - Status panel in SQL editor (fetch + log)

4. **Shutdown message support**  
   Dependencies: Phase 0  
   Est: 0.5–1d  
   - Add shutdown request path (admin only)

5. **SBLR bytecode execution path**  
   Dependencies: Phase 0  
   Est: 1–2d  
   - Execute bytecode with SQL payload  
   - Expose in SQL editor (advanced)

---

## Notes

- Phase ordering is optimized to unlock COPY first, then cancellation, then prepared statements and streaming.
- Some items depend on a ScratchBird server to validate. Those remain offline-wired until server access is available.
