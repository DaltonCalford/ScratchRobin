#include "trigger_manager_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace scratchrobin {

// Simple SQL syntax highlighter for triggers
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
            "TRIGGER", "BEFORE", "AFTER", "FOR", "EACH", "ROW", "STATEMENT", "WHEN", "DECLARE", "SET", "IF", "THEN", "ELSE",
            "WHILE", "FOR", "LOOP", "NEW", "OLD", "TG_NAME", "TG_WHEN", "TG_LEVEL", "TG_OP"
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

TriggerManagerDialog::TriggerManagerDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , tabWidget_(nullptr)
    , basicTab_(nullptr)
    , basicLayout_(nullptr)
    , triggerNameEdit_(nullptr)
    , tableNameEdit_(nullptr)
    , schemaEdit_(nullptr)
    , timingCombo_(nullptr)
    , eventCombo_(nullptr)
    , conditionEdit_(nullptr)
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
    , advancedTab_(nullptr)
    , advancedLayout_(nullptr)
    , optionsGroup_(nullptr)
    , optionsLayout_(nullptr)
    , definerEdit_(nullptr)
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
    setWindowTitle("Trigger Manager");
    setModal(true);
    resize(900, 700);
}

void TriggerManagerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Create tab widget
    tabWidget_ = new QTabWidget(this);

    // Setup all tabs
    setupBasicTab();
    setupEditorTab();
    setupAdvancedTab();
    setupSQLTab();

    mainLayout_->addWidget(tabWidget_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &TriggerManagerDialog::onPreviewSQL);

    mainLayout_->addWidget(dialogButtons_);

    // Set up initial state
    updateButtonStates();
}

void TriggerManagerDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    triggerNameEdit_ = new QLineEdit(basicTab_);
    tableNameEdit_ = new QLineEdit(basicTab_);
    schemaEdit_ = new QLineEdit(basicTab_);
    timingCombo_ = new QComboBox(basicTab_);
    eventCombo_ = new QComboBox(basicTab_);
    conditionEdit_ = new QLineEdit(basicTab_);
    commentEdit_ = new QTextEdit(basicTab_);
    commentEdit_->setMaximumHeight(60);

    // Make table name and schema read-only (set via setTableInfo)
    tableNameEdit_->setReadOnly(true);
    schemaEdit_->setReadOnly(true);

    // Populate combo boxes
    timingCombo_->addItem("BEFORE", "BEFORE");
    timingCombo_->addItem("AFTER", "AFTER");

    eventCombo_->addItem("INSERT", "INSERT");
    eventCombo_->addItem("UPDATE", "UPDATE");
    eventCombo_->addItem("DELETE", "DELETE");
    eventCombo_->addItem("INSERT OR UPDATE", "INSERT OR UPDATE");
    eventCombo_->addItem("UPDATE OR DELETE", "UPDATE OR DELETE");
    eventCombo_->addItem("INSERT OR DELETE", "INSERT OR DELETE");
    eventCombo_->addItem("INSERT OR UPDATE OR DELETE", "INSERT OR UPDATE OR DELETE");

    basicLayout_->addRow("Trigger Name:", triggerNameEdit_);
    basicLayout_->addRow("Table:", tableNameEdit_);
    basicLayout_->addRow("Schema:", schemaEdit_);
    basicLayout_->addRow("Timing:", timingCombo_);
    basicLayout_->addRow("Event:", eventCombo_);
    basicLayout_->addRow("Condition:", conditionEdit_);
    basicLayout_->addRow("Comment:", commentEdit_);

    // Connect signals
    connect(triggerNameEdit_, &QLineEdit::textChanged, this, &TriggerManagerDialog::onTriggerNameChanged);
    connect(timingCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TriggerManagerDialog::onTimingChanged);
    connect(eventCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TriggerManagerDialog::onEventChanged);
    connect(conditionEdit_, &QLineEdit::textChanged, this, &TriggerManagerDialog::onConditionChanged);

    tabWidget_->addTab(basicTab_, "Basic");
}

