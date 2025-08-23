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
#include <QLabel>
#include <QAction>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QSplitter>
#include "ui/object_browser/tree_model.h"

namespace scratchrobin {


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
    std::shared_ptr<TreeModel> treeModel_;

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
