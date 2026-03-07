#include "schema_compare.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Schema Compare Panel
// ============================================================================

SchemaComparePanel::SchemaComparePanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("schema_compare", parent)
    , client_(client) {
    setupUi();
    setupModels();
}

void SchemaComparePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(6);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    
    // Schema selection area
    auto* selectionGroup = new QGroupBox(tr("Schema Selection"), this);
    auto* selectionLayout = new QGridLayout(selectionGroup);
    
    selectionLayout->addWidget(new QLabel(tr("Source:")), 0, 0);
    sourceConnectionCombo_ = new QComboBox(this);
    sourceSchemaCombo_ = new QComboBox(this);
    selectionLayout->addWidget(sourceConnectionCombo_, 0, 1);
    selectionLayout->addWidget(sourceSchemaCombo_, 0, 2);
    
    selectionLayout->addWidget(new QLabel(tr("Target:")), 1, 0);
    targetConnectionCombo_ = new QComboBox(this);
    targetSchemaCombo_ = new QComboBox(this);
    selectionLayout->addWidget(targetConnectionCombo_, 1, 1);
    selectionLayout->addWidget(targetSchemaCombo_, 1, 2);
    
    auto* compareBtn = new QPushButton(tr("Compare"), this);
    connect(compareBtn, &QPushButton::clicked, this, &SchemaComparePanel::onRunComparison);
    selectionLayout->addWidget(compareBtn, 0, 3, 2, 1);
    
    mainLayout->addWidget(selectionGroup);
    
    // Options area
    auto* optionsGroup = new QGroupBox(tr("Comparison Options"), this);
    auto* optionsLayout = new QGridLayout(optionsGroup);
    
    compareTablesCheck_ = new QCheckBox(tr("Tables"), this);
    compareTablesCheck_->setChecked(true);
    compareViewsCheck_ = new QCheckBox(tr("Views"), this);
    compareViewsCheck_->setChecked(true);
    compareFunctionsCheck_ = new QCheckBox(tr("Functions/Procedures"), this);
    compareFunctionsCheck_->setChecked(true);
    compareIndexesCheck_ = new QCheckBox(tr("Indexes"), this);
    compareIndexesCheck_->setChecked(true);
    compareConstraintsCheck_ = new QCheckBox(tr("Constraints"), this);
    compareConstraintsCheck_->setChecked(true);
    compareTriggersCheck_ = new QCheckBox(tr("Triggers"), this);
    compareTriggersCheck_->setChecked(false);
    comparePrivilegesCheck_ = new QCheckBox(tr("Privileges"), this);
    comparePrivilegesCheck_->setChecked(false);
    
    optionsLayout->addWidget(compareTablesCheck_, 0, 0);
    optionsLayout->addWidget(compareViewsCheck_, 0, 1);
    optionsLayout->addWidget(compareFunctionsCheck_, 0, 2);
    optionsLayout->addWidget(compareIndexesCheck_, 1, 0);
    optionsLayout->addWidget(compareConstraintsCheck_, 1, 1);
    optionsLayout->addWidget(compareTriggersCheck_, 1, 2);
    optionsLayout->addWidget(comparePrivilegesCheck_, 1, 3);
    
    ignoreWhitespaceCheck_ = new QCheckBox(tr("Ignore whitespace differences"), this);
    ignoreCommentsCheck_ = new QCheckBox(tr("Ignore comments"), this);
    optionsLayout->addWidget(ignoreWhitespaceCheck_, 2, 0, 1, 2);
    optionsLayout->addWidget(ignoreCommentsCheck_, 2, 2, 1, 2);
    
    mainLayout->addWidget(optionsGroup);
    
    // Summary
    auto* summaryGroup = new QGroupBox(tr("Summary"), this);
    auto* summaryLayout = new QHBoxLayout(summaryGroup);
    summaryLabel_ = new QLabel(tr("No comparison run yet"), this);
    identicalLabel_ = new QLabel(tr("Identical: 0"), this);
    createdLabel_ = new QLabel(tr("<font color='green'>Only in Source: 0</font>"), this);
    droppedLabel_ = new QLabel(tr("<font color='red'>Only in Target: 0</font>"), this);
    modifiedLabel_ = new QLabel(tr("<font color='orange'>Modified: 0</font>"), this);
    summaryLayout->addWidget(summaryLabel_);
    summaryLayout->addStretch();
    summaryLayout->addWidget(identicalLabel_);
    summaryLayout->addWidget(createdLabel_);
    summaryLayout->addWidget(droppedLabel_);
    summaryLayout->addWidget(modifiedLabel_);
    mainLayout->addWidget(summaryGroup);
    
    // Main splitter for results
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Diff tree
    diffTree_ = new QTreeView(this);
    diffTree_->setHeaderHidden(false);
    diffTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    diffTree_->setAlternatingRowColors(true);
    connect(diffTree_, &QTreeView::clicked, this, &SchemaComparePanel::onObjectSelected);
    splitter->addWidget(diffTree_);
    
    // Detail tabs
    tabWidget_ = new QTabWidget(this);
    sourceDdlEdit_ = new QTextEdit(this);
    sourceDdlEdit_->setReadOnly(true);
    sourceDdlEdit_->setFont(QFont("Consolas", 9));
    tabWidget_->addTab(sourceDdlEdit_, tr("Source DDL"));
    
    targetDdlEdit_ = new QTextEdit(this);
    targetDdlEdit_->setReadOnly(true);
    targetDdlEdit_->setFont(QFont("Consolas", 9));
    tabWidget_->addTab(targetDdlEdit_, tr("Target DDL"));
    
    diffEdit_ = new QTextEdit(this);
    diffEdit_->setReadOnly(true);
    diffEdit_->setFont(QFont("Consolas", 9));
    tabWidget_->addTab(diffEdit_, tr("Differences"));
    
    scriptEdit_ = new QTextEdit(this);
    scriptEdit_->setReadOnly(true);
    scriptEdit_->setFont(QFont("Consolas", 9));
    tabWidget_->addTab(scriptEdit_, tr("Sync Script"));
    
    splitter->addWidget(tabWidget_);
    splitter->setSizes({300, 500});
    
    mainLayout->addWidget(splitter, 1);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* genScriptBtn = new QPushButton(tr("Generate Script"), this);
    connect(genScriptBtn, &QPushButton::clicked, this, &SchemaComparePanel::onGenerateSyncScript);
    btnLayout->addWidget(genScriptBtn);
    
    auto* applyBtn = new QPushButton(tr("Apply Changes"), this);
    connect(applyBtn, &QPushButton::clicked, this, &SchemaComparePanel::onApplyChanges);
    btnLayout->addWidget(applyBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SchemaComparePanel::onExportResults);
    btnLayout->addWidget(exportBtn);
    
    mainLayout->addLayout(btnLayout);
}

