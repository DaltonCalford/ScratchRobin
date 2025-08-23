#ifndef SCRATCHROBIN_METADATA_MANAGER_H
#define SCRATCHROBIN_METADATA_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "metadata/schema_collector.h"
#include <future>

namespace scratchrobin {

// Forward declarations and type definitions
struct SchemaChange {
    std::string schema;
    std::string objectName;
    SchemaObjectType objectType;
    std::chrono::system_clock::time_point timestamp;
};

enum class MetadataLoadStrategy {
    LAZY,           // Load metadata on demand
    EAGER,          // Load all metadata upfront
    HYBRID,         // Load frequently used metadata eagerly
    INCREMENTAL     // Load metadata incrementally as needed
};

enum class MetadataRefreshPolicy {
    MANUAL,         // Manual refresh only
    ON_DEMAND,      // Refresh when requested
    PERIODIC,       // Refresh at regular intervals
    ON_CHANGE,      // Refresh when schema changes detected
    ADAPTIVE        // Adjust refresh based on usage patterns
};

struct MetadataConfiguration {
    MetadataLoadStrategy loadStrategy = MetadataLoadStrategy::HYBRID;
    MetadataRefreshPolicy refreshPolicy = MetadataRefreshPolicy::ON_CHANGE;
    std::chrono::seconds refreshInterval{300}; // 5 minutes
    std::chrono::seconds cacheTTL{3600};       // 1 hour
    int maxConcurrentLoads = 5;
    int maxCacheSize = 1000;
    bool enableBackgroundRefresh = true;
    bool enableChangeTracking = true;
    bool enableMetrics = true;
    std::vector<std::string> includedSchemas;
    std::vector<std::string> excludedSchemas;
    std::vector<SchemaObjectType> includedTypes;
    std::vector<SchemaObjectType> excludedTypes;
};

struct MetadataLoadRequest {
    std::string schema;
    std::string object;
    SchemaObjectType type;
    bool includeDependencies = true;
    bool includeDependents = false;
    int maxDepth = 5;
    bool forceRefresh = false;
    std::chrono::seconds timeout{30};
};

struct MetadataLoadResult {
    std::string requestId;
    bool success = false;
    std::vector<SchemaObject> objects;
    std::chrono::system_clock::time_point loadedAt;
    std::chrono::milliseconds loadTime;
    std::size_t objectsLoaded = 0;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct MetadataQuery {
    std::string schema;
    std::string object;
    SchemaObjectType type;
    std::string filterPattern;
    bool caseSensitive = false;
    int limit = 100;
    int offset = 0;
    bool includeSystemObjects = false;
    std::vector<SchemaObjectType> includedTypes;
    std::string database;
    bool includeTemporaryObjects = false;
};

struct MetadataQueryResult {
    std::vector<SchemaObject> objects;
    int totalCount = 0;
    bool hasMore = false;
    std::chrono::milliseconds queryTime;
};

struct MetadataMetrics {
    std::atomic<std::size_t> totalLoadRequests{0};
    std::atomic<std::size_t> successfulLoads{0};
    std::atomic<std::size_t> failedLoads{0};
    std::atomic<std::size_t> cacheHits{0};
    std::atomic<std::size_t> cacheMisses{0};
    std::atomic<std::size_t> totalObjects{0};
    std::atomic<std::size_t> totalSchemas{0};
    std::chrono::milliseconds averageLoadTime{0};
    std::chrono::milliseconds averageQueryTime{0};
    std::chrono::system_clock::time_point lastUpdated;
};

class IConnection;
class ISchemaCollector;
class IObjectHierarchy;
class ICacheManager;
class IChangeTracker;

class IMetadataManager {
public:
    virtual ~IMetadataManager() = default;

    virtual Result<void> initialize(const MetadataConfiguration& config) = 0;
    virtual Result<void> shutdown() = 0;

    virtual Result<MetadataLoadResult> loadMetadata(const MetadataLoadRequest& request) = 0;
    virtual Result<MetadataLoadResult> loadMetadataAsync(const MetadataLoadRequest& request) = 0;

    virtual Result<MetadataQueryResult> queryMetadata(const MetadataQuery& query) = 0;
    virtual Result<SchemaObject> getObjectDetails(const std::string& schema,
                                                const std::string& object,
                                                SchemaObjectType type) = 0;

    virtual Result<bool> isMetadataLoaded(const std::string& schema,
                                        const std::string& object,
                                        SchemaObjectType type) = 0;

    virtual Result<std::chrono::system_clock::time_point> getLastLoadTime(
        const std::string& schema, const std::string& object, SchemaObjectType type) = 0;

    virtual Result<void> refreshMetadata(const std::string& schema = "",
                                       const std::string& object = "",
                                       SchemaObjectType type = SchemaObjectType::TABLE) = 0;

    virtual Result<void> invalidateMetadata(const std::string& schema = "",
                                          const std::string& object = "",
                                          SchemaObjectType type = SchemaObjectType::TABLE) = 0;

