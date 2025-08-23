#include "query/query_history.h"
#include "execution/sql_executor.h"
#include "editor/text_editor.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>

namespace scratchrobin {

// Helper functions
std::string generateQueryHistoryId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "qh_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}

std::string generateFavoriteId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "fav_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}

std::string generateTemplateId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "tmpl_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}

QDateTime chronoToQDateTime(const std::chrono::system_clock::time_point& tp) {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    return QDateTime::fromSecsSinceEpoch(time_t);
}

std::chrono::system_clock::time_point qDateTimeToChrono(const QDateTime& dt) {
    return std::chrono::system_clock::from_time_t(dt.toSecsSinceEpoch());
}

std::string queryTypeToString(QueryType type) {
    switch (type) {
        case QueryType::SELECT: return "SELECT";
        case QueryType::INSERT: return "INSERT";
        case QueryType::UPDATE: return "UPDATE";
        case QueryType::DELETE: return "DELETE";
        case QueryType::CREATE: return "CREATE";
        case QueryType::ALTER: return "ALTER";
        case QueryType::DROP: return "DROP";
        case QueryType::COMMIT: return "COMMIT";
        case QueryType::ROLLBACK: return "ROLLBACK";
        default: return "UNKNOWN";
    }
}

QueryType stringToQueryType(const std::string& str) {
    if (str == "SELECT") return QueryType::SELECT;
    if (str == "INSERT") return QueryType::INSERT;
    if (str == "UPDATE") return QueryType::UPDATE;
    if (str == "DELETE") return QueryType::DELETE;
    if (str == "CREATE") return QueryType::CREATE;
    if (str == "ALTER") return QueryType::ALTER;
    if (str == "DROP") return QueryType::DROP;
    if (str == "COMMIT") return QueryType::COMMIT;
    if (str == "ROLLBACK") return QueryType::ROLLBACK;
    return QueryType::UNKNOWN;
}

std::string favoriteTypeToString(QueryFavoriteType type) {
    switch (type) {
        case QueryFavoriteType::QUERY: return "QUERY";
        case QueryFavoriteType::TEMPLATE: return "TEMPLATE";
        case QueryFavoriteType::SNIPPET: return "SNIPPET";
        case QueryFavoriteType::MACRO: return "MACRO";
        case QueryFavoriteType::BOOKMARK: return "BOOKMARK";
        default: return "QUERY";
    }
}

QueryFavoriteType stringToFavoriteType(const std::string& str) {
    if (str == "TEMPLATE") return QueryFavoriteType::TEMPLATE;
    if (str == "SNIPPET") return QueryFavoriteType::SNIPPET;
    if (str == "MACRO") return QueryFavoriteType::MACRO;
    if (str == "BOOKMARK") return QueryFavoriteType::BOOKMARK;
    return QueryFavoriteType::QUERY;
}

class QueryHistory::Impl {
public:
    Impl(QueryHistory* parent) : parent_(parent) {}

    void setupDatabase() {
        if (config_.databasePath == ":memory:") {
            database_ = QSqlDatabase::addDatabase("QSQLITE", "query_history_memory");
            database_.setDatabaseName(":memory:");
        } else {
            QString path = QString::fromStdString(config_.databasePath);
            if (path.startsWith("~")) {
                path = QDir::homePath() + path.mid(1);
            }
            database_ = QSqlDatabase::addDatabase("QSQLITE", "query_history_file");
            database_.setDatabaseName(path);
        }

        if (!database_.open()) {
            throw std::runtime_error("Failed to open query history database: " +
                                   database_.lastError().text().toStdString());
        }

        createTables();
    }

    void createTables() {
        QSqlQuery query(database_);

        // Create history table
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS query_history (
                id TEXT PRIMARY KEY,
                query_id TEXT,
                query_text TEXT,
                query_type TEXT,
                connection_id TEXT,
                database_name TEXT,
                user_name TEXT,
                start_time TEXT,
                end_time TEXT,
                duration_ms INTEGER,
                row_count INTEGER,
                affected_rows INTEGER,
                success INTEGER,
                error_message TEXT,
                execution_plan TEXT,
                statistics TEXT,
                metadata TEXT,
                tags TEXT,
                is_favorite INTEGER,
                execution_count INTEGER,
                created_at TEXT,
                last_executed_at TEXT
            )
        )")) {
            throw std::runtime_error("Failed to create query_history table: " +
                                   query.lastError().text().toStdString());
        }

