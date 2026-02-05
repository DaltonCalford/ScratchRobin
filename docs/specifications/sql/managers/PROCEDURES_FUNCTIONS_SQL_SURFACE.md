# Procedures and Functions SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for procedures and functions emitted by ScratchRobin.

---

## Supported Statements

### Emitted

- `CREATE PROCEDURE`
- `CREATE FUNCTION`
- `DROP PROCEDURE`
- `DROP FUNCTION`

---

## CREATE PROCEDURE Rules

- Schema-qualify names.
- Explicit parameter list.
- Use ScratchBirdSQL procedural body.

**Example**

```sql
CREATE PROCEDURE public.sp_update_user_email(IN user_id INT64, IN new_email VARCHAR(255))
BEGIN
  UPDATE public.users SET email = new_email WHERE id = user_id;
END;
```

---

## CREATE FUNCTION Rules

- Schema-qualify names.
- Explicit return type.

**Example**

```sql
CREATE FUNCTION public.fn_user_count()
RETURNS INT64
BEGIN
  RETURN (SELECT COUNT(*) FROM public.users);
END;
```

---

## Edge Cases

- If body contains unsupported constructs, emit warning.
- Changes to signature require DROP + CREATE in v1 migrations.

