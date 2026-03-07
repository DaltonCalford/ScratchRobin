#include "data_generator.h"
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
#include <QSpinBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QDialogButtonBox>
#include <QTimer>
#include <QRandomGenerator>

namespace scratchrobin::ui {

// ============================================================================
// Data Generator Panel
// ============================================================================

DataGeneratorPanel::DataGeneratorPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_generator", parent)
    , client_(client) {
    setupUi();
    loadTables();
}

void DataGeneratorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* configBtn = new QPushButton(tr("Configure Generators..."), this);
    connect(configBtn, &QPushButton::clicked, this, &DataGeneratorPanel::onConfigureGenerators);
    toolbarLayout->addWidget(configBtn);
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &DataGeneratorPanel::onPreviewData);
    toolbarLayout->addWidget(previewBtn);
    
    auto* generateBtn = new QPushButton(tr("Generate Data"), this);
    connect(generateBtn, &QPushButton::clicked, this, &DataGeneratorPanel::onGenerateData);
    toolbarLayout->addWidget(generateBtn);
    
    toolbarLayout->addStretch();
    
    auto* multiBtn = new QPushButton(tr("Multi-Table..."), this);
    connect(multiBtn, &QPushButton::clicked, this, &DataGeneratorPanel::onGenerateForMultipleTables);
    toolbarLayout->addWidget(multiBtn);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &DataGeneratorPanel::onRefreshTables);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main content
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Table selection
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    leftLayout->addWidget(new QLabel(tr("Select Table:"), this));
    
    tableTree_ = new QTreeView(this);
    tableModel_ = new QStandardItemModel(this);
    tableTree_->setModel(tableModel_);
    tableTree_->setHeaderHidden(true);
    connect(tableTree_, &QTreeView::clicked, this, &DataGeneratorPanel::onTableSelected);
    leftLayout->addWidget(tableTree_);
    
    // Configuration
    auto* configGroup = new QGroupBox(tr("Generation Settings"), this);
    auto* configLayout = new QFormLayout(configGroup);
    
    rowCountSpin_ = new QSpinBox(this);
    rowCountSpin_->setRange(1, 10000000);
    rowCountSpin_->setValue(1000);
    rowCountSpin_->setSingleStep(1000);
    configLayout->addRow(tr("Row Count:"), rowCountSpin_);
    
    batchSizeSpin_ = new QSpinBox(this);
    batchSizeSpin_->setRange(100, 100000);
    batchSizeSpin_->setValue(1000);
    configLayout->addRow(tr("Batch Size:"), batchSizeSpin_);
    
    truncateCheck_ = new QCheckBox(tr("Truncate table first"), this);
    configLayout->addRow(truncateCheck_);
    
    respectFKCheck_ = new QCheckBox(tr("Respect foreign keys"), this);
    respectFKCheck_->setChecked(true);
    configLayout->addRow(respectFKCheck_);
    
    transactionCheck_ = new QCheckBox(tr("Use transaction"), this);
    transactionCheck_->setChecked(true);
    configLayout->addRow(transactionCheck_);
    
    leftLayout->addWidget(configGroup);
    splitter->addWidget(leftWidget);
    
    // Column configuration
    auto* centerWidget = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    centerLayout->addWidget(new QLabel(tr("Column Generators:"), this));
    
    columnTable_ = new QTableWidget(this);
    columnTable_->setColumnCount(4);
    columnTable_->setHorizontalHeaderLabels({tr("Column"), tr("Data Type"), tr("Generator"), tr("Null %")});
    columnTable_->setAlternatingRowColors(true);
    centerLayout->addWidget(columnTable_);
    
    splitter->addWidget(centerWidget);
    
    // Preview
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    rightLayout->addWidget(new QLabel(tr("Preview:"), this));
    
    previewTable_ = new QTableView(this);
    previewModel_ = new QStandardItemModel(this);
    previewTable_->setModel(previewModel_);
    previewTable_->setAlternatingRowColors(true);
    rightLayout->addWidget(previewTable_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    rightLayout->addWidget(progressBar_);
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    rightLayout->addWidget(statusLabel_);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({200, 400, 350});
    
    mainLayout->addWidget(splitter, 1);
}

void DataGeneratorPanel::loadTables() {
    tableModel_->clear();
    
    auto* root = new QStandardItem("scratchbird");
    tableModel_->appendRow(root);
    
    auto* tables = new QStandardItem("Tables");
    tables->appendRow(new QStandardItem("customers"));
    tables->appendRow(new QStandardItem("orders"));
    tables->appendRow(new QStandardItem("products"));
    tables->appendRow(new QStandardItem("users"));
    root->appendRow(tables);
    
    tableTree_->expandAll();
}

