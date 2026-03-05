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
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Metric types
enum class MetricType {
  kCounter,
  kGauge,
  kHistogram,
  kTimer
};

// Query performance metrics
struct QueryMetrics {
  std::string query_id;
  std::string query_hash;
  std::string query_text;
  int64_t execution_count{0};
  int64_t total_time_ms{0};
  int64_t min_time_ms{0};
  int64_t max_time_ms{0};
  double avg_time_ms{0.0};
  int64_t rows_returned{0};
  int64_t rows_affected{0};
  int64_t error_count{0};
  std::chrono::system_clock::time_point last_executed;
};

// Connection pool metrics
struct ConnectionPoolMetrics {
  std::string pool_id;
  int total_connections{0};
  int active_connections{0};
  int idle_connections{0};
  int waiting_requests{0};
  int64_t total_requests{0};
  int64_t failed_requests{0};
  int64_t avg_wait_time_ms{0};
  int64_t avg_use_time_ms{0};
};

// System resource metrics
struct SystemMetrics {
  std::chrono::system_clock::time_point timestamp;
  double cpu_percent{0.0};
  int64_t memory_used_bytes{0};
  int64_t memory_total_bytes{0};
  double memory_percent{0.0};
  int64_t disk_read_bytes{0};
  int64_t disk_write_bytes{0};
  int64_t network_in_bytes{0};
  int64_t network_out_bytes{0};
  int thread_count{0};
  int open_file_descriptors{0};
};

// Database performance metrics
struct DatabaseMetrics {
  std::string database_name;
  std::chrono::system_clock::time_point timestamp;
  int active_connections{0};
  int total_connections{0};
  int64_t transactions_per_sec{0};
  int64_t queries_per_sec{0};
  int64_t cache_hits{0};
  int64_t cache_misses{0};
  double cache_hit_ratio{0.0};
  int64_t disk_reads{0};
  int64_t disk_writes{0};
  int64_t lock_waits{0};
  int64_t deadlocks{0};
  int64_t temp_files_created{0};
  int64_t temp_bytes_written{0};
};

// Performance alert
struct PerformanceAlert {
  std::string alert_id;
  std::string metric_name;
  std::string severity;  // "info", "warning", "critical"
  std::string message;
  double threshold_value{0.0};
  double current_value{0.0};
  std::chrono::system_clock::time_point triggered_at;
  std::chrono::system_clock::time_point resolved_at;
  bool is_resolved{false};
};

// Threshold configuration
struct MetricThreshold {
  std::string metric_name;
  std::optional<double> warning_threshold;
  std::optional<double> critical_threshold;
  std::optional<double> min_value;
  std::optional<double> max_value;
  int duration_seconds{60};  // Duration before alert triggers
};

// Performance snapshot
struct PerformanceSnapshot {
  std::chrono::system_clock::time_point timestamp;
  SystemMetrics system;
  std::vector<DatabaseMetrics> databases;
  std::vector<ConnectionPoolMetrics> connection_pools;
  std::vector<QueryMetrics> top_queries;
  std::vector<PerformanceAlert> active_alerts;
};

// ============================================================================
// PerformanceMonitor
// ============================================================================

class PerformanceMonitor {
 public:
  using AlertCallback = std::function<void(const PerformanceAlert&)>;
  using MetricsCallback = std::function<void(const PerformanceSnapshot&)>;

  PerformanceMonitor();
  ~PerformanceMonitor();

  // Disable copy
  PerformanceMonitor(const PerformanceMonitor&) = delete;
  PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

  // Initialize and lifecycle
  bool Initialize(std::chrono::seconds collection_interval = std::chrono::seconds(60));
  void Shutdown();
  bool IsRunning() const;

  // Metric collection
  void StartCollection();
  void StopCollection();
  void CollectNow();
  
  // Query metrics
  void RecordQueryExecution(const std::string& query_hash,
                            const std::string& query_text,
                            int64_t execution_time_ms,
                            int64_t rows_affected,
                            bool success);
  std::vector<QueryMetrics> GetSlowQueries(int limit = 10) const;
  std::vector<QueryMetrics> GetFrequentQueries(int limit = 10) const;
  std::optional<QueryMetrics> GetQueryMetrics(const std::string& query_hash) const;
  
  // Connection pool metrics
  void RecordPoolMetrics(const std::string& pool_id,
                         const ConnectionPoolMetrics& metrics);
  std::vector<ConnectionPoolMetrics> GetPoolMetrics() const;
  std::optional<ConnectionPoolMetrics> GetPoolMetrics(const std::string& pool_id) const;
  
  // System metrics
  SystemMetrics GetCurrentSystemMetrics() const;
  std::vector<SystemMetrics> GetSystemMetricsHistory(
      std::chrono::minutes duration = std::chrono::minutes(60)) const;
  
  // Database metrics
  void RecordDatabaseMetrics(const std::string& database_name,
                             const DatabaseMetrics& metrics);
  std::vector<DatabaseMetrics> GetDatabaseMetrics(
      const std::string& database_name) const;
  
  // Snapshots
  PerformanceSnapshot GetCurrentSnapshot() const;
  std::vector<PerformanceSnapshot> GetSnapshotHistory(
      std::chrono::minutes duration = std::chrono::minutes(60)) const;
  
  // Alerting
  void AddThreshold(const MetricThreshold& threshold);
  void RemoveThreshold(const std::string& metric_name);
  std::vector<MetricThreshold> GetThresholds() const;
  std::vector<PerformanceAlert> GetActiveAlerts() const;
  std::vector<PerformanceAlert> GetAlertHistory(
      std::chrono::hours duration = std::chrono::hours(24)) const;
  void AcknowledgeAlert(const std::string& alert_id);
  void ResolveAlert(const std::string& alert_id);
  void SetAlertCallback(AlertCallback callback);
  
  // Performance analysis
  Status AnalyzeQueryPerformance(const std::string& query_hash,
                                 std::string* analysis_report);
  Status GeneratePerformanceReport(std::chrono::hours duration,
                                   std::string* report);
  std::vector<std::string> GetPerformanceRecommendations() const;
  
  // Baseline management
  void SetPerformanceBaseline();
  void ClearPerformanceBaseline();
  bool CompareToBaseline(const PerformanceSnapshot& current,
                         std::vector<PerformanceAlert>* deviations);
  
  // Callbacks
  void SetMetricsCallback(MetricsCallback callback);
  
  // Data management
  void ClearHistory();
  void ClearQueryMetrics();
  void ExportMetrics(const std::string& file_path, 
                     std::chrono::hours duration = std::chrono::hours(24));

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
