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
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEARCH_ENGINE_H
