#include "ui/table_designer_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>

namespace scratchrobin::ui {

// ============================================================================
// Table Designer Dialog
// ============================================================================

TableDesignerDialog::TableDesignerDialog(Mode mode, QWidget* parent)
    : QDialog(parent), mode_(mode) {
    setWindowTitle(mode == Mode::Create ? tr("Create Table") : tr("Alter Table"));
    setMinimumSize(800, 600);
    resize(900, 700);
    
    setupUi();
    setupTabs();
}

TableDesignerDialog::~TableDesignerDialog() = default;

void TableDesignerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    
    // Table name
    auto* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel(tr("Table name:"), this));
    auto* nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(tr("Enter table name"));
    nameLayout->addWidget(nameEdit);
    mainLayout->addLayout(nameLayout);
    
    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_, 1);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("&Update Preview"), this);
    buttonLayout->addWidget(previewBtn);
    
    buttonLayout->addSpacing(20);
    
    applyBtn_ = new QPushButton(mode_ == Mode::Create ? tr("&Create") : tr("&Alter"), this);
    applyBtn_->setDefault(true);
    buttonLayout->addWidget(applyBtn_);
    
    cancelBtn_ = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(previewBtn, &QPushButton::clicked, this, &TableDesignerDialog::updateSqlPreview);
    connect(applyBtn_, &QPushButton::clicked, this, &TableDesignerDialog::accept);
    connect(cancelBtn_, &QPushButton::clicked, this, &TableDesignerDialog::reject);
    connect(tabWidget_, &QTabWidget::currentChanged, this, &TableDesignerDialog::onTabChanged);
}

void TableDesignerDialog::setupTabs() {
    // Columns tab
    auto* columnsTab = new ColumnsTab(this);
    tabWidget_->addTab(columnsTab, tr("&Columns"));
    connect(columnsTab, &ColumnsTab::columnsChanged, this, &TableDesignerDialog::updateSqlPreview);
    
    // Constraints tab
    auto* constraintsTab = new ConstraintsTab(this);
    tabWidget_->addTab(constraintsTab, tr("C&onstraints"));
    connect(constraintsTab, &ConstraintsTab::constraintsChanged, this, &TableDesignerDialog::updateSqlPreview);
    
    // Indexes tab
    auto* indexesTab = new IndexesTab(this);
    tabWidget_->addTab(indexesTab, tr("&Indexes"));
    connect(indexesTab, &IndexesTab::indexesChanged, this, &TableDesignerDialog::updateSqlPreview);
    
    // SQL Preview tab
    sqlPreview_ = new QPlainTextEdit(this);
    sqlPreview_->setReadOnly(true);
    sqlPreview_->setFont(QFont("Consolas", 10));
    sqlPreview_->setPlaceholderText(tr("SQL preview will appear here..."));
    tabWidget_->addTab(sqlPreview_, tr("SQL &Preview"));
    
    // Initial preview
    updateSqlPreview();
}

void TableDesignerDialog::setTableName(const QString& name) {
    tableName_ = name;
}

void TableDesignerDialog::setExistingTable(const QString& tableName, const QString& schema) {
    tableName_ = tableName;
    schema_ = schema;
    setWindowTitle(tr("Alter Table - %1").arg(tableName));
}

QString TableDesignerDialog::generatedSql() const {
    return sqlPreview_->toPlainText();
}

void TableDesignerDialog::accept() {
    // Validate before accepting
    if (tableName_.isEmpty()) {
        QMessageBox::warning(this, tr("Validation Error"), tr("Table name cannot be empty."));
        return;
    }
    
    QDialog::accept();
}

void TableDesignerDialog::onTabChanged(int index) {
    if (tabWidget_->tabText(index) == tr("SQL &Preview")) {
        updateSqlPreview();
    }
}

