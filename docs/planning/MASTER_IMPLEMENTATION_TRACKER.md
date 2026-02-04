# ScratchRobin Master Implementation Tracker

**Status**: Active Planning Document  
**Created**: 2026-02-03  
**Last Updated**: 2026-02-03 (ALL PHASE 3 ITEMS COMPLETE - Phase 1, 2, 3 now 100%)  
**Scope**: Full ScratchRobin GUI implementation to manage ScratchBird database

---

## Executive Summary

This document provides a comprehensive, prioritized implementation plan for expanding ScratchRobin from its current template state into a full-featured database administration tool for ScratchBird. The plan is organized into phases with clear dependencies, acceptance criteria, and status tracking.

### Current State Summary

| Component | Status | Notes |
|-----------|--------|-------|
| Core Framework | ✅ Complete | SDI windows, menu system, connection backends |
| SQL Editor | ✅ Complete | Async execution, results grid, export |
| Main Window | ✅ Complete | Catalog tree, inspector panels |
| Object Managers | ✅ Complete | All managers wired to backend |
| ERD/Diagramming | ✅ Complete | 4 notations, auto-layout, forward/reverse engineering |
| Connection Editor | ✅ Complete | Full editor with test connection, SSL, all backends |

### Legend

| Symbol | Status |
|--------|--------|
| ✅ | Completed |
| 🟡 | In Progress / Partial |
| 🔴 | Not Started |
| ⏸️ | Blocked / Deferred |

---

## Phase 1: Foundation Layer (P0 - Critical)

**Goal**: Establish essential infrastructure that unblocks all other work.  
**Timeline**: 2-3 weeks  
**Dependencies**: None

### 1.1 Connection Profile Editor

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 1.1.1 | Create connection editor dialog UI | ✅ | P0 | 2d | - | Dialog with tabs for each backend type |
| 1.1.2 | Implement ScratchBird connection form | ✅ | P0 | 1d | 1.1.1 | Host, port, database, SSL mode fields |
| 1.1.3 | Implement PostgreSQL connection form | ✅ | P0 | 1d | 1.1.1 | Standard PG connection parameters |
| 1.1.4 | Implement MySQL connection form | ✅ | P0 | 1d | 1.1.1 | Standard MySQL connection parameters |
| 1.1.5 | Implement Firebird connection form | ✅ | P0 | 1d | 1.1.1 | Database path, auth parameters |
| 1.1.6 | Add credential store integration | ✅ | P0 | 2d | 1.1.1 | Full CRUD with libsecret (Store, Delete, HasPassword methods) |
| 1.1.7 | Add connection test workflow | ✅ | P0 | 1d | 1.1.2-1.1.6 | Test button with success/failure feedback |
| 1.1.8 | Add SSL/TLS configuration panel | ✅ | P0 | 2d | 1.1.1 | CA cert, client cert, verify mode options |
| 1.1.9 | Implement profile CRUD operations | ✅ | P0 | 2d | 1.1.1 | Create, edit, duplicate, delete profiles |
| 1.1.10 | Wire into main window Connections menu | ✅ | P0 | 0.5d | 1.1.9 | Menu items enabled and functional |

**Specification**: `docs/specifications/CONNECTION_PROFILE_EDITOR.md` (to be created)  
**Implementation**: `src/ui/connection_editor_dialog.h`, `src/ui/connection_editor_dialog.cpp`  
**Completed**: 2026-02-03

### 1.2 Transaction Management Specification & Implementation

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 1.2.1 | Write transaction management spec | ✅ | P0 | 1d | - | Document covering states, UI indicators, isolation levels |
| 1.2.2 | Implement transaction state tracking | ✅ | P0 | 1d | 1.2.1 | Track auto-commit vs explicit transaction state |
| 1.2.3 | Add transaction indicator to SQL Editor | ✅ | P0 | 1d | 1.2.2 | Visual indicator showing transaction state |
| 1.2.4 | Wire Begin/Commit/Rollback buttons | ✅ | P0 | 0.5d | 1.2.2 | Buttons execute correct SQL commands |
| 1.2.5 | Add uncommitted transaction warning on close | ✅ | P0 | 1d | 1.2.2 | Dialog with Commit/Rollback/Cancel options |
| 1.2.6 | Add isolation level selector | ✅ | P1 | 1d | 1.2.2 | Dropdown for READ COMMITTED, SERIALIZABLE, etc. |
| 1.2.7 | Implement savepoint support | ✅ | P2 | 2d | 1.2.2 | Create, rollback to, release savepoints with SQL templates per backend |

**Specification**: `docs/specifications/TRANSACTION_MANAGEMENT.md` (COMPLETE)  
**Implementation**: `src/ui/sql_editor_frame.h`, `src/ui/sql_editor_frame.cpp`  
**Completed**: 2026-02-03

### 1.3 Error Handling Framework

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 1.3.1 | Create error classification system | ✅ | P0 | 1d | - | Enum/classification for error types |
| 1.3.2 | Define ScratchRobin error code namespace | ✅ | P0 | 0.5d | 1.3.1 | SR-XXXX format documented |
| 1.3.3 | Create error dialog component | ✅ | P0 | 1d | 1.3.1 | Reusable error display with details expansion |
| 1.3.4 | Map backend error codes to categories | ✅ | P0 | 2d | 1.3.1 | PostgreSQL, MySQL, Firebird error mapping |
| 1.3.5 | Add error logging infrastructure | ✅ | P1 | 1d | 1.3.1 | Logging with file rotation |
| 1.3.6 | Surface server notices in UI | ✅ | P1 | 1d | - | QueryResult::messages integration, AppendMessages() display |

**Specification**: `docs/specifications/ERROR_HANDLING.md` (COMPLETE)  
**Implementation**: `src/core/error_handler.h`, `src/core/error_handler.cpp`, `src/ui/error_dialog.h`, `src/ui/error_dialog.cpp`  
**Completed**: 2026-02-03

### 1.4 Capability Flags System

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 1.4.1 | Define capability flag enumeration | ✅ | P0 | 0.5d | - | Flags for features (jobs, domains, etc.) |
| 1.4.2 | Implement server capability detection | ✅ | P0 | 1d | 1.4.1 | Query server version/features on connect |
| 1.4.3 | Wire capability flags to UI enablement | ✅ | P0 | 2d | 1.4.2 | Menu items/windows disabled when unsupported |
| 1.4.4 | Add capability matrix documentation | ✅ | P0 | 0.5d | 1.4.1 | Matrix showing features per backend type |

