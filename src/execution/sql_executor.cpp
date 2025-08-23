#include "execution/sql_executor.h"
#include "core/connection_manager.h"
#include "metadata/metadata_manager.h"
#include "editor/text_editor.h"
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlField>
#include <QVariant>
#include <QThread>
#include <QMetaType>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

namespace scratchrobin {

// Helper function to convert QueryType to string
std::string queryTypeToString(QueryType type) {
    switch (type) {
        case QueryType::SELECT: return "SELECT";
        case QueryType::INSERT: return "INSERT";
        case QueryType::UPDATE: return "UPDATE";
        case QueryType::DELETE: return "DELETE";
        case QueryType::CREATE: return "CREATE";
        case QueryType::ALTER: return "ALTER";
        case QueryType::DROP: return "DROP";
        case QueryType::UNKNOWN: return "UNKNOWN";
        case QueryType::COMMIT: return "COMMIT";
        case QueryType::ROLLBACK: return "ROLLBACK";
        default: return "UNKNOWN";
    }
}

// Helper function to determine query type from SQL
QueryType determineQueryType(const std::string& query) {
    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    // Remove leading whitespace and comments
    size_t start = upperQuery.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        upperQuery = upperQuery.substr(start);
    }

    if (upperQuery.substr(0, 6) == "SELECT") return QueryType::SELECT;
    if (upperQuery.substr(0, 6) == "INSERT") return QueryType::INSERT;
    if (upperQuery.substr(0, 6) == "UPDATE") return QueryType::UPDATE;
    if (upperQuery.substr(0, 6) == "DELETE") return QueryType::DELETE;
    if (upperQuery.substr(0, 6) == "CREATE") return QueryType::CREATE;
    if (upperQuery.substr(0, 5) == "ALTER") return QueryType::ALTER;
    if (upperQuery.substr(0, 4) == "DROP") return QueryType::DROP;
    // TRUNCATE not in QueryType enum
    // MERGE, CALL, EXPLAIN, DESCRIBE, SHOW, USE, SET not in QueryType enum
    if (upperQuery.substr(0, 6) == "COMMIT") return QueryType::COMMIT;
    if (upperQuery.substr(0, 8) == "ROLLBACK") return QueryType::ROLLBACK;

    return QueryType::UNKNOWN;
}

// Helper function to format error message
std::string formatErrorMessage(const QSqlError& error) {
    std::stringstream ss;
    ss << "SQL Error [" << error.type() << "]: " << error.text().toStdString();
    if (!error.driverText().isEmpty()) {
        ss << " (Driver: " << error.driverText().toStdString() << ")";
    }
    if (!error.databaseText().isEmpty()) {
        ss << " (Database: " << error.databaseText().toStdString() << ")";
    }
    return ss.str();
}

// Helper function to get current timestamp
std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::now();
}

