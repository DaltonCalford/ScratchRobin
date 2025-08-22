# Phase 3.4: Search and Indexing Implementation

## Overview
This phase implements comprehensive search and indexing functionality for ScratchRobin, providing full-text search, intelligent indexing strategies, query optimization, and advanced search capabilities across all database objects and metadata with seamless integration with the existing systems.

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
- Metadata loading system from Phase 3.1
- Object browser UI from Phase 3.2
- Property viewers from Phase 3.3

## Implementation Tasks

### Task 3.4.1: Search Engine Core

#### 3.4.1.1: Full-Text Search Engine
**Objective**: Create a comprehensive full-text search engine with multiple algorithms and indexing strategies

**Implementation Steps:**
1. Implement search engine with multiple algorithms (exact, fuzzy, regex, wildcard)
2. Create search indexer with background indexing and incremental updates
3. Add search result ranking and relevance scoring
4. Implement search history and suggestions
5. Create search analytics and performance monitoring

**Code Changes:**

**File: src/search/search_engine.h**
```cpp
#ifndef SCRATCHROBIN_SEARCH_ENGINE_H
#define SCRATCHROBIN_SEARCH_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <algorithm>
#include <functional>
#include <mutex>
#include <atomic>
#include <QObject>
#include <QTimer>
#include <QThread>

namespace scratchrobin {

class IMetadataManager;
class ITreeModel;
class ICacheManager;

enum class SearchAlgorithm {
    EXACT_MATCH,
    PREFIX_MATCH,
    SUBSTRING_MATCH,
    FUZZY_MATCH,
    REGEX_MATCH,
    WILDCARD_MATCH,
    PHONETIC_MATCH,
    SEMANTIC_MATCH
};

enum class SearchScope {
    ALL,
    CURRENT_CONNECTION,
    CURRENT_SCHEMA,
    CURRENT_DATABASE,
    SELECTED_OBJECTS,
    RECENT_OBJECTS,
    FAVORITE_OBJECTS
};

enum class SearchField {
    NAME,
    TYPE,
    SCHEMA,
    DATABASE,
    OWNER,
    PROPERTIES,
    COMMENTS,
    DEFINITION,
    DATA,
    ALL
};

enum class IndexType {
    INVERTED_INDEX,
    TRIE_INDEX,
    HASH_INDEX,
    SUFFIX_ARRAY,
    FULL_TEXT_INDEX,
    VECTOR_INDEX
};

struct SearchQuery {
    std::string pattern;
    SearchAlgorithm algorithm = SearchAlgorithm::SUBSTRING_MATCH;
    SearchScope scope = SearchScope::ALL;
    std::vector<SearchField> fields = {SearchField::ALL};
    bool caseSensitive = false;
    int maxResults = 1000;
    int maxDistance = 2; // For fuzzy search
    std::chrono::milliseconds timeout{5000};
    double minScore = 0.0;
    bool includeSynonyms = false;
    std::vector<std::string> languageFilters;
};

struct SearchResult {
    std::string objectId;
    std::string objectName;
    SchemaObjectType objectType;
    std::string schema;
    std::string database;
    std::string connectionId;
    SearchField matchedField;
    std::string matchedText;
    std::string contextSnippet;
    double relevanceScore = 0.0;
    int matchPosition = 0;
    std::vector<std::string> highlightedMatches;
    std::chrono::system_clock::time_point foundAt;
    std::unordered_map<std::string, std::string> metadata;
};

struct SearchIndex {
    IndexType type;
    std::string name;
    std::unordered_map<std::string, std::vector<std::string>> termIndex;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> tfidfIndex;
    std::unordered_map<std::string, int> documentFrequency;
    int totalDocuments = 0;
    std::chrono::system_clock::time_point builtAt;
    std::chrono::system_clock::time_point lastUpdated;
    int totalTerms = 0;
    int indexSize = 0;
    std::string checksum;
    std::string version;
};

struct SearchSuggestion {
    std::string text;
    std::string type;
    double confidence = 0.0;
    int frequency = 0;
    std::chrono::system_clock::time_point lastUsed;
};

struct SearchConfiguration {
    bool enableIndexing = true;
    bool enableBackgroundIndexing = true;
    int indexUpdateIntervalSeconds = 300;
    int maxIndexSize = 1000000;
    bool enableSuggestions = true;
    int maxSuggestions = 10;
    bool enableHistory = true;
    int maxHistorySize = 100;
    bool enableAnalytics = true;
    std::vector<SearchAlgorithm> enabledAlgorithms;
    std::vector<IndexType> enabledIndexTypes;
    std::unordered_map<SearchAlgorithm, int> algorithmWeights;
    std::unordered_map<IndexType, int> indexWeights;
    bool enableStemming = false;
    bool enableStopWords = true;
    std::vector<std::string> stopWords;
    std::vector<std::string> synonymGroups;
};

class ISearchEngine {
public:
    virtual ~ISearchEngine() = default;

    virtual void initialize(const SearchConfiguration& config) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setTreeModel(std::shared_ptr<ITreeModel> treeModel) = 0;
    virtual void setCacheManager(std::shared_ptr<ICacheManager> cacheManager) = 0;

    virtual std::vector<SearchResult> search(const SearchQuery& query) = 0;
    virtual void searchAsync(const SearchQuery& query) = 0;

    virtual void buildIndex(IndexType type = IndexType::INVERTED_INDEX) = 0;
    virtual void rebuildIndex(IndexType type = IndexType::INVERTED_INDEX) = 0;
    virtual void clearIndex(IndexType type = IndexType::INVERTED_INDEX) = 0;
    virtual std::vector<SearchIndex> getIndexInfo() const = 0;

    virtual std::vector<SearchSuggestion> getSuggestions(const std::string& partialQuery, int maxSuggestions = 10) = 0;
    virtual std::vector<std::string> getSearchHistory() const = 0;
    virtual void clearSearchHistory() = 0;

    virtual void addToIndex(const std::string& objectId, const std::string& content, SearchField field, IndexType type = IndexType::INVERTED_INDEX) = 0;
    virtual void removeFromIndex(const std::string& objectId, IndexType type = IndexType::INVERTED_INDEX) = 0;
    virtual void updateIndex(const std::string& objectId, const std::string& oldContent, const std::string& newContent, SearchField field, IndexType type = IndexType::INVERTED_INDEX) = 0;

    virtual SearchConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const SearchConfiguration& config) = 0;

    using SearchCompletedCallback = std::function<void(const std::vector<SearchResult>&, bool)>;
    using IndexProgressCallback = std::function<void(int, int, const std::string&)>;

    virtual void setSearchCompletedCallback(SearchCompletedCallback callback) = 0;
    virtual void setIndexProgressCallback(IndexProgressCallback callback) = 0;
};

class SearchEngine : public QObject, public ISearchEngine {
    Q_OBJECT

public:
    SearchEngine(QObject* parent = nullptr);
    ~SearchEngine() override;

    // ISearchEngine implementation
    void initialize(const SearchConfiguration& config) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setTreeModel(std::shared_ptr<ITreeModel> treeModel) override;
    void setCacheManager(std::shared_ptr<ICacheManager> cacheManager) override;

    std::vector<SearchResult> search(const SearchQuery& query) override;
    void searchAsync(const SearchQuery& query) override;

    void buildIndex(IndexType type = IndexType::INVERTED_INDEX) override;
    void rebuildIndex(IndexType type = IndexType::INVERTED_INDEX) override;
    void clearIndex(IndexType type = IndexType::INVERTED_INDEX) override;
    std::vector<SearchIndex> getIndexInfo() const override;

    std::vector<SearchSuggestion> getSuggestions(const std::string& partialQuery, int maxSuggestions = 10) override;
    std::vector<std::string> getSearchHistory() const override;
    void clearSearchHistory() override;

    void addToIndex(const std::string& objectId, const std::string& content, SearchField field, IndexType type = IndexType::INVERTED_INDEX) override;
    void removeFromIndex(const std::string& objectId, IndexType type = IndexType::INVERTED_INDEX) override;
    void updateIndex(const std::string& objectId, const std::string& oldContent, const std::string& newContent, SearchField field, IndexType type = IndexType::INVERTED_INDEX) override;

    SearchConfiguration getConfiguration() const override;
    void updateConfiguration(const SearchConfiguration& config) override;

    void setSearchCompletedCallback(SearchCompletedCallback callback) override;
    void setIndexProgressCallback(IndexProgressCallback callback) override;

private slots:
    void onIndexTimer();
    void onSearchCompleted();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ITreeModel> treeModel_;
    std::shared_ptr<ICacheManager> cacheManager_;

    // Search infrastructure
    std::unordered_map<IndexType, SearchIndex> searchIndexes_;
    SearchConfiguration config_;
    std::deque<std::string> searchHistory_;
    std::vector<SearchSuggestion> searchSuggestions_;

    // Async search
    QThread* searchThread_;
    std::atomic<bool> searchInProgress_{false};
    std::vector<SearchResult> currentSearchResults_;
    bool currentSearchSuccess_ = false;

    // Callbacks
    SearchCompletedCallback searchCompletedCallback_;
    IndexProgressCallback indexProgressCallback_;

    // Timers
    QTimer* indexTimer_;

    // Internal methods
    void performExactSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performPrefixSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performSubstringSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performFuzzySearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performRegexSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performWildcardSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performPhoneticSearch(const SearchQuery& query, std::vector<SearchResult>& results);
    void performSemanticSearch(const SearchQuery& query, std::vector<SearchResult>& results);

    void buildInvertedIndex();
    void buildTrieIndex();
    void buildHashIndex();
    void buildSuffixArrayIndex();
    void buildFullTextIndex();
    void buildVectorIndex();

    void indexObject(const SchemaObject& object, IndexType type);
    void indexField(const std::string& objectId, const std::string& content, SearchField field, IndexType type);

    double calculateRelevanceScore(const SearchResult& result, const SearchQuery& query);
    void rankResults(std::vector<SearchResult>& results);
    void filterResults(std::vector<SearchResult>& results, const SearchQuery& query);

    std::vector<SearchSuggestion> generateSuggestions(const std::string& partialQuery, int maxSuggestions);
    void updateSearchSuggestions(const SearchQuery& query, const std::vector<SearchResult>& results);

    bool matchesSearchScope(const SearchResult& result, const SearchQuery& query);
    bool matchesSearchFields(const std::string& content, const std::string& pattern,
                           SearchAlgorithm algorithm, bool caseSensitive);

    void updateSearchHistory(const SearchQuery& query);
    void pruneSearchHistory();

    std::string normalizePattern(const std::string& pattern, SearchAlgorithm algorithm);
    std::string normalizeContent(const std::string& content, bool caseSensitive);
    std::vector<std::string> tokenizeContent(const std::string& content);
    std::vector<std::string> stemTokens(const std::vector<std::string>& tokens);
    std::vector<std::string> removeStopWords(const std::vector<std::string>& tokens);

    // Fuzzy search helpers
    int levenshteinDistance(const std::string& s1, const std::string& s2);
    bool fuzzyMatch(const std::string& text, const std::string& pattern, int maxDistance);

    // Phonetic search helpers
    std::string soundex(const std::string& word);
    std::string metaphone(const std::string& word);

    // Semantic search helpers
    std::vector<std::string> findSynonyms(const std::string& word);
    double calculateSemanticSimilarity(const std::string& word1, const std::string& word2);

    // TF-IDF helpers
    double calculateTfIdf(const std::string& term, const std::string& document, IndexType type);
    void updateDocumentFrequency(const std::string& term, IndexType type);

    // Thread-safe operations
    mutable std::mutex indexMutex_;
    mutable std::mutex historyMutex_;
    mutable std::mutex searchMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEARCH_ENGINE_H
```

