#include "metadata/cache_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <openssl/evp.h>
#include <zlib.h>

namespace scratchrobin {

class CacheManager::Impl {
public:
    Impl() = default;

    Result<void> initializeMemoryCache(const CacheConfiguration& config) {
        try {
            // Initialize memory cache structures
            memoryCache_.clear();
            metadataCache_.clear();
            memoryUsage_ = 0;
            totalItems_ = 0;

            return Result<void>::success();
        } catch (const std::exception& e) {
            return Result<void>::failure(std::string("Failed to initialize memory cache: ") + e.what());
        }
    }

    Result<void> initializeDiskCache(const CacheConfiguration& config) {
        try {
            // Create cache directory if it doesn't exist
            std::filesystem::create_directories(config.cacheDirectory);

            // Clean up old cache files
            cleanupDiskCache(config.cacheDirectory);

            return Result<void>::success();
        } catch (const std::exception& e) {
            return Result<void>::failure(std::string("Failed to initialize disk cache: ") + e.what());
        }
    }

    Result<void> initializeDistributedCache(const CacheConfiguration& config) {
        // Placeholder for distributed cache initialization
        // This would initialize connections to Redis, Memcached, etc.
        return Result<void>::failure("Distributed cache not yet implemented");
    }

    Result<bool> putMemory(const std::string& key, const std::vector<char>& data,
                         const std::chrono::seconds& ttl, const std::string& etag) {
        try {
            auto now = std::chrono::system_clock::now();

            CacheItem item;
            item.key = key;
            item.data = data;
            item.createdAt = now;
            item.lastAccessed = now;
            item.expiresAt = now + ttl;
            item.accessCount = 0;
            item.dataSize = data.size();
            item.etag = etag.empty() ? generateETag(data) : etag;
            item.level = CacheLevel::L1_MEMORY;
            item.isCompressed = false;
            item.isEncrypted = false;

            // Check if we need to evict items to make space
            std::size_t requiredSize = data.size();
            if (memoryUsage_ + requiredSize > config_.maxMemorySize) {
                evictMemoryItems(requiredSize);
            }

            // Store the item
            memoryCache_[key] = item;
            memoryUsage_ += requiredSize;
            totalItems_++;

            // Update metadata
            CacheEntryMetadata metadata;
            metadata.key = key;
            metadata.size = requiredSize;
            metadata.createdAt = now;
            metadata.lastAccessed = now;
            metadata.expiresAt = item.expiresAt;
            metadata.accessCount = 0;
            metadata.etag = item.etag;
            metadata.level = CacheLevel::L1_MEMORY;

            metadataCache_[key] = metadata;

            return Result<bool>::success(true);

        } catch (const std::exception& e) {
            return Result<bool>::failure(std::string("Failed to put item in memory cache: ") + e.what());
        }
    }

    Result<bool> putDisk(const std::string& key, const std::vector<char>& data,
                       const std::chrono::seconds& ttl, const std::string& etag) {
        try {
            std::string filePath = generateCacheFilePath(key);

            // Create cache file
            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                return Result<bool>::failure("Failed to create cache file: " + filePath);
            }

            // Write metadata header
            std::string header = "SCRATCHROBIN_CACHE_V1\n";
            header += std::to_string(data.size()) + "\n";
            header += std::to_string(ttl.count()) + "\n";
            header += etag + "\n";
            header += "---END_HEADER---\n";

            file.write(header.c_str(), header.size());
            file.write(data.data(), data.size());
            file.close();

            // Update disk usage
            diskUsage_ += header.size() + data.size();

            return Result<bool>::success(true);

        } catch (const std::exception& e) {
            return Result<bool>::failure(std::string("Failed to put item in disk cache: ") + e.what());
        }
    }

