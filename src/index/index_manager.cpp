#include "index/index_manager.h"
#include "metadata/metadata_manager.h"
#include "execution/sql_executor.h"
#include "core/connection_manager.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "utils/string_utils.h"
#include <regex>
#include <set>

namespace scratchrobin {

// Helper functions


IndexType stringToIndexType(const std::string& str) {
    if (str == "HASH") return IndexType::HASH;
    if (str == "GIN") return IndexType::GIN;
    if (str == "GIST") return IndexType::GIST;
    if (str == "SPGIST") return IndexType::SPGIST;
    if (str == "BRIN") return IndexType::BRIN;
    if (str == "UNIQUE") return IndexType::UNIQUE;
    if (str == "PARTIAL") return IndexType::PARTIAL;
    if (str == "EXPRESSION") return IndexType::EXPRESSION;
    if (str == "COMPOSITE") return IndexType::COMPOSITE;
    return IndexType::BTREE;
}

std::string indexStatusToString(IndexStatus status) {
    switch (status) {
        case IndexStatus::VALID: return "VALID";
        case IndexStatus::INVALID: return "INVALID";
        case IndexStatus::BUILDING: return "BUILDING";
        case IndexStatus::REBUILDING: return "REBUILDING";
        case IndexStatus::DROPPED: return "DROPPED";
        default: return "UNKNOWN";
    }
}

IndexStatus stringToIndexStatus(const std::string& str) {
    if (str == "INVALID") return IndexStatus::INVALID;
    if (str == "BUILDING") return IndexStatus::BUILDING;
    if (str == "REBUILDING") return IndexStatus::REBUILDING;
    if (str == "DROPPED") return IndexStatus::DROPPED;
    return IndexStatus::VALID;
}

std::string operationToString(IndexOperation op) {
    switch (op) {
        case IndexOperation::CREATE: return "CREATE";
        case IndexOperation::DROP: return "DROP";
        case IndexOperation::REBUILD: return "REBUILD";
        case IndexOperation::RENAME: return "RENAME";
        case IndexOperation::ALTER: return "ALTER";
        case IndexOperation::VACUUM: return "VACUUM";
        case IndexOperation::ANALYZE: return "ANALYZE";
        default: return "UNKNOWN";
    }
}

IndexOperation stringToOperation(const std::string& str) {
    if (str == "DROP") return IndexOperation::DROP;
    if (str == "REBUILD") return IndexOperation::REBUILD;
    if (str == "RENAME") return IndexOperation::RENAME;
    if (str == "ALTER") return IndexOperation::ALTER;
    if (str == "VACUUM") return IndexOperation::VACUUM;
    if (str == "ANALYZE") return IndexOperation::ANALYZE;
    return IndexOperation::CREATE;
}

std::string generateIndexId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "idx_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}



class IndexManager::Impl {
public:
    Impl(IndexManager* parent) : parent_(parent) {}

    void initialize() {
        // Set up database connection for index metadata
        // This would be initialized with a proper database connection
    }

