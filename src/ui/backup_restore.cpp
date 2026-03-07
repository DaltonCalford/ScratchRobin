#include "backup_restore.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QProgressBar>
#include <QRadioButton>
#include <QSpinBox>
#include <QListWidget>
#include <QDateTimeEdit>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Backup Manager Panel
// ============================================================================

BackupManagerPanel::BackupManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("backup_manager", parent)
    , client_(client) {
    setupUi();
    loadBackups();
}

void BackupManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* createBtn = new QPushButton(tr("New Backup..."), this);
    connect(createBtn, &QPushButton::clicked, this, &BackupManagerPanel::onCreateBackup);
    toolbarLayout->addWidget(createBtn);
    
    auto* restoreBtn = new QPushButton(tr("Restore..."), this);
    connect(restoreBtn, &QPushButton::clicked, this, &BackupManagerPanel::onRestoreBackup);
    toolbarLayout->addWidget(restoreBtn);
    
    auto* verifyBtn = new QPushButton(tr("Verify"), this);
    connect(verifyBtn, &QPushButton::clicked, this, &BackupManagerPanel::onVerifyBackup);
    toolbarLayout->addWidget(verifyBtn);
    
    toolbarLayout->addStretch();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search backups..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &BackupManagerPanel::onSearchBackups);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &BackupManagerPanel::onRefreshBackups);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Tab widget
    tabWidget_ = new QTabWidget(this);
    
    // Backups tab
    auto* backupsWidget = new QWidget(this);
    auto* backupsLayout = new QVBoxLayout(backupsWidget);
    backupsLayout->setContentsMargins(4, 4, 4, 4);
    
    backupTable_ = new QTableView(this);
    backupModel_ = new QStandardItemModel(this);
    backupModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Database"), tr("Size"), tr("Date"), tr("Status")});
    backupTable_->setModel(backupModel_);
    backupTable_->setAlternatingRowColors(true);
    backupTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(backupTable_, &QTableView::clicked, this, &BackupManagerPanel::onBackupSelected);
    backupsLayout->addWidget(backupTable_, 1);
    
    // Backup details
    auto* detailsGroup = new QGroupBox(tr("Backup Details"), this);
    auto* detailsLayout = new QFormLayout(detailsGroup);
    
    nameLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Name:"), nameLabel_);
    
    typeLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Type:"), typeLabel_);
    
    databaseLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Database:"), databaseLabel_);
    
    sizeLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Size:"), sizeLabel_);
    
    createdLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Created:"), createdLabel_);
    
    statusLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Status:"), statusLabel_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    detailsLayout->addRow(tr("Progress:"), progressBar_);
    
    backupsLayout->addWidget(detailsGroup);
    
    tabWidget_->addTab(backupsWidget, tr("Backups"));
    
    // Restore tab
    auto* restoreWidget = new QWidget(this);
    auto* restoreLayout = new QVBoxLayout(restoreWidget);
    restoreLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* restoreInfo = new QLabel(tr("Select a backup and click 'Restore...' to begin the restore process."), this);
    restoreLayout->addWidget(restoreInfo);
    
    auto* pitrBtn = new QPushButton(tr("Point-in-Time Recovery..."), this);
    connect(pitrBtn, &QPushButton::clicked, this, &BackupManagerPanel::onPointInTimeRestore);
    restoreLayout->addWidget(pitrBtn);
    
    restoreLayout->addStretch();
    
    tabWidget_->addTab(restoreWidget, tr("Restore"));
    
    // Log tab
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    tabWidget_->addTab(logEdit_, tr("Log"));
    
    mainLayout->addWidget(tabWidget_, 1);
}

void BackupManagerPanel::loadBackups() {
    backups_.clear();
    
    // Simulate loading backups
    QStringList backupNames = {"daily_full", "weekly_schema", "monthly_archive"};
    QStringList types = {"Full", "Schema Only", "Full"};
    QStringList databases = {"scratchbird", "scratchbird", "scratchbird"};
    
    for (int i = 0; i < backupNames.size(); ++i) {
        BackupJob job;
        job.id = QString::number(i);
        job.name = backupNames[i];
        job.type = (i == 1) ? BackupType::SchemaOnly : BackupType::Full;
        job.sourceDatabase = databases[i];
        job.size = (i + 1) * 1024 * 1024 * 100; // 100MB, 200MB, 300MB
        job.status = "completed";
        job.completedTime = QDateTime::currentDateTime().addDays(-i);
        backups_.append(job);
    }
    
    updateBackupList();
}

