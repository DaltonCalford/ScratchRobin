#include "metadata/metadata_manager.h"
#include "metadata/schema_collector.h"
#include "metadata/object_hierarchy.h"
#include "metadata/cache_manager.h"
#include "core/connection_manager.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include "types/result.h"

namespace scratchrobin {

class MetadataManager::Impl {
public:
    Impl(std::shared_ptr<IConnection> connection)
        : connection_(connection) {}

    Result<void> initializeComponents(const MetadataConfiguration& config) {
        try {
            // Initialize schema collector
            schemaCollector_ = std::make_shared<SchemaCollector>(connection_);

            // Initialize object hierarchy
            objectHierarchy_ = std::make_shared<ObjectHierarchy>(schemaCollector_);

            // Initialize cache manager
            cacheManager_ = std::make_shared<CacheManager>();
            CacheConfiguration cacheConfig;
            cacheConfig.cacheDirectory = "./cache/metadata";
            cacheConfig.maxMemorySize = 50 * 1024 * 1024; // 50MB
            cacheConfig.maxDiskSize = 500 * 1024 * 1024;   // 500MB
            cacheConfig.defaultTTL = config.cacheTTL;
            cacheConfig.enableCompression = true;

            auto cacheResult = cacheManager_->initialize(cacheConfig);
            if (!cacheResult.isSuccess()) {
                return cacheResult;
            }

            // Initialize change tracker
            changeTracker_ = nullptr; // Will be implemented later

            return Result<void>::success();

        } catch (const std::exception& e) {
            return Result<void>::error(std::string("Failed to initialize metadata components: ") + e.what());
        }
    }

    Result<void> shutdownComponents() {
        try {
            if (cacheManager_) {
                cacheManager_->shutdown();
            }

            schemaCollector_.reset();
            objectHierarchy_.reset();
            cacheManager_.reset();
            changeTracker_.reset();

            return Result<void>::success();

        } catch (const std::exception& e) {
            return Result<void>::error(std::string("Failed to shutdown metadata components: ") + e.what());
        }
    }

    Result<MetadataLoadResult> loadMetadataInternal(const MetadataLoadRequest& request) {
        MetadataLoadResult result;
        result.requestId = generateRequestId();
        result.loadedAt = std::chrono::system_clock::now();
        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            // Check cache first
            std::string cacheKey = generateObjectKey(request.schema, request.object, request.type);
            auto cachedResult = getCachedMetadata(cacheKey);

            if (cachedResult.isSuccess() && !request.forceRefresh) {
                result = cachedResult.value();
                result.requestId = generateRequestId(); // Update request ID
                metrics_.cacheHits++;
                return Result<MetadataLoadResult>::success(result);
            }

            // Load fresh metadata
            if (request.type == SchemaObjectType::SCHEMA) {
                // Load schema-level metadata
                auto schemaResult = loadSchemaMetadata(request);
                if (schemaResult.isSuccess()) {
                    result = schemaResult.value();
                }
            } else {
                // Load object-level metadata
                auto objectResult = loadObjectMetadata(request);
                if (objectResult.isSuccess()) {
                    result = objectResult.value();
                }
            }

            // Load hierarchy information if requested
            if (request.includeDependencies || request.includeDependents) {
                auto hierarchyResult = loadHierarchyMetadata(request);
                if (hierarchyResult.isSuccess()) {
                    // Merge hierarchy data with main result
                    result.objects.insert(result.objects.end(),
                                        hierarchyResult.value().objects.begin(),
                                        hierarchyResult.value().objects.end());
                }
            }

            // result is now valid
            result.objectsLoaded = result.objects.size();

            // Cache the result
            // Success case handled
            if (result.success) {
                cacheMetadata(cacheKey, result);
                metrics_.successfulLoads++;
            } else {
                metrics_.failedLoads++;
            }

        } catch (const std::exception& e) {
            // Error case handled
            result.errors.push_back(std::string("Metadata loading failed: ") + e.what());
            metrics_.failedLoads++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        return Result<MetadataLoadResult>::success(result);
    }