        // Create favorites table
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS query_favorites (
                id TEXT PRIMARY KEY,
                name TEXT,
                description TEXT,
                type TEXT,
                query_text TEXT,
                category TEXT,
                tags TEXT,
                parameters TEXT,
                metadata TEXT,
                usage_count INTEGER,
                created_at TEXT,
                last_used_at TEXT,
                expires_at TEXT
            )
        )")) {
            throw std::runtime_error("Failed to create query_favorites table: " +
                                   query.lastError().text().toStdString());
        }

        // Create templates table
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS query_templates (
                id TEXT PRIMARY KEY,
                name TEXT,
                description TEXT,
                template_text TEXT,
                category TEXT,
                parameter_names TEXT,
                default_values TEXT,
                metadata TEXT,
                created_at TEXT,
                last_used_at TEXT
            )
        )")) {
            throw std::runtime_error("Failed to create query_templates table: " +
                                   query.lastError().text().toStdString());
        }

        // Create tags table
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS query_tags (
                query_id TEXT,
                tag TEXT,
                PRIMARY KEY (query_id, tag)
            )
        )")) {
            throw std::runtime_error("Failed to create query_tags table: " +
                                   query.lastError().text().toStdString());
        }

        // Create indexes for better performance
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_query_id ON query_history(query_id)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_start_time ON query_history(start_time)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_history_success ON query_history(success)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_favorites_category ON query_favorites(category)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_templates_category ON query_templates(category)");
    }

    QueryHistoryEntry resultToEntry(const QueryResult& result) {
        QueryHistoryEntry entry;
        entry.id = generateQueryHistoryId();
        entry.sql = result.originalQuery;
        entry.timestamp = result.startTime;
        entry.duration = result.executionTime;
        entry.rowsAffected = result.affectedRows;
        entry.success = result.success;
        entry.errorMessage = result.errorMessage;
        entry.connectionId = result.connectionId;

        return entry;
    }

    void insertHistoryEntry(const QueryHistoryEntry& entry) {
        QSqlQuery query(database_);
        query.prepare(R"(
            INSERT OR REPLACE INTO query_history (
                id, query_id, query_text, query_type, connection_id, database_name, user_name,
                start_time, end_time, duration_ms, row_count, affected_rows, success, error_message,
                execution_plan, statistics, metadata, tags, is_favorite, execution_count,
                created_at, last_executed_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");

        query.addBindValue(QString::fromStdString(entry.id));
        query.addBindValue(QString::fromStdString(entry.sql));
        query.addBindValue(QString("UNKNOWN")); // QueryType not available in QueryHistoryEntry
        query.addBindValue(QString::fromStdString(entry.connectionId));
        query.addBindValue(QString("")); // databaseName not available
        query.addBindValue(QString("")); // userName not available
        query.addBindValue(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entry.timestamp.time_since_epoch()).count()).toString(Qt::ISODate));
        query.addBindValue(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entry.timestamp.time_since_epoch()).count()).toString(Qt::ISODate)); // No endTime, use timestamp
        query.addBindValue(static_cast<qint64>(entry.duration.count()));
        query.addBindValue(0); // rowCount not available in QueryHistoryEntry
        query.addBindValue(entry.rowsAffected);
        query.addBindValue(entry.success ? 1 : 0);
        query.addBindValue(QString::fromStdString(entry.errorMessage));
        query.addBindValue(QString("")); // executionPlan not available
        query.addBindValue(QString("")); // statistics not available
        query.addBindValue(QString("")); // metadata not available
        query.addBindValue(QString("[]")); // tags not available, use empty array
        query.addBindValue(0); // isFavorite not available
        query.addBindValue(1); // executionCount not available, use 1
        query.addBindValue(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entry.timestamp.time_since_epoch()).count()).toString(Qt::ISODate));
        query.addBindValue(QDateTime::fromSecsSinceEpoch(std::chrono::duration_cast<std::chrono::seconds>(entry.timestamp.time_since_epoch()).count()).toString(Qt::ISODate));

        if (!query.exec()) {
            qWarning() << "Failed to insert history entry:" << query.lastError().text();
        }
    }

    void insertFavorite(const QueryFavorite& favorite) {
        QSqlQuery query(database_);
        query.prepare(R"(
            INSERT OR REPLACE INTO query_favorites (
                id, name, description, type, query_text, category, tags, parameters, metadata,
                usage_count, created_at, last_used_at, expires_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");

        query.addBindValue(QString::fromStdString(favorite.id));
        query.addBindValue(QString::fromStdString(favorite.name));
        query.addBindValue(QString::fromStdString(favorite.description));
        query.addBindValue(QString::fromStdString(favoriteTypeToString(favorite.type)));
        query.addBindValue(QString::fromStdString(favorite.queryText));
        query.addBindValue(QString::fromStdString(favorite.category));

        // Convert tags to JSON array
        QJsonArray tagsArray;
        for (const auto& tag : favorite.tags) {
            tagsArray.append(QString::fromStdString(tag));
        }
        query.addBindValue(QString::fromUtf8(QJsonDocument(tagsArray).toJson()));

        // Convert parameters to JSON object
        QJsonObject paramsObj;
        for (const auto& [key, value] : favorite.parameters) {
            paramsObj[QString::fromStdString(key)] = QString::fromStdString(value);
        }
        query.addBindValue(QString::fromUtf8(QJsonDocument(paramsObj).toJson()));

        // Convert metadata to JSON object
        QJsonObject metaObj;
        for (const auto& [key, value] : favorite.metadata) {
            metaObj[QString::fromStdString(key)] = QString::fromStdString(value);
        }
        query.addBindValue(QString::fromUtf8(QJsonDocument(metaObj).toJson()));

        query.addBindValue(favorite.usageCount);
        query.addBindValue(favorite.createdAt.toString(Qt::ISODate));
        query.addBindValue(favorite.lastUsedAt.toString(Qt::ISODate));
        query.addBindValue(favorite.expiresAt.isValid() ? favorite.expiresAt.toString(Qt::ISODate) : QVariant());

        if (!query.exec()) {
            qWarning() << "Failed to insert favorite:" << query.lastError().text();
        }
    }

    std::vector<QueryHistoryEntry> queryHistoryFromDatabase(
        const QueryHistoryFilterCriteria& criteria, QueryHistorySort sort, int limit, int offset) {

        std::vector<QueryHistoryEntry> results;
        QStringList whereConditions;
        QVariantList bindValues;

        // Build WHERE clause
        if (criteria.filter != QueryHistoryFilter::ALL) {
            switch (criteria.filter) {
                case QueryHistoryFilter::SUCCESSFUL:
                    whereConditions << "success = 1";
                    break;
                case QueryHistoryFilter::FAILED:
                    whereConditions << "success = 0";
                    break;
                case QueryHistoryFilter::BY_DATE_RANGE:
                    if (criteria.startDate.isValid()) {
                        whereConditions << "start_time >= ?";
                        bindValues << criteria.startDate.toString(Qt::ISODate);
                    }
                    if (criteria.endDate.isValid()) {
                        whereConditions << "start_time <= ?";
                        bindValues << criteria.endDate.toString(Qt::ISODate);
                    }
                    break;
                case QueryHistoryFilter::BY_EXECUTION_TIME:
                    if (criteria.minDuration.count() > 0) {
                        whereConditions << "duration_ms >= ?";
                        bindValues << static_cast<qint64>(criteria.minDuration.count());
                    }
                    if (criteria.maxDuration.count() > 0) {
                        whereConditions << "duration_ms <= ?";
                        bindValues << static_cast<qint64>(criteria.maxDuration.count());
                    }
                    break;
                case QueryHistoryFilter::BY_ROW_COUNT:
                    if (criteria.minRowCount > 0) {
                        whereConditions << "row_count >= ?";
                        bindValues << criteria.minRowCount;
                    }
                    if (criteria.maxRowCount > 0) {
                        whereConditions << "row_count <= ?";
                        bindValues << criteria.maxRowCount;
                    }
                    break;
                case QueryHistoryFilter::BY_QUERY_TYPE:
                    if (criteria.queryType != QueryType::UNKNOWN) {
                        whereConditions << "query_type = ?";
                        bindValues << QString::fromStdString(queryTypeToString(criteria.queryType));
                    }
                    break;
                case QueryHistoryFilter::BY_CONNECTION:
                    if (!criteria.connectionId.empty()) {
                        whereConditions << "connection_id = ?";
                        bindValues << QString::fromStdString(criteria.connectionId);
                    }
                    break;
                case QueryHistoryFilter::BY_USER:
                    if (!criteria.userName.empty()) {
                        whereConditions << "user_name = ?";
                        bindValues << QString::fromStdString(criteria.userName);
                    }
                    break;
                default:
                    break;
            }
        }

        // Add search text filter
        if (!criteria.searchText.empty()) {
            if (criteria.regex) {
                whereConditions << "query_text REGEXP ?";
                bindValues << QString::fromStdString(criteria.searchText);
            } else {
                QString searchPattern = "%" + QString::fromStdString(criteria.searchText) + "%";
                if (criteria.caseSensitive) {
                    whereConditions << "query_text LIKE ?";
                } else {
                    whereConditions << "LOWER(query_text) LIKE LOWER(?)";
                }
                bindValues << searchPattern;
            }
        }

        // Build query
        QString queryStr = "SELECT * FROM query_history";
        if (!whereConditions.isEmpty()) {
            queryStr += " WHERE " + whereConditions.join(" AND ");
        }

        // Add sorting
        switch (sort) {
            case QueryHistorySort::EXECUTION_TIME_DESC:
                queryStr += " ORDER BY duration_ms DESC";
                break;
            case QueryHistorySort::EXECUTION_TIME_ASC:
                queryStr += " ORDER BY duration_ms ASC";
                break;
            case QueryHistorySort::DATE_DESC:
                queryStr += " ORDER BY start_time DESC";
                break;
            case QueryHistorySort::DATE_ASC:
                queryStr += " ORDER BY start_time ASC";
                break;
            case QueryHistorySort::DURATION_DESC:
                queryStr += " ORDER BY duration_ms DESC";
                break;
            case QueryHistorySort::DURATION_ASC:
                queryStr += " ORDER BY duration_ms ASC";
                break;
            case QueryHistorySort::ROW_COUNT_DESC:
                queryStr += " ORDER BY row_count DESC";
                break;
            case QueryHistorySort::ROW_COUNT_ASC:
                queryStr += " ORDER BY row_count ASC";
                break;
            default:
                queryStr += " ORDER BY start_time DESC";
                break;
        }

        // Add pagination
        queryStr += " LIMIT ? OFFSET ?";
        bindValues << limit << offset;

        QSqlQuery query(database_);
        query.prepare(queryStr);
        for (int i = 0; i < bindValues.size(); ++i) {
            query.bindValue(i, bindValues[i]);
        }

        if (!query.exec()) {
            qWarning() << "Failed to query history:" << query.lastError().text();
            return results;
        }

        while (query.next()) {
            QueryHistoryEntry entry;
            entry.id = query.value("id").toString().toStdString();
            entry.sql = query.value("query_text").toString().toStdString();
            entry.connectionId = query.value("connection_id").toString().toStdString();
            entry.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(query.value("start_time").toDateTime().toMSecsSinceEpoch()));
            entry.duration = std::chrono::milliseconds(query.value("duration_ms").toLongLong());
            entry.rowsAffected = query.value("affected_rows").toInt();
            entry.success = query.value("success").toBool();
            entry.errorMessage = query.value("error_message").toString().toStdString();

            // Note: rowCount, executionPlan, statistics, metadata, tags, isFavorite, executionCount, createdAt, and lastExecutedAt not available in QueryHistoryEntry

            results.push_back(entry);
        }

        return results;
    }

public:
    QueryHistory* parent_;
    QueryHistoryConfiguration config_;
    QSqlDatabase database_;
};