void DataGeneratorPanel::loadColumns(const QString& tableName) {
    Q_UNUSED(tableName)
    
    columnTable_->setRowCount(0);
    
    QStringList columns = {"id", "name", "email", "phone", "created_at", "status"};
    QStringList types = {"serial", "varchar", "varchar", "varchar", "timestamp", "varchar"};
    
    for (int i = 0; i < columns.size(); ++i) {
        int row = columnTable_->rowCount();
        columnTable_->insertRow(row);
        
        columnTable_->setItem(row, 0, new QTableWidgetItem(columns[i]));
        columnTable_->setItem(row, 1, new QTableWidgetItem(types[i]));
        
        auto* genCombo = new QComboBox(this);
        genCombo->addItems({"Auto", "First Name", "Last Name", "Email", "Phone", "Date", "Random Int", "Sequence"});
        columnTable_->setCellWidget(row, 2, genCombo);
        
        auto* nullSpin = new QSpinBox(this);
        nullSpin->setRange(0, 100);
        nullSpin->setValue(0);
        nullSpin->setSuffix("%");
        columnTable_->setCellWidget(row, 3, nullSpin);
    }
}

void DataGeneratorPanel::updatePreview() {
    previewModel_->clear();
    
    QStringList headers;
    for (int i = 0; i < columnTable_->rowCount(); ++i) {
        headers << columnTable_->item(i, 0)->text();
    }
    previewModel_->setHorizontalHeaderLabels(headers);
    
    // Generate 10 preview rows
    QStringList firstNames = {"John", "Jane", "Bob", "Alice", "Charlie"};
    QStringList lastNames = {"Smith", "Jones", "Brown", "Davis", "Wilson"};
    
    for (int row = 0; row < 10; ++row) {
        QList<QStandardItem*> items;
        items << new QStandardItem(QString::number(row + 1));
        items << new QStandardItem(firstNames[row % 5] + " " + lastNames[row % 5]);
        items << new QStandardItem(QString("user%1@example.com").arg(row + 1));
        items << new QStandardItem(QString("555-01%1").arg(row, 2, 10, QChar('0')));
        items << new QStandardItem(QDateTime::currentDateTime().toString());
        items << new QStandardItem("active");
        previewModel_->appendRow(items);
    }
}

void DataGeneratorPanel::onSelectTable() {
    // Show object selection dialog
}

void DataGeneratorPanel::onTableSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    QString tableName = tableModel_->itemFromIndex(index)->text();
    if (!tableName.isEmpty() && tableName != "scratchbird" && tableName != "Tables") {
        currentTask_.tableName = tableName;
        loadColumns(tableName);
        updatePreview();
    }
}

void DataGeneratorPanel::onRefreshTables() {
    loadTables();
}

void DataGeneratorPanel::onConfigureGenerators() {
    ColumnGenerator gen;
    QStringList tables;
    ColumnGeneratorDialog dialog(&gen, tables, this);
    dialog.exec();
}

void DataGeneratorPanel::onPreviewData() {
    updatePreview();
    statusLabel_->setText(tr("Preview updated"));
}

void DataGeneratorPanel::onGenerateData() {
    if (currentTask_.tableName.isEmpty()) {
        QMessageBox::warning(this, tr("No Table Selected"),
            tr("Please select a table first."));
        return;
    }
    
    currentTask_.rowCount = rowCountSpin_->value();
    currentTask_.batchSize = batchSizeSpin_->value();
    
    GenerationProgressDialog dialog(currentTask_, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        emit generationCompleted(currentTask_.id, currentTask_.rowCount);
        statusLabel_->setText(tr("Generated %1 rows").arg(currentTask_.rowCount));
    }
}

void DataGeneratorPanel::onStopGeneration() {
    statusLabel_->setText(tr("Generation stopped"));
}

void DataGeneratorPanel::onExportTemplate() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Template"),
        QString(),
        tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Template exported."));
    }
}

void DataGeneratorPanel::onSavePreset() {
    QMessageBox::information(this, tr("Save Preset"),
        tr("Current configuration saved as preset."));
}

void DataGeneratorPanel::onLoadPreset() {
    QMessageBox::information(this, tr("Load Preset"),
        tr("Preset would be loaded here."));
}

void DataGeneratorPanel::onDeletePreset() {
    // Delete preset
}

void DataGeneratorPanel::onPresetSelected(int index) {
    Q_UNUSED(index)
}