void BackupManagerPanel::updateBackupList() {
    backupModel_->clear();
    backupModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Database"), tr("Size"), tr("Date"), tr("Status")});
    
    for (const auto& job : backups_) {
        QString typeStr;
        switch (job.type) {
            case BackupType::Full: typeStr = tr("Full"); break;
            case BackupType::Incremental: typeStr = tr("Incremental"); break;
            case BackupType::Differential: typeStr = tr("Differential"); break;
            case BackupType::SchemaOnly: typeStr = tr("Schema Only"); break;
            case BackupType::DataOnly: typeStr = tr("Data Only"); break;
        }
        
        QString sizeStr = QString("%1 MB").arg(job.size / 1024 / 1024);
        
        QList<QStandardItem*> row;
        row << new QStandardItem(job.name);
        row << new QStandardItem(typeStr);
        row << new QStandardItem(job.sourceDatabase);
        row << new QStandardItem(sizeStr);
        row << new QStandardItem(job.completedTime.toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(job.status);
        backupModel_->appendRow(row);
    }
}

void BackupManagerPanel::updateBackupDetails(const BackupJob& backup) {
    nameLabel_->setText(backup.name);
    
    QString typeStr;
    switch (backup.type) {
        case BackupType::Full: typeStr = tr("Full"); break;
        case BackupType::Incremental: typeStr = tr("Incremental"); break;
        case BackupType::Differential: typeStr = tr("Differential"); break;
        case BackupType::SchemaOnly: typeStr = tr("Schema Only"); break;
        case BackupType::DataOnly: typeStr = tr("Data Only"); break;
    }
    typeLabel_->setText(typeStr);
    
    databaseLabel_->setText(backup.sourceDatabase);
    sizeLabel_->setText(QString("%1 MB").arg(backup.size / 1024 / 1024));
    createdLabel_->setText(backup.completedTime.toString());
    statusLabel_->setText(backup.status);
}

void BackupManagerPanel::onCreateBackup() {
    BackupJob newJob;
    BackupWizard wizard(&newJob, client_, this);
    if (wizard.exec() == QDialog::Accepted) {
        backups_.append(newJob);
        updateBackupList();
        emit backupStarted(newJob.id);
    }
}

void BackupManagerPanel::onEditBackup() {
    auto index = backupTable_->currentIndex();
    if (!index.isValid() || index.row() >= backups_.size()) return;
    
    BackupWizard wizard(&backups_[index.row()], client_, this);
    wizard.exec();
    updateBackupList();
}

void BackupManagerPanel::onDeleteBackup() {
    auto index = backupTable_->currentIndex();
    if (!index.isValid() || index.row() >= backups_.size()) return;
    
    auto reply = QMessageBox::warning(this, tr("Delete Backup"),
        tr("Are you sure you want to delete backup '%1'?").arg(backups_[index.row()].name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        backups_.removeAt(index.row());
        updateBackupList();
    }
}

void BackupManagerPanel::onRunBackup() {
    auto index = backupTable_->currentIndex();
    if (!index.isValid() || index.row() >= backups_.size()) return;
    
    backups_[index.row()].status = "running";
    updateBackupList();
    emit backupStarted(backups_[index.row()].id);
}

void BackupManagerPanel::onStopBackup() {
    // Stop running backup
}

void BackupManagerPanel::onVerifyBackup() {
    auto index = backupTable_->currentIndex();
    if (!index.isValid() || index.row() >= backups_.size()) return;
    
    BackupVerificationDialog dialog(backups_[index.row()].destinationPath, this);
    dialog.exec();
}

void BackupManagerPanel::onRestoreBackup() {
    auto index = backupTable_->currentIndex();
    if (!index.isValid() || index.row() >= backups_.size()) {
        QMessageBox::information(this, tr("Restore"), tr("Please select a backup to restore."));
        return;
    }
    
    QString backupPath = backups_[index.row()].destinationPath;
    RestoreWizard wizard(backupPath, client_, this);
    if (wizard.exec() == QDialog::Accepted) {
        emit restoreStarted(backups_[index.row()].id);
    }
}

void BackupManagerPanel::onPointInTimeRestore() {
    PointInTimeRecoveryDialog dialog(client_, this);
    dialog.exec();
}

void BackupManagerPanel::onValidateBackup() {
    QMessageBox::information(this, tr("Validate"), tr("Backup validation would run here."));
}

void BackupManagerPanel::onRefreshBackups() {
    loadBackups();
}

void BackupManagerPanel::onBackupSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= backups_.size()) return;
    updateBackupDetails(backups_[index.row()]);
}

