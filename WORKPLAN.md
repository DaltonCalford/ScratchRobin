# ScratchRobin Implementation Workplan
## P0 (Critical) and P1 (Short-Term) Tasks

**Date:** March 2026  
**Objective:** Close the gap between architectural intent and implementation reality

---

## Executive Summary

Based on the code audit and review of the ScratchBird SBLR v3 codebase, this workplan addresses the critical disconnect between ScratchRobin's "native compiled execution path" architecture and its actual implementation. The key deliverables are:

1. **Real SQL→SBLR compilation** (replacing the fake `"SBLR:" + sql` prefix)
2. **Unified backend execution** (eliminating dual-path confusion)
3. **Security hardening** (parameterized queries replacing string interpolation)
4. **Integration integrity** (fixing macro naming bugs)
5. **Feature completion** (replacing "not yet implemented" stubs)

---

## Phase 1: P0 Critical Fixes (Weeks 1-2)

### Task 1.1: Fix SBWP Integration Macro Naming Bug
**Priority:** P0 - Integration Break  
**Effort:** 2 hours  
**Files:** `src/backend/scratchbird_sbwp_client.cpp`, `docs/testing/CONFORMANCE_MATRIX.md`

**Problem:** The CMake defines `SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP=1` but source code checks for `ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP`. This means the real SBWP integration is **compiled out**.

**Implementation:**
```cpp
// BEFORE (src/backend/scratchbird_sbwp_client.cpp line 17):
#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)

// AFTER:
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
```

**Verification:**
- Build with `SCRATCHROBIN_ENABLE_SBWP=ON`
- Verify `scratchbird_sbwp_client.cpp` compiles with real integration (not fallback stubs)
- Check that SBWP headers are included (lines 25-32)

**Acceptance Criteria:**
- [ ] All 8 occurrences of `ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP` replaced
- [ ] Build succeeds with SBWP enabled
- [ ] Real SBWP code path is active (not fallback mocks)

---

### Task 1.2: Unify Backend Execution Paths
**Priority:** P0 - Architectural Integrity  
**Effort:** 16 hours  
**Files:** `src/backend/*.cpp`, `src/backend/*.h`, `src/ui/main_window.cpp`

**Problem:** The app has two overlapping execution models:
- **Strict path:** `SessionClient → NativeAdapterGateway → ServerSessionGateway` (SBLR bytecode only)
- **Pragmatic path:** `ScratchbirdConnection → direct SQL execution` (bypasses native path)

**Current State:**
- `MainWindow` uses `db_connection_->execute(...)` (direct SQL)
- Panels receive `session_client_` (native path) but aren't used consistently
- `ServerSessionGateway` validates bytecode then returns **fake results**

**Solution Architecture:**

```
┌─────────────────────────────────────────────────────────────────┐
│                      ScratchRobin UI Layer                       │
│  (SQL Editor, Query Builder, Data Grid, Metadata Provider)      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     QueryRouter (NEW)                            │
│  - Analyzes query type (DDL, DML, SELECT, etc.)                 │
│  - Routes to appropriate execution path                         │
│  - Enforces bytecode policy for DML/SELECT                      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
           ┌───────────────┴───────────────┐
           │                               │
           ▼                               ▼
┌──────────────────────┐      ┌──────────────────────────┐
│  Native Path         │      │  Direct SQL Path         │
│  (new implementation)│      │  (DDL, metadata queries) │
├──────────────────────┤      ├──────────────────────────┤
│ SQL→SBLR Compiler    │      │ ScratchbirdConnection    │
│ (v3 QueryCompiler)   │      │ (existing)               │
│         ↓            │      │         ↓                │
│ SBWP Client          │      │ Direct SQL execution     │
│ (executeBytecode)    │      │ (information_schema,     │
│         ↓            │      │  SHOW CREATE, etc.)      │
│ ScratchBird Server   │      │                          │
└──────────────────────┘      └──────────────────────────┘
```

**Implementation Steps:**

1. **Create QueryRouter** (`src/backend/query_router.h`, `.cpp`)
   ```cpp
   enum class QueryType {
     DDL_CREATE, DDL_ALTER, DDL_DROP,  // Route to direct SQL
     DML_SELECT, DML_INSERT, DML_UPDATE, DML_DELETE,  // Route to native
     UTILITY_SHOW, UTILITY_SET,  // Route to direct SQL
     UNKNOWN
   };
   
   class QueryRouter {
    public:
     QueryResponse execute(const std::string& sql, const ExecutionPolicy& policy);
     void setPreferNative(bool prefer) { prefer_native_ = prefer; }
    private:
     QueryType classifyQuery(const std::string& sql);
     bool prefer_native_ = true;
   };
   ```

