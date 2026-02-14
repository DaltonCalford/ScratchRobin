# Sequences SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for sequence creation and deletion.

---

## Supported Statements

### Emitted

- `CREATE SEQUENCE`
- `ALTER SEQUENCE`
- `DROP SEQUENCE`

---

## CREATE SEQUENCE Rules

- Always schema-qualify.
- Defaults should be explicit when non-default.

**Example**

```sql
CREATE SEQUENCE public.user_id_seq START WITH 1 INCREMENT BY 1;
```

---

## ALTER SEQUENCE Rules

- Supported changes: increment, restart, min/max, cycle.

---

## Edge Cases

- If sequence is tied to a column default, ensure updates reflect the dependency.

