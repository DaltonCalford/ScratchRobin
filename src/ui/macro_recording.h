#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>
#include <QKeySequence>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QListWidget;
class QPlainTextEdit;
class QDialogButtonBox;
class QKeySequenceEdit;
class QTreeWidget;
class QProgressBar;
class QSpinBox;
class QStackedWidget;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Macro Recording - Automation
 * 
 * Record and replay user actions:
 * - Record keyboard and mouse actions
 * - Edit macro steps
 * - Save and load macros
 * - Assign hotkeys to macros
 * - Conditional logic and loops
 * - Variable support
 */

// ============================================================================
// Macro Types
// ============================================================================
enum class MacroActionType {
    KeyPress,
    KeyRelease,
    MouseClick,
    MouseMove,
    MouseWheel,
    Delay,
    TextInput,
    Command,
    SQLExecute,
    MenuSelect,
    ToolbarClick,
    DialogAction,
    WaitFor,
    Conditional,
    Loop,
    VariableSet,
    VariableGet
};

enum class MacroTriggerType {
    Hotkey,
    Menu,
    Toolbar,
    Startup,
    Scheduled,
    EventBased
};

// ============================================================================
// Macro Structures
// ============================================================================
struct MacroAction {
    QString id;
    MacroActionType type;
    QString description;
    QDateTime timestamp;
    
    // Key action data
    int keyCode = 0;
    Qt::KeyboardModifiers modifiers;
    QString text;
    
    // Mouse action data
    QPoint position;
    Qt::MouseButton button;
    int wheelDelta = 0;
    
    // Command data
    QString command;
    QStringList arguments;
    
    // SQL data
    QString sql;
    
    // Delay
    int delayMs = 0;
    
    // Variable data
    QString variableName;
    QVariant variableValue;
    
    // Conditional/loop data
    QString condition;
    QStringList nestedActions;
    
    bool enabled = true;
    bool captureScreenshot = false;
};

struct Macro {
    QString id;
    QString name;
    QString description;
    QString category;
    QString author;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QDateTime lastRunAt;
    int runCount = 0;
    
    QList<MacroAction> actions;
    QKeySequence hotkey;
    MacroTriggerType triggerType;
    QStringList tags;
    bool isReadOnly = false;
    bool showInToolbar = false;
    QString icon;
    
    // Variables used by the macro
    QHash<QString, QVariant> variables;
    
    bool operator==(const Macro& other) const {
        return id == other.id;
    }
    
    // Playback settings
    int playbackSpeed = 100;  // Percentage
    bool repeat = false;
    int repeatCount = 1;
    bool stopOnError = true;
};

struct MacroRecordingState {
    bool isRecording = false;
    bool isPaused = false;
    bool isPlaying = false;
    QDateTime startedAt;
    int actionCount = 0;
    Macro currentMacro;
};

// ============================================================================
// Macro Manager Panel
// ============================================================================
class MacroManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit MacroManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Macro Manager"); }
    QString panelCategory() const override { return "automation"; }

public slots:
    // Recording control
    void onStartRecording();
    void onStopRecording();
    void onPauseRecording();
    void onResumeRecording();
    
    // Playback control
    void onPlayMacro();
    void onPausePlayback();
    void onStopPlayback();
    void onStepForward();
    void onStepBackward();
    
    // Macro management
    void onMacroSelected(const QModelIndex& index);
    void onNewMacro();
    void onEditMacro();
    void onDuplicateMacro();
    void onDeleteMacro();
    void onSaveMacro();
    void onLoadMacro();
    void onExportMacro();
    void onImportMacro();
    
    // Organization
    void onFilterByCategory(const QString& category);
    void onSearchTextChanged(const QString& text);
    void onAssignHotkey();
    void onAddToToolbar();
    void onCreateFolder();

signals:
    void macroStarted(const QString& macroId);
    void macroStopped(const QString& macroId);
    void macroPaused(const QString& macroId);
    void recordingStarted();
    void recordingStopped();

private:
    void setupUi();
    void setupMacroList();
    void setupRecordingPanel();
    void setupPlaybackPanel();
    void loadMacros();
    void updateMacroList();
    void updateRecordingStatus();
    void updatePlaybackStatus();
    
    backend::SessionClient* client_;
    QList<Macro> macros_;
    Macro currentMacro_;
    MacroRecordingState recordingState_;
    int currentActionIndex_ = -1;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    
    // Macro list
    QTableView* macroTable_ = nullptr;
    QStandardItemModel* macroModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* categoryFilter_ = nullptr;
    
    // Recording panel
    QLabel* recordingStatusLabel_ = nullptr;
    QLabel* actionCountLabel_ = nullptr;
    QPushButton* recordBtn_ = nullptr;
    QPushButton* stopRecordBtn_ = nullptr;
    QPushButton* pauseRecordBtn_ = nullptr;
    QListWidget* recordingLog_ = nullptr;
    
    // Playback panel
    QLabel* playbackStatusLabel_ = nullptr;
    QProgressBar* playbackProgress_ = nullptr;
    QPushButton* playBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* stepForwardBtn_ = nullptr;
    QPushButton* stepBackBtn_ = nullptr;
    QSpinBox* speedSpin_ = nullptr;
};

