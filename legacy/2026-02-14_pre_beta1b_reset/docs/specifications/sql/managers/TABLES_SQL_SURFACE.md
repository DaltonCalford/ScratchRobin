# Tables SQL Surface (ScratchBirdSQL)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the SQL ScratchRobin emits and accepts for table creation, alteration, and deletion in ScratchBirdSQL.

---

## Supported Statements

### Emitted

- `CREATE TABLE`
- `ALTER TABLE` (add/drop/alter columns, add/drop constraints)
- `DROP TABLE`

### Accepted

- All emitted statements
- `TRUNCATE TABLE` (execution only; no model change unless explicitly confirmed)

---

## CREATE TABLE Rules

- Always schema-qualify.
- Column order is preserved.
- Inline constraints allowed for:
  - `NOT NULL`
  - single-column `PRIMARY KEY`
- Table-level constraints required for multi-column keys and foreign keys.

**Example**

```sql
CREATE TABLE public.users (
  id INT64 PRIMARY KEY,
  email VARCHAR(255) NOT NULL,
  created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT uq_users_email UNIQUE (email)
);
```

---

## ALTER TABLE Rules

Supported alterations:
- `ADD COLUMN`
- `DROP COLUMN`
- `ALTER COLUMN TYPE`
- `ALTER COLUMN SET/DROP DEFAULT`
- `ALTER COLUMN SET/DROP NOT NULL`
- `ADD CONSTRAINT`
- `DROP CONSTRAINT`

**Example**

```sql
ALTER TABLE public.users ADD COLUMN last_login TIMESTAMPTZ;
```

---

## DROP TABLE Rules

- Emit `DROP TABLE schema.name;`
- If dependencies exist, warn and require confirmation.

---

## Edge Cases

- Column rename is emitted as `DROP COLUMN` + `ADD COLUMN` in v1.
- Type narrowing emits a warning.
- Dropping a column used by an index must drop index first or within same migration step.

---

## Validation

- All statements must parse via ScratchBirdSQL grammar.
- Identifiers must be quoted if reserved keywords.

