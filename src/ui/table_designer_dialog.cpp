#include "table_designer_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <algorithm>

namespace scratchrobin {

TableDesignerDialog::TableDesignerDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , tableNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , engineCombo_(nullptr)
    , charsetCombo_(nullptr)
    , collationCombo_(nullptr)
    , commentEdit_(nullptr)
    , columnsTab_(nullptr)
    , columnsLayout_(nullptr)
    , columnsTable_(nullptr)
    , columnsButtonLayout_(nullptr)
    , addColumnButton_(nullptr)
    , editColumnButton_(nullptr)
    , deleteColumnButton_(nullptr)
    , moveUpButton_(nullptr)
    , moveDownButton_(nullptr)
    , columnGroup_(nullptr)
    , columnLayout_(nullptr)
    , columnNameEdit_(nullptr)
    , dataTypeCombo_(nullptr)
    , lengthSpin_(nullptr)
    , precisionSpin_(nullptr)
    , scaleSpin_(nullptr)
    , nullableCheck_(nullptr)
    , defaultValueEdit_(nullptr)
    , primaryKeyCheck_(nullptr)
    , uniqueCheck_(nullptr)
    , autoIncrementCheck_(nullptr)
    , columnCommentEdit_(nullptr)
    , indexesTab_(nullptr)
    , indexesLayout_(nullptr)
    , indexesTable_(nullptr)
    , indexesButtonLayout_(nullptr)
    , addIndexButton_(nullptr)
    , editIndexButton_(nullptr)
    , deleteIndexButton_(nullptr)
    , constraintsTab_(nullptr)
    , constraintsLayout_(nullptr)
    , constraintsTable_(nullptr)
    , constraintsButtonLayout_(nullptr)
    , addConstraintButton_(nullptr)
    , editConstraintButton_(nullptr)
    , deleteConstraintButton_(nullptr)
    , foreignKeysTab_(nullptr)
    , foreignKeysLayout_(nullptr)
    , foreignKeysTable_(nullptr)
    , foreignKeysButtonLayout_(nullptr)
    , addForeignKeyButton_(nullptr)
    , editForeignKeyButton_(nullptr)
    , deleteForeignKeyButton_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
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
    setWindowTitle("Table Designer");
    setModal(true);
    resize(900, 700);
}

void TableDesignerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupColumnsTab();
    setupIndexesTab();
    setupConstraintsTab();
    setupForeignKeysTab();
    setupAdvancedTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &TableDesignerDialog::onGenerateSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void TableDesignerDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    tableNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    engineCombo_ = new QComboBox(basicTab_);
    charsetCombo_ = new QComboBox(basicTab_);
    collationCombo_ = new QComboBox(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    basicLayout_->addRow("Table Name:", tableNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Engine:", engineCombo_);
    basicLayout_->addRow("Charset:", charsetCombo_);
    basicLayout_->addRow("Collation:", collationCombo_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Populate combo boxes
    populateEngines();
    populateCharsets();
    populateCollations();

    // Connect signals
    connect(tableNameEdit_, &QLineEdit::textChanged, this, &TableDesignerDialog::onTableNameChanged);
    connect(engineCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TableDesignerDialog::onEngineChanged);
    connect(charsetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TableDesignerDialog::onCharsetChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void TableDesignerDialog::setupColumnsTab() {
    columnsTab_ = new QWidget();
    columnsLayout_ = new QVBoxLayout(columnsTab_);

    // Column table
    setupColumnTable();
    columnsLayout_->addWidget(columnsTable_);

    // Column buttons
    columnsButtonLayout_ = new QHBoxLayout();
    addColumnButton_ = new QPushButton("Add Column", columnsTab_);
    editColumnButton_ = new QPushButton("Edit Column", columnsTab_);
    deleteColumnButton_ = new QPushButton("Delete Column", columnsTab_);
    moveUpButton_ = new QPushButton("Move Up", columnsTab_);
    moveDownButton_ = new QPushButton("Move Down", columnsTab_);

    columnsButtonLayout_->addWidget(addColumnButton_);
    columnsButtonLayout_->addWidget(editColumnButton_);
    columnsButtonLayout_->addWidget(deleteColumnButton_);
    columnsButtonLayout_->addStretch();
    columnsButtonLayout_->addWidget(moveUpButton_);
    columnsButtonLayout_->addWidget(moveDownButton_);

    columnsLayout_->addLayout(columnsButtonLayout_);

    // Column edit group
    columnGroup_ = new QGroupBox("Column Properties", columnsTab_);
    columnLayout_ = new QFormLayout(columnGroup_);

    columnNameEdit_ = new QLineEdit(columnGroup_);
    dataTypeCombo_ = new QComboBox(columnGroup_);
    lengthSpin_ = new QSpinBox(columnGroup_);
    precisionSpin_ = new QSpinBox(columnGroup_);
    scaleSpin_ = new QSpinBox(columnGroup_);
    nullableCheck_ = new QCheckBox("Nullable", columnGroup_);
    defaultValueEdit_ = new QLineEdit(columnGroup_);
    primaryKeyCheck_ = new QCheckBox("Primary Key", columnGroup_);
    uniqueCheck_ = new QCheckBox("Unique", columnGroup_);
    autoIncrementCheck_ = new QCheckBox("Auto Increment", columnGroup_);
    columnCommentEdit_ = new QTextEdit(columnGroup_);
    columnCommentEdit_->setMaximumHeight(40);

    columnLayout_->addRow("Name:", columnNameEdit_);
    columnLayout_->addRow("Data Type:", dataTypeCombo_);
    columnLayout_->addRow("Length:", lengthSpin_);
    columnLayout_->addRow("Precision:", precisionSpin_);
    columnLayout_->addRow("Scale:", scaleSpin_);
    columnLayout_->addRow("", nullableCheck_);
    columnLayout_->addRow("Default Value:", defaultValueEdit_);
    columnLayout_->addRow("", primaryKeyCheck_);
    columnLayout_->addRow("", uniqueCheck_);
    columnLayout_->addRow("", autoIncrementCheck_);
    columnLayout_->addRow("Comment:", columnCommentEdit_);

    columnsLayout_->addWidget(columnGroup_);

    // Populate data types
    populateDataTypes();

    // Connect signals
    connect(addColumnButton_, &QPushButton::clicked, this, &TableDesignerDialog::onAddColumn);
    connect(editColumnButton_, &QPushButton::clicked, this, &TableDesignerDialog::onEditColumn);
    connect(deleteColumnButton_, &QPushButton::clicked, this, &TableDesignerDialog::onDeleteColumn);
    connect(moveUpButton_, &QPushButton::clicked, this, &TableDesignerDialog::onMoveColumnUp);
    connect(moveDownButton_, &QPushButton::clicked, this, &TableDesignerDialog::onMoveColumnDown);
    connect(columnsTable_, &QTableWidget::itemSelectionChanged, this, &TableDesignerDialog::onColumnSelectionChanged);

    tabWidget_->addTab(columnsTab_, "Columns");
}

void TableDesignerDialog::setupIndexesTab() {
    indexesTab_ = new QWidget();
    indexesLayout_ = new QVBoxLayout(indexesTab_);

    // Index table
    setupIndexTable();
    indexesLayout_->addWidget(indexesTable_);

    // Index buttons
    indexesButtonLayout_ = new QHBoxLayout();
    addIndexButton_ = new QPushButton("Add Index", indexesTab_);
    editIndexButton_ = new QPushButton("Edit Index", indexesTab_);
    deleteIndexButton_ = new QPushButton("Delete Index", indexesTab_);

    indexesButtonLayout_->addWidget(addIndexButton_);
    indexesButtonLayout_->addWidget(editIndexButton_);
    indexesButtonLayout_->addWidget(deleteIndexButton_);
    indexesButtonLayout_->addStretch();

    indexesLayout_->addLayout(indexesButtonLayout_);

    // Connect signals
    connect(addIndexButton_, &QPushButton::clicked, this, &TableDesignerDialog::onAddIndex);
    connect(editIndexButton_, &QPushButton::clicked, this, &TableDesignerDialog::onEditIndex);
    connect(deleteIndexButton_, &QPushButton::clicked, this, &TableDesignerDialog::onDeleteIndex);

    tabWidget_->addTab(indexesTab_, "Indexes");
}

void TableDesignerDialog::setupConstraintsTab() {
    constraintsTab_ = new QWidget();
    constraintsLayout_ = new QVBoxLayout(constraintsTab_);

    // Constraint table
    setupConstraintTable();
    constraintsLayout_->addWidget(constraintsTable_);

    // Constraint buttons
    constraintsButtonLayout_ = new QHBoxLayout();
    addConstraintButton_ = new QPushButton("Add Constraint", constraintsTab_);
    editConstraintButton_ = new QPushButton("Edit Constraint", constraintsTab_);
    deleteConstraintButton_ = new QPushButton("Delete Constraint", constraintsTab_);

    constraintsButtonLayout_->addWidget(addConstraintButton_);
    constraintsButtonLayout_->addWidget(editConstraintButton_);
    constraintsButtonLayout_->addWidget(deleteConstraintButton_);
    constraintsButtonLayout_->addStretch();

    constraintsLayout_->addLayout(constraintsButtonLayout_);

    // Connect signals
    connect(addConstraintButton_, &QPushButton::clicked, this, &TableDesignerDialog::onAddConstraint);
    connect(editConstraintButton_, &QPushButton::clicked, this, &TableDesignerDialog::onEditConstraint);
    connect(deleteConstraintButton_, &QPushButton::clicked, this, &TableDesignerDialog::onDeleteConstraint);

    tabWidget_->addTab(constraintsTab_, "Constraints");
}

void TableDesignerDialog::setupForeignKeysTab() {
    foreignKeysTab_ = new QWidget();
    foreignKeysLayout_ = new QVBoxLayout(foreignKeysTab_);

    // Foreign key table
    setupForeignKeyTable();
    foreignKeysLayout_->addWidget(foreignKeysTable_);

    // Foreign key buttons
    foreignKeysButtonLayout_ = new QHBoxLayout();
    addForeignKeyButton_ = new QPushButton("Add Foreign Key", foreignKeysTab_);
    editForeignKeyButton_ = new QPushButton("Edit Foreign Key", foreignKeysTab_);
    deleteForeignKeyButton_ = new QPushButton("Delete Foreign Key", foreignKeysTab_);

    foreignKeysButtonLayout_->addWidget(addForeignKeyButton_);
    foreignKeysButtonLayout_->addWidget(editForeignKeyButton_);
    foreignKeysButtonLayout_->addWidget(deleteForeignKeyButton_);
    foreignKeysButtonLayout_->addStretch();

    foreignKeysLayout_->addLayout(foreignKeysButtonLayout_);

    // Connect signals
    connect(addForeignKeyButton_, &QPushButton::clicked, this, &TableDesignerDialog::onAddForeignKey);
    connect(editForeignKeyButton_, &QPushButton::clicked, this, &TableDesignerDialog::onEditForeignKey);
    connect(deleteForeignKeyButton_, &QPushButton::clicked, this, &TableDesignerDialog::onDeleteForeignKey);

    tabWidget_->addTab(foreignKeysTab_, "Foreign Keys");
}

void TableDesignerDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Table Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    // Add common table options based on database type
    // These will be populated dynamically based on the selected database type

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void TableDesignerDialog::setupSQLTab() {
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
    connect(generateSqlButton_, &QPushButton::clicked, this, &TableDesignerDialog::onGenerateSQL);
    connect(validateButton_, &QPushButton::clicked, this, &TableDesignerDialog::onValidateTable);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void TableDesignerDialog::setupColumnTable() {
    columnsTable_ = new QTableWidget(columnsTab_);
    columnsTable_->setColumnCount(8);
    columnsTable_->setHorizontalHeaderLabels({
        "Name", "Data Type", "Length", "Nullable", "Default", "Primary Key", "Unique", "Auto Inc"
    });

    columnsTable_->horizontalHeader()->setStretchLastSection(true);
    columnsTable_->verticalHeader()->setDefaultSectionSize(25);
    columnsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    columnsTable_->setAlternatingRowColors(true);
}

void TableDesignerDialog::setupIndexTable() {
    indexesTable_ = new QTableWidget(indexesTab_);
    indexesTable_->setColumnCount(4);
    indexesTable_->setHorizontalHeaderLabels({"Name", "Type", "Columns", "Method"});

    indexesTable_->horizontalHeader()->setStretchLastSection(true);
    indexesTable_->verticalHeader()->setDefaultSectionSize(25);
    indexesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    indexesTable_->setAlternatingRowColors(true);
}

void TableDesignerDialog::setupConstraintTable() {
    constraintsTable_ = new QTableWidget(constraintsTab_);
    constraintsTable_->setColumnCount(4);
    constraintsTable_->setHorizontalHeaderLabels({"Name", "Type", "Expression", "Columns"});

    constraintsTable_->horizontalHeader()->setStretchLastSection(true);
    constraintsTable_->verticalHeader()->setDefaultSectionSize(25);
    constraintsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    constraintsTable_->setAlternatingRowColors(true);
}

void TableDesignerDialog::setupForeignKeyTable() {
    foreignKeysTable_ = new QTableWidget(foreignKeysTab_);
    foreignKeysTable_->setColumnCount(6);
    foreignKeysTable_->setHorizontalHeaderLabels({
        "Name", "Column", "Referenced Table", "Referenced Column", "On Delete", "On Update"
    });

    foreignKeysTable_->horizontalHeader()->setStretchLastSection(true);
    foreignKeysTable_->verticalHeader()->setDefaultSectionSize(25);
    foreignKeysTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    foreignKeysTable_->setAlternatingRowColors(true);
}

void TableDesignerDialog::populateDataTypes() {
    dataTypeCombo_->clear();

    // Common SQL data types
    QStringList dataTypes = {
        "VARCHAR", "TEXT", "INT", "BIGINT", "SMALLINT", "TINYINT",
        "DECIMAL", "FLOAT", "DOUBLE", "BOOLEAN", "DATE", "TIME", "DATETIME", "TIMESTAMP",
        "BLOB", "CLOB", "JSON", "UUID", "SERIAL", "BIGSERIAL"
    };

    dataTypeCombo_->addItems(dataTypes);
}

void TableDesignerDialog::populateEngines() {
    engineCombo_->clear();
    engineCombo_->addItem("Default", "");
    engineCombo_->addItem("InnoDB", "InnoDB");
    engineCombo_->addItem("MyISAM", "MyISAM");
    engineCombo_->addItem("MEMORY", "MEMORY");
    engineCombo_->addItem("CSV", "CSV");
    engineCombo_->addItem("ARCHIVE", "ARCHIVE");
}

void TableDesignerDialog::populateCharsets() {
    charsetCombo_->clear();
    charsetCombo_->addItem("Default", "");
    charsetCombo_->addItem("UTF-8", "utf8");
    charsetCombo_->addItem("UTF-8 MB4", "utf8mb4");
    charsetCombo_->addItem("Latin1", "latin1");
    charsetCombo_->addItem("ASCII", "ascii");
}

void TableDesignerDialog::populateCollations() {
    collationCombo_->clear();
    collationCombo_->addItem("Default", "");

    // UTF-8 collations
    collationCombo_->addItem("utf8_general_ci", "utf8_general_ci");
    collationCombo_->addItem("utf8_unicode_ci", "utf8_unicode_ci");
    collationCombo_->addItem("utf8_bin", "utf8_bin");

    // UTF-8 MB4 collations
    collationCombo_->addItem("utf8mb4_general_ci", "utf8mb4_general_ci");
    collationCombo_->addItem("utf8mb4_unicode_ci", "utf8mb4_unicode_ci");
    collationCombo_->addItem("utf8mb4_bin", "utf8mb4_bin");
}

void TableDesignerDialog::setTableDefinition(const TableDesignerDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    tableNameEdit_->setText(definition.name);
    schemaEdit_->setText(definition.schema);
    commentEdit_->setPlainText(definition.comment);

    // Set engine, charset, collation
    if (!definition.engine.isEmpty()) {
        int index = engineCombo_->findData(definition.engine);
        if (index >= 0) engineCombo_->setCurrentIndex(index);
    }

    if (!definition.charset.isEmpty()) {
        int index = charsetCombo_->findData(definition.charset);
        if (index >= 0) charsetCombo_->setCurrentIndex(index);
    }

    if (!definition.collation.isEmpty()) {
        int index = collationCombo_->findData(definition.collation);
        if (index >= 0) collationCombo_->setCurrentIndex(index);
    }

    // Update tables
    updateColumnTable();
    updateIndexTable();
    updateConstraintTable();
    updateForeignKeyTable();
}

TableDesignerDefinition TableDesignerDialog::getTableDefinition() const {
    TableDesignerDefinition definition = currentDefinition_;

    definition.name = tableNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.comment = commentEdit_->toPlainText();

    // Get engine, charset, collation
    definition.engine = engineCombo_->currentData().toString();
    definition.charset = charsetCombo_->currentData().toString();
    definition.collation = collationCombo_->currentData().toString();

    return definition;
}

void TableDesignerDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Table");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Table");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void TableDesignerDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update UI elements based on database type
    // For example, show/hide MySQL-specific options
    bool isMySQL = (type == DatabaseType::MYSQL || type == DatabaseType::MARIADB);
    engineCombo_->setVisible(isMySQL);
    charsetCombo_->setVisible(isMySQL);
    collationCombo_->setVisible(isMySQL);

    basicLayout_->labelForField(engineCombo_)->setVisible(isMySQL);
    basicLayout_->labelForField(charsetCombo_)->setVisible(isMySQL);
    basicLayout_->labelForField(collationCombo_)->setVisible(isMySQL);
}

void TableDesignerDialog::loadExistingTable(const QString& schema, const QString& tableName) {
    // This would load an existing table definition
    // For now, just set the basic properties
    originalSchema_ = schema;
    originalTableName_ = tableName;

    tableNameEdit_->setText(tableName);
    schemaEdit_->setText(schema);
    setEditMode(true);

    // TODO: Load actual table definition from database
    // This would require database connectivity
}

void TableDesignerDialog::accept() {
    if (validateTable()) {
        emit tableSaved(getTableDefinition());
        QDialog::accept();
    }
}

void TableDesignerDialog::reject() {
    QDialog::reject();
}

// Column management slots
void TableDesignerDialog::onAddColumn() {
    clearColumnDialog();
    // Switch to columns tab to show the edit form
    tabWidget_->setCurrentWidget(columnsTab_);
}

void TableDesignerDialog::onEditColumn() {
    int row = columnsTable_->currentRow();
    if (row >= 0) {
        loadColumnToDialog(row);
    }
}

void TableDesignerDialog::onDeleteColumn() {
    int row = columnsTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.columns.removeAt(row);
        updateColumnTable();
        updateButtonStates();
    }
}

void TableDesignerDialog::onMoveColumnUp() {
    int row = columnsTable_->currentRow();
    if (row > 0) {
        currentDefinition_.columns.move(row, row - 1);
        updateColumnTable();
        columnsTable_->setCurrentCell(row - 1, 0);
    }
}

void TableDesignerDialog::onMoveColumnDown() {
    int row = columnsTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.columns.size() - 1) {
        currentDefinition_.columns.move(row, row + 1);
        updateColumnTable();
        columnsTable_->setCurrentCell(row + 1, 0);
    }
}

void TableDesignerDialog::onColumnSelectionChanged() {
    updateButtonStates();
}

// Index management slots
void TableDesignerDialog::onAddIndex() {
    // TODO: Implement index dialog
    QMessageBox::information(this, "Add Index", "Index management will be implemented in the next update.");
}

void TableDesignerDialog::onEditIndex() {
    // TODO: Implement index dialog
    QMessageBox::information(this, "Edit Index", "Index management will be implemented in the next update.");
}

void TableDesignerDialog::onDeleteIndex() {
    int row = indexesTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.indexes.removeAt(row);
        updateIndexTable();
    }
}