void TriggerManagerDialog::setupEditorTab() {
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
    connect(formatButton_, &QPushButton::clicked, this, &TriggerManagerDialog::onFormatSQL);
    connect(validateButton_, &QPushButton::clicked, this, &TriggerManagerDialog::onValidateSQL);
    connect(previewButton_, &QPushButton::clicked, this, &TriggerManagerDialog::onPreviewSQL);

    tabWidget_->addTab(editorTab_, "Editor");
}

void TriggerManagerDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    optionsGroup_ = new QGroupBox("Trigger Options", advancedTab_);
    optionsLayout_ = new QFormLayout(optionsGroup_);

    definerEdit_ = new QLineEdit(advancedTab_);
    enabledCheck_ = new QCheckBox("Trigger is enabled", advancedTab_);
    enabledCheck_->setChecked(true);

    optionsLayout_->addRow("Definer:", definerEdit_);
    optionsLayout_->addRow("", enabledCheck_);

    advancedLayout_->addWidget(optionsGroup_);
    advancedLayout_->addStretch();

    // Connect signals
    connect(enabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        currentDefinition_.isEnabled = checked;
    });

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void TriggerManagerDialog::setupSQLTab() {
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
    connect(generateSqlButton_, &QPushButton::clicked, this, &TriggerManagerDialog::onPreviewSQL);
    connect(validateSqlButton_, &QPushButton::clicked, this, &TriggerManagerDialog::onValidateSQL);

    tabWidget_->addTab(sqlTab_, "SQL");
}

void TriggerManagerDialog::setupCodeEditor() {
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
    codeEditor_->setPlaceholderText("-- Trigger body SQL code here\n-- Use NEW and OLD to reference row data");
}

void TriggerManagerDialog::setTriggerDefinition(const TriggerDefinition& definition) {
    currentDefinition_ = definition;

    // Update UI
    triggerNameEdit_->setText(definition.name);
    tableNameEdit_->setText(definition.tableName);
    schemaEdit_->setText(definition.schema);
    conditionEdit_->setText(definition.condition);
    commentEdit_->setPlainText(definition.comment);
    codeEditor_->setPlainText(definition.body);
    definerEdit_->setText(definition.definer);
    enabledCheck_->setChecked(definition.isEnabled);

    // Set timing
    if (!definition.timing.isEmpty()) {
        int index = timingCombo_->findData(definition.timing);
        if (index >= 0) timingCombo_->setCurrentIndex(index);
    }

    // Set event
    if (!definition.event.isEmpty()) {
        int index = eventCombo_->findData(definition.event);
        if (index >= 0) eventCombo_->setCurrentIndex(index);
    }
}

TriggerDefinition TriggerManagerDialog::getTriggerDefinition() const {
    TriggerDefinition definition = currentDefinition_;

    definition.name = triggerNameEdit_->text();
    definition.tableName = tableNameEdit_->text();
    definition.schema = schemaEdit_->text();
    definition.timing = timingCombo_->currentData().toString();
    definition.event = eventCombo_->currentData().toString();
    definition.condition = conditionEdit_->text();
    definition.body = codeEditor_->toPlainText();
    definition.comment = commentEdit_->toPlainText();
    definition.definer = definerEdit_->text();
    definition.isEnabled = enabledCheck_->isChecked();

    return definition;
}

