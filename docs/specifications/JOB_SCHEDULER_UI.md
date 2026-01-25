# ScratchRobin Job Scheduler Integration (ScratchBird)

Status: Draft  
Last Updated: 2026-01-09

This specification defines ScratchRobin’s UI and data contracts for the
ScratchBird job scheduler (single-database). It is based on the current
ScratchBird parser and core scheduler implementation.

## Scope

- ScratchBird-native only (no emulated backends).
- Single database scheduler (no cluster assignment).
- Job creation, modification, execution, cancellation, and run history.
- Read-only view of scheduler configuration (with optional ALTER SYSTEM edits).

Out of scope (Alpha):
- Cluster job assignment, shard-aware runs.

## Server Behavior Summary (Observed)

Scheduler:
- Per-database JobScheduler thread.
- Poll interval: `scheduler.polling_interval_seconds` (default 10).
- Executes up to `scheduler.max_jobs_per_tick` per cycle (default 16).
- Concurrency cap: `scheduler.max_concurrent_jobs` (default 10).
- Default run timeout: `scheduler.job_timeout_seconds` (default 3600).
- Cron evaluation uses 5 fields and UTC (gmtime).
- SHOW commands provide the canonical query surface for jobs and runs.
- `CREATE OR ALTER JOB` and `RECREATE JOB` are supported for job upserts.

Built-in seeded jobs:
- `daily_sweep` (CRON `0 2 * * *`, `SWEEP DATABASE`, enabled)
- `update_stats` (CRON `0 * * * *`, `ANALYZE`, enabled)
- `rebuild_indexes` (CRON `0 3 * * 0`, `REINDEX DATABASE`, disabled)

Permissions:
- `CREATE JOB` privilege (or DB_OWNER/superuser) required to create jobs.
- Alter/Drop: job owner, job admin, DB_OWNER, or superuser.
- Execute/Cancel: job owner, job admin, or `EXECUTE` on the job.
- `EXECUTE EXTERNAL JOB` privilege required for external job bodies.
- `VIEW JOB HISTORY` privilege expands SHOW access across jobs/runs.

## SQL Surface (Parser Support)

### CREATE JOB

```
CREATE JOB job_name
  SCHEDULE = CRON 'min hour dom mon dow'
  [DEPENDS ON job_a, job_b]
  [CLASS = class_name]
  [PARTITION BY ALL_SHARDS | SINGLE_SHARD 'uuid' | SHARD_SET 'expr' | DYNAMIC 'expr']
  [MAX_RETRIES = n]
  [RETRY_BACKOFF = n [S|M|H|D]]
  [TIMEOUT = n [S|M|H|D]]
  [ON COMPLETION PRESERVE | DROP]
  [RUN AS role_name]
  [DESCRIPTION = 'text']
  [STATE = ENABLED | DISABLED | PAUSED]
  AS 'SQL text'
  | CALL schema.proc()
  | EXEC 'external command'
```

Schedule variants:
- `SCHEDULE = AT 'timestamp'`
- `SCHEDULE = EVERY n [unit] [STARTS 'timestamp'] [ENDS 'timestamp']`

Units: `S/SEC/SECOND`, `M/MIN/MINUTE`, `H/HOUR`, `D/DAY` (case-insensitive).

Also supported:
```
CREATE OR ALTER JOB job_name ...
RECREATE JOB job_name ...
```

### ALTER JOB

```
ALTER JOB job_name
  [SET] SCHEDULE = CRON '...'
  [SET] SCHEDULE = AT 'timestamp'
  [SET] SCHEDULE = EVERY n [unit] [STARTS 'timestamp'] [ENDS 'timestamp']
  [SET] STATE = ENABLED | DISABLED | PAUSED
  [SET] MAX_RETRIES = n
  [SET] RETRY_BACKOFF = n [unit]
  [SET] TIMEOUT = n [unit]
  [SET] RUN AS role_name
  [SET] DESCRIPTION = 'text'
  [SET] ON COMPLETION PRESERVE | DROP
  [SET] DEPENDS ON job_a, job_b | DEPENDS ON NONE
  [SET] CLASS = class_name
  [SET] PARTITION BY ALL_SHARDS | SINGLE_SHARD 'uuid' | SHARD_SET 'expr' | DYNAMIC 'expr'
  [SET] AS 'SQL text'
  [SET] CALL schema.proc()
  [SET] EXEC 'external command'
```

Notes:
- `DEPENDS ON NONE` clears dependencies.
- `PAUSED` behaves like a non-running state (scheduler only executes ENABLED).
- External job body changes require `EXECUTE EXTERNAL JOB` privilege.

