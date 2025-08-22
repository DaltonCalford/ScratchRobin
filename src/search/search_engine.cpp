#include "search/search_engine.h"
#include "metadata/metadata_manager.h"
#include "ui/object_browser/tree_model.h"
#include "metadata/cache_manager.h"
#include <QApplication>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace scratchrobin {

class SearchEngine::Impl {
public:
    Impl() = default;

    std::vector<SearchResult> performSearch(const SearchQuery& query) {
        std::vector<SearchResult> allResults;

        // Search across all available indexes
        for (const auto& [type, index] : searchIndexes_) {
            if (config_.enabledIndexTypes.empty() ||
                std::find(config_.enabledIndexTypes.begin(), config_.enabledIndexTypes.end(), type) != config_.enabledIndexTypes.end()) {

                auto indexResults = searchIndex(index, query, type);
                allResults.insert(allResults.end(), indexResults.begin(), indexResults.end());
            }
        }

        // If no indexes are available, fall back to direct metadata search
        if (allResults.empty() && searchIndexes_.empty()) {
            allResults = performDirectSearch(query);
        }

        // Deduplicate and rank results
        deduplicateResults(allResults);
        rankResults(allResults, query);
        filterResults(allResults, query);

        // Limit results
        if (allResults.size() > static_cast<size_t>(query.maxResults)) {
            allResults.resize(query.maxResults);
        }

        return allResults;
    }

    std::vector<SearchResult> searchIndex(const SearchIndex& index, const SearchQuery& query, IndexType type) {
        std::vector<SearchResult> results;

        // Tokenize the search pattern
        auto tokens = tokenizeContent(query.pattern);

        // Apply stemming if enabled
        if (config_.enableStemming) {
            tokens = stemTokens(tokens);
        }

        // Remove stop words if enabled
        if (config_.enableStopWords) {
            tokens = removeStopWords(tokens);
        }

        // Search based on algorithm
        switch (query.algorithm) {
            case SearchAlgorithm::EXACT_MATCH:
                results = exactSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::PREFIX_MATCH:
                results = prefixSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::SUBSTRING_MATCH:
                results = substringSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::FUZZY_MATCH:
                results = fuzzySearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::REGEX_MATCH:
                results = regexSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::WILDCARD_MATCH:
                results = wildcardSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::PHONETIC_MATCH:
                results = phoneticSearch(index, tokens, query, type);
                break;
            case SearchAlgorithm::SEMANTIC_MATCH:
                results = semanticSearch(index, tokens, query, type);
                break;
            default:
                results = substringSearch(index, tokens, query, type);
                break;
        }

        return results;
    }

