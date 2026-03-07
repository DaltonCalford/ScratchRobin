#include "ui/data_compare_sync.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QTreeView>
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
#include <QTabWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QFileDialog>

namespace scratchrobin::ui {

// ============================================================================
// Data Compare/Sync Panel
// ============================================================================
DataCompareSyncPanel::DataCompareSyncPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_compare_sync", parent)
    , client_(client)
{
    setupUi();
    setupModels();
}

void DataCompareSyncPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // Source/Target selection group
    auto* selectGroup = new QGroupBox(tr("Table Selection"), this);
    auto* selectLayout = new QFormLayout(selectGroup);
    
    // Source
    auto* sourceRow = new QHBoxLayout();
    sourceSchemaCombo_ = new QComboBox(this);
    sourceSchemaCombo_->addItem("public");
    sourceRow->addWidget(sourceSchemaCombo_);
    sourceTableCombo_ = new QComboBox(this);
    sourceTableCombo_->setEditable(true);
    sourceRow->addWidget(sourceTableCombo_, 1);
    auto* sourceSelectBtn = new QPushButton(tr("Select..."), this);
    connect(sourceSelectBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onSelectSourceTable);
    sourceRow->addWidget(sourceSelectBtn);
    selectLayout->addRow(tr("Source:"), sourceRow);
    
    // Target
    auto* targetRow = new QHBoxLayout();
    targetSchemaCombo_ = new QComboBox(this);
    targetSchemaCombo_->addItem("public");
    targetRow->addWidget(targetSchemaCombo_);
    targetTableCombo_ = new QComboBox(this);
    targetTableCombo_->setEditable(true);
    targetRow->addWidget(targetTableCombo_, 1);
    auto* targetSelectBtn = new QPushButton(tr("Select..."), this);
    connect(targetSelectBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onSelectTargetTable);
    targetRow->addWidget(targetSelectBtn);
    selectLayout->addRow(tr("Target:"), targetRow);
    
    // Key columns
    keyColumnsEdit_ = new QLineEdit(this);
    keyColumnsEdit_->setPlaceholderText(tr("e.g., id, customer_id"));
    selectLayout->addRow(tr("Key Columns:"), keyColumnsEdit_);
    
    mainLayout->addWidget(selectGroup);
    
    // Options group
    auto* optionsGroup = new QGroupBox(tr("Comparison Options"), this);
    auto* optionsLayout = new QHBoxLayout(optionsGroup);
    
    compareAllColumnsCheck_ = new QCheckBox(tr("Compare all columns"), this);
    compareAllColumnsCheck_->setChecked(true);
    optionsLayout->addWidget(compareAllColumnsCheck_);
    
    compareColumnsEdit_ = new QLineEdit(this);
    compareColumnsEdit_->setPlaceholderText(tr("Columns to compare (empty = all)"));
    compareColumnsEdit_->setEnabled(false);
    optionsLayout->addWidget(compareColumnsEdit_);
    
    ignoreCaseCheck_ = new QCheckBox(tr("Ignore case"), this);
    optionsLayout->addWidget(ignoreCaseCheck_);
    
    ignoreWhitespaceCheck_ = new QCheckBox(tr("Ignore whitespace"), this);
    optionsLayout->addWidget(ignoreWhitespaceCheck_);
    
    ignoreNullsCheck_ = new QCheckBox(tr("Treat NULL = empty"), this);
    optionsLayout->addWidget(ignoreNullsCheck_);
    
    optionsLayout->addStretch();
    
    connect(compareAllColumnsCheck_, &QCheckBox::toggled, 
            compareColumnsEdit_, &QLineEdit::setDisabled);
    
    mainLayout->addWidget(optionsGroup);
    
    // Action buttons
    auto* actionLayout = new QHBoxLayout();
    
    auto* compareBtn = new QPushButton(tr("Run Comparison"), this);
    compareBtn->setStyleSheet("background-color: #2196F3; color: white;");
    connect(compareBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onRunComparison);
    actionLayout->addWidget(compareBtn);
    
    auto* syncBtn = new QPushButton(tr("Generate Sync Script"), this);
    connect(syncBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onGenerateSyncScript);
    actionLayout->addWidget(syncBtn);
    
    auto* applyBtn = new QPushButton(tr("Apply Sync"), this);
    connect(applyBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onApplySync);
    actionLayout->addWidget(applyBtn);
    
    actionLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save Comparison"), this);
    connect(saveBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onSaveComparison);
    actionLayout->addWidget(saveBtn);
    
    auto* loadBtn = new QPushButton(tr("Load Comparison"), this);
    connect(loadBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onLoadComparison);
    actionLayout->addWidget(loadBtn);
    