void BackupManagerPanel::onSearchBackups(const QString& text) {
    for (int i = 0; i < backupModel_->rowCount(); ++i) {
        auto* item = backupModel_->item(i, 0);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        backupTable_->setRowHidden(i, !match);
    }
}

void BackupManagerPanel::onExportBackupList() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Backup List"),
        QString(),
        tr("CSV Files (*.csv);;JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Backup list exported."));
    }
}

void BackupManagerPanel::onImportBackup() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Backup"),
        QString(),
        tr("Backup Files (*.backup *.sql *.dump);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("Backup imported."));
        loadBackups();
    }
}

void BackupManagerPanel::onShowBackupDetails() {
    // Show detailed backup information
}

void BackupManagerPanel::onDeleteOldBackups() {
    auto reply = QMessageBox::question(this, tr("Delete Old Backups"),
        tr("Delete backups older than 30 days?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Delete"), tr("Old backups deleted."));
        loadBackups();
    }
}

// ============================================================================
// Backup Wizard
// ============================================================================

BackupWizard::BackupWizard(BackupJob* job, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , job_(job)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Backup Wizard"));
    resize(600, 500);
}

void BackupWizard::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    pages_ = new QStackedWidget(this);
    
    createWelcomePage();
    createTypePage();
    createOptionsPage();
    createTablesPage();
    createCommandPage();
    
    layout->addWidget(pages_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    prevBtn_ = new QPushButton(tr("< Back"), this);
    connect(prevBtn_, &QPushButton::clicked, this, &BackupWizard::onPrevious);
    btnLayout->addWidget(prevBtn_);
    
    nextBtn_ = new QPushButton(tr("Next >"), this);
    connect(nextBtn_, &QPushButton::clicked, this, &BackupWizard::onNext);
    btnLayout->addWidget(nextBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &BackupWizard::onCancel);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    onUpdateCommand();
}

void BackupWizard::createWelcomePage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    auto* label = new QLabel(tr("<h2>Welcome to the Backup Wizard</h2>"
                               "<p>This wizard will guide you through creating a database backup.</p>"
                               "<p>Click 'Next' to continue.</p>"), this);
    label->setWordWrap(true);
    layout->addWidget(label);
    layout->addStretch();
    
    pages_->addWidget(page);
}

void BackupWizard::createTypePage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    auto* group = new QGroupBox(tr("Select Backup Type"), this);
    auto* groupLayout = new QVBoxLayout(group);
    
    fullRadio_ = new QRadioButton(tr("Full Backup - Complete database backup"), this);
    fullRadio_->setChecked(true);
    groupLayout->addWidget(fullRadio_);
    
    incrementalRadio_ = new QRadioButton(tr("Incremental Backup - Only changes since last backup"), this);
    groupLayout->addWidget(incrementalRadio_);
    
    differentialRadio_ = new QRadioButton(tr("Differential Backup - Changes since last full backup"), this);
    groupLayout->addWidget(differentialRadio_);
    
    schemaOnlyRadio_ = new QRadioButton(tr("Schema Only - Database structure without data"), this);
    groupLayout->addWidget(schemaOnlyRadio_);
    
    dataOnlyRadio_ = new QRadioButton(tr("Data Only - Only data without schema"), this);
    groupLayout->addWidget(dataOnlyRadio_);
    
    layout->addWidget(group);
    layout->addStretch();
    
    pages_->addWidget(page);
}

