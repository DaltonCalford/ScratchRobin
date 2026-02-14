# Domains (Custom Types) SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines SQL for domain/custom type creation and deletion.

---

## Supported Statements

### Emitted

- `CREATE DOMAIN`
- `ALTER DOMAIN`
- `DROP DOMAIN`

---

## CREATE DOMAIN Rules

- Schema-qualify domain name.
- Base type must be a ScratchBirdSQL type.

**Example**

```sql
CREATE DOMAIN public.email_type AS VARCHAR(255)
  CHECK (VALUE LIKE '%@%');
```

---

## ALTER DOMAIN Rules

- Only constraints and default value can be altered in v1.

---

## Edge Cases

- Changing base type is treated as DROP + CREATE.

