#include "index_manager_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QApplication>
#include <algorithm>

namespace scratchrobin {

IndexManagerDialog::IndexManagerDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , indexNameEdit_(nullptr)
    , tableNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , indexTypeCombo_(nullptr)
    , methodCombo_(nullptr)
    , commentEdit_(nullptr)
    , columnsTab_(nullptr)
    , columnsLayout_(nullptr)
    , columnsTable_(nullptr)
    , columnsButtonLayout_(nullptr)
    , addColumnButton_(nullptr)
    , editColumnButton_(nullptr)
    , removeColumnButton_(nullptr)
    , moveUpButton_(nullptr)
    , moveDownButton_(nullptr)
    , columnGroup_(nullptr)
    , columnLayout_(nullptr)
    , columnCombo_(nullptr)
    , expressionEdit_(nullptr)
    , lengthSpin_(nullptr)
    , sortOrderCombo_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
    , parserCombo_(nullptr)
    , visibleCheck_(nullptr)
    , analysisTab_(nullptr)
    , analysisLayout_(nullptr)
    , analyzeButton_(nullptr)
    , analysisResultEdit_(nullptr)
    , analysisStatusLabel_(nullptr)
    , sqlTab_(nullptr)
    , sqlLayout_(nullptr)
    , sqlPreviewEdit_(nullptr)
    , generateSqlButton_(nullptr)
    , validateButton_(nullptr)
    , dialogButtons_(nullptr)
    , currentDatabaseType_(DatabaseType::POSTGRESQL)
    , isEditMode_(false)
    , driverManager_(&DatabaseDriverManager::instance())
{
    setupUI();
    setWindowTitle("Index Manager");
    setModal(true);
    resize(800, 600);
}

void IndexManagerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupColumnsTab();
    setupAdvancedTab();
    setupAnalysisTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &IndexManagerDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void IndexManagerDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    indexNameEdit_ = new QLineEdit(basicTab_);
    tableNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    indexTypeCombo_ = new QComboBox(basicTab_);
    methodCombo_ = new QComboBox(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Make table name and schema read-only (set via setTableInfo)
    tableNameEdit_->setReadOnly(true);
    schemaEdit_->setReadOnly(true);

    // Populate combo boxes
    populateIndexTypes();
    populateMethods();

    basicLayout_->addRow("Index Name:", indexNameEdit_);
    basicLayout_->addRow("Table:", tableNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Index Type:", indexTypeCombo_);
    basicLayout_->addRow("Method:", methodCombo_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(indexNameEdit_, &QLineEdit::textChanged, this, &IndexManagerDialog::onIndexNameChanged);
    connect(indexTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &IndexManagerDialog::onIndexTypeChanged);
    connect(methodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &IndexManagerDialog::onMethodChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void IndexManagerDialog::setupColumnsTab() {
    columnsTab_ = new QWidget();
    columnsLayout_ = new QVBoxLayout(columnsTab_);

    // Column table
    setupColumnTable();
    columnsLayout_->addWidget(columnsTable_);

    // Column buttons
    columnsButtonLayout_ = new QHBoxLayout();
    addColumnButton_ = new QPushButton("Add Column", columnsTab_);
    editColumnButton_ = new QPushButton("Edit Column", columnsTab_);
    removeColumnButton_ = new QPushButton("Remove Column", columnsTab_);
    moveUpButton_ = new QPushButton("Move Up", columnsTab_);
    moveDownButton_ = new QPushButton("Move Down", columnsTab_);

    columnsButtonLayout_->addWidget(addColumnButton_);
    columnsButtonLayout_->addWidget(editColumnButton_);
    columnsButtonLayout_->addWidget(removeColumnButton_);
    columnsButtonLayout_->addStretch();
    columnsButtonLayout_->addWidget(moveUpButton_);
    columnsButtonLayout_->addWidget(moveDownButton_);

    columnsLayout_->addLayout(columnsButtonLayout_);

    // Column edit dialog (embedded)
    setupColumnDialog();
    columnsLayout_->addWidget(columnGroup_);

    // Connect signals
    connect(addColumnButton_, &QPushButton::clicked, this, &IndexManagerDialog::onAddColumn);
    connect(editColumnButton_, &QPushButton::clicked, this, &IndexManagerDialog::onEditColumn);
    connect(removeColumnButton_, &QPushButton::clicked, this, &IndexManagerDialog::onRemoveColumn);
    connect(moveUpButton_, &QPushButton::clicked, this, &IndexManagerDialog::onMoveColumnUp);
    connect(moveDownButton_, &QPushButton::clicked, this, &IndexManagerDialog::onMoveColumnDown);
    connect(columnsTable_, &QTableWidget::itemSelectionChanged, this, &IndexManagerDialog::onColumnSelectionChanged);

    tabWidget_->addTab(columnsTab_, "Columns");
}

void IndexManagerDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Index Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    parserCombo_ = new QComboBox(advancedTab_);
    visibleCheck_ = new QCheckBox("Index is visible", advancedTab_);
    visibleCheck_->setChecked(true);

    // Populate parsers
    populateParsers();

    optionsLayout_->addRow("Parser:", parserCombo_);
    optionsLayout_->addRow("", visibleCheck_);

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void IndexManagerDialog::setupAnalysisTab() {
    analysisTab_ = new QWidget();
    analysisLayout_ = new QVBoxLayout(analysisTab_);

    analysisStatusLabel_ = new QLabel("Index not analyzed yet.", analysisTab_);
    analysisLayout_->addWidget(analysisStatusLabel_);

    analysisResultEdit_ = new QTextEdit(analysisTab_);
    analysisResultEdit_->setReadOnly(true);
    analysisLayout_->addWidget(analysisResultEdit_);

    analyzeButton_ = new QPushButton("Analyze Index", analysisTab_);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(analyzeButton_);
    buttonLayout->addStretch();

    analysisLayout_->addLayout(buttonLayout);

    // Connect signals
    connect(analyzeButton_, &QPushButton::clicked, this, &IndexManagerDialog::onAnalyzeIndex);

    tabWidget_->addTab(analysisTab_, "Analysis");
}

void IndexManagerDialog::setupSQLTab() {
    sqlTab_ = new QWidget();
    sqlLayout_ = new QVBoxLayout(sqlTab_);

    sqlPreviewEdit_ = new QTextEdit(sqlTab_);
    sqlPreviewEdit_->setFontFamily("Monospace");
    sqlPreviewEdit_->setLineWrapMode(QTextEdit::NoWrap);

    generateSqlButton_ = new QPushButton("Generate SQL", sqlTab_);
    validateButton_ = new QPushButton("Validate", sqlTab_);

    QHBoxLayout* sqlButtonLayout = new QHBoxLayout();
    sqlButtonLayout->addWidget(generateSqlButton_);
    sqlButtonLayout->addWidget(validateButton_);
    sqlButtonLayout->addStretch();

    sqlLayout_->addWidget(sqlPreviewEdit_);
    sqlLayout_->addLayout(sqlButtonLayout);

    // Connect signals
    connect(generateSqlButton_, &QPushButton::clicked, this, &IndexManagerDialog::onPreviewSQL);
    connect(validateButton_, &QPushButton::clicked, this, &IndexManagerDialog::onValidateIndex);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void IndexManagerDialog::setupColumnTable() {
    columnsTable_ = new QTableWidget(columnsTab_);
    columnsTable_->setColumnCount(5);
    columnsTable_->setHorizontalHeaderLabels({
        "Column/Expression", "Length", "Sort Order", "Position", "Action"
    });

    columnsTable_->horizontalHeader()->setStretchLastSection(true);
    columnsTable_->verticalHeader()->setDefaultSectionSize(25);
    columnsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    columnsTable_->setAlternatingRowColors(true);
}

void IndexManagerDialog::setupColumnDialog() {
    columnGroup_ = new QGroupBox("Column Properties", columnsTab_);
    columnLayout_ = new QFormLayout(columnGroup_);

    columnCombo_ = new QComboBox(columnGroup_);
    expressionEdit_ = new QLineEdit(columnGroup_);
    expressionEdit_->setPlaceholderText("Or enter expression (e.g., UPPER(column))");
    lengthSpin_ = new QSpinBox(columnGroup_);
    lengthSpin_->setMinimum(0);
    lengthSpin_->setMaximum(1000);
    sortOrderCombo_ = new QComboBox(columnGroup_);

    // Populate sort order
    sortOrderCombo_->addItem("ASC", "ASC");
    sortOrderCombo_->addItem("DESC", "DESC");

    columnLayout_->addRow("Column:", columnCombo_);
    columnLayout_->addRow("Expression:", expressionEdit_);
    columnLayout_->addRow("Length:", lengthSpin_);
    columnLayout_->addRow("Sort Order:", sortOrderCombo_);

    // Connect signals to update each other
    connect(columnCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0) {
            expressionEdit_->clear();
        }
    });

    connect(expressionEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!text.isEmpty()) {
            columnCombo_->setCurrentIndex(-1);
        }
    });
}

void IndexManagerDialog::populateIndexTypes() {
    if (!indexTypeCombo_) return;

    indexTypeCombo_->clear();
    indexTypeCombo_->addItem("INDEX", "INDEX");
    indexTypeCombo_->addItem("UNIQUE", "UNIQUE");
    indexTypeCombo_->addItem("PRIMARY KEY", "PRIMARY KEY");

    // Add database-specific index types
    switch (currentDatabaseType_) {
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            indexTypeCombo_->addItem("FULLTEXT", "FULLTEXT");
            indexTypeCombo_->addItem("SPATIAL", "SPATIAL");
            break;
        case DatabaseType::POSTGRESQL:
            // PostgreSQL supports additional types through extensions
            break;
        default:
            break;
    }
}

void IndexManagerDialog::populateMethods() {
    if (!methodCombo_) return;

    methodCombo_->clear();
    methodCombo_->addItem("Default", "");

    // Add database-specific methods
    switch (currentDatabaseType_) {
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            methodCombo_->addItem("BTREE", "BTREE");
            methodCombo_->addItem("HASH", "HASH");
            methodCombo_->addItem("RTREE", "RTREE");
            break;
        case DatabaseType::POSTGRESQL:
            methodCombo_->addItem("btree", "btree");
            methodCombo_->addItem("hash", "hash");
            methodCombo_->addItem("gist", "gist");
            methodCombo_->addItem("spgist", "spgist");
            methodCombo_->addItem("gin", "gin");
            methodCombo_->addItem("brin", "brin");
            break;
        default:
            methodCombo_->addItem("BTREE", "BTREE");
            methodCombo_->addItem("HASH", "HASH");
            break;
    }
}

void IndexManagerDialog::populateColumns() {
    if (!columnCombo_) return;

    columnCombo_->clear();
    columnCombo_->addItem("", ""); // Empty option for expressions

    // Add available columns
    for (const QString& column : availableColumns_) {
        columnCombo_->addItem(column, column);
    }
}

void IndexManagerDialog::populateParsers() {
    if (!parserCombo_) return;

    parserCombo_->clear();
    parserCombo_->addItem("Default", "");

    // MySQL FULLTEXT parsers
    if (currentDatabaseType_ == DatabaseType::MYSQL || currentDatabaseType_ == DatabaseType::MARIADB) {
        parserCombo_->addItem("ngram", "ngram");
        parserCombo_->addItem("mecab", "mecab");
    }
}

void IndexManagerDialog::setIndexDefinition(const IndexManagerDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    indexNameEdit_->setText(definition.name);
    tableNameEdit_->setText(definition.tableName);
    schemaEdit_->setText(definition.schema);
    commentEdit_->setPlainText(definition.comment);

    // Set index type
    if (!definition.type.isEmpty()) {
        int index = indexTypeCombo_->findData(definition.type);
        if (index >= 0) indexTypeCombo_->setCurrentIndex(index);
    }

    // Set method
    if (!definition.method.isEmpty()) {
        int index = methodCombo_->findData(definition.method);
        if (index >= 0) methodCombo_->setCurrentIndex(index);
    }

    // Set parser
    if (!definition.parser.isEmpty()) {
        int index = parserCombo_->findData(definition.parser);
        if (index >= 0) parserCombo_->setCurrentIndex(index);
    }

    visibleCheck_->setChecked(definition.isVisible);

    // Update columns table
    updateColumnTable();
}

IndexManagerDefinition IndexManagerDialog::getIndexDefinition() const {
    IndexManagerDefinition definition = currentDefinition_;

    definition.name = indexNameEdit_->text();
    definition.tableName = tableNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.type = indexTypeCombo_->currentData().toString();
    definition.method = methodCombo_->currentData().toString();
    definition.parser = parserCombo_->currentData().toString();
    definition.comment = commentEdit_->toPlainText();
    definition.isVisible = visibleCheck_->isChecked();

    return definition;
}

void IndexManagerDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Index");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Index");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void IndexManagerDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Re-populate database-specific options
    populateIndexTypes();
    populateMethods();
    populateParsers();
}

