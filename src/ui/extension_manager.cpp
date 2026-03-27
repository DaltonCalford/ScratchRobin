#include "ui/extension_manager.h"
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
#include <QSplitter>
#include <QTableWidget>
#include <QToolBar>
#include <QLabel>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>

namespace scratchrobin::ui {

// ============================================================================
// Extension Manager Panel
// ============================================================================
ExtensionManagerPanel::ExtensionManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("extension_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void ExtensionManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* installBtn = new QPushButton(tr("Install"), this);
    auto* uninstallBtn = new QPushButton(tr("Uninstall"), this);
    auto* updateBtn = new QPushButton(tr("Update"), this);
    auto* configureBtn = new QPushButton(tr("Configure"), this);
    auto* docsBtn = new QPushButton(tr("Documentation"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(installBtn);
    toolbar->addWidget(uninstallBtn);
    toolbar->addWidget(updateBtn);
    toolbar->addWidget(configureBtn);
    toolbar->addWidget(docsBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(installBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::onInstallExtension);
    connect(uninstallBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::onUninstallExtension);
    connect(updateBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::onUpdateExtension);
    connect(configureBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::onConfigureExtension);
    connect(docsBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::onViewDocumentation);
    connect(refreshBtn, &QPushButton::clicked, this, &ExtensionManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter extensions..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ExtensionManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Extension tree
    extensionTree_ = new QTreeView(this);
    extensionTree_->setAlternatingRowColors(true);
    extensionTree_->setSortingEnabled(true);
    
    splitter->addWidget(extensionTree_);
    
    // Right: Details panel
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* detailsGroup = new QGroupBox(tr("Extension Details"), this);
    auto* detailsLayout = new QFormLayout(detailsGroup);
    
    nameLabel_ = new QLabel("-", this);
    versionLabel_ = new QLabel("-", this);
    installedLabel_ = new QLabel("-", this);
    relocatableLabel_ = new QLabel("-", this);
    requiresLabel_ = new QLabel("-", this);
    
    detailsLayout->addRow(tr("Name:"), nameLabel_);
    detailsLayout->addRow(tr("Version:"), versionLabel_);
    detailsLayout->addRow(tr("Installed:"), installedLabel_);
    detailsLayout->addRow(tr("Relocatable:"), relocatableLabel_);
    detailsLayout->addRow(tr("Requires:"), requiresLabel_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setReadOnly(true);
    descriptionEdit_->setMaximumHeight(100);
    detailsLayout->addRow(tr("Description:"), descriptionEdit_);
    
    rightLayout->addWidget(detailsGroup);
    
    ddlPreview_ = new QTextEdit(this);
    ddlPreview_->setReadOnly(true);
    ddlPreview_->setPlaceholderText(tr("DDL Preview..."));
    rightLayout->addWidget(ddlPreview_, 1);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({400, 400});
    
    mainLayout->addWidget(splitter);
}

void ExtensionManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(4);
    model_->setHorizontalHeaderLabels({tr("Extension"), tr("Version"), tr("Schema"), tr("Installed")});
    extensionTree_->setModel(model_);
    extensionTree_->header()->setStretchLastSection(false);
    extensionTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    // Connect selection model AFTER model is set
    connect(extensionTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ExtensionManagerPanel::onExtensionSelected);
}

void ExtensionManagerPanel::refresh()
{
    loadExtensions();
}

void ExtensionManagerPanel::panelActivated()
{
    refresh();
}

void ExtensionManagerPanel::loadExtensions()
{
    model_->removeRows(0, model_->rowCount());
    
    if (!client_) return;
    
    // Query installed extensions from pg_extension
    std::string sql = 
        "SELECT e.extname, e.extversion, n.nspname, c.description "
        "FROM pg_extension e "
        "JOIN pg_namespace n ON n.oid = e.extnamespace "
        "LEFT JOIN pg_description c ON c.objoid = e.oid "
        "ORDER BY e.extname";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 4) continue;
            
            QList<QStandardItem*> items;
            items << new QStandardItem(QString::fromStdString(row[0])); // Name
            items << new QStandardItem(QString::fromStdString(row[1])); // Version
            items << new QStandardItem(QString::fromStdString(row[2])); // Schema
            items << new QStandardItem(QString::fromStdString(row[3])); // Description
            
            // Determine status based on version (simplified)
            items << new QStandardItem(tr("Active"));
            
            model_->appendRow(items);
        }
    }
}

void ExtensionManagerPanel::onInstallExtension()
{
    InstallExtensionDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void ExtensionManagerPanel::onUninstallExtension()
{
    auto index = extensionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    auto reply = QMessageBox::question(this, tr("Uninstall Extension"),
        tr("Uninstall extension '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (client_) {
            // Note: Actual uninstall requires DROP EXTENSION
            std::string sql = QString("DROP EXTENSION IF EXISTS %1").arg(name).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        refresh();
    }
}

void ExtensionManagerPanel::onUpdateExtension()
{
    auto index = extensionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    auto reply = QMessageBox::question(this, tr("Update Extension"),
        tr("Update extension '%1' to latest version?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Note: Extension updates typically require:
        // ALTER EXTENSION name UPDATE TO 'new_version'
        QMessageBox::information(this, tr("Update Extension"),
            tr("Extension updates should be performed manually via SQL:\n"
               "ALTER EXTENSION %1 UPDATE TO 'new_version';").arg(name));
        refresh();
    }
}

void ExtensionManagerPanel::onConfigureExtension()
{
    auto index = extensionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    ConfigureExtensionDialog dialog(client_, name, this);
    dialog.exec();
}

void ExtensionManagerPanel::onViewDocumentation()
{
    auto index = extensionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    
    // Open PostgreSQL documentation for this extension
    QString url = QString("https://www.postgresql.org/docs/current/%1.html").arg(name);
    QDesktopServices::openUrl(QUrl(url));
}

void ExtensionManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < model_->rowCount(); ++i) {
        QString name = model_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        extensionTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void ExtensionManagerPanel::onExtensionSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    QString name = model_->item(index.row(), 0)->text();
    emit extensionSelected(name);
}

void ExtensionManagerPanel::updateDetails(const ExtensionInfo& info)
{
    nameLabel_->setText(info.name);
    versionLabel_->setText(info.version);
    installedLabel_->setText(info.installed ? tr("Yes") : tr("No"));
    installedLabel_->setStyleSheet(info.installed ? "color: green;" : "color: gray;");
    relocatableLabel_->setText(info.relocatable ? tr("Yes") : tr("No"));
    requiresLabel_->setText(info.requiresList.join(", "));
    descriptionEdit_->setPlainText(info.description);
    
    // Generate DDL
    QString ddl;
    if (info.installed) {
        ddl = QString("DROP EXTENSION %1;").arg(info.name);
    } else {
        ddl = QString("CREATE EXTENSION %1;").arg(info.name);
    }
    ddlPreview_->setPlainText(ddl);
}

// ============================================================================
// Install Extension Dialog
// ============================================================================
InstallExtensionDialog::InstallExtensionDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Install Extension"));
    setMinimumSize(400, 300);
    setupUi();
}

void InstallExtensionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    extensionCombo_ = new QComboBox(this);
    extensionCombo_->setEditable(true);
    formLayout->addRow(tr("Extension:"), extensionCombo_);
    
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    formLayout->addRow(tr("Schema:"), schemaCombo_);
    
    versionCombo_ = new QComboBox(this);
    versionCombo_->setEditable(true);
    versionCombo_->addItem(tr("(latest)"));
    formLayout->addRow(tr("Version:"), versionCombo_);
    
    cascadeCheck_ = new QCheckBox(tr("Cascade (install required extensions)"), this);
    formLayout->addRow(QString(), cascadeCheck_);
    
    mainLayout->addLayout(formLayout);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &InstallExtensionDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &InstallExtensionDialog::onInstall);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    loadAvailableExtensions();
}

void InstallExtensionDialog::loadAvailableExtensions()
{
    if (!client_) {
        // Add common extensions as defaults
        extensionCombo_->addItem("postgis");
        extensionCombo_->addItem("uuid-ossp");
        extensionCombo_->addItem("pgcrypto");
        extensionCombo_->addItem("hstore");
        extensionCombo_->addItem("pg_stat_statements");
        extensionCombo_->addItem("pg_trgm");
        return;
    }
    
    // Query available extensions from pg_available_extensions
    std::string sql = 
        "SELECT name, default_version, comment "
        "FROM pg_available_extensions "
        "WHERE installed_version IS NULL "
        "ORDER BY name";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() > 0) {
                extensionCombo_->addItem(QString::fromStdString(row[0]));
            }
        }
    }
    
    if (extensionCombo_->count() == 0) {
        // Fallback: add common extensions
        extensionCombo_->addItem("postgis");
        extensionCombo_->addItem("uuid-ossp");
        extensionCombo_->addItem("pgcrypto");
        extensionCombo_->addItem("hstore");
    }
}

void InstallExtensionDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void InstallExtensionDialog::onInstall()
{
    if (client_) {
        std::string sql = generateDdl().toStdString();
        auto response = client_->ExecuteSql(4044, "scratchbird", sql);
        
        if (!response.status.ok) {
            QMessageBox::warning(this, tr("Installation Failed"),
                                 tr("Failed to install extension: %1")
                                 .arg(QString::fromStdString(response.status.message)));
            return;
        }
    }
    accept();
}

QString InstallExtensionDialog::generateDdl()
{
    QString sql = QString("CREATE EXTENSION %1").arg(extensionCombo_->currentText());
    
    if (schemaCombo_->currentText() != "public") {
        sql += QString(" WITH SCHEMA %1").arg(schemaCombo_->currentText());
    }
    
    if (versionCombo_->currentText() != tr("(latest)")) {
        sql += QString(" VERSION %1").arg(versionCombo_->currentText());
    }
    
    if (cascadeCheck_->isChecked()) {
        sql += " CASCADE";
    }
    
    sql += ";";
    return sql;
}

// ============================================================================
// Configure Extension Dialog
// ============================================================================
ConfigureExtensionDialog::ConfigureExtensionDialog(backend::SessionClient* client,
                                                  const QString& extensionName,
                                                  QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , extensionName_(extensionName)
{
    setWindowTitle(tr("Configure %1").arg(extensionName));
    setMinimumSize(500, 400);
    setupUi();
}

void ConfigureExtensionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    mainLayout->addWidget(new QLabel(tr("Extension: %1").arg(extensionName_)));
    
    settingsTable_ = new QTableWidget(this);
    settingsTable_->setColumnCount(2);
    settingsTable_->setHorizontalHeaderLabels({tr("Setting"), tr("Value")});
    settingsTable_->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(settingsTable_);
    
    auto* buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tr("Reset"), QDialogButtonBox::ResetRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigureExtensionDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    loadSettings();
}