void TableDesignerDialog::updateSqlPreview() {
    // Build SQL statement
    QString sql;
    
    if (mode_ == Mode::Create) {
        sql = tr("CREATE TABLE %1 (\n").arg(tableName_.isEmpty() ? tr("new_table") : tableName_);
    } else {
        sql = tr("ALTER TABLE %1\n").arg(tableName_);
    }
    
    // Get columns from columns tab
    auto* columnsTab = qobject_cast<ColumnsTab*>(tabWidget_->widget(0));
    if (columnsTab) {
        auto columns = columnsTab->columns();
        QStringList columnSql;
        
        for (const auto& col : columns) {
            QString colDef = tr("    %1 %2").arg(col.name, col.type);
            
            if (col.length > 0) {
                if (col.precision > 0) {
                    colDef += tr("(%1,%2)").arg(col.length).arg(col.precision);
                } else {
                    colDef += tr("(%1)").arg(col.length);
                }
            }
            
            if (!col.nullable) {
                colDef += tr(" NOT NULL");
            }
            
            if (col.primaryKey) {
                colDef += tr(" PRIMARY KEY");
            }
            
            if (col.autoIncrement) {
                colDef += tr(" AUTO_INCREMENT");
            }
            
            if (col.unique) {
                colDef += tr(" UNIQUE");
            }
            
            if (!col.defaultValue.isEmpty()) {
                colDef += tr(" DEFAULT %1").arg(col.defaultValue);
            }
            
            columnSql.append(colDef);
        }
        
        sql += columnSql.join(",\n");
    }
    
    sql += tr("\n);");
    
    sqlPreview_->setPlainText(sql);
}

// ============================================================================
// Columns Tab
// ============================================================================

ColumnsTab::ColumnsTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ColumnsTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Table view
    tableView_ = new QTableView(this);
    tableView_->setAlternatingRowColors(true);
    tableView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView_->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView_->horizontalHeader()->setStretchLastSection(true);
    tableView_->verticalHeader()->setVisible(false);
    mainLayout->addWidget(tableView_);
    
    // Model
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(11);
    model_->setHorizontalHeaderLabels({
        tr("Name"), tr("Type"), tr("Length"), tr("Precision"), tr("Scale"),
        tr("Default"), tr("Nullable"), tr("PK"), tr("AI"), tr("Unique"), tr("Comment")
    });
    tableView_->setModel(model_);
    
    // Button layout
    auto* buttonLayout = new QHBoxLayout();
    
    addBtn_ = new QPushButton(tr("&Add Column"), this);
    buttonLayout->addWidget(addBtn_);
    
    removeBtn_ = new QPushButton(tr("&Remove"), this);
    removeBtn_->setEnabled(false);
    buttonLayout->addWidget(removeBtn_);
    
    buttonLayout->addSpacing(20);
    
    upBtn_ = new QPushButton(tr("Move &Up"), this);
    upBtn_->setEnabled(false);
    buttonLayout->addWidget(upBtn_);
    
    downBtn_ = new QPushButton(tr("Move &Down"), this);
    downBtn_->setEnabled(false);
    buttonLayout->addWidget(downBtn_);
    
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(addBtn_, &QPushButton::clicked, this, &ColumnsTab::addColumn);
    connect(removeBtn_, &QPushButton::clicked, this, &ColumnsTab::removeSelectedColumn);
    connect(upBtn_, &QPushButton::clicked, this, &ColumnsTab::moveColumnUp);
    connect(downBtn_, &QPushButton::clicked, this, &ColumnsTab::moveColumnDown);
    connect(model_, &QStandardItemModel::itemChanged, this, &ColumnsTab::onCellChanged);
    connect(tableView_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ColumnsTab::onSelectionChanged);
}

void ColumnsTab::populateTypeCombo(QComboBox* combo) {
    combo->addItems({
        "INT", "BIGINT", "SMALLINT", "TINYINT",
        "VARCHAR", "CHAR", "TEXT", "LONGTEXT",
        "DECIMAL", "NUMERIC", "FLOAT", "DOUBLE",
        "DATE", "TIME", "DATETIME", "TIMESTAMP",
        "BOOLEAN", "BLOB", "JSON", "UUID"
    });
}

