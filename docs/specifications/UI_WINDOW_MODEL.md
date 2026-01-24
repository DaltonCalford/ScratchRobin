# ScratchRobin UI Window Model

Status: Draft
Last Updated: 2026-01-09

This specification defines ScratchRobin's window set, menu merge rules, and
per-window UI objects. It is a design reference and does not imply immediate
implementation.

## Global Rules

- ScratchRobin uses SDI-style top-level windows.
- Windows may be detachable or tabbed within their own window group.
- Diagram windows can tab with other diagram windows, but not with non-diagram
  windows.
- Menu items are context-aware: each active window contributes its menu items
  into the global menu bar.
- The main window menu can be replicated onto other window types based on
  per-window configuration.
- The Connections menu is the canonical entry point for creating, connecting,
  and dropping servers/clusters/databases/projects/diagrams.
- Use database-oriented verbs (`Create`, `Connect`, `Disconnect`, `Drop`).
  `Remove` is reserved for removing UI-only entries from recent lists.
- Command icons live in `assets/icons/` with SVG and PNG variants at 24/32/48.
- Context-sensitive help content is indexed in `assets/help/` (see
  `docs/specifications/CONTEXT_SENSITIVE_HELP.md`).

## Menu and Iconbar Replication

- Each window type can opt into a local menu bar, a local iconbar, both, or
  neither.
- The main window menu can be reused in local windows, with each window type
  contributing its context-specific items.
- A configurable drag/drop iconbar mirrors menu actions and can be:
  - global (main window only), or
  - replicated per window type.
- Users may hide menus and iconbars on specific windows to maximize workspace,
  especially for multi-monitor setups.
- Window configuration is user preference and persisted per window type.

## Connections Menu (Canonical)

Connections
- Server
  - Create
    - Local ScratchBird server…
    - Local emulated engine…
  - Connect…
  - Disconnect
  - Drop…
  - Remove from list…
- Cluster
  - Create…
  - Connect…
  - Disconnect
  - Drop…
  - Remove from list…
- Database
  - Create
    - Native ScratchBird database…
    - Emulated catalog inside ScratchBird…
  - Connect…
  - Disconnect
  - Drop…
- Project
  - Create
    - Local project…
    - Shared project (stored in database/cluster)…
  - Connect…
  - Disconnect
  - Drop…
- Diagram
  - Create
    - ERD
    - Data Flow
    - UML
  - Open…
  - Drop…
- Git
  - Configure local identity…
  - Connect to database/cluster Git…
  - Open project repo…
  - Pull / Push / Status
- Recent / Quick Connections
  - <dynamic list>
- Exit

## Context-Aware Edit Menu

- Base actions (all windows): Cut / Copy / Paste / Select All
- SQL Editor: Undo/Redo, Find/Replace, Format SQL
- Results grid: Copy As (CSV/Markdown/Text), Find, Sort, Filter
- Diagram: Align, Distribute, Group/Ungroup, Order, Snap

## Window Specifications

### 1) Catalog Browser (Main Window)

Purpose:
- Primary navigation for servers/clusters/databases/projects and objects.

Menus (merge items):
- View: Toggle inspector tabs, Refresh metadata
- Window: New SQL Editor, New Diagram, Monitoring, Users & Roles, Job Scheduler

UI Objects:
- Catalog tree (server -> database -> schema -> objects)
- Filter/search control
- Inspector tabs: Overview / DDL / Dependencies / Notes
- Context menu: Open in SQL Editor, Copy Name, Copy DDL, Show Dependencies, Refresh

Notes:
- The tree is the default landing surface after launch.
- Connection profiles are loaded from config and displayed in the tree.

### 2) SQL Editor

Purpose:
- Execute SQL, view results, manage transactions.

Menus (merge items):
- Edit: Undo/Redo, Find/Replace, Format SQL
- View: Results tabs (Results/Plan/SBLR/Messages)

UI Objects:
- Connection selector
- Connect/Disconnect
- Auto-commit toggle
- Begin/Commit/Rollback
- Run/Cancel
- Paging/Streaming controls + page size
- Row limit control (default 200, configurable)
- Result tabs: grid / plan / sblr / messages
- Statement history
- Export: CSV/JSON
- Query stats output (elapsed time, rows returned, row limit hit)
- Context-aware suggestions (variables, parameters, object names)

### SQL Editor Capability Matrix

Capabilities are gated by backend type and the active connection. The matrix
below defines intended behavior for UI enablement.

