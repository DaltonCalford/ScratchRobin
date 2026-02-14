# ScratchRobin Trigger Manager UI Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/trigger_manager_frame.cpp`

---

## Overview

The Trigger Manager provides a user interface for managing database triggers across all supported backends. Triggers are stored procedures that automatically execute in response to specific events on a table or view.

---

## Scope

### In Scope
- List all triggers in the database
- Create new triggers with event/timing specification
- Enable/disable triggers
- Drop triggers
- View trigger definition and status

### Out of Scope
- Trigger debugging interface (Future)
- Trigger performance profiling (Future)
- Complex trigger dependency visualization

---

## Backend Support

| Backend | Trigger Support | Events | Timing |
|---------|----------------|--------|--------|
| PostgreSQL | ✅ Full | INSERT, UPDATE, DELETE, TRUNCATE | BEFORE, AFTER, INSTEAD OF |
| MySQL | ✅ Full | INSERT, UPDATE, DELETE | BEFORE, AFTER |
| Firebird | ✅ Full | INSERT, UPDATE, DELETE | BEFORE, AFTER |
| ScratchBird | ✅ Full | INSERT, UPDATE, DELETE, TRUNCATE | BEFORE, AFTER, INSTEAD OF |

---

## SQL Surface

### Create Trigger

**PostgreSQL/ScratchBird:**
```sql
CREATE [OR REPLACE] TRIGGER trigger_name
  {BEFORE | AFTER | INSTEAD OF} {event [OR ...]}
  ON table_name
  [FROM referenced_table_name]
  [NOT DEFERRABLE | [DEFERRABLE] [INITIALLY IMMEDIATE | INITIALLY DEFERRED]]
  [FOR [EACH] {ROW | STATEMENT}]
  [WHEN (condition)]
  EXECUTE FUNCTION function_name(arguments);
```

**MySQL:**
```sql
CREATE [DEFINER = user] TRIGGER trigger_name
  {BEFORE | AFTER} {INSERT | UPDATE | DELETE}
  ON table_name
  FOR EACH ROW
  [trigger_order]
  trigger_body;
```

**Firebird:**
```sql
CREATE TRIGGER trigger_name
  [ACTIVE | INACTIVE]
  {BEFORE | AFTER}
  {INSERT | UPDATE | DELETE}
  [OR {INSERT | UPDATE | DELETE}]
  ON table_name
  [POSITION position_number]
  AS
  trigger_body;
```

### Alter Trigger

**PostgreSQL:**
```sql
ALTER TRIGGER trigger_name ON table_name RENAME TO new_name;
ALTER TRIGGER trigger_name ON table_name DEPENDS ON EXTENSION extension_name;
```

**Firebird:**
```sql
ALTER TRIGGER trigger_name
  [ACTIVE | INACTIVE]
  [BEFORE | AFTER]
  [INSERT | UPDATE | DELETE]
  [POSITION position_number]
  AS trigger_body;
```

### Drop Trigger
```sql
DROP TRIGGER [IF EXISTS] trigger_name ON table_name [CASCADE | RESTRICT];
```

### Enable/Disable Trigger

**PostgreSQL:**
```sql
ALTER TABLE table_name DISABLE TRIGGER trigger_name;
ALTER TABLE table_name ENABLE TRIGGER trigger_name;
-- or ALL
ALTER TABLE table_name DISABLE TRIGGER ALL;
```

---

## Query Wiring

### List Triggers

**PostgreSQL:**
```sql
SELECT 
  trigger_name,
  event_manipulation,
  event_object_table,
  action_timing,
  action_orientation,
  action_statement
FROM information_schema.triggers
WHERE trigger_schema = 'public'
ORDER BY event_object_table, trigger_name;
```

**Firebird:**
```sql
SELECT 
  RDB$TRIGGER_NAME,
  RDB$RELATION_NAME,
  RDB$TRIGGER_TYPE,
  RDB$TRIGGER_INACTIVE,
  RDB$TRIGGER_SOURCE
FROM RDB$TRIGGERS
WHERE RDB$SYSTEM_FLAG = 0;
```

**ScratchBird:**
```sql
SELECT 
  name,
  table_name,
  event,
  timing,
  is_enabled,
  definition
FROM sb_catalog.sb_triggers
ORDER BY table_name, name;
```

---

## UI Requirements

### Menu Placement
- Main menu: `Window -> Triggers`

### Trigger Manager Window

**Layout:**
```
+--------------------------------------------------+
| Toolbar: [Create] [Alter] [Drop] [Enable/Disable]|                                                |
+--------------------------------------------------+
| +------------------+  +-----------------------+  |
| | Trigger List     |  | Details Panel         |  |
| |                  |  |                       |  |
| | • trg_users_bi   |  | Name: trg_users_bi    |  |
| | • trg_orders_ai  |  | Table: users          |  |
| | • ...            |  | Event: INSERT         |  |
| |                  |  | Timing: BEFORE        |  |
| |                  |  | For Each: ROW         |  |
| |                  |  | Status: Enabled       |  |
| |                  |  | Position: 1           |  |
| |                  |  |                       |  |
| |                  |  | [View Definition]     |  |
| +------------------+  +-----------------------+  |
+--------------------------------------------------+
| Status: 12 triggers (3 disabled)                 |
+--------------------------------------------------+
```

**Trigger List Columns:**
- Trigger Name
- Table
- Event (INSERT/UPDATE/DELETE/TRUNCATE)
- Timing (BEFORE/AFTER/INSTEAD OF)
- Status (Enabled/Disabled)
- For Each (ROW/STATEMENT)

### Create Trigger Dialog

**Fields:**
- **Trigger Name** (required)
- **Table** (dropdown of available tables)
- **Timing** (BEFORE, AFTER, INSTEAD OF)
- **Event** (checkboxes: INSERT, UPDATE, DELETE, TRUNCATE)
- **For Each** (ROW, STATEMENT)
- **Position** (for ordering, Firebird/ScratchBird)
- **When Condition** (optional, for conditional triggers)
- **Trigger Body** (SQL/procedure body editor)
- **OR REPLACE** (checkbox)

**Features:**
- Event combination validation
- Table column reference helper
- Function/procedure selector
- Syntax highlighting for trigger body

### Alter Trigger Dialog

**Read-Only:**
- Trigger Name
- Table
- Original Event/Timing

**Editable:**
- **Status**: Enable / Disable
- **Rename To** (if supported)
- **New Body** (if supported by backend)
- **Position** (if supported)

### Drop Confirmation
- Trigger name and table
- Warning about dependencies
- Confirmation required

---

## Implementation Details

### Files
- `src/ui/trigger_manager_frame.h` / `.cpp` - Main window
- `src/ui/trigger_editor_dialog.h` / `.cpp` - Create/Alter dialogs

### Key Classes
```cpp
class TriggerManagerFrame : public wxFrame {
    // Trigger list with filter/sort
    // Enable/disable actions
    // Definition viewer
};

class TriggerEditorDialog : public wxDialog {
    // Event/timing selection
    // SQL body editor
    // Validation
};
```

### Backend-Specific Features
- **PostgreSQL**: INSTEAD OF triggers, WHEN conditions, STATEMENT triggers
- **MySQL**: DEFINER clause, trigger ordering
- **Firebird**: Position ordering, ACTIVE/INACTIVE
- **ScratchBird**: Full feature set

---

## Future Enhancements

- Trigger template library
- Trigger testing interface
- Trigger performance metrics
- Cross-database trigger comparison
