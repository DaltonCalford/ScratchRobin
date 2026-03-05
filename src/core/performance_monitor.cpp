/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/performance_monitor.h"

#include <algorithm>
#include <thread>

namespace scratchrobin::core {

// Private implementation
struct PerformanceMonitor::Impl {
  std::atomic<bool> running{false};
  std::atomic<bool> collecting{false};
  std::chrono::seconds collection_interval{60};
  std::thread collection_thread;
  
  mutable std::mutex mutex;
  std::map<std::string, QueryMetrics> query_metrics;
  std::map<std::string, ConnectionPoolMetrics> pool_metrics;
  std::vector<SystemMetrics> system_history;
  std::map<std::string, std::vector<DatabaseMetrics>> database_metrics;
  std::vector<PerformanceSnapshot> snapshots;
  std::vector<PerformanceAlert> alerts;
  std::vector<MetricThreshold> thresholds;
  
  AlertCallback alert_callback;
  MetricsCallback metrics_callback;
  
  PerformanceSnapshot baseline;
  bool has_baseline = false;
};

PerformanceMonitor::PerformanceMonitor()
    : impl_(std::make_unique<Impl>()) {
}

PerformanceMonitor::~PerformanceMonitor() {
  Shutdown();
}

bool PerformanceMonitor::Initialize(std::chrono::seconds collection_interval) {
  impl_->collection_interval = collection_interval;
  impl_->running = true;
  return true;
}

void PerformanceMonitor::Shutdown() {
  StopCollection();
  impl_->running = false;
}

bool PerformanceMonitor::IsRunning() const {
  return impl_->running.load();
}

void PerformanceMonitor::StartCollection() {
  if (impl_->collecting.load()) {
    return;
  }
  
  impl_->collecting = true;
  impl_->collection_thread = std::thread([this]() {
    while (impl_->collecting.load() && impl_->running.load()) {
      CollectNow();
      std::this_thread::sleep_for(impl_->collection_interval);
    }
  });
}

void PerformanceMonitor::StopCollection() {
  impl_->collecting = false;
  if (impl_->collection_thread.joinable()) {
    impl_->collection_thread.join();
  }
}

void PerformanceMonitor::CollectNow() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  PerformanceSnapshot snapshot;
  snapshot.timestamp = std::chrono::system_clock::now();
  snapshot.system = GetCurrentSystemMetrics();
  
  // Copy pool metrics
  for (const auto& [id, metrics] : impl_->pool_metrics) {
    (void)id;
    snapshot.connection_pools.push_back(metrics);
  }
  
  // Get top queries by execution count
  std::vector<QueryMetrics> queries;
  for (const auto& [hash, metrics] : impl_->query_metrics) {
    (void)hash;
    queries.push_back(metrics);
  }
  std::sort(queries.begin(), queries.end(),
            [](const QueryMetrics& a, const QueryMetrics& b) {
              return a.execution_count > b.execution_count;
            });
  if (queries.size() > 10) {
    queries.resize(10);
  }
  snapshot.top_queries = queries;
  
  // Get active alerts
  for (const auto& alert : impl_->alerts) {
    if (!alert.is_resolved) {
      snapshot.active_alerts.push_back(alert);
    }
  }
  
  impl_->snapshots.push_back(snapshot);
  impl_->system_history.push_back(snapshot.system);
  
  // Trim history (keep last 24 hours)
  auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24);
  impl_->snapshots.erase(
      std::remove_if(impl_->snapshots.begin(), impl_->snapshots.end(),
                     [cutoff](const PerformanceSnapshot& s) {
                       return s.timestamp < cutoff;
                     }),
      impl_->snapshots.end());
  
  if (impl_->metrics_callback) {
    impl_->metrics_callback(snapshot);
  }
}

