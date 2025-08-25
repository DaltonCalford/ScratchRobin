#include "view_editor_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QFontDatabase>
#include <QRegularExpression>
#include <algorithm>

namespace scratchrobin {

// SQL syntax highlighter
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
            "CREATE", "VIEW", "ALTER", "DROP", "WITH", "RECURSIVE", "CHECK", "OPTION"
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

ViewEditorDialog::ViewEditorDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , viewNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , algorithmCombo_(nullptr)
    , securityTypeCombo_(nullptr)
    , checkOptionCombo_(nullptr)
    , commentEdit_(nullptr)
    , editorTab_(nullptr)
    , editorLayout_(nullptr)
    , editorToolbar_(nullptr)
    , formatButton_(nullptr)
    , validateButton_(nullptr)
    , previewButton_(nullptr)
    , templateButton_(nullptr)
    , templateMenu_(nullptr)
    , codeEditor_(nullptr)
    , definitionEdit_(nullptr)
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
    , dependenciesTab_(nullptr)
    , dependenciesLayout_(nullptr)
    , referencedTablesList_(nullptr)
    , analyzeButton_(nullptr)
    , showTablesButton_(nullptr)
    , dependencyStatusLabel_(nullptr)
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
    setWindowTitle("View Editor");
    setModal(true);
    resize(900, 700);
}

void ViewEditorDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupEditorTab();
    setupAdvancedTab();
    setupDependenciesTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ViewEditorDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void ViewEditorDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    viewNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    algorithmCombo_ = new QComboBox(basicTab_);
    securityTypeCombo_ = new QComboBox(basicTab_);
    checkOptionCombo_ = new QComboBox(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Populate combo boxes
    algorithmCombo_->addItem("UNDEFINED", "UNDEFINED");
    algorithmCombo_->addItem("MERGE", "MERGE");
    algorithmCombo_->addItem("TEMPTABLE", "TEMPTABLE");

    securityTypeCombo_->addItem("DEFINER", "DEFINER");
    securityTypeCombo_->addItem("INVOKER", "INVOKER");

    checkOptionCombo_->addItem("NONE", "NONE");
    checkOptionCombo_->addItem("LOCAL", "LOCAL");
    checkOptionCombo_->addItem("CASCADE", "CASCADE");

    basicLayout_->addRow("View Name:", viewNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Algorithm:", algorithmCombo_);
    basicLayout_->addRow("Security Type:", securityTypeCombo_);
    basicLayout_->addRow("Check Option:", checkOptionCombo_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(viewNameEdit_, &QLineEdit::textChanged, this, &ViewEditorDialog::onViewNameChanged);
    connect(algorithmCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ViewEditorDialog::onAlgorithmChanged);
    connect(securityTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ViewEditorDialog::onSecurityTypeChanged);
    connect(checkOptionCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ViewEditorDialog::onCheckOptionChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void ViewEditorDialog::setupEditorTab() {
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

    // Alternative definition editor (for simple views)
    definitionEdit_ = new QTextEdit(editorTab_);
    definitionEdit_->setMaximumHeight(100);
    definitionEdit_->setPlaceholderText("SELECT statement for the view...");
    definitionEdit_->hide(); // Hidden by default

    editorLayout_->addWidget(definitionEdit_);

    // Connect signals
    connect(formatButton_, &QPushButton::clicked, this, &ViewEditorDialog::onFormatSQL);
    connect(validateButton_, &QPushButton::clicked, this, &ViewEditorDialog::onValidateSQL);
    connect(previewButton_, &QPushButton::clicked, this, &ViewEditorDialog::onPreviewSQL);

    tabWidget_->addTab(editorTab_, "Editor");
}

void ViewEditorDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("View Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    // Add database-specific options
    // These will be populated based on the selected database type

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void ViewEditorDialog::setupDependenciesTab() {
    dependenciesTab_ = new QWidget();
    dependenciesLayout_ = new QVBoxLayout(dependenciesTab_);

    dependencyStatusLabel_ = new QLabel("Dependencies not analyzed yet.", dependenciesTab_);
    dependenciesLayout_->addWidget(dependencyStatusLabel_);

    referencedTablesList_ = new QListWidget(dependenciesTab_);
    dependenciesLayout_->addWidget(referencedTablesList_);

    analyzeButton_ = new QPushButton("Analyze Dependencies", dependenciesTab_);
    showTablesButton_ = new QPushButton("Show Referenced Tables", dependenciesTab_);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(analyzeButton_);
    buttonLayout->addWidget(showTablesButton_);
    buttonLayout->addStretch();

    dependenciesLayout_->addLayout(buttonLayout);

    // Connect signals
    connect(analyzeButton_, &QPushButton::clicked, this, &ViewEditorDialog::onAnalyzeDependencies);
    connect(showTablesButton_, &QPushButton::clicked, this, &ViewEditorDialog::onShowReferencedTables);

    tabWidget_->addTab(dependenciesTab_, "Dependencies");
}

void ViewEditorDialog::setupSQLTab() {
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
    connect(generateSqlButton_, &QPushButton::clicked, this, &ViewEditorDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &ViewEditorDialog::onValidateSQL);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void ViewEditorDialog::setupCodeEditor() {
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

void ViewEditorDialog::setupDependenciesTable() {
    // Dependencies are shown in a list widget, so no additional setup needed
}

void ViewEditorDialog::populateTemplates() {
    if (!templateMenu_) return;

    templateMenu_->clear();

    // Common view templates
    QStringList templates = {
        "Simple Table View", "Join View", "Aggregate View", "Filtered View",
        "Union View", "Recursive View", "Materialized View"
    };

    for (const QString& templateName : templates) {
        QAction* action = templateMenu_->addAction(templateName);
        connect(action, &QAction::triggered, this, [this, templateName]() {
            applyTemplate(templateName);
        });
    }
}

void ViewEditorDialog::setViewDefinition(const ViewDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    viewNameEdit_->setText(definition.name);
    schemaEdit_->setText(definition.schema);
    commentEdit_->setPlainText(definition.comment);

    // Set combo box values
    if (!definition.algorithm.isEmpty()) {
        int index = algorithmCombo_->findData(definition.algorithm);
        if (index >= 0) algorithmCombo_->setCurrentIndex(index);
    }

    if (!definition.securityType.isEmpty()) {
        int index = securityTypeCombo_->findData(definition.securityType);
        if (index >= 0) securityTypeCombo_->setCurrentIndex(index);
    }

    if (!definition.checkOption.isEmpty()) {
        int index = checkOptionCombo_->findData(definition.checkOption);
        if (index >= 0) checkOptionCombo_->setCurrentIndex(index);
    }

    // Set definition text
    if (!definition.definition.isEmpty()) {
        codeEditor_->setPlainText(definition.definition);
    }

    // Update referenced tables
    referencedTablesList_->clear();
    for (const QString& table : definition.referencedTables) {
        referencedTablesList_->addItem(table);
    }
}

ViewDefinition ViewEditorDialog::getViewDefinition() const {
    ViewDefinition definition = currentDefinition_;

    definition.name = viewNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.comment = commentEdit_->toPlainText();

    // Get combo box values
    definition.algorithm = algorithmCombo_->currentData().toString();
    definition.securityType = securityTypeCombo_->currentData().toString();
    definition.checkOption = checkOptionCombo_->currentData().toString();

    // Get definition from editor
    definition.definition = codeEditor_->toPlainText();

    // Get referenced tables from list
    definition.referencedTables.clear();
    for (int i = 0; i < referencedTablesList_->count(); ++i) {
        definition.referencedTables.append(referencedTablesList_->item(i)->text());
    }

    return definition;
}

void ViewEditorDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit View");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create View");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void ViewEditorDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update UI elements based on database type
    bool isMySQL = (type == DatabaseType::MYSQL || type == DatabaseType::MARIADB);
    algorithmCombo_->setVisible(isMySQL);
    securityTypeCombo_->setVisible(isMySQL);
    checkOptionCombo_->setVisible(isMySQL);

    basicLayout_->labelForField(algorithmCombo_)->setVisible(isMySQL);
    basicLayout_->labelForField(securityTypeCombo_)->setVisible(isMySQL);
    basicLayout_->labelForField(checkOptionCombo_)->setVisible(isMySQL);
}

void ViewEditorDialog::loadExistingView(const QString& schema, const QString& viewName) {
    // This would load an existing view definition
    // For now, just set the basic properties
    originalSchema_ = schema;
    originalViewName_ = viewName;

    viewNameEdit_->setText(viewName);
    schemaEdit_->setText(schema);
    setEditMode(true);

    // TODO: Load actual view definition from database
}

void ViewEditorDialog::accept() {
    if (validateView()) {
        emit viewSaved(getViewDefinition());
        QDialog::accept();
    }
}

void ViewEditorDialog::reject() {
    QDialog::reject();
}

// Editor actions
void ViewEditorDialog::onFormatSQL() {
    // Basic SQL formatting for SELECT statements
    QString sql = codeEditor_->toPlainText();
    QStringList lines = sql.split('\n');
    QStringList formattedLines;

    int indentLevel = 0;
    for (QString line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Keywords that decrease indent
        if (trimmed.toUpper().contains("FROM") ||
            trimmed.toUpper().contains("WHERE") ||
            trimmed.toUpper().contains("GROUP BY") ||
            trimmed.toUpper().contains("ORDER BY") ||
            trimmed.toUpper().contains("HAVING")) {
            if (indentLevel > 0) indentLevel--;
        }

        // Add proper indentation
        if (indentLevel > 0) {
            formattedLines.append(QString(indentLevel * 4, ' ') + trimmed);
        } else {
            formattedLines.append(trimmed);
        }

        // Keywords that increase indent
        if (trimmed.toUpper().startsWith("SELECT") ||
            (trimmed.toUpper().contains("FROM") && !trimmed.toUpper().contains("FROM(")) ||
            trimmed.toUpper().contains("WHERE") ||
            trimmed.toUpper().contains("GROUP BY") ||
            trimmed.toUpper().contains("ORDER BY") ||
            trimmed.toUpper().contains("HAVING")) {
            indentLevel++;
        }
    }

    codeEditor_->setPlainText(formattedLines.join('\n'));
}

void ViewEditorDialog::onValidateSQL() {
    QString sql = codeEditor_->toPlainText();
    if (sql.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "View definition cannot be empty.");
        return;
    }

    // Basic validation - check for SELECT statement
    if (!sql.contains(QRegularExpression("\\bSELECT\\b", QRegularExpression::CaseInsensitiveOption))) {
        QMessageBox::warning(this, "Validation Error", "View definition must contain a SELECT statement.");
        return;
    }

    // Check for CREATE VIEW syntax if it exists
    if (sql.contains(QRegularExpression("\\bCREATE\\s+VIEW\\b", QRegularExpression::CaseInsensitiveOption))) {
        QMessageBox::warning(this, "Validation Warning",
            "The SQL contains CREATE VIEW syntax. Only the SELECT statement should be entered in the definition field.");
    }

    QMessageBox::information(this, "Validation", "View definition appears valid.");
}

void ViewEditorDialog::onPreviewSQL() {
    if (validateView()) {
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

void ViewEditorDialog::onGenerateTemplate() {
    // Template generation is handled by the menu
}

// Analysis slots
void ViewEditorDialog::onAnalyzeDependencies() {
    analyzeDependencies();
}

void ViewEditorDialog::onShowReferencedTables() {
    parseViewDefinition();
    analyzeDependencies();

    // Show referenced tables in a message box
    QStringList tables;
    for (int i = 0; i < referencedTablesList_->count(); ++i) {
        tables.append(referencedTablesList_->item(i)->text());
    }

    if (tables.isEmpty()) {
        QMessageBox::information(this, "Dependencies", "No referenced tables found.");
    } else {
        QMessageBox::information(this, "Referenced Tables",
            QString("Found %1 referenced table(s):\n\n%2")
            .arg(tables.size()).arg(tables.join("\n")));
    }
}

// Settings slots
void ViewEditorDialog::onViewNameChanged(const QString& name) {
    // Validate view name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void ViewEditorDialog::onAlgorithmChanged(int index) {
    // Handle algorithm changes if needed
}

void ViewEditorDialog::onSecurityTypeChanged(int index) {
    // Handle security type changes if needed
}

void ViewEditorDialog::onCheckOptionChanged(int index) {
    // Handle check option changes if needed
}

bool ViewEditorDialog::validateView() {
    QString viewName = viewNameEdit_->text().trimmed();

    if (viewName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "View name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        viewNameEdit_->setFocus();
        return false;
    }

    QString definition = codeEditor_->toPlainText().trimmed();
    if (definition.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "View definition cannot be empty.");
        tabWidget_->setCurrentWidget(editorTab_);
        codeEditor_->setFocus();
        return false;
    }

    // Check for SELECT statement
    if (!definition.contains(QRegularExpression("\\bSELECT\\b", QRegularExpression::CaseInsensitiveOption))) {
        QMessageBox::warning(this, "Validation Error", "View definition must contain a SELECT statement.");
        tabWidget_->setCurrentWidget(editorTab_);
        codeEditor_->setFocus();
        return false;
    }

    return true;
}

QString ViewEditorDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString fullViewName = viewNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullViewName = QString("%1.%2").arg(schemaEdit_->text(), fullViewName);
    }

    sqlParts.append(QString("CREATE VIEW %1").arg(fullViewName));

    // Add algorithm for MySQL
    bool isMySQL = (currentDatabaseType_ == DatabaseType::MYSQL || currentDatabaseType_ == DatabaseType::MARIADB);
    if (isMySQL && algorithmCombo_->currentData().toString() != "UNDEFINED") {
        sqlParts.append(QString("ALGORITHM = %1").arg(algorithmCombo_->currentData().toString()));
    }

    // Add security type for MySQL
    if (isMySQL && securityTypeCombo_->currentData().toString() != "DEFINER") {
        sqlParts.append(QString("SQL SECURITY %1").arg(securityTypeCombo_->currentData().toString()));
    }

    sqlParts.append("AS");

    // Add the view definition
    QString definition = codeEditor_->toPlainText().trimmed();

    // Remove any trailing semicolon if present
    if (definition.endsWith(';')) {
        definition = definition.left(definition.length() - 1);
    }

    sqlParts.append(definition);

    // Add check option
    QString checkOption = checkOptionCombo_->currentData().toString();
    if (checkOption != "NONE") {
        sqlParts.append(QString("WITH %1 CHECK OPTION").arg(checkOption));
    }

    return sqlParts.join("\n");
}

QString ViewEditorDialog::generateAlterSQL() const {
    // For views, ALTER VIEW is similar to CREATE OR REPLACE VIEW
    QStringList sqlParts;

    QString fullViewName = viewNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullViewName = QString("%1.%2").arg(schemaEdit_->text(), fullViewName);
    }

    sqlParts.append(QString("CREATE OR REPLACE VIEW %1").arg(fullViewName));

    // Add algorithm for MySQL
    bool isMySQL = (currentDatabaseType_ == DatabaseType::MYSQL || currentDatabaseType_ == DatabaseType::MARIADB);
    if (isMySQL && algorithmCombo_->currentData().toString() != "UNDEFINED") {
        sqlParts.append(QString("ALGORITHM = %1").arg(algorithmCombo_->currentData().toString()));
    }

    // Add security type for MySQL
    if (isMySQL && securityTypeCombo_->currentData().toString() != "DEFINER") {
        sqlParts.append(QString("SQL SECURITY %1").arg(securityTypeCombo_->currentData().toString()));
    }

    sqlParts.append("AS");

    // Add the view definition
    QString definition = codeEditor_->toPlainText().trimmed();

    // Remove any trailing semicolon if present
    if (definition.endsWith(';')) {
        definition = definition.left(definition.length() - 1);
    }

    sqlParts.append(definition);

    // Add check option
    QString checkOption = checkOptionCombo_->currentData().toString();
    if (checkOption != "NONE") {
        sqlParts.append(QString("WITH %1 CHECK OPTION").arg(checkOption));
    }

    return sqlParts.join("\n");
}

QString ViewEditorDialog::generateDropSQL() const {
    QString viewName = viewNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        viewName = QString("%1.%2").arg(schemaEdit_->text(), viewName);
    }

    return QString("DROP VIEW IF EXISTS %1;").arg(viewName);
}

void ViewEditorDialog::parseViewDefinition() {
    QString definition = codeEditor_->toPlainText();

    // Simple parsing to extract table names from FROM clauses
    QRegularExpression fromRegex("\\bFROM\\s+([\\w\\.]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator matches = fromRegex.globalMatch(definition);

    referencedTablesList_->clear();
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString tableName = match.captured(1);
        if (!tableName.isEmpty()) {
            // Avoid duplicates
            bool exists = false;
            for (int i = 0; i < referencedTablesList_->count(); ++i) {
                if (referencedTablesList_->item(i)->text() == tableName) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                referencedTablesList_->addItem(tableName);
            }
        }
    }
}

void ViewEditorDialog::analyzeDependencies() {
    parseViewDefinition();

    int tableCount = referencedTablesList_->count();
    if (tableCount == 0) {
        dependencyStatusLabel_->setText("No dependencies found.");
    } else {
        dependencyStatusLabel_->setText(QString("Found %1 referenced table(s)").arg(tableCount));
    }

    // In a full implementation, this would analyze column dependencies,
    // check for circular references, etc.
}

void ViewEditorDialog::applyTemplate(const QString& templateName) {
    QString templateCode;

    if (templateName == "Simple Table View") {
        templateCode = "SELECT *\nFROM table_name\nWHERE condition";
    } else if (templateName == "Join View") {
        templateCode = "SELECT t1.column1, t2.column2\nFROM table1 t1\nJOIN table2 t2 ON t1.id = t2.table1_id\nWHERE condition";
    } else if (templateName == "Aggregate View") {
        templateCode = "SELECT column1, COUNT(*), SUM(column2), AVG(column3)\nFROM table_name\nGROUP BY column1\nHAVING COUNT(*) > 1";
    } else if (templateName == "Filtered View") {
        templateCode = "SELECT *\nFROM table_name\nWHERE active = 1\n  AND created_date >= '2024-01-01'";
    } else if (templateName == "Union View") {
        templateCode = "SELECT column1, column2 FROM table1\nUNION\nSELECT column1, column2 FROM table2";
    } else if (templateName == "Recursive View") {
        templateCode = "WITH RECURSIVE recursive_cte AS (\n    SELECT id, parent_id, name\n    FROM table_name\n    WHERE parent_id IS NULL\n    \n    UNION ALL\n    \n    SELECT t.id, t.parent_id, t.name\n    FROM table_name t\n    JOIN recursive_cte r ON t.parent_id = r.id\n)\nSELECT * FROM recursive_cte";
    } else if (templateName == "Materialized View") {
        templateCode = "-- Note: Use CREATE MATERIALIZED VIEW for materialized views\nSELECT * FROM table_name";
    }

    codeEditor_->setPlainText(templateCode);
}

void ViewEditorDialog::updateButtonStates() {
    // Update button states based on current content
    bool hasDefinition = !codeEditor_->toPlainText().trimmed().isEmpty();
    bool hasViewName = !viewNameEdit_->text().trimmed().isEmpty();

    previewButton_->setEnabled(hasDefinition && hasViewName);
    validateButton_->setEnabled(hasDefinition);
    showTablesButton_->setEnabled(hasDefinition);
}

} // namespace scratchrobin