**Implementation**: `src/core/connection_backend.h`, `src/core/capability_detector.h`, `src/core/capability_detector.cpp`  
**Completed**: 2026-02-03

---

## Phase 2: Object Manager Wiring (P0 - Critical) - COMPLETE

**Goal**: Connect existing UI stubs to ScratchBird backend.  
**Timeline**: 3-4 weeks  
**Status**: ✅ COMPLETE  
**Completed**: 2026-02-03

### 2.1 Table Designer Completion

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.1.1 | Verify table list query works | ✅ | P0 | 0.5d | - | `SELECT name, schema_name FROM sb_catalog.sb_tables` |
| 2.1.2 | Implement async table list loading | ✅ | P0 | 1d | 2.1.1 | Non-blocking load with progress indicator |
| 2.1.3 | Wire SHOW CREATE TABLE for details | ✅ | P0 | 0.5d | 2.1.2 | Display table DDL in details panel |
| 2.1.4 | Wire SHOW COLUMNS for columns tab | ✅ | P0 | 0.5d | 2.1.2 | Column grid populated from SHOW COLUMNS |
| 2.1.5 | Implement CREATE TABLE dialog | ✅ | P0 | 2d | 2.1.1 | Dialog with columns/constraints input |
| 2.1.6 | Implement ALTER TABLE dialog | ✅ | P0 | 2d | 2.1.1 | Action selector for ALTER operations |
| 2.1.7 | Implement DROP TABLE with cascade option | ✅ | P0 | 0.5d | 2.1.1 | Confirmation dialog with CASCADE/RESTRICT |
| 2.1.8 | Add table refresh after DDL operations | ✅ | P0 | 0.5d | 2.1.5-2.1.7 | Auto-refresh table list after changes |
| 2.1.9 | Add table search/filter | ✅ | P1 | 1d | 2.1.2 | Filter box with real-time filtering |
| 2.1.10 | Add GRANT/REVOKE UI | ✅ | P2 | 0.5d | 2.1.1 | Full dialog with privilege checklist for tables |

**Specification**: `docs/specifications/TABLE_DESIGNER_UI.md`  
**Implementation**: `src/ui/table_designer_frame.h/cpp` - Full backend integration complete

### 2.2 Index Designer Completion

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.2.1 | Verify index list query works | ✅ | P0 | 0.5d | - | `SELECT name, schema_name, table_name FROM sb_catalog.sb_indexes` |
| 2.2.2 | Implement async index list loading | ✅ | P0 | 1d | 2.2.1 | Non-blocking load with progress |
| 2.2.3 | Wire SHOW INDEX for details | ✅ | P0 | 0.5d | 2.2.2 | Index definition display |
| 2.2.4 | Wire SHOW INDEXES FROM table | ✅ | P0 | 0.5d | 2.2.2 | Column-level index info grid |
| 2.2.5 | Implement CREATE INDEX dialog | ✅ | P0 | 2d | 2.2.1 | Index type selector, columns, WHERE clause |
| 2.2.6 | Implement ALTER INDEX dialog | ✅ | P0 | 1d | 2.2.1 | RENAME, SET TABLESPACE, REBUILD |
| 2.2.7 | Implement DROP INDEX | ✅ | P0 | 0.5d | 2.2.1 | Confirmation dialog |
| 2.2.8 | Add index type icons | ✅ | P2 | 1d | 2.2.2 | Visual distinction for B-Tree, GIN, etc. |
| 2.2.9 | Add index usage statistics | ✅ | P2 | 1d | 2.2.2 | Usage Statistics tab in Index Designer with scan counts |

**Specification**: `docs/specifications/INDEX_DESIGNER_UI.md`  
**Implementation**: `src/ui/index_designer_frame.h/cpp` - Full backend integration complete

### 2.3 Schema Manager Completion

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.3.1 | Implement SHOW SCHEMAS query | ✅ | P0 | 0.5d | - | List all schemas |
| 2.3.2 | Implement async schema list loading | ✅ | P0 | 1d | 2.3.1 | Grid with schema names |
| 2.3.3 | Wire SHOW SCHEMA for details | ✅ | P0 | 0.5d | 2.3.2 | Schema properties display |
| 2.3.4 | Implement CREATE SCHEMA dialog | ✅ | P0 | 1d | 2.3.1 | Name, authorization inputs |
| 2.3.5 | Implement ALTER SCHEMA dialog | ✅ | P0 | 1d | 2.3.1 | RENAME, OWNER TO, SET PATH |
| 2.3.6 | Implement DROP SCHEMA with CASCADE | ✅ | P0 | 0.5d | 2.3.1 | Cascade/Restrict options |
| 2.3.7 | Add schema object count display | ✅ | P1 | 1d | 2.3.2 | Table, view, function counts per schema |

**Specification**: `docs/specifications/SCHEMA_MANAGER_UI.md`  
**Implementation**: `src/ui/schema_manager_frame.h/cpp` - Full backend integration complete

### 2.4 Domain Manager Completion

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.4.1 | Implement domain list query | ✅ | P0 | 0.5d | - | `SELECT name FROM sb_catalog.sb_domains` |
| 2.4.2 | Implement async domain list loading | ✅ | P0 | 1d | 2.4.1 | Grid with domain names |
| 2.4.3 | Wire SHOW DOMAIN for details | ✅ | P0 | 0.5d | 2.4.2 | Domain properties display |
| 2.4.4 | Implement CREATE DOMAIN dialog | ✅ | P0 | 2d | 2.4.1 | BASIC, RECORD, ENUM, SET, VARIANT types |
| 2.4.5 | Implement ALTER DOMAIN dialog | ✅ | P0 | 1d | 2.4.1 | SET DEFAULT, ADD CHECK, RENAME |
| 2.4.6 | Implement DROP DOMAIN | ✅ | P0 | 0.5d | 2.4.1 | Confirmation with dependency check |
| 2.4.7 | Add domain usage reference | ✅ | P1 | 1d | 2.4.2 | Show tables/columns using domain |

**Specification**: `docs/specifications/DOMAIN_MANAGER_UI.md`  
**Implementation**: `src/ui/domain_manager_frame.h/cpp` - Full backend integration complete

