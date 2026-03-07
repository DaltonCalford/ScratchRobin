#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPlainTextEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTabWidget;
class QSplitter;
class QListWidget;
class QTreeWidget;
class QStandardItemModel;
class QTableView;
class QTextBrowser;
class QSyntaxHighlighter;
QT_END_NAMESPACE

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Scripting Console - Python/JS scripting
 * 
 * Execute scripts within the application:
 * - JavaScript/Python script execution
 * - Interactive REPL console
 * - Script editor with syntax highlighting
 * - Access to application API
 * - Script history and favorites
 * - Output capture and logging
 */

// ============================================================================
// Script Types
// ============================================================================
enum class ScriptLanguage {
    JavaScript,
    Python,
    SQL,
    Shell
};

// ============================================================================
// Script Structure
// ============================================================================
struct Script {
    QString id;
    QString name;
    QString description;
    ScriptLanguage language;
    QString content;
    QString author;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QStringList tags;
    bool isFavorite = false;
};

struct ScriptOutput {
    QString text;
    bool isError = false;
    QDateTime timestamp;
};

// ============================================================================
// Scripting Console Panel
// ============================================================================
class ScriptingConsolePanel : public DockPanel {
    Q_OBJECT

public:
    explicit ScriptingConsolePanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Scripting Console"); }
    QString panelCategory() const override { return "development"; }

public slots:
    // Execution
    void onExecuteScript();
    void onExecuteSelection();
    void onStopExecution();
    void onClearConsole();
    
    // Script management
    void onNewScript();
    void onOpenScript();
    void onSaveScript();
    void onSaveScriptAs();
    void onScriptSelected(int index);
    
    // Language
    void onLanguageChanged(int index);
    void onToggleInterpreter();
    
    // History
    void onPreviousCommand();
    void onNextCommand();
    void onClearHistory();
    
    // API
    void onShowApiReference();
    void onInsertApiCall(const QString& call);

signals:
    void scriptExecuted(const QString& script, const QString& output);
    void executionError(const QString& error);

private:
    void setupUi();
    void setupEditor();
    void setupConsole();
    void setupApiBrowser();
    void executeCode(const QString& code);
    void executeJavaScript(const QString& code);
    void executeSQL(const QString& code);
    void appendOutput(const QString& text, bool isError = false);
    
    backend::SessionClient* client_;
    Script currentScript_;
    QStringList commandHistory_;
    int historyIndex_ = -1;
    bool isExecuting_ = false;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    
    // Editor
    QPlainTextEdit* editor_ = nullptr;
    QComboBox* languageCombo_ = nullptr;
    QComboBox* scriptSelector_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    // Console
    QTextBrowser* consoleOutput_ = nullptr;
    QLineEdit* consoleInput_ = nullptr;
    QPushButton* executeBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    
    // API Browser
    QTreeWidget* apiTree_ = nullptr;
    QTextBrowser* apiDoc_ = nullptr;
    
    // JavaScript engine (placeholder)
    void* jsEngine_ = nullptr;
};

// ============================================================================
// Script Editor Dialog
// ============================================================================
class ScriptEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScriptEditorDialog(Script* script, backend::SessionClient* client,
                                QWidget* parent = nullptr);

public slots:
    void onRun();
    void onDebug();
    void onFormat();
    void onValidate();
    void onInsertSnippet(const QString& snippet);
    void onFindReplace();
    void onGotoLine();

private:
    void setupUi();
    
    Script* script_;
    backend::SessionClient* client_;
    
    QPlainTextEdit* editor_ = nullptr;
    QComboBox* languageCombo_ = nullptr;
    QTextBrowser* outputBrowser_ = nullptr;
};

// ============================================================================
// Script Manager Dialog
// ============================================================================
class ScriptManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScriptManagerDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onNewScript();
    void onEditScript();
    void onDeleteScript();
    void onDuplicateScript();
    void onRunScript();
    void onToggleFavorite();
    void onFilterByTag(const QString& tag);
    void onSearchTextChanged(const QString& text);
    void onImportScript();
    void onExportScript();

private:
    void setupUi();
    void loadScripts();
    void updateScriptList();
    
    backend::SessionClient* client_;
    QList<Script> scripts_;
    Script currentScript_;
    
    QTableView* scriptTable_ = nullptr;
    QStandardItemModel* scriptModel_ = nullptr;
    QListWidget* tagList_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
};

// ============================================================================
// Script Syntax Highlighter
// ============================================================================
class ScriptSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit ScriptSyntaxHighlighter(ScriptLanguage language, QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupJavaScriptRules();
    void setupPythonRules();
    void setupSQLRules();
    
    ScriptLanguage language_;
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> rules_;
    
    QTextCharFormat keywordFormat_;
    QTextCharFormat stringFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat functionFormat_;
    QTextCharFormat numberFormat_;
};

} // namespace scratchrobin::ui
