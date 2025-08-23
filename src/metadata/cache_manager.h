#ifndef SCRATCHROBIN_CACHE_MANAGER_H
#define SCRATCHROBIN_CACHE_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include "types/result.h"

namespace scratchrobin {

enum class CacheLevel {
    L1_MEMORY,     // Fast in-memory cache
    L2_DISK,       // Persistent disk cache
    L3_DISTRIBUTED // Distributed cache (future)
};

enum class CacheInvalidationStrategy {
    TIME_BASED,        // Invalidate based on time
    SIZE_BASED,        // Invalidate based on size
    ACCESS_BASED,      // Invalidate based on access patterns
    VERSION_BASED,     // Invalidate based on version changes
    MANUAL,            // Manual invalidation only
    HYBRID             // Combination of strategies
};

enum class CacheEvictionPolicy {
    LRU,               // Least Recently Used
    LFU,               // Least Frequently Used
    FIFO,              // First In, First Out
    RANDOM,            // Random eviction
    SIZE_BASED,        // Evict largest items first
    TTL_BASED          // Time To Live based eviction
};

struct CacheConfiguration {
    CacheLevel level = CacheLevel::L1_MEMORY;
    std::string cacheDirectory = "./cache";
    std::size_t maxMemorySize = 100 * 1024 * 1024; // 100MB
    std::size_t maxDiskSize = 1024 * 1024 * 1024;  // 1GB
    std::chrono::seconds defaultTTL{3600};         // 1 hour
    CacheEvictionPolicy evictionPolicy = CacheEvictionPolicy::LRU;
    CacheInvalidationStrategy invalidationStrategy = CacheInvalidationStrategy::HYBRID;
    bool enableCompression = true;
    bool enableEncryption = false;
    int maxConcurrency = 10;
    std::chrono::milliseconds cleanupInterval{300000}; // 5 minutes
    bool enableMetrics = true;
};

struct CacheItem {
    std::string key;
    std::vector<char> data;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccessed;
    std::chrono::system_clock::time_point expiresAt;
    std::size_t accessCount = 0;
    std::size_t dataSize = 0;
    std::string etag;
    std::string contentType;
    CacheLevel level;
    bool isCompressed = false;
    bool isEncrypted = false;
};

struct CacheMetrics {
    std::atomic<std::size_t> totalRequests{0};
    std::atomic<std::size_t> cacheHits{0};
    std::atomic<std::size_t> cacheMisses{0};
    std::atomic<std::size_t> evictions{0};
    std::atomic<std::size_t> invalidations{0};
    std::atomic<std::size_t> memoryUsage{0};
    std::atomic<std::size_t> diskUsage{0};
    std::atomic<std::size_t> totalItems{0};
    std::atomic<std::chrono::milliseconds::rep> averageAccessTime{0};
    std::atomic<std::chrono::milliseconds::rep> averageWriteTime{0};
    std::atomic<std::chrono::system_clock::time_point::rep> lastUpdated{0};

    // Copy constructor and assignment (atomic-safe)
    CacheMetrics(const CacheMetrics& other) {
        totalRequests.store(other.totalRequests.load());
        cacheHits.store(other.cacheHits.load());
        cacheMisses.store(other.cacheMisses.load());
        evictions.store(other.evictions.load());
        invalidations.store(other.invalidations.load());
        memoryUsage.store(other.memoryUsage.load());
        diskUsage.store(other.diskUsage.load());
        totalItems.store(other.totalItems.load());
        averageAccessTime.store(other.averageAccessTime.load());
        averageWriteTime.store(other.averageWriteTime.load());
        lastUpdated.store(other.lastUpdated.load());
    }

    CacheMetrics& operator=(const CacheMetrics& other) {
        if (this != &other) {
            totalRequests.store(other.totalRequests.load());
            cacheHits.store(other.cacheHits.load());
            cacheMisses.store(other.cacheMisses.load());
            evictions.store(other.evictions.load());
            invalidations.store(other.invalidations.load());
            memoryUsage.store(other.memoryUsage.load());
            diskUsage.store(other.diskUsage.load());
            totalItems.store(other.totalItems.load());
            averageAccessTime.store(other.averageAccessTime.load());
            averageWriteTime.store(other.averageWriteTime.load());
            lastUpdated.store(other.lastUpdated.load());
        }
        return *this;
    }

    // Default constructor and destructor
    CacheMetrics() = default;
    ~CacheMetrics() = default;
};

struct CacheEntryMetadata {
    std::string key;
    std::size_t size = 0;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccessed;
    std::chrono::system_clock::time_point expiresAt;
    std::size_t accessCount = 0;
    std::string etag;
    CacheLevel level;
};

class ICacheManager {
public:
    virtual ~ICacheManager() = default;

    virtual Result<void> initialize(const CacheConfiguration& config) = 0;
    virtual Result<void> shutdown() = 0;

