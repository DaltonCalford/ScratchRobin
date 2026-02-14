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

## Implementation Notes (ScratchRobin)

These notes describe the intended implementation wiring for offline development:

- **Storage**: Reporting assets live in the project file under the `RPTG` chunk (see `PROJECT_PERSISTENCE_FORMAT.md`).
- **Runtime Context**: Each execution must resolve a `connection_ref`, role context, and environment gates before running.
- **Cache**: In-memory cache keyed by normalized SQL + params + connection + model version. Persistent cache is optional.
- **Scheduler**: Runs via the project job queue; refresh and alert evaluations are background tasks.
  - Project persistence stores reporting schedules in the `RPTS` chunk for offline execution.
  - ScratchRobin UI ticks schedules locally (e.g., reporting panel timer) when no server is available.
- **Audit**: Emits project audit entries with `action=run|refresh|alert_eval|subscription`.
- **Failure**: For scheduled runs, mark the schedule as failed after configured retries and notify owners.

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
- Optional `metadata` (UI panel name, schedule id, dashboard id, etc.)

---

## Related Specs

- `docs/specifications/reporting/CACHING_AND_REFRESH.md`
- `docs/specifications/reporting/ALERTS_AND_SUBSCRIPTIONS.md`
- `docs/specifications/core/PROJECT_DEFINITION_AND_GOVERNANCE.md`
