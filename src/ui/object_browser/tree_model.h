#ifndef SCRATCHROBIN_TREE_MODEL_H
#define SCRATCHROBIN_TREE_MODEL_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QIcon>
#include <QFont>
#include <QBrush>
#include <QDateTime>
#include <QTimer>
#include "metadata/schema_collector.h"

namespace scratchrobin {

class IMetadataManager;
class IConnectionManager;

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
    FUNCTION,
    PROCEDURE,
    TRIGGER,
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

struct TreeModelConfiguration {
    bool autoExpand = true;
    bool showSystemObjects = false;
    bool showStatistics = true;
    int maxLoadTime = 30000; // 30 seconds
    int refreshInterval = 300000; // 5 minutes
    int maxHistorySize = 100;
    std::string defaultConnection;
};

struct TreeStatistics {
    std::unordered_map<std::string, int> objectCounts;
    std::unordered_map<std::string, std::chrono::milliseconds> loadTimes;
    QDateTime lastRefresh;
    int totalNodes = 0;
    int loadedNodes = 0;
    int visibleNodes = 0;
    int expandedNodes = 0;
    int loadingNodes = 0;
    int errorNodes = 0;
    QDateTime lastUpdated;
};

struct TreeFilter {
    std::string pattern;
    bool caseSensitive = false;
    bool showOnlyMatching = false;
    bool regex = false;
    bool showSystemObjects = true;
    bool showTemporaryObjects = true;
    std::vector<TreeNodeType> nodeTypes;
    std::vector<SchemaObjectType> schemaTypes;
};

struct TreeNode {
    std::string id;
    std::string name;
    std::string displayName;
    std::string connectionId;
    TreeNodeType type;
    SchemaObjectType schemaType;
    std::shared_ptr<TreeNode> parent;
    std::vector<std::shared_ptr<TreeNode>> children;
    bool isExpandable = false;
    bool isExpanded = false;
    NodeLoadState loadState = NodeLoadState::NOT_LOADED;
    QString statusMessage;
    QIcon icon;
    QFont font;
    QBrush background;
    QBrush foreground;
    QString tooltip;
    QVariant userData;
    bool isVisible = true;
    bool isFiltered = false;
};

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    // QAbstractItemModel implementation
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Tree model specific methods
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager);
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager);

    void refresh();
    void refreshNode(const QModelIndex& index);
    void expandNode(const QModelIndex& index);
    void collapseNode(const QModelIndex& index);

    QModelIndex findNode(const std::string& nodeId) const;
    std::shared_ptr<TreeNode> getNode(const QModelIndex& index) const;
    std::shared_ptr<TreeNode> getRootNode() const;

    // Node operations
    bool addConnectionNode(const std::string& connectionId, const std::string& connectionName);
    bool removeConnectionNode(const std::string& connectionId);
    bool updateConnectionNode(const std::string& connectionId, const std::string& newName);

    // Additional methods needed by implementation
    void addConnection(const std::string& connectionId, const std::string& connectionName);
    void removeConnection(const std::string& connectionId);
    void refreshConnection(const std::string& connectionId);
    void applyFilter(const std::string& filter);
    void clearFilter();
    std::vector<std::string> getMatchingNodes(const std::string& pattern) const;
    QModelIndex getIndex(const std::shared_ptr<TreeNode>& node) const;

    // Event callbacks
    using NodeExpandedCallback = std::function<void(const QModelIndex&, const std::shared_ptr<TreeNode>&)>;
    using NodeCollapsedCallback = std::function<void(const QModelIndex&, const std::shared_ptr<TreeNode>&)>;
    using NodeSelectedCallback = std::function<void(const QModelIndex&, const std::shared_ptr<TreeNode>&)>;
    using NodeLoadedCallback = std::function<void(const QModelIndex&, const std::shared_ptr<TreeNode>&)>;
    using StatisticsChangedCallback = std::function<void(const TreeStatistics&)>;

    // Missing method declarations
    TreeStatistics getStatistics() const;
    TreeModelConfiguration getConfiguration() const;
    void updateConfiguration(const TreeModelConfiguration& config);
    std::string toString(TreeNodeType type) const;
    void applyFilter(const TreeFilter& filter);
    NodeLoadedCallback getNodeLoadedCallback() const;
    StatisticsChangedCallback getStatisticsChangedCallback() const;
    void setNodeLoadedCallback(NodeLoadedCallback callback);
    void setStatisticsChangedCallback(StatisticsChangedCallback callback);

    void setNodeExpandedCallback(NodeExpandedCallback callback);
    void setNodeCollapsedCallback(NodeCollapsedCallback callback);
    void setNodeSelectedCallback(NodeSelectedCallback callback);

    void onLoadTimeout();
    void onRefreshTimer();
    std::string toString(TreeNodeType type);

signals:
    void nodeExpanded(const QModelIndex& index, const std::shared_ptr<TreeNode>& node);
    void nodeCollapsed(const QModelIndex& index, const std::shared_ptr<TreeNode>& node);
    void nodeSelected(const QModelIndex& index, const std::shared_ptr<TreeNode>& node);
    void modelRefreshed();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Helper methods
    QModelIndex createIndexFromNode(const std::shared_ptr<TreeNode>& node) const;
    std::shared_ptr<TreeNode> getNodeFromIndex(const QModelIndex& index) const;
    void populateChildren(const std::shared_ptr<TreeNode>& node);
    void setupNodeProperties(const std::shared_ptr<TreeNode>& node);

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Callbacks
    NodeExpandedCallback nodeExpandedCallback_;
    NodeCollapsedCallback nodeCollapsedCallback_;
    NodeSelectedCallback nodeSelectedCallback_;

    // Disable copy and assignment
    TreeModel(const TreeModel&) = delete;
    TreeModel& operator=(const TreeModel&) = delete;

protected:
    std::shared_ptr<TreeNode> rootNode_;
    TreeFilter activeFilter_;
    TreeStatistics statistics_;
    TreeModelConfiguration config_;
    std::vector<std::string> matchingNodes_;
    QTimer* loadTimer_;
    QTimer* refreshTimer_;
};

// Tree item roles for custom data
enum TreeItemRole {
    NodeTypeRole = Qt::UserRole + 1,
    SchemaTypeRole,
    NodeIdRole,
    NodeDataRole,
    IsExpandableRole,
    TooltipRole
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TREE_MODEL_H
