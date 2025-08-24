#include "ui/object_browser/object_browser.h"
#include "ui/object_browser/tree_model.h"
#include "core/connection_manager.h"
#include "metadata/metadata_manager.h"
#include <QApplication>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QDrag>
#include <QMimeData>
#include <QStyle>
#include <QIcon>
#include <QDebug>
#include <algorithm>

namespace scratchrobin {

class ObjectBrowser::Impl {
public:
    Impl(ObjectBrowser* parent)
        : parent_(parent) {}

    void setupUI() {
        // Create main layout
        mainLayout_ = new QVBoxLayout(parent_);
        mainLayout_->setContentsMargins(5, 5, 5, 5);
        mainLayout_->setSpacing(5);

        // Create toolbar layout
        toolbarLayout_ = new QHBoxLayout();
        toolbarLayout_->setSpacing(2);

        // Create splitter for main content
        splitter_ = new QSplitter(Qt::Vertical);
        mainLayout_->addLayout(toolbarLayout_);
        mainLayout_->addWidget(splitter_);

        setupToolbar();
        setupTreeView();
        setupStatusBar();
        setupContextMenu();
    }

    void setupToolbar() {
        // Search box
        searchBox_ = new QLineEdit();
        searchBox_->setPlaceholderText("Search objects...");
        searchBox_->setMaximumWidth(300);
        toolbarLayout_->addWidget(searchBox_);

        // Search buttons
        searchButton_ = new QPushButton("Search");
        searchButton_->setMaximumWidth(80);
        toolbarLayout_->addWidget(searchButton_);

        clearSearchButton_ = new QPushButton("Clear");
        clearSearchButton_->setMaximumWidth(60);
        toolbarLayout_->addWidget(clearSearchButton_);

        toolbarLayout_->addSpacing(10);

        // View mode selector
        viewModeCombo_ = new QComboBox();
        viewModeCombo_->addItem("Tree View", static_cast<int>(BrowserViewMode::TREE));
        viewModeCombo_->addItem("Flat View", static_cast<int>(BrowserViewMode::FLAT));
        viewModeCombo_->addItem("Category View", static_cast<int>(BrowserViewMode::CATEGORY));
        viewModeCombo_->setMaximumWidth(120);
        toolbarLayout_->addWidget(viewModeCombo_);

        toolbarLayout_->addSpacing(10);

        // Control buttons
        refreshButton_ = new QPushButton("Refresh");
        refreshButton_->setMaximumWidth(80);
        toolbarLayout_->addWidget(refreshButton_);

        expandAllButton_ = new QPushButton("Expand All");
        expandAllButton_->setMaximumWidth(90);
        toolbarLayout_->addWidget(expandAllButton_);

        collapseAllButton_ = new QPushButton("Collapse All");
        collapseAllButton_->setMaximumWidth(100);
        toolbarLayout_->addWidget(collapseAllButton_);

        toolbarLayout_->addStretch();

        // Filter controls
        showSystemObjectsCheck_ = new QCheckBox("Show System Objects");
        toolbarLayout_->addWidget(showSystemObjectsCheck_);

        showTemporaryObjectsCheck_ = new QCheckBox("Show Temporary Objects");
        toolbarLayout_->addWidget(showTemporaryObjectsCheck_);

        filterTypeCombo_ = new QComboBox();
        filterTypeCombo_->addItem("All Types", -1);
        filterTypeCombo_->addItem("Tables", static_cast<int>(TreeNodeType::TABLE));
        filterTypeCombo_->addItem("Views", static_cast<int>(TreeNodeType::VIEW));
        filterTypeCombo_->addItem("Columns", static_cast<int>(TreeNodeType::COLUMN));
        filterTypeCombo_->addItem("Indexes", static_cast<int>(TreeNodeType::INDEX));
        filterTypeCombo_->addItem("Constraints", static_cast<int>(TreeNodeType::CONSTRAINT));
        filterTypeCombo_->setMaximumWidth(120);
        toolbarLayout_->addWidget(filterTypeCombo_);
    }