// ============================================================================
// Macro Editor Dialog
// ============================================================================
class MacroEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit MacroEditorDialog(Macro* macro, backend::SessionClient* client, 
                               QWidget* parent = nullptr);

public slots:
    // Action editing
    void onActionSelected(const QModelIndex& index);
    void onAddAction();
    void onEditAction();
    void onDeleteAction();
    void onMoveUp();
    void onMoveDown();
    void onDuplicateAction();
    void onToggleActionEnabled();
    
    // Recording
    void onRecordNewActions();
    void onInsertRecording();
    
    // Properties
    void onNameChanged(const QString& name);
    void onDescriptionChanged();
    void onHotkeyChanged(const QKeySequence& key);
    void onCategoryChanged(const QString& category);
    
    // Variables
    void onAddVariable();
    void onEditVariable();
    void onDeleteVariable();
    
    void onValidate();
    void onTestMacro();

private:
    void setupUi();
    void setupActionList();
    void setupPropertiesPanel();
    void setupVariablesPanel();
    void updateActionList();
    void updateActionDetails(const MacroAction& action);
    
    Macro* macro_;
    backend::SessionClient* client_;
    int currentActionIndex_ = -1;
    
    QTabWidget* tabWidget_ = nullptr;
    
    // Actions tab
    QTableView* actionTable_ = nullptr;
    QStandardItemModel* actionModel_ = nullptr;
    QPlainTextEdit* actionDetailsEdit_ = nullptr;
    
    // Properties tab
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QKeySequenceEdit* hotkeyEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QCheckBox* toolbarCheck_ = nullptr;
    QSpinBox* speedSpin_ = nullptr;
    
    // Variables tab
    QTableView* variableTable_ = nullptr;
    QStandardItemModel* variableModel_ = nullptr;
    
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Action Editor Dialog
// ============================================================================
class ActionEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ActionEditorDialog(MacroAction* action, QWidget* parent = nullptr);
    
    MacroAction action() const;

public slots:
    void onTypeChanged(int index);
    void onValidate();

private:
    void setupUi();
    void updateFieldsForType(MacroActionType type);
    
    MacroAction* action_;
    MacroAction originalAction_;
    
    QComboBox* typeCombo_ = nullptr;
    QStackedWidget* fieldsStack_ = nullptr;
    
    // Key action fields
    QKeySequenceEdit* keyEdit_ = nullptr;
    
    // Text input fields
    QPlainTextEdit* textEdit_ = nullptr;
    
    // Command fields
    QLineEdit* commandEdit_ = nullptr;
    QLineEdit* argsEdit_ = nullptr;
    
    // SQL fields
    QPlainTextEdit* sqlEdit_ = nullptr;
    
    // Delay fields
    QSpinBox* delaySpin_ = nullptr;
    
    // Variable fields
    QLineEdit* varNameEdit_ = nullptr;
    QLineEdit* varValueEdit_ = nullptr;
    
    QDialogButtonBox* buttonBox_ = nullptr;
};

// ============================================================================
// Macro Recorder Overlay
// ============================================================================
class MacroRecorderOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MacroRecorderOverlay(QWidget* parent = nullptr);
    
    void showOverlay();
    void hideOverlay();
    void updateStatus(const QString& status, int actionCount);

public slots:
    void onStopRecording();
    void onPauseRecording();

signals:
    void stopRequested();
    void pauseRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void setupUi();
    
    QLabel* statusLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
};

// ============================================================================
// Macro Playback Dialog
// ============================================================================
class MacroPlaybackDialog : public QDialog {
    Q_OBJECT

public:
    explicit MacroPlaybackDialog(const Macro& macro, backend::SessionClient* client,
                                 QWidget* parent = nullptr);

public slots:
    void onStart();
    void onPause();
    void onStop();
    void onStep();
    void onSpeedChanged(int speed);
    void onRepeatChanged(int count);

private:
    void setupUi();
    void executeNextAction();
    
    Macro macro_;
    backend::SessionClient* client_;
    int currentActionIndex_ = 0;
    bool isPaused_ = false;
    bool isRunning_ = false;
    
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QPlainTextEdit* logEdit_ = nullptr;
    QSpinBox* speedSpin_ = nullptr;
    QSpinBox* repeatSpin_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
};

// ============================================================================
// Macro Scheduler Dialog
// ============================================================================
class MacroSchedulerDialog : public QDialog {
    Q_OBJECT

public:
    explicit MacroSchedulerDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onAddSchedule();
    void onEditSchedule();
    void onDeleteSchedule();
    void onEnableSchedule();
    void onDisableSchedule();
    void onTestSchedule();

private:
    void setupUi();
    void loadSchedules();
    
    backend::SessionClient* client_;
    
    QTableView* scheduleTable_ = nullptr;
    QStandardItemModel* scheduleModel_ = nullptr;
};

} // namespace scratchrobin::ui