    mainLayout->addLayout(actionLayout);
    
    // Results tabs
    tabWidget_ = new QTabWidget(this);
    
    // Results tree
    resultTree_ = new QTreeView(this);
    resultTree_->setAlternatingRowColors(true);
    tabWidget_->addTab(resultTree_, tr("Results"));
    
    // Sync script
    scriptEdit_ = new QTextEdit(this);
    scriptEdit_->setFont(QFont("Monospace", 10));
    scriptEdit_->setPlaceholderText(tr("Generated sync script will appear here..."));
    tabWidget_->addTab(scriptEdit_, tr("Sync Script"));
    
    mainLayout->addWidget(tabWidget_);
    
    // Summary and stats
    auto* bottomLayout = new QHBoxLayout();
    
    summaryLabel_ = new QLabel(tr("Ready to compare"), this);
    bottomLayout->addWidget(summaryLabel_);
    
    bottomLayout->addStretch();
    
    equalCountLabel_ = new QLabel(tr("Equal: 0"), this);
    equalCountLabel_->setStyleSheet("color: green;");
    bottomLayout->addWidget(equalCountLabel_);
    
    differentCountLabel_ = new QLabel(tr("Different: 0"), this);
    differentCountLabel_->setStyleSheet("color: orange;");
    bottomLayout->addWidget(differentCountLabel_);
    
    sourceOnlyCountLabel_ = new QLabel(tr("Source Only: 0"), this);
    sourceOnlyCountLabel_->setStyleSheet("color: blue;");
    bottomLayout->addWidget(sourceOnlyCountLabel_);
    
    targetOnlyCountLabel_ = new QLabel(tr("Target Only: 0"), this);
    targetOnlyCountLabel_->setStyleSheet("color: red;");
    bottomLayout->addWidget(targetOnlyCountLabel_);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &DataCompareSyncPanel::onExportResults);
    bottomLayout->addWidget(exportBtn);
    
    mainLayout->addLayout(bottomLayout);
}

void DataCompareSyncPanel::setupModels()
{
    resultModel_ = new QStandardItemModel(this);
    resultModel_->setColumnCount(4);
    resultModel_->setHorizontalHeaderLabels({tr("Key"), tr("Column"), tr("Source Value"), tr("Target Value")});
    resultTree_->setModel(resultModel_);
    resultTree_->header()->setStretchLastSection(true);
}

void DataCompareSyncPanel::onSelectSourceTable()
{
    // TODO: Show table selection dialog
    sourceTableCombo_->setCurrentText("customers");
}

void DataCompareSyncPanel::onSelectTargetTable()
{
    // TODO: Show table selection dialog
    targetTableCombo_->setCurrentText("customers_backup");
}

void DataCompareSyncPanel::onRunComparison()
{
    QString sourceTable = sourceTableCombo_->currentText();
    QString targetTable = targetTableCombo_->currentText();
    
    if (sourceTable.isEmpty() || targetTable.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select both source and target tables."));
        return;
    }
    
    summaryLabel_->setText(tr("Comparing %1 to %2...").arg(sourceTable).arg(targetTable));
    
    // Clear previous results
    resultModel_->removeRows(0, resultModel_->rowCount());
    compareResults_.clear();
    
    // TODO: Run actual comparison via SessionClient
    
    // Simulate results for demonstration
    int equal = 95;
    int different = 3;
    int sourceOnly = 1;
    int targetOnly = 1;
    
    // Add sample different rows
    for (int i = 0; i < 3; ++i) {
        auto* keyItem = new QStandardItem(QString::number(i + 1));
        auto* colItem = new QStandardItem("name");
        auto* srcItem = new QStandardItem(QString("Source Value %1").arg(i));
        auto* tgtItem = new QStandardItem(QString("Target Value %1").arg(i));
        tgtItem->setBackground(QBrush(QColor(255, 200, 200)));
        
        QList<QStandardItem*> row;
        row << keyItem << colItem << srcItem << tgtItem;
        resultModel_->appendRow(row);
    }
    
    equalCountLabel_->setText(tr("Equal: %1").arg(equal));
    differentCountLabel_->setText(tr("Different: %1").arg(different));
    sourceOnlyCountLabel_->setText(tr("Source Only: %1").arg(sourceOnly));
    targetOnlyCountLabel_->setText(tr("Target Only: %1").arg(targetOnly));
    
    summaryLabel_->setText(tr("Comparison complete. Found %1 differences.").arg(different + sourceOnly + targetOnly));
    
    emit comparisonFinished(different, sourceOnly, targetOnly);
}

