# ScratchRobin Roadmap (Tracked Implementation Plan)

This plan aligns ScratchRobin with the ScratchBird Alpha scope, while keeping Beta-only items as UI stubs.
ScratchBird network listeners are not ready yet, so early phases rely on a mock backend and capability flags.

## Status Legend
- Completed
- In progress
- Pending

## Phase 0: Baseline Review (Completed)
- Inventory existing ScratchRobin capabilities and gaps against ScratchBird scope.
- Confirm SDI UI and current connection manager wiring.

## Phase 1: Core Service Layer (Completed)
- Add a connection backend abstraction (network + mock) to avoid blocking on listeners. (Done)
- Add background job queue for connect/query/cancel with timeouts. (Done)
- Add capability flags per connection (features supported by server/build). (Done)
- Align config defaults with ScratchBird (port 3092, SSL mode handling). (Done)

## Phase 2: Result Pipeline (Completed)
- Typed decoding for JSON/JSONB/UUID/vector/geometry and binary fallbacks. (Done)
- Streaming grid with paging controls (append mode for large result sets). (Done)
- Export formats (CSV, JSON) and query runtime stats. (Done)
- Surface server notices, warnings, and error stacks in UI. (Done)

## Phase 3: Metadata System (Pending)
- Live catalog fetch (information_schema + ScratchBird catalogs).
- Recursive schema paths and emulated catalog support.
- Metadata cache + observer refresh on DDL.
- Tree search/filter and object context menus.

## Phase 4: Editor + Inspectors (In progress)
- Multi-result execution and statement history.
- Async execution with cancel/status in UI. (Done)
- DDL extract and dependency views in object inspector.
- Plan/Explain and SBLR views when exposed by server.

## Phase 5: Admin Tools (Pending)
- Users/roles/groups management UI.
- Backup/restore UI tied to ScratchBird tooling.
- Monitoring panels (sessions, locks, perf counters).

## Phase 6: Beta Placeholders (Pending)
- Replication and cluster/HA panels as disabled stubs.
- ETL and extended tooling hooks (future Beta scope).

## Implementation Notes
- Mock backend should return protocol-shaped results so UI work is reusable for network mode.
- Network wiring will be enabled once listeners are available, without reworking UI.
- Core unit tests added for job queue + mock backend fixtures.

## Verification
- UI build + unit tests pass with wxWidgets installed; grid component falls back to core/base libs when missing.
