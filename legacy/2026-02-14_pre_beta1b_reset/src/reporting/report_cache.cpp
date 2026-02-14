// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "report_cache.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace scratchrobin {
namespace reporting {

ReportCache::ReportCache(const CacheConfig& config) : config_(config) {}

void ReportCache::SetPersistentStorage(ResultStorage* storage) {
    std::lock_guard<std::mutex> lock(mutex_);
    persistent_storage_ = storage;
}

std::optional<QueryResult> ReportCache::Get(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = entries_.find(cache_key);
    if (it == entries_.end()) {
        misses_++;
        return std::nullopt;
    }
    
    // Check if expired
    auto now = std::chrono::steady_clock::now();
    if (now > it->second.cached_at + it->second.ttl) {
        entries_.erase(it);
        misses_++;
        return std::nullopt;
    }
    
    it->second.hit_count++;
    hits_++;
    return it->second.result;
}

void ReportCache::Put(const std::string& cache_key,
                      const QueryResult& result,
                      std::optional<std::chrono::seconds> ttl) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    EvictIfNeeded();
    
    CacheEntry entry;
    entry.cache_key = cache_key;
    entry.result = result;
    entry.cached_at = std::chrono::steady_clock::now();
    entry.ttl = ttl.value_or(config_.default_ttl);
    
    entries_[cache_key] = entry;
}

void ReportCache::PutWithPersistence(const std::string& cache_key,
                                     const QueryResult& result,
                                     const std::string& question_id,
                                     const std::string& execution_id,
                                     const std::map<std::string, std::string>& parameters,
                                     std::optional<std::chrono::seconds> ttl) {
    // Store in memory cache first
    Put(cache_key, result, ttl);
    
    // Also store in persistent storage if available
    if (persistent_storage_ && config_.persist_to_storage) {
        auto handle = persistent_storage_->StoreResult(result, question_id, execution_id, 
                                                        parameters, {"auto_cached"});
        if (handle.valid) {
            std::lock_guard<std::mutex> lock(mutex_);
            persistent_stores_++;
        }
    }
}

std::string ReportCache::GenerateKey(const std::string& sql,
                                     const std::map<std::string, std::string>& params,
                                     const std::string& connection_ref,
                                     const std::string& model_version) {
    CacheKeyBuilder builder;
    builder.WithSql(sql);
    for (const auto& [name, value] : params) {
        builder.WithParameter(name, value);
    }
    builder.WithConnection(connection_ref);
    builder.WithModelVersion(model_version);
    return builder.Build();
}

void ReportCache::Invalidate(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.erase(cache_key);
}

void ReportCache::InvalidateByConnection(const std::string& connection_ref) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.connection_ref == connection_ref) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void ReportCache::InvalidateByModel(const std::string& model_ref) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.model_version.find(model_ref) != std::string::npos) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void ReportCache::InvalidateAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

std::optional<QueryResult> ReportCache::GetFromPersistentStorage(const std::string& result_id) {
    if (!persistent_storage_) return std::nullopt;
    
    auto result = persistent_storage_->RetrieveResult(result_id);
    if (result) {
        std::lock_guard<std::mutex> lock(mutex_);
        persistent_hits_++;
    }
    return result;
}

std::vector<StoredResultMetadata> ReportCache::GetPersistentHistory(const std::string& question_id,
                                                                     int limit) {
    if (!persistent_storage_) return {};
    
    HistoricalResultQuery query;
    query.question_id = question_id;
    query.limit = limit;
    
    return persistent_storage_->QueryMetadata(query);
}

std::vector<StoredResultMetadata> ReportCache::GetPersistentResultsInRange(
    const std::string& question_id,
    std::time_t from_date,
    std::time_t to_date) {
    if (!persistent_storage_) return {};
    
    HistoricalResultQuery query;
    query.question_id = question_id;
    query.from_date = from_date;
    query.to_date = to_date;
    
    return persistent_storage_->QueryMetadata(query);
}

ReportCache::Stats ReportCache::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats stats;
    stats.entries = entries_.size();
    stats.hits = hits_;
    stats.misses = misses_;
    stats.evictions = evictions_;
    stats.persistent_hits = persistent_hits_;
    stats.persistent_stores = persistent_stores_;
    return stats;
}

void ReportCache::CleanupExpired() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::steady_clock::now();
    for (auto it = entries_.begin(); it != entries_.end();) {
        if (now > it->second.cached_at + it->second.ttl) {
            it = entries_.erase(it);
        } else {
            ++it;
        }
    }
}

void ReportCache::SetConfig(const CacheConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void ReportCache::EvictIfNeeded() {
    if (entries_.size() < config_.max_entries) {
        return;
    }
    
    // Evict least recently used (lowest hit count)
    auto lru_it = std::min_element(entries_.begin(), entries_.end(),
        [](const auto& a, const auto& b) {
            return a.second.hit_count < b.second.hit_count;
        });
    
    if (lru_it != entries_.end()) {
        entries_.erase(lru_it);
        evictions_++;
    }
}

std::string ReportCache::NormalizeSql(const std::string& sql) {
    std::string result;
    result.reserve(sql.size());
    bool in_space = false;
    
    for (char c : sql) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!in_space) {
                result.push_back(' ');
                in_space = true;
            }
        } else {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            in_space = false;
        }
    }
    
    // Trim
    size_t start = result.find_first_not_of(' ');
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(' ');
    return result.substr(start, end - start + 1);
}

// CacheKeyBuilder implementation

CacheKeyBuilder& CacheKeyBuilder::WithSql(const std::string& sql) {
    sql_ = sql;
    return *this;
}

CacheKeyBuilder& CacheKeyBuilder::WithParameter(const std::string& name, 
                                                 const std::string& value) {
    params_[name] = value;
    return *this;
}

CacheKeyBuilder& CacheKeyBuilder::WithConnection(const std::string& connection_ref) {
    connection_ref_ = connection_ref;
    return *this;
}

CacheKeyBuilder& CacheKeyBuilder::WithModelVersion(const std::string& version) {
    model_version_ = version;
    return *this;
}

std::string CacheKeyBuilder::Build() const {
    std::ostringstream oss;
    
    // Hash SQL (simplified - use first 100 chars normalized)
    std::string normalized = sql_;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    // Remove extra whitespace
    bool in_space = false;
    std::string cleaned;
    for (char c : normalized) {
        if (std::isspace(c)) {
            if (!in_space) {
                cleaned += ' ';
                in_space = true;
            }
        } else {
            cleaned += c;
            in_space = false;
        }
    }
    
    oss << "q:" << std::hash<std::string>{}(cleaned.substr(0, 500));
    
    // Add parameters
    if (!params_.empty()) {
        oss << "|p:";
        for (const auto& [name, value] : params_) {
            oss << name << "=" << std::hash<std::string>{}(value) << "&";
        }
    }
    
    // Add connection
    if (!connection_ref_.empty()) {
        oss << "|c:" << connection_ref_;
    }
    
    // Add model version
    if (!model_version_.empty()) {
        oss << "|v:" << model_version_;
    }
    
    return oss.str();
}

} // namespace reporting
} // namespace scratchrobin
