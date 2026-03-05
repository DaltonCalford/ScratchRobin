/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/migration_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

MigrationManager& MigrationManager::Instance() {
  static MigrationManager instance;
  return instance;
}

MigrationManager::MigrationManager() = default;

// ============================================================================
// Migration Registration
// ============================================================================

void MigrationManager::RegisterMigration(const MigrationScript& migration) {
  // Check if already exists
  auto it = std::find_if(migrations_.begin(), migrations_.end(),
                         [&migration](const MigrationScript& m) {
                           return m.id == migration.id;
                         });
  if (it != migrations_.end()) {
    *it = migration;
  } else {
    migrations_.push_back(migration);
  }
}

void MigrationManager::RegisterMigrations(
    const std::vector<MigrationScript>& migrations) {
  for (const auto& migration : migrations) {
    RegisterMigration(migration);
  }
}

bool MigrationManager::UnregisterMigration(const std::string& migration_id) {
  auto it = std::remove_if(migrations_.begin(), migrations_.end(),
                           [&migration_id](const MigrationScript& m) {
                             return m.id == migration_id;
                           });
  if (it != migrations_.end()) {
    migrations_.erase(it, migrations_.end());
    return true;
  }
  return false;
}

// ============================================================================
// Get Migrations
// ============================================================================

std::vector<MigrationScript> MigrationManager::GetAllMigrations() {
  return GetOrderedMigrations();
}

std::vector<MigrationScript> MigrationManager::GetPendingMigrations() {
  std::vector<MigrationScript> pending;
  for (const auto& migration : migrations_) {
    if (IsMigrationPending(migration.id)) {
      pending.push_back(migration);
    }
  }
  return GetOrderedMigrations();  // Would filter to only pending
}

std::vector<MigrationScript> MigrationManager::GetExecutedMigrations() {
  std::vector<MigrationScript> executed;
  for (const auto& exec : execution_history_) {
    auto migration = GetMigration(exec.migration_id);
    if (migration.has_value()) {
      executed.push_back(migration.value());
    }
  }
  return executed;
}

std::optional<MigrationScript> MigrationManager::GetMigration(
    const std::string& migration_id) {
  for (const auto& migration : migrations_) {
    if (migration.id == migration_id) {
      return migration;
    }
  }
  return std::nullopt;
}

std::optional<MigrationScript> MigrationManager::GetMigrationByName(
    const std::string& name) {
  for (const auto& migration : migrations_) {
    if (migration.name == name) {
      return migration;
    }
  }
  return std::nullopt;
}

// ============================================================================
// Check Status
// ============================================================================

bool MigrationManager::IsMigrationPending(const std::string& migration_id) {
  return !IsMigrationExecuted(migration_id);
}

bool MigrationManager::IsMigrationExecuted(const std::string& migration_id) {
  for (const auto& exec : execution_history_) {
    if (exec.migration_id == migration_id && 
        exec.status == MigrationStatus::kCompleted) {
      return true;
    }
  }
  return false;
}

MigrationStatus MigrationManager::GetMigrationStatus(
    const std::string& migration_id) {
  for (const auto& exec : execution_history_) {
    if (exec.migration_id == migration_id) {
      return exec.status;
    }
  }
  return MigrationStatus::kPending;
}

// ============================================================================
// Execute Migrations
// ============================================================================

MigrationResult MigrationManager::Migrate(const MigrationOptions& options) {
  MigrationResult result;
  
  auto pending = GetPendingMigrations();
  result.total_scripts = static_cast<int>(pending.size());
  
  for (const auto& migration : pending) {
    auto single_result = MigrateOne(migration.id, options);
    if (single_result.success) {
      result.executed_migrations.push_back(migration.id);
      result.completed_scripts++;
    } else {
      result.failed_migrations.push_back(migration.id);
      result.success = false;
      result.message = single_result.message;
      break;
    }
  }
  
  if (result.failed_migrations.empty()) {
    result.success = true;
    result.message = "All migrations executed successfully";
  }
  
  return result;
}

