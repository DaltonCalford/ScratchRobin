#include "backup_restore_dialog.h"
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSettings>
#include <QTimer>
#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

namespace scratchrobin {

BackupRestoreDialog::BackupRestoreDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Database Backup & Restore");
    setMinimumSize(900, 700);
    resize(1100, 800);

    // Load sample backup history
    loadSampleHistory();
}

void BackupRestoreDialog::setCurrentDatabase(const QString& databaseName) {
    currentDatabase_ = databaseName;
    if (databaseLabel_) {
        databaseLabel_->setText(QString("Database: %1").arg(databaseName));
    }
}

void BackupRestoreDialog::setAvailableTables(const QStringList& tables) {
    availableTables_ = tables;
}

void BackupRestoreDialog::setAvailableSchemas(const QStringList& schemas) {
    availableSchemas_ = schemas;
}

void BackupRestoreDialog::setBackupHistory(const QList<BackupInfo>& history) {
    backupHistory_ = history;
    populateBackupHistory();
}

void BackupRestoreDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    databaseLabel_ = new QLabel("Database: Not Connected");
    databaseLabel_->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(databaseLabel_);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Tab widget
    tabWidget_ = new QTabWidget();
    setupBackupTab();
    setupRestoreTab();
    setupHistoryTab();
    setupScheduleTab();

    connect(tabWidget_, &QTabWidget::currentChanged, this, &BackupRestoreDialog::onTabChanged);
    mainLayout->addWidget(tabWidget_);

    // Progress bar
    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    progressBar_->setRange(0, 100);
    mainLayout->addWidget(progressBar_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* helpButton = new QPushButton("Help");
    helpButton->setIcon(QIcon(":/icons/help.png"));
    buttonLayout->addWidget(helpButton);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);
}

