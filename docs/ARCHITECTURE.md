# ScratchRobin Architecture (Rewrite Baseline)

## Goals
- SDI model: each major surface is its own top-level window.
- Native widgets, low footprint, fast startup.
- ScratchBird-first, with optional external backends (PostgreSQL/MySQL/Firebird) when client libs are available.
- Metadata stays live via observer notifications, with fixture-backed catalogs when offline.

## High-Level Components

### App Shell
- `WindowManager`: tracks all open windows per connection.
- `MainFrame`: tree + inspector panels anchored to a connection profile.
- `MenuBuilder` / `IconBar`: shared menu + toolbar construction.

### Core Services
- `ConnectionManager`: owns connection lifecycle, backend selection, and cancellation.
- `ConnectionBackend`: adapter interface (ScratchBird network, mock, external).
- `CredentialStore`: OS keychain/secret integration.
- `MetadataModel`: live catalog fetch for external backends with fixture fallback and observer refresh hooks.
- `StatementSplitter`: multi-statement parsing with delimiter support.
- `ResultExporter`: CSV/JSON export from query results.
- `ValueFormatter`: consistent display formatting for UI grids.

### UI Surfaces (SDI)
- `MainFrame`: tree of servers/catalogs/objects with inspector tabs.
- `SqlEditorFrame`: editor + results grid + statement history.
- `MonitoringFrame`: read-only panels for sessions/locks/perf queries.
- `UsersRolesFrame`: read-only users/roles browser with backend-gated admin templates.
- `DiagramFrame`: multi-tab diagram host with palette/canvas placeholders.
- `StartupFrame`: optional branding splash (logo + status + progress).
- `ResultGridTable`: virtualized grid model for large result sets.

### Background Work
- Long operations (connect, query, fetch, backup) run off the UI thread.
- UI updates are posted via event queue; no direct cross-thread UI access.

## Data Flow (Observer Pattern)
1. User runs DDL in `SqlEditorFrame`.
2. `ConnectionManager` executes through the selected `ConnectionBackend`.
3. `MetadataModel` invalidates and refreshes affected nodes.
4. Observers (tree, inspector) update immediately.

## Extensibility
- External adapters can be added without reworking UI or job execution.
- Capability flags gate features when a backend lacks support.
- Fixture packs allow UI and tests to run before live listeners exist.

## Related Specifications
- UI window model: `docs/specifications/UI_WINDOW_MODEL.md`
- ERD modeling: `docs/specifications/ERD_MODELING_AND_ENGINEERING.md`
- Context-sensitive help: `docs/specifications/CONTEXT_SENSITIVE_HELP.md`
- Icon assets: `docs/specifications/UI_ICON_ASSETS.md`