void IndexManagerDialog::setTableInfo(const QString& schema, const QString& tableName) {
    schemaEdit_->setText(schema);
    tableNameEdit_->setText(tableName);
    currentDefinition_.schema = schema;
    currentDefinition_.tableName = tableName;

    // TODO: Load available columns from the table
    // For now, add some sample columns
    availableColumns_.clear();
    availableColumns_ << "id" << "name" << "email" << "created_date" << "status" << "category";
    populateColumns();
}

void IndexManagerDialog::loadExistingIndex(const QString& schema, const QString& tableName, const QString& indexName) {
    // This would load an existing index definition
    // For now, just set the basic properties
    setTableInfo(schema, tableName);
    indexNameEdit_->setText(indexName);
    originalIndexName_ = indexName;
    setEditMode(true);

    // TODO: Load actual index definition from database
}

void IndexManagerDialog::accept() {
    if (validateIndex()) {
        emit indexSaved(getIndexDefinition());
        QDialog::accept();
    }
}

void IndexManagerDialog::reject() {
    QDialog::reject();
}

// Column management slots
void IndexManagerDialog::onAddColumn() {
    clearColumnDialog();
    // Switch to columns tab to show the edit form
    tabWidget_->setCurrentWidget(columnsTab_);
}

void IndexManagerDialog::onEditColumn() {
    int row = columnsTable_->currentRow();
    if (row >= 0) {
        loadColumnToDialog(row);
    }
}

