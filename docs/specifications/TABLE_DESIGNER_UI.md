# ScratchRobin Table Designer (ScratchBird)

Status: Draft  
Last Updated: 2026-01-09

This specification defines ScratchRobin's UI and SQL wiring for ScratchBird
table management.

## Scope

- ScratchBird-native only (no emulated backends).
- Create/alter/drop tables.
- Read-only table details via `SHOW CREATE TABLE` and `SHOW COLUMNS`.

Out of scope (Alpha):
- Visual column editor (raw text entry only).
- Table-level GRANT/REVOKE UI.

## SQL Surface (Parser Support)

### CREATE TABLE

```
CREATE TABLE [IF NOT EXISTS] table_name (
  column_definitions,
  table_constraints
)
[table_options]
```

### ALTER TABLE

```
ALTER TABLE table_name ADD column_definition
ALTER TABLE table_name DROP COLUMN column_name
ALTER TABLE table_name RENAME COLUMN old_name TO new_name
ALTER TABLE table_name RENAME TO new_name
ALTER TABLE table_name ADD constraint_clause
ALTER TABLE table_name DROP CONSTRAINT constraint_name
ALTER TABLE table_name RENAME CONSTRAINT old_name TO new_name
ALTER TABLE table_name SET TABLESPACE tablespace_name
ALTER TABLE table_name SET SCHEMA schema_path
```

### DROP TABLE

```
DROP TABLE table_name
DROP TABLE table_name CASCADE
DROP TABLE table_name RESTRICT
```

### SHOW CREATE TABLE / SHOW COLUMNS

```
SHOW CREATE TABLE table_name
SHOW COLUMNS FROM table_name
```

## Query Wiring Plan (ScratchBird)

List tables:
```
SELECT name, schema_name
FROM sb_catalog.sb_tables
WHERE name NOT LIKE 'sb_%'
ORDER BY schema_name, name
```

Table details:
```
SHOW CREATE TABLE table_name
```

Table columns:
```
SHOW COLUMNS FROM table_name
```

## ScratchRobin UI Requirements

### Menu Placement

- Main menu: `Window -> Tables`

### Table Designer Window

Layout:
- Table list grid (left)
- Details panel (right) with tabs
  - Definition (text)
  - Columns (grid)
- Status + message bar

Actions:
- Create / Alter / Drop
- Refresh

### Table Editor (Create)

Fields:
- Table name
- IF NOT EXISTS
- Columns (one per line)
- Table constraints (one per line)
- Table options (raw)

Rules:
- At least one column or constraint is required.
- Options are appended verbatim after the closing parenthesis.

### Table Editor (Alter)

Fields:
- Table name (read-only)
- Action selector:
  - ADD COLUMN
  - DROP COLUMN
  - RENAME COLUMN
  - RENAME TABLE
  - ADD CONSTRAINT
  - DROP CONSTRAINT
  - RENAME CONSTRAINT
  - SET TABLESPACE
  - SET SCHEMA
- Value input(s) based on action

### Capability Gating

- Window only shown for ScratchBird-native connections.
- Actions disabled when not connected or query is in flight.