    void setupTreeView() {
        // Create tree model
        treeModel_ = std::make_shared<TreeModel>();

        // Create tree view
        treeView_ = new QTreeView();
        treeView_->setModel(treeModel_.get());
        treeView_->setAlternatingRowColors(true);
        treeView_->setRootIsDecorated(true);
        treeView_->setUniformRowHeights(true);
        treeView_->setIndentation(20);
        treeView_->setExpandsOnDoubleClick(true);
        treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
        treeView_->setDragEnabled(true);
        treeView_->setAcceptDrops(true);
        treeView_->setDropIndicatorShown(true);

        // Set up headers
        treeView_->header()->setStretchLastSection(true);
        treeView_->header()->setDefaultSectionSize(200);
        treeView_->header()->setSectionsClickable(true);
        treeView_->header()->setSectionsMovable(false);

        // Set column headers
        treeModel_->setHeaderData(0, Qt::Horizontal, "Name");
        treeModel_->setHeaderData(1, Qt::Horizontal, "Type");

        splitter_->addWidget(treeView_);
    }

    void setupStatusBar() {
        statusBar_ = new QStatusBar();
        statusBar_->setSizeGripEnabled(false);
        statusBar_->setMaximumHeight(25);

        statusLabel_ = new QLabel("Ready");
        statusBar_->addWidget(statusLabel_);

        progressBar_ = new QProgressBar();
        progressBar_->setMaximumWidth(200);
        progressBar_->setVisible(false);
        statusBar_->addPermanentWidget(progressBar_);

        mainLayout_->addWidget(statusBar_);
    }

    void setupContextMenu() {
        contextMenu_ = new QMenu(parent_);

        // Create context menu actions
        createContextMenuActions();
    }

    void createContextMenuActions() {
        // View Actions
        auto viewDataAction = contextMenu_->addAction("View Data");
        viewDataAction->setData(static_cast<int>(ObjectAction::VIEW_DATA));
        viewDataAction->setIcon(QIcon::fromTheme("view-list-text"));

        auto viewPropertiesAction = contextMenu_->addAction("View Properties");
        viewPropertiesAction->setData(static_cast<int>(ObjectAction::VIEW_PROPERTIES));
        viewPropertiesAction->setIcon(QIcon::fromTheme("document-properties"));

        contextMenu_->addSeparator();

        // Modify Actions
        auto createAction = contextMenu_->addAction("Create");
        createAction->setData(static_cast<int>(ObjectAction::CREATE));
        createAction->setIcon(QIcon::fromTheme("document-new"));

        auto editAction = contextMenu_->addAction("Edit");
        editAction->setData(static_cast<int>(ObjectAction::EDIT));
        editAction->setIcon(QIcon::fromTheme("document-edit"));

        auto alterAction = contextMenu_->addAction("Alter");
        alterAction->setData(static_cast<int>(ObjectAction::ALTER));
        alterAction->setIcon(QIcon::fromTheme("document-edit"));

        auto dropAction = contextMenu_->addAction("Drop");
        dropAction->setData(static_cast<int>(ObjectAction::DROP));
        dropAction->setIcon(QIcon::fromTheme("edit-delete"));
        dropAction->setShortcut(QKeySequence::Delete);

        contextMenu_->addSeparator();

        // Analysis Actions
        auto viewDependenciesAction = contextMenu_->addAction("View Dependencies");
        viewDependenciesAction->setData(static_cast<int>(ObjectAction::VIEW_DEPENDENCIES));
        viewDependenciesAction->setIcon(QIcon::fromTheme("network-server"));

        auto viewDependentsAction = contextMenu_->addAction("View Dependents");
        viewDependentsAction->setData(static_cast<int>(ObjectAction::VIEW_DEPENDENTS));
        viewDependentsAction->setIcon(QIcon::fromTheme("network-workgroup"));

        auto analyzeAction = contextMenu_->addAction("Analyze");
        analyzeAction->setData(static_cast<int>(ObjectAction::ANALYZE));
        analyzeAction->setIcon(QIcon::fromTheme("edit-find"));

        contextMenu_->addSeparator();

        // Maintenance Actions
        auto optimizeAction = contextMenu_->addAction("Optimize");
        optimizeAction->setData(static_cast<int>(ObjectAction::OPTIMIZE));
        optimizeAction->setIcon(QIcon::fromTheme("run-build"));

        auto reindexAction = contextMenu_->addAction("Reindex");
        reindexAction->setData(static_cast<int>(ObjectAction::REINDEX));
        reindexAction->setIcon(QIcon::fromTheme("view-refresh"));

        auto vacuumAction = contextMenu_->addAction("Vacuum");
        vacuumAction->setData(static_cast<int>(ObjectAction::VACUUM));
        vacuumAction->setIcon(QIcon::fromTheme("edit-clear"));

        contextMenu_->addSeparator();

        // Utility Actions
        auto refreshAction = contextMenu_->addAction("Refresh");
        refreshAction->setData(static_cast<int>(ObjectAction::REFRESH));
        refreshAction->setIcon(QIcon::fromTheme("view-refresh"));
        refreshAction->setShortcut(QKeySequence::Refresh);

        auto copyNameAction = contextMenu_->addAction("Copy Name");
        copyNameAction->setData(static_cast<int>(ObjectAction::COPY_NAME));
        copyNameAction->setIcon(QIcon::fromTheme("edit-copy"));

        auto copyDdlAction = contextMenu_->addAction("Copy DDL");
        copyDdlAction->setData(static_cast<int>(ObjectAction::COPY_DDL));
        copyDdlAction->setIcon(QIcon::fromTheme("text-x-sql"));

        auto generateScriptAction = contextMenu_->addAction("Generate Script");
        generateScriptAction->setData(static_cast<int>(ObjectAction::GENERATE_SCRIPT));
        generateScriptAction->setIcon(QIcon::fromTheme("text-x-script"));

        contextMenu_->addSeparator();

        // Export/Import Actions
        auto exportAction = contextMenu_->addAction("Export");
        exportAction->setData(static_cast<int>(ObjectAction::EXPORT));
        exportAction->setIcon(QIcon::fromTheme("document-save"));

        auto importAction = contextMenu_->addAction("Import");
        importAction->setData(static_cast<int>(ObjectAction::IMPORT));
        importAction->setIcon(QIcon::fromTheme("document-open"));
    }

