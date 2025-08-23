#include "table/table_designer.h"
#include "metadata/metadata_manager.h"
#include "execution/sql_executor.h"
#include "core/connection_manager.h"
#include "ui/object_browser/tree_model.h"
#include "ui/properties/property_viewer.h"
#include <QApplication>
#include <set>
#include <regex>
#include <QDialog>

// Qt meta-type declarations for custom types
Q_DECLARE_METATYPE(scratchrobin::TableStorageType)
Q_DECLARE_METATYPE(scratchrobin::ColumnType)
Q_DECLARE_METATYPE(scratchrobin::ConstraintDefinition)
Q_DECLARE_METATYPE(scratchrobin::IndexDefinition)
Q_DECLARE_METATYPE(scratchrobin::TreeNodeType)
Q_DECLARE_METATYPE(scratchrobin::SchemaObjectType)
Q_DECLARE_METATYPE(scratchrobin::PropertyDataType)
Q_DECLARE_METATYPE(scratchrobin::PropertyCategory)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QHeaderView>
#include <QStandardItemModel>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace scratchrobin {

// Helper functions
std::string columnTypeToString(ColumnType type) {
    switch (type) {
        case ColumnType::TEXT: return "TEXT";
        case ColumnType::INTEGER: return "INTEGER";
        case ColumnType::BIGINT: return "BIGINT";
        case ColumnType::FLOAT: return "FLOAT";
        case ColumnType::DOUBLE: return "DOUBLE";
        case ColumnType::DECIMAL: return "DECIMAL";
        case ColumnType::BOOLEAN: return "BOOLEAN";
        case ColumnType::DATE: return "DATE";
        case ColumnType::TIME: return "TIME";
        case ColumnType::DATETIME: return "DATETIME";
        case ColumnType::TIMESTAMP: return "TIMESTAMP";
        case ColumnType::BLOB: return "BLOB";
        case ColumnType::CLOB: return "CLOB";
        case ColumnType::BINARY: return "BINARY";
        case ColumnType::UUID: return "UUID";
        case ColumnType::JSON: return "JSON";
        case ColumnType::XML: return "XML";
        case ColumnType::ARRAY: return "ARRAY";
        default: return "TEXT";
    }
}

ColumnType stringToColumnType(const std::string& str) {
    if (str == "INTEGER") return ColumnType::INTEGER;
    if (str == "BIGINT") return ColumnType::BIGINT;
    if (str == "FLOAT") return ColumnType::FLOAT;
    if (str == "DOUBLE") return ColumnType::DOUBLE;
    if (str == "DECIMAL") return ColumnType::DECIMAL;
    if (str == "BOOLEAN") return ColumnType::BOOLEAN;
    if (str == "DATE") return ColumnType::DATE;
    if (str == "TIME") return ColumnType::TIME;
    if (str == "DATETIME") return ColumnType::DATETIME;
    if (str == "TIMESTAMP") return ColumnType::TIMESTAMP;
    if (str == "BLOB") return ColumnType::BLOB;
    if (str == "CLOB") return ColumnType::CLOB;
    if (str == "BINARY") return ColumnType::BINARY;
    if (str == "UUID") return ColumnType::UUID;
    if (str == "JSON") return ColumnType::JSON;
    if (str == "XML") return ColumnType::XML;
    if (str == "ARRAY") return ColumnType::ARRAY;
    return ColumnType::TEXT;
}

std::string constraintTypeToString(ConstraintType type) {
    switch (type) {
        case ConstraintType::PRIMARY_KEY: return "PRIMARY KEY";
        case ConstraintType::FOREIGN_KEY: return "FOREIGN KEY";
        case ConstraintType::UNIQUE: return "UNIQUE";
        case ConstraintType::CHECK: return "CHECK";
        case ConstraintType::NOT_NULL: return "NOT NULL";
        case ConstraintType::DEFAULT: return "DEFAULT";
        case ConstraintType::EXCLUDE: return "EXCLUDE";
        case ConstraintType::DOMAIN: return "DOMAIN";
        default: return "UNKNOWN";
    }
}

std::string indexTypeToString(IndexType type) {
    switch (type) {
        case IndexType::BTREE: return "BTREE";
        case IndexType::HASH: return "HASH";
        case IndexType::GIN: return "GIN";
        case IndexType::GIST: return "GIST";
        case IndexType::SPGIST: return "SPGIST";
        case IndexType::BRIN: return "BRIN";
        case IndexType::UNIQUE: return "UNIQUE";
        case IndexType::PARTIAL: return "PARTIAL";
        default: return "BTREE";
    }
}

std::string storageTypeToString(TableStorageType type) {
    switch (type) {
        case TableStorageType::REGULAR: return "REGULAR";
        case TableStorageType::TEMPORARY: return "TEMPORARY";
        case TableStorageType::UNLOGGED: return "UNLOGGED";
        case TableStorageType::INHERITED: return "INHERITED";
        case TableStorageType::PARTITIONED: return "PARTITIONED";
        default: return "REGULAR";
    }
}

class TableDesigner::Impl {
public:
    Impl(TableDesigner* parent) : parent_(parent) {}