MigrationResult MigrationManager::MigrateTo(
    const std::string& target_migration_id,
    const MigrationOptions& options) {
  MigrationResult result;
  
  bool target_reached = false;
  auto ordered = GetOrderedMigrations();
  
  for (const auto& migration : ordered) {
    if (migration.id == target_migration_id) {
      target_reached = true;
    }
    
    if (IsMigrationPending(migration.id) && target_reached) {
      auto single_result = MigrateOne(migration.id, options);
      if (single_result.success) {
        result.executed_migrations.push_back(migration.id);
        result.completed_scripts++;
      } else {
        result.failed_migrations.push_back(migration.id);
        break;
      }
    }
  }
  
  result.total_scripts = result.completed_scripts + 
                         static_cast<int>(result.failed_migrations.size());
  result.success = result.failed_migrations.empty();
  result.message = result.success ? "Migrated to target successfully" : 
                   "Migration to target failed";
  
  return result;
}

MigrationResult MigrationManager::MigrateOne(
    const std::string& migration_id,
    const MigrationOptions& options) {
  MigrationResult result;
  
  auto migration = GetMigration(migration_id);
  if (!migration.has_value()) {
    result.success = false;
    result.message = "Migration not found: " + migration_id;
    return result;
  }
  
  if (!options.force && IsMigrationExecuted(migration_id)) {
    result.success = true;
    result.message = "Migration already executed";
    return result;
  }
  
  // Check dependencies
  for (const auto& dep_id : migration->dependencies) {
    if (!IsMigrationExecuted(dep_id)) {
      result.success = false;
      result.message = "Dependency not met: " + dep_id;
      return result;
    }
  }
  
  // Validate checksum
  if (options.validate_checksums) {
    auto status = ValidateMigration(migration.value());
    if (!status.ok) {
      result.success = false;
      result.message = "Validation failed: " + status.message;
      return result;
    }
  }
  
  if (options.dry_run) {
    result.success = true;
    result.message = "Dry run: would execute migration";
    return result;
  }
  
  // Execute migration
  auto start = std::chrono::high_resolution_clock::now();
  
  auto exec_status = ExecuteScript(migration->upgrade_script, false);
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  MigrationExecution exec;
  exec.migration_id = migration_id;
  exec.executed_at = std::chrono::system_clock::now();
  exec.execution_time = duration;
  exec.status = exec_status.ok ? MigrationStatus::kCompleted : MigrationStatus::kFailed;
  exec.error_message = exec_status.message;
  exec.checksum_at_execution = migration->checksum;
  exec.execution_order = static_cast<int>(execution_history_.size()) + 1;
  
  execution_history_.push_back(exec);
  
  result.success = exec_status.ok;
  result.message = exec_status.ok ? "Migration executed successfully" : exec_status.message;
  
  return result;
}

// ============================================================================
// Rollback
// ============================================================================

MigrationResult MigrationManager::Rollback(const MigrationOptions& options) {
  MigrationResult result;
  
  if (execution_history_.empty()) {
    result.success = true;
    result.message = "No migrations to rollback";
    return result;
  }
  
  // Rollback last executed migration
  auto last_exec = execution_history_.back();
  return RollbackOne(last_exec.migration_id, options);
}

MigrationResult MigrationManager::RollbackTo(
    const std::string& target_migration_id,
    const MigrationOptions& options) {
  MigrationResult result;
  
  auto to_rollback = GetOrderedMigrationsToRollback(target_migration_id);
  
  for (const auto& migration : to_rollback) {
    auto single_result = RollbackOne(migration.id, options);
    if (single_result.success) {
      result.executed_migrations.push_back(migration.id);
      result.completed_scripts++;
    } else {
      result.failed_migrations.push_back(migration.id);
      break;
    }
  }
  
  result.total_scripts = static_cast<int>(to_rollback.size());
  result.success = result.failed_migrations.empty();
  result.message = result.success ? "Rollback to target completed" : 
                   "Rollback failed";
  
  return result;
}