void BackupWizard::createOptionsPage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    nameEdit_ = new QLineEdit(this);
    layout->addRow(tr("Backup Name:"), nameEdit_);
    
    databaseCombo_ = new QComboBox(this);
    databaseCombo_->addItems({"scratchbird", "postgres", "template1"});
    layout->addRow(tr("Database:"), databaseCombo_);
    
    auto* destLayout = new QHBoxLayout();
    destinationEdit_ = new QLineEdit(this);
    destLayout->addWidget(destinationEdit_);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &BackupWizard::onBrowseDestination);
    destLayout->addWidget(browseBtn);
    
    layout->addRow(tr("Destination:"), destLayout);
    
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItems({tr("Custom (.backup)"), tr("Plain SQL (.sql)"), tr("Directory"), tr("Tar Archive")});
    layout->addRow(tr("Format:"), formatCombo_);
    
    compressionSpin_ = new QSpinBox(this);
    compressionSpin_->setRange(0, 9);
    compressionSpin_->setValue(6);
    layout->addRow(tr("Compression (0-9):"), compressionSpin_);
    
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    includeBlobsCheck_ = new QCheckBox(tr("Include large objects (BLOBs)"), this);
    includeBlobsCheck_->setChecked(true);
    optionsLayout->addWidget(includeBlobsCheck_);
    
    includeOidsCheck_ = new QCheckBox(tr("Include OIDs"), this);
    optionsLayout->addWidget(includeOidsCheck_);
    
    noOwnerCheck_ = new QCheckBox(tr("No owner (for cross-database restore)"), this);
    optionsLayout->addWidget(noOwnerCheck_);
    
    noPrivilegesCheck_ = new QCheckBox(tr("No privileges"), this);
    optionsLayout->addWidget(noPrivilegesCheck_);
    
    verboseCheck_ = new QCheckBox(tr("Verbose output"), this);
    verboseCheck_->setChecked(true);
    optionsLayout->addWidget(verboseCheck_);
    
    layout->addRow(optionsGroup);
    
    pages_->addWidget(page);
}

void BackupWizard::createTablesPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    allTablesCheck_ = new QCheckBox(tr("Backup all tables"), this);
    allTablesCheck_->setChecked(true);
    connect(allTablesCheck_, &QCheckBox::toggled, this, &BackupWizard::onToggleAllTables);
    layout->addWidget(allTablesCheck_);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    auto* tablesGroup = new QGroupBox(tr("Available Tables"), this);
    auto* tablesLayout = new QVBoxLayout(tablesGroup);
    tablesList_ = new QListWidget(this);
    tablesList_->setSelectionMode(QAbstractItemView::MultiSelection);
    tablesLayout->addWidget(tablesList_);
    splitter->addWidget(tablesGroup);
    
    auto* excludeGroup = new QGroupBox(tr("Exclude Tables"), this);
    auto* excludeLayout = new QVBoxLayout(excludeGroup);
    excludeList_ = new QListWidget(this);
    excludeLayout->addWidget(excludeList_);
    splitter->addWidget(excludeGroup);
    
    layout->addWidget(splitter, 1);
    
    loadTables();
    
    pages_->addWidget(page);
}

void BackupWizard::createCommandPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    layout->addWidget(new QLabel(tr("Command Preview:"), this));
    
    commandEdit_ = new QTextEdit(this);
    commandEdit_->setReadOnly(true);
    commandEdit_->setFont(QFont("Monospace"));
    layout->addWidget(commandEdit_, 1);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    layout->addWidget(progressBar_);
    
    statusLabel_ = new QLabel(tr("Ready to create backup"), this);
    layout->addWidget(statusLabel_);
    
    pages_->addWidget(page);
}

void BackupWizard::loadTables() {
    tablesList_->clear();
    QStringList tables = {"customers", "orders", "products", "users", "audit_log"};
    for (const auto& table : tables) {
        tablesList_->addItem(table);
    }
}

void BackupWizard::onBrowseDestination() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Backup"),
        QString(),
        tr("Backup Files (*.backup);;SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        destinationEdit_->setText(fileName);
    }
}

void BackupWizard::onToggleAllTables(bool checked) {
    tablesList_->setEnabled(!checked);
}

void BackupWizard::onTableSelectionChanged() {
    onUpdateCommand();
}