### 2.5 Job Scheduler Completion

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.5.1 | Implement SHOW JOBS query | ✅ | P0 | 0.5d | - | List all jobs |
| 2.5.2 | Implement async job list loading | ✅ | P0 | 1d | 2.5.1 | Grid with name, state, schedule, next run |
| 2.5.3 | Wire SHOW JOB for details | ✅ | P0 | 0.5d | 2.5.2 | Job definition display |
| 2.5.4 | Implement SHOW JOB RUNS query | ✅ | P0 | 0.5d | 2.5.1 | Run history grid |
| 2.5.5 | Implement CREATE JOB dialog | ✅ | P0 | 3d | 2.5.1 | CRON/AT/EVERY schedule builder, job body |
| 2.5.6 | Implement ALTER JOB dialog | ✅ | P0 | 2d | 2.5.1 | Modify schedule, state, properties |
| 2.5.7 | Implement DROP JOB | ✅ | P0 | 0.5d | 2.5.1 | KEEP HISTORY option |
| 2.5.8 | Implement EXECUTE JOB | ✅ | P0 | 0.5d | 2.5.1 | Run now button |
| 2.5.9 | Implement CANCEL JOB RUN | ✅ | P0 | 0.5d | 2.5.4 | Cancel running job |
| 2.5.10 | Add job dependencies visualization | ✅ | P1 | 2d | 2.5.1 | Tree/graph of job dependencies |
| 2.5.11 | Add scheduler config panel | ✅ | P1 | 1d | 2.5.1 | Full config panel with enable, max-concurrent, poll-interval, timezone |

**Specification**: `docs/specifications/JOB_SCHEDULER_UI.md`  
**Implementation**: `src/ui/job_scheduler_frame.h/cpp` - Full backend integration complete

### 2.6 Users & Roles Enhancement

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 2.6.1 | Add ScratchBird native user queries | ✅ | P0 | 1d | - | Query `sys.users`, `sys.roles` for all backends |
| 2.6.2 | Implement CREATE USER dialog | ✅ | P0 | 1d | 2.6.1 | UserEditorDialog with backend-specific SQL |
| 2.6.3 | Implement ALTER USER dialog | ✅ | P0 | 1d | 2.6.1 | UserEditorDialog Alter mode |
| 2.6.4 | Implement DROP USER | ✅ | P0 | 0.5d | 2.6.1 | Confirmation dialog with wxMessageBox |
| 2.6.5 | Implement CREATE ROLE dialog | ✅ | P0 | 1d | 2.6.1 | RoleEditorDialog with LOGIN/NOLOGIN options |
| 2.6.6 | Implement GRANT/REVOKE UI | ✅ | P1 | 2d | 2.6.1 | PrivilegeEditorDialog for table/schema/object privileges |
| 2.6.7 | Add membership management | ✅ | P1 | 1d | 2.6.1 | Grant/Revoke membership buttons in UsersRolesFrame |

---

## Phase 3: ERD and Diagramming System (P0 - Critical) ✅ COMPLETE

**Goal**: Implement full ERD capabilities for database modeling.  
**Timeline**: 6-8 weeks  
**Dependencies**: Phase 2 complete  
**Status**: ✅ COMPLETE  
**Completed**: 2026-02-03

**Implementation Files**:
```
src/diagram/
├── erd_notation.h           # ERD notation type definitions
├── command.h/cpp            # Command pattern for undo/redo
├── layout_engine.h/cpp      # Auto-layout algorithms (Sugiyama, Force-directed, Orthogonal)
├── reverse_engineer.h/cpp   # DB to diagram import (ScratchBird, PG, MySQL, FB)
├── forward_engineer.h/cpp   # Diagram to DDL export (4 backends)
└── export_manager.h/cpp     # PNG/SVG/PDF export
```

### 3.1 ERD Core Infrastructure ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.1.1 | Write ERD notation symbol dictionaries | ✅ | P0 | 2d | - | Crow's Foot, IDEF1X, UML, Chen specs |
| 3.1.2 | Design diagram data model | ✅ | P0 | 2d | 3.1.1 | JSON schema for .sberd files |
| 3.1.3 | Implement diagram document class | ✅ | P0 | 2d | 3.1.2 | Load/save .sberd files |
| 3.1.4 | Create diagram canvas widget | ✅ | P0 | 5d | 3.1.3 | wxWidgets canvas with scroll/zoom |
| 3.1.5 | Implement entity rendering | ✅ | P0 | 3d | 3.1.4 | Draw entity boxes with attributes |
| 3.1.6 | Implement relationship rendering | ✅ | P0 | 3d | 3.1.4 | Draw lines with cardinality symbols |
| 3.1.7 | Add selection and drag-drop | ✅ | P0 | 2d | 3.1.5-3.1.6 | Select, move, resize entities |
| 3.1.8 | Add zoom and pan controls | ✅ | P0 | 1d | 3.1.4 | Zoom 10%-400%, pan canvas |
| 3.1.9 | Implement grid and snap | ✅ | P1 | 1d | 3.1.4 | Configurable grid, snap-to-grid |
| 3.1.10 | Add minimap/navigation panel | ✅ | P2 | 2d | 3.1.4 | DiagramMinimap with viewport rectangle and click-to-navigate |

### 3.2 Notation Renderers ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.2.1 | Implement Crow's Foot notation | ✅ | P0 | 3d | 3.1.5-3.1.6 | Complete symbol set rendering |
| 3.2.2 | Implement IDEF1X notation | ✅ | P1 | 3d | 3.1.5-3.1.6 | Independent/dependent entities, relationships |
| 3.2.3 | Implement UML Class notation | ✅ | P1 | 3d | 3.1.5-3.1.6 | Class boxes, associations, inheritance |
| 3.2.4 | Implement Chen notation | ✅ | P2 | 3d | 3.1.5-3.1.6 | Entities, relationships, attributes as ovals |
| 3.2.5 | Implement Silverston notation | ✅ | P2 | 4d | 3.1.5-3.1.6 | Pattern level indicators |
| 3.2.6 | Add notation switcher UI | ✅ | P1 | 1d | 3.2.1-3.2.3 | Dropdown to change notation |
| 3.2.7 | Create notation test diagrams | ✅ | P1 | 1d | 3.2.1-3.2.3 | NotationTestDialog with Crow's Foot, IDEF1X, UML, Chen tabs |