void BackupRestoreDialog::setupBackupTab() {
    backupTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(backupTab_);

    // Backup configuration
    QGroupBox* configGroup = new QGroupBox("Backup Configuration");
    QFormLayout* configLayout = new QFormLayout(configGroup);

    backupTypeCombo_ = new QComboBox();
    backupTypeCombo_->addItems(backupTypes_);
    connect(backupTypeCombo_, &QComboBox::currentTextChanged, this, &BackupRestoreDialog::onBackupTypeChanged);
    configLayout->addRow("Backup Type:", backupTypeCombo_);

    QHBoxLayout* fileLayout = new QHBoxLayout();
    backupFilePathEdit_ = new QLineEdit();
    backupFilePathEdit_->setPlaceholderText("Select backup file location...");
    fileLayout->addWidget(backupFilePathEdit_);

    backupBrowseButton_ = new QPushButton("Browse...");
    connect(backupBrowseButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onBrowseBackupFile);
    fileLayout->addWidget(backupBrowseButton_);

    configLayout->addRow("Output File:", fileLayout);

    backupFormatCombo_ = new QComboBox();
    backupFormatCombo_->addItems(backupFormats_);
    configLayout->addRow("Format:", backupFormatCombo_);

    layout->addWidget(configGroup);

    // Objects to backup
    QGroupBox* objectsGroup = new QGroupBox("Objects to Backup");
    QVBoxLayout* objectsLayout = new QVBoxLayout(objectsGroup);

    backupIncludeDataCheck_ = new QCheckBox("Include table data");
    backupIncludeDataCheck_->setChecked(true);
    objectsLayout->addWidget(backupIncludeDataCheck_);

    backupIncludeIndexesCheck_ = new QCheckBox("Include indexes");
    backupIncludeIndexesCheck_->setChecked(true);
    objectsLayout->addWidget(backupIncludeIndexesCheck_);

    backupIncludeConstraintsCheck_ = new QCheckBox("Include constraints");
    backupIncludeConstraintsCheck_->setChecked(true);
    objectsLayout->addWidget(backupIncludeConstraintsCheck_);

    backupIncludeTriggersCheck_ = new QCheckBox("Include triggers");
    backupIncludeTriggersCheck_->setChecked(true);
    objectsLayout->addWidget(backupIncludeTriggersCheck_);

    backupIncludeViewsCheck_ = new QCheckBox("Include views");
    backupIncludeViewsCheck_->setChecked(true);
    objectsLayout->addWidget(backupIncludeViewsCheck_);

    layout->addWidget(objectsGroup);

    // Options
    QGroupBox* optionsGroup = new QGroupBox("Backup Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    QHBoxLayout* compressionRow = new QHBoxLayout();
    backupCompressCheck_ = new QCheckBox("Compress backup");
    backupCompressCheck_->setChecked(true);
    compressionRow->addWidget(backupCompressCheck_);

    compressionRow->addWidget(new QLabel("Level:"));
    backupCompressionLevelCombo_ = new QComboBox();
    backupCompressionLevelCombo_->addItems(compressionLevels_);
    backupCompressionLevelCombo_->setCurrentText("Medium");
    compressionRow->addWidget(backupCompressionLevelCombo_);
    compressionRow->addStretch();

    optionsLayout->addLayout(compressionRow);

    QHBoxLayout* encryptionRow = new QHBoxLayout();
    backupEncryptCheck_ = new QCheckBox("Encrypt backup");
    encryptionRow->addWidget(backupEncryptCheck_);

    encryptionRow->addWidget(new QLabel("Password:"));
    backupPasswordEdit_ = new QLineEdit();
    backupPasswordEdit_->setEchoMode(QLineEdit::Password);
    backupPasswordEdit_->setEnabled(false);
    connect(backupEncryptCheck_, &QCheckBox::toggled, backupPasswordEdit_, &QLineEdit::setEnabled);
    encryptionRow->addWidget(backupPasswordEdit_);

    optionsLayout->addLayout(encryptionRow);

    backupVerifyCheck_ = new QCheckBox("Verify backup after creation");
    backupVerifyCheck_->setChecked(true);
    optionsLayout->addWidget(backupVerifyCheck_);

    layout->addWidget(optionsGroup);

    // Comment
    QGroupBox* commentGroup = new QGroupBox("Backup Comment");
    QVBoxLayout* commentLayout = new QVBoxLayout(commentGroup);

    backupCommentEdit_ = new QTextEdit();
    backupCommentEdit_->setMaximumHeight(80);
    backupCommentEdit_->setPlaceholderText("Optional comment for this backup...");
    commentLayout->addWidget(backupCommentEdit_);

    layout->addWidget(commentGroup);

    // Create backup button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    createBackupButton_ = new QPushButton("Create Backup");
    createBackupButton_->setIcon(QIcon(":/icons/backup.png"));
    createBackupButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 10px 20px; border-radius: 6px; font-weight: bold; font-size: 12px; } QPushButton:hover { background-color: #45a049; }");
    connect(createBackupButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onCreateBackup);
    buttonLayout->addWidget(createBackupButton_);
    layout->addLayout(buttonLayout);

    tabWidget_->addTab(backupTab_, "Backup");
}

void BackupRestoreDialog::setupRestoreTab() {
    restoreTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(restoreTab_);

    // Restore configuration
    QGroupBox* configGroup = new QGroupBox("Restore Configuration");
    QFormLayout* configLayout = new QFormLayout(configGroup);

    QHBoxLayout* fileLayout = new QHBoxLayout();
    restoreFilePathEdit_ = new QLineEdit();
    restoreFilePathEdit_->setPlaceholderText("Select backup file to restore...");
    fileLayout->addWidget(restoreFilePathEdit_);

    restoreBrowseButton_ = new QPushButton("Browse...");
    connect(restoreBrowseButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onBrowseRestoreFile);
    fileLayout->addWidget(restoreBrowseButton_);

    configLayout->addRow("Backup File:", fileLayout);

    restoreModeCombo_ = new QComboBox();
    restoreModeCombo_->addItems({"Full Restore", "Schema Only", "Data Only", "Custom Restore"});
    configLayout->addRow("Restore Mode:", restoreModeCombo_);

    layout->addWidget(configGroup);

    // Objects to restore
    QGroupBox* objectsGroup = new QGroupBox("Objects to Restore");
    QVBoxLayout* objectsLayout = new QVBoxLayout(objectsGroup);

    restoreDropExistingCheck_ = new QCheckBox("Drop existing objects before restore");
    objectsLayout->addWidget(restoreDropExistingCheck_);

    restoreCreateSchemasCheck_ = new QCheckBox("Create schemas");
    restoreCreateSchemasCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateSchemasCheck_);

    restoreCreateTablesCheck_ = new QCheckBox("Create tables");
    restoreCreateTablesCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateTablesCheck_);

    restoreCreateIndexesCheck_ = new QCheckBox("Create indexes");
    restoreCreateIndexesCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateIndexesCheck_);

    restoreCreateConstraintsCheck_ = new QCheckBox("Create constraints");
    restoreCreateConstraintsCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateConstraintsCheck_);

    restoreCreateTriggersCheck_ = new QCheckBox("Create triggers");
    restoreCreateTriggersCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateTriggersCheck_);

    restoreCreateViewsCheck_ = new QCheckBox("Create views");
    restoreCreateViewsCheck_->setChecked(true);
    objectsLayout->addWidget(restoreCreateViewsCheck_);

    layout->addWidget(objectsGroup);

    // Restore options
    QGroupBox* optionsGroup = new QGroupBox("Restore Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    QHBoxLayout* conflictRow = new QHBoxLayout();
    conflictRow->addWidget(new QLabel("Conflict Resolution:"));
    restoreConflictCombo_ = new QComboBox();
    restoreConflictCombo_->addItems(conflictResolutions_);
    conflictRow->addWidget(restoreConflictCombo_);
    conflictRow->addStretch();
    optionsLayout->addLayout(conflictRow);

    restoreIgnoreErrorsCheck_ = new QCheckBox("Ignore errors during restore");
    optionsLayout->addWidget(restoreIgnoreErrorsCheck_);

    restorePreviewOnlyCheck_ = new QCheckBox("Preview only (no changes)");
    optionsLayout->addWidget(restorePreviewOnlyCheck_);

    QHBoxLayout* passwordRow = new QHBoxLayout();
    passwordRow->addWidget(new QLabel("Password (if encrypted):"));
    restorePasswordEdit_ = new QLineEdit();
    restorePasswordEdit_->setEchoMode(QLineEdit::Password);
    passwordRow->addWidget(restorePasswordEdit_);
    optionsLayout->addLayout(passwordRow);

    layout->addWidget(optionsGroup);

    // Preview
    QGroupBox* previewGroup = new QGroupBox("Backup Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    restorePreviewText_ = new QTextEdit();
    restorePreviewText_->setMaximumHeight(150);
    restorePreviewText_->setReadOnly(true);
    restorePreviewText_->setPlaceholderText("Click 'Preview Backup' to see backup contents...");
    previewLayout->addWidget(restorePreviewText_);

    layout->addWidget(previewGroup);

    // Restore button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    restoreButton_ = new QPushButton("Restore Database");
    restoreButton_->setIcon(QIcon(":/icons/restore.png"));
    restoreButton_->setStyleSheet("QPushButton { background-color: #FF9800; color: white; padding: 10px 20px; border-radius: 6px; font-weight: bold; font-size: 12px; } QPushButton:hover { background-color: #F57C00; }");
    connect(restoreButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onRestoreBackup);
    buttonLayout->addWidget(restoreButton_);
    layout->addLayout(buttonLayout);

    tabWidget_->addTab(restoreTab_, "Restore");
}

void BackupRestoreDialog::setupHistoryTab() {
    historyTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(historyTab_);

    // Backup history table
    backupHistoryTable_ = new QTableWidget();
    backupHistoryTable_->setColumnCount(6);
    backupHistoryTable_->setHorizontalHeaderLabels({
        "File Name", "Created", "Size", "Type", "Status", "Comment"
    });
    backupHistoryTable_->horizontalHeader()->setStretchLastSection(true);
    backupHistoryTable_->verticalHeader()->setVisible(false);
    backupHistoryTable_->setAlternatingRowColors(true);
    backupHistoryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(backupHistoryTable_, &QTableWidget::itemSelectionChanged, this, &BackupRestoreDialog::onBackupSelected);
    layout->addWidget(backupHistoryTable_);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    verifyBackupButton_ = new QPushButton("Verify");
    verifyBackupButton_->setIcon(QIcon(":/icons/verify.png"));
    connect(verifyBackupButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onVerifyBackup);
    buttonLayout->addWidget(verifyBackupButton_);

    deleteBackupButton_ = new QPushButton("Delete");
    deleteBackupButton_->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteBackupButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onDeleteBackup);
    buttonLayout->addWidget(deleteBackupButton_);

    buttonLayout->addStretch();

    refreshHistoryButton_ = new QPushButton("Refresh");
    refreshHistoryButton_->setIcon(QIcon(":/icons/refresh.png"));
    connect(refreshHistoryButton_, &QPushButton::clicked, this, &BackupRestoreDialog::onRefreshHistory);
    buttonLayout->addWidget(refreshHistoryButton_);

    layout->addLayout(buttonLayout);

    tabWidget_->addTab(historyTab_, "History");
}