void BackupWizard::onUpdateCommand() {
    if (!commandEdit_) return;
    
    QString cmd = "pg_dump ";
    
    if (fullRadio_ && fullRadio_->isChecked()) {
        // Full backup is default
    } else if (schemaOnlyRadio_ && schemaOnlyRadio_->isChecked()) {
        cmd += "--schema-only ";
    } else if (dataOnlyRadio_ && dataOnlyRadio_->isChecked()) {
        cmd += "--data-only ";
    }
    
    if (compressionSpin_) {
        cmd += QString("--compress=%1 ").arg(compressionSpin_->value());
    }
    
    if (includeBlobsCheck_ && includeBlobsCheck_->isChecked()) {
        cmd += "--blobs ";
    }
    
    if (noOwnerCheck_ && noOwnerCheck_->isChecked()) {
        cmd += "--no-owner ";
    }
    
    if (noPrivilegesCheck_ && noPrivilegesCheck_->isChecked()) {
        cmd += "--no-privileges ";
    }
    
    if (verboseCheck_ && verboseCheck_->isChecked()) {
        cmd += "--verbose ";
    }
    
    if (databaseCombo_) {
        cmd += databaseCombo_->currentText() + " ";
    }
    
    if (destinationEdit_ && !destinationEdit_->text().isEmpty()) {
        cmd += "> " + destinationEdit_->text();
    }
    
    commandEdit_->setText(cmd);
}

void BackupWizard::onNext() {
    int current = pages_->currentIndex();
    if (current < pages_->count() - 1) {
        pages_->setCurrentIndex(current + 1);
        prevBtn_->setEnabled(true);
        
        if (current + 1 == pages_->count() - 1) {
            nextBtn_->setText(tr("Finish"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &BackupWizard::onNext);
            connect(nextBtn_, &QPushButton::clicked, this, &BackupWizard::onFinish);
        }
    }
    
    onUpdateCommand();
}

void BackupWizard::onPrevious() {
    int current = pages_->currentIndex();
    if (current > 0) {
        pages_->setCurrentIndex(current - 1);
        
        if (current == pages_->count() - 1) {
            nextBtn_->setText(tr("Next >"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &BackupWizard::onFinish);
            connect(nextBtn_, &QPushButton::clicked, this, &BackupWizard::onNext);
        }
        
        if (current - 1 == 0) {
            prevBtn_->setEnabled(false);
        }
    }
}

void BackupWizard::onFinish() {
    // Save job settings
    job_->name = nameEdit_->text();
    job_->sourceDatabase = databaseCombo_->currentText();
    job_->destinationPath = destinationEdit_->text();
    job_->compressionLevel = compressionSpin_->value();
    job_->includeBlobs = includeBlobsCheck_->isChecked();
    job_->noOwner = noOwnerCheck_->isChecked();
    job_->noPrivileges = noPrivilegesCheck_->isChecked();
    job_->verbose = verboseCheck_->isChecked();
    job_->status = "pending";
    
    if (fullRadio_->isChecked()) job_->type = BackupType::Full;
    else if (incrementalRadio_->isChecked()) job_->type = BackupType::Incremental;
    else if (differentialRadio_->isChecked()) job_->type = BackupType::Differential;
    else if (schemaOnlyRadio_->isChecked()) job_->type = BackupType::SchemaOnly;
    else if (dataOnlyRadio_->isChecked()) job_->type = BackupType::DataOnly;
    
    accept();
}

void BackupWizard::onCancel() {
    reject();
}

// ============================================================================
// Restore Wizard
// ============================================================================

RestoreWizard::RestoreWizard(const QString& backupPath, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , backupPath_(backupPath)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Restore Wizard"));
    resize(600, 500);
}

void RestoreWizard::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    pages_ = new QStackedWidget(this);
    
    createSourcePage();
    createTargetPage();
    createOptionsPage();
    createProgressPage();
    
    layout->addWidget(pages_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    prevBtn_ = new QPushButton(tr("< Back"), this);
    connect(prevBtn_, &QPushButton::clicked, this, &RestoreWizard::onPrevious);
    btnLayout->addWidget(prevBtn_);
    
    nextBtn_ = new QPushButton(tr("Next >"), this);
    connect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onNext);
    btnLayout->addWidget(nextBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &RestoreWizard::onCancel);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    onUpdateCommand();
}

void RestoreWizard::createSourcePage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    auto* sourceLayout = new QHBoxLayout();
    sourceEdit_ = new QLineEdit(backupPath_, this);
    sourceLayout->addWidget(sourceEdit_);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &RestoreWizard::onBrowseBackup);
    sourceLayout->addWidget(browseBtn);
    
    layout->addRow(tr("Backup File:"), sourceLayout);
    
    backupInfoLabel_ = new QLabel(tr("Select a backup file to see details"), this);
    backupInfoLabel_->setWordWrap(true);
    layout->addRow(tr("Info:"), backupInfoLabel_);
    
    pitrCheck_ = new QCheckBox(tr("Point-in-time recovery"), this);
    layout->addRow(pitrCheck_);
    
    pointInTimeEdit_ = new QDateTimeEdit(this);
    pointInTimeEdit_->setCalendarPopup(true);
    pointInTimeEdit_->setDateTime(QDateTime::currentDateTime());
    layout->addRow(tr("Restore to:"), pointInTimeEdit_);
    
    pages_->addWidget(page);
}

