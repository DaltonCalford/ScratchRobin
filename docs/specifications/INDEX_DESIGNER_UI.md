# ScratchRobin Index Designer (ScratchBird)

Status: Draft  
Last Updated: 2026-01-09

This specification defines ScratchRobin's UI and SQL wiring for ScratchBird
index management.

## Scope

- ScratchBird-native only (no emulated backends).
- Create/alter/drop indexes.
- Read-only index details via `SHOW INDEX` and `SHOW INDEXES`.

Out of scope (Alpha):
- Visual column builder (raw text entry only).
- Rebuild progress tracking.

## SQL Surface (Parser Support)

### CREATE INDEX

```
CREATE [UNIQUE] INDEX [IF NOT EXISTS] index_name
  ON table_name
  [USING index_type]
  (column_expr [, ...])
  [INCLUDE (column [, ...])]
  [WHERE predicate]
  [index_options]
```

### ALTER INDEX

```
ALTER INDEX index_name RENAME TO new_name
ALTER INDEX index_name SET TABLESPACE tablespace_name
ALTER INDEX index_name SET SCHEMA schema_path
ALTER INDEX index_name SET options
ALTER INDEX index_name REBUILD
```

### DROP INDEX

```
DROP INDEX index_name
```

### SHOW INDEX / SHOW INDEXES

```
SHOW INDEX index_name
SHOW INDEXES FROM table_name
```

## Query Wiring Plan (ScratchBird)

List indexes:
```
SELECT name, schema_name, table_name, index_type, is_unique
FROM sb_catalog.sb_indexes
WHERE name NOT LIKE 'sb_%'
ORDER BY schema_name, table_name, name
```

Index details:
```
SHOW INDEX index_name
```

Index columns:
```
SHOW INDEXES FROM table_name
```

The columns grid filters the `SHOW INDEXES` result to the selected index.

## ScratchRobin UI Requirements

### Menu Placement

- Main menu: `Window -> Indexes`

### Index Designer Window

Layout:
- Index list grid (left)
- Details panel (right) with tabs
  - Definition (text)
  - Columns (grid)
- Status + message bar

Actions:
- Create / Alter / Drop
- Refresh

### Index Editor (Create)

Fields:
- Index name
- Table name
- IF NOT EXISTS
- UNIQUE
- Index type (DEFAULT/BTREE/HASH/GIN/GIST/BRIN/RTREE/SPGIST/FULLTEXT)
- Index columns (one per line)
- INCLUDE columns (one per line)
- WHERE clause (optional)
- Index options (raw)

Rules:
- Index name, table name, and at least one column are required.
- Options are appended verbatim at the end of the CREATE statement.

### Index Editor (Alter)

Fields:
- Index name (read-only)
- Action selector:
  - RENAME TO
  - SET TABLESPACE
  - SET SCHEMA
  - SET OPTIONS
  - REBUILD
- Value input (enabled only when needed)

### Capability Gating

- Window only shown for ScratchBird-native connections.
- Actions disabled when not connected or query is in flight.
