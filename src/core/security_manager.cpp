/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/security_manager.h"

#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace scratchrobin::core {

// Private implementation
struct SecurityManager::Impl {
  std::shared_ptr<Connection> audit_connection;
  SecurityPolicy policy;
  std::vector<AuditLogEntry> audit_log;
  std::vector<SessionSecurityInfo> active_sessions;
  std::vector<DataMaskingRule> masking_rules;
  std::map<std::string, int> failed_login_attempts;
  std::map<std::string, std::chrono::system_clock::time_point> locked_accounts;
  mutable std::mutex mutex;
  
  AuditCallback audit_callback;
  SecurityAlertCallback alert_callback;
};

SecurityManager::SecurityManager()
    : impl_(std::make_unique<Impl>()) {
}

SecurityManager::~SecurityManager() = default;

bool SecurityManager::Initialize(std::shared_ptr<Connection> audit_connection) {
  impl_->audit_connection = audit_connection;
  return true;
}

void SecurityManager::Shutdown() {
  impl_->audit_connection.reset();
}

void SecurityManager::LogEvent(const AuditLogEntry& entry) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->audit_log.push_back(entry);
  
  if (impl_->audit_callback) {
    impl_->audit_callback(entry);
  }
}

void SecurityManager::LogEvent(AuditEventType type,
                               AuditSeverity severity,
                               const std::string& username,
                               const std::string& action,
                               const std::string& details,
                               bool success) {
  AuditLogEntry entry;
  entry.entry_id = "audit_" + std::to_string(impl_->audit_log.size());
  entry.event_type = type;
  entry.severity = severity;
  entry.timestamp = std::chrono::system_clock::now();
  entry.username = username;
  entry.action = action;
  entry.details = details;
  entry.success = success;
  
  LogEvent(entry);
}

std::vector<AuditLogEntry> SecurityManager::GetAuditLog(
    std::chrono::system_clock::time_point from,
    std::chrono::system_clock::time_point to,
    std::optional<AuditEventType> type) const {
  std::vector<AuditLogEntry> result;
  
  for (const auto& entry : impl_->audit_log) {
    if (entry.timestamp >= from && entry.timestamp <= to) {
      if (!type || entry.event_type == *type) {
        result.push_back(entry);
      }
    }
  }
  
  return result;
}

Status SecurityManager::ExportAuditLog(const std::string& file_path,
                                       std::chrono::hours duration) {
  (void)file_path;
  (void)duration;
  return Status::Ok();
}

Status SecurityManager::PurgeAuditLog(std::chrono::months retention) {
  (void)retention;
  return Status::Ok();
}

void SecurityManager::SetPolicy(const SecurityPolicy& policy) {
  impl_->policy = policy;
}

SecurityPolicy SecurityManager::GetPolicy() const {
  return impl_->policy;
}

bool SecurityManager::IsEventAudited(AuditEventType type) const {
  if (impl_->policy.audited_events.empty()) {
    return true;  // Audit all by default
  }
  
  for (const auto& audited : impl_->policy.audited_events) {
    if (audited == type) {
      return true;
    }
  }
  return false;
}

void SecurityManager::RegisterSession(const SessionSecurityInfo& session) {
  impl_->active_sessions.push_back(session);
}

void SecurityManager::UnregisterSession(const std::string& session_id) {
  for (auto it = impl_->active_sessions.begin(); 
       it != impl_->active_sessions.end(); ++it) {
    if (it->session_id == session_id) {
      impl_->active_sessions.erase(it);
      break;
    }
  }
}

void SecurityManager::UpdateSessionActivity(const std::string& session_id) {
  for (auto& session : impl_->active_sessions) {
    if (session.session_id == session_id) {
      session.last_activity = std::chrono::system_clock::now();
      break;
    }
  }
}

std::vector<SessionSecurityInfo> SecurityManager::GetActiveSessions() const {
  return impl_->active_sessions;
}

