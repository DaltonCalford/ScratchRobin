#include "scripting_console.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QTextBrowser>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QScrollBar>

namespace scratchrobin::ui {

// ============================================================================
// Scripting Console Panel
// ============================================================================
ScriptingConsolePanel::ScriptingConsolePanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("scripting_console", parent), client_(client) {
    setupUi();
}

void ScriptingConsolePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // JavaScript engine not available in this build
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    languageCombo_ = new QComboBox(this);
    languageCombo_->addItem(tr("JavaScript"), static_cast<int>(ScriptLanguage::JavaScript));
    languageCombo_->addItem(tr("Python"), static_cast<int>(ScriptLanguage::Python));
    languageCombo_->addItem(tr("SQL"), static_cast<int>(ScriptLanguage::SQL));
    connect(languageCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScriptingConsolePanel::onLanguageChanged);
    toolbarLayout->addWidget(languageCombo_);
    
    toolbarLayout->addSpacing(16);
    
    executeBtn_ = new QPushButton(tr("▶ Run"), this);
    connect(executeBtn_, &QPushButton::clicked, this, &ScriptingConsolePanel::onExecuteScript);
    toolbarLayout->addWidget(executeBtn_);
    
    stopBtn_ = new QPushButton(tr("■ Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &ScriptingConsolePanel::onStopExecution);
    toolbarLayout->addWidget(stopBtn_);
    
    toolbarLayout->addSpacing(16);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ScriptingConsolePanel::onClearConsole);
    toolbarLayout->addWidget(clearBtn);
    
    auto* apiBtn = new QPushButton(tr("API"), this);
    connect(apiBtn, &QPushButton::clicked, this, &ScriptingConsolePanel::onShowApiReference);
    toolbarLayout->addWidget(apiBtn);
    
    toolbarLayout->addStretch();
    
    mainLayout->addLayout(toolbarLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Editor
    editor_ = new QPlainTextEdit(this);
    editor_->setPlaceholderText(tr("// Enter your script here...\n"
                                   "// Example:\n"
                                   "console.log('Hello from ScratchRobin!');"));
    
    // Setup syntax highlighter
    auto* highlighter = new ScriptSyntaxHighlighter(
        static_cast<ScriptLanguage>(languageCombo_->currentData().toInt()),
        editor_->document());
    Q_UNUSED(highlighter)
    
    splitter->addWidget(editor_);
    
    // Console output
    consoleOutput_ = new QTextBrowser(this);
    consoleOutput_->setPlaceholderText(tr("Output will appear here..."));
    splitter->addWidget(consoleOutput_);
    
    splitter->setSizes({300, 200});
    
    mainLayout->addWidget(splitter, 1);
    
    // Console input
    auto* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel(tr(">"), this));
    consoleInput_ = new QLineEdit(this);
    consoleInput_->setPlaceholderText(tr("Enter command..."));
    connect(consoleInput_, &QLineEdit::returnPressed, [this]() {
        QString code = consoleInput_->text();
        if (!code.isEmpty()) {
            commandHistory_.append(code);
            historyIndex_ = commandHistory_.size();
            appendOutput("> " + code);
            executeCode(code);
            consoleInput_->clear();
        }
    });
    inputLayout->addWidget(consoleInput_, 1);
    
    mainLayout->addLayout(inputLayout);
    
    // Status
    statusLabel_ = new QLabel(tr("Ready"), this);
    mainLayout->addWidget(statusLabel_);
}

void ScriptingConsolePanel::setupEditor() {
    // Additional editor setup if needed
}

void ScriptingConsolePanel::setupConsole() {
    // Console setup
}

void ScriptingConsolePanel::setupApiBrowser() {
    // API browser setup
}

void ScriptingConsolePanel::onExecuteScript() {
    QString code = editor_->toPlainText();
    if (code.trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Empty Script"), tr("Please enter some code to execute."));
        return;
    }
    
    isExecuting_ = true;
    executeBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("Executing..."));
    
    executeCode(code);
    
    isExecuting_ = false;
    executeBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    statusLabel_->setText(tr("Ready"));
}