    std::vector<SearchResult> performDirectSearch(const SearchQuery& query) {
        std::vector<SearchResult> results;

        if (!metadataManager_) {
            return results;
        }

        try {
            // Get all objects from metadata manager
            // This is a simplified implementation - in practice, you'd iterate through all available objects
            SchemaCollectionOptions options;
            options.includeSystemObjects = query.scope == SearchScope::ALL;

            auto collectionResult = metadataManager_->collectSchema(options);
            if (!collectionResult.success()) {
                return results;
            }

            for (const auto& object : collectionResult.value().objects) {
                if (matchesSearchCriteria(object, query)) {
                    SearchResult result = createSearchResult(object, query);
                    if (result.relevanceScore >= query.minScore) {
                        results.push_back(result);
                    }
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "Error in direct search:" << e.what();
        }

        return results;
    }

    bool matchesSearchCriteria(const SchemaObject& object, const SearchQuery& query) {
        std::string searchText;

        // Build searchable text based on fields
        for (auto field : query.fields) {
            switch (field) {
                case SearchField::NAME:
                    searchText += object.name + " ";
                    break;
                case SearchField::TYPE:
                    searchText += toString(object.type) + " ";
                    break;
                case SearchField::SCHEMA:
                    searchText += object.schema + " ";
                    break;
                case SearchField::DATABASE:
                    searchText += object.database + " ";
                    break;
                case SearchField::OWNER:
                    searchText += object.owner + " ";
                    break;
                case SearchField::PROPERTIES:
                    for (const auto& [key, value] : object.properties) {
                        searchText += key + " " + value + " ";
                    }
                    break;
                case SearchField::DEFINITION:
                    // Object definitions would be stored separately
                    break;
                case SearchField::COMMENTS:
                    // Comments would be stored separately
                    break;
                case SearchField::ALL:
                    searchText += object.name + " " + toString(object.type) + " " +
                                object.schema + " " + object.database + " " + object.owner + " ";
                    for (const auto& [key, value] : object.properties) {
                        searchText += key + " " + value + " ";
                    }
                    break;
            }
        }

        // Apply case sensitivity
        if (!query.caseSensitive) {
            searchText = toLower(searchText);
        }

        std::string normalizedPattern = normalizePattern(query.pattern, query.algorithm);
        if (!query.caseSensitive) {
            normalizedPattern = toLower(normalizedPattern);
        }

        // Perform the actual search based on algorithm
        return matchesSearchText(searchText, normalizedPattern, query.algorithm,
                               query.caseSensitive, query.maxDistance);
    }

    SearchResult createSearchResult(const SchemaObject& object, const SearchQuery& query) {
        SearchResult result;
        result.objectId = generateObjectId(object);
        result.objectName = object.name;
        result.objectType = object.type;
        result.schema = object.schema;
        result.database = object.database;
        result.connectionId = ""; // Would be set from connection context
        result.foundAt = std::chrono::system_clock::now();

        // Determine matched field and extract context
        std::tie(result.matchedField, result.matchedText, result.contextSnippet, result.matchPosition) =
            findBestMatch(object, query);

        // Calculate relevance score
        result.relevanceScore = calculateRelevanceScore(object, query);

        // Set metadata
        result.metadata["object_size"] = "0"; // Would be calculated
        result.metadata["last_modified"] = formatTimestamp(object.modifiedAt);
        result.metadata["created"] = formatTimestamp(object.createdAt);

        return result;
    }

    std::tuple<SearchField, std::string, std::string, int> findBestMatch(
        const SchemaObject& object, const SearchQuery& query) {

        std::string bestMatch;
        std::string bestContext;
        SearchField bestField = SearchField::NAME;
        int bestPosition = -1;
        double bestScore = 0.0;

        // Check each field for matches
        for (auto field : query.fields) {
            std::string fieldText = getFieldText(object, field);
            if (fieldText.empty()) continue;

            auto [match, context, position] = findMatchInText(fieldText, query.pattern, query.algorithm);
            if (!match.empty()) {
                double score = calculateFieldMatchScore(fieldText, match, field, query.algorithm);
                if (score > bestScore) {
                    bestScore = score;
                    bestMatch = match;
                    bestContext = context;
                    bestField = field;
                    bestPosition = position;
                }
            }
        }

        return {bestField, bestMatch, bestContext, bestPosition};
    }

    std::string getFieldText(const SchemaObject& object, SearchField field) {
        switch (field) {
            case SearchField::NAME: return object.name;
            case SearchField::TYPE: return toString(object.type);
            case SearchField::SCHEMA: return object.schema;
            case SearchField::DATABASE: return object.database;
            case SearchField::OWNER: return object.owner;
            case SearchField::PROPERTIES: {
                std::string props;
                for (const auto& [key, value] : object.properties) {
                    props += key + " " + value + " ";
                }
                return props;
            }
            case SearchField::DEFINITION:
            case SearchField::COMMENTS:
                return ""; // Would be populated from separate storage
            case SearchField::ALL: {
                std::string all = object.name + " " + toString(object.type) + " " +
                                object.schema + " " + object.database + " " + object.owner + " ";
                for (const auto& [key, value] : object.properties) {
                    all += key + " " + value + " ";
                }
                return all;
            }
            default: return "";
        }
    }

    std::tuple<std::string, std::string, int> findMatchInText(
        const std::string& text, const std::string& pattern, SearchAlgorithm algorithm) {

        std::string searchText = text;
        std::string searchPattern = pattern;

        if (algorithm != SearchAlgorithm::REGEX_MATCH && algorithm != SearchAlgorithm::CASE_SENSITIVE) {
            searchText = toLower(text);
            searchPattern = toLower(pattern);
        }

        size_t pos = std::string::npos;
        switch (algorithm) {
            case SearchAlgorithm::EXACT_MATCH:
                pos = searchText.find(searchPattern);
                break;
            case SearchAlgorithm::PREFIX_MATCH:
                pos = searchText.find(searchPattern);
                if (pos != std::string::npos && pos > 0 && searchText[pos-1] != ' ') {
                    pos = std::string::npos;
                }
                break;
            case SearchAlgorithm::SUBSTRING_MATCH:
                pos = searchText.find(searchPattern);
                break;
            case SearchAlgorithm::FUZZY_MATCH:
                pos = findFuzzyMatch(searchText, searchPattern, 2); // max distance 2
                break;
            case SearchAlgorithm::REGEX_MATCH:
                try {
                    std::regex regex(searchPattern);
                    std::smatch match;
                    std::string::const_iterator searchStart(text.cbegin());
                    if (std::regex_search(searchStart, text.cend(), match, regex)) {
                        pos = match.position();
                    }
                } catch (const std::regex_error&) {
                    pos = std::string::npos;
                }
                break;
            case SearchAlgorithm::WILDCARD_MATCH:
                pos = findWildcardMatch(searchText, searchPattern);
                break;
            default:
                pos = searchText.find(searchPattern);
                break;
        }

        if (pos != std::string::npos) {
            // Extract match and context
            std::string match = text.substr(pos, pattern.length());
            std::string context = extractContext(text, pos, pattern.length(), 50);
            return {match, context, static_cast<int>(pos)};
        }

        return {"", "", -1};
    }

    size_t findFuzzyMatch(const std::string& text, const std::string& pattern, int maxDistance) {
        size_t bestPos = std::string::npos;
        int bestDistance = maxDistance + 1;

        for (size_t i = 0; i <= text.length() - pattern.length(); ++i) {
            std::string substring = text.substr(i, pattern.length());
            int distance = levenshteinDistance(substring, pattern);
            if (distance <= maxDistance && distance < bestDistance) {
                bestDistance = distance;
                bestPos = i;
            }
        }

        return bestPos;
    }

    size_t findWildcardMatch(const std::string& text, const std::string& pattern) {
        // Convert wildcard pattern to regex
        std::string regexPattern = pattern;
        regexPattern = std::regex_replace(regexPattern, std::regex("\\*"), ".*");
        regexPattern = std::regex_replace(regexPattern, std::regex("\\?"), ".");

        try {
            std::regex regex(regexPattern);
            std::smatch match;
            std::string::const_iterator searchStart(text.cbegin());
            if (std::regex_search(searchStart, text.cend(), match, regex)) {
                return match.position();
            }
        } catch (const std::regex_error&) {
            // Fallback to literal search
            return text.find(pattern);
        }

        return std::string::npos;
    }

    std::string extractContext(const std::string& text, size_t pos, size_t length, size_t contextSize) {
        size_t start = (pos > contextSize) ? pos - contextSize : 0;
        size_t end = std::min(text.length(), pos + length + contextSize);

        std::string context = text.substr(start, end - start);

        // Add ellipsis if truncated
        if (start > 0) context = "..." + context;
        if (end < text.length()) context = context + "...";

        return context;
    }

    double calculateRelevanceScore(const SchemaObject& object, const SearchQuery& query) {
        double score = 0.0;

        // Base score by match quality
        score += calculateBaseScore(object, query);

        // Boost by field importance
        score += calculateFieldBoost(object, query);

        // Boost by object type relevance
        score += calculateTypeBoost(object, query);

        // Boost by recency
        score += calculateRecencyBoost(object);

        // Normalize score to 0-1 range
        return std::min(1.0, score / 10.0);
    }

    double calculateFieldMatchScore(const std::string& fieldText, const std::string& match,
                                  SearchField field, SearchAlgorithm algorithm) {
        double score = 0.0;

        // Base score by match length vs field length
        score += static_cast<double>(match.length()) / fieldText.length();

        // Boost by field importance
        switch (field) {
            case SearchField::NAME: score *= 1.5; break;
            case SearchField::TYPE: score *= 1.2; break;
            case SearchField::PROPERTIES: score *= 1.0; break;
            default: score *= 1.0; break;
        }

        // Boost by algorithm precision
        switch (algorithm) {
            case SearchAlgorithm::EXACT_MATCH: score *= 2.0; break;
            case SearchAlgorithm::PREFIX_MATCH: score *= 1.5; break;
            case SearchAlgorithm::SUBSTRING_MATCH: score *= 1.0; break;
            case SearchAlgorithm::FUZZY_MATCH: score *= 0.8; break;
            default: score *= 1.0; break;
        }

        return score;
    }

    bool matchesSearchText(const std::string& text, const std::string& pattern,
                          SearchAlgorithm algorithm, bool caseSensitive, int maxDistance) {
        switch (algorithm) {
            case SearchAlgorithm::EXACT_MATCH:
                return caseSensitive ? text.find(pattern) != std::string::npos :
                                     toLower(text).find(toLower(pattern)) != std::string::npos;

            case SearchAlgorithm::PREFIX_MATCH: {
                std::string searchText = caseSensitive ? text : toLower(text);
                std::string searchPattern = caseSensitive ? pattern : toLower(pattern);
                size_t pos = searchText.find(searchPattern);
                return pos != std::string::npos && (pos == 0 || searchText[pos-1] == ' ');
            }

            case SearchAlgorithm::SUBSTRING_MATCH:
                return caseSensitive ? text.find(pattern) != std::string::npos :
                                     toLower(text).find(toLower(pattern)) != std::string::npos;

            case SearchAlgorithm::FUZZY_MATCH:
                return findFuzzyMatch(caseSensitive ? text : toLower(text),
                                    caseSensitive ? pattern : toLower(pattern),
                                    maxDistance) != std::string::npos;

            case SearchAlgorithm::REGEX_MATCH:
                try {
                    QRegularExpression regex(QString::fromStdString(pattern),
                                           caseSensitive ? QRegularExpression::NoPatternOption :
                                                         QRegularExpression::CaseInsensitiveOption);
                    return regex.match(QString::fromStdString(text)).hasMatch();
                } catch (const std::exception&) {
                    return false;
                }

            case SearchAlgorithm::WILDCARD_MATCH:
                return findWildcardMatch(caseSensitive ? text : toLower(text),
                                       caseSensitive ? pattern : toLower(pattern)) != std::string::npos;

            default:
                return text.find(pattern) != std::string::npos;
        }
    }

    void deduplicateResults(std::vector<SearchResult>& results) {
        std::unordered_map<std::string, SearchResult> uniqueResults;

        for (const auto& result : results) {
            std::string key = result.objectId;
            if (uniqueResults.find(key) == uniqueResults.end()) {
                uniqueResults[key] = result;
            } else {
                // Keep the result with higher relevance score
                if (result.relevanceScore > uniqueResults[key].relevanceScore) {
                    uniqueResults[key] = result;
                }
            }
        }

        results.clear();
        for (const auto& [key, result] : uniqueResults) {
            results.push_back(result);
        }
    }

    void rankResults(std::vector<SearchResult>& results, const SearchQuery& query) {
        std::sort(results.begin(), results.end(),
                 [this, query](const SearchResult& a, const SearchResult& b) {
                     // Primary sort by relevance score
                     if (std::abs(a.relevanceScore - b.relevanceScore) > 0.001) {
                         return a.relevanceScore > b.relevanceScore;
                     }

                     // Secondary sort by match position
                     if (a.matchPosition != b.matchPosition) {
                         return a.matchPosition < b.matchPosition;
                     }

                     // Tertiary sort by object name
                     return a.objectName < b.objectName;
                 });
    }

    void filterResults(std::vector<SearchResult>& results, const SearchQuery& query) {
        // Apply scope filtering
        results.erase(std::remove_if(results.begin(), results.end(),
                      [this, query](const SearchResult& result) {
                          return !matchesSearchScope(result, query);
                      }), results.end());

        // Apply score filtering
        results.erase(std::remove_if(results.begin(), results.end(),
                      [query](const SearchResult& result) {
                          return result.relevanceScore < query.minScore;
                      }), results.end());
    }

    bool matchesSearchScope(const SearchResult& result, const SearchQuery& query) {
        // This would check against the actual scope (current connection, schema, etc.)
        // For now, always return true
        return true;
    }

    std::vector<std::string> tokenizeContent(const std::string& content) {
        std::vector<std::string> tokens;
        std::stringstream ss(content);
        std::string token;

        while (ss >> token) {
            // Remove punctuation and convert to lowercase
            token.erase(std::remove_if(token.begin(), token.end(),
                                     [](char c) { return !std::isalnum(c); }), token.end());
            if (!token.empty()) {
                tokens.push_back(toLower(token));
            }
        }

        return tokens;
    }

    std::vector<std::string> stemTokens(const std::vector<std::string>& tokens) {
        // Simple stemming implementation
        std::vector<std::string> stemmed;
        for (const auto& token : tokens) {
            std::string stem = token;
            // Remove common suffixes
            if (stem.size() > 3) {
                if (stem.substr(stem.size() - 3) == "ing") {
                    stem = stem.substr(0, stem.size() - 3);
                } else if (stem.substr(stem.size() - 2) == "ed") {
                    stem = stem.substr(0, stem.size() - 2);
                } else if (stem.substr(stem.size() - 1) == "s") {
                    stem = stem.substr(0, stem.size() - 1);
                }
            }
            stemmed.push_back(stem);
        }
        return stemmed;
    }

    std::vector<std::string> removeStopWords(const std::vector<std::string>& tokens) {
        static const std::unordered_set<std::string> stopWords = {
            "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by",
            "is", "are", "was", "were", "be", "been", "being", "have", "has", "had", "do", "does",
            "did", "will", "would", "could", "should", "may", "might", "must", "can"
        };

        std::vector<std::string> filtered;
        for (const auto& token : tokens) {
            if (stopWords.find(token) == stopWords.end()) {
                filtered.push_back(token);
            }
        }
        return filtered;
    }

    int levenshteinDistance(const std::string& s1, const std::string& s2) {
        const int m = s1.size();
        const int n = s2.size();

        if (m == 0) return n;
        if (n == 0) return m;

        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));

        for (int i = 0; i <= m; ++i) dp[i][0] = i;
        for (int j = 0; j <= n; ++j) dp[0][j] = j;

        for (int i = 1; i <= m; ++i) {
            for (int j = 1; j <= n; ++j) {
                int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
                dp[i][j] = std::min({
                    dp[i-1][j] + 1,      // deletion
                    dp[i][j-1] + 1,      // insertion
                    dp[i-1][j-1] + cost  // substitution
                });
            }
        }

        return dp[m][n];
    }

