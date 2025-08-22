# Phase 3.2: Object Browser UI Implementation

## Overview
This phase implements a comprehensive object browser user interface for ScratchRobin, providing intuitive database object navigation, search capabilities, and context-sensitive operations with seamless integration to the metadata loading system.

## Prerequisites
- ✅ Phase 1.1 (Project Setup) completed
- ✅ Phase 1.2 (Architecture Design) completed
- ✅ Phase 1.3 (Dependency Management) completed
- ✅ Phase 2.1 (Connection Pooling) completed
- ✅ Phase 2.2 (Authentication System) completed
- ✅ Phase 2.3 (SSL/TLS Integration) completed
- ✅ Phase 3.1 (Metadata Loader) completed
- Metadata loading system from Phase 3.1
- UI framework components from earlier phases

## Implementation Tasks

### Task 3.2.1: Tree Navigation Component

#### 3.2.1.1: Database Object Tree Model
**Objective**: Create a hierarchical tree model representing database objects with metadata integration

**Implementation Steps:**
1. Implement tree node data structure with metadata integration
2. Create tree model with lazy loading and caching
3. Add object type-specific icons and styling
4. Implement tree node expansion and collapse with progress indication

**Code Changes:**

**File: src/ui/object_browser/tree_model.h**
```cpp
#ifndef SCRATCHROBIN_TREE_MODEL_H
#define SCRATCHROBIN_TREE_MODEL_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <QAbstractItemModel>
#include <QIcon>
#include <QFont>
#include <QBrush>
#include <QTimer>
#include <mutex>

namespace scratchrobin {

class IMetadataManager;
class IConnection;

enum class TreeNodeType {
    ROOT,
    CONNECTION,
    DATABASE,
    SCHEMA,
    TABLE,
    VIEW,
    COLUMN,
    INDEX,
    CONSTRAINT,
    TRIGGER,
    FUNCTION,
    PROCEDURE,
    SEQUENCE,
    DOMAIN,
    TYPE,
    RULE
};

enum class NodeLoadState {
    NOT_LOADED,
    LOADING,
    LOADED,
    ERROR
};

struct TreeNode {
    std::string id;
    std::string name;
    std::string displayName;
    TreeNodeType type;
    SchemaObjectType schemaType;
    std::string schema;
    std::string database;
    std::string connectionId;
    bool isExpandable = false;
    bool isExpanded = false;
    NodeLoadState loadState = NodeLoadState::NOT_LOADED;
    std::chrono::system_clock::time_point lastLoaded;
    std::vector<std::shared_ptr<TreeNode>> children;
    std::weak_ptr<TreeNode> parent;
    std::unordered_map<std::string, std::string> properties;
    QIcon icon;
    QFont font;
    QBrush background;
    bool isVisible = true;
    bool isFiltered = false;
    std::string tooltip;
    std::string statusMessage;
};

struct TreeModelConfiguration {
    bool showSystemObjects = false;
    bool showTemporaryObjects = false;
    bool groupBySchema = true;
    bool groupByType = false;
    bool showCounts = true;
    bool enableLazyLoading = true;
    int maxChildrenPerNode = 1000;
    int loadTimeoutSeconds = 30;
    bool enableAutoRefresh = true;
    int refreshIntervalSeconds = 300;
    std::vector<TreeNodeType> visibleNodeTypes;
    std::unordered_map<TreeNodeType, QIcon> nodeIcons;
    std::unordered_map<TreeNodeType, QColor> nodeColors;
};

struct TreeFilter {
    std::string pattern;
    bool caseSensitive = false;
    bool regex = false;
    std::vector<TreeNodeType> nodeTypes;
    std::vector<std::string> schemas;
    bool showOnlyMatching = false;
    bool expandMatching = true;
    int maxMatches = 1000;
};

struct TreeStatistics {
    int totalNodes = 0;
    int visibleNodes = 0;
    int expandedNodes = 0;
    int loadingNodes = 0;
    int errorNodes = 0;
    std::chrono::milliseconds averageLoadTime{0};
    std::chrono::system_clock::time_point lastUpdated;
};

class ITreeModel {
public:
    virtual ~ITreeModel() = default;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const = 0;
    virtual QModelIndex parent(const QModelIndex& child) const = 0;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const = 0;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const = 0;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const = 0;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const = 0;
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const = 0;
    virtual bool canFetchMore(const QModelIndex& parent) const = 0;
    virtual void fetchMore(const QModelIndex& parent) = 0;

    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void addConnection(const std::string& connectionId, const std::string& connectionName) = 0;
    virtual void removeConnection(const std::string& connectionId) = 0;
    virtual void refreshConnection(const std::string& connectionId) = 0;

    virtual void applyFilter(const TreeFilter& filter) = 0;
    virtual void clearFilter() = 0;
    virtual std::vector<std::string> getMatchingNodes(const std::string& pattern) const = 0;

    virtual std::shared_ptr<TreeNode> getNode(const QModelIndex& index) const = 0;
    virtual QModelIndex getIndex(const std::shared_ptr<TreeNode>& node) const = 0;

    virtual void expandNode(const QModelIndex& index) = 0;
    virtual void collapseNode(const QModelIndex& index) = 0;

    virtual TreeStatistics getStatistics() const = 0;
    virtual TreeModelConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const TreeModelConfiguration& config) = 0;

    using NodeLoadedCallback = std::function<void(const std::string&, bool)>;
    using StatisticsChangedCallback = std::function<void(const TreeStatistics&)>;

    virtual void setNodeLoadedCallback(NodeLoadedCallback callback) = 0;
    virtual void setStatisticsChangedCallback(StatisticsChangedCallback callback) = 0;
};

class TreeModel : public QAbstractItemModel, public ITreeModel {
    Q_OBJECT

public:
    TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    // QAbstractItemModel implementation
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // ITreeModel implementation
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void addConnection(const std::string& connectionId, const std::string& connectionName) override;
    void removeConnection(const std::string& connectionId) override;
    void refreshConnection(const std::string& connectionId) override;

    void applyFilter(const TreeFilter& filter) override;
    void clearFilter() override;
    std::vector<std::string> getMatchingNodes(const std::string& pattern) const override;

    std::shared_ptr<TreeNode> getNode(const QModelIndex& index) const override;
    QModelIndex getIndex(const std::shared_ptr<TreeNode>& node) const override;

    void expandNode(const QModelIndex& index) override;
    void collapseNode(const QModelIndex& index) override;

    TreeStatistics getStatistics() const override;
    TreeModelConfiguration getConfiguration() const override;
    void updateConfiguration(const TreeModelConfiguration& config) override;

    void setNodeLoadedCallback(NodeLoadedCallback callback) override;
    void setStatisticsChangedCallback(StatisticsChangedCallback callback) override;

private slots:
    void onLoadTimeout();
    void onRefreshTimer();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;

    // Tree structure
    std::shared_ptr<TreeNode> rootNode_;
    std::unordered_map<std::string, std::shared_ptr<TreeNode>> nodeMap_;
    std::unordered_map<std::shared_ptr<TreeNode>, QModelIndex> indexMap_;

    // Configuration and state
    TreeModelConfiguration config_;
    TreeFilter activeFilter_;
    TreeStatistics statistics_;

    // Callbacks
    NodeLoadedCallback nodeLoadedCallback_;
    StatisticsChangedCallback statisticsChangedCallback_;

    // Timers
    QTimer* loadTimer_;
    QTimer* refreshTimer_;

    // Internal methods
    void initializeRootNode();
    void createConnectionNode(const std::string& connectionId, const std::string& connectionName);
    void loadNodeChildren(const std::shared_ptr<TreeNode>& node);
    void loadSchemaObjects(const std::shared_ptr<TreeNode>& parentNode);
    void loadTableObjects(const std::shared_ptr<TreeNode>& parentNode);
    void loadColumnObjects(const std::shared_ptr<TreeNode>& parentNode);

    std::shared_ptr<TreeNode> createNode(const std::string& id, const std::string& name,
                                       TreeNodeType type, SchemaObjectType schemaType = SchemaObjectType::TABLE,
                                       const std::shared_ptr<TreeNode>& parent = nullptr);

    void setupNodeProperties(const std::shared_ptr<TreeNode>& node);
    QIcon getNodeIcon(TreeNodeType type) const;
    QColor getNodeColor(TreeNodeType type) const;

    void applyFilterToNode(const std::shared_ptr<TreeNode>& node, const TreeFilter& filter);
    bool nodeMatchesFilter(const std::shared_ptr<TreeNode>& node, const TreeFilter& filter);

    void updateStatistics();
    void notifyStatisticsChanged();

    void handleLoadError(const std::shared_ptr<TreeNode>& node, const std::string& error);
    void handleLoadSuccess(const std::shared_ptr<TreeNode>& node);

    // Async loading management
    std::unordered_map<std::string, std::future<void>> activeLoads_;
    mutable std::mutex modelMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TREE_MODEL_H
```

