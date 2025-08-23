#ifndef SCRATCHROBIN_QUERY_HISTORY_H
#define SCRATCHROBIN_QUERY_HISTORY_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <set>
#include <functional>
#include <mutex>
#include <atomic>
#include <QObject>
#include <QDateTime>
#include <QJsonObject>

namespace scratchrobin {

class ISQLExecutor;
class ITextEditor;

enum class QueryHistoryFilter {
    ALL,
    SUCCESSFUL,
    FAILED,
    BY_DATE_RANGE,
    BY_EXECUTION_TIME,
    BY_ROW_COUNT,
    BY_QUERY_TYPE,
    BY_CONNECTION,
    BY_USER,
    BY_TAG,
    BY_PERFORMANCE,
    CUSTOM
};

enum class QueryHistorySort {
    EXECUTION_TIME_DESC,
    EXECUTION_TIME_ASC,
    DATE_DESC,
    DATE_ASC,
    DURATION_DESC,
    DURATION_ASC,
    ROW_COUNT_DESC,
    ROW_COUNT_ASC,
    SUCCESS_RATE_DESC,
    SUCCESS_RATE_ASC,
    FREQUENCY_DESC,
    FREQUENCY_ASC
};

enum class QueryFavoriteType {
    QUERY,
    TEMPLATE,
    SNIPPET,
    MACRO,
    BOOKMARK
};

struct QueryHistoryEntry {
    std::string id;
    std::string queryId;
    std::string queryText;
    QueryType queryType;
    std::string connectionId;
    std::string databaseName;
    std::string userName;
    QDateTime startTime;
    QDateTime endTime;
    std::chrono::milliseconds duration{0};
    int rowCount = 0;
    int affectedRows = 0;
    bool success = false;
    std::string errorMessage;
    std::string executionPlan;
    std::unordered_map<std::string, QVariant> statistics;
    std::unordered_map<std::string, std::string> metadata;
    std::set<std::string> tags;
    bool isFavorite = false;
    int executionCount = 1;
    QDateTime createdAt;
    QDateTime lastExecutedAt;
};

struct QueryFavorite {
    std::string id;
    std::string name;
    std::string description;
    QueryFavoriteType type;
    std::string queryText;
    std::string category;
    std::set<std::string> tags;
    std::unordered_map<std::string, std::string> parameters;
    std::unordered_map<std::string, std::string> metadata;
    int usageCount = 0;
    QDateTime createdAt;
    QDateTime lastUsedAt;
    QDateTime expiresAt;
    bool isExpired() const { return expiresAt.isValid() && expiresAt < QDateTime::currentDateTime(); }
};

struct QueryTemplate {
    std::string id;
    std::string name;
    std::string description;
    std::string templateText;
    std::string category;
    std::vector<std::string> parameterNames;
    std::unordered_map<std::string, std::string> defaultValues;
    std::unordered_map<std::string, std::string> metadata;
    QDateTime createdAt;
    QDateTime lastUsedAt;
};

struct QueryHistoryFilterCriteria {
    QueryHistoryFilter filter = QueryHistoryFilter::ALL;
    QDateTime startDate;
    QDateTime endDate;
    std::chrono::milliseconds minDuration{0};
    std::chrono::milliseconds maxDuration{0};
    int minRowCount = -1;
    int maxRowCount = -1;
    QueryType queryType = QueryType::UNKNOWN;
    std::string connectionId;
    std::string userName;
    std::set<std::string> tags;
    std::string searchText;
    bool caseSensitive = false;
    bool regex = false;
};

struct QueryHistoryConfiguration {
    bool enableHistory = true;
    int maxHistorySize = 10000;
    int maxDaysToKeep = 365;
    bool autoCleanup = true;
    int cleanupIntervalDays = 7;
    bool enableFavorites = true;
    int maxFavorites = 1000;
    bool enableTemplates = true;
    bool enableTags = true;
    bool enableAnalytics = true;
    std::string databasePath = ":memory:";
    bool compressHistory = false;
    int maxQueryLength = 10000;
    bool logFailedQueries = true;
    bool logSuccessfulQueries = true;
};

class IQueryHistory {
public:
    virtual ~IQueryHistory() = default;

    virtual void initialize(const QueryHistoryConfiguration& config) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;
    virtual void setTextEditor(std::shared_ptr<ITextEditor> textEditor) = 0;