    virtual Result<bool> put(const std::string& key, const std::vector<char>& data,
                           const std::chrono::seconds& ttl = std::chrono::seconds(3600),
                           const std::string& etag = "") = 0;

    virtual Result<std::vector<char>> get(const std::string& key) = 0;
    virtual Result<bool> exists(const std::string& key) = 0;
    virtual Result<bool> remove(const std::string& key) = 0;
    virtual Result<void> clear() = 0;

    virtual Result<bool> invalidate(const std::string& key) = 0;
    virtual Result<void> invalidatePattern(const std::string& pattern) = 0;
    virtual Result<void> invalidateByAge(const std::chrono::seconds& maxAge) = 0;
    virtual Result<void> invalidateBySize(std::size_t maxSize) = 0;

    virtual Result<CacheEntryMetadata> getMetadata(const std::string& key) = 0;
    virtual Result<std::vector<CacheEntryMetadata>> listEntries(const std::string& pattern = "") = 0;

    virtual Result<void> setTTL(const std::string& key, const std::chrono::seconds& ttl) = 0;
    virtual Result<std::chrono::seconds> getTTL(const std::string& key) = 0;

    virtual CacheMetrics getMetrics() const = 0;
    virtual Result<void> resetMetrics() = 0;

    virtual Result<void> cleanup() = 0;
    virtual Result<void> optimize() = 0;

    using CacheEventCallback = std::function<void(const std::string&, const std::string&)>;
    virtual void setCacheEventCallback(CacheEventCallback callback) = 0;
};

class CacheManager : public ICacheManager {
public:
    CacheManager();
    ~CacheManager() override;

    Result<void> initialize(const CacheConfiguration& config) override;
    Result<void> shutdown() override;

    Result<bool> put(const std::string& key, const std::vector<char>& data,
                   const std::chrono::seconds& ttl = std::chrono::seconds(3600),
                   const std::string& etag = "") override;

    Result<std::vector<char>> get(const std::string& key) override;
    Result<bool> exists(const std::string& key) override;
    Result<bool> remove(const std::string& key) override;
    Result<void> clear() override;

    Result<bool> invalidate(const std::string& key) override;
    Result<void> invalidatePattern(const std::string& pattern) override;
    Result<void> invalidateByAge(const std::chrono::seconds& maxAge) override;
    Result<void> invalidateBySize(std::size_t maxSize) override;

    Result<CacheEntryMetadata> getMetadata(const std::string& key) override;
    Result<std::vector<CacheEntryMetadata>> listEntries(const std::string& pattern = "") override;

    Result<void> setTTL(const std::string& key, const std::chrono::seconds& ttl) override;
    Result<std::chrono::seconds> getTTL(const std::string& key) override;

    CacheMetrics getMetrics() const override;
    Result<void> resetMetrics() override;

    Result<void> cleanup() override;
    Result<void> optimize() override;

    void setCacheEventCallback(CacheEventCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    CacheConfiguration config_;
    CacheMetrics metrics_;
    CacheEventCallback eventCallback_;

    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, CacheItem> memoryCache_;
    std::unordered_map<std::string, CacheEntryMetadata> metadataCache_;

    // Cache management threads
    std::atomic<bool> running_{false};
    std::thread cleanupThread_;
    std::thread metricsThread_;
    std::condition_variable cleanupCondition_;
    mutable std::mutex cleanupMutex_;

    // Helper methods
    Result<void> initializeMemoryCache();
    Result<void> initializeDiskCache();
    Result<void> initializeDistributedCache();

    Result<bool> putMemory(const std::string& key, const std::vector<char>& data,
                         const std::chrono::seconds& ttl, const std::string& etag);
    Result<bool> putDisk(const std::string& key, const std::vector<char>& data,
                       const std::chrono::seconds& ttl, const std::string& etag);

    Result<std::vector<char>> getMemory(const std::string& key);
    Result<std::vector<char>> getDisk(const std::string& key);

    Result<bool> existsMemory(const std::string& key);
    Result<bool> existsDisk(const std::string& key);

    void evictMemoryItems();
    void evictDiskItems();

    std::string generateCacheFilePath(const std::string& key);
    std::string generateETag(const std::vector<char>& data);

    bool isExpired(const CacheItem& item);
    bool shouldEvict(const CacheItem& item);

    void updateMetrics(const std::string& operation, bool success,
                      std::chrono::milliseconds duration);

    void cleanupThreadFunction();
    void metricsThreadFunction();

    void performLRUEviction();
    void performLFUEviction();
    void performFIFOEciction();
    void performRandomEviction();
    void performSizeBasedEviction();
    void performTTLBasedEviction();

    void compressData(std::vector<char>& data);
    void decompressData(std::vector<char>& data);
    void encryptData(std::vector<char>& data);
    void decryptData(std::vector<char>& data);

    std::vector<std::string> findKeysByPattern(const std::string& pattern);
    void invalidateKeysByAge(const std::chrono::seconds& maxAge);
    void invalidateKeysBySize(std::size_t maxSize);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CACHE_MANAGER_H