// Constraint management slots
void TableDesignerDialog::onAddConstraint() {
    // TODO: Implement constraint dialog
    QMessageBox::information(this, "Add Constraint", "Constraint management will be implemented in the next update.");
}

void TableDesignerDialog::onEditConstraint() {
    // TODO: Implement constraint dialog
    QMessageBox::information(this, "Edit Constraint", "Constraint management will be implemented in the next update.");
}

void TableDesignerDialog::onDeleteConstraint() {
    int row = constraintsTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.constraints.removeAt(row);
        updateConstraintTable();
    }
}

// Foreign key management slots
void TableDesignerDialog::onAddForeignKey() {
    // TODO: Implement foreign key dialog
    QMessageBox::information(this, "Add Foreign Key", "Foreign key management will be implemented in the next update.");
}

void TableDesignerDialog::onEditForeignKey() {
    // TODO: Implement foreign key dialog
    QMessageBox::information(this, "Edit Foreign Key", "Foreign key management will be implemented in the next update.");
}

void TableDesignerDialog::onDeleteForeignKey() {
    int row = foreignKeysTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.foreignKeys.removeAt(row);
        updateForeignKeyTable();
    }
}

// Table properties slots
void TableDesignerDialog::onTableNameChanged(const QString& name) {
    // Validate table name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void TableDesignerDialog::onEngineChanged(int index) {
    // Update available charsets based on engine
    // This is MySQL specific
}