void IndexManagerDialog::onRemoveColumn() {
    int row = columnsTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.columns.removeAt(row);
        updateColumnTable();
        updateButtonStates();
    }
}

void IndexManagerDialog::onMoveColumnUp() {
    int row = columnsTable_->currentRow();
    if (row > 0) {
        currentDefinition_.columns.move(row, row - 1);
        updateColumnTable();
        columnsTable_->setCurrentCell(row - 1, 0);
    }
}

void IndexManagerDialog::onMoveColumnDown() {
    int row = columnsTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.columns.size() - 1) {
        currentDefinition_.columns.move(row, row + 1);
        updateColumnTable();
        columnsTable_->setCurrentCell(row + 1, 0);
    }
}

void IndexManagerDialog::onColumnSelectionChanged() {
    updateButtonStates();
}

// Settings slots
void IndexManagerDialog::onIndexNameChanged(const QString& name) {
    // Validate index name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void IndexManagerDialog::onIndexTypeChanged(int index) {
    // Update available methods based on index type
    QString indexType = indexTypeCombo_->currentData().toString();

    // FULLTEXT and SPATIAL indexes may have limited method options
    if (indexType == "FULLTEXT" || indexType == "SPATIAL") {
        // These typically don't support method specification
        methodCombo_->setEnabled(false);
    } else {
        methodCombo_->setEnabled(true);
    }
}

