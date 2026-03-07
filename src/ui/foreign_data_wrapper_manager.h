#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTableWidget;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QCheckBox;
class QSpinBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Foreign Data Wrapper (FDW) Manager
 * 
 * Manages external data sources and foreign tables.
 * - Configure FDW extensions
 * - Create/manage foreign servers
 * - Create/manage user mappings
 * - Create/manage foreign tables
 * - Import foreign schemas
 */

// ============================================================================
// FDW Info Structures
// ============================================================================
struct FdwInfo {
    QString name;
    QString handler;
    QString validator;
    QStringList options;
};

struct ForeignServerInfo {
    QString name;
    QString fdwName;
    QString host;
    QString database;
    int port = 0;
    QStringList options;
};

struct UserMappingInfo {
    QString localUser;
    QString serverName;
    QString remoteUser;
    bool hasPassword = false;
};

struct ForeignTableInfo {
    QString name;
    QString schema;
    QString serverName;
    QString remoteTable;
    QStringList columns;
};

// ============================================================================
// Foreign Data Wrapper Manager Panel
// ============================================================================
class ForeignDataWrapperManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ForeignDataWrapperManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Foreign Data Wrappers"); }
    QString panelCategory() const override { return "database"; }
    
    void refresh();

public slots:
    void onCreateFDW();
    void onCreateServer();
    void onCreateUserMapping();
    void onCreateForeignTable();
    void onImportForeignSchema();
    void onDropObject();
    void onTestConnection();
    void onFilterChanged(const QString& filter);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void setupModels();
    void loadFDWs();
    void loadServers();
    void loadUserMappings();
    void loadForeignTables();
    
    backend::SessionClient* client_;
    
    QTabWidget* tabWidget_ = nullptr;
    
    // FDW tab
    QTreeView* fdwTree_ = nullptr;
    QStandardItemModel* fdwModel_ = nullptr;
    
    // Servers tab
    QTreeView* serverTree_ = nullptr;
    QStandardItemModel* serverModel_ = nullptr;
    
    // User mappings tab
    QTreeView* mappingTree_ = nullptr;
    QStandardItemModel* mappingModel_ = nullptr;
    
    // Foreign tables tab
    QTreeView* tableTree_ = nullptr;
    QStandardItemModel* tableModel_ = nullptr;
    
    QLineEdit* filterEdit_ = nullptr;
};

// ============================================================================
// Create FDW Dialog
// ============================================================================
class CreateFdwDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateFdwDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();

private:
    void setupUi();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* handlerCombo_ = nullptr;
    QComboBox* validatorCombo_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Create Foreign Server Dialog
// ============================================================================
class CreateForeignServerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateForeignServerDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();
    void onTestConnection();

private:
    void setupUi();
    void loadFDWs();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* serverNameEdit_ = nullptr;
    QComboBox* fdwCombo_ = nullptr;
    QLineEdit* hostEdit_ = nullptr;
    QSpinBox* portSpin_ = nullptr;
    QLineEdit* databaseEdit_ = nullptr;
    QTextEdit* optionsEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Create User Mapping Dialog
// ============================================================================
class CreateUserMappingDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateUserMappingDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();

private:
    void setupUi();
    void loadServers();
    void loadUsers();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QComboBox* userCombo_ = nullptr;
    QComboBox* serverCombo_ = nullptr;
    QLineEdit* remoteUserEdit_ = nullptr;
    QLineEdit* remotePasswordEdit_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Create Foreign Table Dialog
// ============================================================================
class CreateForeignTableDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateForeignTableDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onCreate();
    void onLoadRemoteTables();

private:
    void setupUi();
    void loadServers();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QLineEdit* tableNameEdit_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* serverCombo_ = nullptr;
    QLineEdit* remoteTableEdit_ = nullptr;
    QTableWidget* columnsTable_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

// ============================================================================
// Import Foreign Schema Dialog
// ============================================================================
class ImportForeignSchemaDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImportForeignSchemaDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onPreview();
    void onImport();
    void onLoadRemoteSchemas();

private:
    void setupUi();
    void loadServers();
    QString generateDdl();
    
    backend::SessionClient* client_;
    
    QComboBox* serverCombo_ = nullptr;
    QComboBox* remoteSchemaCombo_ = nullptr;
    QComboBox* localSchemaCombo_ = nullptr;
    QLineEdit* tableListEdit_ = nullptr;
    QCheckBox* exceptCheck_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
};

} // namespace scratchrobin::ui
