/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/user_manager.h"

namespace scratchrobin::core {

// Private implementation
struct UserManager::Impl {
  std::shared_ptr<Connection> connection;
  PasswordPolicy password_policy;
  ChangeCallback change_callback;
};

UserManager::UserManager()
    : impl_(std::make_unique<Impl>()) {
}

UserManager::~UserManager() = default;

bool UserManager::Initialize(std::shared_ptr<Connection> connection) {
  impl_->connection = connection;
  return true;
}

void UserManager::Shutdown() {
  impl_->connection.reset();
}

Status UserManager::CreateUser(const UserCreateParams& params) {
  if (impl_->change_callback) {
    impl_->change_callback(params.username, "CREATE");
  }
  return Status::Ok();
}

Status UserManager::AlterUser(const std::string& username,
                              const std::map<std::string, std::string>& changes) {
  (void)changes;
  if (impl_->change_callback) {
    impl_->change_callback(username, "ALTER");
  }
  return Status::Ok();
}

Status UserManager::DropUser(const std::string& username, bool cascade) {
  (void)cascade;
  if (impl_->change_callback) {
    impl_->change_callback(username, "DROP");
  }
  return Status::Ok();
}

std::optional<UserInfo> UserManager::GetUser(const std::string& username) const {
  (void)username;
  return std::nullopt;
}

std::vector<UserInfo> UserManager::ListUsers() const {
  return {};
}

std::vector<UserInfo> UserManager::ListUsersByRole(const std::string& role_name) const {
  (void)role_name;
  return {};
}

Status UserManager::ChangePassword(const std::string& username,
                                   const std::string& old_password,
                                   const std::string& new_password) {
  (void)username;
  (void)old_password;
  (void)new_password;
  return Status::Ok();
}

Status UserManager::ResetPassword(const std::string& username,
                                  const std::string& new_password) {
  (void)username;
  (void)new_password;
  return Status::Ok();
}

Status UserManager::SetPasswordExpiry(const std::string& username, int days) {
  (void)username;
  (void)days;
  return Status::Ok();
}

bool UserManager::ValidatePassword(const std::string& password) const {
  (void)password;
  return true;
}

Status UserManager::ActivateUser(const std::string& username) {
  (void)username;
  return Status::Ok();
}

Status UserManager::DeactivateUser(const std::string& username) {
  (void)username;
  return Status::Ok();
}

Status UserManager::LockUser(const std::string& username, const std::string& reason) {
  (void)username;
  (void)reason;
  return Status::Ok();
}

Status UserManager::UnlockUser(const std::string& username) {
  (void)username;
  return Status::Ok();
}

Status UserManager::GrantRole(const std::string& username, const std::string& role_name) {
  (void)username;
  (void)role_name;
  return Status::Ok();
}

Status UserManager::RevokeRole(const std::string& username, const std::string& role_name) {
  (void)username;
  (void)role_name;
  return Status::Ok();
}

std::vector<UserRole> UserManager::ListRoles() const {
  return {};
}

std::optional<UserRole> UserManager::GetRole(const std::string& role_name) const {
  (void)role_name;
  return std::nullopt;
}

Status UserManager::GrantPrivilege(const std::string& username,
                                   UserPrivilege privilege,
                                   const std::string& object_name) {
  (void)username;
  (void)privilege;
  (void)object_name;
  return Status::Ok();
}

Status UserManager::RevokePrivilege(const std::string& username,
                                    UserPrivilege privilege,
                                    const std::string& object_name) {
  (void)username;
  (void)privilege;
  (void)object_name;
  return Status::Ok();
}

std::vector<std::pair<UserPrivilege, std::string>> 
    UserManager::GetUserPrivileges(const std::string& username) const {
  (void)username;
  return {};
}

void UserManager::SetPasswordPolicy(const PasswordPolicy& policy) {
  impl_->password_policy = policy;
}

PasswordPolicy UserManager::GetPasswordPolicy() const {
  return impl_->password_policy;
}

bool UserManager::UserExists(const std::string& username) const {
  (void)username;
  return false;
}

Status UserManager::TerminateUserSessions(const std::string& username) {
  (void)username;
  return Status::Ok();
}

void UserManager::SetChangeCallback(ChangeCallback callback) {
  impl_->change_callback = callback;
}

}  // namespace scratchrobin::core
