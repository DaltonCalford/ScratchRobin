# Triggers SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for trigger creation and deletion.

---

## Supported Statements

### Emitted

- `CREATE TRIGGER`
- `DROP TRIGGER`

---

## CREATE TRIGGER Rules

- Always schema-qualify.
- Trigger timing must be explicit (BEFORE/AFTER).
- Events: INSERT, UPDATE, DELETE.

**Example**

```sql
CREATE TRIGGER tr_users_update
AFTER UPDATE ON public.users
FOR EACH ROW
BEGIN
  -- trigger body
END;
```

---

## DROP TRIGGER Rules

- Emit `DROP TRIGGER schema.trigger_name;`

---

## Edge Cases

- Multiple events must be expanded explicitly.
- Disabled triggers should be represented as metadata flags (no SQL in v1).