void TableDesignerDialog::onCharsetChanged(int index) {
    // Update available collations based on charset
    // This is MySQL specific
}

// Action slots
void TableDesignerDialog::onGenerateSQL() {
    if (validateTable()) {
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

void TableDesignerDialog::onPreviewSQL() {
    onGenerateSQL();
}

void TableDesignerDialog::onValidateTable() {
    if (validateTable()) {
        QMessageBox::information(this, "Validation", "Table definition is valid.");
    }
}

void TableDesignerDialog::onImportColumns() {
    // TODO: Implement column import
    QMessageBox::information(this, "Import Columns", "Column import will be implemented in the next update.");
}

void TableDesignerDialog::onExportColumns() {
    // TODO: Implement column export
    QMessageBox::information(this, "Export Columns", "Column export will be implemented in the next update.");
}

void TableDesignerDialog::updateColumnTable() {
    columnsTable_->setRowCount(currentDefinition_.columns.size());

    for (int i = 0; i < currentDefinition_.columns.size(); ++i) {
        const TableColumnDefinition& column = currentDefinition_.columns[i];

        columnsTable_->setItem(i, 0, new QTableWidgetItem(column.name));
        columnsTable_->setItem(i, 1, new QTableWidgetItem(column.dataType));
        columnsTable_->setItem(i, 2, new QTableWidgetItem(column.length > 0 ? QString::number(column.length) : ""));
        columnsTable_->setItem(i, 3, new QTableWidgetItem(column.isNullable ? "Yes" : "No"));
        columnsTable_->setItem(i, 4, new QTableWidgetItem(column.defaultValue));
        columnsTable_->setItem(i, 5, new QTableWidgetItem(column.isPrimaryKey ? "Yes" : "No"));
        columnsTable_->setItem(i, 6, new QTableWidgetItem(column.isUnique ? "Yes" : "No"));
        columnsTable_->setItem(i, 7, new QTableWidgetItem(column.isAutoIncrement ? "Yes" : "No"));
    }
}

void TableDesignerDialog::updateIndexTable() {
    indexesTable_->setRowCount(currentDefinition_.indexes.size());

    for (int i = 0; i < currentDefinition_.indexes.size(); ++i) {
        const TableIndexDefinition& index = currentDefinition_.indexes[i];

        indexesTable_->setItem(i, 0, new QTableWidgetItem(index.name));
        indexesTable_->setItem(i, 1, new QTableWidgetItem(index.type));
        indexesTable_->setItem(i, 2, new QTableWidgetItem(index.columns.join(", ")));
        indexesTable_->setItem(i, 3, new QTableWidgetItem(index.method));
    }
}

void TableDesignerDialog::updateConstraintTable() {
    constraintsTable_->setRowCount(currentDefinition_.constraints.size());

    for (int i = 0; i < currentDefinition_.constraints.size(); ++i) {
        const TableConstraintDefinition& constraint = currentDefinition_.constraints[i];

        constraintsTable_->setItem(i, 0, new QTableWidgetItem(constraint.name));
        constraintsTable_->setItem(i, 1, new QTableWidgetItem(constraint.type));
        constraintsTable_->setItem(i, 2, new QTableWidgetItem(constraint.expression));
        constraintsTable_->setItem(i, 3, new QTableWidgetItem(constraint.columns.join(", ")));
    }
}

void TableDesignerDialog::updateForeignKeyTable() {
    foreignKeysTable_->setRowCount(currentDefinition_.foreignKeys.size());

    for (int i = 0; i < currentDefinition_.foreignKeys.size(); ++i) {
        const ForeignKeyDefinition& fk = currentDefinition_.foreignKeys[i];

        foreignKeysTable_->setItem(i, 0, new QTableWidgetItem(fk.name));
        foreignKeysTable_->setItem(i, 1, new QTableWidgetItem(fk.column));
        foreignKeysTable_->setItem(i, 2, new QTableWidgetItem(fk.referencedTable));
        foreignKeysTable_->setItem(i, 3, new QTableWidgetItem(fk.referencedColumn));
        foreignKeysTable_->setItem(i, 4, new QTableWidgetItem(fk.onDelete));
        foreignKeysTable_->setItem(i, 5, new QTableWidgetItem(fk.onUpdate));
    }
}

bool TableDesignerDialog::validateTable() {
    QString tableName = tableNameEdit_->text().trimmed();

    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Table name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        tableNameEdit_->setFocus();
        return false;
    }

    if (currentDefinition_.columns.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "At least one column is required.");
        tabWidget_->setCurrentWidget(columnsTab_);
        return false;
    }

    // Check for duplicate column names
    QSet<QString> columnNames;
    for (const TableColumnDefinition& column : currentDefinition_.columns) {
        if (columnNames.contains(column.name)) {
            QMessageBox::warning(this, "Validation Error",
                QString("Duplicate column name: %1").arg(column.name));
            tabWidget_->setCurrentWidget(columnsTab_);
            return false;
        }
        columnNames.insert(column.name);
    }

    // Check for primary key
    bool hasPrimaryKey = false;
    for (const TableColumnDefinition& column : currentDefinition_.columns) {
        if (column.isPrimaryKey) {
            hasPrimaryKey = true;
            break;
        }
    }

    if (!hasPrimaryKey) {
        int result = QMessageBox::question(this, "No Primary Key",
            "The table does not have a primary key. Continue anyway?",
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::No) {
            tabWidget_->setCurrentWidget(columnsTab_);
            return false;
        }
    }

    return true;
}

