# ScratchRobin

**Status**: 🟡 **97% Complete** - Final QA Phase  
**Last Updated**: 2026-02-03  

ScratchRobin is a lightweight, ScratchBird-native database administration tool inspired by FlameRobin. It provides a comprehensive GUI for managing ScratchBird databases with support for PostgreSQL, MySQL, and Firebird through optional backend adapters.

> **Project Progress**: 253 of 259 tasks complete (97%). All major development phases finished, currently in final testing and documentation phase. See [PROJECT_STATUS.md](PROJECT_STATUS.md) for detailed status.

## Features

### Core Infrastructure (Phase 1 - Complete)
- **Connection Profile Editor**: Full-featured connection management with support for all backends
- **Credential Store**: Secure password storage via OS keychain (libsecret integration)
- **Transaction Management**: Visual transaction state tracking, Begin/Commit/Rollback controls
- **Savepoint Support**: Create, rollback to, and release savepoints with backend-specific SQL templates
- **Error Handling Framework**: Comprehensive error classification, user-friendly error dialogs
- **Server Notices**: Display of database server notices and messages in SQL Editor

### Object Managers (Phase 2 - Complete)
- **Table Designer**: Create, alter, drop tables with column/constraints UI, GRANT/REVOKE privileges
- **Index Designer**: Create, alter, drop indexes with usage statistics panel
- **Schema Manager**: Schema creation, modification, object counting
- **Domain Manager**: Domain creation and management for all domain types
- **Job Scheduler**: Job creation with CRON/AT/EVERY scheduling, dependencies visualization, configuration panel
- **Users & Roles**: User/role creation dialogs, privilege management, membership management

### ERD/Diagramming System (Phase 3 - Complete)
- **4 Notation Types**: Crow's Foot, IDEF1X, UML Class, Chen notation
- **Diagram Canvas**: Zoom, pan, grid, snap-to-grid, selection handles
- **Auto-Layout**: Sugiyama (hierarchical), Force-Directed, Orthogonal algorithms with options dialog
- **Reverse Engineering**: Import database schema to diagram with wizard interface
- **Forward Engineering**: Generate DDL for ScratchBird, PostgreSQL, MySQL, Firebird
- **Diagram Export**: PNG, SVG, PDF export with options dialog
- **Minimap/Navigation**: Overview panel for large diagrams
- **Copy/Paste**: Duplicate diagram elements
- **Alignment Tools**: Left, right, top, bottom, center, distribute horizontal/vertical
- **Grouping**: Group/ungroup entities for collective operations
- **Pin/Unpin Nodes**: Exclude nodes from auto-layout
- **Print Support**: Print diagrams with preview and page setup

### Additional Features
- **SQL Editor**: Syntax highlighting, async execution, result grid with export
- **Monitoring**: Sessions, locks, transactions panels
- **DDL Preview/Execution**: Preview and execute generated DDL with progress tracking
- **Incremental Refresh**: Update diagrams from changed database schema

### Beta Placeholders (Phase 7 - UI Complete, Functionality Pending)
- **Cluster Manager**: High-availability cluster management UI (preview available)
- **Replication Manager**: Replication monitoring and management (preview available)
- **ETL Manager**: Extract, Transform, Load workflow design (preview available)
- **Git Integration**: Database schema version control (preview available)

## Build Status

**All Development Phases: COMPLETE**

| Phase | Status | Tasks |
|-------|--------|-------|
| Phase 1: Foundation | ✅ Complete | 24/24 |
| Phase 2: Object Managers | ✅ Complete | 46/46 |
| Phase 3: ERD/Diagramming | ✅ Complete | 52/52 |
| Phase 4: Additional Managers | ✅ Complete | 43/43 |
| Phase 5: Admin Tools | ✅ Complete | 34/34 |
| Phase 6: Infrastructure | ✅ Complete | 31/31 |
| Phase 7: Beta Placeholders | ✅ Complete | 12/12 |
| Phase 8: Testing & QA | 🟡 Active | 20/26 |
| **Total** | **97%** | **253/259** |

### Prerequisites
- CMake 3.20+
- C++17 compiler
- wxWidgets 3.2+ development packages

### Optional (External Backends)
- libpq (PostgreSQL)
- mysqlclient or MariaDB Connector/C
- fbclient (Firebird)
- libsecret (credential store integration)

### Build
```bash
mkdir build
cmake -S . -B build
cmake --build build
```

### Windows (MinGW cross-compile from Linux)

Prereqs:
- MinGW-w64 toolchain (x86_64-w64-mingw32)
- wxWidgets built for MinGW (see `/home/dcalford/CliWork/wxwidgets-3.2.9-mingw64`)

Configure/build:
```bash
cmake -S . -B build-mingw64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake \
  -DwxWidgets_CONFIG_EXECUTABLE=/home/dcalford/CliWork/wxwidgets-3.2.9-mingw64/bin/wx-config \
  -DSCRATCHROBIN_USE_LIBSECRET=OFF \
  -DSCRATCHROBIN_USE_LIBPQ=OFF \
  -DSCRATCHROBIN_USE_MYSQL=OFF \
  -DSCRATCHROBIN_USE_FIREBIRD=OFF \
  -DSCRATCHROBIN_USE_SCRATCHBIRD=OFF
cmake --build build-mingw64
```
Resulting binary: `build-mingw64/scratchrobin.exe`

