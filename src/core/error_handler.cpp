/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "error_handler.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace scratchrobin {

namespace {

std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string FormatTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

} // namespace

// =============================================================================
// ErrorInfo
// =============================================================================

bool ErrorInfo::IsRetryable() const {
    // Connection errors are often retryable
    if (category == ErrorCategory::Connection) {
        return code == "SR-1001" ||  // Connection refused
               code == "SR-1002" ||  // Timeout
               code == "SR-1401";    // Resource error
    }
    // Deadlocks are retryable
    if (code == "SR-1105") return true;
    
    return false;
}

std::string ErrorInfo::GetFullMessage() const {
    std::ostringstream oss;
    oss << message;
    if (!detail.empty()) {
        oss << "\n\n" << detail;
    }
    if (!hint.empty()) {
        oss << "\n\nHint: " << hint;
    }
    return oss.str();
}

std::string ErrorInfo::ToLogString() const {
    std::ostringstream oss;
    oss << "[" << FormatTimestamp(timestamp) << "] ";
    
    switch (severity) {
        case ErrorSeverity::Fatal: oss << "[FATAL]"; break;
        case ErrorSeverity::Error: oss << "[ERROR]"; break;
        case ErrorSeverity::Warning: oss << "[WARN]"; break;
        case ErrorSeverity::Notice: oss << "[INFO]"; break;
    }
    
    oss << " [";
    switch (category) {
        case ErrorCategory::Connection: oss << "Connection"; break;
        case ErrorCategory::Query: oss << "Query"; break;
        case ErrorCategory::Transaction: oss << "Transaction"; break;
        case ErrorCategory::Metadata: oss << "Metadata"; break;
        case ErrorCategory::System: oss << "System"; break;
        case ErrorCategory::Configuration: oss << "Config"; break;
        default: oss << "Unknown"; break;
    }
    oss << "] ";
    
    oss << code << ": " << message;
    
    if (!backend.empty()) {
        oss << "\n  Backend: " << backend;
    }
    if (!connection.empty()) {
        oss << "\n  Connection: " << connection;
    }
    if (!sqlState.empty()) {
        oss << "\n  SQLSTATE: " << sqlState;
    }
    if (!backendCode.empty()) {
        oss << "\n  Backend Code: " << backendCode;
    }
    if (!sql.empty()) {
        oss << "\n  SQL: " << sql;
    }
    if (!detail.empty()) {
        oss << "\n  Detail: " << detail;
    }
    if (!hint.empty()) {
        oss << "\n  Hint: " << hint;
    }
    
    return oss.str();
}

// =============================================================================
// ErrorMapper
// =============================================================================

ErrorInfo ErrorMapper::MapBackendError(const std::string& backend,
                                       const std::string& backendCode,
                                       const std::string& sqlState,
                                       const std::string& message,
                                       const std::string& connection,
                                       const std::string& sql) {
    ErrorInfo error;
    error.backend = backend;
    error.backendCode = backendCode;
    error.sqlState = sqlState;
    error.message = message;
    error.connection = connection;
    error.sql = sql;
    error.timestamp = std::chrono::system_clock::now();
    
    std::string backendLower = ToLower(backend);
    
    if (backendLower == "postgresql" || backendLower == "postgres") {
        error = MapPostgreSQLError(sqlState, message);
    } else if (backendLower == "mysql" || backendLower == "mariadb") {
        error = MapMySQLError(backendCode, message);
    } else if (backendLower == "firebird" || backendLower == "fb") {
        error = MapFirebirdError(backendCode, message);
    } else if (backendLower == "native" || backendLower == "scratchbird") {
        error = MapScratchBirdError(backendCode, message);
    } else {
        // Generic error mapping
        error.code = "SR-0000";
        error.category = ErrorCategory::Unknown;
        error.severity = ErrorSeverity::Error;
    }
    
    // Preserve original info
    error.backend = backend;
    error.backendCode = backendCode;
    error.sqlState = sqlState;
    error.connection = connection;
    error.sql = sql;
    
    return error;
}

