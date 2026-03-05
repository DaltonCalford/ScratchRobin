/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/audit_log_manager.h"

#include <algorithm>
#include <fstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

AuditLogManager& AuditLogManager::Instance() {
  static AuditLogManager instance;
  return instance;
}

AuditLogManager::AuditLogManager() = default;

// ============================================================================
// Event Logging
// ============================================================================

void AuditLogManager::LogEvent(const AuditEvent& event) {
  if (!enabled_) {
    return;
  }
  
  if (!ShouldLogEvent(event)) {
    return;
  }
  
  AuditEvent new_event = event;
  new_event.id = next_id_++;
  if (new_event.timestamp == std::chrono::system_clock::time_point{}) {
    new_event.timestamp = std::chrono::system_clock::now();
  }
  
  events_.push_back(new_event);
  WriteEventToLog(new_event);
  
  if (event_callback_) {
    event_callback_(new_event);
  }
}

void AuditLogManager::LogEvent(AuditEventType type,
                               const std::string& action,
                               const std::string& description) {
  AuditEvent event;
  event.event_type = type;
  event.action = action;
  event.description = description;
  LogEvent(event);
}

// ============================================================================
// Convenience Methods
// ============================================================================

void AuditLogManager::LogLogin(const std::string& username,
                               const std::string& connection_name,
                               bool success,
                               const std::string& client_ip) {
  AuditEvent event;
  event.event_type = AuditEventType::kLogin;
  event.username = username;
  event.connection_name = connection_name;
  event.success = success;
  event.client_ip = client_ip;
  event.action = "Login";
  event.description = success ? "User logged in successfully" : "Login failed";
  LogEvent(event);
}

void AuditLogManager::LogLogout(const std::string& username,
                                const std::string& connection_name) {
  AuditEvent event;
  event.event_type = AuditEventType::kLogout;
  event.username = username;
  event.connection_name = connection_name;
  event.action = "Logout";
  event.description = "User logged out";
  LogEvent(event);
}

void AuditLogManager::LogQuery(const std::string& username,
                               const std::string& database_name,
                               const std::string& sql,
                               int64_t rows_affected,
                               bool success) {
  AuditEvent event;
  event.event_type = AuditEventType::kQueryExecuted;
  event.username = username;
  event.database_name = database_name;
  event.sql_statement = sql;
  event.rows_affected = rows_affected;
  event.success = success;
  event.action = "Query Executed";
  LogEvent(event);
}

void AuditLogManager::LogSchemaChange(const std::string& username,
                                      const std::string& database_name,
                                      const std::string& action,
                                      const std::string& object_name) {
  AuditEvent event;
  event.event_type = AuditEventType::kSchemaChanged;
  event.username = username;
  event.database_name = database_name;
  event.action = action;
  event.affected_object = object_name;
  event.description = "Schema change: " + action + " on " + object_name;
  LogEvent(event);
}

void AuditLogManager::LogError(AuditSeverity severity,
                               const std::string& username,
                               const std::string& error_message) {
  AuditEvent event;
  event.event_type = AuditEventType::kError;
  event.severity = severity;
  event.username = username;
  event.error_message = error_message;
  event.success = false;
  event.action = "Error";
  event.description = error_message;
  LogEvent(event);
}

// ============================================================================
// Query Events
// ============================================================================

std::vector<AuditEvent> AuditLogManager::GetEvents(const AuditFilter& filter,
                                                   int limit) {
  std::vector<AuditEvent> result;
  int count = 0;
  for (auto it = events_.rbegin(); it != events_.rend() && count < limit; ++it) {
    if (MatchesFilter(*it, filter)) {
      result.push_back(*it);
      ++count;
    }
  }
  return result;
}

std::vector<AuditEvent> AuditLogManager::GetRecentEvents(int limit) {
  std::vector<AuditEvent> result;
  int count = 0;
  for (auto it = events_.rbegin(); it != events_.rend() && count < limit; ++it, ++count) {
    result.push_back(*it);
  }
  return result;
}

std::optional<AuditEvent> AuditLogManager::GetEvent(int64_t id) {
  for (const auto& event : events_) {
    if (event.id == id) {
      return event;
    }
  }
  return std::nullopt;
}

// ============================================================================
// Statistics
// ============================================================================

int AuditLogManager::GetEventCount(const AuditFilter& filter) {
  return std::count_if(events_.begin(), events_.end(),
                       [&filter, this](const AuditEvent& e) {
                         return MatchesFilter(e, filter);
                       });
}

std::vector<std::pair<std::string, int>> AuditLogManager::GetEventCountsByType(
    const std::chrono::system_clock::time_point& from,
    const std::chrono::system_clock::time_point& to) {
  std::vector<std::pair<std::string, int>> counts;
  // Implementation would count events by type within time range
  return counts;
}

std::vector<std::pair<std::string, int>> AuditLogManager::GetEventCountsByUser(
    const std::chrono::system_clock::time_point& from,
    const std::chrono::system_clock::time_point& to) {
  std::vector<std::pair<std::string, int>> counts;
  // Implementation would count events by user within time range
  return counts;
}

// ============================================================================
// Export
// ============================================================================

