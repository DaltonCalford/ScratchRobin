#include "query_builder.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Visual Query
// ============================================================================

QString VisualQuery::toSql() const {
    if (queryType == "SELECT") {
        QString sql = "SELECT ";
        
        if (isDistinct) {
            sql += "DISTINCT ";
        }
        
        // Columns
        if (columns.isEmpty()) {
            sql += "*";
        } else {
            QStringList colList;
            for (const auto& col : columns) {
                QString colStr;
                if (!col.tableAlias.isEmpty()) {
                    colStr = col.tableAlias + ".";
                }
                colStr += col.name;
                if (!col.aggregate.isEmpty()) {
                    colStr = col.aggregate + "(" + colStr + ")";
                }
                if (!col.alias.isEmpty()) {
                    colStr += " AS " + col.alias;
                }
                colList.append(colStr);
            }
            sql += colList.join(", ");
        }
        
        // FROM
        if (!tables.isEmpty()) {
            sql += "\nFROM ";
            QStringList tableList;
            for (const auto& table : tables) {
                QString tableStr = table.name;
                if (!table.alias.isEmpty()) {
                    tableStr += " " + table.alias;
                }
                tableList.append(tableStr);
            }
            sql += tableList.join(", ");
        }
        
        // WHERE
        if (!whereConditions.isEmpty()) {
            sql += "\nWHERE ";
            QStringList condList;
            for (const auto& cond : whereConditions) {
                QString condStr = cond.column + " " + cond.op + " " + cond.value;
                if (!cond.logicOp.isEmpty() && condList.size() > 0) {
                    condStr = cond.logicOp + " " + condStr;
                }
                condList.append(condStr);
            }
            sql += condList.join(" ");
        }
        
        // GROUP BY
        if (!groupByColumns.isEmpty()) {
            sql += "\nGROUP BY ";
            QStringList groupList;
            for (const auto& col : groupByColumns) {
                groupList.append(col.name);
            }
            sql += groupList.join(", ");
        }
        
        // ORDER BY
        if (!orderByColumns.isEmpty()) {
            sql += "\nORDER BY ";
            QStringList orderList;
            for (const auto& col : orderByColumns) {
                QString orderStr = col.name;
                if (!col.sortOrder.isEmpty()) {
                    orderStr += " " + col.sortOrder;
                }
                orderList.append(orderStr);
            }
            sql += orderList.join(", ");
        }
        
        // LIMIT
        if (limit > 0) {
            sql += "\nLIMIT " + QString::number(limit);
        }
        
        // OFFSET
        if (offset > 0) {
            sql += "\nOFFSET " + QString::number(offset);
        }
        
        return sql;
    } else if (queryType == "INSERT") {
        QString sql = "INSERT INTO ";
        
        // Table
        if (!tables.isEmpty()) {
            sql += tables[0].name;
        }
        
        // Columns
        if (!columns.isEmpty()) {
            sql += " (";
            QStringList colList;
            for (const auto& col : columns) {
                colList.append(col.name);
            }
            sql += colList.join(", ");
            sql += ")";
        }
        
        // VALUES
        sql += "\nVALUES (";
        QStringList valueList;
        for (const auto& col : columns) {
            if (col.expression.isEmpty()) {
                valueList.append("NULL");
            } else {
                valueList.append(col.expression);
            }
        }
        sql += valueList.join(", ");
        sql += ")";
        
        return sql;
    } else if (queryType == "UPDATE") {
        QString sql = "UPDATE ";
        
        // Table
        if (!tables.isEmpty()) {
            sql += tables[0].name;
        }
        
        // SET
        sql += "\nSET ";
        QStringList setList;
        for (const auto& col : columns) {
            if (!col.expression.isEmpty()) {
                setList.append(col.name + " = " + col.expression);
            }
        }
        sql += setList.join(", ");
        
        // WHERE
        if (!whereConditions.isEmpty()) {
            sql += "\nWHERE ";
            QStringList condList;
            for (const auto& cond : whereConditions) {
                QString condStr = cond.column + " " + cond.op + " " + cond.value;
                if (!cond.logicOp.isEmpty() && condList.size() > 0) {
                    condStr = cond.logicOp + " " + condStr;
                }
                condList.append(condStr);
            }
            sql += condList.join(" ");
        }
        
        return sql;
    } else if (queryType == "DELETE") {
        QString sql = "DELETE FROM ";
        
        // Table
        if (!tables.isEmpty()) {
            sql += tables[0].name;
        }
        
        // WHERE
        if (!whereConditions.isEmpty()) {
            sql += "\nWHERE ";
            QStringList condList;
            for (const auto& cond : whereConditions) {
                QString condStr = cond.column + " " + cond.op + " " + cond.value;
                if (!cond.logicOp.isEmpty() && condList.size() > 0) {
                    condStr = cond.logicOp + " " + condStr;
                }
                condList.append(condStr);
            }
            sql += condList.join(" ");
        }
        
        return sql;
    }
    
    return "-- Query type not supported: " + queryType;
}

