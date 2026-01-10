# ScratchRobin Architecture (Rewrite Baseline)

## Goals
- SDI model: each major surface is its own top-level window.
- Native widgets, low footprint, fast startup.
- ScratchBird-only for the first release.
- Metadata stays live via observer notifications.

## High-Level Components

### App Shell
- `WindowManager`: tracks all open windows per connection.
- `SessionRegistry`: manages active connections and their UI attachments.
- `CommandBus`: dispatches UI actions to core services.

### Core Services
- `ConnectionManager`: owns connection lifecycle, pooling, and cancellation.
- `MetadataCache`: lazy-loads and invalidates schema objects.
- `QueryRunner`: executes statements, handles statement splitting.
- `HistoryStore`: persists executed statements and metadata.

### UI Surfaces (SDI)
- `MainFrame`: tree of servers/databases/objects.
- `SqlEditorFrame`: editor + results pane + commit/rollback controls.
- `ObjectInspectorFrame`: HTML-based property viewer (template-driven).
- `DataGridFrame`: large result sets with fetch-on-demand.

### Background Work
- Long operations (connect, query, fetch, backup) run off the UI thread.
- UI updates are posted via event queue; no direct cross-thread UI access.

## Data Flow (Observer Pattern)
1. User runs DDL in `SqlEditorFrame`.
2. `QueryRunner` executes and detects affected objects.
3. `MetadataCache` invalidates and refreshes affected nodes.
4. Observers (tree, inspector, autocomplete) update immediately.

## Extensibility
- Dialect adapters may be added later, but the core remains ScratchBird-first.
- HTML templates can be customized without recompilation.

