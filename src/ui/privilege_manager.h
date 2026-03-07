#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QCheckBox;
class QListWidget;
class QSplitter;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Privilege Manager - Grant/Revoke privileges on database objects
 * 
 * - Browse privileges by object or by grantee
 * - Grant privileges with fine-grained control
 * - Revoke privileges
 * - View effective privileges (including inherited)
 * - Manage WITH GRANT OPTION
 */

// ============================================================================
// Privilege Info
// ============================================================================
struct PrivilegeInfo {
    QString grantee;           // User/role receiving privilege
    QString grantor;           // User/role who granted it
    QString objectName;        // Object name
    QString objectType;        // TABLE, VIEW, FUNCTION, etc.
    QStringList privileges;    // SELECT, INSERT, etc.
    bool withGrantOption = false;
    bool isInherited = false;  // From role membership
    QString inheritedFrom;     // Role providing inheritance
};

// ============================================================================
// Privilege Manager Panel
// ============================================================================
class PrivilegeManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PrivilegeManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Privilege Manager"); }
    QString panelCategory() const override { return "security"; }
    
    void refresh();

public slots:
    void onGrant();
    void onRevoke();
    void onShowEffective();
    void onFilterChanged(const QString& filter);

signals:
    void privilegesModified();

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadObjects();
    void loadGrantees();
    void loadPrivilegesByObject(const QString& objectName);
    void loadPrivilegesByGrantee(const QString& grantee);
    
    backend::SessionClient* client_;
    
    // Filter
    QLineEdit* filterEdit_ = nullptr;
    
    // Tabs
    QTabWidget* tabWidget_ = nullptr;
    
    // By Object view
    QTreeView* byObjectTree_ = nullptr;
    QStandardItemModel* byObjectModel_ = nullptr;
    
    // By Grantee view
    QTreeView* byGranteeTree_ = nullptr;
    QStandardItemModel* byGranteeModel_ = nullptr;
    QPushButton* revokeBtn_ = nullptr;
    QPushButton* effectiveBtn_ = nullptr;
    
    QString currentObject_;
    QString currentObjectType_;
    QString currentGrantee_;
};

// ============================================================================
// Grant Dialog
// ============================================================================
class GrantDialog : public QDialog {
    Q_OBJECT

public:
    explicit GrantDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onGrant();
    void onPreview();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QComboBox* objectTypeCombo_ = nullptr;
    QComboBox* objectNameCombo_ = nullptr;
    QComboBox* granteeTypeCombo_ = nullptr;
    QLineEdit* granteeEdit_ = nullptr;
    
    QCheckBox* selectCheck_ = nullptr;
    QCheckBox* insertCheck_ = nullptr;
    QCheckBox* updateCheck_ = nullptr;
    QCheckBox* deleteCheck_ = nullptr;
    QCheckBox* allCheck_ = nullptr;
    QCheckBox* withGrantOption_ = nullptr;
    
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Revoke Dialog
// ============================================================================
class RevokeDialog : public QDialog {
    Q_OBJECT

public:
    explicit RevokeDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRevoke();
    void onPreview();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QComboBox* objectTypeCombo_ = nullptr;
    QLineEdit* objectNameEdit_ = nullptr;
    QComboBox* granteeTypeCombo_ = nullptr;
    QLineEdit* granteeEdit_ = nullptr;
    QLineEdit* privilegesEdit_ = nullptr;
    QCheckBox* cascadeCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Effective Permissions Dialog
// ============================================================================
class EffectivePermissionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit EffectivePermissionsDialog(backend::SessionClient* client,
                                        QWidget* parent = nullptr);

public slots:
    void onCalculate();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QComboBox* userCombo_ = nullptr;
    QLineEdit* objectEdit_ = nullptr;
    QTreeView* resultsTree_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
};

} // namespace scratchrobin::ui
