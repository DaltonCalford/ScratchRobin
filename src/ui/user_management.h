#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QFormLayout;
class QDateTimeEdit;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief User Management - Create and edit database users
 * 
 * Manage database user accounts:
 * - Create new users with passwords
 * - Edit user properties
 * - Lock/unlock accounts
 * - Set expiration dates
 * - Manage user attributes
 */

// ============================================================================
// User Account Info
// ============================================================================
struct UserAccountInfo {
    QString name;
    QString password;
    bool isSuperuser = false;
    bool canCreateDB = false;
    bool canCreateRole = false;
    bool inheritPrivileges = true;
    bool canLogin = true;
    bool isReplication = false;
    bool bypassRLS = false;
    int connectionLimit = -1; // -1 = unlimited
    QDateTime validUntil;
    QStringList memberOf; // Groups/roles this user belongs to
    QStringList ownsObjects; // Objects owned by this user
    QString comment;
};

// ============================================================================
// User Management Panel
// ============================================================================
class UserManagementPanel : public DockPanel {
    Q_OBJECT

public:
    explicit UserManagementPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("User Management"); }
    QString panelCategory() const override { return "security"; }

public slots:
    void onCreateUser();
    void onEditUser();
    void onDeleteUser();
    void onCloneUser();
    void onRefreshUsers();
    void onUserSelected(const QModelIndex& index);
    void onSearchUsers(const QString& text);
    
    // User actions
    void onLockUser();
    void onUnlockUser();
    void onChangePassword();
    void onResetPassword();
    void onSetExpiration();
    void onViewUserObjects();
    
    // Import/Export
    void onExportUsers();
    void onImportUsers();

signals:
    void userCreated(const QString& username);
    void userModified(const QString& username);
    void userDeleted(const QString& username);

private:
    void setupUi();
    void loadUsers();
    void updateUserDetails(const UserAccountInfo& user);
    
    backend::SessionClient* client_;
    QList<UserAccountInfo> users_;
    
    // UI
    QTableView* userTable_ = nullptr;
    QStandardItemModel* userModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Details panel
    QLabel* nameLabel_ = nullptr;
    QLabel* superuserLabel_ = nullptr;
    QLabel* createdbLabel_ = nullptr;
    QLabel* loginLabel_ = nullptr;
    QLabel* expiryLabel_ = nullptr;
    QLabel* connectionLimitLabel_ = nullptr;
    QTableView* memberOfTable_ = nullptr;
    QStandardItemModel* memberOfModel_ = nullptr;
};

// ============================================================================
// Create/Edit User Dialog
// ============================================================================
class UserDialog : public QDialog {
    Q_OBJECT

public:
    explicit UserDialog(UserAccountInfo* user, bool isNew, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onGeneratePassword();
    void onCopyPassword();
    void onTestConnection();
    void onSave();
    void onShowPassword(bool show);

private:
    void setupUi();
    void loadRoles();
    
    UserAccountInfo* user_;
    bool isNew_;
    backend::SessionClient* client_;
    QString generatedPassword_;
    
    // Basic info
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QCheckBox* showPasswordCheck_ = nullptr;
    
    // Privileges
    QCheckBox* superuserCheck_ = nullptr;
    QCheckBox* createdbCheck_ = nullptr;
    QCheckBox* createroleCheck_ = nullptr;
    QCheckBox* inheritCheck_ = nullptr;
    QCheckBox* loginCheck_ = nullptr;
    QCheckBox* replicationCheck_ = nullptr;
    QCheckBox* bypassRLSCheck_ = nullptr;
    
    // Limits
    QLineEdit* connectionLimitEdit_ = nullptr;
    QDateTimeEdit* validUntilEdit_ = nullptr;
    
    // Membership
    QTableView* rolesTable_ = nullptr;
    QStandardItemModel* rolesModel_ = nullptr;
    
    // Comment
    QTextEdit* commentEdit_ = nullptr;
};

// ============================================================================
// Change Password Dialog
// ============================================================================
class ChangePasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangePasswordDialog(const QString& username, QWidget* parent = nullptr);

public slots:
    void onGeneratePassword();
    void onCopyPassword();
    void onChangePassword();

private:
    void setupUi();
    
    QString username_;
    
    QLineEdit* currentPasswordEdit_ = nullptr;
    QLineEdit* newPasswordEdit_ = nullptr;
    QLineEdit* confirmPasswordEdit_ = nullptr;
    QCheckBox* showPasswordCheck_ = nullptr;
    QCheckBox* forceChangeCheck_ = nullptr;
};

// ============================================================================
// User Objects Dialog
// ============================================================================
class UserObjectsDialog : public QDialog {
    Q_OBJECT

public:
    explicit UserObjectsDialog(const QString& username, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onReassignObjects();
    void onDropObjects();
    void onExportList();

private:
    void setupUi();
    void loadObjects();
    
    QString username_;
    backend::SessionClient* client_;
    
    QTableView* objectsTable_ = nullptr;
    QStandardItemModel* objectsModel_ = nullptr;
    QComboBox* newOwnerCombo_ = nullptr;
};

} // namespace scratchrobin::ui