    std::string generateObjectId(const SchemaObject& object) {
        return object.database + "." + object.schema + "." + object.name + "." +
               std::to_string(static_cast<int>(object.type));
    }

    double calculateBaseScore(const SchemaObject& object, const SearchQuery& query) {
        // Base score calculation - would be more sophisticated in real implementation
        return 1.0;
    }

    double calculateFieldBoost(const SchemaObject& object, const SearchQuery& query) {
        // Field importance boost - would be configurable
        return 1.0;
    }

    double calculateTypeBoost(const SchemaObject& object, const SearchQuery& query) {
        // Object type relevance boost
        switch (object.type) {
            case SchemaObjectType::TABLE: return 1.5;
            case SchemaObjectType::VIEW: return 1.3;
            case SchemaObjectType::COLUMN: return 1.0;
            case SchemaObjectType::INDEX: return 1.2;
            case SchemaObjectType::CONSTRAINT: return 1.1;
            default: return 1.0;
        }
    }

    double calculateRecencyBoost(const SchemaObject& object) {
        // Boost recently accessed objects
        return 1.0;
    }

    std::string normalizePattern(const std::string& pattern, SearchAlgorithm algorithm) {
        std::string normalized = pattern;

        // Remove extra whitespace
        normalized.erase(std::unique(normalized.begin(), normalized.end(),
                                   [](char a, char b) { return a == ' ' && b == ' '; }),
                        normalized.end());

        // Trim whitespace
        normalized.erase(normalized.begin(), std::find_if(normalized.begin(), normalized.end(),
                                                        [](char c) { return !std::isspace(c); }));
        normalized.erase(std::find_if(normalized.rbegin(), normalized.rend(),
                                    [](char c) { return !std::isspace(c); }).base(), normalized.end());

        return normalized;
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string toString(SchemaObjectType type) {
        switch (type) {
            case SchemaObjectType::SCHEMA: return "schema";
            case SchemaObjectType::TABLE: return "table";
            case SchemaObjectType::VIEW: return "view";
            case SchemaObjectType::COLUMN: return "column";
            case SchemaObjectType::INDEX: return "index";
            case SchemaObjectType::CONSTRAINT: return "constraint";
            case SchemaObjectType::TRIGGER: return "trigger";
            case SchemaObjectType::FUNCTION: return "function";
            case SchemaObjectType::PROCEDURE: return "procedure";
            case SchemaObjectType::SEQUENCE: return "sequence";
            case SchemaObjectType::DOMAIN: return "domain";
            case SchemaObjectType::TYPE: return "type";
            case SchemaObjectType::RULE: return "rule";
            default: return "unknown";
        }
    }

    std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Placeholder implementations for different search algorithms
    std::vector<SearchResult> exactSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                        const SearchQuery& query, IndexType type) {
        return performDirectSearch(query); // Fallback to direct search
    }

