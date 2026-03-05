/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/server_monitor.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

ServerMonitor& ServerMonitor::Instance() {
  static ServerMonitor instance;
  return instance;
}

// ============================================================================
// Connection Monitoring
// ============================================================================

std::vector<ServerConnectionInfo> ServerMonitor::GetActiveConnections() {
  std::vector<ServerConnectionInfo> connections;
  // Would query database for active connections
  return connections;
}

std::vector<ServerConnectionInfo> ServerMonitor::GetConnectionsForDatabase(
    const std::string& database_name) {
  auto all = GetActiveConnections();
  std::vector<ServerConnectionInfo> filtered;
  std::copy_if(all.begin(), all.end(), std::back_inserter(filtered),
               [&database_name](const ServerConnectionInfo& c) {
                 return c.database_name == database_name;
               });
  return filtered;
}

bool ServerMonitor::TerminateConnection(int64_t process_id) {
  // Would send termination signal to database process
  return true;
}

// ============================================================================
// Performance Monitoring
// ============================================================================

ServerPerformanceMetrics ServerMonitor::GetCurrentMetrics() {
  ServerPerformanceMetrics metrics;
  metrics.timestamp = std::chrono::system_clock::now();
  
  // Would query database for current metrics
  // This is a stub implementation
  metrics.cpu_percent = 0.0;
  metrics.memory_used_bytes = 0;
  metrics.connections_total = 0;
  
  return metrics;
}

std::vector<ServerPerformanceMetrics> ServerMonitor::GetMetricsHistory(
    const std::chrono::system_clock::time_point& from,
    const std::chrono::system_clock::time_point& to) {
  std::vector<ServerPerformanceMetrics> result;
  for (const auto& metrics : metrics_history_) {
    if (metrics.timestamp >= from && metrics.timestamp <= to) {
      result.push_back(metrics);
    }
  }
  return result;
}

// ============================================================================
// Database Statistics
// ============================================================================

std::vector<DatabaseStatistics> ServerMonitor::GetDatabaseStatistics() {
  std::vector<DatabaseStatistics> stats;
  // Would query database for statistics
  return stats;
}

DatabaseStatistics ServerMonitor::GetDatabaseStatistics(
    const std::string& database_name) {
  auto all = GetDatabaseStatistics();
  for (const auto& stat : all) {
    if (stat.database_name == database_name) {
      return stat;
    }
  }
  return DatabaseStatistics{};
}

// ============================================================================
// Real-time Monitoring
// ============================================================================

void ServerMonitor::StartMonitoring(int interval_seconds) {
  if (is_monitoring_) {
    return;
  }
  
  is_monitoring_ = true;
  monitoring_interval_seconds_ = interval_seconds;
  
  // In a real implementation, this would start a background thread
  // For this stub, we'll just set the flag
}

void ServerMonitor::StopMonitoring() {
  is_monitoring_ = false;
}

void ServerMonitor::SetMetricsCallback(MetricsCallback callback) {
  metrics_callback_ = callback;
}

// ============================================================================
// Alerting
// ============================================================================

void ServerMonitor::SetAlertThresholds(const AlertThresholds& thresholds) {
  thresholds_ = thresholds;
}

void ServerMonitor::SetAlertCallback(AlertCallback callback) {
  alert_callback_ = callback;
}

void ServerMonitor::AcknowledgeAlert(int64_t alert_id) {
  for (auto& alert : alert_history_) {
    if (alert.id == alert_id) {
      alert.acknowledged = true;
      break;
    }
  }
}

std::vector<ServerAlert> ServerMonitor::GetActiveAlerts() {
  std::vector<ServerAlert> active;
  for (const auto& alert : alert_history_) {
    if (!alert.acknowledged) {
      active.push_back(alert);
    }
  }
  return active;
}

std::vector<ServerAlert> ServerMonitor::GetAlertHistory(int limit) {
  std::vector<ServerAlert> result;
  int count = 0;
  for (auto it = alert_history_.rbegin(); 
       it != alert_history_.rend() && count < limit; 
       ++it, ++count) {
    result.push_back(*it);
  }
  return result;
}

// ============================================================================
// Long-running Queries
// ============================================================================

std::vector<ServerConnectionInfo> ServerMonitor::GetLongRunningQueries(
    int min_seconds) {
  auto connections = GetActiveConnections();
  std::vector<ServerConnectionInfo> long_running;
  
  auto now = std::chrono::system_clock::now();
  for (const auto& conn : connections) {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        now - conn.last_activity).count();
    if (duration >= min_seconds) {
      long_running.push_back(conn);
    }
  }
  
  return long_running;
}

// ============================================================================
// Blocking Queries
// ============================================================================