// QueryHistory implementation

QueryHistory::QueryHistory(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {

    // Set default configuration
    config_.enableHistory = true;
    config_.maxHistorySize = 10000;
    config_.maxDaysToKeep = 365;
    config_.autoCleanup = true;
    config_.cleanupIntervalDays = 7;
    config_.enableFavorites = true;
    config_.maxFavorites = 1000;
    config_.enableTemplates = true;
    config_.enableTags = true;
    config_.enableAnalytics = true;
    config_.databasePath = ":memory:";
    config_.compressHistory = false;
    config_.maxQueryLength = 10000;
    config_.logFailedQueries = true;
    config_.logSuccessfulQueries = true;
}

QueryHistory::~QueryHistory() = default;

void QueryHistory::initialize(const QueryHistoryConfiguration& config) {
    config_ = config;
    impl_->setupDatabase();
}

void QueryHistory::setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) {
    sqlExecutor_ = sqlExecutor;
}

void QueryHistory::setTextEditor(std::shared_ptr<ITextEditor> textEditor) {
    textEditor_ = textEditor;
}

void QueryHistory::addQuery(const QueryResult& queryResult) {
    if (!config_.enableHistory) {
        return;
    }

    if (!config_.logSuccessfulQueries && queryResult.success) {
        return;
    }

    if (!config_.logFailedQueries && !queryResult.success) {
        return;
    }

    try {
        auto entry = impl_->resultToEntry(queryResult);

        // Limit query text length
        if (entry.sql.length() > config_.maxQueryLength) {
            entry.sql = entry.sql.substr(0, config_.maxQueryLength) + "...";
        }

        impl_->insertHistoryEntry(entry);

        // Add to in-memory cache
        recentHistory_.push_front(entry);
        if (recentHistory_.size() > 100) { // Keep recent 100 in memory
            recentHistory_.pop_back();
        }
        historyCache_[entry.id] = entry;

        // Emit signal
        emit historyChangedCallback_();

    } catch (const std::exception& e) {
        qWarning() << "Failed to add query to history:" << e.what();
    }
}