2. **Modify MainWindow** to use QueryRouter instead of direct `db_connection_`

3. **Update ServerSessionGateway** to call real SBWP client instead of returning fake results

**Acceptance Criteria:**
- [ ] QueryRouter implemented and integrated
- [ ] DDL queries use direct SQL path
- [ ] DML queries use native SBLR path
- [ ] All queries route through single entry point

---

### Task 1.3: Implement Real SQL→SBLR Compilation
**Priority:** P0 - Core Feature  
**Effort:** 24 hours  
**Files:** `src/backend/native_parser_compiler.cpp`, `src/backend/native_parser_compiler.h`

**Problem:** Current implementation is fake:
```cpp
// Current (EMBARRASSING):
CompileOutput NativeParserCompiler::CompileSqlToSblr(const std::string& sql) {
  payload.body = "SBLR:" + sql;  // Just a prefix!
}
```

**Solution:** Integrate with ScratchBird's `QueryCompilerV3`

**Implementation:**

```cpp
#include "scratchbird/sblr/query_compiler_v3.h"
#include "scratchbird/sblr/v3_container.h"
#include "scratchbird/parser/parser_v3.h"

CompileOutput NativeParserCompiler::CompileSqlToSblr(const std::string& sql) const {
  if (sql.empty()) {
    return {core::Status::Error("SQL input is empty"), {}};
  }

  // Use ScratchBird's v3 QueryCompiler
  scratchbird::sblr::QueryCompilerV3 compiler;
  compiler.setStatsEnabled(true);
  
  auto result = compiler.compile(sql);
  
  if (!result.success()) {
    std::string error_msg = "SBLR compilation failed:";
    for (const auto& err : result.errors()) {
      error_msg += "\n  - " + err;
    }
    return {core::Status::Error(error_msg), {}};
  }
  
  // Convert to QueryPayload
  core::QueryPayload payload;
  payload.type = core::QueryPayloadType::kSblrBytecode;
  payload.body = std::string(result.bytecode().begin(), result.bytecode().end());
  payload.metadata["parser_time_us"] = std::to_string(result.stats().parser_time.count());
  payload.metadata["bytecode_size"] = std::to_string(result.stats().bytecode_size);
  
  return {core::Status::Ok(), payload};
}

// NEW: Add compilation tracing for debugging
CompileOutput NativeParserCompiler::CompileSqlToSblrWithTrace(
    const std::string& sql,
    std::string* trace_output) const {
  scratchbird::sblr::QueryCompilerV3 compiler;
  auto trace_result = compiler.compileTrace(sql);
  
  if (trace_output && trace_result.success()) {
    std::ostringstream trace;
    trace << "SQL Hash: " << trace_result.digest().sql_hash << "\n";
    trace << "AST Hash: " << trace_result.digest().ast_hash << "\n";
    trace << "SBLR Hash: " << trace_result.digest().sblr_hash << "\n";
    trace << "Root Opcode: " << trace_result.digest().root_opcode_symbol << "\n";
    trace << "Normalized: " << trace_result.digest().normalized_sql << "\n";
    *trace_output = trace.str();
  }
  
  // Then do actual compilation
  return CompileSqlToSblr(sql);
}
```

**Build Integration:**
Add to `src/CMakeLists.txt`:
```cmake
# ScratchBird SBLR compilation support
target_include_directories(scratchrobin_backend PRIVATE
    ${SCRATCHBIRD_SOURCE_DIR}/include
)

target_compile_definitions(scratchrobin_backend PRIVATE
    SCRATCHROBIN_WITH_SBLR_COMPILER=1
)
```

**Acceptance Criteria:**
- [ ] Real SBLR bytecode generated (not `"SBLR:" + sql`)
- [ ] Compilation errors properly propagated to UI
- [ ] Stats (bytecode size, parser time) available for display
- [ ] Trace mode available for debugging

---

### Task 1.4: Implement Real Server Session Execution
**Priority:** P0 - Core Feature  
**Effort:** 16 hours  
**Files:** `src/backend/server_session_gateway.cpp`, `src/backend/server_session_gateway.h`

**Problem:** Current implementation returns fake results:
```cpp
// Current (simulated):
QueryResponse response;
response.result_set.rows = {{"ok", "simulated sblr execution"}};  // FAKE!
```

**Solution:** Route to SBWP client for actual execution

**Implementation:**

