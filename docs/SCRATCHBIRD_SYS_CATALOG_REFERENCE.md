# ScratchBird Sys Catalog Reference

> **Local Reference Document** - Generated from ScratchBird source analysis
> This document provides a comprehensive reference for the ScratchBird `sys.*` catalog schema.

## Overview

The ScratchBird engine exposes system metadata, metrics, and status information through a virtual catalog accessible via the `sys` schema. These tables can be queried using standard SQL SELECT statements.

## Catalog Architecture

```
sys (schema)
├── schemas                    - Database schemas
├── tables                     - All tables in the database
├── columns                    - Column definitions
├── indexes                    - Index definitions
├── index_columns              - Index column mappings
├── constraints                - Table constraints
├── foreign_keys               - Foreign key relationships
├── primary_keys               - Primary key constraints
├── types                      - System data types
├── domains                    - Domain definitions
├── sessions                   - Active sessions
├── transactions               - Active transactions
├── locks                      - Lock manager state
├── statements                 - Active statements
├── io_stats                   - I/O statistics
├── cache_stats                - Cache performance metrics
├── buffer_pool_stats          - Buffer pool statistics
├── statement_cache            - Prepared statement cache
├── context_variables          - Session context variables
├── server_capabilities        - Server feature flags
├── jobs                       - Job scheduler definitions
├── job_runs                   - Job execution history
├── job_dependencies           - Job dependency graph
├── performance                - Performance metrics
├── migration_status           - Migration connector status
├── migration_audit_summary    - Migration audit trail
├── replication_channel_status - Replication channels
├── replication_conflict_queue - Replication conflicts
├── replication_cursor_status  - Replication cursors
├── shard_status               - Shard/distribution status
├── shard_migrations           - Shard migration progress
├── plugin                     - Loaded plugins/modules
└── prepared_statements        - Prepared statement handles
```

---

## Schema & Metadata Tables

### sys.schemas
Database schema definitions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| schema_id | UUID | No | Unique schema identifier |
| schema_name | TEXT | No | Schema name |
| owner_id | UUID | Yes | Owner user UUID |
| default_tablespace_id | UUID | Yes | Default tablespace |
| default_charset | UUID | Yes | Default character set |
| default_collation_id | UINT32 | Yes | Default collation |
| is_valid | BOOLEAN | Yes | Schema validity flag |

**Example Queries:**
```sql
-- List all schemas
SELECT schema_name, owner_id, is_valid FROM sys.schemas;

-- Find schemas by owner
SELECT schema_name FROM sys.schemas WHERE owner_id = '...';
```

---

### sys.tables
All tables in the database.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| table_id | UUID | No | Unique table identifier |
| schema_id | UUID | No | Parent schema UUID |
| table_name | TEXT | No | Table name |
| table_type | TEXT | Yes | HEAP, INDEX, TEMPORARY, EXTERNAL, MATERIALIZED_VIEW, TOAST |
| owner_id | UUID | Yes | Owner user UUID |
| tablespace_id | UUID | Yes | Tablespace UUID |
| row_count | INT64 | Yes | Approximate row count |
| has_toast | BOOLEAN | Yes | Has TOAST storage |
| toast_table_id | UUID | Yes | TOAST table UUID |
| is_valid | BOOLEAN | Yes | Table validity flag |
| partition_strategy | TEXT | Yes | RANGE, LIST, HASH, or NULL |
| partition_columns | TEXT | Yes | Partition column list |
| partition_parent_name | TEXT | Yes | Parent table if partitioned |
| is_partition_child | BOOLEAN | Yes | Is a partition child |

**Example Queries:**
```sql
-- List all user tables with row counts
SELECT t.table_name, t.row_count, s.schema_name
FROM sys.tables t
JOIN sys.schemas s ON t.schema_id = s.schema_id
WHERE t.table_type = 'HEAP';

-- Find partitioned tables
SELECT table_name, partition_strategy, partition_columns
FROM sys.tables WHERE partition_strategy IS NOT NULL;

-- Find large tables (> 1M rows)
SELECT table_name, row_count FROM sys.tables WHERE row_count > 1000000;
```

---