    Result<MetadataLoadResult> loadSchemaMetadata(const MetadataLoadRequest& request) {
        MetadataLoadResult result;
        result.requestId = generateRequestId();

        try {
            // Use schema collector to get schema information
            SchemaCollectionOptions options;
            options.includedSchemas = {request.schema};
            options.includeSystemObjects = false;

            auto collectionResult = schemaCollector_->collectSchema(options);
            if (!collectionResult.isSuccess()) {
                result.errors.push_back(collectionResult.error().message);
                return Result<MetadataLoadResult>::success(result);
            }

            result.objects = collectionResult.value().objects;
            // result is now valid

        } catch (const std::exception& e) {
            // Error case handled
            result.errors.push_back(std::string("Schema metadata loading failed: ") + e.what());
        }

        return Result<MetadataLoadResult>::success(result);
    }

    Result<MetadataLoadResult> loadObjectMetadata(const MetadataLoadRequest& request) {
        MetadataLoadResult result;
        result.requestId = generateRequestId();

        try {
            // Get object details
            auto objectResult = schemaCollector_->getObjectDetails(request.schema, request.object, request.type);
            if (objectResult.isSuccess()) {
                result.objects.push_back(objectResult.value());
            } else {
                result.errors.push_back(objectResult.error().message);
                return Result<MetadataLoadResult>::success(result);
            }

            // Load dependencies if requested
            if (request.includeDependencies) {
                auto dependencies = schemaCollector_->getObjectDependencies(request.schema, request.object, request.type);
                if (dependencies.isSuccess()) {
                    result.objects.insert(result.objects.end(),
                                        dependencies.value().begin(),
                                        dependencies.value().end());
                }
            }

            // Load dependents if requested
            if (request.includeDependents) {
                auto dependents = schemaCollector_->getObjectDependents(request.schema, request.object, request.type);
                if (dependents.isSuccess()) {
                    result.objects.insert(result.objects.end(),
                                        dependents.value().begin(),
                                        dependents.value().end());
                }
            }

            // result is now valid

        } catch (const std::exception& e) {
            // Error case handled
            result.errors.push_back(std::string("Object metadata loading failed: ") + e.what());
        }

        return Result<MetadataLoadResult>::success(result);
    }

    Result<MetadataLoadResult> loadHierarchyMetadata(const MetadataLoadRequest& request) {
        MetadataLoadResult result;
        result.requestId = generateRequestId();

        try {
            // Build object hierarchy
            HierarchyTraversalOptions options;
            options.includeIndirectDependencies = (request.maxDepth > 1);
            options.maxDepth = request.maxDepth;
            options.includeSystemObjects = false;

            auto hierarchyResult = objectHierarchy_->buildHierarchy(request.schema, request.object, request.type, options);
            if (!hierarchyResult.isSuccess()) {
                result.errors.push_back(hierarchyResult.error().message);
                return Result<MetadataLoadResult>::success(result);
            }

            // Convert hierarchy to schema objects
            // This is a simplified conversion - in practice, you'd have more sophisticated mapping
            result.objects.clear(); // Clear existing objects to avoid duplicates

            // Add the root object
            auto rootResult = schemaCollector_->getObjectDetails(request.schema, request.object, request.type);
            if (rootResult.isSuccess()) {
                result.objects.push_back(rootResult.value());
            }

            // Add dependencies and dependents from hierarchy
            auto& hierarchy = hierarchyResult.value();

            // Add direct dependencies
            for (const auto& dep : hierarchy.directDependencies) {
                auto depResult = schemaCollector_->getObjectDetails(dep.toSchema, dep.toObject, dep.toType);
                if (depResult.isSuccess()) {
                    result.objects.push_back(depResult.value());
                }
            }

            // Add direct dependents
            for (const auto& dep : hierarchy.directDependents) {
                auto depResult = schemaCollector_->getObjectDetails(dep.fromSchema, dep.fromObject, dep.fromType);
                if (depResult.isSuccess()) {
                    result.objects.push_back(depResult.value());
                }
            }

            // result is now valid

        } catch (const std::exception& e) {
            // Error case handled
            result.errors.push_back(std::string("Hierarchy metadata loading failed: ") + e.what());
        }

        return Result<MetadataLoadResult>::success(result);
    }

