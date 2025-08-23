#ifndef SCRATCHROBIN_SCHEMA_COLLECTOR_H
#define SCRATCHROBIN_SCHEMA_COLLECTOR_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <mutex>
#include "types/result.h"

namespace scratchrobin {

enum class DatabaseType {
    SCRATCHBIRD,
    POSTGRESQL,
    MYSQL,
    SQLSERVER,
    ORACLE,
    SQLITE
};

enum class SchemaObjectType {
    SCHEMA,
    TABLE,
    VIEW,
    COLUMN,
    INDEX,
    CONSTRAINT,
    TRIGGER,
    FUNCTION,
    PROCEDURE,
    SEQUENCE,
    DOMAIN,
    TYPE,
    RULE
};

struct SchemaObject {
    std::string name;
    std::string schema;
    std::string database;
    SchemaObjectType type;
    std::string owner;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point modifiedAt;
    std::unordered_map<std::string, std::string> properties;
    std::vector<std::string> dependencies;
    std::vector<std::string> dependents;
    bool isSystemObject = false;
    bool isTemporary = false;
};

struct SchemaCollectionOptions {
    std::vector<std::string> includedSchemas;
    std::vector<std::string> excludedSchemas;
    std::vector<SchemaObjectType> includedTypes;
    std::vector<SchemaObjectType> excludedTypes;
    bool includeSystemObjects = false;
    bool includeTemporaryObjects = false;
    bool followDependencies = true;
    int maxDepth = 10;
    int pageSize = 1000;
    int pageNumber = 0;
    std::string filterPattern;
    bool caseSensitive = false;
};

struct SchemaCollectionResult {
    std::vector<SchemaObject> objects;
    int totalCount = 0;
    int pageCount = 0;
    bool hasMorePages = false;
    std::chrono::system_clock::time_point collectedAt;
    std::chrono::milliseconds collectionTime;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class IConnection;

class ISchemaCollector {
public:
    virtual ~ISchemaCollector() = default;

    virtual DatabaseType getDatabaseType() const = 0;
    virtual std::string getDatabaseVersion() const = 0;

    virtual Result<SchemaCollectionResult> collectSchema(
        const SchemaCollectionOptions& options = {}) = 0;

    virtual Result<SchemaObject> getObjectDetails(
        const std::string& schema, const std::string& name, SchemaObjectType type) = 0;

    virtual Result<std::vector<SchemaObject>> getObjectDependencies(
        const std::string& schema, const std::string& name, SchemaObjectType type) = 0;

    virtual Result<std::vector<SchemaObject>> getObjectDependents(
        const std::string& schema, const std::string& name, SchemaObjectType type) = 0;

    virtual Result<bool> objectExists(
        const std::string& schema, const std::string& name, SchemaObjectType type) = 0;

    virtual Result<std::chrono::system_clock::time_point> getObjectLastModified(
        const std::string& schema, const std::string& name, SchemaObjectType type) = 0;

    virtual Result<void> refreshSchemaCache() = 0;
    virtual Result<SchemaCollectionResult> getCachedSchema(
        const SchemaCollectionOptions& options = {}) = 0;

    using ProgressCallback = std::function<void(int, int, const std::string&)>;
    virtual void setProgressCallback(ProgressCallback callback) = 0;

    using ObjectFilter = std::function<bool(const SchemaObject&)>;
    virtual void setObjectFilter(ObjectFilter filter) = 0;
};

class SchemaCollector : public ISchemaCollector {
public:
    SchemaCollector(std::shared_ptr<IConnection> connection);
    ~SchemaCollector() override;

    DatabaseType getDatabaseType() const override;
    std::string getDatabaseVersion() const override;

    Result<SchemaCollectionResult> collectSchema(
        const SchemaCollectionOptions& options = {}) override;

    Result<SchemaObject> getObjectDetails(
        const std::string& schema, const std::string& name, SchemaObjectType type) override;

    Result<std::vector<SchemaObject>> getObjectDependencies(
        const std::string& schema, const std::string& name, SchemaObjectType type) override;

    Result<std::vector<SchemaObject>> getObjectDependents(
        const std::string& schema, const std::string& name, SchemaObjectType type) override;

    Result<bool> objectExists(
        const std::string& schema, const std::string& name, SchemaObjectType type) override;

    Result<std::chrono::system_clock::time_point> getObjectLastModified(
        const std::string& schema, const std::string& name, SchemaObjectType type) override;

    Result<void> refreshSchemaCache() override;
    Result<SchemaCollectionResult> getCachedSchema(
        const SchemaCollectionOptions& options = {}) override;

    void setProgressCallback(ProgressCallback callback) override;
    void setObjectFilter(ObjectFilter filter) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::shared_ptr<IConnection> connection_;
    DatabaseType databaseType_;
    std::string databaseVersion_;
    ProgressCallback progressCallback_;
    ObjectFilter objectFilter_;

    mutable std::mutex cacheMutex_;
    std::unordered_map<std::string, SchemaCollectionResult> schemaCache_;
    std::chrono::system_clock::time_point lastCacheRefresh_;

    // Database-specific implementation methods
    Result<SchemaCollectionResult> collectScratchBirdSchema(const SchemaCollectionOptions& options);
    Result<SchemaCollectionResult> collectPostgreSQLSchema(const SchemaCollectionOptions& options);
    Result<SchemaCollectionResult> collectMySQLSchema(const SchemaCollectionOptions& options);
    Result<SchemaCollectionResult> collectSQLServerSchema(const SchemaCollectionOptions& options);
    Result<SchemaCollectionResult> collectOracleSchema(const SchemaCollectionOptions& options);
    Result<SchemaCollectionResult> collectSQLiteSchema(const SchemaCollectionOptions& options);

    // Helper methods
    DatabaseType detectDatabaseType();
    std::string getDatabaseVersionString();
    void buildObjectHierarchy(std::vector<SchemaObject>& objects);
    void resolveObjectDependencies(std::vector<SchemaObject>& objects);
    bool applyObjectFilter(const SchemaObject& object);
    void updateCache(const SchemaCollectionResult& result, const SchemaCollectionOptions& options);
    std::string generateCacheKey(const SchemaCollectionOptions& options);
    bool isCacheValid(const SchemaCollectionOptions& options);
    void cleanupExpiredCache();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SCHEMA_COLLECTOR_H