MigrationResult MigrationManager::RollbackOne(
    const std::string& migration_id,
    const MigrationOptions& options) {
  MigrationResult result;
  
  auto migration = GetMigration(migration_id);
  if (!migration.has_value()) {
    result.success = false;
    result.message = "Migration not found: " + migration_id;
    return result;
  }
  
  if (!migration->is_reversible) {
    result.success = false;
    result.message = "Migration is not reversible";
    return result;
  }
  
  if (migration->downgrade_script.empty()) {
    result.success = false;
    result.message = "No downgrade script available";
    return result;
  }
  
  if (options.dry_run) {
    result.success = true;
    result.message = "Dry run: would rollback migration";
    return result;
  }
  
  // Execute rollback
  auto exec_status = ExecuteScript(migration->downgrade_script, true);
  
  // Update execution history
  auto it = std::remove_if(execution_history_.begin(), execution_history_.end(),
                           [&migration_id](const MigrationExecution& e) {
                             return e.migration_id == migration_id;
                           });
  execution_history_.erase(it, execution_history_.end());
  
  result.success = exec_status.ok;
  result.message = exec_status.ok ? "Rollback completed" : exec_status.message;
  
  return result;
}

// ============================================================================
// Redo
// ============================================================================

MigrationResult MigrationManager::Redo(
    const std::string& migration_id,
    const MigrationOptions& options) {
  MigrationResult result;
  
  auto rollback_result = RollbackOne(migration_id, options);
  if (!rollback_result.success) {
    return rollback_result;
  }
  
  return MigrateOne(migration_id, options);
}

// ============================================================================
// Progress Callback
// ============================================================================

void MigrationManager::SetProgressCallback(MigrationProgressCallback callback) {
  progress_callback_ = callback;
}

// ============================================================================
// Validation
// ============================================================================

Status MigrationManager::ValidateMigration(const MigrationScript& migration) {
  if (migration.id.empty()) {
    return Status::Error("Migration ID is required");
  }
  if (migration.name.empty()) {
    return Status::Error("Migration name is required");
  }
  if (migration.upgrade_script.empty()) {
    return Status::Error("Upgrade script is required");
  }
  
  std::string computed_checksum = ComputeChecksum(migration.upgrade_script);
  if (!migration.checksum.empty() && migration.checksum != computed_checksum) {
    return Status::Error("Checksum mismatch - migration has been modified");
  }
  
  return Status::Ok();
}

Status MigrationManager::ValidateMigrations() {
  for (const auto& migration : migrations_) {
    auto status = ValidateMigration(migration);
    if (!status.ok) {
      return status;
    }
  }
  return Status::Ok();
}

std::vector<std::string> MigrationManager::CheckForConflicts() {
  std::vector<std::string> conflicts;
  // Check for duplicate IDs or names
  for (size_t i = 0; i < migrations_.size(); ++i) {
    for (size_t j = i + 1; j < migrations_.size(); ++j) {
      if (migrations_[i].id == migrations_[j].id) {
        conflicts.push_back("Duplicate migration ID: " + migrations_[i].id);
      }
      if (migrations_[i].name == migrations_[j].name) {
        conflicts.push_back("Duplicate migration name: " + migrations_[i].name);
      }
    }
  }
  return conflicts;
}

std::vector<std::string> MigrationManager::CheckForMissingDependencies() {
  std::vector<std::string> missing;
  for (const auto& migration : migrations_) {
    for (const auto& dep : migration.dependencies) {
      if (!GetMigration(dep).has_value()) {
        missing.push_back("Missing dependency: " + dep + " (required by " + 
                          migration.id + ")");
      }
    }
  }
  return missing;
}

// ============================================================================
// Baseline
// ============================================================================

Status MigrationManager::CreateBaseline(const std::string& description) {
  MigrationScript baseline;
  baseline.id = "baseline";
  baseline.name = "Baseline";
  baseline.description = description;
  baseline.is_baseline = true;
  baseline.is_reversible = false;
  baseline.upgrade_script = "-- Baseline migration - represents current state";
  
  RegisterMigration(baseline);
  
  MigrationExecution exec;
  exec.migration_id = baseline.id;
  exec.executed_at = std::chrono::system_clock::now();
  exec.status = MigrationStatus::kCompleted;
  execution_history_.push_back(exec);
  
  return Status::Ok();
}

// ============================================================================
// History
// ============================================================================

std::vector<MigrationExecution> MigrationManager::GetExecutionHistory() {
  return execution_history_;
}