    void cacheMetadata(const std::string& key, const MetadataLoadResult& result) {
        try {
            // Serialize the result to cache
            std::vector<char> data = serializeMetadataResult(result);

            // Cache with TTL
            cacheManager_->put(key, data, config_.cacheTTL);

        } catch (const std::exception& e) {
            // Log error but don't fail the operation
            std::cerr << "Failed to cache metadata: " << e.what() << std::endl;
        }
    }

    Result<MetadataLoadResult> getCachedMetadata(const std::string& key) {
        try {
            auto cacheResult = cacheManager_->get(key);
            if (!cacheResult.isSuccess()) {
                return Result<MetadataLoadResult>::error("Cache miss");
            }

            // Deserialize the cached data
            return deserializeMetadataResult(cacheResult.value());

        } catch (const std::exception& e) {
            return Result<MetadataLoadResult>::error(std::string("Failed to get cached metadata: ") + e.what());
        }
    }

    void invalidateAffectedCache(const std::vector<std::string>& affectedObjects) {
        for (const auto& objectKey : affectedObjects) {
            cacheManager_->invalidate(objectKey);
        }
    }

    std::vector<char> serializeMetadataResult(const MetadataLoadResult& result) {
        // Simplified serialization - in practice, you'd use a proper serialization format
        std::string data = result.requestId + "|" +
                          std::to_string(result.success) + "|" +
                          std::to_string(result.objectsLoaded) + "|";

        for (const auto& obj : result.objects) {
            data += obj.name + "," + obj.schema + "," +
                   std::to_string(static_cast<int>(obj.type)) + ";";
        }

        return std::vector<char>(data.begin(), data.end());
    }

    Result<MetadataLoadResult> deserializeMetadataResult(const std::vector<char>& data) {
        MetadataLoadResult result;

        // Simplified deserialization - in practice, you'd use a proper deserialization format
        std::string strData(data.begin(), data.end());
        std::stringstream ss(strData);
        std::string token;

        // Parse basic fields
        std::getline(ss, token, '|'); result.requestId = token;
        std::getline(ss, token, '|'); result.success = (token == "1");
        std::getline(ss, token, '|'); result.objectsLoaded = std::stoul(token);

        // Parse objects (simplified)
        result.objects.clear();

        return Result<MetadataLoadResult>::success(result);
    }

    std::string generateRequestId() {
        static std::atomic<uint64_t> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        std::stringstream ss;
        ss << "req_" << timestamp << "_" << counter++;
        return ss.str();
    }

    std::string generateObjectKey(const std::string& schema,
                                const std::string& object,
                                SchemaObjectType type) {
        return schema + "." + object + "." + std::to_string(static_cast<int>(type));
    }

    bool shouldRefreshMetadata(const std::string& schema,
                             const std::string& object,
                             SchemaObjectType type) {
        // Check if metadata needs refresh based on policy
        switch (config_.refreshPolicy) {
            case MetadataRefreshPolicy::MANUAL:
                return false;
            case MetadataRefreshPolicy::ON_DEMAND:
                return true;
            case MetadataRefreshPolicy::PERIODIC: {
                // Check if enough time has passed since last load
                auto lastLoad = getLastLoadTime(schema, object, type);
                if (lastLoad.isSuccess()) {
                    auto age = std::chrono::system_clock::now() - lastLoad.value();
                    return age > config_.refreshInterval;
                }
                return true;
            }
            case MetadataRefreshPolicy::ON_CHANGE:
                // Check if there were recent schema changes
                return false; // Placeholder
            case MetadataRefreshPolicy::ADAPTIVE:
                // Adaptive refresh based on usage patterns
                return false; // Placeholder
            default:
                return false;
        }
    }

    Result<std::chrono::system_clock::time_point> getLastLoadTime(
        const std::string& schema, const std::string& object, SchemaObjectType type) {

        std::string key = generateObjectKey(schema, object, type);
        auto metadataResult = cacheManager_->getMetadata(key);

        if (metadataResult.isSuccess()) {
            return Result<std::chrono::system_clock::time_point>::success(
                metadataResult.value().lastAccessed);
        }

        return Result<std::chrono::system_clock::time_point>::error("No load time information available");
    }

