# ScratchRobin UI Inventory

Status: Draft
Last Updated: 2026-02-03

This inventory captures every visible UI surface, menus, and core widgets so it
is easy to review what exists and what is pending.

## How to View Without a Database

1. Create a fixture-only connection:

```toml
[[connection]]
name = "Mock Catalogs"
backend = "mock"
fixture_path = "config/fixtures/default.json"
```

Place it at `~/.config/scratchrobin/connections.toml` or keep it in
`config/connections.toml.example` for local runs.

2. Launch the app and open the views from the menu:

- `Window -> New SQL Editor`
- `Window -> Monitoring`
- `Window -> Users & Roles`
- `Window -> New Diagram`

## Main Window (Catalog + Inspector)

File: `src/ui/main_frame.cpp`

### Menus
- Connections
  - Server/Cluster/Database/Project/Diagram/Git (actions stubbed)
- Edit
  - Cut / Copy / Paste / Select All
- View
  - Refresh (placeholder)
- Window
  - New SQL Editor
  - New Diagram
  - Monitoring
  - Users & Roles
- Help
  - Help for this window / command / language (placeholders)

### Layout
- Left panel: catalog tree with filter box and clear button
- Right panel: inspector notebook
  - Overview tab (read-only text)
  - DDL tab (read-only text)
  - Dependencies tab (read-only text)

### Context Menu (Tree)
- Open in SQL Editor
- Copy name
- Copy DDL
- Show dependencies
- Refresh

### Implemented
- Tree filter/search
- Per-node overview/DDL/dependencies
- Context menu actions
- Metadata refresh from fixtures

### Pending
- Live ScratchBird catalog queries (listeners not ready)
- Inline DDL execution from inspector

## SQL Editor Window

File: `src/ui/sql_editor_frame.cpp`

### Session Bar
- Connection selector
- Connect / Disconnect
- Auto-commit toggle
- Begin / Commit / Rollback

### Execution Bar
- Run / Cancel
- Paging toggle + Stream toggle
- Page size spinner
- Prev / Next page buttons
- Export CSV / Export JSON
- Status label

### Results Bar
- Result selector (multi-statement)
- History selector + Load
- Explain button
- SBLR button

### Split View
- SQL editor (multi-line text)
- Result notebook
  - Results grid (virtualized)
  - Plan view (text)
  - SBLR view (text)
  - Messages view (notices/errors)

### Implemented
- Async execution + cancel
- Statement splitter and multi-result handling
- Paging/streaming grid
- Export to CSV/JSON
- Statement history
- Explain/SBLR view placeholders

### Pending
- ScratchBird plan/SBLR output (waiting on server)
- Advanced editor features (syntax highlight, autocompletion)

## Diagram Window

File: `src/ui/diagram_frame.cpp`

### Layout
- Notebook of diagram tabs
- Palette stub (left)
- Canvas stub (center)
- Properties stub (right)

### Implemented
- Diagram host window with multiple tabs
- Menu/iconbar replication rules

### Pending
- ERD canvas, tools, and notation rendering
- Save/load and export pipelines

## Startup Window (Branding)

File: `src/ui/startup_frame.cpp`

### Layout
- Optional logo
- Status label
- Optional progress gauge

### Implemented
- Optional startup frame that hides on click
- Auto-dismiss once main window is shown

## Monitoring Window

File: `src/ui/monitoring_frame.cpp`

### Controls
- Connection selector
- View selector: Sessions, Locks, Perf, Statements, Long-running
- Connect / Disconnect / Refresh buttons

### Implemented
- External backend monitoring queries (PostgreSQL/MySQL/Firebird)
- Results grid display

### Pending
- ScratchBird native monitoring queries (listeners not ready)
- Column-mapping parity cleanup once native views are final

## Users & Roles Window

File: `src/ui/users_roles_frame.cpp`

### Controls
- Connection selector
- Connect / Disconnect / Refresh buttons
- Notebook tabs:
  - Users
  - Roles
  - Memberships

### Templates
- Create/Drop user and role templates per backend
- Grant/Revoke membership templates

### Implemented
- External backend listing queries
- Backend-gated template buttons

### Pending
- ScratchBird native user/role catalogs
- Execution of admin templates (currently read-only helpers)

## Shared UI Components

### Result Grid
- File: `src/ui/result_grid_table.cpp`
- Virtualized paging support

### Window Manager
- File: `src/ui/window_manager.cpp`
- Tracks and closes all open windows

### Menu/Icon Bar
- File: `src/ui/menu_builder.cpp`
- File: `src/ui/icon_bar.cpp`
- Shared menu and toolbar construction

## Fixture Coverage

- Primary mock fixture: `config/fixtures/default.json`
- Multi-catalog test fixture: `tests/fixtures/metadata_multicatalog.json`
- Fixture schema: `docs/fixtures/README.md`

## Docker Manager Panel

File: `src/ui/docker_manager_panel.cpp`

### Purpose
Manage Docker containers for ScratchBird deployment with multiple database services.

### Components
- Container list with status, health, CPU/memory usage
- Log viewer with auto-scroll
- Configuration panel for services
- Template selection (Development, Production, Minimal)

### Features
- Start/stop/restart containers
- View real-time logs
- Configure services (Native, PostgreSQL, MySQL, Firebird)
- Port mapping configuration
- Resource limits (memory, CPU)
- Volume mounting

---

## Test Runner Panel

File: `src/ui/test_runner_panel.cpp`

### Purpose
Execute and manage database tests with multiple test types.

### Components
- Test suite tree view (grouped by type)
- Results list with pass/fail status
- Progress gauge
- Statistics panel (passed/failed/skipped/total)
- Test details panel
- Execution log

### Supported Test Types
1. **Unit** - Object structure validation
2. **Integration** - Workflow testing
3. **Performance** - Benchmark and load tests
4. **Data Quality** - Data integrity checks
5. **Security** - Access control validation
6. **Migration** - Migration validation

### Output Formats
- Text, JSON, HTML, JUnit XML, Markdown

---

## Whiteboard Canvas

File: `src/ui/whiteboard_canvas.cpp`

### Purpose
Free-form diagramming with typed objects for database architecture visualization.

### Components
- Canvas with zoom/pan/grid
- Object palette (12 typed object types)
- Connection lines
- Selection and resize handles

### Typed Objects
Rectangular objects with:
- **Header**: Colored bar showing `Type: Name`
- **Body**: Free-form details area

**Types**: Database, Schema, Table, View, Procedure, Function, Trigger, Index, Datastore, Server, Cluster, Generic

### Interactions
- Double-click header → Edit name
- Double-click body → Edit details
- F2 → Quick edit name
- Enter → Quick edit details
- Drag → Move
- Resize handles → Resize

---

## Known Gaps to Track

- Connection editor UI (profiles are still config-file driven)
- Native ScratchBird monitoring, users/roles, and catalog refresh
- Backup/restore UI
- Beta placeholders (cluster/replication/ETL panels)
- Diagram engine + ERD tooling integration
- Context-sensitive help content and icon asset set

## Related Specs

- `docs/specifications/ui/UI_WINDOW_MODEL.md`
- `docs/specifications/diagramming/ERD_MODELING_AND_ENGINEERING.md`