ErrorInfo ErrorMapper::MapPostgreSQLError(const std::string& sqlState,
                                          const std::string& message) {
    ErrorInfo error;
    error.sqlState = sqlState;
    error.backend = "PostgreSQL";
    
    // Class 28 - Connection errors
    if (sqlState.substr(0, 2) == "28") {
        error.category = ErrorCategory::Connection;
        error.code = "SR-1003";
        error.severity = ErrorSeverity::Error;
        error.message = "Authentication failed";
        error.hint = "Verify your username and password are correct.";
    }
    // Class 08 - Connection exceptions
    else if (sqlState.substr(0, 2) == "08") {
        error.category = ErrorCategory::Connection;
        if (sqlState == "08006") {
            error.code = "SR-1002";
            error.message = "Connection timeout";
            error.hint = "Check network connectivity and firewall settings.";
        } else {
            error.code = "SR-1001";
            error.message = "Connection error";
            error.hint = "Check that the database server is running.";
        }
        error.severity = ErrorSeverity::Error;
    }
    // Class 3D - Catalog errors (database not found)
    else if (sqlState.substr(0, 2) == "3D") {
        error.category = ErrorCategory::Connection;
        error.code = "SR-1102";
        error.severity = ErrorSeverity::Error;
        error.message = "Database does not exist";
        error.hint = "Check that the database name is spelled correctly.";
    }
    // Class 42 - Syntax and access errors
    else if (sqlState == "42601") {
        error.category = ErrorCategory::Query;
        error.code = "SR-1101";
        error.severity = ErrorSeverity::Error;
        error.message = "SQL syntax error";
        error.hint = "Review the SQL statement for syntax issues.";
    }
    else if (sqlState == "42P01") {
        error.category = ErrorCategory::Query;
        error.code = "SR-1102";
        error.severity = ErrorSeverity::Error;
        error.message = "Table or object not found";
        error.hint = "Check that the table name is spelled correctly.";
    }
    else if (sqlState == "42703") {
        error.category = ErrorCategory::Query;
        error.code = "SR-1103";
        error.severity = ErrorSeverity::Error;
        error.message = "Column not found";
        error.hint = "Verify the column name exists in the table.";
    }
    // Class 23 - Integrity constraint violations
    else if (sqlState.substr(0, 2) == "23") {
        error.category = ErrorCategory::Query;
        error.code = "SR-1104";
        error.severity = ErrorSeverity::Error;
        error.message = "Constraint violation";
        error.hint = "Ensure the data meets the constraint requirements.";
    }
    // Class 40 - Transaction rollback
    else if (sqlState == "40P01") {
        error.category = ErrorCategory::Transaction;
        error.code = "SR-1105";
        error.severity = ErrorSeverity::Warning;
        error.message = "Deadlock detected";
        error.hint = "Retry the transaction; consider the order of row locking.";
    }
    else if (sqlState.substr(0, 2) == "25") {
        error.category = ErrorCategory::Transaction;
        error.code = "SR-1201";
        error.severity = ErrorSeverity::Error;
        error.message = "Transaction failed";
        error.hint = "The transaction was aborted. Retry from the beginning.";
    }
    // Class 42 - Permission errors
    else if (sqlState == "42501") {
        error.category = ErrorCategory::Metadata;
        error.code = "SR-1301";
        error.severity = ErrorSeverity::Error;
        error.message = "Permission denied";
        error.hint = "Contact your database administrator for access.";
    }
    // Default
    else {
        error.category = ErrorCategory::Query;
        error.code = "SR-1100";
        error.severity = ErrorSeverity::Error;
        error.message = message;
    }
    
    error.detail = message;
    return error;
}

ErrorInfo ErrorMapper::MapMySQLError(const std::string& code,
                                     const std::string& message) {
    ErrorInfo error;
    error.backendCode = code;
    error.backend = "MySQL";
    
    int errorCode = 0;
    try {
        errorCode = std::stoi(code);
    } catch (...) {
        errorCode = 0;
    }
    
    switch (errorCode) {
        case 1045:  // Access denied
            error.category = ErrorCategory::Connection;
            error.code = "SR-1003";
            error.severity = ErrorSeverity::Error;
            error.message = "Authentication failed";
            error.hint = "Verify your username and password are correct.";
            break;
            
        case 2003:  // Connection refused
        case 2005:  // Unknown server host
            error.category = ErrorCategory::Connection;
            error.code = "SR-1001";
            error.severity = ErrorSeverity::Error;
            error.message = "Cannot connect to database server";
            error.hint = "Check that the server is running and accessible.";
            break;
            
        case 2013:  // Lost connection
            error.category = ErrorCategory::Connection;
            error.code = "SR-1002";
            error.severity = ErrorSeverity::Error;
            error.message = "Connection lost";
            error.hint = "Check network connectivity.";
            break;
            
        case 1064:  // Syntax error
            error.category = ErrorCategory::Query;
            error.code = "SR-1101";
            error.severity = ErrorSeverity::Error;
            error.message = "SQL syntax error";
            error.hint = "Review the SQL statement for syntax issues.";
            break;
            
        case 1146:  // Table doesn't exist
            error.category = ErrorCategory::Query;
            error.code = "SR-1102";
            error.severity = ErrorSeverity::Error;
            error.message = "Table not found";
            error.hint = "Check that the table name is spelled correctly.";
            break;
            
        case 1054:  // Unknown column
            error.category = ErrorCategory::Query;
            error.code = "SR-1103";
            error.severity = ErrorSeverity::Error;
            error.message = "Column not found";
            error.hint = "Verify the column name exists in the table.";
            break;
            
        case 1062:  // Duplicate entry
            error.category = ErrorCategory::Query;
            error.code = "SR-1104";
            error.severity = ErrorSeverity::Error;
            error.message = "Duplicate value";
            error.hint = "A unique constraint was violated. Check for duplicates.";
            break;
            
        case 1213:  // Deadlock
            error.category = ErrorCategory::Transaction;
            error.code = "SR-1105";
            error.severity = ErrorSeverity::Warning;
            error.message = "Deadlock detected";
            error.hint = "Retry the transaction; consider the order of row locking.";
            break;
            
        case 1044:  // Access denied for database
            error.category = ErrorCategory::Metadata;
            error.code = "SR-1301";
            error.severity = ErrorSeverity::Error;
            error.message = "Permission denied";
            error.hint = "Contact your database administrator for access.";
            break;
            
        default:
            error.category = ErrorCategory::Query;
            error.code = "SR-1100";
            error.severity = ErrorSeverity::Error;
            error.message = message;
            break;
    }
    
    error.detail = message;
    return error;
}