```cpp
#include "backend/scratchbird_sbwp_client.h"

class ServerSessionGateway {
 public:
  // Dependency injection for testing
  void setSbwpClient(std::shared_ptr<ScratchbirdSbwpClient> client) {
    sbwp_client_ = client;
  }
  
  QueryResponse Execute(const QueryRequest& request) const {
    // Validation remains the same
    if (!request.bytecode_flag) {
      return {core::Status::Error("BYTECODE flag required"), {}, 
              "server_session::reject_missing_bytecode_flag"};
    }
    if (request.payload.type != core::QueryPayloadType::kSblrBytecode) {
      return {core::Status::Error("SBLR bytecode required"), {},
              "server_session::reject_sql_text"};
    }
    
    // NEW: Real execution via SBWP
    if (!sbwp_client_) {
      return {core::Status::Error("SBWP client not available"), {},
              "server_session::no_client"};
    }
    
    // Convert payload to vector<uint8_t>
    std::vector<uint8_t> bytecode(request.payload.body.begin(), 
                                   request.payload.body.end());
    
    // Execute via SBWP
    auto sbwp_result = sbwp_client_->executeBytecode(bytecode, request.sql_text);
    
    if (!sbwp_result.success) {
      return {core::Status::Error(sbwp_result.error_message), {},
              "server_session::execute_failed"};
    }
    
    // Convert SBWP result to QueryResponse
    QueryResponse response;
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set = convertSbwpResult(sbwp_result);
    
    return response;
  }
  
 private:
  std::shared_ptr<ScratchbirdSbwpClient> sbwp_client_;
  
  core::ResultSet convertSbwpResult(const SbwpResult& sbwp);
};
```

**Acceptance Criteria:**
- [ ] Real SBLR bytecode execution (not simulated)
- [ ] Results from ScratchBird server returned to UI
- [ ] Error handling propagated properly
- [ ] Execution path tracking maintained

---

## Phase 2: P1 Security & Stability (Weeks 3-4)

### Task 2.1: Implement SessionClient::ConfigureRuntime
**Priority:** P1  
**Effort:** 8 hours  
**Files:** `src/backend/session_client.cpp`, `src/backend/session_client.h`

**Problem:** Currently marked TODO and discards config:
```cpp
void SessionClient::ConfigureRuntime(const ScratchbirdRuntimeConfig& config) const {
  // TODO: Implement runtime configuration
  (void)config;  // Discarded!
}
```

**Solution:** Pass runtime config to SBWP client and adapter gateway

**Implementation:**
```cpp
void SessionClient::ConfigureRuntime(const ScratchbirdRuntimeConfig& config) const {
  // Store config for future connections
  runtime_config_ = config;
  
  // Apply to adapter gateway
  adapter_->setRuntimeConfig(config);
  
  // Apply to SBWP client if connected
  if (sbwp_client_ && sbwp_client_->isConnected()) {
    sbwp_client_->configureRuntime(config);
  }
  
  // Log configuration change
  qDebug() << "Runtime config updated:" 
           << "max_connections:" << config.max_connections
           << "query_timeout_ms:" << config.query_timeout_ms;
}
```

**Acceptance Criteria:**
- [ ] Runtime config stored and applied
- [ ] SBWP client receives config updates
- [ ] Adapter gateway respects config settings

---

### Task 2.2: SQL Injection Prevention in Metadata Provider
**Priority:** P1 - Security Critical  
**Effort:** 24 hours  
**Files:** `src/backend/scratchbird_metadata_provider.cpp`

**Problem:** Extensive string interpolation without quoting:
```cpp
// Current (VULNERABLE):
QString sql = QString("SELECT * FROM information_schema.tables WHERE table_name = '%1'")
              .arg(name);  // No escaping!
```

**Solution:** Implement parameterized query support

**Implementation:**

1. **Add Identifier Escaping Utility** (`src/core/sql_utils.h`):
```cpp
namespace scratchrobin::core {

// Escape SQL identifiers (table names, column names, etc.)
std::string escapeIdentifier(const std::string& identifier);

// Escape SQL string literals
std::string escapeStringLiteral(const std::string& value);

// Validate identifier format (prevent injection via identifier names)
bool isValidIdentifier(const std::string& identifier);

} // namespace scratchrobin::core
```