### sys.columns
Column definitions for all tables.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| column_id | UUID | No | Unique column identifier |
| table_id | UUID | No | Parent table UUID |
| column_name | TEXT | No | Column name |
| data_type_id | UINT16 | Yes | Type identifier |
| data_type_name | TEXT | Yes | Type name (VARCHAR, INTEGER, etc.) |
| ordinal_position | INT32 | Yes | Column position in table |
| is_nullable | BOOLEAN | Yes | NULL allowed |
| default_value | TEXT | Yes | Default expression |
| domain_id | UUID | Yes | Domain UUID if applicable |
| collation_id | UINT32 | Yes | Collation identifier |
| charset_id | UUID | Yes | Character set UUID |
| is_identity | BOOLEAN | Yes | Is identity column |
| is_generated | BOOLEAN | Yes | Is generated column |
| generation_expression | TEXT | Yes | Generation expression |
| is_valid | BOOLEAN | Yes | Column validity |

**Example Queries:**
```sql
-- Get columns for a specific table
SELECT column_name, data_type_name, is_nullable, ordinal_position
FROM sys.columns c
JOIN sys.tables t ON c.table_id = t.table_id
WHERE t.table_name = 'my_table'
ORDER BY ordinal_position;

-- Find identity columns
SELECT t.table_name, c.column_name
FROM sys.columns c
JOIN sys.tables t ON c.table_id = t.table_id
WHERE c.is_identity = true;

-- Find generated columns
SELECT table_id, column_name, generation_expression
FROM sys.columns WHERE is_generated = true;
```

---

### sys.indexes
Index definitions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| index_id | UUID | No | Unique index identifier |
| table_id | UUID | No | Parent table UUID |
| index_name | TEXT | No | Index name |
| index_type | TEXT | Yes | BTREE, HASH, HNSW, IVF_FLAT, etc. |
| is_unique | BOOLEAN | Yes | Unique constraint |
| is_expression | BOOLEAN | Yes | Expression index |
| is_partial | BOOLEAN | Yes | Partial index (with WHERE) |
| expression_sql | TEXT | Yes | Index expression |
| predicate_sql | TEXT | Yes | Partial index predicate |
| state | TEXT | Yes | BUILDING, ACTIVE, RETIRED, FAILED, INACTIVE |
| tablespace_id | UUID | Yes | Tablespace UUID |
| is_valid | BOOLEAN | Yes | Index validity |

**Example Queries:**
```sql
-- List all indexes on a table
SELECT index_name, index_type, is_unique, state
FROM sys.indexes i
JOIN sys.tables t ON i.table_id = t.table_id
WHERE t.table_name = 'my_table';

-- Find vector indexes
SELECT index_name, index_type, table_id
FROM sys.indexes WHERE index_type IN ('HNSW', 'IVF_FLAT', 'IVF_PQ');

-- Find invalid/failed indexes
SELECT index_name, table_id, state FROM sys.indexes WHERE state != 'ACTIVE';
```

---

### sys.index_columns
Index-to-column mapping.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| index_id | UUID | No | Index UUID |
| column_id | UUID | Yes | Column UUID |
| column_name | TEXT | Yes | Column name (for expression indexes) |
| ordinal_position | INT32 | Yes | Position in index |
| is_included | BOOLEAN | Yes | INCLUDE column (not key) |

---

### sys.constraints
Table constraints.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| constraint_id | UUID | No | Unique constraint ID |
| table_id | UUID | No | Parent table |
| constraint_name | TEXT | No | Constraint name |
| constraint_type | TEXT | Yes | PRIMARY_KEY, UNIQUE, CHECK, FOREIGN_KEY, NOT_NULL, EXCLUSION |
| is_deferrable | BOOLEAN | Yes | Can be deferred |
| initially_deferred | BOOLEAN | Yes | Initially deferred |
| is_enabled | BOOLEAN | Yes | Constraint enabled |
| is_validated | BOOLEAN | Yes | Constraint validated |
| check_expression | TEXT | Yes | CHECK expression |

---

