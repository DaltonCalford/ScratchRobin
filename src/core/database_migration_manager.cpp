/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/database_migration_manager.h"

namespace scratchrobin::core {

// Private implementation
struct DatabaseMigrationManager::Impl {
  std::shared_ptr<Connection> connection;
  MigrationConfig config;
  std::vector<Migration> migrations;
  std::vector<SchemaVersion> history;
  ProgressCallback progress_callback;
  BeforeCallback before_callback;
  AfterCallback after_callback;
};

DatabaseMigrationManager::DatabaseMigrationManager()
    : impl_(std::make_unique<Impl>()) {
}

DatabaseMigrationManager::~DatabaseMigrationManager() = default;

bool DatabaseMigrationManager::Initialize(std::shared_ptr<Connection> connection,
                                          const MigrationConfig& config) {
  impl_->connection = connection;
  impl_->config = config;
  return true;
}

void DatabaseMigrationManager::Shutdown() {
  impl_->connection.reset();
}

Status DatabaseMigrationManager::LoadMigrationsFromDirectory(const std::string& path) {
  (void)path;
  return Status::Ok();
}

Status DatabaseMigrationManager::LoadMigration(const Migration& migration) {
  impl_->migrations.push_back(migration);
  return Status::Ok();
}

std::vector<Migration> DatabaseMigrationManager::GetAvailableMigrations() const {
  return impl_->migrations;
}

std::vector<Migration> DatabaseMigrationManager::GetPendingMigrations() const {
  std::vector<Migration> pending;
  for (const auto& migration : impl_->migrations) {
    bool found = false;
    for (const auto& version : impl_->history) {
      if (version.version == migration.version) {
        found = true;
        break;
      }
    }
    if (!found) {
      pending.push_back(migration);
    }
  }
  return pending;
}

MigrationResult DatabaseMigrationManager::MigrateToLatest() {
  return Migrate(1000);  // Large number to get all pending
}

MigrationResult DatabaseMigrationManager::MigrateToVersion(const std::string& target_version) {
  (void)target_version;
  
  MigrationResult result;
  result.status = Status::Ok();
  
  auto pending = GetPendingMigrations();
  int total = static_cast<int>(pending.size());
  int current = 0;
  
  for (auto& migration : pending) {
    ++current;
    
    if (impl_->progress_callback) {
      impl_->progress_callback(migration, current, total);
    }
    
    if (impl_->before_callback) {
      Status before_status = impl_->before_callback(migration);
      if (!before_status.ok) {
        result.failed_migrations.push_back(migration.id);
        result.errors[migration.id] = before_status.message;
        continue;
      }
    }
    
    // Apply migration (stub)
    migration.status = MigrationStatus::kCompleted;
    result.applied_migrations.push_back(migration.id);
    
    if (impl_->after_callback) {
      impl_->after_callback(migration, Status::Ok());
    }
  }
  
  return result;
}

MigrationResult DatabaseMigrationManager::Migrate(int count) {
  auto pending = GetPendingMigrations();
  if (count > 0 && static_cast<size_t>(count) < pending.size()) {
    pending.resize(count);
  }
  
  // Apply migrations (stub)
  MigrationResult result;
  result.status = Status::Ok();
  return result;
}

MigrationResult DatabaseMigrationManager::RedoLastMigration() {
  MigrationResult result;
  result.status = Status::Ok();
  return result;
}

MigrationResult DatabaseMigrationManager::Rollback(int count) {
  (void)count;
  MigrationResult result;
  result.status = Status::Ok();
  return result;
}

MigrationResult DatabaseMigrationManager::RollbackToVersion(const std::string& version) {
  (void)version;
  MigrationResult result;
  result.status = Status::Ok();
  return result;
}

Status DatabaseMigrationManager::ValidateMigrations() {
  return Status::Ok();
}

Status DatabaseMigrationManager::RepairMigrationHistory() {
  return Status::Ok();
}

Status DatabaseMigrationManager::Baseline(const std::string& version) {
  (void)version;
  return Status::Ok();
}

std::optional<SchemaVersion> DatabaseMigrationManager::GetCurrentVersion() const {
  if (impl_->history.empty()) {
    return std::nullopt;
  }
  return impl_->history.back();
}

std::vector<SchemaVersion> DatabaseMigrationManager::GetMigrationHistory() const {
  return impl_->history;
}

bool DatabaseMigrationManager::IsVersionApplied(const std::string& version) const {
  for (const auto& v : impl_->history) {
    if (v.version == version) {
      return true;
    }
  }
  return false;
}

MigrationDiff DatabaseMigrationManager::CalculateDiff(const std::string& from_version,
                                                      const std::string& to_version) const {
  (void)from_version;
  (void)to_version;
  MigrationDiff diff;
  diff.is_upgrade = true;
  return diff;
}

Status DatabaseMigrationManager::PreviewMigration(const std::string& target_version,
                                                  std::vector<std::string>* scripts) {
  (void)target_version;
  (void)scripts;
  return Status::Ok();
}

bool DatabaseMigrationManager::HasPendingMigrations() const {
  return !GetPendingMigrations().empty();
}

int DatabaseMigrationManager::GetPendingCount() const {
  return static_cast<int>(GetPendingMigrations().size());
}

std::optional<Migration> DatabaseMigrationManager::GetMigration(const std::string& id) const {
  for (const auto& m : impl_->migrations) {
    if (m.id == id) {
      return m;
    }
  }
  return std::nullopt;
}

void DatabaseMigrationManager::SetProgressCallback(ProgressCallback callback) {
  impl_->progress_callback = callback;
}

void DatabaseMigrationManager::SetBeforeMigrateCallback(BeforeCallback callback) {
  impl_->before_callback = callback;
}

void DatabaseMigrationManager::SetAfterMigrateCallback(AfterCallback callback) {
  impl_->after_callback = callback;
}

Status DatabaseMigrationManager::Clean() {
  return Status::Ok();
}

Status DatabaseMigrationManager::CleanSchema(const std::string& schema_name) {
  (void)schema_name;
  return Status::Ok();
}

Status DatabaseMigrationManager::GenerateMigration(const std::string& name,
                                                   MigrationType type,
                                                   const std::string& output_path) {
  (void)name;
  (void)type;
  (void)output_path;
  return Status::Ok();
}

Status DatabaseMigrationManager::GenerateBaseline(const std::string& output_path) {
  (void)output_path;
  return Status::Ok();
}

}  // namespace scratchrobin::core