void PerformanceMonitor::RecordQueryExecution(const std::string& query_hash,
                                              const std::string& query_text,
                                              int64_t execution_time_ms,
                                              int64_t rows_affected,
                                              bool success) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto& metrics = impl_->query_metrics[query_hash];
  metrics.query_hash = query_hash;
  if (metrics.query_text.empty()) {
    metrics.query_text = query_text;
  }
  ++metrics.execution_count;
  metrics.total_time_ms += execution_time_ms;
  if (metrics.min_time_ms == 0 || execution_time_ms < metrics.min_time_ms) {
    metrics.min_time_ms = execution_time_ms;
  }
  if (execution_time_ms > metrics.max_time_ms) {
    metrics.max_time_ms = execution_time_ms;
  }
  metrics.avg_time_ms = static_cast<double>(metrics.total_time_ms) / 
                        metrics.execution_count;
  metrics.rows_affected += rows_affected;
  if (!success) {
    ++metrics.error_count;
  }
  metrics.last_executed = std::chrono::system_clock::now();
}

std::vector<QueryMetrics> PerformanceMonitor::GetSlowQueries(int limit) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::vector<QueryMetrics> queries;
  for (const auto& [hash, metrics] : impl_->query_metrics) {
    (void)hash;
    queries.push_back(metrics);
  }
  
  std::sort(queries.begin(), queries.end(),
            [](const QueryMetrics& a, const QueryMetrics& b) {
              return a.avg_time_ms > b.avg_time_ms;
            });
  
  if (static_cast<size_t>(limit) < queries.size()) {
    queries.resize(limit);
  }
  
  return queries;
}

std::vector<QueryMetrics> PerformanceMonitor::GetFrequentQueries(int limit) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::vector<QueryMetrics> queries;
  for (const auto& [hash, metrics] : impl_->query_metrics) {
    (void)hash;
    queries.push_back(metrics);
  }
  
  std::sort(queries.begin(), queries.end(),
            [](const QueryMetrics& a, const QueryMetrics& b) {
              return a.execution_count > b.execution_count;
            });
  
  if (static_cast<size_t>(limit) < queries.size()) {
    queries.resize(limit);
  }
  
  return queries;
}

std::optional<QueryMetrics> PerformanceMonitor::GetQueryMetrics(
    const std::string& query_hash) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->query_metrics.find(query_hash);
  if (it != impl_->query_metrics.end()) {
    return it->second;
  }
  return std::nullopt;
}

void PerformanceMonitor::RecordPoolMetrics(const std::string& pool_id,
                                           const ConnectionPoolMetrics& metrics) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->pool_metrics[pool_id] = metrics;
}

std::vector<ConnectionPoolMetrics> PerformanceMonitor::GetPoolMetrics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::vector<ConnectionPoolMetrics> result;
  for (const auto& [id, metrics] : impl_->pool_metrics) {
    (void)id;
    result.push_back(metrics);
  }
  return result;
}

std::optional<ConnectionPoolMetrics> PerformanceMonitor::GetPoolMetrics(
    const std::string& pool_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pool_metrics.find(pool_id);
  if (it != impl_->pool_metrics.end()) {
    return it->second;
  }
  return std::nullopt;
}

SystemMetrics PerformanceMonitor::GetCurrentSystemMetrics() const {
  SystemMetrics metrics;
  metrics.timestamp = std::chrono::system_clock::now();
  // Stub - would read actual system metrics
  metrics.cpu_percent = 0.0;
  metrics.memory_used_bytes = 0;
  return metrics;
}

std::vector<SystemMetrics> PerformanceMonitor::GetSystemMetricsHistory(
    std::chrono::minutes duration) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto cutoff = std::chrono::system_clock::now() - duration;
  std::vector<SystemMetrics> result;
  
  for (const auto& metrics : impl_->system_history) {
    if (metrics.timestamp >= cutoff) {
      result.push_back(metrics);
    }
  }
  
  return result;
}

void PerformanceMonitor::RecordDatabaseMetrics(const std::string& database_name,
                                               const DatabaseMetrics& metrics) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->database_metrics[database_name].push_back(metrics);
}

std::vector<DatabaseMetrics> PerformanceMonitor::GetDatabaseMetrics(
    const std::string& database_name) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->database_metrics.find(database_name);
  if (it != impl_->database_metrics.end()) {
    return it->second;
  }
  return {};
}