### sys.foreign_keys
Foreign key relationships.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| fk_id | UUID | No | Foreign key ID |
| fk_name | TEXT | No | Constraint name |
| child_table_id | UUID | No | Referencing table |
| parent_table_id | UUID | No | Referenced table |
| child_columns | TEXT | Yes | Referencing columns (CSV) |
| parent_columns | TEXT | Yes | Referenced columns (CSV) |
| on_delete | TEXT | Yes | NO_ACTION, RESTRICT, CASCADE, SET_NULL, SET_DEFAULT |
| on_update | TEXT | Yes | NO_ACTION, RESTRICT, CASCADE, SET_NULL, SET_DEFAULT |
| match_type | TEXT | Yes | SIMPLE, FULL, PARTIAL |
| is_enabled | BOOLEAN | Yes | FK enabled |
| is_deferrable | BOOLEAN | Yes | Deferrable |
| initially_deferred | BOOLEAN | Yes | Initially deferred |

---

### sys.domains
Domain definitions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| domain_id | UUID | No | Domain UUID |
| schema_id | UUID | No | Parent schema |
| domain_name | TEXT | No | Domain name |
| domain_type | TEXT | Yes | BASIC, RECORD, ENUM, SET, VARIANT, RANGE |
| base_type_id | UINT16 | Yes | Base type ID |
| base_type_name | TEXT | Yes | Base type name |
| precision | UINT32 | Yes | Precision for numeric |
| scale | UINT32 | Yes | Scale for numeric |
| is_nullable | BOOLEAN | Yes | NULL allowed |
| default_value | TEXT | Yes | Default value |
| parent_domain_id | UUID | Yes | Parent domain (inheritance) |
| is_enum | BOOLEAN | Yes | Is ENUM type |
| enum_labels | TEXT | Yes | ENUM labels (CSV) |
| collation_name | TEXT | Yes | Collation |

---

## Runtime & Session Tables

### sys.sessions
Active database sessions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| session_id | UUID | No | Session UUID |
| connection_id | INT64 | No | Connection ID |
| user_name | TEXT | Yes | Connected user |
| role_name | TEXT | Yes | Current role |
| database_name | TEXT | Yes | Connected database |
| protocol | TEXT | Yes | Connection protocol |
| client_addr | TEXT | Yes | Client IP address |
| client_port | INT32 | Yes | Client port |
| state | TEXT | Yes | Session state |
| connected_at | TIMESTAMP | Yes | Connection time |
| last_activity_at | TIMESTAMP | Yes | Last activity |
| transaction_id | INT64 | Yes | Current transaction |
| statement_id | INT64 | Yes | Current statement |
| current_query | TEXT | Yes | Executing query |
| wait_event | TEXT | Yes | Wait event type |
| wait_resource | TEXT | Yes | Resource being waited on |

**Example Queries:**
```sql
-- List active sessions
SELECT session_id, user_name, client_addr, current_query, wait_event
FROM sys.sessions WHERE state = 'ACTIVE';

-- Find long-running queries (> 5 minutes)
SELECT session_id, user_name, current_query,
       EXTRACT(EPOCH FROM (NOW() - last_activity_at))/60 AS minutes
FROM sys.sessions
WHERE last_activity_at < NOW() - INTERVAL '5 minutes';

-- Session count by user
SELECT user_name, COUNT(*) as session_count
FROM sys.sessions GROUP BY user_name;
```

---

### sys.transactions
Active transactions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| transaction_id | INT64 | No | Transaction ID |
| transaction_uuid | UUID | Yes | Transaction UUID |
| session_id | UUID | Yes | Owning session |
| state | TEXT | Yes | Transaction state |
| isolation_level | TEXT | Yes | read_committed, repeatable_read, serializable |
| read_only | BOOLEAN | Yes | Read-only transaction |
| start_time | TIMESTAMP | Yes | Transaction start |
| duration_ms | INT64 | Yes | Duration in milliseconds |
| current_query | TEXT | Yes | Current query |
| wait_event | TEXT | Yes | Wait event |
| locks_held | INT32 | Yes | Number of locks held |
| pages_modified | INT32 | Yes | Pages modified |
| distributed | BOOLEAN | Yes | Distributed transaction |
| coordinator_uuid | UUID | Yes | Distributed coordinator |