void ScriptingConsolePanel::onExecuteSelection() {
    QTextCursor cursor = editor_->textCursor();
    QString code = cursor.selectedText();
    if (code.isEmpty()) {
        code = editor_->toPlainText();
    }
    executeCode(code);
}

void ScriptingConsolePanel::onStopExecution() {
    isExecuting_ = false;
    executeBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    statusLabel_->setText(tr("Stopped"));
}

void ScriptingConsolePanel::onClearConsole() {
    consoleOutput_->clear();
}

void ScriptingConsolePanel::executeCode(const QString& code) {
    ScriptLanguage lang = static_cast<ScriptLanguage>(languageCombo_->currentData().toInt());
    
    switch (lang) {
        case ScriptLanguage::JavaScript:
            executeJavaScript(code);
            break;
        case ScriptLanguage::SQL:
            executeSQL(code);
            break;
        case ScriptLanguage::Python:
            appendOutput(tr("Python support not yet implemented."), true);
            break;
        case ScriptLanguage::Shell:
            appendOutput(tr("Shell support not yet implemented."), true);
            break;
    }
}

void ScriptingConsolePanel::executeJavaScript(const QString& code) {
    Q_UNUSED(code)
    // JavaScript execution would require QJSEngine
    // For now, just echo the code
    appendOutput(tr("[JavaScript execution not available in this build]"));
    appendOutput(tr("Would execute: %1").arg(code.left(50)));
}

void ScriptingConsolePanel::executeSQL(const QString& code) {
    Q_UNUSED(code)
    appendOutput(tr("SQL execution would run: %1").arg(code));
    // TODO: Execute SQL through backend
}

void ScriptingConsolePanel::appendOutput(const QString& text, bool isError) {
    QString color = isError ? "red" : "black";
    consoleOutput_->append(QString("<span style='color: %1;'>%2</span>")
        .arg(color)
        .arg(text.toHtmlEscaped()));
    
    // Scroll to bottom
    QScrollBar* sb = consoleOutput_->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ScriptingConsolePanel::onNewScript() {
    editor_->clear();
    currentScript_ = Script();
}

void ScriptingConsolePanel::onOpenScript() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Script"),
        QString(), tr("JavaScript (*.js);;Python (*.py);;SQL (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            editor_->setPlainText(file.readAll());
        }
    }
}

void ScriptingConsolePanel::onSaveScript() {
    if (currentScript_.id.isEmpty()) {
        onSaveScriptAs();
    } else {
        // Save to existing
        QMessageBox::information(this, tr("Saved"), tr("Script saved."));
    }
}

void ScriptingConsolePanel::onSaveScriptAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Script"),
        tr("script.js"), tr("JavaScript (*.js);;Python (*.py);;SQL (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::WriteOnly)) {
            file.write(editor_->toPlainText().toUtf8());
        }
    }
}

void ScriptingConsolePanel::onScriptSelected(int index) {
    Q_UNUSED(index)
}

void ScriptingConsolePanel::onLanguageChanged(int index) {
    Q_UNUSED(index)
    // Update syntax highlighter
}

void ScriptingConsolePanel::onToggleInterpreter() {
    // Toggle interpreter state
}

void ScriptingConsolePanel::onPreviousCommand() {
    if (historyIndex_ > 0) {
        historyIndex_--;
        consoleInput_->setText(commandHistory_[historyIndex_]);
    }
}

void ScriptingConsolePanel::onNextCommand() {
    if (historyIndex_ < commandHistory_.size() - 1) {
        historyIndex_++;
        consoleInput_->setText(commandHistory_[historyIndex_]);
    } else {
        historyIndex_ = commandHistory_.size();
        consoleInput_->clear();
    }
}

void ScriptingConsolePanel::onClearHistory() {
    commandHistory_.clear();
    historyIndex_ = -1;
}

void ScriptingConsolePanel::onShowApiReference() {
    QMessageBox::information(this, tr("API Reference"),
        tr("API reference would be shown here.\n\n"
           "Available APIs:\n"
           "- query(sql): Execute SQL query\n"
           "- getConnection(): Get current connection\n"
           "- log(message): Log message to console"));
}

