#include "import_export_dialog.h"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QSettings>
#include <QMessageBox>
#include <QApplication>
#include <QSplitter>
#include <QScrollArea>
#include <QFrame>

namespace scratchrobin {

ImportExportDialog::ImportExportDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    loadSettings();
    setWindowTitle("Import / Export Database");
    setMinimumSize(800, 600);
    resize(1000, 700);
}

void ImportExportDialog::setCurrentDatabase(const QString& databaseName) {
    currentDatabase_ = databaseName;
    if (databaseLabel_) {
        databaseLabel_->setText(QString("Database: %1").arg(databaseName));
    }
}

void ImportExportDialog::setAvailableTables(const QStringList& tables) {
    availableTables_ = tables;
    exportTablesList_->clear();
    for (const QString& table : tables) {
        QListWidgetItem* item = new QListWidgetItem(table);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        exportTablesList_->addItem(item);
    }
}

void ImportExportDialog::setAvailableSchemas(const QStringList& schemas) {
    availableSchemas_ = schemas;
    exportSchemasList_->clear();
    for (const QString& schema : schemas) {
        QListWidgetItem* item = new QListWidgetItem(schema);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        exportSchemasList_->addItem(item);
    }
}

void ImportExportDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    databaseLabel_ = new QLabel("Database: Not Connected");
    databaseLabel_->setStyleSheet("font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(databaseLabel_);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Tab widget
    tabWidget_ = new QTabWidget();
    setupExportTab();
    setupImportTab();
    setupSettingsTab();

    connect(tabWidget_, &QTabWidget::currentChanged, this, &ImportExportDialog::onTabChanged);
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

    closeButton_ = new QPushButton("Close");
    closeButton_->setDefault(true);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton_);

    mainLayout->addLayout(buttonLayout);
}