**Example Queries:**
```sql
-- Long-running transactions
SELECT transaction_id, session_id, start_time, duration_ms, locks_held
FROM sys.transactions
WHERE duration_ms > 60000;

-- Distributed transactions
SELECT * FROM sys.transactions WHERE distributed = true;
```

---

### sys.locks
Lock manager state.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| lock_id | INT64 | No | Lock ID |
| lock_type | TEXT | Yes | Lock target type |
| lock_mode | TEXT | Yes | access_share, row_share, row_exclusive, etc. |
| granted | BOOLEAN | Yes | Lock granted |
| lock_state | TEXT | Yes | Lock state |
| database_uuid | UUID | Yes | Database |
| relation_uuid | UUID | Yes | Relation (table) |
| relation_name | TEXT | Yes | Relation name |
| page | INT64 | Yes | Page number |
| tuple | INT64 | Yes | Tuple number |
| transaction_id | INT64 | Yes | Holder transaction |
| session_id | UUID | Yes | Holder session |
| virtual_xid | TEXT | Yes | Virtual transaction ID |
| grant_time | TIMESTAMP | Yes | When granted |
| wait_start | TIMESTAMP | Yes | When started waiting |

**Example Queries:**
```sql
-- Current locks with waiters
SELECT lock_mode, relation_name, transaction_id, granted, wait_start
FROM sys.locks WHERE granted = false;

-- Lock counts by table
SELECT relation_name, COUNT(*) as lock_count
FROM sys.locks GROUP BY relation_name;
```

---

### sys.statements
Active statement execution.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| statement_id | INT64 | No | Statement ID |
| session_id | UUID | Yes | Owning session |
| transaction_id | INT64 | Yes | Transaction ID |
| state | TEXT | Yes | Statement state |
| sql_text | TEXT | Yes | SQL being executed |
| start_time | TIMESTAMP | Yes | Start time |
| elapsed_ms | INT64 | Yes | Elapsed milliseconds |
| rows_processed | INT64 | Yes | Rows processed |
| wait_event | TEXT | Yes | Wait event |
| wait_resource | TEXT | Yes | Wait resource |

---

## Performance Tables

### sys.performance
Key performance metrics.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| metric | TEXT | No | Metric name |
| value | FLOAT64 | No | Metric value |
| unit | TEXT | Yes | Unit (ms, ops/sec, etc.) |
| scope | TEXT | Yes | Global, Database, Table, etc. |
| database_name | TEXT | Yes | Database context |
| updated_at | TIMESTAMP | Yes | Last update time |

**Example Queries:**
```sql
-- All performance metrics
SELECT metric, value, unit FROM sys.performance ORDER BY metric;

-- Database-specific metrics
SELECT metric, value FROM sys.performance
WHERE scope = 'Database' AND database_name = 'mydb';
```

---

### sys.cache_stats
Cache performance statistics.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| cache_type | TEXT | No | metadata, data, index, etc. |
| database_name | TEXT | Yes | Database context |
| hits_total | INT64 | Yes | Cache hits |
| misses_total | INT64 | Yes | Cache misses |
| evictions_total | INT64 | Yes | Evictions |
| entries | INT64 | Yes | Current entries |
| memory_bytes | INT64 | Yes | Memory used |
| hit_ratio | FLOAT64 | Yes | Hit ratio (0-1) |
| updated_at | TIMESTAMP | Yes | Last update |

**Example Queries:**
```sql
-- Cache hit ratios
SELECT cache_type, hit_ratio, hits_total, misses_total
FROM sys.cache_stats ORDER BY hit_ratio ASC;

-- Low cache hit ratios (< 90%)
SELECT * FROM sys.cache_stats WHERE hit_ratio < 0.9;
```

---

### sys.buffer_pool_stats
Buffer pool statistics.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| database_name | TEXT | Yes | Database |
| pool_size_bytes | INT64 | Yes | Pool size |
| pages_total | INT64 | Yes | Total pages |
| pages_dirty | INT64 | Yes | Dirty pages |
| hits_total | INT64 | Yes | Buffer hits |
| misses_total | INT64 | Yes | Buffer misses |
| reads_total | INT64 | Yes | Physical reads |
| writes_total | INT64 | Yes | Physical writes |
| hit_ratio | FLOAT64 | Yes | Hit ratio |
| updated_at | TIMESTAMP | Yes | Last update |