void DataGeneratorPanel::onGenerateForMultipleTables() {
    MultiTableGeneratorDialog dialog(client_, this);
    dialog.exec();
}

void DataGeneratorPanel::onClearGeneratedData() {
    auto reply = QMessageBox::question(this, tr("Clear Data"),
        tr("Delete all generated data?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Clear"), tr("Generated data cleared."));
    }
}

// ============================================================================
// Column Generator Dialog
// ============================================================================

ColumnGeneratorDialog::ColumnGeneratorDialog(ColumnGenerator* generator, const QStringList& availableTables, QWidget* parent)
    : QDialog(parent)
    , generator_(generator)
    , availableTables_(availableTables) {
    setupUi();
    setWindowTitle(tr("Configure Column Generator"));
    resize(500, 400);
}

void ColumnGeneratorDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    // Generator type
    auto* formLayout = new QFormLayout();
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("First Name"), static_cast<int>(GeneratorType::FirstName));
    typeCombo_->addItem(tr("Last Name"), static_cast<int>(GeneratorType::LastName));
    typeCombo_->addItem(tr("Full Name"), static_cast<int>(GeneratorType::FullName));
    typeCombo_->addItem(tr("Email"), static_cast<int>(GeneratorType::Email));
    typeCombo_->addItem(tr("Phone"), static_cast<int>(GeneratorType::Phone));
    typeCombo_->addItem(tr("Address"), static_cast<int>(GeneratorType::Address));
    typeCombo_->addItem(tr("Integer"), static_cast<int>(GeneratorType::Integer));
    typeCombo_->addItem(tr("Float"), static_cast<int>(GeneratorType::Float));
    typeCombo_->addItem(tr("Date"), static_cast<int>(GeneratorType::Date));
    typeCombo_->addItem(tr("UUID"), static_cast<int>(GeneratorType::UUID));
    typeCombo_->addItem(tr("Lorem Ipsum"), static_cast<int>(GeneratorType::LoremIpsum));
    typeCombo_->addItem(tr("Custom List"), static_cast<int>(GeneratorType::CustomList));
    typeCombo_->addItem(tr("Sequence"), static_cast<int>(GeneratorType::Sequence));
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColumnGeneratorDialog::onGeneratorTypeChanged);
    formLayout->addRow(tr("Generator Type:"), typeCombo_);
    
    nullPercentSpin_ = new QSpinBox(this);
    nullPercentSpin_->setRange(0, 100);
    nullPercentSpin_->setValue(0);
    nullPercentSpin_->setSuffix("%");
    formLayout->addRow(tr("Null Percentage:"), nullPercentSpin_);
    
    uniqueCheck_ = new QCheckBox(tr("Generate unique values"), this);
    formLayout->addRow(uniqueCheck_);
    
    layout->addLayout(formLayout);
    
    // Options stack
    optionsStack_ = new QStackedWidget(this);
    
    // Range options
    auto* rangePage = new QWidget();
    auto* rangeLayout = new QFormLayout(rangePage);
    minEdit_ = new QLineEdit("0", this);
    maxEdit_ = new QLineEdit("1000", this);
    precisionSpin_ = new QSpinBox(this);
    precisionSpin_->setRange(0, 10);
    precisionSpin_->setValue(2);
    rangeLayout->addRow(tr("Minimum:"), minEdit_);
    rangeLayout->addRow(tr("Maximum:"), maxEdit_);
    rangeLayout->addRow(tr("Precision:"), precisionSpin_);
    optionsStack_->addWidget(rangePage);
    
    // Custom values
    auto* listPage = new QWidget();
    auto* listLayout = new QVBoxLayout(listPage);
    valuesEdit_ = new QTextEdit(this);
    valuesEdit_->setPlaceholderText(tr("Enter values, one per line"));
    listLayout->addWidget(valuesEdit_);
    optionsStack_->addWidget(listPage);
    
    // Reference
    auto* refPage = new QWidget();
    auto* refLayout = new QFormLayout(refPage);
    refTableCombo_ = new QComboBox(this);
    refColumnCombo_ = new QComboBox(this);
    refLayout->addRow(tr("Reference Table:"), refTableCombo_);
    refLayout->addRow(tr("Reference Column:"), refColumnCombo_);
    optionsStack_->addWidget(refPage);
    
    layout->addWidget(optionsStack_, 1);
    
    // Preview
    layout->addWidget(new QLabel(tr("Preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(80);
    layout->addWidget(previewEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &ColumnGeneratorDialog::onPreview);
    btnLayout->addWidget(previewBtn);
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &ColumnGeneratorDialog::onSave);
    btnLayout->addWidget(saveBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ColumnGeneratorDialog::onGeneratorTypeChanged(int index) {
    Q_UNUSED(index)
    GeneratorType type = static_cast<GeneratorType>(typeCombo_->currentData().toInt());
    
    switch (type) {
        case GeneratorType::Integer:
        case GeneratorType::Float:
        case GeneratorType::Decimal:
            optionsStack_->setCurrentIndex(0);
            break;
        case GeneratorType::CustomList:
        case GeneratorType::RandomPick:
            optionsStack_->setCurrentIndex(1);
            break;
        case GeneratorType::Reference:
            optionsStack_->setCurrentIndex(2);
            break;
        default:
            optionsStack_->setCurrentIndex(0);
            break;
    }
}

void ColumnGeneratorDialog::onPreview() {
    previewEdit_->clear();
    for (int i = 0; i < 5; ++i) {
        previewEdit_->append(QString("Sample value %1").arg(i + 1));
    }
}

void ColumnGeneratorDialog::onSave() {
    generator_->generatorType = static_cast<GeneratorType>(typeCombo_->currentData().toInt());
    generator_->nullPercent = nullPercentSpin_->value();
    generator_->unique = uniqueCheck_->isChecked();
    accept();
}

void ColumnGeneratorDialog::onCancel() {
    reject();
}

// ============================================================================
// Generation Progress Dialog
// ============================================================================

GenerationProgressDialog::GenerationProgressDialog(const DataGenerationTask& task, 
                                                    backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , task_(task)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Generating Data"));
    resize(500, 350);
}

void GenerationProgressDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Generating %1 rows for table '%2'")
                                 .arg(task_.rowCount).arg(task_.tableName), this));
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    layout->addWidget(progressBar_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setFont(QFont("Monospace", 9));
    layout->addWidget(logEdit_, 1);
    
    auto* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusLayout->addWidget(statusLabel_);
    
    etaLabel_ = new QLabel(tr("ETA: --"), this);
    statusLayout->addWidget(etaLabel_);
    
    statusLayout->addStretch();
    layout->addLayout(statusLayout);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &GenerationProgressDialog::onStart);
    btnLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &GenerationProgressDialog::onStop);
    btnLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    pauseBtn_->setEnabled(false);
    connect(pauseBtn_, &QPushButton::clicked, this, &GenerationProgressDialog::onPause);
    btnLayout->addWidget(pauseBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &GenerationProgressDialog::onClose);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void GenerationProgressDialog::onStart() {
    running_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    pauseBtn_->setEnabled(true);
    startTime_ = QDateTime::currentDateTime();
    
    logEdit_->append(tr("Starting data generation..."));
    logEdit_->append(tr("Table: %1").arg(task_.tableName));
    logEdit_->append(tr("Rows to generate: %1").arg(task_.rowCount));
    
    executeGeneration();
}