#### 3.2.1.2: Object Browser Widget
**Objective**: Create the main object browser widget with tree view, search, and filtering capabilities

**Implementation Steps:**
1. Implement main object browser widget with Qt integration
2. Add search and filter controls
3. Create context menus with object-specific actions
4. Implement drag-and-drop functionality for objects

**Code Changes:**

**File: src/ui/object_browser/object_browser.h**
```cpp
#ifndef SCRATCHROBIN_OBJECT_BROWSER_H
#define SCRATCHROBIN_OBJECT_BROWSER_H

#include <memory>
#include <string>
#include <vector>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QSplitter>

namespace scratchrobin {

class ITreeModel;
class IConnectionManager;
class IMetadataManager;

enum class BrowserViewMode {
    TREE,
    FLAT,
    CATEGORY
};

enum class ObjectAction {
    SELECT,
    EDIT,
    DROP,
    CREATE,
    ALTER,
    VIEW_DATA,
    VIEW_PROPERTIES,
    VIEW_DEPENDENCIES,
    VIEW_DEPENDENTS,
    GENERATE_SCRIPT,
    EXPORT,
    IMPORT,
    REFRESH,
    COPY_NAME,
    COPY_DDL,
    ANALYZE,
    OPTIMIZE,
    REINDEX,
    VACUUM
};

struct BrowserConfiguration {
    BrowserViewMode defaultViewMode = BrowserViewMode::TREE;
    bool showToolbar = true;
    bool showStatusBar = true;
    bool showSearchBox = true;
    bool enableDragDrop = true;
    bool enableContextMenus = true;
    bool autoExpandNewNodes = false;
    int defaultTreeIndentation = 20;
    bool showLineNumbers = false;
    bool enableAnimations = true;
    std::vector<ObjectAction> enabledActions;
};

struct SearchOptions {
    std::string pattern;
    bool caseSensitive = false;
    bool regex = false;
    bool searchInNames = true;
    bool searchInProperties = false;
    bool searchInComments = true;
    std::vector<TreeNodeType> searchTypes;
    int maxResults = 1000;
};

class IObjectBrowser {
public:
    virtual ~IObjectBrowser() = default;

    virtual void initialize(const BrowserConfiguration& config) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;

    virtual void addConnection(const std::string& connectionId, const std::string& connectionName) = 0;
    virtual void removeConnection(const std::string& connectionId) = 0;
    virtual void selectConnection(const std::string& connectionId) = 0;
    virtual std::string getSelectedConnection() const = 0;

    virtual void search(const SearchOptions& options) = 0;
    virtual void clearSearch() = 0;
    virtual std::vector<std::string> getSearchResults() const = 0;

    virtual void refreshView() = 0;
    virtual void expandAll() = 0;
    virtual void collapseAll() = 0;
    virtual void expandNode(const std::string& nodeId) = 0;

    virtual BrowserConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const BrowserConfiguration& config) = 0;

    using SelectionChangedCallback = std::function<void(const std::string&)>;
    using ObjectActionCallback = std::function<void(ObjectAction, const std::string&)>;

    virtual void setSelectionChangedCallback(SelectionChangedCallback callback) = 0;
    virtual void setObjectActionCallback(ObjectActionCallback callback) = 0;

    virtual QWidget* getWidget() = 0;
};

class ObjectBrowser : public QWidget, public IObjectBrowser {
    Q_OBJECT

public:
    ObjectBrowser(QWidget* parent = nullptr);
    ~ObjectBrowser() override;

    // IObjectBrowser implementation
    void initialize(const BrowserConfiguration& config) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;

    void addConnection(const std::string& connectionId, const std::string& connectionName) override;
    void removeConnection(const std::string& connectionId) override;
    void selectConnection(const std::string& connectionId) override;
    std::string getSelectedConnection() const override;

    void search(const SearchOptions& options) override;
    void clearSearch() override;
    std::vector<std::string> getSearchResults() const override;

    void refreshView() override;
    void expandAll() override;
    void collapseAll() override;
    void expandNode(const std::string& nodeId) override;

    BrowserConfiguration getConfiguration() const override;
    void updateConfiguration(const BrowserConfiguration& config) override;

    void setSelectionChangedCallback(SelectionChangedCallback callback) override;
    void setObjectActionCallback(ObjectActionCallback callback) override;

    QWidget* getWidget() override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchButtonClicked();
    void onClearSearchButtonClicked();
    void onTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onTreeDoubleClicked(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& pos);
    void onRefreshButtonClicked();
    void onExpandAllButtonClicked();
    void onCollapseAllButtonClicked();
    void onViewModeChanged(int index);
    void onFilterChanged();
    void onLoadProgress(int current, int total);
    void onLoadCompleted(bool success);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // UI Components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* toolbarLayout_;
    QSplitter* splitter_;

    // Toolbar
    QLineEdit* searchBox_;
    QPushButton* searchButton_;
    QPushButton* clearSearchButton_;
    QComboBox* viewModeCombo_;
    QPushButton* refreshButton_;
    QPushButton* expandAllButton_;
    QPushButton* collapseAllButton_;

    // Filter controls
    QCheckBox* showSystemObjectsCheck_;
    QCheckBox* showTemporaryObjectsCheck_;
    QComboBox* filterTypeCombo_;

    // Tree view
    QTreeView* treeView_;
    std::shared_ptr<ITreeModel> treeModel_;

    // Status and progress
    QStatusBar* statusBar_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    // Context menu
    QMenu* contextMenu_;

    // Configuration and state
    BrowserConfiguration config_;
    SearchOptions currentSearch_;
    std::string selectedConnection_;

    // Callbacks
    SelectionChangedCallback selectionChangedCallback_;
    ObjectActionCallback objectActionCallback_;

    // Core components
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;

    // Internal methods
    void setupUI();
    void setupToolbar();
    void setupTreeView();
    void setupStatusBar();
    void setupContextMenu();
    void setupConnections();

    void createContextMenuActions();
    void updateContextMenu(const std::string& nodeId);
    void executeObjectAction(ObjectAction action, const std::string& nodeId);

    void updateSearchResults();
    void updateStatusBar();
    void showProgress(bool visible, const std::string& message = "");

    void applyViewMode(BrowserViewMode mode);
    void applyFilters();

    std::string getSelectedNodeId() const;
    QModelIndex getSelectedIndex() const;

    void handleTreeModelStatisticsChanged(const TreeStatistics& stats);

    // Drag and drop support
    void setupDragAndDrop();
    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList& indexes) const;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_OBJECT_BROWSER_H
```