void QueryHistory::updateQuery(const std::string& queryId, const QueryResult& queryResult) {
    // Implementation would update existing query
}

void QueryHistory::removeQuery(const std::string& queryId) {
    QSqlQuery query(impl_->database_);
    query.prepare("DELETE FROM query_history WHERE id = ?");
    query.addBindValue(QString::fromStdString(queryId));

    if (!query.exec()) {
        qWarning() << "Failed to remove query:" << query.lastError().text();
    }

    // Remove from cache
    historyCache_.erase(queryId);
    recentHistory_.erase(std::remove_if(recentHistory_.begin(), recentHistory_.end(),
                                       [&](const QueryHistoryEntry& e) { return e.id == queryId; }),
                        recentHistory_.end());

    emit historyChangedCallback_();
}

std::vector<QueryHistoryEntry> QueryHistory::getHistory(
    const QueryHistoryFilterCriteria& criteria, QueryHistorySort sort, int limit, int offset) {

    return impl_->queryHistoryFromDatabase(criteria, sort, limit, offset);
}

QueryHistoryEntry QueryHistory::getQuery(const std::string& queryId) const {
    auto it = historyCache_.find(queryId);
    if (it != historyCache_.end()) {
        return it->second;
    }

    QSqlQuery query(impl_->database_);
    query.prepare("SELECT * FROM query_history WHERE id = ?");
    query.addBindValue(QString::fromStdString(queryId));

    if (query.exec() && query.next()) {
        QueryHistoryEntry entry;
        entry.id = query.value("id").toString().toStdString();
        entry.sql = query.value("query_text").toString().toStdString();
        entry.connectionId = query.value("connection_id").toString().toStdString();
        entry.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(query.value("start_time").toDateTime().toMSecsSinceEpoch()));
        entry.duration = std::chrono::milliseconds(query.value("duration_ms").toLongLong());
        entry.rowsAffected = query.value("affected_rows").toInt();
        entry.success = query.value("success").toBool();
        entry.errorMessage = query.value("error_message").toString().toStdString();

        // Note: rowCount, executionPlan, statistics, metadata, tags, isFavorite, executionCount, createdAt, and lastExecutedAt not available in QueryHistoryEntry

        return entry;
    }

    return QueryHistoryEntry(); // Return empty entry if not found
}