void ColumnsTab::addColumn() {
    int row = model_->rowCount();
    model_->insertRow(row);
    
    // Name
    model_->setItem(row, 0, new QStandardItem(tr("column_%1").arg(row + 1)));
    
    // Type combo
    auto* typeCombo = new QComboBox(this);
    populateTypeCombo(typeCombo);
    typeCombo->setCurrentText("VARCHAR");
    typeCombos_[row] = typeCombo;
    tableView_->setIndexWidget(model_->index(row, 1), typeCombo);
    
    // Length
    model_->setItem(row, 2, new QStandardItem("255"));
    
    // Precision/Scale
    model_->setItem(row, 3, new QStandardItem("0"));
    model_->setItem(row, 4, new QStandardItem("0"));
    
    // Default
    model_->setItem(row, 5, new QStandardItem(""));
    
    // Nullable
    auto* nullableItem = new QStandardItem();
    nullableItem->setCheckable(true);
    nullableItem->setCheckState(Qt::Checked);
    model_->setItem(row, 6, nullableItem);
    
    // PK
    auto* pkItem = new QStandardItem();
    pkItem->setCheckable(true);
    model_->setItem(row, 7, pkItem);
    
    // Auto increment
    auto* aiItem = new QStandardItem();
    aiItem->setCheckable(true);
    model_->setItem(row, 8, aiItem);
    
    // Unique
    auto* uniqueItem = new QStandardItem();
    uniqueItem->setCheckable(true);
    model_->setItem(row, 9, uniqueItem);
    
    // Comment
    model_->setItem(row, 10, new QStandardItem(""));
    
    emit columnsChanged();
}

void ColumnsTab::removeSelectedColumn() {
    auto selection = tableView_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        model_->removeRow(row);
        emit columnsChanged();
    }
    updateButtonStates();
}

void ColumnsTab::moveColumnUp() {
    auto selection = tableView_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        if (row > 0) {
            model_->insertRow(row + 1, model_->takeRow(row - 1));
            tableView_->selectRow(row - 1);
            emit columnsChanged();
        }
    }
}

void ColumnsTab::moveColumnDown() {
    auto selection = tableView_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        if (row < model_->rowCount() - 1) {
            model_->insertRow(row, model_->takeRow(row + 1));
            tableView_->selectRow(row + 1);
            emit columnsChanged();
        }
    }
}

void ColumnsTab::onCellChanged() {
    emit columnsChanged();
}

void ColumnsTab::onSelectionChanged() {
    updateButtonStates();
}

void ColumnsTab::onTypeChanged(int row) {
    Q_UNUSED(row)
    emit columnsChanged();
}

void ColumnsTab::updateButtonStates() {
    auto selection = tableView_->selectionModel()->selectedRows();
    bool hasSelection = !selection.isEmpty();
    
    removeBtn_->setEnabled(hasSelection);
    
    if (hasSelection) {
        int row = selection.first().row();
        upBtn_->setEnabled(row > 0);
        downBtn_->setEnabled(row < model_->rowCount() - 1);
    } else {
        upBtn_->setEnabled(false);
        downBtn_->setEnabled(false);
    }
}

QList<ColumnsTab::ColumnDef> ColumnsTab::columns() const {
    QList<ColumnDef> result;
    
    for (int row = 0; row < model_->rowCount(); ++row) {
        ColumnDef col;
        col.name = model_->item(row, 0)->text();
        
        auto* combo = typeCombos_.value(row);
        col.type = combo ? combo->currentText() : model_->item(row, 1)->text();
        
        col.length = model_->item(row, 2)->text().toInt();
        col.precision = model_->item(row, 3)->text().toInt();
        col.scale = model_->item(row, 4)->text().toInt();
        col.defaultValue = model_->item(row, 5)->text();
        col.nullable = model_->item(row, 6)->checkState() == Qt::Checked;
        col.primaryKey = model_->item(row, 7)->checkState() == Qt::Checked;
        col.autoIncrement = model_->item(row, 8)->checkState() == Qt::Checked;
        col.unique = model_->item(row, 9)->checkState() == Qt::Checked;
        col.comment = model_->item(row, 10)->text();
        
        result.append(col);
    }
    
    return result;
}

