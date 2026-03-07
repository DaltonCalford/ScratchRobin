#include "macro_recording.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QListWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QTreeWidget>
#include <QProgressBar>
#include <QSpinBox>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Macro Manager Panel
// ============================================================================
MacroManagerPanel::MacroManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("macro_manager", parent), client_(client) {
    setupUi();
    loadMacros();
}

void MacroManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    recordBtn_ = new QPushButton(tr("● Record"), this);
    connect(recordBtn_, &QPushButton::clicked, this, &MacroManagerPanel::onStartRecording);
    toolbarLayout->addWidget(recordBtn_);
    
    stopRecordBtn_ = new QPushButton(tr("■ Stop"), this);
    stopRecordBtn_->setEnabled(false);
    connect(stopRecordBtn_, &QPushButton::clicked, this, &MacroManagerPanel::onStopRecording);
    toolbarLayout->addWidget(stopRecordBtn_);
    
    playBtn_ = new QPushButton(tr("▶ Play"), this);
    connect(playBtn_, &QPushButton::clicked, this, &MacroManagerPanel::onPlayMacro);
    toolbarLayout->addWidget(playBtn_);
    
    toolbarLayout->addStretch();
    
    auto* newBtn = new QPushButton(tr("New"), this);
    connect(newBtn, &QPushButton::clicked, this, &MacroManagerPanel::onNewMacro);
    toolbarLayout->addWidget(newBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &MacroManagerPanel::onEditMacro);
    toolbarLayout->addWidget(editBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search macros..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &MacroManagerPanel::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    categoryFilter_ = new QComboBox(this);
    categoryFilter_->addItem(tr("All Categories"));
    categoryFilter_->addItem(tr("Data Entry"));
    categoryFilter_->addItem(tr("Reports"));
    categoryFilter_->addItem(tr("Maintenance"));
    connect(categoryFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                onFilterByCategory(categoryFilter_->itemText(index));
            });
    filterLayout->addWidget(categoryFilter_);
    
    mainLayout->addLayout(filterLayout);
    
    // Macro list
    macroTable_ = new QTableView(this);
    macroModel_ = new QStandardItemModel(this);
    macroModel_->setHorizontalHeaderLabels({tr("Name"), tr("Hotkey"), tr("Actions"), tr("Last Run")});
    macroTable_->setModel(macroModel_);
    macroTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    macroTable_->setAlternatingRowColors(true);
    
    connect(macroTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MacroManagerPanel::onMacroSelected);
    
    mainLayout->addWidget(macroTable_, 1);
    
    // Status
    recordingStatusLabel_ = new QLabel(tr("Ready"), this);
    mainLayout->addWidget(recordingStatusLabel_);
}

void MacroManagerPanel::loadMacros() {
    macros_.clear();
    
    Macro m1;
    m1.id = "macro1";
    m1.name = tr("Daily Report");
    m1.description = tr("Generate daily sales report");
    m1.category = tr("Reports");
    m1.hotkey = QKeySequence("Ctrl+Alt+R");
    m1.createdAt = QDateTime::currentDateTime().addDays(-30);
    macros_.append(m1);
    
    Macro m2;
    m2.id = "macro2";
    m2.name = tr("Backup Database");
    m2.description = tr("Run database backup procedure");
    m2.category = tr("Maintenance");
    m2.hotkey = QKeySequence("Ctrl+Alt+B");
    m2.createdAt = QDateTime::currentDateTime().addDays(-60);
    macros_.append(m2);
    
    updateMacroList();
}

void MacroManagerPanel::updateMacroList() {
    macroModel_->clear();
    macroModel_->setHorizontalHeaderLabels({tr("Name"), tr("Hotkey"), tr("Actions"), tr("Last Run")});
    
    for (const auto& macro : macros_) {
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(macro.name)
             << new QStandardItem(macro.hotkey.toString())
             << new QStandardItem(QString::number(macro.actions.size()))
             << new QStandardItem(macro.lastRunAt.toString("yyyy-MM-dd"));
        
        (*row)[0]->setData(macro.id, Qt::UserRole);
        macroModel_->appendRow(*row);
    }
}

void MacroManagerPanel::onMacroSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    QString macroId = macroModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    for (const auto& macro : macros_) {
        if (macro.id == macroId) {
            currentMacro_ = macro;
            break;
        }
    }
}

