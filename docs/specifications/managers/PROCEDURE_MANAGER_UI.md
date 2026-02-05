# ScratchRobin Procedure Manager UI Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/procedure_manager_frame.cpp`

---

## Overview

The Procedure Manager provides a user interface for managing stored procedures and functions across all supported backends. Stored procedures are precompiled SQL statements stored in the database for reuse.

---

## Scope

### In Scope
- List all procedures and functions
- Create new procedures/functions with parameters
- Alter procedure/function definitions
- Drop procedures/functions
- View procedure definition and signature
- Parameter management

### Out of Scope
- Procedure debugging interface (Future)
- Procedure profiling/performance (Future)
- Graphical procedure builder (Future)

---

## Backend Support

| Backend | Procedures | Functions | Languages |
|---------|-----------|-----------|-----------|
| PostgreSQL | ✅ CREATE PROCEDURE | ✅ CREATE FUNCTION | SQL, PL/pgSQL, C, etc. |
| MySQL | ✅ CREATE PROCEDURE | ✅ CREATE FUNCTION | SQL |
| Firebird | ✅ CREATE PROCEDURE | ✅ CREATE FUNCTION | PSQL |
| ScratchBird | ✅ CREATE PROCEDURE | ✅ CREATE FUNCTION | SQL, SBL |

---

## SQL Surface

### Create Procedure/Function

**PostgreSQL:**
```sql
CREATE [OR REPLACE] FUNCTION function_name([argmode] [argname] argtype [{DEFAULT | =} default_expr] [, ...])
  [RETURNS rettype
   | RETURNS TABLE (column_name column_type [, ...])]
  {LANGUAGE lang_name
   | TRANSFORM {FOR TYPE type_name} [, ... ]
   | WINDOW
   | IMMUTABLE | STABLE | VOLATILE
   | [NOT] LEAKPROOF
   | CALLED ON NULL INPUT | RETURNS NULL ON NULL INPUT | STRICT
   | [EXTERNAL] SECURITY INVOKER | [EXTERNAL] SECURITY DEFINER
   | PARALLEL {UNSAFE | RESTRICTED | SAFE}
   | COST execution_cost
   | ROWS result_rows
   | SUPPORT support_function
   | SET configuration_parameter {TO value | = value | FROM CURRENT}
   | AS 'definition'
   | AS 'obj_file', 'link_symbol'
  } ...
```

**MySQL:**
```sql
CREATE [DEFINER = user] 
  PROCEDURE procedure_name([proc_parameter[,...]])
  [characteristic ...] 
  routine_body

CREATE [DEFINER = user]
  FUNCTION function_name([func_parameter[,...]])
  RETURNS type
  [characteristic ...] 
  routine_body
```

**Firebird:**
```sql
CREATE PROCEDURE procedure_name
  [(param_name datatype [= default], ...)]
  [RETURNS (param_name datatype, ...)]
  AS
  procedure_body;

CREATE FUNCTION function_name
  [(param_name datatype [= default], ...)]
  RETURNS datatype
  [DETERMINISTIC]
  AS
  function_body;
```

### Drop Procedure/Function
```sql
DROP PROCEDURE [IF EXISTS] procedure_name;
DROP FUNCTION [IF EXISTS] function_name;
```

---

## Query Wiring

### List Procedures/Functions

**PostgreSQL:**
```sql
SELECT 
  routine_name,
  routine_type,
  data_type as return_type,
  routine_definition
FROM information_schema.routines
WHERE routine_schema = 'public'
  AND routine_type IN ('PROCEDURE', 'FUNCTION')
ORDER BY routine_type, routine_name;
```

**Firebird:**
```sql
SELECT 
  RDB$PROCEDURE_NAME,
  RDB$PROCEDURE_SOURCE,
  RDB$SYSTEM_FLAG
FROM RDB$PROCEDURES
WHERE RDB$SYSTEM_FLAG = 0;
```

**ScratchBird:**
```sql
SELECT 
  name,
  type,
  return_type,
  language,
  definition
FROM sb_catalog.sb_procedures
ORDER BY type, name;
```

### Get Parameters

**PostgreSQL:**
```sql
SELECT 
  parameter_name,
  data_type,
  parameter_mode,
  ordinal_position
FROM information_schema.parameters
WHERE specific_schema = 'public'
  AND specific_name = (SELECT specific_name 
                       FROM information_schema.routines 
                       WHERE routine_name = 'proc_name');
```

---

## UI Requirements

### Menu Placement
- Main menu: `Window -> Procedures`

### Procedure Manager Window

**Layout:**
```
+--------------------------------------------------+
| Toolbar: [Create Proc] [Create Func] [Alter]     |                                                |
|          [Drop] [Refresh]                        |
+--------------------------------------------------+
| +------------------+  +-----------------------+  |
| | Procedure List   |  | Details Panel         |  |
| |                  |  |                       |  |
| | • get_user()     |  | Name: get_user        |  |
| | • calc_total()   |  | Type: FUNCTION        |  |
| | • process_data   |  | Returns: TABLE        |  |
| | • ...            |  | Language: SQL         |  |
| |                  |  |                       |  |
| |                  |  | Parameters:           |  |
| |                  |  |   • user_id INTEGER   |  |
| |                  |  |   • active BOOLEAN    |  |
| |                  |  |                       |  |
| |                  |  | [View Definition]     |  |
| +------------------+  +-----------------------+  |
+--------------------------------------------------+
| Status: 8 procedures, 12 functions               |
+--------------------------------------------------+
```

**Procedure List Columns:**
- Name
- Type (Procedure/Function)
- Returns
- Language
- Schema
- Arguments (signature preview)

**Details Panel Tabs:**
- **General**: Basic info, parameters table
- **Definition**: SQL source code
- **Privileges**: Access permissions
- **Calls**: Usage statistics (if available)

### Create Procedure/Function Dialog

**Fields:**
- **Name** (required)
- **Type** (Procedure, Function)
- **Schema** (dropdown)
- **Language** (SQL, PL/pgSQL, etc.)
- **Return Type** (for functions)
- **Parameters Table:**
  - Name
  - Data Type
  - Mode (IN, OUT, INOUT)
  - Default Value
  - Add/Remove buttons
- **Definition** (SQL body editor)
- **Characteristics:**
  - Deterministic
  - SQL Security (Definer/Invoker)
  - Comment

### Alter Dialog

**Read-Only:**
- Name
- Type
- Original signature

**Editable:**
- Replace definition
- Rename (if supported)
- Change owner (if supported)

---

## Implementation Details

### Files
- `src/ui/procedure_manager_frame.h` / `.cpp` - Main window
- `src/ui/routine_editor_dialog.h` / `.cpp` - Create/Alter dialogs

### Key Classes
```cpp
class ProcedureManagerFrame : public wxFrame {
    // Procedure/function list
    // Filter by type
    // Definition viewer
};

class RoutineEditorDialog : public wxDialog {
    // Parameter grid editor
    // SQL body editor
    // Signature preview
};
```

### Backend-Specific Handling
- **PostgreSQL**: Support for multiple languages, complex return types
- **MySQL**: DEFINER, SQL SECURITY
- **Firebird**: PSQL syntax, EXECUTE PROCEDURE
- **ScratchBird**: SBL language support

---

## Future Enhancements

- Stored procedure debugger
- Performance profiling integration
- Procedure template library
- Dependency tracking and visualization
