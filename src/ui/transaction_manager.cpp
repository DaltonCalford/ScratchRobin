#include "ui/transaction_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QAction>
#include <QTextEdit>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QDebug>

namespace scratchrobin::ui {

const QStringList TransactionManager::ISOLATION_LEVELS = {
    "READ UNCOMMITTED",
    "READ COMMITTED",
    "REPEATABLE READ",
    "SERIALIZABLE",
    "SNAPSHOT"
};

// ============================================================================
// TransactionManager
// ============================================================================

TransactionManager::TransactionManager(backend::SessionClient* client, QObject* parent)
    : QObject(parent), client_(client) {
    // Default state
    state_.autoCommit = true;
    state_.isolationLevel = "READ COMMITTED";
}

QStringList TransactionManager::supportedIsolationLevels() const {
    return ISOLATION_LEVELS;
}

qint64 TransactionManager::transactionDurationMs() const {
    if (!state_.inTransaction || state_.startTime.isNull()) {
        return 0;
    }
    return state_.startTime.msecsTo(QDateTime::currentDateTime());
}

void TransactionManager::setAutoCommit(bool enabled) {
    if (state_.autoCommit == enabled) return;
    
    state_.autoCommit = enabled;
    
    // If turning off auto-commit, we need to start a transaction
    if (!enabled && !state_.inTransaction) {
        beginTransaction();
    }
    
    emit autoCommitChanged(enabled);
    emit stateChanged(state_);
}

void TransactionManager::toggleAutoCommit() {
    setAutoCommit(!state_.autoCommit);
}

void TransactionManager::setIsolationLevel(const QString& level) {
    if (!ISOLATION_LEVELS.contains(level)) {
        emit transactionError(tr("Unsupported isolation level: %1").arg(level));
        return;
    }
    
    state_.isolationLevel = level;
    
    // Apply to current transaction if active
    if (state_.inTransaction) {
        QString sql = QString("SET TRANSACTION ISOLATION LEVEL %1").arg(level);
        executeSql(sql);
    }
    
    emit isolationLevelChanged(level);
    emit stateChanged(state_);
}

bool TransactionManager::beginTransaction() {
    if (state_.inTransaction) {
        emit transactionError(tr("Transaction already in progress"));
        return false;
    }
    
    if (!executeSql("BEGIN TRANSACTION")) {
        return false;
    }
    
    state_.inTransaction = true;
    state_.startTime = QDateTime::currentDateTime();
    state_.statementCount = 0;
    state_.savepointCount = 0;
    state_.executedStatements.clear();
    
    emit transactionStarted();
    emit stateChanged(state_);
    
    return true;
}

bool TransactionManager::commit() {
    if (!state_.inTransaction) {
        emit transactionError(tr("No transaction in progress"));
        return false;
    }
    
    if (!executeSql("COMMIT")) {
        return false;
    }
    
    state_.inTransaction = false;
    state_.savepointCount = 0;
    state_.executedStatements.clear();
    
    emit transactionCommitted();
    emit stateChanged(state_);
    
    // If auto-commit is off, start a new transaction
    if (!state_.autoCommit) {
        beginTransaction();
    }
    
    return true;
}

bool TransactionManager::rollback() {
    if (!state_.inTransaction) {
        emit transactionError(tr("No transaction in progress"));
        return false;
    }
    
    if (!executeSql("ROLLBACK")) {
        return false;
    }
    
    state_.inTransaction = false;
    state_.savepointCount = 0;
    state_.executedStatements.clear();
    
    emit transactionRolledBack();
    emit stateChanged(state_);
    
    // If auto-commit is off, start a new transaction
    if (!state_.autoCommit) {
        beginTransaction();
    }
    
    return true;
}

bool TransactionManager::createSavepoint(const QString& name) {
    if (!state_.inTransaction) {
        emit transactionError(tr("No transaction in progress"));
        return false;
    }
    
    QString sql = QString("SAVEPOINT %1").arg(name);
    if (!executeSql(sql)) {
        return false;
    }
    
    state_.savepointCount++;
    emit savepointCreated(name);
    emit stateChanged(state_);
    
    return true;
}

bool TransactionManager::rollbackToSavepoint(const QString& name) {
    if (!state_.inTransaction) {
        emit transactionError(tr("No transaction in progress"));
        return false;
    }
    
    QString sql = QString("ROLLBACK TO SAVEPOINT %1").arg(name);
    if (!executeSql(sql)) {
        return false;
    }
    
    emit savepointRolledBack(name);
    emit stateChanged(state_);
    
    return true;
}

bool TransactionManager::releaseSavepoint(const QString& name) {
    if (!state_.inTransaction) {
        emit transactionError(tr("No transaction in progress"));
        return false;
    }
    
    QString sql = QString("RELEASE SAVEPOINT %1").arg(name);
    if (!executeSql(sql)) {
        return false;
    }
    
    state_.savepointCount--;
    emit savepointReleased(name);
    emit stateChanged(state_);
    
    return true;
}

void TransactionManager::recordStatement(const QString& sql) {
    if (state_.inTransaction) {
        state_.statementCount++;
        state_.executedStatements.append(sql.simplified().left(200));
        emit stateChanged(state_);
    }
}

void TransactionManager::clearTransaction() {
    state_.inTransaction = false;
    state_.statementCount = 0;
    state_.savepointCount = 0;
    state_.executedStatements.clear();
    emit stateChanged(state_);
}

bool TransactionManager::executeSql(const QString& sql) {
    // Mock implementation - would execute via SessionClient
    Q_UNUSED(sql)
    // In real implementation:
    // return client_->execute(sql);
    return true;
}

void TransactionManager::updateState() {
    emit stateChanged(state_);
}

// ============================================================================
// TransactionIndicator
// ============================================================================

TransactionIndicator::TransactionIndicator(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    updateAppearance();
}

void TransactionIndicator::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(8);
    