### 3.3 Diagram Editing ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.3.1 | Write undo/redo specification | ✅ | P0 | 1d | - | Command pattern design |
| 3.3.2 | Implement command pattern framework | ✅ | P0 | 2d | 3.3.1 | Command base class, history |
| 3.3.3 | Implement add/remove entity commands | ✅ | P0 | 1d | 3.3.2 | Undoable entity creation/deletion |
| 3.3.4 | Implement move/resize commands | ✅ | P0 | 1d | 3.3.2 | Undoable position/size changes |
| 3.3.5 | Implement relationship commands | ✅ | P0 | 1d | 3.3.2 | Add/remove relationships |
| 3.3.6 | Implement attribute editing commands | ✅ | P1 | 2d | 3.3.2 | Add/remove/modify attributes |
| 3.3.7 | Add multi-select support | ✅ | P1 | 1d | 3.1.7 | Shift+click, marquee selection |
| 3.3.8 | Add copy/paste | ✅ | P1 | 2d | 3.3.7 | Copy/Paste with clipboard storage in DiagramCanvas |
| 3.3.9 | Add alignment tools | ✅ | P2 | 1d | 3.3.7 | Align left/right/top/bottom/center + distribute horizontal/vertical |
| 3.3.10 | Implement grouping | ✅ | P2 | 2d | 3.3.7 | DiagramGrouping with create/ungroup/add/remove/move operations |

### 3.4 Auto-Layout ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.4.1 | Write auto-layout specification | ✅ | P1 | 1d | - | Sugiyama method parameters |
| 3.4.2 | Implement Sugiyama layout algorithm | ✅ | P1 | 5d | 3.4.1 | Layered hierarchical layout |
| 3.4.3 | Implement force-directed layout | ✅ | P2 | 4d | 3.4.1 | Spring-based layout |
| 3.4.4 | Implement orthogonal layout | ✅ | P2 | 4d | 3.4.1 | Right-angle routing |
| 3.4.5 | Add layout options dialog | ✅ | P1 | 1d | 3.4.2 | LayoutOptionsDialog with algorithm, direction, spacing settings |
| 3.4.6 | Add pin/unpin for nodes | ✅ | P2 | 0.5d | 3.4.2 | DiagramCanvas Pin/Unpin/Toggle methods with pinned field in DiagramNode |
| 3.4.7 | Integrate Graphviz as optional engine | ✅ | P3 | 3d | 3.4.2 | GraphvizLayoutEngine with dot subprocess integration |

### 3.5 Reverse Engineering (Database → Diagram) ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.5.1 | Implement schema reading | ✅ | P0 | 2d | 3.1.3 | Query tables, columns, constraints |
| 3.5.2 | Create entity from table | ✅ | P0 | 1d | 3.5.1 | Convert table to diagram entity |
| 3.5.3 | Create relationships from FKs | ✅ | P0 | 2d | 3.5.1 | Convert foreign keys to relationships |
| 3.5.4 | Add reverse engineer wizard | ✅ | P0 | 2d | 3.5.2-3.5.3 | ReverseEngineerWizard with schema/table selection and progress dialog |
| 3.5.5 | Add incremental refresh | ✅ | P1 | 2d | 3.5.4 | IncrementalRefreshDialog with schema comparison and selective apply |
| 3.5.6 | Handle schema changes (diff) | ✅ | P2 | 3d | 3.5.5 | IncrementalRefreshDialog shows schema changes with apply/reject |
| 3.5.7 | Support external backends | ✅ | P1 | 2d | 3.5.1 | PG/MySQL/FB reverse engineering |

### 3.6 Forward Engineering (Diagram → DDL) ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.6.1 | Write data type mapping specification | ✅ | P0 | 2d | - | Cross-dialect type mappings |
| 3.6.2 | Implement DDL generator base | ✅ | P0 | 2d | 3.6.1 | Framework for dialect-specific gen |
| 3.6.3 | Implement ScratchBird DDL generator | ✅ | P0 | 3d | 3.6.2 | CREATE TABLE, INDEX, etc. |
| 3.6.4 | Implement PostgreSQL DDL generator | ✅ | P1 | 2d | 3.6.2 | PG-compatible DDL |
| 3.6.5 | Implement MySQL DDL generator | ✅ | P1 | 2d | 3.6.2 | MySQL-compatible DDL |
| 3.6.6 | Implement Firebird DDL generator | ✅ | P1 | 2d | 3.6.2 | Firebird-compatible DDL |
| 3.6.7 | Add DDL preview dialog | ✅ | P0 | 1d | 3.6.3 | DdlPreviewDialog with copy/save/execute options |
| 3.6.8 | Add DDL execution | ✅ | P0 | 1d | 3.6.7 | DdlExecutionDialog with progress and error reporting |
| 3.6.9 | Add migration script generation | ✅ | P2 | 2d | 3.6.3 | MigrationGenerator and MigrationDialog for upgrade/downgrade scripts |

### 3.7 Diagram Export/Import ✅

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 3.7.1 | Implement PNG export | ✅ | P1 | 1d | 3.1.4 | Export to PNG at specified DPI |
| 3.7.2 | Implement SVG export | ✅ | P1 | 2d | 3.1.4 | Scalable vector export |
| 3.7.3 | Implement PDF export | ✅ | P2 | 2d | 3.7.2 | PDF generation (HTML-based) |
| 3.7.4 | Add print support | ✅ | P2 | 1d | 3.1.4 | DiagramPrintDialog with DiagramPrintout, preview, and page setup |
| 3.7.5 | Add export options dialog | ✅ | P1 | 0.5d | 3.7.1 | ExportOptionsDialog with format, scope, DPI options |

---

## Phase 4: Additional Object Managers (P1 - Important)

**Goal**: Complete coverage of ScratchBird database objects.  
**Timeline**: 3-4 weeks  
**Dependencies**: Phase 2 complete

### 4.1 Sequence Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 4.1.1 | Create Sequence Manager spec | ✅ | P1 | 0.5d | - | UI design document |
| 4.1.2 | Create sequence list query | ✅ | P1 | 0.5d | - | Query `sb_catalog.sb_sequences` |
| 4.1.3 | Implement Sequence Manager frame | ✅ | P1 | 2d | 4.1.1-4.1.2 | Window with sequence grid |
| 4.1.4 | Implement CREATE SEQUENCE dialog | ✅ | P1 | 1d | 4.1.3 | Start, increment, min, max, cycle |
| 4.1.5 | Implement ALTER SEQUENCE dialog | ✅ | P1 | 1d | 4.1.3 | Modify sequence properties |
| 4.1.6 | Implement DROP SEQUENCE | ✅ | P1 | 0.5d | 4.1.3 | Confirmation dialog |
| 4.1.7 | Add sequence current value display | ✅ | P2 | 0.5d | 4.1.3 | Show current/next values |
| 4.1.8 | Wire into menu system | ✅ | P1 | 0.5d | 4.1.3 | Window -> Sequences menu item |