#### 3.4.1.2: Search Analytics and Optimization
**Objective**: Implement search analytics, performance monitoring, and query optimization

**Implementation Steps:**
1. Create search analytics with usage tracking and performance metrics
2. Implement query optimization with caching and result prefetching
3. Add search performance monitoring and alerting
4. Create search pattern analysis and recommendations

**Code Changes:**

**File: src/search/search_analytics.h**
```cpp
#ifndef SCRATCHROBIN_SEARCH_ANALYTICS_H
#define SCRATCHROBIN_SEARCH_ANALYTICS_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <functional>
#include <mutex>
#include <atomic>
#include <QObject>
#include <QTimer>

namespace scratchrobin {

class ISearchEngine;

struct SearchMetrics {
    std::atomic<std::size_t> totalQueries{0};
    std::atomic<std::size_t> successfulQueries{0};
    std::atomic<std::size_t> failedQueries{0};
    std::atomic<std::size_t> noResultsQueries{0};
    std::atomic<std::size_t> cachedQueries{0};
    std::atomic<std::size_t> indexHits{0};
    std::atomic<std::size_t> indexMisses{0};
    std::chrono::milliseconds averageQueryTime{0};
    std::chrono::milliseconds minQueryTime{0};
    std::chrono::milliseconds maxQueryTime{0};
    std::atomic<std::size_t> totalResults{0};
    std::atomic<std::size_t> uniqueObjectsSearched{0};
    std::atomic<std::size_t> indexSize{0};
    std::chrono::system_clock::time_point lastUpdated;
};

struct QueryProfile {
    std::string queryId;
    std::string pattern;
    SearchAlgorithm algorithm;
    SearchScope scope;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds duration;
    int resultsCount = 0;
    bool success = false;
    std::string errorMessage;
    std::vector<std::string> indexTypesUsed;
    std::vector<std::string> optimizationsApplied;
    std::unordered_map<std::string, std::string> performanceData;
};

struct SearchPattern {
    std::string pattern;
    int frequency = 0;
    std::chrono::system_clock::time_point firstSeen;
    std::chrono::system_clock::time_point lastSeen;
    double averageResults = 0.0;
    std::chrono::milliseconds averageTime;
    std::vector<std::string> commonResults;
    bool isPopular = false;
    std::vector<std::string> suggestedOptimizations;
};

struct SearchOptimization {
    std::string optimizationId;
    std::string name;
    std::string description;
    std::string type;
    double impactScore = 0.0;
    bool isApplied = false;
    std::chrono::system_clock::time_point suggestedAt;
    std::chrono::system_clock::time_point appliedAt;
    std::unordered_map<std::string, std::string> metadata;
};

struct SearchAlert {
    std::string alertId;
    std::string type;
    std::string severity;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged = false;
    std::unordered_map<std::string, std::string> context;
};

struct SearchAnalyticsConfiguration {
    bool enableMetrics = true;
    bool enableQueryProfiling = true;
    int maxQueryHistorySize = 10000;
    int maxPatternHistorySize = 1000;
    bool enablePerformanceAlerts = true;
    int alertCheckIntervalSeconds = 60;
    double slowQueryThresholdMs = 1000.0;
    double highResultThreshold = 10000;
    bool enableRecommendations = true;
    bool enableAutoOptimization = false;
    int optimizationCheckIntervalSeconds = 3600;
};

class ISearchAnalytics {
public:
    virtual ~ISearchAnalytics() = default;

    virtual void initialize(const SearchAnalyticsConfiguration& config) = 0;
    virtual void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) = 0;

    virtual void recordQuery(const SearchQuery& query, const std::vector<SearchResult>& results,
                           std::chrono::milliseconds duration, bool success) = 0;
    virtual void recordQueryError(const SearchQuery& query, const std::string& error,
                                std::chrono::milliseconds duration) = 0;

    virtual SearchMetrics getMetrics() const = 0;
    virtual std::vector<QueryProfile> getRecentQueries(int limit = 100) const = 0;
    virtual std::vector<SearchPattern> getPopularPatterns(int limit = 50) const = 0;

    virtual std::vector<SearchOptimization> getRecommendedOptimizations() const = 0;
    virtual void applyOptimization(const std::string& optimizationId) = 0;

    virtual std::vector<SearchAlert> getActiveAlerts() const = 0;
    virtual void acknowledgeAlert(const std::string& alertId) = 0;
    virtual void clearAlerts() = 0;

    virtual void generatePerformanceReport(const std::string& filePath) const = 0;
    virtual void generateUsageReport(const std::string& filePath) const = 0;

    virtual SearchAnalyticsConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const SearchAnalyticsConfiguration& config) = 0;

    using MetricsUpdatedCallback = std::function<void(const SearchMetrics&)>;
    using AlertTriggeredCallback = std::function<void(const SearchAlert&)>;
    using OptimizationRecommendedCallback = std::function<void(const SearchOptimization&)>;

    virtual void setMetricsUpdatedCallback(MetricsUpdatedCallback callback) = 0;
    virtual void setAlertTriggeredCallback(AlertTriggeredCallback callback) = 0;
    virtual void setOptimizationRecommendedCallback(OptimizationRecommendedCallback callback) = 0;
};

class SearchAnalytics : public QObject, public ISearchAnalytics {
    Q_OBJECT

public:
    SearchAnalytics(QObject* parent = nullptr);
    ~SearchAnalytics() override;

    // ISearchAnalytics implementation
    void initialize(const SearchAnalyticsConfiguration& config) override;
    void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) override;

    void recordQuery(const SearchQuery& query, const std::vector<SearchResult>& results,
                   std::chrono::milliseconds duration, bool success) override;
    void recordQueryError(const SearchQuery& query, const std::string& error,
                        std::chrono::milliseconds duration) override;

    SearchMetrics getMetrics() const override;
    std::vector<QueryProfile> getRecentQueries(int limit = 100) const override;
    std::vector<SearchPattern> getPopularPatterns(int limit = 50) const override;

    std::vector<SearchOptimization> getRecommendedOptimizations() const override;
    void applyOptimization(const std::string& optimizationId) override;

    std::vector<SearchAlert> getActiveAlerts() const override;
    void acknowledgeAlert(const std::string& alertId) override;
    void clearAlerts() override;

    void generatePerformanceReport(const std::string& filePath) const override;
    void generateUsageReport(const std::string& filePath) const override;

    SearchAnalyticsConfiguration getConfiguration() const override;
    void updateConfiguration(const SearchAnalyticsConfiguration& config) override;

    void setMetricsUpdatedCallback(MetricsUpdatedCallback callback) override;
    void setAlertTriggeredCallback(AlertTriggeredCallback callback) override;
    void setOptimizationRecommendedCallback(OptimizationRecommendedCallback callback) override;

private slots:
    void onAlertCheckTimer();
    void onOptimizationCheckTimer();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<ISearchEngine> searchEngine_;

    // Analytics data
    SearchMetrics metrics_;
    std::deque<QueryProfile> queryHistory_;
    std::unordered_map<std::string, SearchPattern> searchPatterns_;
    std::vector<SearchOptimization> optimizations_;
    std::vector<SearchAlert> alerts_;
    SearchAnalyticsConfiguration config_;

    // Callbacks
    MetricsUpdatedCallback metricsCallback_;
    AlertTriggeredCallback alertCallback_;
    OptimizationRecommendedCallback optimizationCallback_;

    // Timers
    QTimer* alertCheckTimer_;
    QTimer* optimizationCheckTimer_;

    // Internal methods
    void updateMetrics(const QueryProfile& profile);
    void updateSearchPatterns(const QueryProfile& profile);
    void checkPerformanceAlerts();
    void generateOptimizations();

    void createSlowQueryAlert(const QueryProfile& profile);
    void createHighResultAlert(const QueryProfile& profile);
    void createNoResultAlert(const QueryProfile& profile);
    void createIndexMissAlert();

    void suggestIndexOptimization(const SearchPattern& pattern);
    void suggestCacheOptimization(const SearchPattern& pattern);
    void suggestQueryOptimization(const SearchPattern& pattern);

    bool isQuerySlow(std::chrono::milliseconds duration) const;
    bool hasHighResultCount(int count) const;
    bool isPatternPopular(const SearchPattern& pattern) const;

    void pruneQueryHistory();
    void pruneSearchPatterns();

    std::string generateQueryId();
    std::string generateAlertId();
    std::string generateOptimizationId();

    void writeReportHeader(std::ofstream& file, const std::string& title) const;
    void writeMetricsSection(std::ofstream& file) const;
    void writeQueryAnalysisSection(std::ofstream& file) const;
    void writePatternAnalysisSection(std::ofstream& file) const;
    void writeOptimizationSection(std::ofstream& file) const;
    void writeAlertSection(std::ofstream& file) const;

    // Thread-safe operations
    mutable std::mutex metricsMutex_;
    mutable std::mutex historyMutex_;
    mutable std::mutex patternMutex_;
    mutable std::mutex alertMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEARCH_ANALYTICS_H
```

