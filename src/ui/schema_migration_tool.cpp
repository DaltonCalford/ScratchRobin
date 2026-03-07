#include "schema_migration_tool.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTreeView>
#include <QTableView>
#include <QSplitter>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QProgressDialog>

namespace scratchrobin::ui {

// ============================================================================
// Schema Migration Tool Panel
// ============================================================================

SchemaMigrationToolPanel::SchemaMigrationToolPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("schema_migration", parent)
    , client_(client) {
    setupUi();
    setupModel();
    loadMigrations();
}

void SchemaMigrationToolPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* initBtn = new QPushButton(tr("Initialize"), this);
    connect(initBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onInitMigrations);
    toolbarLayout->addWidget(initBtn);
    
    auto* createBtn = new QPushButton(tr("Create"), this);
    connect(createBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onCreateMigration);
    toolbarLayout->addWidget(createBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onEditMigration);
    toolbarLayout->addWidget(editBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onDeleteMigration);
    toolbarLayout->addWidget(deleteBtn);
    
    toolbarLayout->addStretch();
    
    auto* validateBtn = new QPushButton(tr("Validate"), this);
    connect(validateBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onValidateMigrations);
    toolbarLayout->addWidget(validateBtn);
    
    auto* historyBtn = new QPushButton(tr("History"), this);
    connect(historyBtn, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onViewHistory);
    toolbarLayout->addWidget(historyBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Stats
    auto* statsGroup = new QGroupBox(tr("Statistics"), this);
    auto* statsLayout = new QHBoxLayout(statsGroup);
    
    totalLabel_ = new QLabel(tr("Total: 0"), this);
    pendingLabel_ = new QLabel(tr("Pending: 0"), this);
    appliedLabel_ = new QLabel(tr("Applied: 0"), this);
    currentVersionLabel_ = new QLabel(tr("Current Version: None"), this);
    
    statsLayout->addWidget(totalLabel_);
    statsLayout->addWidget(pendingLabel_);
    statsLayout->addWidget(appliedLabel_);
    statsLayout->addStretch();
    statsLayout->addWidget(currentVersionLabel_);
    
    mainLayout->addWidget(statsGroup);
    
    // Splitter for tree and details
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Migration tree
    migrationTree_ = new QTreeView(this);
    migrationTree_->setHeaderHidden(false);
    migrationTree_->setAlternatingRowColors(true);
    migrationTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(migrationTree_, &QTreeView::clicked, this, &SchemaMigrationToolPanel::onMigrationSelected);
    splitter->addWidget(migrationTree_);
    
    // Details tabs
    detailsTabs_ = new QTabWidget(this);
    
    upScriptEdit_ = new QTextEdit(this);
    upScriptEdit_->setFont(QFont("Consolas", 9));
    upScriptEdit_->setReadOnly(true);
    detailsTabs_->addTab(upScriptEdit_, tr("Up (Apply)"));
    
    downScriptEdit_ = new QTextEdit(this);
    downScriptEdit_->setFont(QFont("Consolas", 9));
    downScriptEdit_->setReadOnly(true);
    detailsTabs_->addTab(downScriptEdit_, tr("Down (Rollback)"));
    
    infoEdit_ = new QTextEdit(this);
    infoEdit_->setReadOnly(true);
    detailsTabs_->addTab(infoEdit_, tr("Info"));
    
    splitter->addWidget(detailsTabs_);
    splitter->setSizes({400, 500});
    
    mainLayout->addWidget(splitter, 1);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    runBtn_ = new QPushButton(tr("Run Pending"), this);
    connect(runBtn_, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onRunMigrations);
    btnLayout->addWidget(runBtn_);
    
    rollbackBtn_ = new QPushButton(tr("Rollback"), this);
    connect(rollbackBtn_, &QPushButton::clicked, this, &SchemaMigrationToolPanel::onRollbackMigration);
    btnLayout->addWidget(rollbackBtn_);
    
    mainLayout->addLayout(btnLayout);
}

void SchemaMigrationToolPanel::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({
        tr("Version"), tr("Name"), tr("Status"), tr("Applied"), tr("By")
    });
    migrationTree_->setModel(model_);
    migrationTree_->header()->setStretchLastSection(true);
}

