#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QTreeView;
class QListWidget;
class QStackedWidget;
class QRadioButton;
class QPlainTextEdit;
class QDialogButtonBox;
class QDateTimeEdit;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Team Sharing - Shared queries and snippets
 * 
 * Collaborate with team members:
 * - Share SQL queries and snippets
 * - Organize by categories/tags
 * - Permission management (read/write)
 * - Version history for shared items
 * - Team activity feed
 * - Favorites and ratings
 */

// ============================================================================
// Shared Item Types
// ============================================================================
enum class SharedItemType {
    SqlQuery,
    CodeSnippet,
    Template,
    Script,
    Documentation
};

enum class PermissionLevel {
    Owner,
    Editor,
    Viewer
};

// ============================================================================
// Shared Item Structure
// ============================================================================
struct SharedItem {
    QString id;
    QString name;
    QString description;
    SharedItemType type;
    QString content;
    QString author;
    QString authorId;
    QDateTime createdAt;
    QDateTime updatedAt;
    QStringList tags;
    QString category;
    int version = 1;
    bool isPublic = false;
    int viewCount = 0;
    int favoriteCount = 0;
    bool isFavorite = false;
    PermissionLevel myPermission = PermissionLevel::Viewer;
    
    bool operator==(const SharedItem& other) const {
        return id == other.id;
    }
};

struct TeamMember {
    QString id;
    QString name;
    QString email;
    QString role;
    QString avatar;
    QDateTime joinedAt;
    bool isOnline = false;
};

struct SharePermission {
    QString userId;
    QString userName;
    PermissionLevel level;
    QDateTime grantedAt;
    QString grantedBy;
};

// ============================================================================
// Team Sharing Panel
// ============================================================================
class TeamSharingPanel : public DockPanel {
    Q_OBJECT

public:
    explicit TeamSharingPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Team Sharing"); }
    QString panelCategory() const override { return "collaboration"; }

public slots:
    // Navigation
    void onCategoryChanged(int index);
    void onTagFilterChanged(const QString& tag);
    void onSearchTextChanged(const QString& text);
    
    // Item actions
    void onItemSelected(const QModelIndex& index);
    void onNewItem();
    void onEditItem();
    void onDeleteItem();
    void onDuplicateItem();
    void onShareItem();
    
    // Sharing actions
    void onToggleFavorite();
    void onCopyToClipboard();
    void onInsertIntoEditor();
    void onDownloadItem();
    
    // Team actions
    void onRefreshItems();
    void onViewTeamMembers();
    void onViewActivityFeed();

signals:
    void itemSelected(const SharedItem& item);
    void itemInserted(const QString& content);
    void shareRequested(const QString& itemId);

private:
    void setupUi();
    void setupCategories();
    void setupItemsList();
    void setupDetailsPanel();
    void loadItems();
    void updateItemsList();
    void updateItemDetails(const SharedItem& item);
    void filterItems();
    
    backend::SessionClient* client_;
    QList<SharedItem> items_;
    SharedItem currentItem_;
    QString currentFilter_;
    QString currentCategory_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTreeView* categoryTree_ = nullptr;
    QStandardItemModel* categoryModel_ = nullptr;
    
    // Items list
    QTableView* itemsTable_ = nullptr;
    QStandardItemModel* itemsModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    
    // Details panel
    QStackedWidget* detailsStack_ = nullptr;
    QLabel* itemNameLabel_ = nullptr;
    QLabel* itemAuthorLabel_ = nullptr;
    QLabel* itemDateLabel_ = nullptr;
    QLabel* itemStatsLabel_ = nullptr;
    QTextEdit* itemDescriptionEdit_ = nullptr;
    QPlainTextEdit* itemContentEdit_ = nullptr;
    QListWidget* tagsList_ = nullptr;
    QPushButton* favoriteBtn_ = nullptr;
    
    // Actions
    QPushButton* newBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
    QPushButton* shareBtn_ = nullptr;
    QPushButton* insertBtn_ = nullptr;
};

// ============================================================================
// New Shared Item Dialog
// ============================================================================
class NewSharedItemDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewSharedItemDialog(backend::SessionClient* client, QWidget* parent = nullptr);
    
    SharedItem item() const;

public slots:
    void onTypeChanged(int index);
    void onCategoryChanged(const QString& category);
    void onAddTag();
    void onRemoveTag();
    void onValidate();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QPlainTextEdit* contentEdit_ = nullptr;
    QListWidget* tagsList_ = nullptr;
    QLineEdit* newTagEdit_ = nullptr;
    QCheckBox* publicCheck_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Share Item Dialog
// ============================================================================
class ShareItemDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShareItemDialog(const SharedItem& item, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onAddUser();
    void onRemoveUser();
    void onPermissionChanged(const QString& userId, PermissionLevel level);
    void onMakePublic(bool checked);
    void onCopyLink();
    void onSendInvite();

private:
    void setupUi();
    void loadCurrentPermissions();
    void updatePermissionsList();
    
    SharedItem item_;
    backend::SessionClient* client_;
    QList<SharePermission> permissions_;
    
    QLabel* itemNameLabel_ = nullptr;
    QLineEdit* linkEdit_ = nullptr;
    QCheckBox* publicCheck_ = nullptr;
    QListWidget* permissionsList_ = nullptr;
    QComboBox* userCombo_ = nullptr;
    QComboBox* permissionCombo_ = nullptr;
    QPushButton* addUserBtn_ = nullptr;
};

// ============================================================================
// Team Members Dialog
// ============================================================================
class TeamMembersDialog : public QDialog {
    Q_OBJECT

public:
    explicit TeamMembersDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onInviteMember();
    void onRemoveMember();
    void onChangeRole();
    void onSendMessage();
    void onRefreshMembers();

private:
    void setupUi();
    void loadMembers();
    void updateMembersList();
    
    backend::SessionClient* client_;
    QList<TeamMember> members_;
    
    QTableView* membersTable_ = nullptr;
    QStandardItemModel* membersModel_ = nullptr;
    QLabel* teamStatsLabel_ = nullptr;
    QPushButton* inviteBtn_ = nullptr;
};

// ============================================================================
// Activity Feed Dialog
// ============================================================================
class ActivityFeedDialog : public QDialog {
    Q_OBJECT

public:
    explicit ActivityFeedDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onFilterByType(int type);
    void onFilterByUser(const QString& user);
    void onClearFilter();
    void onRefresh();
    void onViewActivityDetails();

private:
    void setupUi();
    void loadActivities();
    void updateActivityList();
    
    backend::SessionClient* client_;
    
    QTableView* activityTable_ = nullptr;
    QStandardItemModel* activityModel_ = nullptr;
    QComboBox* typeFilterCombo_ = nullptr;
    QComboBox* userFilterCombo_ = nullptr;
    QDateTimeEdit* dateFromEdit_ = nullptr;
    QDateTimeEdit* dateToEdit_ = nullptr;
};

// ============================================================================
// Import Shared Item Dialog
// ============================================================================
class ImportSharedItemDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportSharedItemDialog(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString selectedItemId() const;

public slots:
    void onSearch();
    void onItemSelected(const QModelIndex& index);
    void onPreviewItem();
    void onImport();

private:
    void setupUi();
    void searchItems(const QString& query);
    
    backend::SessionClient* client_;
    QString selectedItemId_;
    
    QLineEdit* searchEdit_ = nullptr;
    QTableView* resultsTable_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    QPushButton* importBtn_ = nullptr;
};

} // namespace scratchrobin::ui
