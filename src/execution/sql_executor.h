#ifndef SCRATCHROBIN_SQL_EXECUTOR_H
#define SCRATCHROBIN_SQL_EXECUTOR_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <QObject>
#include <QTimer>
#include <QVariant>
#include <QMetaType>
#include "types/query_types.h"
#include <QThreadPool>

namespace scratchrobin {

class IConnectionManager;
class IConnection;
class IMetadataManager;
class ITextEditor;

// QueryState is defined in types/query_types.h

// QueryType is defined in types/query_types.h

enum class ExecutionMode {
    SYNCHRONOUS,
    ASYNCHRONOUS,
    BATCH,
    TRANSACTIONAL
};

enum class ResultFormat {
    TABLE,
    JSON,
    XML,
    CSV,
    TSV,
    CUSTOM
};

struct QueryParameter {
    std::string name;
    QVariant value;
    QMetaType::Type type = QMetaType::UnknownType;
    bool isNull = false;
    int position = -1;
    std::string description;
};

struct QueryExecutionContext {
    std::string connectionId;
    std::string databaseName;
    std::string schemaName;
    std::string userName;
    std::chrono::milliseconds timeout{30000};
    ExecutionMode mode = ExecutionMode::SYNCHRONOUS;
    bool autoCommit = true;
    bool readOnly = false;
    int fetchSize = 1000;
    int maxRows = -1; // -1 means no limit
    std::unordered_map<std::string, std::string> properties;
};

struct QueryResult {
    std::string queryId;
    QueryState state = QueryState::PENDING;
    QueryType queryType = QueryType::UNKNOWN;
    std::string originalQuery;
    std::string executedQuery;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds executionTime{0};
    std::chrono::milliseconds networkTime{0};
    std::chrono::milliseconds processingTime{0};

    // Results
    std::vector<std::string> columnNames;
    std::vector<QMetaType::Type> columnTypes;
    std::vector<std::vector<QVariant>> rows;
    int rowCount = 0;
    int affectedRows = 0;
    int totalRows = 0; // For paginated results
    bool hasMoreRows = false;

    // Metadata
    std::string executionPlan;
    std::unordered_map<std::string, QVariant> statistics;
    std::vector<std::string> warnings;
    std::vector<std::string> infoMessages;

    // Status
    bool success = false;
    std::string errorMessage;
    std::string errorCode;
    int errorPosition = -1;
    std::string sqlState;

    // Performance
    double rowsPerSecond = 0.0;
    double bytesPerSecond = 0.0;
    std::size_t resultSizeBytes = 0;

    // Connection info
    std::string connectionId;
    std::string databaseName;
    std::string serverVersion;

    std::chrono::system_clock::time_point completedAt;
};

struct QueryBatch {
    std::string batchId;
    std::vector<std::string> queries;
    std::vector<QueryParameter> parameters;
    QueryExecutionContext context;
    bool stopOnError = true;
    bool transactional = true;
    std::vector<QueryResult> results;
    QueryState state = QueryState::PENDING;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds totalTime{0};
    bool success = false;
    std::string errorMessage;
    int completedQueries = 0;
    int totalQueries = 0;
};

struct QueryExecutionOptions {
    bool enableProfiling = true;
    bool enableTracing = false;
    bool enableResultCaching = false;
    int resultCacheTTLSeconds = 300;
    bool enableQueryLogging = true;
    bool enablePerformanceMonitoring = true;
    bool enableErrorReporting = true;
    bool enableAutoReconnect = true;
    int maxRetryAttempts = 3;
    std::chrono::milliseconds retryDelay{1000};
    bool enableProgressReporting = true;
    int progressReportIntervalMs = 100;
    bool enableResultStreaming = false;
    int streamingBufferSize = 10000;
};

class ISQLExecutor {
public:
    virtual ~ISQLExecutor() = default;

    virtual void initialize(const QueryExecutionOptions& options) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setTextEditor(std::shared_ptr<ITextEditor> textEditor) = 0;

    virtual QueryResult executeQuery(const std::string& query, const QueryExecutionContext& context) = 0;
    virtual void executeQueryAsync(const std::string& query, const QueryExecutionContext& context) = 0;

    virtual QueryResult executeQueryWithParams(const std::string& query,
                                             const std::vector<QueryParameter>& parameters,
                                             const QueryExecutionContext& context) = 0;
    virtual void executeQueryWithParamsAsync(const std::string& query,
                                           const std::vector<QueryParameter>& parameters,
                                           const QueryExecutionContext& context) = 0;

    virtual QueryBatch executeBatch(const std::vector<std::string>& queries,
                                  const QueryExecutionContext& context) = 0;
    virtual void executeBatchAsync(const std::vector<std::string>& queries,
                                 const QueryExecutionContext& context) = 0;

    virtual bool cancelQuery(const std::string& queryId) = 0;
    virtual bool cancelBatch(const std::string& batchId) = 0;
    virtual QueryState getQueryState(const std::string& queryId) const = 0;
    virtual QueryState getBatchState(const std::string& batchId) const = 0;

    virtual std::vector<QueryResult> getQueryHistory(int limit = 100) const = 0;
    virtual QueryResult getQueryResult(const std::string& queryId) const = 0;
    virtual QueryBatch getBatchResult(const std::string& batchId) const = 0;

    virtual void clearQueryHistory() = 0;
    virtual void clearCache() = 0;

    virtual QueryExecutionOptions getOptions() const = 0;
    virtual void updateOptions(const QueryExecutionOptions& options) = 0;