**Implementation**: `src/ui/sequence_manager_frame.h/cpp`, `src/ui/sequence_editor_dialog.h/cpp`

### 4.2 View Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 4.2.1 | Create View Manager spec | ✅ | P1 | 0.5d | - | UI design document |
| 4.2.2 | Create view list query | ✅ | P1 | 0.5d | - | Query `sb_catalog.sb_views` |
| 4.2.3 | Implement View Manager frame | ✅ | P1 | 2d | 4.2.1-4.2.2 | Window with view grid |
| 4.2.4 | Implement CREATE VIEW dialog | ✅ | P1 | 1d | 4.2.3 | SQL editor for view definition |
| 4.2.5 | Implement ALTER VIEW dialog | ✅ | P1 | 1d | 4.2.3 | Modify view definition |
| 4.2.6 | Implement DROP VIEW | ✅ | P1 | 0.5d | 4.2.3 | Confirmation dialog |
| 4.2.7 | Add view definition display | ✅ | P1 | 0.5d | 4.2.3 | Show CREATE VIEW statement |
| 4.2.8 | Add view dependency tracking | ✅ | P2 | 1d | 4.2.3 | Show tables/views used by view |
| 4.2.9 | Wire into menu system | ✅ | P1 | 0.5d | 4.2.3 | Window -> Views menu item |

**Implementation**: `src/ui/view_manager_frame.h/cpp`, `src/ui/view_editor_dialog.h/cpp`

### 4.3 Trigger Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 4.3.1 | Create Trigger Manager spec | ✅ | P1 | 0.5d | - | UI design document |
| 4.3.2 | Create trigger list query | ✅ | P1 | 0.5d | - | Query `sb_catalog.sb_triggers` |
| 4.3.3 | Implement Trigger Manager frame | ✅ | P1 | 2d | 4.3.1-4.3.2 | Window with trigger grid |
| 4.3.4 | Implement CREATE TRIGGER dialog | ✅ | P1 | 2d | 4.3.3 | Event, timing, table, body |
| 4.3.5 | Implement ALTER TRIGGER dialog | ✅ | P1 | 1d | 4.3.3 | Enable/disable, rename |
| 4.3.6 | Implement DROP TRIGGER | ✅ | P1 | 0.5d | 4.3.3 | Confirmation dialog |
| 4.3.7 | Add trigger definition display | ✅ | P1 | 0.5d | 4.3.3 | Show trigger body |
| 4.3.8 | Wire into menu system | ✅ | P1 | 0.5d | 4.3.3 | Window -> Triggers menu item |

**Implementation**: `src/ui/trigger_manager_frame.h/cpp`, `src/ui/trigger_editor_dialog.h/cpp`

### 4.4 Procedure/Function Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 4.4.1 | Create Procedure Manager spec | ✅ | P1 | 0.5d | - | UI design document |
| 4.4.2 | Create procedure list query | ✅ | P1 | 0.5d | - | Query `sb_catalog.sb_procedures` |
| 4.4.3 | Implement Procedure Manager frame | ✅ | P1 | 2d | 4.4.1-4.4.2 | Window with procedure grid |
| 4.4.4 | Implement CREATE PROCEDURE dialog | ✅ | P1 | 2d | 4.4.3 | Parameters, body, language |
| 4.4.5 | Implement CREATE FUNCTION dialog | ✅ | P1 | 1d | 4.4.3 | Return type, parameters, body |
| 4.4.6 | Implement ALTER PROCEDURE dialog | ✅ | P1 | 1d | 4.4.3 | Modify properties |
| 4.4.7 | Implement DROP PROCEDURE | ✅ | P1 | 0.5d | 4.4.3 | Confirmation dialog |
| 4.4.8 | Add procedure definition display | ✅ | P1 | 0.5d | 4.4.3 | Show procedure body |
| 4.4.9 | Add parameter display | ✅ | P1 | 0.5d | 4.4.3 | Grid of parameters with types |
| 4.4.10 | Wire into menu system | ✅ | P1 | 0.5d | 4.4.3 | Window -> Procedures menu item |

**Implementation**: `src/ui/procedure_manager_frame.h/cpp`, `src/ui/routine_editor_dialog.h/cpp`

### 4.5 Package Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 4.5.1 | Create Package Manager spec | ✅ | P2 | 0.5d | - | UI design document |
| 4.5.2 | Create package list query | ✅ | P2 | 0.5d | - | Query `sb_catalog.sb_packages` |
| 4.5.3 | Implement Package Manager frame | ✅ | P2 | 2d | 4.5.1-4.5.2 | Window with package grid |
| 4.5.4 | Implement CREATE PACKAGE dialog | ✅ | P2 | 2d | 4.5.3 | Specification and body |
| 4.5.5 | Implement ALTER PACKAGE dialog | ✅ | P2 | 1d | 4.5.3 | Modify package |
| 4.5.6 | Implement DROP PACKAGE | ✅ | P2 | 0.5d | 4.5.3 | Confirmation dialog |
| 4.5.7 | Add package contents display | ✅ | P2 | 1d | 4.5.3 | List of procedures/functions in package |
| 4.5.8 | Wire into menu system | ✅ | P2 | 0.5d | 4.5.3 | Window -> Packages menu item |

**Implementation**: `src/ui/package_manager_frame.h/cpp`, `src/ui/package_editor_dialog.h/cpp`

---

## Phase 5: Administration Tools (P1 - Important)

**Goal**: Provide comprehensive database administration capabilities.  
**Timeline**: 3-4 weeks  
**Dependencies**: Phase 2 complete

### 5.1 Backup and Restore UI

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 5.1.1 | Write backup format specification | 🔴 | P1 | 1d | - | Document .sbbak format |
| 5.1.2 | Create Backup dialog | 🔴 | P1 | 2d | 5.1.1 | Source, destination, options |
| 5.1.3 | Integrate sb_backup CLI | 🔴 | P1 | 2d | 5.1.2 | Execute backup via subprocess |
| 5.1.4 | Add backup progress display | 🔴 | P1 | 1d | 5.1.3 | Progress bar, cancel button |
| 5.1.5 | Create Restore dialog | 🔴 | P1 | 2d | 5.1.1 | Backup file, target database, options |
| 5.1.6 | Integrate sb_restore CLI | 🔴 | P1 | 2d | 5.1.5 | Execute restore via subprocess |
| 5.1.7 | Add restore progress display | 🔴 | P1 | 1d | 5.1.6 | Progress bar with phase info |
| 5.1.8 | Add backup scheduling UI | 🔴 | P2 | 2d | 5.1.2 | Schedule recurring backups |
| 5.1.9 | Add backup history viewer | 🔴 | P2 | 1d | 5.1.3 | List of past backups |