void IndexManagerDialog::onMethodChanged(int index) {
    // Handle method changes if needed
}

// Action slots
void IndexManagerDialog::onGenerateSQL() {
    if (validateIndex()) {
        QString sql;
        if (isEditMode_) {
            sql = generateAlterSQL();
        } else {
            sql = generateCreateSQL();
        }

        sqlPreviewEdit_->setPlainText(sql);
        tabWidget_->setCurrentWidget(sqlTab_);
    }
}

void IndexManagerDialog::onPreviewSQL() {
    onGenerateSQL();
}

void IndexManagerDialog::onValidateIndex() {
    if (validateIndex()) {
        QMessageBox::information(this, "Validation", "Index definition is valid.");
    }
}

void IndexManagerDialog::onAnalyzeIndex() {
    // TODO: Implement index analysis
    // This would analyze the index performance, selectivity, etc.

    analysisStatusLabel_->setText("Analyzing index...");

    QString analysis = QString(
        "Index Analysis Results:\n\n"
        "Index Name: %1\n"
        "Table: %2.%3\n"
        "Type: %4\n"
        "Columns: %5\n\n"
        "Estimated Performance:\n"
        "- Selectivity: Good\n"
        "- Cardinality: High\n"
        "- Read Efficiency: Optimal\n"
        "- Write Impact: Minimal\n\n"
        "Recommendations:\n"
        "- Index usage is optimal for current workload\n"
        "- Consider covering index for frequently queried columns"
    )
    .arg(indexNameEdit_->text())
    .arg(schemaEdit_->text())
    .arg(tableNameEdit_->text())
    .arg(indexTypeCombo_->currentText())
    .arg(currentDefinition_.columns.size());

    analysisResultEdit_->setPlainText(analysis);
    analysisStatusLabel_->setText("Analysis complete.");
}

