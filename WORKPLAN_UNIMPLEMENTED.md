# ScratchRobin Unimplemented Areas - Detailed Workplan & Tracker

**Version:** 1.0  
**Created:** March 2026  
**Status:** Active Development  

---

## Legend

- [ ] Not Started
- [~] In Progress
- [x] Complete
- [!] Blocked/Needs Dependency

**Priority:** P0 (Critical) → P1 (High) → P2 (Medium) → P3 (Low)

**Effort:** S (Small <4h) / M (Medium 4-16h) / L (Large 16-40h) / XL (Extra Large 40h+)

---

## PHASE 1: Core Query Execution (P0)

### 1.1 Async Query Executor
**File:** `src/core/async_query_executor.cpp`  
**Status:** Empty Implementation  
**Impact:** Query cancellation, background execution  
**Effort:** L (24h)

| Line | Item | Status | Notes |
|------|------|--------|-------|
| 53 | `AsyncQueryExecutor::AsyncQueryExecutor` - Initialize thread pool | [ ] | Create worker threads |
| 58 | `AsyncQueryExecutor::~AsyncQueryExecutor` - Cleanup threads | [ ] | Join all threads |
| 63 | `AsyncQueryExecutor::execute` - Start async execution | [ ] | Dispatch to thread pool |
| 67 | `AsyncQueryExecutor::cancel` - Cancel running query | [ ] | Signal cancellation |
| 72 | `AsyncQueryExecutor::isRunning` - Check status | [ ] | Atomic bool check |
| 77 | `AsyncQueryExecutor::waitForFinished` - Block until done | [ ] | Condition variable |
| 86 | `AsyncQueryExecutor::progress` - Get progress % | [ ] | Progress tracking |
| 91 | `AsyncQueryExecutor::result` - Get results | [ ] | Return QueryResult |
| 96 | `AsyncQueryExecutor::error` - Get error info | [ ] | Error struct |
| 109 | `executeQueryInternal` - Thread worker | [ ] | Actual DB execution |
| 114 | `processResults` - Parse results | [ ] | Convert to QueryResult |
| 119 | `handleError` - Error handling | [ ] | Error classification |
| 135 | `executeInThread` - Thread entry point | [ ] | Full pipeline |
| 141 | `cleanup` - Resource cleanup | [ ] | RAII cleanup |

**Implementation Notes:**
- Use `std::thread` + `std::condition_variable` for synchronization
- Add `std::atomic<bool> cancel_requested_` flag
- Implement progress callback using `std::function<void(int)>`
- Handle thread pool size (default: 4 threads)

---

### 1.2 Query Execution Control
**File:** `src/ui/main_window.cpp`

| Line | Function | Status | Priority | Effort | Implementation |
|------|----------|--------|----------|--------|----------------|
| 1156 | `onQueryExecuteScript()` | [ ] | P0 | M | Batch execute multiple statements |
| 1160 | `onQueryStop()` | [ ] | P0 | M | Integrate with AsyncQueryExecutor::cancel |
| 1164 | `onQueryExplain()` | [ ] | P0 | S | Execute "EXPLAIN " + sql |
| 1168 | `onQueryExplainAnalyze()` | [ ] | P0 | S | Execute "EXPLAIN ANALYZE " + sql |

**Implementation Details:**
```cpp
// onQueryStop() - Line 1160
void MainWindow::onQueryStop() {
  if (async_executor_ && async_executor_->isRunning()) {
    async_executor_->cancel();
    showStatusMessage(tr("Query cancelled"), 2000);
  }
}
```

---

## PHASE 2: SQL Editor Features (P0-P1)

### 2.1 SQL Formatting
**File:** `src/ui/main_window.cpp:1172`

| Component | Status | Effort | Notes |
|-----------|--------|--------|-------|
| Basic formatting | [ ] | M | Capitalize keywords, indent |
| Configuration dialog | [ ] | M | Indent size, case preferences |
| Formatter engine | [ ] | L | Full SQL parser for formatting |