    statusLabel_ = new QLabel(this);
    layout->addWidget(statusLabel_);
    
    countLabel_ = new QLabel(this);
    countLabel_->setStyleSheet("font-size: 10px; color: gray;");
    layout->addWidget(countLabel_);
    
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click for transaction details"));
}

void TransactionIndicator::setState(const TransactionState& state) {
    setInTransaction(state.inTransaction);
    setAutoCommit(state.autoCommit);
    setStatementCount(state.statementCount);
}

void TransactionIndicator::setInTransaction(bool inTransaction) {
    inTransaction_ = inTransaction;
    updateAppearance();
}

void TransactionIndicator::setAutoCommit(bool autoCommit) {
    autoCommit_ = autoCommit;
    updateAppearance();
}

void TransactionIndicator::setStatementCount(int count) {
    statementCount_ = count;
    if (count > 0) {
        countLabel_->setText(QString("(%1)").arg(count));
        countLabel_->setVisible(true);
    } else {
        countLabel_->setVisible(false);
    }
}

void TransactionIndicator::updateAppearance() {
    QString text;
    QString style;
    
    if (inTransaction_ && !autoCommit_) {
        text = tr("● TX Active");
        style = "background-color: #ff6b6b; color: white; border-radius: 4px; padding: 2px 8px; font-weight: bold;";
    } else if (!autoCommit_) {
        text = tr("○ TX Ready");
        style = "background-color: #4ecdc4; color: white; border-radius: 4px; padding: 2px 8px;";
    } else {
        text = tr("Auto-Commit");
        style = "background-color: #95a5a6; color: white; border-radius: 4px; padding: 2px 8px;";
    }
    
    statusLabel_->setText(text);
    setStyleSheet(style);
}

void TransactionIndicator::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}

void TransactionIndicator::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event)
    emit clicked();
}

// ============================================================================
// TransactionToolbar
// ============================================================================

TransactionToolbar::TransactionToolbar(TransactionManager* manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    
    if (manager_) {
        connect(manager_, &TransactionManager::stateChanged,
                this, &TransactionToolbar::updateButtonStates);
    }
}

