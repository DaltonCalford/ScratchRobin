#include "user_defined_types_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QHeaderView>
#include <QApplication>
#include <algorithm>

namespace scratchrobin {

UserDefinedTypesDialog::UserDefinedTypesDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , mainSplitter_(nullptr)
    , leftWidget_(nullptr)
    , leftLayout_(nullptr)
    , toolbarLayout_(nullptr)
    , createTypeButton_(nullptr)
    , refreshButton_(nullptr)
    , typesTree_(nullptr)
    , rightWidget_(nullptr)
    , rightLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , typeNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , typeCategoryCombo_(nullptr)
    , descriptionEdit_(nullptr)
    , compositeTab_(nullptr)
    , compositeLayout_(nullptr)
    , fieldsTable_(nullptr)
    , fieldsButtonLayout_(nullptr)
    , addFieldButton_(nullptr)
    , editFieldButton_(nullptr)
    , removeFieldButton_(nullptr)
    , moveFieldUpButton_(nullptr)
    , moveFieldDownButton_(nullptr)
    , fieldGroup_(nullptr)
    , fieldLayout_(nullptr)
    , fieldNameEdit_(nullptr)
    , fieldTypeCombo_(nullptr)
    , fieldLengthSpin_(nullptr)
    , fieldPrecisionSpin_(nullptr)
    , fieldScaleSpin_(nullptr)
    , fieldDefaultEdit_(nullptr)
    , fieldCommentEdit_(nullptr)
    , enumTab_(nullptr)
    , enumTabLayout_(nullptr)
    , enumTable_(nullptr)
    , enumButtonLayout_(nullptr)
    , addEnumButton_(nullptr)
    , editEnumButton_(nullptr)
    , removeEnumButton_(nullptr)
    , moveEnumUpButton_(nullptr)
    , moveEnumDownButton_(nullptr)
    , enumGroup_(nullptr)
    , enumFormLayout_(nullptr)
    , enumValueEdit_(nullptr)
    , enumCommentEdit_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , advancedGroup_(nullptr)
    , advancedFormLayout_(nullptr)
    , baseTypeCombo_(nullptr)
    , elementTypeCombo_(nullptr)
    , inputFunctionEdit_(nullptr)
    , outputFunctionEdit_(nullptr)
    , checkConstraintEdit_(nullptr)
    , domainDefaultEdit_(nullptr)
    , notNullCheck_(nullptr)
    , sqlTab_(nullptr)
    , sqlLayout_(nullptr)
    , sqlPreviewEdit_(nullptr)
    , generateSqlButton_(nullptr)
    , validateButton_(nullptr)
    , dialogButtons_(nullptr)
    , currentDatabaseType_(DatabaseType::POSTGRESQL)
    , isEditMode_(false)
    , currentSchema_("")
    , currentTypeName_("")
    , originalTypeName_("")
    , originalSchema_("")
    , driverManager_(&DatabaseDriverManager::instance())
{
    setupUI();
    setWindowTitle("User-Defined Types Manager");
    setModal(true);
    resize(1000, 700);
}

void UserDefinedTypesDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    // Setup left side (types tree)
    setupTypesTree();

    // Setup right side (type editor)
    setupBasicTab();
    setupCompositeTab();
    setupEnumTab();
    setupAdvancedTab();
    setupSQLTab();

    mainLayout_->addWidget(mainSplitter_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &UserDefinedTypesDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set splitter proportions
    mainSplitter_->setStretchFactor(0, 1);
    mainSplitter_->setStretchFactor(1, 2);
}

void UserDefinedTypesDialog::setupTypesTree() {
    leftWidget_ = new QWidget();
    leftLayout_ = new QVBoxLayout(leftWidget_);

    // Toolbar
    toolbarLayout_ = new QHBoxLayout();
    createTypeButton_ = new QPushButton("Create Type", this);
    refreshButton_ = new QPushButton("Refresh", this);

    toolbarLayout_->addWidget(createTypeButton_);
    toolbarLayout_->addWidget(refreshButton_);
    toolbarLayout_->addStretch();

    leftLayout_->addLayout(toolbarLayout_);

    // Types tree
    typesTree_ = new QTreeWidget(leftWidget_);
    typesTree_->setHeaderLabel("User-Defined Types");
    typesTree_->setContextMenuPolicy(Qt::CustomContextMenu);

    leftLayout_->addWidget(typesTree_);

    // Connect signals
    connect(createTypeButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onCreateType);
    connect(refreshButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onRefreshTypes);
    connect(typesTree_, &QTreeWidget::itemSelectionChanged, [this]() {
        // Handle type selection
        updateButtonStates();
    });

    mainSplitter_->addWidget(leftWidget_);
}

void UserDefinedTypesDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    typeNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    typeCategoryCombo_ = new QComboBox(basicTab_);
    descriptionEdit_ = new QTextEdit(basicTab_);
    descriptionEdit_->setMaximumHeight(60);

