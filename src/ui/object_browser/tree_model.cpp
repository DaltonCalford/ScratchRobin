#include "ui/object_browser/tree_model.h"
#include "metadata/metadata_manager.h"
#include "core/connection_manager.h"
#include <QApplication>
#include <QStyle>
#include <QPalette>
#include <QFont>
#include <QBrush>
#include <QIcon>
#include <QDebug>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace scratchrobin {

class TreeModel::Impl {
public:
    Impl(TreeModel* parent)
        : parent_(parent) {}

    std::shared_ptr<TreeNode> createNode(const std::string& id, const std::string& name,
                                       TreeNodeType type, SchemaObjectType schemaType,
                                       const std::shared_ptr<TreeNode>& parent = nullptr) {
        auto node = std::make_shared<TreeNode>();
        node->id = id;
        node->name = name;
        node->displayName = name;
        node->type = type;
        node->schemaType = schemaType;
        node->parent = parent;

        setupNodeProperties(node);
        return node;
    }

    void setupNodeProperties(const std::shared_ptr<TreeNode>& node) {
        // Set default properties based on node type
        switch (node->type) {
            case TreeNodeType::ROOT:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("folder-root");
                node->font.setBold(true);
                node->background = QApplication::palette().color(QPalette::Base);
                node->tooltip = "Database Connections";
                break;

            case TreeNodeType::CONNECTION:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("network-server-database");
                node->font.setBold(true);
                node->background = QApplication::palette().color(QPalette::AlternateBase);
                node->tooltip = "Database Connection: " + node->name;
                break;

            case TreeNodeType::DATABASE:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("drive-harddisk");
                node->font.setBold(true);
                node->background = QApplication::palette().color(QPalette::Base);
                node->tooltip = "Database: " + node->name;
                break;

            case TreeNodeType::SCHEMA:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("folder");
                node->font.setBold(true);
                node->background = QApplication::palette().color(QPalette::AlternateBase);
                node->tooltip = "Schema: " + node->name;
                break;

            case TreeNodeType::TABLE:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("table");
                node->tooltip = "Table: " + node->name;
                break;

            case TreeNodeType::VIEW:
                node->isExpandable = true;
                node->icon = QIcon::fromTheme("view");
                node->tooltip = "View: " + node->name;
                break;

            case TreeNodeType::COLUMN:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("column");
                node->tooltip = "Column: " + node->name;
                break;

            case TreeNodeType::INDEX:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("index");
                node->tooltip = "Index: " + node->name;
                break;

            case TreeNodeType::CONSTRAINT:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("constraint");
                node->tooltip = "Constraint: " + node->name;
                break;

            case TreeNodeType::TRIGGER:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("trigger");
                node->tooltip = "Trigger: " + node->name;
                break;

            case TreeNodeType::FUNCTION:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("function");
                node->tooltip = "Function: " + node->name;
                break;

            case TreeNodeType::PROCEDURE:
                node->isExpandable = false;
                node->icon = QIcon::fromTheme("procedure");
                node->tooltip = "Procedure: " + node->name;
                break;

            default:
                node->icon = QIcon::fromTheme("unknown");
                node->tooltip = node->name;
                break;
        }

        // Set common properties
        node->loadState = NodeLoadState::NOT_LOADED;
        node->isVisible = true;
        node->isFiltered = false;
    }

    void loadNodeChildren(const std::shared_ptr<TreeNode>& node) {
        if (node->loadState == NodeLoadState::LOADING ||
            node->loadState == NodeLoadState::LOADED) {
            return;
        }

        node->loadState = NodeLoadState::LOADING;
        node->statusMessage = "Loading...";

        try {
            switch (node->type) {
                case TreeNodeType::ROOT:
                    loadConnectionNodes(node);
                    break;
                case TreeNodeType::CONNECTION:
                    loadDatabaseNodes(node);
                    break;
                case TreeNodeType::DATABASE:
                    loadSchemaNodes(node);
                    break;
                case TreeNodeType::SCHEMA:
                    loadSchemaObjectNodes(node);
                    break;
                case TreeNodeType::TABLE:
                    loadTableChildNodes(node);
                    break;
                case TreeNodeType::VIEW:
                    loadViewChildNodes(node);
                    break;
                default:
                    // Leaf nodes have no children
                    break;
            }

            node->loadState = NodeLoadState::LOADED;
            node->statusMessage = "";
            node->lastLoaded = std::chrono::system_clock::now();

        } catch (const std::exception& e) {
            node->loadState = NodeLoadState::ERROR;
            node->statusMessage = std::string("Error: ") + e.what();
            qWarning() << "Error loading node children:" << e.what();
        }
    }