void TransactionToolbar::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // Commit button
    commitBtn_ = new QPushButton(tr("Commit"), this);
    commitBtn_->setToolTip(tr("Commit transaction (Ctrl+Shift+C)"));
    commitBtn_->setEnabled(false);
    layout->addWidget(commitBtn_);
    
    commitAction_ = new QAction(tr("&Commit"), this);
    commitAction_->setShortcut(QKeySequence("Ctrl+Shift+C"));
    addAction(commitAction_);
    
    // Rollback button
    rollbackBtn_ = new QPushButton(tr("Rollback"), this);
    rollbackBtn_->setToolTip(tr("Rollback transaction (Ctrl+Shift+R)"));
    rollbackBtn_->setEnabled(false);
    layout->addWidget(rollbackBtn_);
    
    rollbackAction_ = new QAction(tr("&Rollback"), this);
    rollbackAction_->setShortcut(QKeySequence("Ctrl+Shift+R"));
    addAction(rollbackAction_);
    
    // Savepoint button
    savepointBtn_ = new QPushButton(tr("Savepoint"), this);
    savepointBtn_->setToolTip(tr("Create savepoint"));
    savepointBtn_->setEnabled(false);
    layout->addWidget(savepointBtn_);
    
    layout->addSpacing(16);
    
    // Auto-commit checkbox
    autoCommitAction_ = new QAction(tr("&Auto-Commit"), this);
    autoCommitAction_->setCheckable(true);
    autoCommitAction_->setChecked(true);
    layout->addWidget(new QLabel(tr("Auto-Commit:"), this));
    
    // Isolation level
    layout->addWidget(new QLabel(tr("Isolation:"), this));
    isolationCombo_ = new QComboBox(this);
    isolationCombo_->addItems(manager_->supportedIsolationLevels());
    isolationCombo_->setCurrentText("READ COMMITTED");
    layout->addWidget(isolationCombo_);
    
    // Status label
    statusLabel_ = new QLabel(tr("Ready"), this);
    layout->addWidget(statusLabel_);
    
    layout->addStretch();
    
    // Connections
    connect(commitBtn_, &QPushButton::clicked, this, &TransactionToolbar::onCommit);
    connect(rollbackBtn_, &QPushButton::clicked, this, &TransactionToolbar::onRollback);
    connect(commitAction_, &QAction::triggered, this, &TransactionToolbar::onCommit);
    connect(rollbackAction_, &QAction::triggered, this, &TransactionToolbar::onRollback);
    connect(autoCommitAction_, &QAction::toggled, manager_, &TransactionManager::setAutoCommit);
    connect(isolationCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TransactionToolbar::onIsolationLevelChanged);
}

void TransactionToolbar::updateButtonStates() {
    if (!manager_) return;
    
    bool inTx = manager_->inTransaction();
    bool autoCommit = manager_->autoCommit();
    
    commitBtn_->setEnabled(inTx && !autoCommit);
    rollbackBtn_->setEnabled(inTx && !autoCommit);
    savepointBtn_->setEnabled(inTx && !autoCommit);
    
    if (inTx && !autoCommit) {
        qint64 duration = manager_->transactionDurationMs();
        int seconds = duration / 1000;
        statusLabel_->setText(tr("TX Active - %1s, %2 statements")
                              .arg(seconds)
                              .arg(manager_->statementCount()));
    } else {
        statusLabel_->setText(autoCommit ? tr("Auto-Commit ON") : tr("TX Ready"));
    }
}