void SchemaComparePanel::setupModels() {
    diffModel_ = new QStandardItemModel(this);
    diffModel_->setHorizontalHeaderLabels({
        tr("Object"), tr("Type"), tr("Status")
    });
    diffTree_->setModel(diffModel_);
    diffTree_->header()->setStretchLastSection(true);
}

void SchemaComparePanel::onSelectSourceSchema() {
    // Load schemas for source connection
}

void SchemaComparePanel::onSelectTargetSchema() {
    // Load schemas for target connection
}

void SchemaComparePanel::onRunComparison() {
    runComparison();
    displayResults();
}

void SchemaComparePanel::runComparison() {
    differences_.clear();
    
    // Simulate comparison results for demo
    SchemaObjectDiff diff1;
    diff1.objectType = "TABLE";
    diff1.objectName = "customers";
    diff1.schemaName = "public";
    diff1.diffType = DiffType::Modified;
    diff1.differences << "Column 'email' type changed from VARCHAR(100) to VARCHAR(255)";
    diff1.differences << "Index 'idx_email' added";
    diff1.sourceDdl = "CREATE TABLE customers (\n  id INT PRIMARY KEY,\n  name VARCHAR(100),\n  email VARCHAR(255)\n);";
    diff1.targetDdl = "CREATE TABLE customers (\n  id INT PRIMARY KEY,\n  name VARCHAR(100),\n  email VARCHAR(100)\n);";
    differences_.append(diff1);
    
    SchemaObjectDiff diff2;
    diff2.objectType = "TABLE";
    diff2.objectName = "orders_new";
    diff2.schemaName = "public";
    diff2.diffType = DiffType::Created;
    diff2.differences << "Table does not exist in target";
    diff2.sourceDdl = "CREATE TABLE orders_new (\n  id INT PRIMARY KEY,\n  customer_id INT,\n  order_date TIMESTAMP\n);";
    diff2.targetDdl = "-- Table does not exist";
    differences_.append(diff2);
    
    SchemaObjectDiff diff3;
    diff3.objectType = "FUNCTION";
    diff3.objectName = "calculate_total";
    diff3.schemaName = "public";
    diff3.diffType = DiffType::Dropped;
    diff3.differences << "Function exists only in target";
    diff3.sourceDdl = "-- Function does not exist";
    diff3.targetDdl = "CREATE FUNCTION calculate_total(amount DECIMAL) RETURNS DECIMAL AS $$\nBEGIN\n  RETURN amount * 1.1;\nEND;\n$$ LANGUAGE plpgsql;";
    differences_.append(diff3);
}

