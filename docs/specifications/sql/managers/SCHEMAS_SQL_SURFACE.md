# Schemas SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for schema creation and deletion.

---

## Supported Statements

### Emitted

- `CREATE SCHEMA`
- `DROP SCHEMA`

---

## CREATE SCHEMA Rules

- Always use explicit schema name.
- If ownership is supported, use explicit `AUTHORIZATION` clause.

**Example**

```sql
CREATE SCHEMA analytics;
```

---

## DROP SCHEMA Rules

- Emit `DROP SCHEMA name;`
- Require explicit confirmation if schema is non-empty.

