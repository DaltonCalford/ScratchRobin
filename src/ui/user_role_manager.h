#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QCheckBox;
class QListWidget;
class QSpinBox;
class QDateTimeEdit;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
class ScratchBirdMetadataProvider;
}

namespace scratchrobin::ui {

/**
 * @brief User/Role Manager - Security principal management
 * 
 * - List users, roles, and groups
 * - Create/edit/drop users and roles
 * - Manage role memberships and inheritances
 * - Set passwords and authentication
 * - Configure connection limits and validity periods
 */

// ============================================================================
// User Info
// ============================================================================
struct UserInfo {
    QString name;
    bool isRole = false;
    bool isGroup = false;
    bool isUser = false;
    
    // Privilege flags
    bool superuser = false;
    bool createdb = false;
    bool createrole = false;
    bool inherit = true;
    bool login = true;
    bool replication = false;
    bool bypassrls = false;
    
    // Connection settings
    int connectionLimit = -1;  // -1 = unlimited
    QString password;
    QDateTime validUntil;
    
    // Memberships
    QStringList memberOf;      // Roles this user/role is a member of
    QStringList members;       // Users/roles that are members of this role
    
    // Attributes
    QString description;
    QDateTime created;
};

// ============================================================================
// User/Role Manager Panel
// ============================================================================
class UserRoleManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit UserRoleManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("User/Role Manager"); }
    QString panelCategory() const override { return "security"; }
    
    void refresh();

public slots:
    void onCreateUser();
    void onCreateRole();
    void onCreateGroup();
    void onEditPrincipal();
    void onDropPrincipal();
    void onChangePassword();
    void onManageMemberships();
    void onGrantPrivileges();

    void onFilterChanged(const QString& filter);

signals:
    void principalSelected(const QString& name);
    void privilegesRequested(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadPrincipals();
    void updateDetails();
    
    backend::SessionClient* client_;
    
    QTreeView* principalTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
    
    QPushButton* createUserBtn_ = nullptr;
    QPushButton* createRoleBtn_ = nullptr;
    QPushButton* createGroupBtn_ = nullptr;
    QPushButton* editBtn_ = nullptr;
    QPushButton* dropBtn_ = nullptr;
    QPushButton* passwordBtn_ = nullptr;
    QPushButton* membershipBtn_ = nullptr;
    QPushButton* privilegesBtn_ = nullptr;
};

// ============================================================================
// User/Role Editor Dialog
// ============================================================================
class UserRoleEditorDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { CreateUser, CreateRole, CreateGroup, Edit };
    
    explicit UserRoleEditorDialog(backend::SessionClient* client,
                                  Mode mode,
                                  const QString& principalName = QString(),
                                  QWidget* parent = nullptr);

public slots:
    void onValidate();
    void onSave();
    void onGeneratePassword();
    void onCopyPassword();

private:
    void setupUi();
    void loadPrincipal(const QString& name);
    QString generateDdl();
    
    backend::SessionClient* client_;
    Mode mode_;
    QString originalName_;
    bool isEditing_ = false;
    QString generatedPassword_;
    
    // Identity
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;  // USER, ROLE, GROUP
    
    // Privileges
    QCheckBox* superuserCheck_ = nullptr;
    QCheckBox* createdbCheck_ = nullptr;
    QCheckBox* createroleCheck_ = nullptr;
    QCheckBox* inheritCheck_ = nullptr;
    QCheckBox* loginCheck_ = nullptr;
    QCheckBox* replicationCheck_ = nullptr;
    QCheckBox* bypassrlsCheck_ = nullptr;
    
    // Connection
    QSpinBox* connLimitSpin_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QPushButton* generatePwdBtn_ = nullptr;
    QPushButton* copyPwdBtn_ = nullptr;
    QDateTimeEdit* validUntilEdit_ = nullptr;
    
    // Memberships
    QListWidget* memberOfList_ = nullptr;
    QListWidget* availableRolesList_ = nullptr;
    
    // Preview
    QTextEdit* sqlPreview_ = nullptr;
};

// ============================================================================
// Membership Manager Dialog
// ============================================================================
class MembershipManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit MembershipManagerDialog(backend::SessionClient* client,
                                    const QString& principalName,
                                    QWidget* parent = nullptr);

public slots:
    void onAddToRole();
    void onRemoveFromRole();
    void onAddMember();
    void onRemoveMember();
    void onRefresh();

private:
    void setupUi();
    void loadMemberships();
    
    backend::SessionClient* client_;
    QString principalName_;
    
    // Principal's memberships (roles this principal is a member of)
    QListWidget* memberOfList_ = nullptr;
    QListWidget* availableRolesList_ = nullptr;
    
    // Members of this principal (if it's a role/group)
    QListWidget* membersList_ = nullptr;
    QListWidget* availableMembersList_ = nullptr;
};

// ============================================================================
// Password Change Dialog
// ============================================================================
class PasswordChangeDialog : public QDialog {
    Q_OBJECT

public:
    explicit PasswordChangeDialog(backend::SessionClient* client,
                                 const QString& userName,
                                 QWidget* parent = nullptr);

public slots:
    void onGenerate();
    void onCopy();
    void onSave();

private:
    void setupUi();
    QString generateSecurePassword();
    
    backend::SessionClient* client_;
    QString userName_;
    
    QLineEdit* currentPwdEdit_ = nullptr;
    QLineEdit* newPwdEdit_ = nullptr;
    QLineEdit* confirmPwdEdit_ = nullptr;
    QPushButton* generateBtn_ = nullptr;
    QPushButton* copyBtn_ = nullptr;
    QCheckBox* expireCheck_ = nullptr;
};

// ============================================================================
// Effective Privileges Dialog
// ============================================================================
class EffectivePrivilegesDialog : public QDialog {
    Q_OBJECT

public:
    explicit EffectivePrivilegesDialog(backend::SessionClient* client,
                                      const QString& principalName,
                                      QWidget* parent = nullptr);

private:
    void setupUi();
    void loadPrivileges();
    
    backend::SessionClient* client_;
    QString principalName_;
    
    QTableView* privilegesTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QTextEdit* sqlPreview_ = nullptr;
};

} // namespace scratchrobin::ui