    virtual Result<std::shared_ptr<MetadataMetrics>> getMetrics() const = 0;
    virtual Result<void> resetMetrics() = 0;

    virtual Result<MetadataConfiguration> getConfiguration() const = 0;
    virtual Result<void> updateConfiguration(const MetadataConfiguration& config) = 0;

    using LoadCallback = std::function<void(const MetadataLoadResult&)>;
    using ChangeCallback = std::function<void(const SchemaChange&)>;

    virtual void setLoadCallback(LoadCallback callback) = 0;
    virtual void setChangeCallback(ChangeCallback callback) = 0;

    virtual Result<std::vector<MetadataLoadResult>> getActiveLoads() = 0;
    virtual Result<void> cancelLoad(const std::string& requestId) = 0;
};

class MetadataManager : public IMetadataManager {
public:
    MetadataManager(std::shared_ptr<IConnection> connection);
    ~MetadataManager() override;

    Result<void> initialize(const MetadataConfiguration& config) override;
    Result<void> shutdown() override;

    Result<MetadataLoadResult> loadMetadata(const MetadataLoadRequest& request) override;
    Result<MetadataLoadResult> loadMetadataAsync(const MetadataLoadRequest& request) override;

    Result<MetadataQueryResult> queryMetadata(const MetadataQuery& query) override;
    Result<SchemaObject> getObjectDetails(const std::string& schema,
                                        const std::string& object,
                                        SchemaObjectType type) override;

    Result<bool> isMetadataLoaded(const std::string& schema,
                                const std::string& object,
                                SchemaObjectType type) override;

    Result<std::chrono::system_clock::time_point> getLastLoadTime(
        const std::string& schema, const std::string& object, SchemaObjectType type) override;

    Result<void> refreshMetadata(const std::string& schema = "",
                               const std::string& object = "",
                               SchemaObjectType type = SchemaObjectType::TABLE) override;

    Result<void> invalidateMetadata(const std::string& schema = "",
                                  const std::string& object = "",
                                  SchemaObjectType type = SchemaObjectType::TABLE) override;

    Result<std::shared_ptr<MetadataMetrics>> getMetrics() const override;
    Result<void> resetMetrics() override;

    Result<MetadataConfiguration> getConfiguration() const override;
    Result<void> updateConfiguration(const MetadataConfiguration& config) override;

    void setLoadCallback(LoadCallback callback) override;
    void setChangeCallback(ChangeCallback callback) override;

    Result<std::vector<MetadataLoadResult>> getActiveLoads() override;
    Result<void> cancelLoad(const std::string& requestId) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::shared_ptr<IConnection> connection_;
    MetadataConfiguration config_;
    MetadataMetrics metrics_;

    LoadCallback loadCallback_;
    ChangeCallback changeCallback_;

    // Core components
    std::shared_ptr<ISchemaCollector> schemaCollector_;
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<ICacheManager> cacheManager_;
    std::shared_ptr<IChangeTracker> changeTracker_;

    // Async loading management
    mutable std::mutex loadMutex_;
    std::unordered_map<std::string, MetadataLoadResult> activeLoads_;
    std::unordered_map<std::string, std::future<MetadataLoadResult>> loadFutures_;

    // Background processing
    std::atomic<bool> running_{false};
    std::thread refreshThread_;
    std::thread cleanupThread_;
    std::condition_variable refreshCondition_;
    mutable std::mutex refreshMutex_;

    // Helper methods
    Result<void> initializeComponents();
    Result<void> shutdownComponents();

    Result<MetadataLoadResult> loadMetadataInternal(const MetadataLoadRequest& request);
    Result<MetadataLoadResult> loadSchemaMetadata(const MetadataLoadRequest& request);
    Result<MetadataLoadResult> loadObjectMetadata(const MetadataLoadRequest& request);
    Result<MetadataLoadResult> loadHierarchyMetadata(const MetadataLoadRequest& request);

    void processAsyncLoad(const std::string& requestId,
                         std::future<MetadataLoadResult> future);

    void refreshThreadFunction();
    void cleanupThreadFunction();

    void handleSchemaChange(const SchemaChange& change);
    void updateMetrics(const std::string& operation, bool success,
                      std::chrono::milliseconds duration);

    std::string generateRequestId();
    std::string generateObjectKey(const std::string& schema,
                                const std::string& object,
                                SchemaObjectType type);

    bool shouldRefreshMetadata(const std::string& schema,
                             const std::string& object,
                             SchemaObjectType type);

    void cacheMetadata(const std::string& key, const MetadataLoadResult& result);
    Result<MetadataLoadResult> getCachedMetadata(const std::string& key);

    void optimizeCache();
    void cleanupExpiredData();

    std::vector<std::string> getAffectedObjects(const SchemaChange& change);
    void invalidateAffectedCache(const std::vector<std::string>& affectedObjects);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_METADATA_MANAGER_H
