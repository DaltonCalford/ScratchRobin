#include "sql_debugger.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QListView>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// SQL Debugger Panel
// ============================================================================

SqlDebuggerPanel::SqlDebuggerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("sql_debugger", parent)
    , client_(client) {
    setupUi();
    updateDebugState();
}

void SqlDebuggerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    setupToolbar();
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("Idle"), this);
    statusLayout->addWidget(new QLabel(tr("Status:"), this));
    statusLayout->addWidget(statusLabel_);
    statusLayout->addSpacing(20);
    lineLabel_ = new QLabel("-", this);
    statusLayout->addWidget(new QLabel(tr("Line:"), this));
    statusLayout->addWidget(lineLabel_);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Code editor
    setupCodeView();
    splitter->addWidget(codeEditor_);
    
    // Bottom tabs
    auto* bottomTabs = new QTabWidget(this);
    
    // Variables tab
    setupVariablesView();
    bottomTabs->addTab(variablesTable_, tr("Variables"));
    
    // Call stack tab
    setupStackView();
    bottomTabs->addTab(stackList_, tr("Call Stack"));
    
    // Breakpoints tab
    setupBreakpointsView();
    bottomTabs->addTab(breakpointsTable_, tr("Breakpoints"));
    
    // Watches tab
    setupWatchesView();
    bottomTabs->addTab(watchesTable_, tr("Watches"));
    
    splitter->addWidget(bottomTabs);
    splitter->setSizes({300, 200});
    
    mainLayout->addWidget(splitter, 1);
}

void SqlDebuggerPanel::setupToolbar() {
    auto* toolbarLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, [this]() {
        onStartDebugging(codeEditor_->toPlainText());
    });
    toolbarLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    connect(stopBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onStopDebugging);
    toolbarLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    connect(pauseBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onPauseExecution);
    toolbarLayout->addWidget(pauseBtn_);
    
    continueBtn_ = new QPushButton(tr("Continue"), this);
    connect(continueBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onContinueExecution);
    toolbarLayout->addWidget(continueBtn_);
    
    toolbarLayout->addSpacing(20);
    
    stepIntoBtn_ = new QPushButton(tr("Step Into"), this);
    connect(stepIntoBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onStepInto);
    toolbarLayout->addWidget(stepIntoBtn_);
    
    stepOverBtn_ = new QPushButton(tr("Step Over"), this);
    connect(stepOverBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onStepOver);
    toolbarLayout->addWidget(stepOverBtn_);
    
    stepOutBtn_ = new QPushButton(tr("Step Out"), this);
    connect(stepOutBtn_, &QPushButton::clicked, this, &SqlDebuggerPanel::onStepOut);
    toolbarLayout->addWidget(stepOutBtn_);
    
    toolbarLayout->addStretch();
    
    auto* addWatchBtn = new QPushButton(tr("Add Watch"), this);
    connect(addWatchBtn, &QPushButton::clicked, this, &SqlDebuggerPanel::onAddWatch);
    toolbarLayout->addWidget(addWatchBtn);
    
    auto* evalBtn = new QPushButton(tr("Evaluate"), this);
    connect(evalBtn, &QPushButton::clicked, this, &SqlDebuggerPanel::onEvaluateExpression);
    toolbarLayout->addWidget(evalBtn);
    
    static_cast<QVBoxLayout*>(layout())->addLayout(toolbarLayout);
}

void SqlDebuggerPanel::setupCodeView() {
    codeEditor_ = new QTextEdit(this);
    codeEditor_->setFont(QFont("Consolas", 10));
    codeEditor_->setPlaceholderText(tr("Enter SQL to debug..."));
}

void SqlDebuggerPanel::setupVariablesView() {
    variablesTable_ = new QTableView(this);
    variablesModel_ = new QStandardItemModel(this);
    variablesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type"), tr("Scope")});
    variablesTable_->setModel(variablesModel_);
    variablesTable_->setAlternatingRowColors(true);
}

void SqlDebuggerPanel::setupStackView() {
    stackList_ = new QListView(this);
    stackModel_ = new QStandardItemModel(this);
    stackList_->setModel(stackModel_);
}

void SqlDebuggerPanel::setupBreakpointsView() {
    breakpointsTable_ = new QTableView(this);
    breakpointsModel_ = new QStandardItemModel(this);
    breakpointsModel_->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Condition"), tr("Enabled")});
    breakpointsTable_->setModel(breakpointsModel_);
    breakpointsTable_->setAlternatingRowColors(true);
}

void SqlDebuggerPanel::setupWatchesView() {
    watchesTable_ = new QTableView(this);
    watchesModel_ = new QStandardItemModel(this);
    watchesModel_->setHorizontalHeaderLabels({tr("Expression"), tr("Value"), tr("Type")});
    watchesTable_->setModel(watchesModel_);
    watchesTable_->setAlternatingRowColors(true);
}