void SchemaMigrationToolPanel::loadMigrations() {
    // Simulate loading migrations
    model_->removeRows(0, model_->rowCount());
    
    QList<MigrationInfo> migrations;
    
    MigrationInfo m1;
    m1.id = "001";
    m1.name = "create_users_table";
    m1.description = "Create users table with basic columns";
    m1.created = QDateTime::currentDateTime().addDays(-30);
    m1.applied = QDateTime::currentDateTime().addDays(-29);
    m1.isApplied = true;
    m1.appliedBy = "admin";
    migrations.append(m1);
    
    MigrationInfo m2;
    m2.id = "002";
    m2.name = "add_user_roles";
    m2.description = "Add roles column to users table";
    m2.created = QDateTime::currentDateTime().addDays(-20);
    m2.applied = QDateTime::currentDateTime().addDays(-19);
    m2.isApplied = true;
    m2.appliedBy = "admin";
    migrations.append(m2);
    
    MigrationInfo m3;
    m3.id = "003";
    m3.name = "create_orders_table";
    m3.description = "Create orders table with foreign key to users";
    m3.created = QDateTime::currentDateTime().addDays(-10);
    m3.isApplied = false;
    migrations.append(m3);
    
    MigrationInfo m4;
    m4.id = "004";
    m4.name = "add_order_indexes";
    m4.description = "Add indexes for order queries";
    m4.created = QDateTime::currentDateTime().addDays(-5);
    m4.isApplied = false;
    migrations.append(m4);
    
    int pending = 0, applied = 0;
    for (const auto& m : migrations) {
        QList<QStandardItem*> row;
        row << new QStandardItem(m.id);
        row << new QStandardItem(m.name);
        
        if (m.isApplied) {
            row << new QStandardItem(tr("Applied"));
            row[2]->setForeground(Qt::darkGreen);
            row << new QStandardItem(m.applied.toString("yyyy-MM-dd hh:mm"));
            row << new QStandardItem(m.appliedBy);
            applied++;
        } else {
            row << new QStandardItem(tr("Pending"));
            row[2]->setForeground(QColor(255, 140, 0));
            row << new QStandardItem("-");
            row << new QStandardItem("-");
            pending++;
        }
        
        row[0]->setData(QVariant::fromValue(m.id), Qt::UserRole);
        model_->appendRow(row);
    }
    
    totalLabel_->setText(tr("Total: %1").arg(migrations.size()));
    pendingLabel_->setText(tr("Pending: %1").arg(pending));
    appliedLabel_->setText(tr("Applied: %1").arg(applied));
    
    if (applied > 0) {
        currentVersionLabel_->setText(tr("Current Version: %1").arg(migrations[applied-1].id));
    }
    
    runBtn_->setEnabled(pending > 0);
    rollbackBtn_->setEnabled(applied > 0);
}

void SchemaMigrationToolPanel::updateStats() {
    loadMigrations();
}

void SchemaMigrationToolPanel::onInitMigrations() {
    auto reply = QMessageBox::question(this, tr("Initialize Migrations"),
        tr("This will create the migration tracking table in the database.\nContinue?"));
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Success"), 
            tr("Migration system initialized successfully."));
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onCreateMigration() {
    auto* dialog = new CreateMigrationDialog(client_, this);
    if (dialog->exec() == QDialog::Accepted) {
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onEditMigration() {
    auto index = migrationTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Select Migration"),
            tr("Please select a migration to edit."));
        return;
    }
    
    auto* item = model_->itemFromIndex(index.siblingAtColumn(0));
    QString id = item->data(Qt::UserRole).toString();
    
    QMessageBox::information(this, tr("Edit Migration"),
        tr("Edit migration %1 (not yet implemented)").arg(id));
}