std::vector<MigrationExecution> MigrationManager::GetExecutionHistory(
    const std::string& migration_id) {
  std::vector<MigrationExecution> result;
  for (const auto& exec : execution_history_) {
    if (exec.migration_id == migration_id) {
      result.push_back(exec);
    }
  }
  return result;
}

void MigrationManager::ClearExecutionHistory() {
  execution_history_.clear();
}

// ============================================================================
// Version Info
// ============================================================================

std::string MigrationManager::GetCurrentVersion() {
  if (execution_history_.empty()) {
    return "";
  }
  return execution_history_.back().migration_id;
}

std::string MigrationManager::GetLatestVersion() {
  auto ordered = GetOrderedMigrations();
  if (ordered.empty()) {
    return "";
  }
  return ordered.back().id;
}

bool MigrationManager::IsUpToDate() {
  return GetCurrentVersion() == GetLatestVersion();
}

// ============================================================================
// Export/Import
// ============================================================================

bool MigrationManager::ExportMigrationScript(
    const MigrationScript& migration,
    const std::string& filepath) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  file << "-- Migration: " << migration.name << "\n";
  file << "-- ID: " << migration.id << "\n";
  file << "-- Description: " << migration.description << "\n";
  file << "-- Created: " << migration.created_at << "\n\n";
  file << "-- Upgrade --\n";
  file << migration.upgrade_script << "\n\n";
  
  if (!migration.downgrade_script.empty()) {
    file << "-- Downgrade --\n";
    file << migration.downgrade_script << "\n";
  }
  
  return true;
}

std::optional<MigrationScript> MigrationManager::ImportMigrationScript(
    const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return std::nullopt;
  }
  
  MigrationScript migration;
  // Would parse migration file format
  return migration;
}

// ============================================================================
// Load/Save from Directory
// ============================================================================

bool MigrationManager::LoadMigrationsFromDirectory(const std::string& directory) {
  // Would scan directory for migration files
  return true;
}

bool MigrationManager::SaveMigrationsToDirectory(const std::string& directory) {
  // Would save each migration to a file in the directory
  return true;
}

// ============================================================================
// Persistence
// ============================================================================

bool MigrationManager::LoadState(const std::string& filepath) {
  // Would load migrations and execution history from JSON
  return true;
}

bool MigrationManager::SaveState(const std::string& filepath) {
  // Would save migrations and execution history to JSON
  return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string MigrationManager::ComputeChecksum(const std::string& content) {
  std::hash<std::string> hasher;
  return std::to_string(hasher(content));
}

std::vector<MigrationScript> MigrationManager::GetOrderedMigrations() {
  std::vector<MigrationScript> ordered = migrations_;
  // Sort by creation order (assuming IDs are sequential or timestamps)
  std::sort(ordered.begin(), ordered.end(),
            [](const MigrationScript& a, const MigrationScript& b) {
              return a.created_at < b.created_at;
            });
  return ordered;
}

std::vector<MigrationScript> MigrationManager::GetOrderedMigrationsToRollback(
    const std::string& target_id) {
  std::vector<MigrationScript> to_rollback;
  
  // Get executed migrations in reverse order
  for (auto it = execution_history_.rbegin(); 
       it != execution_history_.rend(); ++it) {
    if (it->migration_id == target_id) {
      break;
    }
    auto migration = GetMigration(it->migration_id);
    if (migration.has_value()) {
      to_rollback.push_back(migration.value());
    }
  }
  
  return to_rollback;
}

bool MigrationManager::HasDependency(const MigrationScript& migration,
                                     const std::string& dependency_id) {
  return std::find(migration.dependencies.begin(), 
                   migration.dependencies.end(), 
                   dependency_id) != migration.dependencies.end();
}

void MigrationManager::NotifyProgress(const MigrationProgress& progress) {
  if (progress_callback_) {
    progress_callback_(progress);
  }
}

Status MigrationManager::ExecuteScript(const std::string& script, 
                                       bool is_downgrade) {
  // Would execute SQL script against database
  // This is a stub implementation
  return Status::Ok();
}

}  // namespace scratchrobin::core