### Task 3.4.2: Advanced Search Features

#### 3.4.2.1: Multi-Database Search
**Objective**: Implement search capabilities across multiple databases and connections

**Implementation Steps:**
1. Create federated search across multiple database connections
2. Implement search result aggregation and deduplication
3. Add search result ranking across databases
4. Create cross-database search analytics

**Code Changes:**

**File: src/search/federated_search.h**
```cpp
#ifndef SCRATCHROBIN_FEDERATED_SEARCH_H
#define SCRATCHROBIN_FEDERATED_SEARCH_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <atomic>
#include <QObject>
#include <QTimer>

namespace scratchrobin {

class IConnectionManager;
class IMetadataManager;
class ISearchEngine;

struct FederatedSearchQuery {
    std::string pattern;
    SearchAlgorithm algorithm = SearchAlgorithm::SUBSTRING_MATCH;
    std::vector<std::string> connectionIds;
    std::vector<std::string> databaseFilters;
    std::vector<std::string> schemaFilters;
    bool parallelExecution = true;
    int maxResultsPerConnection = 100;
    int maxTotalResults = 1000;
    std::chrono::milliseconds timeout{30000};
    double minScore = 0.0;
    bool includeSystemObjects = false;
    bool enableDeduplication = true;
};

struct FederatedSearchResult {
    std::string queryId;
    std::vector<SearchResult> results;
    std::unordered_map<std::string, int> connectionResultCounts;
    std::unordered_map<std::string, std::chrono::milliseconds> connectionTimes;
    int totalResults = 0;
    int uniqueResults = 0;
    int duplicateResults = 0;
    std::chrono::milliseconds totalTime;
    bool success = false;
    std::string errorMessage;
    std::chrono::system_clock::time_point completedAt;
};

struct ConnectionSearchResult {
    std::string connectionId;
    std::string connectionName;
    std::vector<SearchResult> results;
    std::chrono::milliseconds searchTime;
    bool success = false;
    std::string errorMessage;
};

struct SearchResultDeduplication {
    std::string deduplicationKey;
    std::vector<SearchResult> duplicates;
    SearchResult bestMatch;
};

struct FederatedSearchConfiguration {
    bool enableParallelSearch = true;
    int maxConcurrentConnections = 5;
    int connectionTimeoutSeconds = 30;
    bool enableResultCaching = true;
    int cacheTTLSeconds = 300;
    bool enableLoadBalancing = true;
    bool enableFailover = true;
    int maxRetriesPerConnection = 3;
    bool enableProgressReporting = true;
    int progressReportIntervalMs = 1000;
    bool enableResultStreaming = false;
};

class IFederatedSearch {
public:
    virtual ~IFederatedSearch() = default;

    virtual void initialize(const FederatedSearchConfiguration& config) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) = 0;

    virtual FederatedSearchResult search(const FederatedSearchQuery& query) = 0;
    virtual void searchAsync(const FederatedSearchQuery& query) = 0;

    virtual std::vector<std::string> getAvailableConnections() const = 0;
    virtual ConnectionSearchResult searchConnection(const std::string& connectionId, const SearchQuery& query) = 0;
    virtual void searchConnectionAsync(const std::string& connectionId, const SearchQuery& query) = 0;

    virtual void cancelSearch(const std::string& queryId) = 0;
    virtual bool isSearchActive(const std::string& queryId) const = 0;

    virtual FederatedSearchConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const FederatedSearchConfiguration& config) = 0;

    using SearchProgressCallback = std::function<void(const std::string&, int, int, const std::string&)>;
    using SearchCompletedCallback = std::function<void(const FederatedSearchResult&)>;
    using ConnectionSearchCallback = std::function<void(const ConnectionSearchResult&)>;

    virtual void setSearchProgressCallback(SearchProgressCallback callback) = 0;
    virtual void setSearchCompletedCallback(SearchCompletedCallback callback) = 0;
    virtual void setConnectionSearchCallback(ConnectionSearchCallback callback) = 0;
};

class FederatedSearch : public QObject, public IFederatedSearch {
    Q_OBJECT

public:
    FederatedSearch(QObject* parent = nullptr);
    ~FederatedSearch() override;

    // IFederatedSearch implementation
    void initialize(const FederatedSearchConfiguration& config) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) override;

    FederatedSearchResult search(const FederatedSearchQuery& query) override;
    void searchAsync(const FederatedSearchQuery& query) override;

    std::vector<std::string> getAvailableConnections() const override;
    ConnectionSearchResult searchConnection(const std::string& connectionId, const SearchQuery& query) override;
    void searchConnectionAsync(const std::string& connectionId, const SearchQuery& query) override;

    void cancelSearch(const std::string& queryId) override;
    bool isSearchActive(const std::string& queryId) const override;

    FederatedSearchConfiguration getConfiguration() const override;
    void updateConfiguration(const FederatedSearchConfiguration& config) override;

    void setSearchProgressCallback(SearchProgressCallback callback) override;
    void setSearchCompletedCallback(SearchCompletedCallback callback) override;
    void setConnectionSearchCallback(ConnectionSearchCallback callback) override;

private slots:
    void onProgressTimer();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISearchEngine> searchEngine_;

    // Search infrastructure
    FederatedSearchConfiguration config_;
    std::unordered_map<std::string, std::future<FederatedSearchResult>> activeSearches_;
    std::unordered_map<std::string, std::future<ConnectionSearchResult>> activeConnectionSearches_;
    std::atomic<int> activeSearchCount_{0};

    // Callbacks
    SearchProgressCallback progressCallback_;
    SearchCompletedCallback completedCallback_;
    ConnectionSearchCallback connectionCallback_;

    // Timers
    QTimer* progressTimer_;

    // Internal methods
    FederatedSearchResult executeFederatedSearch(const FederatedSearchQuery& query);
    ConnectionSearchResult executeConnectionSearch(const std::string& connectionId, const SearchQuery& query);

    void executeParallelSearch(const FederatedSearchQuery& query, FederatedSearchResult& result);
    void executeSequentialSearch(const FederatedSearchQuery& query, FederatedSearchResult& result);

    std::vector<SearchResult> collectResults(const std::vector<ConnectionSearchResult>& connectionResults,
                                           const FederatedSearchQuery& query);
    std::vector<SearchResult> deduplicateResults(const std::vector<SearchResult>& results);
    void rankFederatedResults(std::vector<SearchResult>& results);

    std::vector<std::string> selectConnectionsForSearch(const FederatedSearchQuery& query);
    void loadBalanceConnections(std::vector<std::string>& connectionIds);

    ConnectionSearchResult searchWithRetry(const std::string& connectionId, const SearchQuery& query, int maxRetries);
    ConnectionSearchResult performConnectionSearch(const std::string& connectionId, const SearchQuery& query);

    void updateSearchProgress(const std::string& queryId, int completed, int total, const std::string& message);
    void notifySearchCompletion(const FederatedSearchResult& result);

    std::string generateQueryId();
    std::string createDeduplicationKey(const SearchResult& result);

    double calculateFederatedScore(const SearchResult& result, const std::string& connectionId);
    int getConnectionPriority(const std::string& connectionId) const;

    void cacheSearchResults(const std::string& queryId, const FederatedSearchResult& result);
    FederatedSearchResult getCachedResults(const FederatedSearchQuery& query);

    void handleSearchError(const std::string& queryId, const std::string& error);
    void handleConnectionError(const std::string& connectionId, const std::string& error);

    // Thread-safe operations
    mutable std::mutex searchMutex_;
    mutable std::mutex cacheMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_FEDERATED_SEARCH_H
```