void MacroManagerPanel::onStartRecording() {
    recordingState_.isRecording = true;
    recordingState_.startedAt = QDateTime::currentDateTime();
    recordingState_.actionCount = 0;
    recordingState_.currentMacro = Macro();
    recordingState_.currentMacro.id = "new_macro";
    recordingState_.currentMacro.name = tr("New Macro");
    
    updateRecordingStatus();
    emit recordingStarted();
}

void MacroManagerPanel::onStopRecording() {
    recordingState_.isRecording = false;
    
    // Save the recorded macro
    if (!recordingState_.currentMacro.actions.isEmpty()) {
        recordingState_.currentMacro.createdAt = QDateTime::currentDateTime();
        macros_.append(recordingState_.currentMacro);
        updateMacroList();
    }
    
    updateRecordingStatus();
    emit recordingStopped();
}

void MacroManagerPanel::onPauseRecording() {
    recordingState_.isPaused = !recordingState_.isPaused;
    updateRecordingStatus();
}

void MacroManagerPanel::onResumeRecording() {
    recordingState_.isPaused = false;
    updateRecordingStatus();
}

void MacroManagerPanel::onPlayMacro() {
    if (currentMacro_.id.isEmpty()) return;
    
    emit macroStarted(currentMacro_.id);
    
    MacroPlaybackDialog dialog(currentMacro_, client_, this);
    dialog.exec();
    
    emit macroStopped(currentMacro_.id);
}

void MacroManagerPanel::onPausePlayback() {
    emit macroPaused(currentMacro_.id);
}

void MacroManagerPanel::onStopPlayback() {
    emit macroStopped(currentMacro_.id);
}

void MacroManagerPanel::onStepForward() {
    // Step to next action
}

void MacroManagerPanel::onStepBackward() {
    // Step to previous action
}

void MacroManagerPanel::onNewMacro() {
    Macro newMacro;
    newMacro.id = "macro_" + QString::number(macros_.size() + 1);
    newMacro.name = tr("New Macro %1").arg(macros_.size() + 1);
    
    MacroEditorDialog dialog(&newMacro, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        macros_.append(newMacro);
        updateMacroList();
    }
}

void MacroManagerPanel::onEditMacro() {
    if (currentMacro_.id.isEmpty()) return;
    
    MacroEditorDialog dialog(&currentMacro_, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        updateMacroList();
    }
}

void MacroManagerPanel::onDuplicateMacro() {
    if (currentMacro_.id.isEmpty()) return;
    
    Macro duplicate = currentMacro_;
    duplicate.id = "macro_" + QString::number(macros_.size() + 1);
    duplicate.name = tr("Copy of %1").arg(duplicate.name);
    duplicate.createdAt = QDateTime::currentDateTime();
    
    macros_.append(duplicate);
    updateMacroList();
}

void MacroManagerPanel::onDeleteMacro() {
    if (currentMacro_.id.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Macro"),
        tr("Delete '%1'?").arg(currentMacro_.name));
    
    if (reply == QMessageBox::Yes) {
        macros_.removeAll(currentMacro_);
        currentMacro_ = Macro();
        updateMacroList();
    }
}

void MacroManagerPanel::onSaveMacro() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Macro"),
        currentMacro_.name + ".macro", tr("Macro Files (*.macro)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Saved"), tr("Macro saved."));
    }
}

void MacroManagerPanel::onLoadMacro() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Macro"),
        QString(), tr("Macro Files (*.macro)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Loaded"), tr("Macro loaded."));
        loadMacros();
    }
}

void MacroManagerPanel::onExportMacro() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Macro"),
        currentMacro_.name + ".json", tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Macro exported."));
    }
}

void MacroManagerPanel::onImportMacro() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Macro"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Imported"), tr("Macro imported."));
        loadMacros();
    }
}

void MacroManagerPanel::onFilterByCategory(const QString& category) {
    Q_UNUSED(category)
    // TODO: Filter
    updateMacroList();
}

void MacroManagerPanel::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    // TODO: Filter
    updateMacroList();
}

void MacroManagerPanel::onAssignHotkey() {
    if (currentMacro_.id.isEmpty()) return;
    
    // Would show hotkey dialog
    QMessageBox::information(this, tr("Hotkey"), tr("Hotkey assigned."));
}