    std::vector<IndexDefinition> getTableIndexes(const std::string& tableName, const std::string& connectionId) {
        std::vector<IndexDefinition> indexes;

        if (!sqlExecutor_) {
            return indexes;
        }

        try {
            // Query to get indexes for a table
            std::string query = R"(
                SELECT
                    i.indexname,
                    i.tablename,
                    i.indexdef,
                    pg_size_pretty(pg_relation_size(i.indexrelid)) as size,
                    idx.indisunique,
                    idx.indisprimary,
                    idx.indisclustered,
                    idx.indnatts,
                    array_to_string(array(
                        select pg_get_indexdef(idx.indexrelid, k + 1, true)
                        from generate_subscripts(idx.indkey, 1) as k
                        order by k
                    ), ', ') as columns
                FROM pg_indexes i
                JOIN pg_class c ON c.relname = i.indexname
                JOIN pg_index idx ON idx.indexrelid = c.oid
                WHERE i.tablename = ')" + tableName + R"('
                ORDER BY i.indexname;
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                for (const auto& row : result.rows) {
                    IndexDefinition index;
                    index.name = row[0].toString().toStdString();
                    index.tableName = row[1].toString().toStdString();

                    // Parse index definition
                    std::string indexDef = row[2].toString().toStdString();
                    index = parseIndexDefinitionFromSQL(indexDef);

                    indexes.push_back(index);
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get table indexes:" << e.what();
        }

        return indexes;
    }

    IndexDefinition getIndexDefinition(const std::string& indexName, const std::string& connectionId) {
        IndexDefinition definition;

        if (!sqlExecutor_) {
            return definition;
        }

        try {
            std::string query = R"(
                SELECT indexdef
                FROM pg_indexes
                WHERE indexname = ')" + indexName + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                std::string indexDef = result.rows[0][0].toString().toStdString();
                definition = parseIndexDefinitionFromSQL(indexDef);
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get index definition:" << e.what();
        }

        return definition;
    }

    std::vector<IndexStatistics> getIndexStatistics(const std::string& indexName, const std::string& connectionId) {
        std::vector<IndexStatistics> statistics;

        if (!sqlExecutor_) {
            return statistics;
        }

        try {
            std::string query = R"(
                SELECT
                    schemaname,
                    tablename,
                    indexname,
                    pg_size_pretty(pg_relation_size(indexrelid)) as size,
                    idx_scan,
                    idx_tup_read,
                    idx_tup_fetch,
                    pg_stat_get_numscans(indexrelid) as scans
                FROM pg_stat_user_indexes
                WHERE indexname = ')" + indexName + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                IndexStatistics stats;
                stats.indexName = result.rows[0][2].toString().toStdString();
                stats.tableName = result.rows[0][1].toString().toStdString();

                // Parse size (this is a simplified version)
                std::string sizeStr = result.rows[0][3].toString().toStdString();
                stats.sizeBytes = parseSizeString(sizeStr);

                stats.scannedTuples = result.rows[0][4].toLongLong();
                stats.tupleCount = result.rows[0][5].toLongLong();

                if (stats.scannedTuples > 0) {
                    stats.selectivity = static_cast<double>(stats.tupleCount) / stats.scannedTuples;
                }

                stats.collectedAt = QDateTime::currentDateTime();
                statistics.push_back(stats);
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get index statistics:" << e.what();
        }

        return statistics;
    }

    std::vector<IndexPerformance> getIndexPerformance(const std::string& indexName, const std::string& connectionId) {
        std::vector<IndexPerformance> performance;

        // This would require analyzing query execution plans and index usage
        // For now, return empty vector
        return performance;
    }

    bool createIndex(const IndexDefinition& definition, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = generateIndexDDL(definition, connectionId);

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000); // 5 minutes for index creation

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->indexCreated(definition.name, connectionId);
                return true;
            } else {
                qWarning() << "Failed to create index:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to create index:" << e.what();
            return false;
        }
    }

    bool dropIndex(const std::string& indexName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "DROP INDEX IF EXISTS " + indexName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(30000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->indexDropped(indexName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to drop index:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to drop index:" << e.what();
            return false;
        }
    }

    bool rebuildIndex(const std::string& indexName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "REINDEX INDEX " + indexName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000); // 5 minutes for rebuild

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->indexModified(indexName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to rebuild index:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to rebuild index:" << e.what();
            return false;
        }
    }

    bool renameIndex(const std::string& oldName, const std::string& newName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string ddl = "ALTER INDEX " + oldName + " RENAME TO " + newName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(30000);

            QueryResult result = sqlExecutor_->executeQuery(ddl, context);

            if (result.success) {
                parent_->indexModified(newName, connectionId);
                return true;
            } else {
                qWarning() << "Failed to rename index:" << result.errorMessage.c_str();
                return false;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to rename index:" << e.what();
            return false;
        }
    }

    bool alterIndex(const std::string& indexName, const IndexDefinition& definition, const std::string& connectionId) {
        // PostgreSQL doesn't support ALTER INDEX for most properties
        // This would need to be implemented as DROP and CREATE for most changes
        return false;
    }

