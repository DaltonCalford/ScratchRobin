# Roadmap (High-Level)

For the tracked implementation plan and phase status, see `docs/plans/ROADMAP.md`.

## Current Focus (Phase 3)
- Live catalog fetch is wired for ScratchBird sys.* catalogs and external backends.
- External backends (PostgreSQL/MySQL/Firebird) are implemented with live catalog queries, gated by client libraries.

## Completed Milestones
- Connection backend abstraction, job queue, capability flags.
- SQL editor async execution, paging grid, exports, status reporting.
- Fixture-backed metadata tree, inspector tabs, and context menus.

## Early Phase 5 Progress
- Monitoring window for sessions/transactions/locks/perf/table stats/I/O stats (native + external).
- Users/Roles browser with memberships tab and backend-gated admin templates.

## Pending
- Admin tools (users/roles, backup/restore, monitoring panels).
- Beta placeholders for cluster/replication/ETL panels.
