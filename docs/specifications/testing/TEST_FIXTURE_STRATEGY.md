# Test Fixture Strategy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how test data and fixture schemas are managed for unit and integration tests.

---

## Fixtures

- Stored under `tests/fixtures/`.
- Each fixture set includes:
  - Schema DDL
  - Seed data
  - Expected results

---

## Rules

- Fixtures must be deterministic and minimal.
- Seed data should avoid PII.
- Large datasets should be generated, not stored.

---

## Example Layout

```
tests/fixtures/
  basic_schema/
    schema.sql
    seed.sql
    expected.json
```

---

## Edge Cases

- If fixture fails to load, tests should skip with clear message.