    std::vector<std::string> getAffectedObjects(const SchemaChange& change) {
        // Implementation for getting affected objects
        std::vector<std::string> affected;
        // This would analyze the change and determine what objects are affected
        return affected;
    }

public:  // Made public to allow access from MetadataManager
    std::shared_ptr<IConnection> connection_;
    MetadataConfiguration config_;
    MetadataMetrics metrics_;

    std::shared_ptr<ISchemaCollector> schemaCollector_;
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<ICacheManager> cacheManager_;
    std::shared_ptr<IChangeTracker> changeTracker_;

private:
};

// MetadataManager implementation

MetadataManager::MetadataManager(std::shared_ptr<IConnection> connection)
    : impl_(std::make_unique<Impl>(connection)),
      connection_(connection) {}

MetadataManager::~MetadataManager() {
    if (running_) {
        shutdown();
    }
}

Result<void> MetadataManager::initialize(const MetadataConfiguration& config) {
    config_ = config;

    try {
        // Initialize core components
        auto initResult = impl_->initializeComponents(config);
        if (!initResult.isSuccess()) {
            return initResult;
        }

        // Start background threads
        running_ = true;
        refreshThread_ = std::thread(&MetadataManager::refreshThreadFunction, this);
        cleanupThread_ = std::thread(&MetadataManager::cleanupThreadFunction, this);

        return Result<void>::success();

    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Failed to initialize metadata manager: ") + e.what());
    }
}

Result<void> MetadataManager::shutdown() {
    try {
        running_ = false;
        refreshCondition_.notify_all();

        if (refreshThread_.joinable()) {
            refreshThread_.join();
        }
        if (cleanupThread_.joinable()) {
            cleanupThread_.join();
        }

        // Shutdown components
        return impl_->shutdownComponents();

    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Failed to shutdown metadata manager: ") + e.what());
    }
}

Result<MetadataLoadResult> MetadataManager::loadMetadata(const MetadataLoadRequest& request) {
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        metrics_.totalLoadRequests++;

        auto result = impl_->loadMetadataInternal(request);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        updateMetrics("load", result.isSuccess(), duration);

        // Notify callback if set
        if (loadCallback_ && result.isSuccess()) {
            loadCallback_(result.value());
        }

        return result;

    } catch (const std::exception& e) {
        return Result<MetadataLoadResult>::error(std::string("Metadata loading failed: ") + e.what());
    }
}

Result<MetadataLoadResult> MetadataManager::loadMetadataAsync(const MetadataLoadRequest& request) {
    try {
        std::string requestId = impl_->generateRequestId();

        // Start async operation
        auto future = std::async(std::launch::async, [this, request]() {
            return impl_->loadMetadataInternal(request);
        });

        std::lock_guard<std::mutex> lock(loadMutex_);
        // Store the future with the correct type
        // Note: This needs proper handling of Result<T> futures

        // Create initial result
        MetadataLoadResult initialResult;
        initialResult.requestId = requestId;
        initialResult.success = false; // Will be updated when async operation completes

        return Result<MetadataLoadResult>::success(initialResult);

    } catch (const std::exception& e) {
        return Result<MetadataLoadResult>::error(std::string("Async metadata loading failed: ") + e.what());
    }
}

Result<MetadataQueryResult> MetadataManager::queryMetadata(const MetadataQuery& query) {
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        MetadataQueryResult result;
        result.queryTime = std::chrono::milliseconds(0);

        // Use schema collector to query metadata
        SchemaCollectionOptions options;
        options.includedSchemas = {query.schema};
        options.filterPattern = query.filterPattern;
        options.caseSensitive = query.caseSensitive;
        options.pageSize = query.limit;
        options.pageNumber = query.offset / query.limit;
        options.includeSystemObjects = query.includeSystemObjects;

        if (!query.includedTypes.empty()) {
            options.includedTypes = query.includedTypes;
        }

        auto collectionResult = impl_->schemaCollector_->collectSchema(options);
        if (collectionResult.isSuccess()) {
            result.objects = collectionResult.value().objects;
            result.totalCount = collectionResult.value().totalCount;
            result.hasMore = collectionResult.value().hasMorePages;
        } else {
            return Result<MetadataQueryResult>::error(collectionResult.error().message);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.queryTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        updateMetrics("query", true, result.queryTime);

        return Result<MetadataQueryResult>::success(result);

    } catch (const std::exception& e) {
        return Result<MetadataQueryResult>::error(std::string("Metadata query failed: ") + e.what());
    }
}