    void setupUI() {
        dialog_ = new QDialog();
        dialog_->setWindowTitle("Table Designer");
        dialog_->setMinimumSize(800, 600);
        dialog_->resize(1200, 800);

        mainWidget_ = new QWidget(dialog_);
        mainLayout_ = new QVBoxLayout(mainWidget_);

        setupTableProperties();
        setupTabWidget();
        setupButtons();

        mainLayout_->addWidget(tableGroup_);
        mainLayout_->addWidget(tabWidget_);
        mainLayout_->addLayout(buttonLayout_);

        dialog_->setLayout(new QVBoxLayout());
        dialog_->layout()->addWidget(mainWidget_);
    }

    void setupTableProperties() {
        tableGroup_ = new QGroupBox("Table Properties");
        tableLayout_ = new QFormLayout(tableGroup_);

        tableNameEdit_ = new QLineEdit();
        tableNameEdit_->setPlaceholderText("Enter table name");
        connect(tableNameEdit_, &QLineEdit::textChanged, parent_, &TableDesigner::onTableNameChanged);

        schemaCombo_ = new QComboBox();
        storageTypeCombo_ = new QComboBox();
        tablespaceEdit_ = new QLineEdit();
        commentEdit_ = new QTextEdit();
        commentEdit_->setMaximumHeight(60);

        tableLayout_->addRow("Table Name:", tableNameEdit_);
        tableLayout_->addRow("Schema:", schemaCombo_);
        tableLayout_->addRow("Storage Type:", storageTypeCombo_);
        tableLayout_->addRow("Tablespace:", tablespaceEdit_);
        tableLayout_->addRow("Comment:", commentEdit_);
    }

    void setupTabWidget() {
        tabWidget_ = new QTabWidget();

        setupColumnsTab();
        setupConstraintsTab();
        setupIndexesTab();
        setupPreviewTab();

        tabWidget_->addTab(columnsTab_, "Columns");
        tabWidget_->addTab(constraintsTab_, "Constraints");
        tabWidget_->addTab(indexesTab_, "Indexes");
        tabWidget_->addTab(previewTab_, "Preview");
    }

    void setupColumnsTab() {
        columnsTab_ = new QWidget();
        columnsLayout_ = new QVBoxLayout(columnsTab_);

        // Columns table
        columnsTable_ = new QTableWidget();
        columnsTable_->setColumnCount(8);
        columnsTable_->setHorizontalHeaderLabels({
            "Name", "Type", "Length", "Nullable", "Default", "PK", "Auto Inc", "Comment"
        });
        columnsTable_->horizontalHeader()->setStretchLastSection(true);
        columnsTable_->verticalHeader()->setVisible(true);
        columnsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(columnsTable_, &QTableWidget::itemSelectionChanged, parent_, &TableDesigner::onColumnSelectionChanged);

        // Buttons
        columnButtonsLayout_ = new QHBoxLayout();
        addColumnButton_ = new QPushButton("Add Column");
        removeColumnButton_ = new QPushButton("Remove Column");
        moveUpButton_ = new QPushButton("Move Up");
        moveDownButton_ = new QPushButton("Move Down");

        connect(addColumnButton_, &QPushButton::clicked, parent_, &TableDesigner::onAddColumnClicked);
        connect(removeColumnButton_, &QPushButton::clicked, parent_, &TableDesigner::onRemoveColumnClicked);
        connect(moveUpButton_, &QPushButton::clicked, parent_, &TableDesigner::onMoveColumnUpClicked);
        connect(moveDownButton_, &QPushButton::clicked, parent_, &TableDesigner::onMoveColumnDownClicked);

        columnButtonsLayout_->addWidget(addColumnButton_);
        columnButtonsLayout_->addWidget(removeColumnButton_);
        columnButtonsLayout_->addWidget(moveUpButton_);
        columnButtonsLayout_->addWidget(moveDownButton_);
        columnButtonsLayout_->addStretch();

        columnsLayout_->addWidget(columnsTable_);
        columnsLayout_->addLayout(columnButtonsLayout_);
    }

    void setupConstraintsTab() {
        constraintsTab_ = new QWidget();
        constraintsLayout_ = new QVBoxLayout(constraintsTab_);

        constraintsList_ = new QListWidget();
        connect(constraintsList_, &QListWidget::itemSelectionChanged, parent_, &TableDesigner::onConstraintSelectionChanged);

        constraintButtonsLayout_ = new QHBoxLayout();
        addConstraintButton_ = new QPushButton("Add Constraint");
        removeConstraintButton_ = new QPushButton("Remove Constraint");

        connect(addConstraintButton_, &QPushButton::clicked, parent_, &TableDesigner::onAddConstraintClicked);
        connect(removeConstraintButton_, &QPushButton::clicked, parent_, &TableDesigner::onRemoveConstraintClicked);

        constraintButtonsLayout_->addWidget(addConstraintButton_);
        constraintButtonsLayout_->addWidget(removeConstraintButton_);
        constraintButtonsLayout_->addStretch();

        constraintsLayout_->addWidget(constraintsList_);
        constraintsLayout_->addLayout(constraintButtonsLayout_);
    }

