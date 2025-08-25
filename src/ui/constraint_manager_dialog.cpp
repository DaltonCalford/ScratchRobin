#include "constraint_manager_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QApplication>
#include <algorithm>

namespace scratchrobin {

ConstraintManagerDialog::ConstraintManagerDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , constraintNameEdit_(nullptr)
    , tableNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , constraintTypeCombo_(nullptr)
    , expressionEdit_(nullptr)
    , commentEdit_(nullptr)
    , columnsTab_(nullptr)
    , columnsLayout_(nullptr)
    , columnsTable_(nullptr)
    , columnsButtonLayout_(nullptr)
    , addColumnButton_(nullptr)
    , removeColumnButton_(nullptr)
    , availableColumnsList_(nullptr)
    , foreignKeyTab_(nullptr)
    , foreignKeyLayout_(nullptr)
    , referencedTableEdit_(nullptr)
    , referencedColumnEdit_(nullptr)
    , onDeleteCombo_(nullptr)
    , onUpdateCombo_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
    , enabledCheck_(nullptr)
    , sqlTab_(nullptr)
    , sqlLayout_(nullptr)
    , sqlPreviewEdit_(nullptr)
    , generateSqlButton_(nullptr)
    , validateSqlButton_(nullptr)
    , dialogButtons_(nullptr)
    , currentDatabaseType_(DatabaseType::POSTGRESQL)
    , isEditMode_(false)
    , driverManager_(&DatabaseDriverManager::instance())
{
    setupUI();
    setWindowTitle("Constraint Manager");
    setModal(true);
    resize(800, 600);
}

void ConstraintManagerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupColumnsTab();
    setupForeignKeyTab();
    setupAdvancedTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConstraintManagerDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void ConstraintManagerDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    constraintNameEdit_ = new QLineEdit(basicTab_);
    tableNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    constraintTypeCombo_ = new QComboBox(basicTab_);
    expressionEdit_ = new QTextEdit(basicTab_);
    expressionEdit_->setMaximumHeight(60);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Make table name and schema read-only (set via setTableInfo)
    tableNameEdit_->setReadOnly(true);
    schemaEdit_->setReadOnly(true);

    // Populate constraint types
    populateConstraintTypes();