    Result<std::vector<char>> getMemory(const std::string& key) {
        auto it = memoryCache_.find(key);
        if (it == memoryCache_.end()) {
            return Result<std::vector<char>>::failure("Item not found in memory cache");
        }

        auto& item = it->second;

        // Check if item has expired
        if (isExpired(item)) {
            memoryCache_.erase(it);
            return Result<std::vector<char>>::failure("Item has expired");
        }

        // Update access statistics
        item.lastAccessed = std::chrono::system_clock::now();
        item.accessCount++;

        // Update metadata
        if (metadataCache_.count(key)) {
            metadataCache_[key].lastAccessed = item.lastAccessed;
            metadataCache_[key].accessCount = item.accessCount;
        }

        return Result<std::vector<char>>::success(item.data);
    }

    Result<std::vector<char>> getDisk(const std::string& key) {
        try {
            std::string filePath = generateCacheFilePath(key);

            if (!std::filesystem::exists(filePath)) {
                return Result<std::vector<char>>::failure("Cache file not found: " + filePath);
            }

            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                return Result<std::vector<char>>::failure("Failed to open cache file: " + filePath);
            }

            // Read header
            std::string line;
            std::getline(file, line); // Version
            if (line != "SCRATCHROBIN_CACHE_V1") {
                return Result<std::vector<char>>::failure("Invalid cache file format");
            }

            std::getline(file, line); // Data size
            std::size_t dataSize = std::stoul(line);

            std::getline(file, line); // TTL
            std::chrono::seconds ttl(std::stoul(line));

            std::getline(file, line); // ETag
            std::string etag = line;

            std::getline(file, line); // End header
            if (line != "---END_HEADER---") {
                return Result<std::vector<char>>::failure("Invalid cache file header");
            }

            // Read data
            std::vector<char> data(dataSize);
            file.read(data.data(), dataSize);

            // Check if file has expired
            auto fileTime = std::filesystem::last_write_time(filePath);
            auto now = std::chrono::system_clock::now();
            auto age = now - std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime);

            if (age > ttl) {
                std::filesystem::remove(filePath);
                return Result<std::vector<char>>::failure("Cache file has expired");
            }