void ConfigureExtensionDialog::loadSettings()
{
    settingsTable_->setRowCount(0);
    
    if (!client_) return;
    
    // Query extension settings from pg_settings
    // Extension settings typically have the extension name as prefix
    std::string sql = QString(
        "SELECT name, setting, unit, short_desc "
        "FROM pg_settings "
        "WHERE name LIKE '%1.%' OR context = 'user' "
        "ORDER BY name"
    ).arg(extensionName_.toLower()).toStdString();
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        int row = 0;
        for (const auto& rowData : response.result_set.rows) {
            if (rowData.size() < 4) continue;
            
            settingsTable_->insertRow(row);
            settingsTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(rowData[0])));
            
            QString value = QString::fromStdString(rowData[1]);
            if (!rowData[2].empty()) {
                value += " " + QString::fromStdString(rowData[2]);
            }
            settingsTable_->setItem(row, 1, new QTableWidgetItem(value));
            
            // Store description as tooltip
            settingsTable_->item(row, 0)->setToolTip(QString::fromStdString(rowData[3]));
            settingsTable_->item(row, 1)->setToolTip(QString::fromStdString(rowData[3]));
            
            row++;
        }
    }
    
    // If no settings found, show a message
    if (settingsTable_->rowCount() == 0) {
        settingsTable_->insertRow(0);
        auto* item = new QTableWidgetItem(tr("No configurable settings found for this extension"));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        settingsTable_->setItem(0, 0, item);
        settingsTable_->setItem(0, 1, new QTableWidgetItem(""));
    }
}

void ConfigureExtensionDialog::onSave()
{
    if (!client_) {
        accept();
        return;
    }
    
    // Apply changed settings
    bool hasErrors = false;
    for (int row = 0; row < settingsTable_->rowCount(); ++row) {
        QString name = settingsTable_->item(row, 0)->text();
        QString value = settingsTable_->item(row, 1)->text();
        
        // Skip the "no settings" row
        if (name.startsWith(tr("No configurable"))) continue;
        
        std::string sql = QString("ALTER SYSTEM SET %1 = '%2'").arg(name).arg(value).toStdString();
        auto response = client_->ExecuteSql(4044, "scratchbird", sql);
        
        if (!response.status.ok) {
            hasErrors = true;
            QMessageBox::warning(this, tr("Setting Failed"),
                                 tr("Failed to set %1: %2")
                                 .arg(name)
                                 .arg(QString::fromStdString(response.status.message)));
        }
    }
    
    if (!hasErrors) {
        // Reload to confirm changes
        QMessageBox::information(this, tr("Settings Saved"),
                                 tr("Extension settings have been updated. "
                                    "Some changes may require a server restart."));
    }
    
    accept();
}

void ConfigureExtensionDialog::onReset()
{
    loadSettings();
}

} // namespace scratchrobin::ui