PerformanceSnapshot PerformanceMonitor::GetCurrentSnapshot() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  if (!impl_->snapshots.empty()) {
    return impl_->snapshots.back();
  }
  
  return PerformanceSnapshot{};
}

std::vector<PerformanceSnapshot> PerformanceMonitor::GetSnapshotHistory(
    std::chrono::minutes duration) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto cutoff = std::chrono::system_clock::now() - duration;
  std::vector<PerformanceSnapshot> result;
  
  for (const auto& snapshot : impl_->snapshots) {
    if (snapshot.timestamp >= cutoff) {
      result.push_back(snapshot);
    }
  }
  
  return result;
}

void PerformanceMonitor::AddThreshold(const MetricThreshold& threshold) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->thresholds.push_back(threshold);
}

void PerformanceMonitor::RemoveThreshold(const std::string& metric_name) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  impl_->thresholds.erase(
      std::remove_if(impl_->thresholds.begin(), impl_->thresholds.end(),
                     [&metric_name](const MetricThreshold& t) {
                       return t.metric_name == metric_name;
                     }),
      impl_->thresholds.end());
}

std::vector<MetricThreshold> PerformanceMonitor::GetThresholds() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->thresholds;
}

std::vector<PerformanceAlert> PerformanceMonitor::GetActiveAlerts() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::vector<PerformanceAlert> result;
  for (const auto& alert : impl_->alerts) {
    if (!alert.is_resolved) {
      result.push_back(alert);
    }
  }
  return result;
}

std::vector<PerformanceAlert> PerformanceMonitor::GetAlertHistory(
    std::chrono::hours duration) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto cutoff = std::chrono::system_clock::now() - duration;
  std::vector<PerformanceAlert> result;
  
  for (const auto& alert : impl_->alerts) {
    if (alert.triggered_at >= cutoff) {
      result.push_back(alert);
    }
  }
  return result;
}

void PerformanceMonitor::AcknowledgeAlert(const std::string& alert_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  for (auto& alert : impl_->alerts) {
    if (alert.alert_id == alert_id) {
      // Mark as acknowledged (could add acknowledged field)
      break;
    }
  }
}

void PerformanceMonitor::ResolveAlert(const std::string& alert_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  for (auto& alert : impl_->alerts) {
    if (alert.alert_id == alert_id) {
      alert.is_resolved = true;
      alert.resolved_at = std::chrono::system_clock::now();
      break;
    }
  }
}

void PerformanceMonitor::SetAlertCallback(AlertCallback callback) {
  impl_->alert_callback = callback;
}

Status PerformanceMonitor::AnalyzeQueryPerformance(const std::string& query_hash,
                                                   std::string* analysis_report) {
  (void)query_hash;
  (void)analysis_report;
  return Status::Ok();
}

Status PerformanceMonitor::GeneratePerformanceReport(std::chrono::hours duration,
                                                     std::string* report) {
  (void)duration;
  (void)report;
  return Status::Ok();
}

std::vector<std::string> PerformanceMonitor::GetPerformanceRecommendations() const {
  return {};
}

void PerformanceMonitor::SetPerformanceBaseline() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->baseline = GetCurrentSnapshot();
  impl_->has_baseline = true;
}

void PerformanceMonitor::ClearPerformanceBaseline() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->has_baseline = false;
}

bool PerformanceMonitor::CompareToBaseline(const PerformanceSnapshot& current,
                                           std::vector<PerformanceAlert>* deviations) {
  (void)current;
  (void)deviations;
  return impl_->has_baseline;
}

void PerformanceMonitor::SetMetricsCallback(MetricsCallback callback) {
  impl_->metrics_callback = callback;
}

void PerformanceMonitor::ClearHistory() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->snapshots.clear();
  impl_->system_history.clear();
}

void PerformanceMonitor::ClearQueryMetrics() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->query_metrics.clear();
}

void PerformanceMonitor::ExportMetrics(const std::string& file_path,
                                       std::chrono::hours duration) {
  (void)file_path;
  (void)duration;
}

}  // namespace scratchrobin::core