    void setupConnections() {
        // Search connections
        connect(searchBox_, &QLineEdit::returnPressed, [this]() { parent_->onSearchButtonClicked(); });
        connect(searchButton_, &QPushButton::clicked, parent_, &ObjectBrowser::onSearchButtonClicked);
        connect(clearSearchButton_, &QPushButton::clicked, parent_, &ObjectBrowser::onClearSearchButtonClicked);
        connect(searchBox_, &QLineEdit::textChanged, parent_, &ObjectBrowser::onSearchTextChanged);

        // Tree view connections
        connect(treeView_, &QTreeView::customContextMenuRequested, parent_, &ObjectBrowser::onContextMenuRequested);
        connect(treeView_->selectionModel(), &QItemSelectionModel::selectionChanged,
                parent_, &ObjectBrowser::onTreeSelectionChanged);
        connect(treeView_, &QTreeView::doubleClicked, parent_, &ObjectBrowser::onTreeDoubleClicked);

        // Toolbar connections
        connect(refreshButton_, &QPushButton::clicked, parent_, &ObjectBrowser::onRefreshButtonClicked);
        connect(expandAllButton_, &QPushButton::clicked, parent_, &ObjectBrowser::onExpandAllButtonClicked);
        connect(collapseAllButton_, &QPushButton::clicked, parent_, &ObjectBrowser::onCollapseAllButtonClicked);
        connect(viewModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                parent_, &ObjectBrowser::onViewModeChanged);

        // Filter connections
        connect(showSystemObjectsCheck_, &QCheckBox::toggled, parent_, &ObjectBrowser::onFilterChanged);
        connect(showTemporaryObjectsCheck_, &QCheckBox::toggled, parent_, &ObjectBrowser::onFilterChanged);
        connect(filterTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                parent_, &ObjectBrowser::onFilterChanged);

        // Context menu connections
        for (auto action : contextMenu_->actions()) {
            if (action->data().isValid()) {
                connect(action, &QAction::triggered, [this, action]() {
                    ObjectAction objectAction = static_cast<ObjectAction>(action->data().toInt());
                    std::string nodeId = getSelectedNodeId();
                    if (!nodeId.empty()) {
                        executeObjectAction(objectAction, nodeId);
                    }
                });
            }
        }

        // Tree model connections
        treeModel_->setStatisticsChangedCallback([this](const TreeStatistics& stats) {
            handleTreeModelStatisticsChanged(stats);
        });
    }

