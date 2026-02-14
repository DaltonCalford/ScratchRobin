# Rollback Policy

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines when and how deployment rollback is allowed, and how rollback scripts are generated and executed.

---

## Rollback Rules

- Rollback is optional and must be explicitly requested.
- Rollback scripts are generated alongside forward migrations when possible.
- Destructive changes may be non-reversible and must be flagged.

---

## Failure Modes

- Rollback script missing: require explicit user confirmation.
- Rollback failure: mark plan as ROLLBACK_PARTIAL and stop further rollback.

---

## Example

```sql
-- Rollback for V0002__add_orders
DROP TABLE public.orders;
```