void VisualQuery::clear() {
    tables.clear();
    joins.clear();
    columns.clear();
    whereConditions.clear();
    groupByColumns.clear();
    orderByColumns.clear();
    isDistinct = false;
    limit = 0;
    offset = 0;
}

// ============================================================================
// Query Builder Panel
// ============================================================================

QueryBuilderPanel::QueryBuilderPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("query_builder", parent)
    , client_(client) {
    setupUi();
}

void QueryBuilderPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    queryTypeCombo_ = new QComboBox(this);
    queryTypeCombo_->addItems({"SELECT", "INSERT", "UPDATE", "DELETE"});
    connect(queryTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onQueryTypeChanged(queryTypeCombo_->currentText()); });
    toolbarLayout->addWidget(new QLabel(tr("Query Type:"), this));
    toolbarLayout->addWidget(queryTypeCombo_);
    
    toolbarLayout->addSpacing(20);
    
    distinctCheck_ = new QCheckBox(tr("DISTINCT"), this);
    connect(distinctCheck_, &QCheckBox::toggled, this, &QueryBuilderPanel::updateSqlPreview);
    toolbarLayout->addWidget(distinctCheck_);
    
    toolbarLayout->addSpacing(20);
    
    auto* addTableBtn = new QPushButton(tr("Add Table"), this);
    connect(addTableBtn, &QPushButton::clicked, this, &QueryBuilderPanel::onAddTable);
    toolbarLayout->addWidget(addTableBtn);
    
    auto* addJoinBtn = new QPushButton(tr("Add Join"), this);
    connect(addJoinBtn, &QPushButton::clicked, this, &QueryBuilderPanel::onAddJoin);
    toolbarLayout->addWidget(addJoinBtn);
    
    toolbarLayout->addStretch();
    
    auto* genSqlBtn = new QPushButton(tr("Generate SQL"), this);
    connect(genSqlBtn, &QPushButton::clicked, this, &QueryBuilderPanel::onGenerateSql);
    toolbarLayout->addWidget(genSqlBtn);
    
    auto* execBtn = new QPushButton(tr("Execute"), this);
    connect(execBtn, &QPushButton::clicked, this, &QueryBuilderPanel::onExecuteQuery);
    toolbarLayout->addWidget(execBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Table browser
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel(tr("Available Tables"), this));
    
    tableBrowser_ = new QTreeView(this);
    tableModel_ = new QStandardItemModel(this);
    tableModel_->setHorizontalHeaderLabels({tr("Table"), tr("Schema")});
    tableBrowser_->setModel(tableModel_);
    leftLayout->addWidget(tableBrowser_, 1);
    
    // Add sample tables
    auto* tablesItem = new QStandardItem(tr("Tables"));
    QStringList tables = {"customers", "orders", "products", "categories"};
    for (const auto& t : tables) {
        QList<QStandardItem*> row;
        row << new QStandardItem(t);
        row << new QStandardItem("public");
        tablesItem->appendRow(row);
    }
    tableModel_->appendRow(tablesItem);
    tableBrowser_->expandAll();
    
    splitter->addWidget(leftWidget);
    
    // Center: Canvas
    setupCanvas();
    splitter->addWidget(canvas_);
    
    // Right: Properties panel
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    // Columns tab
    rightLayout->addWidget(new QLabel(tr("Selected Columns"), this));
    columnsTable_ = new QTableView(this);
    columnsModel_ = new QStandardItemModel(this);
    columnsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Column"), tr("Alias"), tr("Aggregate")});
    columnsTable_->setModel(columnsModel_);
    rightLayout->addWidget(columnsTable_);
    
    // Conditions tab
    rightLayout->addWidget(new QLabel(tr("WHERE Conditions"), this));
    conditionsTable_ = new QTableView(this);
    conditionsModel_ = new QStandardItemModel(this);
    conditionsModel_->setHorizontalHeaderLabels({tr("And/Or"), tr("Column"), tr("Operator"), tr("Value")});
    conditionsTable_->setModel(conditionsModel_);
    rightLayout->addWidget(conditionsTable_);
    
    // SQL Preview
    rightLayout->addWidget(new QLabel(tr("Generated SQL"), this));
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setFont(QFont("Consolas", 9));
    sqlPreview_->setMaximumHeight(150);
    rightLayout->addWidget(sqlPreview_);
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({200, 500, 300});
    
    mainLayout->addWidget(splitter, 1);
}