    void setupIndexesTab() {
        indexesTab_ = new QWidget();
        indexesLayout_ = new QVBoxLayout(indexesTab_);

        indexesList_ = new QListWidget();
        connect(indexesList_, &QListWidget::itemSelectionChanged, parent_, &TableDesigner::onIndexSelectionChanged);

        indexButtonsLayout_ = new QHBoxLayout();
        addIndexButton_ = new QPushButton("Add Index");
        removeIndexButton_ = new QPushButton("Remove Index");

        connect(addIndexButton_, &QPushButton::clicked, parent_, &TableDesigner::onAddIndexClicked);
        connect(removeIndexButton_, &QPushButton::clicked, parent_, &TableDesigner::onRemoveIndexClicked);

        indexButtonsLayout_->addWidget(addIndexButton_);
        indexButtonsLayout_->addWidget(removeIndexButton_);
        indexButtonsLayout_->addStretch();

        indexesLayout_->addWidget(indexesList_);
        indexesLayout_->addLayout(indexButtonsLayout_);
    }

    void setupPreviewTab() {
        previewTab_ = new QWidget();
        previewLayout_ = new QVBoxLayout(previewTab_);

        ddlPreview_ = new QTextEdit();
        ddlPreview_->setFont(QFont("Monaco", 10));
        ddlPreview_->setReadOnly(true);

        refreshPreviewButton_ = new QPushButton("Refresh Preview");
        connect(refreshPreviewButton_, &QPushButton::clicked, parent_, &TableDesigner::onPreviewClicked);

        previewLayout_->addWidget(ddlPreview_);
        previewLayout_->addWidget(refreshPreviewButton_);
    }

    void setupButtons() {
        buttonLayout_ = new QHBoxLayout();

        validateButton_ = new QPushButton("Validate");
        createButton_ = new QPushButton("Create Table");
        cancelButton_ = new QPushButton("Cancel");

        connect(validateButton_, &QPushButton::clicked, parent_, &TableDesigner::onValidateClicked);
        connect(createButton_, &QPushButton::clicked, parent_, &TableDesigner::onCreateClicked);
        connect(cancelButton_, &QPushButton::clicked, parent_, &TableDesigner::onCancelClicked);

        buttonLayout_->addWidget(validateButton_);
        buttonLayout_->addStretch();
        buttonLayout_->addWidget(createButton_);
        buttonLayout_->addWidget(cancelButton_);
    }

    void populateSchemas() {
        if (!metadataManager_) return;

        schemaCombo_->clear();
        // Add default schema
        schemaCombo_->addItem("public");

        // Get schemas from metadata manager
        // This would be populated from the actual metadata
    }

    void populateDataTypes() {
        // Clear existing columns and add new ones
        columnsTable_->setRowCount(0);

        // Add a default column
        addColumnToTable(ColumnDefinition());
    }

    void populateStorageTypes() {
        storageTypeCombo_->clear();
        storageTypeCombo_->addItem("Regular", QVariant::fromValue(TableStorageType::REGULAR));
        storageTypeCombo_->addItem("Temporary", QVariant::fromValue(TableStorageType::TEMPORARY));
        storageTypeCombo_->addItem("Unlogged", QVariant::fromValue(TableStorageType::UNLOGGED));
        storageTypeCombo_->addItem("Inherited", QVariant::fromValue(TableStorageType::INHERITED));
        storageTypeCombo_->addItem("Partitioned", QVariant::fromValue(TableStorageType::PARTITIONED));
    }