    virtual void addQuery(const QueryResult& queryResult) = 0;
    virtual void updateQuery(const std::string& queryId, const QueryResult& queryResult) = 0;
    virtual void removeQuery(const std::string& queryId) = 0;

    virtual std::vector<QueryHistoryEntry> getHistory(const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria(),
                                                     QueryHistorySort sort = QueryHistorySort::DATE_DESC,
                                                     int limit = 100, int offset = 0) = 0;

    virtual QueryHistoryEntry getQuery(const std::string& queryId) const = 0;
    virtual std::vector<QueryHistoryEntry> searchHistory(const std::string& searchText, bool caseSensitive = false,
                                                        bool regex = false, int limit = 100) = 0;

    virtual void addFavorite(const QueryHistoryEntry& entry, const std::string& name = "", const std::string& description = "") = 0;
    virtual void removeFavorite(const std::string& favoriteId) = 0;
    virtual void updateFavorite(const std::string& favoriteId, const QueryFavorite& favorite) = 0;
    virtual std::vector<QueryFavorite> getFavorites(const std::string& category = "", const std::set<std::string>& tags = {}) = 0;
    virtual QueryFavorite getFavorite(const std::string& favoriteId) const = 0;

    virtual void addTag(const std::string& queryId, const std::string& tag) = 0;
    virtual void removeTag(const std::string& queryId, const std::string& tag) = 0;
    virtual std::set<std::string> getTags() const = 0;
    virtual std::vector<QueryHistoryEntry> getQueriesByTag(const std::string& tag) = 0;

    virtual void createTemplate(const std::string& name, const std::string& description,
                              const std::string& templateText, const std::string& category = "") = 0;
    virtual void updateTemplate(const std::string& templateId, const QueryTemplate& template_) = 0;
    virtual void removeTemplate(const std::string& templateId) = 0;
    virtual std::vector<QueryTemplate> getTemplates(const std::string& category = "") = 0;
    virtual QueryTemplate getTemplate(const std::string& templateId) const = 0;
    virtual std::string instantiateTemplate(const std::string& templateId,
                                          const std::unordered_map<std::string, std::string>& parameters) = 0;

    virtual void clearHistory() = 0;
    virtual void cleanupHistory(int daysToKeep = 30) = 0;
    virtual void optimizeHistory() = 0;

    virtual std::unordered_map<std::string, int> getQueryStatistics(const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) = 0;
    virtual std::vector<std::pair<std::string, int>> getPopularQueries(int limit = 10) = 0;
    virtual std::vector<std::pair<std::string, int>> getPopularTags(int limit = 10) = 0;
    virtual std::vector<std::pair<std::string, double>> getQueryPerformanceTrends(int days = 30) = 0;

    virtual void exportHistory(const std::string& filePath, const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) = 0;
    virtual void importHistory(const std::string& filePath) = 0;

    virtual QueryHistoryConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const QueryHistoryConfiguration& config) = 0;

    using HistoryChangedCallback = std::function<void()>;
    using FavoritesChangedCallback = std::function<void()>;
    using TemplatesChangedCallback = std::function<void()>;

    virtual void setHistoryChangedCallback(HistoryChangedCallback callback) = 0;
    virtual void setFavoritesChangedCallback(FavoritesChangedCallback callback) = 0;
    virtual void setTemplatesChangedCallback(TemplatesChangedCallback callback) = 0;
};

class QueryHistory : public QObject, public IQueryHistory {
    Q_OBJECT

public:
    QueryHistory(QObject* parent = nullptr);
    ~QueryHistory() override;

    // IQueryHistory implementation
    void initialize(const QueryHistoryConfiguration& config) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;
    void setTextEditor(std::shared_ptr<ITextEditor> textEditor) override;

    void addQuery(const QueryResult& queryResult) override;
    void updateQuery(const std::string& queryId, const QueryResult& queryResult) override;
    void removeQuery(const std::string& queryId) override;

    std::vector<QueryHistoryEntry> getHistory(const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria(),
                                            QueryHistorySort sort = QueryHistorySort::DATE_DESC,
                                            int limit = 100, int offset = 0) override;

    QueryHistoryEntry getQuery(const std::string& queryId) const override;
    std::vector<QueryHistoryEntry> searchHistory(const std::string& searchText, bool caseSensitive = false,
                                               bool regex = false, int limit = 100) override;