| Capability | ScratchBird | PostgreSQL | MySQL/MariaDB | Firebird | Mock |
|------------|-------------|------------|---------------|----------|------|
| Connect/Disconnect | Yes | Yes | Yes | Yes | Yes |
| Transactions | Yes | Yes | Yes | Yes | Yes |
| Cancel query | Yes | Yes | Limited | Limited | No |
| Paging/Streaming | Yes | Yes | Yes | Yes | Yes |
| Explain/Plan | Yes | Yes | Yes | Yes | Stub |
| SBLR view | Yes | No | No | No | Stub |
| Per-statement timing | Yes | Yes | Yes | Yes | Yes |
| Rows returned | Yes | Yes | Yes | Yes | Yes |
| Row limit enforcement | Yes | Yes | Yes | Yes | Yes |
| Server-side stats (IO/counters) | Yes | Partial | Partial | Partial | No |
| Context-aware suggestions | Yes | Yes | Yes | Yes | Yes |

### 3) Monitoring

Purpose:
- Read-only session/lock/performance monitoring.

Menus (merge items):
- View: Sessions/Locks/Perf/Statements/Long-running
- Edit: Copy row/Copy grid

UI Objects:
- Connection selector
- View selector
- Refresh
- Result grid

### 4) Users & Roles

Purpose:
- User/role catalog browsing and admin templates.

Menus (merge items):
- Edit: Copy grid/Copy row

UI Objects:
- Tabs: Users / Roles / Memberships
- Template actions: Create/Drop/Grant/Revoke
- Result grid

### 5) Job Scheduler

Purpose:
- Create/alter/drop jobs and review run history (ScratchBird only).

Menus (merge items):
- Window: Job Scheduler
- Edit: Copy row/Copy grid

UI Objects:
- Jobs grid (name/state/next run)
- Details panel (schedule, retries, timeout)
- Tabs: Runs / Dependencies / Config / Privileges

### 6) Diagram Window (ERD/Data Flow/UML)

Purpose:
- Visual modeling. May be stored under a Project schema.

Menus (merge items):
- Edit: Align/Distribute/Group/Ungroup/Order
- View: Zoom, Snap, Grid, Fit

UI Objects:
- Canvas
- Palette/toolbox
- Properties panel
- Navigator/minimap

Rules:
- Diagram windows may tab together, but cannot tab with non-diagram windows.

### 7) Project Workspace

Purpose:
- Application-level workspace stored as a schema with its own SQL objects.

Menus (merge items):
- View: Project tree / notes / activity

UI Objects:
- Project tree (diagrams, scripts, notes, tasks)
- Properties panel (owner, storage target, sync)
- Activity log

Notes:
- Projects may be local or shared (database/cluster).

### 8) Server Manager

Purpose:
- Create/manage local ScratchBird servers and emulated engines.

Menus (merge items):
- View: Server status/health

UI Objects:
- Server list
- Status/health panel
- Configuration pane (ports, listeners, pool)

Notes:
- Wizard-driven creation with local host analysis.

### 9) Cluster Manager (Beta)

Purpose:
- Cluster topology, health, failover policy.

UI Objects:
- Node map
- Health panel
- Policy controls

### 10) Database Manager

Purpose:
- Create/drop databases and emulated catalogs.

UI Objects:
- Database list
- Create/Drop controls
- Filespace/tablespace selectors (future)

### 11) Git Integration

Purpose:
- Configure local Git identity and connect to database/cluster Git services.

UI Objects:
- Identity settings
- Repo targets (local + DB/cluster)
- Status/Pull/Push panel

### 12) Backup & Restore (Phase 5)

Purpose:
- UI for sb_backup / restore workflows.

UI Objects:
- Source/Target pickers
- Schedule options
- Progress log

### 13) Preferences

Purpose:
- App-wide settings.

UI Objects:
- Theme/fonts
- Editor defaults
- Network timeouts
- Credential store status

### 14) Activity Log

Purpose:
- Long-running jobs and notifications.

UI Objects:
- Log timeline
- Filters

### 15) Startup/Branding Window

Purpose:
- Optional startup window to show branding, logo, and loading status.

UI Objects:
- Logo (centered)
- Background image or gradient
- Optional progress bar and status text

Behavior:
- Appears at startup while initialization runs.
- Dismisses on click or when the main window is ready.
- Can be disabled in preferences.

## Project Window Schema

Projects are first-class application schemas. A project creates its own SQL
objects, allows storage of diagrams, scripts, notes, and task metadata, and
may be hosted locally or in a shared database/cluster.

## Server/Cluster Creation Workflow

- A wizard + configuration window guides creation.
- The wizard can analyze the local host to suggest settings.
- Optional host agent (service) may enable remote setup/configuration even
  if no database is installed on the target host.

## Diagram Storage Rules

- Diagrams are local by default.
- Diagrams can be stored under a Project schema when connected.
- Diagram windows can be detached or tabbed within the diagram group.