            return Result<std::vector<char>>::success(data);

        } catch (const std::exception& e) {
            return Result<std::vector<char>>::failure(std::string("Failed to get item from disk cache: ") + e.what());
        }
    }

    void evictMemoryItems(std::size_t requiredSize = 0) {
        switch (config_.evictionPolicy) {
            case CacheEvictionPolicy::LRU:
                performLRUEviction(requiredSize);
                break;
            case CacheEvictionPolicy::LFU:
                performLFUEviction(requiredSize);
                break;
            case CacheEvictionPolicy::FIFO:
                performFIFOEciction(requiredSize);
                break;
            case CacheEvictionPolicy::RANDOM:
                performRandomEviction(requiredSize);
                break;
            case CacheEvictionPolicy::SIZE_BASED:
                performSizeBasedEviction(requiredSize);
                break;
            case CacheEvictionPolicy::TTL_BASED:
                performTTLBasedEviction(requiredSize);
                break;
        }
    }

    void performLRUEviction(std::size_t requiredSize) {
        // Find the least recently used items
        std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> items;

        for (const auto& [key, item] : memoryCache_) {
            items.emplace_back(key, item.lastAccessed);
        }

        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });

        std::size_t freedSize = 0;
        for (const auto& [key, _] : items) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    void performLFUEviction(std::size_t requiredSize) {
        // Find the least frequently used items
        std::vector<std::pair<std::string, std::size_t>> items;

        for (const auto& [key, item] : memoryCache_) {
            items.emplace_back(key, item.accessCount);
        }

        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });

        std::size_t freedSize = 0;
        for (const auto& [key, _] : items) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    void performFIFOEciction(std::size_t requiredSize) {
        // Find the oldest items (first in)
        std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> items;

        for (const auto& [key, item] : memoryCache_) {
            items.emplace_back(key, item.createdAt);
        }

        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });

        std::size_t freedSize = 0;
        for (const auto& [key, _] : items) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    void performRandomEviction(std::size_t requiredSize) {
        std::vector<std::string> keys;
        for (const auto& [key, _] : memoryCache_) {
            keys.push_back(key);
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(keys.begin(), keys.end(), gen);

        std::size_t freedSize = 0;
        for (const auto& key : keys) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    void performSizeBasedEviction(std::size_t requiredSize) {
        // Find the largest items
        std::vector<std::pair<std::string, std::size_t>> items;

        for (const auto& [key, item] : memoryCache_) {
            items.emplace_back(key, item.dataSize);
        }

        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });

        std::size_t freedSize = 0;
        for (const auto& [key, _] : items) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    void performTTLBasedEviction(std::size_t requiredSize) {
        // Find expired items first, then closest to expiration
        auto now = std::chrono::system_clock::now();
        std::vector<std::pair<std::string, std::chrono::seconds>> items;

        for (const auto& [key, item] : memoryCache_) {
            auto timeToExpiry = std::chrono::duration_cast<std::chrono::seconds>(item.expiresAt - now);
            items.emplace_back(key, timeToExpiry);
        }

        std::sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });

        std::size_t freedSize = 0;
        for (const auto& [key, _] : items) {
            if (memoryCache_.count(key)) {
                auto& item = memoryCache_[key];
                freedSize += item.dataSize;
                memoryCache_.erase(key);
                metadataCache_.erase(key);
                totalItems_--;
                evictions_++;

                if (requiredSize > 0 && freedSize >= requiredSize) {
                    break;
                }
            }
        }

        memoryUsage_ -= freedSize;
    }

    std::string generateCacheFilePath(const std::string& key) {
        // Create a hash of the key for the filename
        std::hash<std::string> hasher;
        std::size_t hash = hasher(key);

        std::stringstream ss;
        ss << config_.cacheDirectory << "/cache_" << std::hex << hash << ".dat";
        return ss.str();
    }

    std::string generateETag(const std::vector<char>& data) {
        // Generate a simple ETag using hash of data
        std::hash<std::string> hasher;
        std::string dataStr(data.begin(), data.end());
        std::size_t hash = hasher(dataStr);

        std::stringstream ss;
        ss << "\"" << std::hex << hash << "\"";
        return ss.str();
    }

    bool isExpired(const CacheItem& item) {
        return std::chrono::system_clock::now() > item.expiresAt;
    }

    void compressData(std::vector<char>& data) {
        if (!config_.enableCompression || data.empty()) {
            return;
        }

        uLongf compressedSize = compressBound(data.size());
        std::vector<char> compressed(compressedSize);

        if (compress2(reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
                     reinterpret_cast<const Bytef*>(data.data()), data.size(), Z_BEST_COMPRESSION) == Z_OK) {
            compressed.resize(compressedSize);
            data = compressed;
        }
    }

    void decompressData(std::vector<char>& data) {
        if (!config_.enableCompression || data.empty()) {
            return;
        }

        // For decompression, we would need the original size
        // This is a simplified implementation
        uLongf decompressedSize = data.size() * 4; // Estimate
        std::vector<char> decompressed(decompressedSize);

        if (uncompress2(reinterpret_cast<Bytef*>(decompressed.data()), &decompressedSize,
                       reinterpret_cast<const Bytef*>(data.data()), data.size()) == Z_OK) {
            decompressed.resize(decompressedSize);
            data = decompressed;
        }
    }

    void cleanupDiskCache(const std::string& cacheDir) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(cacheDir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                    // Check if file is older than 24 hours
                    auto fileTime = entry.last_write_time();
                    auto now = std::filesystem::file_time_type::clock::now();
                    auto age = now - fileTime;

                    if (age > std::chrono::hours(24)) {
                        std::filesystem::remove(entry.path());
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error cleaning up disk cache: " << e.what() << std::endl;
        }
    }

    std::vector<std::string> findKeysByPattern(const std::string& pattern) {
        std::vector<std::string> matchingKeys;

        // Simple pattern matching using std::regex
        try {
            std::regex regexPattern(pattern);

            for (const auto& [key, _] : memoryCache_) {
                if (std::regex_search(key, regexPattern)) {
                    matchingKeys.push_back(key);
                }
            }
        } catch (const std::regex_error& e) {
            // If regex is invalid, treat pattern as literal string
            for (const auto& [key, _] : memoryCache_) {
                if (key.find(pattern) != std::string::npos) {
                    matchingKeys.push_back(key);
                }
            }
        }

        return matchingKeys;
    }