    void addFavorite(const QueryHistoryEntry& entry, const std::string& name = "", const std::string& description = "") override;
    void removeFavorite(const std::string& favoriteId) override;
    void updateFavorite(const std::string& favoriteId, const QueryFavorite& favorite) override;
    std::vector<QueryFavorite> getFavorites(const std::string& category = "", const std::set<std::string>& tags = {}) override;
    QueryFavorite getFavorite(const std::string& favoriteId) const override;

    void addTag(const std::string& queryId, const std::string& tag) override;
    void removeTag(const std::string& queryId, const std::string& tag) override;
    std::set<std::string> getTags() const override;
    std::vector<QueryHistoryEntry> getQueriesByTag(const std::string& tag) override;

    void createTemplate(const std::string& name, const std::string& description,
                       const std::string& templateText, const std::string& category = "") override;
    void updateTemplate(const std::string& templateId, const QueryTemplate& template_) override;
    void removeTemplate(const std::string& templateId) override;
    std::vector<QueryTemplate> getTemplates(const std::string& category = "") override;
    QueryTemplate getTemplate(const std::string& templateId) const override;
    std::string instantiateTemplate(const std::string& templateId,
                                  const std::unordered_map<std::string, std::string>& parameters) override;

    void clearHistory() override;
    void cleanupHistory(int daysToKeep = 30) override;
    void optimizeHistory() override;

    std::unordered_map<std::string, int> getQueryStatistics(const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) override;
    std::vector<std::pair<std::string, int>> getPopularQueries(int limit = 10) override;
    std::vector<std::pair<std::string, int>> getPopularTags(int limit = 10) override;
    std::vector<std::pair<std::string, double>> getQueryPerformanceTrends(int days = 30) override;

    void exportHistory(const std::string& filePath, const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) override;
    void importHistory(const std::string& filePath) override;

    QueryHistoryConfiguration getConfiguration() const override;
    void updateConfiguration(const QueryHistoryConfiguration& config) override;

    void setHistoryChangedCallback(HistoryChangedCallback callback) override;
    void setFavoritesChangedCallback(FavoritesChangedCallback callback) override;
    void setTemplatesChangedCallback(TemplatesChangedCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<ITextEditor> textEditor_;

    // Configuration and state
    QueryHistoryConfiguration config_;
    QSqlDatabase database_;

    // In-memory caches
    std::deque<QueryHistoryEntry> recentHistory_;
    std::unordered_map<std::string, QueryHistoryEntry> historyCache_;
    std::unordered_map<std::string, QueryFavorite> favorites_;
    std::unordered_map<std::string, QueryTemplate> templates_;
    std::set<std::string> tags_;

    // Callbacks
    HistoryChangedCallback historyChangedCallback_;
    FavoritesChangedCallback favoritesChangedCallback_;
    TemplatesChangedCallback templatesChangedCallback_;

    // Internal methods
    void setupDatabase();
    void createTables();
    void upgradeDatabase();

    std::string generateId();
    QueryHistoryEntry resultToEntry(const QueryResult& result);
    QueryFavorite entryToFavorite(const QueryHistoryEntry& entry, const std::string& name, const std::string& description);

    void insertHistoryEntry(const QueryHistoryEntry& entry);
    void insertFavorite(const QueryFavorite& favorite);
    void insertTemplate(const QueryTemplate& template_);
    void insertTag(const std::string& queryId, const std::string& tag);

    std::vector<QueryHistoryEntry> queryHistoryFromDatabase(const QueryHistoryFilterCriteria& criteria,
                                                          QueryHistorySort sort, int limit, int offset);
    std::vector<QueryFavorite> queryFavoritesFromDatabase(const std::string& category, const std::set<std::string>& tags);
    std::vector<QueryTemplate> queryTemplatesFromDatabase(const std::string& category);

    bool matchesFilter(const QueryHistoryEntry& entry, const QueryHistoryFilterCriteria& criteria);
    void sortHistory(std::vector<QueryHistoryEntry>& history, QueryHistorySort sort);

    void cleanupExpiredEntries();
    void optimizeDatabase();

    void emitHistoryChanged();
    void emitFavoritesChanged();
    void emitTemplatesChanged();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_HISTORY_H
