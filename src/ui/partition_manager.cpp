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
#include <QInputDialog>

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
    
    // Connect selection model AFTER model is set
    connect(tableTree_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PartitionManagerPanel::onPartitionSelected);
    
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
    
    if (!client_) return;
    
    // Query partitioned tables from pg_class
    std::string sql = 
        "SELECT n.nspname || '.' || c.relname as table_name, "
        "pg_get_partkeydef(c.oid) as partition_key "
        "FROM pg_class c "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "WHERE c.relkind = 'p' "
        "AND n.nspname NOT IN ('pg_catalog', 'information_schema') "
        "ORDER BY 1";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() >= 2) {
                QList<QStandardItem*> items;
                items << new QStandardItem(QString::fromStdString(row[0]));
                items << new QStandardItem(QString::fromStdString(row[1]));
                tableModel_->appendRow(items);
            }
        }
    }
}

void PartitionManagerPanel::loadPartitions(const QString& tableName)
{
    partitionModel_->removeRows(0, partitionModel_->rowCount());
    
    if (!client_ || tableName.isEmpty()) return;
    
    // Parse schema and table name
    QStringList parts = tableName.split('.');
    if (parts.size() != 2) return;
    
    QString schema = parts[0];
    QString table = parts[1];
    
    // Query partitions
    std::string sql = QString(
        "SELECT c.relname as partition_name, "
        "pg_get_expr(c.relpartbound, c.oid) as partition_bound, "
        "pg_total_relation_size(c.oid) as size_bytes "
        "FROM pg_class c "
        "JOIN pg_namespace n ON n.oid = c.relnamespace "
        "JOIN pg_inherits i ON i.inhrelid = c.oid "
        "JOIN pg_class pc ON pc.oid = i.inhparent "
        "JOIN pg_namespace pn ON pn.oid = pc.relnamespace "
        "WHERE pc.relname = '%1' AND pn.nspname = '%2' "
        "ORDER BY c.relname"
    ).arg(table).arg(schema).toStdString();
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() >= 3) {
                QList<QStandardItem*> items;
                items << new QStandardItem(QString::fromStdString(row[0]));
                items << new QStandardItem(QString::fromStdString(row[1]));
                items << new QStandardItem(QString::fromStdString(row[2]));
                partitionModel_->appendRow(items);
            }
        }
    }
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
        if (client_) {
            auto index = tableTree_->currentIndex();
            QString tableName = index.isValid() ? tableModel_->item(index.row(), 0)->text() : "";
            std::string sql = QString("ALTER TABLE %1 DETACH PARTITION %2")
                .arg(tableName).arg(partitionName).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
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
    QMessageBox::information(this, tr("Merge Partitions"),
        tr("Merge partitions requires detaching partitions and reattaching as a single partition. "
           "This operation should be done manually via SQL."));
}

void PartitionManagerPanel::onAttachPartition()
{
    auto index = tableTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Attach Partition"),
            tr("Please select a partitioned table first."));
        return;
    }
    
    QString tableName = tableModel_->item(index.row(), 0)->text();
    
    bool ok;
    QString partitionName = QInputDialog::getText(this, tr("Attach Partition"),
                                                  tr("Partition table name to attach:"),
                                                  QLineEdit::Normal, QString(), &ok);
    if (!ok || partitionName.isEmpty()) return;
    
    if (client_) {
        std::string sql = QString("ALTER TABLE %1 ATTACH PARTITION %2 FOR VALUES FROM (MINVALUE) TO (MAXVALUE)")
            .arg(tableName).arg(partitionName).toStdString();
        auto response = client_->ExecuteSql(4044, "scratchbird", sql);
        if (response.status.ok) {
            QMessageBox::information(this, tr("Success"),
                tr("Partition attached successfully."));
        }
    }
    refresh();
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
        if (client_) {
            std::string sql = QString("ALTER TABLE %1 DETACH PARTITION %2")
                .arg(partitionName).arg(partitionName).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        refresh();
    }
}

void PartitionManagerPanel::onAnalyzePartition()
{
    auto index = partitionTree_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, tr("Analyze Partition"),
            tr("Please select a partition to analyze."));
        return;
    }
    
    QString partitionName = partitionModel_->item(index.row(), 0)->text();
    
    if (client_) {
        std::string sql = QString("ANALYZE %1").arg(partitionName).toStdString();
        client_->ExecuteSql(4044, "scratchbird", sql);
    }
    
    QMessageBox::information(this, tr("Analyze Partition"),
        tr("Partition %1 analyzed successfully.").arg(partitionName));
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
    if (!client_) {
        accept();
        return;
    }
    
    std::string sql = generateDdl().toStdString();
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (!response.status.ok) {
        QMessageBox::warning(this, tr("Error"),
            tr("Failed to create partitioned table: %1")
            .arg(QString::fromStdString(response.status.message)));
        return;
    }
    
    QMessageBox::information(this, tr("Success"),
        tr("Partitioned table created successfully."));
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
    if (!client_) {
        accept();
        return;
    }
    
    std::string sql = generateDdl().toStdString();
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (!response.status.ok) {
        QMessageBox::warning(this, tr("Error"),
            tr("Failed to add partition: %1")
            .arg(QString::fromStdString(response.status.message)));
        return;
    }
    
    QMessageBox::information(this, tr("Success"),
        tr("Partition added successfully."));
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
    // Note: PostgreSQL doesn't have native SPLIT PARTITION
    // This would require: detach, create new partitions, move data
    QMessageBox::information(this, tr("Split Partition"),
        tr("Splitting a partition requires detaching the partition, "
           "creating new partitions, and migrating data. "
           "This should be done manually via SQL scripts."));
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
