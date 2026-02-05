# ScratchRobin Error Handling Specification

**Status**: Draft  
**Created**: 2026-02-03  
**Last Updated**: 2026-02-03  
**Scope**: Error classification, display, and logging

---

## 1. Overview

This specification defines error handling behavior in ScratchRobin, including error classification, user-facing messages, error dialogs, and logging infrastructure.

## 2. Error Classification

### 2.1 Error Categories

| Category | Description | Examples |
|----------|-------------|----------|
| **Connection** | Network/auth issues | Connection refused, timeout, auth failure |
| **Query** | SQL execution errors | Syntax error, constraint violation, deadlock |
| **Transaction** | Transaction failures | Deadlock, timeout, constraint in transaction |
| **Metadata** | Catalog access errors | Permission denied, object not found |
| **System** | Internal/resource errors | Out of memory, disk full, internal error |
| **Configuration** | Setup issues | Invalid config, missing drivers |

### 2.2 Severity Levels

| Level | Icon | Description | User Action |
|-------|------|-------------|-------------|
| **Fatal** | ❌ | Requires disconnect | Restart connection |
| **Error** | ⚠️ | Operation failed | Fix and retry |
| **Warning** | ⚡ | Partial success | Review warnings |
| **Notice** | ℹ️ | Informational | No action needed |

### 2.3 ScratchRobin Error Code Namespace

Format: `SR-XXXX` where:
- `SR` = ScratchRobin
- `XXXX` = 4-digit numeric code

**Code Ranges:**
| Range | Category |
|-------|----------|
| 1000-1099 | Connection errors |
| 1100-1199 | Query/SQL errors |
| 1200-1299 | Transaction errors |
| 1300-1399 | Metadata errors |
| 1400-1499 | System errors |
| 1500-1599 | Configuration errors |

**Specific Codes:**
| Code | Description |
|------|-------------|
| SR-1001 | Connection refused |
| SR-1002 | Connection timeout |
| SR-1003 | Authentication failed |
| SR-1004 | SSL/TLS handshake failed |
| SR-1101 | SQL syntax error |
| SR-1102 | Table not found |
| SR-1103 | Column not found |
| SR-1104 | Constraint violation |
| SR-1105 | Deadlock detected |
| SR-1201 | Transaction aborted |
| SR-1202 | Transaction timeout |
| SR-1301 | Permission denied |
| SR-1401 | Out of memory |
| SR-1402 | Disk full |
| SR-1501 | Invalid configuration |

## 3. Error Dialog Component

### 3.1 Dialog Design

```
+----------------------------------------------------------+
| ⚠️  Query Error                                           |
+----------------------------------------------------------+
|                                                          |
| An error occurred while executing the query:             |
|                                                          |
| ERROR: relation "userss" does not exist                  |
| LINE 1: SELECT * FROM userss                             |
|                             ^                            |
| HINT: Did you mean "users"?                              |
|                                                          |
+----------------------------------------------------------+
| Error Code: SR-1102 (PostgreSQL: 42P01)                  |
+----------------------------------------------------------+
| [Copy to Clipboard]  [View Details]  [OK]               |
+----------------------------------------------------------+
```

### 3.2 Details Expansion

Clicking "View Details" expands to show:
- Full error stack trace
- Backend-specific error code
- Connection info (server, database)
- Timestamp
- Suggested actions

## 4. Backend Error Mapping

### 4.1 PostgreSQL Error Codes

| SQLSTATE | Category | SR Code | Message |
|----------|----------|---------|---------|
| 28xxx | Connection | SR-1003 | Authentication failed |
| 3D000 | Connection | SR-1102 | Database does not exist |
| 42P01 | Query | SR-1102 | Table does not exist |
| 42703 | Query | SR-1103 | Column does not exist |
| 42601 | Query | SR-1101 | Syntax error |
| 23505 | Query | SR-1104 | Unique constraint violation |
| 40P01 | Transaction | SR-1105 | Deadlock detected |
| 25P02 | Transaction | SR-1201 | Transaction aborted |
| 42501 | Metadata | SR-1301 | Permission denied |

### 4.2 MySQL Error Codes

| Error | Category | SR Code | Message |
|-------|----------|---------|---------|
| 1045 | Connection | SR-1003 | Access denied |
| 2003 | Connection | SR-1001 | Connection refused |
| 2013 | Connection | SR-1002 | Lost connection |
| 1064 | Query | SR-1101 | Syntax error |
| 1146 | Query | SR-1102 | Table doesn't exist |
| 1054 | Query | SR-1103 | Unknown column |
| 1062 | Query | SR-1104 | Duplicate entry |
| 1213 | Transaction | SR-1105 | Deadlock |
| 1044 | Metadata | SR-1301 | Access denied for database |

### 4.3 Firebird Error Codes