    std::vector<SearchResult> prefixSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                         const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> substringSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                            const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> fuzzySearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                        const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> regexSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                        const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> wildcardSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                           const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> phoneticSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                           const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }

    std::vector<SearchResult> semanticSearch(const SearchIndex& index, const std::vector<std::string>& tokens,
                                           const SearchQuery& query, IndexType type) {
        return performDirectSearch(query);
    }
};

// SearchEngine implementation

SearchEngine::SearchEngine(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>()) {

    // Set default configuration
    config_.enableIndexing = true;
    config_.enableBackgroundIndexing = true;
    config_.indexUpdateIntervalSeconds = 300;
    config_.maxIndexSize = 1000000;
    config_.enableSuggestions = true;
    config_.maxSuggestions = 10;
    config_.enableHistory = true;
    config_.maxHistorySize = 100;
    config_.enableStemming = false;
    config_.enableStopWords = true;
    config_.enabledAlgorithms = {SearchAlgorithm::SUBSTRING_MATCH, SearchAlgorithm::EXACT_MATCH,
                               SearchAlgorithm::FUZZY_MATCH};
    config_.enabledIndexTypes = {IndexType::INVERTED_INDEX};

    // Initialize index timer
    indexTimer_ = new QTimer(this);
    connect(indexTimer_, &QTimer::timeout, this, &SearchEngine::onIndexTimer);
}