    // Populate type categories
    populateTypeCategories();

    basicLayout_->addRow("Type Name:", typeNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Category:", typeCategoryCombo_);
    basicLayout_->addRow("Description:", descriptionEdit_);

    // Connect signals
    connect(typeNameEdit_, &QLineEdit::textChanged, [this](const QString& name) {
        // Validate type name
        static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
        if (!name.isEmpty() && !validName.match(name).hasMatch()) {
            // Could show a warning
        }
    });
    connect(typeCategoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserDefinedTypesDialog::onTypeCategoryChanged);

    // Add to tab widget
    if (!tabWidget_) {
        rightWidget_ = new QWidget();
        rightLayout_ = new QVBoxLayout(rightWidget_);
        tabWidget_ = new QTabWidget(rightWidget_);
        rightLayout_->addWidget(tabWidget_);
        mainSplitter_->addWidget(rightWidget_);
    }

    tabWidget_->addTab(basicTab_, "Basic");
}

void UserDefinedTypesDialog::setupCompositeTab() {
    compositeTab_ = new QWidget();
    compositeLayout_ = new QVBoxLayout(compositeTab_);

    // Fields table
    setupFieldsTable();
    compositeLayout_->addWidget(fieldsTable_);

    // Field buttons
    fieldsButtonLayout_ = new QHBoxLayout();
    addFieldButton_ = new QPushButton("Add Field", compositeTab_);
    editFieldButton_ = new QPushButton("Edit Field", compositeTab_);
    removeFieldButton_ = new QPushButton("Remove Field", compositeTab_);
    moveFieldUpButton_ = new QPushButton("Move Up", compositeTab_);
    moveFieldDownButton_ = new QPushButton("Move Down", compositeTab_);

    fieldsButtonLayout_->addWidget(addFieldButton_);
    fieldsButtonLayout_->addWidget(editFieldButton_);
    fieldsButtonLayout_->addWidget(removeFieldButton_);
    fieldsButtonLayout_->addStretch();
    fieldsButtonLayout_->addWidget(moveFieldUpButton_);
    fieldsButtonLayout_->addWidget(moveFieldDownButton_);

    compositeLayout_->addLayout(fieldsButtonLayout_);

    // Field edit dialog
    setupFieldDialog();
    compositeLayout_->addWidget(fieldGroup_);

    // Connect signals
    connect(addFieldButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onAddField);
    connect(editFieldButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onEditField);
    connect(removeFieldButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onRemoveField);
    connect(moveFieldUpButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onMoveFieldUp);
    connect(moveFieldDownButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onMoveFieldDown);
    connect(fieldsTable_, &QTableWidget::itemSelectionChanged, [this]() {
        updateButtonStates();
    });

    tabWidget_->addTab(compositeTab_, "Composite Fields");
}

void UserDefinedTypesDialog::setupEnumTab() {
    enumTab_ = new QWidget();
    enumTabLayout_ = new QVBoxLayout(enumTab_);

    // Enum table
    setupEnumTable();
    enumTabLayout_->addWidget(enumTable_);

    // Enum buttons
    enumButtonLayout_ = new QHBoxLayout();
    addEnumButton_ = new QPushButton("Add Value", enumTab_);
    editEnumButton_ = new QPushButton("Edit Value", enumTab_);
    removeEnumButton_ = new QPushButton("Remove Value", enumTab_);
    moveEnumUpButton_ = new QPushButton("Move Up", enumTab_);
    moveEnumDownButton_ = new QPushButton("Move Down", enumTab_);

    enumButtonLayout_->addWidget(addEnumButton_);
    enumButtonLayout_->addWidget(editEnumButton_);
    enumButtonLayout_->addWidget(removeEnumButton_);
    enumButtonLayout_->addStretch();
    enumButtonLayout_->addWidget(moveEnumUpButton_);
    enumButtonLayout_->addWidget(moveEnumDownButton_);

    enumTabLayout_->addLayout(enumButtonLayout_);

    // Enum edit dialog
    setupEnumDialog();
    enumTabLayout_->addWidget(enumGroup_);

    // Connect signals
    connect(addEnumButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onAddEnumValue);
    connect(editEnumButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onEditEnumValue);
    connect(removeEnumButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onRemoveEnumValue);
    connect(moveEnumUpButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onMoveEnumValueUp);
    connect(moveEnumDownButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onMoveEnumValueDown);
    connect(enumTable_, &QTableWidget::itemSelectionChanged, [this]() {
        updateButtonStates();
    });

    tabWidget_->addTab(enumTab_, "Enum Values");
}

void UserDefinedTypesDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    advancedGroup_ = new QGroupBox("Advanced Options", advancedTab_);
    advancedFormLayout_ = new QFormLayout(advancedGroup_);

