#include "ui/query_menu.h"
#include "ui/sql_editor.h"

#include <QMenu>
#include <QAction>
#include <QWidget>

namespace scratchrobin::ui {

QueryMenu::QueryMenu(QWidget* parent)
    : QObject(parent)
    , parent_(parent) {
    setupActions();
}

QueryMenu::~QueryMenu() = default;

void QueryMenu::setupActions() {
    menu_ = new QMenu(tr("&Query"), parent_);

    // Execute group
    actionExecute_ = menu_->addAction(tr("&Execute"), this, &QueryMenu::onExecute);
    actionExecute_->setShortcut(QKeySequence("F9"));
    actionExecute_->setStatusTip(tr("Execute current SQL statement"));

    actionExecuteSelection_ = menu_->addAction(tr("Execute &Selection"), this, &QueryMenu::onExecuteSelection);
    actionExecuteSelection_->setShortcut(QKeySequence("Ctrl+F9"));
    actionExecuteSelection_->setStatusTip(tr("Execute selected SQL text"));

    actionExecuteScript_ = menu_->addAction(tr("Execute &Script"), this, &QueryMenu::onExecuteScript);
    actionExecuteScript_->setStatusTip(tr("Execute all statements in script"));

    actionStop_ = menu_->addAction(tr("&Stop"), this, &QueryMenu::onStop);
    actionStop_->setStatusTip(tr("Stop current execution"));
    actionStop_->setEnabled(false);

    menu_->addSeparator();

    // Explain group
    actionExplain_ = menu_->addAction(tr("&Explain Plan"), this, &QueryMenu::onExplain);
    actionExplain_->setStatusTip(tr("Show query execution plan"));

    actionExplainAnalyze_ = menu_->addAction(tr("Explain &Analyze"), this, &QueryMenu::onExplainAnalyze);
    actionExplainAnalyze_->setStatusTip(tr("Execute and analyze query plan"));

    menu_->addSeparator();

    // Format group
    actionFormatSql_ = menu_->addAction(tr("&Format SQL"), this, &QueryMenu::onFormatSql);
    actionFormatSql_->setStatusTip(tr("Format SQL code"));

    actionComment_ = menu_->addAction(tr("&Comment Lines"), this, &QueryMenu::onComment);
    actionComment_->setStatusTip(tr("Comment selected lines"));

    actionUncomment_ = menu_->addAction(tr("&Uncomment Lines"), this, &QueryMenu::onUncomment);
    actionUncomment_->setStatusTip(tr("Uncomment selected lines"));

    actionToggleComment_ = menu_->addAction(tr("&Toggle Comment"), this, &QueryMenu::onToggleComment);
    actionToggleComment_->setShortcut(QKeySequence("Ctrl+/"));
    actionToggleComment_->setStatusTip(tr("Toggle comment on current line or selection"));

    menu_->addSeparator();

    // Case conversion group
    actionUppercase_ = menu_->addAction(tr("To &Uppercase"), this, &QueryMenu::onUppercase);
    actionUppercase_->setStatusTip(tr("Convert selection to uppercase"));

    actionLowercase_ = menu_->addAction(tr("To &Lowercase"), this, &QueryMenu::onLowercase);
    actionLowercase_->setStatusTip(tr("Convert selection to lowercase"));

    updateActionStates();
}

void QueryMenu::updateActionStates() {
    bool hasEditor = (editor_ != nullptr);

    actionExecute_->setEnabled(hasEditor && connected_ && !executing_);
    actionExecuteSelection_->setEnabled(hasEditor && connected_ && !executing_ && hasSelection_);
    actionExecuteScript_->setEnabled(hasEditor && connected_ && !executing_);
    actionStop_->setEnabled(executing_);
    actionExplain_->setEnabled(hasEditor && connected_ && !executing_);
    actionExplainAnalyze_->setEnabled(hasEditor && connected_ && !executing_);
    actionFormatSql_->setEnabled(hasEditor && !executing_);
    actionComment_->setEnabled(hasEditor && hasSelection_ && !executing_);
    actionUncomment_->setEnabled(hasEditor && hasSelection_ && !executing_);
    actionToggleComment_->setEnabled(hasEditor && !executing_);
    actionUppercase_->setEnabled(hasEditor && hasSelection_ && !executing_);
    actionLowercase_->setEnabled(hasEditor && hasSelection_ && !executing_);
}

void QueryMenu::setEditor(SqlEditor* editor) {
    editor_ = editor;
    updateActionStates();
}

void QueryMenu::setConnected(bool connected) {
    connected_ = connected;
    updateActionStates();
}

void QueryMenu::setExecuting(bool executing) {
    executing_ = executing;
    updateActionStates();
}

void QueryMenu::setHasSelection(bool hasSelection) {
    hasSelection_ = hasSelection;
    updateActionStates();
}

// Slot implementations
void QueryMenu::onExecute() {
    emit executeRequested();
}

void QueryMenu::onExecuteSelection() {
    emit executeSelectionRequested();
}

void QueryMenu::onExecuteScript() {
    emit executeScriptRequested();
}

void QueryMenu::onStop() {
    emit stopRequested();
}

void QueryMenu::onExplain() {
    emit explainRequested();
}

void QueryMenu::onExplainAnalyze() {
    emit explainAnalyzeRequested();
}

void QueryMenu::onFormatSql() {
    emit formatSqlRequested();
}

void QueryMenu::onComment() {
    emit commentRequested();
}

void QueryMenu::onUncomment() {
    emit uncommentRequested();
}

void QueryMenu::onToggleComment() {
    emit toggleCommentRequested();
}

void QueryMenu::onUppercase() {
    emit uppercaseRequested();
}

void QueryMenu::onLowercase() {
    emit lowercaseRequested();
}

} // namespace scratchrobin::ui
