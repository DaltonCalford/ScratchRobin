/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */

#include "core/async_query_executor.h"

#include <algorithm>
#include <chrono>
#include <uuid/uuid.h>

namespace scratchrobin::core {

// Helper to generate unique task IDs
static std::string GenerateTaskId() {
  uuid_t uuid;
  uuid_generate(uuid);
  char uuid_str[37];
  uuid_unparse(uuid, uuid_str);
  return std::string(uuid_str);
}

AsyncQueryExecutor::AsyncQueryExecutor() = default;

AsyncQueryExecutor::~AsyncQueryExecutor() {
  Shutdown();
}

// Disable move (too complex for now)
AsyncQueryExecutor::AsyncQueryExecutor(AsyncQueryExecutor&&) noexcept = delete;
AsyncQueryExecutor& AsyncQueryExecutor::operator=(AsyncQueryExecutor&&) noexcept = delete;

bool AsyncQueryExecutor::Initialize(size_t thread_count) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (running_) {
    return false;  // Already initialized
  }
  
  running_ = true;
  for (size_t i = 0; i < thread_count; ++i) {
    workers_.emplace_back(&AsyncQueryExecutor::WorkerThread, this);
  }
  return true;
}

void AsyncQueryExecutor::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
      return;  // Already shut down
    }
    running_ = false;
  }
  cv_.notify_all();
  
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  
  std::lock_guard<std::mutex> lock(mutex_);
  workers_.clear();
}

std::string AsyncQueryExecutor::SubmitQuery(const std::string& query,
                                            std::shared_ptr<Connection> connection,
                                            CompletionCallback callback) {
  std::string task_id = GenerateTaskId();
  
  QueryTask task;
  task.task_id = task_id;
  task.query = query;
  task.connection = connection;
  task.callback = callback;
  
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
      return "";  // Executor not running
    }
    
    TaskInfo info;
    info.state = QueryExecutionState::kPending;
    info.progress.task_id = task_id;
    tasks_[task_id] = std::move(info);
    pending_queue_.push(std::move(task));
  }
  
  cv_.notify_one();
  return task_id;
}

bool AsyncQueryExecutor::CancelQuery(const std::string& task_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return false;  // Task not found
  }
  
  // Can only cancel pending or running tasks
  if (it->second.state == QueryExecutionState::kPending) {
    it->second.state = QueryExecutionState::kCancelled;
    // Note: Task remains in queue but will be skipped when popped
    return true;
  } else if (it->second.state == QueryExecutionState::kRunning) {
    // For running tasks, we mark it for cancellation
    // The worker thread will check this flag periodically
    it->second.state = QueryExecutionState::kCancelled;
    return true;
  }
  
  return false;  // Already completed/failed/cancelled
}

void AsyncQueryExecutor::CancelAllQueries() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  // Mark all pending and running tasks as cancelled
  for (auto& [id, info] : tasks_) {
    if (info.state == QueryExecutionState::kPending || 
        info.state == QueryExecutionState::kRunning) {
      info.state = QueryExecutionState::kCancelled;
    }
  }
  
  // Clear pending queue
  std::queue<QueryTask> empty;
  std::swap(pending_queue_, empty);
}

QueryExecutionState AsyncQueryExecutor::GetQueryState(const std::string& task_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return QueryExecutionState::kFailed;  // Task not found
  }
  
  return it->second.state;
}

bool AsyncQueryExecutor::IsRunning(const std::string& task_id) const {
  return GetQueryState(task_id) == QueryExecutionState::kRunning;
}

bool AsyncQueryExecutor::IsPending(const std::string& task_id) const {
  return GetQueryState(task_id) == QueryExecutionState::kPending;
}

void AsyncQueryExecutor::SetProgressCallback(ProgressCallback callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  progress_callback_ = callback;
}

QueryProgress AsyncQueryExecutor::GetProgress(const std::string& task_id) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return QueryProgress{};  // Task not found
  }
  
  return it->second.progress;
}

std::optional<AsyncQueryResult> AsyncQueryExecutor::GetResult(const std::string& task_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = tasks_.find(task_id);
  if (it == tasks_.end() || !it->second.result.has_value()) {
    return std::nullopt;
  }
  
  return it->second.result;
}

void AsyncQueryExecutor::ClearCompletedTasks() {
  std::lock_guard<std::mutex> lock(mutex_);
  
  auto it = tasks_.begin();
  while (it != tasks_.end()) {
    auto state = it->second.state;
    if (state == QueryExecutionState::kCompleted || 
        state == QueryExecutionState::kFailed || 
        state == QueryExecutionState::kCancelled) {
      it = tasks_.erase(it);
    } else {
      ++it;
    }
  }
}

size_t AsyncQueryExecutor::GetPendingCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_queue_.size();
}

size_t AsyncQueryExecutor::GetRunningCount() const {
  return running_count_.load();
}

size_t AsyncQueryExecutor::GetCompletedCount() const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  size_t count = 0;
  for (const auto& [id, info] : tasks_) {
    if (info.state == QueryExecutionState::kCompleted) {
      ++count;
    }
  }
  return count;
}