// Helper function to calculate duration
std::chrono::milliseconds duration(const std::chrono::system_clock::time_point& start,
                                 const std::chrono::system_clock::time_point& end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

class SQLExecutor::Impl {
public:
    Impl(SQLExecutor* parent)
        : parent_(parent), threadPool_(nullptr) {}

    void setupThreadPool() {
        if (!threadPool_) {
            threadPool_ = new QThreadPool();
            threadPool_->setMaxThreadCount(QThread::idealThreadCount());
        }
    }

    QueryResult executeQueryInternal(const std::string& queryId, const std::string& query,
                                   const std::vector<QueryParameter>& parameters,
                                   const QueryExecutionContext& context) {

        QueryResult result;
        result.queryId = queryId;
        result.originalQuery = query;
        result.startTime = now();
        result.state = QueryState::PENDING;
        result.connectionId = context.connectionId;
        result.databaseName = context.databaseName;

        try {
            // Get connection
            auto connection = getConnection(context.connectionId);
            if (!connection) {
                throw std::runtime_error("Failed to obtain database connection");
            }

            result.state = QueryState::EXECUTING;

            // Prepare query
            std::string preparedQuery = prepareQuery(query, parameters);
            result.executedQuery = preparedQuery;

            // Execute query
            QSqlQuery sqlQuery(QString::fromStdString(connection->getDatabaseName()));
            if (!parameters.empty()) {
                sqlQuery.prepare(QString::fromStdString(preparedQuery));

                // Bind parameters
                for (const auto& param : parameters) {
                    if (param.position >= 0) {
                        sqlQuery.bindValue(param.position, param.value);
                    } else if (!param.name.empty()) {
                        sqlQuery.bindValue(QString::fromStdString(param.name), param.value);
                    }
                }
            } else {
                sqlQuery.prepare(QString::fromStdString(preparedQuery));
            }

            // Set query timeout
            if (context.timeout.count() > 0) {
                sqlQuery.setForwardOnly(true); // Optimize for forward-only queries
            }

            auto executeStart = now();
            bool querySuccess = sqlQuery.exec();
            auto executeEnd = now();

            result.networkTime = duration(executeStart, executeEnd);
            result.endTime = executeEnd;
            result.executionTime = duration(result.startTime, result.endTime);

            if (!querySuccess) {
                QSqlError error = sqlQuery.lastError();
                result.success = false;
                result.errorMessage = formatErrorMessage(error);
                result.errorCode = error.nativeErrorCode().toStdString();
                result.sqlState = error.type() == QSqlError::ConnectionError ? "08000" : "XX000";
                result.state = QueryState::FAILED;

                // Try to find error position
                if (error.text().contains("position")) {
                    // Extract position from error message
                    QRegularExpression posRegex("position (\\d+)");
                    QRegularExpressionMatch match = posRegex.match(error.text());
                    if (match.hasMatch()) {
                        result.errorPosition = match.captured(1).toInt();
                    }
                }

                returnConnection(context.connectionId, connection);
                return result;
            }

            result.state = QueryState::EXECUTING;

            // Process results
            auto processStart = now();

            // Get column information
            QSqlRecord record = sqlQuery.record();
            for (int i = 0; i < record.count(); ++i) {
                QSqlField field = record.field(i);
                result.columnNames.push_back(field.name().toStdString());

                QMetaType::Type fieldType = static_cast<QMetaType::Type>(field.type());
                result.columnTypes.push_back(fieldType);
            }

            // Get rows
            result.rowCount = 0;
            result.affectedRows = sqlQuery.numRowsAffected();

            if (sqlQuery.isSelect()) {
                int maxRows = context.maxRows > 0 ? context.maxRows : INT_MAX;
                int fetchSize = context.fetchSize > 0 ? context.fetchSize : 1000;

                std::vector<std::vector<QVariant>> rows;
                int rowCount = 0;

                while (sqlQuery.next() && rowCount < maxRows) {
                    std::vector<QVariant> row;
                    for (int i = 0; i < record.count(); ++i) {
                        row.push_back(sqlQuery.value(i));
                    }
                    rows.push_back(row);
                    rowCount++;

                    // Report progress for large result sets
                    if (rowCount % fetchSize == 0 && parent_->queryProgressCallback_) {
                        parent_->queryProgressCallback_(queryId, rowCount, -1, "Fetching rows...");
                    }
                }

                result.rows = std::move(rows);
                result.rowCount = rowCount;
                result.hasMoreRows = sqlQuery.next(); // Check if there are more rows
            }

            auto processEnd = now();
            result.processingTime = duration(processStart, processEnd);

            // Get additional query information
            result.statistics["lastInsertId"] = sqlQuery.lastInsertId();
            result.statistics["numRowsAffected"] = sqlQuery.numRowsAffected();
            result.statistics["size"] = static_cast<int>(result.rows.size());

            // Calculate performance metrics
            if (result.rowCount > 0) {
                result.rowsPerSecond = static_cast<double>(result.rowCount) / (result.executionTime.count() / 1000.0);
            }

            // Estimate result size
            result.resultSizeBytes = estimateResultSize(result);

            result.success = true;
            result.state = QueryState::COMPLETED;
            result.completedAt = now();

            // Handle auto-commit
            // Note: Auto-commit handling would need proper database access
            // For now, skipping this functionality

            returnConnection(context.connectionId, connection);

        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = std::string("Execution error: ") + e.what();
            result.state = QueryState::FAILED;
            result.endTime = now();
            result.executionTime = duration(result.startTime, result.endTime);
            result.completedAt = now();

            // Try to return connection if we got one
            try {
                auto connection = getConnection(context.connectionId);
                if (connection) {
                    returnConnection(context.connectionId, connection);
                }
            } catch (...) {
                // Ignore errors during cleanup
            }
        }

        return result;
    }

    QueryBatch executeBatchInternal(const std::string& batchId,
                                  const std::vector<std::string>& queries,
                                  const QueryExecutionContext& context) {

        QueryBatch batch;
        batch.batchId = batchId;
        batch.queries = queries;
        batch.context = context;
        batch.startTime = now();
        batch.totalQueries = queries.size();
        batch.state = QueryState::PENDING;

        try {
            // Get connection
            auto connection = getConnection(context.connectionId);
            if (!connection) {
                throw std::runtime_error("Failed to obtain database connection");
            }

            // Start transaction if needed
            if (batch.transactional && // Database transaction support check would need proper database access) {
                connection->getDatabaseName().transaction();
            }

            batch.state = QueryState::EXECUTING;

            int completedQueries = 0;
            for (size_t i = 0; i < queries.size(); ++i) {
                const std::string& query = queries[i];

                // Create individual query result
                QueryResult queryResult = executeQueryInternal(
                    generateQueryId(), query, {}, context);

                batch.results.push_back(queryResult);

                if (queryResult.success) {
                    completedQueries++;
                } else if (batch.stopOnError) {
                    // Rollback transaction on error
                    if (batch.transactional && // Database transaction support check would need proper database access) {
                        connection->getDatabaseName().rollback();
                    }
                    batch.success = false;
                    batch.errorMessage = queryResult.errorMessage;
                    batch.state = QueryState::FAILED;
                    break;
                }

                // Report progress
                if (parent_->batchProgressCallback_) {
                    parent_->batchProgressCallback_(batchId, completedQueries, batch.totalQueries,
                                                  "Executing batch queries...");
                }
            }

            batch.completedQueries = completedQueries;

            // Commit transaction if successful
            // Note: Batch transaction handling would need proper database access
            // if (batch.transactional && batch.success) {
            //     connection->getDatabaseName().commit();
            // }

            batch.endTime = now();
            batch.totalTime = duration(batch.startTime, batch.endTime);

            if (batch.success) {
                batch.state = QueryState::COMPLETED;
            }

            returnConnection(context.connectionId, connection);

        } catch (const std::exception& e) {
            batch.success = false;
            batch.errorMessage = std::string("Batch execution error: ") + e.what();
            batch.state = QueryState::FAILED;
            batch.endTime = now();
            batch.totalTime = duration(batch.startTime, batch.endTime);
        }

        return batch;
    }

    std::shared_ptr<IConnection> getConnection(const std::string& connectionId) {
        if (!connectionManager_) {
            throw std::runtime_error("Connection manager not set");
        }

        auto connection = connectionManager_->getConnection(QString::fromStdString(connectionId));
        if (!connection) {
            throw std::runtime_error("Failed to get connection: " + connectionId);
        }

        return connection;
    }

    void returnConnection(const std::string& connectionId, std::shared_ptr<IConnection> connection) {
        // Connection will be automatically returned by shared_ptr
    }

    std::string prepareQuery(const std::string& query, const std::vector<QueryParameter>& parameters) {
        if (parameters.empty()) {
            return query;
        }

        std::string preparedQuery = query;

        // Replace named parameters with positional parameters
        for (const auto& param : parameters) {
            if (!param.name.empty()) {
                std::string placeholder = ":" + param.name;
                std::string::size_type pos = preparedQuery.find(placeholder);
                while (pos != std::string::npos) {
                    preparedQuery.replace(pos, placeholder.length(), "?");
                    pos = preparedQuery.find(placeholder, pos + 1);
                }
            }
        }

        return preparedQuery;
    }

    std::string generateQueryId() {
        static std::atomic<int> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return "query_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
    }

    std::string generateBatchId() {
        static std::atomic<int> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return "batch_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
    }

    std::size_t estimateResultSize(const QueryResult& result) {
        std::size_t size = 0;

        // Estimate column name sizes
        for (const auto& col : result.columnNames) {
            size += col.size();
        }

        // Estimate row data sizes
        for (const auto& row : result.rows) {
            for (const auto& value : row) {
                size += value.toString().toStdString().size();
            }
        }

        return size;
    }

    void logQueryExecution(const QueryResult& result) {
        qDebug() << "Query executed:" << result.queryId.c_str()
                 << "Type:" << queryTypeToString(result.queryType).c_str()
                 << "Rows:" << result.rowCount
                 << "Time:" << result.executionTime.count() << "ms"
                 << "Success:" << result.success;
    }

    void logBatchExecution(const QueryBatch& batch) {
        qDebug() << "Batch executed:" << batch.batchId.c_str()
                 << "Queries:" << batch.totalQueries
                 << "Completed:" << batch.completedQueries
                 << "Time:" << batch.totalTime.count() << "ms"
                 << "Success:" << batch.success;
    }

private:
    SQLExecutor* parent_;
    QThreadPool* threadPool_;
    std::shared_ptr<IConnectionManager> connectionManager_;
};

// SQLExecutor implementation

SQLExecutor::SQLExecutor(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {

    // Set default options
    options_.enableProfiling = true;
    options_.enableTracing = false;
    options_.enableResultCaching = false;
    options_.resultCacheTTLSeconds = 300;
    options_.enableQueryLogging = true;
    options_.enablePerformanceMonitoring = true;
    options_.enableErrorReporting = true;
    options_.enableAutoReconnect = true;
    options_.maxRetryAttempts = 3;
    options_.retryDelay = std::chrono::milliseconds(1000);
    options_.enableProgressReporting = true;
    options_.progressReportIntervalMs = 100;
    options_.enableResultStreaming = false;
    options_.streamingBufferSize = 10000;

    impl_->setupThreadPool();

    // Set up progress timer
    progressTimer_ = new QTimer(this);
    connect(progressTimer_, &QTimer::timeout, this, &SQLExecutor::onProgressTimer);
}

SQLExecutor::~SQLExecutor() = default;

void SQLExecutor::initialize(const QueryExecutionOptions& options) {
    options_ = options;
}

void SQLExecutor::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
    impl_->connectionManager_ = connectionManager;
}

void SQLExecutor::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
}