### Task 3.2.2: Search and Filter System

#### 3.2.2.1: Advanced Search Engine
**Objective**: Implement a powerful search system with pattern matching, fuzzy search, and indexing

**Implementation Steps:**
1. Create search indexer with background indexing
2. Implement multiple search algorithms (exact, fuzzy, regex)
3. Add search result ranking and scoring
4. Create search history and suggestions

**Code Changes:**

**File: src/ui/object_browser/search_engine.h**
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

class ITreeModel;
class IMetadataManager;

enum class SearchAlgorithm {
    EXACT_MATCH,
    PREFIX_MATCH,
    SUBSTRING_MATCH,
    FUZZY_MATCH,
    REGEX_MATCH,
    WILDCARD_MATCH
};

enum class SearchScope {
    ALL,
    CURRENT_CONNECTION,
    CURRENT_SCHEMA,
    SELECTED_NODES,
    RECENT_NODES
};

enum class SearchField {
    NAME,
    TYPE,
    SCHEMA,
    PROPERTIES,
    COMMENTS,
    ALL
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
};

struct SearchResult {
    std::string nodeId;
    std::string nodeName;
    TreeNodeType nodeType;
    std::string schema;
    std::string database;
    std::string connectionId;
    SearchField matchedField;
    std::string matchedText;
    int matchScore = 0;
    int matchPosition = 0;
    std::vector<std::string> contextLines;
    std::chrono::system_clock::time_point foundAt;
};