void TriggerManagerDialog::setEditMode(bool isEdit) {
    isEditMode_ = isEdit;
    if (isEdit) {
        setWindowTitle("Edit Trigger");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Update");
    } else {
        setWindowTitle("Create Trigger");
        dialogButtons_->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

void TriggerManagerDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;

    // Update available events based on database type
    switch (type) {
        case DatabaseType::MYSQL:
        case DatabaseType::MARIADB:
            // MySQL supports multiple events in one trigger
            break;
        case DatabaseType::POSTGRESQL:
            // PostgreSQL triggers are more flexible
            break;
        default:
            break;
    }
}

void TriggerManagerDialog::setTableInfo(const QString& schema, const QString& tableName) {
    schemaEdit_->setText(schema);
    tableNameEdit_->setText(tableName);
    currentDefinition_.schema = schema;
    currentDefinition_.tableName = tableName;
}

void TriggerManagerDialog::loadExistingTrigger(const QString& schema, const QString& tableName, const QString& triggerName) {
    setTableInfo(schema, tableName);
    triggerNameEdit_->setText(triggerName);
    originalTriggerName_ = triggerName;
    setEditMode(true);

    // TODO: Load actual trigger definition from database
}

void TriggerManagerDialog::accept() {
    if (validateTrigger()) {
        emit triggerSaved(getTriggerDefinition());
        QDialog::accept();
    }
}

void TriggerManagerDialog::reject() {
    QDialog::reject();
}

// Editor actions
void TriggerManagerDialog::onFormatSQL() {
    // Basic SQL formatting for trigger body
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

void TriggerManagerDialog::onValidateSQL() {
    QString sql = codeEditor_->toPlainText();
    if (sql.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Trigger body cannot be empty.");
        return;
    }

    // Basic validation
    QStringList errors;

    // Check for basic SQL structure
    if (!sql.contains(QRegularExpression("\\bBEGIN\\b", QRegularExpression::CaseInsensitiveOption)) &&
        !sql.contains(QRegularExpression("\\bSELECT\\b", QRegularExpression::CaseInsensitiveOption))) {
        errors.append("Trigger body appears to be empty or invalid");
    }

    if (errors.isEmpty()) {
        QMessageBox::information(this, "Validation", "Trigger definition appears valid.");
    } else {
        QMessageBox::warning(this, "Validation Errors", errors.join("\n"));
    }
}

void TriggerManagerDialog::onPreviewSQL() {
    if (validateTrigger()) {
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

void TriggerManagerDialog::onGenerateTemplate() {
    // Template generation is handled by the menu
}

void TriggerManagerDialog::onLoadTemplate() {
    // TODO: Implement template loading from file
    QMessageBox::information(this, "Load Template", "Template loading will be implemented in the next update.");
}

void TriggerManagerDialog::onSaveTemplate() {
    // TODO: Implement template saving
    QMessageBox::information(this, "Save Template", "Template saving will be implemented in the next update.");
}

// Settings slots
void TriggerManagerDialog::onTriggerNameChanged(const QString& name) {
    // Validate trigger name
    static QRegularExpression validName("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!name.isEmpty() && !validName.match(name).hasMatch()) {
        // Could show a warning, but for now just accept
    }
}

void TriggerManagerDialog::onTimingChanged(int index) {
    // Handle timing changes if needed
}

void TriggerManagerDialog::onEventChanged(int index) {
    // Handle event changes if needed
}

void TriggerManagerDialog::onConditionChanged() {
    currentDefinition_.condition = conditionEdit_->text();
}

void TriggerManagerDialog::populateTemplates() {
    if (!templateMenu_) return;

    templateMenu_->clear();

    // Common trigger templates
    QStringList templates = {
        "Audit Trigger", "Validation Trigger", "Auto-timestamp Trigger",
        "Replication Trigger", "Notification Trigger", "Logging Trigger"
    };

    for (const QString& templateName : templates) {
        QAction* action = templateMenu_->addAction(templateName);
        connect(action, &QAction::triggered, this, [this, templateName]() {
            applyTemplate(templateName);
        });
    }

    templateMenu_->addSeparator();
    templateMenu_->addAction("Load from File...", this, &TriggerManagerDialog::onLoadTemplate);
    templateMenu_->addAction("Save as Template...", this, &TriggerManagerDialog::onSaveTemplate);
}

bool TriggerManagerDialog::validateTrigger() {
    QString triggerName = triggerNameEdit_->text().trimmed();

    if (triggerName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Trigger name is required.");
        tabWidget_->setCurrentWidget(basicTab_);
        triggerNameEdit_->setFocus();
        return false;
    }

    QString tableName = tableNameEdit_->text().trimmed();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Table name is required.");
        return false;
    }

    QString body = codeEditor_->toPlainText().trimmed();
    if (body.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Trigger body cannot be empty.");
        tabWidget_->setCurrentWidget(editorTab_);
        codeEditor_->setFocus();
        return false;
    }

    return true;
}

QString TriggerManagerDialog::generateCreateSQL() const {
    QStringList sqlParts;

    QString fullTriggerName = triggerNameEdit_->text();
    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), tableNameEdit_->text());
    }

    sqlParts.append(QString("CREATE TRIGGER %1").arg(fullTriggerName));
    sqlParts.append(QString("%1 %2 ON %3")
        .arg(timingCombo_->currentData().toString(), eventCombo_->currentData().toString(), fullTableName));

    // Add condition if present
    QString condition = conditionEdit_->text().trimmed();
    if (!condition.isEmpty()) {
        sqlParts.append(QString("WHEN (%1)").arg(condition));
    }

    sqlParts.append("FOR EACH ROW");

    // Add trigger body
    QString body = codeEditor_->toPlainText().trimmed();
    if (!body.isEmpty()) {
        sqlParts.append("BEGIN");
        sqlParts.append(body);
        sqlParts.append("END;");
    }

    // Add comment if present
    QString comment = commentEdit_->toPlainText().trimmed();
    if (!comment.isEmpty()) {
        sqlParts.append(QString("COMMENT ON TRIGGER %1 ON %2 IS '%3'")
            .arg(fullTriggerName, fullTableName, comment.replace("'", "''")));
    }

    return sqlParts.join("\n");
}

