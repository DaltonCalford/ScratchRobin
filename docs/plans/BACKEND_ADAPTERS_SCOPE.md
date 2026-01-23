# External Backend Adapters Scope

This document scopes direct connections to real PostgreSQL, MySQL, and Firebird
servers for full query execution and catalog browsing. ScratchRobin will use the
native client libraries and map results into the existing `ConnectionBackend`
interface.

## Goals
- Connect to PostgreSQL/MySQL/Firebird and execute SQL from day one.
- Populate the metadata tree from real catalogs (schemas, tables, columns).
- Reuse existing job queue, result grid, and credential store paths.
- Keep config-driven backend selection (`backend` field in connections).

## Non-goals (Alpha)
- No cross-dialect translation or SQL rewriting.
- No server-side replication tooling or cluster features.
- No unified query planner/explain normalization across engines.
- No write-back of metadata edits (read-only catalog browsing).

## Backend Targets
- `backend = postgresql` -> libpq (PostgreSQL client).
- `backend = mysql` -> libmysqlclient or MariaDB Connector/C (preferred if distro).
- `backend = firebird` -> fbclient (Firebird client).

## Build/Link Strategy
Add optional CMake toggles and link when available:
- `SCRATCHROBIN_USE_LIBPQ` (find_package(PostgreSQL) or PkgConfig libpq).
- `SCRATCHROBIN_USE_MYSQL` (find_package(MySQL) or PkgConfig mysqlclient/mariadb).
- `SCRATCHROBIN_USE_FIREBIRD` (find_package(Firebird) or PkgConfig fbclient).

Each backend is compiled only when its client library is detected. If not
available, `ConnectionManager` returns a clear error.

## Connection Profile Fields
Existing fields already cover most needs:
- `host`, `port`, `database`, `username`, `credentialId`, `sslMode`.
- `backend` selects the adapter.

Planned additions (if needed):
- `options` (string) for extra DSN flags (e.g., `application_name` for libpq).
- `service` (string) for Firebird service manager operations later.

## Capability Flags
Initial defaults per backend:
- PostgreSQL: `supportsTransactions=true`, `supportsCancel=true` (PQcancel),
  `supportsPaging=true`.
- MySQL: `supportsTransactions=true`, `supportsCancel=false` (until KILL support),
  `supportsPaging=true`.
- Firebird: `supportsTransactions=true`, `supportsCancel=false` (future),
  `supportsPaging=true`.

Explain/SBLR/DDL extract are backend-specific and will remain disabled until
explicitly implemented.

## Metadata Queries (Phase 3)
Use lightweight catalog queries to build:
- Catalog -> schema -> table/view -> column nodes.

PostgreSQL (examples):
```
SELECT nspname FROM pg_namespace WHERE nspname NOT LIKE 'pg_%';
SELECT schemaname, tablename FROM pg_tables;
SELECT table_schema, table_name, column_name, data_type
  FROM information_schema.columns;
```

MySQL (examples):
```
SELECT schema_name FROM information_schema.schemata;
SELECT table_schema, table_name, table_type FROM information_schema.tables;
SELECT table_schema, table_name, column_name, column_type
  FROM information_schema.columns;
```

Firebird (examples):
```
SELECT rdb$owner_name FROM rdb$database;
SELECT rdb$relation_name FROM rdb$relations WHERE rdb$system_flag = 0;
SELECT rdb$relation_name, rdb$field_name FROM rdb$relation_fields
  WHERE rdb$system_flag = 0;
```

Results are normalized into `MetadataNode` with:
- `catalog` = backend name,
- `path` = `<catalog>.<schema>.<object>`,
- `kind` = schema/table/view/column.

## Result Mapping
Each adapter translates native result metadata into:
- `QueryColumn.name`
- `QueryColumn.type` (string from native type OID/enum)
- `QueryValue.raw` (binary where available)
- `QueryValue.text` (fallback string)

Binary handling should be consistent with existing formatter paths.

## Error Handling
Surface backend-native errors into `LastError` and `QueryResult.errorStack` when
available. Keep messages user-readable for the SQL editor.

## Test Strategy
- Unit tests: mocked fixtures remain the default.
- Optional integration tests gated by env vars:
  - `SCRATCHROBIN_TEST_PG_DSN`
  - `SCRATCHROBIN_TEST_MYSQL_DSN`
  - `SCRATCHROBIN_TEST_FB_DSN`

## Delivery Steps (High Level)
1. Add adapter classes and link flags.
2. Extend `ConnectionManager::CreateBackendForProfile`.
3. Implement metadata fetch per backend.
4. Add integration test harness (env-gated).
5. Wire capabilities and UI feature gating.