---

### sys.statement_cache
Prepared statement cache.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| database_name | TEXT | Yes | Database |
| sql_text | TEXT | Yes | SQL text |
| fingerprint | TEXT | Yes | Query fingerprint |
| statement_type | TEXT | Yes | SELECT, INSERT, etc. |
| hit_count | INT64 | Yes | Cache hits |
| miss_count | INT64 | Yes | Cache misses |
| execution_count | INT64 | Yes | Total executions |
| error_count | INT64 | Yes | Error count |
| created_at | TIMESTAMP | Yes | Created time |
| last_accessed | TIMESTAMP | Yes | Last access |
| last_executed | TIMESTAMP | Yes | Last execution |
| avg_execution_time_ms | INT64 | Yes | Average time |
| memory_bytes | INT64 | Yes | Memory used |
| plan_memory_bytes | INT64 | Yes | Plan memory |

**Example Queries:**
```sql
-- Most executed statements
SELECT sql_text, execution_count, avg_execution_time_ms
FROM sys.statement_cache
ORDER BY execution_count DESC LIMIT 10;

-- Slowest statements
SELECT sql_text, avg_execution_time_ms, execution_count
FROM sys.statement_cache
WHERE execution_count > 100
ORDER BY avg_execution_time_ms DESC LIMIT 10;
```

---

### sys.io_stats
I/O statistics.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| stat_id | INT64 | No | Stat ID |
| stat_group | INT16 | No | Stat group |
| session_id | UUID | Yes | Session |
| transaction_id | INT64 | Yes | Transaction |
| statement_id | INT64 | Yes | Statement |
| page_reads | INT64 | Yes | Pages read |
| page_writes | INT64 | Yes | Pages written |
| page_fetches | INT64 | Yes | Pages fetched |
| page_marks | INT64 | Yes | Pages marked dirty |

---

## Job Scheduler Tables

### sys.jobs
Job definitions.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| job_uuid | UUID | No | Job ID |
| job_name | VARCHAR | No | Job name |
| description | TEXT | Yes | Description |
| job_class | VARCHAR | Yes | LOCAL_SAFE, LEADER_ONLY, QUORUM_REQUIRED |
| job_type | VARCHAR | No | SQL, PROCEDURE, EXTERNAL |
| job_sql | TEXT | Yes | SQL to execute |
| procedure_uuid | UUID | Yes | Procedure reference |
| external_command | TEXT | Yes | External command |
| schedule_kind | VARCHAR | No | CRON, AT, EVERY |
| cron_expression | TEXT | Yes | CRON expression |
| interval_seconds | INT64 | Yes | Interval for EVERY |
| starts_at | INT64 | Yes | Start timestamp |
| ends_at | INT64 | Yes | End timestamp |
| schedule_tz | VARCHAR | Yes | Timezone |
| next_run_time | INT64 | Yes | Next scheduled run |
| on_completion | VARCHAR | Yes | PRESERVE, DROP |
| partition_strategy | VARCHAR | Yes | Partition strategy |
| partition_shard_uuid | UUID | Yes | Shard UUID |
| partition_expression | TEXT | Yes | Partition expression |
| max_retries | INT32 | Yes | Max retry attempts |
| retry_backoff_seconds | INT32 | Yes | Backoff between retries |
| timeout_seconds | INT32 | Yes | Timeout |
| created_by_user_uuid | UUID | Yes | Creator |
| run_as_role_uuid | UUID | Yes | Execution role |
| created_at | INT64 | Yes | Creation time |
| state | VARCHAR | Yes | ENABLED, DISABLED, PAUSED |

**Example Queries:**
```sql
-- List all enabled jobs
SELECT job_name, job_type, schedule_kind, next_run_time, state
FROM sys.jobs WHERE state = 'ENABLED';

-- Jobs scheduled to run soon
SELECT job_name, next_run_time
FROM sys.jobs
WHERE next_run_time < EXTRACT(EPOCH FROM NOW()) + 3600;
```

---

