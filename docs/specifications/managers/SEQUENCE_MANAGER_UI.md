# ScratchRobin Sequence Manager UI Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/sequence_manager_frame.cpp`

---

## Overview

The Sequence Manager provides a user interface for managing database sequences (generators) across all supported backends. Sequences are used to generate unique numeric values, typically for primary key columns.

---

## Scope

### In Scope
- List all sequences in the database
- Create new sequences with custom parameters
- Alter existing sequences (restart, increment, min/max values)
- Drop sequences with dependency checking
- View current value and next value

### Out of Scope
- Sequence ownership/authorization UI (Alpha)
- Sequence caching configuration details
- Cross-database sequence synchronization

---

## Backend Support

| Backend | Sequence Support | Implementation |
|---------|-----------------|----------------|
| PostgreSQL | ✅ SERIAL, BIGSERIAL, CREATE SEQUENCE | Native |
| MySQL | ✅ AUTO_INCREMENT (simulated) | Native |
| Firebird | ✅ GENERATOR, SEQUENCE | Native |
| ScratchBird | ✅ SEQUENCE | Native |

---

## SQL Surface

### PostgreSQL
```sql
-- Create sequence
CREATE SEQUENCE [IF NOT EXISTS] sequence_name
  [AS data_type]
  [START WITH start]
  [INCREMENT BY increment]
  [MINVALUE minvalue | NO MINVALUE]
  [MAXVALUE maxvalue | NO MAXVALUE]
  [CACHE cache]
  [CYCLE | NO CYCLE];

-- Alter sequence
ALTER SEQUENCE sequence_name
  [RESTART [WITH restart]]
  [INCREMENT BY increment]
  [SET {MINVALUE minvalue | NO MINVALUE}]
  [SET {MAXVALUE maxvalue | NO MAXVALUE}]
  [CACHE cache]
  [CYCLE | NO CYCLE];

-- Drop sequence
DROP SEQUENCE [IF EXISTS] sequence_name [CASCADE | RESTRICT];

-- Get current/next value
SELECT currval('sequence_name');
SELECT nextval('sequence_name');
SELECT lastval();
```

### MySQL
```sql
-- Sequences simulated via AUTO_INCREMENT
-- Create table with auto_increment
CREATE TABLE table_name (
  id INT AUTO_INCREMENT PRIMARY KEY,
  ...
);

-- Alter auto_increment value
ALTER TABLE table_name AUTO_INCREMENT = value;
```

### Firebird
```sql
-- Create generator (legacy) / sequence
CREATE SEQUENCE sequence_name;

-- Set initial value
ALTER SEQUENCE sequence_name RESTART WITH value;

-- Get next value
SELECT NEXT VALUE FOR sequence_name FROM RDB$DATABASE;

-- Get current value (via GEN_ID)
SELECT GEN_ID(sequence_name, 0) FROM RDB$DATABASE;

-- Drop sequence
DROP SEQUENCE sequence_name;
```

---

## Query Wiring

### List Sequences

**PostgreSQL:**
```sql
SELECT sequencename, data_type, start_value, increment_by, 
       min_value, max_value, cycle
FROM pg_sequences
WHERE schemaname = 'public';
```

**Firebird:**
```sql
SELECT RDB$GENERATOR_NAME
FROM RDB$GENERATORS
WHERE RDB$SYSTEM_FLAG = 0;
```

**ScratchBird:**
```sql
SELECT name, start_value, increment, min_value, max_value, cycle
FROM sb_catalog.sb_sequences
ORDER BY name;
```

### Get Sequence Details

**PostgreSQL:**
```sql
SELECT * FROM pg_sequences WHERE sequencename = 'seq_name';
```

**Firebird:**
```sql
SELECT GEN_ID(sequence_name, 0) as current_value FROM RDB$DATABASE;
```

---

## UI Requirements

### Menu Placement
- Main menu: `Window -> Sequences`

### Sequence Manager Window

**Layout:**
```
+--------------------------------------------------+
| Toolbar: [Create] [Alter] [Drop] [Refresh]       |
+--------------------------------------------------+
| +------------------+  +-----------------------+  |
| | Sequence List    |  | Details Panel         |  |
| |                  |  |                       |  |
| | • seq_users      |  | Name: seq_users       |  |
| | • seq_orders     |  | Type: INTEGER         |  |
| | • ...            |  | Start: 1              |  |
| |                  |  | Increment: 1          |  |
| |                  |  | Min: 1                |  |
| |                  |  | Max: 9223372036854775 |  |
| |                  |  | Current: 1543         |  |
| |                  |  | Next: 1544            |  |
| |                  |  | Cache: 1              |  |
| |                  |  | Cycle: No             |  |
| +------------------+  +-----------------------+  |
+--------------------------------------------------+
| Status: 3 sequences                              |
+--------------------------------------------------+
```

**Sequence List Columns:**
- Name
- Data Type
- Current Value (if available)
- Increment
- Schema (if applicable)

**Details Panel Tabs:**
- **General**: Basic properties (name, type, start, increment)
- **Limits**: Min/max values, cycle behavior
- **Statistics**: Current value, next value, cache info

### Sequence Editor Dialog (Create)

**Fields:**
- **Sequence Name** (required)
- **Schema** (dropdown, default: current schema)
- **Data Type** (INTEGER, BIGINT, SMALLINT)
- **Start Value** (default: 1)
- **Increment** (default: 1)
- **Minimum Value** (optional, default: 1)
- **Maximum Value** (optional)
- **Cache Size** (default: 1)
- **Cycle** (checkbox, default: false)
- **IF NOT EXISTS** (checkbox)

### Sequence Editor Dialog (Alter)

**Read-Only Fields:**
- Sequence Name
- Data Type

**Editable Fields:**
- **Action Selector:**
  - RESTART [WITH value]
  - INCREMENT BY value
  - SET MINVALUE value / NO MINVALUE
  - SET MAXVALUE value / NO MAXVALUE
  - SET CACHE value
  - CYCLE / NO CYCLE
  - RENAME TO new_name
- **Value Input** (contextual based on action)

### Drop Confirmation
- Show sequence name
- Warning about dependencies (tables using the sequence)
- CASCADE/RESTRICT options (backend-dependent)

---

## Implementation Details

### Files
- `src/ui/sequence_manager_frame.h` / `.cpp` - Main window
- `src/ui/sequence_editor_dialog.h` / `.cpp` - Create/Alter dialogs

### Key Classes
```cpp
class SequenceManagerFrame : public wxFrame {
    // Sequence list grid
    // Details panel
    // Action handlers
};

class SequenceEditorDialog : public wxDialog {
    // Create/Alter sequence UI
    // SQL generation
};
```

### Backend Integration
- PostgreSQL: Uses `pg_sequences` catalog view
- MySQL: Simulates sequences via AUTO_INCREMENT
- Firebird: Uses `RDB$GENERATORS` system table
- ScratchBird: Uses `sb_catalog.sb_sequences`

### Capability Gating
- Window enabled when connected
- Actions disabled during query execution
- Backend-specific features shown/hidden based on capability detection

---

## Future Enhancements

- Visual sequence value graph over time
- Sequence usage statistics
- Batch sequence operations
- Sequence comparison between databases