    std::vector<IndexMaintenanceOperation> getMaintenanceHistory(const std::string& indexName, const std::string& connectionId) {
        // This would require a maintenance log table
        // For now, return empty vector
        return std::vector<IndexMaintenanceOperation>();
    }

    IndexMaintenanceOperation performMaintenance(const std::string& indexName, IndexOperation operation, const std::string& connectionId) {
        IndexMaintenanceOperation maintenanceOp;
        maintenanceOp.operationId = generateOperationId();
        maintenanceOp.indexName = indexName;
        maintenanceOp.operation = operation;
        maintenanceOp.startedAt = QDateTime::currentDateTime();

        if (!sqlExecutor_) {
            maintenanceOp.success = false;
            maintenanceOp.errorMessage = "SQL executor not available";
            maintenanceOp.completedAt = QDateTime::currentDateTime();
            return maintenanceOp;
        }

        try {
            std::string sqlStatement;

            switch (operation) {
                case IndexOperation::REBUILD:
                    sqlStatement = "REINDEX INDEX " + indexName + ";";
                    break;
                case IndexOperation::VACUUM:
                    sqlStatement = "VACUUM " + indexName + ";";
                    break;
                case IndexOperation::ANALYZE:
                    sqlStatement = "ANALYZE " + indexName + ";";
                    break;
                default:
                    maintenanceOp.success = false;
                    maintenanceOp.errorMessage = "Unsupported maintenance operation";
                    maintenanceOp.completedAt = QDateTime::currentDateTime();
                    return maintenanceOp;
            }

            maintenanceOp.sqlStatement = sqlStatement;

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000); // 5 minutes for maintenance

            QueryResult result = sqlExecutor_->executeQuery(sqlStatement, context);

            maintenanceOp.success = result.success;
            if (!result.success) {
                maintenanceOp.errorMessage = result.errorMessage;
            } else {
                maintenanceOp.outputMessage = "Operation completed successfully";
            }

            maintenanceOp.completedAt = QDateTime::currentDateTime();

            if (maintenanceOp.success) {
                parent_->indexModified(indexName, connectionId);
            }

            parent_->maintenanceCompleted(maintenanceOp);

        } catch (const std::exception& e) {
            maintenanceOp.success = false;
            maintenanceOp.errorMessage = std::string("Maintenance failed: ") + e.what();
            maintenanceOp.completedAt = QDateTime::currentDateTime();
        }