    void addColumnToTable(const ColumnDefinition& column) {
        int row = columnsTable_->rowCount();
        columnsTable_->insertRow(row);

        // Name
        auto nameItem = new QTableWidgetItem(QString::fromStdString(column.name.empty() ? "column_" + std::to_string(row + 1) : column.name));
        columnsTable_->setItem(row, 0, nameItem);

        // Type
        auto typeCombo = new QComboBox();
        typeCombo->addItem("TEXT", QVariant::fromValue(ColumnType::TEXT));
        typeCombo->addItem("INTEGER", QVariant::fromValue(ColumnType::INTEGER));
        typeCombo->addItem("BIGINT", QVariant::fromValue(ColumnType::BIGINT));
        typeCombo->addItem("FLOAT", QVariant::fromValue(ColumnType::FLOAT));
        typeCombo->addItem("DOUBLE", QVariant::fromValue(ColumnType::DOUBLE));
        typeCombo->addItem("DECIMAL", QVariant::fromValue(ColumnType::DECIMAL));
        typeCombo->addItem("BOOLEAN", QVariant::fromValue(ColumnType::BOOLEAN));
        typeCombo->addItem("DATE", QVariant::fromValue(ColumnType::DATE));
        typeCombo->addItem("TIME", QVariant::fromValue(ColumnType::TIME));
        typeCombo->addItem("DATETIME", QVariant::fromValue(ColumnType::DATETIME));
        typeCombo->addItem("TIMESTAMP", QVariant::fromValue(ColumnType::TIMESTAMP));
        typeCombo->addItem("BLOB", QVariant::fromValue(ColumnType::BLOB));
        typeCombo->addItem("CLOB", QVariant::fromValue(ColumnType::CLOB));
        typeCombo->addItem("JSON", QVariant::fromValue(ColumnType::JSON));
        typeCombo->addItem("XML", QVariant::fromValue(ColumnType::XML));

        int typeIndex = typeCombo->findData(QVariant::fromValue(column.type));
        if (typeIndex >= 0) {
            typeCombo->setCurrentIndex(typeIndex);
        }

        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), parent_, &TableDesigner::onColumnTypeChanged);
        columnsTable_->setCellWidget(row, 1, typeCombo);

        // Length
        auto lengthSpin = new QSpinBox();
        lengthSpin->setRange(0, 10000);
        lengthSpin->setValue(column.length);
        columnsTable_->setCellWidget(row, 2, lengthSpin);

        // Nullable
        auto nullableCheck = new QCheckBox();
        nullableCheck->setChecked(column.isNullable);
        columnsTable_->setCellWidget(row, 3, nullableCheck);

        // Default
        auto defaultEdit = new QLineEdit(QString::fromStdString(column.defaultValue));
        columnsTable_->setCellWidget(row, 4, defaultEdit);

        // Primary Key
        auto pkCheck = new QCheckBox();
        pkCheck->setChecked(column.isPrimaryKey);
        columnsTable_->setCellWidget(row, 5, pkCheck);

        // Auto Increment
        auto autoIncCheck = new QCheckBox();
        autoIncCheck->setChecked(column.isAutoIncrement);
        columnsTable_->setCellWidget(row, 6, autoIncCheck);

        // Comment
        auto commentEdit = new QLineEdit(QString::fromStdString(column.comment));
        columnsTable_->setCellWidget(row, 7, commentEdit);
    }

    void addConstraintToList(const ConstraintDefinition& constraint) {
        QString constraintText = QString("%1 (%2)")
            .arg(QString::fromStdString(constraintTypeToString(constraint.type)))
            .arg(QString::fromStdString(constraint.name.empty() ? "constraint_" + std::to_string(constraintsList_->count() + 1) : constraint.name));

        QListWidgetItem* item = new QListWidgetItem(constraintText);
        item->setData(Qt::UserRole, QVariant::fromValue(constraint));
        constraintsList_->addItem(item);
    }

    void addIndexToList(const IndexDefinition& index) {
        QString indexText = QString("%1 (%2)")
            .arg(QString::fromStdString(indexTypeToString(index.type)))
            .arg(QString::fromStdString(index.name.empty() ? "index_" + std::to_string(indexesList_->count() + 1) : index.name));

        QListWidgetItem* item = new QListWidgetItem(indexText);
        item->setData(Qt::UserRole, QVariant::fromValue(index));
        indexesList_->addItem(item);
    }

    ColumnDefinition getColumnFromTable(int row) const {
        ColumnDefinition column;

        if (row < 0 || row >= columnsTable_->rowCount()) {
            return column;
        }

        // Name
        auto nameItem = columnsTable_->item(row, 0);
        if (nameItem) {
            column.name = nameItem->text().toStdString();
        }

        // Type
        auto typeCombo = qobject_cast<QComboBox*>(columnsTable_->cellWidget(row, 1));
        if (typeCombo) {
            column.type = typeCombo->currentData().value<ColumnType>();
        }

        // Length
        auto lengthSpin = qobject_cast<QSpinBox*>(columnsTable_->cellWidget(row, 2));
        if (lengthSpin) {
            column.length = lengthSpin->value();
        }

        // Nullable
        auto nullableCheck = qobject_cast<QCheckBox*>(columnsTable_->cellWidget(row, 3));
        if (nullableCheck) {
            column.isNullable = nullableCheck->isChecked();
        }

        // Default
        auto defaultEdit = qobject_cast<QLineEdit*>(columnsTable_->cellWidget(row, 4));
        if (defaultEdit) {
            column.defaultValue = defaultEdit->text().toStdString();
        }

        // Primary Key
        auto pkCheck = qobject_cast<QCheckBox*>(columnsTable_->cellWidget(row, 5));
        if (pkCheck) {
            column.isPrimaryKey = pkCheck->isChecked();
        }

        // Auto Increment
        auto autoIncCheck = qobject_cast<QCheckBox*>(columnsTable_->cellWidget(row, 6));
        if (autoIncCheck) {
            column.isAutoIncrement = autoIncCheck->isChecked();
        }

        // Comment
        auto commentEdit = qobject_cast<QLineEdit*>(columnsTable_->cellWidget(row, 7));
        if (commentEdit) {
            column.comment = commentEdit->text().toStdString();
        }

        return column;
    }

    TableDefinition getCurrentDefinition() const {
        TableDefinition definition;

        definition.name = tableNameEdit_->text().toStdString();
        definition.schema = schemaCombo_->currentText().toStdString();
        definition.database = currentConnectionId_;
        definition.storageType = storageTypeCombo_->currentData().value<TableStorageType>();
        definition.tablespace = tablespaceEdit_->text().toStdString();
        definition.comment = commentEdit_->toPlainText().toStdString();

        // Get columns
        for (int i = 0; i < columnsTable_->rowCount(); ++i) {
            definition.columns.push_back(getColumnFromTable(i));
        }

        // Get constraints
        for (int i = 0; i < constraintsList_->count(); ++i) {
            auto item = constraintsList_->item(i);
            if (item) {
                auto constraint = item->data(Qt::UserRole).value<ConstraintDefinition>();
                definition.constraints.push_back(constraint);
            }
        }

        // Get indexes
        for (int i = 0; i < indexesList_->count(); ++i) {
            auto item = indexesList_->item(i);
            if (item) {
                auto index = item->data(Qt::UserRole).value<IndexDefinition>();
                definition.indexes.push_back(index);
            }
        }

        return definition;
    }

    void updateDDLPreview() {
        auto definition = getCurrentDefinition();
        std::string ddl = generateDDL(definition, currentConnectionId_);
        ddlPreview_->setPlainText(QString::fromStdString(ddl));
    }

    std::string generateDDL(const TableDefinition& definition, const std::string& connectionId) {
        std::stringstream ddl;

        // CREATE TABLE statement
        ddl << "CREATE TABLE ";
        if (!definition.schema.empty() && definition.schema != "public") {
            ddl << definition.schema << ".";
        }
        ddl << definition.name << " (\n";

        // Columns
        for (size_t i = 0; i < definition.columns.size(); ++i) {
            const auto& column = definition.columns[i];
            ddl << "  " << column.name << " " << columnTypeToString(column.type);

            if (column.length > 0) {
                ddl << "(" << column.length << ")";
            }

            if (!column.isNullable) {
                ddl << " NOT NULL";
            }

            if (!column.defaultValue.empty()) {
                ddl << " DEFAULT " << column.defaultValue;
            }

            if (column.isAutoIncrement) {
                ddl << " AUTO_INCREMENT";
            }

            if (!column.comment.empty()) {
                ddl << " COMMENT '" << column.comment << "'";
            }

            if (i < definition.columns.size() - 1) {
                ddl << ",";
            }
            ddl << "\n";
        }

        // Primary key constraint
        std::vector<std::string> pkColumns;
        for (const auto& column : definition.columns) {
            if (column.isPrimaryKey) {
                pkColumns.push_back(column.name);
            }
        }

        if (!pkColumns.empty()) {
            ddl << "  PRIMARY KEY (" << join(pkColumns, ", ") << "),\n";
        }

        // Other constraints
        for (size_t i = 0; i < definition.constraints.size(); ++i) {
            const auto& constraint = definition.constraints[i];
            ddl << "  CONSTRAINT " << constraint.name << " " << constraintTypeToString(constraint.type);

            // Note: ConstraintDefinition uses constraintData variant instead of direct column access
            // This would need to be implemented based on the specific constraint type
            // For now, using the constraint definition string
            if (!constraint.definition.empty()) {
                ddl << " " << constraint.definition;
            }

            // Note: CHECK constraint expression would be in constraintData variant
            // For now, using the constraint definition string

            if (i < definition.constraints.size() - 1) {
                ddl << ",";
            }
            ddl << "\n";
        }

        ddl << ")";

        // Table options
        if (!definition.tablespace.empty()) {
            ddl << " TABLESPACE " << definition.tablespace;
        }

        if (!definition.comment.empty()) {
            ddl << " COMMENT = '" << definition.comment << "'";
        }

        ddl << ";";

        // Indexes
        for (const auto& index : definition.indexes) {
            ddl << "\n\nCREATE ";
            if (index.isUnique) {
                ddl << "UNIQUE ";
            }
            ddl << "INDEX " << index.name << " ON ";
            if (!definition.schema.empty() && definition.schema != "public") {
                ddl << definition.schema << ".";
            }
            ddl << definition.name << " ";

            if (index.type != IndexType::BTREE) {
                ddl << "USING " << indexTypeToString(index.type) << " ";
            }

            // Note: index.columns is std::vector<IndexColumn>, need to extract column names
            // For now, using a simple approach - this would need proper IndexColumn handling
            std::vector<std::string> columnNames;
            for (const auto& col : index.columns) {
                columnNames.push_back(col.columnName); // Assuming IndexColumn has columnName member
            }
            ddl << "(" << join(columnNames, ", ") << ")";

            if (!index.whereClause.empty()) {
                ddl << " WHERE " << index.whereClause;
            }

            if (!index.tablespace.empty()) {
                ddl << " TABLESPACE " << index.tablespace;
            }

            ddl << ";";
        }

        return ddl.str();
    }

    std::vector<std::string> validateTable(const TableDefinition& definition) {
        std::vector<std::string> errors;

        // Validate table name
        if (definition.name.empty()) {
            errors.push_back("Table name is required");
        } else if (!std::regex_match(definition.name, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"))) {
            errors.push_back("Table name contains invalid characters");
        }

        // Validate columns
        if (definition.columns.empty()) {
            errors.push_back("Table must have at least one column");
        }

        std::set<std::string> columnNames;
        bool hasPrimaryKey = false;

        for (const auto& column : definition.columns) {
            if (column.name.empty()) {
                errors.push_back("Column name is required");
            } else if (columnNames.count(column.name)) {
                errors.push_back("Duplicate column name: " + column.name);
            } else {
                columnNames.insert(column.name);
            }

            if (column.isPrimaryKey) {
                hasPrimaryKey = true;
            }

            // Validate column type specific requirements
            if (column.type == ColumnType::DECIMAL && (column.precision == 0 || column.scale == 0)) {
                errors.push_back("DECIMAL type requires precision and scale for column: " + column.name);
            }
        }

        if (!hasPrimaryKey && options_.autoGenerateConstraints) {
            // This is a warning, not an error
            qDebug() << "Warning: Table does not have a primary key";
        }

        // Validate constraints
        std::set<std::string> constraintNames;
        for (const auto& constraint : definition.constraints) {
            if (constraint.name.empty()) {
                errors.push_back("Constraint name is required");
            } else if (constraintNames.count(constraint.name)) {
                errors.push_back("Duplicate constraint name: " + constraint.name);
            } else {
                constraintNames.insert(constraint.name);
            }

            // Note: Constraint validation would need to access constraintData variant
            // For now, basic validation based on constraint definition
            if (constraint.definition.empty()) {
                errors.push_back("Constraint '" + constraint.name + "' must have a definition");
            }

            // Validate foreign key constraints - simplified
            if (constraint.type == ConstraintType::FOREIGN_KEY) {
                // Would need to check ForeignKeyConstraint data in constraintData variant
                // errors.push_back("Foreign key constraint '" + constraint.name + "' validation needed");
            }

            // Validate check constraints - simplified
            if (constraint.type == ConstraintType::CHECK) {
                // Would need to check CheckConstraint data in constraintData variant
                // errors.push_back("Check constraint '" + constraint.name + "' validation needed");
            }
        }

        // Validate indexes
        std::set<std::string> indexNames;
        for (const auto& index : definition.indexes) {
            if (index.name.empty()) {
                errors.push_back("Index name is required");
            } else if (indexNames.count(index.name)) {
                errors.push_back("Duplicate index name: " + index.name);
            } else {
                indexNames.insert(index.name);
            }

            if (index.columns.empty()) {
                errors.push_back("Index '" + index.name + "' must have at least one column");
            }
        }

        return errors;
    }

    std::string join(const std::vector<std::string>& strings, const std::string& delimiter) {
        if (strings.empty()) return "";
        std::string result = strings[0];
        for (size_t i = 1; i < strings.size(); ++i) {
            result += delimiter + strings[i];
        }
        return result;
    }

public:
    TableDesigner* parent_;
    TableDesignOptions options_;

    // UI Components
    QDialog* dialog_;
    QWidget* mainWidget_;
    QVBoxLayout* mainLayout_;
    QHBoxLayout* buttonLayout_;

    QGroupBox* tableGroup_;
    QFormLayout* tableLayout_;
    QLineEdit* tableNameEdit_;
    QComboBox* schemaCombo_;
    QComboBox* storageTypeCombo_;
    QLineEdit* tablespaceEdit_;
    QTextEdit* commentEdit_;

    QTabWidget* tabWidget_;
    QWidget* columnsTab_;
    QVBoxLayout* columnsLayout_;
    QTableWidget* columnsTable_;
    QHBoxLayout* columnButtonsLayout_;
    QPushButton* addColumnButton_;
    QPushButton* removeColumnButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;

    QWidget* constraintsTab_;
    QVBoxLayout* constraintsLayout_;
    QListWidget* constraintsList_;
    QHBoxLayout* constraintButtonsLayout_;
    QPushButton* addConstraintButton_;
    QPushButton* removeConstraintButton_;

    QWidget* indexesTab_;
    QVBoxLayout* indexesLayout_;
    QListWidget* indexesList_;
    QHBoxLayout* indexButtonsLayout_;
    QPushButton* addIndexButton_;
    QPushButton* removeIndexButton_;

    QWidget* previewTab_;
    QVBoxLayout* previewLayout_;
    QTextEdit* ddlPreview_;
    QPushButton* refreshPreviewButton_;

    QPushButton* validateButton_;
    QPushButton* createButton_;
    QPushButton* cancelButton_;

    // State
    TableDefinition currentDefinition_;
    std::string currentConnectionId_;
    bool isModifyMode_ = false;
    std::string originalTableName_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;
};