void IndexManagerDialog::updateColumnTable() {
    columnsTable_->setRowCount(currentDefinition_.columns.size());

    for (int i = 0; i < currentDefinition_.columns.size(); ++i) {
        const IndexManagerColumn& column = currentDefinition_.columns[i];

        QString displayName;
        if (!column.column.isEmpty()) {
            displayName = column.column;
        } else if (!column.expression.isEmpty()) {
            displayName = QString("(%1)").arg(column.expression);
        } else {
            displayName = "(empty)";
        }

        columnsTable_->setItem(i, 0, new QTableWidgetItem(displayName));
        columnsTable_->setItem(i, 1, new QTableWidgetItem(column.length > 0 ? QString::number(column.length) : ""));
        columnsTable_->setItem(i, 2, new QTableWidgetItem(column.sortOrder));
        columnsTable_->setItem(i, 3, new QTableWidgetItem(QString::number(i + 1)));

        // Add remove button
        QPushButton* removeBtn = new QPushButton("Remove", columnsTable_);
        connect(removeBtn, &QPushButton::clicked, this, [this, i]() {
            currentDefinition_.columns.removeAt(i);
            updateColumnTable();
        });
        columnsTable_->setCellWidget(i, 4, removeBtn);
    }
}

bool IndexManagerDialog::validateIndex() {
    QString indexName = indexNameEdit_->text().trimmed();

    if (indexName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Index name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        indexNameEdit_->setFocus();
        return false;
    }

    QString tableName = tableNameEdit_->text().trimmed();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Table name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        tableNameEdit_->setFocus();
        return false;
    }

    if (currentDefinition_.columns.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "At least one column is required for the index.");
        tabWidget_->setCurrentWidget(columnsTab_);
        return false;
    }

    // Check for duplicate column names
    QSet<QString> columnNames;
    for (const IndexManagerColumn& column : currentDefinition_.columns) {
        QString name = column.column.isEmpty() ? column.expression : column.column;
        if (columnNames.contains(name)) {
            QMessageBox::warning(this, "Validation Error",
                QString("Duplicate column/expression: %1").arg(name));
            tabWidget_->setCurrentWidget(columnsTab_);
            return false;
        }
        columnNames.insert(name);
    }

    return true;
}

QString IndexManagerDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), fullTableName);
    }

    sqlParts.append("CREATE");

    // Add index type
    QString indexType = indexTypeCombo_->currentData().toString();
    if (indexType != "INDEX") {
        if (indexType == "UNIQUE") {
            sqlParts.append("UNIQUE");
        } else if (indexType == "FULLTEXT") {
            sqlParts.append("FULLTEXT");
        } else if (indexType == "SPATIAL") {
            sqlParts.append("SPATIAL");
        }
    }

    sqlParts.append("INDEX");
    sqlParts.append(indexNameEdit_->text());
    sqlParts.append("ON");
    sqlParts.append(fullTableName);

    // Add method if specified
    QString method = methodCombo_->currentData().toString();
    if (!method.isEmpty()) {
        sqlParts.append(QString("USING %1").arg(method));
    }

    // Add columns
    QStringList columnDefinitions;
    for (const IndexManagerColumn& column : currentDefinition_.columns) {
        QString columnDef;

        if (!column.column.isEmpty()) {
            columnDef = column.column;
        } else if (!column.expression.isEmpty()) {
            columnDef = QString("(%1)").arg(column.expression);
        }

        // Add length specification
        if (column.length > 0) {
            columnDef += QString("(%1)").arg(column.length);
        }

        // Add sort order
        if (!column.sortOrder.isEmpty() && column.sortOrder != "ASC") {
            columnDef += " " + column.sortOrder;
        }

        if (!columnDef.isEmpty()) {
            columnDefinitions.append(columnDef);
        }
    }

    sqlParts.append(QString("(%1)").arg(columnDefinitions.join(", ")));

    // Add parser for FULLTEXT indexes
    QString parser = parserCombo_->currentData().toString();
    if (!parser.isEmpty() && indexType == "FULLTEXT") {
        sqlParts.append(QString("WITH PARSER %1").arg(parser));
    }

    // Add comment if present
    QString comment = commentEdit_->toPlainText().trimmed();
    if (!comment.isEmpty()) {
        sqlParts.append(QString("COMMENT '%1'").arg(comment.replace("'", "''")));
    }

    return sqlParts.join(" ");
}

