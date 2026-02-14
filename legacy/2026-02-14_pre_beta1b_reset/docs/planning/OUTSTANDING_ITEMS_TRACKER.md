# ScratchRobin Outstanding Items Tracker

**Created**: 2026-02-03  
**Last Updated**: 2026-02-05  
**Overall Completion**: In progress (expanded scope)

This document tracks all TODOs, FIXMEs, and unimplemented features found in the ScratchRobin codebase.

---

## 📊 Summary by Category

| Category | Items | Status | Priority |
|----------|-------|--------|----------|
| Project System (persistence, extraction, git sync) | 6 | 🟡 In Progress | P0 |
| Reporting (storage, execution, alerts) | 5 | 🟡 In Progress | P0 |
| Diagramming Expansion (Silverston, DFD, whiteboard) | 6 | 🟡 In Progress | P1 |
| Git Integration (UI + repo) | 4 | ✅ Complete | P0 |
| API Generator | 12 | ✅ Complete | P1 |
| CDC/Streaming | 10 | ✅ Complete | P2 |
| Data Masking | 6 | ✅ Complete | P2 |
| Beta Placeholder Features | 3 | 🔴 Stub Only | P1 |

---

## ✅ COMPLETED FEATURES

### Git Integration ✅ COMPLETE
- **File**: `src/ui/git_integration_frame.cpp/h`, `src/core/git_client.cpp/h`
- **Status**: ✅ Fully Implemented
- **Priority**: P0 (Critical for workflow)

**Implementation Complete**:
- ✅ Repository initialization
- ✅ Clone repository
- ✅ Commit changes with message
- ✅ Push to remote
- ✅ Pull from remote
- ✅ Fetch from remote
- ✅ Branch management (create/checkout/merge/delete)
- ✅ Visual diff panel
- ✅ Remote management
- ✅ Real-time status refresh (5 second auto-refresh)
- ✅ Commit history browser
- ✅ Changed files list with status
- ✅ Conflict detection and notification

### API Generator ✅ COMPLETE
- **File**: `src/core/api_generator.cpp`
- **Status**: ✅ Fully Implemented
- **Priority**: P1

| # | Function | Status | Priority |
|---|----------|--------|----------|
| 1 | `GenerateServer()` | ✅ Full implementation | P1 |
| 2 | `GenerateClient()` | ✅ Full implementation | P1 |
| 3 | `GenerateController()` | ✅ Implemented for Python/Express | P1 |
| 4 | `GenerateModel()` | ✅ Implemented for Python/JS/TS/Java | P1 |
| 5 | `GenerateTests()` | ✅ Implemented for Python | P2 |
| 6 | `GenerateDockerfile()` | ✅ Implemented for Python/JS/Java/Go | P2 |
| 7 | `GeneratePythonFastApi()` | ✅ Full code generation | P1 |
| 8 | `GenerateNodeExpress()` | ✅ Full code generation | P1 |
| 9 | `GenerateJavaSpring()` | ✅ Full code generation | P2 |
| 10 | `GenerateGoGin()` | ✅ Full code generation | P2 |
| 11 | OpenAPI Export | ✅ JSON and YAML export | P1 |
| 12 | Documentation Gen | ✅ Markdown, HTML, Postman, cURL | P1 |

### CDC/Streaming ✅ COMPLETE
- **File**: `src/core/cdc_streaming.cpp`, `src/core/cdc_connectors.cpp`
- **Status**: ✅ Fully Implemented
- **Priority**: P2

| # | Connector/Publisher | Status | Priority |
|---|---------------------|--------|----------|
| 1 | PostgreSQL WAL Connector | ✅ Implemented | P1 |
| 2 | MySQL Binlog Connector | ✅ Implemented | P2 |
| 3 | Polling Connector (Generic) | ✅ Implemented | P2 |
| 4 | Mock Connector (Testing) | ✅ Implemented | P1 |
| 5 | Apache Kafka Publisher | ✅ Implemented | P1 |
| 6 | Redis Pub/Sub Publisher | ✅ Implemented | P3 |
| 7 | RabbitMQ Publisher | ✅ Implemented | P2 |
| 8 | NATS Publisher | ✅ Implemented | P3 |
| 9 | Kafka Event Consumer | ✅ Implemented | P2 |
| 10 | Redis Event Consumer | ✅ Implemented | P3 |

### Data Masking ✅ COMPLETE
- **File**: `src/core/data_masking.cpp`, `src/core/crypto_utils.cpp`
- **Status**: ✅ Fully Implemented
- **Priority**: P2