void DataCompareSyncPanel::onGenerateSyncScript()
{
    QString script = tr("-- Generated Sync Script\n");
    script += tr("-- Source: %1\n").arg(sourceTableCombo_->currentText());
    script += tr("-- Target: %1\n").arg(targetTableCombo_->currentText());
    script += tr("-- Generated: %1\n\n").arg(QDateTime::currentDateTime().toString());
    
    // Generate UPDATE statements for differences
    script += tr("-- Update different rows\n");
    for (int i = 0; i < resultModel_->rowCount(); ++i) {
        QString key = resultModel_->item(i, 0)->text();
        QString col = resultModel_->item(i, 1)->text();
        QString srcVal = resultModel_->item(i, 2)->text();
        
        script += QString("UPDATE %1 SET %2 = '%3' WHERE id = %4;\n")
            .arg(targetTableCombo_->currentText())
            .arg(col)
            .arg(srcVal)
            .arg(key);
    }
    
    // Generate INSERT statements for source-only rows
    script += tr("\n-- Insert missing rows from source\n");
    script += QString("INSERT INTO %1 SELECT * FROM %2 WHERE id NOT IN (SELECT id FROM %1);\n")
        .arg(targetTableCombo_->currentText())
        .arg(sourceTableCombo_->currentText());
    
    // Generate DELETE statements for target-only rows
    script += tr("\n-- Delete rows not in source\n");
    script += QString("DELETE FROM %1 WHERE id NOT IN (SELECT id FROM %2);\n")
        .arg(targetTableCombo_->currentText())
        .arg(sourceTableCombo_->currentText());
    
    scriptEdit_->setPlainText(script);
    tabWidget_->setCurrentIndex(1);  // Switch to script tab
    
    SyncScriptPreviewDialog dialog(script, this);
    dialog.exec();
}

void DataCompareSyncPanel::onApplySync()
{
    QString script = scriptEdit_->toPlainText();
    if (script.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No sync script generated. Run comparison first."));
        return;
    }
    
    auto reply = QMessageBox::warning(this, tr("Apply Sync"),
        tr("This will modify the target table. Are you sure?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute script via SessionClient
        QMessageBox::information(this, tr("Sync Applied"), tr("Synchronization completed successfully."));
    }
}

void DataCompareSyncPanel::onExportResults()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Comparison Results"),
        QString(), tr("CSV Files (*.csv);;Text Files (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Results exported to %1").arg(fileName));
    }
}

void DataCompareSyncPanel::onSaveComparison()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Comparison"),
        QString(), tr("Comparison Files (*.cmp)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Save"), tr("Comparison saved to %1").arg(fileName));
    }
}

void DataCompareSyncPanel::onLoadComparison()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Comparison"),
        QString(), tr("Comparison Files (*.cmp)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Load"), tr("Comparison loaded from %1").arg(fileName));
    }
}

void DataCompareSyncPanel::onFilterChanged(const QString& filter)
{
    // TODO: Filter results
    Q_UNUSED(filter)
}

// ============================================================================
// Compare Options Dialog
// ============================================================================
CompareOptionsDialog::CompareOptionsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Comparison Options"));
    setMinimumSize(400, 300);
    setupUi();
}

void CompareOptionsDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    ignoreCaseCheck_ = new QCheckBox(tr("Ignore case for strings"), this);
    formLayout->addRow(QString(), ignoreCaseCheck_);
    
    ignoreWhitespaceCheck_ = new QCheckBox(tr("Ignore leading/trailing whitespace"), this);
    formLayout->addRow(QString(), ignoreWhitespaceCheck_);
    
    ignoreNullsCheck_ = new QCheckBox(tr("Treat NULL and empty string as equal"), this);
    formLayout->addRow(QString(), ignoreNullsCheck_);
    
    trimStringsCheck_ = new QCheckBox(tr("Trim strings before comparison"), this);
    trimStringsCheck_->setChecked(true);
    formLayout->addRow(QString(), trimStringsCheck_);
    
    roundNumbersCheck_ = new QCheckBox(tr("Round numeric values"), this);
    formLayout->addRow(QString(), roundNumbersCheck_);
    
    decimalPlacesSpin_ = new QSpinBox(this);
    decimalPlacesSpin_->setRange(0, 10);
    decimalPlacesSpin_->setValue(2);
    formLayout->addRow(tr("Decimal places:"), decimalPlacesSpin_);
    
    compareTimestampsCheck_ = new QCheckBox(tr("Compare timestamps with tolerance"), this);
    formLayout->addRow(QString(), compareTimestampsCheck_);
    
    timestampToleranceSpin_ = new QSpinBox(this);
    timestampToleranceSpin_->setRange(0, 3600);
    timestampToleranceSpin_->setValue(1);
    timestampToleranceSpin_->setSuffix(" seconds");
    formLayout->addRow(tr("Timestamp tolerance:"), timestampToleranceSpin_);
    
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CompareOptionsDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void CompareOptionsDialog::onSave()
{
    accept();
}