    baseTypeCombo_ = new QComboBox(advancedTab_);
    elementTypeCombo_ = new QComboBox(advancedTab_);
    inputFunctionEdit_ = new QLineEdit(advancedTab_);
    outputFunctionEdit_ = new QLineEdit(advancedTab_);
    checkConstraintEdit_ = new QTextEdit(advancedTab_);
    checkConstraintEdit_->setMaximumHeight(60);
    domainDefaultEdit_ = new QLineEdit(advancedTab_);
    notNullCheck_ = new QCheckBox("NOT NULL", advancedTab_);

    // Populate combo boxes
    populateBaseTypes();
    populateElementTypes();

    advancedFormLayout_->addRow("Base Type:", baseTypeCombo_);
    advancedFormLayout_->addRow("Element Type:", elementTypeCombo_);
    advancedFormLayout_->addRow("Input Function:", inputFunctionEdit_);
    advancedFormLayout_->addRow("Output Function:", outputFunctionEdit_);
    advancedFormLayout_->addRow("Check Constraint:", checkConstraintEdit_);
    advancedFormLayout_->addRow("Default Value:", domainDefaultEdit_);
    advancedFormLayout_->addRow("", notNullCheck_);

    advancedLayout_->addWidget(advancedGroup_);
    advancedLayout_->addStretch();