void SchemaComparePanel::displayResults() {
    diffModel_->clear();
    diffModel_->setHorizontalHeaderLabels({
        tr("Object"), tr("Type"), tr("Status")
    });
    
    int identical = 0, created = 0, dropped = 0, modified = 0;
    
    // Group by type
    QMap<QString, QStandardItem*> typeGroups;
    
    for (const auto& diff : differences_) {
        QStandardItem* typeGroup;
        if (!typeGroups.contains(diff.objectType)) {
            typeGroup = new QStandardItem(diff.objectType + "s");
            diffModel_->appendRow(typeGroup);
            typeGroups[diff.objectType] = typeGroup;
        } else {
            typeGroup = typeGroups[diff.objectType];
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(diff.objectName);
        row << new QStandardItem(diff.objectType);
        
        QString statusText;
        QColor statusColor;
        switch (diff.diffType) {
        case DiffType::Identical:
            statusText = tr("Identical");
            statusColor = Qt::gray;
            identical++;
            break;
        case DiffType::Created:
            statusText = tr("Only in Source");
            statusColor = Qt::green;
            created++;
            break;
        case DiffType::Dropped:
            statusText = tr("Only in Target");
            statusColor = Qt::red;
            dropped++;
            break;
        case DiffType::Modified:
            statusText = tr("Modified");
            statusColor = QColor(255, 165, 0);
            modified++;
            break;
        }
        
        auto* statusItem = new QStandardItem(statusText);
        statusItem->setForeground(statusColor);
        row << statusItem;
        
        // Store diff data in first item
        row[0]->setData(QVariant::fromValue(diff.objectName), Qt::UserRole);
        
        typeGroup->appendRow(row);
    }
    
    // Update summary
    summaryLabel_->setText(tr("Comparison complete: %1 differences found").arg(differences_.size()));
    identicalLabel_->setText(tr("Identical: %1").arg(identical));
    createdLabel_->setText(tr("<font color='green'>Only in Source: %1</font>").arg(created));
    droppedLabel_->setText(tr("<font color='red'>Only in Target: %1</font>").arg(dropped));
    modifiedLabel_->setText(tr("<font color='orange'>Modified: %1</font>").arg(modified));
    
    diffTree_->expandAll();
    
    emit comparisonFinished(modified, created, dropped, modified);
}

void SchemaComparePanel::onObjectSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    auto* item = diffModel_->itemFromIndex(index);
    if (!item) return;
    
    QString objectName = item->data(Qt::UserRole).toString();
    
    for (const auto& diff : differences_) {
        if (diff.objectName == objectName) {
            sourceDdlEdit_->setPlainText(diff.sourceDdl);
            targetDdlEdit_->setPlainText(diff.targetDdl);
            diffEdit_->setPlainText(diff.differences.join("\n"));
            
            // Generate sync script for this object
            QString script;
            if (diff.diffType == DiffType::Created) {
                script = diff.sourceDdl + ";";
            } else if (diff.diffType == DiffType::Dropped) {
                script = QString("DROP %1 %2;").arg(diff.objectType, diff.objectName);
            } else if (diff.diffType == DiffType::Modified) {
                script = QString("-- Alter %1 %2\n").arg(diff.objectType, diff.objectName);
                script += "-- Manual review required for modifications\n";
            }
            scriptEdit_->setPlainText(script);
            break;
        }
    }
}

void SchemaComparePanel::onGenerateSyncScript() {
    QString fullScript;
    fullScript += "-- Schema Synchronization Script\n";
    fullScript += "-- Generated by ScratchRobin Schema Compare\n\n";
    fullScript += "BEGIN;\n\n";
    
    for (const auto& diff : differences_) {
        if (diff.diffType == DiffType::Created) {
            fullScript += diff.sourceDdl + ";\n\n";
        } else if (diff.diffType == DiffType::Dropped) {
            fullScript += QString("DROP %1 IF EXISTS %2;\n\n")
                .arg(diff.objectType, diff.objectName);
        } else if (diff.diffType == DiffType::Modified) {
            fullScript += QString("-- TODO: Modify %1 %2\n\n")
                .arg(diff.objectType, diff.objectName);
        }
    }
    
    fullScript += "COMMIT;\n";
    
    auto* dialog = new SchemaSyncScriptDialog(fullScript, this);
    dialog->exec();
}

void SchemaComparePanel::onApplyChanges() {
    QMessageBox::warning(this, tr("Not Implemented"), 
        tr("Direct application of changes is not yet implemented.\n"
           "Please use the generated script to apply changes manually."));
}

void SchemaComparePanel::onExportResults() {
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Export Comparison Results"),
        QString(),
        tr("JSON (*.json);;HTML (*.html);;Text (*.txt)"));
    
    if (fileName.isEmpty()) return;
    
    if (fileName.endsWith(".json")) {
        QJsonArray array;
        for (const auto& diff : differences_) {
            QJsonObject obj;
            obj["objectType"] = diff.objectType;
            obj["objectName"] = diff.objectName;
            obj["schemaName"] = diff.schemaName;
            obj["diffType"] = static_cast<int>(diff.diffType);
            obj["differences"] = QJsonArray::fromStringList(diff.differences);
            array.append(obj);
        }
        
        QJsonDocument doc(array);
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
        }
    }
}

