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

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/result_set.h"
#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Sync direction
enum class SyncDirection {
  kSourceToTarget,
  kTargetToSource,
  kBidirectional
};

// Conflict resolution strategy
enum class ConflictResolution {
  kSourceWins,
  kTargetWins,
  kNewestWins,
  kManual,
  kSkip
};

// Sync operation type
enum class SyncOperationType {
  kInsert,
  kUpdate,
  kDelete,
  kNone
};

// Table sync configuration
struct TableSyncConfig {
  std::string source_table;
  std::string target_table;
  std::vector<std::string> key_columns;
  std::vector<std::string> sync_columns;
  std::vector<std::string> exclude_columns;
  std::string timestamp_column;
  ConflictResolution conflict_resolution{ConflictResolution::kNewestWins};
  bool sync_deletes{true};
  bool sync_inserts{true};
  bool sync_updates{true};
  std::string where_clause;
};

// Sync job configuration
struct SyncJobConfig {
  std::string job_id;
  std::string job_name;
  std::shared_ptr<Connection> source_connection;
  std::shared_ptr<Connection> target_connection;
  std::vector<TableSyncConfig> tables;
  SyncDirection direction{SyncDirection::kSourceToTarget};
  bool dry_run{false};
  int batch_size{1000};
  int max_retries{3};
  bool continue_on_error{false};
};

// Sync progress information
struct SyncProgress {
  std::string job_id;
  std::string current_table;
  int64_t total_tables{0};
  int64_t current_table_index{0};
  int64_t rows_processed{0};
  int64_t total_rows{0};
  int64_t rows_inserted{0};
  int64_t rows_updated{0};
  int64_t rows_deleted{0};
  int64_t rows_skipped{0};
  int64_t conflicts{0};
  std::string status_message;
  double percentage_complete{0.0};
  std::chrono::milliseconds elapsed_time{0};
  std::chrono::milliseconds estimated_remaining{0};
};

// Sync result statistics
struct SyncResult {
  Status status;
  std::string job_id;
  int64_t total_rows_processed{0};
  int64_t total_rows_inserted{0};
  int64_t total_rows_updated{0};
  int64_t total_rows_deleted{0};
  int64_t total_rows_skipped{0};
  int64_t total_conflicts{0};
  int64_t tables_synced{0};
  int64_t tables_failed{0};
  std::vector<std::string> errors;
  std::chrono::milliseconds total_duration{0};
  std::map<std::string, std::string> table_results;
};

// Conflict record for manual resolution
struct SyncConflict {
  std::string table_name;
  std::map<std::string, std::string> key_values;
  std::map<std::string, std::string> source_row;
  std::map<std::string, std::string> target_row;
  SyncOperationType proposed_operation;
  std::chrono::system_clock::time_point source_timestamp;
  std::chrono::system_clock::time_point target_timestamp;
};

// ============================================================================
// DataSyncManager
// ============================================================================

class DataSyncManager {
 public:
  using ProgressCallback = std::function<void(const SyncProgress&)>;
  using ConflictCallback = std::function<ConflictResolution(const SyncConflict&)>;
  using CompletionCallback = std::function<void(const SyncResult&)>;

  DataSyncManager();
  ~DataSyncManager();

  // Disable copy
  DataSyncManager(const DataSyncManager&) = delete;
  DataSyncManager& operator=(const DataSyncManager&) = delete;

  // Configuration
  void SetProgressCallback(ProgressCallback callback);
  void SetConflictCallback(ConflictCallback callback);
  void SetCompletionCallback(CompletionCallback callback);

  // Sync operations
  SyncResult ExecuteSync(const SyncJobConfig& config);
  void ExecuteSyncAsync(const SyncJobConfig& config);
  
  // Job management
  void CancelSync(const std::string& job_id);
  bool IsSyncRunning(const std::string& job_id) const;
  SyncProgress GetProgress(const std::string& job_id) const;
  std::optional<SyncResult> GetResult(const std::string& job_id);
  
  // Table comparison
  Status CompareTables(const TableSyncConfig& config,
                       std::shared_ptr<Connection> source,
                       std::shared_ptr<Connection> target,
                       ResultSet* differences);
  
  // Schema validation
  Status ValidateTableSchema(const TableSyncConfig& config,
                             std::shared_ptr<Connection> source,
                             std::shared_ptr<Connection> target);
  
  // Conflict resolution
  std::vector<SyncConflict> GetConflicts(const std::string& job_id) const;
  Status ResolveConflict(const std::string& job_id,
                         int conflict_index,
                         ConflictResolution resolution);
  
  // Batch operations
  Status SyncSingleTable(const TableSyncConfig& config,
                         std::shared_ptr<Connection> source,
                         std::shared_ptr<Connection> target,
                         SyncDirection direction);
  
  // Cleanup
  void ClearCompletedJobs();
  void ClearAllJobs();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
