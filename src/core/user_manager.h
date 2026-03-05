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

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// User privilege types
enum class UserPrivilege {
  kCreate,
  kAlter,
  kDrop,
  kSelect,
  kInsert,
  kUpdate,
  kDelete,
  kExecute,
  kReferences,
  kAll
};

// User role definition
struct UserRole {
  std::string role_name;
  std::string description;
  std::vector<UserPrivilege> privileges;
  bool is_system_role{false};
};

// Database user information
struct UserInfo {
  std::string username;
  std::string full_name;
  std::string description;
  bool is_active{true};
  bool is_admin{false};
  std::vector<std::string> roles;
  std::map<std::string, std::string> attributes;
  std::string created_at;
  std::string last_login;
};

// User creation parameters
struct UserCreateParams {
  std::string username;
  std::string password;
  std::string full_name;
  std::string description;
  bool is_admin{false};
  std::vector<std::string> initial_roles;
  std::map<std::string, std::string> attributes;
  bool must_change_password{false};
};

// Password policy
struct PasswordPolicy {
  int min_length{8};
  int max_length{128};
  bool require_uppercase{true};
  bool require_lowercase{true};
  bool require_digits{true};
  bool require_special{false};
  int expiration_days{90};
  int history_count{5};
  int max_failed_attempts{5};
  int lockout_duration_minutes{30};
};

// ============================================================================
// UserManager
// ============================================================================

class UserManager {
 public:
  UserManager();
  ~UserManager();

  // Initialize with database connection
  bool Initialize(std::shared_ptr<Connection> connection);
  void Shutdown();

  // User CRUD operations
  Status CreateUser(const UserCreateParams& params);
  Status AlterUser(const std::string& username,
                   const std::map<std::string, std::string>& changes);
  Status DropUser(const std::string& username, bool cascade = false);
  
  // User retrieval
  std::optional<UserInfo> GetUser(const std::string& username) const;
  std::vector<UserInfo> ListUsers() const;
  std::vector<UserInfo> ListUsersByRole(const std::string& role_name) const;
  
  // Password management
  Status ChangePassword(const std::string& username,
                        const std::string& old_password,
                        const std::string& new_password);
  Status ResetPassword(const std::string& username,
                       const std::string& new_password);
  Status SetPasswordExpiry(const std::string& username, int days);
  bool ValidatePassword(const std::string& password) const;
  
  // User activation/deactivation
  Status ActivateUser(const std::string& username);
  Status DeactivateUser(const std::string& username);
  Status LockUser(const std::string& username, const std::string& reason);
  Status UnlockUser(const std::string& username);
  
  // Role management
  Status GrantRole(const std::string& username, const std::string& role_name);
  Status RevokeRole(const std::string& username, const std::string& role_name);
  std::vector<UserRole> ListRoles() const;
  std::optional<UserRole> GetRole(const std::string& role_name) const;
  
  // Privilege management
  Status GrantPrivilege(const std::string& username,
                        UserPrivilege privilege,
                        const std::string& object_name = "");
  Status RevokePrivilege(const std::string& username,
                         UserPrivilege privilege,
                         const std::string& object_name = "");
  std::vector<std::pair<UserPrivilege, std::string>> 
      GetUserPrivileges(const std::string& username) const;
  
  // Password policy
  void SetPasswordPolicy(const PasswordPolicy& policy);
  PasswordPolicy GetPasswordPolicy() const;
  
  // User exists check
  bool UserExists(const std::string& username) const;
  
  // Session management
  Status TerminateUserSessions(const std::string& username);
  
  // Change tracking
  using ChangeCallback = std::function<void(const std::string& username,
                                            const std::string& operation)>;
  void SetChangeCallback(ChangeCallback callback);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
