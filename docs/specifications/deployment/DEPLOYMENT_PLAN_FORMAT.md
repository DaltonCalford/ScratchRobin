# Deployment Plan Format

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines the structure of deployment plans used to execute migrations, validations, and post-deploy checks in ScratchRobin.

---

## File Format

Deployment plans are JSON documents stored in `deployments/` with extension `.srdeploy`.

---

## Schema (v1)

```json
{
  "version": "1.0",
  "plan_id": "uuid",
  "name": "Deploy v0.2.1",
  "target_env": {
    "id": "staging",
    "name": "Staging",
    "backend": "scratchbird",
    "connection_ref": "conn_staging"
  },
  "created_at": "2026-02-05T12:00:00Z",
  "created_by": "alice",
  "approval_required": true,
  "pre_checks": [
    {"id": "check_connection", "type": "connection"},
    {"id": "check_disk", "type": "disk_space", "min_gb": 5}
  ],
  "steps": [
    {
      "id": "step_1",
      "name": "Apply migration V0002",
      "type": "migration",
      "script": "migrations/V0002__add_orders.sql",
      "rollback": "migrations/R0002__rollback.sql",
      "timeout_sec": 600,
      "retry": {"max": 1, "backoff_sec": 10}
    },
    {
      "id": "step_2",
      "name": "Verify indexes",
      "type": "sql_check",
      "sql": "SELECT 1 FROM sys.indexes WHERE name='idx_orders_user'",
      "expect": "non_empty"
    }
  ],
  "post_checks": [
    {"id": "check_migrations", "type": "migration_history"},
    {"id": "check_object_count", "type": "object_count", "min": 100}
  ]
}
```

---

## Step Types

| Type | Purpose | Required Fields |
| --- | --- | --- |
| migration | Apply SQL migration file | `script` |
| sql_check | Run SQL and validate | `sql`, `expect` |
| backup | Create backup snapshot | `target` |
| pause | Manual gate | `message` |

---

## Execution Semantics

- Steps execute sequentially.
- A failed step stops execution unless `retry` allows reattempt.
- Rollback is executed only if explicitly requested.

---

## Failure Modes

- **Missing script**: fail before execution.
- **Timeout**: mark step failed; log error context.
- **Connection lost**: mark plan FAILED; allow resume.
- **Partial execution**: record last successful step.

---

## Recovery and Resume

- Plans can be resumed from the last successful step.
- Resume requires revalidation of pre-checks.