### DROP JOB

```
DROP JOB job_name [KEEP HISTORY]
```

### EXECUTE JOB

```
EXECUTE JOB job_name
```

Execution runs the job immediately and records a job_run entry.

### CANCEL JOB RUN

```
CANCEL JOB RUN 'job_run_uuid'
```

The UUID parser accepts brace/URN formats. Cancellation updates run state and
interrupts execution when supported by the server.

### SHOW JOBS / SHOW JOB / SHOW JOB RUNS

```
SHOW JOBS [LIKE 'pattern']
SHOW JOB job_name
SHOW JOB RUNS [FOR] job_name
```

`SHOW JOB` currently returns a subset of fields (schedule/type/state). Richer
details should come from `sys.jobs` when available.

### GRANT / REVOKE JOB

```
GRANT EXECUTE ON JOB job_name TO user_or_role;
REVOKE EXECUTE ON JOB job_name FROM user_or_role;
```

## Scheduler Config Keys

```
[scheduler]
enabled = true
polling_interval_seconds = 10
max_jobs_per_tick = 16
max_concurrent_jobs = 10
job_timeout_seconds = 3600
cron_fallback_seconds = 60
pre_execute_delay_ms = 0
```

`ALTER SYSTEM SET scheduler.key = value` applies config at runtime.

## Catalog Visibility (Confirmed/Assumed)

- ScratchBird core stores jobs in catalog heap tables (JobRecord/JobRunRecord/
  JobDependencyRecord).
- The canonical query surface is `SHOW JOBS`, `SHOW JOB`, and `SHOW JOB RUNS`.
- If `sys.jobs`, `sys.job_runs`, and `sys.job_dependencies` views are exposed,
  ScratchRobin may use them for richer details.

## Data Contracts (ScratchRobin Expectations)

ScratchRobin expects system catalog views (or table equivalents) aligned to
the server’s JobInfo/JobRunInfo fields when exposed. Otherwise, the UI relies
on SHOW commands and hides fields that are not available.

### sys.jobs

Columns (recommended):
- `job_id` UUID
- `job_name` VARCHAR
- `description` TEXT
- `job_type` TEXT (`SQL`, `PROCEDURE`, `EXTERNAL`)
- `job_sql` TEXT
- `procedure_uuid` UUID
- `external_command` TEXT
- `schedule_kind` TEXT (`CRON`, `AT`, `EVERY`)
- `cron_expression` TEXT
- `interval_seconds` BIGINT
- `starts_at` BIGINT (ms since epoch)
- `ends_at` BIGINT (ms since epoch)
- `schedule_tz` VARCHAR
- `next_run_time` BIGINT (ms since epoch)
- `on_completion` TEXT (`PRESERVE`, `DROP`)
- `partition_strategy` TEXT
- `partition_shard_uuid` UUID
- `partition_expression` TEXT
- `max_retries` INT
- `retry_backoff_seconds` INT
- `timeout_seconds` INT
- `created_by_user_uuid` UUID
- `run_as_role_uuid` UUID
- `created_at` BIGINT (ms since epoch)
- `state` TEXT (`ENABLED`, `DISABLED`, `PAUSED`)

### sys.job_runs

Columns (recommended):
- `job_run_id` UUID
- `job_id` UUID
- `assigned_node_uuid` UUID
- `shard_uuid` UUID
- `scheduled_time` BIGINT (ms since epoch)
- `started_at` BIGINT (ms since epoch)
- `completed_at` BIGINT (ms since epoch)
- `state` TEXT (`PENDING`, `RUNNING`, `COMPLETED`, `FAILED`, `CANCELLED`)
- `retry_count` INT
- `rows_affected` BIGINT
- `error_code` INT
- `result_message` TEXT

### sys.job_dependencies

Columns (recommended):
- `job_id` UUID
- `depends_on_job_id` UUID
- `created_time` BIGINT (ms since epoch)

## Query Wiring Plan (ScratchBird)

List jobs (primary):
```
SHOW JOBS;
```

List jobs (filtered):
```
SHOW JOBS LIKE 'pattern';
```

Job details (primary):
```
SHOW JOB job_name;
```

Job runs (latest first):
```
SHOW JOB RUNS FOR job_name;
```

Dependencies (optional):
```
SELECT *
FROM sys.job_dependencies
WHERE job_id = ?;
```

Privileges (optional):
```
SHOW GRANTS FOR job_name;
```