void QueryBuilderPanel::setupCanvas() {
    scene_ = new QGraphicsScene(this);
    canvas_ = new QGraphicsView(scene_, this);
    canvas_->setRenderHint(QPainter::Antialiasing);
    canvas_->setDragMode(QGraphicsView::RubberBandDrag);
    canvas_->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
}

void QueryBuilderPanel::setupTablesPanel() {
    // Implemented in setupUi
}

void QueryBuilderPanel::setupColumnsPanel() {
    // Implemented in setupUi
}

void QueryBuilderPanel::setupConditionsPanel() {
    // Implemented in setupUi
}

void QueryBuilderPanel::refreshCanvas() {
    scene_->clear();
    
    // Draw tables
    for (const auto& table : currentQuery_.tables) {
        QRectF rect(table.position, QSizeF(150, 100));
        auto* rectItem = scene_->addRect(rect, QPen(Qt::black), QBrush(QColor(255, 255, 200)));
        
        auto* text = scene_->addText(table.name, QFont("Arial", 10, QFont::Bold));
        text->setPos(table.position.x() + 5, table.position.y() + 5);
    }
}

void QueryBuilderPanel::updateSqlPreview() {
    currentQuery_.isDistinct = distinctCheck_->isChecked();
    sqlPreview_->setPlainText(currentQuery_.toSql());
}

void QueryBuilderPanel::onAddTable() {
    bool ok;
    QString tableName = QInputDialog::getText(this, tr("Add Table"),
        tr("Table name:"), QLineEdit::Normal, QString(), &ok);
    
    if (!ok || tableName.isEmpty()) return;
    
    QueryTable table;
    table.id = QString::number(currentQuery_.tables.size() + 1);
    table.name = tableName;
    table.alias = tableName.left(1).toLower();
    table.position = QPointF(50 + currentQuery_.tables.size() * 160, 50);
    
    currentQuery_.tables.append(table);
    refreshCanvas();
    updateSqlPreview();
}

void QueryBuilderPanel::onRemoveTable() {
    // Remove selected table
}

void QueryBuilderPanel::onAddJoin() {
    if (currentQuery_.tables.size() < 2) {
        QMessageBox::information(this, tr("Add Join"),
            tr("Need at least 2 tables to create a join."));
        return;
    }
    
    QueryJoin join;
    JoinConfigDialog dialog(&join, currentQuery_.tables, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentQuery_.joins.append(join);
        updateSqlPreview();
    }
}

void QueryBuilderPanel::onRemoveJoin() {
    // Remove selected join
}

void QueryBuilderPanel::onTableSelected(const QString& tableId) {
    Q_UNUSED(tableId)
}

void QueryBuilderPanel::onCanvasContextMenu(const QPoint& pos) {
    Q_UNUSED(pos)
}

void QueryBuilderPanel::onAddColumn() {
    QueryColumn col;
    col.name = "id";
    col.tableAlias = "a";
    currentQuery_.columns.append(col);
    
    QList<QStandardItem*> row;
    row << new QStandardItem(col.tableAlias);
    row << new QStandardItem(col.name);
    row << new QStandardItem(col.alias);
    row << new QStandardItem(col.aggregate);
    columnsModel_->appendRow(row);
    
    updateSqlPreview();
}

