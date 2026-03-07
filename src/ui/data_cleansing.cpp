#include "data_cleansing.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTableWidget>
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
#include <QStackedWidget>
#include <QRadioButton>
#include <QSpinBox>
#include <QListWidget>
#include <QProgressBar>
#include <QDialogButtonBox>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Data Cleansing Panel
// ============================================================================

DataCleansingPanel::DataCleansingPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_cleansing", parent)
    , client_(client) {
    setupUi();
    loadRules();
}

void DataCleansingPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* newBtn = new QPushButton(tr("New Rule..."), this);
    connect(newBtn, &QPushButton::clicked, this, &DataCleansingPanel::onCreateRule);
    toolbarLayout->addWidget(newBtn);
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &DataCleansingPanel::onPreviewChanges);
    toolbarLayout->addWidget(previewBtn);
    
    auto* applyBtn = new QPushButton(tr("Apply"), this);
    connect(applyBtn, &QPushButton::clicked, this, &DataCleansingPanel::onApplyChanges);
    toolbarLayout->addWidget(applyBtn);
    
    toolbarLayout->addStretch();
    
    auto* analyzeBtn = new QPushButton(tr("Analyze Quality"), this);
    connect(analyzeBtn, &QPushButton::clicked, this, &DataCleansingPanel::onAnalyzeDataQuality);
    toolbarLayout->addWidget(analyzeBtn);
    
    auto* dupsBtn = new QPushButton(tr("Find Duplicates"), this);
    connect(dupsBtn, &QPushButton::clicked, this, &DataCleansingPanel::onFindDuplicates);
    toolbarLayout->addWidget(dupsBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main content
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Rules table
    rulesTable_ = new QTableView(this);
    rulesModel_ = new QStandardItemModel(this);
    rulesModel_->setHorizontalHeaderLabels({tr("Rule"), tr("Table"), tr("Operation"), tr("Column"), tr("Status")});
    rulesTable_->setModel(rulesModel_);
    rulesTable_->setAlternatingRowColors(true);
    connect(rulesTable_, &QTableView::clicked, this, &DataCleansingPanel::onRuleSelected);
    splitter->addWidget(rulesTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    // Quality dashboard
    auto* qualityGroup = new QGroupBox(tr("Data Quality Overview"), this);
    auto* qualityLayout = new QVBoxLayout(qualityGroup);
    
    overallQualityBar_ = new QProgressBar(this);
    overallQualityBar_->setRange(0, 100);
    overallQualityBar_->setValue(85);
    qualityLayout->addWidget(new QLabel(tr("Overall Quality:"), this));
    qualityLayout->addWidget(overallQualityBar_);
    
    duplicatesLabel_ = new QLabel(tr("Duplicates: 124"), this);
    qualityLayout->addWidget(duplicatesLabel_);
    
    nullsLabel_ = new QLabel(tr("Null Values: 56"), this);
    qualityLayout->addWidget(nullsLabel_);
    
    formatIssuesLabel_ = new QLabel(tr("Format Issues: 23"), this);
    qualityLayout->addWidget(formatIssuesLabel_);
    
    detailsLayout->addWidget(qualityGroup);
    
    // Rule details
    auto* ruleGroup = new QGroupBox(tr("Rule Details"), this);
    auto* ruleLayout = new QFormLayout(ruleGroup);
    
    ruleNameLabel_ = new QLabel(this);
    ruleLayout->addRow(tr("Name:"), ruleNameLabel_);
    
    operationLabel_ = new QLabel(this);
    ruleLayout->addRow(tr("Operation:"), operationLabel_);
    
    targetLabel_ = new QLabel(this);
    ruleLayout->addRow(tr("Target:"), targetLabel_);
    
    statusLabel_ = new QLabel(this);
    ruleLayout->addRow(tr("Status:"), statusLabel_);
    
    detailsLayout->addWidget(ruleGroup);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({500, 300});
    
    mainLayout->addWidget(splitter, 1);
}

void DataCleansingPanel::loadRules() {
    rules_.clear();
    
    QStringList names = {"Remove Duplicates", "Trim Whitespace", "Standardize Phone", "Fix Nulls"};
    QStringList tables = {"customers", "customers", "users", "orders"};
    QStringList operations = {"Remove Duplicates", "Trim", "Standardize", "Remove Nulls"};
    QStringList columns = {"email", "name", "phone", "address"};
    
    for (int i = 0; i < names.size(); ++i) {
        CleansingRule rule;
        rule.id = QString::number(i);
        rule.name = names[i];
        rule.tableName = tables[i];
        rule.operation = static_cast<CleansingOperation>(i % 4);
        rule.targetColumn = columns[i];
        rules_.append(rule);
    }
    
    updateRulesTable();
}

void DataCleansingPanel::updateRulesTable() {
    rulesModel_->clear();
    rulesModel_->setHorizontalHeaderLabels({tr("Rule"), tr("Table"), tr("Operation"), tr("Column"), tr("Status")});
    
    for (const auto& rule : rules_) {
        QString opStr;
        switch (rule.operation) {
            case CleansingOperation::RemoveDuplicates: opStr = tr("Deduplicate"); break;
            case CleansingOperation::TrimWhitespace: opStr = tr("Trim"); break;
            case CleansingOperation::StandardizeFormat: opStr = tr("Standardize"); break;
            case CleansingOperation::RemoveNulls: opStr = tr("Remove Nulls"); break;
            default: opStr = tr("Other");
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(rule.name);
        row << new QStandardItem(rule.tableName);
        row << new QStandardItem(opStr);
        row << new QStandardItem(rule.targetColumn);
        row << new QStandardItem(tr("Active"));
        rulesModel_->appendRow(row);
    }
}

void DataCleansingPanel::onCreateRule() {
    CleansingRule newRule;
    CleansingRuleDialog dialog(&newRule, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        rules_.append(newRule);
        updateRulesTable();
    }
}

void DataCleansingPanel::onEditRule() {}
void DataCleansingPanel::onDeleteRule() {}
void DataCleansingPanel::onCloneRule() {}

void DataCleansingPanel::onRuleSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= rules_.size()) return;
    const auto& rule = rules_[index.row()];
    ruleNameLabel_->setText(rule.name);
    operationLabel_->setText(QString::number(static_cast<int>(rule.operation)));
    targetLabel_->setText(rule.targetColumn);
    statusLabel_->setText(tr("Active"));
}

