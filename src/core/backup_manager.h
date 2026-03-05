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

// Backup format types
enum class BackupFormat {
  kNative,
  kSQLDump,
  kCompressed,
  kIncremental,
  kDifferential
};

// Compression level
enum class CompressionLevel {
  kNone,
  kFast,
  kBalanced,
  kMaximum
};

// Backup scope
enum class BackupScope {
  kFull,
  kSchemaOnly,
  kDataOnly,
  kSelectedTables,
  kExcludeTables
};

// Backup configuration
struct BackupConfig {
  std::string backup_id;
  std::string backup_name;
  std::string source_connection_string;
  std::string destination_path;
  BackupFormat format{BackupFormat::kNative};
  BackupScope scope{BackupScope::kFull};
  CompressionLevel compression{CompressionLevel::kBalanced};
  
  // Scope options
  std::vector<std::string> include_tables;
  std::vector<std::string> exclude_tables;
  std::vector<std::string> include_schemas;
  
  // Performance options
  int parallel_workers{1};
  int64_t buffer_size{1048576};  // 1MB default
  bool use_encryption{false};
  std::string encryption_key;
  
  // Validation
  bool verify_after_backup{true};
  bool calculate_checksum{true};
  
  // Scheduling
  bool is_scheduled{false};
  std::string schedule_expression;  // Cron-like expression
};

// Restore configuration
struct RestoreConfig {
  std::string backup_id;
  std::string backup_path;
  std::string destination_connection_string;
  std::string target_database;
  bool create_database{true};
  bool overwrite_existing{false};
  
  // Restore options
  bool restore_schema{true};
  bool restore_data{true};
  bool restore_constraints{true};
  bool restore_indexes{true};
  
  // Partial restore
  std::vector<std::string> include_tables;
  std::vector<std::string> exclude_tables;
  std::string where_clause;  // For data filtering
  
  // Performance
  int parallel_workers{1};
  bool disable_triggers{false};
  bool disable_constraints{false};
};

// Backup metadata
struct BackupMetadata {
  std::string backup_id;
  std::string backup_name;
  std::string source_database;
  std::string source_version;
  BackupFormat format;
  BackupScope scope;
  CompressionLevel compression;
  int64_t original_size{0};
  int64_t compressed_size{0};
  std::string checksum;
  std::chrono::system_clock::time_point created_at;
  std::string created_by;
  std::chrono::system_clock::time_point expires_at;
  std::map<std::string, std::string> properties;
  std::vector<std::string> included_objects;
};

// Backup progress
struct BackupProgress {
  std::string operation_id;
  std::string phase;  // "schema", "data", "indexes", "verification"
  int64_t bytes_processed{0};
  int64_t total_bytes{0};
  int64_t objects_processed{0};
  int64_t total_objects{0};
  int64_t rows_processed{0};
  double percentage_complete{0.0};
  std::chrono::milliseconds elapsed_time{0};
  std::chrono::milliseconds estimated_remaining{0};
  std::string current_object;
  std::string status_message;
};

// Backup/restore result
struct BackupResult {
  Status status;
  std::string operation_id;
  std::string backup_path;
  int64_t total_bytes{0};
  int64_t processed_bytes{0};
  int64_t duration_ms{0};
  std::string checksum;
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
};

// Scheduled backup job
struct ScheduledBackupJob {
  std::string job_id;
  std::string job_name;
  BackupConfig config;
  std::string schedule_expression;
  bool enabled{true};
  std::chrono::system_clock::time_point last_run;
  std::chrono::system_clock::time_point next_run;
  int success_count{0};
  int failure_count{0};
  std::string last_status;
};

// ============================================================================
// BackupManager
// ============================================================================

class BackupManager {
 public:
  using ProgressCallback = std::function<void(const BackupProgress&)>;
  using CompletionCallback = std::function<void(const BackupResult&)>;

  BackupManager();
  ~BackupManager();

  // Disable copy
  BackupManager(const BackupManager&) = delete;
  BackupManager& operator=(const BackupManager&) = delete;

  // Initialize
  bool Initialize(const std::string& storage_path);
  void Shutdown();
  
  // Backup operations
  BackupResult CreateBackup(const BackupConfig& config);
  void CreateBackupAsync(const BackupConfig& config);
  
  // Restore operations
  BackupResult RestoreBackup(const RestoreConfig& config);
  void RestoreBackupAsync(const RestoreConfig& config);
  
  // Backup information
  std::vector<BackupMetadata> ListBackups() const;
  std::optional<BackupMetadata> GetBackupMetadata(const std::string& backup_id) const;
  bool BackupExists(const std::string& backup_id) const;
  
  // Backup management
  Status DeleteBackup(const std::string& backup_id);
  Status VerifyBackup(const std::string& backup_id);
  Status ArchiveBackup(const std::string& backup_id, const std::string& archive_path);
  
  // Progress and control
  BackupProgress GetProgress(const std::string& operation_id) const;
  void CancelOperation(const std::string& operation_id);
  bool IsOperationRunning(const std::string& operation_id) const;
  
  // Callbacks
  void SetProgressCallback(ProgressCallback callback);
  void SetCompletionCallback(CompletionCallback callback);
  
  // Scheduled backups
  Status ScheduleBackup(const std::string& job_name,
                        const BackupConfig& config,
                        const std::string& schedule_expression);
  Status UnscheduleBackup(const std::string& job_id);
  std::vector<ScheduledBackupJob> ListScheduledJobs() const;
  Status EnableScheduledJob(const std::string& job_id, bool enabled);
  Status RunScheduledJobNow(const std::string& job_id);
  
  // Import/export
  Status ExportBackupToFile(const std::string& backup_id,
                            const std::string& destination_path);
  Status ImportBackupFromFile(const std::string& source_path,
                              std::string* imported_backup_id);
  
  // Comparison
  Status CompareBackups(const std::string& backup_id1,
                        const std::string& backup_id2,
                        std::vector<std::string>* differences);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
