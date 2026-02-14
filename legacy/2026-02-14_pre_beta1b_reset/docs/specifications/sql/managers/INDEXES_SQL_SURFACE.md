# Indexes SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for index creation and deletion.

---

## Supported Statements

### Emitted

- `CREATE INDEX`
- `DROP INDEX`

---

## CREATE INDEX Rules

- Always schema-qualify table.
- Index name must be stable and deterministic.

**Example**

```sql
CREATE INDEX idx_users_email ON public.users (email);
```

---

## DROP INDEX Rules

- Emit `DROP INDEX schema.index_name;`

---

## Edge Cases

- If index is tied to constraint, prefer dropping constraint.
- Unique indexes must be emitted as `CREATE UNIQUE INDEX`.