bool AuditLogManager::ExportToCsv(const std::string& filepath,
                                  const AuditFilter& filter) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  // Write header
  file << "ID,Type,Severity,Timestamp,Username,Database,Action,Description\n";
  
  // Write filtered events
  auto events = GetEvents(filter, 10000);
  for (const auto& event : events) {
    file << event.id << ","
         << EventTypeToString(event.event_type) << ","
         << SeverityToString(event.severity) << ","
         << ","
         << event.username << ","
         << event.database_name << ","
         << event.action << ","
         << event.description << "\n";
  }
  
  return true;
}

bool AuditLogManager::ExportToJson(const std::string& filepath,
                                   const AuditFilter& filter) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  // Implementation would write JSON format
  file << "[]";
  return true;
}

// ============================================================================
// Maintenance
// ============================================================================

int AuditLogManager::PurgeEventsOlderThan(
    const std::chrono::system_clock::time_point& cutoff) {
  auto it = std::remove_if(events_.begin(), events_.end(),
                           [&cutoff](const AuditEvent& e) {
                             return e.timestamp < cutoff;
                           });
  int count = static_cast<int>(std::distance(it, events_.end()));
  events_.erase(it, events_.end());
  return count;
}

int AuditLogManager::PurgeEventsByFilter(const AuditFilter& filter) {
  auto it = std::remove_if(events_.begin(), events_.end(),
                           [&filter, this](const AuditEvent& e) {
                             return MatchesFilter(e, filter);
                           });
  int count = static_cast<int>(std::distance(it, events_.end()));
  events_.erase(it, events_.end());
  return count;
}

// ============================================================================
// Event Callback
// ============================================================================

void AuditLogManager::SetEventCallback(EventCallback callback) {
  event_callback_ = callback;
}

// ============================================================================
// Persistence
// ============================================================================

bool AuditLogManager::LoadFromFile(const std::string& filepath) {
  // Implementation would load from JSON or binary format
  current_filepath_ = filepath;
  return true;
}

bool AuditLogManager::SaveToFile(const std::string& filepath) {
  // Implementation would save to JSON or binary format
  current_filepath_ = filepath;
  return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void AuditLogManager::WriteEventToLog(const AuditEvent& event) {
  // Implementation would write to persistent log
}

std::string AuditLogManager::EventTypeToString(AuditEventType type) {
  switch (type) {
    case AuditEventType::kLogin: return "LOGIN";
    case AuditEventType::kLogout: return "LOGOUT";
    case AuditEventType::kQueryExecuted: return "QUERY";
    case AuditEventType::kDataModified: return "DATA_MODIFIED";
    case AuditEventType::kSchemaChanged: return "SCHEMA_CHANGED";
    case AuditEventType::kUserCreated: return "USER_CREATED";
    case AuditEventType::kUserDeleted: return "USER_DELETED";
    case AuditEventType::kPermissionChanged: return "PERMISSION_CHANGED";
    case AuditEventType::kConfigChanged: return "CONFIG_CHANGED";
    case AuditEventType::kBackupCreated: return "BACKUP_CREATED";
    case AuditEventType::kRestorePerformed: return "RESTORE_PERFORMED";
    case AuditEventType::kExportPerformed: return "EXPORT_PERFORMED";
    case AuditEventType::kImportPerformed: return "IMPORT_PERFORMED";
    case AuditEventType::kConnectionOpened: return "CONNECTION_OPENED";
    case AuditEventType::kConnectionClosed: return "CONNECTION_CLOSED";
    case AuditEventType::kError: return "ERROR";
    case AuditEventType::kCustom: return "CUSTOM";
    default: return "UNKNOWN";
  }
}

std::string AuditLogManager::SeverityToString(AuditSeverity severity) {
  switch (severity) {
    case AuditSeverity::kInfo: return "INFO";
    case AuditSeverity::kWarning: return "WARNING";
    case AuditSeverity::kError: return "ERROR";
    case AuditSeverity::kCritical: return "CRITICAL";
    default: return "UNKNOWN";
  }
}

bool AuditLogManager::MatchesFilter(const AuditEvent& event,
                                    const AuditFilter& filter) {
  if (filter.event_type.has_value() &&
      event.event_type != filter.event_type.value()) {
    return false;
  }
  if (filter.severity.has_value() &&
      event.severity != filter.severity.value()) {
    return false;
  }
  if (filter.username.has_value() &&
      event.username != filter.username.value()) {
    return false;
  }
  if (filter.database_name.has_value() &&
      event.database_name != filter.database_name.value()) {
    return false;
  }
  if (filter.search_text.has_value() &&
      event.description.find(filter.search_text.value()) == std::string::npos) {
    return false;
  }
  return true;
}

bool AuditLogManager::ShouldLogEvent(const AuditEvent& event) {
  // Check minimum severity
  if (static_cast<int>(event.severity) < static_cast<int>(min_severity_)) {
    return false;
  }
  
  // Check query logging settings
  if (event.event_type == AuditEventType::kQueryExecuted) {
    if (event.success && !log_successful_queries_) {
      return false;
    }
    if (!event.success && !log_failed_queries_) {
      return false;
    }
  }
  
  return true;
}

}  // namespace scratchrobin::core