SearchEngine::~SearchEngine() = default;

void SearchEngine::initialize(const SearchConfiguration& config) {
    config_ = config;

    if (config_.enableBackgroundIndexing) {
        indexTimer_->start(config_.indexUpdateIntervalSeconds * 1000);
    }
}

void SearchEngine::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    impl_->metadataManager_ = metadataManager;
}

void SearchEngine::setTreeModel(std::shared_ptr<ITreeModel> treeModel) {
    impl_->treeModel_ = treeModel;
}

void SearchEngine::setCacheManager(std::shared_ptr<ICacheManager> cacheManager) {
    impl_->cacheManager_ = cacheManager;
}

std::vector<SearchResult> SearchEngine::search(const SearchQuery& query) {
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        auto results = impl_->performSearch(query);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Update search history
        if (config_.enableHistory) {
            impl_->searchHistory_.push_front(query.pattern);
            if (impl_->searchHistory_.size() > static_cast<size_t>(config_.maxHistorySize)) {
                impl_->searchHistory_.resize(config_.maxHistorySize);
            }
        }

        // Notify completion callback
        if (searchCompletedCallback_) {
            searchCompletedCallback_(results, true);
        }

        return results;

    } catch (const std::exception& e) {
        qWarning() << "Search error:" << e.what();

        if (searchCompletedCallback_) {
            searchCompletedCallback_({}, false);
        }

        return {};
    }
}

