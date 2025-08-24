#include "column_editor_dialog.h"
#include <QDialogButtonBox>
#include <QSplitter>
#include <QScrollArea>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QStandardPaths>

namespace scratchrobin {

ColumnEditorDialog::ColumnEditorDialog(QWidget* parent)
    : QDialog(parent), isNewColumn_(true) {
    setupUI();
    setWindowTitle(isNewColumn_ ? "New Column" : "Edit Column");
    setMinimumSize(600, 500);
    resize(700, 600);

    connect(this, &ColumnEditorDialog::columnDefinitionChanged,
            this, &ColumnEditorDialog::updatePreview);
}

void ColumnEditorDialog::setColumnDefinition(const ColumnEditorDefinition& definition, bool isNewColumn) {
    currentDefinition_ = definition;
    isNewColumn_ = isNewColumn;

    setWindowTitle(isNewColumn_ ? "New Column" : QString("Edit Column: %1").arg(definition.name));

    // Basic tab
    columnNameEdit_->setText(definition.name);
    dataTypeCombo_->setCurrentIndex(static_cast<int>(definition.dataType));
    lengthSpin_->setValue(definition.length > 0 ? definition.length : 255);
    precisionSpin_->setValue(definition.precision > 0 ? definition.precision : 10);
    scaleSpin_->setValue(definition.scale > 0 ? definition.scale : 2);
    nullableCheck_->setChecked(definition.nullable);
    defaultValueEdit_->setText(definition.defaultValue);
    commentEdit_->setText(definition.comment);

    // Advanced tab
    autoIncrementCheck_->setChecked(definition.autoIncrement);
    compressedCheck_->setChecked(definition.compressed);

    // Constraints tab
    primaryKeyCheck_->setChecked(definition.primaryKey);
    uniqueCheck_->setChecked(definition.unique);
    checkConstraintEdit_->setText(definition.checkConstraint);
    foreignKeyTableEdit_->setText(definition.foreignKeyTable);
    foreignKeyColumnEdit_->setText(definition.foreignKeyColumn);

    updateDataTypeOptions();
    updateUIForDataType(definition.dataType);
    updatePreview();
}

ColumnEditorDefinition ColumnEditorDialog::getColumnDefinition() const {
    return currentDefinition_;
}

ColumnEditorResult ColumnEditorDialog::showColumnEditor(QWidget* parent,
                                                       const ColumnEditorDefinition& initial,
                                                       bool isNew) {
    ColumnEditorDialog dialog(parent);
    dialog.setColumnDefinition(initial, isNew);

    ColumnEditorResult result;
    result.isNewColumn = isNew;

    if (dialog.exec() == QDialog::Accepted) {
        result.definition = dialog.getColumnDefinition();
        result.applyToAll = false; // Could add a checkbox for this
    } else {
        // User cancelled
        result.definition = ColumnEditorDefinition();
    }

    return result;
}

void ColumnEditorDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Column Editor");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Tab widget
    tabWidget_ = new QTabWidget();
    setupBasicTab();
    setupAdvancedTab();
    setupConstraintsTab();
    setupPreviewTab();

    mainLayout->addWidget(tabWidget_);

    // Action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    validateButton_ = new QPushButton("Validate");
    validateButton_->setIcon(QIcon(":/icons/validate.png"));
    connect(validateButton_, &QPushButton::clicked, this, &ColumnEditorDialog::onValidateColumn);
    buttonLayout->addWidget(validateButton_);

    generateSQLButton_ = new QPushButton("Generate SQL");
    generateSQLButton_->setIcon(QIcon(":/icons/sql.png"));
    connect(generateSQLButton_, &QPushButton::clicked, this, &ColumnEditorDialog::onGenerateSQL);
    buttonLayout->addWidget(generateSQLButton_);

    buttonLayout->addStretch();

    saveButton_ = new QPushButton("Save");
    saveButton_->setIcon(QIcon(":/icons/save.png"));
    saveButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    connect(saveButton_, &QPushButton::clicked, this, &ColumnEditorDialog::onSave);
    buttonLayout->addWidget(saveButton_);