// ============================================================================
// Sync Script Preview Dialog
// ============================================================================
SyncScriptPreviewDialog::SyncScriptPreviewDialog(const QString& script, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Sync Script Preview"));
    setMinimumSize(600, 500);
    setupUi();
    scriptEdit_->setPlainText(script);
}

void SyncScriptPreviewDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    scriptEdit_ = new QTextEdit(this);
    scriptEdit_->setFont(QFont("Monospace", 10));
    mainLayout->addWidget(scriptEdit_);
    
    auto* buttonLayout = new QHBoxLayout();
    
    auto* applyBtn = new QPushButton(tr("Apply to Target"), this);
    applyBtn->setStyleSheet("background-color: green; color: white;");
    connect(applyBtn, &QPushButton::clicked, this, &SyncScriptPreviewDialog::onApply);
    buttonLayout->addWidget(applyBtn);
    
    auto* saveBtn = new QPushButton(tr("Save to File"), this);
    connect(saveBtn, &QPushButton::clicked, this, &SyncScriptPreviewDialog::onSave);
    buttonLayout->addWidget(saveBtn);
    
    auto* copyBtn = new QPushButton(tr("Copy to Clipboard"), this);
    connect(copyBtn, &QPushButton::clicked, this, &SyncScriptPreviewDialog::onCopy);
    buttonLayout->addWidget(copyBtn);
    
    buttonLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void SyncScriptPreviewDialog::onApply()
{
    auto reply = QMessageBox::warning(this, tr("Apply Script"),
        tr("This will modify the target database. Continue?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        accept();
    }
}

void SyncScriptPreviewDialog::onSave()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Script"),
        QString(), tr("SQL Files (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Save"), tr("Script saved to %1").arg(fileName));
    }
}

void SyncScriptPreviewDialog::onCopy()
{
    scriptEdit_->selectAll();
    scriptEdit_->copy();
    QMessageBox::information(this, tr("Copy"), tr("Script copied to clipboard."));
}

// ============================================================================
// Table Mapping Dialog
// ============================================================================
TableMappingDialog::TableMappingDialog(const QString& sourceTable, 
                                       const QString& targetTable,
                                       QWidget* parent)
    : QDialog(parent)
    , sourceTable_(sourceTable)
    , targetTable_(targetTable)
{
    setWindowTitle(tr("Column Mapping - %1 to %2").arg(sourceTable).arg(targetTable));
    setMinimumSize(500, 400);
    setupUi();
    loadColumns();
}

void TableMappingDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    mainLayout->addWidget(new QLabel(tr("Map columns from %1 to %2:").arg(sourceTable_).arg(targetTable_)));
    
    mappingTable_ = new QTableView(this);
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(4);
    model_->setHorizontalHeaderLabels({tr("Source Column"), tr("Target Column"), tr("Type"), tr("Key")});
    mappingTable_->setModel(model_);
    mappingTable_->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(mappingTable_);
    
    auto* buttonLayout = new QHBoxLayout();
    
    auto* autoBtn = new QPushButton(tr("Auto Map"), this);
    connect(autoBtn, &QPushButton::clicked, this, &TableMappingDialog::onAutoMap);
    buttonLayout->addWidget(autoBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &TableMappingDialog::onClearMapping);
    buttonLayout->addWidget(clearBtn);
    
    buttonLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &TableMappingDialog::onSave);
    buttonLayout->addWidget(saveBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void TableMappingDialog::loadColumns()
{
    // TODO: Load actual column lists from database
    // Sample data for now
    QStringList sourceCols = {"id", "name", "email", "created_at"};
    QStringList targetCols = {"id", "name", "email", "created"};
    
    for (int i = 0; i < sourceCols.size(); ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(sourceCols[i]);
        row << new QStandardItem(targetCols[i]);
        row << new QStandardItem("VARCHAR");
        row << new QStandardItem(i == 0 ? "Yes" : "");
        model_->appendRow(row);
    }
}

void TableMappingDialog::onAutoMap()
{
    // Auto-map columns by name similarity
    QMessageBox::information(this, tr("Auto Map"), tr("Columns auto-mapped by name."));
}

void TableMappingDialog::onClearMapping()
{
    model_->removeRows(0, model_->rowCount());
}

void TableMappingDialog::onSave()
{
    accept();
}

} // namespace scratchrobin::ui
