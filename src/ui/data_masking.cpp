#include "data_masking.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
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
#include <QSplitter>
#include <QSpinBox>
#include <QRadioButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QStackedWidget>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Data Masking Panel
// ============================================================================

DataMaskingPanel::DataMaskingPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_masking", parent)
    , client_(client) {
    setupUi();
    loadRules();
}

void DataMaskingPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* createBtn = new QPushButton(tr("New Rule..."), this);
    connect(createBtn, &QPushButton::clicked, this, &DataMaskingPanel::onCreateRule);
    toolbarLayout->addWidget(createBtn);
    
    auto* scanBtn = new QPushButton(tr("Scan for Sensitive Data"), this);
    connect(scanBtn, &QPushButton::clicked, this, &DataMaskingPanel::onScanForSensitiveData);
    toolbarLayout->addWidget(scanBtn);
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &DataMaskingPanel::onPreviewMasking);
    toolbarLayout->addWidget(previewBtn);
    
    toolbarLayout->addStretch();
    
    auto* policyBtn = new QPushButton(tr("Policies..."), this);
    connect(policyBtn, &QPushButton::clicked, this, &DataMaskingPanel::onCreatePolicy);
    toolbarLayout->addWidget(policyBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main content
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Rules table
    rulesTable_ = new QTableView(this);
    rulesModel_ = new QStandardItemModel(this);
    rulesModel_->setHorizontalHeaderLabels({tr("Rule"), tr("Table.Column"), tr("Type"), tr("Masking"), tr("Status")});
    rulesTable_->setModel(rulesModel_);
    rulesTable_->setAlternatingRowColors(true);
    rulesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(rulesTable_, &QTableView::clicked, this, &DataMaskingPanel::onRuleSelected);
    splitter->addWidget(rulesTable_);
    
    // Rule details
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* infoGroup = new QGroupBox(tr("Rule Details"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    ruleNameLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Name:"), ruleNameLabel_);
    
    tableColumnLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Table.Column:"), tableColumnLabel_);
    
    dataTypeLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Data Type:"), dataTypeLabel_);
    
    maskingTypeLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Masking Type:"), maskingTypeLabel_);
    
    statusLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Status:"), statusLabel_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setReadOnly(true);
    descriptionEdit_->setMaximumHeight(80);
    infoLayout->addRow(tr("Description:"), descriptionEdit_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Quick actions
    auto* actionsGroup = new QGroupBox(tr("Quick Actions"), this);
    auto* actionsLayout = new QVBoxLayout(actionsGroup);
    
    auto* compareBtn = new QPushButton(tr("Compare Before/After"), this);
    connect(compareBtn, &QPushButton::clicked, this, &DataMaskingPanel::onCompareBeforeAfter);
    actionsLayout->addWidget(compareBtn);
    
    auto* exportBtn = new QPushButton(tr("Export Rules"), this);
    connect(exportBtn, &QPushButton::clicked, this, &DataMaskingPanel::onExportRules);
    actionsLayout->addWidget(exportBtn);
    
    auto* importBtn = new QPushButton(tr("Import Rules"), this);
    connect(importBtn, &QPushButton::clicked, this, &DataMaskingPanel::onImportRules);
    actionsLayout->addWidget(importBtn);
    
    detailsLayout->addWidget(actionsGroup);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({500, 300});
    
    mainLayout->addWidget(splitter, 1);
}

void DataMaskingPanel::loadRules() {
    rules_.clear();
    
    // Simulate loading rules
    QStringList names = {"SSN Masking", "Credit Card Masking", "Email Masking", "Phone Masking"};
    QStringList columns = {"customers.ssn", "orders.credit_card", "users.email", "users.phone"};
    QStringList types = {"SSN", "Credit Card", "Email", "Phone"};
    QStringList masking = {"Partial", "Partial", "Partial", "Partial"};
    
    for (int i = 0; i < names.size(); ++i) {
        MaskingRule rule;
        rule.id = QString::number(i);
        rule.name = names[i];
        rule.tableName = columns[i].split('.').first();
        rule.columnName = columns[i].split('.').last();
        rule.sensitiveType = static_cast<SensitiveDataType>(i);
        rule.maskingType = MaskingType::Partial;
        rule.enabled = true;
        rules_.append(rule);
    }
    
    updateRulesTable();
}

void DataMaskingPanel::updateRulesTable() {
    rulesModel_->clear();
    rulesModel_->setHorizontalHeaderLabels({tr("Rule"), tr("Table.Column"), tr("Type"), tr("Masking"), tr("Status")});
    
    for (const auto& rule : rules_) {
        QString typeStr;
        switch (rule.sensitiveType) {
            case SensitiveDataType::SSN: typeStr = "SSN"; break;
            case SensitiveDataType::CreditCard: typeStr = "Credit Card"; break;
            case SensitiveDataType::Email: typeStr = "Email"; break;
            case SensitiveDataType::Phone: typeStr = "Phone"; break;
            default: typeStr = "Custom";
        }
        
        QString maskingStr;
        switch (rule.maskingType) {
            case MaskingType::Full: maskingStr = "Full"; break;
            case MaskingType::Partial: maskingStr = "Partial"; break;
            case MaskingType::Hash: maskingStr = "Hash"; break;
            case MaskingType::Tokenization: maskingStr = "Token"; break;
            default: maskingStr = "Other";
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(rule.name);
        row << new QStandardItem(QString("%1.%2").arg(rule.tableName, rule.columnName));
        row << new QStandardItem(typeStr);
        row << new QStandardItem(maskingStr);
        row << new QStandardItem(rule.enabled ? tr("Enabled") : tr("Disabled"));
        rulesModel_->appendRow(row);
    }
}

void DataMaskingPanel::updateRuleDetails(const MaskingRule& rule) {
    ruleNameLabel_->setText(rule.name);
    tableColumnLabel_->setText(QString("%1.%2").arg(rule.tableName, rule.columnName));
    dataTypeLabel_->setText("VARCHAR");
    
    QString maskingStr;
    switch (rule.maskingType) {
        case MaskingType::Full: maskingStr = tr("Full Masking"); break;
        case MaskingType::Partial: maskingStr = tr("Partial Masking (show last 4)"); break;
        case MaskingType::Hash: maskingStr = tr("Hash"); break;
        case MaskingType::Tokenization: maskingStr = tr("Tokenization"); break;
        default: maskingStr = tr("Other");
    }
    maskingTypeLabel_->setText(maskingStr);
    statusLabel_->setText(rule.enabled ? tr("Enabled") : tr("Disabled"));
    descriptionEdit_->setText(rule.description);
}

void DataMaskingPanel::onCreateRule() {
    MaskingRule newRule;
    MaskingRuleDialog dialog(&newRule, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        newRule.id = QString::number(rules_.size());
        rules_.append(newRule);
        updateRulesTable();
        emit ruleCreated(newRule.id);
    }
}

void DataMaskingPanel::onEditRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    MaskingRuleDialog dialog(&rules_[index.row()], client_, this);
    dialog.exec();
    updateRulesTable();
    emit ruleModified(rules_[index.row()].id);
}

void DataMaskingPanel::onDeleteRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    QString ruleId = rules_[index.row()].id;
    auto reply = QMessageBox::warning(this, tr("Delete Rule"),
        tr("Delete masking rule '%1'?").arg(rules_[index.row()].name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        rules_.removeAt(index.row());
        updateRulesTable();
        emit ruleDeleted(ruleId);
    }
}

void DataMaskingPanel::onDuplicateRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    MaskingRule clone = rules_[index.row()];
    clone.id = QString::number(rules_.size());
    clone.name += " (Copy)";
    
    MaskingRuleDialog dialog(&clone, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        rules_.append(clone);
        updateRulesTable();
    }
}

void DataMaskingPanel::onToggleRule(bool enabled) {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    rules_[index.row()].enabled = enabled;
    updateRulesTable();
}

void DataMaskingPanel::onCreatePolicy() {
    PolicyManagementDialog dialog(&policies_, this);
    dialog.exec();
}

void DataMaskingPanel::onEditPolicy() {
    onCreatePolicy();
}

void DataMaskingPanel::onDeletePolicy() {
    // Delete policy
}

void DataMaskingPanel::onApplyPolicy() {
    QMessageBox::information(this, tr("Apply Policy"),
        tr("Masking policy would be applied to the database."));
}

void DataMaskingPanel::onScanForSensitiveData() {
    SensitiveDataScanner dialog(client_, this);
    dialog.exec();
}

void DataMaskingPanel::onAutoGenerateRules() {
    QMessageBox::information(this, tr("Auto-Generate"),
        tr("Scanning for sensitive data and generating rules..."));
}

void DataMaskingPanel::onRuleSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= rules_.size()) return;
    updateRuleDetails(rules_[index.row()]);
}