    cancelButton_ = new QPushButton("Cancel");
    cancelButton_->setIcon(QIcon(":/icons/cancel.png"));
    connect(cancelButton_, &QPushButton::clicked, this, &ColumnEditorDialog::onCancel);
    buttonLayout->addWidget(cancelButton_);

    mainLayout->addLayout(buttonLayout);
}

void ColumnEditorDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(basicTab_);

    QGroupBox* columnPropertiesGroup = new QGroupBox("Column Properties");
    QFormLayout* propertiesLayout = new QFormLayout(columnPropertiesGroup);

    // Column name
    columnNameEdit_ = new QLineEdit();
    columnNameEdit_->setPlaceholderText("Enter column name...");
    connect(columnNameEdit_, &QLineEdit::textChanged, this, &ColumnEditorDialog::onColumnNameChanged);
    propertiesLayout->addRow("Column Name:", columnNameEdit_);

    // Data type
    dataTypeCombo_ = new QComboBox();
    dataTypeCombo_->addItem("VARCHAR - Variable length string", static_cast<int>(ColumnDataType::VARCHAR));
    dataTypeCombo_->addItem("TEXT - Unlimited length string", static_cast<int>(ColumnDataType::TEXT));
    dataTypeCombo_->addItem("INTEGER - 32-bit integer", static_cast<int>(ColumnDataType::INTEGER));
    dataTypeCombo_->addItem("BIGINT - 64-bit integer", static_cast<int>(ColumnDataType::BIGINT));
    dataTypeCombo_->addItem("SMALLINT - 16-bit integer", static_cast<int>(ColumnDataType::SMALLINT));
    dataTypeCombo_->addItem("DECIMAL - Fixed precision decimal", static_cast<int>(ColumnDataType::DECIMAL));
    dataTypeCombo_->addItem("NUMERIC - Fixed precision decimal", static_cast<int>(ColumnDataType::NUMERIC));
    dataTypeCombo_->addItem("FLOAT - 32-bit floating point", static_cast<int>(ColumnDataType::FLOAT));
    dataTypeCombo_->addItem("DOUBLE - 64-bit floating point", static_cast<int>(ColumnDataType::DOUBLE));
    dataTypeCombo_->addItem("BOOLEAN - True/false value", static_cast<int>(ColumnDataType::BOOLEAN));
    dataTypeCombo_->addItem("DATE - Date value", static_cast<int>(ColumnDataType::DATE));
    dataTypeCombo_->addItem("TIME - Time value", static_cast<int>(ColumnDataType::TIME));
    dataTypeCombo_->addItem("DATETIME - Date and time", static_cast<int>(ColumnDataType::DATETIME));
    dataTypeCombo_->addItem("TIMESTAMP - Date and time with timezone", static_cast<int>(ColumnDataType::TIMESTAMP));
    dataTypeCombo_->addItem("BINARY - Binary data", static_cast<int>(ColumnDataType::BINARY));
    dataTypeCombo_->addItem("BLOB - Binary large object", static_cast<int>(ColumnDataType::BLOB));
    dataTypeCombo_->addItem("JSON - JSON data", static_cast<int>(ColumnDataType::JSON));
    dataTypeCombo_->addItem("JSONB - Binary JSON data", static_cast<int>(ColumnDataType::JSONB));
    dataTypeCombo_->addItem("UUID - Universally unique identifier", static_cast<int>(ColumnDataType::UUID));
    dataTypeCombo_->addItem("SERIAL - Auto-incrementing integer", static_cast<int>(ColumnDataType::SERIAL));
    dataTypeCombo_->addItem("BIGSERIAL - Auto-incrementing bigint", static_cast<int>(ColumnDataType::BIGSERIAL));
    dataTypeCombo_->addItem("SMALLSERIAL - Auto-incrementing smallint", static_cast<int>(ColumnDataType::SMALLSERIAL));

    connect(dataTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ColumnEditorDialog::onDataTypeChanged);
    propertiesLayout->addRow("Data Type:", dataTypeCombo_);

    // Size specifications
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    QLabel* lengthLabel = new QLabel("Length:");
    lengthSpin_ = new QSpinBox();
    lengthSpin_->setRange(1, 65535);
    lengthSpin_->setValue(255);
    connect(lengthSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColumnEditorDialog::onLengthChanged);
    sizeLayout->addWidget(lengthLabel);
    sizeLayout->addWidget(lengthSpin_);

    sizeLayout->addSpacing(20);

    QLabel* precisionLabel = new QLabel("Precision:");
    precisionSpin_ = new QSpinBox();
    precisionSpin_->setRange(1, 38);
    precisionSpin_->setValue(10);
    connect(precisionSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColumnEditorDialog::onPrecisionChanged);
    sizeLayout->addWidget(precisionLabel);
    sizeLayout->addWidget(precisionSpin_);

    sizeLayout->addSpacing(10);

    QLabel* scaleLabel = new QLabel("Scale:");
    scaleSpin_ = new QSpinBox();
    scaleSpin_->setRange(0, 38);
    scaleSpin_->setValue(2);
    connect(scaleSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColumnEditorDialog::onScaleChanged);
    sizeLayout->addWidget(scaleLabel);
    sizeLayout->addWidget(scaleSpin_);

    sizeLayout->addStretch();
    propertiesLayout->addRow("Size:", sizeLayout);

    // Options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    nullableCheck_ = new QCheckBox("Nullable");
    nullableCheck_->setChecked(true);
    connect(nullableCheck_, &QCheckBox::toggled, this, &ColumnEditorDialog::onNullableChanged);
    optionsLayout->addWidget(nullableCheck_);

    autoIncrementCheck_ = new QCheckBox("Auto Increment");
    connect(autoIncrementCheck_, &QCheckBox::toggled, [this](bool checked) {
        currentDefinition_.autoIncrement = checked;
    });
    optionsLayout->addWidget(autoIncrementCheck_);

    optionsLayout->addStretch();
    propertiesLayout->addRow("Options:", optionsLayout);

    // Default value
    defaultValueEdit_ = new QLineEdit();
    defaultValueEdit_->setPlaceholderText("Enter default value (optional)...");
    connect(defaultValueEdit_, &QLineEdit::textChanged, this, &ColumnEditorDialog::onDefaultValueChanged);
    propertiesLayout->addRow("Default Value:", defaultValueEdit_);

    // Comment
    commentEdit_ = new QLineEdit();
    commentEdit_->setPlaceholderText("Column description (optional)...");
    connect(commentEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        currentDefinition_.comment = text;
    });
    propertiesLayout->addRow("Comment:", commentEdit_);

    layout->addWidget(columnPropertiesGroup);
    layout->addStretch();

    tabWidget_->addTab(basicTab_, "Basic");
}

void ColumnEditorDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab_);

    QGroupBox* advancedOptionsGroup = new QGroupBox("Advanced Options");
    QFormLayout* advancedLayout = new QFormLayout(advancedOptionsGroup);

    // Collation
    collationCombo_ = new QComboBox();
    collationCombo_->addItem("Default", "DEFAULT");
    collationCombo_->addItem("UTF8", "UTF8");
    collationCombo_->addItem("C", "C");
    collationCombo_->addItem("POSIX", "POSIX");
    collationCombo_->addItem("en_US.UTF8", "en_US.UTF8");
    collationCombo_->addItem("en_GB.UTF8", "en_GB.UTF8");
    collationCombo_->addItem("de_DE.UTF8", "de_DE.UTF8");
    collationCombo_->addItem("fr_FR.UTF8", "fr_FR.UTF8");
    advancedLayout->addRow("Collation:", collationCombo_);

    // Character set
    characterSetCombo_ = new QComboBox();
    characterSetCombo_->addItem("Default", "DEFAULT");
    characterSetCombo_->addItem("UTF8", "UTF8");
    characterSetCombo_->addItem("LATIN1", "LATIN1");
    characterSetCombo_->addItem("ASCII", "ASCII");
    advancedLayout->addRow("Character Set:", characterSetCombo_);

    // Storage type
    storageTypeCombo_ = new QComboBox();
    storageTypeCombo_->addItem("Default", "DEFAULT");
    storageTypeCombo_->addItem("PLAIN", "PLAIN");
    storageTypeCombo_->addItem("MAIN", "MAIN");
    storageTypeCombo_->addItem("EXTERNAL", "EXTERNAL");
    storageTypeCombo_->addItem("EXTENDED", "EXTENDED");
    advancedLayout->addRow("Storage Type:", storageTypeCombo_);

    // Compression
    compressedCheck_ = new QCheckBox("Enable compression");
    advancedLayout->addRow("Compression:", compressedCheck_);

    layout->addWidget(advancedOptionsGroup);

    QGroupBox* performanceGroup = new QGroupBox("Performance Tuning");
    QFormLayout* perfLayout = new QFormLayout(performanceGroup);

    QLabel* infoLabel = new QLabel(
        "<b>Performance Tips:</b><br>"
        "• Use appropriate data types for better storage<br>"
        "• Consider indexes for frequently queried columns<br>"
        "• Use TEXT for large strings instead of VARCHAR<br>"
        "• Consider compression for large columns<br>"
        "• Use UUID for distributed systems"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    perfLayout->addRow("", infoLabel);

    layout->addWidget(performanceGroup);
    layout->addStretch();

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void ColumnEditorDialog::setupConstraintsTab() {
    constraintsTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(constraintsTab_);

    // Primary Key and Unique constraints
    QGroupBox* keyConstraintsGroup = new QGroupBox("Key Constraints");
    QVBoxLayout* keyLayout = new QVBoxLayout(keyConstraintsGroup);

    primaryKeyCheck_ = new QCheckBox("Primary Key");
    connect(primaryKeyCheck_, &QCheckBox::toggled, this, &ColumnEditorDialog::onPrimaryKeyChanged);
    keyLayout->addWidget(primaryKeyCheck_);

    uniqueCheck_ = new QCheckBox("Unique");
    connect(uniqueCheck_, &QCheckBox::toggled, [this](bool checked) {
        currentDefinition_.unique = checked;
    });
    keyLayout->addWidget(uniqueCheck_);

    layout->addWidget(keyConstraintsGroup);

    // Check constraint
    QGroupBox* checkConstraintGroup = new QGroupBox("Check Constraint");
    QFormLayout* checkLayout = new QFormLayout(checkConstraintGroup);

    checkConstraintEdit_ = new QLineEdit();
    checkConstraintEdit_->setPlaceholderText("e.g., age >= 0 AND age <= 150");
    connect(checkConstraintEdit_, &QLineEdit::textChanged, [this](const QString& text) {
        currentDefinition_.checkConstraint = text;
    });
    checkLayout->addRow("Expression:", checkConstraintEdit_);

    layout->addWidget(checkConstraintGroup);

    // Foreign Key constraint
    foreignKeyGroup_ = new QGroupBox("Foreign Key Constraint");
    QFormLayout* fkLayout = new QFormLayout(foreignKeyGroup_);

    QCheckBox* enableFKCheck = new QCheckBox("Enable foreign key");
    connect(enableFKCheck, &QCheckBox::toggled, this, &ColumnEditorDialog::onForeignKeyChanged);
    fkLayout->addRow("", enableFKCheck);

    foreignKeyTableEdit_ = new QLineEdit();
    foreignKeyTableEdit_->setPlaceholderText("Referenced table name...");
    foreignKeyTableEdit_->setEnabled(false);
    fkLayout->addRow("Table:", foreignKeyTableEdit_);

    foreignKeyColumnEdit_ = new QLineEdit();
    foreignKeyColumnEdit_->setPlaceholderText("Referenced column name...");
    foreignKeyColumnEdit_->setEnabled(false);
    fkLayout->addRow("Column:", foreignKeyColumnEdit_);

    QHBoxLayout* actionsLayout = new QHBoxLayout();
    QLabel* onDeleteLabel = new QLabel("ON DELETE:");
    onDeleteActionCombo_ = new QComboBox();
    onDeleteActionCombo_->addItems({"NO ACTION", "RESTRICT", "CASCADE", "SET NULL", "SET DEFAULT"});
    onDeleteActionCombo_->setEnabled(false);
    actionsLayout->addWidget(onDeleteLabel);
    actionsLayout->addWidget(onDeleteActionCombo_);

    actionsLayout->addSpacing(20);

    QLabel* onUpdateLabel = new QLabel("ON UPDATE:");
    onUpdateActionCombo_ = new QComboBox();
    onUpdateActionCombo_->addItems({"NO ACTION", "RESTRICT", "CASCADE", "SET NULL", "SET DEFAULT"});
    onUpdateActionCombo_->setEnabled(false);
    actionsLayout->addWidget(onUpdateLabel);
    actionsLayout->addWidget(onUpdateActionCombo_);

    fkLayout->addRow("Actions:", actionsLayout);

    layout->addWidget(foreignKeyGroup_);
    layout->addStretch();

    tabWidget_->addTab(constraintsTab_, "Constraints");
}

void ColumnEditorDialog::setupPreviewTab() {
    previewTab_ = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(previewTab_);

    QGroupBox* previewGroup = new QGroupBox("Column Definition Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);

    previewTextEdit_ = new QTextEdit();
    previewTextEdit_->setReadOnly(true);
    previewTextEdit_->setFontFamily("monospace");
    previewTextEdit_->setStyleSheet("QTextEdit { background-color: #f5f5f5; }");
    previewLayout->addWidget(previewTextEdit_);

    validationLabel_ = new QLabel("Ready to validate");
    validationLabel_->setStyleSheet("color: #666; font-style: italic;");
    previewLayout->addWidget(validationLabel_);

    layout->addWidget(previewGroup);
    layout->addStretch();

    tabWidget_->addTab(previewTab_, "Preview");
}

void ColumnEditorDialog::onDataTypeChanged(int index) {
    if (index < 0) return;

    ColumnDataType dataType = static_cast<ColumnDataType>(dataTypeCombo_->currentData().toInt());
    currentDefinition_.dataType = dataType;

    updateUIForDataType(dataType);
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onColumnNameChanged(const QString& text) {
    currentDefinition_.name = text;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onNullableChanged(bool checked) {
    currentDefinition_.nullable = checked;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onPrimaryKeyChanged(bool checked) {
    currentDefinition_.primaryKey = checked;
    if (checked) {
        // Primary key cannot be nullable
        nullableCheck_->setChecked(false);
        currentDefinition_.nullable = false;
    }
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onForeignKeyChanged(bool checked) {
    bool enabled = checked;
    foreignKeyTableEdit_->setEnabled(enabled);
    foreignKeyColumnEdit_->setEnabled(enabled);
    onDeleteActionCombo_->setEnabled(enabled);
    onUpdateActionCombo_->setEnabled(enabled);

    if (!enabled) {
        foreignKeyTableEdit_->clear();
        foreignKeyColumnEdit_->clear();
        onDeleteActionCombo_->setCurrentIndex(0);
        onUpdateActionCombo_->setCurrentIndex(0);
    }
}

void ColumnEditorDialog::onDefaultValueChanged(const QString& text) {
    currentDefinition_.defaultValue = text;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onLengthChanged(int value) {
    currentDefinition_.length = value;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onPrecisionChanged(int value) {
    currentDefinition_.precision = value;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onScaleChanged(int value) {
    currentDefinition_.scale = value;
    emit columnDefinitionChanged(currentDefinition_);
}

void ColumnEditorDialog::onValidateColumn() {
    validateColumnDefinition();
}

void ColumnEditorDialog::onGenerateSQL() {
    updatePreview();
}

void ColumnEditorDialog::onSave() {
    if (validateColumnDefinition()) {
        accept();
    }
}

void ColumnEditorDialog::onCancel() {
    reject();
}

void ColumnEditorDialog::updateDataTypeOptions() {
    // Update advanced options based on data type
    ColumnDataType dataType = static_cast<ColumnDataType>(dataTypeCombo_->currentData().toInt());

    // Reset advanced options
    collationCombo_->setCurrentIndex(0);
    characterSetCombo_->setCurrentIndex(0);
    storageTypeCombo_->setCurrentIndex(0);
    compressedCheck_->setChecked(false);
}

void ColumnEditorDialog::updateUIForDataType(ColumnDataType dataType) {
    // Show/hide size controls based on data type
    bool showLength = false;
    bool showPrecision = false;
    bool showScale = false;

    switch (dataType) {
        case ColumnDataType::VARCHAR:
        case ColumnDataType::BINARY:
            showLength = true;
            break;
        case ColumnDataType::DECIMAL:
        case ColumnDataType::NUMERIC:
            showPrecision = true;
            showScale = true;
            break;
        default:
            break;
    }

    lengthSpin_->setVisible(showLength);
    lengthSpin_->parentWidget()->layout()->itemAt(0)->widget()->setVisible(showLength);
    precisionSpin_->setVisible(showPrecision);
    precisionSpin_->parentWidget()->layout()->itemAt(2)->widget()->setVisible(showPrecision);
    scaleSpin_->setVisible(showScale);
    scaleSpin_->parentWidget()->layout()->itemAt(4)->widget()->setVisible(showScale);

    // Enable/disable auto increment for appropriate types
    bool canAutoIncrement = (dataType == ColumnDataType::INTEGER ||
                            dataType == ColumnDataType::BIGINT ||
                            dataType == ColumnDataType::SMALLINT ||
                            dataType == ColumnDataType::SERIAL ||
                            dataType == ColumnDataType::BIGSERIAL ||
                            dataType == ColumnDataType::SMALLSERIAL);
    autoIncrementCheck_->setEnabled(canAutoIncrement);

    // Set sensible defaults
    switch (dataType) {
        case ColumnDataType::VARCHAR:
            lengthSpin_->setValue(255);
            break;
        case ColumnDataType::DECIMAL:
        case ColumnDataType::NUMERIC:
            precisionSpin_->setValue(10);
            scaleSpin_->setValue(2);
            break;
        case ColumnDataType::FLOAT:
            lengthSpin_->setValue(4);
            break;
        case ColumnDataType::DOUBLE:
            lengthSpin_->setValue(8);
            break;
        default:
            break;
    }
}

void ColumnEditorDialog::updatePreview() {
    previewTextEdit_->setPlainText(generateColumnSQL());
}

bool ColumnEditorDialog::validateColumnDefinition() {
    QStringList errors;
    QStringList warnings;

    // Check column name
    if (currentDefinition_.name.isEmpty()) {
        errors << "Column name is required";
    } else if (!QRegularExpression("^[a-zA-Z_][a-zA-Z0-9_]*$").match(currentDefinition_.name).hasMatch()) {
        errors << "Column name must start with letter or underscore and contain only letters, numbers, and underscores";
    }

    // Check data type
    ColumnDataType dataType = currentDefinition_.dataType;

    // Check size specifications
    if ((dataType == ColumnDataType::VARCHAR || dataType == ColumnDataType::BINARY) &&
        currentDefinition_.length <= 0) {
        errors << "Length must be greater than 0 for VARCHAR and BINARY types";
    }

    if ((dataType == ColumnDataType::DECIMAL || dataType == ColumnDataType::NUMERIC) &&
        (currentDefinition_.precision <= 0 || currentDefinition_.scale < 0 ||
         currentDefinition_.scale >= currentDefinition_.precision)) {
        errors << "Precision must be greater than 0 and scale must be less than precision for DECIMAL/NUMERIC types";
    }

    // Check constraints
    if (currentDefinition_.primaryKey && !currentDefinition_.nullable) {
        errors << "Primary key columns cannot be nullable";
    }

    // Check foreign key
    if (!currentDefinition_.foreignKeyTable.isEmpty() && currentDefinition_.foreignKeyColumn.isEmpty()) {
        errors << "Foreign key column is required when foreign key table is specified";
    }

    // Warnings
    if (currentDefinition_.name.length() > 63) {
        warnings << "Column name is longer than 63 characters (may not be compatible with all databases)";
    }

    if (dataType == ColumnDataType::TEXT && currentDefinition_.length > 1000) {
        warnings << "Consider using TEXT type for very long strings instead of VARCHAR";
    }

    // Update validation label
    QString validationText;
    if (errors.isEmpty()) {
        validationText = "<span style='color: green;'>✓ Column definition is valid</span>";
        isValid_ = true;
    } else {
        validationText = "<span style='color: red;'>✗ Validation errors:<br>" +
                        errors.join("<br>") + "</span>";
        isValid_ = false;
    }

    if (!warnings.isEmpty()) {
        validationText += "<br><br><span style='color: orange;'>⚠ Warnings:<br>" +
                         warnings.join("<br>") + "</span>";
    }

    validationLabel_->setText(validationText);
    validationLabel_->setTextFormat(Qt::RichText);

    updatePreview();

    return isValid_;
}

QString ColumnEditorDialog::generateColumnSQL() const {
    QStringList parts;

    // Column name
    parts << currentDefinition_.name;

    // Data type with specifications
    QString typeString = getDataTypeString(currentDefinition_.dataType);
    parts << typeString;

    // Constraints
    if (!currentDefinition_.nullable) {
        parts << "NOT NULL";
    }

    if (!currentDefinition_.defaultValue.isEmpty()) {
        parts << "DEFAULT " + currentDefinition_.defaultValue;
    }

    if (currentDefinition_.autoIncrement) {
        // Different databases have different syntax
        parts << "AUTO_INCREMENT"; // MySQL style, could be adapted for other databases
    }

    if (currentDefinition_.primaryKey) {
        parts << "PRIMARY KEY";
    }

    if (currentDefinition_.unique) {
        parts << "UNIQUE";
    }

    if (!currentDefinition_.checkConstraint.isEmpty()) {
        parts << "CHECK (" + currentDefinition_.checkConstraint + ")";
    }

    if (!currentDefinition_.foreignKeyTable.isEmpty()) {
        parts << QString("REFERENCES %1(%2)")
                 .arg(currentDefinition_.foreignKeyTable)
                 .arg(currentDefinition_.foreignKeyColumn);

        if (currentDefinition_.onDeleteAction != "NO ACTION") {
            parts << "ON DELETE " + currentDefinition_.onDeleteAction;
        }

        if (currentDefinition_.onUpdateAction != "NO ACTION") {
            parts << "ON UPDATE " + currentDefinition_.onUpdateAction;
        }
    }

    return parts.join(" ");
}

QString ColumnEditorDialog::getDataTypeString(ColumnDataType type) const {
    switch (type) {
        case ColumnDataType::VARCHAR:
            return QString("VARCHAR(%1)").arg(currentDefinition_.length);
        case ColumnDataType::TEXT:
            return "TEXT";
        case ColumnDataType::INTEGER:
            return "INTEGER";
        case ColumnDataType::BIGINT:
            return "BIGINT";
        case ColumnDataType::SMALLINT:
            return "SMALLINT";
        case ColumnDataType::DECIMAL:
            return QString("DECIMAL(%1,%2)")
                   .arg(currentDefinition_.precision)
                   .arg(currentDefinition_.scale);
        case ColumnDataType::NUMERIC:
            return QString("NUMERIC(%1,%2)")
                   .arg(currentDefinition_.precision)
                   .arg(currentDefinition_.scale);
        case ColumnDataType::FLOAT:
            return "FLOAT";
        case ColumnDataType::DOUBLE:
            return "DOUBLE PRECISION";
        case ColumnDataType::BOOLEAN:
            return "BOOLEAN";
        case ColumnDataType::DATE:
            return "DATE";
        case ColumnDataType::TIME:
            return "TIME";
        case ColumnDataType::DATETIME:
            return "DATETIME";
        case ColumnDataType::TIMESTAMP:
            return "TIMESTAMP";
        case ColumnDataType::BINARY:
            return QString("BINARY(%1)").arg(currentDefinition_.length);
        case ColumnDataType::BLOB:
            return "BLOB";
        case ColumnDataType::JSON:
            return "JSON";
        case ColumnDataType::JSONB:
            return "JSONB";
        case ColumnDataType::UUID:
            return "UUID";
        case ColumnDataType::SERIAL:
            return "SERIAL";
        case ColumnDataType::BIGSERIAL:
            return "BIGSERIAL";
        case ColumnDataType::SMALLSERIAL:
            return "SMALLSERIAL";
        default:
            return "UNKNOWN";
    }
}

} // namespace scratchrobin
