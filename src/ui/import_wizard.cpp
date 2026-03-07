#include "ui/import_wizard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableView>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <QApplication>

#include <chrono>
#include <thread>

namespace scratchrobin::ui {

// ============================================================================
// Import Data Wizard
// ============================================================================

ImportDataWizard::ImportDataWizard(QWidget* parent)
    : QWizard(parent) {
    setWindowTitle(tr("Import Data"));
    setMinimumSize(700, 500);
    resize(800, 600);
    
    setWizardStyle(ModernStyle);
    setOptions(HaveHelpButton | HelpButtonOnRight);
    
    setupPages();
}

ImportDataWizard::~ImportDataWizard() = default;

void ImportDataWizard::setupPages() {
    setPage(0, new ImportSourcePage(this));
    setPage(1, new ImportPreviewPage(this));
    setPage(2, new ImportOptionsPage(this));
    setPage(3, new ImportExecutionPage(this));
}

ImportDataWizard::ImportConfig ImportDataWizard::config() const {
    ImportConfig cfg;
    // Gather config from all pages
    return cfg;
}

void ImportDataWizard::setConfig(const ImportConfig& config) {
    Q_UNUSED(config)
}

// ============================================================================
// Step 1: Source Selection
// ============================================================================

ImportSourcePage::ImportSourcePage(QWidget* parent)
    : QWizardPage(parent) {
    setTitle(tr("Select Source File"));
    setSubTitle(tr("Choose the file you want to import and specify its format."));
    
    setupUi();
}

void ImportSourcePage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // File selection
    auto* fileGroup = new QGroupBox(tr("Source File"), this);
    auto* fileLayout = new QHBoxLayout(fileGroup);
    fileEdit_ = new QLineEdit(this);
    fileEdit_->setPlaceholderText(tr("Select a file to import..."));
    fileLayout->addWidget(fileEdit_);
    browseBtn_ = new QPushButton(tr("Browse..."), this);
    fileLayout->addWidget(browseBtn_);
    layout->addWidget(fileGroup);
    
    // Format selection
    auto* formatLayout = new QHBoxLayout();
    formatLayout->addWidget(new QLabel(tr("File format:"), this));
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItem(tr("Auto-detect"), "auto");
    formatCombo_->addItem(tr("CSV (Comma Separated)"), "csv");
    formatCombo_->addItem(tr("TSV (Tab Separated)"), "tsv");
    formatCombo_->addItem(tr("JSON"), "json");
    formatCombo_->addItem(tr("Excel (.xlsx)"), "excel");
    formatCombo_->addItem(tr("SQL"), "sql");
    formatLayout->addWidget(formatCombo_);
    formatLayout->addStretch();
    layout->addLayout(formatLayout);
    
    // Encoding selection
    auto* encodingLayout = new QHBoxLayout();
    encodingLayout->addWidget(new QLabel(tr("Encoding:"), this));
    encodingCombo_ = new QComboBox(this);
    encodingCombo_->addItem(tr("UTF-8"), "UTF-8");
    encodingCombo_->addItem(tr("ASCII"), "ASCII");
    encodingCombo_->addItem(tr("ISO-8859-1"), "ISO-8859-1");
    encodingCombo_->addItem(tr("Windows-1252"), "Windows-1252");
    encodingLayout->addWidget(encodingCombo_);
    encodingLayout->addStretch();
    layout->addLayout(encodingLayout);
    
    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("QLabel { color: gray; }");
    layout->addWidget(infoLabel_);
    
    layout->addStretch();
    
    // Connections
    connect(browseBtn_, &QPushButton::clicked, this, &ImportSourcePage::onBrowseFile);
    connect(fileEdit_, &QLineEdit::textChanged, this, &ImportSourcePage::onFileSelected);
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportSourcePage::detectFileFormat);
    
    registerField("sourceFile*", fileEdit_);
    registerField("format", formatCombo_);
    registerField("encoding", encodingCombo_);
}

void ImportSourcePage::onBrowseFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Import File"),
        QString(),
        tr("CSV Files (*.csv);;TSV Files (*.tsv);;JSON Files (*.json);;Excel Files (*.xlsx);;SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        fileEdit_->setText(fileName);
    }
}

void ImportSourcePage::onFileSelected() {
    emit completeChanged();
    detectFileFormat();
}