#### 3.4.2.2: Search UI Integration
**Objective**: Create comprehensive search UI with advanced features and real-time feedback

**Implementation Steps:**
1. Implement advanced search dialog with all search options
2. Create search results display with highlighting and navigation
3. Add search history and saved searches
4. Implement search result export and sharing

**Code Changes:**

**File: src/ui/search/search_dialog.h**
```cpp
#ifndef SCRATCHROBIN_SEARCH_DIALOG_H
#define SCRATCHROBIN_SEARCH_DIALOG_H

#include <memory>
#include <string>
#include <vector>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QAction>
#include <QMenu>

namespace scratchrobin {

class ISearchEngine;
class IFederatedSearch;
class IConnectionManager;

enum class SearchDialogMode {
    BASIC,
    ADVANCED,
    FEDERATED,
    SAVED_SEARCHES
};

enum class SearchResultView {
    LIST,
    TREE,
    TABLE,
    PREVIEW
};

struct SearchDialogConfiguration {
    SearchDialogMode defaultMode = SearchDialogMode::BASIC;
    bool showAdvancedOptions = true;
    bool showFederatedOptions = true;
    bool showSavedSearches = true;
    bool enableRealTimeSearch = false;
    int realTimeDelayMs = 300;
    bool showSearchHistory = true;
    int maxHistoryItems = 20;
    bool enableResultPreview = true;
    bool enableResultExport = true;
    bool enableResultSharing = false;
    int maxResultsDisplay = 1000;
    bool showProgressDetails = true;
    bool autoSaveSearches = true;
};

class ISearchDialog {
public:
    virtual ~ISearchDialog() = default;

    virtual void initialize(const SearchDialogConfiguration& config) = 0;
    virtual void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) = 0;
    virtual void setFederatedSearch(std::shared_ptr<IFederatedSearch> federatedSearch) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;

    virtual void setSearchQuery(const SearchQuery& query) = 0;
    virtual SearchQuery getSearchQuery() const = 0;

    virtual void executeSearch() = 0;
    virtual void stopSearch() = 0;
    virtual bool isSearchActive() const = 0;

    virtual std::vector<SearchResult> getSearchResults() const = 0;
    virtual SearchResult getSelectedResult() const = 0;

    virtual void saveCurrentSearch(const std::string& name) = 0;
    virtual void loadSavedSearch(const std::string& name) = 0;
    virtual void deleteSavedSearch(const std::string& name) = 0;
    virtual std::vector<std::string> getSavedSearches() const = 0;

    virtual void exportResults(const std::string& filePath, const std::string& format = "CSV") = 0;

    virtual SearchDialogConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const SearchDialogConfiguration& config) = 0;

    using SearchStartedCallback = std::function<void(const SearchQuery&)>;
    using SearchCompletedCallback = std::function<void(const std::vector<SearchResult>&, bool)>;
    using ResultSelectedCallback = std::function<void(const SearchResult&)>;

    virtual void setSearchStartedCallback(SearchStartedCallback callback) = 0;
    virtual void setSearchCompletedCallback(SearchCompletedCallback callback) = 0;
    virtual void setResultSelectedCallback(ResultSelectedCallback callback) = 0;

    virtual QDialog* getDialog() = 0;
    virtual int exec() = 0;
};

class SearchDialog : public QDialog, public ISearchDialog {
    Q_OBJECT

public:
    SearchDialog(QWidget* parent = nullptr);
    ~SearchDialog() override;

    // ISearchDialog implementation
    void initialize(const SearchDialogConfiguration& config) override;
    void setSearchEngine(std::shared_ptr<ISearchEngine> searchEngine) override;
    void setFederatedSearch(std::shared_ptr<IFederatedSearch> federatedSearch) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;

    void setSearchQuery(const SearchQuery& query) override;
    SearchQuery getSearchQuery() const override;

    void executeSearch() override;
    void stopSearch() override;
    bool isSearchActive() const override;

    std::vector<SearchResult> getSearchResults() const override;
    SearchResult getSelectedResult() const override;

    void saveCurrentSearch(const std::string& name) override;
    void loadSavedSearch(const std::string& name) override;
    void deleteSavedSearch(const std::string& name) override;
    std::vector<std::string> getSavedSearches() const override;

    void exportResults(const std::string& filePath, const std::string& format = "CSV") override;

    SearchDialogConfiguration getConfiguration() const override;
    void updateConfiguration(const SearchDialogConfiguration& config) override;

    void setSearchStartedCallback(SearchStartedCallback callback) override;
    void setSearchCompletedCallback(SearchCompletedCallback callback) override;
    void setResultSelectedCallback(ResultSelectedCallback callback) override;

    QDialog* getDialog() override;
    int exec() override;

private slots:
    void onSearchButtonClicked();
    void onStopButtonClicked();
    void onClearButtonClicked();
    void onSaveSearchButtonClicked();
    void onLoadSearchButtonClicked();
    void onExportButtonClicked();
    void onAdvancedOptionsToggled(bool checked);
    void onFederatedOptionsToggled(bool checked);
    void onSearchTextChanged(const QString& text);
    void onAlgorithmChanged(int index);
    void onScopeChanged(int index);
    void onResultItemDoubleClicked(QListWidgetItem* item);
    void onResultItemSelectionChanged();
    void onTabChanged(int index);
    void onRealTimeSearchToggled(bool checked);
    void onPreviewToggled(bool checked);
    void onHistoryItemClicked(QListWidgetItem* item);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Dialog components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* searchLayout_;
    QTabWidget* tabWidget_;

    // Basic search tab
    QWidget* basicTab_;
    QVBoxLayout* basicLayout_;
    QLineEdit* searchText_;
    QComboBox* algorithmCombo_;
    QComboBox* scopeCombo_;
    QPushButton* searchButton_;
    QPushButton* stopButton_;
    QPushButton* clearButton_;

    // Advanced search tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* fieldGroup_;
    QGroupBox* filterGroup_;
    QGroupBox* optionGroup_;

    // Federated search tab
    QWidget* federatedTab_;
    QVBoxLayout* federatedLayout_;
    QListWidget* connectionList_;
    QGroupBox* federatedOptionGroup_;

    // Saved searches tab
    QWidget* savedTab_;
    QVBoxLayout* savedLayout_;
    QListWidget* savedSearchList_;
    QPushButton* loadSearchButton_;
    QPushButton* deleteSearchButton_;

    // Results area
    QSplitter* resultSplitter_;
    QWidget* resultWidget_;
    QVBoxLayout* resultLayout_;
    QListWidget* resultList_;
    QTreeWidget* resultTree_;
    QTableWidget* resultTable_;
    QTextEdit* resultPreview_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    // Side panel
    QWidget* sideWidget_;
    QVBoxLayout* sideLayout_;
    QGroupBox* historyGroup_;
    QListWidget* searchHistory_;
    QGroupBox* previewGroup_;
    QTextEdit* previewText_;

    // Dialog buttons
    QHBoxLayout* buttonLayout_;
    QPushButton* saveSearchButton_;
    QPushButton* exportButton_;
    QPushButton* closeButton_;
    QPushButton* helpButton_;

    // Configuration and state
    SearchDialogConfiguration config_;
    SearchQuery currentQuery_;
    std::vector<SearchResult> searchResults_;
    std::vector<std::string> savedSearches_;
    std::vector<std::string> searchHistory_;
    bool searchActive_ = false;
    SearchResultView currentView_ = SearchResultView::LIST;

    // Callbacks
    SearchStartedCallback searchStartedCallback_;
    SearchCompletedCallback searchCompletedCallback_;
    ResultSelectedCallback resultSelectedCallback_;

    // Core components
    std::shared_ptr<ISearchEngine> searchEngine_;
    std::shared_ptr<IFederatedSearch> federatedSearch_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Internal methods
    void setupUI();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupFederatedTab();
    void setupSavedTab();
    void setupResultArea();
    void setupSidePanel();
    void setupButtons();
    void setupConnections();

    void populateAlgorithmCombo();
    void populateScopeCombo();
    void populateConnectionList();
    void updateSavedSearchList();
    void updateSearchHistory();

    void showSearchProgress(bool visible, const std::string& message = "");
    void updateSearchStatus(const std::string& message, int progress = -1);

    void displayResultsList();
    void displayResultsTree();
    void displayResultsTable();
    void displayResultPreview(const SearchResult& result);

    void createResultListItem(const SearchResult& result);
    void createResultTreeItem(const SearchResult& result);
    void createResultTableItem(const SearchResult& result);

    void highlightSearchText(const std::string& text, const std::string& searchPattern);
    void clearResultDisplay();

    void saveSearchToSettings(const std::string& name, const SearchQuery& query);
    SearchQuery loadSearchFromSettings(const std::string& name);
    void deleteSearchFromSettings(const std::string& name);

    void addToSearchHistory(const SearchQuery& query);
    void loadSearchHistory();

    void exportToCsv(const std::string& filePath);
    void exportToJson(const std::string& filePath);
    void exportToXml(const std::string& filePath);

    void handleSearchCompletion(const std::vector<SearchResult>& results, bool success);
    void handleSearchError(const std::string& error);

    void showAdvancedOptions(bool show);
    void showFederatedOptions(bool show);

    void enableRealTimeSearch(bool enable);
    void performRealTimeSearch();

    std::string getSelectedConnection() const;
    std::vector<std::string> getSelectedConnections() const;

    void resizeDialogToContent();
    void saveDialogState();
    void restoreDialogState();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEARCH_DIALOG_H
```

## Summary

This phase 3.4 implementation provides comprehensive search and indexing functionality for ScratchRobin with the following key features:

✅ **Full-Text Search Engine**: Multi-algorithm search with exact, fuzzy, regex, phonetic, and semantic matching
✅ **Advanced Indexing**: 6 different index types (inverted, trie, hash, suffix array, full-text, vector)
✅ **Search Analytics**: Performance monitoring, query profiling, and optimization recommendations
✅ **Federated Search**: Cross-database and cross-connection search with load balancing
✅ **Search UI Integration**: Professional search dialog with advanced options and result visualization
✅ **Real-time Search**: Incremental search with suggestions and auto-completion
✅ **Search Optimization**: Query optimization, result caching, and performance tuning
✅ **Enterprise Features**: Search history, saved searches, result export, and analytics

The search and indexing system provides enterprise-grade search capabilities comparable to commercial database tools, with comprehensive functionality, excellent performance, and seamless integration with the existing metadata and UI systems.

**Next Phase**: Phase 4.1 - Editor Component Implementation
