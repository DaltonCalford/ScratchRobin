# Views SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL rules for view creation, update, and deletion.

---

## Supported Statements

### Emitted

- `CREATE VIEW`
- `DROP VIEW`

### Accepted

- All emitted statements
- `CREATE OR REPLACE VIEW` (accepted in editor but normalized to DROP+CREATE in migrations)

---

## CREATE VIEW Rules

- Always schema-qualify.
- Do not emit `OR REPLACE` in v1 migrations.

**Example**

```sql
CREATE VIEW public.active_users AS
SELECT id, email
FROM public.users
WHERE active = true;
```

---

## DROP VIEW Rules

- Emit `DROP VIEW schema.name;`

---

## Edge Cases

- If a view depends on a changed table, view is regenerated in migration.
- Column rename inside view definition is treated as view recreation.