    void updateContextMenu(const std::string& nodeId) {
        // Get the node to determine which actions are applicable
        auto index = getSelectedIndex();
        if (!index.isValid()) {
            return;
        }

        auto node = treeModel_->getNode(index);
        if (!node) {
            return;
        }

        // Enable/disable actions based on node type
        for (auto action : contextMenu_->actions()) {
            if (action->data().isValid()) {
                ObjectAction objectAction = static_cast<ObjectAction>(action->data().toInt());
                bool enabled = isActionEnabledForNode(objectAction, node);
                action->setEnabled(enabled);
            }
        }
    }

    bool isActionEnabledForNode(ObjectAction action, const std::shared_ptr<TreeNode>& node) {
        switch (action) {
            case ObjectAction::VIEW_DATA:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::VIEW;

            case ObjectAction::EDIT:
            case ObjectAction::ALTER:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::VIEW ||
                       node->type == TreeNodeType::FUNCTION || node->type == TreeNodeType::PROCEDURE;

            case ObjectAction::DROP:
                return node->type != TreeNodeType::ROOT && node->type != TreeNodeType::CONNECTION &&
                       node->type != TreeNodeType::DATABASE;

            case ObjectAction::CREATE:
                return node->type == TreeNodeType::SCHEMA || node->type == TreeNodeType::DATABASE;

            case ObjectAction::VIEW_PROPERTIES:
                return node->type != TreeNodeType::ROOT;

            case ObjectAction::VIEW_DEPENDENCIES:
            case ObjectAction::VIEW_DEPENDENTS:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::VIEW ||
                       node->type == TreeNodeType::FUNCTION || node->type == TreeNodeType::PROCEDURE;

            case ObjectAction::ANALYZE:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::INDEX;

            case ObjectAction::OPTIMIZE:
            case ObjectAction::REINDEX:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::INDEX;

            case ObjectAction::VACUUM:
                return node->type == TreeNodeType::TABLE;

            case ObjectAction::REFRESH:
                return node->isExpandable;

            case ObjectAction::COPY_NAME:
            case ObjectAction::COPY_DDL:
            case ObjectAction::GENERATE_SCRIPT:
                return node->type != TreeNodeType::ROOT;

            case ObjectAction::EXPORT:
                return node->type == TreeNodeType::TABLE || node->type == TreeNodeType::VIEW;

            case ObjectAction::IMPORT:
                return node->type == TreeNodeType::TABLE;

            default:
                return true;
        }
    }

    void executeObjectAction(ObjectAction action, const std::string& nodeId) {
        if (parent_->objectActionCallback_) {
            parent_->objectActionCallback_(action, nodeId);
        }

        // Execute built-in actions
        switch (action) {
            case ObjectAction::COPY_NAME:
                copyNodeName(nodeId);
                break;
            case ObjectAction::REFRESH:
                refreshNode(nodeId);
                break;
            case ObjectAction::SELECT:
                selectNode(nodeId);
                break;
            default:
                // Other actions are handled by the callback
                break;
        }
    }

    void copyNodeName(const std::string& nodeId) {
        auto index = findNodeIndex(nodeId);
        if (index.isValid()) {
            auto node = treeModel_->getNode(index);
            if (node) {
                QApplication::clipboard()->setText(QString::fromStdString(node->name));
                statusLabel_->setText("Copied node name to clipboard");
            }
        }
    }