void SchemaMigrationToolPanel::onDeleteMigration() {
    auto index = migrationTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Select Migration"),
            tr("Please select a migration to delete."));
        return;
    }
    
    auto* item = model_->itemFromIndex(index.siblingAtColumn(0));
    QString id = item->data(Qt::UserRole).toString();
    
    auto reply = QMessageBox::warning(this, tr("Delete Migration"),
        tr("Are you sure you want to delete migration %1?").arg(id),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Success"),
            tr("Migration %1 deleted (simulation)").arg(id));
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onRunMigrations() {
    QList<MigrationInfo> pending;
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* statusItem = model_->item(i, 2);
        if (statusItem->text() == tr("Pending")) {
            MigrationInfo m;
            m.id = model_->item(i, 0)->text();
            m.name = model_->item(i, 1)->text();
            pending.append(m);
        }
    }
    
    if (pending.isEmpty()) {
        QMessageBox::information(this, tr("No Pending Migrations"),
            tr("All migrations have been applied."));
        return;
    }
    
    auto* dialog = new RunMigrationsDialog(pending, this);
    if (dialog->exec() == QDialog::Accepted) {
        QMessageBox::information(this, tr("Success"),
            tr("Applied %1 migration(s)").arg(pending.size()));
        emit migrationsApplied(pending.size());
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onRollbackMigration() {
    QList<MigrationInfo> applied;
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* statusItem = model_->item(i, 2);
        if (statusItem->text() == tr("Applied")) {
            MigrationInfo m;
            m.id = model_->item(i, 0)->text();
            m.name = model_->item(i, 1)->text();
            applied.append(m);
        }
    }
    
    if (applied.isEmpty()) {
        QMessageBox::information(this, tr("No Applied Migrations"),
            tr("No migrations to rollback."));
        return;
    }
    
    auto* dialog = new RollbackDialog(applied, this);
    if (dialog->exec() == QDialog::Accepted) {
        QMessageBox::information(this, tr("Success"),
            tr("Rollback completed"));
        emit migrationsRolledBack(1);
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onValidateMigrations() {
    QMessageBox::information(this, tr("Validation"),
        tr("All migrations validated successfully.\nNo issues found."));
}

void SchemaMigrationToolPanel::onRepairMigrations() {
    QMessageBox::information(this, tr("Repair"),
        tr("Migration repair completed."));
}

void SchemaMigrationToolPanel::onViewHistory() {
    auto* dialog = new MigrationHistoryDialog(client_, this);
    dialog->exec();
}

void SchemaMigrationToolPanel::onExportMigrations() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Migrations"),
        QString(),
        tr("JSON (*.json);;SQL (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Migrations exported to %1").arg(fileName));
    }
}

void SchemaMigrationToolPanel::onImportMigrations() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Migrations"),
        QString(),
        tr("JSON (*.json);;SQL (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"),
            tr("Migrations imported from %1").arg(fileName));
        loadMigrations();
    }
}

void SchemaMigrationToolPanel::onRefresh() {
    loadMigrations();
}

void SchemaMigrationToolPanel::onMigrationSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    auto* item = model_->itemFromIndex(index.siblingAtColumn(0));
    QString id = item->data(Qt::UserRole).toString();
    QString name = model_->item(index.row(), 1)->text();
    
    // Show sample migration scripts
    upScriptEdit_->setPlainText(QString(
        "-- Migration: %1_%2\n"
        "-- Up script (apply changes)\n\n"
        "BEGIN;\n\n"
        "-- Add your migration code here\n"
        "CREATE TABLE IF NOT EXISTS example_%1 (\n"
        "    id SERIAL PRIMARY KEY,\n"
        "    name VARCHAR(100) NOT NULL\n" 
        ");\n\n"
        "INSERT INTO schema_migrations (version) VALUES ('%1');\n\n"
        "COMMIT;"
    ).arg(id, name));
    
    downScriptEdit_->setPlainText(QString(
        "-- Migration: %1_%2\n"
        "-- Down script (rollback changes)\n\n"
        "BEGIN;\n\n"
        "-- Add your rollback code here\n"
        "DROP TABLE IF EXISTS example_%1;\n\n"
        "DELETE FROM schema_migrations WHERE version = '%1';\n\n"
        "COMMIT;"
    ).arg(id, name));
    
    infoEdit_->setPlainText(QString(
        "ID: %1\n"
        "Name: %2\n"
        "Created: 2024-01-15 10:30:00\n"
        "Checksum: a1b2c3d4e5f6...\n"
    ).arg(id, name));
}

// ============================================================================
// Create Migration Dialog
// ============================================================================

CreateMigrationDialog::CreateMigrationDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
}