std::optional<SessionSecurityInfo> SecurityManager::GetSession(
    const std::string& session_id) const {
  for (const auto& session : impl_->active_sessions) {
    if (session.session_id == session_id) {
      return session;
    }
  }
  return std::nullopt;
}

Status SecurityManager::TerminateSession(const std::string& session_id) {
  UnregisterSession(session_id);
  return Status::Ok();
}

Status SecurityManager::TerminateUserSessions(const std::string& username) {
  for (auto it = impl_->active_sessions.begin(); 
       it != impl_->active_sessions.end();) {
    if (it->username == username) {
      it = impl_->active_sessions.erase(it);
    } else {
      ++it;
    }
  }
  return Status::Ok();
}

void SecurityManager::AddMaskingRule(const DataMaskingRule& rule) {
  impl_->masking_rules.push_back(rule);
}

void SecurityManager::RemoveMaskingRule(const std::string& rule_id) {
  for (auto it = impl_->masking_rules.begin(); 
       it != impl_->masking_rules.end(); ++it) {
    if (it->rule_id == rule_id) {
      impl_->masking_rules.erase(it);
      break;
    }
  }
}

std::vector<DataMaskingRule> SecurityManager::GetMaskingRules() const {
  return impl_->masking_rules;
}

std::string SecurityManager::ApplyMasking(const std::string& table,
                                          const std::string& column,
                                          const std::string& value,
                                          const std::string& user_role) const {
  // Check if there's a masking rule for this table/column
  for (const auto& rule : impl_->masking_rules) {
    if (rule.table == table && rule.column == column) {
      // Check if this role is exempt
      if (std::find(rule.exempt_roles.begin(), rule.exempt_roles.end(), user_role) 
          != rule.exempt_roles.end()) {
        return value;  // Role is exempt, return full value
      }
      
      // Apply masking based on rule type
      switch (rule.mask_type) {
        case MaskingRule::MaskType::Full:
          return std::string(value.length(), '*');
        case MaskingRule::MaskType::Partial:
          if (value.length() > 4) {
            return std::string(value.length() - 4, '*') + value.substr(value.length() - 4);
          }
          return std::string(value.length(), '*');
        case MaskingRule::MaskType::Regex:
          // Would apply regex-based masking
          return "[MASKED]";
        case MaskingRule::MaskType::Null:
          return "NULL";
        case MaskingRule::MaskType::Custom:
          return rule.custom_value;
      }
    }
  }
  
  // No masking rule found, return original value
  return value;
}

Status SecurityManager::EncryptData(const std::string& plaintext,
                                    EncryptionAlgorithm algorithm,
                                    const std::string& key,
                                    std::string* ciphertext) {
  (void)plaintext;
  (void)algorithm;
  (void)key;
  (void)ciphertext;
  return Status::Ok();
}

Status SecurityManager::DecryptData(const std::string& ciphertext,
                                    EncryptionAlgorithm algorithm,
                                    const std::string& key,
                                    std::string* plaintext) {
  (void)ciphertext;
  (void)algorithm;
  (void)key;
  (void)plaintext;
  return Status::Ok();
}

std::string SecurityManager::GenerateSecureKey(int length) {
  if (length <= 0 || length > 1024) {
    length = 32; // Default to 256 bits
  }
  
  std::vector<unsigned char> key(length);
  if (RAND_bytes(key.data(), length) != 1) {
    // Fallback to random device if OpenSSL fails
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < length; ++i) {
      key[i] = static_cast<unsigned char>(dis(gen));
    }
  }
  
  // Convert to hex string
  std::ostringstream oss;
  for (auto byte : key) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  return oss.str();
}

