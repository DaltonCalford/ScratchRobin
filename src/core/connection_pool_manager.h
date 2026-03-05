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
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Connection pool configuration
struct PoolConfig {
  std::string pool_id;
  std::string connection_string;
  std::string username;
  std::string password;
  int min_connections{2};
  int max_connections{20};
  int max_idle_time_seconds{300};
  int max_lifetime_seconds{3600};
  int connection_timeout_ms{30000};
  int acquire_timeout_ms{10000};
  bool test_on_borrow{true};
  bool test_on_return{false};
  std::string test_query{"SELECT 1"};
  int idle_check_interval_seconds{60};
  int retry_attempts{3};
  int retry_delay_ms{1000};
};

// Connection statistics
struct ConnectionStats {
  std::string pool_id;
  int total_connections{0};
  int active_connections{0};
  int idle_connections{0};
  int waiting_requests{0};
  int64_t total_requests{0};
  int64_t failed_requests{0};
  int64_t average_wait_time_ms{0};
  int64_t average_use_time_ms{0};
  std::chrono::system_clock::time_point created_at;
  std::chrono::system_clock::time_point last_activity;
};

// Pooled connection wrapper
struct PooledConnection {
  std::string pool_id;
  std::string connection_id;
  std::shared_ptr<Connection> connection;
  std::chrono::system_clock::time_point acquired_at;
  std::chrono::system_clock::time_point created_at;
  int use_count{0};
};

// Connection validation result
struct ValidationResult {
  bool valid{false};
  std::string error_message;
  int response_time_ms{0};
};

// ============================================================================
// ConnectionPoolManager
// ============================================================================

class ConnectionPoolManager {
 public:
  ConnectionPoolManager();
  ~ConnectionPoolManager();

  // Disable copy
  ConnectionPoolManager(const ConnectionPoolManager&) = delete;
  ConnectionPoolManager& operator=(const ConnectionPoolManager&) = delete;

  // Pool lifecycle
  Status CreatePool(const PoolConfig& config);
  Status DestroyPool(const std::string& pool_id);
  bool PoolExists(const std::string& pool_id) const;
  std::vector<std::string> ListPools() const;
  
  // Connection acquisition
  std::optional<PooledConnection> AcquireConnection(const std::string& pool_id);
  std::optional<PooledConnection> AcquireConnection(const std::string& pool_id,
                                                    int timeout_ms);
  Status ReleaseConnection(PooledConnection& pooled_conn);
  Status InvalidateConnection(PooledConnection& pooled_conn);
  
  // Pool configuration
  Status UpdatePoolConfig(const std::string& pool_id, const PoolConfig& config);
  std::optional<PoolConfig> GetPoolConfig(const std::string& pool_id) const;
  
  // Statistics and monitoring
  ConnectionStats GetStats(const std::string& pool_id) const;
  std::map<std::string, ConnectionStats> GetAllStats() const;
  
  // Health checks
  ValidationResult ValidateConnection(const PooledConnection& pooled_conn);
  Status HealthCheck(const std::string& pool_id);
  Status HealthCheckAll();
  
  // Pool maintenance
  Status ResizePool(const std::string& pool_id, int new_min, int new_max);
  Status PurgeIdleConnections(const std::string& pool_id);
  Status RefreshAllConnections(const std::string& pool_id);
  
  // Emergency operations
  Status EmergencyShutdown(const std::string& pool_id);
  Status EmergencyShutdownAll();
  Status RestartPool(const std::string& pool_id);

  // Event callbacks
  using ConnectionEventCallback = 
      std::function<void(const std::string& pool_id,
                         const std::string& connection_id,
                         const std::string& event)>;
  void SetEventCallback(ConnectionEventCallback callback);

 private:
  struct Pool;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