void DataMaskingPanel::onPreviewMasking() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    MaskingPreviewDialog dialog(rules_[index.row()], client_, this);
    dialog.exec();
}

void DataMaskingPanel::onCompareBeforeAfter() {
    onPreviewMasking();
}

void DataMaskingPanel::onExportRules() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Rules"),
        QString(),
        tr("JSON Files (*.json);;XML Files (*.xml)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Rules exported successfully."));
    }
}

void DataMaskingPanel::onImportRules() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Rules"),
        QString(),
        tr("JSON Files (*.json);;XML Files (*.xml)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("Rules imported successfully."));
        loadRules();
    }
}

// ============================================================================
// Masking Rule Dialog
// ============================================================================

MaskingRuleDialog::MaskingRuleDialog(MaskingRule* rule, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , rule_(rule)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Masking Rule"));
    resize(600, 500);
}

void MaskingRuleDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    pages_ = new QStackedWidget(this);
    
    setupBasicPage();
    setupMaskingOptionsPage();
    setupConditionsPage();
    
    layout->addWidget(pages_, 1);
    
    // Navigation
    auto* navLayout = new QHBoxLayout();
    auto* basicBtn = new QPushButton(tr("Basic"), this);
    connect(basicBtn, &QPushButton::clicked, [this]() { pages_->setCurrentIndex(0); });
    navLayout->addWidget(basicBtn);
    
    auto* optionsBtn = new QPushButton(tr("Options"), this);
    connect(optionsBtn, &QPushButton::clicked, [this]() { pages_->setCurrentIndex(1); });
    navLayout->addWidget(optionsBtn);
    
    auto* conditionsBtn = new QPushButton(tr("Conditions"), this);
    connect(conditionsBtn, &QPushButton::clicked, [this]() { pages_->setCurrentIndex(2); });
    navLayout->addWidget(conditionsBtn);
    
    navLayout->addStretch();
    layout->addLayout(navLayout);
    
    // Preview
    layout->addWidget(new QLabel(tr("Preview:"), this));
    previewTable_ = new QTableView(this);
    previewModel_ = new QStandardItemModel(this);
    previewModel_->setHorizontalHeaderLabels({tr("Original"), tr("Masked")});
    previewTable_->setModel(previewModel_);
    previewTable_->setMaximumHeight(100);
    layout->addWidget(previewTable_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &MaskingRuleDialog::onPreview);
    btnLayout->addWidget(previewBtn);
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &MaskingRuleDialog::onSave);
    btnLayout->addWidget(saveBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void MaskingRuleDialog::setupBasicPage() {
    auto* page = new QWidget();
    auto* layout = new QFormLayout(page);
    
    nameEdit_ = new QLineEdit(rule_->name, this);
    layout->addRow(tr("Rule Name:"), nameEdit_);
    
    descriptionEdit_ = new QTextEdit(rule_->description, this);
    descriptionEdit_->setMaximumHeight(60);
    layout->addRow(tr("Description:"), descriptionEdit_);
    
    tableCombo_ = new QComboBox(this);
    tableCombo_->addItems({"customers", "orders", "users", "employees"});
    layout->addRow(tr("Table:"), tableCombo_);
    
    columnCombo_ = new QComboBox(this);
    columnCombo_->addItems({"ssn", "email", "phone", "credit_card"});
    layout->addRow(tr("Column:"), columnCombo_);
    
    sensitiveTypeCombo_ = new QComboBox(this);
    sensitiveTypeCombo_->addItem(tr("SSN"), static_cast<int>(SensitiveDataType::SSN));
    sensitiveTypeCombo_->addItem(tr("Credit Card"), static_cast<int>(SensitiveDataType::CreditCard));
    sensitiveTypeCombo_->addItem(tr("Email"), static_cast<int>(SensitiveDataType::Email));
    sensitiveTypeCombo_->addItem(tr("Phone"), static_cast<int>(SensitiveDataType::Phone));
    sensitiveTypeCombo_->addItem(tr("Name"), static_cast<int>(SensitiveDataType::Name));
    layout->addRow(tr("Sensitive Type:"), sensitiveTypeCombo_);
    
    maskingTypeCombo_ = new QComboBox(this);
    maskingTypeCombo_->addItem(tr("Full"), static_cast<int>(MaskingType::Full));
    maskingTypeCombo_->addItem(tr("Partial"), static_cast<int>(MaskingType::Partial));
    maskingTypeCombo_->addItem(tr("Hash"), static_cast<int>(MaskingType::Hash));
    maskingTypeCombo_->addItem(tr("Tokenization"), static_cast<int>(MaskingType::Tokenization));
    connect(maskingTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MaskingRuleDialog::onMaskingTypeChanged);
    layout->addRow(tr("Masking Type:"), maskingTypeCombo_);
    
    pages_->addWidget(page);
}

void MaskingRuleDialog::setupMaskingOptionsPage() {
    auto* page = new QWidget();
    optionsStack_ = new QStackedWidget(page);
    
    auto* layout = new QVBoxLayout(page);
    layout->addWidget(optionsStack_);
    
    // Partial masking options
    auto* partialPage = new QWidget();
    auto* partialLayout = new QFormLayout(partialPage);
    showFirstSpin_ = new QSpinBox(this);
    showFirstSpin_->setRange(0, 10);
    partialLayout->addRow(tr("Show First:"), showFirstSpin_);
    
    showLastSpin_ = new QSpinBox(this);
    showLastSpin_->setRange(0, 10);
    showLastSpin_->setValue(4);
    partialLayout->addRow(tr("Show Last:"), showLastSpin_);
    
    maskCharEdit_ = new QLineEdit("*", this);
    maskCharEdit_->setMaxLength(1);
    partialLayout->addRow(tr("Mask Character:"), maskCharEdit_);
    
    optionsStack_->addWidget(partialPage);
    
    // Hash options
    auto* hashPage = new QWidget();
    auto* hashLayout = new QFormLayout(hashPage);
    algorithmCombo_ = new QComboBox(this);
    algorithmCombo_->addItems({"SHA-256", "SHA-512", "MD5"});
    hashLayout->addRow(tr("Algorithm:"), algorithmCombo_);
    optionsStack_->addWidget(hashPage);
    
    pages_->addWidget(page);
}

void MaskingRuleDialog::setupConditionsPage() {
    auto* page = new QWidget();
    auto* layout = new QFormLayout(page);
    
    conditionalCheck_ = new QCheckBox(tr("Enable conditional masking"), this);
    layout->addRow(conditionalCheck_);
    
    conditionColumnCombo_ = new QComboBox(this);
    conditionColumnCombo_->addItems({"role", "department", "access_level"});
    layout->addRow(tr("Condition Column:"), conditionColumnCombo_);
    
    conditionOpCombo_ = new QComboBox(this);
    conditionOpCombo_->addItems({"equals", "not equals", "in", "not in"});
    layout->addRow(tr("Operator:"), conditionOpCombo_);
    
    conditionValueEdit_ = new QLineEdit(this);
    layout->addRow(tr("Value:"), conditionValueEdit_);
    
    pages_->addWidget(page);
}

void MaskingRuleDialog::onSensitiveTypeChanged(int index) {
    Q_UNUSED(index)
}

void MaskingRuleDialog::onMaskingTypeChanged(int index) {
    Q_UNUSED(index)
    MaskingType type = static_cast<MaskingType>(maskingTypeCombo_->currentData().toInt());
    
    switch (type) {
        case MaskingType::Partial:
            optionsStack_->setCurrentIndex(0);
            break;
        case MaskingType::Hash:
            optionsStack_->setCurrentIndex(1);
            break;
        default:
            break;
    }
}

void MaskingRuleDialog::onTableChanged(int index) {
    Q_UNUSED(index)
}

void MaskingRuleDialog::onPreview() {
    previewModel_->clear();
    previewModel_->setHorizontalHeaderLabels({tr("Original"), tr("Masked")});
    
    previewModel_->appendRow({new QStandardItem("123-45-6789"), new QStandardItem("***-**-6789")});
    previewModel_->appendRow({new QStandardItem("john@example.com"), new QStandardItem("j***@example.com")});
}

void MaskingRuleDialog::onSave() {
    rule_->name = nameEdit_->text();
    rule_->description = descriptionEdit_->toPlainText();
    rule_->tableName = tableCombo_->currentText();
    rule_->columnName = columnCombo_->currentText();
    rule_->sensitiveType = static_cast<SensitiveDataType>(sensitiveTypeCombo_->currentData().toInt());
    rule_->maskingType = static_cast<MaskingType>(maskingTypeCombo_->currentData().toInt());
    rule_->showFirst = showFirstSpin_->value();
    rule_->showLast = showLastSpin_->value();
    rule_->maskChar = maskCharEdit_->text();
    rule_->enabled = true;
    
    accept();
}

void MaskingRuleDialog::onCancel() {
    reject();
}

// ============================================================================
// Sensitive Data Scanner
// ============================================================================

SensitiveDataScanner::SensitiveDataScanner(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Scan for Sensitive Data"));
    resize(650, 450);
}

void SensitiveDataScanner::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("This will scan all tables for potentially sensitive data columns."), this));
    
    // Results table
    resultsTable_ = new QTableView(this);
    resultsModel_ = new QStandardItemModel(this);
    resultsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Column"), tr("Type"), tr("Confidence"), tr("Select")});
    resultsTable_->setModel(resultsModel_);
    resultsTable_->setAlternatingRowColors(true);
    layout->addWidget(resultsTable_, 1);
    
    progressBar_ = new QProgressBar(this);
    layout->addWidget(progressBar_);
    
    statusLabel_ = new QLabel(tr("Ready to scan"), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    scanBtn_ = new QPushButton(tr("Start Scan"), this);
    connect(scanBtn_, &QPushButton::clicked, this, &SensitiveDataScanner::onStartScan);
    btnLayout->addWidget(scanBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &SensitiveDataScanner::onStopScan);
    btnLayout->addWidget(stopBtn_);
    
    btnLayout->addStretch();
    
    auto* selectAllBtn = new QPushButton(tr("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &SensitiveDataScanner::onSelectAll);
    btnLayout->addWidget(selectAllBtn);
    
    auto* generateBtn = new QPushButton(tr("Generate Rules"), this);
    connect(generateBtn, &QPushButton::clicked, this, &SensitiveDataScanner::onGenerateRules);
    btnLayout->addWidget(generateBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void SensitiveDataScanner::onStartScan() {
    scanning_ = true;
    scanBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("Scanning..."));
    progressBar_->setRange(0, 0);
    
    // Simulate scan
    QTimer::singleShot(2000, this, [this]() {
        addDetectedColumn("public", "customers", "ssn", SensitiveDataType::SSN);
        addDetectedColumn("public", "customers", "credit_card", SensitiveDataType::CreditCard);
        addDetectedColumn("public", "users", "email", SensitiveDataType::Email);
        
        scanning_ = false;
        scanBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        progressBar_->setRange(0, 100);
        progressBar_->setValue(100);
        statusLabel_->setText(tr("Scan complete. Found %1 sensitive columns.").arg(detectedColumns_.size()));
    });
}

void SensitiveDataScanner::onStopScan() {
    scanning_ = false;
    scanBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    statusLabel_->setText(tr("Scan stopped"));
}

void SensitiveDataScanner::onSelectAll() {
    // Select all detected columns
}

void SensitiveDataScanner::onDeselectAll() {
    // Deselect all
}

void SensitiveDataScanner::onGenerateRules() {
    QMessageBox::information(this, tr("Generate Rules"),
        tr("Would create masking rules for selected columns."));
    accept();
}

void SensitiveDataScanner::onExportResults() {
    // Export scan results
}

void SensitiveDataScanner::addDetectedColumn(const QString& schema, const QString& table,
                                              const QString& column, SensitiveDataType type) {
    DetectedColumn col;
    col.schema = schema;
    col.table = table;
    col.column = column;
    col.detectedType = type;
    col.confidence = 95;
    col.selected = true;
    detectedColumns_.append(col);
    
    QString typeStr;
    switch (type) {
        case SensitiveDataType::SSN: typeStr = "SSN"; break;
        case SensitiveDataType::CreditCard: typeStr = "Credit Card"; break;
        case SensitiveDataType::Email: typeStr = "Email"; break;
        default: typeStr = "Other";
    }
    
    QList<QStandardItem*> row;
    row << new QStandardItem(table);
    row << new QStandardItem(column);
    row << new QStandardItem(typeStr);
    row << new QStandardItem("95%");
    row << new QStandardItem(tr("Yes"));
    resultsModel_->appendRow(row);
}

void SensitiveDataScanner::performScan() {
    // Scan implementation
}

// ============================================================================
// Masking Preview Dialog
// ============================================================================

MaskingPreviewDialog::MaskingPreviewDialog(const MaskingRule& rule, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , rule_(rule)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Masking Preview - %1").arg(rule.name));
    resize(600, 400);
}

void MaskingPreviewDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Rule: %1").arg(rule_.name), this));
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Before
    auto* beforeGroup = new QGroupBox(tr("Before Masking"), this);
    auto* beforeLayout = new QVBoxLayout(beforeGroup);
    beforeTable_ = new QTableView(this);
    beforeModel_ = new QStandardItemModel(this);
    beforeTable_->setModel(beforeModel_);
    beforeLayout->addWidget(beforeTable_);
    splitter->addWidget(beforeGroup);
    
    // After
    auto* afterGroup = new QGroupBox(tr("After Masking"), this);
    auto* afterLayout = new QVBoxLayout(afterGroup);
    afterTable_ = new QTableView(this);
    afterModel_ = new QStandardItemModel(this);
    afterTable_->setModel(afterModel_);
    afterLayout->addWidget(afterTable_);
    splitter->addWidget(afterGroup);
    
    layout->addWidget(splitter, 1);
    
    loadPreviewData();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &MaskingPreviewDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void MaskingPreviewDialog::loadPreviewData() {
    beforeModel_->clear();
    afterModel_->clear();
    
    beforeModel_->setHorizontalHeaderLabels({rule_.columnName});
    afterModel_->setHorizontalHeaderLabels({rule_.columnName});
    
    QStringList samples;
    switch (rule_.sensitiveType) {
        case SensitiveDataType::SSN:
            samples = {"123-45-6789", "987-65-4321", "456-78-9123"};
            break;
        case SensitiveDataType::Email:
            samples = {"john@example.com", "jane@test.org", "bob@company.net"};
            break;
        case SensitiveDataType::Phone:
            samples = {"555-123-4567", "555-987-6543", "555-456-7890"};
            break;
        default:
            samples = {"value1", "value2", "value3"};
    }
    
    for (const auto& value : samples) {
        beforeModel_->appendRow(new QStandardItem(value));
        afterModel_->appendRow(new QStandardItem(applyMasking(value)));
    }
}

QString MaskingPreviewDialog::applyMasking(const QString& value) {
    if (rule_.maskingType == MaskingType::Partial) {
        if (value.length() <= rule_.showLast) return value;
        
        QString masked;
        masked.fill(rule_.maskChar.at(0), value.length() - rule_.showLast);
        masked += value.right(rule_.showLast);
        return masked;
    }
    return QString(rule_.maskChar).repeated(8);
}

void MaskingPreviewDialog::onRefresh() {
    loadPreviewData();
}

void MaskingPreviewDialog::onExport() {
    // Export preview
}

// ============================================================================
// Policy Management Dialog
// ============================================================================

PolicyManagementDialog::PolicyManagementDialog(QList<MaskingPolicy>* policies, QWidget* parent)
    : QDialog(parent)
    , policies_(policies) {
    setupUi();
    setWindowTitle(tr("Masking Policies"));
    resize(550, 400);
}

void PolicyManagementDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Policy list
    policyList_ = new QListWidget(this);
    policyList_->addItem(tr("Default Policy"));
    policyList_->addItem(tr("Developer Access"));
    policyList_->addItem(tr("External API"));
    splitter->addWidget(policyList_);
    
    // Policy details
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    
    enabledCheck_ = new QCheckBox(tr("Enabled"), this);
    enabledCheck_->setChecked(true);
    detailsLayout->addWidget(enabledCheck_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setPlaceholderText(tr("Policy description..."));
    detailsLayout->addWidget(descriptionEdit_);
    
    detailsLayout->addWidget(new QLabel(tr("Applied Rules:"), this));
    
    rulesTable_ = new QTableWidget(this);
    rulesTable_->setColumnCount(2);
    rulesTable_->setHorizontalHeaderLabels({tr("Rule"), tr("Status")});
    detailsLayout->addWidget(rulesTable_);
    
    detailsLayout->addWidget(new QLabel(tr("Applies to Roles:"), this));
    
    rolesList_ = new QListWidget(this);
    rolesList_->addItems({"analyst", "developer", "readonly", "external"});
    detailsLayout->addWidget(rolesList_);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({150, 350});
    
    layout->addWidget(splitter, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* addBtn = new QPushButton(tr("Add Policy"), this);
    connect(addBtn, &QPushButton::clicked, this, &PolicyManagementDialog::onCreatePolicy);
    btnLayout->addWidget(addBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &PolicyManagementDialog::onDeletePolicy);
    btnLayout->addWidget(deleteBtn);
    
    btnLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &PolicyManagementDialog::onSaveChanges);
    btnLayout->addWidget(saveBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void PolicyManagementDialog::onCreatePolicy() {
    policyList_->addItem(tr("New Policy"));
}

void PolicyManagementDialog::onEditPolicy() {
    // Edit selected policy
}

void PolicyManagementDialog::onDeletePolicy() {
    delete policyList_->takeItem(policyList_->currentRow());
}

void PolicyManagementDialog::onClonePolicy() {
    // Clone policy
}

void PolicyManagementDialog::onPolicySelected(const QModelIndex& index) {
    Q_UNUSED(index)
}

void PolicyManagementDialog::onSaveChanges() {
    QMessageBox::information(this, tr("Save"), tr("Policies saved."));
}

} // namespace scratchrobin::ui