void ScriptingConsolePanel::onInsertApiCall(const QString& call) {
    editor_->insertPlainText(call);
}

// ============================================================================
// Script Editor Dialog
// ============================================================================
ScriptEditorDialog::ScriptEditorDialog(Script* script, backend::SessionClient* client,
                                       QWidget* parent)
    : QDialog(parent), script_(script), client_(client) {
    setupUi();
}

void ScriptEditorDialog::setupUi() {
    setWindowTitle(tr("Script Editor: %1").arg(script_->name));
    setMinimumSize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    languageCombo_ = new QComboBox(this);
    languageCombo_->addItems({tr("JavaScript"), tr("Python"), tr("SQL")});
    toolbarLayout->addWidget(languageCombo_);
    
    toolbarLayout->addSpacing(16);
    
    auto* runBtn = new QPushButton(tr("Run"), this);
    connect(runBtn, &QPushButton::clicked, this, &ScriptEditorDialog::onRun);
    toolbarLayout->addWidget(runBtn);
    
    auto* debugBtn = new QPushButton(tr("Debug"), this);
    connect(debugBtn, &QPushButton::clicked, this, &ScriptEditorDialog::onDebug);
    toolbarLayout->addWidget(debugBtn);
    
    toolbarLayout->addStretch();
    
    auto* formatBtn = new QPushButton(tr("Format"), this);
    connect(formatBtn, &QPushButton::clicked, this, &ScriptEditorDialog::onFormat);
    toolbarLayout->addWidget(formatBtn);
    
    layout->addLayout(toolbarLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    editor_ = new QPlainTextEdit(this);
    editor_->setPlainText(script_->content);
    splitter->addWidget(editor_);
    
    outputBrowser_ = new QTextBrowser(this);
    outputBrowser_->setMaximumHeight(150);
    splitter->addWidget(outputBrowser_);
    
    layout->addWidget(splitter, 1);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &ScriptEditorDialog::onValidate);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void ScriptEditorDialog::onRun() {
    outputBrowser_->append(tr("Running script..."));
    // Execute script
}

void ScriptEditorDialog::onDebug() {
    outputBrowser_->append(tr("Starting debugger..."));
}

void ScriptEditorDialog::onFormat() {
    outputBrowser_->append(tr("Formatting code..."));
}

void ScriptEditorDialog::onValidate() {
    script_->content = editor_->toPlainText();
    accept();
}

void ScriptEditorDialog::onInsertSnippet(const QString& snippet) {
    editor_->insertPlainText(snippet);
}

void ScriptEditorDialog::onFindReplace() {
    // Show find/replace dialog
}

void ScriptEditorDialog::onGotoLine() {
    // Show goto line dialog
}

// ============================================================================
// Script Manager Dialog
// ============================================================================
ScriptManagerDialog::ScriptManagerDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadScripts();
}