void QueryBuilderPanel::onRemoveColumn() {
    auto index = columnsTable_->currentIndex();
    if (!index.isValid()) return;
    
    columnsModel_->removeRow(index.row());
    if (index.row() < currentQuery_.columns.size()) {
        currentQuery_.columns.removeAt(index.row());
    }
    updateSqlPreview();
}

void QueryBuilderPanel::onColumnMoved(int from, int to) {
    if (from < 0 || from >= currentQuery_.columns.size()) return;
    if (to < 0 || to >= currentQuery_.columns.size()) return;
    
    currentQuery_.columns.move(from, to);
    updateSqlPreview();
}

void QueryBuilderPanel::onAddWhereCondition() {
    QueryCondition cond;
    cond.id = QString::number(currentQuery_.whereConditions.size() + 1);
    cond.column = "column_name";
    cond.op = "=";
    cond.value = "value";
    cond.logicOp = currentQuery_.whereConditions.isEmpty() ? "" : "AND";
    
    currentQuery_.whereConditions.append(cond);
    
    QList<QStandardItem*> row;
    row << new QStandardItem(cond.logicOp);
    row << new QStandardItem(cond.column);
    row << new QStandardItem(cond.op);
    row << new QStandardItem(cond.value);
    conditionsModel_->appendRow(row);
    
    updateSqlPreview();
}

void QueryBuilderPanel::onEditCondition() {
    // Edit selected condition
}

void QueryBuilderPanel::onRemoveCondition() {
    auto index = conditionsTable_->currentIndex();
    if (!index.isValid()) return;
    
    conditionsModel_->removeRow(index.row());
    if (index.row() < currentQuery_.whereConditions.size()) {
        currentQuery_.whereConditions.removeAt(index.row());
    }
    updateSqlPreview();
}

void QueryBuilderPanel::onQueryTypeChanged(const QString& type) {
    currentQuery_.queryType = type;
    updateSqlPreview();
}

void QueryBuilderPanel::onGenerateSql() {
    updateSqlPreview();
    emit sqlGenerated(sqlPreview_->toPlainText());
}

void QueryBuilderPanel::onExecuteQuery() {
    updateSqlPreview();
    emit queryExecuted(sqlPreview_->toPlainText());
}