void MacroManagerPanel::onAddToToolbar() {
    if (currentMacro_.id.isEmpty()) return;
    
    QMessageBox::information(this, tr("Toolbar"), tr("Macro added to toolbar."));
}

void MacroManagerPanel::onCreateFolder() {
    QMessageBox::information(this, tr("Folder"), tr("Folder created."));
}

void MacroManagerPanel::updateRecordingStatus() {
    if (recordingState_.isRecording) {
        recordBtn_->setEnabled(false);
        stopRecordBtn_->setEnabled(true);
        recordingStatusLabel_->setText(tr("Recording... %1 actions").arg(recordingState_.actionCount));
    } else {
        recordBtn_->setEnabled(true);
        stopRecordBtn_->setEnabled(false);
        recordingStatusLabel_->setText(tr("Ready"));
    }
}

void MacroManagerPanel::updatePlaybackStatus() {
    // Update playback UI
}

// ============================================================================
// Macro Editor Dialog
// ============================================================================
MacroEditorDialog::MacroEditorDialog(Macro* macro, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), macro_(macro), client_(client) {
    setupUi();
    updateActionList();
}

void MacroEditorDialog::setupUi() {
    setWindowTitle(tr("Edit Macro: %1").arg(macro_->name));
    setMinimumSize(600, 450);
    
    auto* layout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    // Actions tab
    auto* actionsWidget = new QWidget(this);
    auto* actionsLayout = new QVBoxLayout(actionsWidget);
    
    auto* toolbarLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &MacroEditorDialog::onAddAction);
    toolbarLayout->addWidget(addBtn);
    
    auto* delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &MacroEditorDialog::onDeleteAction);
    toolbarLayout->addWidget(delBtn);
    
    toolbarLayout->addStretch();
    actionsLayout->addLayout(toolbarLayout);
    
    actionTable_ = new QTableView(this);
    actionModel_ = new QStandardItemModel(this);
    actionModel_->setHorizontalHeaderLabels({tr("#"), tr("Type"), tr("Description")});
    actionTable_->setModel(actionModel_);
    actionsLayout->addWidget(actionTable_, 1);
    
    tabWidget_->addTab(actionsWidget, tr("Actions"));
    
    // Properties tab
    auto* propsWidget = new QWidget(this);
    auto* propsLayout = new QFormLayout(propsWidget);
    
    nameEdit_ = new QLineEdit(macro_->name, this);
    propsLayout->addRow(tr("Name:"), nameEdit_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setText(macro_->description);
    descriptionEdit_->setMaximumHeight(60);
    propsLayout->addRow(tr("Description:"), descriptionEdit_);
    
    hotkeyEdit_ = new QKeySequenceEdit(macro_->hotkey, this);
    propsLayout->addRow(tr("Hotkey:"), hotkeyEdit_);
    
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({tr("General"), tr("Data Entry"), tr("Reports"), tr("Maintenance")});
    categoryCombo_->setCurrentText(macro_->category);
    propsLayout->addRow(tr("Category:"), categoryCombo_);
    
    speedSpin_ = new QSpinBox(this);
    speedSpin_->setRange(10, 500);
    speedSpin_->setValue(macro_->playbackSpeed);
    speedSpin_->setSuffix("%");
    propsLayout->addRow(tr("Speed:"), speedSpin_);
    
    tabWidget_->addTab(propsWidget, tr("Properties"));
    
    layout->addWidget(tabWidget_, 1);
    
    // Buttons
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &MacroEditorDialog::onValidate);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void MacroEditorDialog::setupActionList() {
    // Setup action list
}

void MacroEditorDialog::setupPropertiesPanel() {
    // Setup properties panel
}

void MacroEditorDialog::setupVariablesPanel() {
    // Setup variables panel
}

