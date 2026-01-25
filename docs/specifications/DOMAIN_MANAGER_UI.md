# ScratchRobin Domain Manager (ScratchBird)

Status: Draft  
Last Updated: 2026-01-09

This specification defines ScratchRobin’s UI and SQL wiring for ScratchBird
domain management. Domains are global (not schema-bound). It aligns to the
current V2 parser and executor behavior.

## Scope

- ScratchBird-native only (no emulated backends).
- Create/alter/drop domains.
- Read-only domain details via `SHOW DOMAIN`.

Out of scope (Alpha):
- Domain-level GRANT/REVOKE UI.
- Advanced WITH block builders (raw text only).

## SQL Surface (Parser Support)

### CREATE DOMAIN

```
CREATE DOMAIN [IF NOT EXISTS] domain_name
  [AS] base_type
  [NOT NULL]
  [DEFAULT expr]
  [CHECK (expr)]
  [COLLATE collation_name]
  [INHERITS (parent_domain)]
  [WITH ...]
```

Domain kinds:
- BASIC: `AS <type>`
- RECORD: `AS RECORD (field_name type [NOT NULL] [DEFAULT expr], ...)`
- ENUM: `AS ENUM ('label' = n, ...)`
- SET: `AS SET OF <type>`
- VARIANT: `AS VARIANT (type_a, type_b, ...)`

### ALTER DOMAIN

```
ALTER DOMAIN domain_name SET DEFAULT expr
ALTER DOMAIN domain_name DROP DEFAULT
ALTER DOMAIN domain_name ADD CHECK (expr)
ALTER DOMAIN domain_name DROP CONSTRAINT constraint_name
ALTER DOMAIN domain_name SET COMPAT compat_name
ALTER DOMAIN domain_name DROP COMPAT
ALTER DOMAIN domain_name RENAME TO new_name
```

### DROP DOMAIN

```
DROP DOMAIN domain_name
```

### SHOW DOMAIN

```
SHOW DOMAIN domain_name
```

`SHOW DOMAIN` is detail-only and requires a name. Listing uses the catalog view
query below.

## Query Wiring Plan (ScratchBird)

List domains (primary):
```
SELECT name
FROM sb_catalog.sb_domains
ORDER BY name;
```

Domain details:
```
SHOW DOMAIN 'domain_name';
```

## ScratchRobin UI Requirements

### Menu Placement

- Main menu: `Window -> Domains`

### Domain Manager Window

Layout:
- Domain list grid (left)
- Details panel (right)
- Status + message bar

Actions:
- Create / Alter / Drop
- Refresh

Details panel:
- Uses `SHOW DOMAIN` output to show properties and values.

### Domain Editor (Create)

Fields:
- Domain name
- IF NOT EXISTS
- Domain kind (BASIC, RECORD, ENUM, SET, VARIANT)
- Type definition (kind-specific)
- NOT NULL, DEFAULT, CHECK
- Extra constraints (raw lines)
- Collation
- Inherits
- WITH blocks (raw)

Rules:
- BASIC domains require a base type.
- RECORD/ENUM/VARIANT require at least one item.
- SET requires an element type.
- WITH blocks are appended verbatim (prefix `WITH` if missing).

### Domain Editor (Alter)

Fields:
- Domain name (read-only)
- Action selector:
  - SET DEFAULT
  - DROP DEFAULT
  - ADD CHECK
  - DROP CONSTRAINT
  - SET COMPAT
  - DROP COMPAT
  - RENAME
- Value input (enabled only when needed)

### Capability Gating

- Window only shown for ScratchBird-native connections.
- Actions disabled when not connected or query is in flight.