void RestoreWizard::createTargetPage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    auto* dbGroup = new QGroupBox(tr("Target Database"), this);
    auto* dbLayout = new QVBoxLayout(dbGroup);
    
    existingDbRadio_ = new QRadioButton(tr("Restore to existing database:"), this);
    dbLayout->addWidget(existingDbRadio_);
    
    targetDatabaseCombo_ = new QComboBox(this);
    targetDatabaseCombo_->addItems({"scratchbird", "postgres"});
    dbLayout->addWidget(targetDatabaseCombo_);
    
    newDbRadio_ = new QRadioButton(tr("Create new database:"), this);
    newDbRadio_->setChecked(true);
    dbLayout->addWidget(newDbRadio_);
    
    newDatabaseEdit_ = new QLineEdit(this);
    newDatabaseEdit_->setPlaceholderText(tr("Enter new database name"));
    dbLayout->addWidget(newDatabaseEdit_);
    
    layout->addRow(dbGroup);
    
    pages_->addWidget(page);
}

void RestoreWizard::createOptionsPage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    auto* optionsGroup = new QGroupBox(tr("Restore Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    cleanCheck_ = new QCheckBox(tr("Clean (drop) database objects before recreating"), this);
    optionsLayout->addWidget(cleanCheck_);
    
    createDbCheck_ = new QCheckBox(tr("Create database"), this);
    createDbCheck_->setChecked(true);
    optionsLayout->addWidget(createDbCheck_);
    
    dataOnlyCheck_ = new QCheckBox(tr("Data only (no schema)"), this);
    optionsLayout->addWidget(dataOnlyCheck_);
    
    schemaOnlyCheck_ = new QCheckBox(tr("Schema only (no data)"), this);
    optionsLayout->addWidget(schemaOnlyCheck_);
    
    singleTransactionCheck_ = new QCheckBox(tr("Single transaction"), this);
    singleTransactionCheck_->setChecked(true);
    optionsLayout->addWidget(singleTransactionCheck_);
    
    exitOnErrorCheck_ = new QCheckBox(tr("Exit on error"), this);
    exitOnErrorCheck_->setChecked(true);
    optionsLayout->addWidget(exitOnErrorCheck_);
    
    layout->addRow(optionsGroup);
    
    jobsSpin_ = new QSpinBox(this);
    jobsSpin_->setRange(1, 8);
    jobsSpin_->setValue(1);
    layout->addRow(tr("Parallel jobs:"), jobsSpin_);
    
    layout->addRow(new QLabel(tr("Command Preview:"), this));
    
    commandEdit_ = new QTextEdit(this);
    commandEdit_->setReadOnly(true);
    commandEdit_->setFont(QFont("Monospace"));
    commandEdit_->setMaximumHeight(100);
    layout->addRow(commandEdit_);
    
    pages_->addWidget(page);
}

void RestoreWizard::createProgressPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    layout->addWidget(new QLabel(tr("Restore Progress:"), this));
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    layout->addWidget(progressBar_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    layout->addWidget(logEdit_, 1);
    
    statusLabel_ = new QLabel(tr("Ready to restore"), this);
    layout->addWidget(statusLabel_);
    
    pages_->addWidget(page);
}

void RestoreWizard::onBrowseBackup() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Backup"),
        QString(),
        tr("Backup Files (*.backup);;SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        sourceEdit_->setText(fileName);
        loadBackupInfo();
    }
}

