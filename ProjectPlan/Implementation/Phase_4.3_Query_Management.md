# Phase 4.3: Query Management Implementation

## Overview
This phase implements comprehensive query management functionality for ScratchRobin, providing advanced query history management, templates, favorites, export/import capabilities, and seamless integration with the editor and execution systems.

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
- Query execution system from Phase 4.2
- Text editor from Phase 4.1

## Implementation Tasks

### Task 4.3.1: Query History and Favorites

#### 4.3.1.1: Query History Manager
**Objective**: Create a comprehensive query history management system with advanced search, filtering, and organization capabilities

**Implementation Steps:**
1. Implement query history storage with persistent database backend
2. Add advanced search and filtering capabilities for history
3. Create query favorites and bookmarking system
4. Implement query tagging and categorization
5. Add query performance analytics and trends
6. Create query comparison and diff functionality
7. Implement query history cleanup and maintenance

**Code Changes:**

**File: src/query/query_history.h**
```cpp
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
#include <QSqlDatabase>

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
```

#### 4.3.1.2: Query Export/Import System
**Objective**: Implement comprehensive query export and import functionality with multiple formats and advanced options

**Implementation Steps:**
1. Create export system with multiple format support (SQL, JSON, XML, CSV, HTML)
2. Add import functionality with validation and conflict resolution
3. Implement query backup and restore capabilities
4. Create query sharing and collaboration features
5. Add query versioning and change tracking
6. Implement query comparison and diff functionality

**Code Changes:**