    // Connect signals
    connect(baseTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserDefinedTypesDialog::onBaseTypeChanged);
    connect(elementTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &UserDefinedTypesDialog::onElementTypeChanged);

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void UserDefinedTypesDialog::setupSQLTab() {
    sqlTab_ = new QWidget();
    sqlLayout_ = new QVBoxLayout(sqlTab_);

    sqlPreviewEdit_ = new QTextEdit(sqlTab_);
    sqlPreviewEdit_->setFontFamily("Monospace");
    sqlPreviewEdit_->setLineWrapMode(QTextEdit::NoWrap);

    generateSqlButton_ = new QPushButton("Generate SQL", sqlTab_);
    validateButton_ = new QPushButton("Validate", sqlTab_);

    QHBoxLayout* sqlButtonLayout = new QHBoxLayout();
    sqlButtonLayout->addWidget(generateSqlButton_);
    sqlButtonLayout->addWidget(validateSqlButton_);
    sqlButtonLayout->addStretch();

    sqlLayout_->addWidget(sqlPreviewEdit_);
    sqlLayout_->addLayout(sqlButtonLayout);

    // Connect signals
    connect(generateSqlButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &UserDefinedTypesDialog::onValidateType);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void UserDefinedTypesDialog::setupFieldsTable() {
    fieldsTable_ = new QTableWidget(compositeTab_);
    fieldsTable_->setColumnCount(5);
    fieldsTable_->setHorizontalHeaderLabels({"Field Name", "Data Type", "Length", "Default", "Comment"});

    fieldsTable_->horizontalHeader()->setStretchLastSection(true);
    fieldsTable_->verticalHeader()->setDefaultSectionSize(25);
    fieldsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    fieldsTable_->setAlternatingRowColors(true);
}

void UserDefinedTypesDialog::setupEnumTable() {
    enumTable_ = new QTableWidget(enumTab_);
    enumTable_->setColumnCount(2);
    enumTable_->setHorizontalHeaderLabels({"Value", "Comment"});

    enumTable_->horizontalHeader()->setStretchLastSection(true);
    enumTable_->verticalHeader()->setDefaultSectionSize(25);
    enumTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    enumTable_->setAlternatingRowColors(true);
}

void UserDefinedTypesDialog::setupFieldDialog() {
    fieldGroup_ = new QGroupBox("Field Properties", compositeTab_);
    fieldLayout_ = new QFormLayout(fieldGroup_);

    fieldNameEdit_ = new QLineEdit(fieldGroup_);
    fieldTypeCombo_ = new QComboBox(fieldGroup_);
    fieldLengthSpin_ = new QSpinBox(fieldGroup_);
    fieldPrecisionSpin_ = new QSpinBox(fieldGroup_);
    fieldScaleSpin_ = new QSpinBox(fieldGroup_);
    fieldDefaultEdit_ = new QLineEdit(fieldGroup_);
    fieldCommentEdit_ = new QTextEdit(fieldGroup_);
    fieldCommentEdit_->setMaximumHeight(40);

    // Populate field types
    QStringList fieldTypes = {
        "INT", "BIGINT", "SMALLINT", "TINYINT", "VARCHAR", "TEXT", "DECIMAL", "FLOAT", "DOUBLE",
        "BOOLEAN", "DATE", "TIME", "DATETIME", "TIMESTAMP", "BLOB", "CLOB", "JSON"
    };
    fieldTypeCombo_->addItems(fieldTypes);

    fieldLayout_->addRow("Field Name:", fieldNameEdit_);
    fieldLayout_->addRow("Data Type:", fieldTypeCombo_);
    fieldLayout_->addRow("Length:", fieldLengthSpin_);
    fieldLayout_->addRow("Precision:", fieldPrecisionSpin_);
    fieldLayout_->addRow("Scale:", fieldScaleSpin_);
    fieldLayout_->addRow("Default Value:", fieldDefaultEdit_);
    fieldLayout_->addRow("Comment:", fieldCommentEdit_);
}

void UserDefinedTypesDialog::setupEnumDialog() {
    enumGroup_ = new QGroupBox("Enum Value Properties", enumTab_);
    enumFormLayout_ = new QFormLayout(enumGroup_);

    enumValueEdit_ = new QLineEdit(enumGroup_);
    enumCommentEdit_ = new QTextEdit(enumGroup_);
    enumCommentEdit_->setMaximumHeight(40);

    enumFormLayout_->addRow("Value:", enumValueEdit_);
    enumFormLayout_->addRow("Comment:", enumCommentEdit_);
}

void UserDefinedTypesDialog::populateTypeCategories() {
    if (!typeCategoryCombo_) return;

    typeCategoryCombo_->clear();
    typeCategoryCombo_->addItem("COMPOSITE", "COMPOSITE");
    typeCategoryCombo_->addItem("ENUM", "ENUM");

    // Add database-specific categories
    switch (currentDatabaseType_) {
        case DatabaseType::POSTGRESQL:
            typeCategoryCombo_->addItem("BASE", "BASE");
            typeCategoryCombo_->addItem("ARRAY", "ARRAY");
            typeCategoryCombo_->addItem("DOMAIN", "DOMAIN");
            break;
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            // Limited UDT support
            break;
        case DatabaseType::ORACLE:
            typeCategoryCombo_->addItem("OBJECT", "OBJECT");
            typeCategoryCombo_->addItem("COLLECTION", "COLLECTION");
            break;
        case DatabaseType::SQLSERVER:
        case DatabaseType::MSSQL:
            typeCategoryCombo_->addItem("TABLE", "TABLE");
            break;
        default:
            break;
    }
}

void UserDefinedTypesDialog::populateBaseTypes() {
    if (!baseTypeCombo_) return;

    baseTypeCombo_->clear();
    baseTypeCombo_->addItem(""); // Empty option

    QStringList baseTypes = {
        "INT", "BIGINT", "SMALLINT", "VARCHAR", "TEXT", "DECIMAL", "FLOAT", "DOUBLE",
        "BOOLEAN", "DATE", "TIME", "TIMESTAMP", "BLOB", "CLOB"
    };
    baseTypeCombo_->addItems(baseTypes);
}

void UserDefinedTypesDialog::populateElementTypes() {
    if (!elementTypeCombo_) return;

    elementTypeCombo_->clear();
    elementTypeCombo_->addItem(""); // Empty option

    QStringList elementTypes = {
        "INT", "BIGINT", "VARCHAR", "TEXT", "DECIMAL", "FLOAT", "DOUBLE", "BOOLEAN", "DATE"
    };
    elementTypeCombo_->addItems(elementTypes);
}

void UserDefinedTypesDialog::setUserDefinedType(const UserDefinedType& type) {
    currentDefinition_ = type;

    // Update UI
    typeNameEdit_->setText(type.name);
    schemaEdit_->setText(type.schema);
    descriptionEdit_->setPlainText(type.description);

    // Set category
    if (!type.typeCategory.isEmpty()) {
        int index = typeCategoryCombo_->findData(type.typeCategory);
        if (index >= 0) typeCategoryCombo_->setCurrentIndex(index);
    }

    // Set advanced options
    if (!type.baseType.isEmpty()) {
        int index = baseTypeCombo_->findText(type.baseType);
        if (index >= 0) baseTypeCombo_->setCurrentIndex(index);
    }

    if (!type.elementType.isEmpty()) {
        int index = elementTypeCombo_->findText(type.elementType);
        if (index >= 0) elementTypeCombo_->setCurrentIndex(index);
    }

    inputFunctionEdit_->setText(type.inputFunction);
    outputFunctionEdit_->setText(type.outputFunction);
    checkConstraintEdit_->setPlainText(type.checkConstraint);
    domainDefaultEdit_->setText(type.domainDefault);
    notNullCheck_->setChecked(type.notNull);

    // Update tables
    updateFieldsTable();
    updateEnumTable();

    updateUIForTypeCategory();
}

UserDefinedType UserDefinedTypesDialog::getUserDefinedType() const {
    UserDefinedType definition = currentDefinition_;

    definition.name = typeNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.typeCategory = typeCategoryCombo_->currentData().toString();
    definition.description = descriptionEdit_->toPlainText();
    definition.baseType = baseTypeCombo_->currentText();
    definition.elementType = elementTypeCombo_->currentText();
    definition.inputFunction = inputFunctionEdit_->text();
    definition.outputFunction = outputFunctionEdit_->text();
    definition.checkConstraint = checkConstraintEdit_->toPlainText();
    definition.domainDefault = domainDefaultEdit_->text();
    definition.notNull = notNullCheck_->isChecked();

    return definition;
}

void UserDefinedTypesDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit User-Defined Type");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create User-Defined Type");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void UserDefinedTypesDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update available categories and types
    populateTypeCategories();
    populateBaseTypes();
    populateElementTypes();
}