void SchemaComparePanel::onSaveSnapshot() {
    // Save current schema state as snapshot
}

void SchemaComparePanel::onLoadSnapshot() {
    // Load saved snapshot for comparison
}

void SchemaComparePanel::onFilterChanged(const QString& filter) {
    // Filter displayed results
    for (int i = 0; i < diffModel_->rowCount(); ++i) {
        auto* group = diffModel_->item(i);
        for (int j = 0; j < group->rowCount(); ++j) {
            auto* item = group->child(j, 0);
            bool match = item->text().contains(filter, Qt::CaseInsensitive);
            diffTree_->setRowHidden(j, group->index(), !match);
        }
    }
}

// ============================================================================
// Schema Compare Options Dialog
// ============================================================================

SchemaCompareOptionsDialog::SchemaCompareOptionsDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void SchemaCompareOptionsDialog::setupUi() {
    setWindowTitle(tr("Schema Compare Options"));
    setMinimumWidth(400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* generalGroup = new QGroupBox(tr("General Options"), this);
    auto* generalLayout = new QVBoxLayout(generalGroup);
    
    ignoreCaseCheck_ = new QCheckBox(tr("Ignore case in object names"), this);
    ignoreWhitespaceCheck_ = new QCheckBox(tr("Ignore whitespace differences"), this);
    ignoreCommentsCheck_ = new QCheckBox(tr("Ignore comments in SQL"), this);
    
    generalLayout->addWidget(ignoreCaseCheck_);
    generalLayout->addWidget(ignoreWhitespaceCheck_);
    generalLayout->addWidget(ignoreCommentsCheck_);
    layout->addWidget(generalGroup);
    
    auto* dbGroup = new QGroupBox(tr("Database-Specific Options"), this);
    auto* dbLayout = new QVBoxLayout(dbGroup);
    
    ignoreTablespaceCheck_ = new QCheckBox(tr("Ignore tablespace differences"), this);
    ignoreOwnershipCheck_ = new QCheckBox(tr("Ignore ownership/authorization"), this);
    ignoreGrantCheck_ = new QCheckBox(tr("Ignore GRANT statements"), this);
    compareExtendedStatsCheck_ = new QCheckBox(tr("Compare extended statistics"), this);
    
    dbLayout->addWidget(ignoreTablespaceCheck_);
    dbLayout->addWidget(ignoreOwnershipCheck_);
    dbLayout->addWidget(ignoreGrantCheck_);
    dbLayout->addWidget(compareExtendedStatsCheck_);
    layout->addWidget(dbGroup);
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &SchemaCompareOptionsDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
    
    layout->addStretch();
}

void SchemaCompareOptionsDialog::onSave() {
    // Save options
    accept();
}

// ============================================================================
// Schema Sync Script Dialog
// ============================================================================

SchemaSyncScriptDialog::SchemaSyncScriptDialog(const QString& script, QWidget* parent)
    : QDialog(parent) {
    setupUi();
    scriptEdit_->setPlainText(script);
}

void SchemaSyncScriptDialog::setupUi() {
    setWindowTitle(tr("Synchronization Script"));
    resize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    scriptEdit_ = new QTextEdit(this);
    scriptEdit_->setFont(QFont("Consolas", 10));
    layout->addWidget(scriptEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* copyBtn = new QPushButton(tr("Copy to Clipboard"), this);
    connect(copyBtn, &QPushButton::clicked, this, &SchemaSyncScriptDialog::onCopy);
    btnLayout->addWidget(copyBtn);
    
    auto* saveBtn = new QPushButton(tr("Save to File"), this);
    connect(saveBtn, &QPushButton::clicked, this, &SchemaSyncScriptDialog::onSave);
    btnLayout->addWidget(saveBtn);
    
    auto* applyBtn = new QPushButton(tr("Apply to Database"), this);
    connect(applyBtn, &QPushButton::clicked, this, &SchemaSyncScriptDialog::onApply);
    btnLayout->addWidget(applyBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void SchemaSyncScriptDialog::onApply() {
    QMessageBox::warning(this, tr("Not Implemented"), 
        tr("Direct script execution is not yet implemented.\n"
           "Please save and run the script manually."));
}

void SchemaSyncScriptDialog::onSave() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Script"),
        QString(),
        tr("SQL Files (*.sql);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(scriptEdit_->toPlainText().toUtf8());
            file.close();
        }
    }
}

void SchemaSyncScriptDialog::onCopy() {
    // Copy to clipboard
}

} // namespace scratchrobin::ui