2. **Update Metadata Provider** to use escaping:
```cpp
// BEFORE (vulnerable):
QString sql = QString("SELECT * FROM information_schema.tables WHERE table_name = '%1'")
              .arg(name);

// AFTER (safe):
QString sql = QString("SELECT * FROM information_schema.tables WHERE table_name = '%1'")
              .arg(QString::fromStdString(core::escapeStringLiteral(name.toStdString())));

// For identifiers (schema/table names):
QString fullTable = schema.isEmpty() 
    ? QString::fromStdString(core::escapeIdentifier(name.toStdString()))
    : QString::fromStdString(core::escapeIdentifier(schema.toStdString())) + "." +
      QString::fromStdString(core::escapeIdentifier(name.toStdString()));
```

3. **Create ParameterizedQueryBuilder** for complex queries:
```cpp
class ParameterizedQueryBuilder {
 public:
  ParameterizedQueryBuilder& select(const std::vector<std::string>& columns);
  ParameterizedQueryBuilder& from(const std::string& table);
  ParameterizedQueryBuilder& where(const std::string& column, 
                                    const std::string& op,
                                    const std::string& value);
  
  struct Query {
    std::string sql;
    std::vector<std::string> params;
  };
  
  Query build() const;
};
```

**Acceptance Criteria:**
- [ ] All string interpolations use proper escaping
- [ ] Identifier validation prevents injection
- [ ] No raw `.arg()` without escaping in metadata queries
- [ ] Unit tests for injection attempts

---

### Task 2.3: Implement Critical UI Stubs
**Priority:** P1 - Feature Completeness  
**Effort:** 32 hours (can parallelize)  
**Files:** `src/ui/main_window.cpp`, various feature files

**High-Priority Stubs to Implement:**

| Feature | File | Lines | Implementation |
|---------|------|-------|----------------|
| Transaction Menu | `main_window.cpp` | ~20 | Wire to SBWP begin/commit/rollback |
| Find/Replace | `main_window.cpp` | ~10 | Integrate with SQLEditor find |
| Layout Management | `main_window.cpp` | ~5 | Save/restore dock layouts |
| Query Plan Load | `query_plan_visualizer.cpp` | ~10 | File dialog + parse |
| Schema Compare Apply | `schema_compare.cpp` | ~20 | Generate + execute DDL |
| Data Modeler Import | `data_modeler.cpp` | ~15 | CSV/JSON import |

**Implementation Example (Transaction Menu):**
```cpp
void MainWindow::on_actionCommit_triggered() {
  if (!db_connection_ || !db_connection_->isConnected()) {
    QMessageBox::warning(this, tr("Error"), tr("Not connected to database"));
    return;
  }
  
  if (db_connection_->commit()) {
    showStatusMessage(tr("Transaction committed"), 2000);
    updateTransactionIndicator(false);
  } else {
    QMessageBox::critical(this, tr("Error"), 
                          tr("Commit failed: %1").arg(
                            QString::fromStdString(db_connection_->lastError())));
  }
}
```

**Acceptance Criteria:**
- [ ] Transaction menu fully functional
- [ ] Find/Replace dialog integrated
- [ ] Layout save/restore working
- [ ] No "not yet implemented" for P1 features

---

## Phase 3: P1 Architecture Hardening (Weeks 5-6)

### Task 3.1: Add Query Parameterization Support
**Priority:** P1  
**Effort:** 24 hours  
**Files:** `src/backend/*.cpp`, `src/ui/query_parameters.cpp`

**Problem:** No support for parameterized queries (`$1`, `$2`, `?` placeholders)

**Solution:** Extend execution pipeline to support parameter binding

**Implementation:**
```cpp
// Extend QueryRequest
struct QueryRequest {
  // ... existing fields ...
  std::vector<core::QueryParameter> parameters;
};

struct QueryParameter {
  enum class Type { NULL, BOOL, INT, FLOAT, STRING, BYTES };
  Type type;
  std::variant<std::monostate, bool, int64_t, double, std::string, std::vector<uint8_t>> value;
};

// Update NativeAdapterGateway to handle parameters
QueryResponse NativeAdapterGateway::SubmitSqlWithParams(
    const std::string& sql,
    const std::vector<core::QueryParameter>& params) {
  
  // Compile SQL with parameter placeholders
  auto compile_result = compiler_->CompileSqlToSblr(sql);
  
  // Bind parameters to SBLR container
  if (!params.empty()) {
    bindParametersToSblr(compile_result.payload, params);
  }
  
  return gateway_->Execute(createRequest(compile_result));
}
```

**Acceptance Criteria:**
- [ ] Parameter binding in native path
- [ ] Parameter binding in direct SQL path
- [ ] UI parameter dialog functional
- [ ] Type conversion handling

---

### Task 3.2: Add Execution Metrics & Observability
**Priority:** P1  
**Effort:** 16 hours  
**Files:** `src/backend/`, `src/ui/performance_dashboard.cpp`