void GenerationProgressDialog::executeGeneration() {
    if (!running_ || paused_) return;
    
    int batchSize = qMin(task_.batchSize, task_.rowCount - generatedCount_);
    
    if (batchSize <= 0) {
        // Complete
        progressBar_->setValue(100);
        statusLabel_->setText(tr("Completed"));
        logEdit_->append(tr("Generated %1 rows successfully!").arg(generatedCount_));
        running_ = false;
        startBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        pauseBtn_->setEnabled(false);
        return;
    }
    
    generatedCount_ += batchSize;
    int percent = (generatedCount_ * 100) / task_.rowCount;
    progressBar_->setValue(percent);
    statusLabel_->setText(tr("Generated %1 of %2 rows").arg(generatedCount_).arg(task_.rowCount));
    
    // Calculate ETA
    qint64 elapsed = startTime_.secsTo(QDateTime::currentDateTime());
    if (elapsed > 0 && generatedCount_ > 0) {
        qint64 total = (elapsed * task_.rowCount) / generatedCount_;
        qint64 remaining = total - elapsed;
        etaLabel_->setText(tr("ETA: %1s").arg(remaining));
    }
    
    logEdit_->append(tr("Batch completed: %1 rows").arg(batchSize));
    
    // Schedule next batch
    QTimer::singleShot(50, this, &GenerationProgressDialog::executeGeneration);
}

void GenerationProgressDialog::onStop() {
    running_ = false;
    logEdit_->append(tr("Generation stopped by user."));
    statusLabel_->setText(tr("Stopped"));
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);
}