void UserDefinedTypesDialog::loadExistingType(const QString& schema, const QString& typeName) {
    setTableInfo(schema, typeName);
    setEditMode(true);

    // TODO: Load actual type definition from database
}

void UserDefinedTypesDialog::refreshTypesList() {
    // TODO: Refresh types from database
    QMessageBox::information(this, "Refresh", "Types refresh will be implemented when database connectivity is available.");
}

void UserDefinedTypesDialog::accept() {
    if (validateType()) {
        emit typeSaved(getUserDefinedType());
        QDialog::accept();
    }
}

void UserDefinedTypesDialog::reject() {
    QDialog::reject();
}

// Type management slots
void UserDefinedTypesDialog::onCreateType() {
    // Clear current definition and show creation form
    currentDefinition_ = UserDefinedType();
    typeNameEdit_->clear();
    schemaEdit_->clear();
    descriptionEdit_->clear();

    updateUIForTypeCategory();
}

void UserDefinedTypesDialog::onEditType() {
    // TODO: Load selected type for editing
    QMessageBox::information(this, "Edit Type", "Type editing will be implemented when database connectivity is available.");
}

void UserDefinedTypesDialog::onDeleteType() {
    // TODO: Delete selected type
    QMessageBox::information(this, "Delete Type", "Type deletion will be implemented when database connectivity is available.");
}

void UserDefinedTypesDialog::onRefreshTypes() {
    refreshTypesList();
}

// Type selection slots
void UserDefinedTypesDialog::onTypeCategoryChanged(int index) {
    updateUIForTypeCategory();
}

void UserDefinedTypesDialog::onBaseTypeChanged(int index) {
    // Handle base type changes if needed
}

void UserDefinedTypesDialog::onElementTypeChanged(int index) {
    // Handle element type changes if needed
}

// Composite type field slots
void UserDefinedTypesDialog::onAddField() {
    clearFieldDialog();
    // Switch to composite tab to show the edit form
    tabWidget_->setCurrentWidget(compositeTab_);
}

void UserDefinedTypesDialog::onEditField() {
    int row = fieldsTable_->currentRow();
    if (row >= 0) {
        loadFieldToDialog(row);
    }
}

void UserDefinedTypesDialog::onRemoveField() {
    int row = fieldsTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.fields.removeAt(row);
        updateFieldsTable();
        updateButtonStates();
    }
}

void UserDefinedTypesDialog::onMoveFieldUp() {
    int row = fieldsTable_->currentRow();
    if (row > 0) {
        currentDefinition_.fields.move(row, row - 1);
        updateFieldsTable();
        fieldsTable_->setCurrentCell(row - 1, 0);
    }
}

void UserDefinedTypesDialog::onMoveFieldDown() {
    int row = fieldsTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.fields.size() - 1) {
        currentDefinition_.fields.move(row, row + 1);
        updateFieldsTable();
        fieldsTable_->setCurrentCell(row + 1, 0);
    }
}

// Enum value slots
void UserDefinedTypesDialog::onAddEnumValue() {
    clearEnumDialog();
    // Switch to enum tab to show the edit form
    tabWidget_->setCurrentWidget(enumTab_);
}

void UserDefinedTypesDialog::onEditEnumValue() {
    int row = enumTable_->currentRow();
    if (row >= 0) {
        loadEnumToDialog(row);
    }
}

void UserDefinedTypesDialog::onRemoveEnumValue() {
    int row = enumTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.enumValues.removeAt(row);
        updateEnumTable();
        updateButtonStates();
    }
}

void UserDefinedTypesDialog::onMoveEnumValueUp() {
    int row = enumTable_->currentRow();
    if (row > 0) {
        currentDefinition_.enumValues.move(row, row - 1);
        updateEnumTable();
        enumTable_->setCurrentCell(row - 1, 0);
    }
}

void UserDefinedTypesDialog::onMoveEnumValueDown() {
    int row = enumTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.enumValues.size() - 1) {
        currentDefinition_.enumValues.move(row, row + 1);
        updateEnumTable();
        enumTable_->setCurrentCell(row + 1, 0);
    }
}