QString TriggerManagerDialog::generateDropSQL() const {
    QString triggerName = triggerNameEdit_->text();
    QString fullTableName = tableNameEdit_->text();
    if (!schemaEdit_->text().isEmpty()) {
        fullTableName = QString("%1.%2").arg(schemaEdit_->text(), tableNameEdit_->text());
    }

    return QString("DROP TRIGGER IF EXISTS %1 ON %2;")
        .arg(triggerName, fullTableName);
}

QString TriggerManagerDialog::generateAlterSQL() const {
    // For triggers, ALTER typically means DROP and CREATE
    QStringList sqlParts;
    sqlParts.append(generateDropSQL());
    sqlParts.append(generateCreateSQL());
    return sqlParts.join("\n");
}

void TriggerManagerDialog::applyTemplate(const QString& templateName) {
    QString templateCode;

    if (templateName == "Audit Trigger") {
        templateCode = "-- Audit trigger: Log changes to audit table\n"
                      "INSERT INTO audit_log (table_name, operation, old_data, new_data, user_id, timestamp)\n"
                      "VALUES (TG_TABLE_NAME, TG_OP, row_to_json(OLD), row_to_json(NEW), current_user, now());";
    } else if (templateName == "Validation Trigger") {
        templateCode = "-- Validation trigger: Check data integrity\n"
                      "IF NEW.status NOT IN ('active', 'inactive', 'pending') THEN\n"
                      "    RAISE EXCEPTION 'Invalid status value: %', NEW.status;\n"
                      "END IF;";
    } else if (templateName == "Auto-timestamp Trigger") {
        templateCode = "-- Auto-timestamp trigger: Update modification time\n"
                      "NEW.updated_at = NOW();";
    } else if (templateName == "Replication Trigger") {
        templateCode = "-- Replication trigger: Copy changes to replica table\n"
                      "INSERT INTO replica_table SELECT * FROM NEW;";
    } else if (templateName == "Notification Trigger") {
        templateCode = "-- Notification trigger: Send notification on changes\n"
                      "PERFORM pg_notify('table_changes', json_build_object('table', TG_TABLE_NAME, 'operation', TG_OP)::text);";
    } else if (templateName == "Logging Trigger") {
        templateCode = "-- Logging trigger: Log all operations\n"
                      "INSERT INTO operation_log (operation, table_name, record_id, user_name, timestamp)\n"
                      "VALUES (TG_OP, TG_TABLE_NAME, COALESCE(NEW.id, OLD.id), current_user, now());";
    }

    codeEditor_->setPlainText(templateCode);
}

void TriggerManagerDialog::updateButtonStates() {
    // Update button states based on current content
    bool hasBody = !codeEditor_->toPlainText().trimmed().isEmpty();
    bool hasName = !triggerNameEdit_->text().trimmed().isEmpty();

    previewButton_->setEnabled(hasBody && hasName);
    validateButton_->setEnabled(hasBody);
}

} // namespace scratchrobin