    basicLayout_->addRow("Constraint Name:", constraintNameEdit_);
    basicLayout_->addRow("Table:", tableNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Type:", constraintTypeCombo_);
    basicLayout_->addRow("Expression:", expressionEdit_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(constraintNameEdit_, &QLineEdit::textChanged, this, &ConstraintManagerDialog::onConstraintNameChanged);
    connect(constraintTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConstraintManagerDialog::onConstraintTypeChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void ConstraintManagerDialog::setupColumnsTab() {
    columnsTab_ = new QWidget();
    columnsLayout_ = new QVBoxLayout(columnsTab_);

    // Column table
    setupColumnTable();
    columnsLayout_->addWidget(columnsTable_);

    // Column buttons
    columnsButtonLayout_ = new QHBoxLayout();
    addColumnButton_ = new QPushButton("Add Column", columnsTab_);
    removeColumnButton_ = new QPushButton("Remove Column", columnsTab_);

    columnsButtonLayout_->addWidget(addColumnButton_);
    columnsButtonLayout_->addWidget(removeColumnButton_);
    columnsButtonLayout_->addStretch();

    columnsLayout_->addLayout(columnsButtonLayout_);

    // Available columns list
    QLabel* availableLabel = new QLabel("Available Columns:", columnsTab_);
    columnsLayout_->addWidget(availableLabel);

    availableColumnsList_ = new QListWidget(columnsTab_);
    columnsLayout_->addWidget(availableColumnsList_);

    // Connect signals
    connect(addColumnButton_, &QPushButton::clicked, this, static_cast<void (ConstraintManagerDialog::*)()>(&ConstraintManagerDialog::onAddColumn));
    connect(removeColumnButton_, &QPushButton::clicked, this, &ConstraintManagerDialog::onRemoveColumn);
    connect(columnsTable_, &QTableWidget::itemSelectionChanged, this, static_cast<void (ConstraintManagerDialog::*)()>(&ConstraintManagerDialog::onColumnSelectionChanged));
    connect(availableColumnsList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            onAddColumn(item->text());
        }
    });

    tabWidget_->addTab(columnsTab_, "Columns");
}

void ConstraintManagerDialog::setupForeignKeyTab() {
    foreignKeyTab_ = new QWidget();
    foreignKeyLayout_ = new QFormLayout(foreignKeyTab_);

    referencedTableEdit_ = new QLineEdit(foreignKeyTab_);
    referencedColumnEdit_ = new QLineEdit(foreignKeyTab_);
    onDeleteCombo_ = new QComboBox(foreignKeyTab_);
    onUpdateCombo_ = new QComboBox(foreignKeyTab_);

    // Populate actions
    populateActions();

    foreignKeyLayout_->addRow("Referenced Table:", referencedTableEdit_);
    foreignKeyLayout_->addRow("Referenced Column:", referencedColumnEdit_);
    foreignKeyLayout_->addRow("ON DELETE:", onDeleteCombo_);
    foreignKeyLayout_->addRow("ON UPDATE:", onUpdateCombo_);

    foreignKeyLayout_->addRow("", new QLabel("Note: Foreign key constraints require exactly one column."));

    // Connect signals
    connect(referencedTableEdit_, &QLineEdit::textChanged, this, &ConstraintManagerDialog::onReferencedTableChanged);
    connect(onDeleteCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConstraintManagerDialog::onOnDeleteChanged);
    connect(onUpdateCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConstraintManagerDialog::onOnUpdateChanged);

    tabWidget_->addTab(foreignKeyTab_, "Foreign Key");
}

void ConstraintManagerDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Constraint Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    enabledCheck_ = new QCheckBox("Constraint is enabled", advancedTab_);
    enabledCheck_->setChecked(true);

    optionsLayout_->addRow("", enabledCheck_);

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void ConstraintManagerDialog::setupSQLTab() {
    sqlTab_ = new QWidget();
    sqlLayout_ = new QVBoxLayout(sqlTab_);

    sqlPreviewEdit_ = new QTextEdit(sqlTab_);
    sqlPreviewEdit_->setFontFamily("Monospace");
    sqlPreviewEdit_->setLineWrapMode(QTextEdit::NoWrap);

    generateSqlButton_ = new QPushButton("Generate SQL", sqlTab_);
    validateSqlButton_ = new QPushButton("Validate", sqlTab_);

    QHBoxLayout* sqlButtonLayout = new QHBoxLayout();
    sqlButtonLayout->addWidget(generateSqlButton_);
    sqlButtonLayout->addWidget(validateSqlButton_);
    sqlButtonLayout->addStretch();

    sqlLayout_->addWidget(sqlPreviewEdit_);
    sqlLayout_->addLayout(sqlButtonLayout);

    // Connect signals
    connect(generateSqlButton_, &QPushButton::clicked, this, &ConstraintManagerDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &ConstraintManagerDialog::onValidateConstraint);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void ConstraintManagerDialog::setupColumnTable() {
    columnsTable_ = new QTableWidget(columnsTab_);
    columnsTable_->setColumnCount(2);
    columnsTable_->setHorizontalHeaderLabels({"Column Name", "Position"});

    columnsTable_->horizontalHeader()->setStretchLastSection(true);
    columnsTable_->verticalHeader()->setDefaultSectionSize(25);
    columnsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    columnsTable_->setAlternatingRowColors(true);
}

void ConstraintManagerDialog::setConstraintDefinition(const ConstraintManagerDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    constraintNameEdit_->setText(definition.name);
    tableNameEdit_->setText(definition.tableName);
    schemaEdit_->setText(definition.schema);
    expressionEdit_->setPlainText(definition.expression);
    commentEdit_->setPlainText(definition.comment);
    enabledCheck_->setChecked(definition.isEnabled);

    // Set constraint type
    if (!definition.type.isEmpty()) {
        int index = constraintTypeCombo_->findData(definition.type);
        if (index >= 0) constraintTypeCombo_->setCurrentIndex(index);
    }

    // Set foreign key fields
    referencedTableEdit_->setText(definition.referencedTable);
    referencedColumnEdit_->setText(definition.referencedColumn);

    if (!definition.onDelete.isEmpty()) {
        int index = onDeleteCombo_->findData(definition.onDelete);
        if (index >= 0) onDeleteCombo_->setCurrentIndex(index);
    }

    if (!definition.onUpdate.isEmpty()) {
        int index = onUpdateCombo_->findData(definition.onUpdate);
        if (index >= 0) onUpdateCombo_->setCurrentIndex(index);
    }

    // Update columns table
    updateColumnTable();
}

ConstraintManagerDefinition ConstraintManagerDialog::getConstraintDefinition() const {
    ConstraintManagerDefinition definition = currentDefinition_;

    definition.name = constraintNameEdit_->text();
    definition.tableName = tableNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.type = constraintTypeCombo_->currentData().toString();
    definition.expression = expressionEdit_->toPlainText();
    definition.comment = commentEdit_->toPlainText();
    definition.isEnabled = enabledCheck_->isChecked();
    definition.referencedTable = referencedTableEdit_->text();
    definition.referencedColumn = referencedColumnEdit_->text();
    definition.onDelete = onDeleteCombo_->currentData().toString();
    definition.onUpdate = onUpdateCombo_->currentData().toString();

    return definition;
}

void ConstraintManagerDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Constraint");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Constraint");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void ConstraintManagerDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update available constraint types based on database
    populateConstraintTypes();
    populateActions();
}

void ConstraintManagerDialog::setTableInfo(const QString& schema, const QString& tableName) {
    schemaEdit_->setText(schema);
    tableNameEdit_->setText(tableName);
    currentDefinition_.schema = schema;
    currentDefinition_.tableName = tableName;

    // TODO: Load available columns from the table
    // For now, add some sample columns
    availableColumns_.clear();
    availableColumns_ << "id" << "name" << "email" << "created_date" << "status" << "category" << "parent_id" << "description";
    populateColumns();
}

void ConstraintManagerDialog::loadExistingConstraint(const QString& schema, const QString& tableName, const QString& constraintName) {
    setTableInfo(schema, tableName);
    constraintNameEdit_->setText(constraintName);
    originalConstraintName_ = constraintName;
    setEditMode(true);

    // TODO: Load actual constraint definition from database
}

void ConstraintManagerDialog::accept() {
    if (validateConstraint()) {
        emit constraintSaved(getConstraintDefinition());
        QDialog::accept();
    }
}

void ConstraintManagerDialog::reject() {
    QDialog::reject();
}

// Basic slots
void ConstraintManagerDialog::onConstraintNameChanged(const QString& name) {
    // Validate constraint name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void ConstraintManagerDialog::onConstraintTypeChanged(int index) {
    updateUIForConstraintType();
}

// Column management slots
void ConstraintManagerDialog::onAddColumn() {
    if (availableColumnsList_->currentItem()) {
        onAddColumn(availableColumnsList_->currentItem()->text());
    }
}

void ConstraintManagerDialog::onAddColumn(const QString& columnName) {
    // Check if column is already in the constraint
    for (const QString& existing : currentDefinition_.columns) {
        if (existing == columnName) {
            QMessageBox::warning(this, "Duplicate Column", "This column is already part of the constraint.");
            return;
        }
    }

    currentDefinition_.columns.append(columnName);
    updateColumnTable();
    updateButtonStates();
}

void ConstraintManagerDialog::onRemoveColumn() {
    int row = columnsTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.columns.removeAt(row);
        updateColumnTable();
        updateButtonStates();
    }
}

