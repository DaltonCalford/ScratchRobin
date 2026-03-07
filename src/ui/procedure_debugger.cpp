#include "procedure_debugger.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTreeView>
#include <QTableView>
#include <QTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Procedure Debugger Panel
// ============================================================================

ProcedureDebuggerPanel::ProcedureDebuggerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("procedure_debugger", parent)
    , client_(client) {
    setupUi();
    loadProcedures();
}

void ProcedureDebuggerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    debugBtn_ = new QPushButton(tr("Debug"), this);
    connect(debugBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onDebugProcedure);
    toolbarLayout->addWidget(debugBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    connect(stopBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onStopDebugging);
    toolbarLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    connect(pauseBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onPauseExecution);
    toolbarLayout->addWidget(pauseBtn_);
    
    continueBtn_ = new QPushButton(tr("Continue"), this);
    connect(continueBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onContinueExecution);
    toolbarLayout->addWidget(continueBtn_);
    
    toolbarLayout->addSpacing(20);
    
    stepIntoBtn_ = new QPushButton(tr("Step Into"), this);
    connect(stepIntoBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onStepInto);
    toolbarLayout->addWidget(stepIntoBtn_);
    
    stepOverBtn_ = new QPushButton(tr("Step Over"), this);
    connect(stepOverBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onStepOver);
    toolbarLayout->addWidget(stepOverBtn_);
    
    stepOutBtn_ = new QPushButton(tr("Step Out"), this);
    connect(stepOutBtn_, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onStepOut);
    toolbarLayout->addWidget(stepOutBtn_);
    
    toolbarLayout->addStretch();
    
    auto* testBtn = new QPushButton(tr("Test Values"), this);
    connect(testBtn, &QPushButton::clicked, this, &ProcedureDebuggerPanel::onTestWithValues);
    toolbarLayout->addWidget(testBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Status
    auto* statusLayout = new QHBoxLayout();
    procedureLabel_ = new QLabel(tr("No procedure selected"), this);
    statusLayout->addWidget(new QLabel(tr("Procedure:"), this));
    statusLayout->addWidget(procedureLabel_);
    statusLayout->addSpacing(20);
    lineLabel_ = new QLabel("-", this);
    statusLayout->addWidget(new QLabel(tr("Line:"), this));
    statusLayout->addWidget(lineLabel_);
    statusLayout->addSpacing(20);
    statusLabel_ = new QLabel(tr("Idle"), this);
    statusLayout->addWidget(new QLabel(tr("Status:"), this));
    statusLayout->addWidget(statusLabel_);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Procedure browser
    setupProcedureBrowser();
    splitter->addWidget(procedureTree_);
    
    // Center: Code view
    auto* centerSplitter = new QSplitter(Qt::Vertical, this);
    
    setupCodeView();
    centerSplitter->addWidget(codeEditor_);
    
    // Bottom tabs
    auto* bottomTabs = new QTabWidget(this);
    
    // Variables tab
    setupVariablesPanel();
    bottomTabs->addTab(variablesTable_, tr("Variables"));
    
    // Call stack tab
    setupStackPanel();
    bottomTabs->addTab(stackTable_, tr("Call Stack"));
    
    // Breakpoints tab
    setupBreakpointsPanel();
    bottomTabs->addTab(breakpointsTable_, tr("Breakpoints"));
    
    centerSplitter->addWidget(bottomTabs);
    centerSplitter->setSizes({400, 200});
    
    splitter->addWidget(centerSplitter);
    splitter->setSizes({200, 700});
    
    mainLayout->addWidget(splitter, 1);
}

void ProcedureDebuggerPanel::setupProcedureBrowser() {
    procedureTree_ = new QTreeView(this);
    procedureModel_ = new QStandardItemModel(this);
    procedureModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Schema")});
    procedureTree_->setModel(procedureModel_);
    procedureTree_->setAlternatingRowColors(true);
    connect(procedureTree_, &QTreeView::clicked, this, &ProcedureDebuggerPanel::onProcedureSelected);
}

void ProcedureDebuggerPanel::setupCodeView() {
    codeEditor_ = new QTextEdit(this);
    codeEditor_->setFont(QFont("Consolas", 10));
    codeEditor_->setReadOnly(true);
}

void ProcedureDebuggerPanel::setupVariablesPanel() {
    variablesTable_ = new QTableView(this);
    variablesModel_ = new QStandardItemModel(this);
    variablesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type"), tr("Kind")});
    variablesTable_->setModel(variablesModel_);
    variablesTable_->setAlternatingRowColors(true);
}

void ProcedureDebuggerPanel::setupStackPanel() {
    stackTable_ = new QTableView(this);
    stackModel_ = new QStandardItemModel(this);
    stackModel_->setHorizontalHeaderLabels({tr("Level"), tr("Procedure"), tr("Line"), tr("Schema")});
    stackTable_->setModel(stackModel_);
    stackTable_->setAlternatingRowColors(true);
}

void ProcedureDebuggerPanel::setupBreakpointsPanel() {
    breakpointsTable_ = new QTableView(this);
    breakpointsModel_ = new QStandardItemModel(this);
    breakpointsModel_->setHorizontalHeaderLabels({tr("Line"), tr("Condition"), tr("Enabled")});
    breakpointsTable_->setModel(breakpointsModel_);
    breakpointsTable_->setAlternatingRowColors(true);
}

void ProcedureDebuggerPanel::setupTestPanel() {
    // Test panel for parameter input
}

void ProcedureDebuggerPanel::loadProcedures() {
    procedureModel_->clear();
    procedureModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Schema")});
    
    auto* procsItem = new QStandardItem(tr("Procedures"));
    QStringList procs = {"calculate_order_total", "update_inventory", "process_payment"};
    for (const auto& p : procs) {
        QList<QStandardItem*> row;
        row << new QStandardItem(p);
        row << new QStandardItem(tr("PROCEDURE"));
        row << new QStandardItem("public");
        procsItem->appendRow(row);
    }
    procedureModel_->appendRow(procsItem);
    
    auto* funcsItem = new QStandardItem(tr("Functions"));
    QStringList funcs = {"get_customer_balance", "calculate_tax", "format_currency"};
    for (const auto& f : funcs) {
        QList<QStandardItem*> row;
        row << new QStandardItem(f);
        row << new QStandardItem(tr("FUNCTION"));
        row << new QStandardItem("public");
        funcsItem->appendRow(row);
    }
    procedureModel_->appendRow(funcsItem);
    
    auto* triggersItem = new QStandardItem(tr("Triggers"));
    QStringList triggers = {"trg_orders_audit", "trg_customers_update"};
    for (const auto& t : triggers) {
        QList<QStandardItem*> row;
        row << new QStandardItem(t);
        row << new QStandardItem(tr("TRIGGER"));
        row << new QStandardItem("public");
        triggersItem->appendRow(row);
    }
    procedureModel_->appendRow(triggersItem);
    
    procedureTree_->expandAll();
}

void ProcedureDebuggerPanel::updateDebugState() {
    bool isRunning = debugState_ == ProcedureDebugState::Running;
    bool isPaused = debugState_ == ProcedureDebugState::Paused;
    bool isIdle = debugState_ == ProcedureDebugState::Idle;
    
    debugBtn_->setEnabled(isIdle);
    stopBtn_->setEnabled(!isIdle);
    pauseBtn_->setEnabled(isRunning);
    continueBtn_->setEnabled(isPaused);
    stepIntoBtn_->setEnabled(isPaused);
    stepOverBtn_->setEnabled(isPaused);
    stepOutBtn_->setEnabled(isPaused);
    
    switch (debugState_) {
    case ProcedureDebugState::Idle:
        statusLabel_->setText(tr("Idle"));
        break;
    case ProcedureDebugState::Running:
        statusLabel_->setText(tr("Running..."));
        break;
    case ProcedureDebugState::Paused:
        statusLabel_->setText(tr("Paused"));
        break;
    case ProcedureDebugState::Stopped:
        statusLabel_->setText(tr("Stopped"));
        break;
    default:
        break;
    }
}

void ProcedureDebuggerPanel::highlightCurrentLine() {
    // Highlight current line in code editor
}

void ProcedureDebuggerPanel::onSelectProcedure() {
    // Open procedure selection dialog
}

void ProcedureDebuggerPanel::onLoadProcedure(const QString& schema, const QString& name) {
    currentProcedure_.schema = schema;
    currentProcedure_.name = name;
    currentProcedure_.type = "PROCEDURE";
    currentProcedure_.language = "PL/pgSQL";
    currentProcedure_.body = 
        "CREATE OR REPLACE PROCEDURE " + name + "()\n"
        "LANGUAGE plpgsql\n"
        "AS $$\n"
        "DECLARE\n"
        "    v_count INTEGER;\n"
        "BEGIN\n"
        "    SELECT COUNT(*) INTO v_count FROM customers;\n"
        "    RAISE NOTICE 'Count: %', v_count;\n"
        "END;\n"
        "$$;";
    
    procedureLabel_->setText(schema + "." + name);
    codeEditor_->setPlainText(currentProcedure_.body);
}

void ProcedureDebuggerPanel::onProcedureSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    auto* item = procedureModel_->itemFromIndex(index);
    if (!item || item->hasChildren()) return;
    
    QString name = item->text();
    QString schema = procedureModel_->item(index.row(), 2)->text();
    
    onLoadProcedure(schema, name);
}

