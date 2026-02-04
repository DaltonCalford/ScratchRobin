# ScratchRobin Outstanding Items Tracker

**Created**: 2026-02-03  
**Last Updated**: 2026-02-03  
**Overall Completion**: ~78-80%

This document tracks all TODOs, FIXMEs, and unimplemented features found in the ScratchRobin codebase.

---

## 📊 Summary by Category

| Category | Items | Status | Priority |
|----------|-------|--------|----------|
| Beta Placeholder Features | 4 | 🔴 Not Started | P1 |
| API Generator | 8 | 🔴 Stub Only | P1 |
| CDC/Streaming | 8 | 🔴 Framework Only | P2 |
| Data Masking | 6 | 🟡 Models Only | P2 |
| Git Integration | 4 | 🔴 Not Started | P0 |
| UI Polish | 11 | 🟡 Minor TODOs | P3 |
| Core Features | 2 | 🟡 Partial | P1 |

---

## 🔴 BETA PLACEHOLDER FEATURES (4 items)

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

### 4. Git Integration
- **File**: `src/ui/git_integration_frame.cpp/h`
- **Status**: 🔴 Stub Only
- **Priority**: P0 (Critical for workflow)
- **Description**: Shows mock Git status display only
- **Required Implementation**:
  - [ ] Repository initialization
  - [ ] Clone repository
  - [ ] Commit changes with message
  - [ ] Push to remote
  - [ ] Pull from remote
  - [ ] Branch management (create/checkout/merge)
  - [ ] Visual diff for schema changes
  - [ ] Migration script versioning
  - [ ] Conflict resolution UI
  - [ ] Pull request integration
- **Dependencies**: `src/core/project.cpp` TODOs

---

## 🔴 API GENERATOR (8 items)

**File**: `src/core/api_generator.cpp`

All code generation functions return stub strings.

| # | Function | Status | Priority |
|---|----------|--------|----------|
| 1 | `GenerateServer()` | 🔴 Returns `true` only | P1 |
| 2 | `GenerateClient()` | 🔴 Returns `true` only | P1 |
| 3 | `GenerateController()` | 🔴 Returns "// Controller stub" | P1 |
| 4 | `GenerateModel()` | 🔴 Returns "// Model stub" | P1 |
| 5 | `GenerateTests()` | 🔴 Returns "// Tests stub" | P2 |
| 6 | `GenerateDockerfile()` | 🔴 Returns "# Dockerfile stub" | P2 |
| 7 | `GeneratePythonFastApi()` | 🔴 Returns "# FastAPI stub" | P1 |
| 8 | `GenerateNodeExpress()` | 🔴 Returns "// Express.js stub" | P1 |
| 9 | `GenerateJavaSpring()` | 🔴 Returns "// Spring Boot stub" | P2 |
| 10 | `GenerateGoGin()` | 🔴 Returns "// Gin stub" | P2 |
| 11 | **UI for API Generation** | 🔴 Not Created | P1 |

### Required Implementation:
- [ ] Actual code generation for each language/framework
- [ ] Template system for code generation
- [ ] Database-to-API mapping logic
- [ ] OpenAPI spec generation
- [ ] UI dialog for API configuration
- [ ] Preview generated code before saving

---

## 🔴 CDC/STREAMING (8 items)

**File**: `src/core/cdc_streaming.cpp`

Framework exists but no actual connectors implemented.

| # | Connector | Status | Priority |
|---|-----------|--------|----------|
| 1 | Apache Kafka | 🔴 Not Implemented | P1 |
| 2 | RabbitMQ | 🔴 Not Implemented | P2 |
| 3 | AWS Kinesis | 🔴 Not Implemented | P2 |
| 4 | Google Pub/Sub | 🔴 Not Implemented | P2 |
| 5 | Azure Event Hubs | 🔴 Not Implemented | P2 |
| 6 | Redis Pub/Sub | 🔴 Not Implemented | P3 |
| 7 | NATS | 🔴 Not Implemented | P3 |
| 8 | Apache Pulsar | 🔴 Not Implemented | P3 |

### Required Implementation:
- [ ] CdcConnector base class implementations for each broker
- [ ] Message serialization (Debezium format)
- [ ] Connection management for each broker type
- [ ] Topic/queue management
- [ ] Error handling and retry logic
- [ ] UI for CDC pipeline configuration

---

## 🟡 DATA MASKING (6 items)

**File**: `src/core/data_masking.cpp`

Data models exist but masking algorithms not implemented.

| # | Algorithm | Status | Priority |
|---|-----------|--------|----------|
| 1 | Cryptographic Hash (SHA-256) | 🔴 Not Implemented | P1 |
| 2 | Format-Preserving Encryption | 🔴 Not Implemented | P1 |
| 3 | Randomization | 🔴 Not Implemented | P2 |
| 4 | Shuffling | 🔴 Not Implemented | P2 |
| 5 | Substitution/Fake Data | 🔴 Not Implemented | P2 |
| 6 | **UI for Masking Rules** | 🔴 Not Created | P1 |