// Action slots
void UserDefinedTypesDialog::onGenerateSQL() {
    if (validateType()) {
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

void UserDefinedTypesDialog::onPreviewSQL() {
    onGenerateSQL();
}

void UserDefinedTypesDialog::onValidateType() {
    if (validateType()) {
        QMessageBox::information(this, "Validation", "Type definition is valid.");
    }
}

void UserDefinedTypesDialog::onAnalyzeUsage() {
    QMessageBox::information(this, "Analyze Usage", "Type usage analysis will be implemented when database connectivity is available.");
}

void UserDefinedTypesDialog::updateFieldsTable() {
    fieldsTable_->setRowCount(currentDefinition_.fields.size());

    for (int i = 0; i < currentDefinition_.fields.size(); ++i) {
        const CompositeField& field = currentDefinition_.fields[i];

        fieldsTable_->setItem(i, 0, new QTableWidgetItem(field.name));
        fieldsTable_->setItem(i, 1, new QTableWidgetItem(field.dataType));
        fieldsTable_->setItem(i, 2, new QTableWidgetItem(field.length > 0 ? QString::number(field.length) : ""));
        fieldsTable_->setItem(i, 3, new QTableWidgetItem(field.defaultValue));
        fieldsTable_->setItem(i, 4, new QTableWidgetItem(field.comment));
    }
}

void UserDefinedTypesDialog::updateEnumTable() {
    enumTable_->setRowCount(currentDefinition_.enumValues.size());

    for (int i = 0; i < currentDefinition_.enumValues.size(); ++i) {
        const EnumValue& enumVal = currentDefinition_.enumValues[i];

        enumTable_->setItem(i, 0, new QTableWidgetItem(enumVal.value));
        enumTable_->setItem(i, 1, new QTableWidgetItem(enumVal.comment));
    }
}

void UserDefinedTypesDialog::updateUIForTypeCategory() {
    QString category = typeCategoryCombo_->currentData().toString();

    // Show/hide tabs based on category
    bool showComposite = (category == "COMPOSITE");
    bool showEnum = (category == "ENUM");
    bool showAdvanced = (category == "BASE" || category == "ARRAY" || category == "DOMAIN");

    tabWidget_->setTabEnabled(tabWidget_->indexOf(compositeTab_), showComposite);
    tabWidget_->setTabEnabled(tabWidget_->indexOf(enumTab_), showEnum);
    tabWidget_->setTabEnabled(tabWidget_->indexOf(advancedTab_), showAdvanced);

    // Update advanced options based on category
    if (category == "DOMAIN") {
        baseTypeCombo_->setEnabled(true);
        elementTypeCombo_->setEnabled(false);
        inputFunctionEdit_->setEnabled(false);
        outputFunctionEdit_->setEnabled(false);
        checkConstraintEdit_->setEnabled(true);
        domainDefaultEdit_->setEnabled(true);
        notNullCheck_->setEnabled(true);
    } else if (category == "ARRAY") {
        baseTypeCombo_->setEnabled(false);
        elementTypeCombo_->setEnabled(true);
        inputFunctionEdit_->setEnabled(false);
        outputFunctionEdit_->setEnabled(false);
        checkConstraintEdit_->setEnabled(false);
        domainDefaultEdit_->setEnabled(false);
        notNullCheck_->setEnabled(false);
    } else if (category == "BASE") {
        baseTypeCombo_->setEnabled(true);
        elementTypeCombo_->setEnabled(false);
        inputFunctionEdit_->setEnabled(true);
        outputFunctionEdit_->setEnabled(true);
        checkConstraintEdit_->setEnabled(false);
        domainDefaultEdit_->setEnabled(false);
        notNullCheck_->setEnabled(false);
    } else {
        baseTypeCombo_->setEnabled(false);
        elementTypeCombo_->setEnabled(false);
        inputFunctionEdit_->setEnabled(false);
        outputFunctionEdit_->setEnabled(false);
        checkConstraintEdit_->setEnabled(false);
        domainDefaultEdit_->setEnabled(false);
        notNullCheck_->setEnabled(false);
    }

    updateButtonStates();
}

bool UserDefinedTypesDialog::validateType() {
    QString typeName = typeNameEdit_->text().trimmed();
    QString category = typeCategoryCombo_->currentData().toString();

    if (typeName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Type name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        typeNameEdit_->setFocus();
        return false;
    }

    // Category-specific validation
    if (category == "COMPOSITE" && currentDefinition_.fields.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Composite types must have at least one field.");
        tabWidget_->setCurrentWidget(compositeTab_);
        return false;
    }

    if (category == "ENUM" && currentDefinition_.enumValues.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Enum types must have at least one value.");
        tabWidget_->setCurrentWidget(enumTab_);
        return false;
    }

    if (category == "DOMAIN" && baseTypeCombo_->currentText().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Domain types must specify a base type.");
        tabWidget_->setCurrentWidget(advancedTab_);
        baseTypeCombo_->setFocus();
        return false;
    }

    if (category == "ARRAY" && elementTypeCombo_->currentText().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Array types must specify an element type.");
        tabWidget_->setCurrentWidget(advancedTab_);
        elementTypeCombo_->setFocus();
        return false;
    }

    return true;
}

QString UserDefinedTypesDialog::generateCreateSQL() const {
    QStringList sqlParts;
    QString category = typeCategoryCombo_->currentData().toString();
    QString fullTypeName = typeNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTypeName = QString("%1.%2").arg(schemaEdit_->text(), typeNameEdit_->text());
    }

    if (category == "COMPOSITE") {
        sqlParts.append(QString("CREATE TYPE %1 AS (").arg(fullTypeName));

        QStringList fieldDefinitions;
        for (const CompositeField& field : currentDefinition_.fields) {
            QString fieldDef = field.name + " " + field.dataType;
            if (field.length > 0) {
                if (field.precision > 0) {
                    fieldDef += QString("(%1,%2)").arg(field.length).arg(field.precision);
                } else {
                    fieldDef += QString("(%1)").arg(field.length);
                }
            }
            fieldDefinitions.append(fieldDef);
        }

        sqlParts.append(fieldDefinitions.join(",\n"));
        sqlParts.append(");");

    } else if (category == "ENUM") {
        sqlParts.append(QString("CREATE TYPE %1 AS ENUM (").arg(fullTypeName));

        QStringList enumValues;
        for (const EnumValue& enumVal : currentDefinition_.enumValues) {
            enumValues.append(QString("'%1'").arg(enumVal.value));
        }

        sqlParts.append(enumValues.join(", "));
        sqlParts.append(");");

    } else if (category == "DOMAIN") {
        QString baseType = baseTypeCombo_->currentText();
        QString checkConstraint = checkConstraintEdit_->toPlainText().trimmed();
        QString defaultValue = domainDefaultEdit_->text().trimmed();

        sqlParts.append(QString("CREATE DOMAIN %1 AS %2").arg(fullTypeName, baseType));

        if (!defaultValue.isEmpty()) {
            sqlParts.append(QString("DEFAULT %1").arg(defaultValue));
        }

        if (notNullCheck_->isChecked()) {
            sqlParts.append("NOT NULL");
        }

        if (!checkConstraint.isEmpty()) {
            sqlParts.append(QString("CHECK (%1)").arg(checkConstraint));
        }

        sqlParts.append(";");

    } else if (category == "ARRAY") {
        QString elementType = elementTypeCombo_->currentText();
        sqlParts.append(QString("CREATE TYPE %1 AS %2[];").arg(fullTypeName, elementType));

    } else if (category == "BASE") {
        QString baseType = baseTypeCombo_->currentText();
        QString inputFunc = inputFunctionEdit_->text().trimmed();
        QString outputFunc = outputFunctionEdit_->text().trimmed();

        sqlParts.append(QString("CREATE TYPE %1 (").arg(fullTypeName));
        sqlParts.append(QString("    INPUT = %1,").arg(inputFunc));
        sqlParts.append(QString("    OUTPUT = %1").arg(outputFunc));
        sqlParts.append(");");
    }

    // Add comment if present
    QString comment = descriptionEdit_->toPlainText().trimmed();
    if (!comment.isEmpty()) {
        sqlParts.append(QString("COMMENT ON TYPE %1 IS '%2';").arg(fullTypeName, comment.replace("'", "''")));
    }

    return sqlParts.join("\n");
}

QString UserDefinedTypesDialog::generateDropSQL() const {
    QString fullTypeName = typeNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTypeName = QString("%1.%2").arg(schemaEdit_->text(), typeNameEdit_->text());
    }

    return QString("DROP TYPE IF EXISTS %1;").arg(fullTypeName);
}