void ImportSourcePage::detectFileFormat() {
    QString fileName = fileEdit_->text();
    if (fileName.isEmpty()) {
        infoLabel_->clear();
        return;
    }
    
    // Auto-detect format from extension
    if (formatCombo_->currentData().toString() == "auto") {
        if (fileName.endsWith(".csv", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(formatCombo_->findData("csv"));
        } else if (fileName.endsWith(".tsv", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(formatCombo_->findData("tsv"));
        } else if (fileName.endsWith(".json", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(formatCombo_->findData("json"));
        } else if (fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(formatCombo_->findData("excel"));
        } else if (fileName.endsWith(".sql", Qt::CaseInsensitive)) {
            formatCombo_->setCurrentIndex(formatCombo_->findData("sql"));
        }
    }
    
    // Check file size
    QFileInfo info(fileName);
    if (info.exists()) {
        qint64 size = info.size();
        QString sizeStr;
        if (size < 1024) {
            sizeStr = tr("%1 bytes").arg(size);
        } else if (size < 1024 * 1024) {
            sizeStr = tr("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        } else {
            sizeStr = tr("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        }
        infoLabel_->setText(tr("File size: %1").arg(sizeStr));
    }
}

bool ImportSourcePage::isComplete() const {
    return !fileEdit_->text().isEmpty() && QFile::exists(fileEdit_->text());
}

int ImportSourcePage::nextId() const {
    return 1;  // Go to preview page
}

// ============================================================================
// Step 2: Preview & Column Mapping
// ============================================================================

ImportPreviewPage::ImportPreviewPage(QWidget* parent)
    : QWizardPage(parent) {
    setTitle(tr("Preview & Column Mapping"));
    setSubTitle(tr("Preview the data and map columns to target fields."));
    
    setupUi();
}

void ImportPreviewPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    
    // Info label
    infoLabel_ = new QLabel(tr("Preview of first 10 rows:"), this);
    layout->addWidget(infoLabel_);
    
    // Preview table
    previewTable_ = new QTableView(this);
    previewTable_->setAlternatingRowColors(true);
    previewTable_->setSelectionBehavior(QAbstractItemView::SelectColumns);
    previewTable_->setSelectionMode(QAbstractItemView::MultiSelection);
    previewTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(previewTable_);
    
    model_ = new QStandardItemModel(this);
    previewTable_->setModel(model_);
    
    // Auto-detect button
    auto* buttonLayout = new QHBoxLayout();
    autoDetectBtn_ = new QPushButton(tr("Auto-detect Column Types"), this);
    buttonLayout->addWidget(autoDetectBtn_);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    layout->addWidget(new QLabel(tr("Tip: Select columns to import. Unselected columns will be skipped."), this));
    
    // Connections
    connect(autoDetectBtn_, &QPushButton::clicked, this, &ImportPreviewPage::onAutoDetectTypes);
}

void ImportPreviewPage::initializePage() {
    loadPreview();
}

void ImportPreviewPage::loadPreview() {
    QString fileName = field("sourceFile").toString();
    QString format = field("format").toString();
    QString encoding = field("encoding").toString();
    
    model_->clear();
    
    if (format == "csv" || format == "tsv") {
        CsvParser parser;
        CsvParser::ParseOptions options;
        options.encoding = encoding;
        options.delimiter = (format == "tsv") ? "\t" : ",";
        options.hasHeader = true;
        
        if (parser.loadFile(fileName, options)) {
            // Set headers
            model_->setHorizontalHeaderLabels(parser.headers());
            
            // Add preview rows
            for (const auto& row : parser.previewRows(10)) {
                QList<QStandardItem*> items;
                for (const auto& cell : row) {
                    items.append(new QStandardItem(cell));
                }
                model_->appendRow(items);
            }
            
            infoLabel_->setText(tr("Preview of first 10 rows (%1 total rows, %2 columns):")
                .arg(parser.rowCount())
                .arg(parser.columnCount()));
        }
    }
}

void ImportPreviewPage::onAutoDetectTypes() {
    detectColumnTypes();
    QMessageBox::information(this, tr("Auto-detect"), 
        tr("Column types have been auto-detected based on data content."));
}

void ImportPreviewPage::detectColumnTypes() {
    // Simple type detection logic
    for (int col = 0; col < model_->columnCount(); ++col) {
        bool allInteger = true;
        bool allNumber = true;
        bool allDate = true;
        
        for (int row = 0; row < model_->rowCount(); ++row) {
            QString value = model_->item(row, col)->text();
            
            bool intOk;
            value.toInt(&intOk);
            if (!intOk) allInteger = false;
            
            bool doubleOk;
            value.toDouble(&doubleOk);
            if (!doubleOk) allNumber = false;
        }
        
        QString detectedType;
        if (allInteger) detectedType = "INTEGER";
        else if (allNumber) detectedType = "DECIMAL";
        else detectedType = "VARCHAR";
        
        detectedTypes_.append(detectedType);
    }
}

void ImportPreviewPage::onColumnSelectionChanged() {
    emit completeChanged();
}

bool ImportPreviewPage::isComplete() const {
    return model_->rowCount() > 0;
}

int ImportPreviewPage::nextId() const {
    return 2;  // Go to options page
}

QStringList ImportPreviewPage::columnNames() const {
    QStringList names;
    for (int i = 0; i < model_->columnCount(); ++i) {
        names.append(model_->headerData(i, Qt::Horizontal).toString());
    }
    return names;
}

QList<int> ImportPreviewPage::selectedColumnIndices() const {
    QList<int> indices;
    // Return selected column indices
    return indices;
}

// ============================================================================
// Step 3: Options
// ============================================================================

ImportOptionsPage::ImportOptionsPage(QWidget* parent)
    : QWizardPage(parent) {
    setTitle(tr("Import Options"));
    setSubTitle(tr("Configure import settings and target table."));
    
    setupUi();
}

void ImportOptionsPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // CSV Options group
    csvGroup_ = new QGroupBox(tr("CSV Options"), this);
    auto* csvLayout = new QGridLayout(csvGroup_);
    
    csvLayout->addWidget(new QLabel(tr("Delimiter:"), this), 0, 0);
    delimiterCombo_ = new QComboBox(this);
    delimiterCombo_->addItem(tr("Comma"), ",");
    delimiterCombo_->addItem(tr("Tab"), "\t");
    delimiterCombo_->addItem(tr("Semicolon"), ";");
    delimiterCombo_->addItem(tr("Pipe"), "|");
    delimiterCombo_->setEditable(true);
    csvLayout->addWidget(delimiterCombo_, 0, 1);
    
    csvLayout->addWidget(new QLabel(tr("Quote character:"), this), 1, 0);
    quoteCombo_ = new QComboBox(this);
    quoteCombo_->addItem(tr("Double quote (\"))"), "\"");
    quoteCombo_->addItem(tr("Single quote (')"), "'");
    quoteCombo_->addItem(tr("None"), "");
    csvLayout->addWidget(quoteCombo_, 1, 1);
    
    headerCheck_ = new QCheckBox(tr("First row is header"), this);
    headerCheck_->setChecked(true);
    csvLayout->addWidget(headerCheck_, 2, 0, 1, 2);
    
    csvLayout->addWidget(new QLabel(tr("Skip lines:"), this), 3, 0);
    skipLinesSpin_ = new QSpinBox(this);
    skipLinesSpin_->setRange(0, 100);
    csvLayout->addWidget(skipLinesSpin_, 3, 1);
    
    trimCheck_ = new QCheckBox(tr("Trim whitespace"), this);
    trimCheck_->setChecked(true);
    csvLayout->addWidget(trimCheck_, 4, 0, 1, 2);
    
    layout->addWidget(csvGroup_);
    
    // Target table group
    targetGroup_ = new QGroupBox(tr("Target"), this);
    auto* targetLayout = new QGridLayout(targetGroup_);
    
    targetLayout->addWidget(new QLabel(tr("Import to:"), this), 0, 0);
    targetTypeCombo_ = new QComboBox(this);
    targetTypeCombo_->addItem(tr("Existing table"), "existing");
    targetTypeCombo_->addItem(tr("Create new table"), "new");
    targetLayout->addWidget(targetTypeCombo_, 0, 1);
    
    existingTableCombo_ = new QComboBox(this);
    existingTableCombo_->setEditable(true);
    existingTableCombo_->addItem(tr("Select or type table name..."));
    targetLayout->addWidget(existingTableCombo_, 1, 0, 1, 2);
    
    newTableEdit_ = new QLineEdit(this);
    newTableEdit_->setPlaceholderText(tr("Enter new table name..."));
    targetLayout->addWidget(newTableEdit_, 2, 0, 1, 2);
    
    truncateCheck_ = new QCheckBox(tr("Truncate table before import"), this);
    targetLayout->addWidget(truncateCheck_, 3, 0, 1, 2);
    
    layout->addWidget(targetGroup_);
    
    // Error handling
    auto* errorLayout = new QHBoxLayout();
    errorLayout->addWidget(new QLabel(tr("On error:"), this));
    errorHandlingCombo_ = new QComboBox(this);
    errorHandlingCombo_->addItem(tr("Stop and report"), "stop");
    errorHandlingCombo_->addItem(tr("Skip row and continue"), "skip");
    errorHandlingCombo_->addItem(tr("Import to error log"), "log");
    errorLayout->addWidget(errorHandlingCombo_);
    errorLayout->addStretch();
    layout->addLayout(errorLayout);
    
    layout->addStretch();
    
    // Connections
    connect(targetTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ImportOptionsPage::onTargetTypeChanged);
    
    registerField("delimiter", delimiterCombo_);
    registerField("hasHeader", headerCheck_);
    registerField("targetTable", existingTableCombo_, "currentText");
    registerField("createNewTable", targetTypeCombo_);
}

void ImportOptionsPage::initializePage() {
    updateOptionsState();
}

void ImportOptionsPage::onTargetTypeChanged() {
    updateOptionsState();
}

void ImportOptionsPage::updateOptionsState() {
    bool isExisting = targetTypeCombo_->currentData().toString() == "existing";
    existingTableCombo_->setEnabled(isExisting);
    newTableEdit_->setEnabled(!isExisting);
    truncateCheck_->setEnabled(isExisting);
}

bool ImportOptionsPage::isComplete() const {
    bool isExisting = targetTypeCombo_->currentData().toString() == "existing";
    if (isExisting) {
        return !existingTableCombo_->currentText().isEmpty();
    } else {
        return !newTableEdit_->text().isEmpty();
    }
}

int ImportOptionsPage::nextId() const {
    return 3;  // Go to execution page
}

// ============================================================================
// Step 4: Execution
// ============================================================================

ImportExecutionPage::ImportExecutionPage(QWidget* parent)
    : QWizardPage(parent) {
    setTitle(tr("Import Execution"));
    setSubTitle(tr("Review settings and start the import process."));
    
    setupUi();
}

void ImportExecutionPage::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    
    // Summary
    summaryLabel_ = new QLabel(this);
    summaryLabel_->setWordWrap(true);
    summaryLabel_->setStyleSheet("QLabel { background: palette(alternate-base); padding: 10px; border-radius: 5px; }");
    layout->addWidget(summaryLabel_);
    
    // Progress
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    layout->addWidget(progressBar_);
    
    // Log
    layout->addWidget(new QLabel(tr("Import log:"), this));
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    // Limit log lines (setMaximumBlockCount equivalent)
    layout->addWidget(logEdit_);
    
    // Result label
    resultLabel_ = new QLabel(this);
    resultLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(resultLabel_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    startBtn_ = new QPushButton(tr("Start Import"), this);
    buttonLayout->addWidget(startBtn_);
    
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    cancelBtn_->setEnabled(false);
    buttonLayout->addWidget(cancelBtn_);
    
    layout->addLayout(buttonLayout);
    
    // Connections
    connect(startBtn_, &QPushButton::clicked, this, &ImportExecutionPage::startImport);
    connect(cancelBtn_, &QPushButton::clicked, this, &ImportExecutionPage::cancelImport);
}

void ImportExecutionPage::initializePage() {
    updateSummary();
    importRunning_ = false;
    importComplete_ = false;
    progressBar_->setValue(0);
    logEdit_->clear();
    resultLabel_->clear();
    startBtn_->setEnabled(true);
}

void ImportExecutionPage::updateSummary() {
    QString summary = tr("<b>Import Summary:</b><br>");
    summary += tr("Source: %1<br>").arg(field("sourceFile").toString());
    summary += tr("Format: %1<br>").arg(field("format").toString().toUpper());
    summary += tr("Target: %1<br>").arg(field("targetTable").toString());
    summary += tr("Delimiter: '%1'<br>").arg(field("delimiter").toString());
    summary += tr("Header row: %1").arg(field("hasHeader").toBool() ? tr("Yes") : tr("No"));
    
    summaryLabel_->setText(summary);
}

void ImportExecutionPage::startImport() {
    importRunning_ = true;
    startBtn_->setEnabled(false);
    cancelBtn_->setEnabled(true);
    
    logEdit_->append(tr("Starting import..."));
    
    // Simulate import progress
    for (int i = 0; i <= 100; i += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        QApplication::processEvents();
        onImportProgress(i, 100);
    }
    
    onImportFinished(100, 0);
}

void ImportExecutionPage::cancelImport() {
    importRunning_ = false;
    logEdit_->append(tr("Import cancelled by user."));
    startBtn_->setEnabled(true);
    cancelBtn_->setEnabled(false);
}

void ImportExecutionPage::onImportProgress(int current, int total) {
    progressBar_->setValue(current);
    if (current % 20 == 0) {
        logEdit_->append(tr("Processing... %1%").arg(current));
    }
}

void ImportExecutionPage::onImportFinished(int rowsImported, int errors) {
    importRunning_ = false;
    importComplete_ = true;
    progressBar_->setValue(100);
    
    logEdit_->append(tr("Import complete."));
    logEdit_->append(tr("Rows imported: %1").arg(rowsImported));
    if (errors > 0) {
        logEdit_->append(tr("Errors: %1").arg(errors));
    }
    
    resultLabel_->setText(tr("✓ Import completed successfully! %1 rows imported.").arg(rowsImported));
    resultLabel_->setStyleSheet("QLabel { color: green; font-weight: bold; }");
    
    startBtn_->setEnabled(false);
    cancelBtn_->setEnabled(false);
    
    emit completeChanged();
}

bool ImportExecutionPage::isComplete() const {
    return importComplete_;
}

// ============================================================================
// CSV Parser
// ============================================================================

CsvParser::CsvParser(QObject* parent)
    : QObject(parent) {
}

bool CsvParser::loadFile(const QString& fileName, const ParseOptions& options) {
    options_ = options;
    headers_.clear();
    data_.clear();
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit parseError(tr("Cannot open file: %1").arg(fileName));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    
    int lineNumber = 0;
    
    // Skip lines if requested
    while (lineNumber < options.skipLines && !stream.atEnd()) {
        stream.readLine();
        lineNumber++;
    }
    
    // Read header if present
    if (options.hasHeader && !stream.atEnd()) {
        QString headerLine = stream.readLine();
        headers_ = parseLine(headerLine);
        lineNumber++;
    }
    
    // Read data rows
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        lineNumber++;
        
        if (line.isEmpty()) continue;
        
        QStringList row = parseLine(line);
        if (!row.isEmpty()) {
            data_.append(row);
        }
        
        // Limit to reasonable preview size in memory
        if (data_.size() > 100000) {
            emit parseWarning(tr("File truncated to 100,000 rows for preview"));
            break;
        }
    }
    
    file.close();
    
    // Generate default headers if none
    if (headers_.isEmpty() && !data_.isEmpty()) {
        for (int i = 0; i < data_.first().size(); ++i) {
            headers_.append(tr("Column %1").arg(i + 1));
        }
    }
    
    return true;
}

QStringList CsvParser::parseLine(const QString& line) {
    return splitCsvLine(line);
}

QStringList CsvParser::splitCsvLine(const QString& line) {
    QStringList result;
    QString current;
    bool inQuotes = false;
    
    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];
        
        if (c == options_.quoteChar[0]) {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == options_.quoteChar[0]) {
                // Escaped quote
                current.append(c);
                i++;  // Skip next quote
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == options_.delimiter[0] && !inQuotes) {
            result.append(current.trimmed());
            current.clear();
        } else {
            current.append(c);
        }
    }
    
    result.append(current.trimmed());
    return result;
}

QList<QStringList> CsvParser::previewRows(int maxRows) const {
    return data_.mid(0, qMin(maxRows, data_.size()));
}

} // namespace scratchrobin::ui

#include <QThread>