QString TableDesignerDialog::generateCreateSQL() const {
    QStringList sqlParts;

    // Table name with schema
    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), fullTableName);
    }

    sqlParts.append(QString("CREATE TABLE %1 (").arg(fullTableName));

    // Columns
    QStringList columnDefinitions;
    for (const TableColumnDefinition& column : currentDefinition_.columns) {
        QStringList parts;
        parts.append(column.name);

        // Data type
        QString dataType = column.dataType;
        if (column.length > 0) {
            if (column.precision > 0) {
                dataType += QString("(%1,%2)").arg(column.length).arg(column.precision);
            } else {
                dataType += QString("(%1)").arg(column.length);
            }
        }
        parts.append(dataType);

        // Constraints
        if (!column.isNullable) {
            parts.append("NOT NULL");
        }

        if (!column.defaultValue.isEmpty()) {
            parts.append(QString("DEFAULT %1").arg(column.defaultValue));
        }

        if (column.isPrimaryKey) {
            parts.append("PRIMARY KEY");
        }

        if (column.isUnique) {
            parts.append("UNIQUE");
        }

        if (column.isAutoIncrement) {
            switch (currentDatabaseType_) {
                case DatabaseType::MYSQL:
                case DatabaseType::MARIADB:
                    parts.append("AUTO_INCREMENT");
                    break;
                case DatabaseType::POSTGRESQL:
                    parts.append("SERIAL");
                    break;
                default:
                    parts.append("AUTO_INCREMENT");
                    break;
            }
        }

        columnDefinitions.append(parts.join(" "));
    }

    sqlParts.append(columnDefinitions.join(",\n"));

    // Indexes
    for (const TableIndexDefinition& index : currentDefinition_.indexes) {
        if (index.type == "PRIMARY") {
            // Primary key already handled in columns
            continue;
        }

        QString indexSql;
        if (index.type == "UNIQUE") {
            indexSql = QString("UNIQUE INDEX %1 (%2)").arg(index.name, index.columns.join(", "));
        } else {
            indexSql = QString("INDEX %1 (%2)").arg(index.name, index.columns.join(", "));
        }

        if (!index.method.isEmpty()) {
            indexSql += QString(" USING %1").arg(index.method);
        }

        sqlParts.append(indexSql);
    }

    // Foreign keys
    for (const ForeignKeyDefinition& fk : currentDefinition_.foreignKeys) {
        QString fkSql = QString("FOREIGN KEY (%1) REFERENCES %2(%3)")
            .arg(fk.column, fk.referencedTable, fk.referencedColumn);

        if (!fk.onDelete.isEmpty() && fk.onDelete != "NO ACTION") {
            fkSql += QString(" ON DELETE %1").arg(fk.onDelete);
        }

        if (!fk.onUpdate.isEmpty() && fk.onUpdate != "NO ACTION") {
            fkSql += QString(" ON UPDATE %1").arg(fk.onUpdate);
        }

        if (!fk.name.isEmpty()) {
            fkSql = QString("CONSTRAINT %1 ").arg(fk.name) + fkSql;
        }

        sqlParts.append(fkSql);
    }

    // Close table definition
    sqlParts.append(")");

    // Table options
    QStringList options;

    if (!engineCombo_->currentData().toString().isEmpty()) {
        options.append(QString("ENGINE = %1").arg(engineCombo_->currentData().toString()));
    }

    if (!charsetCombo_->currentData().toString().isEmpty()) {
        options.append(QString("CHARSET = %1").arg(charsetCombo_->currentData().toString()));
    }

    if (!collationCombo_->currentData().toString().isEmpty()) {
        options.append(QString("COLLATE = %1").arg(collationCombo_->currentData().toString()));
    }

    if (!commentEdit_->toPlainText().isEmpty()) {
        options.append(QString("COMMENT = '%1'").arg(commentEdit_->toPlainText().replace("'", "''")));
    }

    if (!options.isEmpty()) {
        sqlParts.append(options.join(",\n"));
    }

    return sqlParts.join("\n");
}

