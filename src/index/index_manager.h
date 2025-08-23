#ifndef SCRATCHROBIN_INDEX_MANAGER_H
#define SCRATCHROBIN_INDEX_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <functional>
#include <QObject>
#include <QDateTime>
#include <QJsonObject>

namespace scratchrobin {

class IMetadataManager;
class ISQLExecutor;
class IConnectionManager;

enum class IndexType {
    BTREE,
    HASH,
    GIN,
    GIST,
    SPGIST,
    BRIN,
    UNIQUE,
    PARTIAL,
    EXPRESSION,
    COMPOSITE
};

enum class IndexStatus {
    VALID,
    INVALID,
    BUILDING,
    REBUILDING,
    DROPPED
};

enum class IndexOperation {
    CREATE,
    DROP,
    REBUILD,
    RENAME,
    ALTER,
    VACUUM,
    ANALYZE
};

struct IndexColumn {
    std::string columnName;
    std::string expression;  // For expression indexes
    bool ascending = true;
    std::string nullsOrder;  // "FIRST" or "LAST"
    std::string collation;
    std::string operatorClass;
};

struct IndexDefinition {
    std::string name;
    std::string schema;
    std::string tableName;
    IndexType type = IndexType::BTREE;
    std::vector<IndexColumn> columns;
    std::string whereClause;      // For partial indexes
    std::string tablespace;
    bool isUnique = false;
    bool isPrimary = false;
    bool isClustered = false;
    bool isConcurrent = false;
    int fillFactor = 90;
    std::string storageParameters;
    std::string comment;
    QDateTime createdAt;
    QDateTime lastUsed;
    std::unordered_map<std::string, std::string> properties;
};

struct IndexStatistics {
    std::string indexName;
    std::string tableName;
    IndexType type;
    IndexStatus status;
    qint64 sizeBytes = 0;
    qint64 tupleCount = 0;
    qint64 scannedTuples = 0;
    double selectivity = 0.0;
    qint64 reads = 0;
    qint64 writes = 0;
    qint64 hits = 0;
    qint64 misses = 0;
    double hitRatio = 0.0;
    qint64 duplicateKeys = 0;
    qint64 leafPages = 0;
    qint64 internalPages = 0;
    qint64 emptyPages = 0;
    qint64 deletedPages = 0;
    double fragmentation = 0.0;
    QDateTime lastScan;
    QDateTime lastVacuum;
    QDateTime lastAnalyze;
    QDateTime collectedAt;
};

struct IndexPerformance {
    std::string indexName;
    std::string queryPattern;
    qint64 executionCount = 0;
    qint64 totalTimeMs = 0;
    double avgTimeMs = 0.0;
    double minTimeMs = 0.0;
    double maxTimeMs = 0.0;
    qint64 rowsReturned = 0;
    double rowsPerExecution = 0.0;
    double efficiency = 0.0;
    QDateTime firstUsed;
    QDateTime lastUsed;
    QDateTime analyzedAt;
};

struct IndexRecommendation {
    std::string tableName;
    std::string recommendationId;
    std::string title;
    std::string description;
    IndexType suggestedType;
    std::vector<std::string> suggestedColumns;
    std::string whereClause;
    double estimatedImprovement = 0.0;
    double confidence = 0.0;
    std::string reasoning;
    QDateTime generatedAt;
    bool isImplemented = false;
    QDateTime implementedAt;
};

struct IndexMaintenanceOperation {
    std::string operationId;
    std::string indexName;
    IndexOperation operation;
    std::string sqlStatement;
    IndexStatus statusBefore;
    IndexStatus statusAfter;
    QDateTime startedAt;
    QDateTime completedAt;
    bool success = false;
    std::string errorMessage;
    std::string outputMessage;
    qint64 affectedRows = 0;
};

class IIndexManager {
public:
    virtual ~IIndexManager() = default;

    virtual void initialize() = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;

    virtual std::vector<IndexDefinition> getTableIndexes(const std::string& tableName, const std::string& connectionId) = 0;
    virtual IndexDefinition getIndexDefinition(const std::string& indexName, const std::string& connectionId) = 0;
    virtual std::vector<IndexStatistics> getIndexStatistics(const std::string& indexName, const std::string& connectionId) = 0;
    virtual std::vector<IndexPerformance> getIndexPerformance(const std::string& indexName, const std::string& connectionId) = 0;

    virtual bool createIndex(const IndexDefinition& definition, const std::string& connectionId) = 0;
    virtual bool dropIndex(const std::string& indexName, const std::string& connectionId) = 0;
    virtual bool rebuildIndex(const std::string& indexName, const std::string& connectionId) = 0;
    virtual bool renameIndex(const std::string& oldName, const std::string& newName, const std::string& connectionId) = 0;
    virtual bool alterIndex(const std::string& indexName, const IndexDefinition& definition, const std::string& connectionId) = 0;

