# Phase 5.2: Index Management Implementation

## Overview
This phase implements comprehensive index management functionality for ScratchRobin, providing index creation, modification, performance analysis, usage tracking, and optimization capabilities with seamless integration with the existing table management and query execution systems.

## Prerequisites
- ✅ Phase 1.1 (Project Setup) completed
- ✅ Phase 1.2 (Architecture Design) completed
- ✅ Phase 1.3 (Dependency Management) completed
- ✅ Phase 2.1 (Connection Pooling) completed
- ✅ Phase 2.2 (Authentication System) completed
- ✅ Phase 2.3 (SSL/TLS Integration) completed
- ✅ Phase 3.1 (Metadata Loader) completed
- ✅ Phase 3.2 (Object Browser UI) completed
- ✅ Phase 3.3 (Property Viewers) completed
- ✅ Phase 3.4 (Search and Indexing) completed
- ✅ Phase 4.1 (Editor Component) completed
- ✅ Phase 4.2 (Query Execution) completed
- ✅ Phase 4.3 (Query Management) completed
- ✅ Phase 5.1 (Table Management) completed
- Metadata system from Phase 3.1
- Query execution system from Phase 4.2
- Table management system from Phase 5.1

## Implementation Tasks

### Task 5.2.1: Index Creation and Modification

#### 5.2.1.1: Index Designer Component
**Objective**: Create a comprehensive index creation and modification interface with advanced features and validation

**Implementation Steps:**
1. Implement index creation wizard with column selection and type configuration
2. Add index modification capabilities with ALTER INDEX operations
3. Create index analysis tools for performance evaluation
4. Implement index usage tracking and statistics gathering
5. Add index maintenance operations (rebuild, reindex, vacuum)
6. Create index storage management and tablespace configuration

**Code Changes:**

**File: src/index/index_manager.h**
```cpp
#ifndef SCRATCHROBIN_INDEX_MANAGER_H
#define SCRATCHROBIN_INDEX_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
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

    // Internal methods
    std::string generateIndexName(const std::string& tableName, const std::vector<std::string>& columns);
    std::string getIndexTypeSQL(IndexType type);
    std::string buildIndexColumnsSQL(const std::vector<IndexColumn>& columns);
    std::string buildIndexOptionsSQL(const IndexDefinition& definition);

    IndexDefinition parseIndexDefinitionFromSQL(const std::string& sql);
    IndexStatistics parseIndexStatisticsFromSQL(const std::string& sql);
    IndexPerformance parseIndexPerformanceFromSQL(const std::string& sql);

    std::vector<IndexRecommendation> generateTableRecommendations(const std::string& tableName, const std::string& connectionId);
    std::vector<IndexRecommendation> generateQueryRecommendations(const std::string& query, const std::string& connectionId);

    void logIndexOperation(const std::string& operation, const std::string& indexName, const std::string& connectionId, bool success);
    void updateIndexCache(const std::string& indexName, const std::string& connectionId);

    bool isValidIndexName(const std::string& name);
    bool isValidColumnForIndex(const std::string& tableName, const std::string& columnName, const std::string& connectionId);

    std::vector<std::string> getTableColumns(const std::string& tableName, const std::string& connectionId);
    std::vector<std::string> getTableConstraints(const std::string& tableName, const std::string& connectionId);

    void emitIndexCreated(const std::string& indexName, const std::string& connectionId);
    void emitIndexDropped(const std::string& indexName, const std::string& connectionId);
    void emitIndexModified(const std::string& indexName, const std::string& connectionId);
    void emitMaintenanceCompleted(const IndexMaintenanceOperation& operation);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INDEX_MANAGER_H
```

#### 5.2.1.2: Index Performance Analyzer
**Objective**: Implement comprehensive index performance analysis and optimization tools

**Implementation Steps:**
1. Create index usage analysis and tracking
2. Add index efficiency metrics and scoring
3. Implement index fragmentation analysis
4. Create index size and storage optimization
5. Add index access pattern analysis
6. Implement index recommendation engine

**Code Changes:**

