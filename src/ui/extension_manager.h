#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTableWidget;
class QLabel;
class QCheckBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Extension/Module Manager
 * 
 * Manages database extensions/modules.
 * - Install/enable extensions
 * - Configure extension settings
 * - View extension documentation
 * - Update extensions
 * - Manage extension dependencies
 */

// ============================================================================
// Extension Info
// ============================================================================
struct ExtensionInfo {
    QString name;
    QString schema;
    QString version;
    QString installedVersion;
    bool installed = false;
    bool relocatable = false;
    QStringList requiresList;
    QString description;
    int oid = 0;
};

// ============================================================================
// Extension Manager Panel
// ============================================================================
class ExtensionManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ExtensionManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Extension Manager"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onInstallExtension();
    void onUninstallExtension();
    void onUpdateExtension();
    void onConfigureExtension();
    void onViewDocumentation();
    void onFilterChanged(const QString& filter);
    void onExtensionSelected(const QModelIndex& index);

signals:
    void extensionSelected(const QString& name);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModel();
    void loadExtensions();
    void updateDetails(const ExtensionInfo& info);
    
    backend::SessionClient* client_;
    
    QTreeView* extensionTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    
    // Details panel
    QLabel* nameLabel_ = nullptr;
    QLabel* versionLabel_ = nullptr;
    QLabel* installedLabel_ = nullptr;
    QLabel* relocatableLabel_ = nullptr;
    QLabel* requiresLabel_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QTextEdit* ddlPreview_ = nullptr;
};

// ============================================================================
// Install Extension Dialog
// ============================================================================
class InstallExtensionDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstallExtensionDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onInstall();

private:
    void setupUi();
    void loadAvailableExtensions();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QComboBox* extensionCombo_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* versionCombo_ = nullptr;
    QCheckBox* cascadeCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Configure Extension Dialog
// ============================================================================
class ConfigureExtensionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureExtensionDialog(backend::SessionClient* client,
                                     const QString& extensionName,
                                     QWidget* parent = nullptr);

public slots:
    void onSave();
    void onReset();

private:
    void setupUi();
    void loadSettings();
    
    backend::SessionClient* client_;
    QString extensionName_;
    
    QTableWidget* settingsTable_ = nullptr;
};

} // namespace scratchrobin::ui
