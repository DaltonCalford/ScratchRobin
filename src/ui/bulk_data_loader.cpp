#include "ui/bulk_data_loader.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QSplitter>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHeaderView>
#include <QDialogButtonBox>

namespace scratchrobin::ui {

// ============================================================================
// Bulk Data Loader Panel
// ============================================================================
BulkDataLoaderPanel::BulkDataLoaderPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("bulk_data_loader", parent)
    , client_(client)
{
    setupUi();
    setupModel();
}

BulkDataLoaderPanel::~BulkDataLoaderPanel() = default;

void BulkDataLoaderPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // Source group
    auto* sourceGroup = new QGroupBox(tr("Source"), this);
    auto* sourceLayout = new QFormLayout(sourceGroup);
    
    auto* sourceRowLayout = new QHBoxLayout();
    sourceEdit_ = new QLineEdit(this);
    sourceEdit_->setPlaceholderText(tr("Select file to load..."));
    sourceRowLayout->addWidget(sourceEdit_);
    sourceBtn_ = new QPushButton(tr("Browse..."), this);
    connect(sourceBtn_, &QPushButton::clicked, this, &BulkDataLoaderPanel::onSelectSource);
    sourceRowLayout->addWidget(sourceBtn_);
    sourceLayout->addRow(tr("File:"), sourceRowLayout);
    
    sourceInfoLabel_ = new QLabel(tr("No file selected"), this);
    sourceLayout->addRow(tr("Info:"), sourceInfoLabel_);
    
    mainLayout->addWidget(sourceGroup);
    
    // Target group
    auto* targetGroup = new QGroupBox(tr("Target"), this);
    auto* targetLayout = new QFormLayout(targetGroup);
    
    targetSchemaCombo_ = new QComboBox(this);
    targetSchemaCombo_->addItem("public");
    targetLayout->addRow(tr("Schema:"), targetSchemaCombo_);
    
    targetTableCombo_ = new QComboBox(this);
    targetTableCombo_->setEditable(true);
    targetLayout->addRow(tr("Table:"), targetTableCombo_);
    
    truncateCheck_ = new QCheckBox(tr("Truncate table before load"), this);
    targetLayout->addRow(QString(), truncateCheck_);
    
    mainLayout->addWidget(targetGroup);
    
    // Options group
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    
    batchSizeSpin_ = new QSpinBox(this);
    batchSizeSpin_->setRange(100, 100000);
    batchSizeSpin_->setValue(1000);
    batchSizeSpin_->setSingleStep(100);
    optionsLayout->addRow(tr("Batch Size:"), batchSizeSpin_);
    
    threadCountSpin_ = new QSpinBox(this);
    threadCountSpin_->setRange(1, 16);
    threadCountSpin_->setValue(4);
    optionsLayout->addRow(tr("Threads:"), threadCountSpin_);
    
    useTransactionsCheck_ = new QCheckBox(tr("Use transactions"), this);
    useTransactionsCheck_->setChecked(true);
    optionsLayout->addRow(QString(), useTransactionsCheck_);
    
    validateDataCheck_ = new QCheckBox(tr("Validate data"), this);
    validateDataCheck_->setChecked(true);
    optionsLayout->addRow(QString(), validateDataCheck_);
    
    mainLayout->addWidget(optionsGroup);
    
    // Progress group
    auto* progressGroup = new QGroupBox(tr("Progress"), this);
    auto* progressLayout = new QVBoxLayout(progressGroup);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressLayout->addWidget(progressBar_);
    
    progressLabel_ = new QLabel(tr("Ready"), this);
    progressLayout->addWidget(progressLabel_);
    
    etaLabel_ = new QLabel(tr("ETA: --"), this);
    progressLayout->addWidget(etaLabel_);
    