    void refreshNode(const std::string& nodeId) {
        auto index = findNodeIndex(nodeId);
        if (index.isValid()) {
            treeModel_->fetchMore(index);
            statusLabel_->setText("Refreshing node...");
        }
    }

    void selectNode(const std::string& nodeId) {
        auto index = findNodeIndex(nodeId);
        if (index.isValid()) {
            treeView_->setCurrentIndex(index);
            treeView_->scrollTo(index);
        }
    }

    QModelIndex findNodeIndex(const std::string& nodeId) {
        // Search through the tree model to find the node
        std::function<QModelIndex(const QModelIndex&)> findNode =
            [&](const QModelIndex& parent) -> QModelIndex {
                for (int row = 0; row < treeModel_->rowCount(parent); ++row) {
                    QModelIndex index = treeModel_->index(row, 0, parent);
                    auto node = treeModel_->getNode(index);
                    if (node && node->id == nodeId) {
                        return index;
                    }

                    // Recursively search children
                    QModelIndex childIndex = findNode(index);
                    if (childIndex.isValid()) {
                        return childIndex;
                    }
                }
                return QModelIndex();
            };

        return findNode(QModelIndex());
    }

    void updateSearchResults() {
        // This would integrate with the search engine
        // For now, use the tree model's built-in filtering
        TreeFilter filter;
        filter.pattern = currentSearch_.pattern;
        filter.caseSensitive = currentSearch_.caseSensitive;
        filter.regex = currentSearch_.regex;
        filter.showOnlyMatching = !currentSearch_.pattern.empty();

        treeModel_->applyFilter(filter);
    }

    void updateStatusBar() {
        auto stats = treeModel_->getStatistics();

        QString status = QString("Nodes: %1 visible, %2 total | %3 expanded | %4 loading | %5 error")
                        .arg(stats.visibleNodes)
                        .arg(stats.totalNodes)
                        .arg(stats.expandedNodes)
                        .arg(stats.loadingNodes)
                        .arg(stats.errorNodes);

        if (!selectedConnection_.empty()) {
            status += QString(" | Connection: %1").arg(QString::fromStdString(selectedConnection_));
        }

        statusLabel_->setText(status);
    }

    void showProgress(bool visible, const std::string& message) {
        progressBar_->setVisible(visible);
        if (visible && !message.empty()) {
            statusLabel_->setText(QString::fromStdString(message));
        }
    }

    void applyViewMode(BrowserViewMode mode) {
        // This would change how the tree is displayed
        // For now, just update the UI
        switch (mode) {
            case BrowserViewMode::TREE:
                treeView_->setRootIsDecorated(true);
                break;
            case BrowserViewMode::FLAT:
                treeView_->setRootIsDecorated(false);
                break;
            case BrowserViewMode::CATEGORY:
                // Would group items by category
                treeView_->setRootIsDecorated(true);
                break;
        }
    }

    void applyFilters() {
        TreeFilter filter;

        // Apply system objects filter
        filter.showSystemObjects = showSystemObjectsCheck_->isChecked();

        // Apply temporary objects filter
        filter.showTemporaryObjects = showTemporaryObjectsCheck_->isChecked();

        // Note: TreeModelConfiguration doesn't have showTemporaryObjects member
        // Configuration updates would need to be implemented via setConfiguration method

        // Apply type filter
        int typeIndex = filterTypeCombo_->currentData().toInt();
        if (typeIndex >= 0) {
            filter.nodeTypes = {static_cast<TreeNodeType>(typeIndex)};
        }

        treeModel_->applyFilter(filter);
    }

    std::string getSelectedNodeId() const {
        auto index = getSelectedIndex();
        if (index.isValid()) {
            auto node = treeModel_->getNode(index);
            return node ? node->id : "";
        }
        return "";
    }

    QModelIndex getSelectedIndex() const {
        auto selection = treeView_->selectionModel()->selectedIndexes();
        return selection.isEmpty() ? QModelIndex() : selection.first();
    }

    void handleTreeModelStatisticsChanged(const TreeStatistics& stats) {
        updateStatusBar();
    }