void SQLExecutor::setTextEditor(std::shared_ptr<ITextEditor> textEditor) {
    textEditor_ = textEditor;
}

QueryResult SQLExecutor::executeQuery(const std::string& query, const QueryExecutionContext& context) {
    std::string queryId = impl_->generateQueryId();

    try {
        QueryResult result = impl_->executeQueryInternal(queryId, query, {}, context);

        // Store result
        {
            std::lock_guard<std::mutex> lock(historyMutex_);
            queryHistory_.push_front(result);
            if (queryHistory_.size() > 100) { // Keep last 100 queries
                queryHistory_.pop_back();
            }
            queryResults_[queryId] = result;
        }

        // Log execution
        if (options_.enableQueryLogging) {
            impl_->logQueryExecution(result);
        }

        // Notify callback
        if (queryCompletedCallback_) {
            queryCompletedCallback_(result);
        }

        return result;

    } catch (const std::exception& e) {
        QueryResult errorResult;
        errorResult.queryId = queryId;
        errorResult.originalQuery = query;
        errorResult.success = false;
        errorResult.errorMessage = std::string("Query execution failed: ") + e.what();
        errorResult.state = QueryState::FAILED;
        errorResult.startTime = now();
        errorResult.endTime = now();
        errorResult.executionTime = std::chrono::milliseconds(0);
        errorResult.completedAt = now();

        {
            std::lock_guard<std::mutex> lock(historyMutex_);
            queryHistory_.push_front(errorResult);
            queryResults_[queryId] = errorResult;
        }

        if (queryCompletedCallback_) {
            queryCompletedCallback_(errorResult);
        }

        return errorResult;
    }
}