void BackupRestoreDialog::setupScheduleTab() {
    scheduleTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(scheduleTab_);

    QGroupBox* scheduleGroup = new QGroupBox("Automated Backup Schedule");
    QFormLayout* scheduleLayout = new QFormLayout(scheduleGroup);

    scheduleEnabledCheck_ = new QCheckBox("Enable automated backups");
    scheduleLayout->addRow("", scheduleEnabledCheck_);

    QHBoxLayout* intervalLayout = new QHBoxLayout();
    scheduleIntervalSpin_ = new QSpinBox();
    scheduleIntervalSpin_->setRange(1, 365);
    scheduleIntervalSpin_->setValue(7);
    intervalLayout->addWidget(scheduleIntervalSpin_);

    scheduleUnitCombo_ = new QComboBox();
    scheduleUnitCombo_->addItems({"Days", "Weeks", "Months"});
    scheduleUnitCombo_->setCurrentText("Days");
    intervalLayout->addWidget(scheduleUnitCombo_);

    scheduleLayout->addRow("Backup Interval:", intervalLayout);

    QHBoxLayout* timeLayout = new QHBoxLayout();
    timeLayout->addWidget(new QLabel("Time:"));
    scheduleTimeEdit_ = new QTimeEdit();
    scheduleTimeEdit_->setTime(QTime(2, 0)); // 2 AM
    timeLayout->addWidget(scheduleTimeEdit_);
    timeLayout->addStretch();
    scheduleLayout->addRow("", timeLayout);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    schedulePathEdit_ = new QLineEdit();
    schedulePathEdit_->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/backups");
    pathLayout->addWidget(schedulePathEdit_);

    scheduleBrowseButton_ = new QPushButton("Browse...");
    connect(scheduleBrowseButton_, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Directory",
                                                        schedulePathEdit_->text());
        if (!dir.isEmpty()) {
            schedulePathEdit_->setText(dir);
        }
    });
    pathLayout->addWidget(scheduleBrowseButton_);

    scheduleLayout->addRow("Backup Directory:", pathLayout);

    layout->addWidget(scheduleGroup);

    // Save button
    QHBoxLayout* saveButtonLayout = new QHBoxLayout();
    saveButtonLayout->addStretch();
    scheduleSaveButton_ = new QPushButton("Save Schedule Settings");
    scheduleSaveButton_->setIcon(QIcon(":/icons/save.png"));
    connect(scheduleSaveButton_, &QPushButton::clicked, [this]() {
        QMessageBox::information(this, "Schedule Saved",
                               "Automated backup schedule has been configured.");
    });
    saveButtonLayout->addWidget(scheduleSaveButton_);
    layout->addLayout(saveButtonLayout);

    layout->addStretch();

    tabWidget_->addTab(scheduleTab_, "Schedule");
}