    void setupDragAndDrop() {
        // Tree view already has drag and drop enabled in setupTreeView()
    }

private:
    ObjectBrowser* parent_;

    // UI Components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* toolbarLayout_;
    QSplitter* splitter_;

    QLineEdit* searchBox_;
    QPushButton* searchButton_;
    QPushButton* clearSearchButton_;
    QComboBox* viewModeCombo_;
    QPushButton* refreshButton_;
    QPushButton* expandAllButton_;
    QPushButton* collapseAllButton_;

    QCheckBox* showSystemObjectsCheck_;
    QCheckBox* showTemporaryObjectsCheck_;
    QComboBox* filterTypeCombo_;

    QTreeView* treeView_;
    std::shared_ptr<TreeModel> treeModel_;

    QStatusBar* statusBar_;
    QProgressBar* progressBar_;
    QLabel* statusLabel_;

    QMenu* contextMenu_;

    // Configuration and state
    BrowserConfiguration config_;
    SearchOptions currentSearch_;
    std::string selectedConnection_;

    // Core components
    std::shared_ptr<IConnectionManager> connectionManager_;
    std::shared_ptr<IMetadataManager> metadataManager_;
};

// ObjectBrowser implementation

ObjectBrowser::ObjectBrowser(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>(this)) {

    impl_->setupUI();
    impl_->setupConnections();
}

ObjectBrowser::~ObjectBrowser() = default;

void ObjectBrowser::initialize(const BrowserConfiguration& config) {
    config_ = config;

    // Apply configuration
    viewModeCombo_->setCurrentIndex(static_cast<int>(config.defaultViewMode));
    applyViewMode(config.defaultViewMode);

    treeView_->setIndentation(config.defaultTreeIndentation);
    treeView_->setAlternatingRowColors(config.enableAnimations);
    treeView_->setDragEnabled(config.enableDragDrop);
    treeView_->setAcceptDrops(config.enableDragDrop);

    if (config.enableDragDrop) {
        impl_->setupDragAndDrop();
    }
}

void ObjectBrowser::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
    treeModel_->setConnectionManager(connectionManager);
}

void ObjectBrowser::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
    treeModel_->setMetadataManager(metadataManager);
}

void ObjectBrowser::addConnection(const std::string& connectionId, const std::string& connectionName) {
    treeModel_->addConnection(connectionId, connectionName);
    selectedConnection_ = connectionId;
    impl_->updateStatusBar();
}

void ObjectBrowser::removeConnection(const std::string& connectionId) {
    treeModel_->removeConnection(connectionId);
    if (selectedConnection_ == connectionId) {
        selectedConnection_.clear();
    }
    impl_->updateStatusBar();
}

void ObjectBrowser::selectConnection(const std::string& connectionId) {
    selectedConnection_ = connectionId;
    // Find and select the connection node in the tree
    // This would require searching through the tree model
    impl_->updateStatusBar();
}

std::string ObjectBrowser::getSelectedConnection() const {
    return selectedConnection_;
}

void ObjectBrowser::search(const SearchOptions& options) {
    currentSearch_ = options;
    searchBox_->setText(QString::fromStdString(options.pattern));
    impl_->updateSearchResults();
}

void ObjectBrowser::clearSearch() {
    currentSearch_ = SearchOptions();
    searchBox_->clear();
    treeModel_->clearFilter();
    impl_->updateStatusBar();
}

std::vector<std::string> ObjectBrowser::getSearchResults() const {
    return treeModel_->getMatchingNodes(currentSearch_.pattern);
}

void ObjectBrowser::refreshView() {
    if (!selectedConnection_.empty()) {
        treeModel_->refreshConnection(selectedConnection_);
    }
    impl_->updateStatusBar();
}

void ObjectBrowser::expandAll() {
    treeView_->expandAll();
    impl_->updateStatusBar();
}

void ObjectBrowser::collapseAll() {
    treeView_->collapseAll();
    impl_->updateStatusBar();
}

void ObjectBrowser::expandNode(const std::string& nodeId) {
    auto index = impl_->findNodeIndex(nodeId);
    if (index.isValid()) {
        treeView_->expand(index);
    }
}