void ColumnsTab::setColumns(const QList<ColumnDef>& columns) {
    model_->clear();
    model_->setHorizontalHeaderLabels({
        tr("Name"), tr("Type"), tr("Length"), tr("Precision"), tr("Scale"),
        tr("Default"), tr("Nullable"), tr("PK"), tr("AI"), tr("Unique"), tr("Comment")
    });
    
    typeCombos_.clear();
    
    for (const auto& col : columns) {
        int row = model_->rowCount();
        model_->insertRow(row);
        
        model_->setItem(row, 0, new QStandardItem(col.name));
        
        auto* typeCombo = new QComboBox(this);
        populateTypeCombo(typeCombo);
        typeCombo->setCurrentText(col.type);
        typeCombos_[row] = typeCombo;
        tableView_->setIndexWidget(model_->index(row, 1), typeCombo);
        
        model_->setItem(row, 2, new QStandardItem(QString::number(col.length)));
        model_->setItem(row, 3, new QStandardItem(QString::number(col.precision)));
        model_->setItem(row, 4, new QStandardItem(QString::number(col.scale)));
        model_->setItem(row, 5, new QStandardItem(col.defaultValue));
        
        auto* nullableItem = new QStandardItem();
        nullableItem->setCheckable(true);
        nullableItem->setCheckState(col.nullable ? Qt::Checked : Qt::Unchecked);
        model_->setItem(row, 6, nullableItem);
        
        auto* pkItem = new QStandardItem();
        pkItem->setCheckable(true);
        pkItem->setCheckState(col.primaryKey ? Qt::Checked : Qt::Unchecked);
        model_->setItem(row, 7, pkItem);
        
        auto* aiItem = new QStandardItem();
        aiItem->setCheckable(true);
        aiItem->setCheckState(col.autoIncrement ? Qt::Checked : Qt::Unchecked);
        model_->setItem(row, 8, aiItem);
        
        auto* uniqueItem = new QStandardItem();
        uniqueItem->setCheckable(true);
        uniqueItem->setCheckState(col.unique ? Qt::Checked : Qt::Unchecked);
        model_->setItem(row, 9, uniqueItem);
        
        model_->setItem(row, 10, new QStandardItem(col.comment));
    }
}

// ============================================================================
// Constraints Tab
// ============================================================================

ConstraintsTab::ConstraintsTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ConstraintsTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    
    // Foreign Keys group
    auto* fkGroup = new QGroupBox(tr("Foreign Keys"), this);
    auto* fkLayout = new QVBoxLayout(fkGroup);
    
    fkTable_ = new QTableView(this);
    fkTable_->setAlternatingRowColors(true);
    fkTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    fkLayout->addWidget(fkTable_);
    
    fkModel_ = new QStandardItemModel(this);
    fkModel_->setHorizontalHeaderLabels({tr("Name"), tr("Columns"), tr("Ref Table"), tr("Ref Columns"), tr("On Delete"), tr("On Update")});
    fkTable_->setModel(fkModel_);
    
    auto* fkBtnLayout = new QHBoxLayout();
    auto* addFkBtn = new QPushButton(tr("&Add"), this);
    auto* editFkBtn = new QPushButton(tr("&Edit"), this);
    auto* removeFkBtn = new QPushButton(tr("&Remove"), this);
    fkBtnLayout->addWidget(addFkBtn);
    fkBtnLayout->addWidget(editFkBtn);
    fkBtnLayout->addWidget(removeFkBtn);
    fkBtnLayout->addStretch();
    fkLayout->addLayout(fkBtnLayout);
    
    mainLayout->addWidget(fkGroup, 1);
    
    // Check Constraints group
    auto* checkGroup = new QGroupBox(tr("Check Constraints"), this);
    auto* checkLayout = new QVBoxLayout(checkGroup);
    
    checkTable_ = new QTableView(this);
    checkTable_->setAlternatingRowColors(true);
    checkTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    checkLayout->addWidget(checkTable_);
    
    checkModel_ = new QStandardItemModel(this);
    checkModel_->setHorizontalHeaderLabels({tr("Name"), tr("Expression")});
    checkTable_->setModel(checkModel_);
    
    auto* checkBtnLayout = new QHBoxLayout();
    auto* addCheckBtn = new QPushButton(tr("&Add"), this);
    auto* editCheckBtn = new QPushButton(tr("&Edit"), this);
    auto* removeCheckBtn = new QPushButton(tr("&Remove"), this);
    checkBtnLayout->addWidget(addCheckBtn);
    checkBtnLayout->addWidget(editCheckBtn);
    checkBtnLayout->addWidget(removeCheckBtn);
    checkBtnLayout->addStretch();
    checkLayout->addLayout(checkBtnLayout);
    
    mainLayout->addWidget(checkGroup, 1);
    
    // Connections
    connect(addFkBtn, &QPushButton::clicked, this, &ConstraintsTab::onAddForeignKey);
    connect(editFkBtn, &QPushButton::clicked, this, &ConstraintsTab::onEditForeignKey);
    connect(removeFkBtn, &QPushButton::clicked, this, &ConstraintsTab::onRemoveForeignKey);
    connect(addCheckBtn, &QPushButton::clicked, this, &ConstraintsTab::onAddCheck);
    connect(editCheckBtn, &QPushButton::clicked, this, &ConstraintsTab::onEditCheck);
    connect(removeCheckBtn, &QPushButton::clicked, this, &ConstraintsTab::onRemoveCheck);
}