| Error | Category | SR Code | Message |
|-------|----------|---------|---------|
| 335544721 | Connection | SR-1003 | Authentication failed |
| 335544344 | Connection | SR-1001 | Connection refused |
| 335544569 | Query | SR-1101 | Syntax error |
| 335544580 | Query | SR-1102 | Table not found |
| 335544351 | Query | SR-1104 | Violation of constraint |
| 335544336 | Transaction | SR-1105 | Deadlock |
| 335544352 | Transaction | SR-1201 | Transaction aborted |
| 335544352 | Metadata | SR-1301 | No permission for operation |

## 5. UI Error Display

### 5.1 Inline Error Display

In SQL Editor messages tab:
```
[14:32:15] ERROR: relation "userss" does not exist
           LINE 1: SELECT * FROM userss
                                       ^
           HINT: Did you mean "users"?
           [Error Code: SR-1102]
```

### 5.2 Status Bar Error

Brief error summary in status bar:
```
Query failed: Table not found (Click for details)
```

### 5.3 Toast Notifications

For non-critical errors:
- Slide in from bottom-right
- Auto-dismiss after 5 seconds
- Click to view full error dialog

## 6. Error Logging

### 6.1 Log Format

```
[2026-02-03 14:32:15.234] [ERROR] [Query] SR-1102: Table not found
  Backend: PostgreSQL
  Connection: localhost:5432/testdb
  SQL: SELECT * FROM userss
  Detail: relation "userss" does not exist
  Hint: Did you mean "users"?
  SQLSTATE: 42P01
```

### 6.2 Log Levels

| Level | Description | Default |
|-------|-------------|---------|
| DEBUG | Detailed diagnostics | Off |
| INFO | General operations | On |
| WARN | Non-fatal issues | On |
| ERROR | Operation failures | On |
| FATAL | Critical errors | On |

### 6.3 Log Rotation

- Maximum file size: 10MB
- Maximum files: 5
- Location: 
  - Linux: `~/.local/share/scratchrobin/logs/`
  - macOS: `~/Library/Logs/ScratchRobin/`
  - Windows: `%APPDATA%\ScratchRobin\logs\`

## 7. Implementation

### 7.1 ErrorInfo Structure

```cpp
struct ErrorInfo {
    std::string code;           // SR-XXXX
    ErrorCategory category;
    ErrorSeverity severity;
    std::string message;        // User-friendly message
    std::string detail;         // Detailed description
    std::string hint;           // Suggested fix
    std::string sqlState;       // Backend SQLSTATE
    std::string backendCode;    // Backend-specific code
    std::string backend;        // Which backend (pg/mysql/fb)
    std::string connection;     // Connection info
    std::string sql;            // SQL that caused error
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> stackTrace;
};
```

### 7.2 ErrorDialog Class

```cpp
class ErrorDialog : public wxDialog {
public:
    ErrorDialog(wxWindow* parent, const ErrorInfo& error);
    
    void ShowDetails(bool show);
    void CopyToClipboard();
    
private:
    ErrorInfo error_;
    wxTextCtrl* details_ctrl_ = nullptr;
    bool details_visible_ = false;
};
```

### 7.3 ErrorMapper Class

```cpp
class ErrorMapper {
public:
    static ErrorInfo MapBackendError(const std::string& backend,
                                     const std::string& backendCode,
                                     const std::string& message);
    
    static std::string GetUserMessage(const ErrorInfo& error);
    static std::string GetSuggestedAction(const ErrorInfo& error);
};
```

## 8. User-Facing Messages

### 8.1 Message Templates

| Code | User Message | Suggested Action |
|------|--------------|------------------|
| SR-1001 | Cannot connect to database server | Check that the server is running and accessible |
| SR-1002 | Connection timed out | Check network connectivity and firewall settings |
| SR-1003 | Authentication failed | Verify username and password are correct |
| SR-1101 | SQL syntax error | Review the SQL statement for syntax issues |
| SR-1102 | Table or object not found | Check that the table name is spelled correctly |
| SR-1103 | Column not found | Verify the column name exists in the table |
| SR-1104 | Constraint violation | Ensure the data meets the constraint requirements |
| SR-1105 | Deadlock detected | Retry the transaction; consider locking order |
| SR-1201 | Transaction failed | The transaction was aborted; retry from beginning |
| SR-1301 | Permission denied | Contact your database administrator |
| SR-1401 | System resource error | Free up system resources and retry |
| SR-1501 | Configuration error | Check your configuration settings |

## 9. Testing

### 9.1 Error Scenarios

1. **Connection refused** - Server not running
2. **Authentication failure** - Wrong credentials
3. **SQL syntax error** - Typo in SQL
4. **Table not found** - Wrong table name
5. **Constraint violation** - Duplicate key
6. **Deadlock** - Concurrent transactions
7. **Permission denied** - Insufficient privileges

### 9.2 Verify Error Display

- Error dialog appears for all error categories
- Correct SR code assigned
- Backend code preserved
- Suggested actions relevant
- Details expandable
- Copy to clipboard works

---

**Related Documents:**
- `docs/specifications/core/TRANSACTION_MANAGEMENT.md`
- `docs/planning/MASTER_IMPLEMENTATION_TRACKER.md` - Task 1.3.x