ErrorInfo ErrorMapper::MapFirebirdError(const std::string& code,
                                        const std::string& message) {
    ErrorInfo error;
    error.backendCode = code;
    error.backend = "Firebird";
    
    // Firebird uses numeric error codes
    int errorCode = 0;
    try {
        errorCode = std::stoi(code);
    } catch (...) {
        errorCode = 0;
    }
    
    switch (errorCode) {
        case 335544721:  // Authentication failed
            error.category = ErrorCategory::Connection;
            error.code = "SR-1003";
            error.severity = ErrorSeverity::Error;
            error.message = "Authentication failed";
            error.hint = "Verify your username and password are correct.";
            break;
            
        case 335544344:  // Connection refused
            error.category = ErrorCategory::Connection;
            error.code = "SR-1001";
            error.severity = ErrorSeverity::Error;
            error.message = "Cannot connect to database server";
            error.hint = "Check that the server is running.";
            break;
            
        case 335544569:  // Syntax error
            error.category = ErrorCategory::Query;
            error.code = "SR-1101";
            error.severity = ErrorSeverity::Error;
            error.message = "SQL syntax error";
            error.hint = "Review the SQL statement for syntax issues.";
            break;
            
        case 335544580:  // Table not found
            error.category = ErrorCategory::Query;
            error.code = "SR-1102";
            error.severity = ErrorSeverity::Error;
            error.message = "Table not found";
            error.hint = "Check that the table name is spelled correctly.";
            break;
            
        case 335544351:  // Constraint violation
            error.category = ErrorCategory::Query;
            error.code = "SR-1104";
            error.severity = ErrorSeverity::Error;
            error.message = "Constraint violation";
            error.hint = "Ensure the data meets the constraint requirements.";
            break;
            
        case 335544336:  // Deadlock
            error.category = ErrorCategory::Transaction;
            error.code = "SR-1105";
            error.severity = ErrorSeverity::Warning;
            error.message = "Deadlock detected";
            error.hint = "Retry the transaction.";
            break;
            
        case 335544352:  // No permission
            error.category = ErrorCategory::Metadata;
            error.code = "SR-1301";
            error.severity = ErrorSeverity::Error;
            error.message = "Permission denied";
            error.hint = "Contact your database administrator for access.";
            break;
            
        default:
            error.category = ErrorCategory::Query;
            error.code = "SR-1100";
            error.severity = ErrorSeverity::Error;
            error.message = message;
            break;
    }
    
    error.detail = message;
    return error;
}

ErrorInfo ErrorMapper::MapScratchBirdError(const std::string& code,
                                           const std::string& message) {
    ErrorInfo error;
    error.backendCode = code;
    error.backend = "ScratchBird";
    
    // ScratchBird uses similar codes to PostgreSQL
    // For now, use generic mapping
    error.category = ErrorCategory::Query;
    error.code = "SR-1100";
    error.severity = ErrorSeverity::Error;
    error.message = message;
    error.detail = message;
    
    return error;
}

std::string ErrorMapper::GetUserMessage(const std::string& srCode) {
    static const std::unordered_map<std::string, std::string> messages = {
        {"SR-1001", "Cannot connect to database server"},
        {"SR-1002", "Connection timed out"},
        {"SR-1003", "Authentication failed"},
        {"SR-1101", "SQL syntax error"},
        {"SR-1102", "Table or object not found"},
        {"SR-1103", "Column not found"},
        {"SR-1104", "Constraint violation"},
        {"SR-1105", "Deadlock detected"},
        {"SR-1201", "Transaction failed"},
        {"SR-1301", "Permission denied"},
        {"SR-1401", "System resource error"},
        {"SR-1501", "Configuration error"}
    };
    
    auto it = messages.find(srCode);
    if (it != messages.end()) {
        return it->second;
    }
    return "An error occurred";
}