QList<ConstraintsTab::ForeignKey> ConstraintsTab::foreignKeys() const {
    QList<ForeignKey> result;
    for (int row = 0; row < fkModel_->rowCount(); ++row) {
        ForeignKey fk;
        fk.name = fkModel_->item(row, 0)->text();
        fk.columns = fkModel_->item(row, 1)->text().split(",");
        fk.refTable = fkModel_->item(row, 2)->text();
        fk.refColumns = fkModel_->item(row, 3)->text().split(",");
        fk.onDelete = fkModel_->item(row, 4)->text();
        fk.onUpdate = fkModel_->item(row, 5)->text();
        result.append(fk);
    }
    return result;
}

QList<ConstraintsTab::CheckConstraint> ConstraintsTab::checkConstraints() const {
    QList<CheckConstraint> result;
    for (int row = 0; row < checkModel_->rowCount(); ++row) {
        CheckConstraint cc;
        cc.name = checkModel_->item(row, 0)->text();
        cc.expression = checkModel_->item(row, 1)->text();
        result.append(cc);
    }
    return result;
}

void ConstraintsTab::setForeignKeys(const QList<ForeignKey>& keys) {
    fkModel_->clear();
    fkModel_->setHorizontalHeaderLabels({tr("Name"), tr("Columns"), tr("Ref Table"), tr("Ref Columns"), tr("On Delete"), tr("On Update")});
    for (const auto& fk : keys) {
        QList<QStandardItem*> row;
        row << new QStandardItem(fk.name)
            << new QStandardItem(fk.columns.join(", "))
            << new QStandardItem(fk.refTable)
            << new QStandardItem(fk.refColumns.join(", "))
            << new QStandardItem(fk.onDelete)
            << new QStandardItem(fk.onUpdate);
        fkModel_->appendRow(row);
    }
}

void ConstraintsTab::setCheckConstraints(const QList<CheckConstraint>& constraints) {
    checkModel_->clear();
    checkModel_->setHorizontalHeaderLabels({tr("Name"), tr("Expression")});
    for (const auto& cc : constraints) {
        QList<QStandardItem*> row;
        row << new QStandardItem(cc.name)
            << new QStandardItem(cc.expression);
        checkModel_->appendRow(row);
    }
}

void ConstraintsTab::onAddForeignKey() {
    // TODO: Open foreign key editor dialog
    ForeignKey fk;
    fk.name = tr("fk_%1").arg(fkModel_->rowCount() + 1);
    fk.onDelete = "NO ACTION";
    fk.onUpdate = "NO ACTION";
    
    QList<QStandardItem*> row;
    row << new QStandardItem(fk.name)
        << new QStandardItem("column")
        << new QStandardItem("ref_table")
        << new QStandardItem("ref_column")
        << new QStandardItem(fk.onDelete)
        << new QStandardItem(fk.onUpdate);
    fkModel_->appendRow(row);
    emit constraintsChanged();
}

void ConstraintsTab::onEditForeignKey() {
    // TODO: Edit selected foreign key
}

void ConstraintsTab::onRemoveForeignKey() {
    auto selection = fkTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        fkModel_->removeRow(selection.first().row());
        emit constraintsChanged();
    }
}

void ConstraintsTab::onAddCheck() {
    QList<QStandardItem*> row;
    row << new QStandardItem(tr("chk_%1").arg(checkModel_->rowCount() + 1))
        << new QStandardItem("expression");
    checkModel_->appendRow(row);
    emit constraintsChanged();
}

void ConstraintsTab::onEditCheck() {
    // TODO: Edit selected check constraint
}

void ConstraintsTab::onRemoveCheck() {
    auto selection = checkTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        checkModel_->removeRow(selection.first().row());
        emit constraintsChanged();
    }
}

