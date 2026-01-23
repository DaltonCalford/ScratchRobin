# ScratchRobin

ScratchRobin is a lightweight, ScratchBird-native database administration tool inspired by FlameRobin.
This repository has been reset to a fresh rewrite baseline; the previous implementation remains in Git history only.

## Scope (Initial)
- ScratchBird-first, with optional direct connections to PostgreSQL/MySQL/Firebird when client libs are available
- SDI windowing model (each editor/inspector is its own top-level window)
- Native widget stack via wxWidgets for portability and low overhead

## Status
Active rewrite. The app boots, opens a main window, and supports:
- Async connect/query jobs with cancel status
- Fixture-backed metadata tree + inspector panels
- Streaming result grid with export support
- Optional external backends (libpq/mysqlclient/fbclient) when enabled

## Build
Prerequisites:
- CMake 3.20+
- C++17 compiler
- wxWidgets 3.2+ development packages
Optional (external backends):
- libpq (PostgreSQL)
- mysqlclient or MariaDB Connector/C
- fbclient (Firebird)

Build:
```bash
mkdir build
cmake -S . -B build
cmake --build build
```

Firebird detection note: if the client libraries live outside system paths,
set `FIREBIRD_ROOT=/opt/firebird` (default) or add the prefix to `CMAKE_PREFIX_PATH`.

Run:
```bash
./build/scratchrobin
```

## Docs
- `docs/ARCHITECTURE.md`
- `docs/CONFIGURATION.md`
- `docs/ROADMAP.md`
- `docs/UI_INVENTORY.md`
- `docs/specifications/README.md`
- `docs/plans/ROADMAP.md`
- `docs/plans/BACKEND_ADAPTERS_SCOPE.md`

## Configuration
Modernized config lives in TOML. Example files:
- `config/scratchrobin.toml.example`
- `config/connections.toml.example`

## Tests
Optional integration tests (env-gated):
Set `SCRATCHROBIN_TEST_PG_DSN` to a key=value DSN (libpq style). Example:
```bash
export SCRATCHROBIN_TEST_PG_DSN="host=127.0.0.1 port=5432 dbname=postgres user=postgres password=secret sslmode=disable"
```
You may also use `password_env=ENV_VAR_NAME` instead of `password=` to read from the environment.
Additional tests are gated by:
- `SCRATCHROBIN_TEST_MYSQL_DSN`
- `SCRATCHROBIN_TEST_FB_DSN`
