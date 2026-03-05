/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/connection_pool_manager.h"

namespace scratchrobin::core {

// Private implementation structures
struct ConnectionPoolManager::Pool {
  PoolConfig config;
  std::queue<PooledConnection> idle_connections;
  std::atomic<int> active_count{0};
  std::atomic<int> total_count{0};
};

struct ConnectionPoolManager::Impl {
  std::map<std::string, std::unique_ptr<Pool>> pools;
  mutable std::mutex mutex;
  std::function<void(const std::string& pool_id,
                     const std::string& connection_id,
                     const std::string& event)> event_callback;
};

ConnectionPoolManager::ConnectionPoolManager()
    : impl_(std::make_unique<Impl>()) {
}

ConnectionPoolManager::~ConnectionPoolManager() = default;

Status ConnectionPoolManager::CreatePool(const PoolConfig& config) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  if (impl_->pools.find(config.pool_id) != impl_->pools.end()) {
    return Status::Error("Pool already exists: " + config.pool_id);
  }
  
  auto pool = std::make_unique<Pool>();
  pool->config = config;
  
  impl_->pools[config.pool_id] = std::move(pool);
  
  if (impl_->event_callback) {
    impl_->event_callback(config.pool_id, "", "created");
  }
  
  return Status::Ok();
}

Status ConnectionPoolManager::DestroyPool(const std::string& pool_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pool_id);
  }
  
  impl_->pools.erase(it);
  
  if (impl_->event_callback) {
    impl_->event_callback(pool_id, "", "destroyed");
  }
  
  return Status::Ok();
}

bool ConnectionPoolManager::PoolExists(const std::string& pool_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->pools.find(pool_id) != impl_->pools.end();
}

std::vector<std::string> ConnectionPoolManager::ListPools() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::vector<std::string> result;
  for (const auto& [id, pool] : impl_->pools) {
    (void)pool;
    result.push_back(id);
  }
  return result;
}

std::optional<PooledConnection> ConnectionPoolManager::AcquireConnection(
    const std::string& pool_id) {
  return AcquireConnection(pool_id, -1);
}

std::optional<PooledConnection> ConnectionPoolManager::AcquireConnection(
    const std::string& pool_id, int timeout_ms) {
  (void)timeout_ms;
  
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return std::nullopt;
  }
  
  auto& pool = it->second;
  
  if (!pool->idle_connections.empty()) {
    PooledConnection conn = std::move(pool->idle_connections.front());
    pool->idle_connections.pop();
    ++pool->active_count;
    conn.acquired_at = std::chrono::system_clock::now();
    
    if (impl_->event_callback) {
      impl_->event_callback(pool_id, conn.connection_id, "acquired");
    }
    
    return conn;
  }
  
  if (pool->total_count < pool->config.max_connections) {
    PooledConnection conn;
    conn.pool_id = pool_id;
    conn.connection_id = pool_id + "_" + std::to_string(++pool->total_count);
    conn.acquired_at = std::chrono::system_clock::now();
    conn.created_at = std::chrono::system_clock::now();
    ++pool->active_count;
    
    if (impl_->event_callback) {
      impl_->event_callback(pool_id, conn.connection_id, "created");
    }
    
    return conn;
  }
  
  return std::nullopt;
}

Status ConnectionPoolManager::ReleaseConnection(PooledConnection& pooled_conn) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pooled_conn.pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pooled_conn.pool_id);
  }
  
  auto& pool = it->second;
  --pool->active_count;
  ++pooled_conn.use_count;
  
  pooled_conn.connection.reset();
  pool->idle_connections.push(std::move(pooled_conn));
  
  if (impl_->event_callback) {
    impl_->event_callback(pooled_conn.pool_id, pooled_conn.connection_id, "released");
  }
  
  return Status::Ok();
}

Status ConnectionPoolManager::InvalidateConnection(PooledConnection& pooled_conn) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pooled_conn.pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pooled_conn.pool_id);
  }
  
  auto& pool = it->second;
  --pool->active_count;
  --pool->total_count;
  
  if (impl_->event_callback) {
    impl_->event_callback(pooled_conn.pool_id, pooled_conn.connection_id, "invalidated");
  }
  
  return Status::Ok();
}

Status ConnectionPoolManager::UpdatePoolConfig(const std::string& pool_id,
                                               const PoolConfig& config) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pool_id);
  }
  
  it->second->config = config;
  return Status::Ok();
}

std::optional<PoolConfig> ConnectionPoolManager::GetPoolConfig(
    const std::string& pool_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return std::nullopt;
  }
  
  return it->second->config;
}

ConnectionStats ConnectionPoolManager::GetStats(const std::string& pool_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return ConnectionStats{};
  }
  
  const auto& pool = it->second;
  ConnectionStats stats;
  stats.pool_id = pool_id;
  stats.total_connections = pool->total_count.load();
  stats.active_connections = pool->active_count.load();
  stats.idle_connections = static_cast<int>(pool->idle_connections.size());
  
  return stats;
}

std::map<std::string, ConnectionStats> ConnectionPoolManager::GetAllStats() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  std::map<std::string, ConnectionStats> result;
  for (const auto& [id, pool] : impl_->pools) {
    ConnectionStats stats;
    stats.pool_id = id;
    stats.total_connections = pool->total_count.load();
    stats.active_connections = pool->active_count.load();
    stats.idle_connections = static_cast<int>(pool->idle_connections.size());
    result[id] = stats;
  }
  return result;
}

ValidationResult ConnectionPoolManager::ValidateConnection(
    const PooledConnection& pooled_conn) {
  (void)pooled_conn;
  ValidationResult result;
  result.valid = true;
  return result;
}

Status ConnectionPoolManager::HealthCheck(const std::string& pool_id) {
  (void)pool_id;
  return Status::Ok();
}

Status ConnectionPoolManager::HealthCheckAll() {
  return Status::Ok();
}

Status ConnectionPoolManager::ResizePool(const std::string& pool_id,
                                         int new_min, int new_max) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pool_id);
  }
  
  it->second->config.min_connections = new_min;
  it->second->config.max_connections = new_max;
  
  return Status::Ok();
}

Status ConnectionPoolManager::PurgeIdleConnections(const std::string& pool_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  
  auto it = impl_->pools.find(pool_id);
  if (it == impl_->pools.end()) {
    return Status::Error("Pool not found: " + pool_id);
  }
  
  auto& pool = it->second;
  while (!pool->idle_connections.empty()) {
    pool->idle_connections.pop();
    --pool->total_count;
  }
  
  return Status::Ok();
}

Status ConnectionPoolManager::RefreshAllConnections(const std::string& pool_id) {
  (void)pool_id;
  return Status::Ok();
}

Status ConnectionPoolManager::EmergencyShutdown(const std::string& pool_id) {
  return DestroyPool(pool_id);
}

Status ConnectionPoolManager::EmergencyShutdownAll() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->pools.clear();
  return Status::Ok();
}

Status ConnectionPoolManager::RestartPool(const std::string& pool_id) {
  auto config = GetPoolConfig(pool_id);
  if (!config) {
    return Status::Error("Pool not found: " + pool_id);
  }
  
  Status status = DestroyPool(pool_id);
  if (!status.ok) {
    return status;
  }
  
  return CreatePool(*config);
}

void ConnectionPoolManager::SetEventCallback(ConnectionEventCallback callback) {
  impl_->event_callback = callback;
}

}  // namespace scratchrobin::core