void ImportExportDialog::setupExportTab() {
    exportTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(exportTab_);

    // Export format and file selection
    QGroupBox* formatGroup = new QGroupBox("Export Format & File");
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    exportFormatCombo_ = new QComboBox();
    exportFormatCombo_->addItems(exportFormats_);
    connect(exportFormatCombo_, &QComboBox::currentTextChanged,
            this, &ImportExportDialog::onExportFormatChanged);
    formatLayout->addRow("Format:", exportFormatCombo_);

    QHBoxLayout* fileLayout = new QHBoxLayout();
    exportFilePathEdit_ = new QLineEdit();
    exportFilePathEdit_->setPlaceholderText("Select output file...");
    fileLayout->addWidget(exportFilePathEdit_);

    exportBrowseButton_ = new QPushButton("Browse...");
    connect(exportBrowseButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onBrowseExportFile);
    fileLayout->addWidget(exportBrowseButton_);

    formatLayout->addRow("Output File:", fileLayout);
    layout->addWidget(formatGroup);

    // Object selection
    QGroupBox* objectsGroup = new QGroupBox("Objects to Export");
    QVBoxLayout* objectsLayout = new QVBoxLayout(objectsGroup);

    // Tables
    QHBoxLayout* tablesHeaderLayout = new QHBoxLayout();
    QLabel* tablesLabel = new QLabel("Tables:");
    tablesLabel->setStyleSheet("font-weight: bold;");
    tablesHeaderLayout->addWidget(tablesLabel);

    exportSelectAllTablesButton_ = new QPushButton("Select All");
    connect(exportSelectAllTablesButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onSelectAllTables);
    tablesHeaderLayout->addWidget(exportSelectAllTablesButton_);

    exportClearSelectionButton_ = new QPushButton("Clear");
    connect(exportClearSelectionButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onClearSelection);
    tablesHeaderLayout->addWidget(exportClearSelectionButton_);

    objectsLayout->addLayout(tablesHeaderLayout);

    exportTablesList_ = new QListWidget();
    exportTablesList_->setMaximumHeight(150);
    objectsLayout->addWidget(exportTablesList_);

    // Schemas
    QHBoxLayout* schemasHeaderLayout = new QHBoxLayout();
    QLabel* schemasLabel = new QLabel("Schemas:");
    schemasLabel->setStyleSheet("font-weight: bold;");
    schemasHeaderLayout->addWidget(schemasLabel);

    QPushButton* exportSelectAllSchemasButton_ = new QPushButton("Select All");
    connect(exportSelectAllSchemasButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onSelectAllSchemas);
    schemasHeaderLayout->addWidget(exportSelectAllSchemasButton_);

    QPushButton* exportClearSchemasButton_ = new QPushButton("Clear");
    connect(exportClearSchemasButton_, &QPushButton::clicked, [this]() {
        for (int i = 0; i < exportSchemasList_->count(); ++i) {
            QListWidgetItem* item = exportSchemasList_->item(i);
            item->setCheckState(Qt::Unchecked);
        }
    });
    schemasHeaderLayout->addWidget(exportClearSchemasButton_);

    schemasHeaderLayout->addStretch();
    objectsLayout->addLayout(schemasHeaderLayout);

    exportSchemasList_ = new QListWidget();
    exportSchemasList_->setMaximumHeight(100);
    objectsLayout->addWidget(exportSchemasList_);

    layout->addWidget(objectsGroup);

    // Options
    QGroupBox* optionsGroup = new QGroupBox("Export Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    QHBoxLayout* optionsRow1 = new QHBoxLayout();
    exportIncludeDataCheck_ = new QCheckBox("Include table data");
    exportIncludeDropCheck_ = new QCheckBox("Include DROP statements");
    exportIncludeCreateCheck_ = new QCheckBox("Include CREATE statements");
    exportIncludeCreateCheck_->setChecked(true);
    optionsRow1->addWidget(exportIncludeDataCheck_);
    optionsRow1->addWidget(exportIncludeDropCheck_);
    optionsRow1->addWidget(exportIncludeCreateCheck_);

    QHBoxLayout* optionsRow2 = new QHBoxLayout();
    exportIncludeIndexesCheck_ = new QCheckBox("Include indexes");
    exportIncludeIndexesCheck_->setChecked(true);
    exportIncludeConstraintsCheck_ = new QCheckBox("Include constraints");
    exportIncludeConstraintsCheck_->setChecked(true);
    exportIncludeTriggersCheck_ = new QCheckBox("Include triggers");
    exportIncludeTriggersCheck_->setChecked(true);
    optionsRow2->addWidget(exportIncludeIndexesCheck_);
    optionsRow2->addWidget(exportIncludeConstraintsCheck_);
    optionsRow2->addWidget(exportIncludeTriggersCheck_);

    QHBoxLayout* optionsRow3 = new QHBoxLayout();
    exportIncludeViewsCheck_ = new QCheckBox("Include views");
    exportIncludeViewsCheck_->setChecked(true);
    exportIncludeSequencesCheck_ = new QCheckBox("Include sequences");
    exportIncludeSequencesCheck_->setChecked(true);
    optionsRow3->addWidget(exportIncludeViewsCheck_);
    optionsRow3->addWidget(exportIncludeSequencesCheck_);
    optionsRow3->addStretch();

    optionsLayout->addLayout(optionsRow1);
    optionsLayout->addLayout(optionsRow2);
    optionsLayout->addLayout(optionsRow3);

    // Encoding and compression
    QHBoxLayout* encodingLayout = new QHBoxLayout();
    exportEncodingCombo_ = new QComboBox();
    exportEncodingCombo_->addItems(encodings_);
    exportCompressCheck_ = new QCheckBox("Compress output file");
    encodingLayout->addWidget(new QLabel("Encoding:"));
    encodingLayout->addWidget(exportEncodingCombo_);
    encodingLayout->addStretch();
    encodingLayout->addWidget(exportCompressCheck_);

    optionsLayout->addLayout(encodingLayout);
    layout->addWidget(optionsGroup);

    // Export button
    QHBoxLayout* exportButtonLayout = new QHBoxLayout();
    exportButtonLayout->addStretch();
    exportButton_ = new QPushButton("Export Database");
    exportButton_->setIcon(QIcon(":/icons/export.png"));
    exportButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    connect(exportButton_, &QPushButton::clicked, this, &ImportExportDialog::onExportClicked);
    exportButtonLayout->addWidget(exportButton_);
    layout->addLayout(exportButtonLayout);

    tabWidget_->addTab(exportTab_, "Export");
}

void ImportExportDialog::setupImportTab() {
    importTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(importTab_);

    // Import format and file selection
    QGroupBox* formatGroup = new QGroupBox("Import Format & File");
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    importFormatCombo_ = new QComboBox();
    importFormatCombo_->addItems(importFormats_);
    connect(importFormatCombo_, &QComboBox::currentTextChanged,
            this, &ImportExportDialog::onImportFormatChanged);
    formatLayout->addRow("Format:", importFormatCombo_);

    QHBoxLayout* fileLayout = new QHBoxLayout();
    importFilePathEdit_ = new QLineEdit();
    importFilePathEdit_->setPlaceholderText("Select input file...");
    fileLayout->addWidget(importFilePathEdit_);

    importBrowseButton_ = new QPushButton("Browse...");
    connect(importBrowseButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onBrowseImportFile);
    fileLayout->addWidget(importBrowseButton_);

    formatLayout->addRow("Input File:", fileLayout);
    layout->addWidget(formatGroup);

    // Preview
    QGroupBox* previewGroup = new QGroupBox("File Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    QHBoxLayout* previewButtonLayout = new QHBoxLayout();
    importPreviewButton_ = new QPushButton("Preview File");
    importPreviewButton_->setIcon(QIcon(":/icons/preview.png"));
    connect(importPreviewButton_, &QPushButton::clicked,
            this, &ImportExportDialog::onPreviewClicked);
    previewButtonLayout->addWidget(importPreviewButton_);
    previewButtonLayout->addStretch();

    previewLayout->addLayout(previewButtonLayout);

    importPreviewText_ = new QTextEdit();
    importPreviewText_->setMaximumHeight(200);
    importPreviewText_->setReadOnly(true);
    importPreviewText_->setFontFamily("monospace");
    importPreviewText_->setPlaceholderText("Click 'Preview File' to see file contents...");
    previewLayout->addWidget(importPreviewText_);

    layout->addWidget(previewGroup);

    // Options
    QGroupBox* optionsGroup = new QGroupBox("Import Options");
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);

    QHBoxLayout* optionsRow1 = new QHBoxLayout();
    importIgnoreErrorsCheck_ = new QCheckBox("Ignore errors");
    importContinueOnErrorCheck_ = new QCheckBox("Continue on error");
    importDropExistingCheck_ = new QCheckBox("Drop existing objects");
    optionsRow1->addWidget(importIgnoreErrorsCheck_);
    optionsRow1->addWidget(importContinueOnErrorCheck_);
    optionsRow1->addWidget(importDropExistingCheck_);

    QHBoxLayout* optionsRow2 = new QHBoxLayout();
    importCreateSchemasCheck_ = new QCheckBox("Create schemas");
    importCreateSchemasCheck_->setChecked(true);
    importCreateTablesCheck_ = new QCheckBox("Create tables");
    importCreateTablesCheck_->setChecked(true);
    importCreateIndexesCheck_ = new QCheckBox("Create indexes");
    importCreateIndexesCheck_->setChecked(true);
    optionsRow2->addWidget(importCreateSchemasCheck_);
    optionsRow2->addWidget(importCreateTablesCheck_);
    optionsRow2->addWidget(importCreateIndexesCheck_);

    QHBoxLayout* optionsRow3 = new QHBoxLayout();
    importCreateConstraintsCheck_ = new QCheckBox("Create constraints");
    importCreateConstraintsCheck_->setChecked(true);
    importCreateTriggersCheck_ = new QCheckBox("Create triggers");
    importCreateTriggersCheck_->setChecked(true);
    importCreateViewsCheck_ = new QCheckBox("Create views");
    importCreateViewsCheck_->setChecked(true);
    optionsRow3->addWidget(importCreateConstraintsCheck_);
    optionsRow3->addWidget(importCreateTriggersCheck_);
    optionsRow3->addWidget(importCreateViewsCheck_);

    optionsLayout->addLayout(optionsRow1);
    optionsLayout->addLayout(optionsRow2);
    optionsLayout->addLayout(optionsRow3);

    // Encoding and preview
    QHBoxLayout* encodingLayout = new QHBoxLayout();
    importEncodingCombo_ = new QComboBox();
    importEncodingCombo_->addItems(encodings_);
    importPreviewOnlyCheck_ = new QCheckBox("Preview only (no changes)");
    encodingLayout->addWidget(new QLabel("Encoding:"));
    encodingLayout->addWidget(importEncodingCombo_);
    encodingLayout->addStretch();
    encodingLayout->addWidget(importPreviewOnlyCheck_);

    optionsLayout->addLayout(encodingLayout);
    layout->addWidget(optionsGroup);

    // Import button
    QHBoxLayout* importButtonLayout = new QHBoxLayout();
    importButtonLayout->addStretch();
    importButton_ = new QPushButton("Import Data");
    importButton_->setIcon(QIcon(":/icons/import.png"));
    importButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; }");
    connect(importButton_, &QPushButton::clicked, this, &ImportExportDialog::onImportClicked);
    importButtonLayout->addWidget(importButton_);
    layout->addLayout(importButtonLayout);

    tabWidget_->addTab(importTab_, "Import");
}