    void loadConnectionNodes(const std::shared_ptr<TreeNode>& parent) {
        // In a real implementation, this would get connections from connection manager
        // For now, create a placeholder connection node

        auto connectionNode = createNode("conn_default", "Default Connection",
                                       TreeNodeType::CONNECTION, SchemaObjectType::SCHEMA, parent);
        parent->children.push_back(connectionNode);
    }

    void loadDatabaseNodes(const std::shared_ptr<TreeNode>& parent) {
        // Load databases for the connection
        // This would typically query the database for available databases

        auto databaseNode = createNode("db_default", "default",
                                     TreeNodeType::DATABASE, SchemaObjectType::SCHEMA, parent);
        parent->children.push_back(databaseNode);
    }

    void loadSchemaNodes(const std::shared_ptr<TreeNode>& parent) {
        if (!metadataManager_) {
            return;
        }

        try {
            // Query metadata for schemas
            MetadataQuery query;
            query.database = parent->name;
            query.type = SchemaObjectType::SCHEMA;

            auto result = metadataManager_->queryMetadata(query);
            if (result.success()) {
                for (const auto& obj : result.value().objects) {
                    if (!config_.showSystemObjects &&
                        (obj.name == "information_schema" || obj.name == "pg_catalog")) {
                        continue;
                    }

                    auto schemaNode = createNode("schema_" + obj.name, obj.name,
                                               TreeNodeType::SCHEMA, SchemaObjectType::SCHEMA, parent);
                    schemaNode->schema = obj.name;
                    schemaNode->database = parent->name;
                    parent->children.push_back(schemaNode);
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "Error loading schema nodes:" << e.what();
        }
    }

    void loadSchemaObjectNodes(const std::shared_ptr<TreeNode>& parent) {
        if (!metadataManager_) {
            return;
        }

        try {
            std::vector<SchemaObjectType> objectTypes = {
                SchemaObjectType::TABLE,
                SchemaObjectType::VIEW
            };

            for (auto type : objectTypes) {
                MetadataQuery query;
                query.schema = parent->name;
                query.database = parent->database;
                query.type = type;

                auto result = metadataManager_->queryMetadata(query);
                if (result.success()) {
                    for (const auto& obj : result.value().objects) {
                        TreeNodeType nodeType = (type == SchemaObjectType::TABLE) ?
                                              TreeNodeType::TABLE : TreeNodeType::VIEW;

                        auto objectNode = createNode(
                            "obj_" + obj.name + "_" + std::to_string(static_cast<int>(type)),
                            obj.name, nodeType, type, parent);

                        objectNode->schema = parent->name;
                        objectNode->database = parent->database;
                        objectNode->properties = obj.properties;

                        parent->children.push_back(objectNode);
                    }
                }
            }
        } catch (const std::exception& e) {
            qWarning() << "Error loading schema object nodes:" << e.what();
        }
    }

    void loadTableChildNodes(const std::shared_ptr<TreeNode>& parent) {
        if (!metadataManager_) {
            return;
        }

        try {
            // Load columns
            MetadataQuery columnQuery;
            columnQuery.schema = parent->schema;
            columnQuery.database = parent->database;
            columnQuery.type = SchemaObjectType::COLUMN;

            auto columnResult = metadataManager_->queryMetadata(columnQuery);
            if (columnResult.success()) {
                for (const auto& obj : columnResult.value().objects) {
                    if (obj.properties.count("table_name") &&
                        obj.properties.at("table_name") == parent->name) {

                        auto columnNode = createNode("col_" + obj.name, obj.name,
                                                   TreeNodeType::COLUMN, SchemaObjectType::COLUMN, parent);
                        columnNode->schema = parent->schema;
                        columnNode->database = parent->database;
                        columnNode->properties = obj.properties;

                        parent->children.push_back(columnNode);
                    }
                }
            }

            // Load indexes
            MetadataQuery indexQuery;
            indexQuery.schema = parent->schema;
            indexQuery.database = parent->database;
            indexQuery.type = SchemaObjectType::INDEX;

            auto indexResult = metadataManager_->queryMetadata(indexQuery);
            if (indexResult.success()) {
                for (const auto& obj : indexResult.value().objects) {
                    if (obj.properties.count("table_name") &&
                        obj.properties.at("table_name") == parent->name) {

                        auto indexNode = createNode("idx_" + obj.name, obj.name,
                                                  TreeNodeType::INDEX, SchemaObjectType::INDEX, parent);
                        indexNode->schema = parent->schema;
                        indexNode->database = parent->database;
                        indexNode->properties = obj.properties;

                        parent->children.push_back(indexNode);
                    }
                }
            }

            // Load constraints
            MetadataQuery constraintQuery;
            constraintQuery.schema = parent->schema;
            constraintQuery.database = parent->database;
            constraintQuery.type = SchemaObjectType::CONSTRAINT;

            auto constraintResult = metadataManager_->queryMetadata(constraintQuery);
            if (constraintResult.success()) {
                for (const auto& obj : constraintResult.value().objects) {
                    if (obj.properties.count("table_name") &&
                        obj.properties.at("table_name") == parent->name) {

                        auto constraintNode = createNode("con_" + obj.name, obj.name,
                                                       TreeNodeType::CONSTRAINT, SchemaObjectType::CONSTRAINT, parent);
                        constraintNode->schema = parent->schema;
                        constraintNode->database = parent->database;
                        constraintNode->properties = obj.properties;

                        parent->children.push_back(constraintNode);
                    }
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "Error loading table child nodes:" << e.what();
        }
    }

    void loadViewChildNodes(const std::shared_ptr<TreeNode>& parent) {
        if (!metadataManager_) {
            return;
        }

        try {
            // Load columns for the view
            MetadataQuery columnQuery;
            columnQuery.schema = parent->schema;
            columnQuery.database = parent->database;
            columnQuery.type = SchemaObjectType::COLUMN;

            auto columnResult = metadataManager_->queryMetadata(columnQuery);
            if (columnResult.success()) {
                for (const auto& obj : columnResult.value().objects) {
                    if (obj.properties.count("table_name") &&
                        obj.properties.at("table_name") == parent->name) {

                        auto columnNode = createNode("col_" + obj.name, obj.name,
                                                   TreeNodeType::COLUMN, SchemaObjectType::COLUMN, parent);
                        columnNode->schema = parent->schema;
                        columnNode->database = parent->database;
                        columnNode->properties = obj.properties;

                        parent->children.push_back(columnNode);
                    }
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "Error loading view child nodes:" << e.what();
        }
    }

    void applyFilterToNode(const std::shared_ptr<TreeNode>& node, const TreeFilter& filter) {
        if (filter.pattern.empty()) {
            node->isVisible = true;
            node->isFiltered = false;
            return;
        }

        // Check if node name matches the filter pattern
        bool matchesPattern = false;
        std::string searchText = filter.caseSensitive ? node->name : toLower(node->name);
        std::string pattern = filter.caseSensitive ? filter.pattern : toLower(filter.pattern);

        if (filter.regex) {
            try {
                std::regex regexPattern(pattern);
                matchesPattern = std::regex_search(searchText, regexPattern);
            } catch (const std::regex_error&) {
                matchesPattern = searchText.find(pattern) != std::string::npos;
            }
        } else {
            matchesPattern = searchText.find(pattern) != std::string::npos;
        }

        // Check node type filter
        bool matchesType = filter.nodeTypes.empty() ||
                          std::find(filter.nodeTypes.begin(), filter.nodeTypes.end(), node->type) != filter.nodeTypes.end();

        node->isVisible = matchesPattern && matchesType;
        node->isFiltered = !node->isVisible;

        // Recursively apply filter to children
        for (auto& child : node->children) {
            applyFilterToNode(child, filter);
        }

        // If any child is visible, parent should be visible too (for expansion)
        if (!node->isVisible && !filter.showOnlyMatching) {
            for (const auto& child : node->children) {
                if (child->isVisible) {
                    node->isVisible = true;
                    node->isFiltered = false;
                    break;
                }
            }
        }
    }

    void updateStatistics() {
        statistics_.totalNodes = 0;
        statistics_.visibleNodes = 0;
        statistics_.expandedNodes = 0;
        statistics_.loadingNodes = 0;
        statistics_.errorNodes = 0;

        if (rootNode_) {
            updateNodeStatistics(rootNode_);
        }

        statistics_.lastUpdated = std::chrono::system_clock::now();
    }

    void updateNodeStatistics(const std::shared_ptr<TreeNode>& node) {
        statistics_.totalNodes++;

        if (node->isVisible) {
            statistics_.visibleNodes++;
        }

        if (node->isExpanded) {
            statistics_.expandedNodes++;
        }

        if (node->loadState == NodeLoadState::LOADING) {
            statistics_.loadingNodes++;
        }

        if (node->loadState == NodeLoadState::ERROR) {
            statistics_.errorNodes++;
        }

        for (const auto& child : node->children) {
            updateNodeStatistics(child);
        }
    }

    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    std::string generateNodeId(const std::string& prefix, const std::string& name) {
        static std::atomic<uint64_t> counter{0};
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        return prefix + "_" + name + "_" + std::to_string(timestamp) + "_" + std::to_string(counter++);
    }

private:
    TreeModel* parent_;
    std::shared_ptr<IMetadataManager> metadataManager_;
    TreeModelConfiguration config_;
    TreeStatistics statistics_;
};

// TreeModel implementation

TreeModel::TreeModel(QObject* parent)
    : QAbstractItemModel(parent), impl_(std::make_unique<Impl>(this)) {

    // Initialize root node
    rootNode_ = std::make_shared<TreeNode>();
    rootNode_->id = "root";
    rootNode_->name = "Root";
    rootNode_->displayName = "Database Browser";
    rootNode_->type = TreeNodeType::ROOT;
    rootNode_->isExpandable = true;
    rootNode_->loadState = NodeLoadState::LOADED;

    impl_->setupNodeProperties(rootNode_);

    // Initialize load timer
    loadTimer_ = new QTimer(this);
    loadTimer_->setSingleShot(true);
    connect(loadTimer_, &QTimer::timeout, this, &TreeModel::onLoadTimeout);

    // Initialize refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &TreeModel::onRefreshTimer);
}

TreeModel::~TreeModel() = default;

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    std::shared_ptr<TreeNode> parentNode;
    if (!parent.isValid()) {
        parentNode = rootNode_;
    } else {
        parentNode = static_cast<TreeNode*>(parent.internalPointer())->shared_from_this();
    }

    if (row < 0 || row >= static_cast<int>(parentNode->children.size())) {
        return QModelIndex();
    }

    auto childNode = parentNode->children[row];
    return createIndex(row, column, childNode.get());
}

QModelIndex TreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) {
        return QModelIndex();
    }

    auto childNode = static_cast<TreeNode*>(child.internalPointer());
    auto parentNode = childNode->parent.lock();

    if (!parentNode || parentNode == rootNode_) {
        return QModelIndex();
    }

    // Find the row of the parent in its parent's children
    auto grandParent = parentNode->parent.lock();
    auto searchParent = grandParent ? grandParent : rootNode_;

    auto it = std::find_if(searchParent->children.begin(), searchParent->children.end(),
                          [parentNode](const auto& node) { return node == parentNode; });

    if (it != searchParent->children.end()) {
        int row = std::distance(searchParent->children.begin(), it);
        return createIndex(row, 0, parentNode.get());
    }

    return QModelIndex();
}

int TreeModel::rowCount(const QModelIndex& parent) const {
    std::shared_ptr<TreeNode> parentNode;
    if (!parent.isValid()) {
        parentNode = rootNode_;
    } else {
        parentNode = static_cast<TreeNode*>(parent.internalPointer())->shared_from_this();
    }

    return static_cast<int>(parentNode->children.size());
}

int TreeModel::columnCount(const QModelIndex& parent) const {
    return 2; // Name and Type columns
}

QVariant TreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    auto node = static_cast<TreeNode*>(index.internalPointer());

    switch (role) {
        case Qt::DisplayRole:
            if (index.column() == 0) {
                return QString::fromStdString(node->displayName);
            } else if (index.column() == 1) {
                return QString::fromStdString(toString(node->type));
            }
            break;

        case Qt::DecorationRole:
            if (index.column() == 0) {
                return node->icon;
            }
            break;

        case Qt::FontRole:
            return node->font;

        case Qt::BackgroundRole:
            return node->background;

        case Qt::ToolTipRole:
            return QString::fromStdString(node->tooltip);

        case Qt::UserRole:
            return QString::fromStdString(node->id);

        case Qt::UserRole + 1:
            return static_cast<int>(node->loadState);

        case Qt::UserRole + 2:
            return QString::fromStdString(node->statusMessage);
    }

    return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    auto node = static_cast<TreeNode*>(index.internalPointer());
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (node->isExpandable) {
        flags |= Qt::ItemIsTristate;
    }

    return flags;
}

bool TreeModel::hasChildren(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return !rootNode_->children.empty();
    }

    auto node = static_cast<TreeNode*>(parent.internalPointer());
    return node->isExpandable && !node->children.empty();
}

bool TreeModel::canFetchMore(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return false;
    }

