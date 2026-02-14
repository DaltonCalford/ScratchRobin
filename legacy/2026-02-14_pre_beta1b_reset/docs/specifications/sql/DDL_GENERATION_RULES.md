# DDL Generation Rules (ScratchRobin)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the rules ScratchRobin uses to generate DDL for project objects in ScratchBirdSQL.

---

## General Principles

- Generate deterministic, idempotent DDL where possible.
- Use explicit schema qualification for all objects.
- Use stable ordering for columns and constraints.
- Avoid backend-specific clauses unless explicitly required.

---

## Table Generation

### Column Ordering

- Preserve user-defined order.
- Primary key columns should be listed in table-level constraint if multi-column.

### Constraints

- Inline constraints allowed for NOT NULL and single-column PRIMARY KEY.
- All other constraints emitted at table-level in deterministic order:
  1. PRIMARY KEY
  2. UNIQUE
  3. CHECK
  4. FOREIGN KEY

### Example

```sql
CREATE TABLE public.orders (
  id INT64 PRIMARY KEY,
  user_id INT64 NOT NULL,
  total NUMERIC(12,2) NOT NULL,
  created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_orders_user FOREIGN KEY (user_id)
    REFERENCES public.users (id)
);
```

---

## Index Generation

- Indexes emitted separately from `CREATE TABLE`.
- Index names required and must be stable across re-generations.

Example:

```sql
CREATE INDEX idx_orders_user_id ON public.orders (user_id);
```

---

## View Generation

- Views emitted as:
  - `CREATE VIEW schema.name AS SELECT ...;`
- No `OR REPLACE` in v1.

---

## Procedure/Function Generation

- Use ScratchBirdSQL procedural syntax.
- Emit explicit language specifier if needed.

---

## Edge Cases

- **Rename**: treated as DROP + CREATE in v1 migrations.
- **Type narrowing**: if a change reduces precision/scale, emit warning.
- **Constraint name collisions**: generate deterministic suffixes.

---

## Implementation Notes

- Use a shared SQL builder to avoid UI-specific formatting differences.
- All DDL generation must be tested with ScratchBirdSQL parser.