bool AsyncQueryExecutor::WaitForQuery(const std::string& task_id, int timeout_ms) {
  std::unique_lock<std::mutex> lock(mutex_);
  
  auto it = tasks_.find(task_id);
  if (it == tasks_.end()) {
    return false;  // Task not found
  }
  
  // If already completed, return immediately
  if (it->second.state == QueryExecutionState::kCompleted ||
      it->second.state == QueryExecutionState::kFailed ||
      it->second.state == QueryExecutionState::kCancelled) {
    return true;
  }
  
  // Store the promise future for waiting
  auto future = it->second.promise.get_future();
  lock.unlock();
  
  if (timeout_ms < 0) {
    // Wait indefinitely
    future.wait();
    return true;
  } else {
    // Wait with timeout
    auto status = future.wait_for(std::chrono::milliseconds(timeout_ms));
    return status == std::future_status::ready;
  }
}

void AsyncQueryExecutor::WaitForAll(int timeout_ms) {
  auto start = std::chrono::steady_clock::now();
  
  while (true) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      
      // Check if all tasks are done
      bool all_done = true;
      for (const auto& [id, info] : tasks_) {
        if (info.state == QueryExecutionState::kPending || 
            info.state == QueryExecutionState::kRunning) {
          all_done = false;
          break;
        }
      }
      
      if (all_done) {
        return;
      }
    }
    
    // Check timeout
    if (timeout_ms >= 0) {
      auto elapsed = std::chrono::steady_clock::now() - start;
      if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeout_ms) {
        return;  // Timeout reached
      }
    }
    
    // Sleep a bit to avoid busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void AsyncQueryExecutor::WorkerThread() {
  while (running_) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !pending_queue_.empty() || !running_; });
    
    if (!running_) break;
    if (pending_queue_.empty()) continue;
    
    // Get next task
    auto task = std::move(pending_queue_.front());
    pending_queue_.pop();
    
    // Check if task was cancelled while in queue
    auto it = tasks_.find(task.task_id);
    if (it == tasks_.end() || it->second.state == QueryExecutionState::kCancelled) {
      lock.unlock();
      continue;  // Skip cancelled tasks
    }
    
    // Mark as running
    it->second.state = QueryExecutionState::kRunning;
    lock.unlock();
    
    ++running_count_;
    
    // Execute the query
    auto result = ExecuteQuery(task);
    
    --running_count_;
    
    // Store result and notify
    {
      std::lock_guard<std::mutex> result_lock(mutex_);
      auto task_it = tasks_.find(task.task_id);
      if (task_it != tasks_.end()) {
        task_it->second.result = result;
        task_it->second.state = result.status.ok ? 
            QueryExecutionState::kCompleted : QueryExecutionState::kFailed;
        task_it->second.promise.set_value(result);
      }
    }
    
    // Call completion callback if provided
    if (task.callback) {
      task.callback(result);
    }
  }
}

AsyncQueryResult AsyncQueryExecutor::ExecuteQuery(const QueryTask& task) {
  AsyncQueryResult result;
  result.query = task.query;
  
  auto start_time = std::chrono::steady_clock::now();
  
  // Check if task was cancelled before starting
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task.task_id);
    if (it != tasks_.end() && it->second.state == QueryExecutionState::kCancelled) {
      result.status = Status::Error("Query cancelled");
      return result;
    }
  }
  
  // Execute the query using the connection
  if (!task.connection) {
    result.status = Status::Error("No connection available");
    return result;
  }
  
  // Update progress - started
  QueryProgress progress;
  progress.task_id = task.task_id;
  progress.percentage = 0;
  progress.status_message = "Executing...";
  UpdateProgress(task.task_id, progress);
  
  // Execute query
  auto query_result = task.connection->query(task.query);
  
  auto end_time = std::chrono::steady_clock::now();
  result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time).count();
  
  // Check for cancellation after execution
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task.task_id);
    if (it != tasks_.end() && it->second.state == QueryExecutionState::kCancelled) {
      result.status = Status::Error("Query cancelled");
      return result;
    }
  }
  
  // Convert QueryResult to ResultSet
  if (query_result.success) {
    result.status = Status::Ok();
    result.rows_affected = query_result.affected_rows;
    
    // Convert columns
    for (const auto& col : query_result.columns) {
      result.result_set.columns.push_back(col.name);
    }
    
    // Convert rows
    for (const auto& row : query_result.rows) {
      result.result_set.rows.push_back(row);
    }
    
    progress.percentage = 100;
    progress.status_message = "Completed";
    progress.rows_processed = query_result.rows.size();
  } else {
    result.status = Status::Error(query_result.error_message);
    progress.status_message = "Failed: " + query_result.error_message;
  }
  
  UpdateProgress(task.task_id, progress);
  
  return result;
}

void AsyncQueryExecutor::UpdateProgress(const std::string& task_id, const QueryProgress& progress) {
  // Store progress
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
      it->second.progress = progress;
    }
  }
  
  // Call callback
  if (progress_callback_) {
    progress_callback_(progress);
  }
}

}  // namespace scratchrobin::core
