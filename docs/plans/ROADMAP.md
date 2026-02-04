# ScratchRobin Roadmap

**Status**: Superseded by detailed implementation tracker  
**See**: [../planning/MASTER_IMPLEMENTATION_TRACKER.md](../planning/MASTER_IMPLEMENTATION_TRACKER.md)

---

## Quick Status Overview

| Phase | Status | Completion |
|-------|--------|------------|
| Phase 0-2: Foundation | ✅ Complete | 100% |
| Phase 3: Metadata System | 🟡 Partial | 70% |
| Phase 4: Editor + Inspectors | ✅ Complete | 100% |
| Phase 5: Admin Tools | 🟡 In Progress | 40% |
| Phase 6-7: Beta Features | 🔴 Not Started | 0% |

## Historical Roadmap (Retained for Context)

### Phase 0: Baseline Review (Completed)
- Inventory existing ScratchRobin capabilities and gaps against ScratchBird scope.
- Confirm SDI UI and current connection manager wiring.

### Phase 1: Core Service Layer (Completed)
- Add a connection backend abstraction (network + mock) to avoid blocking on listeners.
- Add background job queue for connect/query/cancel with timeouts.
- Add capability flags per connection (features supported by server/build).
- Align config defaults with ScratchBird (port 3092, SSL mode handling).

### Phase 2: Result Pipeline (Completed)
- Typed decoding for JSON/JSONB/UUID/vector/geometry and binary fallbacks.
- Streaming grid with paging controls (append mode for large result sets).
- Export formats (CSV, JSON) and query runtime stats.
- Surface server notices, warnings, and error stacks in UI.

### Phase 3: Metadata System (In Progress)
- Live catalog fetch (information_schema + ScratchBird catalogs). (Done; depends on listener availability)
- Recursive schema paths and emulated catalog support. (Done, fixture-backed)
- Metadata cache + observer refresh on DDL. (Done, fixture-backed)
- Tree search/filter and object context menus. (Done)
- Fixture-driven metadata tests (complex/invalid coverage). (Done)
- Direct catalog browsing + full query execution via external backends (PostgreSQL/MySQL/Firebird). (In progress)
  - Scope: `docs/plans/BACKEND_ADAPTERS_SCOPE.md`
  - Adapters implemented behind optional CMake toggles.
  - Integration tests added (env-gated) for PostgreSQL/MySQL/Firebird.
  - External backend metadata queries implemented; ScratchBird catalogs still pending on listeners.

### Phase 4: Editor + Inspectors (Completed)
- Multi-result execution and statement history.
- Async execution with cancel/status in UI.
- DDL extract and dependency views in object inspector.
- Plan/Explain and SBLR views when exposed by server.

### Phase 5: Admin Tools (In Progress)
- Users/roles/groups management UI. (In progress: users/roles browser + membership tab + templates)
- Backup/restore UI tied to ScratchBird tooling.
- Monitoring panels (sessions, transactions, locks, perf, table stats, I/O stats). (In progress)

### Phase 6: Beta Placeholders (Pending)
- Replication and cluster/HA panels as disabled stubs.
- ETL and extended tooling hooks (future Beta scope).

## Current Focus

See the [Master Implementation Tracker](../planning/MASTER_IMPLEMENTATION_TRACKER.md) for:
- **200+ detailed tasks** with priorities and dependencies
- **8 implementation phases** with estimated timelines
- **Acceptance criteria** for each task
- **Current status tracking** for all components

### Immediate Priorities (Next 4 Weeks)

Based on the master tracker:

1. **Connection Profile Editor** - Eliminate manual TOML editing
2. **Object Manager Wiring** - Connect stub UIs to ScratchBird backend
3. **Transaction Management** - Complete transaction state handling
4. **Error Handling Framework** - Consistent error display

## Planning Documents

| Document | Purpose |
|----------|---------|
| [../planning/MASTER_IMPLEMENTATION_TRACKER.md](../planning/MASTER_IMPLEMENTATION_TRACKER.md) | Comprehensive task tracker |
| [../planning/QUICK_START_CHECKLIST.md](../planning/QUICK_START_CHECKLIST.md) | First 12 weeks checklist |
| [../planning/INDEX.md](../planning/INDEX.md) | Planning documents index |
| [BACKEND_ADAPTERS_SCOPE.md](BACKEND_ADAPTERS_SCOPE.md) | External backend adapter details |

## Implementation Notes

- Mock backend should return protocol-shaped results so UI work is reusable for network mode.
- Network wiring will be enabled once listeners are available, without reworking UI.
- Core unit tests added for job queue + mock backend fixtures.

## Verification

- UI build + unit tests pass with wxWidgets installed; grid component falls back to core/base libs when missing.

---

*This roadmap is maintained for historical context. For current planning, refer to the [Master Implementation Tracker](../planning/MASTER_IMPLEMENTATION_TRACKER.md).*