**Implementation Approach:**
- Option 1: Use external library (sqlparse, pg_format)
- Option 2: Implement basic regex-based formatter
- Add settings: `indent_size`, `keyword_case`, `newline_before_select_item`

---

### 2.2 Comment/Uncomment
**File:** `src/ui/main_window.cpp:1176-1184`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 1176 | `onQueryComment()` | [ ] | S | Add `-- ` at line start |
| 1180 | `onQueryUncomment()` | [ ] | S | Remove `-- ` from line start |
| 1184 | `onQueryToggleComment()` | [ ] | S | Check first 3 chars |

**Implementation:**
```cpp
void MainWindow::onQueryComment() {
  if (auto* editor = currentEditor()) {
    QString text = editor->selectedSql();
    QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
      lines[i] = "-- " + lines[i];
    }
    editor->replaceSelectedText(lines.join('\n'));
  }
}
```

---

### 2.3 Case Conversion
**File:** `src/ui/main_window.cpp:1190-1197`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 1190 | `onQueryUppercase()` | [ ] | S | `text.toUpper()` |
| 1197 | `onQueryLowercase()` | [ ] | S | `text.toLower()` |

---

### 2.4 SQL Editor Color Schemes
**File:** `src/ui/sql_editor.cpp:105`

| Feature | Status | Effort | Notes |
|---------|--------|--------|-------|
| Light theme | [ ] | S | Default Qt palette |
| Dark theme | [ ] | S | Dark color definitions |
| Theme switching | [ ] | M | Runtime theme change |
| Custom themes | [ ] | L | User-defined themes |

---

## PHASE 3: Import/Export (P1)

### 3.1 CSV Import
**File:** `src/ui/main_window.cpp:1402`

| Component | Status | Effort | Implementation Details |
|-----------|--------|--------|------------------------|
| File dialog | [x] | - | Already implemented |
| CSV Parser | [ ] | M | Use existing library or implement RFC 4180 |
| Preview dialog | [ ] | M | Show first 10 rows |
| Column mapping | [ ] | L | Map CSV columns to table columns |
| Batch insert | [ ] | M | INSERT in transactions |
| Progress bar | [ ] | S | Show import progress |

**Implementation Steps:**
1. Create `CsvImportDialog` class
2. Add CSV parser (recommend: `fast-cpp-csv-parser` or custom)
3. Implement preview with `QTableWidget`
4. Generate INSERT statements with parameter binding
5. Execute in batches (1000 rows per transaction)

---

### 3.2 JSON Import
**File:** `src/ui/main_window.cpp:1406`

| Component | Status | Effort | Implementation |
|-----------|--------|--------|----------------|
| JSON Parser | [ ] | S | Use Qt's `QJsonDocument` |
| Schema inference | [ ] | M | Detect column types |
| Flattening | [ ] | M | Handle nested objects |
| Import dialog | [ ] | M | Similar to CSV |

---

### 3.3 Excel Import
**File:** `src/ui/main_window.cpp:1410`

| Component | Status | Effort | Notes |
|-----------|--------|--------|-------|
| XLSX Parser | [ ] | L | Use `QXlsx` library or similar |
| Sheet selection | [ ] | S | Multiple sheets |
| Type detection | [ ] | M | Excel types → SQL types |

**Blocker:** Requires external library (QXlsx or libxlsxwriter)

---

### 3.4 Export Implementations
**Files:** `main_window.cpp:1413-1472`

| Format | Status | Effort | Implementation |
|--------|--------|--------|----------------|
| SQL Export | [ ] | M | Generate INSERT statements |
| CSV Export | [ ] | M | RFC 4180 compliant |
| JSON Export | [ ] | M | QJsonArray/QJsonObject |
| XML Export | [ ] | M | QDomDocument |
| HTML Export | [ ] | S | Simple table markup |
| Excel Export | [ ] | L | Requires external library |