void ConstraintManagerDialog::onColumnSelectionChanged() {
    updateButtonStates();
}

// Foreign key slots
void ConstraintManagerDialog::onReferencedTableChanged(const QString& table) {
    // TODO: Load columns from the referenced table
}

void ConstraintManagerDialog::onReferencedColumnChanged(const QString& column) {
    // Handle column changes if needed
}

void ConstraintManagerDialog::onOnDeleteChanged(int index) {
    // Handle ON DELETE changes if needed
}

void ConstraintManagerDialog::onOnUpdateChanged(int index) {
    // Handle ON UPDATE changes if needed
}

// Action slots
void ConstraintManagerDialog::onGenerateSQL() {
    if (validateConstraint()) {
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

void ConstraintManagerDialog::onPreviewSQL() {
    onGenerateSQL();
}

void ConstraintManagerDialog::onValidateConstraint() {
    if (validateConstraint()) {
        QMessageBox::information(this, "Validation", "Constraint definition is valid.");
    }
}

void ConstraintManagerDialog::onAnalyzeConstraint() {
    // TODO: Implement constraint analysis
    QMessageBox::information(this, "Analyze Constraint", "Constraint analysis will be implemented in the next update.");
}

void ConstraintManagerDialog::updateColumnTable() {
    columnsTable_->setRowCount(currentDefinition_.columns.size());

    for (int i = 0; i < currentDefinition_.columns.size(); ++i) {
        columnsTable_->setItem(i, 0, new QTableWidgetItem(currentDefinition_.columns[i]));
        columnsTable_->setItem(i, 1, new QTableWidgetItem(QString::number(i + 1)));
    }
}

void ConstraintManagerDialog::populateConstraintTypes() {
    if (!constraintTypeCombo_) return;

    constraintTypeCombo_->clear();
    constraintTypeCombo_->addItem("PRIMARY KEY", "PRIMARY KEY");
    constraintTypeCombo_->addItem("FOREIGN KEY", "FOREIGN KEY");
    constraintTypeCombo_->addItem("UNIQUE", "UNIQUE");
    constraintTypeCombo_->addItem("CHECK", "CHECK");
    constraintTypeCombo_->addItem("NOT NULL", "NOT NULL");

    // Add database-specific constraint types
    switch (currentDatabaseType_) {
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            // MySQL-specific constraints
            break;
        case DatabaseType::POSTGRESQL:
            // PostgreSQL-specific constraints
            break;
        default:
            break;
    }
}

void ConstraintManagerDialog::populateColumns() {
    if (!availableColumnsList_) return;

    availableColumnsList_->clear();
    availableColumnsList_->addItems(availableColumns_);
}

void ConstraintManagerDialog::populateTables() {
    // TODO: Load available tables from database
    availableTables_.clear();
    availableTables_ << "users" << "products" << "orders" << "categories" << "customers";
}

void ConstraintManagerDialog::populateActions() {
    if (!onDeleteCombo_ || !onUpdateCombo_) return;

    QStringList actions;
    actions << "NO ACTION" << "RESTRICT" << "CASCADE" << "SET NULL" << "SET DEFAULT";

    onDeleteCombo_->clear();
    onUpdateCombo_->clear();

    for (const QString& action : actions) {
        onDeleteCombo_->addItem(action, action);
        onUpdateCombo_->addItem(action, action);
    }

    // Set defaults
    onDeleteCombo_->setCurrentIndex(0); // NO ACTION
    onUpdateCombo_->setCurrentIndex(0); // NO ACTION
}

bool ConstraintManagerDialog::validateConstraint() {
    QString constraintName = constraintNameEdit_->text().trimmed();
    QString constraintType = constraintTypeCombo_->currentData().toString();

    if (constraintName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Constraint name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        constraintNameEdit_->setFocus();
        return false;
    }

    QString tableName = tableNameEdit_->text().trimmed();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Table name is required.");
        return false;
    }

    // Check if columns are required for this constraint type
    if ((constraintType == "PRIMARY KEY" || constraintType == "UNIQUE" || constraintType == "FOREIGN KEY")
        && currentDefinition_.columns.isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
            QString("At least one column is required for %1 constraints.").arg(constraintType));
        tabWidget_->setCurrentWidget(columnsTab_);
        return false;
    }

    // Check foreign key specific validation
    if (constraintType == "FOREIGN KEY") {
        if (currentDefinition_.columns.size() != 1) {
            QMessageBox::warning(this, "Validation Error",
                "Foreign key constraints must have exactly one column.");
            tabWidget_->setCurrentWidget(columnsTab_);
            return false;
        }

        if (referencedTableEdit_->text().isEmpty()) {
            QMessageBox::warning(this, "Validation Error", "Referenced table is required for foreign key constraints.");
            tabWidget_->setCurrentWidget(foreignKeyTab_);
            referencedTableEdit_->setFocus();
            return false;
        }

        if (referencedColumnEdit_->text().isEmpty()) {
            QMessageBox::warning(this, "Validation Error", "Referenced column is required for foreign key constraints.");
            tabWidget_->setCurrentWidget(foreignKeyTab_);
            referencedColumnEdit_->setFocus();
            return false;
        }
    }

    // Check check constraint expression
    if (constraintType == "CHECK" && expressionEdit_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Check constraints require an expression.");
        tabWidget_->setCurrentWidget(basicTab_);
        expressionEdit_->setFocus();
        return false;
    }

    return true;
}