        return maintenanceOp;
    }

    std::vector<IndexRecommendation> analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) {
        std::vector<IndexRecommendation> recommendations;

        // This would analyze the table structure and existing indexes
        // to provide recommendations for missing indexes
        return recommendations;
    }

    std::vector<IndexRecommendation> analyzeQueryIndexes(const std::string& query, const std::string& connectionId) {
        std::vector<IndexRecommendation> recommendations;

        // This would analyze the query and suggest indexes for optimal performance
        return recommendations;
    }

    void implementRecommendation(const std::string& recommendationId, const std::string& connectionId) {
        // This would implement a specific index recommendation
    }

    std::string generateIndexDDL(const IndexDefinition& definition, const std::string& connectionId) {
        std::stringstream ddl;

        ddl << "CREATE ";

        if (definition.isUnique) {
            ddl << "UNIQUE ";
        }

        ddl << "INDEX ";

        if (definition.isConcurrent) {
            ddl << "CONCURRENTLY ";
        }

        ddl << definition.name << " ON " << definition.tableName << " ";

        if (definition.type != IndexType::BTREE) {
            ddl << "USING " << indexTypeToString(definition.type) << " ";
        }

        ddl << "(";
        for (size_t i = 0; i < definition.columns.size(); ++i) {
            const auto& column = definition.columns[i];
            if (!column.expression.empty()) {
                ddl << "(" << column.expression << ")";
            } else {
                ddl << column.columnName;
            }

            if (column.ascending) {
                ddl << " ASC";
            } else {
                ddl << " DESC";
            }

            if (!column.nullsOrder.empty()) {
                ddl << " NULLS " << column.nullsOrder;
            }

            if (i < definition.columns.size() - 1) {
                ddl << ", ";
            }
        }
        ddl << ")";

        if (!definition.whereClause.empty()) {
            ddl << " WHERE " << definition.whereClause;
        }

        if (!definition.tablespace.empty()) {
            ddl << " TABLESPACE " << definition.tablespace;
        }

        if (definition.fillFactor != 90) {
            ddl << " WITH (FILLFACTOR = " << definition.fillFactor << ")";
        }

        ddl << ";";

        return ddl.str();
    }

    std::vector<std::string> validateIndex(const IndexDefinition& definition, const std::string& connectionId) {
        std::vector<std::string> errors;

        // Validate index name
        if (definition.name.empty()) {
            errors.push_back("Index name is required");
        } else if (!std::regex_match(definition.name, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            errors.push_back("Index name contains invalid characters");
        }

        // Validate table name
        if (definition.tableName.empty()) {
            errors.push_back("Table name is required");
        }

        // Validate columns
        if (definition.columns.empty()) {
            errors.push_back("Index must have at least one column");
        } else {
            std::set<std::string> columnNames;
            for (const auto& column : definition.columns) {
                if (column.columnName.empty() && column.expression.empty()) {
                    errors.push_back("Column name or expression is required");
                } else if (!column.columnName.empty() && columnNames.count(column.columnName)) {
                    errors.push_back("Duplicate column: " + column.columnName);
                } else if (!column.columnName.empty()) {
                    columnNames.insert(column.columnName);
                }
            }
        }

        // Validate fill factor
        if (definition.fillFactor < 10 || definition.fillFactor > 100) {
            errors.push_back("Fill factor must be between 10 and 100");
        }

        // Validate WHERE clause syntax (basic validation)
        if (!definition.whereClause.empty()) {
            if (definition.whereClause.find(";") != std::string::npos) {
                errors.push_back("WHERE clause cannot contain semicolons");
            }
        }

        return errors;
    }

    bool isIndexNameAvailable(const std::string& name, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return false;
        }

        try {
            std::string query = R"(
                SELECT COUNT(*) as count
                FROM pg_indexes
                WHERE indexname = ')" + name + R"(';
            )";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success && !result.rows.empty()) {
                return result.rows[0][0].toInt() == 0;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to check index name availability:" << e.what();
        }

        return false;
    }

    std::vector<std::string> getAvailableIndexTypes(const std::string& connectionId) {
        // Return common PostgreSQL index types
        return {
            "BTREE", "HASH", "GIN", "GIST", "SPGIST", "BRIN"
        };
    }

    std::vector<std::string> getAvailableTablespaces(const std::string& connectionId) {
        if (!sqlExecutor_) {
            return {"pg_default"};
        }

        try {
            std::string query = "SELECT spcname FROM pg_tablespace ORDER BY spcname;";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(5000);

            QueryResult result = sqlExecutor_->executeQuery(query, context);

            if (result.success) {
                std::vector<std::string> tablespaces;
                for (const auto& row : result.rows) {
                    tablespaces.push_back(row[0].toString().toStdString());
                }
                return tablespaces;
            }

        } catch (const std::exception& e) {
            qWarning() << "Failed to get tablespaces:" << e.what();
        }

        return {"pg_default"};
    }

    std::vector<std::string> getAvailableOperatorClasses(const std::string& columnType, const std::string& connectionId) {
        // Return common operator classes for different data types
        if (columnType == "TEXT" || columnType.find("VARCHAR") != std::string::npos) {
            return {"text_pattern_ops", "varchar_pattern_ops", "bpchar_pattern_ops"};
        }
        return {"default"};
    }

    void collectIndexStatistics(const std::string& indexName, const std::string& connectionId) {
        if (!sqlExecutor_) {
            return;
        }

        try {
            std::string query = "ANALYZE " + indexName + ";";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(300000); // 5 minutes

            sqlExecutor_->executeQuery(query, context);

        } catch (const std::exception& e) {
            qWarning() << "Failed to collect index statistics:" << e.what();
        }
    }

    void collectAllIndexStatistics(const std::string& connectionId) {
        if (!sqlExecutor_) {
            return;
        }

        try {
            std::string query = "ANALYZE;";

            QueryExecutionContext context;
            context.connectionId = connectionId;
            context.timeout = std::chrono::milliseconds(1800000); // 30 minutes

            sqlExecutor_->executeQuery(query, context);

        } catch (const std::exception& e) {
            qWarning() << "Failed to collect all index statistics:" << e.what();
        }
    }