void ScriptManagerDialog::setupUi() {
    setWindowTitle(tr("Script Manager"));
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search scripts..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &ScriptManagerDialog::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    layout->addLayout(filterLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Tag list
    tagList_ = new QListWidget(this);
    tagList_->addItem(tr("All"));
    tagList_->addItem(tr("Favorites"));
    tagList_->addItem(tr("Maintenance"));
    tagList_->addItem(tr("Reports"));
    splitter->addWidget(tagList_);
    
    // Script table
    scriptTable_ = new QTableView(this);
    scriptModel_ = new QStandardItemModel(this);
    scriptModel_->setHorizontalHeaderLabels({tr("Name"), tr("Language"), tr("Modified")});
    scriptTable_->setModel(scriptModel_);
    scriptTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(scriptTable_);
    
    layout->addWidget(splitter, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* newBtn = new QPushButton(tr("New"), this);
    connect(newBtn, &QPushButton::clicked, this, &ScriptManagerDialog::onNewScript);
    btnLayout->addWidget(newBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &ScriptManagerDialog::onEditScript);
    btnLayout->addWidget(editBtn);
    
    auto* runBtn = new QPushButton(tr("Run"), this);
    connect(runBtn, &QPushButton::clicked, this, &ScriptManagerDialog::onRunScript);
    btnLayout->addWidget(runBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ScriptManagerDialog::loadScripts() {
    scripts_.clear();
    
    Script s1;
    s1.id = "script1";
    s1.name = tr("Daily Backup");
    s1.language = ScriptLanguage::SQL;
    s1.modifiedAt = QDateTime::currentDateTime().addDays(-1);
    scripts_.append(s1);
    
    Script s2;
    s2.id = "script2";
    s2.name = tr("Generate Report");
    s2.language = ScriptLanguage::JavaScript;
    s2.modifiedAt = QDateTime::currentDateTime().addDays(-7);
    s2.isFavorite = true;
    scripts_.append(s2);
    
    updateScriptList();
}

void ScriptManagerDialog::updateScriptList() {
    scriptModel_->clear();
    scriptModel_->setHorizontalHeaderLabels({tr("Name"), tr("Language"), tr("Modified")});
    
    for (const auto& script : scripts_) {
        QString langStr;
        switch (script.language) {
            case ScriptLanguage::JavaScript: langStr = tr("JS"); break;
            case ScriptLanguage::Python: langStr = tr("Python"); break;
            case ScriptLanguage::SQL: langStr = tr("SQL"); break;
            case ScriptLanguage::Shell: langStr = tr("Shell"); break;
        }
        
        auto* row = new QList<QStandardItem*>();
        QString name = script.isFavorite ? "★ " + script.name : script.name;
        *row << new QStandardItem(name)
             << new QStandardItem(langStr)
             << new QStandardItem(script.modifiedAt.toString("yyyy-MM-dd"));
        
        scriptModel_->appendRow(*row);
    }
}

void ScriptManagerDialog::onNewScript() {
    Script newScript;
    newScript.id = "script_" + QString::number(scripts_.size() + 1);
    newScript.name = tr("New Script %1").arg(scripts_.size() + 1);
    newScript.language = ScriptLanguage::JavaScript;
    newScript.createdAt = QDateTime::currentDateTime();
    newScript.modifiedAt = QDateTime::currentDateTime();
    
    ScriptEditorDialog dialog(&newScript, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        scripts_.append(newScript);
        updateScriptList();
    }
}

void ScriptManagerDialog::onEditScript() {
    // Edit selected script
}

void ScriptManagerDialog::onDeleteScript() {
    // Delete selected script
}

void ScriptManagerDialog::onDuplicateScript() {
    // Duplicate script
}

void ScriptManagerDialog::onRunScript() {
    QMessageBox::information(this, tr("Run Script"), tr("Script would be executed."));
}

void ScriptManagerDialog::onToggleFavorite() {
    // Toggle favorite status
}

void ScriptManagerDialog::onFilterByTag(const QString& tag) {
    Q_UNUSED(tag)
    updateScriptList();
}

void ScriptManagerDialog::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    updateScriptList();
}

void ScriptManagerDialog::onImportScript() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Script"),
        QString(), tr("Script Files (*.js *.py *.sql)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Imported"), tr("Script imported."));
    }
}

void ScriptManagerDialog::onExportScript() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Script"),
        tr("script.js"), tr("JavaScript (*.js)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Script exported."));
    }
}

// ============================================================================
// Script Syntax Highlighter
// ============================================================================
ScriptSyntaxHighlighter::ScriptSyntaxHighlighter(ScriptLanguage language, QTextDocument* parent)
    : QSyntaxHighlighter(parent), language_(language) {
    
    switch (language_) {
        case ScriptLanguage::JavaScript:
            setupJavaScriptRules();
            break;
        case ScriptLanguage::Python:
            setupPythonRules();
            break;
        case ScriptLanguage::SQL:
            setupSQLRules();
            break;
        default:
            break;
    }
}