// TableDesigner implementation

TableDesigner::TableDesigner(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this)) {

    // Set default options
    options_.enableAutoNaming = true;
    options_.enableDragAndDrop = true;
    options_.enableContextMenus = true;
    options_.enableValidation = true;
    options_.enablePreview = true;
    options_.enableTemplates = true;
    options_.enableImport = true;
    options_.enableExport = true;
    options_.showAdvancedOptions = false;
    options_.autoGenerateConstraints = true;
    options_.autoGenerateIndexes = true;
    options_.maxColumns = 100;
    options_.maxIndexes = 10;
    options_.maxConstraints = 20;
}

TableDesigner::~TableDesigner() = default;

void TableDesigner::initialize(const TableDesignOptions& options) {
    options_ = options;
    impl_->setupUI();
    impl_->populateSchemas();
    impl_->populateDataTypes();
    impl_->populateStorageTypes();
}

void TableDesigner::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
    impl_->metadataManager_ = metadataManager;
}

void TableDesigner::setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) {
    sqlExecutor_ = sqlExecutor;
    impl_->sqlExecutor_ = sqlExecutor;
}

void TableDesigner::setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) {
    connectionManager_ = connectionManager;
    impl_->connectionManager_ = connectionManager;
}

bool TableDesigner::createTable(const TableDefinition& definition, const std::string& connectionId) {
    if (!sqlExecutor_) {
        return false;
    }

    try {
        // Generate DDL
        std::string ddl = generateDDL(definition, connectionId);

        // Execute DDL
        QueryExecutionContext context;
        context.connectionId = connectionId;
        context.databaseName = definition.database;

        QueryResult result = sqlExecutor_->executeQuery(ddl, context);

        if (result.success) {
            emit tableCreatedCallback_(definition.name, connectionId);
            return true;
        } else {
            std::vector<std::string> errors = {result.errorMessage};
            emit validationErrorCallback_(errors);
            return false;
        }

    } catch (const std::exception& e) {
        std::vector<std::string> errors = {std::string("Failed to create table: ") + e.what()};
        emit validationErrorCallback_(errors);
        return false;
    }
}