void BackupRestoreDialog::loadSampleHistory() {
    backupHistory_.clear();

    BackupInfo info1;
    info1.fileName = "mydb_backup_2024-12-23_14-30-15.sql";
    info1.filePath = "/home/user/backups/mydb_backup_2024-12-23_14-30-15.sql";
    info1.createdAt = QDateTime::currentDateTime().addDays(-1);
    info1.fileSize = 2.3 * 1024 * 1024; // 2.3 MB
    info1.backupType = "Full Database";
    info1.databaseName = "mydb";
    info1.comment = "Daily backup";
    info1.isCompressed = true;
    info1.isEncrypted = false;
    info1.isVerified = true;
    backupHistory_.append(info1);

    BackupInfo info2;
    info2.fileName = "mydb_schema_2024-12-22_10-15-30.sql";
    info2.filePath = "/home/user/backups/mydb_schema_2024-12-22_10-15-30.sql";
    info2.createdAt = QDateTime::currentDateTime().addDays(-2);
    info2.fileSize = 156 * 1024; // 156 KB
    info2.backupType = "Schema Only";
    info2.databaseName = "mydb";
    info2.comment = "Schema backup after table modifications";
    info2.isCompressed = false;
    info2.isEncrypted = false;
    info2.isVerified = true;
    backupHistory_.append(info2);

    populateBackupHistory();
}

