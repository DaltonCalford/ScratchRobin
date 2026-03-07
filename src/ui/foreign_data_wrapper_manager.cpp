#include "ui/foreign_data_wrapper_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QToolBar>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QTableWidget>

namespace scratchrobin::ui {

// ============================================================================
// Foreign Data Wrapper Manager Panel
// ============================================================================
ForeignDataWrapperManagerPanel::ForeignDataWrapperManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("foreign_data_wrapper_manager", parent)
    , client_(client)
{
    setupUi();
    setupModels();
    refresh();
}

void ForeignDataWrapperManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createFdwBtn = new QPushButton(tr("Create FDW"), this);
    auto* createServerBtn = new QPushButton(tr("Create Server"), this);
    auto* createMappingBtn = new QPushButton(tr("Create User Mapping"), this);
    auto* createTableBtn = new QPushButton(tr("Create Foreign Table"), this);
    auto* importBtn = new QPushButton(tr("Import Schema"), this);
    auto* dropBtn = new QPushButton(tr("Drop"), this);
    auto* testBtn = new QPushButton(tr("Test Connection"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createFdwBtn);
    toolbar->addWidget(createServerBtn);
    toolbar->addWidget(createMappingBtn);
    toolbar->addWidget(createTableBtn);
    toolbar->addWidget(importBtn);
    toolbar->addWidget(dropBtn);
    toolbar->addWidget(testBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createFdwBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::onCreateFDW);
    connect(createServerBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::onCreateServer);
    connect(createMappingBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::onCreateUserMapping);
    connect(createTableBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::onCreateForeignTable);
    connect(importBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::onImportForeignSchema);
    connect(refreshBtn, &QPushButton::clicked, this, &ForeignDataWrapperManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ForeignDataWrapperManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Tabs
    tabWidget_ = new QTabWidget(this);
    
    // FDWs tab
    fdwTree_ = new QTreeView(this);
    fdwTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(fdwTree_, tr("FDWs"));
    
    // Servers tab
    serverTree_ = new QTreeView(this);
    serverTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(serverTree_, tr("Servers"));
    
    // User Mappings tab
    mappingTree_ = new QTreeView(this);
    mappingTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(mappingTree_, tr("User Mappings"));
    
    // Foreign Tables tab
    tableTree_ = new QTreeView(this);
    tableTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(tableTree_, tr("Foreign Tables"));
    
    mainLayout->addWidget(tabWidget_);
}

void ForeignDataWrapperManagerPanel::setupModels()
{
    fdwModel_ = new QStandardItemModel(this);
    fdwTree_->setModel(fdwModel_);
    
    serverModel_ = new QStandardItemModel(this);
    serverTree_->setModel(serverModel_);
    
    mappingModel_ = new QStandardItemModel(this);
    mappingTree_->setModel(mappingModel_);
    
    tableModel_ = new QStandardItemModel(this);
    tableTree_->setModel(tableModel_);
}

void ForeignDataWrapperManagerPanel::refresh()
{
    loadFDWs();
    loadServers();
    loadUserMappings();
    loadForeignTables();
}

void ForeignDataWrapperManagerPanel::panelActivated()
{
    refresh();
}

void ForeignDataWrapperManagerPanel::loadFDWs()
{
    fdwModel_->removeRows(0, fdwModel_->rowCount());
}

void ForeignDataWrapperManagerPanel::loadServers()
{
    serverModel_->removeRows(0, serverModel_->rowCount());
}

void ForeignDataWrapperManagerPanel::loadUserMappings()
{
    mappingModel_->removeRows(0, mappingModel_->rowCount());
}

void ForeignDataWrapperManagerPanel::loadForeignTables()
{
    tableModel_->removeRows(0, tableModel_->rowCount());
}

void ForeignDataWrapperManagerPanel::onCreateFDW()
{
    CreateFdwDialog dialog(client_, this);
    dialog.exec();
}

void ForeignDataWrapperManagerPanel::onCreateServer()
{
    CreateForeignServerDialog dialog(client_, this);
    dialog.exec();
}

void ForeignDataWrapperManagerPanel::onCreateUserMapping()
{
    CreateUserMappingDialog dialog(client_, this);
    dialog.exec();
}

void ForeignDataWrapperManagerPanel::onCreateForeignTable()
{
    CreateForeignTableDialog dialog(client_, this);
    dialog.exec();
}

void ForeignDataWrapperManagerPanel::onImportForeignSchema()
{
    ImportForeignSchemaDialog dialog(client_, this);
    dialog.exec();
}

void ForeignDataWrapperManagerPanel::onDropObject()
{
    // TODO: Drop selected object
}

void ForeignDataWrapperManagerPanel::onTestConnection()
{
    // TODO: Test connection to selected server
    QMessageBox::information(this, tr("Test Connection"), tr("Connection successful."));
}

void ForeignDataWrapperManagerPanel::onFilterChanged(const QString& filter)
{
    Q_UNUSED(filter)
}

// ============================================================================
// Create FDW Dialog
// ============================================================================
CreateFdwDialog::CreateFdwDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Foreign Data Wrapper"));
    setMinimumSize(400, 300);
    setupUi();
}

void CreateFdwDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Name:"), nameEdit_);
    handlerCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Handler:"), handlerCombo_);
    validatorCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Validator:"), validatorCombo_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateFdwDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateFdwDialog::onCreate()
{
    accept();
}

QString CreateFdwDialog::generateDdl()
{
    return QString("CREATE FOREIGN DATA WRAPPER %1;").arg(nameEdit_->text());
}

// ============================================================================
// Create Foreign Server Dialog
// ============================================================================
CreateForeignServerDialog::CreateForeignServerDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Foreign Server"));
    setMinimumSize(400, 400);
    setupUi();
}

void CreateForeignServerDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    serverNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Server Name:"), serverNameEdit_);
    fdwCombo_ = new QComboBox(this);
    formLayout->addRow(tr("FDW:"), fdwCombo_);
    hostEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Host:"), hostEdit_);
    portSpin_ = new QSpinBox(this);
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5432);
    formLayout->addRow(tr("Port:"), portSpin_);
    databaseEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Database:"), databaseEdit_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateForeignServerDialog::loadFDWs()
{
}

void CreateForeignServerDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateForeignServerDialog::onCreate()
{
    accept();
}

void CreateForeignServerDialog::onTestConnection()
{
    QMessageBox::information(this, tr("Test"), tr("Connection test not implemented."));
}

QString CreateForeignServerDialog::generateDdl()
{
    return QString("CREATE SERVER %1 FOREIGN DATA WRAPPER %2 OPTIONS (host '%3', port '%4', dbname '%5');")
        .arg(serverNameEdit_->text())
        .arg(fdwCombo_->currentText())
        .arg(hostEdit_->text())
        .arg(portSpin_->value())
        .arg(databaseEdit_->text());
}

// ============================================================================
// Create User Mapping Dialog
// ============================================================================
CreateUserMappingDialog::CreateUserMappingDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create User Mapping"));
    setMinimumSize(400, 250);
    setupUi();
}

void CreateUserMappingDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    userCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Local User:"), userCombo_);
    serverCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Server:"), serverCombo_);
    remoteUserEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Remote User:"), remoteUserEdit_);
    remotePasswordEdit_ = new QLineEdit(this);
    remotePasswordEdit_->setEchoMode(QLineEdit::Password);
    formLayout->addRow(tr("Remote Password:"), remotePasswordEdit_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateUserMappingDialog::loadServers()
{
}

void CreateUserMappingDialog::loadUsers()
{
}

void CreateUserMappingDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateUserMappingDialog::onCreate()
{
    accept();
}

QString CreateUserMappingDialog::generateDdl()
{
    return QString("CREATE USER MAPPING FOR %1 SERVER %2 OPTIONS (user '%3', password '%4');")
        .arg(userCombo_->currentText())
        .arg(serverCombo_->currentText())
        .arg(remoteUserEdit_->text())
        .arg(remotePasswordEdit_->text());
}

// ============================================================================
// Create Foreign Table Dialog
// ============================================================================
CreateForeignTableDialog::CreateForeignTableDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Foreign Table"));
    setMinimumSize(500, 500);
    setupUi();
}

void CreateForeignTableDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    tableNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Table Name:"), tableNameEdit_);
    schemaCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Schema:"), schemaCombo_);
    serverCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Server:"), serverCombo_);
    remoteTableEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Remote Table:"), remoteTableEdit_);
    mainLayout->addLayout(formLayout);
    
    columnsTable_ = new QTableWidget(this);
    columnsTable_->setColumnCount(2);
    columnsTable_->setHorizontalHeaderLabels({tr("Column"), tr("Type")});
    mainLayout->addWidget(columnsTable_);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CreateForeignTableDialog::loadServers()
{
}

void CreateForeignTableDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreateForeignTableDialog::onCreate()
{
    accept();
}

void CreateForeignTableDialog::onLoadRemoteTables()
{
}

QString CreateForeignTableDialog::generateDdl()
{
    return QString("CREATE FOREIGN TABLE %1.%2 (...) SERVER %3 OPTIONS (table_name '%4');")
        .arg(schemaCombo_->currentText())
        .arg(tableNameEdit_->text())
        .arg(serverCombo_->currentText())
        .arg(remoteTableEdit_->text());
}

// ============================================================================
// Import Foreign Schema Dialog
// ============================================================================
ImportForeignSchemaDialog::ImportForeignSchemaDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Import Foreign Schema"));
    setMinimumSize(400, 350);
    setupUi();
}

void ImportForeignSchemaDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    serverCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Server:"), serverCombo_);
    remoteSchemaCombo_ = new QComboBox(this);
    remoteSchemaCombo_->setEditable(true);
    formLayout->addRow(tr("Remote Schema:"), remoteSchemaCombo_);
    localSchemaCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Local Schema:"), localSchemaCombo_);
    tableListEdit_ = new QLineEdit(this);
    tableListEdit_->setPlaceholderText(tr("table1, table2, ... (leave empty for all)"));
    formLayout->addRow(tr("Tables:"), tableListEdit_);
    exceptCheck_ = new QCheckBox(tr("Except these tables"), this);
    formLayout->addRow(QString(), exceptCheck_);
    mainLayout->addLayout(formLayout);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    mainLayout->addWidget(previewEdit_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ImportForeignSchemaDialog::loadServers()
{
}

void ImportForeignSchemaDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void ImportForeignSchemaDialog::onImport()
{
    accept();
}

void ImportForeignSchemaDialog::onLoadRemoteSchemas()
{
}

QString ImportForeignSchemaDialog::generateDdl()
{
    QString sql = QString("IMPORT FOREIGN SCHEMA %1 FROM SERVER %2 INTO %3")
        .arg(remoteSchemaCombo_->currentText())
        .arg(serverCombo_->currentText())
        .arg(localSchemaCombo_->currentText());
    
    if (!tableListEdit_->text().isEmpty()) {
        sql += exceptCheck_->isChecked() ? " EXCEPT (" : " LIMIT TO (";
        sql += tableListEdit_->text() + ")";
    }
    
    sql += ";";
    return sql;
}

} // namespace scratchrobin::ui