void ImportExportDialog::setupSettingsTab() {
    settingsTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(settingsTab_);

    QGroupBox* directoriesGroup = new QGroupBox("Default Directories");
    QFormLayout* dirLayout = new QFormLayout(directoriesGroup);

    QHBoxLayout* exportDirLayout = new QHBoxLayout();
    defaultExportDirEdit_ = new QLineEdit();
    defaultExportDirEdit_->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    exportDirLayout->addWidget(defaultExportDirEdit_);

    defaultExportBrowseButton_ = new QPushButton("Browse...");
    connect(defaultExportBrowseButton_, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Default Export Directory",
                                                        defaultExportDirEdit_->text());
        if (!dir.isEmpty()) {
            defaultExportDirEdit_->setText(dir);
        }
    });
    exportDirLayout->addWidget(defaultExportBrowseButton_);

    dirLayout->addRow("Default Export Directory:", exportDirLayout);

    QHBoxLayout* importDirLayout = new QHBoxLayout();
    defaultImportDirEdit_ = new QLineEdit();
    defaultImportDirEdit_->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    importDirLayout->addWidget(defaultImportDirEdit_);

    defaultImportBrowseButton_ = new QPushButton("Browse...");
    connect(defaultImportBrowseButton_, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Default Import Directory",
                                                        defaultImportDirEdit_->text());
        if (!dir.isEmpty()) {
            defaultImportDirEdit_->setText(dir);
        }
    });
    importDirLayout->addWidget(defaultImportBrowseButton_);

    dirLayout->addRow("Default Import Directory:", importDirLayout);
    layout->addWidget(directoriesGroup);

    QGroupBox* behaviorGroup = new QGroupBox("Behavior");
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);

    autoCompressCheck_ = new QCheckBox("Automatically compress exported files");
    showProgressCheck_ = new QCheckBox("Show detailed progress for operations");
    showProgressCheck_->setChecked(true);

    behaviorLayout->addWidget(autoCompressCheck_);
    behaviorLayout->addWidget(showProgressCheck_);

    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addWidget(new QLabel("Preview line limit:"));
    previewLineLimitSpin_ = new QSpinBox();
    previewLineLimitSpin_->setRange(10, 10000);
    previewLineLimitSpin_->setValue(1000);
    previewLayout->addWidget(previewLineLimitSpin_);
    previewLayout->addStretch();

    behaviorLayout->addLayout(previewLayout);
    layout->addWidget(behaviorGroup);

    layout->addStretch();

    // Save button
    QHBoxLayout* saveButtonLayout = new QHBoxLayout();
    saveButtonLayout->addStretch();
    QPushButton* saveSettingsButton = new QPushButton("Save Settings");
    saveSettingsButton->setIcon(QIcon(":/icons/save.png"));
    connect(saveSettingsButton, &QPushButton::clicked, this, &ImportExportDialog::saveSettings);
    saveButtonLayout->addWidget(saveSettingsButton);
    layout->addLayout(saveButtonLayout);

    tabWidget_->addTab(settingsTab_, "Settings");
}