**Implementation Pattern:**
```cpp
void MainWindow::exportToCsv(const QString& fileName) {
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) return;
  
  QTextStream stream(&file);
  // Write headers
  for (int col = 0; col < columnCount; ++col) {
    if (col > 0) stream << ",";
    stream << escapeCsv(headers[col]);
  }
  stream << "\n";
  
  // Write data
  for (const auto& row : results) {
    for (size_t i = 0; i < row.size(); ++i) {
      if (i > 0) stream << ",";
      stream << escapeCsv(QString::fromStdString(row[i]));
    }
    stream << "\n";
  }
}
```

---

## PHASE 4: Data Management Panels (P1-P2)

### 4.1 Data Cleansing Panel
**File:** `src/ui/data_cleansing.cpp`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 710 | `onPreview()` | [ ] | M | Generate preview of changes |
| 715 | `onExecute()` | [ ] | M | Apply cleansing rules |
| 720 | `onSchedule()` | [ ] | L | Schedule recurring job |

**Features:**
- Duplicate detection
- Null value handling
- Format standardization
- Data validation rules

---

### 4.2 Team Sharing Panel
**File:** `src/ui/team_sharing.cpp`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 208 | `loadQueries()` | [ ] | M | Fetch from backend API |
| 366 | `onEditQuery()` | [ ] | S | Open edit dialog |
| 434 | `onDownload()` | [ ] | S | File save dialog |
| 465 | `filterByTag()` | [ ] | S | Tag filtering logic |
| 745 | `updatePermission()` | [ ] | M | Backend permission API |
| 758 | `sendInvite()` | [ ] | M | Email/notification system |
| 894 | `removeMember()` | [ ] | S | Backend API call |
| 990 | `loadActivity()` | [ ] | M | Activity feed API |
| 1107 | `searchBackend()` | [ ] | M | Full-text search |

---

### 4.3 Query Comments Panel
**File:** `src/ui/query_comments.cpp`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 355 | Highlight commented lines | [ ] | M | Editor marker system |
| 500 | Export comments | [ ] | S | File save dialog |
| 515 | Filter comments | [ ] | S | Search text filter |
| 608 | Add mention (@user) | [ ] | M | User autocomplete |
| 886 | Load from backend | [ ] | M | Comments API |
| 918 | Filter logic | [ ] | S | Date/author filtering |

---

## PHASE 5: Schema Management (P1)

### 5.1 Schema Compare - Apply Changes
**File:** `src/ui/schema_compare.cpp:358`

| Feature | Status | Effort | Implementation |
|---------|--------|--------|----------------|
| Generate diff script | [x] | - | Already implemented |
| Preview changes | [ ] | M | Side-by-side diff view |
| Apply changes | [ ] | L | Execute DDL in transaction |
| Rollback on error | [ ] | M | Transaction handling |

**Implementation:**
```cpp
void SchemaComparePanel::onApplyChanges() {
  // Start transaction
  // Execute each DDL statement
  // On error: rollback and show error
  // On success: commit and refresh
}
```

---

### 5.2 Schema Compare - Script Execution
**File:** `src/ui/schema_compare.cpp:511`

| Feature | Status | Effort | Implementation |
|---------|--------|--------|----------------|
| Script editor | [ ] | S | Show generated script |
| Execute button | [ ] | M | Run with progress |
| Error handling | [ ] | M | Show line-by-line errors |

---

## PHASE 6: Monitoring Tools (P2)

### 6.1 Monitoring - Statements
**File:** `src/ui/main_window.cpp:1504`

| Feature | Status | Effort | Query |
|---------|--------|--------|-------|
| Active queries | [ ] | M | `pg_stat_activity` |
| Query text | [ ] | S | Show SQL |
| Execution time | [ ] | S | Duration |
| Kill query | [ ] | M | `pg_cancel_backend` |

---