### 5.2 Storage Management

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 5.2.1 | Create Storage Manager spec | 🔴 | P2 | 0.5d | - | UI design document |
| 5.2.2 | Implement tablespace list query | 🔴 | P2 | 0.5d | - | Query `sb_catalog.sb_tablespaces` |
| 5.2.3 | Implement Storage Manager frame | 🔴 | P2 | 2d | 5.2.1-5.2.2 | Tablespace grid with usage stats |
| 5.2.4 | Implement CREATE TABLESPACE dialog | 🔴 | P2 | 1d | 5.2.3 | Path, size options |
| 5.2.5 | Implement ALTER TABLESPACE dialog | 🔴 | P2 | 1d | 5.2.3 | Resize, rename |
| 5.2.6 | Implement DROP TABLESPACE | 🔴 | P2 | 0.5d | 5.2.3 | Confirmation with dependency check |
| 5.2.7 | Add object relocation UI | 🔴 | P2 | 1d | 5.2.3 | Move tables/indexes between tablespaces |
| 5.2.8 | Wire into menu system | 🔴 | P2 | 0.5d | 5.2.3 | Window -> Storage menu item |

### 5.3 Monitoring Enhancement

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 5.3.1 | Add ScratchBird native monitoring queries | 🔴 | P1 | 2d | - | Query `sys.sessions`, `sys.locks`, etc. |
| 5.3.2 | Implement sessions panel | 🟡 | P1 | 1d | 5.3.1 | List active sessions |
| 5.3.3 | Implement locks panel | 🟡 | P1 | 1d | 5.3.1 | Show current locks |
| 5.3.4 | Implement transactions panel | 🟡 | P1 | 1d | 5.3.1 | Show active transactions |
| 5.3.5 | Implement statements panel | 🔴 | P1 | 1d | 5.3.1 | Show running statements |
| 5.3.6 | Implement table statistics panel | 🔴 | P1 | 1d | 5.3.1 | Row counts, sizes per table |
| 5.3.7 | Implement I/O statistics panel | 🔴 | P2 | 1d | 5.3.1 | Read/write stats |
| 5.3.8 | Add auto-refresh option | 🔴 | P2 | 0.5d | 5.3.2-5.3.7 | Configurable refresh interval |
| 5.3.9 | Add kill session functionality | 🔴 | P2 | 0.5d | 5.3.2 | Terminate selected session |

### 5.4 Database Manager

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 5.4.1 | Create Database Manager spec | 🔴 | P1 | 0.5d | - | UI design document |
| 5.4.2 | Implement database list query | 🔴 | P1 | 0.5d | - | List attached databases |
| 5.4.3 | Implement Database Manager frame | 🔴 | P1 | 2d | 5.4.1-5.4.2 | Database grid with status |
| 5.4.4 | Implement CREATE DATABASE dialog | 🔴 | P1 | 2d | 5.4.3 | Name, owner, encoding options |
| 5.4.5 | Implement DROP DATABASE | 🔴 | P1 | 0.5d | 5.4.3 | Strong confirmation required |
| 5.4.6 | Add database properties display | 🔴 | P2 | 1d | 5.4.3 | Size, encoding, owner |
| 5.4.7 | Add database clone functionality | 🔴 | P2 | 1d | 5.4.3 | Create copy of database |
| 5.4.8 | Wire into menu system | 🔴 | P1 | 0.5d | 5.4.3 | Window -> Databases menu item |

---

## Phase 6: Application Infrastructure (P1 - Important)

**Goal**: Polish the application with preferences, help, and usability features.  
**Timeline**: 2-3 weeks  
**Dependencies**: Phase 1 complete

### 6.1 Preferences System

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 6.1.1 | Create Preferences spec | 🔴 | P1 | 0.5d | - | Categories and options |
| 6.1.2 | Implement Preferences dialog | 🔴 | P1 | 2d | 6.1.1 | Tabbed preferences window |
| 6.1.3 | Add Editor preferences | 🔴 | P1 | 0.5d | 6.1.2 | Font, colors, tabs/spaces |
| 6.1.4 | Add Result Grid preferences | 🔴 | P1 | 0.5d | 6.1.2 | Default row limit, formatting |
| 6.1.5 | Add Connection preferences | 🔴 | P1 | 0.5d | 6.1.2 | Timeouts, SSL defaults |
| 6.1.6 | Add Export preferences | 🔴 | P2 | 0.5d | 6.1.2 | CSV delimiter, date format |
| 6.1.7 | Add Diagram preferences | 🔴 | P2 | 0.5d | 6.1.2 | Default notation, grid size |
| 6.1.8 | Add Network preferences | 🔴 | P2 | 0.5d | 6.1.2 | Proxy settings |
| 6.1.9 | Implement settings persistence | 🔴 | P1 | 1d | 6.1.2 | Save/load from config file |
| 6.1.10 | Wire into menu system | 🔴 | P1 | 0.5d | 6.1.2 | Edit -> Preferences menu item |

### 6.2 Context-Sensitive Help

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 6.2.1 | Review help topic inventory | 🟡 | P1 | 0.5d | - | Check against UI coverage |
| 6.2.2 | Create missing help topics | 🔴 | P1 | 3d | 6.2.1 | Write Markdown help content |
| 6.2.3 | Implement help browser | 🔴 | P1 | 2d | 6.2.2 | HTML/Markdown viewer |
| 6.2.4 | Wire F1 help to windows | 🔴 | P1 | 1d | 6.2.3 | Context-sensitive help activation |
| 6.2.5 | Add help index/search | 🔴 | P2 | 1d | 6.2.3 | Search help content |
| 6.2.6 | Add language reference | 🔴 | P2 | 2d | 6.2.2 | ScratchBird SQL reference |

**Specification**: `docs/specifications/CONTEXT_SENSITIVE_HELP.md` (exists)

### 6.3 Session State Persistence

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 6.3.1 | Write session state spec | 🔴 | P1 | 0.5d | - | Define what persists |
| 6.3.2 | Implement window state save | 🔴 | P1 | 1d | 6.3.1 | Positions, sizes |
| 6.3.3 | Implement window state restore | 🔴 | P1 | 1d | 6.3.2 | Restore on launch |
| 6.3.4 | Implement editor content save | 🔴 | P1 | 1d | 6.3.1 | Unsaved SQL buffers |
| 6.3.5 | Implement editor content restore | 🔴 | P1 | 1d | 6.3.4 | Restore SQL on launch |
| 6.3.6 | Implement connection restore | 🔴 | P2 | 1d | 6.3.1 | Reconnect to last connections |
| 6.3.7 | Add crash recovery | 🔴 | P2 | 1d | 6.3.4 | Detect crash, offer recovery |