### Required Implementation:
- [ ] Hash-based masking with salt
- [ ] FPE (Format-Preserving Encryption) implementation
- [ ] Random data generation for types
- [ ] Column-level shuffling
- [ ] Fake data generation (names, addresses, etc.)
- [ ] Masking rules UI dialog
- [ ] Preview masking results

---

## 🟡 GIT INTEGRATION (4 items)

**File**: `src/core/project.cpp`

| # | Feature | Line | Status | Priority |
|---|---------|------|--------|----------|
| 1 | Git sync to database repo | 480 | 🔴 Not Implemented | P0 |
| 2 | Git sync from database repo | 488 | 🔴 Not Implemented | P0 |
| 3 | Conflict resolution | 496 | 🔴 Not Implemented | P0 |
| 4 | Extraction from database | 502 | 🔴 Not Implemented | P0 |

### Required Implementation:
- [ ] Git repository management
- [ ] Schema-to-Git synchronization
- [ ] Git-to-schema synchronization
- [ ] Conflict detection and resolution
- [ ] Database extraction to design files

---

## 🟡 UI POLISH (11 items)

Minor TODOs throughout UI components.

| # | File | Line | Description | Priority |
|---|------|------|-------------|----------|
| 1 | `reverse_engineer.cpp` | 420 | Schema comparison | P2 |
| 2 | `reverse_engineer.cpp` | 428 | Apply differences to diagram | P2 |
| 3 | `users_roles_frame.cpp` | 907 | Populate user details fields | P3 |
| 4 | `users_roles_frame.cpp` | 965 | Populate role details fields | P3 |
| 5 | `help_browser.cpp` | 600 | Find-in-page dialog | P3 |
| 6 | `package_manager_frame.cpp` | 761 | Set SQL text in editor | P3 |
| 7 | `package_manager_frame.cpp` | 778 | Set SQL text in editor | P3 |
| 8 | `io_statistics_panel.cpp` | 918 | Custom date range dialog | P3 |
| 9 | `ai_assistant_panel.cpp` | 337 | Async AI request (may be done) | P2 |
| 10 | `table_statistics_panel.cpp` | 761 | Analyze all tables | P2 |
| 11 | `table_statistics_panel.cpp` | 767 | Vacuum all tables | P2 |

---

## 🟡 CORE FEATURES (2 items)

| # | Feature | File | Status | Priority |
|---|---------|------|--------|----------|
| 1 | Data lineage retention policy | `data_lineage.cpp:492` | 🔴 Not Implemented | P3 |
| 2 | Query cancellation (needs ScratchBird) | `embedded_backend.cpp:215` | ⏸️ Blocked | P3 |

---

## 📋 IMPLEMENTATION ROADMAP

### Phase A: Critical Features (P0-P1)
**Timeline**: 2-3 weeks

1. **Git Integration** (P0)
   - Implement Git sync in `project.cpp`
   - Make `git_integration_frame.cpp` functional
   - Add commit/push/pull/branch UI

2. **API Generator** (P1)
   - Implement code generation for Python/FastAPI
   - Implement code generation for Node/Express
   - Create UI dialog for API generation

3. **Table Statistics** (P1)
   - Implement Analyze All functionality
   - Implement Vacuum All functionality

### Phase B: Beta Features (P1)
**Timeline**: 3-4 weeks

4. **Cluster Manager** (P1)
5. **Replication Manager** (P1)
6. **ETL Manager** (P1)

### Phase C: Advanced Features (P2)
**Timeline**: 2-3 weeks

7. **Data Masking** (P2)
8. **CDC/Streaming** (P2)
   - Implement Kafka connector first

### Phase D: Polish (P3)
**Timeline**: 1-2 weeks

9. UI polish items
10. Data lineage retention

---

## ✅ DEFINITION OF DONE

For each item to be considered complete:
- [ ] Code implemented and compiles without warnings
- [ ] Unit tests written and passing
- [ ] UI functional and tested (if applicable)
- [ ] Documentation updated
- [ ] TODO/FIXME comment removed from source
- [ ] Item checked off in this tracker

---

## 📝 NOTES

- **Beta Placeholders**: These show "BETA FEATURE PREVIEW" in the UI. Should either be implemented or removed before GA.
- **Build Status**: Currently blocked by ScratchBird btree.cpp compilation errors - these are pre-existing issues in the database engine dependency.
- **Priority Legend**:
  - P0: Critical - blocking for release
  - P1: Important - expected for GA
  - P2: Nice-to-have - can be deferred
  - P3: Polish - quality improvements

---

*Last updated: 2026-02-03*