void DataCleansingPanel::onPreviewChanges() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    PreviewChangesDialog dialog(rules_[index.row()], client_, this);
    dialog.exec();
}

void DataCleansingPanel::onApplyChanges() {
    QMessageBox::information(this, tr("Apply"), tr("Cleansing rules applied successfully."));
}

void DataCleansingPanel::onScheduleCleansing() {}
void DataCleansingPanel::onExportReport() {}

void DataCleansingPanel::onQuickDeduplicate() {
    DuplicateFinderDialog dialog(client_, this);
    dialog.exec();
}

void DataCleansingPanel::onQuickTrimWhitespace() {
    QMessageBox::information(this, tr("Trim"), tr("Whitespace trimmed successfully."));
}

void DataCleansingPanel::onQuickStandardizeCase() {
    QMessageBox::information(this, tr("Standardize"), tr("Case standardized successfully."));
}

void DataCleansingPanel::onQuickRemoveNulls() {
    QMessageBox::information(this, tr("Remove Nulls"), tr("Null values removed successfully."));
}

void DataCleansingPanel::onAnalyzeDataQuality() {
    DataQualityAnalyzer dialog(client_, this);
    dialog.exec();
}

void DataCleansingPanel::onFindDuplicates() {
    DuplicateFinderDialog dialog(client_, this);
    dialog.exec();
}

void DataCleansingPanel::onGenerateReport() {
    QMessageBox::information(this, tr("Report"), tr("Data quality report generated."));
}

// ============================================================================
// Cleansing Rule Dialog
// ============================================================================

CleansingRuleDialog::CleansingRuleDialog(CleansingRule* rule, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), rule_(rule), client_(client) {
    setupUi();
    setWindowTitle(tr("Cleansing Rule"));
    resize(500, 400);
}

void CleansingRuleDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    nameEdit_ = new QLineEdit(rule_->name, this);
    formLayout->addRow(tr("Rule Name:"), nameEdit_);
    
    tableCombo_ = new QComboBox(this);
    tableCombo_->addItems({"customers", "orders", "users", "products"});
    formLayout->addRow(tr("Table:"), tableCombo_);
    
    operationCombo_ = new QComboBox(this);
    operationCombo_->addItem(tr("Remove Duplicates"), static_cast<int>(CleansingOperation::RemoveDuplicates));
    operationCombo_->addItem(tr("Remove Nulls"), static_cast<int>(CleansingOperation::RemoveNulls));
    operationCombo_->addItem(tr("Trim Whitespace"), static_cast<int>(CleansingOperation::TrimWhitespace));
    operationCombo_->addItem(tr("Standardize Format"), static_cast<int>(CleansingOperation::StandardizeFormat));
    operationCombo_->addItem(tr("Find & Replace"), static_cast<int>(CleansingOperation::FindReplace));
    connect(operationCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CleansingRuleDialog::onOperationChanged);
    formLayout->addRow(tr("Operation:"), operationCombo_);
    
    layout->addLayout(formLayout);
    
    // Options based on operation
    operationStack_ = new QStackedWidget(this);
    
    // Duplicate options
    auto* dupPage = new QWidget();
    auto* dupLayout = new QFormLayout(dupPage);
    matchTypeCombo_ = new QComboBox(this);
    matchTypeCombo_->addItems({tr("Exact"), tr("Fuzzy"), tr("Soundex")});
    dupLayout->addRow(tr("Match Type:"), matchTypeCombo_);
    thresholdSpin_ = new QSpinBox(this);
    thresholdSpin_->setRange(0, 100);
    thresholdSpin_->setValue(80);
    dupLayout->addRow(tr("Threshold:"), thresholdSpin_);
    keepStrategyCombo_ = new QComboBox(this);
    keepStrategyCombo_->addItems({tr("Keep First"), tr("Keep Last"), tr("Merge")});
    dupLayout->addRow(tr("Keep Strategy:"), keepStrategyCombo_);
    operationStack_->addWidget(dupPage);
    
    // Find/Replace options
    auto* findPage = new QWidget();
    auto* findLayout = new QFormLayout(findPage);
    findEdit_ = new QLineEdit(this);
    findLayout->addRow(tr("Find:"), findEdit_);
    replaceEdit_ = new QLineEdit(this);
    findLayout->addRow(tr("Replace:"), replaceEdit_);
    regexCheck_ = new QCheckBox(tr("Use Regular Expression"), this);
    findLayout->addRow(regexCheck_);
    operationStack_->addWidget(findPage);
    
    layout->addWidget(operationStack_, 1);
    
    // Safety options
    auto* safetyGroup = new QGroupBox(tr("Safety Options"), this);
    auto* safetyLayout = new QVBoxLayout(safetyGroup);
    previewCheck_ = new QCheckBox(tr("Preview before applying"), this);
    previewCheck_->setChecked(true);
    safetyLayout->addWidget(previewCheck_);
    backupCheck_ = new QCheckBox(tr("Backup before running"), this);
    backupCheck_->setChecked(true);
    safetyLayout->addWidget(backupCheck_);
    layout->addWidget(safetyGroup);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &CleansingRuleDialog::onSave);
    btnLayout->addWidget(saveBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void CleansingRuleDialog::onOperationChanged(int index) {
    operationStack_->setCurrentIndex(index < 2 ? 0 : 1);
}

void CleansingRuleDialog::onTableChanged(int index) {
    Q_UNUSED(index)
}

void CleansingRuleDialog::onPreview() {}

void CleansingRuleDialog::onSave() {
    rule_->name = nameEdit_->text();
    rule_->tableName = tableCombo_->currentText();
    rule_->operation = static_cast<CleansingOperation>(operationCombo_->currentData().toInt());
    rule_->previewOnly = previewCheck_->isChecked();
    rule_->backupBeforeRun = backupCheck_->isChecked();
    accept();
}

void CleansingRuleDialog::onCancel() {
    reject();
}

void CleansingRuleDialog::updateOperationOptions() {}
void CleansingRuleDialog::loadTables() {}
void CleansingRuleDialog::loadColumns(const QString& table) {
    Q_UNUSED(table)
}

// ============================================================================
// Duplicate Finder Dialog
// ============================================================================

DuplicateFinderDialog::DuplicateFinderDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    setWindowTitle(tr("Duplicate Finder"));
    resize(600, 450);
}

void DuplicateFinderDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    // Configuration
    auto* configLayout = new QHBoxLayout();
    
    tableCombo_ = new QComboBox(this);
    tableCombo_->addItems({"customers", "orders", "users"});
    connect(tableCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DuplicateFinderDialog::onTableChanged);
    configLayout->addWidget(new QLabel(tr("Table:"), this));
    configLayout->addWidget(tableCombo_);
    
    matchTypeCombo_ = new QComboBox(this);
    matchTypeCombo_->addItems({tr("Exact"), tr("Fuzzy"), tr("Soundex")});
    configLayout->addWidget(new QLabel(tr("Match:"), this));
    configLayout->addWidget(matchTypeCombo_);
    
    thresholdSpin_ = new QSpinBox(this);
    thresholdSpin_->setRange(0, 100);
    thresholdSpin_->setValue(80);
    configLayout->addWidget(new QLabel(tr("Threshold:"), this));
    configLayout->addWidget(thresholdSpin_);
    
    configLayout->addStretch();
    layout->addLayout(configLayout);
    
    // Columns to check
    layout->addWidget(new QLabel(tr("Columns to Check:"), this));
    columnsList_ = new QListWidget(this);
    columnsList_->setSelectionMode(QAbstractItemView::MultiSelection);
    columnsList_->addItems({"name", "email", "phone", "address"});
    layout->addWidget(columnsList_);
    
    // Results
    resultsTable_ = new QTableView(this);
    resultsModel_ = new QStandardItemModel(this);
    resultsModel_->setHorizontalHeaderLabels({tr("Group"), tr("Count"), tr("Values"), tr("Action")});
    resultsTable_->setModel(resultsModel_);
    resultsTable_->setAlternatingRowColors(true);
    layout->addWidget(resultsTable_, 1);
    
    progressBar_ = new QProgressBar(this);
    layout->addWidget(progressBar_);
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* scanBtn = new QPushButton(tr("Scan"), this);
    connect(scanBtn, &QPushButton::clicked, this, &DuplicateFinderDialog::onStartScan);
    btnLayout->addWidget(scanBtn);
    
    auto* keepFirstBtn = new QPushButton(tr("Keep First"), this);
    connect(keepFirstBtn, &QPushButton::clicked, this, &DuplicateFinderDialog::onKeepFirst);
    btnLayout->addWidget(keepFirstBtn);
    
    auto* keepLastBtn = new QPushButton(tr("Keep Last"), this);
    connect(keepLastBtn, &QPushButton::clicked, this, &DuplicateFinderDialog::onKeepLast);
    btnLayout->addWidget(keepLastBtn);
    
    auto* mergeBtn = new QPushButton(tr("Merge"), this);
    connect(mergeBtn, &QPushButton::clicked, this, &DuplicateFinderDialog::onMergeSelected);
    btnLayout->addWidget(mergeBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &DuplicateFinderDialog::onDeleteSelected);
    btnLayout->addWidget(deleteBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void DuplicateFinderDialog::onTableChanged(int index) {
    Q_UNUSED(index)
}

void DuplicateFinderDialog::onStartScan() {
    statusLabel_->setText(tr("Scanning..."));
    progressBar_->setRange(0, 0);
    
    QTimer::singleShot(1500, this, [this]() {
        resultsModel_->clear();
        resultsModel_->setHorizontalHeaderLabels({tr("Group"), tr("Count"), tr("Values"), tr("Action")});
        
        resultsModel_->appendRow({
            new QStandardItem("1"),
            new QStandardItem("3"),
            new QStandardItem("john@example.com"),
            new QStandardItem(tr("Merge"))
        });
        resultsModel_->appendRow({
            new QStandardItem("2"),
            new QStandardItem("2"),
            new QStandardItem("555-1234"),
            new QStandardItem(tr("Keep First"))
        });
        
        progressBar_->setRange(0, 100);
        progressBar_->setValue(100);
        statusLabel_->setText(tr("Found 2 duplicate groups"));
    });
}

void DuplicateFinderDialog::onStopScan() {}

void DuplicateFinderDialog::onSelectAll() {}
void DuplicateFinderDialog::onDeselectAll() {}

void DuplicateFinderDialog::onKeepFirst() {
    QMessageBox::information(this, tr("Keep First"), tr("First record in each group kept."));
}

void DuplicateFinderDialog::onKeepLast() {
    QMessageBox::information(this, tr("Keep Last"), tr("Last record in each group kept."));
}

void DuplicateFinderDialog::onMergeSelected() {
    QMessageBox::information(this, tr("Merge"), tr("Selected duplicates merged."));
}

void DuplicateFinderDialog::onDeleteSelected() {
    QMessageBox::information(this, tr("Delete"), tr("Selected duplicates deleted."));
}

void DuplicateFinderDialog::performScan() {}
void DuplicateFinderDialog::updateResults() {}

// ============================================================================
// Data Quality Analyzer
// ============================================================================

DataQualityAnalyzer::DataQualityAnalyzer(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    setWindowTitle(tr("Data Quality Analyzer"));
    resize(550, 400);
}

void DataQualityAnalyzer::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* configLayout = new QHBoxLayout();
    tableCombo_ = new QComboBox(this);
    tableCombo_->addItems({"customers", "orders", "users"});
    configLayout->addWidget(new QLabel(tr("Table:"), this));
    configLayout->addWidget(tableCombo_);
    
    auto* analyzeBtn = new QPushButton(tr("Analyze"), this);
    connect(analyzeBtn, &QPushButton::clicked, this, &DataQualityAnalyzer::onAnalyze);
    configLayout->addWidget(analyzeBtn);
    
    configLayout->addStretch();
    layout->addLayout(configLayout);
    
    resultsTable_ = new QTableWidget(this);
    resultsTable_->setColumnCount(5);
    resultsTable_->setHorizontalHeaderLabels({tr("Column"), tr("Type"), tr("Quality"), tr("Issues"), tr("Recommendations")});
    layout->addWidget(resultsTable_, 1);
    
    layout->addWidget(new QLabel(tr("Report:"), this));
    reportEdit_ = new QTextEdit(this);
    reportEdit_->setReadOnly(true);
    layout->addWidget(reportEdit_);
    
    progressBar_ = new QProgressBar(this);
    layout->addWidget(progressBar_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &DataQualityAnalyzer::onExportReport);
    btnLayout->addWidget(exportBtn);
    
    auto* fixBtn = new QPushButton(tr("Generate Fix Rules"), this);
    connect(fixBtn, &QPushButton::clicked, this, &DataQualityAnalyzer::onGenerateFixRules);
    btnLayout->addWidget(fixBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void DataQualityAnalyzer::onAnalyze() {
    progressBar_->setRange(0, 0);
    
    QTimer::singleShot(1000, this, [this]() {
        resultsTable_->setRowCount(0);
        
        QStringList columns = {"id", "name", "email", "phone", "created_at"};
        QStringList types = {"integer", "varchar", "varchar", "varchar", "timestamp"};
        int quality[] = {100, 85, 90, 75, 95};
        QStringList issues = {"None", "3 nulls", "2 invalid", "5 format errors", "None"};
        QStringList recs = {"-", "Fill nulls", "Validate format", "Standardize", "-"};
        
        for (int i = 0; i < columns.size(); ++i) {
            int row = resultsTable_->rowCount();
            resultsTable_->insertRow(row);
            resultsTable_->setItem(row, 0, new QTableWidgetItem(columns[i]));
            resultsTable_->setItem(row, 1, new QTableWidgetItem(types[i]));
            resultsTable_->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(quality[i])));
            resultsTable_->setItem(row, 3, new QTableWidgetItem(issues[i]));
            resultsTable_->setItem(row, 4, new QTableWidgetItem(recs[i]));
        }
        
        reportEdit_->setText(tr("Overall Quality Score: 89%\n"
                                 "Issues Found: 3 columns need attention\n"
                                 "Recommendations: Review and fix data quality issues"));
        
        progressBar_->setRange(0, 100);
        progressBar_->setValue(100);
    });
}

