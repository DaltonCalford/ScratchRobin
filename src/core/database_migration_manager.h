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
#include <optional>
#include <string>
#include <vector>

#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Migration direction
enum class MigrationDirection {
  kForward,
  kBackward
};

// Migration status
enum class MigrationStatus {
  kPending,
  kRunning,
  kCompleted,
  kFailed,
  kSkipped,
  kRolledBack
};

// Migration type
enum class MigrationType {
  kSchema,
  kData,
  kMixed
};

// Single migration script/version
struct Migration {
  std::string id;
  std::string name;
  std::string description;
  std::string author;
  MigrationType type{MigrationType::kMixed};
  std::string version;
  std::string parent_version;
  std::string checksum;
  std::chrono::system_clock::time_point created_at;
  
  // Script content
  std::string upgrade_script;
  std::string downgrade_script;
  std::string verify_script;
  
  // Execution info (populated after running)
  MigrationStatus status{MigrationStatus::kPending};
  std::chrono::system_clock::time_point executed_at;
  int64_t execution_time_ms{0};
  std::string executed_by;
  std::string error_message;
};

// Migration configuration
struct MigrationConfig {
  std::string migrations_table{"schema_migrations"};
  std::string migrations_path;
  bool auto_create_table{true};
  bool validate_checksums{true};
  bool allow_out_of_order{false};
  bool clean_on_validation_error{false};
  int batch_size{1};  // Number of migrations to run in a transaction
  bool dry_run{false};
  bool skip_callbacks{false};
};

// Migration result
struct MigrationResult {
  Status status;
  std::vector<std::string> applied_migrations;
  std::vector<std::string> failed_migrations;
  std::vector<std::string> skipped_migrations;
  int64_t total_execution_time_ms{0};
  std::map<std::string, std::string> errors;
};

// Schema version info
struct SchemaVersion {
  std::string version;
  std::string description;
  std::chrono::system_clock::time_point applied_at;
  std::string applied_by;
  std::string checksum;
  int execution_time_ms{0};
  bool is_current{false};
};

// Migration difference
struct MigrationDiff {
  std::string source_version;
  std::string target_version;
  std::vector<Migration> migrations_to_apply;
  bool is_upgrade{true};
  int64_t estimated_rows_affected{0};
  std::vector<std::string> warnings;
};

// ============================================================================
// DatabaseMigrationManager
// ============================================================================

class DatabaseMigrationManager {
 public:
  using ProgressCallback = 
      std::function<void(const Migration& migration, int current, int total)>;
  using BeforeCallback = std::function<Status(const Migration& migration)>;
  using AfterCallback = std::function<void(const Migration& migration, 
                                           const Status& result)>;

  DatabaseMigrationManager();
  ~DatabaseMigrationManager();

  // Initialization
  bool Initialize(std::shared_ptr<Connection> connection,
                  const MigrationConfig& config);
  void Shutdown();
  
  // Migration discovery
  Status LoadMigrationsFromDirectory(const std::string& path);
  Status LoadMigration(const Migration& migration);
  std::vector<Migration> GetAvailableMigrations() const;
  std::vector<Migration> GetPendingMigrations() const;
  
  // Migration execution
  MigrationResult MigrateToLatest();
  MigrationResult MigrateToVersion(const std::string& target_version);
  MigrationResult Migrate(int count);  // +N forward, -N backward
  MigrationResult RedoLastMigration();
  
  // Rollback operations
  MigrationResult Rollback(int count = 1);
  MigrationResult RollbackToVersion(const std::string& version);
  
  // Validation and repair
  Status ValidateMigrations();
  Status RepairMigrationHistory();
  Status Baseline(const std::string& version);
  
  // Schema information
  std::optional<SchemaVersion> GetCurrentVersion() const;
  std::vector<SchemaVersion> GetMigrationHistory() const;
  bool IsVersionApplied(const std::string& version) const;
  
  // Migration analysis
  MigrationDiff CalculateDiff(const std::string& from_version,
                               const std::string& to_version) const;
  Status PreviewMigration(const std::string& target_version,
                          std::vector<std::string>* scripts);
  
  // Status and info
  bool HasPendingMigrations() const;
  int GetPendingCount() const;
  std::optional<Migration> GetMigration(const std::string& id) const;
  
  // Callbacks
  void SetProgressCallback(ProgressCallback callback);
  void SetBeforeMigrateCallback(BeforeCallback callback);
  void SetAfterMigrateCallback(AfterCallback callback);
  
  // Clean (DANGER: drops all objects)
  Status Clean();
  Status CleanSchema(const std::string& schema_name);
  
  // Info generation
  Status GenerateMigration(const std::string& name,
                           MigrationType type,
                           const std::string& output_path);
  Status GenerateBaseline(const std::string& output_path);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
