/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Audit Event Types
// ============================================================================

enum class AuditEventType {
  kLogin,
  kLogout,
  kQueryExecuted,
  kDataModified,
  kSchemaChanged,
  kUserCreated,
  kUserDeleted,
  kPermissionChanged,
  kConfigChanged,
  kBackupCreated,
  kRestorePerformed,
  kExportPerformed,
  kImportPerformed,
  kConnectionOpened,
  kConnectionClosed,
  kError,
  kCustom
};

// ============================================================================
// Audit Event Severity
// ============================================================================

enum class AuditSeverity {
  kInfo,
  kWarning,
  kError,
  kCritical
};

// ============================================================================
// Audit Event
// ============================================================================

struct AuditEvent {
  int64_t id{0};
  AuditEventType event_type{AuditEventType::kCustom};
  AuditSeverity severity{AuditSeverity::kInfo};
  std::chrono::system_clock::time_point timestamp;
  std::string username;
  std::string connection_name;
  std::string database_name;
  std::string client_ip;
  std::string action;
  std::string description;
  std::string sql_statement;
  std::string affected_object;
  int64_t rows_affected{0};
  bool success{true};
  std::string error_message;
  std::string session_id;
  std::string metadata;  // JSON or custom data
};

// ============================================================================
// Audit Filter
// ============================================================================

struct AuditFilter {
  std::optional<AuditEventType> event_type;
  std::optional<AuditSeverity> severity;
  std::optional<std::string> username;
  std::optional<std::string> database_name;
  std::optional<std::chrono::system_clock::time_point> date_from;
  std::optional<std::chrono::system_clock::time_point> date_to;
  std::optional<std::string> search_text;
};

// ============================================================================
// Audit Log Manager
// ============================================================================

class AuditLogManager {
 public:
  static AuditLogManager& Instance();

  // Event logging
  void LogEvent(const AuditEvent& event);
  void LogEvent(AuditEventType type,
                const std::string& action,
                const std::string& description);

  // Convenience methods
  void LogLogin(const std::string& username,
                const std::string& connection_name,
                bool success,
                const std::string& client_ip = "");
  void LogLogout(const std::string& username,
                 const std::string& connection_name);
  void LogQuery(const std::string& username,
                const std::string& database_name,
                const std::string& sql,
                int64_t rows_affected,
                bool success);
  void LogSchemaChange(const std::string& username,
                       const std::string& database_name,
                       const std::string& action,
                       const std::string& object_name);
  void LogError(AuditSeverity severity,
                const std::string& username,
                const std::string& error_message);

  // Query events
  std::vector<AuditEvent> GetEvents(const AuditFilter& filter, int limit = 1000);
  std::vector<AuditEvent> GetRecentEvents(int limit = 100);
  std::optional<AuditEvent> GetEvent(int64_t id);

  // Statistics
  int GetEventCount(const AuditFilter& filter);
  std::vector<std::pair<std::string, int>> GetEventCountsByType(
      const std::chrono::system_clock::time_point& from,
      const std::chrono::system_clock::time_point& to);
  std::vector<std::pair<std::string, int>> GetEventCountsByUser(
      const std::chrono::system_clock::time_point& from,
      const std::chrono::system_clock::time_point& to);

  // Export
  bool ExportToCsv(const std::string& filepath, const AuditFilter& filter);
  bool ExportToJson(const std::string& filepath, const AuditFilter& filter);

  // Maintenance
  int PurgeEventsOlderThan(const std::chrono::system_clock::time_point& cutoff);
  int PurgeEventsByFilter(const AuditFilter& filter);

  // Configuration
  void SetEnabled(bool enabled) { enabled_ = enabled; }
  bool IsEnabled() const { return enabled_; }

  void SetLogSuccessfulQueries(bool log) { log_successful_queries_ = log; }
  bool GetLogSuccessfulQueries() const { return log_successful_queries_; }

  void SetLogFailedQueries(bool log) { log_failed_queries_ = log; }
  bool GetLogFailedQueries() const { return log_failed_queries_; }

  void SetMinSeverity(AuditSeverity severity) { min_severity_ = severity; }
  AuditSeverity GetMinSeverity() const { return min_severity_; }

  void SetRetentionDays(int days) { retention_days_ = days; }
  int GetRetentionDays() const { return retention_days_; }

  // Event callback
  using EventCallback = std::function<void(const AuditEvent&)>;
  void SetEventCallback(EventCallback callback);

  // Persistence
  bool LoadFromFile(const std::string& filepath);
  bool SaveToFile(const std::string& filepath);

 private:
  AuditLogManager();
  ~AuditLogManager() = default;

  AuditLogManager(const AuditLogManager&) = delete;
  AuditLogManager& operator=(const AuditLogManager&) = delete;

  bool enabled_{true};
  bool log_successful_queries_{true};
  bool log_failed_queries_{true};
  AuditSeverity min_severity_{AuditSeverity::kInfo};
  int retention_days_{90};
  int64_t next_id_{1};

  std::vector<AuditEvent> events_;
  std::string current_filepath_;
  EventCallback event_callback_;

  void WriteEventToLog(const AuditEvent& event);
  std::string EventTypeToString(AuditEventType type);
  std::string SeverityToString(AuditSeverity severity);
  bool MatchesFilter(const AuditEvent& event, const AuditFilter& filter);
  bool ShouldLogEvent(const AuditEvent& event);
};

}  // namespace scratchrobin::core
