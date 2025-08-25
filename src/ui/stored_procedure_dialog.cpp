#include "stored_procedure_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QFontDatabase>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QRegularExpression>
#include <algorithm>

namespace scratchrobin {

// Simple SQL syntax highlighter
class SQLSyntaxHighlighter : public QSyntaxHighlighter {
public:
    SQLSyntaxHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent) {
        setupRules();
    }

protected:
    void highlightBlock(const QString& text) override {
        for (const HighlightingRule& rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightingRule> highlightingRules;

    void setupRules() {
        // Keywords
        QStringList keywords = {
            "SELECT", "INSERT", "UPDATE", "DELETE", "CREATE", "ALTER", "DROP", "BEGIN", "END", "COMMIT", "ROLLBACK",
            "PROCEDURE", "FUNCTION", "DECLARE", "SET", "IF", "THEN", "ELSE", "WHILE", "FOR", "LOOP",
            "INTO", "FROM", "WHERE", "JOIN", "INNER", "LEFT", "RIGHT", "OUTER", "ON", "GROUP", "BY", "ORDER",
            "HAVING", "UNION", "EXISTS", "NOT", "NULL", "DISTINCT", "AS", "AND", "OR", "IN", "BETWEEN"
        };

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(Qt::blue);
        keywordFormat.setFontWeight(QFont::Bold);

        for (const QString& keyword : keywords) {
            HighlightingRule rule;
            rule.pattern = QRegularExpression("\\b" + keyword + "\\b", QRegularExpression::CaseInsensitiveOption);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        // Comments
        QTextCharFormat commentFormat;
        commentFormat.setForeground(Qt::darkGreen);

        HighlightingRule commentRule;
        commentRule.pattern = QRegularExpression("--[^\n]*");
        commentRule.format = commentFormat;
        highlightingRules.append(commentRule);

        // Strings
        QTextCharFormat stringFormat;
        stringFormat.setForeground(Qt::darkRed);

        HighlightingRule stringRule;
        stringRule.pattern = QRegularExpression("'[^']*'");
        stringRule.format = stringFormat;
        highlightingRules.append(stringRule);
    }
};

StoredProcedureDialog::StoredProcedureDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , procedureNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , typeCombo_(nullptr)
    , returnTypeCombo_(nullptr)
    , languageCombo_(nullptr)
    , commentEdit_(nullptr)
    , parametersTab_(nullptr)
    , parametersLayout_(nullptr)
    , parametersTable_(nullptr)
    , parametersButtonLayout_(nullptr)
    , addParameterButton_(nullptr)
    , editParameterButton_(nullptr)
    , deleteParameterButton_(nullptr)
    , moveUpButton_(nullptr)
    , moveDownButton_(nullptr)
    , parameterGroup_(nullptr)
    , parameterLayout_(nullptr)
    , paramNameEdit_(nullptr)
    , paramDataTypeCombo_(nullptr)
    , paramLengthSpin_(nullptr)
    , paramPrecisionSpin_(nullptr)
    , paramScaleSpin_(nullptr)
    , paramDirectionCombo_(nullptr)
    , paramDefaultEdit_(nullptr)
    , paramCommentEdit_(nullptr)
    , editorTab_(nullptr)
    , editorLayout_(nullptr)
    , editorToolbar_(nullptr)
    , formatButton_(nullptr)
    , validateButton_(nullptr)
    , previewButton_(nullptr)
    , templateButton_(nullptr)
    , templateMenu_(nullptr)
    , codeEditor_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
    , deterministicCheck_(nullptr)
    , securityTypeCombo_(nullptr)
    , sqlModeEdit_(nullptr)
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
    setWindowTitle("Stored Procedure Editor");
    setModal(true);
    resize(900, 700);
}

void StoredProcedureDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupParametersTab();
    setupEditorTab();
    setupAdvancedTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &StoredProcedureDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void StoredProcedureDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    procedureNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    typeCombo_ = new QComboBox(basicTab_);
    returnTypeCombo_ = new QComboBox(basicTab_);
    languageCombo_ = new QComboBox(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Populate combo boxes
    typeCombo_->addItem("PROCEDURE", "PROCEDURE");
    typeCombo_->addItem("FUNCTION", "FUNCTION");

    populateDataTypes();
    populateLanguages();

    basicLayout_->addRow("Name:", procedureNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Type:", typeCombo_);
    basicLayout_->addRow("Return Type:", returnTypeCombo_);
    basicLayout_->addRow("Language:", languageCombo_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(procedureNameEdit_, &QLineEdit::textChanged, this, &StoredProcedureDialog::onProcedureNameChanged);
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StoredProcedureDialog::onLanguageChanged);
    connect(languageCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StoredProcedureDialog::onLanguageChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void StoredProcedureDialog::setupParametersTab() {
    parametersTab_ = new QWidget();
    parametersLayout_ = new QVBoxLayout(parametersTab_);

    // Parameter table
    setupParameterTable();
    parametersLayout_->addWidget(parametersTable_);

    // Parameter buttons
    parametersButtonLayout_ = new QHBoxLayout();
    addParameterButton_ = new QPushButton("Add Parameter", parametersTab_);
    editParameterButton_ = new QPushButton("Edit Parameter", parametersTab_);
    deleteParameterButton_ = new QPushButton("Delete Parameter", parametersTab_);
    moveUpButton_ = new QPushButton("Move Up", parametersTab_);
    moveDownButton_ = new QPushButton("Move Down", parametersTab_);

    parametersButtonLayout_->addWidget(addParameterButton_);
    parametersButtonLayout_->addWidget(editParameterButton_);
    parametersButtonLayout_->addWidget(deleteParameterButton_);
    parametersButtonLayout_->addStretch();
    parametersButtonLayout_->addWidget(moveUpButton_);
    parametersButtonLayout_->addWidget(moveDownButton_);

    parametersLayout_->addLayout(parametersButtonLayout_);

    // Parameter edit dialog (embedded)
    setupParameterDialog();
    parametersLayout_->addWidget(parameterGroup_);

    // Connect signals
    connect(addParameterButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onAddParameter);
    connect(editParameterButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onEditParameter);
    connect(deleteParameterButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onDeleteParameter);
    connect(moveUpButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onMoveParameterUp);
    connect(moveDownButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onMoveParameterDown);
    connect(parametersTable_, &QTableWidget::itemSelectionChanged, this, &StoredProcedureDialog::onParameterSelectionChanged);

    tabWidget_->addTab(parametersTab_, "Parameters");
}

void StoredProcedureDialog::setupEditorTab() {
    editorTab_ = new QWidget();
    editorLayout_ = new QVBoxLayout(editorTab_);

    // Toolbar
    editorToolbar_ = new QHBoxLayout();

    formatButton_ = new QPushButton("Format", editorTab_);
    validateButton_ = new QPushButton("Validate", editorTab_);
    previewButton_ = new QPushButton("Preview", editorTab_);
    templateButton_ = new QPushButton("Templates", editorTab_);

    // Template menu
    templateMenu_ = new QMenu(templateButton_);
    populateTemplates();
    templateButton_->setMenu(templateMenu_);

    editorToolbar_->addWidget(formatButton_);
    editorToolbar_->addWidget(validateButton_);
    editorToolbar_->addWidget(previewButton_);
    editorToolbar_->addWidget(templateButton_);
    editorToolbar_->addStretch();

    editorLayout_->addLayout(editorToolbar_);

    // Code editor
    setupCodeEditor();
    editorLayout_->addWidget(codeEditor_);

    // Connect signals
    connect(formatButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onFormatSQL);
    connect(validateButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onValidateSQL);
    connect(previewButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onPreviewSQL);

    tabWidget_->addTab(editorTab_, "Editor");
}

void StoredProcedureDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Procedure Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    deterministicCheck_ = new QCheckBox("Deterministic", advancedTab_);
    securityTypeCombo_ = new QComboBox(advancedTab_);
    sqlModeEdit_ = new QLineEdit(advancedTab_);

    securityTypeCombo_->addItem("DEFINER", "DEFINER");
    securityTypeCombo_->addItem("INVOKER", "INVOKER");

    optionsLayout_->addRow("", deterministicCheck_);
    optionsLayout_->addRow("Security Type:", securityTypeCombo_);
    optionsLayout_->addRow("SQL Mode:", sqlModeEdit_);

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    // Connect signals
    connect(securityTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StoredProcedureDialog::onSecurityTypeChanged);

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void StoredProcedureDialog::setupSQLTab() {
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
    connect(generateSqlButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &StoredProcedureDialog::onValidateSQL);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void StoredProcedureDialog::setupParameterTable() {
    parametersTable_ = new QTableWidget(parametersTab_);
    parametersTable_->setColumnCount(6);
    parametersTable_->setHorizontalHeaderLabels({
        "Name", "Data Type", "Direction", "Length", "Default", "Comment"
    });

    parametersTable_->horizontalHeader()->setStretchLastSection(true);
    parametersTable_->verticalHeader()->setDefaultSectionSize(25);
    parametersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    parametersTable_->setAlternatingRowColors(true);
}

void StoredProcedureDialog::setupParameterDialog() {
    parameterGroup_ = new QGroupBox("Parameter Properties", parametersTab_);
    parameterLayout_ = new QFormLayout(parameterGroup_);

    paramNameEdit_ = new QLineEdit(parameterGroup_);
    paramDataTypeCombo_ = new QComboBox(parameterGroup_);
    paramLengthSpin_ = new QSpinBox(parameterGroup_);
    paramPrecisionSpin_ = new QSpinBox(parameterGroup_);
    paramScaleSpin_ = new QSpinBox(parameterGroup_);
    paramDirectionCombo_ = new QComboBox(parameterGroup_);
    paramDefaultEdit_ = new QLineEdit(parameterGroup_);
    paramCommentEdit_ = new QTextEdit(parameterGroup_);
    paramCommentEdit_->setMaximumHeight(40);

    // Populate parameter direction combo
    paramDirectionCombo_->addItem("IN", "IN");
    paramDirectionCombo_->addItem("OUT", "OUT");
    paramDirectionCombo_->addItem("INOUT", "INOUT");

    // Populate data types
    populateDataTypes();
    paramDataTypeCombo_->addItems(QStringList()); // Will be populated by populateDataTypes()

    parameterLayout_->addRow("Name:", paramNameEdit_);
    parameterLayout_->addRow("Data Type:", paramDataTypeCombo_);
    parameterLayout_->addRow("Length:", paramLengthSpin_);
    parameterLayout_->addRow("Precision:", paramPrecisionSpin_);
    parameterLayout_->addRow("Scale:", paramScaleSpin_);
    parameterLayout_->addRow("Direction:", paramDirectionCombo_);
    parameterLayout_->addRow("Default Value:", paramDefaultEdit_);
    parameterLayout_->addRow("Comment:", paramCommentEdit_);
}

void StoredProcedureDialog::setupCodeEditor() {
    codeEditor_ = new QPlainTextEdit(editorTab_);

    // Set up monospace font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    codeEditor_->setFont(font);

    // Set up syntax highlighting
    new SQLSyntaxHighlighter(codeEditor_->document());

    // Set up line numbers (optional - could be enhanced)
    codeEditor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    codeEditor_->setTabStopDistance(40); // 4 spaces
}

void StoredProcedureDialog::populateDataTypes() {
    if (!returnTypeCombo_) return;

    returnTypeCombo_->clear();

    // Common SQL data types
    QStringList dataTypes = {
        "VOID", "INT", "BIGINT", "SMALLINT", "TINYINT", "VARCHAR", "TEXT", "DECIMAL", "FLOAT", "DOUBLE",
        "BOOLEAN", "DATE", "TIME", "DATETIME", "TIMESTAMP", "BLOB", "CLOB", "JSON"
    };

    returnTypeCombo_->addItems(dataTypes);

    if (paramDataTypeCombo_) {
        paramDataTypeCombo_->clear();
        paramDataTypeCombo_->addItems(dataTypes);
    }
}

void StoredProcedureDialog::populateLanguages() {
    if (!languageCombo_) return;

    languageCombo_->clear();
    languageCombo_->addItem("SQL", "SQL");
    languageCombo_->addItem("PL/SQL", "PLSQL");
    languageCombo_->addItem("PL/pgSQL", "PLPGSQL");
    languageCombo_->addItem("T-SQL", "TSQL");
    languageCombo_->addItem("MySQL", "MYSQL");
}

void StoredProcedureDialog::populateTemplates() {
    if (!templateMenu_) return;

    templateMenu_->clear();

    // Common procedure templates
    QStringList templates = {
        "Empty Procedure", "Select Procedure", "Insert Procedure", "Update Procedure",
        "Delete Procedure", "CRUD Procedure", "Validation Function", "Calculation Function"
    };

    for (const QString& templateName : templates) {
        QAction* action = templateMenu_->addAction(templateName);
        connect(action, &QAction::triggered, this, [this, templateName]() {
            applyTemplate(templateName);
        });
    }

    templateMenu_->addSeparator();
    templateMenu_->addAction("Load from File...", this, &StoredProcedureDialog::onLoadTemplate);
    templateMenu_->addAction("Save as Template...", this, &StoredProcedureDialog::onSaveTemplate);
}

void StoredProcedureDialog::setProcedureDefinition(const StoredProcedureDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    procedureNameEdit_->setText(definition.name);
    schemaEdit_->setText(definition.schema);
    commentEdit_->setPlainText(definition.comment);
    codeEditor_->setPlainText(definition.body);

    // Set type
    if (!definition.type.isEmpty()) {
        int index = typeCombo_->findData(definition.type);
        if (index >= 0) typeCombo_->setCurrentIndex(index);
    }

    // Set return type
    if (!definition.returnType.isEmpty()) {
        int index = returnTypeCombo_->findText(definition.returnType);
        if (index >= 0) returnTypeCombo_->setCurrentIndex(index);
    }

    // Set language
    if (!definition.language.isEmpty()) {
        int index = languageCombo_->findData(definition.language);
        if (index >= 0) languageCombo_->setCurrentIndex(index);
    }

    // Set security type
    if (!definition.securityType.isEmpty()) {
        int index = securityTypeCombo_->findData(definition.securityType);
        if (index >= 0) securityTypeCombo_->setCurrentIndex(index);
    }

    // Set other options
    deterministicCheck_->setChecked(definition.isDeterministic);
    sqlModeEdit_->setText(definition.sqlMode);

    // Update tables
    updateParameterTable();
}

StoredProcedureDefinition StoredProcedureDialog::getProcedureDefinition() const {
    StoredProcedureDefinition definition = currentDefinition_;

    definition.name = procedureNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.comment = commentEdit_->toPlainText();
    definition.body = codeEditor_->toPlainText();

    // Get combo box values
    definition.type = typeCombo_->currentData().toString();
    definition.returnType = returnTypeCombo_->currentText();
    definition.language = languageCombo_->currentData().toString();
    definition.securityType = securityTypeCombo_->currentData().toString();

    // Get other options
    definition.isDeterministic = deterministicCheck_->isChecked();
    definition.sqlMode = sqlModeEdit_->text();

    return definition;
}

void StoredProcedureDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Stored Procedure");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Stored Procedure");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void StoredProcedureDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update UI elements based on database type
    switch (type) {
        case DatabaseType::POSTGRESQL:
            languageCombo_->setCurrentText("PL/pgSQL");
            break;
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            languageCombo_->setCurrentText("SQL");
            break;
        case DatabaseType::ORACLE:
            languageCombo_->setCurrentText("PL/SQL");
            break;
        case DatabaseType::SQLSERVER:
        case DatabaseType::MSSQL:
            languageCombo_->setCurrentText("T-SQL");
            break;
        default:
            languageCombo_->setCurrentText("SQL");
            break;
    }
}

void StoredProcedureDialog::loadExistingProcedure(const QString& schema, const QString& procedureName) {
    // This would load an existing procedure definition
    // For now, just set the basic properties
    originalSchema_ = schema;
    originalProcedureName_ = procedureName;

    procedureNameEdit_->setText(procedureName);
    schemaEdit_->setText(schema);
    setEditMode(true);

    // TODO: Load actual procedure definition from database
}

void StoredProcedureDialog::accept() {
    if (validateProcedure()) {
        emit procedureSaved(getProcedureDefinition());
        QDialog::accept();
    }
}

void StoredProcedureDialog::reject() {
    QDialog::reject();
}

// Parameter management slots
void StoredProcedureDialog::onAddParameter() {
    clearParameterDialog();
    // Switch to parameters tab to show the edit form
    tabWidget_->setCurrentWidget(parametersTab_);
}

void StoredProcedureDialog::onEditParameter() {
    int row = parametersTable_->currentRow();
    if (row >= 0) {
        loadParameterToDialog(row);
    }
}

void StoredProcedureDialog::onDeleteParameter() {
    int row = parametersTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.parameters.removeAt(row);
        updateParameterTable();
        updateButtonStates();
    }
}

void StoredProcedureDialog::onMoveParameterUp() {
    int row = parametersTable_->currentRow();
    if (row > 0) {
        currentDefinition_.parameters.move(row, row - 1);
        updateParameterTable();
        parametersTable_->setCurrentCell(row - 1, 0);
    }
}

void StoredProcedureDialog::onMoveParameterDown() {
    int row = parametersTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.parameters.size() - 1) {
        currentDefinition_.parameters.move(row, row + 1);
        updateParameterTable();
        parametersTable_->setCurrentCell(row + 1, 0);
    }
}

void StoredProcedureDialog::onParameterSelectionChanged() {
    updateButtonStates();
}

// Editor actions
void StoredProcedureDialog::onFormatSQL() {
    // Basic SQL formatting
    QString sql = codeEditor_->toPlainText();
    QStringList lines = sql.split('\n');
    QStringList formattedLines;

    int indentLevel = 0;
    for (QString line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Decrease indent for END, closing braces, etc.
        if (trimmed.toUpper().startsWith("END") ||
            trimmed.toUpper().startsWith("ELSE") ||
            trimmed.contains("}")) {
            indentLevel = qMax(0, indentLevel - 1);
        }

        // Add proper indentation
        if (indentLevel > 0) {
            formattedLines.append(QString(indentLevel * 4, ' ') + trimmed);
        } else {
            formattedLines.append(trimmed);
        }

        // Increase indent for BEGIN, IF, loops, etc.
        if (trimmed.toUpper().contains("BEGIN") ||
            (trimmed.toUpper().contains("IF") && !trimmed.toUpper().contains("END IF")) ||
            trimmed.toUpper().startsWith("WHILE") ||
            trimmed.toUpper().startsWith("FOR") ||
            trimmed.toUpper().startsWith("LOOP") ||
            trimmed.contains("{")) {
            indentLevel++;
        }
    }

    codeEditor_->setPlainText(formattedLines.join('\n'));
}

void StoredProcedureDialog::onValidateSQL() {
    QString sql = codeEditor_->toPlainText();
    if (sql.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Procedure body cannot be empty.");
        return;
    }

    // Basic validation - check for common syntax errors
    QStringList errors;

    // Check for unmatched BEGIN/END
    int beginCount = sql.count(QRegularExpression("\\bBEGIN\\b", QRegularExpression::CaseInsensitiveOption));
    int endCount = sql.count(QRegularExpression("\\bEND\\b", QRegularExpression::CaseInsensitiveOption));

    if (beginCount != endCount) {
        errors.append("Unmatched BEGIN/END blocks");
    }

    // Check for basic SQL syntax
    if (!sql.contains(QRegularExpression("\\bBEGIN\\b", QRegularExpression::CaseInsensitiveOption)) &&
        !sql.contains(QRegularExpression("\\bSELECT\\b", QRegularExpression::CaseInsensitiveOption))) {
        errors.append("Procedure body appears to be empty or invalid");
    }

    if (errors.isEmpty()) {
        QMessageBox::information(this, "Validation", "SQL syntax appears valid.");
    } else {
        QMessageBox::warning(this, "Validation Errors", errors.join("\n"));
    }
}

void StoredProcedureDialog::onPreviewSQL() {
    if (validateProcedure()) {
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

void StoredProcedureDialog::onGenerateTemplate() {
    // Template generation is handled by the menu
}

void StoredProcedureDialog::onLoadTemplate() {
    // TODO: Implement template loading from file
    QMessageBox::information(this, "Load Template", "Template loading will be implemented in the next update.");
}

void StoredProcedureDialog::onSaveTemplate() {
    // TODO: Implement template saving
    QMessageBox::information(this, "Save Template", "Template saving will be implemented in the next update.");
}

// Settings slots
void StoredProcedureDialog::onLanguageChanged(int index) {
    // Update syntax highlighting based on language
    // For now, just keep SQL highlighting
}

void StoredProcedureDialog::onSecurityTypeChanged(int index) {
    // Handle security type changes if needed
}

void StoredProcedureDialog::onProcedureNameChanged(const QString& name) {
    // Validate procedure name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void StoredProcedureDialog::updateParameterTable() {
    parametersTable_->setRowCount(currentDefinition_.parameters.size());

    for (int i = 0; i < currentDefinition_.parameters.size(); ++i) {
        const ProcedureParameterDefinition& param = currentDefinition_.parameters[i];

        parametersTable_->setItem(i, 0, new QTableWidgetItem(param.name));
        parametersTable_->setItem(i, 1, new QTableWidgetItem(param.dataType));
        parametersTable_->setItem(i, 2, new QTableWidgetItem(param.direction));
        parametersTable_->setItem(i, 3, new QTableWidgetItem(param.length > 0 ? QString::number(param.length) : ""));
        parametersTable_->setItem(i, 4, new QTableWidgetItem(param.defaultValue));
        parametersTable_->setItem(i, 5, new QTableWidgetItem(param.comment));
    }
}

bool StoredProcedureDialog::validateProcedure() {
    QString procedureName = procedureNameEdit_->text().trimmed();

    if (procedureName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Procedure name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        procedureNameEdit_->setFocus();
        return false;
    }

    QString body = codeEditor_->toPlainText().trimmed();
    if (body.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Procedure body cannot be empty.");
        tabWidget_->setCurrentWidget(editorTab_);
        codeEditor_->setFocus();
        return false;
    }

    // Check for duplicate parameter names
    QSet<QString> paramNames;
    for (const ProcedureParameterDefinition& param : currentDefinition_.parameters) {
        if (paramNames.contains(param.name)) {
            QMessageBox::warning(this, "Validation Error",
                QString("Duplicate parameter name: %1").arg(param.name));
            tabWidget_->setCurrentWidget(parametersTab_);
            return false;
        }
        paramNames.insert(param.name);
    }

    return true;
}

QString StoredProcedureDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString procedureType = typeCombo_->currentData().toString();
    QString fullName = procedureNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullName = QString("%1.%2").arg(schemaEdit_->text(), fullName);
    }

    if (procedureType == "PROCEDURE") {
        sqlParts.append(QString("CREATE PROCEDURE %1").arg(fullName));
    } else {
        sqlParts.append(QString("CREATE FUNCTION %1").arg(fullName));
        if (!returnTypeCombo_->currentText().isEmpty()) {
            sqlParts.append(QString("RETURNS %1").arg(returnTypeCombo_->currentText()));
        }
    }

    // Parameters
    if (!currentDefinition_.parameters.isEmpty()) {
        QStringList paramList;
        for (const ProcedureParameterDefinition& param : currentDefinition_.parameters) {
            QString paramDef = param.name;
            if (procedureType == "PROCEDURE") {
                paramDef += " " + param.direction;
            }
            paramDef += " " + param.dataType;

            if (param.length > 0) {
                if (param.precision > 0) {
                    paramDef += QString("(%1,%2)").arg(param.length).arg(param.precision);
                } else {
                    paramDef += QString("(%1)").arg(param.length);
                }
            }

            if (!param.defaultValue.isEmpty()) {
                paramDef += " DEFAULT " + param.defaultValue;
            }

            paramList.append(paramDef);
        }

        if (procedureType == "PROCEDURE") {
            sqlParts.append("(" + paramList.join(", ") + ")");
        } else {
            sqlParts.append(paramList.join(", "));
        }
    } else if (procedureType == "PROCEDURE") {
        sqlParts.append("()");
    }

    // Language and options
    QStringList options;
    if (languageCombo_->currentData().toString() != "SQL") {
        options.append(QString("LANGUAGE %1").arg(languageCombo_->currentData().toString()));
    }

    if (securityTypeCombo_->currentData().toString() != "DEFINER") {
        options.append(QString("SECURITY %1").arg(securityTypeCombo_->currentData().toString()));
    }

    if (deterministicCheck_->isChecked()) {
        options.append("DETERMINISTIC");
    }

    if (!sqlModeEdit_->text().isEmpty()) {
        options.append(QString("SQL MODE '%1'").arg(sqlModeEdit_->text()));
    }

    if (!options.isEmpty()) {
        sqlParts.append(options.join("\n"));
    }

    // Body
    QString body = codeEditor_->toPlainText().trimmed();
    if (!body.isEmpty()) {
        sqlParts.append("AS");
        sqlParts.append("$$");
        sqlParts.append(body);
        sqlParts.append("$$");
    }

    return sqlParts.join("\n");
}

QString StoredProcedureDialog::generateAlterSQL() const {
    // For now, return a simple message
    // In a full implementation, this would generate ALTER statements
    return QString("-- ALTER PROCEDURE statements would be generated here\n-- Original procedure: %1.%2")
        .arg(originalSchema_, originalProcedureName_);
}

QString StoredProcedureDialog::generateDropSQL() const {
    QString procedureName = procedureNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        procedureName = QString("%1.%2").arg(schemaEdit_->text(), procedureName);
    }

    QString procedureType = typeCombo_->currentData().toString();
    if (procedureType == "PROCEDURE") {
        return QString("DROP PROCEDURE IF EXISTS %1;").arg(procedureName);
    } else {
        return QString("DROP FUNCTION IF EXISTS %1;").arg(procedureName);
    }
}

void StoredProcedureDialog::loadParameterToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.parameters.size()) {
        return;
    }

    const ProcedureParameterDefinition& param = currentDefinition_.parameters[row];

    paramNameEdit_->setText(param.name);

    int typeIndex = paramDataTypeCombo_->findText(param.dataType);
    if (typeIndex >= 0) {
        paramDataTypeCombo_->setCurrentIndex(typeIndex);
    } else {
        paramDataTypeCombo_->setCurrentText(param.dataType);
    }

    paramLengthSpin_->setValue(param.length);
    paramPrecisionSpin_->setValue(param.precision);
    paramScaleSpin_->setValue(param.scale);

    int directionIndex = paramDirectionCombo_->findData(param.direction);
    if (directionIndex >= 0) {
        paramDirectionCombo_->setCurrentIndex(directionIndex);
    }

    paramDefaultEdit_->setText(param.defaultValue);
    paramCommentEdit_->setPlainText(param.comment);

    // Switch to parameters tab to show the edit form
    tabWidget_->setCurrentWidget(parametersTab_);
}

