# Robin-Migrate Master Specification

## Document Information
- **Version**: 1.0
- **Date**: 2026-03-04
- **Status**: Draft
- **Purpose**: Comprehensive development specification for robin-migrate database GUI tool

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Phase Definitions](#phase-definitions)
3. [Phase 1: Core Infrastructure](#phase-1-core-infrastructure)
4. [Phase 2: SQL Editor & Execution](#phase-2-sql-editor--execution)
5. [Phase 3: Data Management](#phase-3-data-management)
6. [Phase 4: Advanced Features](#phase-4-advanced-features)
7. [Testing Strategy](#testing-strategy)
8. [Completion Criteria](#completion-criteria)

---

## Architecture Overview

### System Architecture
```
┌─────────────────────────────────────────────────────────────────┐
│                        UI Layer (wxWidgets)                      │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────┐ │
│  │  Main Frame  │ │ SQL Editor   │ │  Project Navigator       │ │
│  │  - Menu Bar  │ │  - Editor    │ │  - Tree View             │ │
│  │  - Toolbars  │ │  - Results   │ │  - Object Browser        │ │
│  └──────────────┘ └──────────────┘ └──────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    Window Framework Layer                        │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────┐ │
│  │Window Manager│ │ Action       │ │  Menu/Toolbar            │ │
│  │  - Docking   │ │  Registry    │ │  - Dynamic Menus         │ │
│  │  - Layout    │ └──────────────┘ └──────────────────────────┘ │
│  └──────────────┘                                              │
├─────────────────────────────────────────────────────────────────┤
│                    Core Services Layer                           │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────────┐ │
│  │   Settings   │ │   ScratchBird│ │   Icon/Resource          │ │
│  │   - Config   │ │   - Client   │ │   - SVG Icons            │ │
│  └──────────────┘ └──────────────┘ └──────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Technology Stack
- **UI Framework**: wxWidgets 3.2+
- **Language**: C++17/20
- **Build System**: CMake
- **Database Protocol**: ScratchBird (custom protocol)
- **Graphics**: SVG-based icons via wxBitmapBundle
- **Configuration**: JSON with Boost.JSON

---

## Phase Definitions

| Phase | Name | Duration | Goal |
|-------|------|----------|------|
| 1 | Core Infrastructure | 4-6 weeks | Functional app shell with connection management |
| 2 | SQL Editor & Execution | 6-8 weeks | Full SQL editor with execution and results |
| 3 | Data Management | 4-6 weeks | Import/export, object editing, migration |
| 4 | Advanced Features | 6-10 weeks | ERD, debugging, dashboards, AI |

---

## Phase 1: Core Infrastructure

### 1.1 Completion Criteria
At end of Phase 1, MUST have:
- [ ] Application launches and displays main window
- [ ] Menu bar with all standard menus
- [ ] Project navigator with lazy-loading tree
- [ ] Connection management (connect/disconnect)
- [ ] Basic preferences/settings dialog
- [ ] Toolbar with standard actions
- [ ] Status bar showing connection state

### 1.2 Main Application Window

**Window Type**: `main_container`  
**Class**: `MainFrame`  
**Size**: 1280x800 (min: 1024x600)

#### Menu Bar

**File Menu**:
```
New Connection...        Ctrl+N
Open SQL Script...       Ctrl+O
─────────────────────────
Save                     Ctrl+S
Save As...               Ctrl+Shift+S
─────────────────────────
Exit                     Ctrl+Q
```

**Edit Menu**:
```
Undo                     Ctrl+Z
Redo                     Ctrl+Shift+Z
─────────────────────────
Cut                      Ctrl+X
Copy                     Ctrl+C
Paste                    Ctrl+V
─────────────────────────
Preferences...           Ctrl+Comma
```

**View Menu**:
```
Project Navigator        [Toggle]
Properties Panel         [Toggle]
Output Panel             [Toggle]
─────────────────────────
Refresh                  F5
─────────────────────────
Fullscreen               F11
```

**Database Menu**:
```
Connect
Disconnect
Reconnect
─────────────────────────
New Database...
Register Database...
─────────────────────────
Properties...
```

**Help Menu**:
```
Documentation            F1
─────────────────────────
About...
```

#### Toolbar
```
[New] [Open] [Save] | [Cut] [Copy] [Paste] | [Connect] [Disconnect] | [Refresh]
```

#### Status Bar (4 panels)
1. Connection status: "Connected: database_name" / "Not connected"
2. Current user/schema
3. Operation status: Ready/Busy/Error
4. Current mode: Preview/Managed/Listener/IPC

### 1.3 Project Navigator Panel

**Window Type**: `project_navigator`  
**Dock**: Left (width: 300px)

**Tree Structure**:
```
Servers
└─ [+] Server Name
   ├─ [+] Databases
   │  └─ [+] database_name
   │     ├─ [+] Schemas
   │     │  └─ [+] public
   │     │     ├─ [+] Tables
   │     │     │  ├─ table1
   │     │     │  └─ table2
   │     │     ├─ [+] Views
   │     │     ├─ [+] Procedures
   │     │     └─ [+] Functions
   │     └─ [+] System Objects
   └─ [+] Users
```

**Context Menus**:

| Node | Menu Items |
|------|-----------|
| Server | Connect, Disconnect, New Database..., Properties... |
| Database | Connect, New Object (submenu), Backup..., Restore..., Properties... |
| Table | Browse Data, Edit Table..., Generate DDL, Export Data..., Properties... |

**Features**:
- Lazy loading (children loaded on expand)
- Progress indicator during load
- Search/filter box (real-time)
- Drag & drop support
- Inline rename (F2)

### 1.4 Connection Dialog

**Class**: `ConnectionDialog`

**Fields**:
```
Connection Name:     [________________]

Host:                [________________]
Port:                [____] (default: 4044)

Database:            [________________]

Authentication:      [Dropdown ▼]
Username:            [________________]
Password:            [________________]
[✓] Save password

[ Test Connection ]

[Advanced >>]

       [Cancel]    [Connect]    [Save]
```

**Advanced Options**:
- Connection timeout (default: 30s)
- Keep-alive interval
- SSL/TLS options
- SSH tunnel settings

**Validation**:
- Connection name: required, unique
- Host: required
- Port: numeric, 1-65535

### 1.5 Preferences Dialog

**Class**: `PreferencesDialog`
**Tabs**:

**General**:
- Language selection
- Theme selection
- Confirmations (delete, exit)
- Auto-save SQL scripts

**Editor**:
- Font selection
- Tab size
- Word wrap
- Show line numbers
- Highlight current line

**Connection**:
- Default timeout
- Keep-alive settings
- Auto-connect on startup

### 1.6 Phase 1 Tasks

| ID | Task | Acceptance Criteria |
|----|------|---------------------|
| 1.1 | MainFrame | Window shows, menu bar functional |
| 1.2 | MenuRegistry | Menus populated from ActionCatalog |
| 1.3 | ToolbarManager | Standard toolbar shows, buttons work |
| 1.4 | StatusBar | 4 panels display correct info |
| 1.5 | Settings persistence | Settings save/load from JSON |
| 1.6 | Lazy loading tree | Tree expands, loads on demand |
| 1.7 | Tree filtering | Filter box filters in real-time |
| 1.8 | Context menus | Right-click shows appropriate menu |
| 1.9 | ConnectionDialog | Dialog validates, saves connections |
| 1.10 | Connection registry | Connections persist, load on startup |
| 1.11 | Secure credentials | Passwords encrypted |
| 1.12 | Connection state tracking | UI reflects connection state |

### 1.7 Phase 1 Testing

**Unit Tests**:
- [ ] ActionCatalog returns correct actions
- [ ] MenuRegistry builds menus correctly
- [ ] Settings serialization/deserialization
- [ ] Connection configuration validation

**Integration Tests**:
- [ ] Application launches without errors
- [ ] Menu items execute correct actions
- [ ] Tree loads and displays objects
- [ ] Connect/Disconnect updates UI state

---

## Phase 2: SQL Editor & Execution

### 2.1 Completion Criteria
At end of Phase 2:
- [ ] SQL editor with syntax highlighting
- [ ] Code completion (tables, keywords, functions)
- [ ] Execute statement/selection/script
- [ ] Grid and record result views
- [ ] Query history tracking
- [ ] Result export (CSV, SQL)
- [ ] Explain plan visualization

### 2.2 SQL Editor Window

**Window Type**: `sql_editor`  
**Class**: `SQLEditor`  
**Dock**: Center (tabbed)

**Layout**:
```
┌─────────────────────────────────────────────────────────────────┐
│ [Execute] [Execute Sel] [Execute Script] [Explain] [Stop]      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1  ┌───────────────────────────────────────────────────────┐  │
│  2  │ SELECT u.id, u.name, o.total                          │  │
│  3  │ FROM users u                                          │  │
│  4  │ JOIN orders o ON u.id = o.user_id                     │  │
│  5  │ WHERE u.active = true                                 │  │
│  6  │ ORDER BY u.name;                                      │  │
│     └───────────────────────────────────────────────────────┘  │
│                                                                 │
├─────────────────────────────────────────────────────────────────┤
│ Results | Query Plan | Output                                   │
├─────────────────────────────────────────────────────────────────┤
│ Grid view or Record view displays here                          │
├─────────────────────────────────────────────────────────────────┤
│ Status: Ready | Line 3, Col 15 | Duration: -- | Rows: --       │
└─────────────────────────────────────────────────────────────────┘
```

**Editor Features**:
- Syntax highlighting (keywords, strings, comments, numbers)
- Line numbers
- Current line highlighting
- Bracket matching (visual)
- Code folding
- Auto-indentation
- Code completion (Ctrl+Space)
- Occurrences highlighting
- Word wrap toggle
- Zoom (Ctrl++, Ctrl+-)

**Toolbar Actions**:
| Action | Shortcut | Description |
|--------|----------|-------------|
| Execute | F9 | Run current statement |
| Execute Selection | Ctrl+F9 | Run selected SQL only |
| Execute Script | | Run entire script |
| Explain Plan | | Show execution plan |
| Stop | | Cancel running query |
| Format SQL | | Auto-format code |
| Find/Replace | Ctrl+F | Search in editor |

### 2.3 Results Panel

**Window Type**: `results_panel`  
**Dock**: Bottom of SQL editor

**Grid Tab**:
```
┌──────────────────────────────────────────────────────────┐
│ [Export] [Filter] [Refresh] [Pin]                       │
├──────────────────────────────────────────────────────────┤
│ ▼ id │ ▼ name    │ ▼ total  │ ▼ created                 │
├──────┼───────────┼──────────┼───────────────────────────┤
│  1   │ Alice     │ 1500.00  │ 2026-01-15                │
│  2   │ Bob       │ 2300.50  │ 2026-01-16                │
└──────┴───────────┴──────────┴───────────────────────────┘
Page: 1 of 5 | Rows: 1-100 of 487
```

**Features**:
- Column headers with sort (click to sort)
- Resizable columns
- Copy cell/row/selection
- Column width auto-fit
- Pagination
- Row count display
- Virtual mode for large datasets

**Record Tab**:
```
┌────────────────────────────────────┐
│ Record 1 of 487                    │
│ [< Prev]           [Next >]        │
├────────────────────────────────────┤
│ Field         │ Value              │
├───────────────┼────────────────────┤
│ id            │ 1                  │
│ name          │ Alice              │
│ total         │ 1500.00            │
│ created       │ 2026-01-15         │
└───────────────┴────────────────────┘
```

### 2.4 Phase 2 Tasks

| ID | Task | Acceptance Criteria |
|----|------|---------------------|
| 2.1 | wxStyledTextCtrl integration | Editor displays, accepts input |
| 2.2 | Syntax highlighting | Keywords colored correctly |
| 2.3 | Code completion | Popup suggests tables/columns |
| 2.4 | Bracket matching | Matching brackets highlighted |
| 2.5 | Code folding | Can collapse/expand blocks |
| 2.6 | SQL formatting | Format button formats SQL |
| 2.7 | Statement detection | Identifies current statement |
| 2.8 | Execute statement | Runs and displays results |
| 2.9 | Stop execution | Cancel button stops query |
| 2.10 | Error handling | Shows error with line number |
| 2.11 | Query history | Logs all executions |
| 2.12 | Explain plan | Shows execution plan tree |
| 2.13 | Grid view | Data displays in grid |
| 2.14 | Record view | Single record display works |
| 2.15 | Sorting | Click column sorts |
| 2.16 | Result export | Exports to CSV/SQL |
| 2.17 | Pagination | Handles large result sets |

### 2.5 Phase 2 Testing

**Performance Tests**:
- [ ] Editor handles 10000 line scripts
- [ ] Grid displays 10000 rows smoothly
- [ ] Export handles 100000 rows
- [ ] Code completion < 500ms

---

## Phase 3: Data Management

### 3.1 Completion Criteria
At end of Phase 3:
- [ ] Table data editor with in-place editing
- [ ] Import data (CSV, SQL)
- [ ] Export data (multiple formats)
- [ ] Object DDL generation
- [ ] Database-to-database migration
- [ ] Table designer (create/alter)

### 3.2 Table Data Editor

**Window Type**: `table_data_editor`  
**Class**: `TableDataEditor`  
**Dock**: Center (tabbed)

```
┌──────────────────────────────────────────────────────────────┐
│ Table: users                    [Save] [Refresh] [Filter]   │
├──────────────────────────────────────────────────────────────┤
│ ▼ id │ ▼ name    │ ▼ email         │ ▼ active │ ▼ created   │
├──────┼───────────┼─────────────────┼──────────┼─────────────┤
│  1   │ Alice     │ alice@email.com │ ☑        │ 2026-01-15  │
│  2   │ Bob       │ bob@email.com   │ ☐        │ 2026-01-16  │
│ *3   │ Charlie   │                 │ ☑        │ [NULL]      │
└──────┴───────────┴─────────────────┴──────────┴─────────────┘
* = modified row

[First] [Prev] [Next] [Last]    Page 1 of 50    [Add Row] [Delete Row]
```

**Features**:
- In-place cell editing
- Add/delete rows
- Checkbox for boolean columns
- Dropdown for foreign keys
- Date picker for dates
- Modified row indicators (*)
- Commit/Rollback buttons
- Column filtering

### 3.3 Table Designer

**Window Type**: `table_editor`  
**Tabs**: Columns | Constraints | Indexes | SQL Preview

**Columns Tab**:
```
│ Column Name │ Data Type │ Length │ Nullable │ Default      │
├─────────────┼───────────┼────────┼──────────┼──────────────┤
│ id          │ INTEGER   │        │ [ ]      │              │
│ name        │ VARCHAR   │ 255    │ [✓]      │              │
│ email       │ VARCHAR   │ 255    │ [✓]      │              │
│ active      │ BOOLEAN   │        │ [✓]      │ true         │
```

### 3.4 Migration Wizard

**Class**: `MigrationWizard`  
**Steps**:
1. Source (select connection, objects)
2. Target (select connection, schema)
3. Mapping (table/column mappings)
4. Options (create tables, truncate, batch size)
5. Execution (progress, cancel)

### 3.5 Phase 3 Tasks

| ID | Task | Acceptance Criteria |
|----|------|---------------------|
| 3.1 | Grid editor | In-place editing works |
| 3.2 | Row operations | Add/delete rows |
| 3.3 | Cell editors | Boolean, date, dropdown |
| 3.4 | Commit/rollback | Changes saved/discarded |
| 3.5 | CSV import | Imports CSV correctly |
| 3.6 | SQL import | Runs SQL script |
| 3.7 | Multi-format export | JSON, Excel support |
| 3.8 | Table designer | Can design tables |
| 3.9 | DDL generation | Generates correct SQL |
| 3.10 | Migration wizard | Data copies correctly |

---

## Phase 4: Advanced Features

### 4.1 Completion Criteria
At end of Phase 4:
- [ ] ERD visualization with auto-layout
- [ ] SQL debugging (breakpoints, step-through)
- [ ] Dashboard with SQL-driven charts
- [ ] AI-powered SQL generation

### 4.2 ERD Viewer

**Window Type**: `erd_viewer`  
**Class**: `ERDEditor`

**Features**:
- Auto-layout algorithm
- Table boxes with columns
- FK relationship lines
- Crow's Foot notation
- Zoom and pan
- Export to PNG/SVG

### 4.3 SQL Debugger

**Window Type**: `sql_debugger`  
**Class**: `SQLDebugger`

**Features**:
- Set/remove breakpoints
- Step Into/Over/Return
- Variable inspection
- Call stack panel

### 4.4 Dashboard

**Window Type**: `dashboard_panel`  
**Class**: `DashboardPanel`

**Features**:
- SQL-driven charts
- Time series graphs
- Real-time updates
- Line, Bar, Pie charts

### 4.5 Phase 4 Tasks

| ID | Task | Acceptance Criteria |
|----|------|---------------------|
| 4.1 | ERD auto-layout | Tables arranged clearly |
| 4.2 | Relationship lines | FK connections drawn |
| 4.3 | Breakpoints | Can set/remove breakpoints |
| 4.4 | Step-through | Step Into/Over works |
| 4.5 | Variable inspection | Shows variable values |
| 4.6 | SQL charts | Charts from SQL queries |
| 4.7 | Real-time updates | Charts refresh automatically |
| 4.8 | AI SQL generation | Natural language to SQL |

---

## Testing Strategy

### Test Types

| Type | Purpose | Target |
|------|---------|--------|
| Unit | Individual classes | 80% coverage |
| Integration | Component interaction | Critical paths |
| UI | User interactions | Happy paths |
| Performance | Scalability | Benchmarks |

### Phase Test Requirements

**Phase 1**:
- Unit: Settings, ActionCatalog, MenuRegistry
- Integration: Launch, connection management
- UI: Menu navigation, dialogs

**Phase 2**:
- Unit: SQL parser, completion engine
- Integration: Query execution, results
- Performance: 10k lines, 100k rows

**Phase 3**:
- Unit: Import parsers, DDL generator
- Integration: Import/export, migration
- Performance: 1M row export

---

## Completion Criteria Summary

### Phase 1
- [ ] Main window, menus, toolbars, status bar
- [ ] Project navigator with lazy loading
- [ ] Connection dialog with validation
- [ ] Settings persistence

### Phase 2
- [ ] SQL editor with highlighting, completion
- [ ] Execute query with grid/record views
- [ ] Query history, export
- [ ] Explain plan

### Phase 3
- [ ] Table data editor
- [ ] Import/Export wizards
- [ ] Table designer
- [ ] Migration wizard

### Phase 4
- [ ] ERD visualization
- [ ] SQL debugging
- [ ] Dashboard with charts
- [ ] AI integration