### Windows (MSVC native)

Prereqs:
- Visual Studio 2022 (C++ workload)
- CMake 3.20+
- wxWidgets built with MSVC (or installed via vcpkg)

Configure/build (Developer Command Prompt):
```cmd
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/msvc.cmake ^
  -DwxWidgets_ROOT_DIR=C:\wxwidgets ^
  -DSCRATCHROBIN_USE_LIBSECRET=OFF ^
  -DSCRATCHROBIN_USE_LIBPQ=OFF ^
  -DSCRATCHROBIN_USE_MYSQL=OFF ^
  -DSCRATCHROBIN_USE_FIREBIRD=OFF ^
  -DSCRATCHROBIN_USE_SCRATCHBIRD=OFF
cmake --build build-msvc --config Release
```

If you use vcpkg for wxWidgets, omit `msvc.cmake` and pass:
`-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake`.

Firebird detection note: if the client libraries live outside system paths,
set `FIREBIRD_ROOT=/opt/firebird` (default) or add the prefix to `CMAKE_PREFIX_PATH`.

Run:
```bash
./build/scratchrobin
```

## Testing

ScratchRobin includes a comprehensive test suite with 200+ test cases.

### Running Tests

```bash
cd build
make scratchrobin_tests
./scratchrobin_tests
```

### Test Coverage

| Test Type | Count | Status |
|-----------|-------|--------|
| Unit Tests | 16 suites | ✅ Complete |
| Integration Tests | 3 backends | ✅ Complete |
| Test Cases | 200+ | ✅ Active |

### Unit Tests
- Metadata Model
- Statement Splitter
- Value Formatter
- Result Exporter
- Configuration
- Credentials
- JSON Parser
- Error Handler
- Capability Detector
- Job Queue
- Session State
- Diagram Model
- Layout Engine
- DDL Generator

### Integration Tests (Env-Gated)
Set `SCRATCHROBIN_TEST_PG_DSN` to a key=value DSN (libpq style). Example:
```bash
export SCRATCHROBIN_TEST_PG_DSN="host=127.0.0.1 port=5432 dbname=postgres user=postgres password=secret sslmode=disable"
```
You may also use `password_env=ENV_VAR_NAME` instead of `password=` to read from the environment.
Additional tests are gated by:
- `SCRATCHROBIN_TEST_MYSQL_DSN`
- `SCRATCHROBIN_TEST_FB_DSN`

### Code Quality Tools
- **clang-format**: Code formatting configured (`.clang-format`)
- **clang-tidy**: Static analysis checks (`.clang-tidy`)
- **Sanitizers**: ASan, UBSan, TSan, MSan support
- **Coverage**: gcov/lcov integration

Enable with CMake options:
```bash
cmake -DSCRATCHROBIN_ENABLE_ASAN=ON ..
cmake -DSCRATCHROBIN_ENABLE_COVERAGE=ON ..
```

## Documentation

### Project Overview
- [`PROJECT_STATUS.md`](PROJECT_STATUS.md) - Current project status and completion metrics
- [`docs/ROADMAP.md`](docs/ROADMAP.md) - High-level roadmap and milestones
- [`docs/planning/MASTER_IMPLEMENTATION_TRACKER.md`](docs/planning/MASTER_IMPLEMENTATION_TRACKER.md) - Detailed task tracker (259 tasks)

### Architecture & Design
- `docs/ARCHITECTURE.md` - System architecture overview
- `docs/CONFIGURATION.md` - Configuration options and TOML format
- `docs/UI_INVENTORY.md` - UI component inventory
- `docs/UI_WINDOW_MODEL.md` - Window management specification

### Specifications
- `docs/specifications/CONNECTION_PROFILE_EDITOR.md`
- `docs/specifications/TRANSACTION_MANAGEMENT.md`
- `docs/specifications/ERROR_HANDLING.md`
- `docs/specifications/ERD_NOTATION_DICTIONARIES.md`
- `docs/specifications/AUTO_LAYOUT.md`
- `docs/specifications/UNDO_REDO.md`
- `docs/specifications/DATA_TYPE_MAPPING.md`
- `docs/specifications/REVERSE_ENGINEERING.md`
- `docs/specifications/FORWARD_ENGINEERING.md`
- `docs/specifications/CLUSTER_MANAGER_UI.md`
- `docs/specifications/REPLICATION_MANAGER_UI.md`
- `docs/specifications/ETL_MANAGER_UI.md`
- `docs/specifications/GIT_INTEGRATION_UI.md`

### Planning & Analysis
- `docs/planning/QUICK_START_CHECKLIST.md`
- `docs/planning/BACKEND_ADAPTERS_SCOPE.md`
- `docs/findings/SPECIFICATION_GAPS_AND_NEEDS.md`

### Developer Documentation
- `tests/README.md` - Testing guide
- `.clang-format` - Code formatting rules
- `.clang-tidy` - Static analysis configuration

## Configuration

Modernized config lives in TOML. Example files:
- `config/scratchrobin.toml.example`
- `config/connections.toml.example`

## License

Initial Developer's Public License Version 1.0 (IDPL)
See: https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
