# ScratchRobin Transaction Management Specification

**Status**: Draft  
**Created**: 2026-02-03  
**Last Updated**: 2026-02-03  
**Scope**: SQL Editor transaction behavior and UI indicators

---

## 1. Overview

This specification defines transaction management behavior in ScratchRobin's SQL Editor, including state tracking, visual indicators, isolation levels, and user interaction patterns.

## 2. Transaction States

### 2.1 State Machine

```
                    +-------------+
                    |  No Txn     |
                    | (auto-commit|
                    |   enabled)  |
                    +------+------+
                           |
                Begin Txn  |  Auto-commit off
                           v
                    +-------------+
         +--------->|  Active     |<-----------+
         |          |  Txn        |            |
         |          +------+------+            |
         |                 |                   |
   Rollback               | Commit            |
   or Error               |                   |
         |                 v                   |
         |          +-------------+            |
         +---------+|  No Txn     |------------+
                    | (explicit   |
                    |  commit)    |
                    +-------------+
```

### 2.2 State Definitions

| State | Description | Visual Indicator |
|-------|-------------|------------------|
| **No Transaction** | Auto-commit mode - each statement is its own transaction | Green checkmark or "Auto" text |
| **Transaction Active** | Explicit transaction in progress, statements accumulated | Yellow warning icon or "TX" badge |
| **Transaction Failed** | Error occurred, transaction must be rolled back | Red error icon, requires rollback |
| **Transaction Committed** | Explicit commit successful, returned to auto-commit | Brief green flash, then "Auto" |

## 3. UI Indicators

### 3.1 SQL Editor Status Bar

The status bar at the bottom of the SQL Editor window shall display:

```
+---------------------------------------------------------------+
| [TX: Active] | Server: localhost:3092/testdb | Ready | Row: 0 |
+---------------------------------------------------------------+
```

**Indicator Styles:**