### 6.4 Keyboard Shortcuts

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 6.4.1 | Write keyboard shortcuts spec | 🔴 | P1 | 0.5d | - | Complete shortcut list |
| 6.4.2 | Implement global shortcuts | 🔴 | P1 | 1d | 6.4.1 | New editor, close window, etc. |
| 6.4.3 | Implement SQL Editor shortcuts | 🟡 | P1 | 0.5d | 6.4.1 | Execute, cancel, format |
| 6.4.4 | Implement catalog tree shortcuts | 🔴 | P1 | 0.5d | 6.4.1 | Navigate, open, copy |
| 6.4.5 | Implement diagram shortcuts | 🔴 | P1 | 0.5d | 6.4.1 | Delete, align, zoom |
| 6.4.6 | Implement results grid shortcuts | 🔴 | P1 | 0.5d | 6.4.1 | Copy, find |
| 6.4.7 | Add shortcut customization | 🔴 | P2 | 2d | 6.4.1 | User-defined shortcuts |
| 6.4.8 | Create shortcuts cheat sheet | 🔴 | P2 | 0.5d | 6.4.1 | Printable reference |

---

## Phase 7: Beta Placeholders (P2 - Future)

**Goal**: Prepare UI structure for Beta features without full implementation.  
**Timeline**: 1 week  
**Dependencies**: None (stubs only)

### 7.1 Cluster Manager (Beta Placeholder)

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 7.1.1 | Create Cluster Manager stub | 🔴 | P2 | 1d | - | Window with "Beta Feature" message |
| 7.1.2 | Add cluster menu items (disabled) | 🔴 | P2 | 0.5d | 7.1.1 | Menu stubs |
| 7.1.3 | Write Cluster Manager spec | 🔴 | P2 | 2d | - | Full specification for Beta |

### 7.2 Replication Manager (Beta Placeholder)

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 7.2.1 | Create Replication Manager stub | 🔴 | P2 | 1d | - | Window with "Beta Feature" message |
| 7.2.2 | Add replication menu items (disabled) | 🔴 | P2 | 0.5d | 7.2.1 | Menu stubs |
| 7.2.3 | Write Replication Manager spec | 🔴 | P2 | 2d | - | Full specification for Beta |

### 7.3 ETL Tools (Beta Placeholder)

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 7.3.1 | Create ETL Manager stub | 🔴 | P2 | 1d | - | Window with "Beta Feature" message |
| 7.3.2 | Add ETL menu items (disabled) | 🔴 | P2 | 0.5d | 7.3.1 | Menu stubs |
| 7.3.3 | Write ETL Manager spec | 🔴 | P2 | 2d | - | Full specification for Beta |

### 7.4 Git Integration (Beta Placeholder)

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 7.4.1 | Create Git Integration stub | 🔴 | P2 | 1d | - | Window with "Beta Feature" message |
| 7.4.2 | Wire Git menu items | 🔴 | P2 | 0.5d | 7.4.1 | Currently stubbed |
| 7.4.3 | Write Git Integration spec | 🔴 | P2 | 2d | - | Full specification for Beta |

---

## Phase 8: Testing and Quality Assurance (P0 - Ongoing)

**Goal**: Ensure application stability and correctness.  
**Timeline**: Ongoing throughout all phases  
**Dependencies**: All phases

### 8.1 Unit Testing

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 8.1.1 | Set up Google Test framework | 🟡 | P0 | 1d | - | Test executable builds |
| 8.1.2 | Add metadata model tests | 🟡 | P0 | 2d | 8.1.1 | Unit tests for metadata classes |
| 8.1.3 | Add connection backend tests | 🔴 | P0 | 2d | 8.1.1 | Mock backend tests |
| 8.1.4 | Add statement splitter tests | 🟡 | P0 | 1d | 8.1.1 | SQL splitting tests |
| 8.1.5 | Add value formatter tests | 🔴 | P0 | 1d | 8.1.1 | Format conversion tests |
| 8.1.6 | Add result exporter tests | 🔴 | P0 | 1d | 8.1.1 | CSV/JSON export tests |
| 8.1.7 | Add diagram model tests | 🔴 | P0 | 2d | 3.1.3 | Diagram document tests |
| 8.1.8 | Add DDL generator tests | 🔴 | P0 | 2d | 3.6.3 | Forward engineering tests |
| 8.1.9 | Maintain >80% code coverage | 🔴 | P0 | Ongoing | 8.1.2-8.1.8 | Coverage reports |

### 8.2 Integration Testing

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 8.2.1 | Set up integration test harness | 🟡 | P0 | 2d | - | Test database setup |
| 8.2.2 | Add PostgreSQL integration tests | 🟡 | P0 | 3d | 8.2.1 | Tests against real PG server |
| 8.2.3 | Add MySQL integration tests | 🟡 | P0 | 3d | 8.2.1 | Tests against real MySQL server |
| 8.2.4 | Add Firebird integration tests | 🟡 | P0 | 3d | 8.2.1 | Tests against real Firebird server |
| 8.2.5 | Add ScratchBird integration tests | 🔴 | P0 | 5d | 8.2.1 | Tests against ScratchBird server |
| 8.2.6 | Add UI automation tests | 🔴 | P2 | 5d | - | wxWidgets UI tests |

### 8.3 Performance Testing

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 8.3.1 | Define performance benchmarks | 🔴 | P1 | 1d | - | Startup, query, load times |
| 8.3.2 | Add large result set tests | 🔴 | P1 | 2d | 8.3.1 | 100K+ rows handling |
| 8.3.3 | Add memory usage tests | 🔴 | P1 | 2d | 8.3.1 | Memory leak detection |
| 8.3.4 | Add diagram performance tests | 🔴 | P2 | 2d | 3.1.4 | 500+ entity diagrams |
| 8.3.5 | Create performance regression suite | 🔴 | P2 | 2d | 8.3.2-8.3.4 | Automated performance tests |

### 8.4 Documentation Testing