QString ConstraintManagerDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), fullTableName);
    }

    QString constraintType = constraintTypeCombo_->currentData().toString();
    QString constraintName = constraintNameEdit_->text();

    if (constraintType == "PRIMARY KEY") {
        sqlParts.append(QString("ALTER TABLE %1 ADD CONSTRAINT %2 PRIMARY KEY (%3)")
            .arg(fullTableName, constraintName, currentDefinition_.columns.join(", ")));
    }
    else if (constraintType == "FOREIGN KEY") {
        QString referencedTable = referencedTableEdit_->text();
        QString referencedColumn = referencedColumnEdit_->text();
        QString onDelete = onDeleteCombo_->currentData().toString();
        QString onUpdate = onUpdateCombo_->currentData().toString();

        QString fkSql = QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) REFERENCES %4(%5)")
            .arg(fullTableName, constraintName, currentDefinition_.columns.first(), referencedTable, referencedColumn);

        if (onDelete != "NO ACTION") {
            fkSql += QString(" ON DELETE %1").arg(onDelete);
        }

        if (onUpdate != "NO ACTION") {
            fkSql += QString(" ON UPDATE %1").arg(onUpdate);
        }

        sqlParts.append(fkSql);
    }
    else if (constraintType == "UNIQUE") {
        sqlParts.append(QString("ALTER TABLE %1 ADD CONSTRAINT %2 UNIQUE (%3)")
            .arg(fullTableName, constraintName, currentDefinition_.columns.join(", ")));
    }
    else if (constraintType == "CHECK") {
        QString expression = expressionEdit_->toPlainText().trimmed();
        sqlParts.append(QString("ALTER TABLE %1 ADD CONSTRAINT %2 CHECK (%3)")
            .arg(fullTableName, constraintName, expression));
    }
    else if (constraintType == "NOT NULL") {
        // NOT NULL is handled differently - it's a column constraint
        sqlParts.append(QString("ALTER TABLE %1 ALTER COLUMN %2 SET NOT NULL")
            .arg(fullTableName, currentDefinition_.columns.first()));
    }

    // Add comment if present
    QString comment = commentEdit_->toPlainText().trimmed();
    if (!comment.isEmpty() && constraintType != "NOT NULL") {
        sqlParts.append(QString("COMMENT ON CONSTRAINT %1 ON %2 IS '%3'")
            .arg(constraintName, fullTableName, comment.replace("'", "''")));
    }

    return sqlParts.join("\n");
}