void SQLExecutor::executeQueryAsync(const std::string& query, const QueryExecutionContext& context) {
    std::string queryId = impl_->generateQueryId();

    // Set up cancellation flag
    {
        std::lock_guard<std::mutex> lock(queryMutex_);
        cancellationFlags_[queryId] = false;
    }

    // Start async execution
    auto future = std::async(std::launch::async, [this, queryId, query, context]() {
        try {
            QueryResult result = impl_->executeQueryInternal(queryId, query, {}, context);

            // Store result
            {
                std::lock_guard<std::mutex> lock(historyMutex_);
                queryHistory_.push_front(result);
                queryResults_[queryId] = result;
            }

            // Log execution
            if (options_.enableQueryLogging) {
                impl_->logQueryExecution(result);
            }

            // Notify callback
            if (queryCompletedCallback_) {
                queryCompletedCallback_(result);
            }

            return result;

        } catch (const std::exception& e) {
            QueryResult errorResult;
            errorResult.queryId = queryId;
            errorResult.originalQuery = query;
            errorResult.success = false;
            errorResult.errorMessage = std::string("Async query execution failed: ") + e.what();
            errorResult.state = QueryState::FAILED;
            errorResult.startTime = now();
            errorResult.endTime = now();
            errorResult.executionTime = std::chrono::milliseconds(0);
            errorResult.completedAt = now();

            {
                std::lock_guard<std::mutex> lock(historyMutex_);
                queryHistory_.push_front(errorResult);
                queryResults_[queryId] = errorResult;
            }

            if (queryCompletedCallback_) {
                queryCompletedCallback_(errorResult);
            }

            return errorResult;
        }
    });

    {
        std::lock_guard<std::mutex> lock(queryMutex_);
        activeQueries_[queryId] = std::move(future);
    }
}