**File: src/query/query_export.h**
```cpp
#ifndef SCRATCHROBIN_QUERY_EXPORT_H
#define SCRATCHROBIN_QUERY_EXPORT_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <QObject>
#include <QJsonObject>
#include <QXmlStreamWriter>
#include <QTextStream>

namespace scratchrobin {

class IQueryHistory;
class ISQLExecutor;

enum class ExportFormat {
    SQL,
    JSON,
    XML,
    CSV,
    HTML,
    MARKDOWN,
    PDF,
    EXCEL,
    TEXT
};

enum class ExportScope {
    SELECTED_QUERIES,
    FILTERED_HISTORY,
    ALL_HISTORY,
    FAVORITES,
    TEMPLATES,
    TAGS,
    CATEGORY
};

enum class ImportMode {
    MERGE,
    REPLACE,
    SKIP_DUPLICATES,
    UPDATE_EXISTING
};

struct ExportOptions {
    ExportFormat format = ExportFormat::SQL;
    ExportScope scope = ExportScope::SELECTED_QUERIES;
    std::vector<std::string> queryIds;
    QueryHistoryFilterCriteria filter;
    std::string filePath;
    std::string title = "Query Export";
    std::string description;
    bool includeMetadata = true;
    bool includeStatistics = true;
    bool includeExecutionPlans = true;
    bool includeTags = true;
    bool includeComments = true;
    bool compressOutput = false;
    std::string encoding = "UTF-8";
    std::string dateFormat = "yyyy-MM-dd HH:mm:ss";
    bool prettyPrint = true;
    int maxRowsPerFile = -1; // -1 means no limit
    bool splitByDate = false;
    bool includeSchemaInfo = false;
    std::unordered_map<std::string, std::string> customOptions;
};

struct ImportOptions {
    std::string filePath;
    ImportMode mode = ImportMode::MERGE;
    bool validateQueries = true;
    bool preserveTimestamps = true;
    bool importFavorites = true;
    bool importTemplates = true;
    bool importTags = true;
    bool skipInvalidQueries = true;
    std::set<std::string> excludeCategories;
    std::unordered_map<std::string, std::string> categoryMapping;
    bool backupBeforeImport = true;
    std::string backupPath;
};

struct ExportResult {
    bool success = false;
    std::string filePath;
    int exportedQueries = 0;
    int exportedFavorites = 0;
    int exportedTemplates = 0;
    std::size_t fileSize = 0;
    std::chrono::milliseconds exportTime{0};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::unordered_map<std::string, int> exportStatistics;
};

struct ImportResult {
    bool success = false;
    int importedQueries = 0;
    int importedFavorites = 0;
    int importedTemplates = 0;
    int skippedQueries = 0;
    int failedQueries = 0;
    int updatedQueries = 0;
    std::chrono::milliseconds importTime{0};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::unordered_map<std::string, int> importStatistics;
};

class IQueryExport {
public:
    virtual ~IQueryExport() = default;

    virtual void setQueryHistory(std::shared_ptr<IQueryHistory> queryHistory) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;

    virtual ExportResult exportQueries(const ExportOptions& options) = 0;
    virtual void exportQueriesAsync(const ExportOptions& options) = 0;

    virtual ImportResult importQueries(const ImportOptions& options) = 0;
    virtual void importQueriesAsync(const ImportOptions& options) = 0;

    virtual std::vector<ExportFormat> getSupportedFormats() const = 0;
    virtual std::vector<std::string> getFormatExtensions(ExportFormat format) const = 0;
    virtual ExportFormat detectFormatFromFile(const std::string& filePath) const = 0;

    virtual void backupQueries(const std::string& backupPath, const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) = 0;
    virtual void restoreQueries(const std::string& backupPath, ImportMode mode = ImportMode::MERGE) = 0;

    virtual std::string generatePreview(const ExportOptions& options, int maxPreviewLength = 1000) = 0;
    virtual std::unordered_map<std::string, std::string> validateExportOptions(const ExportOptions& options) = 0;
    virtual std::unordered_map<std::string, std::string> validateImportOptions(const ImportOptions& options) = 0;

    using ExportProgressCallback = std::function<void(int, int, const std::string&)>;
    using ImportProgressCallback = std::function<void(int, int, const std::string&)>;
    using ExportCompletedCallback = std::function<void(const ExportResult&)>;
    using ImportCompletedCallback = std::function<void(const ImportResult&)>;

    virtual void setExportProgressCallback(ExportProgressCallback callback) = 0;
    virtual void setImportProgressCallback(ImportProgressCallback callback) = 0;
    virtual void setExportCompletedCallback(ExportCompletedCallback callback) = 0;
    virtual void setImportCompletedCallback(ImportCompletedCallback callback) = 0;
};

class QueryExport : public QObject, public IQueryExport {
    Q_OBJECT

public:
    QueryExport(QObject* parent = nullptr);
    ~QueryExport() override;

    // IQueryExport implementation
    void setQueryHistory(std::shared_ptr<IQueryHistory> queryHistory) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;

    ExportResult exportQueries(const ExportOptions& options) override;
    void exportQueriesAsync(const ExportOptions& options) override;

    ImportResult importQueries(const ImportOptions& options) override;
    void importQueriesAsync(const ImportOptions& options) override;

    std::vector<ExportFormat> getSupportedFormats() const override;
    std::vector<std::string> getFormatExtensions(ExportFormat format) const override;
    ExportFormat detectFormatFromFile(const std::string& filePath) const override;

    void backupQueries(const std::string& backupPath, const QueryHistoryFilterCriteria& criteria = QueryHistoryFilterCriteria()) override;
    void restoreQueries(const std::string& backupPath, ImportMode mode = ImportMode::MERGE) override;

    std::string generatePreview(const ExportOptions& options, int maxPreviewLength = 1000) override;
    std::unordered_map<std::string, std::string> validateExportOptions(const ExportOptions& options) override;
    std::unordered_map<std::string, std::string> validateImportOptions(const ImportOptions& options) override;

    void setExportProgressCallback(ExportProgressCallback callback) override;
    void setImportProgressCallback(ImportProgressCallback callback) override;
    void setExportCompletedCallback(ExportCompletedCallback callback) override;
    void setImportCompletedCallback(ImportCompletedCallback callback) override;

private slots:
    void onExportCompleted();
    void onImportCompleted();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IQueryHistory> queryHistory_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;

    // Callbacks
    ExportProgressCallback exportProgressCallback_;
    ImportProgressCallback importProgressCallback_;
    ExportCompletedCallback exportCompletedCallback_;
    ImportCompletedCallback importCompletedCallback_;

    // Internal methods
    ExportResult performExport(const ExportOptions& options);
    ImportResult performImport(const ImportOptions& options);

    void exportToSQL(const std::vector<QueryHistoryEntry>& queries,
                    const std::vector<QueryFavorite>& favorites,
                    const std::vector<QueryTemplate>& templates,
                    const ExportOptions& options, ExportResult& result);

    void exportToJSON(const std::vector<QueryHistoryEntry>& queries,
                     const std::vector<QueryFavorite>& favorites,
                     const std::vector<QueryTemplate>& templates,
                     const ExportOptions& options, ExportResult& result);

    void exportToXML(const std::vector<QueryHistoryEntry>& queries,
                    const std::vector<QueryFavorite>& favorites,
                    const std::vector<QueryTemplate>& templates,
                    const ExportOptions& options, ExportResult& result);

    void exportToCSV(const std::vector<QueryHistoryEntry>& queries,
                    const std::vector<QueryFavorite>& favorites,
                    const std::vector<QueryTemplate>& templates,
                    const ExportOptions& options, ExportResult& result);

    void exportToHTML(const std::vector<QueryHistoryEntry>& queries,
                     const std::vector<QueryFavorite>& favorites,
                     const std::vector<QueryTemplate>& templates,
                     const ExportOptions& options, ExportResult& result);

    void exportToMarkdown(const std::vector<QueryHistoryEntry>& queries,
                         const std::vector<QueryFavorite>& favorites,
                         const std::vector<QueryTemplate>& templates,
                         const ExportOptions& options, ExportResult& result);

    ImportResult importFromSQL(const ImportOptions& options);
    ImportResult importFromJSON(const ImportOptions& options);
    ImportResult importFromXML(const ImportOptions& options);
    ImportResult importFromCSV(const ImportOptions& options);

    void writeSQLHeader(QTextStream& stream, const ExportOptions& options);
    void writeSQLQuery(QTextStream& stream, const QueryHistoryEntry& query, const ExportOptions& options);
    void writeSQLFavorite(QTextStream& stream, const QueryFavorite& favorite, const ExportOptions& options);
    void writeSQLTemplate(QTextStream& stream, const QueryTemplate& template_, const ExportOptions& options);

    void writeJSONHeader(QJsonObject& root, const ExportOptions& options);
    void writeJSONQuery(QJsonObject& queryObj, const QueryHistoryEntry& query, const ExportOptions& options);
    void writeJSONFavorite(QJsonObject& favoriteObj, const QueryFavorite& favorite, const ExportOptions& options);
    void writeJSONTemplate(QJsonObject& templateObj, const QueryTemplate& template_, const ExportOptions& options);

    void writeXMLHeader(QXmlStreamWriter& writer, const ExportOptions& options);
    void writeXMLQuery(QXmlStreamWriter& writer, const QueryHistoryEntry& query, const ExportOptions& options);
    void writeXMLFavorite(QXmlStreamWriter& writer, const QueryFavorite& favorite, const ExportOptions& options);
    void writeXMLTemplate(QXmlStreamWriter& writer, const QueryTemplate& template_, const ExportOptions& options);

    void writeCSVHeader(QTextStream& stream, const ExportOptions& options);
    void writeCSVQuery(QTextStream& stream, const QueryHistoryEntry& query, const ExportOptions& options);
    void writeCSVFavorite(QTextStream& stream, const QueryFavorite& favorite, const ExportOptions& options);
    void writeCSVTemplate(QTextStream& stream, const QueryTemplate& template_, const ExportOptions& options);

    std::string escapeSQL(const std::string& text);
    std::string escapeJSON(const std::string& text);
    std::string escapeXML(const std::string& text);
    std::string escapeCSV(const std::string& text, const std::string& delimiter = ",");

    std::string formatDateTime(const QDateTime& dateTime, const std::string& format);
    std::string formatDuration(std::chrono::milliseconds duration);

    bool validateQuerySyntax(const std::string& query);
    void resolveImportConflicts(const std::vector<QueryHistoryEntry>& importedQueries,
                              const ImportOptions& options, ImportResult& result);

    void updateExportProgress(int current, int total, const std::string& message);
    void updateImportProgress(int current, int total, const std::string& message);

    void emitExportProgress(int current, int total, const std::string& message);
    void emitImportProgress(int current, int total, const std::string& message);
    void emitExportCompleted(const ExportResult& result);
    void emitImportCompleted(const ImportResult& result);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_EXPORT_H
```

## Summary

This phase 4.3 implementation provides comprehensive query management functionality for ScratchRobin with advanced query history management, templates, favorites, export/import capabilities, and seamless integration with the editor and execution systems.

✅ **Query History Management**: Comprehensive query history with advanced search, filtering, and organization
✅ **Query Favorites & Templates**: Bookmarking system and code template management
✅ **Query Export/Import**: Multi-format export/import with validation and conflict resolution
✅ **Query Analytics**: Performance trends, popular queries, and usage statistics
✅ **Query Tagging**: Advanced categorization and organization system
✅ **Query Backup/Restore**: Complete backup and restore functionality
✅ **Query Comparison**: Diff functionality for query analysis and optimization
✅ **Enterprise Features**: Query versioning, sharing, and collaboration support

The query management system provides enterprise-grade functionality with comprehensive query organization, sharing capabilities, and advanced analytics comparable to commercial database management tools.

**Next Phase**: Phase 5.1 - Table Management Implementation