    auto node = static_cast<TreeNode*>(parent.internalPointer());
    return node->isExpandable && node->loadState == NodeLoadState::NOT_LOADED;
}

void TreeModel::fetchMore(const QModelIndex& parent) {
    if (!parent.isValid()) {
        return;
    }

    auto node = static_cast<TreeNode*>(parent.internalPointer())->shared_from_this();

    if (node->loadState == NodeLoadState::NOT_LOADED) {
        // Emit signal to start loading
        emit layoutAboutToBeChanged();
        impl_->loadNodeChildren(node);
        emit layoutChanged();

        // Update statistics
        impl_->updateStatistics();
        if (statisticsChangedCallback_) {
            statisticsChangedCallback_(statistics_);
        }

        // Notify callbacks
        if (nodeLoadedCallback_) {
            nodeLoadedCallback_(node->id, true);
        }
    }
}

void TreeModel::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    impl_->metadataManager_ = metadataManager;
}

void TreeModel::addConnection(const std::string& connectionId, const std::string& connectionName) {
    emit layoutAboutToBeChanged();

    auto connectionNode = impl_->createNode("conn_" + connectionId, connectionName,
                                          TreeNodeType::CONNECTION, SchemaObjectType::SCHEMA, rootNode_);
    connectionNode->connectionId = connectionId;
    rootNode_->children.push_back(connectionNode);

    emit layoutChanged();
    impl_->updateStatistics();
}