private:
    CacheConfiguration config_;

    // Memory cache
    std::unordered_map<std::string, CacheItem> memoryCache_;
    std::unordered_map<std::string, CacheEntryMetadata> metadataCache_;

    // Statistics
    std::atomic<std::size_t> memoryUsage_{0};
    std::atomic<std::size_t> diskUsage_{0};
    std::atomic<std::size_t> totalItems_{0};
    std::atomic<std::size_t> evictions_{0};
};

// CacheManager implementation

CacheManager::CacheManager() = default;

CacheManager::~CacheManager() {
    if (running_) {
        shutdown();
    }
}

Result<void> CacheManager::initialize(const CacheConfiguration& config) {
    config_ = config;

    try {
        // Initialize appropriate cache level
        switch (config.level) {
            case CacheLevel::L1_MEMORY:
                if (auto result = impl_->initializeMemoryCache(config); !result.success()) {
                    return result;
                }
                break;
            case CacheLevel::L2_DISK:
                if (auto result = impl_->initializeDiskCache(config); !result.success()) {
                    return result;
                }
                break;
            case CacheLevel::L3_DISTRIBUTED:
                if (auto result = impl_->initializeDistributedCache(config); !result.success()) {
                    return result;
                }
                break;
        }

        // Start background threads
        running_ = true;
        cleanupThread_ = std::thread(&CacheManager::cleanupThreadFunction, this);
        metricsThread_ = std::thread(&CacheManager::metricsThreadFunction, this);

        return Result<void>::success();

    } catch (const std::exception& e) {
        return Result<void>::failure(std::string("Failed to initialize cache manager: ") + e.what());
    }
}

Result<void> CacheManager::shutdown() {
    try {
        running_ = false;
        cleanupCondition_.notify_all();

        if (cleanupThread_.joinable()) {
            cleanupThread_.join();
        }
        if (metricsThread_.joinable()) {
            metricsThread_.join();
        }

        // Clear caches
        clear();

        return Result<void>::success();

    } catch (const std::exception& e) {
        return Result<void>::failure(std::string("Failed to shutdown cache manager: ") + e.what());
    }
}

Result<bool> CacheManager::put(const std::string& key, const std::vector<char>& data,
                             const std::chrono::seconds& ttl, const std::string& etag) {
    if (key.empty()) {
        return Result<bool>::failure("Cache key cannot be empty");
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        bool success = false;

        // Try to store in memory cache first
        if (config_.level == CacheLevel::L1_MEMORY || config_.level == CacheLevel::L2_DISK) {
            auto result = impl_->putMemory(key, data, ttl, etag);
            if (result.success()) {
                success = true;
            }
        }

        // If memory cache failed or disk cache is enabled, try disk cache
        if (!success && config_.level == CacheLevel::L2_DISK) {
            auto result = impl_->putDisk(key, data, ttl, etag);
            if (result.success()) {
                success = true;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        updateMetrics("put", success, duration);

        if (eventCallback_) {
            eventCallback_(key, success ? "PUT_SUCCESS" : "PUT_FAILED");
        }

        return Result<bool>::success(success);

    } catch (const std::exception& e) {
        return Result<bool>::failure(std::string("Failed to put cache item: ") + e.what());
    }
}

Result<std::vector<char>> CacheManager::get(const std::string& key) {
    if (key.empty()) {
        return Result<std::vector<char>>::failure("Cache key cannot be empty");
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        // Try memory cache first
        if (config_.level == CacheLevel::L1_MEMORY || config_.level == CacheLevel::L2_DISK) {
            auto result = impl_->getMemory(key);
            if (result.success()) {
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

                updateMetrics("get", true, duration);
                metrics_.cacheHits++;

                if (eventCallback_) {
                    eventCallback_(key, "GET_SUCCESS_MEMORY");
                }

                return result;
            }
        }

        // Try disk cache if memory cache failed
        if (config_.level == CacheLevel::L2_DISK) {
            auto result = impl_->getDisk(key);
            if (result.success()) {
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

                updateMetrics("get", true, duration);
                metrics_.cacheHits++;

                if (eventCallback_) {
                    eventCallback_(key, "GET_SUCCESS_DISK");
                }

                return result;
            }
        }

        // Cache miss
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        updateMetrics("get", false, duration);
        metrics_.cacheMisses++;

        if (eventCallback_) {
            eventCallback_(key, "GET_MISS");
        }

        return Result<std::vector<char>>::failure("Cache item not found");

    } catch (const std::exception& e) {
        return Result<std::vector<char>>::failure(std::string("Failed to get cache item: ") + e.what());
    }
}

Result<bool> CacheManager::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    // Check memory cache
    if (config_.level == CacheLevel::L1_MEMORY || config_.level == CacheLevel::L2_DISK) {
        auto result = impl_->existsMemory(key);
        if (result.success() && result.value()) {
            return Result<bool>::success(true);
        }
    }

    // Check disk cache
    if (config_.level == CacheLevel::L2_DISK) {
        auto result = impl_->existsDisk(key);
        if (result.success() && result.value()) {
            return Result<bool>::success(true);
        }
    }

    return Result<bool>::success(false);
}

Result<bool> CacheManager::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    bool removed = false;

    // Remove from memory cache
    if (memoryCache_.count(key)) {
        memoryCache_.erase(key);
        metadataCache_.erase(key);
        removed = true;
    }

    // Remove from disk cache
    if (config_.level == CacheLevel::L2_DISK) {
        std::string filePath = impl_->generateCacheFilePath(key);
        if (std::filesystem::exists(filePath)) {
            std::filesystem::remove(filePath);
            removed = true;
        }
    }

    if (removed && eventCallback_) {
        eventCallback_(key, "REMOVED");
    }

    return Result<bool>::success(removed);
}