void BackupRestoreDialog::populateBackupHistory() {
    backupHistoryTable_->setRowCount(0);

    for (int i = 0; i < backupHistory_.size(); ++i) {
        const BackupInfo& info = backupHistory_[i];
        backupHistoryTable_->insertRow(i);

        // File name
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.fileName);
        backupHistoryTable_->setItem(i, 0, nameItem);

        // Created
        QTableWidgetItem* createdItem = new QTableWidgetItem(info.createdAt.toString("yyyy-MM-dd HH:mm"));
        backupHistoryTable_->setItem(i, 1, createdItem);

        // Size
        QString sizeText;
        if (info.fileSize > 1024 * 1024 * 1024) {
            sizeText = QString::number(info.fileSize / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB";
        } else if (info.fileSize > 1024 * 1024) {
            sizeText = QString::number(info.fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
        } else if (info.fileSize > 1024) {
            sizeText = QString::number(info.fileSize / 1024.0, 'f', 1) + " KB";
        } else {
            sizeText = QString::number(info.fileSize) + " bytes";
        }
        QTableWidgetItem* sizeItem = new QTableWidgetItem(sizeText);
        backupHistoryTable_->setItem(i, 2, sizeItem);

        // Type
        QTableWidgetItem* typeItem = new QTableWidgetItem(info.backupType);
        backupHistoryTable_->setItem(i, 3, typeItem);

        // Status
        QString statusText;
        if (info.isVerified) {
            statusText = "Verified ✓";
        } else {
            statusText = "Not Verified";
        }
        if (info.isEncrypted) {
            statusText += " 🔒";
        }
        if (info.isCompressed) {
            statusText += " 🗜️";
        }
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        backupHistoryTable_->setItem(i, 4, statusItem);

        // Comment
        QTableWidgetItem* commentItem = new QTableWidgetItem(info.comment);
        backupHistoryTable_->setItem(i, 5, commentItem);
    }

    backupHistoryTable_->resizeColumnsToContents();
    backupHistoryTable_->horizontalHeader()->setStretchLastSection(true);
}

void BackupRestoreDialog::showBackupDetails(const BackupInfo& info) {
    // This would show detailed information about the selected backup
    // For now, just show a message
    QMessageBox::information(this, "Backup Details",
                           QString("Backup: %1\nCreated: %2\nSize: %3 bytes\nType: %4\nComment: %5")
                           .arg(info.fileName)
                           .arg(info.createdAt.toString())
                           .arg(info.fileSize)
                           .arg(info.backupType)
                           .arg(info.comment));
}

void BackupRestoreDialog::onTabChanged(int index) {
    // Update UI based on current tab
    QString tabText = tabWidget_->tabText(index);

    if (tabText == "Backup") {
        createBackupButton_->setFocus();
    } else if (tabText == "Restore") {
        restoreButton_->setFocus();
    }
}

void BackupRestoreDialog::onBackupTypeChanged(const QString& type) {
    // Update UI based on backup type selection
    if (type == "Schema Only") {
        backupIncludeDataCheck_->setChecked(false);
        backupIncludeDataCheck_->setEnabled(false);
    } else if (type == "Data Only") {
        backupIncludeDataCheck_->setChecked(true);
        backupIncludeDataCheck_->setEnabled(false);
        backupIncludeIndexesCheck_->setChecked(false);
        backupIncludeConstraintsCheck_->setChecked(false);
        backupIncludeTriggersCheck_->setChecked(false);
        backupIncludeViewsCheck_->setChecked(false);
    } else {
        backupIncludeDataCheck_->setEnabled(true);
        backupIncludeIndexesCheck_->setEnabled(true);
        backupIncludeConstraintsCheck_->setEnabled(true);
        backupIncludeTriggersCheck_->setEnabled(true);
        backupIncludeViewsCheck_->setEnabled(true);
    }
}

void BackupRestoreDialog::onBrowseBackupFile() {
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/backups";
    QDir().mkpath(defaultDir); // Ensure directory exists

    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Backup File",
        defaultDir + "/" + currentDatabase_ + "_backup_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".sql",
        "SQL Files (*.sql);;Compressed Archives (*.tar.gz);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        backupFilePathEdit_->setText(fileName);
    }
}

