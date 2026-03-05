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
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "core/result_set.h"
#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Query execution state
enum class QueryExecutionState {
  kPending,
  kRunning,
  kCompleted,
  kCancelled,
  kFailed
};

// Query result wrapper
struct AsyncQueryResult {
  Status status;
  ResultSet result_set;
  std::string query;
  int64_t execution_time_ms{0};
  int64_t rows_affected{0};
};

// Query task definition
struct QueryTask {
  std::string task_id;
  std::string query;
  std::shared_ptr<Connection> connection;
  std::function<void(const AsyncQueryResult&)> callback;
};

// Progress information
struct QueryProgress {
  std::string task_id;
  int percentage{0};
  std::string status_message;
  int64_t rows_processed{0};
  int64_t total_rows{0};
};

// ============================================================================
// AsyncQueryExecutor
// ============================================================================

class AsyncQueryExecutor {
 public:
  using ProgressCallback = std::function<void(const QueryProgress&)>;
  using CompletionCallback = std::function<void(const AsyncQueryResult&)>;

  AsyncQueryExecutor();
  ~AsyncQueryExecutor();

  // Disable copy
  AsyncQueryExecutor(const AsyncQueryExecutor&) = delete;
  AsyncQueryExecutor& operator=(const AsyncQueryExecutor&) = delete;

  // Enable move
  AsyncQueryExecutor(AsyncQueryExecutor&&) noexcept;
  AsyncQueryExecutor& operator=(AsyncQueryExecutor&&) noexcept;

  // Initialize with thread pool size
  bool Initialize(size_t thread_count = 4);
  void Shutdown();

  // Submit a query for async execution
  std::string SubmitQuery(const std::string& query,
                          std::shared_ptr<Connection> connection,
                          CompletionCallback callback = nullptr);

  // Cancel a pending or running query
  bool CancelQuery(const std::string& task_id);
  void CancelAllQueries();

  // Query status
  QueryExecutionState GetQueryState(const std::string& task_id) const;
  bool IsRunning(const std::string& task_id) const;
  bool IsPending(const std::string& task_id) const;

  // Progress tracking
  void SetProgressCallback(ProgressCallback callback);
  QueryProgress GetProgress(const std::string& task_id) const;

  // Results retrieval
  std::optional<AsyncQueryResult> GetResult(const std::string& task_id);
  void ClearCompletedTasks();

  // Statistics
  size_t GetPendingCount() const;
  size_t GetRunningCount() const;
  size_t GetCompletedCount() const;

  // Wait for completion
  bool WaitForQuery(const std::string& task_id, int timeout_ms = -1);
  void WaitForAll(int timeout_ms = -1);

 private:
  void WorkerThread();
  AsyncQueryResult ExecuteQuery(const QueryTask& task);
  void UpdateProgress(const std::string& task_id, const QueryProgress& progress);

  mutable std::mutex mutex_;
  std::condition_variable cv_;
  std::vector<std::thread> workers_;
  std::queue<QueryTask> pending_queue_;

  struct TaskInfo {
    QueryExecutionState state{QueryExecutionState::kPending};
    QueryProgress progress;
    std::optional<AsyncQueryResult> result;
    std::promise<AsyncQueryResult> promise;
  };

  std::unordered_map<std::string, TaskInfo> tasks_;
  ProgressCallback progress_callback_;
  std::atomic<bool> running_{false};
  std::atomic<size_t> running_count_{0};
};

}  // namespace scratchrobin::core