    using QueryProgressCallback = std::function<void(const std::string&, int, int, const std::string&)>;
    using QueryCompletedCallback = std::function<void(const QueryResult&)>;
    using BatchProgressCallback = std::function<void(const std::string&, int, int, const std::string&)>;
    using BatchCompletedCallback = std::function<void(const QueryBatch&)>;

    virtual void setQueryProgressCallback(QueryProgressCallback callback) = 0;
    virtual void setQueryCompletedCallback(QueryCompletedCallback callback) = 0;
    virtual void setBatchProgressCallback(BatchProgressCallback callback) = 0;
    virtual void setBatchCompletedCallback(BatchCompletedCallback callback) = 0;
};

class SQLExecutor : public QObject, public ISQLExecutor {
    Q_OBJECT

public:
    SQLExecutor(QObject* parent = nullptr);
    ~SQLExecutor() override;

    // ISQLExecutor implementation
    void initialize(const QueryExecutionOptions& options) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setTextEditor(std::shared_ptr<ITextEditor> textEditor) override;

    QueryResult executeQuery(const std::string& query, const QueryExecutionContext& context) override;
    void executeQueryAsync(const std::string& query, const QueryExecutionContext& context) override;

    QueryResult executeQueryWithParams(const std::string& query,
                                     const std::vector<QueryParameter>& parameters,
                                     const QueryExecutionContext& context) override;
    void executeQueryWithParamsAsync(const std::string& query,
                                   const std::vector<QueryParameter>& parameters,
                                   const QueryExecutionContext& context) override;

    QueryBatch executeBatch(const std::vector<std::string>& queries,
                          const QueryExecutionContext& context) override;
    void executeBatchAsync(const std::vector<std::string>& queries,
                         const QueryExecutionContext& context) override;

    bool cancelQuery(const std::string& queryId) override;
    bool cancelBatch(const std::string& batchId) override;
    QueryState getQueryState(const std::string& queryId) const = 0;
    QueryState getBatchState(const std::string& batchId) const = 0;

    std::vector<QueryResult> getQueryHistory(int limit = 100) const override;
    QueryResult getQueryResult(const std::string& queryId) const override;
    QueryBatch getBatchResult(const std::string& batchId) const override;

    void clearQueryHistory() override;
    void clearCache() override;

    QueryExecutionOptions getOptions() const override;
    void updateOptions(const QueryExecutionOptions& options) override;

    void setQueryProgressCallback(QueryProgressCallback callback) override;
    void setQueryCompletedCallback(QueryCompletedCallback callback) override;
    void setBatchProgressCallback(BatchProgressCallback callback) override;
    void setBatchCompletedCallback(BatchCompletedCallback callback) override;

private slots:
    void onProgressTimer();
    void onQueryCompleted();
    void onBatchCompleted();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ITextEditor> textEditor_;

    // Execution infrastructure
    QueryExecutionOptions options_;
    QThreadPool* threadPool_;
    std::unordered_map<std::string, std::future<QueryResult>> activeQueries_;
    std::unordered_map<std::string, std::future<QueryBatch>> activeBatches_;
    std::unordered_map<std::string, std::atomic<bool>> cancellationFlags_;
    std::deque<QueryResult> queryHistory_;
    std::unordered_map<std::string, QueryResult> queryResults_;
    std::unordered_map<std::string, QueryBatch> batchResults_;

    // Timers
    QTimer* progressTimer_;

    // Callbacks
    QueryProgressCallback queryProgressCallback_;
    QueryCompletedCallback queryCompletedCallback_;
    BatchProgressCallback batchProgressCallback_;
    BatchCompletedCallback batchCompletedCallback_;

    // Internal methods
    std::string generateQueryId();
    std::string generateBatchId();

    QueryResult executeQueryInternal(const std::string& queryId, const std::string& query,
                                   const std::vector<QueryParameter>& parameters,
                                   const QueryExecutionContext& context);

    QueryBatch executeBatchInternal(const std::string& batchId,
                                  const std::vector<std::string>& queries,
                                  const QueryExecutionContext& context);

    QueryResult executeSingleQuery(const std::string& queryId, const std::string& query,
                                 const std::vector<QueryParameter>& parameters,
                                 const QueryExecutionContext& context);

    void processQueryResult(QueryResult& result);
    void validateQuery(const std::string& query, QueryType& queryType);
    void prepareQuery(std::string& query, const std::vector<QueryParameter>& parameters);

    std::shared_ptr<IConnection> getConnection(const std::string& connectionId);
    void returnConnection(const std::string& connectionId, std::shared_ptr<IConnection> connection);

    void updateQueryProgress(const std::string& queryId, int progress, const std::string& message);
    void updateBatchProgress(const std::string& batchId, int completed, int total, const std::string& message);

    void handleQueryError(QueryResult& result, const std::string& error, const std::string& sqlState = "");
    void handleBatchError(QueryBatch& batch, const std::string& error);

    void logQueryExecution(const QueryResult& result);
    void logBatchExecution(const QueryBatch& batch);

    void cacheQueryResult(const std::string& queryId, const QueryResult& result);
    QueryResult getCachedResult(const std::string& queryId);

    void pruneQueryHistory();
    void pruneCache();

    bool isQueryCancellable(const std::string& queryId) const;
    bool isBatchCancellable(const std::string& batchId) const;

    // Thread-safe operations
    mutable std::mutex queryMutex_;
    mutable std::mutex batchMutex_;
    mutable std::mutex historyMutex_;
    mutable std::mutex cacheMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SQL_EXECUTOR_H