### sys.job_runs
Job execution history.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| job_run_uuid | UUID | No | Run ID |
| job_uuid | UUID | No | Job reference |
| assigned_node_uuid | UUID | Yes | Executing node |
| shard_uuid | UUID | Yes | Shard context |
| scheduled_time | INT64 | Yes | Scheduled time |
| started_at | INT64 | Yes | Start time |
| completed_at | INT64 | Yes | Completion time |
| state | VARCHAR | Yes | PENDING, RUNNING, COMPLETED, FAILED, CANCELLED |
| retry_count | INT32 | Yes | Retry count |
| result_message | TEXT | Yes | Result message |
| rows_affected | INT64 | Yes | Rows affected |
| result_data | BYTEA | Yes | Binary result |
| error_code | INT32 | Yes | Error code |

**Example Queries:**
```sql
-- Recent job failures
SELECT jr.job_run_uuid, j.job_name, jr.state, jr.result_message
FROM sys.job_runs jr
JOIN sys.jobs j ON jr.job_uuid = j.job_uuid
WHERE jr.state = 'FAILED'
ORDER BY jr.completed_at DESC LIMIT 10;

-- Job success rate
SELECT j.job_name,
       COUNT(*) as total,
       SUM(CASE WHEN jr.state = 'COMPLETED' THEN 1 ELSE 0 END) as success,
       SUM(CASE WHEN jr.state = 'FAILED' THEN 1 ELSE 0 END) as failed
FROM sys.jobs j
LEFT JOIN sys.job_runs jr ON j.job_uuid = jr.job_uuid
GROUP BY j.job_name;
```

---

### sys.job_dependencies
Job dependency graph.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| job_uuid | UUID | No | Job ID |
| depends_on_job_uuid | UUID | No | Dependency job ID |

---

## Migration Tables

### sys.migration_status
Remote connector status.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| connector_id | UUID | No | Connector ID |
| connector_name | TEXT | No | Connector name |
| engine_name | TEXT | No | Source engine |
| state | TEXT | No | DISABLED, PROBING, READY, DEGRADED, FAILED |
| failure_count | INT32 | No | Failure count |
| last_probe_time | INT64 | Yes | Last probe |
| last_ready_time | INT64 | Yes | Last ready |
| open_error_count | INT64 | No | Error count |
| snapshot_count | INT64 | No | Snapshots |
| metadata_object_count | INT64 | No | Objects migrated |
| metadata_column_count | INT64 | No | Columns migrated |

---

### sys.migration_audit_summary
Migration audit trail.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| connector_id | UUID | No | Connector ID |
| connector_name | TEXT | No | Connector name |
| request_count | INT64 | No | Total requests |
| success_count | INT64 | No | Successful |
| failed_count | INT64 | No | Failed |
| avg_latency_ms | INT64 | No | Average latency |
| bytes_in_total | INT64 | No | Bytes received |
| bytes_out_total | INT64 | No | Bytes sent |
| last_activity_time | INT64 | Yes | Last activity |
| open_error_count | INT64 | No | Open errors |

---

## Replication Tables

### sys.replication_channel_status
Replication channel status.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| channel_id | UUID | No | Channel ID |
| channel_name | TEXT | No | Channel name |
| direction | TEXT | No | ONE_WAY, BIDIRECTIONAL |
| state | TEXT | No | INIT, SNAPSHOT, CATCHUP, STREAMING, PAUSED, DEGRADED, FENCED, STOPPED, FAILED |
| mode_version | INT64 | No | Protocol version |
| lag_ms | INT64 | No | Replication lag |
| open_conflict_count | INT64 | No | Open conflicts |
| open_error_count | INT64 | No | Open errors |
| active_cursor_count | INT64 | No | Active cursors |
| last_applied_commit_seq | INT64 | No | Last applied LSN |
| last_applied_time | INT64 | Yes | Last apply time |

**Example Queries:**
```sql
-- Replication lag
SELECT channel_name, lag_ms, state FROM sys.replication_channel_status;

-- Stalled replication
SELECT * FROM sys.replication_channel_status WHERE state != 'STREAMING';
```

---