QString TableDesignerDialog::generateAlterSQL() const {
    // For now, return a simple message
    // In a full implementation, this would generate ALTER TABLE statements
    return QString("-- ALTER TABLE statements would be generated here\n-- Original table: %1.%2")
        .arg(originalSchema_, originalTableName_);
}

QString TableDesignerDialog::generateDropSQL() const {
    QString tableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        tableName = QString("%1.%2").arg(schemaEdit_->text(), tableName);
    }

    return QString("DROP TABLE IF EXISTS %1;").arg(tableName);
}

void TableDesignerDialog::loadColumnToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.columns.size()) {
        return;
    }

    const TableColumnDefinition& column = currentDefinition_.columns[row];

    columnNameEdit_->setText(column.name);

    int typeIndex = dataTypeCombo_->findText(column.dataType);
    if (typeIndex >= 0) {
        dataTypeCombo_->setCurrentIndex(typeIndex);
    } else {
        dataTypeCombo_->setCurrentText(column.dataType);
    }

    lengthSpin_->setValue(column.length);
    precisionSpin_->setValue(column.precision);
    scaleSpin_->setValue(column.scale);
    nullableCheck_->setChecked(column.isNullable);
    defaultValueEdit_->setText(column.defaultValue);
    primaryKeyCheck_->setChecked(column.isPrimaryKey);
    uniqueCheck_->setChecked(column.isUnique);
    autoIncrementCheck_->setChecked(column.isAutoIncrement);
    columnCommentEdit_->setPlainText(column.comment);

    // Switch to columns tab to show the edit form
    tabWidget_->setCurrentWidget(columnsTab_);
}

