/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/data_sync_manager.h"

namespace scratchrobin::core {

// Private implementation
struct DataSyncManager::Impl {
  ProgressCallback progress_callback;
  ConflictCallback conflict_callback;
  CompletionCallback completion_callback;
  std::map<std::string, SyncProgress> active_jobs;
  std::map<std::string, SyncResult> completed_jobs;
};

DataSyncManager::DataSyncManager() 
    : impl_(std::make_unique<Impl>()) {
}

DataSyncManager::~DataSyncManager() = default;

void DataSyncManager::SetProgressCallback(ProgressCallback callback) {
  impl_->progress_callback = callback;
}

void DataSyncManager::SetConflictCallback(ConflictCallback callback) {
  impl_->conflict_callback = callback;
}

void DataSyncManager::SetCompletionCallback(CompletionCallback callback) {
  impl_->completion_callback = callback;
}

SyncResult DataSyncManager::ExecuteSync(const SyncJobConfig& config) {
  SyncResult result;
  result.job_id = config.job_id;
  result.status = Status::Ok();
  result.started_at = std::chrono::system_clock::now();
  
  // Track active job
  SyncProgress progress;
  progress.job_id = config.job_id;
  progress.total_tables = static_cast<int64_t>(config.tables.size());
  progress.status_message = "Starting sync...";
  impl_->active_jobs[config.job_id] = progress;
  
  // Report progress
  if (impl_->progress_callback) {
    impl_->progress_callback(progress);
  }
  
  // Process each table
  for (size_t i = 0; i < config.tables.size(); ++i) {
    const auto& table = config.tables[i];
    
    // Update progress
    impl_->active_jobs[config.job_id].current_table = table.source_table;
    impl_->active_jobs[config.job_id].current_table_index = static_cast<int64_t>(i);
    impl_->active_jobs[config.job_id].status_message = "Syncing table: " + table.source_table;
    
    if (impl_->progress_callback) {
      impl_->progress_callback(impl_->active_jobs[config.job_id]);
    }
    
    ++result.tables_synced;
  }
  
  // Move from active to completed
  impl_->active_jobs.erase(config.job_id);
  result.completed_at = std::chrono::system_clock::now();
  impl_->completed_jobs[config.job_id] = result;
  
  if (impl_->completion_callback) {
    impl_->completion_callback(result);
  }
  
  return result;
}

void DataSyncManager::ExecuteSyncAsync(const SyncJobConfig& config) {
  // Execute synchronously for now (in production, would use a thread pool)
  ExecuteSync(config);
}

void DataSyncManager::CancelSync(const std::string& job_id) {
  auto it = impl_->active_jobs.find(job_id);
  if (it != impl_->active_jobs.end()) {
    it->second.status_message = "Cancelled";
    // In production, would signal the sync thread to stop
  }
}

bool DataSyncManager::IsSyncRunning(const std::string& job_id) const {
  return impl_->active_jobs.find(job_id) != impl_->active_jobs.end();
}

SyncProgress DataSyncManager::GetProgress(const std::string& job_id) const {
  auto it = impl_->active_jobs.find(job_id);
  if (it != impl_->active_jobs.end()) {
    return it->second;
  }
  return SyncProgress{};
}

std::optional<SyncResult> DataSyncManager::GetResult(const std::string& job_id) {
  auto it = impl_->completed_jobs.find(job_id);
  if (it != impl_->completed_jobs.end()) {
    return it->second;
  }
  return std::nullopt;
}

Status DataSyncManager::CompareTables(const TableSyncConfig& config,
                                      std::shared_ptr<Connection> source,
                                      std::shared_ptr<Connection> target,
                                      ResultSet* differences) {
  (void)config;
  (void)source;
  (void)target;
  (void)differences;
  return Status::Ok();
}

Status DataSyncManager::ValidateTableSchema(const TableSyncConfig& config,
                                            std::shared_ptr<Connection> source,
                                            std::shared_ptr<Connection> target) {
  (void)config;
  (void)source;
  (void)target;
  return Status::Ok();
}

std::vector<SyncConflict> DataSyncManager::GetConflicts(const std::string& job_id) const {
  (void)job_id;
  return {};
}

Status DataSyncManager::ResolveConflict(const std::string& job_id,
                                        int conflict_index,
                                        ConflictResolution resolution) {
  (void)job_id;
  (void)conflict_index;
  (void)resolution;
  return Status::Ok();
}

Status DataSyncManager::SyncSingleTable(const TableSyncConfig& config,
                                        std::shared_ptr<Connection> source,
                                        std::shared_ptr<Connection> target,
                                        SyncDirection direction) {
  (void)config;
  (void)source;
  (void)target;
  (void)direction;
  return Status::Ok();
}

void DataSyncManager::ClearCompletedJobs() {
  impl_->completed_jobs.clear();
}

void DataSyncManager::ClearAllJobs() {
  impl_->completed_jobs.clear();
  impl_->active_jobs.clear();
}

}  // namespace scratchrobin::core