struct SearchIndex {
    std::unordered_map<std::string, std::vector<std::string>> nameIndex;
    std::unordered_map<std::string, std::vector<std::string>> typeIndex;
    std::unordered_map<std::string, std::vector<std::string>> schemaIndex;
    std::unordered_map<std::string, std::vector<std::string>> propertyIndex;
    std::unordered_map<std::string, std::vector<std::string>> commentIndex;
    std::chrono::system_clock::time_point builtAt;
    int totalEntries = 0;
    std::string checksum;
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
    std::vector<SearchAlgorithm> enabledAlgorithms;
    std::unordered_map<SearchAlgorithm, int> algorithmWeights;
};

class ISearchEngine {
public:
    virtual ~ISearchEngine() = default;

    virtual void initialize(const SearchConfiguration& config) = 0;
    virtual void setTreeModel(std::shared_ptr<ITreeModel> treeModel) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;

    virtual std::vector<SearchResult> search(const SearchQuery& query) = 0;
    virtual void searchAsync(const SearchQuery& query) = 0;

    virtual void buildIndex() = 0;
    virtual void rebuildIndex() = 0;
    virtual void clearIndex() = 0;
    virtual SearchIndex getIndexInfo() const = 0;

    virtual std::vector<std::string> getSuggestions(const std::string& partialQuery, int maxSuggestions = 10) = 0;
    virtual std::vector<std::string> getSearchHistory() const = 0;
    virtual void clearSearchHistory() = 0;