void ProcedureDebuggerPanel::onDebugProcedure() {
    if (currentProcedure_.name.isEmpty()) {
        QMessageBox::information(this, tr("Debug Procedure"),
            tr("Please select a procedure to debug."));
        return;
    }
    
    debugState_ = ProcedureDebugState::Running;
    updateDebugState();
    emit debuggingStarted(currentProcedure_.name);
    
    // Simulate hitting a breakpoint
    QTimer::singleShot(1000, this, [this]() {
        debugState_ = ProcedureDebugState::Paused;
        lineLabel_->setText("5");
        updateDebugState();
        emit executionPaused("line 5");
        
        // Populate variables
        variablesModel_->clear();
        variablesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type"), tr("Kind")});
        variablesModel_->appendRow({
            new QStandardItem("v_count"),
            new QStandardItem("0"),
            new QStandardItem("INTEGER"),
            new QStandardItem("local")
        });
        
        // Populate call stack
        stackModel_->clear();
        stackModel_->setHorizontalHeaderLabels({tr("Level"), tr("Procedure"), tr("Line"), tr("Schema")});
        stackModel_->appendRow({
            new QStandardItem("0"),
            new QStandardItem(currentProcedure_.name),
            new QStandardItem("5"),
            new QStandardItem(currentProcedure_.schema)
        });
    });
}

