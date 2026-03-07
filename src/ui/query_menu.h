#pragma once
#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QAction;
class QWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

class SqlEditor;

/**
 * @brief Query menu implementation for SQL execution and formatting
 * 
 * Implements MENU_SPECIFICATION.md §Query section:
 * - Execute (F9)
 * - Execute Selection (Ctrl+F9)
 * - Execute Script
 * - Stop
 * - Explain Plan / Explain Analyze
 * - Format SQL
 * - Comment / Uncomment / Toggle Comment
 * - Uppercase / Lowercase
 */
class QueryMenu : public QObject {
    Q_OBJECT

public:
    explicit QueryMenu(QWidget* parent = nullptr);
    ~QueryMenu() override;

    QMenu* menu() const { return menu_; }

    // State management
    void setEditor(SqlEditor* editor);
    void setConnected(bool connected);
    void setExecuting(bool executing);
    void setHasSelection(bool hasSelection);

signals:
    // Execution signals
    void executeRequested();
    void executeSelectionRequested();
    void executeScriptRequested();
    void stopRequested();
    
    // Explain signals
    void explainRequested();
    void explainAnalyzeRequested();
    
    // Formatting signals
    void formatSqlRequested();
    void commentRequested();
    void uncommentRequested();
    void toggleCommentRequested();
    void uppercaseRequested();
    void lowercaseRequested();

private slots:
    void onExecute();
    void onExecuteSelection();
    void onExecuteScript();
    void onStop();
    void onExplain();
    void onExplainAnalyze();
    void onFormatSql();
    void onComment();
    void onUncomment();
    void onToggleComment();
    void onUppercase();
    void onLowercase();

private:
    void setupActions();
    void updateActionStates();

    QWidget* parent_;
    SqlEditor* editor_ = nullptr;
    QMenu* menu_ = nullptr;

    // State
    bool connected_ = false;
    bool executing_ = false;
    bool hasSelection_ = false;

    // Actions
    QAction* actionExecute_ = nullptr;
    QAction* actionExecuteSelection_ = nullptr;
    QAction* actionExecuteScript_ = nullptr;
    QAction* actionStop_ = nullptr;
    QAction* actionExplain_ = nullptr;
    QAction* actionExplainAnalyze_ = nullptr;
    QAction* actionFormatSql_ = nullptr;
    QAction* actionComment_ = nullptr;
    QAction* actionUncomment_ = nullptr;
    QAction* actionToggleComment_ = nullptr;
    QAction* actionUppercase_ = nullptr;
    QAction* actionLowercase_ = nullptr;
};

} // namespace scratchrobin::ui