    virtual void addToIndex(const std::string& nodeId, const std::string& content, SearchField field) = 0;
    virtual void removeFromIndex(const std::string& nodeId) = 0;
    virtual void updateIndex(const std::string& nodeId, const std::string& oldContent, const std::string& newContent, SearchField field) = 0;

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
    void setTreeModel(std::shared_ptr<ITreeModel> treeModel) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;

    std::vector<SearchResult> search(const SearchQuery& query) override;
    void searchAsync(const SearchQuery& query) override;

    void buildIndex() override;
    void rebuildIndex() override;
    void clearIndex() override;
    SearchIndex getIndexInfo() const override;

    std::vector<std::string> getSuggestions(const std::string& partialQuery, int maxSuggestions = 10) override;
    std::vector<std::string> getSearchHistory() const override;
    void clearSearchHistory() override;

    void addToIndex(const std::string& nodeId, const std::string& content, SearchField field) override;
    void removeFromIndex(const std::string& nodeId) override;
    void updateIndex(const std::string& nodeId, const std::string& oldContent, const std::string& newContent, SearchField field) override;

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
    std::shared_ptr<ITreeModel> treeModel_;
    std::shared_ptr<IMetadataManager> metadataManager_;

    // Search infrastructure
    SearchIndex searchIndex_;
    SearchConfiguration config_;
    std::deque<std::string> searchHistory_;

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

    void buildIndexInternal();
    void indexNode(const std::string& nodeId);
    void indexObject(const SchemaObject& obj);

    int calculateMatchScore(const SearchResult& result, const SearchQuery& query);
    void rankResults(std::vector<SearchResult>& results);
    void filterResults(std::vector<SearchResult>& results, const SearchQuery& query);

    std::vector<std::string> generateSuggestions(const std::string& partialQuery, int maxSuggestions);

    bool matchesSearchScope(const SearchResult& result, const SearchQuery& query);
    bool matchesSearchFields(const std::string& content, const std::string& pattern,
                           SearchAlgorithm algorithm, bool caseSensitive);

    void updateSearchHistory(const SearchQuery& query);
    void pruneSearchHistory();

    std::string normalizePattern(const std::string& pattern, SearchAlgorithm algorithm);
    std::string normalizeContent(const std::string& content, bool caseSensitive);

    // Fuzzy search helpers
    int levenshteinDistance(const std::string& s1, const std::string& s2);
    bool fuzzyMatch(const std::string& text, const std::string& pattern, int maxDistance);