**Implementation:**
```cpp
struct ExecutionMetrics {
  std::chrono::microseconds compilation_time;
  std::chrono::microseconds execution_time;
  size_t rows_returned;
  size_t bytes_transferred;
  std::string execution_plan;
  std::string cache_hit;  // "hit", "miss", "none"
};

// Add to QueryResponse
struct QueryResponse {
  // ... existing fields ...
  ExecutionMetrics metrics;
};
```

**Acceptance Criteria:**
- [ ] Compilation time tracked
- [ ] Execution time tracked
- [ ] Row counts accurate
- [ ] Display in performance dashboard

---

## Phase 4: Integration & Testing (Week 7)

### Task 4.1: End-to-End Integration Tests
**Priority:** P1  
**Effort:** 24 hours  
**Files:** `tests/integration/`

**Test Scenarios:**
1. **SQL→SBLR→Execution roundtrip**
   ```cpp
   TEST_F(IntegrationTest, SqlToSblrRoundtrip) {
     std::string sql = "SELECT id, name FROM users WHERE active = true";
     
     // Compile
     auto compile_result = compiler.CompileSqlToSblr(sql);
     ASSERT_TRUE(compile_result.status.ok);
     
     // Execute
     auto response = gateway.Execute(createRequest(compile_result.payload));
     ASSERT_TRUE(response.status.ok);
     
     // Verify real results (not "simulated")
     EXPECT_NE(response.execution_path, "server_session::simulated");
   }
   ```

2. **Parameterized query execution**
3. **Transaction begin/commit/rollback**
4. **Error propagation (compilation → UI)**

### Task 4.2: Security Audit Tests
**Priority:** P1  
**Effort:** 8 hours  
**Files:** `tests/security/`

**Test Cases:**
```cpp
TEST_F(SecurityTest, MetadataProvider_NoSqlInjection) {
  // Attempt injection via table name
  QString malicious_table = "users'; DROP TABLE users; --";
  
  // Should escape, not execute injection
  auto result = provider.table(malicious_table);
  
  // Verify escaping worked (no exception, proper handling)
  EXPECT_TRUE(result.name.isEmpty() || result.name != "users");
}
```

---

## Build & Deployment Checklist

### CMake Configuration Updates
```cmake
# In src/CMakeLists.txt

# 1. Fix macro naming
# BEFORE: target_compile_definitions(... ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
# AFTER:  target_compile_definitions(... SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)

# 2. Add ScratchBird includes for SBLR compiler
target_include_directories(scratchrobin_backend PRIVATE
    ${SCRATCHBIRD_SOURCE_DIR}/include
)

# 3. Add link dependencies for SBLR
if(SCRATCHROBIN_ENABLE_SBWP)
    target_link_libraries(scratchrobin_backend PRIVATE
        scratchbird_client  # or appropriate library
    )
endif()
```

---

## Work Breakdown Summary

| Phase | Task | Effort | Risk |
|-------|------|--------|------|
| P0 | 1.1 Macro Naming Fix | 2h | Low |
| P0 | 1.2 Unify Backend Paths | 16h | Medium |
| P0 | 1.3 Real SQL→SBLR | 24h | Medium |
| P0 | 1.4 Real Execution | 16h | Medium |
| P1 | 2.1 ConfigureRuntime | 8h | Low |
| P1 | 2.2 SQL Injection Fix | 24h | High |
| P1 | 2.3 UI Stubs | 32h | Low |
| P1 | 3.1 Parameterization | 24h | Medium |
| P1 | 3.2 Observability | 16h | Low |
| P1 | 4.1 Integration Tests | 24h | Medium |
| P1 | 4.2 Security Tests | 8h | Low |
| **Total** | | **214h (~5 weeks)** | |

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| ScratchBird API changes | Pin to known-good commit; use abstraction layer |
| SBLR compiler bugs | Fallback to direct SQL for known-problematic queries |
| Performance regression | Feature flag for native path; gradual rollout |
| Build complexity | Modular CMake; each task independently toggleable |

---

## Success Metrics

1. **All P0 tasks complete** → Code audit warnings eliminated
2. **SQL→SBLR roundtrip** → Bytecode verified (not `"SBLR:" + sql`)
3. **Security tests pass** → No injection vectors in metadata queries
4. **Integration tests pass** → End-to-end execution works
5. **Macro naming fixed** → SBWP integration active in builds

---

*Document Version: 1.0*  
*Author: AI Assistant*  
*Review Required: Technical Lead, Security Team*