void SearchEngine::searchAsync(const SearchQuery& query) {
    if (searchInProgress_) {
        return; // Already searching
    }

    searchInProgress_ = true;
    currentSearchResults_.clear();
    currentSearchSuccess_ = false;

    // Start async search
    std::thread([this, query]() {
        try {
            currentSearchResults_ = search(query);
            currentSearchSuccess_ = true;
        } catch (const std::exception& e) {
            qWarning() << "Async search error:" << e.what();
            currentSearchSuccess_ = false;
        }

        searchInProgress_ = false;
        emit searchCompleted();
    }).detach();
}

void SearchEngine::buildIndex(IndexType type) {
    switch (type) {
        case IndexType::INVERTED_INDEX:
            impl_->buildInvertedIndex();
            break;
        case IndexType::TRIE_INDEX:
            impl_->buildTrieIndex();
            break;
        case IndexType::HASH_INDEX:
            impl_->buildHashIndex();
            break;
        case IndexType::SUFFIX_ARRAY:
            impl_->buildSuffixArrayIndex();
            break;
        case IndexType::FULL_TEXT_INDEX:
            impl_->buildFullTextIndex();
            break;
        case IndexType::VECTOR_INDEX:
            impl_->buildVectorIndex();
            break;
    }
}

void SearchEngine::rebuildIndex(IndexType type) {
    clearIndex(type);
    buildIndex(type);
}

