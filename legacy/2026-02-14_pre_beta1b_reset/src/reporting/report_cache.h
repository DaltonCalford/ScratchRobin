// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "report_types.h"
#include "result_storage.h"
#include "../core/query_types.h"

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace scratchrobin {
namespace reporting {

/**
 * @brief Cache entry for query results
 */
struct CacheEntry {
    std::string cache_key;
    QueryResult result;
    std::chrono::steady_clock::time_point cached_at;
    std::chrono::seconds ttl;
    int hit_count = 0;
    std::string connection_ref;
    std::string model_version;  // For cache invalidation on model changes
};

/**
 * @brief Cache configuration
 */
struct CacheConfig {
    std::chrono::seconds default_ttl{15 * 60};       // 15 minutes
    std::chrono::seconds dashboard_ttl{30 * 60};     // 30 minutes
    std::chrono::seconds alert_ttl{0};               // Alerts bypass cache
    size_t max_entries = 1000;
    bool persistent_cache = false;  // Use ResultStorage for persistence
    
    // Persistence settings
    bool persist_to_storage = false;     // Enable long-term result storage
    uint32_t storage_retention_days = 90; // How long to keep in persistent storage
    std::string storage_question_id;     // Question ID for stored results
};

/**
 * @brief Query result cache for reporting
 * 
 * Thread-safe cache for storing query results with TTL-based expiration.
 * Supports optional persistence to ResultStorage for long-term retention.
 */
class ReportCache {
public:
    explicit ReportCache(const CacheConfig& config = CacheConfig{});
    
    // Initialize with persistent storage
    void SetPersistentStorage(ResultStorage* storage);
    
    // Get cached result if available and not expired
    std::optional<QueryResult> Get(const std::string& cache_key);
    
    // Store result in cache
    void Put(const std::string& cache_key, 
             const QueryResult& result,
             std::optional<std::chrono::seconds> ttl = std::nullopt);
    
    // Store result in cache AND persistent storage
    void PutWithPersistence(const std::string& cache_key,
                            const QueryResult& result,
                            const std::string& question_id,
                            const std::string& execution_id,
                            const std::map<std::string, std::string>& parameters,
                            std::optional<std::chrono::seconds> ttl = std::nullopt);
    
    // Generate cache key from query components
    std::string GenerateKey(const std::string& sql,
                           const std::map<std::string, std::string>& params,
                           const std::string& connection_ref,
                           const std::string& model_version);
    
    // Invalidation
    void Invalidate(const std::string& cache_key);
    void InvalidateByConnection(const std::string& connection_ref);
    void InvalidateByModel(const std::string& model_ref);
    void InvalidateAll();
    
    // Retrieve from persistent storage (bypasses memory cache)
    std::optional<QueryResult> GetFromPersistentStorage(const std::string& result_id);
    std::vector<StoredResultMetadata> GetPersistentHistory(const std::string& question_id,
                                                            int limit = 100);
    
    // Query persistent storage by time range
    std::vector<StoredResultMetadata> GetPersistentResultsInRange(
        const std::string& question_id,
        std::time_t from_date,
        std::time_t to_date);
    
    // Cache statistics
    struct Stats {
        size_t entries;
        size_t hits;
        size_t misses;
        size_t evictions;
        size_t persistent_hits;  // Results retrieved from persistent storage
        size_t persistent_stores; // Results saved to persistent storage
    };
    Stats GetStats() const;
    
    // Maintenance
    void CleanupExpired();
    void SetConfig(const CacheConfig& config);
    
private:
    mutable std::mutex mutex_;
    CacheConfig config_;
    std::map<std::string, CacheEntry> entries_;
    ResultStorage* persistent_storage_ = nullptr;
    
    // Statistics
    size_t hits_ = 0;
    size_t misses_ = 0;
    size_t evictions_ = 0;
    size_t persistent_hits_ = 0;
    size_t persistent_stores_ = 0;
    
    void EvictIfNeeded();
    std::string NormalizeSql(const std::string& sql);
};

/**
 * @brief Cache key builder helper
 */
class CacheKeyBuilder {
public:
    CacheKeyBuilder& WithSql(const std::string& sql);
    CacheKeyBuilder& WithParameter(const std::string& name, const std::string& value);
    CacheKeyBuilder& WithConnection(const std::string& connection_ref);
    CacheKeyBuilder& WithModelVersion(const std::string& version);
    
    std::string Build() const;
    
private:
    std::string sql_;
    std::map<std::string, std::string> params_;
    std::string connection_ref_;
    std::string model_version_;
};

} // namespace reporting
} // namespace scratchrobin