bool TableDesigner::modifyTable(const std::string& tableName, const TableDefinition& definition,
                               const std::string& connectionId) {
    // Implementation for table modification (ALTER TABLE)
    // This would be more complex and require diff analysis
    return false;
}

bool TableDesigner::dropTable(const std::string& tableName, const std::string& connectionId, bool cascade) {
    if (!sqlExecutor_) {
        return false;
    }

    try {
        std::string ddl = "DROP TABLE " + tableName;
        if (cascade) {
            ddl += " CASCADE";
        }
        ddl += ";";

        QueryExecutionContext context;
        context.connectionId = connectionId;

        QueryResult result = sqlExecutor_->executeQuery(ddl, context);

        if (result.success) {
            emit tableDroppedCallback_(tableName, connectionId);
            return true;
        } else {
            std::vector<std::string> errors = {result.errorMessage};
            emit validationErrorCallback_(errors);
            return false;
        }

    } catch (const std::exception& e) {
        std::vector<std::string> errors = {std::string("Failed to drop table: ") + e.what()};
        emit validationErrorCallback_(errors);
        return false;
    }
}

TableDefinition TableDesigner::getTableDefinition(const std::string& tableName, const std::string& connectionId) {
    // This would query the database metadata to get the table definition
    // For now, return a basic structure
    TableDefinition definition;
    definition.name = tableName;
    definition.database = connectionId;
    return definition;
}