Result<void> CacheManager::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    // Clear memory cache
    memoryCache_.clear();
    metadataCache_.clear();

    // Clear disk cache
    if (config_.level == CacheLevel::L2_DISK) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(config_.cacheDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                    std::filesystem::remove(entry.path());
                }
            }
        } catch (const std::exception& e) {
            return Result<void>::failure(std::string("Failed to clear disk cache: ") + e.what());
        }
    }

    if (eventCallback_) {
        eventCallback_("*", "CLEARED");
    }

    return Result<void>::success();
}

Result<bool> CacheManager::invalidate(const std::string& key) {
    return remove(key);
}

Result<void> CacheManager::invalidatePattern(const std::string& pattern) {
    auto matchingKeys = impl_->findKeysByPattern(pattern);

    for (const auto& key : matchingKeys) {
        invalidate(key);
    }

    return Result<void>::success();
}

Result<void> CacheManager::invalidateByAge(const std::chrono::seconds& maxAge) {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - maxAge;

    std::vector<std::string> keysToRemove;

    // Find expired items in memory cache
    for (const auto& [key, item] : memoryCache_) {
        if (item.createdAt < cutoff) {
            keysToRemove.push_back(key);
        }
    }

    // Remove expired items
    for (const auto& key : keysToRemove) {
        remove(key);
    }

    return Result<void>::success();
}

Result<void> CacheManager::invalidateBySize(std::size_t maxSize) {
    if (memoryUsage_ <= maxSize) {
        return Result<void>::success();
    }

    std::size_t sizeToFree = memoryUsage_ - maxSize;
    impl_->evictMemoryItems(sizeToFree);

    return Result<void>::success();
}

Result<CacheEntryMetadata> CacheManager::getMetadata(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    auto it = metadataCache_.find(key);
    if (it != metadataCache_.end()) {
        return Result<CacheEntryMetadata>::success(it->second);
    }

    return Result<CacheEntryMetadata>::failure("Metadata not found for key: " + key);
}

Result<std::vector<CacheEntryMetadata>> CacheManager::listEntries(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    std::vector<CacheEntryMetadata> entries;

    if (pattern.empty()) {
        // Return all entries
        for (const auto& [key, metadata] : metadataCache_) {
            entries.push_back(metadata);
        }
    } else {
        // Return entries matching pattern
        auto matchingKeys = impl_->findKeysByPattern(pattern);
        for (const auto& key : matchingKeys) {
            if (metadataCache_.count(key)) {
                entries.push_back(metadataCache_[key]);
            }
        }
    }

    return Result<CacheEntryMetadata>::success(entries);
}