void TransactionToolbar::onCommit() {
    if (!manager_) return;
    
    auto reply = QMessageBox::question(this, tr("Commit Transaction"),
        tr("Are you sure you want to commit %1 statement(s)?")
        .arg(manager_->statementCount()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    
    if (reply == QMessageBox::Yes) {
        manager_->commit();
    }
}

void TransactionToolbar::onRollback() {
    if (!manager_) return;
    
    auto reply = QMessageBox::warning(this, tr("Rollback Transaction"),
        tr("Are you sure you want to rollback %1 statement(s)?\n"
           "This action cannot be undone.")
        .arg(manager_->statementCount()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        manager_->rollback();
    }
}

void TransactionToolbar::onToggleAutoCommit() {
    if (manager_) {
        manager_->toggleAutoCommit();
    }
}

void TransactionToolbar::onIsolationLevelChanged(int index) {
    Q_UNUSED(index)
    if (manager_) {
        manager_->setIsolationLevel(isolationCombo_->currentText());
    }
}

// ============================================================================
// TransactionDialog
// ============================================================================

TransactionDialog::TransactionDialog(TransactionManager* manager, QWidget* parent)
    : QDialog(parent), manager_(manager) {
    setWindowTitle(tr("Transaction Details"));
    setMinimumSize(500, 400);
    
    setupUi();
    
    if (manager_) {
        connect(manager_, &TransactionManager::stateChanged,
                this, &TransactionDialog::refresh);
    }
    
    refresh();
}

void TransactionDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Info section
    auto* infoGroup = new QWidget(this);
    auto* infoLayout = new QGridLayout(infoGroup);
    
    infoLayout->addWidget(new QLabel(tr("Status:"), this), 0, 0);
    infoLayout->addWidget(new QLabel(tr("Active"), this), 0, 1);
    
    infoLayout->addWidget(new QLabel(tr("Duration:"), this), 1, 0);
    durationLabel_ = new QLabel(tr("0s"), this);
    infoLayout->addWidget(durationLabel_, 1, 1);
    
    infoLayout->addWidget(new QLabel(tr("Statements:"), this), 2, 0);
    statementCountLabel_ = new QLabel(tr("0"), this);
    infoLayout->addWidget(statementCountLabel_, 2, 1);
    
    mainLayout->addWidget(infoGroup);
    
    // Statement log
    mainLayout->addWidget(new QLabel(tr("Executed Statements:"), this));
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setFont(QFont("Consolas", 9));
    mainLayout->addWidget(logEdit_);
    
    // Savepoints
    mainLayout->addWidget(new QLabel(tr("Savepoints:"), this));
    savepointList_ = new QListWidget(this);
    mainLayout->addWidget(savepointList_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    auto* createSpBtn = new QPushButton(tr("Create Savepoint"), this);
    buttonLayout->addWidget(createSpBtn);
    
    buttonLayout->addStretch();
    
    commitBtn_ = new QPushButton(tr("Commit"), this);
    buttonLayout->addWidget(commitBtn_);
    
    rollbackBtn_ = new QPushButton(tr("Rollback"), this);
    buttonLayout->addWidget(rollbackBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(commitBtn_, &QPushButton::clicked, this, &TransactionDialog::onCommit);
    connect(rollbackBtn_, &QPushButton::clicked, this, &TransactionDialog::onRollback);
    connect(createSpBtn, &QPushButton::clicked, this, &TransactionDialog::onCreateSavepoint);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    // Update timer
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TransactionDialog::refresh);
    timer->start(1000);
}

void TransactionDialog::refresh() {
    if (!manager_) return;
    
    const auto& state = manager_->state();
    
    qint64 duration = manager_->transactionDurationMs();
    durationLabel_->setText(tr("%1s").arg(duration / 1000));
    
    statementCountLabel_->setText(QString::number(state.statementCount));
    
    // Update log
    logEdit_->clear();
    for (const auto& stmt : state.executedStatements) {
        logEdit_->append(stmt);
    }
    
    updateButtons();
}

void TransactionDialog::updateButtons() {
    if (!manager_) return;
    
    bool inTx = manager_->inTransaction();
    commitBtn_->setEnabled(inTx);
    rollbackBtn_->setEnabled(inTx);
}

void TransactionDialog::onCommit() {
    if (manager_) {
        manager_->commit();
    }
}

void TransactionDialog::onRollback() {
    if (manager_) {
        manager_->rollback();
    }
}

void TransactionDialog::onCreateSavepoint() {
    if (!manager_) return;
    
    QString name = QString("sp_%1").arg(manager_->state().savepointCount + 1);
    manager_->createSavepoint(name);
}

void TransactionDialog::onRollbackToSavepoint() {
    // TODO: Implement
}

void TransactionDialog::onReleaseSavepoint() {
    // TODO: Implement
}

// ============================================================================
// SavepointManagerDialog
// ============================================================================

SavepointManagerDialog::SavepointManagerDialog(TransactionManager* manager, QWidget* parent)
    : QDialog(parent), manager_(manager) {
    setWindowTitle(tr("Savepoint Manager"));
    setMinimumSize(400, 300);
    
    setupUi();
    refresh();
}

void SavepointManagerDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // New savepoint
    auto* newLayout = new QHBoxLayout();
    newLayout->addWidget(new QLabel(tr("New Savepoint:"), this));
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Enter name..."));
    newLayout->addWidget(nameEdit_);
    
    auto* createBtn = new QPushButton(tr("Create"), this);
    newLayout->addWidget(createBtn);
    
    mainLayout->addLayout(newLayout);
    
    // Savepoint table
    savepointTable_ = new QTableWidget(this);
    savepointTable_->setColumnCount(3);
    savepointTable_->setHorizontalHeaderLabels({tr("Name"), tr("Created"), tr("Actions")});
    savepointTable_->horizontalHeader()->setStretchLastSection(true);
    mainLayout->addWidget(savepointTable_);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    auto* releaseAllBtn = new QPushButton(tr("Release All"), this);
    buttonLayout->addWidget(releaseAllBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(createBtn, &QPushButton::clicked, this, &SavepointManagerDialog::onCreateSavepoint);
    connect(releaseAllBtn, &QPushButton::clicked, this, &SavepointManagerDialog::onReleaseAll);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void SavepointManagerDialog::refresh() {
    // TODO: Load savepoints from manager
}

void SavepointManagerDialog::onCreateSavepoint() {
    if (!manager_ || nameEdit_->text().isEmpty()) return;
    
    manager_->createSavepoint(nameEdit_->text());
    nameEdit_->clear();
    refresh();
}

void SavepointManagerDialog::onRollbackToSavepoint() {
    // TODO: Implement
}

void SavepointManagerDialog::onReleaseSavepoint() {
    // TODO: Implement
}

void SavepointManagerDialog::onReleaseAll() {
    // TODO: Implement
}

} // namespace scratchrobin::ui
