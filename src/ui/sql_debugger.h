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
class QListView;
class QSplitter;
class QLineEdit;
class QCheckBox;
class QGroupBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief SQL Debugger - Step-through debugging for SQL statements
 * 
 * Debug SQL execution with:
 * - Breakpoint management
 * - Step through execution
 * - Variable inspection
 * - Call stack view
 * - Watch expressions
 */

// ============================================================================
// Debug Types
// ============================================================================
enum class DebugState {
    Idle,
    Starting,
    Running,
    Paused,
    StepInto,
    StepOver,
    StepOut,
    Stopped
};

struct Breakpoint {
    int id;
    QString file;
    int line;
    QString condition;
    bool isEnabled = true;
    int hitCount = 0;
    bool isHit = false;
};

struct DebugVariable {
    QString name;
    QString value;
    QString type;
    QString scope; // local, global, session
    bool isNull = false;
};

struct CallStackFrame {
    int level;
    QString routine;
    QString file;
    int line;
    QString schema;
};

struct DebugSession {
    int sessionId = 0;
    DebugState state = DebugState::Idle;
    QString currentFile;
    int currentLine = 0;
    QList<CallStackFrame> callStack;
    QList<DebugVariable> variables;
    QString lastError;
};

// ============================================================================
// SQL Debugger Panel
// ============================================================================
class SqlDebuggerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SqlDebuggerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("SQL Debugger"); }
    QString panelCategory() const override { return "debug"; }

public slots:
    // Session control
    void onStartDebugging(const QString& sql);
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
    void onRemoveAllBreakpoints();
    void onDisableAllBreakpoints();
    
    // Variables
    void onRefreshVariables();
    void onAddWatch();
    void onRemoveWatch();
    void onEditWatch();
    
    // Stack
    void onStackFrameSelected(int index);
    
    // Evaluation
    void onEvaluateExpression();

signals:
    void debuggingStarted();
    void debuggingStopped();
    void breakpointHit(int line);
    void executionPaused(int line);
    void executionContinued();
    void variableChanged(const QString& name, const QString& value);

private:
    void setupUi();
    void setupToolbar();
    void setupCodeView();
    void setupVariablesView();
    void setupStackView();
    void setupBreakpointsView();
    void setupWatchesView();
    void updateDebugState();
    void highlightCurrentLine();
    
    backend::SessionClient* client_;
    DebugSession session_;
    QList<Breakpoint> breakpoints_;
    QList<QString> watches_;
    
    // UI components
    QTextEdit* codeEditor_ = nullptr;
    QTableView* variablesTable_ = nullptr;
    QStandardItemModel* variablesModel_ = nullptr;
    QListView* stackList_ = nullptr;
    QStandardItemModel* stackModel_ = nullptr;
    QTableView* breakpointsTable_ = nullptr;
    QStandardItemModel* breakpointsModel_ = nullptr;
    QTableView* watchesTable_ = nullptr;
    QStandardItemModel* watchesModel_ = nullptr;
    
    // Toolbar buttons
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* continueBtn_ = nullptr;
    QPushButton* stepIntoBtn_ = nullptr;
    QPushButton* stepOverBtn_ = nullptr;
    QPushButton* stepOutBtn_ = nullptr;
    
    // Status
    QLabel* statusLabel_ = nullptr;
    QLabel* lineLabel_ = nullptr;
};

// ============================================================================
// Breakpoint Dialog
// ============================================================================
class BreakpointDialog : public QDialog {
    Q_OBJECT

public:
    explicit BreakpointDialog(Breakpoint* bp, QWidget* parent = nullptr);

public slots:
    void onAccept();

private:
    void setupUi();
    
    Breakpoint* breakpoint_;
    
    QLineEdit* fileEdit_ = nullptr;
    QLineEdit* lineEdit_ = nullptr;
    QTextEdit* conditionEdit_ = nullptr;
    QCheckBox* enabledCheck_ = nullptr;
    QCheckBox* hitCountCheck_ = nullptr;
    QLineEdit* hitCountEdit_ = nullptr;
};

// ============================================================================
// Watch Expression Dialog
// ============================================================================
class WatchDialog : public QDialog {
    Q_OBJECT

public:
    explicit WatchDialog(QString* expression, QWidget* parent = nullptr);

public slots:
    void onAccept();

private:
    void setupUi();
    
    QString* expression_;
    QLineEdit* exprEdit_ = nullptr;
};

// ============================================================================
// Evaluate Expression Dialog
// ============================================================================
class EvaluateDialog : public QDialog {
    Q_OBJECT

public:
    explicit EvaluateDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onEvaluate();

private:
    void setupUi();
    
    backend::SessionClient* client_;
    
    QLineEdit* exprEdit_ = nullptr;
    QTextEdit* resultEdit_ = nullptr;
};

} // namespace scratchrobin::ui