Result<void> CacheManager::setTTL(const std::string& key, const std::chrono::seconds& ttl) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    if (memoryCache_.count(key)) {
        memoryCache_[key].expiresAt = std::chrono::system_clock::now() + ttl;
        metadataCache_[key].expiresAt = memoryCache_[key].expiresAt;
        return Result<void>::success();
    }

    return Result<void>::failure("Cache item not found: " + key);
}

Result<std::chrono::seconds> CacheManager::getTTL(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    if (memoryCache_.count(key)) {
        auto now = std::chrono::system_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
            memoryCache_[key].expiresAt - now);
        return Result<std::chrono::seconds>::success(remaining);
    }

    return Result<std::chrono::seconds>::failure("Cache item not found: " + key);
}

CacheMetrics CacheManager::getMetrics() const {
    return metrics_;
}

Result<void> CacheManager::resetMetrics() {
    metrics_.totalRequests = 0;
    metrics_.cacheHits = 0;
    metrics_.cacheMisses = 0;
    metrics_.evictions = 0;
    metrics_.invalidations = 0;
    metrics_.memoryUsage = 0;
    metrics_.diskUsage = 0;
    metrics_.totalItems = 0;
    metrics_.averageAccessTime = std::chrono::milliseconds(0);
    metrics_.averageWriteTime = std::chrono::milliseconds(0);
    metrics_.lastUpdated = std::chrono::system_clock::now();

    return Result<void>::success();
}

Result<void> CacheManager::cleanup() {
    std::lock_guard<std::mutex> lock(cacheMutex_);

    // Remove expired items from memory cache
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expiredKeys;

    for (const auto& [key, item] : memoryCache_) {
        if (item.expiresAt < now) {
            expiredKeys.push_back(key);
        }
    }

    for (const auto& key : expiredKeys) {
        memoryCache_.erase(key);
        metadataCache_.erase(key);
        metrics_.evictions++;
    }

    return Result<void>::success();
}

Result<void> CacheManager::optimize() {
    // Perform cache optimization based on usage patterns
    // This is a simplified implementation
    cleanup();

    // Reorganize memory layout for better performance
    // This would involve analyzing access patterns and reorganizing data structures

    return Result<void>::success();
}

void CacheManager::setCacheEventCallback(CacheEventCallback callback) {
    eventCallback_ = callback;
}

void CacheManager::updateMetrics(const std::string& operation, bool success,
                               std::chrono::milliseconds duration) {
    metrics_.totalRequests++;

    if (operation == "get") {
        if (success) {
            metrics_.cacheHits++;
        } else {
            metrics_.cacheMisses++;
        }
        // Update average access time
        auto currentAvg = metrics_.averageAccessTime.load();
        metrics_.averageAccessTime = std::chrono::milliseconds(
            (currentAvg.count() + duration.count()) / 2);
    } else if (operation == "put" && success) {
        // Update average write time
        auto currentAvg = metrics_.averageWriteTime.load();
        metrics_.averageWriteTime = std::chrono::milliseconds(
            (currentAvg.count() + duration.count()) / 2);
    }

    metrics_.lastUpdated = std::chrono::system_clock::now();
}

void CacheManager::cleanupThreadFunction() {
    while (running_) {
        std::unique_lock<std::mutex> lock(cleanupMutex_);
        cleanupCondition_.wait_for(lock, config_.cleanupInterval);

        if (!running_) {
            break;
        }

        try {
            cleanup();
        } catch (const std::exception& e) {
            std::cerr << "Error in cleanup thread: " << e.what() << std::endl;
        }
    }
}

void CacheManager::metricsThreadFunction() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::minutes(1));

        if (!running_) {
            break;
        }

        // Update memory usage metrics
        std::lock_guard<std::mutex> lock(cacheMutex_);
        metrics_.memoryUsage = memoryUsage_;
        metrics_.totalItems = totalItems_;
    }
}

} // namespace scratchrobin