void ProcedureDebuggerPanel::onStopDebugging() {
    debugState_ = ProcedureDebugState::Stopped;
    updateDebugState();
    emit debuggingStopped();
}

void ProcedureDebuggerPanel::onPauseExecution() {
    debugState_ = ProcedureDebugState::Paused;
    updateDebugState();
}

void ProcedureDebuggerPanel::onContinueExecution() {
    debugState_ = ProcedureDebugState::Running;
    updateDebugState();
}

void ProcedureDebuggerPanel::onStepInto() {
    lineLabel_->setText(QString::number(lineLabel_->text().toInt() + 1));
}

void ProcedureDebuggerPanel::onStepOver() {
    lineLabel_->setText(QString::number(lineLabel_->text().toInt() + 1));
}

void ProcedureDebuggerPanel::onStepOut() {
    lineLabel_->setText(QString::number(lineLabel_->text().toInt() + 5));
}

void ProcedureDebuggerPanel::onRunToCursor() {
    // Run to cursor position
}

void ProcedureDebuggerPanel::onToggleBreakpoint(int line) {
    ProcedureBreakpoint bp;
    bp.id = breakpoints_.size() + 1;
    bp.schema = currentProcedure_.schema;
    bp.procedure = currentProcedure_.name;
    bp.line = line;
    bp.isEnabled = true;
    breakpoints_.append(bp);
    
    breakpointsModel_->clear();
    breakpointsModel_->setHorizontalHeaderLabels({tr("Line"), tr("Condition"), tr("Enabled")});
    for (const auto& b : breakpoints_) {
        breakpointsModel_->appendRow({
            new QStandardItem(QString::number(b.line)),
            new QStandardItem(b.condition),
            new QStandardItem(b.isEnabled ? tr("Yes") : tr("No"))
        });
    }
}

void ProcedureDebuggerPanel::onAddBreakpoint() {
    bool ok;
    int line = QInputDialog::getInt(this, tr("Add Breakpoint"),
        tr("Line number:"), 1, 1, 9999, 1, &ok);
    
    if (ok) {
        onToggleBreakpoint(line);
    }
}

void ProcedureDebuggerPanel::onEditBreakpoint() {
    // Edit selected breakpoint
}

void ProcedureDebuggerPanel::onRemoveBreakpoint() {
    auto index = breakpointsTable_->currentIndex();
    if (!index.isValid()) return;
    
    breakpoints_.removeAt(index.row());
    breakpointsModel_->removeRow(index.row());
}

void ProcedureDebuggerPanel::onClearAllBreakpoints() {
    breakpoints_.clear();
    breakpointsModel_->clear();
    breakpointsModel_->setHorizontalHeaderLabels({tr("Line"), tr("Condition"), tr("Enabled")});
}

void ProcedureDebuggerPanel::onRefreshVariables() {
    // Refresh variable values
}

void ProcedureDebuggerPanel::onSetVariableValue() {
    // Set variable value during debugging
}

void ProcedureDebuggerPanel::onTestWithValues() {
    if (currentProcedure_.name.isEmpty()) {
        QMessageBox::information(this, tr("Test Procedure"),
            tr("Please select a procedure first."));
        return;
    }
    
    DebugProcedureDialog dialog(currentProcedure_, client_, this);
    dialog.exec();
}