QueryResult SQLExecutor::executeQueryWithParams(const std::string& query,
                                             const std::vector<QueryParameter>& parameters,
                                             const QueryExecutionContext& context) {
    std::string queryId = impl_->generateQueryId();

    try {
        QueryResult result = impl_->executeQueryInternal(queryId, query, parameters, context);

        // Store result
        {
            std::lock_guard<std::mutex> lock(historyMutex_);
            queryHistory_.push_front(result);
            if (queryHistory_.size() > 100) {
                queryHistory_.pop_back();
            }
            queryResults_[queryId] = result;
        }

        // Log execution
        if (options_.enableQueryLogging) {
            impl_->logQueryExecution(result);
        }

        // Notify callback
        if (queryCompletedCallback_) {
            queryCompletedCallback_(result);
        }

        return result;

    } catch (const std::exception& e) {
        QueryResult errorResult;
        errorResult.queryId = queryId;
        errorResult.originalQuery = query;
        errorResult.success = false;
        errorResult.errorMessage = std::string("Query execution failed: ") + e.what();
        errorResult.state = QueryState::FAILED;
        errorResult.startTime = now();
        errorResult.endTime = now();
        errorResult.executionTime = std::chrono::milliseconds(0);
        errorResult.completedAt = now();

        {
            std::lock_guard<std::mutex> lock(historyMutex_);
            queryHistory_.push_front(errorResult);
            queryResults_[queryId] = errorResult;
        }

        if (queryCompletedCallback_) {
            queryCompletedCallback_(errorResult);
        }

        return errorResult;
    }
}

void SQLExecutor::executeQueryWithParamsAsync(const std::string& query,
                                           const std::vector<QueryParameter>& parameters,
                                           const QueryExecutionContext& context) {
    std::string queryId = impl_->generateQueryId();

    // Set up cancellation flag
    {
        std::lock_guard<std::mutex> lock(queryMutex_);
        cancellationFlags_[queryId] = false;
    }

    // Start async execution
    auto future = std::async(std::launch::async, [this, queryId, query, parameters, context]() {
        try {
            QueryResult result = impl_->executeQueryInternal(queryId, query, parameters, context);

            // Store result
            {
                std::lock_guard<std::mutex> lock(historyMutex_);
                queryHistory_.push_front(result);
                queryResults_[queryId] = result;
            }

            // Log execution
            if (options_.enableQueryLogging) {
                impl_->logQueryExecution(result);
            }

            // Notify callback
            if (queryCompletedCallback_) {
                queryCompletedCallback_(result);
            }

            return result;

        } catch (const std::exception& e) {
            QueryResult errorResult;
            errorResult.queryId = queryId;
            errorResult.originalQuery = query;
            errorResult.success = false;
            errorResult.errorMessage = std::string("Async query execution failed: ") + e.what();
            errorResult.state = QueryState::FAILED;
            errorResult.startTime = now();
            errorResult.endTime = now();
            errorResult.executionTime = std::chrono::milliseconds(0);
            errorResult.completedAt = now();

            {
                std::lock_guard<std::mutex> lock(historyMutex_);
                queryHistory_.push_front(errorResult);
                queryResults_[queryId] = errorResult;
            }

            if (queryCompletedCallback_) {
                queryCompletedCallback_(errorResult);
            }

            return errorResult;
        }
    });

    {
        std::lock_guard<std::mutex> lock(queryMutex_);
        activeQueries_[queryId] = std::move(future);
    }
}