### 6.2 Monitoring - Locks
**File:** `src/ui/main_window.cpp:1508`

| Feature | Status | Effort | Query |
|---------|--------|--------|-------|
| Lock tree | [ ] | M | `pg_locks` joins |
| Blocked processes | [ ] | M | Lock detection |
| Lock types | [ ] | S | Display |

---

### 6.3 Monitoring - Performance
**File:** `src/ui/main_window.cpp:1512`

| Feature | Status | Effort | Implementation |
|---------|--------|--------|----------------|
| Charts | [ ] | L | Use QChart/Custom |
| Metrics collection | [ ] | M | Poll every 5s |
| History | [ ] | M | Store last hour |

---

## PHASE 7: Core Systems (P1-P2)

### 7.1 Database Migration Manager
**File:** `src/core/database_migration_manager.cpp`

| Line | Function | Status | Effort | Implementation |
|------|----------|--------|--------|----------------|
| 104 | Apply migration | [ ] | M | Execute up() |
| 122 | Apply migrations | [ ] | M | Batch with transaction |

**Features:**
- Migration table tracking
- Version control
- Rollback support

---

### 7.2 Query Profiler
**File:** `src/core/query_profiler.cpp:74`

| Feature | Status | Effort | Implementation |
|---------|--------|--------|----------------|
| Timing collection | [ ] | S | QElapsedTimer |
| Plan analysis | [ ] | L | Parse EXPLAIN output |
| Slow query detection | [ ] | M | Threshold config |
| Reporting | [ ] | M | Generate reports |

---

### 7.3 Server Monitor
**File:** `src/core/server_monitor.cpp`

| Line | Feature | Status | Effort |
|------|---------|--------|--------|
| 64 | Status collection | [ ] | M |
| 118 | Alert system | [ ] | L |

---

### 7.4 Security Manager
**File:** `src/core/security_manager.cpp`

| Line | Function | Current | Target | Effort |
|------|----------|---------|--------|--------|
| 231 | `encryptData()` | Returns "stub_key" | Real encryption | L |
| 238 | `hashPassword()` | Returns "stub_hash" | bcrypt/Argon2 | M |
| 242 | `generateSalt()` | Returns "stub_salt" | Crypto random | S |

**Implementation:**
- Use OpenSSL or libsodium
- Replace all stub implementations
- Add secure key storage

---

## PHASE 8: Backend & Infrastructure (P1)

### 8.1 Real SBLR Compilation
**File:** `src/backend/native_parser_compiler.cpp`

| Status | Effort | Blocker |
|--------|--------|---------|
| [!] | XL | Link ScratchBird libraries |

**Requirements:**
- Link `libscratchbird_sblr.a`
- Link `libscratchbird_parser.a`
- Update CMakeLists.txt
- Test with real SQL queries

---

### 8.2 Server Session Gateway - Real Execution
**File:** `src/backend/server_session_gateway.cpp:26`

| Current | Target | Effort |
|---------|--------|--------|
| Returns "simulated sblr execution" | Execute via SBWP | L |

**Implementation:**
```cpp
QueryResponse ServerSessionGateway::Execute(const QueryRequest& request) {
  // Validate bytecode
  // Send to ScratchBird via SBWP
  // Return actual results
}
```

---

### 8.3 ScratchBird Connection - Schema Filtering
**File:** `src/backend/scratchbird_connection.cpp:144`

| Feature | Status | Effort |
|---------|--------|--------|
| Schema filter in getTables() | [ ] | S |
| Schema filter in getColumns() | [ ] | S |

---

## PHASE 9: Replication & Advanced (P2-P3)

### 9.1 Replication Manager
**File:** `src/ui/replication_manager.cpp`

| Line | Function | Status | Effort |
|------|----------|--------|--------|
| 185 | Enable subscription | [ ] | M |
| 191 | Disable subscription | [ ] | M |
| 197 | Refresh subscription | [ ] | M |