### sys.replication_conflict_queue
Replication conflict resolution.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| conflict_id | UUID | No | Conflict ID |
| channel_id | UUID | No | Channel |
| channel_name | TEXT | No | Channel name |
| batch_id | UUID | No | Batch ID |
| conflict_kind | TEXT | No | UPDATE_UPDATE, DELETE_UPDATE, UNIQUE_CONSTRAINT, DDL_DML, DDL_DDL, TYPE_MISMATCH |
| source_commit_seq | INT64 | No | Source LSN |
| resolution_state | TEXT | No | OPEN, AUTO_RESOLVED, MANUAL_PENDING, MANUAL_RESOLVED, IGNORED |
| source_payload | TEXT | Yes | Source data |
| target_payload | TEXT | Yes | Target data |
| resolved_time | INT64 | Yes | Resolution time |

---

### sys.replication_cursor_status
Replication cursor status.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| cursor_id | UUID | No | Cursor ID |
| channel_id | UUID | No | Channel |
| channel_name | TEXT | No | Channel name |
| member_id | UUID | No | Member ID |
| cursor_name | TEXT | No | Cursor name |
| cursor_state | TEXT | No | ACTIVE, STALLED, ERROR, CLOSED |
| source_commit_seq | INT64 | No | Source position |
| applied_commit_seq | INT64 | No | Applied position |
| lag_ms | INT64 | No | Lag milliseconds |
| heartbeat_time | INT64 | Yes | Last heartbeat |
| last_error_id | UUID | Yes | Last error |

---

## Sharding Tables

### sys.shard_status
Shard/distribution status.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| shard_id | UUID | No | Shard ID |
| shard_name | TEXT | No | Shard name |
| cluster_id | UUID | No | Cluster ID |
| state | TEXT | No | CREATING, ONLINE, REBALANCING, DRAINING, OFFLINE |
| kind | TEXT | No | ROW, COLUMN, VECTOR, DOCUMENT |
| policy_id | UUID | No | Policy ID |
| replica_count | INT64 | No | Total replicas |
| online_replica_count | INT64 | No | Online replicas |
| migration_in_progress | BOOLEAN | No | Migration active |

---

### sys.shard_migrations
Shard migration progress.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| migration_id | UUID | No | Migration ID |
| shard_id | UUID | No | Shard ID |
| shard_name | TEXT | No | Shard name |
| source_node_id | UUID | No | Source node |
| target_node_id | UUID | No | Target node |
| state | TEXT | No | PLANNED, RUNNING, PAUSED, COMPLETED, FAILED |
| bytes_total | INT64 | No | Total bytes |
| bytes_copied | INT64 | No | Copied bytes |
| rows_total | INT64 | No | Total rows |
| rows_copied | INT64 | No | Copied rows |
| throttle_state | TEXT | No | NONE, LOW, MEDIUM, HIGH |
| progress_pct | FLOAT64 | No | Progress percentage |
| started_time | INT64 | No | Start time |
| updated_time | INT64 | No | Last update |
| completed_time | INT64 | Yes | Completion time |
| error_code | TEXT | Yes | Error code |
| error_message | TEXT | Yes | Error message |

**Example Queries:**
```sql
-- Active shard migrations
SELECT shard_name, progress_pct, state, bytes_copied, bytes_total
FROM sys.shard_migrations WHERE state = 'RUNNING';

-- Migration history
SELECT * FROM sys.shard_migrations
WHERE completed_time > EXTRACT(EPOCH FROM NOW() - INTERVAL '24 hours');
```

---

## System Tables

### sys.server_capabilities
Server feature flags.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| capability | TEXT | No | Feature name |
| enabled | BOOLEAN | No | Enabled flag |

---

### sys.plugin
Loaded plugins/modules.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| module_id | UUID | No | Module ID |
| module_name | TEXT | No | Module name |
| engine_id | UUID | No | Engine ID |
| library_path | TEXT | Yes | Library path |
| checksum | TEXT | Yes | Checksum |
| entry_point | TEXT | Yes | Entry point |
| is_loaded | BOOLEAN | No | Loaded flag |
| is_validated | BOOLEAN | No | Validated flag |
| loaded_count | INT64 | No | Load count |
| last_modified_time | INT64 | No | Last modified |