QueryBatch SQLExecutor::executeBatch(const std::vector<std::string>& queries,
                                  const QueryExecutionContext& context) {
    std::string batchId = impl_->generateBatchId();

    try {
        QueryBatch batch = impl_->executeBatchInternal(batchId, queries, context);

        // Store batch result
        {
            std::lock_guard<std::mutex> lock(batchMutex_);
            batchResults_[batchId] = batch;
        }

        // Log execution
        if (options_.enableQueryLogging) {
            impl_->logBatchExecution(batch);
        }

        // Notify callback
        if (batchCompletedCallback_) {
            batchCompletedCallback_(batch);
        }

        return batch;

    } catch (const std::exception& e) {
        QueryBatch errorBatch;
        errorBatch.batchId = batchId;
        errorBatch.queries = queries;
        errorBatch.success = false;
        errorBatch.errorMessage = std::string("Batch execution failed: ") + e.what();
        errorBatch.state = QueryState::FAILED;
        errorBatch.startTime = now();
        errorBatch.endTime = now();
        errorBatch.totalTime = std::chrono::milliseconds(0);
        errorBatch.totalQueries = queries.size();

        {
            std::lock_guard<std::mutex> lock(batchMutex_);
            batchResults_[batchId] = errorBatch;
        }

        if (batchCompletedCallback_) {
            batchCompletedCallback_(errorBatch);
        }

        return errorBatch;
    }
}

void SQLExecutor::executeBatchAsync(const std::vector<std::string>& queries,
                                 const QueryExecutionContext& context) {
    std::string batchId = impl_->generateBatchId();

    // Set up cancellation flag
    {
        std::lock_guard<std::mutex> lock(batchMutex_);
        cancellationFlags_[batchId] = false;
    }

    // Start async execution
    auto future = std::async(std::launch::async, [this, batchId, queries, context]() {
        try {
            QueryBatch batch = impl_->executeBatchInternal(batchId, queries, context);

            // Store batch result
            {
                std::lock_guard<std::mutex> lock(batchMutex_);
                batchResults_[batchId] = batch;
            }

            // Log execution
            if (options_.enableQueryLogging) {
                impl_->logBatchExecution(batch);
            }

            // Notify callback
            if (batchCompletedCallback_) {
                batchCompletedCallback_(batch);
            }

            return batch;

        } catch (const std::exception& e) {
            QueryBatch errorBatch;
            errorBatch.batchId = batchId;
            errorBatch.queries = queries;
            errorBatch.success = false;
            errorBatch.errorMessage = std::string("Async batch execution failed: ") + e.what();
            errorBatch.state = QueryState::FAILED;
            errorBatch.startTime = now();
            errorBatch.endTime = now();
            errorBatch.totalTime = std::chrono::milliseconds(0);
            errorBatch.totalQueries = queries.size();

            {
                std::lock_guard<std::mutex> lock(batchMutex_);
                batchResults_[batchId] = errorBatch;
            }

            if (batchCompletedCallback_) {
                batchCompletedCallback_(errorBatch);
            }

            return errorBatch;
        }
    });

    {
        std::lock_guard<std::mutex> lock(batchMutex_);
        activeBatches_[batchId] = std::move(future);
    }
}