void TreeModel::removeConnection(const std::string& connectionId) {
    emit layoutAboutToBeChanged();

    rootNode_->children.erase(
        std::remove_if(rootNode_->children.begin(), rootNode_->children.end(),
                      [connectionId](const auto& node) {
                          return node->connectionId == connectionId;
                      }),
        rootNode_->children.end());

    emit layoutChanged();
    impl_->updateStatistics();
}

void TreeModel::refreshConnection(const std::string& connectionId) {
    // Find the connection node and refresh it
    for (auto& node : rootNode_->children) {
        if (node->connectionId == connectionId) {
            node->loadState = NodeLoadState::NOT_LOADED;
            node->children.clear();
            fetchMore(getIndex(node));
            break;
        }
    }
}

void TreeModel::applyFilter(const TreeFilter& filter) {
    activeFilter_ = filter;

    emit layoutAboutToBeChanged();
    impl_->applyFilterToNode(rootNode_, filter);
    emit layoutChanged();

    impl_->updateStatistics();
    if (statisticsChangedCallback_) {
        statisticsChangedCallback_(statistics_);
    }
}

void TreeModel::clearFilter() {
    activeFilter_ = TreeFilter();

    emit layoutAboutToBeChanged();
    // Reset all nodes to visible
    std::function<void(const std::shared_ptr<TreeNode>&)> resetVisibility =
        [&](const std::shared_ptr<TreeNode>& node) {
            node->isVisible = true;
            node->isFiltered = false;
            for (auto& child : node->children) {
                resetVisibility(child);
            }
        };

    resetVisibility(rootNode_);
    emit layoutChanged();

    impl_->updateStatistics();
}