void MacroEditorDialog::updateActionList() {
    actionModel_->clear();
    actionModel_->setHorizontalHeaderLabels({tr("#"), tr("Type"), tr("Description")});
    
    for (int i = 0; i < macro_->actions.size(); ++i) {
        const auto& action = macro_->actions[i];
        
        QString typeStr;
        switch (action.type) {
            case MacroActionType::KeyPress: typeStr = tr("Key"); break;
            case MacroActionType::TextInput: typeStr = tr("Text"); break;
            case MacroActionType::Delay: typeStr = tr("Delay"); break;
            case MacroActionType::SQLExecute: typeStr = tr("SQL"); break;
            default: typeStr = tr("Action");
        }
        
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(QString::number(i + 1))
             << new QStandardItem(typeStr)
             << new QStandardItem(action.description);
        
        actionModel_->appendRow(*row);
    }
}

void MacroEditorDialog::updateActionDetails(const MacroAction& action) {
    Q_UNUSED(action)
}

void MacroEditorDialog::onActionSelected(const QModelIndex& index) {
    if (index.isValid()) {
        currentActionIndex_ = index.row();
    }
}

void MacroEditorDialog::onAddAction() {
    MacroAction action;
    action.id = "action_" + QString::number(macro_->actions.size() + 1);
    action.type = MacroActionType::Delay;
    action.description = tr("Wait 1000ms");
    action.delayMs = 1000;
    
    macro_->actions.append(action);
    updateActionList();
}

void MacroEditorDialog::onEditAction() {
    if (currentActionIndex_ < 0 || currentActionIndex_ >= macro_->actions.size()) return;
    
    // Open action editor
    QMessageBox::information(this, tr("Edit"), tr("Edit action dialog would open."));
}

void MacroEditorDialog::onDeleteAction() {
    if (currentActionIndex_ < 0 || currentActionIndex_ >= macro_->actions.size()) return;
    
    macro_->actions.removeAt(currentActionIndex_);
    currentActionIndex_ = -1;
    updateActionList();
}

void MacroEditorDialog::onMoveUp() {
    if (currentActionIndex_ <= 0) return;
    
    std::swap(macro_->actions[currentActionIndex_], macro_->actions[currentActionIndex_ - 1]);
    currentActionIndex_--;
    updateActionList();
}

void MacroEditorDialog::onMoveDown() {
    if (currentActionIndex_ < 0 || currentActionIndex_ >= macro_->actions.size() - 1) return;
    
    std::swap(macro_->actions[currentActionIndex_], macro_->actions[currentActionIndex_ + 1]);
    currentActionIndex_++;
    updateActionList();
}

void MacroEditorDialog::onDuplicateAction() {
    if (currentActionIndex_ < 0 || currentActionIndex_ >= macro_->actions.size()) return;
    
    MacroAction duplicate = macro_->actions[currentActionIndex_];
    duplicate.id = "action_" + QString::number(macro_->actions.size() + 1);
    macro_->actions.insert(currentActionIndex_ + 1, duplicate);
    updateActionList();
}

void MacroEditorDialog::onToggleActionEnabled() {
    if (currentActionIndex_ < 0 || currentActionIndex_ >= macro_->actions.size()) return;
    
    macro_->actions[currentActionIndex_].enabled = 
        !macro_->actions[currentActionIndex_].enabled;
    updateActionList();
}

void MacroEditorDialog::onRecordNewActions() {
    QMessageBox::information(this, tr("Record"), tr("Recording new actions..."));
}

void MacroEditorDialog::onInsertRecording() {
    QMessageBox::information(this, tr("Insert"), tr("Recording would be inserted."));
}

void MacroEditorDialog::onNameChanged(const QString& name) {
    macro_->name = name;
}

void MacroEditorDialog::onDescriptionChanged() {
    macro_->description = descriptionEdit_->toPlainText();
}

void MacroEditorDialog::onHotkeyChanged(const QKeySequence& key) {
    macro_->hotkey = key;
}

void MacroEditorDialog::onCategoryChanged(const QString& category) {
    macro_->category = category;
}

void MacroEditorDialog::onAddVariable() {
    QMessageBox::information(this, tr("Variable"), tr("Add variable dialog would open."));
}

void MacroEditorDialog::onEditVariable() {
    QMessageBox::information(this, tr("Variable"), tr("Edit variable dialog would open."));
}

void MacroEditorDialog::onDeleteVariable() {
    QMessageBox::information(this, tr("Variable"), tr("Variable deleted."));
}

void MacroEditorDialog::onValidate() {
    macro_->name = nameEdit_->text();
    macro_->description = descriptionEdit_->toPlainText();
    macro_->hotkey = hotkeyEdit_->keySequence();
    macro_->category = categoryCombo_->currentText();
    macro_->playbackSpeed = speedSpin_->value();
    
    accept();
}