void QueryBuilderPanel::onSaveQuery() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Query"),
        QString(),
        tr("JSON Files (*.json);;SQL Files (*.sql);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    // Save as SQL if .sql extension
    if (fileName.endsWith(".sql", Qt::CaseInsensitive)) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(currentQuery_.toSql().toUtf8());
            file.close();
        }
        return;
    }
    
    // Save as JSON (default)
    QJsonObject obj;
    obj["queryType"] = currentQuery_.queryType;
    obj["isDistinct"] = currentQuery_.isDistinct;
    obj["limit"] = currentQuery_.limit;
    obj["offset"] = currentQuery_.offset;
    
    // Save tables
    QJsonArray tables;
    for (const auto& t : currentQuery_.tables) {
        QJsonObject to;
        to["id"] = t.id;
        to["name"] = t.name;
        to["alias"] = t.alias;
        to["x"] = t.position.x();
        to["y"] = t.position.y();
        tables.append(to);
    }
    obj["tables"] = tables;
    
    // Save columns
    QJsonArray columns;
    for (const auto& c : currentQuery_.columns) {
        QJsonObject co;
        co["name"] = c.name;
        co["tableAlias"] = c.tableAlias;
        co["aggregate"] = c.aggregate;
        co["alias"] = c.alias;
        co["expression"] = c.expression;
        co["isVisible"] = c.isVisible;
        columns.append(co);
    }
    obj["columns"] = columns;
    
    // Save joins
    QJsonArray joins;
    for (const auto& j : currentQuery_.joins) {
        QJsonObject jo;
        jo["id"] = j.id;
        jo["leftTable"] = j.leftTable;
        jo["rightTable"] = j.rightTable;
        jo["leftColumn"] = j.leftColumn;
        jo["rightColumn"] = j.rightColumn;
        jo["joinType"] = j.joinType;
        joins.append(jo);
    }
    obj["joins"] = joins;
    
    // Save where conditions
    QJsonArray conditions;
    for (const auto& c : currentQuery_.whereConditions) {
        QJsonObject co;
        co["id"] = c.id;
        co["column"] = c.column;
        co["op"] = c.op;
        co["value"] = c.value;
        co["logicOp"] = c.logicOp;
        conditions.append(co);
    }
    obj["whereConditions"] = conditions;
    
    QJsonDocument doc(obj);
    
    // Ensure .json extension
    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
        fileName += ".json";
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void QueryBuilderPanel::onLoadQuery() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Query"),
        QString(),
        tr("JSON Files (*.json);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Cannot open file for reading."));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Invalid JSON format."));
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // Clear current query
    currentQuery_.clear();
    columnsModel_->clear();
    conditionsModel_->clear();
    
    // Load query type
    currentQuery_.queryType = obj["queryType"].toString("SELECT");
    queryTypeCombo_->setCurrentText(currentQuery_.queryType);
    
    // Load tables
    QJsonArray tables = obj["tables"].toArray();
    for (const auto& t : tables) {
        QJsonObject to = t.toObject();
        QueryTable table;
        table.id = to["id"].toString();
        table.name = to["name"].toString();
        table.alias = to["alias"].toString();
        table.position = QPointF(to["x"].toDouble(), to["y"].toDouble());
        currentQuery_.tables.append(table);
    }
    
    // Load columns
    QJsonArray columns = obj["columns"].toArray();
    for (const auto& c : columns) {
        QJsonObject co = c.toObject();
        QueryColumn col;
        col.name = co["name"].toString();
        col.tableAlias = co["tableAlias"].toString();
        col.aggregate = co["aggregate"].toString();
        col.alias = co["alias"].toString();
        col.expression = co["expression"].toString();
        col.isVisible = co["isVisible"].toBool(true);
        currentQuery_.columns.append(col);
        
        QList<QStandardItem*> row;
        row << new QStandardItem(col.name);
        row << new QStandardItem(col.tableAlias);
        row << new QStandardItem(col.alias);
        row << new QStandardItem(col.aggregate);
        columnsModel_->appendRow(row);
    }
    
    // Load joins
    QJsonArray joins = obj["joins"].toArray();
    for (const auto& j : joins) {
        QJsonObject jo = j.toObject();
        QueryJoin join;
        join.id = jo["id"].toString();
        join.leftTable = jo["leftTable"].toString();
        join.rightTable = jo["rightTable"].toString();
        join.leftColumn = jo["leftColumn"].toString();
        join.rightColumn = jo["rightColumn"].toString();
        join.joinType = jo["joinType"].toString();
        currentQuery_.joins.append(join);
    }
    
    // Load where conditions
    QJsonArray conditions = obj["whereConditions"].toArray();
    for (const auto& c : conditions) {
        QJsonObject co = c.toObject();
        QueryCondition cond;
        cond.id = co["id"].toString();
        cond.column = co["column"].toString();
        cond.op = co["op"].toString();
        cond.value = co["value"].toString();
        cond.logicOp = co["logicOp"].toString();
        currentQuery_.whereConditions.append(cond);
        
        QList<QStandardItem*> row;
        row << new QStandardItem(cond.logicOp);
        row << new QStandardItem(cond.column);
        row << new QStandardItem(cond.op);
        row << new QStandardItem(cond.value);
        conditionsModel_->appendRow(row);
    }
    
    // Load options
    currentQuery_.isDistinct = obj["isDistinct"].toBool();
    distinctCheck_->setChecked(currentQuery_.isDistinct);
    currentQuery_.limit = obj["limit"].toInt();
    limitEdit_->setText(currentQuery_.limit > 0 ? QString::number(currentQuery_.limit) : "");
    
    refreshCanvas();
    updateSqlPreview();
    
    QMessageBox::information(this, tr("Query Loaded"),
        tr("Query loaded from:\n%1").arg(fileName));
}

void QueryBuilderPanel::onClearQuery() {
    currentQuery_.clear();
    columnsModel_->clear();
    conditionsModel_->clear();
    refreshCanvas();
    updateSqlPreview();
}

void QueryBuilderPanel::onZoomIn() {
    canvas_->scale(1.25, 1.25);
}

void QueryBuilderPanel::onZoomOut() {
    canvas_->scale(0.8, 0.8);
}

