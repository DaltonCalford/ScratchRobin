#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QSplitter;
class QListWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Role Management - Role hierarchy and management
 * 
 * Manage database roles and role hierarchies:
 * - Create/edit roles
 * - Manage role membership (GRANT/REVOKE ROLE)
 * - Visualize role hierarchy
 * - Role-based permission management
 */

// ============================================================================
// Role Types
// ============================================================================
enum class RoleType {
    User,      // Can login
    Group,     // Cannot login, used for grouping
    Application // Application-specific role
};

struct RoleInfo {
    QString name;
    RoleType type = RoleType::Group;
    bool isSuperuser = false;
    bool canCreateDB = false;
    bool canCreateRole = false;
    bool inheritPrivileges = true;
    bool canLogin = false;
    int connectionLimit = -1;
    QString description;
    QStringList memberOf;     // Roles this role is a member of
    QStringList members;      // Roles that are members of this role
    QStringList grants;       // Direct grants
    QString comment;
};

// ============================================================================
// Role Management Panel
// ============================================================================
class RoleManagementPanel : public DockPanel {
    Q_OBJECT

public:
    explicit RoleManagementPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Role Management"); }
    QString panelCategory() const override { return "security"; }

public slots:
    // Role operations
    void onCreateRole();
    void onEditRole();
    void onDeleteRole();
    void onCloneRole();
    void onRefreshRoles();
    
    // Hierarchy operations
    void onManageMembers();
    void onAddMember();
    void onRemoveMember();
    void onSetAdminOption();
    void onViewHierarchy();
    void onEditPermissions();
    void onViewPermissions();
    
    // Selection
    void onRoleSelected(const QModelIndex& index);
    void onSearchRoles(const QString& text);
    
    // Import/Export
    void onExportRoles();
    void onImportRoles();

signals:
    void roleCreated(const QString& roleName);
    void roleModified(const QString& roleName);
    void roleDeleted(const QString& roleName);

private:
    void setupUi();
    void setupHierarchyView();
    void setupMembersView();
    void loadRoles();
    void updateRoleTree();
    void updateRoleDetails(const RoleInfo& role);
    
    backend::SessionClient* client_;
    QList<RoleInfo> roles_;
    
    // UI
    QSplitter* splitter_ = nullptr;
    QTreeView* roleTree_ = nullptr;
    QStandardItemModel* roleModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Details
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* memberCountLabel_ = nullptr;
    QLabel* descriptionLabel_ = nullptr;
    QTableView* membersTable_ = nullptr;
    QStandardItemModel* membersModel_ = nullptr;
    QTreeView* permTree_ = nullptr;
    QTextEdit* hierarchyEdit_ = nullptr;
};

// ============================================================================
// Role Dialog
// ============================================================================
class RoleDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoleDialog(RoleInfo* role, bool isNew, QWidget* parent = nullptr);

public slots:
    void onSave();

private:
    void setupUi();
    void loadAvailableRoles();
    
    RoleInfo* role_;
    bool isNew_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QLineEdit* descriptionEdit_ = nullptr;
    
    // Privileges
    QCheckBox* createdbCheck_ = nullptr;
    QCheckBox* createroleCheck_ = nullptr;
    QCheckBox* loginCheck_ = nullptr;
    
    // Membership
    QTableView* memberOfTable_ = nullptr;
    QStandardItemModel* memberOfModel_ = nullptr;
    
    QTextEdit* commentEdit_ = nullptr;
};

// ============================================================================
// Role Hierarchy Dialog
// ============================================================================
class RoleHierarchyDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoleHierarchyDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExpandAll();
    void onCollapseAll();
    void onExportDiagram();
    void onPrint();

private:
    void setupUi();
    void buildHierarchy();
    
    backend::SessionClient* client_;
    
    QTreeView* hierarchyTree_ = nullptr;
    QStandardItemModel* hierarchyModel_ = nullptr;
};

// ============================================================================
// Grant Role Dialog
// ============================================================================
class GrantRoleDialog : public QDialog {
    Q_OBJECT

public:
    explicit GrantRoleDialog(const QString& grantorRole, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onGrant();

private:
    void setupUi();
    void loadRoles();
    
    QString grantorRole_;
    backend::SessionClient* client_;
    
    QComboBox* granteeCombo_ = nullptr;
    QComboBox* roleToGrantCombo_ = nullptr;
    QCheckBox* adminOptionCheck_ = nullptr;
    QCheckBox* inheritCheck_ = nullptr;
    QCheckBox* setCheck_ = nullptr;
};

// ============================================================================
// Role Membership Dialog
// ============================================================================
class RoleMembershipDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoleMembershipDialog(RoleInfo* role, QWidget* parent = nullptr);

public slots:
    void onAddMember();
    void onRemoveMember();
    void onSave();

private:
    void setupUi();
    
    RoleInfo* role_;
    
    QListWidget* membersList_ = nullptr;
    QListWidget* availableList_ = nullptr;
    QCheckBox* adminOptionCheck_ = nullptr;
};

} // namespace scratchrobin::ui