std::vector<std::pair<ServerConnectionInfo, ServerConnectionInfo>>
ServerMonitor::GetBlockingQueries() {
  std::vector<std::pair<ServerConnectionInfo, ServerConnectionInfo>> blocking;
  // Would query database lock information
  return blocking;
}

// ============================================================================
// Lock Monitoring
// ============================================================================

std::vector<ServerMonitor::LockInfo> ServerMonitor::GetCurrentLocks() {
  std::vector<LockInfo> locks;
  // Would query database for lock information
  return locks;
}

// ============================================================================
// Export
// ============================================================================

bool ServerMonitor::ExportMetricsToCsv(
    const std::string& filepath,
    const std::chrono::system_clock::time_point& from,
    const std::chrono::system_clock::time_point& to) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  file << "Timestamp,CPU%,MemoryUsed,ConnectionsTotal,Transactions/sec,CacheHit%\n";
  
  auto metrics = GetMetricsHistory(from, to);
  for (const auto& m : metrics) {
    file << std::chrono::system_clock::to_time_t(m.timestamp) << ","
         << m.cpu_percent << ","
         << m.memory_used_bytes << ","
         << m.connections_total << ","
         << m.transactions_per_sec << ","
         << m.cache_hit_ratio << "\n";
  }
  
  return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void ServerMonitor::MonitoringLoop() {
  while (is_monitoring_) {
    auto metrics = GetCurrentMetrics();
    metrics_history_.push_back(metrics);
    
    if (metrics_callback_) {
      metrics_callback_(metrics);
    }
    
    CheckAlertConditions(metrics);
    TrimHistory();
    
    // Sleep for interval
    std::this_thread::sleep_for(
        std::chrono::seconds(monitoring_interval_seconds_));
  }
}

void ServerMonitor::CheckAlertConditions(const ServerPerformanceMetrics& metrics) {
  // Check CPU
  if (metrics.cpu_percent >= thresholds_.cpu_critical) {
    ServerAlert alert;
    alert.id = next_alert_id_++;
    alert.severity = AlertSeverity::kCritical;
    alert.category = "cpu";
    alert.message = "CPU usage is critically high";
    alert.current_value = metrics.cpu_percent;
    alert.threshold_value = thresholds_.cpu_critical;
    alert.triggered_at = std::chrono::system_clock::now();
    alert_history_.push_back(alert);
    
    if (alert_callback_) {
      alert_callback_(alert);
    }
  } else if (metrics.cpu_percent >= thresholds_.cpu_warning) {
    ServerAlert alert;
    alert.id = next_alert_id_++;
    alert.severity = AlertSeverity::kWarning;
    alert.category = "cpu";
    alert.message = "CPU usage is high";
    alert.current_value = metrics.cpu_percent;
    alert.threshold_value = thresholds_.cpu_warning;
    alert.triggered_at = std::chrono::system_clock::now();
    alert_history_.push_back(alert);
    
    if (alert_callback_) {
      alert_callback_(alert);
    }
  }
  
  // Check Memory
  if (metrics.memory_percent >= thresholds_.memory_critical) {
    ServerAlert alert;
    alert.id = next_alert_id_++;
    alert.severity = AlertSeverity::kCritical;
    alert.category = "memory";
    alert.message = "Memory usage is critically high";
    alert.current_value = metrics.memory_percent;
    alert.threshold_value = thresholds_.memory_critical;
    alert.triggered_at = std::chrono::system_clock::now();
    alert_history_.push_back(alert);
    
    if (alert_callback_) {
      alert_callback_(alert);
    }
  }
  
  // Check Cache Hit Ratio
  if (metrics.cache_hit_ratio < thresholds_.cache_hit_critical) {
    ServerAlert alert;
    alert.id = next_alert_id_++;
    alert.severity = AlertSeverity::kWarning;
    alert.category = "cache";
    alert.message = "Cache hit ratio is low";
    alert.current_value = metrics.cache_hit_ratio;
    alert.threshold_value = thresholds_.cache_hit_critical;
    alert.triggered_at = std::chrono::system_clock::now();
    alert_history_.push_back(alert);
    
    if (alert_callback_) {
      alert_callback_(alert);
    }
  }
}

void ServerMonitor::TrimHistory() {
  // Trim metrics history
  auto cutoff = std::chrono::system_clock::now() - 
                std::chrono::hours(retention_hours_);
  
  metrics_history_.erase(
      std::remove_if(metrics_history_.begin(), metrics_history_.end(),
                     [&cutoff](const ServerPerformanceMetrics& m) {
                       return m.timestamp < cutoff;
                     }),
      metrics_history_.end());
  
  // Limit data points
  while (static_cast<int>(metrics_history_.size()) > max_data_points_) {
    metrics_history_.erase(metrics_history_.begin());
  }
}

}  // namespace scratchrobin::core
