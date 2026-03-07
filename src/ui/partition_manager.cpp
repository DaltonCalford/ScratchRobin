#include "ui/partition_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeView>
#include <QTableView>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSplitter>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QToolBar>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>

namespace scratchrobin::ui {

// ============================================================================
// Partition Manager Panel
// ============================================================================
PartitionManagerPanel::PartitionManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("partition_manager", parent)
    , client_(client)
{
    setupUi();
    setupModel();
    refresh();
}

void PartitionManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* createTableBtn = new QPushButton(tr("New Partitioned Table"), this);
    auto* addPartitionBtn = new QPushButton(tr("Add Partition"), this);
    auto* dropPartitionBtn = new QPushButton(tr("Drop Partition"), this);
    auto* splitBtn = new QPushButton(tr("Split"), this);
    auto* mergeBtn = new QPushButton(tr("Merge"), this);
    auto* attachBtn = new QPushButton(tr("Attach"), this);
    auto* detachBtn = new QPushButton(tr("Detach"), this);
    auto* analyzeBtn = new QPushButton(tr("Analyze"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(createTableBtn);
    toolbar->addWidget(addPartitionBtn);
    toolbar->addWidget(dropPartitionBtn);
    toolbar->addWidget(splitBtn);
    toolbar->addWidget(mergeBtn);
    toolbar->addSeparator();
    toolbar->addWidget(attachBtn);
    toolbar->addWidget(detachBtn);
    toolbar->addSeparator();
    toolbar->addWidget(analyzeBtn);
    toolbar->addWidget(refreshBtn);
    
    connect(createTableBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onCreatePartitionedTable);
    connect(addPartitionBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onAddPartition);
    connect(dropPartitionBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onDropPartition);
    connect(splitBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onSplitPartition);
    connect(mergeBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onMergePartitions);
    connect(attachBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onAttachPartition);
    connect(detachBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onDetachPartition);
    connect(analyzeBtn, &QPushButton::clicked, this, &PartitionManagerPanel::onAnalyzePartition);
    connect(refreshBtn, &QPushButton::clicked, this, &PartitionManagerPanel::refresh);
    
    mainLayout->addWidget(toolbar);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter tables..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &PartitionManagerPanel::onFilterChanged);
    filterLayout->addWidget(new QLabel(tr("Filter:")));
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Tables
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    tableTree_ = new QTreeView(this);
    tableTree_->setAlternatingRowColors(true);
    tableTree_->setSortingEnabled(true);
    connect(tableTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PartitionManagerPanel::onPartitionSelected);
    
    leftLayout->addWidget(new QLabel(tr("Partitioned Tables:")));
    leftLayout->addWidget(tableTree_);
    splitter->addWidget(leftWidget);
    
    // Middle: Partitions
    auto* midWidget = new QWidget(this);
    auto* midLayout = new QVBoxLayout(midWidget);
    midLayout->setContentsMargins(0, 0, 0, 0);
    
    partitionTree_ = new QTreeView(this);
    partitionTree_->setAlternatingRowColors(true);
    partitionTree_->setSortingEnabled(true);
    
    midLayout->addWidget(new QLabel(tr("Partitions:")));
    midLayout->addWidget(partitionTree_);
    splitter->addWidget(midWidget);
    
    // Right: Details
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* detailsGroup = new QGroupBox(tr("Partition Details"), this);
    auto* detailsLayout = new QFormLayout(detailsGroup);
    
    tableLabel_ = new QLabel("-", this);
    typeLabel_ = new QLabel("-", this);
    keyLabel_ = new QLabel("-", this);
    countLabel_ = new QLabel("-", this);
    sizeLabel_ = new QLabel("-", this);
    
    detailsLayout->addRow(tr("Table:"), tableLabel_);
    detailsLayout->addRow(tr("Type:"), typeLabel_);
    detailsLayout->addRow(tr("Key:"), keyLabel_);
    detailsLayout->addRow(tr("Row Count:"), countLabel_);
    detailsLayout->addRow(tr("Size:"), sizeLabel_);
    
    rightLayout->addWidget(detailsGroup);
    
    ddlPreview_ = new QTextEdit(this);
    ddlPreview_->setReadOnly(true);
    ddlPreview_->setPlaceholderText(tr("DDL Preview..."));
    rightLayout->addWidget(ddlPreview_, 1);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({250, 250, 300});
    
    mainLayout->addWidget(splitter);
}

void PartitionManagerPanel::setupModel()
{
    tableModel_ = new QStandardItemModel(this);
    tableModel_->setColumnCount(3);
    tableModel_->setHorizontalHeaderLabels({tr("Table"), tr("Schema"), tr("Partitions")});
    tableTree_->setModel(tableModel_);
    tableTree_->header()->setStretchLastSection(false);
    tableTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    partitionModel_ = new QStandardItemModel(this);
    partitionModel_->setColumnCount(4);
    partitionModel_->setHorizontalHeaderLabels({tr("Partition"), tr("Range/Values"), tr("Rows"), tr("Size")});
    partitionTree_->setModel(partitionModel_);
    partitionTree_->header()->setStretchLastSection(false);
    partitionTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void PartitionManagerPanel::refresh()
{
    loadPartitionedTables();
}

void PartitionManagerPanel::panelActivated()
{
    refresh();
}

void PartitionManagerPanel::loadPartitionedTables()
{
    tableModel_->removeRows(0, tableModel_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void PartitionManagerPanel::loadPartitions(const QString& tableName)
{
    partitionModel_->removeRows(0, partitionModel_->rowCount());
    
    // TODO: Load from SessionClient when API is available
    Q_UNUSED(tableName)
}

void PartitionManagerPanel::onCreatePartitionedTable()
{
    CreatePartitionedTableDialog dialog(client_, this);
    dialog.exec();
}

void PartitionManagerPanel::onAddPartition()
{
    auto index = tableTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Add Partition"),
            tr("Please select a partitioned table first."));
        return;
    }
    
    QString tableName = tableModel_->item(index.row(), 0)->text();
    
    AddPartitionDialog dialog(client_, tableName, "RANGE", this);
    dialog.exec();
}

void PartitionManagerPanel::onDropPartition()
{
    auto index = partitionTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Drop Partition"),
            tr("Please select a partition to drop."));
        return;
    }
    