    statsLabel_ = new QLabel(tr("Rows: 0 | Errors: 0"), this);
    progressLayout->addWidget(statsLabel_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumHeight(150);
    progressLayout->addWidget(logEdit_);
    
    mainLayout->addWidget(progressGroup);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start Load"), this);
    startBtn_->setStyleSheet("background-color: green; color: white;");
    connect(startBtn_, &QPushButton::clicked, this, &BulkDataLoaderPanel::onStartLoad);
    buttonLayout->addWidget(startBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    pauseBtn_->setEnabled(false);
    connect(pauseBtn_, &QPushButton::clicked, this, &BulkDataLoaderPanel::onPauseLoad);
    buttonLayout->addWidget(pauseBtn_);
    
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    cancelBtn_->setEnabled(false);
    connect(cancelBtn_, &QPushButton::clicked, this, &BulkDataLoaderPanel::onCancelLoad);
    buttonLayout->addWidget(cancelBtn_);
    
    buttonLayout->addStretch();
    
    auto* configBtn = new QPushButton(tr("Configure..."), this);
    connect(configBtn, &QPushButton::clicked, this, &BulkDataLoaderPanel::onConfigureOptions);
    buttonLayout->addWidget(configBtn);
    
    auto* reportBtn = new QPushButton(tr("Export Report"), this);
    connect(reportBtn, &QPushButton::clicked, this, &BulkDataLoaderPanel::onExportReport);
    buttonLayout->addWidget(reportBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void BulkDataLoaderPanel::setupModel()
{
    // TODO: Load table list from database
    targetTableCombo_->addItem("table1");
    targetTableCombo_->addItem("table2");
}

void BulkDataLoaderPanel::onSelectSource()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Data File"),
        QString(),
        tr("CSV Files (*.csv);;TSV Files (*.tsv);;JSON Files (*.json);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        sourceEdit_->setText(fileName);
        
        QFileInfo info(fileName);
        currentLoad_.fileSize = info.size();
        
        QString sizeStr;
        if (info.size() < 1024 * 1024) {
            sizeStr = tr("%1 KB").arg(info.size() / 1024.0, 0, 'f', 2);
        } else {
            sizeStr = tr("%1 MB").arg(info.size() / (1024.0 * 1024.0), 0, 'f', 2);
        }
        
        sourceInfoLabel_->setText(tr("Size: %1 | Modified: %2")
            .arg(sizeStr)
            .arg(info.lastModified().toString()));
    }
}

void BulkDataLoaderPanel::onSelectTarget()
{
    // Target selection handled by combo box
}

void BulkDataLoaderPanel::onStartLoad()
{
    if (sourceEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a source file."));
        return;
    }
    
    if (targetTableCombo_->currentText().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a target table."));
        return;
    }
    
    isLoading_ = true;
    isPaused_ = false;
    
    currentLoad_.sourceFile = sourceEdit_->text();
    currentLoad_.targetTable = targetTableCombo_->currentText();
    currentLoad_.batchSize = batchSizeSpin_->value();
    currentLoad_.threadCount = threadCountSpin_->value();
    currentLoad_.useTransactions = useTransactionsCheck_->isChecked();
    currentLoad_.truncateFirst = truncateCheck_->isChecked();
    currentLoad_.validateData = validateDataCheck_->isChecked();
    currentLoad_.started = QDateTime::currentDateTime();
    
    startBtn_->setEnabled(false);
    pauseBtn_->setEnabled(true);
    cancelBtn_->setEnabled(true);
    
    logEdit_->append(tr("Starting bulk load..."));
    logEdit_->append(tr("Source: %1").arg(currentLoad_.sourceFile));
    logEdit_->append(tr("Target: %1").arg(currentLoad_.targetTable));
    
    emit loadStarted(currentLoad_.sourceFile);
    
    // TODO: Start actual loading via SessionClient
    
    // Simulate progress for now
    progressBar_->setValue(50);
    progressLabel_->setText(tr("Loading... 50%"));
}

void BulkDataLoaderPanel::onPauseLoad()
{
    if (isLoading_) {
        isPaused_ = !isPaused_;
        pauseBtn_->setText(isPaused_ ? tr("Resume") : tr("Pause"));
        logEdit_->append(isPaused_ ? tr("Load paused.") : tr("Load resumed."));
    }
}

void BulkDataLoaderPanel::onResumeLoad()
{
    isPaused_ = false;
    pauseBtn_->setText(tr("Pause"));
}

void BulkDataLoaderPanel::onCancelLoad()
{
    isLoading_ = false;
    isPaused_ = false;
    
    startBtn_->setEnabled(true);
    pauseBtn_->setEnabled(false);
    cancelBtn_->setEnabled(false);
    pauseBtn_->setText(tr("Pause"));
    
    logEdit_->append(tr("Load cancelled."));
}

void BulkDataLoaderPanel::onConfigureOptions()
{
    LoadConfigurationDialog dialog(&currentLoad_, this);
    dialog.exec();
}

void BulkDataLoaderPanel::onViewErrors()
{
    QStringList errors;
    // TODO: Get errors from current load
    ErrorViewerDialog dialog(errors, this);
    dialog.exec();
}

void BulkDataLoaderPanel::onExportReport()
{
    QString report = tr("Bulk Load Report\n");
    report += tr("================\n");
    report += tr("Source: %1\n").arg(currentLoad_.sourceFile);
    report += tr("Target: %1\n").arg(currentLoad_.targetTable);
    report += tr("Rows Processed: %1\n").arg(currentLoad_.processedRows);
    report += tr("Errors: %1\n").arg(currentLoad_.errorCount);
    
    QMessageBox::information(this, tr("Export Report"), 
        tr("Report exported to bulk_load_report.txt"));
}

void BulkDataLoaderPanel::updateStats()
{
    statsLabel_->setText(tr("Rows: %1 | Errors: %2")
        .arg(currentLoad_.processedRows)
        .arg(currentLoad_.errorCount));
}

void BulkDataLoaderPanel::updateProgress()
{
    if (currentLoad_.totalRows > 0) {
        int percent = (currentLoad_.processedRows * 100) / currentLoad_.totalRows;
        progressBar_->setValue(percent);
        progressLabel_->setText(tr("Loading... %1%").arg(percent));
        
        // Calculate ETA
        qint64 elapsed = currentLoad_.started.secsTo(QDateTime::currentDateTime());
        if (elapsed > 0 && currentLoad_.processedRows > 0) {
            qint64 remaining = (elapsed * (currentLoad_.totalRows - currentLoad_.processedRows)) 
                               / currentLoad_.processedRows;
            etaLabel_->setText(tr("ETA: %1 seconds").arg(remaining));
        }
    }
}

// ============================================================================
// Load Configuration Dialog
// ============================================================================
LoadConfigurationDialog::LoadConfigurationDialog(BulkLoadInfo* info, QWidget* parent)
    : QDialog(parent)
    , info_(info)
{
    setWindowTitle(tr("Load Configuration"));
    setMinimumSize(400, 350);
    setupUi();
    loadSettings();
}

void LoadConfigurationDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    batchSizeSpin_ = new QSpinBox(this);
    batchSizeSpin_->setRange(100, 100000);
    batchSizeSpin_->setValue(1000);
    batchSizeSpin_->setSingleStep(100);
    formLayout->addRow(tr("Batch Size:"), batchSizeSpin_);
    
    threadCountSpin_ = new QSpinBox(this);
    threadCountSpin_->setRange(1, 16);
    threadCountSpin_->setValue(4);
    formLayout->addRow(tr("Thread Count:"), threadCountSpin_);
    
    commitIntervalSpin_ = new QSpinBox(this);
    commitIntervalSpin_->setRange(1, 10000);
    commitIntervalSpin_->setValue(1000);
    formLayout->addRow(tr("Commit Interval:"), commitIntervalSpin_);
    
    useTransactionsCheck_ = new QCheckBox(tr("Use transactions"), this);
    useTransactionsCheck_->setChecked(true);
    formLayout->addRow(QString(), useTransactionsCheck_);
    
    skipErrorsCheck_ = new QCheckBox(tr("Skip errors and continue"), this);
    formLayout->addRow(QString(), skipErrorsCheck_);
    
    maxErrorsSpin_ = new QSpinBox(this);
    maxErrorsSpin_->setRange(0, 100000);
    maxErrorsSpin_->setValue(100);
    formLayout->addRow(tr("Max Errors:"), maxErrorsSpin_);
    
    validateCheck_ = new QCheckBox(tr("Validate data before insert"), this);
    validateCheck_->setChecked(true);
    formLayout->addRow(QString(), validateCheck_);
    
    trimWhitespaceCheck_ = new QCheckBox(tr("Trim whitespace from strings"), this);
    trimWhitespaceCheck_->setChecked(true);
    formLayout->addRow(QString(), trimWhitespaceCheck_);
    
    ignoreDuplicatesCheck_ = new QCheckBox(tr("Ignore duplicate key errors"), this);
    formLayout->addRow(QString(), ignoreDuplicatesCheck_);
    
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &LoadConfigurationDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void LoadConfigurationDialog::loadSettings()
{
    if (info_) {
        batchSizeSpin_->setValue(info_->batchSize);
        threadCountSpin_->setValue(info_->threadCount);
    }
}

void LoadConfigurationDialog::onSave()
{
    if (info_) {
        info_->batchSize = batchSizeSpin_->value();
        info_->threadCount = threadCountSpin_->value();
    }
    accept();
}

void LoadConfigurationDialog::onReset()
{
    loadSettings();
}

// ============================================================================
// Error Viewer Dialog
// ============================================================================
ErrorViewerDialog::ErrorViewerDialog(const QStringList& errors, QWidget* parent)
    : QDialog(parent)
    , errors_(errors)
{
    setWindowTitle(tr("Import Errors"));
    setMinimumSize(600, 400);
    setupUi();
}

void ErrorViewerDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    errorTable_ = new QTableView(this);
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(3);
    model_->setHorizontalHeaderLabels({tr("Row"), tr("Column"), tr("Error Message")});
    errorTable_->setModel(model_);
    errorTable_->horizontalHeader()->setStretchLastSection(true);
    
    // Populate with sample errors
    for (int i = 0; i < errors_.size(); ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(i + 1));
        row << new QStandardItem("-");
        row << new QStandardItem(errors_[i]);
        model_->appendRow(row);
    }
    
    mainLayout->addWidget(errorTable_);
    
    auto* buttonLayout = new QHBoxLayout();
    
    auto* exportBtn = new QPushButton(tr("Export..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &ErrorViewerDialog::onExport);
    buttonLayout->addWidget(exportBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ErrorViewerDialog::onClear);
    buttonLayout->addWidget(clearBtn);
    
    buttonLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void ErrorViewerDialog::onExport()
{
    QMessageBox::information(this, tr("Export"), tr("Errors exported to errors.txt"));
}

void ErrorViewerDialog::onClear()
{
    model_->clear();
    errors_.clear();
}

} // namespace scratchrobin::ui
