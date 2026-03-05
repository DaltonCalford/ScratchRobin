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
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Security audit event types
enum class AuditEventType {
  kLoginAttempt,
  kLoginSuccess,
  kLoginFailure,
  kLogout,
  kQueryExecuted,
  kSchemaModified,
  kDataModified,
  kPermissionChanged,
  kUserCreated,
  kUserDeleted,
  kPasswordChanged,
  kBackupCreated,
  kRestorePerformed,
  kConfigurationChanged,
  kSuspiciousActivity
};

// Audit severity levels
enum class AuditSeverity {
  kInfo,
  kWarning,
  kError,
  kCritical
};

// Encryption algorithms
enum class EncryptionAlgorithm {
  kAES128,
  kAES256,
  kChaCha20
};

// Audit log entry
struct AuditLogEntry {
  std::string entry_id;
  AuditEventType event_type;
  AuditSeverity severity;
  std::chrono::system_clock::time_point timestamp;
  std::string username;
  std::string client_address;
  std::string session_id;
  std::string database_name;
  std::string object_name;
  std::string action;
  std::string details;
  bool success{true};
  std::string error_message;
  std::map<std::string, std::string> metadata;
};

// Security policy
struct SecurityPolicy {
  std::string policy_id;
  std::string policy_name;
  bool audit_enabled{true};
  bool encrypt_connections{true};
  bool require_ssl{false};
  int max_login_attempts{5};
  int lockout_duration_minutes{30};
  int session_timeout_minutes{60};
  int password_expiry_days{90};
  bool require_strong_passwords{true};
  std::vector<AuditEventType> audited_events;
  std::vector<std::string> sensitive_columns;
  std::vector<std::string> restricted_operations;
};

// Session security info
struct SessionSecurityInfo {
  std::string session_id;
  std::string username;
  std::string client_address;
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_activity;
  std::chrono::system_clock::time_point expires_at;
  bool is_ssl{false};
  std::string encryption_cipher;
  std::vector<std::string> permissions;
  int request_count{0};
  int failed_request_count{0};
};

// Data masking rule
struct DataMaskingRule {
  std::string rule_id;
  std::string rule_name;
  std::string table_name;
  std::string column_name;
  std::string mask_pattern;  // e.g., "XXX-XX-$1" or "***"
  std::string condition;     // SQL condition for when to apply
  std::vector<std::string> exempt_roles;
  bool is_active{true};
};

// Vulnerability scan result
struct VulnerabilityScan {
  std::string scan_id;
  std::chrono::system_clock::time_point scanned_at;
  std::vector<std::string> findings;
  std::vector<std::string> recommendations;
  int critical_count{0};
  int high_count{0};
  int medium_count{0};
  int low_count{0};
};

// ============================================================================
// SecurityManager
// ============================================================================

class SecurityManager {
 public:
  using AuditCallback = std::function<void(const AuditLogEntry&)>;
  using SecurityAlertCallback = std::function<void(const std::string& alert_message,
                                                    AuditSeverity severity)>;

  SecurityManager();
  ~SecurityManager();

  // Initialize
  bool Initialize(std::shared_ptr<Connection> audit_connection);
  void Shutdown();
  
  // Audit logging
  void LogEvent(const AuditLogEntry& entry);
  void LogEvent(AuditEventType type,
                AuditSeverity severity,
                const std::string& username,
                const std::string& action,
                const std::string& details,
                bool success = true);
  std::vector<AuditLogEntry> GetAuditLog(
      std::chrono::system_clock::time_point from,
      std::chrono::system_clock::time_point to,
      std::optional<AuditEventType> type = std::nullopt) const;
  Status ExportAuditLog(const std::string& file_path,
                        std::chrono::hours duration = std::chrono::hours(24));
  Status PurgeAuditLog(std::chrono::months retention = std::chrono::months(12));
  
  // Security policies
  void SetPolicy(const SecurityPolicy& policy);
  SecurityPolicy GetPolicy() const;
  bool IsEventAudited(AuditEventType type) const;
  
  // Session management
  void RegisterSession(const SessionSecurityInfo& session);
  void UnregisterSession(const std::string& session_id);
  void UpdateSessionActivity(const std::string& session_id);
  std::vector<SessionSecurityInfo> GetActiveSessions() const;
  std::optional<SessionSecurityInfo> GetSession(const std::string& session_id) const;
  Status TerminateSession(const std::string& session_id);
  Status TerminateUserSessions(const std::string& username);
  
  // Data masking
  void AddMaskingRule(const DataMaskingRule& rule);
  void RemoveMaskingRule(const std::string& rule_id);
  std::vector<DataMaskingRule> GetMaskingRules() const;
  std::string ApplyMasking(const std::string& table,
                           const std::string& column,
                           const std::string& value,
                           const std::string& user_role) const;
  
  // Encryption
  Status EncryptData(const std::string& plaintext,
                     EncryptionAlgorithm algorithm,
                     const std::string& key,
                     std::string* ciphertext);
  Status DecryptData(const std::string& ciphertext,
                     EncryptionAlgorithm algorithm,
                     const std::string& key,
                     std::string* plaintext);
  std::string GenerateSecureKey(int length = 32);
  std::string HashPassword(const std::string& password, const std::string& salt);
  std::string GenerateSalt();
  
  // Security scanning
  VulnerabilityScan RunVulnerabilityScan();
  std::vector<std::string> CheckWeakPasswords();
  std::vector<std::string> CheckExcessivePermissions();
  std::vector<std::string> CheckUnencryptedConnections();
  
  // Intrusion detection
  void RecordFailedLogin(const std::string& username,
                         const std::string& client_address);
  bool IsAccountLocked(const std::string& username) const;
  bool ShouldBlockAddress(const std::string& client_address) const;
  void UnlockAccount(const std::string& username);
  void ClearFailedAttempts(const std::string& username);
  
  // Callbacks
  void SetAuditCallback(AuditCallback callback);
  void SetSecurityAlertCallback(SecurityAlertCallback callback);
  
  // Compliance
  Status GenerateComplianceReport(const std::string& standard,  // e.g., "SOX", "HIPAA", "GDPR"
                                  std::string* report);
  std::vector<std::string> GetComplianceViolations(const std::string& standard);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