void TableDesignerDialog::saveColumnFromDialog() {
    TableColumnDefinition column;

    column.name = columnNameEdit_->text().trimmed();
    column.dataType = dataTypeCombo_->currentText();
    column.length = lengthSpin_->value();
    column.precision = precisionSpin_->value();
    column.scale = scaleSpin_->value();
    column.isNullable = nullableCheck_->isChecked();
    column.defaultValue = defaultValueEdit_->text();
    column.isPrimaryKey = primaryKeyCheck_->isChecked();
    column.isUnique = uniqueCheck_->isChecked();
    column.isAutoIncrement = autoIncrementCheck_->isChecked();
    column.comment = columnCommentEdit_->toPlainText();

    // Validate column name
    if (column.name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Column name is required.");
        columnNameEdit_->setFocus();
        return;
    }

    // Check for duplicate column names
    for (int i = 0; i < currentDefinition_.columns.size(); ++i) {
        if (currentDefinition_.columns[i].name == column.name) {
            // If editing existing column, allow it
            if (columnsTable_->currentRow() != i) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Column name '%1' already exists.").arg(column.name));
                columnNameEdit_->setFocus();
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

void TableDesignerDialog::clearColumnDialog() {
    columnNameEdit_->clear();
    dataTypeCombo_->setCurrentIndex(0);
    lengthSpin_->setValue(0);
    precisionSpin_->setValue(0);
    scaleSpin_->setValue(0);
    nullableCheck_->setChecked(true);
    defaultValueEdit_->clear();
    primaryKeyCheck_->setChecked(false);
    uniqueCheck_->setChecked(false);
    autoIncrementCheck_->setChecked(false);
    columnCommentEdit_->clear();

    // Clear table selection
    columnsTable_->clearSelection();
}

void TableDesignerDialog::updateButtonStates() {
    bool hasSelection = columnsTable_->currentRow() >= 0;
    bool hasColumns = currentDefinition_.columns.size() > 0;

    editColumnButton_->setEnabled(hasSelection);
    deleteColumnButton_->setEnabled(hasSelection);
    moveUpButton_->setEnabled(hasSelection && columnsTable_->currentRow() > 0);
    moveDownButton_->setEnabled(hasSelection && columnsTable_->currentRow() < currentDefinition_.columns.size() - 1);

    // Index buttons
    bool hasIndexSelection = indexesTable_->currentRow() >= 0;
    editIndexButton_->setEnabled(hasIndexSelection);
    deleteIndexButton_->setEnabled(hasIndexSelection);

    // Constraint buttons
    bool hasConstraintSelection = constraintsTable_->currentRow() >= 0;
    editConstraintButton_->setEnabled(hasConstraintSelection);
    deleteConstraintButton_->setEnabled(hasConstraintSelection);

    // Foreign key buttons
    bool hasFKSelection = foreignKeysTable_->currentRow() >= 0;
    editForeignKeyButton_->setEnabled(hasFKSelection);
    deleteForeignKeyButton_->setEnabled(hasFKSelection);
}

} // namespace scratchrobin