    QString partitionName = partitionModel_->item(index.row(), 0)->text();
    
    auto reply = QMessageBox::question(this, tr("Drop Partition"),
        tr("Drop partition '%1'?").arg(partitionName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void PartitionManagerPanel::onSplitPartition()
{
    auto index = partitionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString partitionName = partitionModel_->item(index.row(), 0)->text();
    
    SplitPartitionDialog dialog(client_, "table", partitionName, "RANGE", this);
    dialog.exec();
}

void PartitionManagerPanel::onMergePartitions()
{
    // TODO: Implement merge partition dialog
    QMessageBox::information(this, tr("Merge Partitions"),
        tr("Merge partitions functionality not yet implemented."));
}

void PartitionManagerPanel::onAttachPartition()
{
    // TODO: Implement attach partition
    QMessageBox::information(this, tr("Attach Partition"),
        tr("Attach partition functionality not yet implemented."));
}

void PartitionManagerPanel::onDetachPartition()
{
    auto index = partitionTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString partitionName = partitionModel_->item(index.row(), 0)->text();
    
    auto reply = QMessageBox::question(this, tr("Detach Partition"),
        tr("Detach partition '%1'?").arg(partitionName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient when API is available
        refresh();
    }
}

void PartitionManagerPanel::onAnalyzePartition()
{
    // TODO: Execute ANALYZE on partition
    QMessageBox::information(this, tr("Analyze Partition"),
        tr("Partition analyzed successfully."));
}

void PartitionManagerPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < tableModel_->rowCount(); ++i) {
        QString name = tableModel_->item(i, 0)->text();
        bool match = filter.isEmpty() || name.contains(filter, Qt::CaseInsensitive);
        tableTree_->setRowHidden(i, QModelIndex(), !match);
    }
}

void PartitionManagerPanel::onPartitionSelected(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    QString tableName = tableModel_->item(index.row(), 0)->text();
    loadPartitions(tableName);
}

void PartitionManagerPanel::updateDetails(const PartitionInfo& info)
{
    tableLabel_->setText(info.parentTable);
    typeLabel_->setText(info.partitionType);
    keyLabel_->setText(info.partitionKey);
    countLabel_->setText(QString::number(info.rowCount));
    sizeLabel_->setText(QString::number(info.sizeBytes / 1024 / 1024) + " MB");
    
    // Generate DDL preview
    QString ddl = QString("-- Partition: %1\n").arg(info.name);
    ddl += QString("-- Type: %1\n").arg(info.partitionType);
    ddl += QString("-- Key: %1\n").arg(info.partitionKey);
    ddlPreview_->setPlainText(ddl);
}

// ============================================================================
// Create Partitioned Table Dialog
// ============================================================================
CreatePartitionedTableDialog::CreatePartitionedTableDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Create Partitioned Table"));
    setMinimumSize(700, 600);
    setupUi();
}

void CreatePartitionedTableDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    // Table info
    auto* infoGroup = new QGroupBox(tr("Table Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    tableNameEdit_ = new QLineEdit(this);
    infoLayout->addRow(tr("Table Name:"), tableNameEdit_);
    
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    infoLayout->addRow(tr("Schema:"), schemaCombo_);
    
    partitionTypeCombo_ = new QComboBox(this);
    partitionTypeCombo_->addItems({"RANGE", "LIST", "HASH"});
    connect(partitionTypeCombo_, &QComboBox::currentTextChanged,
            this, &CreatePartitionedTableDialog::onPartitionTypeChanged);
    infoLayout->addRow(tr("Partition Type:"), partitionTypeCombo_);
    
    partitionKeyEdit_ = new QLineEdit(this);
    partitionKeyEdit_->setPlaceholderText(tr("e.g., date_column, region"));
    infoLayout->addRow(tr("Partition Key:"), partitionKeyEdit_);
    
    mainLayout->addWidget(infoGroup);
    
    // Columns
    auto* colGroup = new QGroupBox(tr("Columns"), this);
    auto* colLayout = new QVBoxLayout(colGroup);
    
    columnsTable_ = new QTableWidget(this);
    columnsTable_->setColumnCount(3);
    columnsTable_->setHorizontalHeaderLabels({tr("Name"), tr("Data Type"), tr("Not Null")});
    colLayout->addWidget(columnsTable_);
    
    auto* colBtnLayout = new QHBoxLayout();
    auto* addColBtn = new QPushButton(tr("Add Column"), this);
    auto* removeColBtn = new QPushButton(tr("Remove Column"), this);
    colBtnLayout->addWidget(addColBtn);
    colBtnLayout->addWidget(removeColBtn);
    colBtnLayout->addStretch();
    colLayout->addLayout(colBtnLayout);
    
    mainLayout->addWidget(colGroup);
    
    // Partitions
    auto* partGroup = new QGroupBox(tr("Partitions"), this);
    auto* partLayout = new QVBoxLayout(partGroup);
    
    partitionsTable_ = new QTableWidget(this);
    partitionsTable_->setColumnCount(3);
    partitionsTable_->setHorizontalHeaderLabels({tr("Name"), tr("From/Values"), tr("To")});
    partLayout->addWidget(partitionsTable_);
    
    auto* partBtnLayout = new QHBoxLayout();
    auto* addPartBtn = new QPushButton(tr("Add Partition"), this);
    auto* removePartBtn = new QPushButton(tr("Remove Partition"), this);
    connect(addPartBtn, &QPushButton::clicked, this, &CreatePartitionedTableDialog::onAddPartition);
    connect(removePartBtn, &QPushButton::clicked, this, &CreatePartitionedTableDialog::onRemovePartition);
    partBtnLayout->addWidget(addPartBtn);
    partBtnLayout->addWidget(removePartBtn);
    partBtnLayout->addStretch();
    partLayout->addLayout(partBtnLayout);
    
    mainLayout->addWidget(partGroup);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &CreatePartitionedTableDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CreatePartitionedTableDialog::onCreate);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CreatePartitionedTableDialog::onAddPartition()
{
    int row = partitionsTable_->rowCount();
    partitionsTable_->insertRow(row);
    partitionsTable_->setItem(row, 0, new QTableWidgetItem(QString("p%1").arg(row + 1)));
}

void CreatePartitionedTableDialog::onRemovePartition()
{
    int row = partitionsTable_->currentRow();
    if (row >= 0) {
        partitionsTable_->removeRow(row);
    }
}

void CreatePartitionedTableDialog::onPartitionTypeChanged(const QString& type)
{
    if (type == "RANGE") {
        partitionsTable_->setHorizontalHeaderLabels({tr("Name"), tr("From"), tr("To")});
    } else if (type == "LIST") {
        partitionsTable_->setHorizontalHeaderLabels({tr("Name"), tr("Values"), tr("")});
    } else if (type == "HASH") {
        partitionsTable_->setHorizontalHeaderLabels({tr("Name"), tr("Modulus"), tr("Remainder")});
    }
}

void CreatePartitionedTableDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void CreatePartitionedTableDialog::onCreate()
{
    // TODO: Execute via SessionClient when API is available
    accept();
}

QString CreatePartitionedTableDialog::generateDdl()
{
    QString sql = QString("CREATE TABLE %1.%2 (\n")
        .arg(schemaCombo_->currentText())
        .arg(tableNameEdit_->text());
    
    // Add columns
    for (int i = 0; i < columnsTable_->rowCount(); ++i) {
        QString name = columnsTable_->item(i, 0) ? columnsTable_->item(i, 0)->text() : "";
        QString type = columnsTable_->item(i, 1) ? columnsTable_->item(i, 1)->text() : "";
        if (!name.isEmpty() && !type.isEmpty()) {
            sql += QString("    %1 %2").arg(name).arg(type);
            if (i < columnsTable_->rowCount() - 1) sql += ",";
            sql += "\n";
        }
    }
    
    sql += ") PARTITION BY ";
    sql += partitionTypeCombo_->currentText();
    sql += QString(" (%1);\n").arg(partitionKeyEdit_->text());
    
    return sql;
}

// ============================================================================
// Add Partition Dialog
// ============================================================================
AddPartitionDialog::AddPartitionDialog(backend::SessionClient* client,
                                       const QString& parentTable,
                                       const QString& partitionType,
                                       QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , parentTable_(parentTable)
    , partitionType_(partitionType)
{
    setWindowTitle(tr("Add Partition to %1").arg(parentTable));
    setMinimumSize(400, 350);
    setupUi();
}

void AddPartitionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    partitionNameEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Partition Name:"), partitionNameEdit_);
    
    if (partitionType_ == "RANGE") {
        fromValueEdit_ = new QLineEdit(this);
        formLayout->addRow(tr("From Value:"), fromValueEdit_);
        
        toValueEdit_ = new QLineEdit(this);
        formLayout->addRow(tr("To Value:"), toValueEdit_);
    } else if (partitionType_ == "LIST") {
        listValuesEdit_ = new QLineEdit(this);
        listValuesEdit_->setPlaceholderText(tr("e.g., 'A', 'B', 'C' or 1, 2, 3"));
        formLayout->addRow(tr("Values:"), listValuesEdit_);
    }
    
    defaultCheck_ = new QCheckBox(tr("Default Partition"), this);
    formLayout->addRow(QString(), defaultCheck_);
    
    mainLayout->addLayout(formLayout);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &AddPartitionDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddPartitionDialog::onAdd);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AddPartitionDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void AddPartitionDialog::onAdd()
{
    // TODO: Execute via SessionClient when API is available
    accept();
}

QString AddPartitionDialog::generateDdl()
{
    QString sql = QString("CREATE TABLE %1 PARTITION OF %2").arg(partitionNameEdit_->text()).arg(parentTable_);
    
    if (partitionType_ == "RANGE" && !defaultCheck_->isChecked()) {
        sql += QString(" FOR VALUES FROM (%1) TO (%2)")
            .arg(fromValueEdit_->text())
            .arg(toValueEdit_->text());
    } else if (partitionType_ == "LIST" && !defaultCheck_->isChecked()) {
        sql += QString(" FOR VALUES IN (%1)").arg(listValuesEdit_->text());
    } else if (defaultCheck_->isChecked()) {
        sql += " DEFAULT";
    }
    
    sql += ";";
    return sql;
}

// ============================================================================
// Split Partition Dialog
// ============================================================================
SplitPartitionDialog::SplitPartitionDialog(backend::SessionClient* client,
                                          const QString& tableName,
                                          const QString& partitionName,
                                          const QString& partitionType,
                                          QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , tableName_(tableName)
    , partitionName_(partitionName)
    , partitionType_(partitionType)
{
    setWindowTitle(tr("Split Partition %1").arg(partitionName));
    setMinimumSize(400, 300);
    setupUi();
}

void SplitPartitionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    newPartition1Edit_ = new QLineEdit(this);
    formLayout->addRow(tr("New Partition 1 Name:"), newPartition1Edit_);
    
    newPartition2Edit_ = new QLineEdit(this);
    formLayout->addRow(tr("New Partition 2 Name:"), newPartition2Edit_);
    
    if (partitionType_ == "RANGE") {
        splitAtEdit_ = new QLineEdit(this);
        splitAtEdit_->setPlaceholderText(tr("Split point value"));
        formLayout->addRow(tr("Split At:"), splitAtEdit_);
    }
    
    mainLayout->addLayout(formLayout);
    
    // Preview
    auto* previewGroup = new QGroupBox(tr("DDL Preview"), this);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewLayout->addWidget(previewEdit_);
    
    mainLayout->addWidget(previewGroup);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(this);
    auto* previewBtn = buttonBox->addButton(tr("Preview"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    
    mainLayout->addWidget(buttonBox);
    
    connect(previewBtn, &QPushButton::clicked, this, &SplitPartitionDialog::onPreview);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SplitPartitionDialog::onSplit);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SplitPartitionDialog::onPreview()
{
    previewEdit_->setPlainText(generateDdl());
}

void SplitPartitionDialog::onSplit()
{
    // TODO: Execute via SessionClient when API is available
    accept();
}

QString SplitPartitionDialog::generateDdl()
{
    // Note: Split partition syntax varies by database
    // This is a generic representation
    return QString("-- Split partition %1 into %2 and %3")
        .arg(partitionName_)
        .arg(newPartition1Edit_->text())
        .arg(newPartition2Edit_->text());
}

} // namespace scratchrobin::ui
