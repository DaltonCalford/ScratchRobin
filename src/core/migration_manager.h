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
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "status.h"

namespace scratchrobin::core {

// ============================================================================
// Migration Status
// ============================================================================

enum class MigrationStatus {
  kPending,
  kInProgress,
  kCompleted,
  kFailed,
  kRolledBack
};

// ============================================================================
// Migration Type
// ============================================================================

enum class MigrationType {
  kSchema,
  kData,
  kIndex,
  kFunction,
  kProcedure,
  kTrigger,
  kView,
  kCustom
};

// ============================================================================
// Migration Script
// ============================================================================

struct MigrationScript {
  std::string id;
  std::string name;
  std::string description;
  MigrationType type{MigrationType::kSchema};
  std::string upgrade_script;
  std::string downgrade_script;
  std::vector<std::string> dependencies;  // IDs of migrations that must run first
  bool is_baseline{false};
  bool is_reversible{true};
  std::string checksum;
  std::string author;
  std::string created_at;
  std::vector<std::string> tags;
};

// ============================================================================
// Migration Execution Record
// ============================================================================

struct MigrationExecution {
  std::string migration_id;
  std::string executed_by;
  std::chrono::system_clock::time_point executed_at;
  std::chrono::milliseconds execution_time{0};
  MigrationStatus status{MigrationStatus::kCompleted};
  std::string error_message;
  std::string checksum_at_execution;
  int execution_order{0};
};

// ============================================================================
// Migration Result
// ============================================================================

struct MigrationResult {
  bool success{false};
  std::string message;
  std::vector<std::string> executed_migrations;
  std::vector<std::string> failed_migrations;
  int total_scripts{0};
  int completed_scripts{0};
};

// ============================================================================
// Migration Progress
// ============================================================================

struct MigrationProgress {
  int current_step{0};
  int total_steps{0};
  std::string current_migration;
  std::string status_message;
  double percent_complete{0.0};
};

using MigrationProgressCallback = std::function<void(const MigrationProgress&)>;

// ============================================================================
// Migration Options
// ============================================================================

struct MigrationOptions {
  bool dry_run{false};           // Show what would be executed without running
  bool force{false};             // Re-run already executed migrations
  bool validate_checksums{true}; // Validate migration checksums
  bool backup_before{true};      // Create backup before migration
  bool transaction_per_script{true};
  int timeout_seconds{300};
  std::string target_version;    // Specific version to migrate to
};

// ============================================================================
// Migration Manager
// ============================================================================

class MigrationManager {
 public:
  static MigrationManager& Instance();

  // Migration registration
  void RegisterMigration(const MigrationScript& migration);
  void RegisterMigrations(const std::vector<MigrationScript>& migrations);
  bool UnregisterMigration(const std::string& migration_id);

  // Get migrations
  std::vector<MigrationScript> GetAllMigrations();
  std::vector<MigrationScript> GetPendingMigrations();
  std::vector<MigrationScript> GetExecutedMigrations();
  std::optional<MigrationScript> GetMigration(const std::string& migration_id);
  std::optional<MigrationScript> GetMigrationByName(const std::string& name);

  // Check status
  bool IsMigrationPending(const std::string& migration_id);
  bool IsMigrationExecuted(const std::string& migration_id);
  MigrationStatus GetMigrationStatus(const std::string& migration_id);

  // Execute migrations
  MigrationResult Migrate(const MigrationOptions& options = {});
  MigrationResult MigrateTo(const std::string& target_migration_id,
                            const MigrationOptions& options = {});
  MigrationResult MigrateOne(const std::string& migration_id,
                             const MigrationOptions& options = {});

  // Rollback
  MigrationResult Rollback(const MigrationOptions& options = {});
  MigrationResult RollbackTo(const std::string& target_migration_id,
                             const MigrationOptions& options = {});
  MigrationResult RollbackOne(const std::string& migration_id,
                              const MigrationOptions& options = {});

  // Redo (rollback then migrate)
  MigrationResult Redo(const std::string& migration_id,
                       const MigrationOptions& options = {});

  // Progress callback
  void SetProgressCallback(MigrationProgressCallback callback);

  // Validation
  Status ValidateMigration(const MigrationScript& migration);
  Status ValidateMigrations();
  std::vector<std::string> CheckForConflicts();
  std::vector<std::string> CheckForMissingDependencies();

  // Baseline
  Status CreateBaseline(const std::string& description);

  // History
  std::vector<MigrationExecution> GetExecutionHistory();
  std::vector<MigrationExecution> GetExecutionHistory(
      const std::string& migration_id);
  void ClearExecutionHistory();

  // Version info
  std::string GetCurrentVersion();
  std::string GetLatestVersion();
  bool IsUpToDate();

  // Export/Import
  bool ExportMigrationScript(const MigrationScript& migration,
                             const std::string& filepath);
  std::optional<MigrationScript> ImportMigrationScript(
      const std::string& filepath);

  // Load from directory
  bool LoadMigrationsFromDirectory(const std::string& directory);
  bool SaveMigrationsToDirectory(const std::string& directory);

  // Persistence
  bool LoadState(const std::string& filepath);
  bool SaveState(const std::string& filepath);

 private:
  MigrationManager();
  ~MigrationManager() = default;

  MigrationManager(const MigrationManager&) = delete;
  MigrationManager& operator=(const MigrationManager&) = delete;

  std::vector<MigrationScript> migrations_;
  std::vector<MigrationExecution> execution_history_;
  MigrationProgressCallback progress_callback_;

  std::string ComputeChecksum(const std::string& content);
  std::vector<MigrationScript> GetOrderedMigrations();
  std::vector<MigrationScript> GetOrderedMigrationsToRollback(
      const std::string& target_id);
  bool HasDependency(const MigrationScript& migration,
                     const std::string& dependency_id);
  void NotifyProgress(const MigrationProgress& progress);
  Status ExecuteScript(const std::string& script, bool is_downgrade);
};

}  // namespace scratchrobin::core