    virtual std::vector<IndexMaintenanceOperation> getMaintenanceHistory(const std::string& indexName, const std::string& connectionId) = 0;
    virtual IndexMaintenanceOperation performMaintenance(const std::string& indexName, IndexOperation operation, const std::string& connectionId) = 0;

    virtual std::vector<IndexRecommendation> analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) = 0;
    virtual std::vector<IndexRecommendation> analyzeQueryIndexes(const std::string& query, const std::string& connectionId) = 0;
    virtual void implementRecommendation(const std::string& recommendationId, const std::string& connectionId) = 0;

    virtual std::string generateIndexDDL(const IndexDefinition& definition, const std::string& connectionId) = 0;
    virtual std::vector<std::string> validateIndex(const IndexDefinition& definition, const std::string& connectionId) = 0;
    virtual bool isIndexNameAvailable(const std::string& name, const std::string& connectionId) = 0;

    virtual std::vector<std::string> getAvailableIndexTypes(const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableTablespaces(const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableOperatorClasses(const std::string& columnType, const std::string& connectionId) = 0;

    virtual void collectIndexStatistics(const std::string& indexName, const std::string& connectionId) = 0;
    virtual void collectAllIndexStatistics(const std::string& connectionId) = 0;

    using IndexCreatedCallback = std::function<void(const std::string&, const std::string&)>;
    using IndexDroppedCallback = std::function<void(const std::string&, const std::string&)>;
    using IndexModifiedCallback = std::function<void(const std::string&, const std::string&)>;
    using MaintenanceCompletedCallback = std::function<void(const IndexMaintenanceOperation&)>;

    virtual void setIndexCreatedCallback(IndexCreatedCallback callback) = 0;
    virtual void setIndexDroppedCallback(IndexDroppedCallback callback) = 0;
    virtual void setIndexModifiedCallback(IndexModifiedCallback callback) = 0;
    virtual void setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) = 0;
};

class IndexManager : public QObject, public IIndexManager {
    Q_OBJECT

public:
    IndexManager(QObject* parent = nullptr);
    ~IndexManager() override;

    // IIndexManager implementation
    void initialize() override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;

    std::vector<IndexDefinition> getTableIndexes(const std::string& tableName, const std::string& connectionId) override;
    IndexDefinition getIndexDefinition(const std::string& indexName, const std::string& connectionId) override;
    std::vector<IndexStatistics> getIndexStatistics(const std::string& indexName, const std::string& connectionId) override;
    std::vector<IndexPerformance> getIndexPerformance(const std::string& indexName, const std::string& connectionId) override;

    bool createIndex(const IndexDefinition& definition, const std::string& connectionId) override;
    bool dropIndex(const std::string& indexName, const std::string& connectionId) override;
    bool rebuildIndex(const std::string& indexName, const std::string& connectionId) override;
    bool renameIndex(const std::string& oldName, const std::string& newName, const std::string& connectionId) override;
    bool alterIndex(const std::string& indexName, const IndexDefinition& definition, const std::string& connectionId) override;

    std::vector<IndexMaintenanceOperation> getMaintenanceHistory(const std::string& indexName, const std::string& connectionId) override;
    IndexMaintenanceOperation performMaintenance(const std::string& indexName, IndexOperation operation, const std::string& connectionId) override;

    std::vector<IndexRecommendation> analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) override;
    std::vector<IndexRecommendation> analyzeQueryIndexes(const std::string& query, const std::string& connectionId) override;
    void implementRecommendation(const std::string& recommendationId, const std::string& connectionId) override;

    std::string generateIndexDDL(const IndexDefinition& definition, const std::string& connectionId) override;
    std::vector<std::string> validateIndex(const IndexDefinition& definition, const std::string& connectionId) override;
    bool isIndexNameAvailable(const std::string& name, const std::string& connectionId) override;

    std::vector<std::string> getAvailableIndexTypes(const std::string& connectionId) override;
    std::vector<std::string> getAvailableTablespaces(const std::string& connectionId) override;
    std::vector<std::string> getAvailableOperatorClasses(const std::string& columnType, const std::string& connectionId) override;

    void collectIndexStatistics(const std::string& indexName, const std::string& connectionId) override;
    void collectAllIndexStatistics(const std::string& connectionId) override;

    void setIndexCreatedCallback(IndexCreatedCallback callback) override;
    void setIndexDroppedCallback(IndexDroppedCallback callback) override;
    void setIndexModifiedCallback(IndexModifiedCallback callback) override;
    void setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) override;

signals:
    void indexCreated(const std::string& indexName, const std::string& connectionId);
    void indexDropped(const std::string& indexName, const std::string& connectionId);
    void indexModified(const std::string& indexName, const std::string& connectionId);
    void maintenanceCompleted(const IndexMaintenanceOperation& operation);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Callbacks
    IndexCreatedCallback indexCreatedCallback_;
    IndexDroppedCallback indexDroppedCallback_;
    IndexModifiedCallback indexModifiedCallback_;
    MaintenanceCompletedCallback maintenanceCompletedCallback_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INDEX_MANAGER_H