void ScriptSyntaxHighlighter::setupJavaScriptRules() {
    // Keywords
    keywordFormat_.setForeground(Qt::darkBlue);
    keywordFormat_.setFontWeight(QFont::Bold);
    QStringList keywords = {
        "var", "let", "const", "function", "return", "if", "else", "for", "while",
        "do", "switch", "case", "break", "continue", "try", "catch", "finally",
        "throw", "new", "this", "typeof", "instanceof", "null", "undefined", "true", "false"
    };
    for (const QString& keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(keyword));
        rule.format = keywordFormat_;
        rules_.append(rule);
    }
    
    // Strings
    stringFormat_.setForeground(Qt::darkGreen);
    HighlightingRule stringRule;
    stringRule.pattern = QRegularExpression("\".*?\"|'.*?'");
    stringRule.format = stringFormat_;
    rules_.append(stringRule);
    
    // Comments
    commentFormat_.setForeground(Qt::gray);
    HighlightingRule commentRule;
    commentRule.pattern = QRegularExpression("//[^\n]*");
    commentRule.format = commentFormat_;
    rules_.append(commentRule);
    
    // Functions
    functionFormat_.setForeground(Qt::darkMagenta);
    HighlightingRule functionRule;
    functionRule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    functionRule.format = functionFormat_;
    rules_.append(functionRule);
    
    // Numbers
    numberFormat_.setForeground(Qt::darkCyan);
    HighlightingRule numberRule;
    numberRule.pattern = QRegularExpression("\\b[0-9]+\\b");
    numberRule.format = numberFormat_;
    rules_.append(numberRule);
}

void ScriptSyntaxHighlighter::setupPythonRules() {
    // Python-specific rules
    keywordFormat_.setForeground(Qt::darkBlue);
    QStringList keywords = {
        "def", "class", "if", "elif", "else", "for", "while", "try", "except",
        "finally", "with", "import", "from", "as", "return", "yield", "pass",
        "break", "continue", "raise", "True", "False", "None"
    };
    for (const QString& keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(keyword));
        rule.format = keywordFormat_;
        rules_.append(rule);
    }
    
    // Strings
    stringFormat_.setForeground(Qt::darkGreen);
    HighlightingRule stringRule;
    stringRule.pattern = QRegularExpression("\"\"\".*?\"\"\"|\".*?\"|'.*?'");
    stringRule.format = stringFormat_;
    rules_.append(stringRule);
    
    // Comments
    commentFormat_.setForeground(Qt::gray);
    HighlightingRule commentRule;
    commentRule.pattern = QRegularExpression("#[^\n]*");
    commentRule.format = commentFormat_;
    rules_.append(commentRule);
}

void ScriptSyntaxHighlighter::setupSQLRules() {
    // SQL keywords
    keywordFormat_.setForeground(Qt::darkBlue);
    keywordFormat_.setFontWeight(QFont::Bold);
    QStringList keywords = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE", "DROP",
        "ALTER", "TABLE", "INDEX", "VIEW", "JOIN", "INNER", "OUTER", "LEFT", "RIGHT",
        "ON", "GROUP", "BY", "ORDER", "HAVING", "LIMIT", "OFFSET", "AND", "OR", "NOT",
        "NULL", "IS", "IN", "BETWEEN", "LIKE", "EXISTS", "UNION", "ALL", "DISTINCT"
    };
    for (const QString& keyword : keywords) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(QString("\\b%1\\b").arg(keyword), 
            QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat_;
        rules_.append(rule);
    }
    
    // Strings
    stringFormat_.setForeground(Qt::darkGreen);
    HighlightingRule stringRule;
    stringRule.pattern = QRegularExpression("'.*?'|\".*?\"");
    stringRule.format = stringFormat_;
    rules_.append(stringRule);
    
    // Comments
    commentFormat_.setForeground(Qt::gray);
    HighlightingRule commentRule;
    commentRule.pattern = QRegularExpression("--[^\n]*|/\\*.*?\\*/");
    commentRule.format = commentFormat_;
    rules_.append(commentRule);
}

void ScriptSyntaxHighlighter::highlightBlock(const QString& text) {
    for (const HighlightingRule& rule : rules_) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

} // namespace scratchrobin::ui