public:
    IndexManager* parent_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;

private:

    IndexDefinition parseIndexDefinitionFromSQL(const std::string& sql) {
        IndexDefinition definition;

        // Simple parsing - in a real implementation, this would use proper SQL parsing
        std::regex nameRegex(R"(CREATE\s+(?:UNIQUE\s+)?INDEX\s+(?:CONCURRENTLY\s+)?(\w+))", std::regex::icase);
        std::smatch nameMatch;
        if (std::regex_search(sql, nameMatch, nameRegex)) {
            definition.name = nameMatch[1].str();
        }

        if (sql.find("UNIQUE") != std::string::npos) {
            definition.isUnique = true;
        }

        if (sql.find("CONCURRENTLY") != std::string::npos) {
            definition.isConcurrent = true;
        }

        // Parse table name and columns (simplified)
        std::regex tableRegex(R"(ON\s+(\w+))", std::regex::icase);
        std::smatch tableMatch;
        if (std::regex_search(sql, tableMatch, tableRegex)) {
            definition.tableName = tableMatch[1].str();
        }

        return definition;
    }

    qint64 parseSizeString(const std::string& sizeStr) {
        // Simple size parsing - convert "123 MB" to bytes
        std::regex sizeRegex(R"((\d+(?:\.\d+)?)\s*(B|KB|MB|GB|TB))", std::regex::icase);
        std::smatch match;
        if (std::regex_search(sizeStr, match, sizeRegex)) {
            double size = std::stod(match[1].str());
            std::string unit = match[2].str();

            std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);

            if (unit == "KB") size *= 1024;
            else if (unit == "MB") size *= 1024 * 1024;
            else if (unit == "GB") size *= 1024 * 1024 * 1024;
            else if (unit == "TB") size *= 1024 * 1024 * 1024 * 1024;

            return static_cast<qint64>(size);
        }

        return 0;
    }
};

// IndexManager implementation

IndexManager::IndexManager(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {

    // Set up connections
    connect(this, &IndexManager::indexCreated, [this](const std::string& indexName, const std::string& connectionId) {
        if (indexCreatedCallback_) indexCreatedCallback_(indexName, connectionId);
    });

    connect(this, &IndexManager::indexDropped, [this](const std::string& indexName, const std::string& connectionId) {
        if (indexDroppedCallback_) indexDroppedCallback_(indexName, connectionId);
    });

    connect(this, &IndexManager::indexModified, [this](const std::string& indexName, const std::string& connectionId) {
        if (indexModifiedCallback_) indexModifiedCallback_(indexName, connectionId);
    });

    connect(this, &IndexManager::maintenanceCompleted, [this](const IndexMaintenanceOperation& op) {
        if (maintenanceCompletedCallback_) maintenanceCompletedCallback_(op);
    });
}

IndexManager::~IndexManager() = default;

void IndexManager::initialize() {
    impl_->initialize();
}

void IndexManager::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    impl_->metadataManager_ = metadataManager;
}

void IndexManager::setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) {
    impl_->sqlExecutor_ = sqlExecutor;
}

void IndexManager::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
    impl_->connectionManager_ = connectionManager;
}

std::vector<IndexDefinition> IndexManager::getTableIndexes(const std::string& tableName, const std::string& connectionId) {
    return impl_->getTableIndexes(tableName, connectionId);
}

IndexDefinition IndexManager::getIndexDefinition(const std::string& indexName, const std::string& connectionId) {
    return impl_->getIndexDefinition(indexName, connectionId);
}