    // Thread-safe operations
    mutable std::mutex indexMutex_;
    mutable std::mutex historyMutex_;
    mutable std::mutex searchMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SEARCH_ENGINE_H
```

### Task 3.2.3: Context Menu and Actions

#### 3.2.3.1: Object Context Actions
**Objective**: Implement context-sensitive actions for different object types with proper security integration

**Implementation Steps:**
1. Create action manager with object type-specific actions
2. Implement action execution with progress tracking
3. Add security and permission checking
4. Create action result handling and feedback

**Code Changes:**

**File: src/ui/object_browser/action_manager.h**
```cpp
#ifndef SCRATCHROBIN_ACTION_MANAGER_H
#define SCRATCHROBIN_ACTION_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <QObject>
#include <QAction>
#include <QMenu>
#include <QIcon>

namespace scratchrobin {

class IConnectionManager;
class IMetadataManager;
class ISecurityManager;

enum class ActionCategory {
    VIEW,
    MODIFY,
    CREATE,
    DELETE,
    EXPORT,
    IMPORT,
    ANALYZE,
    MAINTENANCE,
    SECURITY,
    TOOLS
};

enum class ActionPermission {
    NONE,
    READ,
    WRITE,
    EXECUTE,
    ADMIN
};

struct ActionDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::string iconName;
    QKeySequence shortcut;
    ActionCategory category;
    ActionPermission requiredPermission;
    std::vector<TreeNodeType> applicableTypes;
    bool requiresConfirmation = false;
    std::string confirmationMessage;
    bool isEnabled = true;
    std::unordered_map<std::string, std::string> properties;
};

struct ActionContext {
    std::string nodeId;
    std::string nodeName;
    TreeNodeType nodeType;
    SchemaObjectType schemaType;
    std::string schema;
    std::string database;
    std::string connectionId;
    std::unordered_map<std::string, std::string> nodeProperties;
    std::vector<std::string> selectedNodes;
    QPoint mousePosition;
};

struct ActionResult {
    std::string actionId;
    bool success = false;
    std::string message;
    std::string detailedMessage;
    std::vector<std::string> affectedObjects;
    std::chrono::system_clock::time_point executedAt;
    std::chrono::milliseconds executionTime;
    std::unordered_map<std::string, std::string> resultData;
};

struct ActionManagerConfiguration {
    bool enableConfirmationDialogs = true;
    bool enableProgressDialogs = true;
    bool enableUndoActions = false;
    int maxUndoSteps = 10;
    bool enableActionHistory = true;
    int maxHistorySize = 1000;
    std::vector<ActionCategory> enabledCategories;
    std::vector<ActionPermission> availablePermissions;
    bool checkPermissionsBeforeAction = true;
};

class IActionManager {
public:
    virtual ~IActionManager() = default;

    virtual void initialize(const ActionManagerConfiguration& config) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSecurityManager(std::shared_ptr<ISecurityManager> securityManager) = 0;

    virtual std::vector<ActionDefinition> getAvailableActions(const ActionContext& context) = 0;
    virtual bool isActionEnabled(const std::string& actionId, const ActionContext& context) = 0;
    virtual ActionResult executeAction(const std::string& actionId, const ActionContext& context) = 0;
    virtual void executeActionAsync(const std::string& actionId, const ActionContext& context) = 0;

    virtual QMenu* createContextMenu(const ActionContext& context, QWidget* parent = nullptr) = 0;
    virtual std::vector<QAction*> createToolbarActions(const ActionContext& context, QWidget* parent = nullptr) = 0;

    virtual void registerAction(const ActionDefinition& action, const std::function<ActionResult(const ActionContext&)>& handler) = 0;
    virtual void unregisterAction(const std::string& actionId) = 0;

    virtual std::vector<ActionResult> getActionHistory() const = 0;
    virtual void clearActionHistory() = 0;

    virtual ActionManagerConfiguration getConfiguration() const = 0;
    virtual void updateConfiguration(const ActionManagerConfiguration& config) = 0;

    using ActionCompletedCallback = std::function<void(const ActionResult&)>;
    using PermissionCheckCallback = std::function<bool(const std::string&, ActionPermission)>;

    virtual void setActionCompletedCallback(ActionCompletedCallback callback) = 0;
    virtual void setPermissionCheckCallback(PermissionCheckCallback callback) = 0;
};

class ActionManager : public QObject, public IActionManager {
    Q_OBJECT

public:
    ActionManager(QObject* parent = nullptr);
    ~ActionManager() override;

    // IActionManager implementation
    void initialize(const ActionManagerConfiguration& config) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSecurityManager(std::shared_ptr<ISecurityManager> securityManager) override;

    std::vector<ActionDefinition> getAvailableActions(const ActionContext& context) override;
    bool isActionEnabled(const std::string& actionId, const ActionContext& context) override;
    ActionResult executeAction(const std::string& actionId, const ActionContext& context) override;
    void executeActionAsync(const std::string& actionId, const ActionContext& context) override;

    QMenu* createContextMenu(const ActionContext& context, QWidget* parent = nullptr) override;
    std::vector<QAction*> createToolbarActions(const ActionContext& context, QWidget* parent = nullptr) override;

    void registerAction(const ActionDefinition& action, const std::function<ActionResult(const ActionContext&)>& handler) override;
    void unregisterAction(const std::string& actionId) override;