std::vector<QueryHistoryEntry> QueryHistory::searchHistory(const std::string& searchText, bool caseSensitive, bool regex, int limit) {
    QueryHistoryFilterCriteria criteria;
    criteria.filter = QueryHistoryFilter::ALL;
    criteria.searchText = searchText;
    criteria.caseSensitive = caseSensitive;
    criteria.regex = regex;

    return getHistory(criteria, QueryHistorySort::DATE_DESC, limit, 0);
}

void QueryHistory::addFavorite(const QueryHistoryEntry& entry, const std::string& name, const std::string& description) {
    if (!config_.enableFavorites) {
        return;
    }

    QueryFavorite favorite;
    favorite.id = generateFavoriteId();
    favorite.name = name.empty() ? "Favorite Query " + std::to_string(favorites_.size() + 1) : name;
    favorite.description = description;
    favorite.type = QueryFavoriteType::QUERY;
    favorite.queryText = entry.sql;
    favorite.category = "history";
    favorite.createdAt = QDateTime::currentDateTime();
    favorite.lastUsedAt = QDateTime::currentDateTime();

    impl_->insertFavorite(favorite);
    favorites_[favorite.id] = favorite;

    emit favoritesChangedCallback_();
}

void QueryHistory::removeFavorite(const std::string& favoriteId) {
    QSqlQuery query(impl_->database_);
    query.prepare("DELETE FROM query_favorites WHERE id = ?");
    query.addBindValue(QString::fromStdString(favoriteId));

    if (!query.exec()) {
        qWarning() << "Failed to remove favorite:" << query.lastError().text();
    }

    favorites_.erase(favoriteId);
    emit favoritesChangedCallback_();
}

void QueryHistory::updateFavorite(const std::string& favoriteId, const QueryFavorite& favorite) {
    auto it = favorites_.find(favoriteId);
    if (it != favorites_.end()) {
        QueryFavorite updatedFavorite = favorite;
        updatedFavorite.id = favoriteId;
        impl_->insertFavorite(updatedFavorite);
        favorites_[favoriteId] = updatedFavorite;
        emit favoritesChangedCallback_();
    }
}

std::vector<QueryFavorite> QueryHistory::getFavorites(const std::string& category, const std::set<std::string>& tags) {
    QStringList whereConditions;
    QVariantList bindValues;

    if (!category.empty()) {
        whereConditions << "category = ?";
        bindValues << QString::fromStdString(category);
    }

    QString queryStr = "SELECT * FROM query_favorites";
    if (!whereConditions.isEmpty()) {
        queryStr += " WHERE " + whereConditions.join(" AND ");
    }
    queryStr += " ORDER BY last_used_at DESC";

    QSqlQuery query(impl_->database_);
    query.prepare(queryStr);
    for (int i = 0; i < bindValues.size(); ++i) {
        query.bindValue(i, bindValues[i]);
    }

    std::vector<QueryFavorite> results;
    if (query.exec()) {
        while (query.next()) {
            QueryFavorite favorite;
            favorite.id = query.value("id").toString().toStdString();
            favorite.name = query.value("name").toString().toStdString();
            favorite.description = query.value("description").toString().toStdString();
            favorite.type = stringToFavoriteType(query.value("type").toString().toStdString());
            favorite.queryText = query.value("query_text").toString().toStdString();
            favorite.category = query.value("category").toString().toStdString();

            // Parse tags JSON
            QString tagsJson = query.value("tags").toString();
            if (!tagsJson.isEmpty()) {
                QJsonDocument tagsDoc = QJsonDocument::fromJson(tagsJson.toUtf8());
                if (tagsDoc.isArray()) {
                    QJsonArray tagsArray = tagsDoc.array();
                    for (const QJsonValue& tagValue : tagsArray) {
                        favorite.tags.insert(tagValue.toString().toStdString());
                    }
                }
            }

            // Parse parameters JSON
            QString paramsJson = query.value("parameters").toString();
            if (!paramsJson.isEmpty()) {
                QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJson.toUtf8());
                if (paramsDoc.isObject()) {
                    QJsonObject paramsObj = paramsDoc.object();
                    for (const QString& key : paramsObj.keys()) {
                        favorite.parameters[key.toStdString()] = paramsObj[key].toString().toStdString();
                    }
                }
            }

            // Parse metadata JSON
            QString metaJson = query.value("metadata").toString();
            if (!metaJson.isEmpty()) {
                QJsonDocument metaDoc = QJsonDocument::fromJson(metaJson.toUtf8());
                if (metaDoc.isObject()) {
                    QJsonObject metaObj = metaDoc.object();
                    for (const QString& key : metaObj.keys()) {
                        favorite.metadata[key.toStdString()] = metaObj[key].toString().toStdString();
                    }
                }
            }

            favorite.usageCount = query.value("usage_count").toInt();
            favorite.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
            favorite.lastUsedAt = QDateTime::fromString(query.value("last_used_at").toString(), Qt::ISODate);
            QString expiresStr = query.value("expires_at").toString();
            if (!expiresStr.isEmpty()) {
                favorite.expiresAt = QDateTime::fromString(expiresStr, Qt::ISODate);
            }

            results.push_back(favorite);
        }
    }

    return results;
}