std::vector<IndexStatistics> IndexManager::getIndexStatistics(const std::string& indexName, const std::string& connectionId) {
    return impl_->getIndexStatistics(indexName, connectionId);
}

std::vector<IndexPerformance> IndexManager::getIndexPerformance(const std::string& indexName, const std::string& connectionId) {
    return impl_->getIndexPerformance(indexName, connectionId);
}

bool IndexManager::createIndex(const IndexDefinition& definition, const std::string& connectionId) {
    return impl_->createIndex(definition, connectionId);
}

bool IndexManager::dropIndex(const std::string& indexName, const std::string& connectionId) {
    return impl_->dropIndex(indexName, connectionId);
}

bool IndexManager::rebuildIndex(const std::string& indexName, const std::string& connectionId) {
    return impl_->rebuildIndex(indexName, connectionId);
}

bool IndexManager::renameIndex(const std::string& oldName, const std::string& newName, const std::string& connectionId) {
    return impl_->renameIndex(oldName, newName, connectionId);
}

bool IndexManager::alterIndex(const std::string& indexName, const IndexDefinition& definition, const std::string& connectionId) {
    return impl_->alterIndex(indexName, definition, connectionId);
}

std::vector<IndexMaintenanceOperation> IndexManager::getMaintenanceHistory(const std::string& indexName, const std::string& connectionId) {
    return impl_->getMaintenanceHistory(indexName, connectionId);
}

IndexMaintenanceOperation IndexManager::performMaintenance(const std::string& indexName, IndexOperation operation, const std::string& connectionId) {
    return impl_->performMaintenance(indexName, operation, connectionId);
}

std::vector<IndexRecommendation> IndexManager::analyzeTableIndexes(const std::string& tableName, const std::string& connectionId) {
    return impl_->analyzeTableIndexes(tableName, connectionId);
}

std::vector<IndexRecommendation> IndexManager::analyzeQueryIndexes(const std::string& query, const std::string& connectionId) {
    return impl_->analyzeQueryIndexes(query, connectionId);
}

void IndexManager::implementRecommendation(const std::string& recommendationId, const std::string& connectionId) {
    impl_->implementRecommendation(recommendationId, connectionId);
}

std::string IndexManager::generateIndexDDL(const IndexDefinition& definition, const std::string& connectionId) {
    return impl_->generateIndexDDL(definition, connectionId);
}

std::vector<std::string> IndexManager::validateIndex(const IndexDefinition& definition, const std::string& connectionId) {
    return impl_->validateIndex(definition, connectionId);
}

bool IndexManager::isIndexNameAvailable(const std::string& name, const std::string& connectionId) {
    return impl_->isIndexNameAvailable(name, connectionId);
}

std::vector<std::string> IndexManager::getAvailableIndexTypes(const std::string& connectionId) {
    return impl_->getAvailableIndexTypes(connectionId);
}

std::vector<std::string> IndexManager::getAvailableTablespaces(const std::string& connectionId) {
    return impl_->getAvailableTablespaces(connectionId);
}

std::vector<std::string> IndexManager::getAvailableOperatorClasses(const std::string& columnType, const std::string& connectionId) {
    return impl_->getAvailableOperatorClasses(columnType, connectionId);
}

void IndexManager::collectIndexStatistics(const std::string& indexName, const std::string& connectionId) {
    impl_->collectIndexStatistics(indexName, connectionId);
}

void IndexManager::collectAllIndexStatistics(const std::string& connectionId) {
    impl_->collectAllIndexStatistics(connectionId);
}

void IndexManager::setIndexCreatedCallback(IndexCreatedCallback callback) {
    indexCreatedCallback_ = callback;
}

void IndexManager::setIndexDroppedCallback(IndexDroppedCallback callback) {
    indexDroppedCallback_ = callback;
}

void IndexManager::setIndexModifiedCallback(IndexModifiedCallback callback) {
    indexModifiedCallback_ = callback;
}

void IndexManager::setMaintenanceCompletedCallback(MaintenanceCompletedCallback callback) {
    maintenanceCompletedCallback_ = callback;
}

} // namespace scratchrobin