bool SQLExecutor::cancelQuery(const std::string& queryId) {
    std::lock_guard<std::mutex> lock(queryMutex_);

    auto it = cancellationFlags_.find(queryId);
    if (it != cancellationFlags_.end()) {
        it->second = true;
        return true;
    }

    return false;
}

bool SQLExecutor::cancelBatch(const std::string& batchId) {
    std::lock_guard<std::mutex> lock(batchMutex_);

    auto it = cancellationFlags_.find(batchId);
    if (it != cancellationFlags_.end()) {
        it->second = true;
        return true;
    }

    return false;
}

QueryState SQLExecutor::getQueryState(const std::string& queryId) const {
    std::lock_guard<std::mutex> lock(queryMutex_);

    auto it = activeQueries_.find(queryId);
    if (it != activeQueries_.end()) {
        if (it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            return QueryState::COMPLETED;
        }
        return QueryState::EXECUTING;
    }

    auto resultIt = queryResults_.find(queryId);
    if (resultIt != queryResults_.end()) {
        return resultIt->second.state;
    }

    return QueryState::PENDING;
}

QueryState SQLExecutor::getBatchState(const std::string& batchId) const {
    std::lock_guard<std::mutex> lock(batchMutex_);

    auto it = activeBatches_.find(batchId);
    if (it != activeBatches_.end()) {
        if (it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            return QueryState::COMPLETED;
        }
        return QueryState::EXECUTING;
    }

    auto resultIt = batchResults_.find(batchId);
    if (resultIt != batchResults_.end()) {
        return resultIt->second.state;
    }

    return QueryState::PENDING;
}

std::vector<QueryResult> SQLExecutor::getQueryHistory(int limit) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    std::vector<QueryResult> history;
    int count = 0;
    for (const auto& result : queryHistory_) {
        if (count >= limit) break;
        history.push_back(result);
        count++;
    }

    return history;
}

QueryResult SQLExecutor::getQueryResult(const std::string& queryId) const {
    std::lock_guard<std::mutex> lock(historyMutex_);

    auto it = queryResults_.find(queryId);
    if (it != queryResults_.end()) {
        return it->second;
    }

    // Return empty result if not found
    QueryResult emptyResult;
    emptyResult.queryId = queryId;
    emptyResult.success = false;
    emptyResult.errorMessage = "Query result not found";
    return emptyResult;
}

QueryBatch SQLExecutor::getBatchResult(const std::string& batchId) const {
    std::lock_guard<std::mutex> lock(batchMutex_);

    auto it = batchResults_.find(batchId);
    if (it != batchResults_.end()) {
        return it->second;
    }

    // Return empty batch if not found
    QueryBatch emptyBatch;
    emptyBatch.batchId = batchId;
    emptyBatch.success = false;
    emptyBatch.errorMessage = "Batch result not found";
    return emptyBatch;
}

void SQLExecutor::clearQueryHistory() {
    std::lock_guard<std::mutex> lock(historyMutex_);
    queryHistory_.clear();
    queryResults_.clear();
}

void SQLExecutor::clearCache() {
    // Implementation for clearing any cached results
}

QueryExecutionOptions SQLExecutor::getOptions() const {
    return options_;
}

void SQLExecutor::updateOptions(const QueryExecutionOptions& options) {
    options_ = options;
}

void SQLExecutor::setQueryProgressCallback(QueryProgressCallback callback) {
    queryProgressCallback_ = callback;
}

void SQLExecutor::setQueryCompletedCallback(QueryCompletedCallback callback) {
    queryCompletedCallback_ = callback;
}

void SQLExecutor::setBatchProgressCallback(BatchProgressCallback callback) {
    batchProgressCallback_ = callback;
}

void SQLExecutor::setBatchCompletedCallback(BatchCompletedCallback callback) {
    batchCompletedCallback_ = callback;
}

// Private slot implementations

void SQLExecutor::onProgressTimer() {
    // Implementation for progress reporting
}

void SQLExecutor::onQueryCompleted() {
    // Implementation for query completion handling
}

void SQLExecutor::onBatchCompleted() {
    // Implementation for batch completion handling
}

} // namespace scratchrobin