void SqlDebuggerPanel::updateDebugState() {
    bool isRunning = session_.state == DebugState::Running;
    bool isPaused = session_.state == DebugState::Paused;
    bool isIdle = session_.state == DebugState::Idle;
    
    startBtn_->setEnabled(isIdle);
    stopBtn_->setEnabled(!isIdle);
    pauseBtn_->setEnabled(isRunning);
    continueBtn_->setEnabled(isPaused);
    stepIntoBtn_->setEnabled(isPaused);
    stepOverBtn_->setEnabled(isPaused);
    stepOutBtn_->setEnabled(isPaused);
    
    switch (session_.state) {
    case DebugState::Idle:
        statusLabel_->setText(tr("Idle"));
        break;
    case DebugState::Running:
        statusLabel_->setText(tr("Running..."));
        break;
    case DebugState::Paused:
        statusLabel_->setText(tr("Paused at line %1").arg(session_.currentLine));
        break;
    case DebugState::Stopped:
        statusLabel_->setText(tr("Stopped"));
        break;
    default:
        break;
    }
}

void SqlDebuggerPanel::highlightCurrentLine() {
    // Highlight current execution line
}

void SqlDebuggerPanel::onStartDebugging(const QString& sql) {
    Q_UNUSED(sql)
    session_.state = DebugState::Running;
    session_.sessionId = 1;
    updateDebugState();
    emit debuggingStarted();
    
    // Simulate hitting a breakpoint
    QTimer::singleShot(1000, this, [this]() {
        session_.state = DebugState::Paused;
        session_.currentLine = 5;
        lineLabel_->setText(QString::number(session_.currentLine));
        updateDebugState();
        emit executionPaused(session_.currentLine);
        
        // Populate variables
        variablesModel_->clear();
        variablesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type"), tr("Scope")});
        variablesModel_->appendRow({
            new QStandardItem("customer_id"),
            new QStandardItem("123"),
            new QStandardItem("INTEGER"),
            new QStandardItem("local")
        });
        variablesModel_->appendRow({
            new QStandardItem("order_total"),
            new QStandardItem("456.78"),
            new QStandardItem("DECIMAL"),
            new QStandardItem("local")
        });
    });
}

void SqlDebuggerPanel::onStopDebugging() {
    session_.state = DebugState::Stopped;
    updateDebugState();
    emit debuggingStopped();
}

void SqlDebuggerPanel::onPauseExecution() {
    session_.state = DebugState::Paused;
    updateDebugState();
}

void SqlDebuggerPanel::onContinueExecution() {
    session_.state = DebugState::Running;
    updateDebugState();
    emit executionContinued();
}

void SqlDebuggerPanel::onStepInto() {
    session_.currentLine++;
    lineLabel_->setText(QString::number(session_.currentLine));
    updateDebugState();
}

void SqlDebuggerPanel::onStepOver() {
    session_.currentLine++;
    lineLabel_->setText(QString::number(session_.currentLine));
    updateDebugState();
}

void SqlDebuggerPanel::onStepOut() {
    session_.currentLine += 5;
    lineLabel_->setText(QString::number(session_.currentLine));
    updateDebugState();
}

void SqlDebuggerPanel::onRunToCursor() {
    // Run to cursor position
}

void SqlDebuggerPanel::onToggleBreakpoint(int line) {
    Q_UNUSED(line)
    
    Breakpoint bp;
    bp.id = breakpoints_.size() + 1;
    bp.file = "query.sql";
    bp.line = 10;
    bp.isEnabled = true;
    breakpoints_.append(bp);
    
    breakpointsModel_->clear();
    breakpointsModel_->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Condition"), tr("Enabled")});
    for (const auto& b : breakpoints_) {
        breakpointsModel_->appendRow({
            new QStandardItem(b.file),
            new QStandardItem(QString::number(b.line)),
            new QStandardItem(b.condition),
            new QStandardItem(b.isEnabled ? tr("Yes") : tr("No"))
        });
    }
}

void SqlDebuggerPanel::onAddBreakpoint() {
    onToggleBreakpoint(10);
}

void SqlDebuggerPanel::onEditBreakpoint() {
    auto index = breakpointsTable_->currentIndex();
    if (!index.isValid() || index.row() >= breakpoints_.size()) return;
    
    BreakpointDialog dialog(&breakpoints_[index.row()], this);
    dialog.exec();
}

void SqlDebuggerPanel::onRemoveBreakpoint() {
    auto index = breakpointsTable_->currentIndex();
    if (!index.isValid()) return;
    
    breakpoints_.removeAt(index.row());
    breakpointsModel_->removeRow(index.row());
}

