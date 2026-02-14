/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ERROR_HANDLER_H
#define SCRATCHROBIN_ERROR_HANDLER_H

#include <chrono>
#include <string>
#include <vector>

namespace scratchrobin {

// Error categories
enum class ErrorCategory {
    Connection,
    Query,
    Transaction,
    Metadata,
    System,
    Configuration,
    Unknown
};

// Error severity levels
enum class ErrorSeverity {
    Fatal,
    Error,
    Warning,
    Notice
};

// Error information structure
struct ErrorInfo {
    std::string code;                    // SR-XXXX format
    ErrorCategory category = ErrorCategory::Unknown;
    ErrorSeverity severity = ErrorSeverity::Error;
    std::string message;                 // User-friendly message
    std::string detail;                  // Detailed description
    std::string hint;                    // Suggested fix
    std::string sqlState;                // Backend SQLSTATE
    std::string backendCode;             // Backend-specific code
    std::string backend;                 // Which backend (pg/mysql/fb/sb)
    std::string connection;              // Connection info
    std::string sql;                     // SQL that caused error
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> stackTrace;
    
    // Convenience methods
    bool IsRetryable() const;
    std::string GetFullMessage() const;
    std::string ToLogString() const;
};

// Error mapper - converts backend errors to ScratchRobin errors
class ErrorMapper {
public:
    // Map a backend error to ScratchRobin error info
    static ErrorInfo MapBackendError(const std::string& backend,
                                     const std::string& backendCode,
                                     const std::string& sqlState,
                                     const std::string& message,
                                     const std::string& connection = "",
                                     const std::string& sql = "");
    
    // Get user-friendly message for error code
    static std::string GetUserMessage(const std::string& srCode);
    
    // Get suggested action for error
    static std::string GetSuggestedAction(const ErrorInfo& error);
    
    // Check if error is retryable
    static bool IsRetryable(const ErrorInfo& error);

private:
    // Backend-specific mappers
    static ErrorInfo MapPostgreSQLError(const std::string& sqlState,
                                        const std::string& message);
    static ErrorInfo MapMySQLError(const std::string& code,
                                   const std::string& message);
    static ErrorInfo MapFirebirdError(const std::string& code,
                                      const std::string& message);
    static ErrorInfo MapScratchBirdError(const std::string& code,
                                         const std::string& message);
    
    // Generate SR code
    static std::string GenerateSRCode(ErrorCategory category, int subCode);
};

// Error logger
class ErrorLogger {
public:
    static ErrorLogger& Instance();
    
    void Log(const ErrorInfo& error);
    void Log(ErrorSeverity severity, const std::string& message);
    
    void SetLogLevel(ErrorSeverity level);
    void SetLogDirectory(const std::string& path);
    
private:
    ErrorLogger() = default;
    
    std::string GetLogPath() const;
    void RotateLogsIfNeeded();
    std::string SeverityToString(ErrorSeverity severity);
    
    ErrorSeverity log_level_ = ErrorSeverity::Notice;
    std::string log_directory_;
    std::string current_log_file_;
    static constexpr size_t MAX_LOG_SIZE = 10 * 1024 * 1024;  // 10MB
    static constexpr int MAX_LOG_FILES = 5;
};

// Convenience logging macros
#define LOG_DEBUG(msg) scratchrobin::ErrorLogger::Instance().Log(scratchrobin::ErrorSeverity::Notice, msg)
#define LOG_INFO(msg) scratchrobin::ErrorLogger::Instance().Log(scratchrobin::ErrorSeverity::Notice, msg)
#define LOG_WARN(msg) scratchrobin::ErrorLogger::Instance().Log(scratchrobin::ErrorSeverity::Warning, msg)
#define LOG_ERROR(msg) scratchrobin::ErrorLogger::Instance().Log(scratchrobin::ErrorSeverity::Error, msg)
#define LOG_FATAL(msg) scratchrobin::ErrorLogger::Instance().Log(scratchrobin::ErrorSeverity::Fatal, msg)

} // namespace scratchrobin

#endif // SCRATCHROBIN_ERROR_HANDLER_H