std::vector<std::string> TreeModel::getMatchingNodes(const std::string& pattern) const {
    std::vector<std::string> matches;

    std::function<void(const std::shared_ptr<TreeNode>&)> findMatches =
        [&](const std::shared_ptr<TreeNode>& node) {
            std::string searchText = activeFilter_.caseSensitive ? node->name : impl_->toLower(node->name);
            std::string searchPattern = activeFilter_.caseSensitive ? pattern : impl_->toLower(pattern);

            if (searchText.find(searchPattern) != std::string::npos) {
                matches.push_back(node->id);
            }

            for (const auto& child : node->children) {
                findMatches(child);
            }
        };

    findMatches(rootNode_);
    return matches;
}

std::shared_ptr<TreeNode> TreeModel::getNode(const QModelIndex& index) const {
    if (!index.isValid()) {
        return nullptr;
    }

    return static_cast<TreeNode*>(index.internalPointer())->shared_from_this();
}

QModelIndex TreeModel::getIndex(const std::shared_ptr<TreeNode>& node) const {
    if (!node || node == rootNode_) {
        return QModelIndex();
    }

    auto parentNode = node->parent.lock();
    if (!parentNode) {
        return QModelIndex();
    }

    // Find the row of this node in its parent's children
    auto it = std::find(parentNode->children.begin(), parentNode->children.end(), node);
    if (it != parentNode->children.end()) {
        int row = std::distance(parentNode->children.begin(), it);
        return createIndex(row, 0, node.get());
    }

    return QModelIndex();
}