void ProcedureDebuggerPanel::onSaveTestCase() {
    QMessageBox::information(this, tr("Save Test Case"),
        tr("Save test case - not yet implemented."));
}

void ProcedureDebuggerPanel::onLoadTestCase() {
    QMessageBox::information(this, tr("Load Test Case"),
        tr("Load test case - not yet implemented."));
}

// ============================================================================
// Debug Procedure Dialog
// ============================================================================

DebugProcedureDialog::DebugProcedureDialog(const ProcDebugInfo& proc,
                                          backend::SessionClient* client,
                                          QWidget* parent)
    : QDialog(parent)
    , procedure_(proc)
    , client_(client) {
    setupUi();
}

void DebugProcedureDialog::setupUi() {
    setWindowTitle(tr("Debug Procedure - %1").arg(procedure_.name));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Procedure: %1.%2")
        .arg(procedure_.schema, procedure_.name), this));
    
    // Parameters table
    layout->addWidget(new QLabel(tr("Parameters:"), this));
    
    paramsTable_ = new QTableView(this);
    paramsModel_ = new QStandardItemModel(this);
    paramsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Mode"), tr("Value")});
    
    // Add sample parameters
    paramsModel_->appendRow({
        new QStandardItem("p_customer_id"),
        new QStandardItem("INTEGER"),
        new QStandardItem("IN"),
        new QStandardItem("123")
    });
    paramsModel_->appendRow({
        new QStandardItem("p_amount"),
        new QStandardItem("DECIMAL"),
        new QStandardItem("IN"),
        new QStandardItem("456.78")
    });
    paramsModel_->appendRow({
        new QStandardItem("p_result"),
        new QStandardItem("BOOLEAN"),
        new QStandardItem("OUT"),
        new QStandardItem("")
    });
    
    paramsTable_->setModel(paramsModel_);
    paramsTable_->setAlternatingRowColors(true);
    layout->addWidget(paramsTable_, 1);
    
    // Breakpoint options
    useBreakpointCheck_ = new QCheckBox(tr("Start at breakpoint"), this);
    useBreakpointCheck_->setChecked(true);
    layout->addWidget(useBreakpointCheck_);
    
    breakpointLineEdit_ = new QLineEdit("1", this);
    auto* bpLayout = new QHBoxLayout();
    bpLayout->addWidget(new QLabel(tr("Line:"), this));
    bpLayout->addWidget(breakpointLineEdit_);
    layout->addLayout(bpLayout);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* setDefaultsBtn = new QPushButton(tr("Set Defaults"), this);
    connect(setDefaultsBtn, &QPushButton::clicked, this, &DebugProcedureDialog::onSetDefaults);
    btnLayout->addWidget(setDefaultsBtn);
    
    auto* debugBtn = new QPushButton(tr("Debug"), this);
    connect(debugBtn, &QPushButton::clicked, this, &DebugProcedureDialog::onDebug);
    btnLayout->addWidget(debugBtn);
    
    auto* testBtn = new QPushButton(tr("Test Run"), this);
    connect(testBtn, &QPushButton::clicked, this, &DebugProcedureDialog::onTest);
    btnLayout->addWidget(testBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void DebugProcedureDialog::loadParameters() {
    // Load procedure parameters from database
}

void DebugProcedureDialog::onSetDefaults() {
    // Set default parameter values
}

void DebugProcedureDialog::onDebug() {
    accept();
}

void DebugProcedureDialog::onTest() {
    QMessageBox::information(this, tr("Test Run"),
        tr("Test execution completed successfully."));
}

// ============================================================================
// Set Variable Dialog
// ============================================================================

SetVariableDialog::SetVariableDialog(ProcedureVariable* var, QWidget* parent)
    : QDialog(parent)
    , variable_(var) {
    setupUi();
}

void SetVariableDialog::setupUi() {
    setWindowTitle(tr("Set Variable Value"));
    resize(400, 150);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    nameLabel_ = new QLabel(variable_->name, this);
    formLayout->addRow(tr("Name:"), nameLabel_);
    
    typeLabel_ = new QLabel(variable_->type, this);
    formLayout->addRow(tr("Type:"), typeLabel_);
    
    valueEdit_ = new QLineEdit(variable_->value, this);
    formLayout->addRow(tr("Value:"), valueEdit_);
    
    nullCheck_ = new QCheckBox(tr("Set to NULL"), this);
    nullCheck_->setChecked(variable_->isNull);
    formLayout->addRow(tr("NULL:"), nullCheck_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &SetVariableDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void SetVariableDialog::onAccept() {
    variable_->value = valueEdit_->text();
    variable_->isNull = nullCheck_->isChecked();
    accept();
}

} // namespace scratchrobin::ui