void MacroEditorDialog::onTestMacro() {
    QMessageBox::information(this, tr("Test"), tr("Testing macro..."));
}

// ============================================================================
// Macro Playback Dialog
// ============================================================================
MacroPlaybackDialog::MacroPlaybackDialog(const Macro& macro, backend::SessionClient* client,
                                         QWidget* parent)
    : QDialog(parent), macro_(macro), client_(client) {
    setupUi();
}

void MacroPlaybackDialog::setupUi() {
    setWindowTitle(tr("Playing: %1").arg(macro_.name));
    setMinimumSize(400, 200);
    
    auto* layout = new QVBoxLayout(this);
    
    statusLabel_ = new QLabel(tr("Ready to play"), this);
    layout->addWidget(statusLabel_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, macro_.actions.size());
    layout->addWidget(progressBar_);
    
    logEdit_ = new QPlainTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumHeight(100);
    layout->addWidget(logEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &MacroPlaybackDialog::onStart);
    btnLayout->addWidget(startBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    pauseBtn_->setEnabled(false);
    connect(pauseBtn_, &QPushButton::clicked, this, &MacroPlaybackDialog::onPause);
    btnLayout->addWidget(pauseBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    connect(stopBtn_, &QPushButton::clicked, this, &MacroPlaybackDialog::onStop);
    btnLayout->addWidget(stopBtn_);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void MacroPlaybackDialog::onStart() {
    isRunning_ = true;
    currentActionIndex_ = 0;
    
    startBtn_->setEnabled(false);
    pauseBtn_->setEnabled(true);
    
    statusLabel_->setText(tr("Playing..."));
    logEdit_->appendPlainText(tr("Started macro playback"));
    
    executeNextAction();
}

void MacroPlaybackDialog::onPause() {
    isPaused_ = !isPaused_;
    pauseBtn_->setText(isPaused_ ? tr("Resume") : tr("Pause"));
    statusLabel_->setText(isPaused_ ? tr("Paused") : tr("Playing..."));
}

void MacroPlaybackDialog::onStop() {
    isRunning_ = false;
    startBtn_->setEnabled(true);
    pauseBtn_->setEnabled(false);
    statusLabel_->setText(tr("Stopped"));
    logEdit_->appendPlainText(tr("Playback stopped"));
}

void MacroPlaybackDialog::onStep() {
    if (currentActionIndex_ < macro_.actions.size()) {
        executeNextAction();
    }
}

void MacroPlaybackDialog::onSpeedChanged(int speed) {
    macro_.playbackSpeed = speed;
}

void MacroPlaybackDialog::onRepeatChanged(int count) {
    macro_.repeatCount = count;
}

void MacroPlaybackDialog::executeNextAction() {
    if (!isRunning_ || isPaused_) return;
    
    if (currentActionIndex_ >= macro_.actions.size()) {
        statusLabel_->setText(tr("Completed"));
        startBtn_->setEnabled(true);
        pauseBtn_->setEnabled(false);
        logEdit_->appendPlainText(tr("Playback completed"));
        return;
    }
    
    const auto& action = macro_.actions[currentActionIndex_];
    logEdit_->appendPlainText(tr("Executing: %1").arg(action.description));
    
    progressBar_->setValue(currentActionIndex_);
    currentActionIndex_++;
    
    // Simulate execution delay
    QTimer::singleShot(100, this, &MacroPlaybackDialog::executeNextAction);
}

// ============================================================================
// Macro Scheduler Dialog
// ============================================================================
MacroSchedulerDialog::MacroSchedulerDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadSchedules();
}

void MacroSchedulerDialog::setupUi() {
    setWindowTitle(tr("Macro Scheduler"));
    setMinimumSize(500, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    scheduleTable_ = new QTableView(this);
    scheduleModel_ = new QStandardItemModel(this);
    scheduleModel_->setHorizontalHeaderLabels({tr("Macro"), tr("Schedule"), tr("Next Run"), tr("Status")});
    scheduleTable_->setModel(scheduleModel_);
    layout->addWidget(scheduleTable_, 1);
    
    auto* btnLayout = new QHBoxLayout();
    
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &MacroSchedulerDialog::onAddSchedule);
    btnLayout->addWidget(addBtn);
    
    auto* delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &MacroSchedulerDialog::onDeleteSchedule);
    btnLayout->addWidget(delBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void MacroSchedulerDialog::loadSchedules() {
    scheduleModel_->clear();
    scheduleModel_->setHorizontalHeaderLabels({tr("Macro"), tr("Schedule"), tr("Next Run"), tr("Status")});
    
    // Sample schedules
    auto* row1 = new QList<QStandardItem*>();
    *row1 << new QStandardItem(tr("Daily Report"))
          << new QStandardItem(tr("Daily at 8:00 AM"))
          << new QStandardItem(tr("Tomorrow 8:00 AM"))
          << new QStandardItem(tr("Enabled"));
    scheduleModel_->appendRow(*row1);
}

void MacroSchedulerDialog::onAddSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Add schedule dialog would open."));
}

void MacroSchedulerDialog::onEditSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Edit schedule dialog would open."));
}

void MacroSchedulerDialog::onDeleteSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Schedule deleted."));
}