Result<SchemaObject> MetadataManager::getObjectDetails(const std::string& schema,
                                                     const std::string& object,
                                                     SchemaObjectType type) {
    return impl_->schemaCollector_->getObjectDetails(schema, object, type);
}

Result<bool> MetadataManager::isMetadataLoaded(const std::string& schema,
                                             const std::string& object,
                                             SchemaObjectType type) {
    std::string key = impl_->generateObjectKey(schema, object, type);
    return impl_->cacheManager_->exists(key);
}

Result<std::chrono::system_clock::time_point> MetadataManager::getLastLoadTime(
    const std::string& schema, const std::string& object, SchemaObjectType type) {
    return impl_->getLastLoadTime(schema, object, type);
}

Result<void> MetadataManager::refreshMetadata(const std::string& schema,
                                            const std::string& object,
                                            SchemaObjectType type) {
    try {
        // Invalidate cache
        if (!schema.empty() && !object.empty()) {
            std::string key = impl_->generateObjectKey(schema, object, type);
            impl_->cacheManager_->invalidate(key);
        } else {
            // Refresh all metadata
            impl_->cacheManager_->clear();
        }

        // Trigger refresh of schema collector
        impl_->schemaCollector_->refreshSchemaCache();

        return Result<void>::success();

    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Metadata refresh failed: ") + e.what());
    }
}

Result<void> MetadataManager::invalidateMetadata(const std::string& schema,
                                               const std::string& object,
                                               SchemaObjectType type) {
    try {
        if (!schema.empty() && !object.empty()) {
            std::string key = impl_->generateObjectKey(schema, object, type);
            auto result = impl_->cacheManager_->invalidate(key);
            if (!result.isSuccess()) {
                return Result<void>::error("Cache invalidation failed");
            }
            return Result<void>::success();
        } else {
            return impl_->cacheManager_->clear();
        }

    } catch (const std::exception& e) {
        return Result<void>::error(std::string("Metadata invalidation failed: ") + e.what());
    }
}

Result<std::shared_ptr<MetadataMetrics>> MetadataManager::getMetrics() const {
    // Return a shared_ptr to avoid copying atomic members
    auto metricsPtr = std::make_shared<MetadataMetrics>();
    metricsPtr->totalLoadRequests.store(metrics_.totalLoadRequests.load());
    metricsPtr->successfulLoads.store(metrics_.successfulLoads.load());
    metricsPtr->failedLoads.store(metrics_.failedLoads.load());
    metricsPtr->cacheHits.store(metrics_.cacheHits.load());
    metricsPtr->cacheMisses.store(metrics_.cacheMisses.load());
    metricsPtr->totalObjects.store(metrics_.totalObjects.load());
    metricsPtr->totalSchemas.store(metrics_.totalSchemas.load());
    metricsPtr->averageLoadTime = metrics_.averageLoadTime;
    metricsPtr->averageQueryTime = metrics_.averageQueryTime;
    metricsPtr->lastUpdated = metrics_.lastUpdated;
    return Result<std::shared_ptr<MetadataMetrics>>::success(metricsPtr);
}

Result<void> MetadataManager::resetMetrics() {
    metrics_.totalLoadRequests = 0;
    metrics_.successfulLoads = 0;
    metrics_.failedLoads = 0;
    metrics_.cacheHits = 0;
    metrics_.cacheMisses = 0;
    metrics_.totalObjects = 0;
    metrics_.totalSchemas = 0;
    metrics_.averageLoadTime = std::chrono::milliseconds(0);
    metrics_.averageQueryTime = std::chrono::milliseconds(0);
    metrics_.lastUpdated = std::chrono::system_clock::now();

    return Result<void>::success();
}