QString ConstraintManagerDialog::generateDropSQL() const {
    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), tableNameEdit_->text());
    }

    QString constraintName = constraintNameEdit_->text();
    return QString("ALTER TABLE %1 DROP CONSTRAINT IF EXISTS %2;")
        .arg(fullTableName, constraintName);
}

QString ConstraintManagerDialog::generateAlterSQL() const {
    // For constraints, ALTER typically means DROP and CREATE
    QStringList sqlParts;
    sqlParts.append(generateDropSQL());
    sqlParts.append(generateCreateSQL() + ";");
    return sqlParts.join("\n");
}

void ConstraintManagerDialog::updateUIForConstraintType() {
    QString constraintType = constraintTypeCombo_->currentData().toString();

    // Show/hide tabs based on constraint type
    bool showColumns = (constraintType == "PRIMARY KEY" || constraintType == "UNIQUE" || constraintType == "FOREIGN KEY");
    bool showForeignKey = (constraintType == "FOREIGN KEY");
    bool showExpression = (constraintType == "CHECK");

    tabWidget_->setTabEnabled(tabWidget_->indexOf(columnsTab_), showColumns);
    tabWidget_->setTabEnabled(tabWidget_->indexOf(foreignKeyTab_), showForeignKey);

    // Update expression edit
    expressionEdit_->setEnabled(showExpression);
    if (!showExpression) {
        expressionEdit_->clear();
    }

    // Update columns if needed
    if (constraintType == "NOT NULL" && currentDefinition_.columns.size() > 1) {
        currentDefinition_.columns = QStringList(currentDefinition_.columns.first());
        updateColumnTable();
    }

    updateButtonStates();
}

void ConstraintManagerDialog::updateButtonStates() {
    bool hasSelection = columnsTable_->currentRow() >= 0;
    bool hasColumns = currentDefinition_.columns.size() > 0;

    removeColumnButton_->setEnabled(hasSelection);

    // Update available columns list
    availableColumnsList_->clear();
    for (const QString& column : availableColumns_) {
        if (!currentDefinition_.columns.contains(column)) {
            availableColumnsList_->addItem(column);
        }
    }
}

void ConstraintManagerDialog::onColumnSelectionChanged(int row) {
    // Handle column selection change with row parameter
    onColumnSelectionChanged();
}

} // namespace scratchrobin