QueryFavorite QueryHistory::getFavorite(const std::string& favoriteId) const {
    auto it = favorites_.find(favoriteId);
    if (it != favorites_.end()) {
        return it->second;
    }

    return QueryFavorite(); // Return empty favorite if not found
}

void QueryHistory::addTag(const std::string& queryId, const std::string& tag) {
    if (!config_.enableTags) {
        return;
    }

    QSqlQuery query(impl_->database_);
    query.prepare("INSERT OR IGNORE INTO query_tags (query_id, tag) VALUES (?, ?)");
    query.addBindValue(QString::fromStdString(queryId));
    query.addBindValue(QString::fromStdString(tag));

    if (!query.exec()) {
        qWarning() << "Failed to add tag:" << query.lastError().text();
    }

    tags_.insert(tag);
    emit historyChangedCallback_();
}

void QueryHistory::removeTag(const std::string& queryId, const std::string& tag) {
    QSqlQuery query(impl_->database_);
    query.prepare("DELETE FROM query_tags WHERE query_id = ? AND tag = ?");
    query.addBindValue(QString::fromStdString(queryId));
    query.addBindValue(QString::fromStdString(tag));

    if (!query.exec()) {
        qWarning() << "Failed to remove tag:" << query.lastError().text();
    }

    emit historyChangedCallback_();
}

std::set<std::string> QueryHistory::getTags() const {
    return tags_;
}

std::vector<QueryHistoryEntry> QueryHistory::getQueriesByTag(const std::string& tag) {
    QSqlQuery query(impl_->database_);
    query.prepare(R"(
        SELECT h.* FROM query_history h
        INNER JOIN query_tags t ON h.id = t.query_id
        WHERE t.tag = ?
        ORDER BY h.start_time DESC
    )");
    query.addBindValue(QString::fromStdString(tag));

    std::vector<QueryHistoryEntry> results;
    if (query.exec()) {
        while (query.next()) {
            QueryHistoryEntry entry;
            // Parse query result into entry (similar to getQuery implementation)
            // ... (implementation would be similar to the getQuery method)
            results.push_back(entry);
        }
    }

    return results;
}

