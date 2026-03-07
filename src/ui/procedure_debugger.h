#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QLineEdit;
class QSplitter;
class QListWidget;
class QCheckBox;
class QGroupBox;
class QTreeView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Stored Procedure Debugger - PL/SQL debugging
 * 
 * Debug stored procedures/functions/triggers:
 * - Source code stepping
 * - Parameter inspection
 * - Local variable tracking
 * - Call stack navigation
 * - Conditional breakpoints
 */

// ============================================================================
// Procedure Debug Types
// ============================================================================
struct ProcDebugInfo {
    QString schema;
    QString name;
    QString type; // PROCEDURE, FUNCTION, TRIGGER, PACKAGE
    QString language; // PL/pgSQL, TSQL, etc.
    QStringList parameters;
    QString returnType;
    QString body;
    int lineCount = 0;
};

struct ProcedureBreakpoint {
    int id;
    QString schema;
    QString procedure;
    int line;
    QString condition;
    bool isEnabled = true;
};

struct ProcedureCallStackFrame {
    int level;
    QString schema;
    QString procedure;
    int line;
    QString statement;
};

struct ProcedureVariable {
    QString name;
    QString value;
    QString type;
    QString kind; // parameter, local, cursor, exception
    bool isNull = false;
};

enum class ProcedureDebugState {
    Idle,
    Starting,
    Running,
    Paused,
    StepInto,
    StepOver,
    StepOut,
    Stopped,
    Error
};

// ============================================================================
// Stored Procedure Debugger Panel
// ============================================================================
class ProcedureDebuggerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ProcedureDebuggerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Procedure Debugger"); }
    QString panelCategory() const override { return "debug"; }

public slots:
    // Procedure selection
    void onSelectProcedure();
    void onLoadProcedure(const QString& schema, const QString& name);
    void onProcedureSelected(const QModelIndex& index);
    
    // Debug control
    void onDebugProcedure();
    void onStopDebugging();
    void onPauseExecution();
    void onContinueExecution();
    
    // Stepping
    void onStepInto();
    void onStepOver();
    void onStepOut();
    void onRunToCursor();
    
    // Breakpoints
    void onToggleBreakpoint(int line);
    void onAddBreakpoint();
    void onEditBreakpoint();
    void onRemoveBreakpoint();
    void onClearAllBreakpoints();
    
    // Variables
    void onRefreshVariables();
    void onSetVariableValue();
    
    // Test execution
    void onTestWithValues();
    void onSaveTestCase();
    void onLoadTestCase();

signals:
    void debuggingStarted(const QString& procedure);
    void debuggingStopped();
    void breakpointHit(int line);
    void executionPaused(const QString& location);
    void variableChanged(const QString& name, const QString& value);

private:
    void setupUi();
    void setupProcedureBrowser();
    void setupCodeView();
    void setupVariablesPanel();
    void setupStackPanel();
    void setupBreakpointsPanel();
    void setupTestPanel();
    void loadProcedures();
    void updateDebugState();
    void highlightCurrentLine();
    
    backend::SessionClient* client_;
    ProcDebugInfo currentProcedure_;
    ProcedureDebugState debugState_ = ProcedureDebugState::Idle;
    QList<ProcedureBreakpoint> breakpoints_;
    QList<ProcedureCallStackFrame> callStack_;
    
    // UI components
    QTreeView* procedureTree_ = nullptr;
    QStandardItemModel* procedureModel_ = nullptr;
    QTextEdit* codeEditor_ = nullptr;
    QTableView* variablesTable_ = nullptr;
    QStandardItemModel* variablesModel_ = nullptr;
    QTableView* stackTable_ = nullptr;
    QStandardItemModel* stackModel_ = nullptr;
    QTableView* breakpointsTable_ = nullptr;
    QStandardItemModel* breakpointsModel_ = nullptr;
    
    // Test panel
    QTableView* parametersTable_ = nullptr;
    QStandardItemModel* parametersModel_ = nullptr;
    
    // Toolbar
    QPushButton* debugBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* continueBtn_ = nullptr;
    QPushButton* stepIntoBtn_ = nullptr;
    QPushButton* stepOverBtn_ = nullptr;
    QPushButton* stepOutBtn_ = nullptr;
    
    // Status
    QLabel* statusLabel_ = nullptr;
    QLabel* procedureLabel_ = nullptr;
    QLabel* lineLabel_ = nullptr;
};

// ============================================================================
// Debug Procedure Dialog
// ============================================================================
class DebugProcedureDialog : public QDialog {
    Q_OBJECT

public:
    explicit DebugProcedureDialog(const ProcDebugInfo& proc,
                                 backend::SessionClient* client,
                                 QWidget* parent = nullptr);

public slots:
    void onSetDefaults();
    void onDebug();
    void onTest();

private:
    void setupUi();
    void loadParameters();
    
    ProcDebugInfo procedure_;
    backend::SessionClient* client_;
    
    QTableView* paramsTable_ = nullptr;
    QStandardItemModel* paramsModel_ = nullptr;
    QCheckBox* useBreakpointCheck_ = nullptr;
    QLineEdit* breakpointLineEdit_ = nullptr;
};

// ============================================================================
// Set Variable Value Dialog
// ============================================================================
class SetVariableDialog : public QDialog {
    Q_OBJECT

public:
    explicit SetVariableDialog(ProcedureVariable* var, QWidget* parent = nullptr);

public slots:
    void onAccept();

private:
    void setupUi();
    
    ProcedureVariable* variable_;
    
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLineEdit* valueEdit_ = nullptr;
    QCheckBox* nullCheck_ = nullptr;
};

} // namespace scratchrobin::ui