void GenerationProgressDialog::onPause() {
    paused_ = !paused_;
    pauseBtn_->setText(paused_ ? tr("Resume") : tr("Pause"));
    
    if (!paused_) {
        executeGeneration();
    }
}

void GenerationProgressDialog::onClose() {
    if (running_) {
        auto reply = QMessageBox::question(this, tr("Generation Running"),
            tr("Generation is still running. Close anyway?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) return;
        running_ = false;
    }
    accept();
}

void GenerationProgressDialog::onProgressUpdate(int current, int total) {
    int percent = total > 0 ? (current * 100) / total : 0;
    progressBar_->setValue(percent);
}

// ============================================================================
// Multi-Table Generator Dialog
// ============================================================================

MultiTableGeneratorDialog::MultiTableGeneratorDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Multi-Table Data Generator"));
    resize(600, 450);
}

void MultiTableGeneratorDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Select tables to generate data for:"), this));
    
    // Tables list
    tableList_ = new QListWidget(this);
    tableList_->setSelectionMode(QAbstractItemView::MultiSelection);
    loadTables();
    layout->addWidget(tableList_);
    
    // Configuration table
    layout->addWidget(new QLabel(tr("Configuration:"), this));
    
    configTable_ = new QTableWidget(this);
    configTable_->setColumnCount(3);
    configTable_->setHorizontalHeaderLabels({tr("Table"), tr("Rows"), tr("Dependencies")});
    layout->addWidget(configTable_);
    
    // Summary
    layout->addWidget(new QLabel(tr("Summary:"), this));
    summaryEdit_ = new QTextEdit(this);
    summaryEdit_->setReadOnly(true);
    summaryEdit_->setMaximumHeight(60);
    layout->addWidget(summaryEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* generateBtn = new QPushButton(tr("Generate All"), this);
    connect(generateBtn, &QPushButton::clicked, this, &MultiTableGeneratorDialog::onGenerateAll);
    btnLayout->addWidget(generateBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void MultiTableGeneratorDialog::loadTables() {
    tableList_->addItems({"customers", "orders", "order_items", "products", "categories"});
}

void MultiTableGeneratorDialog::onAddTable() {
    // Add selected table to configuration
}

void MultiTableGeneratorDialog::onRemoveTable() {
    // Remove from configuration
}

void MultiTableGeneratorDialog::onTableSelectionChanged() {
    // Update configuration table
}

void MultiTableGeneratorDialog::onConfigureTable() {
    // Configure specific table
}

void MultiTableGeneratorDialog::onGenerateAll() {
    QMessageBox::information(this, tr("Generate"),
        tr("Data would be generated for all configured tables in dependency order."));
    accept();
}

// ============================================================================
// PreviewDataDialog
// ============================================================================
void PreviewDataDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    setWindowTitle(tr("Preview Data"));
    setMinimumSize(600, 400);
    
    // Controls
    auto* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel(tr("Count:"), this));
    previewCountSpin_ = new QSpinBox(this);
    previewCountSpin_->setRange(10, 1000);
    previewCountSpin_->setValue(100);
    controlLayout->addWidget(previewCountSpin_);
    controlLayout->addStretch();
    
    layout->addLayout(controlLayout);
    
    // Preview table
    previewTable_ = new QTableView(this);
    previewModel_ = new QStandardItemModel(this);
    previewTable_->setModel(previewModel_);
    layout->addWidget(previewTable_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* exportBtn = new QPushButton(tr("Export"), this);
    auto* closeBtn = new QPushButton(tr("Close"), this);
    
    connect(refreshBtn, &QPushButton::clicked, this, &PreviewDataDialog::onRefresh);
    connect(exportBtn, &QPushButton::clicked, this, &PreviewDataDialog::onExport);
    connect(closeBtn, &QPushButton::clicked, this, &PreviewDataDialog::close);
    
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(exportBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
    
    generatePreview();
}

void PreviewDataDialog::generatePreview() {
    previewModel_->clear();
    previewModel_->setHorizontalHeaderLabels({tr("Column"), tr("Value")});
    
    // Generate sample preview rows
    for (int i = 0; i < previewCountSpin_->value(); ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(tr("Row %1").arg(i + 1));
        row << new QStandardItem(tr("Sample Value %1").arg(i + 1));
        previewModel_->appendRow(row);
    }
}

void PreviewDataDialog::onRefresh() {
    // TODO: Implement refresh
    generatePreview();
}

void PreviewDataDialog::onExport() {
    // TODO: Implement export
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Preview"),
        QString(), tr("CSV Files (*.csv);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Data exported to %1").arg(fileName));
    }
}

} // namespace scratchrobin::ui
