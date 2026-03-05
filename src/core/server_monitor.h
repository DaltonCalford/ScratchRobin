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
#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Server Connection Info
// ============================================================================

struct ServerConnectionInfo {
  int64_t process_id{0};
  std::string username;
  std::string database_name;
  std::string client_address;
  std::string application_name;
  std::chrono::system_clock::time_point connected_at;
  std::chrono::system_clock::time_point last_activity;
  std::string current_query;
  std::string state;  // active, idle, idle in transaction, etc.
};

// ============================================================================
// Server Performance Metrics
// ============================================================================

struct ServerPerformanceMetrics {
  std::chrono::system_clock::time_point timestamp;

  // CPU
  double cpu_percent{0.0};
  double cpu_user_percent{0.0};
  double cpu_system_percent{0.0};

  // Memory
  int64_t memory_used_bytes{0};
  int64_t memory_total_bytes{0};
  double memory_percent{0.0};
  int64_t shared_buffers_bytes{0};
  int64_t cache_bytes{0};

  // Disk
  double disk_read_mbps{0.0};
  double disk_write_mbps{0.0};
  int64_t disk_read_iops{0};
  int64_t disk_write_iops{0};

  // Network
  double network_in_mbps{0.0};
  double network_out_mbps{0.0};

  // Database specific
  int64_t transactions_per_sec{0};
  int64_t queries_per_sec{0};
  int64_t connections_active{0};
  int64_t connections_idle{0};
  int64_t connections_total{0};
  int64_t waiting_connections{0};
  int64_t blocked_queries{0};

  // Cache
  double cache_hit_ratio{0.0};
  int64_t buffer_hits_per_sec{0};
  int64_t buffer_reads_per_sec{0};
};

// ============================================================================
// Database Statistics
// ============================================================================

struct DatabaseStatistics {
  std::string database_name;
  int64_t num_tables{0};
  int64_t num_indexes{0};
  int64_t num_views{0};
  int64_t num_functions{0};
  int64_t num_procedures{0};
  int64_t num_triggers{0};
  int64_t total_size_bytes{0};
  std::chrono::system_clock::time_point last_vacuum;
  std::chrono::system_clock::time_point last_analyze;
  int64_t dead_tuples{0};
  int64_t live_tuples{0};
};

// ============================================================================
// Alert Thresholds
// ============================================================================

struct AlertThresholds {
  double cpu_warning{80.0};
  double cpu_critical{95.0};
  double memory_warning{80.0};
  double memory_critical{95.0};
  double disk_warning{80.0};
  double disk_critical{95.0};
  int64_t connections_warning{100};
  int64_t connections_critical{200};
  double cache_hit_warning{90.0};
  double cache_hit_critical{80.0};
  int64_t blocked_queries_warning{5};
  int64_t blocked_queries_critical{20};
};

// ============================================================================
// Server Alert
// ============================================================================

enum class AlertSeverity {
  kInfo,
  kWarning,
  kCritical
};

struct ServerAlert {
  int64_t id{0};
  AlertSeverity severity{AlertSeverity::kInfo};
  std::string category;  // cpu, memory, disk, connections, etc.
  std::string message;
  std::chrono::system_clock::time_point triggered_at;
  double current_value{0.0};
  double threshold_value{0.0};
  bool acknowledged{false};
};

// ============================================================================
// Alert Callback
// ============================================================================

using AlertCallback = std::function<void(const ServerAlert& alert)>;

// ============================================================================
// Server Monitor
// ============================================================================

class ServerMonitor {
 public:
  static ServerMonitor& Instance();

  // Connection monitoring
  std::vector<ServerConnectionInfo> GetActiveConnections();
  std::vector<ServerConnectionInfo> GetConnectionsForDatabase(
      const std::string& database_name);
  bool TerminateConnection(int64_t process_id);

  // Performance monitoring
  ServerPerformanceMetrics GetCurrentMetrics();
  std::vector<ServerPerformanceMetrics> GetMetricsHistory(
      const std::chrono::system_clock::time_point& from,
      const std::chrono::system_clock::time_point& to);

  // Database statistics
  std::vector<DatabaseStatistics> GetDatabaseStatistics();
  DatabaseStatistics GetDatabaseStatistics(const std::string& database_name);

  // Real-time monitoring
  void StartMonitoring(int interval_seconds = 5);
  void StopMonitoring();
  bool IsMonitoring() const { return is_monitoring_; }

  // Metrics callback
  using MetricsCallback = std::function<void(const ServerPerformanceMetrics&)>;
  void SetMetricsCallback(MetricsCallback callback);

  // Alerting
  void SetAlertThresholds(const AlertThresholds& thresholds);
  AlertThresholds GetAlertThresholds() const { return thresholds_; }

  void SetAlertCallback(AlertCallback callback);
  void AcknowledgeAlert(int64_t alert_id);
  std::vector<ServerAlert> GetActiveAlerts();
  std::vector<ServerAlert> GetAlertHistory(int limit = 100);

  // Long-running queries
  std::vector<ServerConnectionInfo> GetLongRunningQueries(
      int min_seconds = 60);

  // Blocking queries
  std::vector<std::pair<ServerConnectionInfo, ServerConnectionInfo>>
  GetBlockingQueries();

  // Lock monitoring
  struct LockInfo {
    int64_t lock_id{0};
    std::string lock_type;
    std::string lock_mode;
    std::string table_name;
    int64_t granted{false};
    int64_t waiting_pid{0};
  };
  std::vector<LockInfo> GetCurrentLocks();

  // Configuration
  void SetRetentionHours(int hours) { retention_hours_ = hours; }
  int GetRetentionHours() const { return retention_hours_; }

  void SetMaxDataPoints(int max_points) { max_data_points_ = max_points; }
  int GetMaxDataPoints() const { return max_data_points_; }

  // Export
  bool ExportMetricsToCsv(const std::string& filepath,
                          const std::chrono::system_clock::time_point& from,
                          const std::chrono::system_clock::time_point& to);

 private:
  ServerMonitor() = default;
  ~ServerMonitor() = default;

  ServerMonitor(const ServerMonitor&) = delete;
  ServerMonitor& operator=(const ServerMonitor&) = delete;

  bool is_monitoring_{false};
  int monitoring_interval_seconds_{5};
  int retention_hours_{24};
  int max_data_points_{10000};

  AlertThresholds thresholds_;
  std::vector<ServerPerformanceMetrics> metrics_history_;
  std::vector<ServerAlert> alert_history_;
  int64_t next_alert_id_{1};

  MetricsCallback metrics_callback_;
  AlertCallback alert_callback_;

  void MonitoringLoop();
  void CheckAlertConditions(const ServerPerformanceMetrics& metrics);
  void TrimHistory();
};

}  // namespace scratchrobin::core