void StoredProcedureDialog::saveParameterFromDialog() {
    ProcedureParameterDefinition param;

    param.name = paramNameEdit_->text().trimmed();
    param.dataType = paramDataTypeCombo_->currentText();
    param.length = paramLengthSpin_->value();
    param.precision = paramPrecisionSpin_->value();
    param.scale = paramScaleSpin_->value();
    param.direction = paramDirectionCombo_->currentData().toString();
    param.defaultValue = paramDefaultEdit_->text();
    param.comment = paramCommentEdit_->toPlainText();

    // Validate parameter name
    if (param.name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Parameter name is required.");
        paramNameEdit_->setFocus();
        return;
    }

    // Check for duplicate parameter names
    for (int i = 0; i < currentDefinition_.parameters.size(); ++i) {
        if (currentDefinition_.parameters[i].name == param.name) {
            // If editing existing parameter, allow it
            if (parametersTable_->currentRow() != i) {
                QMessageBox::warning(this, "Validation Error",
                    QString("Parameter name '%1' already exists.").arg(param.name));
                paramNameEdit_->setFocus();
                return;
            }
        }
    }

    int currentRow = parametersTable_->currentRow();
    if (currentRow >= 0 && currentRow < currentDefinition_.parameters.size()) {
        // Update existing parameter
        currentDefinition_.parameters[currentRow] = param;
    } else {
        // Add new parameter
        currentDefinition_.parameters.append(param);
    }

    updateParameterTable();
    clearParameterDialog();
    updateButtonStates();
}