std::string SecurityManager::HashPassword(const std::string& password, 
                                          const std::string& salt) {
  if (password.empty()) {
    return "";
  }
  
  // Use PBKDF2 with SHA-256 for password hashing
  const int iterations = 100000;
  const int keylen = 32; // 256 bits
  
  std::vector<unsigned char> out(keylen);
  
  if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                        reinterpret_cast<const unsigned char*>(salt.c_str()), 
                        static_cast<int>(salt.length()),
                        iterations, EVP_sha256(), keylen, out.data()) != 1) {
    return "";
  }
  
  // Format: $pbkdf2-sha256$iterations$salt$hash
  std::ostringstream oss;
  oss << "$pbkdf2-sha256$" << iterations << "$" << salt << "$";
  
  for (auto byte : out) {
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  
  return oss.str();
}

std::string SecurityManager::GenerateSalt() {
  // Generate 16 bytes (128 bits) of random salt
  const int salt_length = 16;
  std::vector<unsigned char> salt(salt_length);
  
  if (RAND_bytes(salt.data(), salt_length) != 1) {
    // Fallback
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (int i = 0; i < salt_length; ++i) {
      salt[i] = static_cast<unsigned char>(dis(gen));
    }
  }
  
  // Encode as base64-like string (url-safe)
  static const char* chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
  std::string result;
  result.reserve(salt_length * 4 / 3 + 4);
  
  for (size_t i = 0; i < salt.size(); i += 3) {
    uint32_t b = (static_cast<uint32_t>(salt[i]) << 16);
    if (i + 1 < salt.size()) b |= (static_cast<uint32_t>(salt[i + 1]) << 8);
    if (i + 2 < salt.size()) b |= static_cast<uint32_t>(salt[i + 2]);
    
    result += chars[(b >> 18) & 0x3F];
    result += chars[(b >> 12) & 0x3F];
    if (i + 1 < salt.size()) result += chars[(b >> 6) & 0x3F];
    if (i + 2 < salt.size()) result += chars[b & 0x3F];
  }
  
  return result;
}

VulnerabilityScan SecurityManager::RunVulnerabilityScan() {
  VulnerabilityScan scan;
  scan.scan_id = "scan_" + std::to_string(
      std::chrono::system_clock::now().time_since_epoch().count());
  scan.scanned_at = std::chrono::system_clock::now();
  return scan;
}

std::vector<std::string> SecurityManager::CheckWeakPasswords() {
  return {};
}

std::vector<std::string> SecurityManager::CheckExcessivePermissions() {
  return {};
}

std::vector<std::string> SecurityManager::CheckUnencryptedConnections() {
  return {};
}

void SecurityManager::RecordFailedLogin(const std::string& username,
                                        const std::string& client_address) {
  (void)client_address;
  impl_->failed_login_attempts[username]++;
}

bool SecurityManager::IsAccountLocked(const std::string& username) const {
  auto it = impl_->locked_accounts.find(username);
  if (it != impl_->locked_accounts.end()) {
    auto now = std::chrono::system_clock::now();
    if (now < it->second) {
      return true;
    }
  }
  return false;
}

bool SecurityManager::ShouldBlockAddress(const std::string& client_address) const {
  (void)client_address;
  return false;
}

void SecurityManager::UnlockAccount(const std::string& username) {
  impl_->locked_accounts.erase(username);
  impl_->failed_login_attempts.erase(username);
}

void SecurityManager::ClearFailedAttempts(const std::string& username) {
  impl_->failed_login_attempts.erase(username);
}

void SecurityManager::SetAuditCallback(AuditCallback callback) {
  impl_->audit_callback = callback;
}

void SecurityManager::SetSecurityAlertCallback(SecurityAlertCallback callback) {
  impl_->alert_callback = callback;
}

Status SecurityManager::GenerateComplianceReport(const std::string& standard,
                                                 std::string* report) {
  (void)standard;
  (void)report;
  return Status::Ok();
}

std::vector<std::string> SecurityManager::GetComplianceViolations(
    const std::string& standard) {
  (void)standard;
  return {};
}

}  // namespace scratchrobin::core
