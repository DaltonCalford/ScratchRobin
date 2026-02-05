# Test Generation Rules

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how ScratchRobin generates automated tests for schema and data quality.

---

## Test Types

- **Schema Tests**: Validate presence of tables, columns, constraints.
- **Data Quality Tests**: Null %, uniqueness, referential integrity.
- **Performance Smoke Tests**: Basic query execution timing.

---

## Generation Rules

1. For each table, generate a table-exists test.
2. For each PK, generate uniqueness test.
3. For each FK, generate referential integrity test (requires config).

---

## Example Output

```sql
SELECT 1 FROM information_schema.tables
WHERE table_schema='public' AND table_name='users';
```

---

## Edge Cases

- Large tables: avoid full scans; use sampled queries.
- Missing privileges: tests should fail with clear errors.

