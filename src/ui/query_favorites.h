#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
class QTableView;
class QStandardItemModel;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QTabWidget;
class QListWidget;
class QSplitter;
class QCheckBox;
class QFormLayout;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

// Forward declaration
class QueryHistoryStorage;
struct QueryHistoryEntry;

/**
 * @brief Query Favorites - Full management system
 * 
 * Organize and manage favorite queries:
 * - Folder organization with drag-and-drop
 * - Tags and categories
 * - Full-text search
 * - Import/export functionality
 * - Quick access from sidebar
 */

// ============================================================================
// Favorite Folder Structure
// ============================================================================
struct FavoriteFolder {
    QString id;
    QString name;
    QString description;
    QString parentId;  // Empty for root folders
    QString icon;
    int sortOrder = 0;
    QDateTime createdAt;
    QDateTime modifiedAt;
};

struct FavoriteQuery {
    QString id;
    QString sql;
    QString name;
    QString description;
    QString folderId;  // Empty for uncategorized
    QStringList tags;
    QColor color;
    QDateTime createdAt;
    QDateTime modifiedAt;
    int executionCount = 0;
    QDateTime lastExecuted;
    bool isGlobal = false;  // Available across all connections
};

// ============================================================================
// Query Favorites Manager Panel
// ============================================================================
class QueryFavoritesManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit QueryFavoritesManagerPanel(QueryHistoryStorage* storage, 
                                 backend::SessionClient* client,
                                 QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Query Favorites"); }
    QString panelCategory() const override { return "sql"; }

public slots:
    // Navigation
    void onFolderSelected(QTreeWidgetItem* item);
    void onQuerySelected(const QModelIndex& index);
    void onSearchTextChanged(const QString& text);
    void onFilterByTag(const QString& tag);
    
    // Query actions
    void onAddFavorite();
    void onEditFavorite();
    void onDeleteFavorite();
    void onDuplicateFavorite();
    void onExecuteFavorite();
    void onCopyToEditor();
    
    // Folder actions
    void onNewFolder();
    void onRenameFolder();
    void onDeleteFolder();
    void onOrganizeFolders();
    
    // Organization
    void onAddTag();
    void onRemoveTag();
    void onSetColor();
    void onToggleGlobal();
    
    // Import/Export
    void onExportFavorites();
    void onImportFavorites();
    void onExportFolder();

signals:
    void querySelected(const QString& sql);
    void queryExecuteRequested(const QString& sql);
    void folderChanged(const QString& folderId);

private:
    void setupUi();
    void setupFolderTree();
    void setupQueryList();
    void setupDetailsPanel();
    void loadFolders();
    void loadQueries();
    void updateQueryList();
    void updateFolderTree();
    void updateDetails(const FavoriteQuery& query);
    QList<FavoriteQuery> getFavoritesInFolder(const QString& folderId);
    void populateFolderTree(QTreeWidgetItem* parent, const QString& parentId);
    
    QueryHistoryStorage* storage_ = nullptr;
    backend::SessionClient* client_ = nullptr;
    
    QList<FavoriteFolder> folders_;
    QList<FavoriteQuery> queries_;
    QString currentFolderId_;
    QString currentQueryId_;
    
    // UI
    QSplitter* splitter_ = nullptr;
    QTreeWidget* folderTree_ = nullptr;
    QTableView* queryTable_ = nullptr;
    QStandardItemModel* queryModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* tagFilter_ = nullptr;
    
    // Details panel
    QLabel* queryNameLabel_ = nullptr;
    QTextEdit* querySqlEdit_ = nullptr;
    QListWidget* tagsList_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    
    // Buttons
    QPushButton* executeBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
};

// ============================================================================
// Add/Edit Favorite Dialog
// ============================================================================
class EditFavoriteDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditFavoriteDialog(FavoriteQuery* query, 
                                const QList<FavoriteFolder>& folders,
                                QWidget* parent = nullptr);
    
    FavoriteQuery query() const;

public slots:
    void onSelectFolder();
    void onAddTag();
    void onRemoveTag();
    void onPickColor();
    void onValidate();

private:
    void setupUi();
    void populateFolderCombo();
    
    FavoriteQuery* originalQuery_;
    QList<FavoriteFolder> folders_;
    
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* sqlEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* folderCombo_ = nullptr;
    QListWidget* tagsList_ = nullptr;
    QLineEdit* newTagEdit_ = nullptr;
    QCheckBox* globalCheck_ = nullptr;
    QPushButton* colorBtn_ = nullptr;
    QColor selectedColor_;
};

// ============================================================================
// Folder Organization Dialog
// ============================================================================
class FolderOrganizationDialog : public QDialog {
    Q_OBJECT

public:
    explicit FolderOrganizationDialog(QList<FavoriteFolder>* folders,
                                      QWidget* parent = nullptr);

public slots:
    void onAddFolder();
    void onEditFolder();
    void onDeleteFolder();
    void onMoveUp();
    void onMoveDown();
    void onItemMoved();

private:
    void setupUi();
    void updateFolderList();
    
    QList<FavoriteFolder>* folders_;
    
    QTreeWidget* folderTree_ = nullptr;
    QPushButton* upBtn_ = nullptr;
    QPushButton* downBtn_ = nullptr;
};

// ============================================================================
// Quick Favorites Widget (for toolbar)
// ============================================================================
class QuickFavoritesWidget : public QWidget {
    Q_OBJECT

public:
    explicit QuickFavoritesWidget(QueryHistoryStorage* storage, QWidget* parent = nullptr);

public slots:
    void onFavoriteSelected(int index);
    void onAddCurrentQuery();
    void onManageFavorites();

signals:
    void favoriteSelected(const QString& sql);
    void favoriteExecuteRequested(const QString& sql);

private:
    void setupUi();
    void loadFavorites();
    
    QueryHistoryStorage* storage_;
    
    QComboBox* favoritesCombo_ = nullptr;
    QPushButton* addBtn_ = nullptr;
    QPushButton* manageBtn_ = nullptr;
};

// ============================================================================
// Favorites Import/Export Dialog
// ============================================================================
class FavoritesImportExportDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { Import, Export };
    
    explicit FavoritesImportExportDialog(Mode mode, 
                                         const QList<FavoriteQuery>& queries,
                                         const QList<FavoriteFolder>& folders,
                                         QWidget* parent = nullptr);

public slots:
    void onBrowseFile();
    void onSelectAll();
    void onSelectNone();
    void onToggleFolder(int state);
    void onExecute();

private:
    void setupUi();
    void populateTree();
    void exportToFile(const QString& fileName);
    void importFromFile(const QString& fileName);
    
    Mode mode_;
    QList<FavoriteQuery> queries_;
    QList<FavoriteFolder> folders_;
    
    QTreeWidget* itemTree_ = nullptr;
    QLineEdit* fileEdit_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QCheckBox* includeFoldersCheck_ = nullptr;
    QPushButton* executeBtn_ = nullptr;
};

} // namespace scratchrobin::ui
