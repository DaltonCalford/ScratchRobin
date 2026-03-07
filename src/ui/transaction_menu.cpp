#include "ui/transaction_menu.h"

#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidget>

namespace scratchrobin::ui {

TransactionMenu::TransactionMenu(QWidget* parent)
    : QObject(parent)
    , parent_(parent) {
    setupActions();
}

TransactionMenu::~TransactionMenu() = default;

void TransactionMenu::setupActions() {
    menu_ = new QMenu(tr("&Transaction"), parent_);

    // Transaction control group
    actionStart_ = menu_->addAction(tr("&Start Transaction"), this, &TransactionMenu::onStartTransaction);
    actionStart_->setStatusTip(tr("Begin a new transaction"));

    actionCommit_ = menu_->addAction(tr("&Commit"), this, &TransactionMenu::onCommit);
    actionCommit_->setStatusTip(tr("Commit current transaction"));

    actionRollback_ = menu_->addAction(tr("&Rollback"), this, &TransactionMenu::onRollback);
    actionRollback_->setStatusTip(tr("Rollback current transaction"));

    actionSavepoint_ = menu_->addAction(tr("Set &Savepoint..."), this, &TransactionMenu::onSetSavepoint);
    actionSavepoint_->setStatusTip(tr("Create a savepoint within current transaction"));

    menu_->addSeparator();

    // Transaction options group
    actionAutoCommit_ = menu_->addAction(tr("&Auto Commit"), this, &TransactionMenu::onAutoCommitToggled);
    actionAutoCommit_->setCheckable(true);
    actionAutoCommit_->setChecked(autoCommit_);
    actionAutoCommit_->setStatusTip(tr("Automatically commit each statement"));

    // Isolation level submenu
    createIsolationSubmenu();

    menu_->addSeparator();

    actionReadOnly_ = menu_->addAction(tr("&Read Only"), this, &TransactionMenu::onReadOnlyToggled);
    actionReadOnly_->setCheckable(true);
    actionReadOnly_->setChecked(readOnly_);
    actionReadOnly_->setStatusTip(tr("Set transaction to read-only mode"));

    updateActionStates();
}

void TransactionMenu::createIsolationSubmenu() {
    isolationMenu_ = new QMenu(tr("&Isolation Level"), menu_);

    auto* isolationGroup = new QActionGroup(this);
    isolationGroup->setExclusive(true);

    actionReadUncommitted_ = isolationMenu_->addAction(tr("&Read Uncommitted"), 
        this, &TransactionMenu::onIsolationLevelTriggered);
    actionReadUncommitted_->setCheckable(true);
    actionReadUncommitted_->setActionGroup(isolationGroup);
    actionReadUncommitted_->setData("Read Uncommitted");

    actionReadCommitted_ = isolationMenu_->addAction(tr("&Read Committed"), 
        this, &TransactionMenu::onIsolationLevelTriggered);
    actionReadCommitted_->setCheckable(true);
    actionReadCommitted_->setActionGroup(isolationGroup);
    actionReadCommitted_->setData("Read Committed");
    actionReadCommitted_->setChecked(true);  // Default

    actionRepeatableRead_ = isolationMenu_->addAction(tr("&Repeatable Read"), 
        this, &TransactionMenu::onIsolationLevelTriggered);
    actionRepeatableRead_->setCheckable(true);
    actionRepeatableRead_->setActionGroup(isolationGroup);
    actionRepeatableRead_->setData("Repeatable Read");

    actionSerializable_ = isolationMenu_->addAction(tr("&Serializable"), 
        this, &TransactionMenu::onIsolationLevelTriggered);
    actionSerializable_->setCheckable(true);
    actionSerializable_->setActionGroup(isolationGroup);
    actionSerializable_->setData("Serializable");

    menu_->addMenu(isolationMenu_);
}

void TransactionMenu::updateActionStates() {
    bool noTx = (txState_ == TxState::NoTransaction);
    bool inTx = (txState_ == TxState::InTransaction);

    // Connection-dependent
    actionStart_->setEnabled(connected_ && noTx && !autoCommit_);
    actionCommit_->setEnabled(connected_ && inTx);
    actionRollback_->setEnabled(connected_ && inTx);
    actionSavepoint_->setEnabled(connected_ && inTx);
    actionAutoCommit_->setEnabled(connected_);
    isolationMenu_->setEnabled(connected_);
    actionReadOnly_->setEnabled(connected_ && noTx);

    // Update check states
    actionAutoCommit_->setChecked(autoCommit_);
    actionReadOnly_->setChecked(readOnly_);

    // Update isolation level check
    actionReadUncommitted_->setChecked(isolationLevel_ == "Read Uncommitted");
    actionReadCommitted_->setChecked(isolationLevel_ == "Read Committed");
    actionRepeatableRead_->setChecked(isolationLevel_ == "Repeatable Read");
    actionSerializable_->setChecked(isolationLevel_ == "Serializable");
}

void TransactionMenu::setConnected(bool connected) {
    connected_ = connected;
    updateActionStates();
}

void TransactionMenu::setTransactionState(TxState state) {
    txState_ = state;
    updateActionStates();
}

void TransactionMenu::setAutoCommit(bool autoCommit) {
    autoCommit_ = autoCommit;
    updateActionStates();
}

void TransactionMenu::setIsolationLevel(const QString& level) {
    isolationLevel_ = level;
    updateActionStates();
}

void TransactionMenu::setReadOnly(bool readOnly) {
    readOnly_ = readOnly;
    updateActionStates();
}

// Slot implementations
void TransactionMenu::onStartTransaction() {
    emit startTransactionRequested();
}

void TransactionMenu::onCommit() {
    emit commitRequested();
}

void TransactionMenu::onRollback() {
    emit rollbackRequested();
}

void TransactionMenu::onSetSavepoint() {
    emit setSavepointRequested();
}

void TransactionMenu::onAutoCommitToggled(bool checked) {
    autoCommit_ = checked;
    emit autoCommitToggled(checked);
    updateActionStates();
}

void TransactionMenu::onIsolationLevelTriggered() {
    auto* action = qobject_cast<QAction*>(sender());
    if (action) {
        isolationLevel_ = action->data().toString();
        emit isolationLevelChanged(isolationLevel_);
        updateActionStates();
    }
}

void TransactionMenu::onReadOnlyToggled(bool checked) {
    readOnly_ = checked;
    emit readOnlyToggled(checked);
    updateActionStates();
}

} // namespace scratchrobin::ui