void QueryBuilderPanel::onZoomFit() {
    canvas_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void QueryBuilderPanel::onToggleGrid(bool show) {
    showGrid_ = show;
    if (show) {
        canvas_->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    } else {
        canvas_->setBackgroundBrush(QBrush(Qt::white));
    }
}

// ============================================================================
// Join Configuration Dialog
// ============================================================================

JoinConfigDialog::JoinConfigDialog(QueryJoin* join, const QList<QueryTable>& tables, QWidget* parent)
    : QDialog(parent)
    , join_(join)
    , tables_(tables) {
    setupUi();
}

void JoinConfigDialog::setupUi() {
    setWindowTitle(tr("Configure Join"));
    resize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    leftTableCombo_ = new QComboBox(this);
    rightTableCombo_ = new QComboBox(this);
    for (const auto& t : tables_) {
        leftTableCombo_->addItem(t.name + " (" + t.alias + ")", t.id);
        rightTableCombo_->addItem(t.name + " (" + t.alias + ")", t.id);
    }
    connect(leftTableCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JoinConfigDialog::onLeftTableChanged);
    connect(rightTableCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JoinConfigDialog::onRightTableChanged);
    
    formLayout->addRow(tr("Left Table:"), leftTableCombo_);
    formLayout->addRow(tr("Right Table:"), rightTableCombo_);
    
    joinTypeCombo_ = new QComboBox(this);
    joinTypeCombo_->addItems({"INNER JOIN", "LEFT JOIN", "RIGHT JOIN", "FULL JOIN", "CROSS JOIN"});
    formLayout->addRow(tr("Join Type:"), joinTypeCombo_);
    
    leftColumnCombo_ = new QComboBox(this);
    rightColumnCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Left Column:"), leftColumnCombo_);
    formLayout->addRow(tr("Right Column:"), rightColumnCombo_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &JoinConfigDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void JoinConfigDialog::onLeftTableChanged(int index) {
    Q_UNUSED(index)
    // Load columns for left table
    leftColumnCombo_->clear();
    leftColumnCombo_->addItems({"id", "name", "created_at"});
}

void JoinConfigDialog::onRightTableChanged(int index) {
    Q_UNUSED(index)
    // Load columns for right table
    rightColumnCombo_->clear();
    rightColumnCombo_->addItems({"id", "name", "created_at"});
}

void JoinConfigDialog::onAccept() {
    join_->leftTable = leftTableCombo_->currentData().toString();
    join_->rightTable = rightTableCombo_->currentData().toString();
    join_->leftColumn = leftColumnCombo_->currentText();
    join_->rightColumn = rightColumnCombo_->currentText();
    join_->joinType = joinTypeCombo_->currentText();
    accept();
}

// ============================================================================
// Where Condition Dialog
// ============================================================================

WhereConditionDialog::WhereConditionDialog(QueryCondition* condition,
                                          const VisualQuery& query,
                                          QWidget* parent)
    : QDialog(parent)
    , condition_(condition)
    , query_(query) {
    setupUi();
}

void WhereConditionDialog::setupUi() {
    setWindowTitle(tr("Edit Condition"));
    resize(400, 200);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    columnCombo_ = new QComboBox(this);
    for (const auto& table : query_.tables) {
        columnCombo_->addItem(table.name + ".id");
        columnCombo_->addItem(table.name + ".name");
    }
    formLayout->addRow(tr("Column:"), columnCombo_);
    
    operatorCombo_ = new QComboBox(this);
    operatorCombo_->addItems({"=", "<>", "<", ">", "<=", ">=", "LIKE", "IN", "BETWEEN", "IS NULL"});
    formLayout->addRow(tr("Operator:"), operatorCombo_);
    
    valueEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Value:"), valueEdit_);
    
    logicOpCombo_ = new QComboBox(this);
    logicOpCombo_->addItems({"", "AND", "OR"});
    formLayout->addRow(tr("Logic:"), logicOpCombo_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &WhereConditionDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void WhereConditionDialog::onColumnChanged(int index) {
    Q_UNUSED(index)
}

void WhereConditionDialog::onOperatorChanged(int index) {
    Q_UNUSED(index)
}

void WhereConditionDialog::updateValueWidget() {
    // Update value widget based on operator
}

void WhereConditionDialog::onAccept() {
    condition_->column = columnCombo_->currentText();
    condition_->op = operatorCombo_->currentText();
    condition_->value = valueEdit_->text();
    condition_->logicOp = logicOpCombo_->currentText();
    accept();
}

} // namespace scratchrobin::ui