void ImportExportDialog::onExportFormatChanged(const QString& format) {
    Q_UNUSED(format);
    updateExportFileExtension();
}

void ImportExportDialog::onImportFormatChanged(const QString& format) {
    Q_UNUSED(format);
    updateImportFileExtension();
}

void ImportExportDialog::onBrowseExportFile() {
    QString defaultDir = defaultExportDirEdit_->text();
    QString format = exportFormatCombo_->currentText().toLower();
    QString filter;

    if (format == "sql") {
        filter = "SQL Files (*.sql);;All Files (*.*)";
    } else if (format == "csv") {
        filter = "CSV Files (*.csv);;All Files (*.*)";
    } else if (format == "json") {
        filter = "JSON Files (*.json);;All Files (*.*)";
    } else if (format == "xml") {
        filter = "XML Files (*.xml);;All Files (*.*)";
    } else if (format == "yaml") {
        filter = "YAML Files (*.yaml *.yml);;All Files (*.*)";
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Export File",
                                                   defaultDir, filter);

    if (!fileName.isEmpty()) {
        exportFilePathEdit_->setText(fileName);
    }
}

void ImportExportDialog::onBrowseImportFile() {
    QString defaultDir = defaultImportDirEdit_->text();
    QString format = importFormatCombo_->currentText().toLower();
    QString filter;

    if (format == "sql") {
        filter = "SQL Files (*.sql);;All Files (*.*)";
    } else if (format == "csv") {
        filter = "CSV Files (*.csv);;All Files (*.*)";
    } else if (format == "json") {
        filter = "JSON Files (*.json);;All Files (*.*)";
    } else if (format == "xml") {
        filter = "XML Files (*.xml);;All Files (*.*)";
    } else if (format == "yaml") {
        filter = "YAML Files (*.yaml *.yml);;All Files (*.*)";
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Open Import File",
                                                   defaultDir, filter);

    if (!fileName.isEmpty()) {
        importFilePathEdit_->setText(fileName);
    }
}