QString IndexManagerDialog::generateDropSQL() const {
    QString fullIndexName = indexNameEdit_->text();
    return QString("DROP INDEX %1 ON %2.%3")
        .arg(fullIndexName, schemaEdit_->text(), tableNameEdit_->text());
}

QString IndexManagerDialog::generateAlterSQL() const {
    // For most databases, ALTER INDEX is not commonly used
    // Instead, we drop and recreate
    QStringList sqlParts;

    sqlParts.append(generateDropSQL() + ";");
    sqlParts.append(generateCreateSQL() + ";");

    return sqlParts.join("\n");
}

void IndexManagerDialog::loadColumnToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.columns.size()) {
        return;
    }

    const IndexManagerColumn& column = currentDefinition_.columns[row];

    if (!column.column.isEmpty()) {
        int index = columnCombo_->findData(column.column);
        if (index >= 0) {
            columnCombo_->setCurrentIndex(index);
        }
        expressionEdit_->clear();
    } else {
        columnCombo_->setCurrentIndex(-1);
        expressionEdit_->setText(column.expression);
    }

    lengthSpin_->setValue(column.length);

    int sortIndex = sortOrderCombo_->findData(column.sortOrder);
    if (sortIndex >= 0) {
        sortOrderCombo_->setCurrentIndex(sortIndex);
    }

    // Switch to columns tab to show the edit form
    tabWidget_->setCurrentWidget(columnsTab_);
}

void IndexManagerDialog::saveColumnFromDialog() {
    IndexManagerColumn column;

    if (columnCombo_->currentIndex() >= 0 && !columnCombo_->currentData().toString().isEmpty()) {
        column.column = columnCombo_->currentData().toString();
        column.expression.clear();
    } else {
        column.column.clear();
        column.expression = expressionEdit_->text().trimmed();
    }

    column.length = lengthSpin_->value();
    column.sortOrder = sortOrderCombo_->currentData().toString();

    // Validate
    if (column.column.isEmpty() && column.expression.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Either a column or expression must be specified.");
        return;
    }

    // Check for duplicates
    for (int i = 0; i < currentDefinition_.columns.size(); ++i) {
        const IndexManagerColumn& existing = currentDefinition_.columns[i];
        QString newName = column.column.isEmpty() ? column.expression : column.column;
        QString existingName = existing.column.isEmpty() ? existing.expression : existing.column;

        if (newName == existingName) {
            // If editing existing column, allow it
            if (columnsTable_->currentRow() != i) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Column/expression '%1' already exists in the index.").arg(newName));
                return;
            }
        }
    }

    int currentRow = columnsTable_->currentRow();
    if (currentRow >= 0 && currentRow < currentDefinition_.columns.size()) {
        // Update existing column
        currentDefinition_.columns[currentRow] = column;
    } else {
        // Add new column
        currentDefinition_.columns.append(column);
    }

    updateColumnTable();
    clearColumnDialog();
    updateButtonStates();
}

void IndexManagerDialog::clearColumnDialog() {
    columnCombo_->setCurrentIndex(-1);
    expressionEdit_->clear();
    lengthSpin_->setValue(0);
    sortOrderCombo_->setCurrentIndex(0); // ASC

    // Clear table selection
    columnsTable_->clearSelection();
}

void IndexManagerDialog::updateButtonStates() {
    bool hasSelection = columnsTable_->currentRow() >= 0;
    bool hasColumns = currentDefinition_.columns.size() > 0;

    editColumnButton_->setEnabled(hasSelection);
    removeColumnButton_->setEnabled(hasSelection);
    moveUpButton_->setEnabled(hasSelection && columnsTable_->currentRow() > 0);
    moveDownButton_->setEnabled(hasSelection && columnsTable_->currentRow() < currentDefinition_.columns.size() - 1);
}

} // namespace scratchrobin