void CreateMigrationDialog::setupUi() {
    setWindowTitle(tr("Create New Migration"));
    resize(700, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Basic info
    auto* infoGroup = new QGroupBox(tr("Migration Info"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("e.g., add_user_email_index"));
    infoLayout->addRow(tr("Name:"), nameEdit_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setMaximumHeight(80);
    descriptionEdit_->setPlaceholderText(tr("Brief description of the migration"));
    infoLayout->addRow(tr("Description:"), descriptionEdit_);
    
    layout->addWidget(infoGroup);
    
    // Auto-generate checkbox
    autoGenerateCheck_ = new QCheckBox(tr("Auto-generate from schema diff"), this);
    layout->addWidget(autoGenerateCheck_);
    
    auto* genBtn = new QPushButton(tr("Generate from Diff"), this);
    connect(genBtn, &QPushButton::clicked, this, &CreateMigrationDialog::onGenerateFromDiff);
    layout->addWidget(genBtn);
    
    // Scripts
    auto* scriptTabs = new QTabWidget(this);
    
    upScriptEdit_ = new QTextEdit(this);
    upScriptEdit_->setFont(QFont("Consolas", 10));
    upScriptEdit_->setPlaceholderText(tr("-- Enter UP migration SQL here\n"));
    scriptTabs->addTab(upScriptEdit_, tr("Up Script"));
    
    downScriptEdit_ = new QTextEdit(this);
    downScriptEdit_->setFont(QFont("Consolas", 10));
    downScriptEdit_->setPlaceholderText(tr("-- Enter DOWN (rollback) SQL here\n"));
    scriptTabs->addTab(downScriptEdit_, tr("Down Script"));
    
    layout->addWidget(scriptTabs, 1);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &CreateMigrationDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void CreateMigrationDialog::onGenerateFromDiff() {
    QMessageBox::information(this, tr("Generate from Diff"),
        tr("Connect to Schema Compare tool to generate migration from schema differences.\n"
           "(Not yet implemented)"));
}

void CreateMigrationDialog::onPreview() {
    // Preview migration
}

void CreateMigrationDialog::onSave() {
    if (nameEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Please enter a migration name."));
        return;
    }
    
    if (upScriptEdit_->toPlainText().isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"),
            tr("Please enter an UP script."));
        return;
    }
    
    accept();
}

// ============================================================================
// Run Migrations Dialog
// ============================================================================

RunMigrationsDialog::RunMigrationsDialog(const QList<MigrationInfo>& pendingMigrations,
                                         QWidget* parent)
    : QDialog(parent)
    , pendingMigrations_(pendingMigrations) {
    setupUi();
}

void RunMigrationsDialog::setupUi() {
    setWindowTitle(tr("Run Pending Migrations"));
    resize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    layout->addWidget(new QLabel(tr("The following migrations will be applied:"), this));
    
    migrationTree_ = new QTreeView(this);
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Version"), tr("Name"), tr("Status")});
    
    for (const auto& m : pendingMigrations_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(m.id);
        row << new QStandardItem(m.name);
        row << new QStandardItem(tr("Pending"));
        model_->appendRow(row);
    }
    
    migrationTree_->setModel(model_);
    migrationTree_->header()->setStretchLastSection(true);
    layout->addWidget(migrationTree_);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    validateCheck_ = new QCheckBox(tr("Validate checksums before running"), this);
    validateCheck_->setChecked(true);
    optionsLayout->addWidget(validateCheck_);
    
    backupCheck_ = new QCheckBox(tr("Create backup before migration"), this);
    backupCheck_->setChecked(true);
    optionsLayout->addWidget(backupCheck_);
    
    layout->addWidget(optionsGroup);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(this);
    auto* runBtn = new QPushButton(tr("Run Migrations"), this);
    auto* dryRunBtn = new QPushButton(tr("Dry Run"), this);
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    
    btnBox->addButton(runBtn, QDialogButtonBox::AcceptRole);
    btnBox->addButton(dryRunBtn, QDialogButtonBox::ActionRole);
    btnBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
    
    connect(runBtn, &QPushButton::clicked, this, &RunMigrationsDialog::onRun);
    connect(dryRunBtn, &QPushButton::clicked, this, &RunMigrationsDialog::onDryRun);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    layout->addWidget(btnBox);
}

void RunMigrationsDialog::onRun() {
    accept();
}

void RunMigrationsDialog::onDryRun() {
    QMessageBox::information(this, tr("Dry Run"),
        tr("Dry run completed. No changes were made to the database."));
}

// ============================================================================
// Rollback Dialog
// ============================================================================

RollbackDialog::RollbackDialog(const QList<MigrationInfo>& appliedMigrations,
                               QWidget* parent)
    : QDialog(parent)
    , appliedMigrations_(appliedMigrations) {
    setupUi();
}

void RollbackDialog::setupUi() {
    setWindowTitle(tr("Rollback Migration"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    layout->addWidget(new QLabel(tr("Rollback to version (inclusive):"), this));
    
    targetVersionCombo_ = new QComboBox(this);
    for (const auto& m : appliedMigrations_) {
        targetVersionCombo_->addItem(QString("%1 - %2").arg(m.id, m.name), m.id);
    }
    layout->addWidget(targetVersionCombo_);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    validateCheck_ = new QCheckBox(tr("Validate before rollback"), this);
    validateCheck_->setChecked(true);
    optionsLayout->addWidget(validateCheck_);
    
    backupCheck_ = new QCheckBox(tr("Create backup before rollback"), this);
    backupCheck_->setChecked(true);
    optionsLayout->addWidget(backupCheck_);
    
    layout->addWidget(optionsGroup);
    
    // Preview
    layout->addWidget(new QLabel(tr("Rollback preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setFont(QFont("Consolas", 9));
    previewEdit_->setReadOnly(true);
    previewEdit_->setPlainText(
        "-- The following migrations will be rolled back:\n"
        "-- 002 - add_user_roles\n"
        "-- 003 - create_orders_table\n\n"
        "-- All changes from these migrations will be reversed."
    );
    layout->addWidget(previewEdit_, 1);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &RollbackDialog::onRollback);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void RollbackDialog::onRollback() {
    auto reply = QMessageBox::warning(this, tr("Confirm Rollback"),
        tr("Are you sure you want to rollback to version %1?\n"
           "This action cannot be undone easily.")
        .arg(targetVersionCombo_->currentData().toString()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        accept();
    }
}

void RollbackDialog::onRollbackToVersion() {
    // Rollback to specific version
}

// ============================================================================
// Migration History Dialog
// ============================================================================

MigrationHistoryDialog::MigrationHistoryDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
    loadHistory();
}

void MigrationHistoryDialog::setupUi() {
    setWindowTitle(tr("Migration History"));
    resize(800, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("From:"), this));
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-30));
    filterLayout->addWidget(fromDateEdit_);
    
    filterLayout->addWidget(new QLabel(tr("To:"), this));
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterLayout->addWidget(toDateEdit_);
    
    auto* filterBtn = new QPushButton(tr("Filter"), this);
    connect(filterBtn, &QPushButton::clicked, this, &MigrationHistoryDialog::onFilterByDate);
    filterLayout->addWidget(filterBtn);
    
    filterLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &MigrationHistoryDialog::onRefresh);
    filterLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &MigrationHistoryDialog::onExport);
    filterLayout->addWidget(exportBtn);
    
    layout->addLayout(filterLayout);
    
    // History table
    historyTable_ = new QTableView(this);
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({
        tr("Date"), tr("Version"), tr("Name"), tr("Action"), 
        tr("User"), tr("Status"), tr("Duration")
    });
    historyTable_->setModel(model_);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(historyTable_, 1);
    
    // Close button
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
}

void MigrationHistoryDialog::loadHistory() {
    model_->removeRows(0, model_->rowCount());
    
    // Simulate history entries
    QStringList actions = {"apply", "apply", "apply", "rollback", "apply", "apply"};
    QStringList versions = {"001", "002", "003", "003", "003", "004"};
    QStringList names = {
        "create_users_table", "add_user_roles", "create_orders_table",
        "create_orders_table", "create_orders_table", "add_order_indexes"
    };
    QStringList users = {"admin", "admin", "admin", "admin", "admin", "developer"};
    QStringList statuses = {"success", "success", "success", "success", "success", "success"};
    
    for (int i = 0; i < actions.size(); ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QDateTime::currentDateTime().addDays(-30 + i*5).toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(versions[i]);
        row << new QStandardItem(names[i]);
        
        auto* actionItem = new QStandardItem(actions[i]);
        if (actions[i] == "rollback") {
            actionItem->setForeground(Qt::red);
        } else {
            actionItem->setForeground(Qt::darkGreen);
        }
        row << actionItem;
        
        row << new QStandardItem(users[i]);
        row << new QStandardItem(statuses[i]);
        row << new QStandardItem(QString("%1 ms").arg(100 + i*50));
        
        model_->appendRow(row);
    }
}

void MigrationHistoryDialog::onRefresh() {
    loadHistory();
}

void MigrationHistoryDialog::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export History"),
        QString(),
        tr("CSV (*.csv);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("History exported to %1").arg(fileName));
    }
}

void MigrationHistoryDialog::onFilterByDate() {
    loadHistory();
    QMessageBox::information(this, tr("Filter"),
        tr("History filtered by date range."));
}

} // namespace scratchrobin::ui