// ============================================================================
// Indexes Tab
// ============================================================================

IndexesTab::IndexesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void IndexesTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    indexTable_ = new QTableView(this);
    indexTable_->setAlternatingRowColors(true);
    indexTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(indexTable_);
    
    indexModel_ = new QStandardItemModel(this);
    indexModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Columns"), tr("Clustered")});
    indexTable_->setModel(indexModel_);
    
    auto* buttonLayout = new QHBoxLayout();
    
    addBtn_ = new QPushButton(tr("&Add Index"), this);
    buttonLayout->addWidget(addBtn_);
    
    editBtn_ = new QPushButton(tr("&Edit"), this);
    editBtn_->setEnabled(false);
    buttonLayout->addWidget(editBtn_);
    
    removeBtn_ = new QPushButton(tr("&Remove"), this);
    removeBtn_->setEnabled(false);
    buttonLayout->addWidget(removeBtn_);
    
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(addBtn_, &QPushButton::clicked, this, &IndexesTab::onAddIndex);
    connect(editBtn_, &QPushButton::clicked, this, &IndexesTab::onEditIndex);
    connect(removeBtn_, &QPushButton::clicked, this, &IndexesTab::onRemoveIndex);
    connect(indexTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &IndexesTab::onIndexSelectionChanged);
}

QList<IndexesTab::IndexDef> IndexesTab::indexes() const {
    QList<IndexDef> result;
    for (int row = 0; row < indexModel_->rowCount(); ++row) {
        IndexDef idx;
        idx.name = indexModel_->item(row, 0)->text();
        idx.type = indexModel_->item(row, 1)->text();
        idx.clustered = indexModel_->item(row, 3)->checkState() == Qt::Checked;
        
        // Parse columns from text
        QStringList cols = indexModel_->item(row, 2)->text().split(",");
        for (const auto& col : cols) {
            idx.columns.append(qMakePair(col.trimmed(), true));
        }
        
        result.append(idx);
    }
    return result;
}

void IndexesTab::setIndexes(const QList<IndexDef>& indexes) {
    indexModel_->clear();
    indexModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Columns"), tr("Clustered")});
    for (const auto& idx : indexes) {
        QStringList colNames;
        for (const auto& col : idx.columns) {
            colNames.append(col.first);
        }
        
        auto* clusteredItem = new QStandardItem();
        clusteredItem->setCheckable(true);
        clusteredItem->setCheckState(idx.clustered ? Qt::Checked : Qt::Unchecked);
        
        QList<QStandardItem*> row;
        row << new QStandardItem(idx.name)
            << new QStandardItem(idx.type)
            << new QStandardItem(colNames.join(", "))
            << clusteredItem;
        indexModel_->appendRow(row);
    }
}

void IndexesTab::onAddIndex() {
    QList<QStandardItem*> row;
    row << new QStandardItem(tr("idx_%1").arg(indexModel_->rowCount() + 1))
        << new QStandardItem("INDEX")
        << new QStandardItem("column")
        << []() {
            auto* item = new QStandardItem();
            item->setCheckable(true);
            return item;
        }();
    indexModel_->appendRow(row);
    emit indexesChanged();
}

void IndexesTab::onEditIndex() {
    // TODO: Open index editor dialog
}

void IndexesTab::onRemoveIndex() {
    auto selection = indexTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        indexModel_->removeRow(selection.first().row());
        emit indexesChanged();
    }
}

void IndexesTab::onIndexSelectionChanged() {
    bool hasSelection = !indexTable_->selectionModel()->selectedRows().isEmpty();
    editBtn_->setEnabled(hasSelection);
    removeBtn_->setEnabled(hasSelection);
}

// ============================================================================
// Index Editor Dialog
// ============================================================================

IndexEditorDialog::IndexEditorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Edit Index"));
    setupUi();
}

void IndexEditorDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    
    auto* formLayout = new QGridLayout();
    formLayout->addWidget(new QLabel(tr("Index name:"), this), 0, 0);
    nameEdit_ = new QLineEdit(this);
    formLayout->addWidget(nameEdit_, 0, 1);
    
    formLayout->addWidget(new QLabel(tr("Type:"), this), 1, 0);
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({"INDEX", "UNIQUE", "FULLTEXT", "SPATIAL"});
    formLayout->addWidget(typeCombo_, 1, 1);
    
    clusteredCheck_ = new QCheckBox(tr("Clustered index"), this);
    formLayout->addWidget(clusteredCheck_, 2, 0, 1, 2);
    
    mainLayout->addLayout(formLayout);
    
    mainLayout->addWidget(new QLabel(tr("Columns:"), this));
    columnsTable_ = new QTableView(this);
    mainLayout->addWidget(columnsTable_);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Column"), tr("Ascending")});
    columnsTable_->setModel(model_);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void IndexEditorDialog::setAvailableColumns(const QStringList& columns) {
    availableColumns_ = columns;
    updateColumnList();
}

void IndexEditorDialog::updateColumnList() {
    // TODO: Populate columns table
}

void IndexEditorDialog::setIndex(const IndexesTab::IndexDef& index) {
    nameEdit_->setText(index.name);
    typeCombo_->setCurrentText(index.type);
    clusteredCheck_->setChecked(index.clustered);
}

IndexesTab::IndexDef IndexEditorDialog::getIndex() const {
    IndexesTab::IndexDef idx;
    idx.name = nameEdit_->text();
    idx.type = typeCombo_->currentText();
    idx.clustered = clusteredCheck_->isChecked();
    return idx;
}

// ============================================================================
// SQL Preview Tab
// ============================================================================
SqlPreviewTab::SqlPreviewTab(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void SqlPreviewTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    sqlEdit_ = new QPlainTextEdit(this);
    sqlEdit_->setFont(QFont("Consolas", 10));
    layout->addWidget(sqlEdit_);
}

void SqlPreviewTab::setSql(const QString& sql) {
    sqlEdit_->setPlainText(sql);
}

QString SqlPreviewTab::sql() const {
    return sqlEdit_->toPlainText();
}

void SqlPreviewTab::setReadOnly(bool readOnly) {
    sqlEdit_->setReadOnly(readOnly);
}

// ============================================================================
// Foreign Key Editor Dialog
// ============================================================================
ForeignKeyEditorDialog::ForeignKeyEditorDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Edit Foreign Key"));
    resize(400, 300);
    setupUi();
}

void ForeignKeyEditorDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QGridLayout();
    formLayout->addWidget(new QLabel(tr("Name:"), this), 0, 0);
    nameEdit_ = new QLineEdit(this);
    formLayout->addWidget(nameEdit_, 0, 1);
    
    formLayout->addWidget(new QLabel(tr("Reference Table:"), this), 1, 0);
    refTableCombo_ = new QComboBox(this);
    formLayout->addWidget(refTableCombo_, 1, 1);
    
    formLayout->addWidget(new QLabel(tr("On Delete:"), this), 2, 0);
    onDeleteCombo_ = new QComboBox(this);
    onDeleteCombo_->addItems({"NO ACTION", "CASCADE", "SET NULL", "SET DEFAULT", "RESTRICT"});
    formLayout->addWidget(onDeleteCombo_, 2, 1);
    
    formLayout->addWidget(new QLabel(tr("On Update:"), this), 3, 0);
    onUpdateCombo_ = new QComboBox(this);
    onUpdateCombo_->addItems({"NO ACTION", "CASCADE", "SET NULL", "SET DEFAULT", "RESTRICT"});
    formLayout->addWidget(onUpdateCombo_, 3, 1);
    
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ForeignKeyEditorDialog::setForeignKey(const ConstraintsTab::ForeignKey& fk) {
    nameEdit_->setText(fk.name);
    onDeleteCombo_->setCurrentText(fk.onDelete);
    onUpdateCombo_->setCurrentText(fk.onUpdate);
}

ConstraintsTab::ForeignKey ForeignKeyEditorDialog::foreignKey() const {
    ConstraintsTab::ForeignKey fk;
    fk.name = nameEdit_->text();
    fk.onDelete = onDeleteCombo_->currentText();
    fk.onUpdate = onUpdateCombo_->currentText();
    return fk;
}

void ForeignKeyEditorDialog::setAvailableColumns(const QStringList& columns) {
    Q_UNUSED(columns)
    // TODO: Populate column lists
}

void ForeignKeyEditorDialog::setAvailableTables(const QStringList& tables) {
    refTableCombo_->clear();
    refTableCombo_->addItems(tables);
}

} // namespace scratchrobin::ui
