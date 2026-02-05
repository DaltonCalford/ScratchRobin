# Reporting Execution Engine (ScratchBird Only)

**Status**: Draft  
**Last Updated**: 2026-02-05

---

## Purpose

Defines how reporting queries are executed, cached, refreshed, and governed for ScratchBird-only connections.

---

## Core Components

- **Query Runner**: Executes ScratchBirdSQL or builder-generated SQL.
- **Cache Layer**: Stores result sets by query + parameter signature.
- **Scheduler**: Runs refresh, alerts, and subscriptions.
- **Permission Gate**: Enforces project roles at execution time.
- **Audit Logger**: Records executions and outputs.

---

## Execution Flow

1. Validate permissions for execution scope.
2. Normalize query + parameters to a cache key.
3. If cache hit and policy allows, return cached result.
4. Execute query using ScratchBird connection.
5. Store result (if cache policy allows).
6. Emit audit event.

---

## Caching Rules

- Cache keys include: SQL, parameters, connection_ref, role context, and model version.
- Default TTL:
  - Question: 15 minutes
  - Dashboard: 30 minutes
  - Alerts: bypass cache
- Cache invalidates on:
  - Schema changes to referenced objects
  - Explicit manual refresh
  - Governance policy change

---

## Scheduler Rules

- Supports hourly, daily, weekly schedules and optional cron.
- Alert evaluation bypasses cache unless explicitly configured.
- Subscriptions run using stored filter state.

---

## Failure Handling

- Query failure logs: error code, SQLSTATE, and truncated SQL.
- Retries for scheduled runs: default 1 retry with exponential backoff.
- Persistent failures disable the schedule and notify owners.

---

## Permissions

Execution requires:

- **View** access to the collection
- **Docs** permission for refresh or subscriptions
- **Deploy** or approved policy for scheduled refreshes

---

## Audit Events

Required audit fields:

- `event_id`, `timestamp`, `actor`
- `action` (run, refresh, alert_eval, subscription)
- `target_id`
- `connection_ref`
- `status` (success/fail)

---

## Related Specs

- `docs/specifications/reporting/CACHING_AND_REFRESH.md`
- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