void QueryHistory::createTemplate(const std::string& name, const std::string& description,
                                 const std::string& templateText, const std::string& category) {
    if (!config_.enableTemplates) {
        return;
    }

    QueryTemplate template_;
    template_.id = generateTemplateId();
    template_.name = name;
    template_.description = description;
    template_.templateText = templateText;
    template_.category = category;
    template_.createdAt = QDateTime::currentDateTime();
    template_.lastUsedAt = QDateTime::currentDateTime();

    // Extract parameter names from template
    std::regex paramRegex("\\$\\{([^}]+)\\}");
    std::smatch match;
    std::string text = templateText;
    while (std::regex_search(text, match, paramRegex)) {
        template_.parameterNames.push_back(match[1].str());
        text = match.suffix();
    }

    QSqlQuery query(impl_->database_);
    query.prepare(R"(
        INSERT INTO query_templates (
            id, name, description, template_text, category, parameter_names,
            default_values, metadata, created_at, last_used_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.addBindValue(QString::fromStdString(template_.id));
    query.addBindValue(QString::fromStdString(template_.name));
    query.addBindValue(QString::fromStdString(template_.description));
    query.addBindValue(QString::fromStdString(template_.templateText));
    query.addBindValue(QString::fromStdString(template_.category));

    // Convert parameter names to JSON array
    QJsonArray paramsArray;
    for (const auto& param : template_.parameterNames) {
        paramsArray.append(QString::fromStdString(param));
    }
    query.addBindValue(QString::fromUtf8(QJsonDocument(paramsArray).toJson()));

    // Convert default values to JSON object
    QJsonObject defaultsObj;
    for (const auto& [key, value] : template_.defaultValues) {
        defaultsObj[QString::fromStdString(key)] = QString::fromStdString(value);
    }
    query.addBindValue(QString::fromUtf8(QJsonDocument(defaultsObj).toJson()));

    // Convert metadata to JSON object
    QJsonObject metaObj;
    for (const auto& [key, value] : template_.metadata) {
        metaObj[QString::fromStdString(key)] = QString::fromStdString(value);
    }
    query.addBindValue(QString::fromUtf8(QJsonDocument(metaObj).toJson()));

    query.addBindValue(template_.createdAt.toString(Qt::ISODate));
    query.addBindValue(template_.lastUsedAt.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to create template:" << query.lastError().text();
    }

    templates_[template_.id] = template_;
    emit templatesChangedCallback_();
}

void QueryHistory::updateTemplate(const std::string& templateId, const QueryTemplate& template_) {
    auto it = templates_.find(templateId);
    if (it != templates_.end()) {
        QueryTemplate updatedTemplate = template_;
        updatedTemplate.id = templateId;

        QSqlQuery query(impl_->database_);
        query.prepare(R"(
            UPDATE query_templates SET
                name = ?, description = ?, template_text = ?, category = ?,
                parameter_names = ?, default_values = ?, metadata = ?, last_used_at = ?
            WHERE id = ?
        )");

        // Similar binding as createTemplate...
        query.addBindValue(QString::fromStdString(updatedTemplate.id));

        if (!query.exec()) {
            qWarning() << "Failed to update template:" << query.lastError().text();
        }

        templates_[templateId] = updatedTemplate;
        emit templatesChangedCallback_();
    }
}

void QueryHistory::removeTemplate(const std::string& templateId) {
    QSqlQuery query(impl_->database_);
    query.prepare("DELETE FROM query_templates WHERE id = ?");
    query.addBindValue(QString::fromStdString(templateId));

    if (!query.exec()) {
        qWarning() << "Failed to remove template:" << query.lastError().text();
    }

    templates_.erase(templateId);
    emit templatesChangedCallback_();
}

std::vector<QueryTemplate> QueryHistory::getTemplates(const std::string& category) {
    QString queryStr = "SELECT * FROM query_templates";
    if (!category.empty()) {
        queryStr += " WHERE category = ?";
    }
    queryStr += " ORDER BY last_used_at DESC";

    QSqlQuery query(impl_->database_);
    query.prepare(queryStr);
    if (!category.empty()) {
        query.addBindValue(QString::fromStdString(category));
    }

    std::vector<QueryTemplate> results;
    if (query.exec()) {
        while (query.next()) {
            QueryTemplate template_;
            template_.id = query.value("id").toString().toStdString();
            template_.name = query.value("name").toString().toStdString();
            template_.description = query.value("description").toString().toStdString();
            template_.templateText = query.value("template_text").toString().toStdString();
            template_.category = query.value("category").toString().toStdString();

            // Parse parameter names JSON
            QString paramsJson = query.value("parameter_names").toString();
            if (!paramsJson.isEmpty()) {
                QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJson.toUtf8());
                if (paramsDoc.isArray()) {
                    QJsonArray paramsArray = paramsDoc.array();
                    for (const QJsonValue& paramValue : paramsArray) {
                        template_.parameterNames.push_back(paramValue.toString().toStdString());
                    }
                }
            }

            // Parse default values JSON
            QString defaultsJson = query.value("default_values").toString();
            if (!defaultsJson.isEmpty()) {
                QJsonDocument defaultsDoc = QJsonDocument::fromJson(defaultsJson.toUtf8());
                if (defaultsDoc.isObject()) {
                    QJsonObject defaultsObj = defaultsDoc.object();
                    for (const QString& key : defaultsObj.keys()) {
                        template_.defaultValues[key.toStdString()] = defaultsObj[key].toString().toStdString();
                    }
                }
            }

            // Parse metadata JSON
            QString metaJson = query.value("metadata").toString();
            if (!metaJson.isEmpty()) {
                QJsonDocument metaDoc = QJsonDocument::fromJson(metaJson.toUtf8());
                if (metaDoc.isObject()) {
                    QJsonObject metaObj = metaDoc.object();
                    for (const QString& key : metaObj.keys()) {
                        template_.metadata[key.toStdString()] = metaObj[key].toString().toStdString();
                    }
                }
            }

            template_.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
            template_.lastUsedAt = QDateTime::fromString(query.value("last_used_at").toString(), Qt::ISODate);

            results.push_back(template_);
        }
    }

    return results;
}

QueryTemplate QueryHistory::getTemplate(const std::string& templateId) const {
    auto it = templates_.find(templateId);
    if (it != templates_.end()) {
        return it->second;
    }

    return QueryTemplate(); // Return empty template if not found
}

std::string QueryHistory::instantiateTemplate(const std::string& templateId,
                                            const std::unordered_map<std::string, std::string>& parameters) {
    auto it = templates_.find(templateId);
    if (it == templates_.end()) {
        return "";
    }

    std::string result = it->second.templateText;

    // Replace parameters in template
    for (const auto& [param, value] : parameters) {
        std::string placeholder = "${" + param + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    // Replace any remaining parameters with default values
    for (const auto& [param, defaultValue] : it->second.defaultValues) {
        if (parameters.find(param) == parameters.end()) {
            std::string placeholder = "${" + param + "}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), defaultValue);
                pos += defaultValue.length();
            }
        }
    }

    return result;
}

void QueryHistory::clearHistory() {
    QSqlQuery query(impl_->database_);
    if (!query.exec("DELETE FROM query_history")) {
        qWarning() << "Failed to clear history:" << query.lastError().text();
    }

    recentHistory_.clear();
    historyCache_.clear();
    emit historyChangedCallback_();
}

void QueryHistory::cleanupHistory(int daysToKeep) {
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-daysToKeep);

    QSqlQuery query(impl_->database_);
    query.prepare("DELETE FROM query_history WHERE start_time < ?");
    query.addBindValue(cutoffDate.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to cleanup history:" << query.lastError().text();
    }

    // Clean up caches
    recentHistory_.erase(std::remove_if(recentHistory_.begin(), recentHistory_.end(),
                                       [cutoffDate](const QueryHistoryEntry& e) {
                                           return std::chrono::system_clock::time_point(std::chrono::milliseconds(cutoffDate.toMSecsSinceEpoch())) > e.timestamp;
                                       }), recentHistory_.end());

    for (auto it = historyCache_.begin(); it != historyCache_.end();) {
        if (std::chrono::system_clock::time_point(std::chrono::milliseconds(cutoffDate.toMSecsSinceEpoch())) > it->second.timestamp) {
            it = historyCache_.erase(it);
        } else {
            ++it;
        }
    }

    emit historyChangedCallback_();
}