QString UserDefinedTypesDialog::generateAlterSQL() const {
    // For most databases, ALTER TYPE is limited
    // For now, use DROP and CREATE
    QStringList sqlParts;
    sqlParts.append(generateDropSQL());
    sqlParts.append(generateCreateSQL());
    return sqlParts.join("\n");
}

void UserDefinedTypesDialog::loadFieldToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.fields.size()) {
        return;
    }

    const CompositeField& field = currentDefinition_.fields[row];

    fieldNameEdit_->setText(field.name);

    int typeIndex = fieldTypeCombo_->findText(field.dataType);
    if (typeIndex >= 0) {
        fieldTypeCombo_->setCurrentIndex(typeIndex);
    } else {
        fieldTypeCombo_->setCurrentText(field.dataType);
    }

    fieldLengthSpin_->setValue(field.length);
    fieldPrecisionSpin_->setValue(field.precision);
    fieldScaleSpin_->setValue(field.scale);
    fieldDefaultEdit_->setText(field.defaultValue);
    fieldCommentEdit_->setPlainText(field.comment);

    // Switch to composite tab to show the edit form
    tabWidget_->setCurrentWidget(compositeTab_);
}

void UserDefinedTypesDialog::saveFieldFromDialog() {
    CompositeField field;

    field.name = fieldNameEdit_->text().trimmed();
    field.dataType = fieldTypeCombo_->currentText();
    field.length = fieldLengthSpin_->value();
    field.precision = fieldPrecisionSpin_->value();
    field.scale = fieldScaleSpin_->value();
    field.defaultValue = fieldDefaultEdit_->text();
    field.comment = fieldCommentEdit_->toPlainText();

    // Validate field name
    if (field.name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Field name is required.");
        fieldNameEdit_->setFocus();
        return;
    }

    // Check for duplicate field names
    for (int i = 0; i < currentDefinition_.fields.size(); ++i) {
        if (currentDefinition_.fields[i].name == field.name) {
            // If editing existing field, allow it
            if (fieldsTable_->currentRow() != i) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Field name '%1' already exists.").arg(field.name));
                fieldNameEdit_->setFocus();
                return;
            }
        }
    }

    int currentRow = fieldsTable_->currentRow();
    if (currentRow >= 0 && currentRow < currentDefinition_.fields.size()) {
        // Update existing field
        currentDefinition_.fields[currentRow] = field;
    } else {
        // Add new field
        currentDefinition_.fields.append(field);
    }

    updateFieldsTable();
    clearFieldDialog();
    updateButtonStates();
}