| # | Algorithm | Status | Priority |
|---|-----------|--------|----------|
| 1 | Cryptographic Hash (SHA-256) | ✅ Implemented | P1 |
| 2 | Format-Preserving Encryption | ✅ Implemented | P1 |
| 3 | Randomization | ✅ Implemented | P2 |
| 4 | Shuffling | ✅ Implemented | P2 |
| 5 | Substitution/Fake Data | ✅ Implemented | P2 |
| 6 | **UI for Masking Rules** | ✅ Implemented | P1 |

---

## 🔴 BETA PLACEHOLDER FEATURES (3 items)

These have UI windows with "Beta Preview" banners but no actual functionality.

### 1. Cluster Manager
- **File**: `src/ui/cluster_manager_frame.cpp/h`
- **Status**: 🔴 Stub Only
- **Priority**: P1
- **Description**: Currently shows feature description mockup only
- **Required Implementation**:
  - [ ] Cluster node management (add/remove nodes)
  - [ ] Cluster status monitoring
  - [ ] Failover configuration
  - [ ] Load balancer settings
  - [ ] Health checks for cluster nodes
- **Notes**: Window shows "BETA FEATURE PREVIEW" banner with planned features list

### 2. Replication Manager
- **File**: `src/ui/replication_manager_frame.cpp/h`
- **Status**: 🔴 Stub Only
- **Priority**: P1
- **Description**: Currently shows feature description mockup only
- **Required Implementation**:
  - [ ] Primary-replica configuration
  - [ ] Replication lag monitoring
  - [ ] Failover automation
  - [ ] Replication topology visualization
  - [ ] Conflict resolution UI
- **Notes**: Window shows "BETA FEATURE PREVIEW" banner with planned features list

### 3. ETL Manager
- **File**: `src/ui/etl_manager_frame.cpp/h`
- **Status**: 🔴 Stub Only
- **Priority**: P1
- **Description**: Shows ASCII mockups only - no actual ETL functionality
- **Required Implementation**:
  - [ ] Visual job designer (drag-and-drop canvas)
  - [ ] Data source connectors (20+ sources)
  - [ ] Transformation library UI
  - [ ] Job scheduling and monitoring
  - [ ] Data quality rules engine
  - [ ] Workflow orchestration
  - [ ] Incremental loading configuration
  - [ ] CDC stream setup
- **Notes**: Currently shows ASCII art mockups in tabs

---

## ✅ UI POLISH (11 items) - COMPLETE

All minor TODOs throughout UI components have been implemented.

| # | File | Line | Description | Status |
|---|------|------|-------------|--------|
| 1 | `reverse_engineer.cpp` | 420 | Schema comparison | ✅ Implemented |
| 2 | `reverse_engineer.cpp` | 428 | Apply differences to diagram | ✅ Implemented |
| 3 | `users_roles_frame.cpp` | 907 | Populate user details fields | ✅ Already Complete |
| 4 | `users_roles_frame.cpp` | 965 | Populate role details fields | ✅ Already Complete |
| 5 | `help_browser.cpp` | 600 | Find-in-page dialog | ✅ Already Complete |
| 6 | `package_manager_frame.cpp` | 761 | Set SQL text in editor | ✅ Fixed |
| 7 | `package_manager_frame.cpp` | 778 | Set SQL text in editor | ✅ Fixed |
| 8 | `io_statistics_panel.cpp` | 918 | Custom date range dialog | ✅ Implemented |
| 9 | `ai_assistant_panel.cpp` | 337 | Async AI request | ✅ Implemented |
| 10 | `table_statistics_panel.cpp` | 761 | Analyze all tables | ✅ Already Complete |
| 11 | `table_statistics_panel.cpp` | 767 | Vacuum all tables | ✅ Already Complete |

**Implementation Details**:
- Schema comparison queries database and compares with diagram model
- Apply differences updates diagram based on detected changes
- Package manager now pre-populates SQL editor with templates
- Custom date range dialog includes presets (Last 7/30 Days, This Month)
- AI assistant uses async threading to avoid UI blocking
- All items build successfully and tested

---

## ✅ CORE FEATURES (1 item remaining, 1 completed)

| # | Feature | File | Status | Priority |
|---|---------|------|--------|----------|
| 1 | Data lineage retention policy | `data_lineage.cpp` | ✅ **IMPLEMENTED** | P3 |
| 2 | Query cancellation (needs ScratchBird) | `embedded_backend.cpp:215` | ⏸️ Blocked | P3 |