void SqlDebuggerPanel::onRemoveAllBreakpoints() {
    breakpoints_.clear();
    breakpointsModel_->clear();
    breakpointsModel_->setHorizontalHeaderLabels({tr("File"), tr("Line"), tr("Condition"), tr("Enabled")});
}

void SqlDebuggerPanel::onDisableAllBreakpoints() {
    for (auto& bp : breakpoints_) {
        bp.isEnabled = false;
    }
    // Refresh view
}

void SqlDebuggerPanel::onRefreshVariables() {
    // Refresh variable values
}

void SqlDebuggerPanel::onAddWatch() {
    bool ok;
    QString expr = QInputDialog::getText(this, tr("Add Watch"),
        tr("Expression:"), QLineEdit::Normal, QString(), &ok);
    
    if (!ok || expr.isEmpty()) return;
    
    watches_.append(expr);
    watchesModel_->appendRow({
        new QStandardItem(expr),
        new QStandardItem("?"),
        new QStandardItem("unknown")
    });
}

void SqlDebuggerPanel::onRemoveWatch() {
    auto index = watchesTable_->currentIndex();
    if (!index.isValid()) return;
    
    watches_.removeAt(index.row());
    watchesModel_->removeRow(index.row());
}

void SqlDebuggerPanel::onEditWatch() {
    // Edit watch expression
}

void SqlDebuggerPanel::onStackFrameSelected(int index) {
    Q_UNUSED(index)
    // Navigate to stack frame
}

void SqlDebuggerPanel::onEvaluateExpression() {
    EvaluateDialog dialog(client_, this);
    dialog.exec();
}

// ============================================================================
// Breakpoint Dialog
// ============================================================================

BreakpointDialog::BreakpointDialog(Breakpoint* bp, QWidget* parent)
    : QDialog(parent)
    , breakpoint_(bp) {
    setupUi();
}

void BreakpointDialog::setupUi() {
    setWindowTitle(tr("Edit Breakpoint"));
    resize(400, 250);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    fileEdit_ = new QLineEdit(breakpoint_->file, this);
    formLayout->addRow(tr("File:"), fileEdit_);
    
    lineEdit_ = new QLineEdit(QString::number(breakpoint_->line), this);
    formLayout->addRow(tr("Line:"), lineEdit_);
    
    conditionEdit_ = new QTextEdit(this);
    conditionEdit_->setMaximumHeight(60);
    conditionEdit_->setPlainText(breakpoint_->condition);
    formLayout->addRow(tr("Condition:"), conditionEdit_);
    
    enabledCheck_ = new QCheckBox(tr("Enabled"), this);
    enabledCheck_->setChecked(breakpoint_->isEnabled);
    formLayout->addRow(tr("Status:"), enabledCheck_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &BreakpointDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void BreakpointDialog::onAccept() {
    breakpoint_->file = fileEdit_->text();
    breakpoint_->line = lineEdit_->text().toInt();
    breakpoint_->condition = conditionEdit_->toPlainText();
    breakpoint_->isEnabled = enabledCheck_->isChecked();
    accept();
}

// ============================================================================
// Watch Dialog
// ============================================================================

WatchDialog::WatchDialog(QString* expression, QWidget* parent)
    : QDialog(parent)
    , expression_(expression) {
    setupUi();
}

void WatchDialog::setupUi() {
    setWindowTitle(tr("Edit Watch"));
    resize(400, 100);
    
    auto* layout = new QVBoxLayout(this);
    
    exprEdit_ = new QLineEdit(*expression_, this);
    layout->addWidget(new QLabel(tr("Expression:"), this));
    layout->addWidget(exprEdit_);
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &WatchDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void WatchDialog::onAccept() {
    *expression_ = exprEdit_->text();
    accept();
}

// ============================================================================
// Evaluate Dialog
// ============================================================================

EvaluateDialog::EvaluateDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
}

void EvaluateDialog::setupUi() {
    setWindowTitle(tr("Evaluate Expression"));
    resize(500, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Expression:"), this));
    
    exprEdit_ = new QLineEdit(this);
    layout->addWidget(exprEdit_);
    
    auto* evalBtn = new QPushButton(tr("Evaluate"), this);
    connect(evalBtn, &QPushButton::clicked, this, &EvaluateDialog::onEvaluate);
    layout->addWidget(evalBtn);
    
    layout->addWidget(new QLabel(tr("Result:"), this));
    
    resultEdit_ = new QTextEdit(this);
    resultEdit_->setFont(QFont("Consolas", 9));
    resultEdit_->setReadOnly(true);
    layout->addWidget(resultEdit_, 1);
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void EvaluateDialog::onEvaluate() {
    // Evaluate expression (simulated)
    QString expr = exprEdit_->text();
    resultEdit_->setPlainText(tr("Evaluating: %1\nResult: %2")
        .arg(expr, "123"));
}

} // namespace scratchrobin::ui