void UserDefinedTypesDialog::clearFieldDialog() {
    fieldNameEdit_->clear();
    fieldTypeCombo_->setCurrentIndex(0);
    fieldLengthSpin_->setValue(0);
    fieldPrecisionSpin_->setValue(0);
    fieldScaleSpin_->setValue(0);
    fieldDefaultEdit_->clear();
    fieldCommentEdit_->clear();

    // Clear table selection
    fieldsTable_->clearSelection();
}

void UserDefinedTypesDialog::loadEnumToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.enumValues.size()) {
        return;
    }

    const EnumValue& enumVal = currentDefinition_.enumValues[row];

    enumValueEdit_->setText(enumVal.value);
    enumCommentEdit_->setPlainText(enumVal.comment);

    // Switch to enum tab to show the edit form
    tabWidget_->setCurrentWidget(enumTab_);
}

void UserDefinedTypesDialog::saveEnumFromDialog() {
    EnumValue enumVal;

    enumVal.value = enumValueEdit_->text().trimmed();
    enumVal.comment = enumCommentEdit_->toPlainText();

    // Validate enum value
    if (enumVal.value.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Enum value is required.");
        enumValueEdit_->setFocus();
        return;
    }

    // Check for duplicate enum values
    for (int i = 0; i < currentDefinition_.enumValues.size(); ++i) {
        if (currentDefinition_.enumValues[i].value == enumVal.value) {
            // If editing existing value, allow it
            if (enumTable_->currentRow() != i) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Enum value '%1' already exists.").arg(enumVal.value));
                enumValueEdit_->setFocus();
                return;
            }
        }
    }

    int currentRow = enumTable_->currentRow();
    if (currentRow >= 0 && currentRow < currentDefinition_.enumValues.size()) {
        // Update existing enum value
        currentDefinition_.enumValues[currentRow] = enumVal;
    } else {
        // Add new enum value
        currentDefinition_.enumValues.append(enumVal);
    }

    updateEnumTable();
    clearEnumDialog();
    updateButtonStates();
}

void UserDefinedTypesDialog::clearEnumDialog() {
    enumValueEdit_->clear();
    enumCommentEdit_->clear();

    // Clear table selection
    enumTable_->clearSelection();
}

void UserDefinedTypesDialog::updateButtonStates() {
    QString category = typeCategoryCombo_->currentData().toString();

    // Update field buttons
    bool hasFieldSelection = fieldsTable_->currentRow() >= 0;
    bool hasFields = currentDefinition_.fields.size() > 0;

    if (category == "COMPOSITE") {
        editFieldButton_->setEnabled(hasFieldSelection);
        removeFieldButton_->setEnabled(hasFieldSelection);
        moveFieldUpButton_->setEnabled(hasFieldSelection && fieldsTable_->currentRow() > 0);
        moveFieldDownButton_->setEnabled(hasFieldSelection && fieldsTable_->currentRow() < currentDefinition_.fields.size() - 1);
    }

    // Update enum buttons
    bool hasEnumSelection = enumTable_->currentRow() >= 0;
    bool hasEnums = currentDefinition_.enumValues.size() > 0;

    if (category == "ENUM") {
        editEnumButton_->setEnabled(hasEnumSelection);
        removeEnumButton_->setEnabled(hasEnumSelection);
        moveEnumUpButton_->setEnabled(hasEnumSelection && enumTable_->currentRow() > 0);
        moveEnumDownButton_->setEnabled(hasEnumSelection && enumTable_->currentRow() < currentDefinition_.enumValues.size() - 1);
    }
}

void UserDefinedTypesDialog::setTableInfo(const QString& schema, const QString& typeName) {
    // Set the schema and type name for the dialog
    currentSchema_ = schema;
    currentTypeName_ = typeName;

    // Update the dialog title or labels to reflect the current type
    if (!typeName.isEmpty()) {
        setWindowTitle(QString("Edit User-Defined Type: %1.%2").arg(schema, typeName));
    }
}

} // namespace scratchrobin