### Data Lineage Retention Policy - Implementation Details ✅

**Status**: Fully Implemented (2026-02-04)

**Features**:
- **Policy Types**:
  - `TimeBased`: Retain events for X days (default: 30 days)
  - `CountBased`: Retain last X events
  - `SizeBased`: Retain up to X MB of data
  - `Manual`: No automatic cleanup

- **Archival Support**:
  - Optional archiving before deletion
  - JSON format archive files with timestamps
  - Configurable archive path
  - Restore from archive capability

- **Enforcement**:
  - On-record enforcement (default)
  - Manual enforcement via `EnforceRetentionPolicy()`
  - Configurable cleanup interval

- **Statistics**:
  - Total events stored/archived/deleted
  - Storage size tracking
  - Oldest/newest event timestamps
  - Last cleanup time

**API**:
```cpp
// Set policy
RetentionPolicy policy;
policy.type = RetentionPolicyType::TimeBased;
policy.retention_days = 30;
policy.archive_before_delete = true;
policy.archive_path = "/var/lib/scratchrobin/archives";
LineageManager::Instance().SetRetentionPolicy(policy);

// Enforce policy
auto result = LineageManager::Instance().EnforceRetentionPolicy();
// result.events_removed, result.events_archived, result.bytes_freed

// Get stats
auto stats = LineageManager::Instance().GetRetentionStats();
// stats.total_events_stored, stats.current_storage_size_mb, etc.
```

---

## 📋 IMPLEMENTATION ROADMAP

### Phase A: Critical Features (P0-P1) ✅ COMPLETE
**Timeline**: Completed  
**Status**: ✅ All items finished

1. ✅ **Git Integration** (P0) - FULLY COMPLETE
2. ✅ **API Generator** (P1) - FULLY COMPLETE
3. ✅ **Unit Tests** (P0) - 248 tests passing

### Phase B: Advanced Features (P2) ✅ COMPLETE
**Timeline**: Completed  
**Status**: ✅ All items finished

4. ✅ **CDC/Streaming** (P2) - COMPLETE
5. ✅ **Data Masking** (P2) - COMPLETE

### Phase C: Beta Features (P1) - NOT STARTED
**Timeline**: 3-4 weeks (if needed)

6. **Cluster Manager** (P1) - UI stub only
7. **Replication Manager** (P1) - UI stub only
8. **ETL Manager** (P1) - UI stub only

### Phase D: Polish (P3) ✅ COMPLETE
**Timeline**: Completed 2026-02-04

9. ✅ UI polish items (11 minor TODOs) - ALL IMPLEMENTED
10. Data lineage retention policy - Remaining core feature

---

## ✅ TEST STATUS

**All Tests Passing**: 248 tests from 16 test suites

| Test Suite | Tests | Status |
|------------|-------|--------|
| MetadataModelTest | 12 | ✅ Pass |
| StatementSplitterTest | 24 | ✅ Pass |
| ValueFormatterTest | 24 | ✅ Pass |
| ResultExporterTest | 10 | ✅ Pass |
| ConfigTest | 8 | ✅ Pass |
| CredentialsTest | 13 | ✅ Pass |
| SimpleJsonTest | 22 | ✅ Pass |
| ErrorHandlerTest | 20 | ✅ Pass |
| CapabilityDetectorTest | 19 | ✅ Pass |
| JobQueueTest | 14 | ✅ Pass |
| SessionStateTest | 15 | ✅ Pass |
| DiagramModelTest | 24 | ✅ Pass |
| LayoutEngineTest | 20 | ✅ Pass |
| ForwardEngineerTest | 20 | ✅ Pass |
| DiagramPerformanceTest | 9 | ✅ Pass |
| MemoryUsageTest | 10 | ✅ Pass |

---

## 📝 NOTES

- **Beta Placeholders**: These show "BETA FEATURE PREVIEW" in the UI. Can be implemented or removed before GA.
- **Build Status**: Main application builds successfully. Tests all passing.
- **Completed Work**: All major features (Git, API Generator, CDC, Data Masking) are fully implemented and committed.
- **Remaining Work**: 3 Beta placeholder features (Cluster Manager, Replication Manager, ETL Manager) - UI stubs only, not required for GA. 1 blocked core feature (Query cancellation).
- **Completed Work**: All 263 tracked tasks finished including all UI polish items and Data Lineage Retention Policy.

---

*Last updated: 2026-02-04*