**File: src/index/index_analyzer.h**
```cpp
#ifndef SCRATCHROBIN_INDEX_ANALYZER_H
#define SCRATCHROBIN_INDEX_ANALYZER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <functional>
#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QSqlDatabase>

namespace scratchrobin {

class IIndexManager;
class IMetadataManager;
class ISQLExecutor;

enum class IndexEfficiency {
    EXCELLENT,   // > 95% hit ratio
    GOOD,        // 80-95% hit ratio
    FAIR,        // 60-80% hit ratio
    POOR,        // 40-60% hit ratio
    CRITICAL     // < 40% hit ratio
};

enum class IndexFragmentation {
    NONE,        // < 5% fragmentation
    LOW,         // 5-15% fragmentation
    MODERATE,    // 15-30% fragmentation
    HIGH,        // 30-50% fragmentation
    SEVERE       // > 50% fragmentation
};

enum class IndexUsagePattern {
    FREQUENT,    // Used multiple times per hour
    REGULAR,     // Used daily
    OCCASIONAL,  // Used weekly
    RARE,        // Used monthly or less
    UNUSED       // Never used
};

struct IndexEfficiencyScore {
    std::string indexName;
    IndexEfficiency efficiency = IndexEfficiency::FAIR;
    double hitRatio = 0.0;
    double usageScore = 0.0;
    double performanceScore = 0.0;
    double overallScore = 0.0;
    std::string recommendation;
    QDateTime analyzedAt;
};

struct IndexFragmentationAnalysis {
    std::string indexName;
    IndexFragmentation fragmentation = IndexFragmentation::NONE;
    double fragmentationRatio = 0.0;
    qint64 emptyPages = 0;
    qint64 deletedPages = 0;
    qint64 totalPages = 0;
    bool needsRebuild = false;
    bool needsVacuum = false;
    std::string recommendation;
    QDateTime analyzedAt;
};

struct IndexSizeAnalysis {
    std::string indexName;
    qint64 sizeBytes = 0;
    qint64 estimatedRows = 0;
    double bytesPerRow = 0.0;
    bool isOversized = false;
    bool isUndersized = false;
    std::string sizeCategory;  // "Small", "Medium", "Large", "Huge"
    std::string recommendation;
    QDateTime analyzedAt;
};

struct IndexAccessPattern {
    std::string indexName;
    IndexUsagePattern pattern = IndexUsagePattern::UNUSED;
    qint64 totalAccesses = 0;
    qint64 readAccesses = 0;
    qint64 writeAccesses = 0;
    double readWriteRatio = 0.0;
    std::vector<std::string> accessTimes;  // Peak usage hours
    std::vector<std::string> commonQueries;
    std::string recommendation;
    QDateTime analyzedAt;
};

struct IndexOptimizationPlan {
    std::string planId;
    std::string indexName;
    std::string title;
    std::string description;
    std::vector<std::string> steps;
    std::vector<std::string> sqlStatements;
    double estimatedImprovement = 0.0;
    double estimatedDowntime = 0.0;
    std::string riskLevel;  // "Low", "Medium", "High"
    bool canBeAutomated = false;
    QDateTime generatedAt;
};

struct IndexAnalysisReport {
    std::string reportId;
    std::string databaseName;
    std::string tableName;
    QDateTime generatedAt;
    std::chrono::milliseconds analysisDuration{0};

    std::vector<IndexEfficiencyScore> efficiencyScores;
    std::vector<IndexFragmentationAnalysis> fragmentationAnalysis;
    std::vector<IndexSizeAnalysis> sizeAnalysis;
    std::vector<IndexAccessPattern> accessPatterns;
    std::vector<IndexOptimizationPlan> optimizationPlans;

    // Summary statistics
    int totalIndexes = 0;
    int excellentIndexes = 0;
    int goodIndexes = 0;
    int fairIndexes = 0;
    int poorIndexes = 0;
    int criticalIndexes = 0;

    int fragmentedIndexes = 0;
    int oversizedIndexes = 0;
    int unusedIndexes = 0;

    double averageHitRatio = 0.0;
    qint64 totalIndexSize = 0;

    std::string overallHealth;  // "Excellent", "Good", "Fair", "Poor", "Critical"
    std::vector<std::string> recommendations;
    std::vector<std::string> warnings;
    std::vector<std::string> criticalIssues;
};

class IIndexAnalyzer {
public:
    virtual ~IIndexAnalyzer() = default;

    virtual void setIndexManager(std::shared_ptr<IIndexManager> indexManager) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;

    virtual IndexEfficiencyScore analyzeIndexEfficiency(const std::string& indexName, const std::string& connectionId) = 0;
    virtual IndexFragmentationAnalysis analyzeIndexFragmentation(const std::string& indexName, const std::string& connectionId) = 0;
    virtual IndexSizeAnalysis analyzeIndexSize(const std::string& indexName, const std::string& connectionId) = 0;
    virtual IndexAccessPattern analyzeIndexAccessPattern(const std::string& indexName, const std::string& connectionId) = 0;

    virtual IndexAnalysisReport analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) = 0;
    virtual IndexAnalysisReport analyzeDatabaseIndexes(const std::string& connectionId) = 0;
    virtual IndexAnalysisReport analyzeQueryIndexes(const std::string& query, const std::string& connectionId) = 0;

    virtual std::vector<IndexOptimizationPlan> generateOptimizationPlans(const std::string& tableName, const std::string& connectionId) = 0;
    virtual void implementOptimizationPlan(const std::string& planId, const std::string& connectionId) = 0;

    virtual std::string generateAnalysisReport(const IndexAnalysisReport& report, const std::string& format = "HTML") = 0;
    virtual void exportAnalysisReport(const IndexAnalysisReport& report, const std::string& filePath) = 0;

    virtual void startMonitoring(const std::string& indexName, const std::string& connectionId) = 0;
    virtual void stopMonitoring(const std::string& indexName, const std::string& connectionId) = 0;
    virtual std::vector<std::string> getMonitoredIndexes(const std::string& connectionId) = 0;

    virtual std::unordered_map<std::string, double> getIndexHealthMetrics(const std::string& connectionId) = 0;
    virtual std::vector<std::pair<std::string, double>> getTopProblematicIndexes(const std::string& connectionId, int limit = 10) = 0;
    virtual std::vector<std::pair<std::string, double>> getMostUsedIndexes(const std::string& connectionId, int limit = 10) = 0;

    using AnalysisCompletedCallback = std::function<void(const IndexAnalysisReport&)>;
    using OptimizationCompletedCallback = std::function<void(const std::string&, bool)>;

    virtual void setAnalysisCompletedCallback(AnalysisCompletedCallback callback) = 0;
    virtual void setOptimizationCompletedCallback(OptimizationCompletedCallback callback) = 0;
};

class IndexAnalyzer : public QObject, public IIndexAnalyzer {
    Q_OBJECT

public:
    IndexAnalyzer(QObject* parent = nullptr);
    ~IndexAnalyzer() override;

    // IIndexAnalyzer implementation
    void setIndexManager(std::shared_ptr<IIndexManager> indexManager) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;

    IndexEfficiencyScore analyzeIndexEfficiency(const std::string& indexName, const std::string& connectionId) override;
    IndexFragmentationAnalysis analyzeIndexFragmentation(const std::string& indexName, const std::string& connectionId) override;
    IndexSizeAnalysis analyzeIndexSize(const std::string& indexName, const std::string& connectionId) override;
    IndexAccessPattern analyzeIndexAccessPattern(const std::string& indexName, const std::string& connectionId) override;

    IndexAnalysisReport analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) override;
    IndexAnalysisReport analyzeDatabaseIndexes(const std::string& connectionId) override;
    IndexAnalysisReport analyzeQueryIndexes(const std::string& query, const std::string& connectionId) override;

    std::vector<IndexOptimizationPlan> generateOptimizationPlans(const std::string& tableName, const std::string& connectionId) override;
    void implementOptimizationPlan(const std::string& planId, const std::string& connectionId) override;

    std::string generateAnalysisReport(const IndexAnalysisReport& report, const std::string& format = "HTML") override;
    void exportAnalysisReport(const IndexAnalysisReport& report, const std::string& filePath) override;

    void startMonitoring(const std::string& indexName, const std::string& connectionId) override;
    void stopMonitoring(const std::string& indexName, const std::string& connectionId) override;
    std::vector<std::string> getMonitoredIndexes(const std::string& connectionId) override;

    std::unordered_map<std::string, double> getIndexHealthMetrics(const std::string& connectionId) override;
    std::vector<std::pair<std::string, double>> getTopProblematicIndexes(const std::string& connectionId, int limit = 10) override;
    std::vector<std::pair<std::string, double>> getMostUsedIndexes(const std::string& connectionId, int limit = 10) override;

    void setAnalysisCompletedCallback(AnalysisCompletedCallback callback) override;
    void setOptimizationCompletedCallback(OptimizationCompletedCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IIndexManager> indexManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;

    // Callbacks
    AnalysisCompletedCallback analysisCompletedCallback_;
    OptimizationCompletedCallback optimizationCompletedCallback_;

    // Internal methods
    IndexEfficiencyScore calculateEfficiencyScore(const std::string& indexName, const std::string& connectionId);
    IndexFragmentationAnalysis calculateFragmentation(const std::string& indexName, const std::string& connectionId);
    IndexSizeAnalysis calculateSizeAnalysis(const std::string& indexName, const std::string& connectionId);
    IndexAccessPattern calculateAccessPattern(const std::string& indexName, const std::string& connectionId);

    IndexEfficiency determineEfficiency(double hitRatio, double usageScore);
    IndexFragmentation determineFragmentation(double fragmentationRatio);
    IndexUsagePattern determineUsagePattern(qint64 totalAccesses, const QDateTime& lastUsed);

    std::vector<IndexOptimizationPlan> generateRebuildPlans(const std::vector<IndexFragmentationAnalysis>& fragmentation);
    std::vector<IndexOptimizationPlan> generateSizePlans(const std::vector<IndexSizeAnalysis>& sizes);
    std::vector<IndexOptimizationPlan> generateUsagePlans(const std::vector<IndexAccessPattern>& patterns);

    std::string generateHTMLReport(const IndexAnalysisReport& report);
    std::string generateJSONReport(const IndexAnalysisReport& report);
    std::string generateXMLReport(const IndexAnalysisReport& report);

    void collectIndexUsageStatistics(const std::string& indexName, const std::string& connectionId);
    void collectQueryExecutionStatistics(const std::string& connectionId);

    std::unordered_map<std::string, qint64> getIndexUsageCounts(const std::string& connectionId);
    std::unordered_map<std::string, QDateTime> getIndexLastUsedTimes(const std::string& connectionId);

    double calculateHitRatio(const std::string& indexName, const std::string& connectionId);
    double calculateFragmentationRatio(const std::string& indexName, const std::string& connectionId);

    bool isIndexBeingUsed(const std::string& indexName, const std::string& connectionId);
    std::vector<std::string> findUnusedIndexes(const std::string& connectionId);

    void emitAnalysisCompleted(const IndexAnalysisReport& report);
    void emitOptimizationCompleted(const std::string& planId, bool success);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INDEX_ANALYZER_H
```

## Summary

This phase 5.2 implementation provides comprehensive index management functionality for ScratchRobin with advanced index creation, modification, performance analysis, usage tracking, and optimization capabilities with seamless integration with the existing table management and query execution systems.

✅ **Index Manager Core**: Comprehensive index creation, modification, and management system
✅ **Index Performance Analyzer**: Advanced index analysis with efficiency scoring and fragmentation detection
✅ **Index Usage Tracking**: Real-time index usage monitoring and access pattern analysis
✅ **Index Optimization Engine**: Intelligent index recommendations and optimization planning
✅ **Index Maintenance Tools**: Index rebuild, vacuum, and maintenance operation management
✅ **Index Statistics Collection**: Comprehensive index statistics gathering and analysis
✅ **Index Health Monitoring**: Real-time index health metrics and alerting
✅ **Index Recommendation System**: Automated index suggestions based on query patterns

The index management system provides enterprise-grade index optimization and management capabilities with comprehensive analysis, monitoring, and optimization features comparable to commercial database management tools.

**Next Phase**: Phase 5.3 - Constraint Management Implementation