void QueryHistory::optimizeHistory() {
    QSqlQuery query(impl_->database_);
    if (!query.exec("VACUUM")) {
        qWarning() << "Failed to optimize history database:" << query.lastError().text();
    }
}

std::unordered_map<std::string, int> QueryHistory::getQueryStatistics(const QueryHistoryFilterCriteria& criteria) {
    std::unordered_map<std::string, int> stats;

    auto history = getHistory(criteria, QueryHistorySort::DATE_DESC, 10000, 0);

    stats["totalQueries"] = history.size();
    stats["successfulQueries"] = std::count_if(history.begin(), history.end(),
                                             [](const QueryHistoryEntry& e) { return e.success; });
    stats["failedQueries"] = stats["totalQueries"] - stats["successfulQueries"];
    // Note: rowCount not available in QueryHistoryEntry, using rowsAffected as approximation
    stats["totalRows"] = std::accumulate(history.begin(), history.end(), 0,
                                       [](int sum, const QueryHistoryEntry& e) { return sum + e.rowsAffected; });
    stats["totalExecutionTime"] = std::accumulate(history.begin(), history.end(), 0LL,
                                                [](long long sum, const QueryHistoryEntry& e) {
                                                    return sum + e.duration.count();
                                                });

    return stats;
}

std::vector<std::pair<std::string, int>> QueryHistory::getPopularQueries(int limit) {
    QSqlQuery query(impl_->database_);
    query.prepare(R"(
        SELECT query_text, COUNT(*) as count
        FROM query_history
        WHERE success = 1
        GROUP BY query_text
        ORDER BY count DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    std::vector<std::pair<std::string, int>> results;
    if (query.exec()) {
        while (query.next()) {
            results.emplace_back(query.value("query_text").toString().toStdString(),
                               query.value("count").toInt());
        }
    }

    return results;
}

std::vector<std::pair<std::string, int>> QueryHistory::getPopularTags(int limit) {
    QSqlQuery query(impl_->database_);
    query.prepare(R"(
        SELECT tag, COUNT(*) as count
        FROM query_tags
        GROUP BY tag
        ORDER BY count DESC
        LIMIT ?
    )");
    query.addBindValue(limit);

    std::vector<std::pair<std::string, int>> results;
    if (query.exec()) {
        while (query.next()) {
            results.emplace_back(query.value("tag").toString().toStdString(),
                               query.value("count").toInt());
        }
    }

    return results;
}

std::vector<std::pair<std::string, double>> QueryHistory::getQueryPerformanceTrends(int days) {
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-days);

    QSqlQuery query(impl_->database_);
    query.prepare(R"(
        SELECT DATE(start_time) as date, AVG(duration_ms) as avg_duration
        FROM query_history
        WHERE start_time >= ? AND success = 1
        GROUP BY DATE(start_time)
        ORDER BY date
    )");
    query.addBindValue(cutoffDate.toString(Qt::ISODate));

    std::vector<std::pair<std::string, double>> results;
    if (query.exec()) {
        while (query.next()) {
            results.emplace_back(query.value("date").toString().toStdString(),
                               query.value("avg_duration").toDouble());
        }
    }

    return results;
}

void QueryHistory::exportHistory(const std::string& filePath, const QueryHistoryFilterCriteria& criteria) {
    // Implementation for export functionality
    // This would use the QueryExport system
}

void QueryHistory::importHistory(const std::string& filePath) {
    // Implementation for import functionality
    // This would use the QueryExport system
}

QueryHistoryConfiguration QueryHistory::getConfiguration() const {
    return config_;
}

void QueryHistory::updateConfiguration(const QueryHistoryConfiguration& config) {
    config_ = config;
}

void QueryHistory::setHistoryChangedCallback(HistoryChangedCallback callback) {
    historyChangedCallback_ = callback;
}

void QueryHistory::setFavoritesChangedCallback(FavoritesChangedCallback callback) {
    favoritesChangedCallback_ = callback;
}

void QueryHistory::setTemplatesChangedCallback(TemplatesChangedCallback callback) {
    templatesChangedCallback_ = callback;
}

} // namespace scratchrobin