Scheduler config (optional):
```
SELECT 'scheduler.enabled' AS key, value FROM sys.settings WHERE key LIKE 'scheduler.%';
```

Config update (optional):
```
ALTER SYSTEM SET scheduler.polling_interval_seconds = 5;
```

DDL actions:
```
CREATE JOB ...
ALTER JOB ...
DROP JOB ...
EXECUTE JOB job_name;
CANCEL JOB RUN 'uuid';
GRANT EXECUTE ON JOB job_name TO user_or_role;
REVOKE EXECUTE ON JOB job_name FROM user_or_role;
```

## ScratchRobin UI Requirements

### Menu Placement

- Main menu: `Window -> Job Scheduler`
- Optional toolbar entry (main window) once icon actions are wired

### Job Scheduler Window

Layout:
- Job list grid (left or top)
- Job detail panel (right or bottom)
- Run history grid (tab)
- Dependency list (tab)
- Scheduler config panel (tab, read-only by default)
- Privileges panel (tab)

Job list columns:
- Name, State, Type, Schedule, Next Run, Last Run, Owner

Actions:
- Create / Edit / Drop (Create dialog supports CREATE / CREATE OR ALTER / RECREATE)
- Enable / Disable / Pause
- Run Now (EXECUTE JOB)
- Cancel Run (CANCEL JOB RUN)
- Refresh
- Grant / Revoke (EXECUTE privilege)

Details panel:
- Schedule builder (CRON/AT/EVERY)
- Start/End timestamps for EVERY
- Retry/backoff/timeout
- On completion (Preserve/Drop)
- Run as role
- Description
- Job body (SQL/CALL/EXEC)
- Dependencies / Class / Partition

Run history:
- State, Started, Completed, Duration, Rows, Error, Message

Dependencies:
- List of required jobs (editable via ALTER JOB ... DEPENDS ON / DEPENDS ON NONE)

Config panel:
- `scheduler.*` settings with Apply action (ALTER SYSTEM)
- Status indicator (enabled/disabled)

Privileges panel:
- Grant EXECUTE on job to user/role/public
- Revoke EXECUTE on job
- View current grants (SHOW GRANTS FOR job_name)
 - Surface CREATE JOB / VIEW JOB HISTORY / EXECUTE EXTERNAL JOB as server-wide privileges

### UI Mock (ASCII)

```
+--------------------------------------------------------------+
| Job Scheduler  [Refresh] [Create] [Edit] [Drop] [Run] [Cancel]|
+-------------------------------+------------------------------+
| Jobs                          | Details                      |
|-------------------------------+------------------------------|
| Name        State  Next Run   | Name:        daily_sweep     |
| daily_sweep Enabled 02:00     | Type:        SQL             |
| update_stats Enabled 13:00    | Schedule:    CRON 0 2 * * *  |
| rebuild_idx Disabled 03:00    | Run As:      DB_OWNER        |
|                               | Retries:     3               |
|                               | Timeout:     3600s           |
|                               | Description: Daily sweep     |
|-------------------------------+------------------------------|
| Runs (tab)   Deps (tab)   Config (tab)   Privs (tab)          |
|--------------------------------------------------------------|
| Run ID     State   Started   Completed   Rows   Message      |
| ...                                                      ... |
+--------------------------------------------------------------+
```

### Capability Gating

- Window only shown when backend == ScratchBird.
- External jobs are enabled when the server policy allows them.

### Error Handling

Surface server error messages verbatim, including permission errors:
- CREATE JOB requires CREATE JOB privilege (DB_OWNER or superuser may imply it).
- SHOW JOB / SHOW JOB RUNS require job owner or VIEW JOB HISTORY privilege.
- ALTER/DROP/EXECUTE/CANCEL require job owner or DB_OWNER/superuser.
- GRANT/REVOKE on JOB requires job owner or admin privileges.
- EXECUTE EXTERNAL JOB requires EXECUTE EXTERNAL JOB privilege.

## Examples

Create a daily sweep job:
```
CREATE JOB daily_sweep
  SCHEDULE = CRON '0 2 * * *'
  DESCRIPTION = 'Daily sweep/GC maintenance'
  ENABLED
  AS 'SWEEP DATABASE';
```

Create a recurring stats job:
```
CREATE JOB update_stats
  SCHEDULE = EVERY 1 HOUR
  DESCRIPTION = 'Hourly stats refresh'
  ENABLED
  AS 'ANALYZE';
```

Cancel a run:
```
CANCEL JOB RUN 'd15f9cf4-2ab1-4e2c-9f33-0b0a6f4a0e0d';
```