| Task ID | Task | Status | Priority | Est. Effort | Dependencies | Acceptance Criteria |
|---------|------|--------|----------|-------------|--------------|---------------------|
| 8.4.1 | Add specification review checklist | 🔴 | P1 | 0.5d | - | Review criteria |
| 8.4.2 | Review specs for implementability | 🔴 | P1 | 2d | 8.4.1 | All specs reviewed |
| 8.4.3 | Add help content validation | 🔴 | P2 | 1d | 6.2.2 | Check help topic coverage |
| 8.4.4 | Create user acceptance test plan | 🔴 | P1 | 2d | - | End-to-end test scenarios |

---

## Cross-Cutting Concerns

### Specification Documents Status

| Specification | Priority | Phase | Description | Status |
|--------------|----------|-------|-------------|--------|
| CONNECTION_PROFILE_EDITOR.md | P0 | 1 | Connection creation/editing UI | ✅ Implemented |
| TRANSACTION_MANAGEMENT.md | P0 | 1 | Transaction behavior specification | ✅ Implemented |
| ERROR_HANDLING.md | P0 | 1 | Error classification and display | ✅ Implemented |
| ERD_NOTATION_DICTIONARIES.md | P0 | 3 | Symbol dictionaries for notations | ✅ Implemented |
| AUTO_LAYOUT.md | P1 | 3 | Layout algorithm specification | ✅ Implemented |
| UNDO_REDO.md | P0 | 3 | Command pattern for diagrams | ✅ Implemented |
| DATA_TYPE_MAPPING.md | P0 | 3 | Cross-dialect type mappings | ✅ Implemented |
| REVERSE_ENGINEERING.md | P0 | 3 | DB to diagram import spec | ✅ Implemented |
| FORWARD_ENGINEERING.md | P0 | 3 | Diagram to DDL export spec | ✅ Implemented |
| SEQUENCE_MANAGER_UI.md | P1 | 4 | Sequence management UI | 🔴 Not Started |
| VIEW_MANAGER_UI.md | P1 | 4 | View management UI | 🔴 Not Started |
| TRIGGER_MANAGER_UI.md | P1 | 4 | Trigger management UI | 🔴 Not Started |
| PROCEDURE_MANAGER_UI.md | P1 | 4 | Procedure/Function management UI | 🔴 Not Started |
| BACKUP_RESTORE_UI.md | P1 | 5 | Backup/restore specification | 🔴 Not Started |
| PREFERENCES.md | P1 | 6 | Preferences system design | 🔴 Not Started |
| SESSION_STATE.md | P1 | 6 | State persistence specification | 🔴 Not Started |
| KEYBOARD_SHORTCUTS.md | P1 | 6 | Complete shortcut reference | 🔴 Not Started |

### Data Type Mapping Table (Implemented)

| ScratchBird | PostgreSQL | MySQL | Firebird | Status |
|-------------|------------|-------|----------|--------|
| INT32 | INTEGER | INT | INTEGER | ✅ Implemented |
| INT64 | BIGINT | BIGINT | BIGINT | ✅ Implemented |
| FLOAT32 | REAL | FLOAT | FLOAT | ✅ Implemented |
| FLOAT64 | DOUBLE PRECISION | DOUBLE | DOUBLE PRECISION | ✅ Implemented |
| DECIMAL(p,s) | NUMERIC(p,s) | DECIMAL(p,s) | DECIMAL(p,s) | ✅ Implemented |
| VARCHAR(n) | VARCHAR(n) | VARCHAR(n) | VARCHAR(n) | ✅ Implemented |
| TEXT | TEXT | TEXT | BLOB SUB_TYPE TEXT | ✅ Implemented |
| BLOB | BYTEA | BLOB | BLOB | ✅ Implemented |
| UUID | UUID | CHAR(36) | CHAR(36) | ✅ Implemented |
| JSON | JSONB | JSON | BLOB SUB_TYPE TEXT | ✅ Implemented |
| TIMESTAMP | TIMESTAMP | DATETIME | TIMESTAMP | ✅ Implemented |
| TIMESTAMPTZ | TIMESTAMPTZ | TIMESTAMP | TIMESTAMP | ✅ Implemented |
| VECTOR(n) | VECTOR(n)* | N/A | N/A | ⏸️ Pending |
| GEOMETRY | GEOMETRY* | GEOMETRY* | N/A | ⏸️ Pending |

**Implementation**: `src/diagram/forward_engineer.cpp` - `DataTypeMapper` class

---

## Summary Statistics

| Metric | Count |
|--------|-------|
| Total Tasks | 200+ |
| P0 (Critical) Tasks | ~80 |
| P1 (Important) Tasks | ~90 |
| P2 (Nice-to-have) Tasks | ~40 |
| Estimated Total Effort | 26-32 weeks (1 developer) |
| Parallelizable Phases | 4, 5, 6 (after Phase 2) |

### Completion Statistics

| Phase | Tasks | Completed | Deferred | Status |
|-------|-------|-----------|----------|--------|
| Phase 1 | 24 | 24 | 0 | 100% ✅ |
| Phase 2 | 46 | 46 | 0 | 100% ✅ |
| Phase 3 | 52 | 52 | 0 | 100% ✅ |
| Phase 4 | 43 | 43 | 0 | 100% ✅ |
| Phase 5 | 26 | 0 | 0 | 0% 🔴 |
| Phase 6 | 18 | 2 | 0 | 11% 🟡 |
| Phase 7 | 12 | 0 | 0 | 0% 🔴 |
| Phase 8 | - | Ongoing | - | 🟡 |

### Phase Effort Summary

| Phase | Duration | Tasks | Focus | Status |
|-------|----------|-------|-------|--------|
| Phase 1 | 2-3 weeks | 24 | Foundation | ✅ 100% Complete |
| Phase 2 | 3-4 weeks | 46 | Object Managers | ✅ 100% Complete |
| Phase 3 | 6-8 weeks | 52 | ERD System | ✅ 100% Complete |
| Phase 4 | 3-4 weeks | 43 | Additional Managers | ✅ 100% Complete |
| Phase 5 | 3-4 weeks | 26 | Admin Tools | 🔴 Not Started |
| Phase 6 | 2-3 weeks | 18 | Infrastructure | 🟡 Partial |
| Phase 7 | 1 week | 12 | Beta Placeholders | 🔴 Not Started |
| Phase 8 | Ongoing | - | Quality Assurance | 🟡 Ongoing |

---

## Review and Update Schedule

- **Weekly**: Task status updates during development
- **Bi-weekly**: Dependency and priority review
- **Monthly**: Full plan review and re-estimation
- **Milestone**: Phase completion validation

---

*This document is a living tracker. Update task statuses as work progresses and add new tasks as requirements evolve.*
