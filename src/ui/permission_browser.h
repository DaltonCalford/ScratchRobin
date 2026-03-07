#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
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
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Permission Browser - Visual GRANT/REVOKE permission management
 * 
 * Visual permission management interface for database objects:
 * - Browse permissions by object
 * - Grant and revoke privileges
 * - View effective permissions
 * - Export permission scripts
 */

// ============================================================================
// Permission Info
// ============================================================================
struct PermissionInfo {
    QString grantee;
    QString objectType;
    QString objectName;
    QString privilege;
    bool isGrantable = false;
    bool withHierarchy = false;
};

// ============================================================================
// Permission Browser Panel
// ============================================================================
class PermissionBrowserPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PermissionBrowserPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Permission Browser"); }
    QString panelCategory() const override { return "security"; }

public slots:
    // Permission operations
    void onGrantPermission();
    void onRevokePermission();
    void onRefreshPermissions();
    
    // Filtering
    void onFilterChanged(int index);
    void onSearch(const QString& text);
    
    // Selection
    void onObjectSelected(const QModelIndex& index);
    
    // Actions
    void onEditGrant();
    void onViewEffective();
    void onAnalyze();
    void onExportPermissions();
    void onShowDDL();

signals:
    void permissionGranted(const QString& grantee, const QString& privilege);
    void permissionRevoked(const QString& grantee, const QString& privilege);

private:
    void setupUi();
    void loadObjects();
    void loadPermissions();
    void updatePermissionsTable();
    
    backend::SessionClient* client_;
    QList<PermissionInfo> permissions_;
    
    // UI
    QComboBox* filterCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    QTreeView* objectTree_ = nullptr;
    QStandardItemModel* objectModel_ = nullptr;
    
    QTableView* permTable_ = nullptr;
    QStandardItemModel* permModel_ = nullptr;
    
    QTextEdit* detailsEdit_ = nullptr;
};

// ============================================================================
// Grant Permission Dialog
// ============================================================================
class GrantPermissionDialog : public QDialog {
    Q_OBJECT

public:
    explicit GrantPermissionDialog(PermissionInfo* perm, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onGrant();
    void onShowHelp();

private:
    void setupUi();
    
    PermissionInfo* perm_;
    
    QComboBox* objectTypeCombo_ = nullptr;
    QLineEdit* objectNameEdit_ = nullptr;
    QComboBox* granteeCombo_ = nullptr;
    QComboBox* privilegeCombo_ = nullptr;
    QCheckBox* grantableCheck_ = nullptr;
    QCheckBox* hierarchyCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Revoke Permission Dialog
// ============================================================================
class RevokePermissionDialog : public QDialog {
    Q_OBJECT

public:
    explicit RevokePermissionDialog(PermissionInfo* perm, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onRevoke();
    void onShowHelp();

private:
    void setupUi();
    
    PermissionInfo* perm_;
    
    QComboBox* objectTypeCombo_ = nullptr;
    QLineEdit* objectNameEdit_ = nullptr;
    QComboBox* granteeCombo_ = nullptr;
    QComboBox* privilegeCombo_ = nullptr;
    QCheckBox* cascadeCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

} // namespace scratchrobin::ui