| State | Background | Text | Icon |
|-------|------------|------|------|
| Auto-commit | Gray (#6c757d) | "Auto" | ✅ (green) |
| Transaction Active | Yellow (#ffc107) | "TX Active" | ⚠️ (yellow) |
| Transaction Failed | Red (#dc3545) | "TX Failed - Rollback Required" | ❌ (red) |

### 3.2 Button States

The transaction control buttons shall reflect current state:

| State | Begin | Commit | Rollback |
|-------|-------|--------|----------|
| Auto-commit | Enabled | Disabled | Disabled |
| Transaction Active | Disabled | Enabled | Enabled |
| Transaction Failed | Disabled | Disabled | Enabled (highlighted) |

### 3.3 Window Title

When a transaction is active, append "[TX]" to the window title:

```
SQL Editor - testdb@localhost [TX]
```

## 4. Isolation Levels

### 4.1 Supported Levels

| Level | Description | Supported By |
|-------|-------------|--------------|
| READ UNCOMMITTED | Dirty reads allowed | PostgreSQL, MySQL, Firebird |
| READ COMMITTED | No dirty reads (default) | All backends |
| REPEATABLE READ | Consistent snapshot | PostgreSQL, MySQL, Firebird |
| SERIALIZABLE | Full isolation | All backends |
| SNAPSHOT | Firebird-style snapshot | Firebird, ScratchBird |

### 4.2 Isolation Level Selection

A dropdown in the SQL Editor toolbar allows selection:

```
Isolation: [READ COMMITTED ▼]
```

- Default: READ COMMITTED
- Selection applies to the next BEGIN transaction
- Changing mid-transaction shows warning dialog

## 5. User Interactions

### 5.1 Begin Transaction

**Actions:**
1. User clicks "Begin" button or executes "BEGIN" statement
2. ConnectionManager sets `in_transaction_ = true`
3. Status bar updates to "TX Active"
4. Button states update (Begin disabled, Commit/Rollback enabled)

**Behavior:**
- If auto-commit was on, it's temporarily disabled
- Isolation level from dropdown is applied
- Statements are accumulated until commit/rollback

### 5.2 Commit Transaction

**Actions:**
1. User clicks "Commit" button or executes "COMMIT" statement
2. ConnectionManager executes COMMIT
3. On success:
   - `in_transaction_ = false`
   - Status bar shows brief "Committed" flash
   - Returns to "Auto" state
4. On failure:
   - Stays in "TX Failed" state
   - Error message displayed

### 5.3 Rollback Transaction

**Actions:**
1. User clicks "Rollback" button or executes "ROLLBACK" statement
2. ConnectionManager executes ROLLBACK
3. All changes discarded
4. Returns to "Auto" state

**Special Cases:**
- Rollback can be issued in "TX Failed" state to clear error
- ROLLBACK TO SAVEPOINT supported (see Section 6)

### 5.4 Window Close with Active Transaction

**Behavior:**
1. User attempts to close SQL Editor window
2. If transaction active, show warning dialog:

```
+--------------------------------------------------+
| ⚠️ Uncommitted Transaction                        |
+--------------------------------------------------+
|                                                  |
| You have an active transaction with uncommitted  |
| changes. What would you like to do?              |
|                                                  |
| Transaction started: 2026-02-03 14:32:15         |
| Statements executed: 5                           |
|                                                  |
| [Commit Changes]  [Rollback]  [Cancel Close]     |
+--------------------------------------------------+
```

**Actions:**
- **Commit Changes**: Execute COMMIT, then close
- **Rollback**: Execute ROLLBACK, then close
- **Cancel Close**: Keep window open, transaction remains active

## 6. Savepoint Support

### 6.1 Savepoint Management

**UI Elements:**
- "Create Savepoint" button with name input
- Savepoint list dropdown showing active savepoints
- "Rollback to Savepoint" button
- "Release Savepoint" button

**Usage Pattern:**
```sql
BEGIN;
  INSERT INTO orders ...;
  SAVEPOINT order_created;
  INSERT INTO order_items ...;
  -- If items fail:
  ROLLBACK TO SAVEPOINT order_created;
  -- Try different items
  INSERT INTO order_items ...;
COMMIT;
```

### 6.2 Savepoint Display

A collapsible panel shows active savepoints:

```
Savepoints (2):
  ► order_created
  ► items_validated
```

## 7. Error Handling

### 7.1 Transaction Errors

When an error occurs during a transaction:

1. **Statement Error**: Individual statement fails
   - Transaction remains active
   - User can retry or rollback
   - Error displayed in messages panel

2. **Transaction Abort**: Database aborts transaction
   - State changes to "TX Failed"
   - User must rollback before continuing
   - All statements in transaction are void

### 7.2 Deadlock Detection

If a deadlock is detected:
- Database typically rolls back the conflicting transaction
- ScratchRobin shows specific deadlock error
- User can retry the transaction

## 8. Multi-Statement Behavior

### 8.1 Statement Splitter Integration

When multiple statements are executed:

1. Each statement runs in the same transaction (if active)
2. If one fails:
   - Subsequent statements are skipped
   - Transaction remains active (if supported by backend)
   - User can fix and retry or rollback

### 8.2 Partial Execution

Example:
```sql
-- Statement 1: Success
INSERT INTO logs VALUES ('Start');

-- Statement 2: Fails (constraint violation)
INSERT INTO orders VALUES (invalid_data);

-- Statement 3: Not executed due to earlier failure
INSERT INTO logs VALUES ('Complete');
```

**Result:**
- Statement 1 is in transaction (can be committed or rolled back)
- Statement 2 error displayed
- Statement 3 not executed

## 9. Backend-Specific Behavior

### 9.1 PostgreSQL
- Default: READ COMMITTED
- Supports all standard isolation levels
- DDL statements implicitly commit

### 9.2 MySQL/MariaDB
- Default: REPEATABLE READ (InnoDB)
- DDL statements implicitly commit
- SET TRANSACTION must be called before BEGIN

### 9.3 Firebird
- Default: SNAPSHOT (concurrency)
- SET TRANSACTION syntax different from standard
- Savepoints fully supported

### 9.4 ScratchBird
- Default: READ COMMITTED
- Full savepoint support
- DDL does not implicitly commit (planned)

## 10. Implementation Notes

### 10.1 ConnectionManager Changes

```cpp
class ConnectionManager {
    // Existing
    bool IsInTransaction() const;
    
    // New
    enum class TransactionState {
        AutoCommit,
        Active,
        Failed
    };
    
    TransactionState GetTransactionState() const;
    std::string GetTransactionStartTime() const;
    int GetTransactionStatementCount() const;
    
    // Savepoint support
    bool CreateSavepoint(const std::string& name);
    bool RollbackToSavepoint(const std::string& name);
    bool ReleaseSavepoint(const std::string& name);
    std::vector<std::string> GetActiveSavepoints() const;
};
```

### 10.2 SQL Editor UI Changes

```cpp
class SqlEditorFrame {
    // New UI elements
    wxStaticText* transaction_indicator_;
    wxButton* begin_button_;
    wxButton* commit_button_;
    wxButton* rollback_button_;
    wxChoice* isolation_choice_;
    
    // Event handlers
    void OnBeginTransaction(wxCommandEvent& event);
    void OnCommit(wxCommandEvent& event);
    void OnRollback(wxCommandEvent& event);
    void OnIsolationLevelChanged(wxCommandEvent& event);
    
    // Update UI based on state
    void UpdateTransactionUI();
    bool ConfirmCloseWithTransaction();
};
```

## 11. Testing Scenarios

### 11.1 Basic Transaction Flow
1. Begin transaction
2. Execute INSERT
3. Verify data not visible in other session
4. Commit
5. Verify data now visible

### 11.2 Rollback Test
1. Begin transaction
2. Execute INSERT
3. Rollback
4. Verify data not present

### 11.3 Window Close Warning
1. Begin transaction
2. Attempt to close window
3. Verify warning dialog appears
4. Test all three options (Commit, Rollback, Cancel)

### 11.4 Error Handling
1. Begin transaction
2. Execute invalid statement
3. Verify error shown, transaction still active
4. Execute valid statement
5. Commit - verify only valid statement persisted

---

## Appendix A: SQL Commands Reference

| Command | Description |
|---------|-------------|
| `BEGIN` or `START TRANSACTION` | Start explicit transaction |
| `COMMIT` | Commit transaction |
| `ROLLBACK` | Rollback transaction |
| `SAVEPOINT name` | Create savepoint |
| `ROLLBACK TO SAVEPOINT name` | Rollback to savepoint |
| `RELEASE SAVEPOINT name` | Release savepoint |
| `SET TRANSACTION ISOLATION LEVEL level` | Set isolation level |

---

**Related Documents:**
- `docs/specifications/ui/UI_WINDOW_MODEL.md`
- `docs/specifications/core/ERROR_HANDLING.md` (to be created)
- `docs/planning/MASTER_IMPLEMENTATION_TRACKER.md` - Task 1.2.x