void ImportExportDialog::onSelectAllTables() {
    for (int i = 0; i < exportTablesList_->count(); ++i) {
        QListWidgetItem* item = exportTablesList_->item(i);
        item->setCheckState(Qt::Checked);
    }
}

void ImportExportDialog::onClearSelection() {
    for (int i = 0; i < exportTablesList_->count(); ++i) {
        QListWidgetItem* item = exportTablesList_->item(i);
        item->setCheckState(Qt::Unchecked);
    }
}

void ImportExportDialog::onSelectAllSchemas() {
    for (int i = 0; i < exportSchemasList_->count(); ++i) {
        QListWidgetItem* item = exportSchemasList_->item(i);
        item->setCheckState(Qt::Checked);
    }
}

void ImportExportDialog::onExportClicked() {
    if (exportFilePathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Export Error", "Please select an output file.");
        return;
    }

    ExportOptions options;
    options.format = exportFormatCombo_->currentText();
    options.filePath = exportFilePathEdit_->text();
    options.includeData = exportIncludeDataCheck_->isChecked();
    options.includeDropStatements = exportIncludeDropCheck_->isChecked();
    options.includeCreateStatements = exportIncludeCreateCheck_->isChecked();
    options.includeIndexes = exportIncludeIndexesCheck_->isChecked();
    options.includeConstraints = exportIncludeConstraintsCheck_->isChecked();
    options.includeTriggers = exportIncludeTriggersCheck_->isChecked();
    options.includeViews = exportIncludeViewsCheck_->isChecked();
    options.includeSequences = exportIncludeSequencesCheck_->isChecked();
    options.encoding = exportEncodingCombo_->currentText();
    options.compressOutput = exportCompressCheck_->isChecked();

    // Get selected tables
    for (int i = 0; i < exportTablesList_->count(); ++i) {
        QListWidgetItem* item = exportTablesList_->item(i);
        if (item->checkState() == Qt::Checked) {
            options.selectedTables.append(item->text());
        }
    }

    // Get selected schemas
    for (int i = 0; i < exportSchemasList_->count(); ++i) {
        QListWidgetItem* item = exportSchemasList_->item(i);
        if (item->checkState() == Qt::Checked) {
            options.selectedSchemas.append(item->text());
        }
    }

    emit exportRequested(options);
}