void BackupRestoreDialog::onBrowseRestoreFile() {
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/backups";

    QString fileName = QFileDialog::getOpenFileName(
        this, "Open Backup File",
        defaultDir,
        "All Backup Files (*.sql *.tar.gz *.backup);;SQL Files (*.sql);;Compressed Archives (*.tar.gz);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        restoreFilePathEdit_->setText(fileName);
    }
}

void BackupRestoreDialog::onCreateBackup() {
    if (backupFilePathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Backup Error", "Please select an output file for the backup.");
        return;
    }

    BackupOptions options;
    options.backupType = backupTypeCombo_->currentText();
    options.filePath = backupFilePathEdit_->text();
    options.format = backupFormatCombo_->currentText();
    options.includeData = backupIncludeDataCheck_->isChecked();
    options.includeIndexes = backupIncludeIndexesCheck_->isChecked();
    options.includeConstraints = backupIncludeConstraintsCheck_->isChecked();
    options.includeTriggers = backupIncludeTriggersCheck_->isChecked();
    options.includeViews = backupIncludeViewsCheck_->isChecked();
    options.compressBackup = backupCompressCheck_->isChecked();
    options.compressionLevel = backupCompressionLevelCombo_->currentText();
    options.encryptBackup = backupEncryptCheck_->isChecked();
    options.password = backupPasswordEdit_->text();
    options.verifyBackup = backupVerifyCheck_->isChecked();
    options.comment = backupCommentEdit_->toPlainText();

    emit backupRequested(options);
}

void BackupRestoreDialog::onRestoreBackup() {
    if (restoreFilePathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Restore Error", "Please select a backup file to restore.");
        return;
    }

    if (!QFile::exists(restoreFilePathEdit_->text())) {
        QMessageBox::warning(this, "Restore Error", "Selected backup file does not exist.");
        return;
    }

    RestoreOptions options;
    options.filePath = restoreFilePathEdit_->text();
    options.restoreMode = restoreModeCombo_->currentText();
    options.dropExistingObjects = restoreDropExistingCheck_->isChecked();
    options.createSchemas = restoreCreateSchemasCheck_->isChecked();
    options.createTables = restoreCreateTablesCheck_->isChecked();
    options.createIndexes = restoreCreateIndexesCheck_->isChecked();
    options.createConstraints = restoreCreateConstraintsCheck_->isChecked();
    options.createTriggers = restoreCreateTriggersCheck_->isChecked();
    options.createViews = restoreCreateViewsCheck_->isChecked();
    options.conflictResolution = restoreConflictCombo_->currentText();
    options.ignoreErrors = restoreIgnoreErrorsCheck_->isChecked();
    options.previewOnly = restorePreviewOnlyCheck_->isChecked();
    options.password = restorePasswordEdit_->text();

    emit restoreRequested(options);
}

void BackupRestoreDialog::onVerifyBackup() {
    QList<QTableWidgetItem*> selectedItems = backupHistoryTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Verify Error", "Please select a backup to verify.");
        return;
    }

    int row = selectedItems.first()->row();
    if (row >= 0 && row < backupHistory_.size()) {
        const BackupInfo& info = backupHistory_[row];
        emit backupVerified(info.filePath);
    }
}

void BackupRestoreDialog::onDeleteBackup() {
    QList<QTableWidgetItem*> selectedItems = backupHistoryTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Delete Error", "Please select a backup to delete.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Backup",
        "Are you sure you want to delete the selected backup file?\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        int row = selectedItems.first()->row();
        if (row >= 0 && row < backupHistory_.size()) {
            const BackupInfo& info = backupHistory_[row];
            emit backupDeleted(info.filePath);
        }
    }
}

void BackupRestoreDialog::onBackupSelected() {
    QList<QTableWidgetItem*> selectedItems = backupHistoryTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    int row = selectedItems.first()->row();
    if (row >= 0 && row < backupHistory_.size()) {
        currentBackupInfo_ = backupHistory_[row];
        showBackupDetails(currentBackupInfo_);
    }
}

void BackupRestoreDialog::onRefreshHistory() {
    // In a real implementation, this would scan the backup directory
    QMessageBox::information(this, "Refresh History", "Backup history has been refreshed.");
}

} // namespace scratchrobin