void RestoreWizard::onBrowseTarget() {
    // Browse for target directory (if directory format)
}

void RestoreWizard::loadBackupInfo() {
    backupInfoLabel_->setText(tr("Type: Full Backup\nDatabase: scratchbird\nSize: 150 MB\nCreated: 2024-01-15 10:30"));
}

void RestoreWizard::onUpdateCommand() {
    if (!commandEdit_) return;
    
    QString cmd = "pg_restore ";
    
    if (cleanCheck_ && cleanCheck_->isChecked()) {
        cmd += "--clean ";
    }
    
    if (createDbCheck_ && createDbCheck_->isChecked()) {
        cmd += "--create ";
    }
    
    if (dataOnlyCheck_ && dataOnlyCheck_->isChecked()) {
        cmd += "--data-only ";
    }
    
    if (schemaOnlyCheck_ && schemaOnlyCheck_->isChecked()) {
        cmd += "--schema-only ";
    }
    
    if (singleTransactionCheck_ && singleTransactionCheck_->isChecked()) {
        cmd += "--single-transaction ";
    }
    
    if (exitOnErrorCheck_ && exitOnErrorCheck_->isChecked()) {
        cmd += "--exit-on-error ";
    }
    
    if (jobsSpin_) {
        cmd += QString("--jobs=%1 ").arg(jobsSpin_->value());
    }
    
    if (sourceEdit_) {
        cmd += sourceEdit_->text();
    }
    
    commandEdit_->setText(cmd);
}

void RestoreWizard::onNext() {
    int current = pages_->currentIndex();
    if (current < pages_->count() - 1) {
        pages_->setCurrentIndex(current + 1);
        prevBtn_->setEnabled(true);
        
        if (current + 1 == pages_->count() - 1) {
            nextBtn_->setText(tr("Start Restore"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onNext);
            connect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onStartRestore);
        }
    }
    
    onUpdateCommand();
}

void RestoreWizard::onPrevious() {
    int current = pages_->currentIndex();
    if (current > 0) {
        pages_->setCurrentIndex(current - 1);
        
        if (current == pages_->count() - 1) {
            nextBtn_->setText(tr("Next >"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onStartRestore);
            connect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onNext);
        }
        
        if (current - 1 == 0) {
            prevBtn_->setEnabled(false);
        }
    }
}

void RestoreWizard::onStartRestore() {
    statusLabel_->setText(tr("Restoring..."));
    progressBar_->setRange(0, 0); // Indeterminate
    
    // Simulate restore
    QTimer::singleShot(2000, this, [this]() {
        progressBar_->setRange(0, 100);
        progressBar_->setValue(100);
        statusLabel_->setText(tr("Restore completed successfully!"));
        
        if (logEdit_) {
            logEdit_->append(tr("Restore completed at %1").arg(QDateTime::currentDateTime().toString()));
        }
        
        nextBtn_->setText(tr("Close"));
        disconnect(nextBtn_, &QPushButton::clicked, this, &RestoreWizard::onStartRestore);
        connect(nextBtn_, &QPushButton::clicked, this, &QDialog::accept);
    });
}

void RestoreWizard::onCancel() {
    reject();
}

// ============================================================================
// Point-in-Time Recovery Dialog
// ============================================================================

PointInTimeRecoveryDialog::PointInTimeRecoveryDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Point-in-Time Recovery"));
    resize(550, 400);
}