void SearchEngine::clearIndex(IndexType type) {
    impl_->searchIndexes_.erase(type);
}

std::vector<SearchIndex> SearchEngine::getIndexInfo() const {
    std::vector<SearchIndex> indexes;
    for (const auto& [type, index] : impl_->searchIndexes_) {
        indexes.push_back(index);
    }
    return indexes;
}

std::vector<SearchSuggestion> SearchEngine::getSuggestions(const std::string& partialQuery, int maxSuggestions) {
    return impl_->generateSuggestions(partialQuery, maxSuggestions);
}

std::vector<std::string> SearchEngine::getSearchHistory() const {
    return impl_->searchHistory_;
}

void SearchEngine::clearSearchHistory() {
    impl_->searchHistory_.clear();
}

void SearchEngine::addToIndex(const std::string& objectId, const std::string& content,
                             SearchField field, IndexType type) {
    impl_->indexObject(SchemaObject(), type); // Placeholder - would need full object
}

void SearchEngine::removeFromIndex(const std::string& objectId, IndexType type) {
    // Implementation would remove object from specific index
}

void SearchEngine::updateIndex(const std::string& objectId, const std::string& oldContent,
                              const std::string& newContent, SearchField field, IndexType type) {
    // Implementation would update object in specific index
}

SearchConfiguration SearchEngine::getConfiguration() const {
    return config_;
}

void SearchEngine::updateConfiguration(const SearchConfiguration& config) {
    config_ = config;

    if (config_.enableBackgroundIndexing) {
        indexTimer_->start(config_.indexUpdateIntervalSeconds * 1000);
    } else {
        indexTimer_->stop();
    }
}

void SearchEngine::setSearchCompletedCallback(SearchCompletedCallback callback) {
    searchCompletedCallback_ = callback;
}

void SearchEngine::setIndexProgressCallback(IndexProgressCallback callback) {
    indexProgressCallback_ = callback;
}

// Private slot implementations

void SearchEngine::onIndexTimer() {
    // Perform background index maintenance
    for (auto indexType : config_.enabledIndexTypes) {
        buildIndex(indexType);
    }
}

void SearchEngine::onSearchCompleted() {
    // Handle async search completion
    if (searchCompletedCallback_) {
        searchCompletedCallback_(currentSearchResults_, currentSearchSuccess_);
    }
}

} // namespace scratchrobin