void StoredProcedureDialog::clearParameterDialog() {
    paramNameEdit_->clear();
    paramDataTypeCombo_->setCurrentIndex(0);
    paramLengthSpin_->setValue(0);
    paramPrecisionSpin_->setValue(0);
    paramScaleSpin_->setValue(0);
    paramDirectionCombo_->setCurrentIndex(0); // IN
    paramDefaultEdit_->clear();
    paramCommentEdit_->clear();

    // Clear table selection
    parametersTable_->clearSelection();
}

void StoredProcedureDialog::applyTemplate(const QString& templateName) {
    QString templateCode;

    if (templateName == "Empty Procedure") {
        templateCode = "BEGIN\n    -- Procedure body goes here\n    NULL;\nEND";
    } else if (templateName == "Select Procedure") {
        templateCode = "BEGIN\n    -- Select data procedure\n    SELECT * FROM table_name;\nEND";
    } else if (templateName == "Insert Procedure") {
        templateCode = "BEGIN\n    -- Insert data procedure\n    INSERT INTO table_name (column1, column2)\n    VALUES (value1, value2);\nEND";
    } else if (templateName == "Update Procedure") {
        templateCode = "BEGIN\n    -- Update data procedure\n    UPDATE table_name\n    SET column1 = value1\n    WHERE condition;\nEND";
    } else if (templateName == "Delete Procedure") {
        templateCode = "BEGIN\n    -- Delete data procedure\n    DELETE FROM table_name\n    WHERE condition;\nEND";
    } else if (templateName == "Validation Function") {
        templateCode = "BEGIN\n    -- Validation function\n    IF condition THEN\n        RETURN TRUE;\n    ELSE\n        RETURN FALSE;\n    END IF;\nEND";
    } else if (templateName == "Calculation Function") {
        templateCode = "BEGIN\n    -- Calculation function\n    DECLARE result datatype;\n    -- Calculation logic here\n    RETURN result;\nEND";
    }

    codeEditor_->setPlainText(templateCode);
}

void StoredProcedureDialog::saveAsTemplate(const QString& templateName) {
    // TODO: Implement template saving
    QMessageBox::information(this, "Save Template", "Template saving will be implemented in the next update.");
}

void StoredProcedureDialog::updateButtonStates() {
    bool hasSelection = parametersTable_->currentRow() >= 0;
    bool hasParameters = currentDefinition_.parameters.size() > 0;

    editParameterButton_->setEnabled(hasSelection);
    deleteParameterButton_->setEnabled(hasSelection);
    moveUpButton_->setEnabled(hasSelection && parametersTable_->currentRow() > 0);
    moveDownButton_->setEnabled(hasSelection && parametersTable_->currentRow() < currentDefinition_.parameters.size() - 1);
}

} // namespace scratchrobin