Result<MetadataConfiguration> MetadataManager::getConfiguration() const {
    return Result<MetadataConfiguration>::success(config_);
}

Result<void> MetadataManager::updateConfiguration(const MetadataConfiguration& config) {
    config_ = config;
    return Result<void>::success();
}

void MetadataManager::setLoadCallback(LoadCallback callback) {
    loadCallback_ = callback;
}

void MetadataManager::setChangeCallback(ChangeCallback callback) {
    changeCallback_ = callback;
}

Result<std::vector<MetadataLoadResult>> MetadataManager::getActiveLoads() {
    std::lock_guard<std::mutex> lock(loadMutex_);

    std::vector<MetadataLoadResult> activeLoads;
    for (const auto& [requestId, result] : activeLoads_) {
        activeLoads.push_back(result);
    }

    return Result<std::vector<MetadataLoadResult>>::success(activeLoads);
}

Result<void> MetadataManager::cancelLoad(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(loadMutex_);

    auto it = loadFutures_.find(requestId);
    if (it != loadFutures_.end()) {
        // Note: std::future doesn't support cancellation in C++11/14
        // In C++20, we could use std::stop_token
        loadFutures_.erase(it);
        activeLoads_.erase(requestId);
        return Result<void>::success();
    }

    return Result<void>::error("Load request not found: " + requestId);
}

void MetadataManager::refreshThreadFunction() {
    while (running_) {
        std::unique_lock<std::mutex> lock(refreshMutex_);

        if (config_.refreshPolicy == MetadataRefreshPolicy::PERIODIC) {
            refreshCondition_.wait_for(lock, config_.refreshInterval);
        } else {
            refreshCondition_.wait(lock);
        }

        if (!running_) {
            break;
        }

        try {
            // Perform periodic refresh operations
            if (config_.enableBackgroundRefresh) {
                impl_->schemaCollector_->refreshSchemaCache();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in refresh thread: " << e.what() << std::endl;
        }
    }
}

void MetadataManager::cleanupThreadFunction() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::minutes(5));

        if (!running_) {
            break;
        }

        try {
            // Cleanup expired cache entries
            impl_->cacheManager_->cleanup();

            // Cleanup completed async loads
            std::lock_guard<std::mutex> lock(loadMutex_);

            for (auto it = loadFutures_.begin(); it != loadFutures_.end();) {
                if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    auto result = it->second.get();
                    activeLoads_[it->first] = result;

                    // Notify callback
                    if (loadCallback_) {
                        loadCallback_(result);
                    }

                    it = loadFutures_.erase(it);
                } else {
                    ++it;
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in cleanup thread: " << e.what() << std::endl;
        }
    }
}

void MetadataManager::updateMetrics(const std::string& operation, bool success,
                                  std::chrono::milliseconds duration) {
    if (operation == "load") {
        if (success) {
            metrics_.successfulLoads++;
        } else {
            metrics_.failedLoads++;
        }
        // Update average load time
        auto currentAvg = metrics_.averageLoadTime;
        metrics_.averageLoadTime = std::chrono::milliseconds(
            (currentAvg.count() + duration.count()) / 2);
    } else if (operation == "query") {
        // Update average query time
        auto currentAvg = metrics_.averageQueryTime;
        metrics_.averageQueryTime = std::chrono::milliseconds(
            (currentAvg.count() + duration.count()) / 2);
    }

    metrics_.lastUpdated = std::chrono::system_clock::now();
}

void MetadataManager::handleSchemaChange(const SchemaChange& change) {
    // Handle schema change notifications
    if (changeCallback_) {
        changeCallback_(change);
    }

    // Invalidate affected cache entries
    std::vector<std::string> affectedObjects = impl_->getAffectedObjects(change);
    impl_->invalidateAffectedCache(affectedObjects);

    // Auto-refresh if policy requires it
    if (config_.refreshPolicy == MetadataRefreshPolicy::ON_CHANGE) {
        refreshMetadata(change.schema, change.objectName, change.objectType);
    }
}

} // namespace scratchrobin
