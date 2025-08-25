#include "function_editor_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTableWidget>
#include <QMenu>
#include <QAction>

namespace scratchrobin {

// SQL syntax highlighter (reused from other dialogs)
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
            "SELECT", "FROM", "WHERE", "JOIN", "INNER", "LEFT", "RIGHT", "OUTER", "ON",
            "GROUP", "BY", "HAVING", "ORDER", "UNION", "DISTINCT", "AS", "AND", "OR", "NOT",
            "CREATE", "FUNCTION", "RETURN", "RETURNS", "DECLARE", "SET", "IF", "THEN", "ELSE",
            "WHILE", "FOR", "LOOP", "CASE", "WHEN", "BEGIN", "END", "LANGUAGE"
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

FunctionEditorDialog::FunctionEditorDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , functionNameEdit_(nullptr)
    , schemaEdit_(nullptr)
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
    setWindowTitle("Function Editor");
    setModal(true);
    resize(900, 700);
}

void FunctionEditorDialog::setupUI() {
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
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &FunctionEditorDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void FunctionEditorDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    functionNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    returnTypeCombo_ = new QComboBox(basicTab_);
    languageCombo_ = new QComboBox(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Populate combo boxes
    populateDataTypes();
    populateLanguages();

    basicLayout_->addRow("Function Name:", functionNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Return Type:", returnTypeCombo_);
    basicLayout_->addRow("Language:", languageCombo_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(functionNameEdit_, &QLineEdit::textChanged, this, &FunctionEditorDialog::onFunctionNameChanged);
    connect(languageCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FunctionEditorDialog::onLanguageChanged);
    connect(returnTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FunctionEditorDialog::onReturnTypeChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void FunctionEditorDialog::setupParametersTab() {
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
    connect(addParameterButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onAddParameter);
    connect(editParameterButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onEditParameter);
    connect(deleteParameterButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onDeleteParameter);
    connect(moveUpButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onMoveParameterUp);
    connect(moveDownButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onMoveParameterDown);
    connect(parametersTable_, &QTableWidget::itemSelectionChanged, this, &FunctionEditorDialog::onParameterSelectionChanged);

    tabWidget_->addTab(parametersTab_, "Parameters");
}

void FunctionEditorDialog::setupEditorTab() {
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
    connect(formatButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onFormatSQL);
    connect(validateButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onValidateSQL);
    connect(previewButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onPreviewSQL);

    tabWidget_->addTab(editorTab_, "Editor");
}

void FunctionEditorDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Function Options", advancedTab_);
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
    connect(securityTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FunctionEditorDialog::onSecurityTypeChanged);

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void FunctionEditorDialog::setupSQLTab() {
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
    connect(generateSqlButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &FunctionEditorDialog::onValidateSQL);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void FunctionEditorDialog::setupParameterTable() {
    parametersTable_ = new QTableWidget(parametersTab_);
    parametersTable_->setColumnCount(5);
    parametersTable_->setHorizontalHeaderLabels({
        "Name", "Data Type", "Length", "Default", "Comment"
    });

    parametersTable_->horizontalHeader()->setStretchLastSection(true);
    parametersTable_->verticalHeader()->setDefaultSectionSize(25);
    parametersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    parametersTable_->setAlternatingRowColors(true);
}

void FunctionEditorDialog::setupParameterDialog() {
    parameterGroup_ = new QGroupBox("Parameter Properties", parametersTab_);
    parameterLayout_ = new QFormLayout(parameterGroup_);

    paramNameEdit_ = new QLineEdit(parameterGroup_);
    paramDataTypeCombo_ = new QComboBox(parameterGroup_);
    paramLengthSpin_ = new QSpinBox(parameterGroup_);
    paramPrecisionSpin_ = new QSpinBox(parameterGroup_);
    paramScaleSpin_ = new QSpinBox(parameterGroup_);
    paramDefaultEdit_ = new QLineEdit(parameterGroup_);
    paramCommentEdit_ = new QTextEdit(parameterGroup_);
    paramCommentEdit_->setMaximumHeight(40);

    // Populate data types
    populateDataTypes();
    paramDataTypeCombo_->addItems(QStringList()); // Will be populated by populateDataTypes()

    parameterLayout_->addRow("Name:", paramNameEdit_);
    parameterLayout_->addRow("Data Type:", paramDataTypeCombo_);
    parameterLayout_->addRow("Length:", paramLengthSpin_);
    parameterLayout_->addRow("Precision:", paramPrecisionSpin_);
    parameterLayout_->addRow("Scale:", paramScaleSpin_);
    parameterLayout_->addRow("Default Value:", paramDefaultEdit_);
    parameterLayout_->addRow("Comment:", paramCommentEdit_);
}

void FunctionEditorDialog::setupCodeEditor() {
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

    // Set placeholder text
    codeEditor_->setPlaceholderText("-- Function body SQL code here\n-- Must contain a RETURN statement");
}

void FunctionEditorDialog::populateDataTypes() {
    if (!returnTypeCombo_ || !paramDataTypeCombo_) return;

    QStringList dataTypes = {
        "VOID", "INT", "BIGINT", "SMALLINT", "TINYINT", "VARCHAR", "TEXT", "DECIMAL", "FLOAT", "DOUBLE",
        "BOOLEAN", "DATE", "TIME", "DATETIME", "TIMESTAMP", "BLOB", "CLOB", "JSON"
    };

    returnTypeCombo_->clear();
    returnTypeCombo_->addItems(dataTypes);

    paramDataTypeCombo_->clear();
    paramDataTypeCombo_->addItems(dataTypes);
}

void FunctionEditorDialog::populateLanguages() {
    if (!languageCombo_) return;

    languageCombo_->clear();
    languageCombo_->addItem("SQL", "SQL");
    languageCombo_->addItem("PL/SQL", "PLSQL");
    languageCombo_->addItem("PL/pgSQL", "PLPGSQL");
    languageCombo_->addItem("T-SQL", "TSQL");
    languageCombo_->addItem("MySQL", "MYSQL");
}

void FunctionEditorDialog::populateTemplates() {
    if (!templateMenu_) return;

    templateMenu_->clear();

    // Common function templates
    QStringList templates = {
        "Scalar Function", "Table Function", "Aggregate Function",
        "String Function", "Math Function", "Date Function"
    };

    for (const QString& templateName : templates) {
        QAction* action = templateMenu_->addAction(templateName);
        connect(action, &QAction::triggered, this, [this, templateName]() {
            applyTemplate(templateName);
        });
    }

    templateMenu_->addSeparator();
    templateMenu_->addAction("Load from File...", this, &FunctionEditorDialog::onLoadTemplate);
    templateMenu_->addAction("Save as Template...", this, &FunctionEditorDialog::onSaveTemplate);
}

void FunctionEditorDialog::setFunctionDefinition(const FunctionDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    functionNameEdit_->setText(definition.name);
    schemaEdit_->setText(definition.schema);
    commentEdit_->setPlainText(definition.comment);
    codeEditor_->setPlainText(definition.body);
    deterministicCheck_->setChecked(definition.isDeterministic);
    sqlModeEdit_->setText(definition.sqlMode);

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

    // Update parameters table
    updateParameterTable();
}

FunctionDefinition FunctionEditorDialog::getFunctionDefinition() const {
    FunctionDefinition definition = currentDefinition_;

    definition.name = functionNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.returnType = returnTypeCombo_->currentText();
    definition.language = languageCombo_->currentData().toString();
    definition.comment = commentEdit_->toPlainText();
    definition.body = codeEditor_->toPlainText();
    definition.isDeterministic = deterministicCheck_->isChecked();
    definition.securityType = securityTypeCombo_->currentData().toString();
    definition.sqlMode = sqlModeEdit_->text();

    return definition;
}

void FunctionEditorDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Function");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Function");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void FunctionEditorDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update language options based on database type
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

void FunctionEditorDialog::loadExistingFunction(const QString& schema, const QString& functionName) {
    // This would load an existing function definition
    // For now, just set the basic properties
    originalSchema_ = schema;
    originalFunctionName_ = functionName;

    functionNameEdit_->setText(functionName);
    schemaEdit_->setText(schema);
    setEditMode(true);

    // TODO: Load actual function definition from database
}

void FunctionEditorDialog::accept() {
    if (validateFunction()) {
        emit functionSaved(getFunctionDefinition());
        QDialog::accept();
    }
}

void FunctionEditorDialog::reject() {
    QDialog::reject();
}

// Parameter management slots
void FunctionEditorDialog::onAddParameter() {
    clearParameterDialog();
    // Switch to parameters tab to show the edit form
    tabWidget_->setCurrentWidget(parametersTab_);
}

void FunctionEditorDialog::onEditParameter() {
    int row = parametersTable_->currentRow();
    if (row >= 0) {
        loadParameterToDialog(row);
    }
}

void FunctionEditorDialog::onDeleteParameter() {
    int row = parametersTable_->currentRow();
    if (row >= 0) {
        currentDefinition_.parameters.removeAt(row);
        updateParameterTable();
        updateButtonStates();
    }
}

void FunctionEditorDialog::onMoveParameterUp() {
    int row = parametersTable_->currentRow();
    if (row > 0) {
        currentDefinition_.parameters.move(row, row - 1);
        updateParameterTable();
        parametersTable_->setCurrentCell(row - 1, 0);
    }
}

void FunctionEditorDialog::onMoveParameterDown() {
    int row = parametersTable_->currentRow();
    if (row >= 0 && row < currentDefinition_.parameters.size() - 1) {
        currentDefinition_.parameters.move(row, row + 1);
        updateParameterTable();
        parametersTable_->setCurrentCell(row + 1, 0);
    }
}

void FunctionEditorDialog::onParameterSelectionChanged() {
    updateButtonStates();
}

// Editor actions
void FunctionEditorDialog::onFormatSQL() {
    // Basic SQL formatting for function body
    QString sql = codeEditor_->toPlainText();
    QStringList lines = sql.split('\n');
    QStringList formattedLines;

    int indentLevel = 0;
    for (QString line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Decrease indent for END, closing braces, etc.
        if (trimmed.toUpper().contains("END") ||
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

void FunctionEditorDialog::onValidateSQL() {
    QString sql = codeEditor_->toPlainText();
    if (sql.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Function body cannot be empty.");
        return;
    }

    // Check for RETURN statement
    if (!sql.contains(QRegularExpression("\\bRETURN\\b", QRegularExpression::CaseInsensitiveOption))) {
        QMessageBox::warning(this, "Validation Warning", "Function body should contain a RETURN statement.");
        return;
    }

    QMessageBox::information(this, "Validation", "Function definition appears valid.");
}

void FunctionEditorDialog::onPreviewSQL() {
    if (validateFunction()) {
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

void FunctionEditorDialog::onGenerateTemplate() {
    // Template generation is handled by the menu
}

void FunctionEditorDialog::onLoadTemplate() {
    // TODO: Implement template loading from file
    QMessageBox::information(this, "Load Template", "Template loading will be implemented in the next update.");
}

void FunctionEditorDialog::onSaveTemplate() {
    // TODO: Implement template saving
    QMessageBox::information(this, "Save Template", "Template saving will be implemented in the next update.");
}

// Settings slots
void FunctionEditorDialog::onLanguageChanged(int index) {
    // Handle language changes if needed
}

void FunctionEditorDialog::onSecurityTypeChanged(int index) {
    // Handle security type changes if needed
}

void FunctionEditorDialog::onFunctionNameChanged(const QString& name) {
    // Validate function name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void FunctionEditorDialog::onReturnTypeChanged(int index) {
    // Handle return type changes if needed
}

void FunctionEditorDialog::updateParameterTable() {
    parametersTable_->setRowCount(currentDefinition_.parameters.size());

    for (int i = 0; i < currentDefinition_.parameters.size(); ++i) {
        const FunctionParameterDefinition& param = currentDefinition_.parameters[i];

        parametersTable_->setItem(i, 0, new QTableWidgetItem(param.name));
        parametersTable_->setItem(i, 1, new QTableWidgetItem(param.dataType));
        parametersTable_->setItem(i, 2, new QTableWidgetItem(param.length > 0 ? QString::number(param.length) : ""));
        parametersTable_->setItem(i, 3, new QTableWidgetItem(param.defaultValue));
        parametersTable_->setItem(i, 4, new QTableWidgetItem(param.comment));
    }
}

bool FunctionEditorDialog::validateFunction() {
    QString functionName = functionNameEdit_->text().trimmed();

    if (functionName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Function name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        functionNameEdit_->setFocus();
        return false;
    }

    QString returnType = returnTypeCombo_->currentText().trimmed();
    if (returnType.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Return type is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        returnTypeCombo_->setFocus();
        return false;
    }

    QString body = codeEditor_->toPlainText().trimmed();
    if (body.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Function body cannot be empty.");
        tabWidget_->setCurrentWidget(editorTab_);
        codeEditor_->setFocus();
        return false;
    }

    // Check for RETURN statement
    if (!body.contains(QRegularExpression("\\bRETURN\\b", QRegularExpression::CaseInsensitiveOption))) {
        int result = QMessageBox::question(this, "No RETURN Statement",
            "The function body does not contain a RETURN statement. Continue anyway?",
            QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::No) {
            tabWidget_->setCurrentWidget(editorTab_);
            return false;
        }
    }

    return true;
}

QString FunctionEditorDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString fullFunctionName = functionNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullFunctionName = QString("%1.%2").arg(schemaEdit_->text(), functionNameEdit_->text());
    }

    sqlParts.append(QString("CREATE FUNCTION %1").arg(fullFunctionName));

    // Parameters
    if (!currentDefinition_.parameters.isEmpty()) {
        QStringList paramList;
        for (const FunctionParameterDefinition& param : currentDefinition_.parameters) {
            QString paramDef = param.name + " " + param.dataType;

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

        sqlParts.append("(" + paramList.join(", ") + ")");
    } else {
        sqlParts.append("()");
    }

    // Return type
    sqlParts.append(QString("RETURNS %1").arg(returnTypeCombo_->currentText()));

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

QString FunctionEditorDialog::generateAlterSQL() const {
    // For functions, ALTER FUNCTION is used to change properties
    // For body changes, we need DROP and CREATE
    QStringList sqlParts;

    QString fullFunctionName = functionNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullFunctionName = QString("%1.%2").arg(schemaEdit_->text(), functionNameEdit_->text());
    }

    // For now, use DROP and CREATE for simplicity
    sqlParts.append(generateDropSQL());
    sqlParts.append(generateCreateSQL());

    return sqlParts.join("\n");
}

QString FunctionEditorDialog::generateDropSQL() const {
    QString functionName = functionNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        functionName = QString("%1.%2").arg(schemaEdit_->text(), functionName);
    }

    return QString("DROP FUNCTION IF EXISTS %1;").arg(functionName);
}

void FunctionEditorDialog::loadParameterToDialog(int row) {
    if (row < 0 || row >= currentDefinition_.parameters.size()) {
        return;
    }

    const FunctionParameterDefinition& param = currentDefinition_.parameters[row];

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
    paramDefaultEdit_->setText(param.defaultValue);
    paramCommentEdit_->setPlainText(param.comment);

    // Switch to parameters tab to show the edit form
    tabWidget_->setCurrentWidget(parametersTab_);
}

void FunctionEditorDialog::saveParameterFromDialog() {
    FunctionParameterDefinition param;

    param.name = paramNameEdit_->text().trimmed();
    param.dataType = paramDataTypeCombo_->currentText();
    param.length = paramLengthSpin_->value();
    param.precision = paramPrecisionSpin_->value();
    param.scale = paramScaleSpin_->value();
    param.direction = "IN"; // Functions only support IN parameters
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

void FunctionEditorDialog::clearParameterDialog() {
    paramNameEdit_->clear();
    paramDataTypeCombo_->setCurrentIndex(0);
    paramLengthSpin_->setValue(0);
    paramPrecisionSpin_->setValue(0);
    paramScaleSpin_->setValue(0);
    paramDefaultEdit_->clear();
    paramCommentEdit_->clear();

    // Clear table selection
    parametersTable_->clearSelection();
}

void FunctionEditorDialog::applyTemplate(const QString& templateName) {
    QString templateCode;

    if (templateName == "Scalar Function") {
        templateCode = "-- Scalar function: returns a single value\n"
                      "BEGIN\n"
                      "    -- Function logic here\n"
                      "    RETURN result_value;\n"
                      "END";
    } else if (templateName == "Table Function") {
        templateCode = "-- Table function: returns a table\n"
                      "BEGIN\n"
                      "    RETURN QUERY SELECT column1, column2 FROM table_name WHERE condition;\n"
                      "END";
    } else if (templateName == "Aggregate Function") {
        templateCode = "-- Aggregate function: operates on a set of values\n"
                      "BEGIN\n"
                      "    -- Initialize state if needed\n"
                      "    -- Process each input value\n"
                      "    RETURN final_result;\n"
                      "END";
    } else if (templateName == "String Function") {
        templateCode = "-- String manipulation function\n"
                      "BEGIN\n"
                      "    -- String processing logic here\n"
                      "    RETURN processed_string;\n"
                      "END";
    } else if (templateName == "Math Function") {
        templateCode = "-- Mathematical function\n"
                      "BEGIN\n"
                      "    -- Mathematical calculation here\n"
                      "    RETURN calculated_value;\n"
                      "END";
    } else if (templateName == "Date Function") {
        templateCode = "-- Date/time function\n"
                      "BEGIN\n"
                      "    -- Date processing logic here\n"
                      "    RETURN processed_date;\n"
                      "END";
    }

    codeEditor_->setPlainText(templateCode);
}

void FunctionEditorDialog::updateButtonStates() {
    bool hasSelection = parametersTable_->currentRow() >= 0;
    bool hasParameters = currentDefinition_.parameters.size() > 0;

    editParameterButton_->setEnabled(hasSelection);
    deleteParameterButton_->setEnabled(hasSelection);
    moveUpButton_->setEnabled(hasSelection && parametersTable_->currentRow() > 0);
    moveDownButton_->setEnabled(hasSelection && parametersTable_->currentRow() < currentDefinition_.parameters.size() - 1);
}

} // namespace scratchrobin