void ImportExportDialog::onImportClicked() {
    if (importFilePathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Import Error", "Please select an input file.");
        return;
    }

    if (!QFile::exists(importFilePathEdit_->text())) {
        QMessageBox::warning(this, "Import Error", "Selected file does not exist.");
        return;
    }

    ImportOptions options;
    options.format = importFormatCombo_->currentText();
    options.filePath = importFilePathEdit_->text();
    options.ignoreErrors = importIgnoreErrorsCheck_->isChecked();
    options.continueOnError = importContinueOnErrorCheck_->isChecked();
    options.dropExistingObjects = importDropExistingCheck_->isChecked();
    options.createSchemas = importCreateSchemasCheck_->isChecked();
    options.createTables = importCreateTablesCheck_->isChecked();
    options.createIndexes = importCreateIndexesCheck_->isChecked();
    options.createConstraints = importCreateConstraintsCheck_->isChecked();
    options.createTriggers = importCreateTriggersCheck_->isChecked();
    options.createViews = importCreateViewsCheck_->isChecked();
    options.encoding = importEncodingCombo_->currentText();
    options.previewOnly = importPreviewOnlyCheck_->isChecked();

    emit importRequested(options);
}

void ImportExportDialog::onPreviewClicked() {
    QString filePath = importFilePathEdit_->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Preview Error", "Please select a file to preview.");
        return;
    }

    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "Preview Error", "Selected file does not exist.");
        return;
    }

    emit previewRequested(filePath);
}

void ImportExportDialog::onTabChanged(int index) {
    // Update UI based on current tab
    QString tabText = tabWidget_->tabText(index);

    if (tabText == "Export") {
        exportButton_->setFocus();
    } else if (tabText == "Import") {
        importButton_->setFocus();
    }
}

void ImportExportDialog::updateExportFileExtension() {
    QString currentPath = exportFilePathEdit_->text();
    if (currentPath.isEmpty()) return;

    QString format = exportFormatCombo_->currentText().toLower();
    QString extension;

    if (format == "sql") {
        extension = ".sql";
    } else if (format == "csv") {
        extension = ".csv";
    } else if (format == "json") {
        extension = ".json";
    } else if (format == "xml") {
        extension = ".xml";
    } else if (format == "yaml") {
        extension = ".yaml";
    }

    if (!currentPath.endsWith(extension)) {
        QFileInfo fileInfo(currentPath);
        QString baseName = fileInfo.baseName();
        QString dir = fileInfo.absolutePath();
        QString newPath = QString("%1/%2%3").arg(dir, baseName, extension);
        exportFilePathEdit_->setText(newPath);
    }
}

void ImportExportDialog::updateImportFileExtension() {
    // Similar to export, but for import file selection
    // This method is called when format changes
}

void ImportExportDialog::loadSettings() {
    QSettings settings("ScratchRobin", "ImportExport");

    defaultExportDirEdit_->setText(settings.value("defaultExportDir",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    defaultImportDirEdit_->setText(settings.value("defaultImportDir",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());

    autoCompressCheck_->setChecked(settings.value("autoCompress", false).toBool());
    showProgressCheck_->setChecked(settings.value("showProgress", true).toBool());
    previewLineLimitSpin_->setValue(settings.value("previewLineLimit", 1000).toInt());
}

void ImportExportDialog::saveSettings() {
    QSettings settings("ScratchRobin", "ImportExport");

    settings.setValue("defaultExportDir", defaultExportDirEdit_->text());
    settings.setValue("defaultImportDir", defaultImportDirEdit_->text());
    settings.setValue("autoCompress", autoCompressCheck_->isChecked());
    settings.setValue("showProgress", showProgressCheck_->isChecked());
    settings.setValue("previewLineLimit", previewLineLimitSpin_->value());

    QMessageBox::information(this, "Settings Saved",
                           "Import/Export settings have been saved successfully.");
}

} // namespace scratchrobin

#include "import_export_dialog.moc"