std::vector<std::string> TableDesigner::getAvailableDataTypes(const std::string& connectionId) {
    // Return common SQL data types
    return {
        "TEXT", "INTEGER", "BIGINT", "FLOAT", "DOUBLE", "DECIMAL",
        "BOOLEAN", "DATE", "TIME", "DATETIME", "TIMESTAMP",
        "BLOB", "CLOB", "JSON", "XML"
    };
}

std::vector<std::string> TableDesigner::getAvailableCollations(const std::string& connectionId) {
    return {"UTF8", "LATIN1", "ASCII"};
}

std::vector<std::string> TableDesigner::getAvailableTablespaces(const std::string& connectionId) {
    return {"pg_default", "pg_global"};
}

std::string TableDesigner::generateDDL(const TableDefinition& definition, const std::string& connectionId) {
    return impl_->generateDDL(definition, connectionId);
}

std::vector<std::string> TableDesigner::validateTable(const TableDefinition& definition, const std::string& connectionId) {
    return impl_->validateTable(definition);
}

TableDesignOptions TableDesigner::getOptions() const {
    return options_;
}

void TableDesigner::updateOptions(const TableDesignOptions& options) {
    options_ = options;
}

void TableDesigner::setTableCreatedCallback(TableCreatedCallback callback) {
    tableCreatedCallback_ = callback;
}

void TableDesigner::setTableModifiedCallback(TableModifiedCallback callback) {
    tableModifiedCallback_ = callback;
}

void TableDesigner::setTableDroppedCallback(TableDroppedCallback callback) {
    tableDroppedCallback_ = callback;
}

void TableDesigner::setValidationErrorCallback(ValidationErrorCallback callback) {
    validationErrorCallback_ = callback;
}

QWidget* TableDesigner::getWidget() {
    return impl_->mainWidget_;
}

QDialog* TableDesigner::getDialog() {
    return impl_->dialog_;
}

int TableDesigner::exec() {
    if (impl_->dialog_) {
        return impl_->dialog_->exec();
    }
    return QDialog::Rejected;
}

// Private slot implementations