std::string ErrorMapper::GetSuggestedAction(const ErrorInfo& error) {
    if (!error.hint.empty()) {
        return error.hint;
    }
    
    switch (error.category) {
        case ErrorCategory::Connection:
            return "Check your network connection and database server status.";
        case ErrorCategory::Query:
            return "Review your SQL statement and try again.";
        case ErrorCategory::Transaction:
            return "Review the transaction and retry.";
        case ErrorCategory::Metadata:
            return "Check object names and permissions.";
        case ErrorCategory::System:
            return "Check system resources and retry.";
        case ErrorCategory::Configuration:
            return "Review your configuration settings.";
        default:
            return "Contact support if the problem persists.";
    }
}

bool ErrorMapper::IsRetryable(const ErrorInfo& error) {
    return error.IsRetryable();
}

std::string ErrorMapper::GenerateSRCode(ErrorCategory category, int subCode) {
    std::ostringstream oss;
    oss << "SR-";
    
    switch (category) {
        case ErrorCategory::Connection: oss << "10"; break;
        case ErrorCategory::Query: oss << "11"; break;
        case ErrorCategory::Transaction: oss << "12"; break;
        case ErrorCategory::Metadata: oss << "13"; break;
        case ErrorCategory::System: oss << "14"; break;
        case ErrorCategory::Configuration: oss << "15"; break;
        default: oss << "00"; break;
    }
    
    oss << std::setfill('0') << std::setw(2) << subCode;
    return oss.str();
}

// =============================================================================
// ErrorLogger
// =============================================================================

ErrorLogger& ErrorLogger::Instance() {
    static ErrorLogger instance;
    return instance;
}

void ErrorLogger::Log(const ErrorInfo& error) {
    if (static_cast<int>(error.severity) > static_cast<int>(log_level_)) {
        return;
    }
    
    RotateLogsIfNeeded();
    
    std::ofstream logFile(current_log_file_, std::ios::app);
    if (logFile.is_open()) {
        logFile << error.ToLogString() << std::endl;
    }
}

void ErrorLogger::Log(ErrorSeverity severity, const std::string& message) {
    if (static_cast<int>(severity) > static_cast<int>(log_level_)) {
        return;
    }
    
    ErrorInfo error;
    error.severity = severity;
    error.message = message;
    error.timestamp = std::chrono::system_clock::now();
    error.code = "SR-0000";
    error.category = ErrorCategory::Unknown;
    
    Log(error);
}

void ErrorLogger::SetLogLevel(ErrorSeverity level) {
    log_level_ = level;
}

void ErrorLogger::SetLogDirectory(const std::string& path) {
    log_directory_ = path;
    current_log_file_.clear();
}

std::string ErrorLogger::GetLogPath() const {
    if (!current_log_file_.empty()) {
        return current_log_file_;
    }
    
    if (log_directory_.empty()) {
        // Default location
        const char* home = std::getenv("HOME");
        if (home) {
            return std::string(home) + "/.local/share/scratchrobin/logs/scratchrobin.log";
        }
        return "scratchrobin.log";
    }
    
    return log_directory_ + "/scratchrobin.log";
}

void ErrorLogger::RotateLogsIfNeeded() {
    if (current_log_file_.empty()) {
        current_log_file_ = GetLogPath();
    }
    
    // Check file size
    std::ifstream checkFile(current_log_file_, std::ios::binary | std::ios::ate);
    if (checkFile.is_open()) {
        auto size = checkFile.tellg();
        if (size > static_cast<std::streamoff>(MAX_LOG_SIZE)) {
            // Rotate logs
            for (int i = MAX_LOG_FILES - 1; i > 0; --i) {
                std::string oldName = current_log_file_ + "." + std::to_string(i - 1);
                std::string newName = current_log_file_ + "." + std::to_string(i);
                std::rename(oldName.c_str(), newName.c_str());
            }
            std::rename(current_log_file_.c_str(), (current_log_file_ + ".0").c_str());
        }
    }
}

std::string ErrorLogger::SeverityToString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Fatal: return "FATAL";
        case ErrorSeverity::Error: return "ERROR";
        case ErrorSeverity::Warning: return "WARN";
        case ErrorSeverity::Notice: return "INFO";
    }
    return "UNKNOWN";
}

} // namespace scratchrobin
