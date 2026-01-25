# ScratchRobin Schema Manager (ScratchBird)

Status: Draft  
Last Updated: 2026-01-09

This specification defines ScratchRobin's UI and SQL wiring for ScratchBird
schema management.

## Scope

- ScratchBird-native only (no emulated backends).
- Create/alter/drop schemas.
- Read-only schema details via `SHOW SCHEMA`.

Out of scope (Alpha):
- Schema-level GRANT/REVOKE UI.
- Search-path editor UI beyond `ALTER SCHEMA SET PATH`.

## SQL Surface (Parser Support)

### CREATE SCHEMA

```
CREATE SCHEMA [IF NOT EXISTS] schema_name
  [AUTHORIZATION owner_name]
```

### ALTER SCHEMA

```
ALTER SCHEMA schema_name RENAME TO new_name
ALTER SCHEMA schema_name OWNER TO new_owner
ALTER SCHEMA schema_name SET PATH search_path
```

### DROP SCHEMA

```
DROP SCHEMA schema_name
DROP SCHEMA schema_name CASCADE
DROP SCHEMA schema_name RESTRICT
```

### SHOW SCHEMAS / SHOW SCHEMA

```
SHOW SCHEMAS
SHOW SCHEMA schema_name
```

`SHOW SCHEMA` is detail-only and requires a name. Listing uses `SHOW SCHEMAS`.

## Query Wiring Plan (ScratchBird)

List schemas:
```
SHOW SCHEMAS
```

Schema details:
```
SHOW SCHEMA schema_name
```

## ScratchRobin UI Requirements

### Menu Placement

- Main menu: `Window -> Schemas`

### Schema Manager Window

Layout:
- Schema list grid (left)
- Details panel (right)
- Status + message bar

Actions:
- Create / Alter / Drop
- Refresh

Details panel:
- Uses `SHOW SCHEMA` output to show properties and values.

### Schema Editor (Create)

Fields:
- Schema name
- IF NOT EXISTS
- Authorization (owner)

Rules:
- Schema name is required.
- Authorization is optional and emitted as `AUTHORIZATION <owner>`.

### Schema Editor (Alter)

Fields:
- Schema name (read-only)
- Action selector:
  - RENAME TO
  - OWNER TO
  - SET PATH
- Value input (enabled only when needed)

### Capability Gating

- Window only shown for ScratchBird-native connections.
- Actions disabled when not connected or query is in flight.