void TableDesigner::onAddColumnClicked() {
    if (impl_->columnsTable_->rowCount() < options_.maxColumns) {
        impl_->addColumnToTable(ColumnDefinition());
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onRemoveColumnClicked() {
    auto selection = impl_->columnsTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        impl_->columnsTable_->removeRow(row);
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onMoveColumnUpClicked() {
    auto selection = impl_->columnsTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        if (row > 0) {
            // Swap with previous row
            for (int col = 0; col < impl_->columnsTable_->columnCount(); ++col) {
                auto currentItem = impl_->columnsTable_->takeItem(row, col);
                auto prevItem = impl_->columnsTable_->takeItem(row - 1, col);
                impl_->columnsTable_->setItem(row, col, prevItem);
                impl_->columnsTable_->setItem(row - 1, col, currentItem);

                auto currentWidget = impl_->columnsTable_->cellWidget(row, col);
                auto prevWidget = impl_->columnsTable_->cellWidget(row - 1, col);
                impl_->columnsTable_->setCellWidget(row, col, prevWidget);
                impl_->columnsTable_->setCellWidget(row - 1, col, currentWidget);
            }
            impl_->columnsTable_->selectRow(row - 1);
            impl_->updateDDLPreview();
        }
    }
}

void TableDesigner::onMoveColumnDownClicked() {
    auto selection = impl_->columnsTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        if (row < impl_->columnsTable_->rowCount() - 1) {
            // Swap with next row
            for (int col = 0; col < impl_->columnsTable_->columnCount(); ++col) {
                auto currentItem = impl_->columnsTable_->takeItem(row, col);
                auto nextItem = impl_->columnsTable_->takeItem(row + 1, col);
                impl_->columnsTable_->setItem(row, col, nextItem);
                impl_->columnsTable_->setItem(row + 1, col, currentItem);

                auto currentWidget = impl_->columnsTable_->cellWidget(row, col);
                auto nextWidget = impl_->columnsTable_->cellWidget(row + 1, col);
                impl_->columnsTable_->setCellWidget(row, col, nextWidget);
                impl_->columnsTable_->setCellWidget(row + 1, col, currentWidget);
            }
            impl_->columnsTable_->selectRow(row + 1);
            impl_->updateDDLPreview();
        }
    }
}

void TableDesigner::onAddConstraintClicked() {
    if (impl_->constraintsList_->count() < options_.maxConstraints) {
        ConstraintDefinition constraint;
        constraint.name = "constraint_" + std::to_string(impl_->constraintsList_->count() + 1);
        impl_->addConstraintToList(constraint);
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onRemoveConstraintClicked() {
    auto selection = impl_->constraintsList_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        delete impl_->constraintsList_->takeItem(selection.first().row());
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onAddIndexClicked() {
    if (impl_->indexesList_->count() < options_.maxIndexes) {
        IndexDefinition index;
        index.name = "index_" + std::to_string(impl_->indexesList_->count() + 1);
        impl_->addIndexToList(index);
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onRemoveIndexClicked() {
    auto selection = impl_->indexesList_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        delete impl_->indexesList_->takeItem(selection.first().row());
        impl_->updateDDLPreview();
    }
}

void TableDesigner::onPreviewClicked() {
    impl_->updateDDLPreview();
}

void TableDesigner::onValidateClicked() {
    auto definition = impl_->getCurrentDefinition();
    auto errors = validateTable(definition, impl_->currentConnectionId_);

    if (errors.empty()) {
        QMessageBox::information(impl_->dialog_, "Validation", "Table definition is valid!");
    } else {
        QString errorText;
        for (const auto& error : errors) {
            errorText += QString::fromStdString(error) + "\n";
        }
        QMessageBox::warning(impl_->dialog_, "Validation Errors", errorText);
        emit validationErrorCallback_(errors);
    }
}

void TableDesigner::onCreateClicked() {
    auto definition = impl_->getCurrentDefinition();
    auto errors = validateTable(definition, impl_->currentConnectionId_);

    if (!errors.empty()) {
        QString errorText;
        for (const auto& error : errors) {
            errorText += QString::fromStdString(error) + "\n";
        }
        QMessageBox::warning(impl_->dialog_, "Validation Errors",
                           "Please fix the following errors:\n" + errorText);
        emit validationErrorCallback_(errors);
        return;
    }

    if (createTable(definition, impl_->currentConnectionId_)) {
        QMessageBox::information(impl_->dialog_, "Success",
                               "Table '" + QString::fromStdString(definition.name) + "' created successfully!");
        impl_->dialog_->accept();
    }
}

void TableDesigner::onCancelClicked() {
    impl_->dialog_->reject();
}

void TableDesigner::onColumnSelectionChanged() {
    // Enable/disable buttons based on selection
    bool hasSelection = !impl_->columnsTable_->selectionModel()->selectedRows().isEmpty();
    impl_->removeColumnButton_->setEnabled(hasSelection);

    auto selection = impl_->columnsTable_->selectionModel()->selectedRows();
    if (!selection.isEmpty()) {
        int row = selection.first().row();
        impl_->moveUpButton_->setEnabled(row > 0);
        impl_->moveDownButton_->setEnabled(row < impl_->columnsTable_->rowCount() - 1);
    }
}

void TableDesigner::onConstraintSelectionChanged() {
    bool hasSelection = !impl_->constraintsList_->selectionModel()->selectedRows().isEmpty();
    impl_->removeConstraintButton_->setEnabled(hasSelection);
}

void TableDesigner::onIndexSelectionChanged() {
    bool hasSelection = !impl_->indexesList_->selectionModel()->selectedRows().isEmpty();
    impl_->removeIndexButton_->setEnabled(hasSelection);
}

void TableDesigner::onTableNameChanged(const QString& text) {
    impl_->updateDDLPreview();
}

void TableDesigner::onColumnTypeChanged(int index) {
    impl_->updateDDLPreview();
}

} // namespace scratchrobin