---

### sys.prepared_statements
Prepared statement handles.

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| remote_prepared_id | UUID | No | Prepared ID |
| remote_connector_id | UUID | No | Connector |
| session_id | UUID | No | Session |
| statement_name | TEXT | No | Statement name |
| statement_fingerprint | INT64 | No | Fingerprint |
| remote_handle | TEXT | Yes | Remote handle |
| created_time | INT64 | No | Creation time |
| last_used_time | INT64 | No | Last use |
| expires_time | INT64 | Yes | Expiration |
| is_valid | BOOLEAN | No | Valid flag |

---

## Useful Query Patterns

### Database Health Check
```sql
-- Active connections
SELECT COUNT(*) as active_connections FROM sys.sessions;

-- Long-running transactions (> 1 min)
SELECT COUNT(*) as long_txns FROM sys.transactions WHERE duration_ms > 60000;

-- Failed indexes
SELECT COUNT(*) as failed_indexes FROM sys.indexes WHERE state != 'ACTIVE';

-- Cache hit ratio below 95%
SELECT COUNT(*) as low_cache FROM sys.cache_stats WHERE hit_ratio < 0.95;
```

### Performance Troubleshooting
```sql
-- Top 10 slowest queries
SELECT sql_text, avg_execution_time_ms, execution_count
FROM sys.statement_cache
ORDER BY avg_execution_time_ms DESC
LIMIT 10;

-- Tables with most indexes
SELECT t.table_name, COUNT(*) as index_count
FROM sys.tables t
JOIN sys.indexes i ON t.table_id = i.table_id
GROUP BY t.table_name
ORDER BY index_count DESC
LIMIT 10;

-- Waiting sessions
SELECT session_id, user_name, wait_event, wait_resource
FROM sys.sessions WHERE wait_event IS NOT NULL;
```

### Schema Analysis
```sql
-- Tables without primary keys
SELECT t.table_name
FROM sys.tables t
LEFT JOIN sys.constraints c ON t.table_id = c.table_id
  AND c.constraint_type = 'PRIMARY_KEY'
WHERE c.constraint_id IS NULL;

-- Foreign keys without indexes
SELECT fk.fk_name, fk.child_table_id, fk.child_columns
FROM sys.foreign_keys fk
LEFT JOIN sys.indexes i ON fk.child_table_id = i.table_id
WHERE i.index_id IS NULL;

-- Unused indexes (if statistics available)
SELECT index_name, table_id, state
FROM sys.indexes WHERE state = 'INACTIVE';
```

---

## Data Type Reference

### Enums and Status Values

**Table Types:**
- `HEAP` - Standard table
- `INDEX` - Index-organized table
- `TEMPORARY` - Temporary table
- `EXTERNAL` - External table
- `MATERIALIZED_VIEW` - Materialized view
- `TOAST` - TOAST storage table

**Index States:**
- `BUILDING` - Under construction
- `ACTIVE` - Ready for use
- `INACTIVE` - Disabled
- `RETIRED` - Marked for removal
- `FAILED` - Build failed

**Job States:**
- `ENABLED` - Scheduled and active
- `DISABLED` - Not scheduled
- `PAUSED` - Temporarily stopped

**Job Run States:**
- `PENDING` - Waiting to execute
- `RUNNING` - Currently executing
- `COMPLETED` - Successfully finished
- `FAILED` - Execution failed
- `CANCELLED` - Cancelled by user

**Replication Channel States:**
- `INIT` - Initializing
- `SNAPSHOT` - Taking initial snapshot
- `CATCHUP` - Catching up
- `STREAMING` - Real-time replication
- `PAUSED` - Paused
- `DEGRADED` - Degraded performance
- `FENCED` - Split-brain protection
- `STOPPED` - Stopped
- `FAILED` - Failed

**Shard States:**
- `CREATING` - Being created
- `ONLINE` - Active and serving
- `REBALANCING` - Data being rebalanced
- `DRAINING` - Being decommissioned
- `OFFLINE` - Not available

---

*Last Updated: 2026-03-04*
*Source: ScratchBird Sys Catalog Implementation*