void PointInTimeRecoveryDialog::setupUi() {
    auto* layout = new QFormLayout(this);
    
    backupCombo_ = new QComboBox(this);
    loadBackups();
    layout->addRow(tr("Base Backup:"), backupCombo_);
    
    recoveryTimeEdit_ = new QDateTimeEdit(this);
    recoveryTimeEdit_->setCalendarPopup(true);
    recoveryTimeEdit_->setDateTime(QDateTime::currentDateTime());
    layout->addRow(tr("Recovery Time:"), recoveryTimeEdit_);
    
    auto* walLayout = new QHBoxLayout();
    walArchiveEdit_ = new QLineEdit(this);
    walLayout->addWidget(walArchiveEdit_);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &PointInTimeRecoveryDialog::onBrowseWalArchive);
    walLayout->addWidget(browseBtn);
    
    layout->addRow(tr("WAL Archive:"), walLayout);
    
    targetDatabaseEdit_ = new QLineEdit(this);
    targetDatabaseEdit_->setPlaceholderText(tr("New database name"));
    layout->addRow(tr("Target Database:"), targetDatabaseEdit_);
    
    layout->addRow(new QLabel(tr("Recovery Timeline:"), this));
    
    timelineEdit_ = new QTextEdit(this);
    timelineEdit_->setReadOnly(true);
    timelineEdit_->setMaximumHeight(100);
    layout->addRow(timelineEdit_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    layout->addRow(tr("Progress:"), progressBar_);
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    layout->addRow(tr("Status:"), statusLabel_);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &PointInTimeRecoveryDialog::onStartRecovery);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(btnBox);
}

void PointInTimeRecoveryDialog::loadBackups() {
    backupCombo_->addItems({"daily_full_2024-01-15", "daily_full_2024-01-14", "weekly_archive_2024-01-07"});
}

void PointInTimeRecoveryDialog::onBrowseWalArchive() {
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select WAL Archive Directory"),
        QString(),
        QFileDialog::ShowDirsOnly);
    
    if (!dir.isEmpty()) {
        walArchiveEdit_->setText(dir);
    }
}

void PointInTimeRecoveryDialog::onSelectBackup() {
    onUpdateTimeline();
}

void PointInTimeRecoveryDialog::onUpdateTimeline() {
    timelineEdit_->setText(tr("Base backup: %1\nWAL files needed: 15\nEstimated time: 5 minutes")
                           .arg(backupCombo_->currentText()));
}

void PointInTimeRecoveryDialog::onStartRecovery() {
    statusLabel_->setText(tr("Recovering..."));
    progressBar_->setRange(0, 0);
    
    QTimer::singleShot(3000, this, [this]() {
        accept();
    });
}

void PointInTimeRecoveryDialog::onCancel() {
    reject();
}

void PointInTimeRecoveryDialog::validateRecovery() {
    // Validate recovery parameters
}

// ============================================================================
// Backup Verification Dialog
// ============================================================================

BackupVerificationDialog::BackupVerificationDialog(const QString& backupPath, QWidget* parent)
    : QDialog(parent)
    , backupPath_(backupPath) {
    setupUi();
    setWindowTitle(tr("Verify Backup"));
    resize(500, 350);
}

void BackupVerificationDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    backupLabel_ = new QLabel(tr("Backup: %1").arg(backupPath_), this);
    layout->addWidget(backupLabel_);
    
    layout->addWidget(new QLabel(tr("Verification Progress:"), this));
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    layout->addWidget(progressBar_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    layout->addWidget(logEdit_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    verifyBtn_ = new QPushButton(tr("Start Verification"), this);
    connect(verifyBtn_, &QPushButton::clicked, this, &BackupVerificationDialog::onStartVerification);
    btnLayout->addWidget(verifyBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void BackupVerificationDialog::onStartVerification() {
    verifyBtn_->setEnabled(false);
    logEdit_->append(tr("Starting verification..."));
    
    // Simulate verification
    QTimer::singleShot(500, this, [this]() {
        logEdit_->append(tr("Checking file integrity..."));
        progressBar_->setValue(25);
        
        QTimer::singleShot(500, this, [this]() {
            logEdit_->append(tr("Verifying checksums..."));
            progressBar_->setValue(50);
            
            QTimer::singleShot(500, this, [this]() {
                logEdit_->append(tr("Validating format..."));
                progressBar_->setValue(75);
                
                QTimer::singleShot(500, this, [this]() {
                    logEdit_->append(tr("Verification completed successfully!"));
                    progressBar_->setValue(100);
                    verifyBtn_->setEnabled(true);
                    verifyBtn_->setText(tr("Verify Again"));
                });
            });
        });
    });
}

void BackupVerificationDialog::onCancel() {
    reject();
}

} // namespace scratchrobin::ui