void TreeModel::expandNode(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    auto node = getNode(index);
    if (node) {
        node->isExpanded = true;
        emit dataChanged(index, index);
    }
}

void TreeModel::collapseNode(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    auto node = getNode(index);
    if (node) {
        node->isExpanded = false;
        emit dataChanged(index, index);
    }
}

TreeStatistics TreeModel::getStatistics() const {
    return statistics_;
}

TreeModelConfiguration TreeModel::getConfiguration() const {
    return config_;
}

void TreeModel::updateConfiguration(const TreeModelConfiguration& config) {
    config_ = config;

    // Update refresh timer
    if (config.enableAutoRefresh) {
        refreshTimer_->start(config.refreshIntervalSeconds * 1000);
    } else {
        refreshTimer_->stop();
    }
}

void TreeModel::setNodeLoadedCallback(NodeLoadedCallback callback) {
    nodeLoadedCallback_ = callback;
}

void TreeModel::setStatisticsChangedCallback(StatisticsChangedCallback callback) {
    statisticsChangedCallback_ = callback;
}

void TreeModel::onLoadTimeout() {
    // Handle load timeout
    qWarning() << "Tree model load timeout";
}

void TreeModel::onRefreshTimer() {
    // Handle automatic refresh
    if (config_.enableAutoRefresh && !rootNode_->children.empty()) {
        refreshConnection(rootNode_->children[0]->connectionId);
    }
}

std::string TreeModel::toString(TreeNodeType type) {
    switch (type) {
        case TreeNodeType::ROOT: return "Root";
        case TreeNodeType::CONNECTION: return "Connection";
        case TreeNodeType::DATABASE: return "Database";
        case TreeNodeType::SCHEMA: return "Schema";
        case TreeNodeType::TABLE: return "Table";
        case TreeNodeType::VIEW: return "View";
        case TreeNodeType::COLUMN: return "Column";
        case TreeNodeType::INDEX: return "Index";
        case TreeNodeType::CONSTRAINT: return "Constraint";
        case TreeNodeType::TRIGGER: return "Trigger";
        case TreeNodeType::FUNCTION: return "Function";
        case TreeNodeType::PROCEDURE: return "Procedure";
        case TreeNodeType::SEQUENCE: return "Sequence";
        case TreeNodeType::DOMAIN: return "Domain";
        case TreeNodeType::TYPE: return "Type";
        case TreeNodeType::RULE: return "Rule";
        default: return "Unknown";
    }
}

} // namespace scratchrobin