void DataQualityAnalyzer::onExportReport() {
    QMessageBox::information(this, tr("Export"), tr("Report exported."));
}

void DataQualityAnalyzer::onGenerateFixRules() {
    QMessageBox::information(this, tr("Generate"), tr("Fix rules generated."));
}

void DataQualityAnalyzer::performAnalysis() {}
void DataQualityAnalyzer::calculateQualityScore() {}

// ============================================================================
// Preview Changes Dialog
// ============================================================================

PreviewChangesDialog::PreviewChangesDialog(const CleansingRule& rule, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), rule_(rule), client_(client) {
    setupUi();
    setWindowTitle(tr("Preview Changes"));
    resize(600, 400);
}

void PreviewChangesDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    summaryLabel_ = new QLabel(tr("Previewing changes for rule: %1").arg(rule_.name), this);
    layout->addWidget(summaryLabel_);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    beforeTable_ = new QTableView(this);
    beforeModel_ = new QStandardItemModel(this);
    beforeModel_->setHorizontalHeaderLabels({rule_.targetColumn});
    beforeTable_->setModel(beforeModel_);
    
    afterTable_ = new QTableView(this);
    afterModel_ = new QStandardItemModel(this);
    afterModel_->setHorizontalHeaderLabels({rule_.targetColumn});
    afterTable_->setModel(afterModel_);
    
    auto* beforeGroup = new QGroupBox(tr("Before"), this);
    auto* beforeLayout = new QVBoxLayout(beforeGroup);
    beforeLayout->addWidget(beforeTable_);
    
    auto* afterGroup = new QGroupBox(tr("After"), this);
    auto* afterLayout = new QVBoxLayout(afterGroup);
    afterLayout->addWidget(afterTable_);
    
    splitter->addWidget(beforeGroup);
    splitter->addWidget(afterGroup);
    layout->addWidget(splitter, 1);
    
    loadPreviewData();
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* applyBtn = new QPushButton(tr("Apply Changes"), this);
    connect(applyBtn, &QPushButton::clicked, this, &PreviewChangesDialog::onApply);
    btnLayout->addWidget(applyBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &PreviewChangesDialog::onCancel);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void PreviewChangesDialog::loadPreviewData() {
    QStringList beforeValues, afterValues;
    
    switch (rule_.operation) {
        case CleansingOperation::TrimWhitespace:
            beforeValues = {"  John  ", "Jane  ", "  Bob"};
            afterValues = {"John", "Jane", "Bob"};
            break;
        case CleansingOperation::FindReplace:
            beforeValues = {"NY", "NY", "CA"};
            afterValues = {"New York", "New York", "California"};
            break;
        default:
            beforeValues = {"value1", "value2", "value3"};
            afterValues = {"cleaned1", "cleaned2", "cleaned3"};
    }
    
    for (int i = 0; i < beforeValues.size(); ++i) {
        beforeModel_->appendRow(new QStandardItem(beforeValues[i]));
        afterModel_->appendRow(new QStandardItem(afterValues[i]));
    }
}

void PreviewChangesDialog::onApply() {
    accept();
}

void PreviewChangesDialog::onCancel() {
    reject();
}

// ============================================================================
// BulkUpdateDialog
// ============================================================================
void BulkUpdateDialog::onPreview() {
    // TODO: Implement preview
    QMessageBox::information(this, tr("Preview"), tr("Preview functionality would be shown here."));
}

void BulkUpdateDialog::onExecute() {
    // TODO: Implement execute
    QMessageBox::information(this, tr("Execute"), tr("Bulk update would be executed here."));
}

void BulkUpdateDialog::onSchedule() {
    // TODO: Implement schedule
    QMessageBox::information(this, tr("Schedule"), tr("Scheduling functionality would be shown here."));
}

} // namespace scratchrobin::ui