---

## PHASE 10: UI Polish (P2-P3)

### 10.1 Art/Icons
**File:** `src/res/art_provider.cpp`

| Line | Icon | Status | Effort |
|------|------|--------|--------|
| 28 | Computed column | [ ] | S |
| 30 | Disconnected database | [ ] | S |
| 57 | Primary+Foreign Key combo | [ ] | S |
| 65 | System index | [ ] | S |
| 69 | System package | [ ] | S |
| 71 | System role | [ ] | S |

---

## Progress Tracker

### Summary

| Phase | Total Items | Complete | In Progress | Blocked | Remaining |
|-------|-------------|----------|-------------|---------|-----------|
| 1. Core Query | 15 | 0 | 0 | 0 | 15 |
| 2. SQL Editor | 15 | 0 | 0 | 0 | 15 |
| 3. Import/Export | 25 | 0 | 0 | 1 | 24 |
| 4. Data Management | 35 | 0 | 0 | 0 | 35 |
| 5. Schema | 8 | 0 | 0 | 0 | 8 |
| 6. Monitoring | 12 | 0 | 0 | 0 | 12 |
| 7. Core Systems | 20 | 0 | 0 | 0 | 20 |
| 8. Backend | 6 | 0 | 0 | 2 | 4 |
| 9. Replication | 3 | 0 | 0 | 0 | 3 |
| 10. UI Polish | 6 | 0 | 0 | 0 | 6 |
| **TOTAL** | **145** | **0** | **0** | **3** | **142** |

---

## Weekly Sprint Planning

### Sprint 1 (Week 1): Core Execution
- [ ] Async Query Executor (all methods)
- [ ] Query Stop/Explain/Explain Analyze

### Sprint 2 (Week 2): SQL Editor
- [ ] Format SQL
- [ ] Comment/Uncomment/Case conversion
- [ ] Color schemes

### Sprint 3 (Week 3): Import/Export
- [ ] CSV Import/Export
- [ ] JSON Import/Export
- [ ] XML/HTML Export

### Sprint 4 (Week 4): Data Management
- [ ] Data Cleansing implementation
- [ ] Team Sharing backend integration
- [ ] Query Comments backend

### Sprint 5 (Week 5): Schema & Monitoring
- [ ] Schema Compare apply changes
- [ ] Monitoring panels (statements, locks, performance)

### Sprint 6 (Week 6): Core Systems
- [ ] Security Manager (encryption, hashing)
- [ ] Query Profiler
- [ ] Migration Manager

### Sprint 7-8 (Weeks 7-8): Backend Integration
- [ ] Real SBLR compilation
- [ ] Server Session Gateway execution
- [ ] Full SBWP integration

---

## Dependencies & Blockers

| Blocker | Blocks | Resolution |
|---------|--------|------------|
| AsyncQueryExecutor | Query stop/cancel | Implement Phase 1.1 |
| ScratchBird libraries | Real SBLR compilation | Build & link libraries |
| QXlsx library | Excel import/export | Add to CMake, build |
| Backend API definition | Team Sharing, Comments | Define REST/gRPC API |

---

## Developer Notes

### Coding Standards
- All new code must have unit tests
- UI changes need screen recording for review
- Database operations must use transactions
- Error handling: show user-friendly messages

### Testing Checklist
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] UI manual testing
- [ ] Performance testing (large datasets)
- [ ] Cross-platform build (Linux, Windows)

### Documentation Updates
- Update UI_ACTIONS.md for new actions
- Update API documentation
- Add user guide sections
- Update CHANGELOG.md

---

## Completion Criteria

This workplan is complete when:
1. All [ ] items are [x]
2. No "stub" or "TODO" comments remain in production code
3. All user-facing features work without "not yet implemented" messages
4. Test coverage > 80% for new code
5. Documentation is updated

**Estimated Total Effort:** ~400 hours (10 weeks @ 40h/week)