BrowserConfiguration ObjectBrowser::getConfiguration() const {
    return config_;
}

void ObjectBrowser::updateConfiguration(const BrowserConfiguration& config) {
    config_ = config;
    initialize(config);
}

void ObjectBrowser::setSelectionChangedCallback(SelectionChangedCallback callback) {
    selectionChangedCallback_ = callback;
}

void ObjectBrowser::setObjectActionCallback(ObjectActionCallback callback) {
    objectActionCallback_ = callback;
}

QWidget* ObjectBrowser::getWidget() {
    return this;
}

// Private slot implementations

void ObjectBrowser::onSearchTextChanged(const QString& text) {
    currentSearch_.pattern = text.toStdString();
    if (config_.defaultViewMode == BrowserViewMode::TREE) {
        // Real-time search for tree view
        impl_->updateSearchResults();
    }
}

void ObjectBrowser::onSearchButtonClicked() {
    currentSearch_.pattern = searchBox_->text().toStdString();
    currentSearch_.caseSensitive = false; // Could add a checkbox for this
    currentSearch_.regex = false; // Could add a checkbox for this
    impl_->updateSearchResults();
}

void ObjectBrowser::onClearSearchButtonClicked() {
    clearSearch();
}

void ObjectBrowser::onTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    if (selectionChangedCallback_ && !selected.isEmpty()) {
        std::string nodeId = impl_->getSelectedNodeId();
        if (!nodeId.empty()) {
            selectionChangedCallback_(nodeId);
        }
    }
    impl_->updateStatusBar();
}

void ObjectBrowser::onTreeDoubleClicked(const QModelIndex& index) {
    if (index.isValid()) {
        auto node = treeModel_->getNode(index);
        if (node && node->isExpandable) {
            if (treeView_->isExpanded(index)) {
                treeView_->collapse(index);
            } else {
                treeView_->expand(index);
            }
        }
    }
}

void ObjectBrowser::onContextMenuRequested(const QPoint& pos) {
    auto index = treeView_->indexAt(pos);
    if (index.isValid()) {
        treeView_->setCurrentIndex(index);
        std::string nodeId = impl_->getSelectedNodeId();
        if (!nodeId.empty()) {
            impl_->updateContextMenu(nodeId);
            contextMenu_->exec(treeView_->mapToGlobal(pos));
        }
    }
}

void ObjectBrowser::onRefreshButtonClicked() {
    refreshView();
}

void ObjectBrowser::onExpandAllButtonClicked() {
    expandAll();
}

void ObjectBrowser::onCollapseAllButtonClicked() {
    collapseAll();
}

void ObjectBrowser::onViewModeChanged(int index) {
    BrowserViewMode mode = static_cast<BrowserViewMode>(
        viewModeCombo_->itemData(index).toInt());
    config_.defaultViewMode = mode;
    impl_->applyViewMode(mode);
}

void ObjectBrowser::onFilterChanged() {
    impl_->applyFilters();
}

void ObjectBrowser::onLoadProgress(int current, int total) {
    if (progressBar_->isVisible()) {
        progressBar_->setValue(current);
        progressBar_->setMaximum(total);
    }
}

void ObjectBrowser::onLoadCompleted(bool success) {
    impl_->showProgress(false, success ? "Load completed successfully" : "Load failed");
}

void ObjectBrowser::applyViewMode(BrowserViewMode mode) {
    // Apply the specified view mode to the object browser
    switch (mode) {
        case BrowserViewMode::TREE:
            // Tree view is the default
            treeView_->setVisible(true);
            break;
        case BrowserViewMode::FLAT:
            // Flat view - show all items in a single level
            treeView_->setVisible(true);
            break;
        case BrowserViewMode::CATEGORY:
            // Category view - group items by type
            treeView_->setVisible(true);
            break;
        default:
            treeView_->setVisible(true);
            break;
    }

    // Store the current view mode (if impl_ has this member)
    // impl_->currentViewMode_ = mode;
}

} // namespace scratchrobin