    std::vector<ActionResult> getActionHistory() const override;
    void clearActionHistory() override;

    ActionManagerConfiguration getConfiguration() const override;
    void updateConfiguration(const ActionManagerConfiguration& config) override;

    void setActionCompletedCallback(ActionCompletedCallback callback) override;
    void setPermissionCheckCallback(PermissionCheckCallback callback) override;

private slots:
    void onActionTriggered();
    void onAsyncActionCompleted();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISecurityManager> securityManager_;

    // Action infrastructure
    std::unordered_map<std::string, ActionDefinition> actionDefinitions_;
    std::unordered_map<std::string, std::function<ActionResult(const ActionContext&)>> actionHandlers_;
    std::vector<ActionResult> actionHistory_;
    ActionManagerConfiguration config_;

    // Callbacks
    ActionCompletedCallback actionCompletedCallback_;
    PermissionCheckCallback permissionCheckCallback_;

    // Internal methods
    void initializeDefaultActions();
    void registerViewActions();
    void registerModifyActions();
    void registerCreateActions();
    void registerDeleteActions();
    void registerExportActions();
    void registerImportActions();
    void registerAnalyzeActions();
    void registerMaintenanceActions();
    void registerSecurityActions();
    void registerToolActions();

    bool checkActionPermission(const ActionDefinition& action, const ActionContext& context);
    bool checkActionPrerequisites(const ActionDefinition& action, const ActionContext& context);

    ActionResult executeViewDataAction(const ActionContext& context);
    ActionResult executeEditObjectAction(const ActionContext& context);
    ActionResult executeDropObjectAction(const ActionContext& context);
    ActionResult executeCreateObjectAction(const ActionContext& context);
    ActionResult executeAlterObjectAction(const ActionContext& context);
    ActionResult executeViewPropertiesAction(const ActionContext& context);
    ActionResult executeViewDependenciesAction(const ActionContext& context);
    ActionResult executeViewDependentsAction(const ActionContext& context);
    ActionResult executeGenerateScriptAction(const ActionContext& context);
    ActionResult executeExportAction(const ActionContext& context);
    ActionResult executeImportAction(const ActionContext& context);
    ActionResult executeRefreshAction(const ActionContext& context);
    ActionResult executeCopyNameAction(const ActionContext& context);
    ActionResult executeCopyDdlAction(const ActionContext& context);
    ActionResult executeAnalyzeAction(const ActionContext& context);
    ActionResult executeOptimizeAction(const ActionContext& context);
    ActionResult executeReindexAction(const ActionContext& context);
    ActionResult executeVacuumAction(const ActionContext& context);

    void addActionToHistory(const ActionResult& result);
    void pruneActionHistory();

    QIcon getActionIcon(const ActionDefinition& action);
    QString getActionTooltip(const ActionDefinition& action);

    void showConfirmationDialog(const ActionDefinition& action, const ActionContext& context,
                              const std::function<void()>& onConfirmed);

    void showProgressDialog(const std::string& actionName, const std::function<void()>& action);

    // Async execution management
    struct AsyncAction {
        std::string actionId;
        ActionContext context;
        std::future<ActionResult> future;
    };

    std::unordered_map<std::string, AsyncAction> activeAsyncActions_;
    mutable std::mutex actionMutex_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_ACTION_MANAGER_H
```

## Summary

This phase 3.2 implementation provides comprehensive object browser UI functionality for ScratchRobin with the following key features:

✅ **Tree Navigation Component**: Hierarchical tree model with lazy loading, caching, and metadata integration
✅ **Object Browser Widget**: Main UI widget with search, filtering, context menus, and drag-and-drop
✅ **Advanced Search Engine**: Multi-algorithm search with indexing, fuzzy matching, and suggestions
✅ **Context Menu and Actions**: Context-sensitive actions with security integration and progress tracking
✅ **Professional UI/UX**: Modern Qt-based interface with animations, icons, and status indicators
✅ **Performance Optimized**: Background loading, caching, and efficient memory management
✅ **Enterprise Ready**: Security integration, audit logging, and comprehensive error handling
✅ **Extensible Architecture**: Plugin-based action system and customizable UI components

The object browser provides a professional-grade database navigation experience with rich metadata display, powerful search capabilities, and seamless integration with the underlying metadata management system.

**Next Phase**: Phase 3.3 - Property Viewers Implementation
