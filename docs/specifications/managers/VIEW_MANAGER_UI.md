# ScratchRobin View Manager UI Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/view_manager_frame.cpp`

---

## Overview

The View Manager provides a user interface for managing database views across all supported backends. Views are virtual tables based on the result set of a SQL query.

---

## Scope

### In Scope
- List all views in the database
- Create new views with SQL definition
- Alter existing views (replace definition)
- Drop views with dependency checking
- View definition display
- View dependency tracking

### Out of Scope
- Materialized view refresh scheduling (Alpha)
- View-level GRANT/REVOKE UI
- Updatable view trigger management

---

## Backend Support

| Backend | View Support | Materialized Views |
|---------|-------------|-------------------|
| PostgreSQL | ✅ CREATE VIEW | ✅ CREATE MATERIALIZED VIEW |
| MySQL | ✅ CREATE VIEW | ❌ Not supported |
| Firebird | ✅ CREATE VIEW | ❌ Not supported |
| ScratchBird | ✅ CREATE VIEW | ✅ Supported |

---

## SQL Surface

### Create View
```sql
CREATE [OR REPLACE] VIEW view_name [(column_name [, ...])]
  AS query
  [WITH CHECK OPTION];
```

### Alter View
```sql
-- PostgreSQL/ScratchBird
ALTER VIEW view_name ALTER COLUMN column_name SET DEFAULT expression;
ALTER VIEW view_name OWNER TO new_owner;
ALTER VIEW view_name RENAME TO new_name;
ALTER VIEW view_name SET SCHEMA new_schema;

-- MySQL
ALTER VIEW view_name AS query;
```

### Drop View
```sql
DROP VIEW [IF EXISTS] view_name [, ...] [CASCADE | RESTRICT];
```

### Show View Definition
```sql
-- PostgreSQL
SELECT pg_get_viewdef('view_name', true);

-- MySQL
SHOW CREATE VIEW view_name;

-- Firebird
SELECT RDB$VIEW_SOURCE FROM RDB$RELATIONS WHERE RDB$RELATION_NAME = 'VIEW_NAME';
```

---

## Query Wiring

### List Views

**PostgreSQL:**
```sql
SELECT table_schema, table_name, view_definition
FROM information_schema.views
WHERE table_schema NOT IN ('pg_catalog', 'information_schema');
```

**Firebird:**
```sql
SELECT RDB$RELATION_NAME
FROM RDB$RELATIONS
WHERE RDB$VIEW_SOURCE IS NOT NULL
  AND RDB$SYSTEM_FLAG = 0;
```

**ScratchBird:**
```sql
SELECT name, schema_name, definition
FROM sb_catalog.sb_views
ORDER BY schema_name, name;
```

### Get View Dependencies

**PostgreSQL:**
```sql
SELECT dependent_ns.nspname as dependent_schema,
       dependent_view.relname as dependent_view,
       source_ns.nspname as source_schema,
       source_table.relname as source_table
FROM pg_depend
JOIN pg_rewrite ON pg_depend.objid = pg_rewrite.oid
JOIN pg_class as dependent_view ON pg_rewrite.ev_class = dependent_view.oid
JOIN pg_class as source_table ON pg_depend.refobjid = source_table.oid
JOIN pg_namespace dependent_ns ON dependent_view.relnamespace = dependent_ns.oid
JOIN pg_namespace source_ns ON source_table.relnamespace = source_ns.oid
WHERE dependent_view.relname = 'view_name';
```

---

## UI Requirements

### Menu Placement
- Main menu: `Window -> Views`

### View Manager Window

**Layout:**
```
+--------------------------------------------------+
| Toolbar: [Create] [Alter] [Drop] [Refresh]       |
+--------------------------------------------------+
| +------------------+  +-----------------------+  |
| | View List        |  | Details Panel         |  |
| |                  |  |                       |  |
| | • vw_users       |  | Tabs:                 |  |
| | • vw_orders      |  |   [Definition]        |  |
| | • ...            |  |   [Dependencies]      |  |
| |                  |  |   [Data Preview]      |  |
| |                  |  |                       |  |
| |                  |  | Definition:           |  |
| |                  |  | CREATE VIEW vw_users  |  |
| |                  |  | AS SELECT ...         |  |
| +------------------+  +-----------------------+  |
+--------------------------------------------------+
| Status: 5 views                                  |
+--------------------------------------------------+
```

**View List Columns:**
- View Name
- Schema
- Type (Standard/Materialized)
- Owner
- Created/Modified Date

**Details Panel Tabs:**
- **Definition**: SQL source with syntax highlighting
- **Dependencies**: Tables and views used by this view
- **Data Preview**: Execute view query (limited rows)
- **Privileges**: Access permissions (read-only)

### Create View Dialog

**Fields:**
- **View Name** (required)
- **Schema** (dropdown)
- **Column Names** (optional, comma-separated)
- **OR REPLACE** (checkbox)
- **SQL Definition** (text editor with SQL syntax highlighting)
- **WITH CHECK OPTION** (checkbox, for updatable views)

**Features:**
- SQL validation before execution
- Format SQL button
- Preview results button
- Column auto-detection from query

### Alter View Dialog

**Fields:**
- **View Name** (read-only)
- **Schema** (read-only)
- **New SQL Definition** (text editor)
- **Action**: Replace / Rename / Change Owner

### Drop View Confirmation
- View name display
- List of dependent objects (if any)
- CASCADE/RESTRICT options
- Warning for materialized views with data

---

## Implementation Details

### Files
- `src/ui/view_manager_frame.h` / `.cpp` - Main window
- `src/ui/view_editor_dialog.h` / `.cpp` - Create/Alter dialogs

### Key Classes
```cpp
class ViewManagerFrame : public wxFrame {
    // View list with filter
    // Details panel with tabs
    // SQL preview component
};

class ViewEditorDialog : public wxDialog {
    // SQL editor with syntax highlighting
    // Validation and preview
};
```

### Backend-Specific Handling
- **PostgreSQL**: Support for MATERIALIZED views, WITH CHECK OPTION
- **MySQL**: Simple view support, no materialized views
- **Firebird**: Check updatable view support
- **ScratchBird**: Full feature support

---

## Future Enhancements

- Visual query builder for views
- View diff/compare functionality
- Materialized view refresh scheduling
- View usage statistics