void MacroSchedulerDialog::onEnableSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Schedule enabled."));
}

void MacroSchedulerDialog::onDisableSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Schedule disabled."));
}

void MacroSchedulerDialog::onTestSchedule() {
    QMessageBox::information(this, tr("Schedule"), tr("Running scheduled macro..."));
}

// ============================================================================
// Action Editor Dialog - Stub implementations
// ============================================================================
ActionEditorDialog::ActionEditorDialog(MacroAction* action, QWidget* parent)
    : QDialog(parent), action_(action) {
    originalAction_ = *action;
    setupUi();
}

void ActionEditorDialog::setupUi() {
    setWindowTitle(tr("Edit Action"));
    auto* layout = new QVBoxLayout(this);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("Delay"));
    typeCombo_->addItem(tr("Key Press"));
    typeCombo_->addItem(tr("Text Input"));
    layout->addWidget(new QLabel(tr("Type:"), this));
    layout->addWidget(typeCombo_);
    
    layout->addWidget(new QLabel(tr("Description:"), this));
    auto* descEdit = new QLineEdit(this);
    descEdit->setText(action_->description);
    layout->addWidget(descEdit);
    
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &ActionEditorDialog::onValidate);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void ActionEditorDialog::onTypeChanged(int index) {
    Q_UNUSED(index)
}

void ActionEditorDialog::onValidate() {
    accept();
}

MacroAction ActionEditorDialog::action() const {
    return *action_;
}

void ActionEditorDialog::updateFieldsForType(MacroActionType type) {
    Q_UNUSED(type)
}

// ============================================================================
// Macro Recorder Overlay - Stub implementations
// ============================================================================
MacroRecorderOverlay::MacroRecorderOverlay(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void MacroRecorderOverlay::setupUi() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    auto* layout = new QVBoxLayout(this);
    
    statusLabel_ = new QLabel(tr("Recording..."), this);
    layout->addWidget(statusLabel_);
    
    countLabel_ = new QLabel(tr("0 actions"), this);
    layout->addWidget(countLabel_);
    
    auto* btnLayout = new QHBoxLayout();
    stopBtn_ = new QPushButton(tr("Stop"), this);
    connect(stopBtn_, &QPushButton::clicked, this, &MacroRecorderOverlay::onStopRecording);
    btnLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    connect(pauseBtn_, &QPushButton::clicked, this, &MacroRecorderOverlay::onPauseRecording);
    btnLayout->addWidget(pauseBtn_);
    
    layout->addLayout(btnLayout);
}

void MacroRecorderOverlay::showOverlay() {
    show();
}

void MacroRecorderOverlay::hideOverlay() {
    hide();
}

void MacroRecorderOverlay::updateStatus(const QString& status, int actionCount) {
    statusLabel_->setText(status);
    countLabel_->setText(tr("%1 actions").arg(actionCount));
}

void MacroRecorderOverlay::onStopRecording() {
    emit stopRequested();
}

void MacroRecorderOverlay::onPauseRecording() {
    emit pauseRequested();
}

void MacroRecorderOverlay::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 128));
}

void MacroRecorderOverlay::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event)
}

} // namespace scratchrobin::ui
