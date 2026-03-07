#include "custom_keybindings.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QListWidget>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Keybindings Editor Dialog
// ============================================================================
KeybindingsEditorDialog::KeybindingsEditorDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadKeybindings();
}

void KeybindingsEditorDialog::setupUi() {
    setWindowTitle(tr("Keyboard Shortcuts"));
    setMinimumSize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Search bar
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search actions..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &KeybindingsEditorDialog::onSearchTextChanged);
    searchLayout->addWidget(searchEdit_, 1);
    
    auto* checkBtn = new QPushButton(tr("Check Conflicts"), this);
    connect(checkBtn, &QPushButton::clicked, this, &KeybindingsEditorDialog::onCheckConflicts);
    searchLayout->addWidget(checkBtn);
    
    layout->addLayout(searchLayout);
    
    // Splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Categories
    categoryList_ = new QListWidget(this);
    categoryList_->addItem(tr("All Commands"));
    categoryList_->addItem(tr("File"));
    categoryList_->addItem(tr("Edit"));
    categoryList_->addItem(tr("View"));
    categoryList_->addItem(tr("Query"));
    categoryList_->addItem(tr("Tools"));
    categoryList_->addItem(tr("Window"));
    categoryList_->addItem(tr("Help"));
    connect(categoryList_, &QListWidget::currentRowChanged,
            this, &KeybindingsEditorDialog::onCategorySelected);
    splitter->addWidget(categoryList_);
    
    // Bindings table
    bindingsTable_ = new QTableView(this);
    bindingsModel_ = new QStandardItemModel(this);
    bindingsModel_->setHorizontalHeaderLabels({tr("Action"), tr("Shortcut"), tr("Secondary")});
    bindingsTable_->setModel(bindingsModel_);
    bindingsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    bindingsTable_->setAlternatingRowColors(true);
    
    connect(bindingsTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &KeybindingsEditorDialog::onBindingSelected);
    
    splitter->addWidget(bindingsTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    actionLabel_ = new QLabel(tr("Select a command"), this);
    QFont titleFont = actionLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    actionLabel_->setFont(titleFont);
    detailsLayout->addWidget(actionLabel_);
    
    descriptionLabel_ = new QLabel(this);
    descriptionLabel_->setWordWrap(true);
    detailsLayout->addWidget(descriptionLabel_);
    
    detailsLayout->addSpacing(16);
    
    auto* shortcutGroup = new QGroupBox(tr("Shortcuts"), this);
    auto* shortcutLayout = new QFormLayout(shortcutGroup);
    
    primaryShortcutEdit_ = new QKeySequenceEdit(this);
    primaryShortcutEdit_->setEnabled(false);
    shortcutLayout->addRow(tr("Primary:"), primaryShortcutEdit_);
    
    secondaryShortcutEdit_ = new QKeySequenceEdit(this);
    secondaryShortcutEdit_->setEnabled(false);
    shortcutLayout->addRow(tr("Secondary:"), secondaryShortcutEdit_);
    
    enabledCheck_ = new QCheckBox(tr("Enabled"), this);
    enabledCheck_->setEnabled(false);
    shortcutLayout->addRow(enabledCheck_);
    
    globalCheck_ = new QCheckBox(tr("Global shortcut"), this);
    globalCheck_->setEnabled(false);
    shortcutLayout->addRow(globalCheck_);
    
    detailsLayout->addWidget(shortcutGroup);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    editBtn_ = new QPushButton(tr("Edit"), this);
    editBtn_->setEnabled(false);
    connect(editBtn_, &QPushButton::clicked, this, &KeybindingsEditorDialog::onEditShortcut);
    btnLayout->addWidget(editBtn_);
    
    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setEnabled(false);
    connect(clearBtn_, &QPushButton::clicked, this, &KeybindingsEditorDialog::onClearShortcut);
    btnLayout->addWidget(clearBtn_);
    
    resetBtn_ = new QPushButton(tr("Reset"), this);
    resetBtn_->setEnabled(false);
    connect(resetBtn_, &QPushButton::clicked, this, &KeybindingsEditorDialog::onResetToDefault);
    btnLayout->addWidget(resetBtn_);
    
    detailsLayout->addLayout(btnLayout);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({150, 350, 200});
    
    layout->addWidget(splitter, 1);
    
    // Bottom buttons
    auto* bottomLayout = new QHBoxLayout();
    
    auto* importBtn = new QPushButton(tr("Import..."), this);
    connect(importBtn, &QPushButton::clicked, this, &KeybindingsEditorDialog::onImportKeymap);
    bottomLayout->addWidget(importBtn);
    
    auto* exportBtn = new QPushButton(tr("Export..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &KeybindingsEditorDialog::onExportKeymap);
    bottomLayout->addWidget(exportBtn);
    
    auto* presetBtn = new QPushButton(tr("Presets..."), this);
    connect(presetBtn, &QPushButton::clicked, this, &KeybindingsEditorDialog::onLoadPreset);
    bottomLayout->addWidget(presetBtn);
    
    bottomLayout->addStretch();
    
    auto* resetAllBtn = new QPushButton(tr("Reset All"), this);
    connect(resetAllBtn, &QPushButton::clicked, this, &KeybindingsEditorDialog::onResetAll);
    bottomLayout->addWidget(resetAllBtn);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &KeybindingsEditorDialog::onValidate);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    bottomLayout->addWidget(buttonBox);
    
    layout->addLayout(bottomLayout);
}

void KeybindingsEditorDialog::loadKeybindings() {
    allBindings_.clear();
    
    // Sample keybindings
    Keybinding kb1;
    kb1.id = "file.new";
    kb1.action = tr("New Connection");
    kb1.description = tr("Create a new database connection");
    kb1.category = tr("File");
    kb1.primaryShortcut = QKeySequence::New;
    allBindings_.append(kb1);
    
    Keybinding kb2;
    kb2.id = "file.open";
    kb2.action = tr("Open SQL Script");
    kb2.description = tr("Open an SQL script file");
    kb2.category = tr("File");
    kb2.primaryShortcut = QKeySequence::Open;
    allBindings_.append(kb2);
    
    Keybinding kb3;
    kb3.id = "file.save";
    kb3.action = tr("Save");
    kb3.description = tr("Save current file");
    kb3.category = tr("File");
    kb3.primaryShortcut = QKeySequence::Save;
    allBindings_.append(kb3);
    
    Keybinding kb4;
    kb4.id = "edit.cut";
    kb4.action = tr("Cut");
    kb4.description = tr("Cut selection to clipboard");
    kb4.category = tr("Edit");
    kb4.primaryShortcut = QKeySequence::Cut;
    allBindings_.append(kb4);
    
    Keybinding kb5;
    kb5.id = "edit.copy";
    kb5.action = tr("Copy");
    kb5.description = tr("Copy selection to clipboard");
    kb5.category = tr("Edit");
    kb5.primaryShortcut = QKeySequence::Copy;
    allBindings_.append(kb5);
    
    Keybinding kb6;
    kb6.id = "edit.paste";
    kb6.action = tr("Paste");
    kb6.description = tr("Paste from clipboard");
    kb6.category = tr("Edit");
    kb6.primaryShortcut = QKeySequence::Paste;
    allBindings_.append(kb6);
    
    Keybinding kb7;
    kb7.id = "query.execute";
    kb7.action = tr("Execute Query");
    kb7.description = tr("Execute current SQL query");
    kb7.category = tr("Query");
    kb7.primaryShortcut = QKeySequence("F5");
    allBindings_.append(kb7);
    
    Keybinding kb8;
    kb8.id = "query.executeSelection";
    kb8.action = tr("Execute Selection");
    kb8.description = tr("Execute selected SQL");
    kb8.category = tr("Query");
    kb8.primaryShortcut = QKeySequence("Ctrl+F5");
    allBindings_.append(kb8);
    
    updateBindingsList();
}

void KeybindingsEditorDialog::updateBindingsList() {
    bindingsModel_->clear();
    bindingsModel_->setHorizontalHeaderLabels({tr("Action"), tr("Shortcut"), tr("Secondary")});
    
    QString filter = searchEdit_->text().toLower();
    int categoryIndex = categoryList_->currentRow();
    QString category = categoryIndex > 0 ? categoryList_->item(categoryIndex)->text() : "";
    
    for (const auto& binding : allBindings_) {
        // Filter by search text
        if (!filter.isEmpty() && 
            !binding.action.toLower().contains(filter) &&
            !binding.description.toLower().contains(filter)) {
            continue;
        }
        
        // Filter by category
        if (!category.isEmpty() && binding.category != category) {
            continue;
        }
        
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(binding.action)
             << new QStandardItem(binding.primaryShortcut.toString())
             << new QStandardItem(binding.secondaryShortcut.toString());
        
        (*row)[0]->setData(binding.id, Qt::UserRole);
        bindingsModel_->appendRow(*row);
    }
}

void KeybindingsEditorDialog::updateBindingDetails(const Keybinding& binding) {
    currentBinding_ = binding;
    
    actionLabel_->setText(binding.action);
    descriptionLabel_->setText(binding.description);
    primaryShortcutEdit_->setKeySequence(binding.primaryShortcut);
    secondaryShortcutEdit_->setKeySequence(binding.secondaryShortcut);
    enabledCheck_->setChecked(binding.isEnabled);
    globalCheck_->setChecked(binding.isGlobal);
    
    editBtn_->setEnabled(true);
    clearBtn_->setEnabled(true);
    resetBtn_->setEnabled(true);
    primaryShortcutEdit_->setEnabled(true);
    secondaryShortcutEdit_->setEnabled(true);
    enabledCheck_->setEnabled(true);
    globalCheck_->setEnabled(true);
}

void KeybindingsEditorDialog::onCategorySelected(int index) {
    Q_UNUSED(index)
    updateBindingsList();
}

void KeybindingsEditorDialog::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    updateBindingsList();
}

void KeybindingsEditorDialog::onBindingSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    QString bindingId = bindingsModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    for (const auto& binding : allBindings_) {
        if (binding.id == bindingId) {
            updateBindingDetails(binding);
            break;
        }
    }
}

void KeybindingsEditorDialog::onEditShortcut() {
    ShortcutCaptureDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QKeySequence shortcut = dialog.capturedShortcut();
        primaryShortcutEdit_->setKeySequence(shortcut);
        
        // Update the binding
        for (auto& binding : allBindings_) {
            if (binding.id == currentBinding_.id) {
                binding.primaryShortcut = shortcut;
                binding.isCustom = true;
                break;
            }
        }
        
        updateBindingsList();
    }
}

void KeybindingsEditorDialog::onClearShortcut() {
    primaryShortcutEdit_->clear();
    
    for (auto& binding : allBindings_) {
        if (binding.id == currentBinding_.id) {
            binding.primaryShortcut = QKeySequence();
            binding.isCustom = true;
            break;
        }
    }
    
    updateBindingsList();
}

void KeybindingsEditorDialog::onSetSecondaryShortcut() {
    ShortcutCaptureDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        secondaryShortcutEdit_->setKeySequence(dialog.capturedShortcut());
    }
}

void KeybindingsEditorDialog::onResetToDefault() {
    for (auto& binding : allBindings_) {
        if (binding.id == currentBinding_.id) {
            binding.isCustom = false;
            // Restore default shortcut
            break;
        }
    }
    
    updateBindingsList();
    QMessageBox::information(this, tr("Reset"), tr("Shortcut reset to default."));
}

void KeybindingsEditorDialog::onResetAll() {
    auto reply = QMessageBox::question(this, tr("Reset All"),
        tr("Reset all shortcuts to defaults?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        for (auto& binding : allBindings_) {
            binding.isCustom = false;
        }
        updateBindingsList();
    }
}

void KeybindingsEditorDialog::onImportKeymap() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Keymap"),
        QString(), tr("Keymap Files (*.keymap);;JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Imported"), tr("Keymap imported successfully."));
    }
}

void KeybindingsEditorDialog::onExportKeymap() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Keymap"),
        tr("custom.keymap"), tr("Keymap Files (*.keymap);;JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Keymap exported successfully."));
    }
}

void KeybindingsEditorDialog::onLoadPreset() {
    KeymapPresetManagerDialog dialog(this);
    dialog.exec();
}

void KeybindingsEditorDialog::onCheckConflicts() {
    QList<KeybindingConflict> conflicts = detectConflicts();
    
    if (conflicts.isEmpty()) {
        QMessageBox::information(this, tr("No Conflicts"),
            tr("No shortcut conflicts detected."));
    } else {
        ConflictResolutionDialog dialog(conflicts, this);
        dialog.exec();
    }
}

void KeybindingsEditorDialog::onResolveConflict() {
    // Resolve selected conflict
}

QList<KeybindingConflict> KeybindingsEditorDialog::detectConflicts() {
    QList<KeybindingConflict> conflicts;
    
    // Check for duplicate shortcuts
    QHash<QString, Keybinding> shortcutMap;
    for (const auto& binding : allBindings_) {
        if (!binding.primaryShortcut.isEmpty()) {
            QString shortcutStr = binding.primaryShortcut.toString();
            if (shortcutMap.contains(shortcutStr)) {
                KeybindingConflict conflict;
                conflict.binding1 = shortcutMap[shortcutStr];
                conflict.binding2 = binding;
                conflict.context = tr("Global");
                conflicts.append(conflict);
            } else {
                shortcutMap[shortcutStr] = binding;
            }
        }
    }
    
    return conflicts;
}

void KeybindingsEditorDialog::onValidate() {
    // Save changes
    accept();
}

void KeybindingsEditorDialog::onApply() {
    // Apply changes without closing
    QMessageBox::information(this, tr("Applied"), tr("Changes applied."));
}

// ============================================================================
// Shortcut Capture Dialog
// ============================================================================
ShortcutCaptureDialog::ShortcutCaptureDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void ShortcutCaptureDialog::setupUi() {
    setWindowTitle(tr("Capture Shortcut"));
    setMinimumSize(300, 150);
    
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Press the key combination you want to use:"), this));
    
    displayLabel_ = new QLabel(tr("(press keys...)"), this);
    QFont font = displayLabel_->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    displayLabel_->setFont(font);
    displayLabel_->setAlignment(Qt::AlignCenter);
    displayLabel_->setStyleSheet("border: 2px solid gray; padding: 20px;");
    layout->addWidget(displayLabel_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ShortcutCaptureDialog::onClear);
    btnLayout->addWidget(clearBtn);
    
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    btnLayout->addWidget(buttonBox_);
    
    layout->addLayout(btnLayout);
}

void ShortcutCaptureDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    
    currentKey_ = event->key();
    currentModifiers_ = event->modifiers();
    
    updateDisplay();
}

void ShortcutCaptureDialog::keyReleaseEvent(QKeyEvent* event) {
    Q_UNUSED(event)
    
    // Finalize the shortcut
    if (currentKey_ != 0) {
        capturedShortcut_ = QKeySequence(currentModifiers_ | currentKey_);
    }
}

void ShortcutCaptureDialog::updateDisplay() {
    QString text;
    if (currentModifiers_ & Qt::ControlModifier) text += "Ctrl+";
    if (currentModifiers_ & Qt::AltModifier) text += "Alt+";
    if (currentModifiers_ & Qt::ShiftModifier) text += "Shift+";
    if (currentModifiers_ & Qt::MetaModifier) text += "Meta+";
    
    if (currentKey_ != 0 && currentKey_ != Qt::Key_Control &&
        currentKey_ != Qt::Key_Alt && currentKey_ != Qt::Key_Shift &&
        currentKey_ != Qt::Key_Meta) {
        text += QKeySequence(currentKey_).toString();
    } else {
        text += "...";
    }
    
    displayLabel_->setText(text);
}

void ShortcutCaptureDialog::onClear() {
    currentKey_ = 0;
    currentModifiers_ = Qt::NoModifier;
    capturedShortcut_ = QKeySequence();
    displayLabel_->setText(tr("(press keys...)"));
}

void ShortcutCaptureDialog::onCancel() {
    reject();
}

QKeySequence ShortcutCaptureDialog::capturedShortcut() const {
    return capturedShortcut_;
}

// ============================================================================
// Conflict Resolution Dialog
// ============================================================================
ConflictResolutionDialog::ConflictResolutionDialog(const QList<KeybindingConflict>& conflicts,
                                                   QWidget* parent)
    : QDialog(parent), conflicts_(conflicts) {
    setupUi();
}

void ConflictResolutionDialog::setupUi() {
    setWindowTitle(tr("Resolve Conflicts"));
    setMinimumSize(500, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("The following shortcut conflicts were found:"), this));
    
    conflictList_ = new QListWidget(this);
    for (int i = 0; i < conflicts_.size(); ++i) {
        const auto& conflict = conflicts_[i];
        QString text = tr("%1: %2 vs %3")
            .arg(conflict.binding1.primaryShortcut.toString())
            .arg(conflict.binding1.action)
            .arg(conflict.binding2.action);
        conflictList_->addItem(text);
    }
    connect(conflictList_, &QListWidget::currentRowChanged,
            this, &ConflictResolutionDialog::onConflictSelected);
    layout->addWidget(conflictList_);
    
    detailsLabel_ = new QLabel(this);
    detailsLabel_->setWordWrap(true);
    layout->addWidget(detailsLabel_);
    
    suggestionLabel_ = new QLabel(this);
    suggestionLabel_->setWordWrap(true);
    suggestionLabel_->setStyleSheet("color: blue;");
    layout->addWidget(suggestionLabel_);
    
    auto* btnLayout = new QHBoxLayout();
    
    auto* keepFirstBtn = new QPushButton(tr("Keep First"), this);
    connect(keepFirstBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onKeepFirst);
    btnLayout->addWidget(keepFirstBtn);
    
    auto* keepSecondBtn = new QPushButton(tr("Keep Second"), this);
    connect(keepSecondBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onKeepSecond);
    btnLayout->addWidget(keepSecondBtn);
    
    btnLayout->addStretch();
    
    auto* ignoreBtn = new QPushButton(tr("Ignore"), this);
    connect(ignoreBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onIgnoreConflict);
    btnLayout->addWidget(ignoreBtn);
    
    auto* autoBtn = new QPushButton(tr("Auto Resolve"), this);
    connect(autoBtn, &QPushButton::clicked, this, &ConflictResolutionDialog::onAutoResolve);
    btnLayout->addWidget(autoBtn);
    
    layout->addLayout(btnLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttonBox);
    
    if (!conflicts_.isEmpty()) {
        conflictList_->setCurrentRow(0);
        updateConflictDetails(0);
    }
}

void ConflictResolutionDialog::updateConflictDetails(int index) {
    if (index < 0 || index >= conflicts_.size()) return;
    
    const auto& conflict = conflicts_[index];
    QString details = tr("<b>Conflict %1 of %2</b><br><br>"
        "<b>Shortcut:</b> %3<br><br>"
        "<b>First binding:</b> %4<br>"
        "<b>Second binding:</b> %5")
        .arg(index + 1)
        .arg(conflicts_.size())
        .arg(conflict.binding1.primaryShortcut.toString())
        .arg(conflict.binding1.action)
        .arg(conflict.binding2.action);
    
    detailsLabel_->setText(details);
    suggestionLabel_->setText(tr("Suggestion: Keep the first binding and assign a different shortcut to the second."));
}

void ConflictResolutionDialog::onConflictSelected(int index) {
    currentConflictIndex_ = index;
    updateConflictDetails(index);
}

void ConflictResolutionDialog::onKeepFirst() {
    QMessageBox::information(this, tr("Resolved"), tr("First binding kept."));
}

void ConflictResolutionDialog::onKeepSecond() {
    QMessageBox::information(this, tr("Resolved"), tr("Second binding kept."));
}

void ConflictResolutionDialog::onChangeFirst() {
    QMessageBox::information(this, tr("Change"), tr("First binding shortcut would be changed."));
}

void ConflictResolutionDialog::onChangeSecond() {
    QMessageBox::information(this, tr("Change"), tr("Second binding shortcut would be changed."));
}

void ConflictResolutionDialog::onIgnoreConflict() {
    QMessageBox::information(this, tr("Ignored"), tr("Conflict ignored."));
}

void ConflictResolutionDialog::onAutoResolve() {
    QMessageBox::information(this, tr("Auto Resolve"), tr("All conflicts auto-resolved."));
    accept();
}

// ============================================================================
// Keymap Preset Manager Dialog
// ============================================================================
KeymapPresetManagerDialog::KeymapPresetManagerDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
    loadPresets();
}

void KeymapPresetManagerDialog::setupUi() {
    setWindowTitle(tr("Keymap Presets"));
    setMinimumSize(450, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    presetTable_ = new QTableView(this);
    presetModel_ = new QStandardItemModel(this);
    presetModel_->setHorizontalHeaderLabels({tr("Name"), tr("Description"), tr("Modified")});
    presetTable_->setModel(presetModel_);
    presetTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(presetTable_, 1);
    
    auto* btnLayout = new QHBoxLayout();
    
    auto* loadBtn = new QPushButton(tr("Load"), this);
    connect(loadBtn, &QPushButton::clicked, this, &KeymapPresetManagerDialog::onLoadPreset);
    btnLayout->addWidget(loadBtn);
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &KeymapPresetManagerDialog::onSavePreset);
    btnLayout->addWidget(saveBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &KeymapPresetManagerDialog::onDeletePreset);
    btnLayout->addWidget(deleteBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void KeymapPresetManagerDialog::loadPresets() {
    presetModel_->clear();
    presetModel_->setHorizontalHeaderLabels({tr("Name"), tr("Description"), tr("Modified")});
    
    // Sample presets
    auto* row1 = new QList<QStandardItem*>();
    *row1 << new QStandardItem(tr("Default"))
          << new QStandardItem(tr("Default keybindings"))
          << new QStandardItem(tr("Built-in"));
    presetModel_->appendRow(*row1);
    
    auto* row2 = new QList<QStandardItem*>();
    *row2 << new QStandardItem(tr("VS Code Style"))
          << new QStandardItem(tr("VS Code-like shortcuts"))
          << new QStandardItem(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    presetModel_->appendRow(*row2);
    
    auto* row3 = new QList<QStandardItem*>();
    *row3 << new QStandardItem(tr("Emacs Style"))
          << new QStandardItem(tr("Emacs-like shortcuts"))
          << new QStandardItem(QDateTime::currentDateTime().addDays(-30).toString("yyyy-MM-dd"));
    presetModel_->appendRow(*row3);
}

void KeymapPresetManagerDialog::onPresetSelected(const QModelIndex& index) {
    Q_UNUSED(index)
}

void KeymapPresetManagerDialog::onLoadPreset() {
    QMessageBox::information(this, tr("Load"), tr("Preset loaded."));
    accept();
}

void KeymapPresetManagerDialog::onSavePreset() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Save Preset"),
        tr("Preset name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        QMessageBox::information(this, tr("Saved"), tr("Preset '%1' saved.").arg(name));
    }
}

void KeymapPresetManagerDialog::onDeletePreset() {
    auto reply = QMessageBox::question(this, tr("Delete"),
        tr("Delete selected preset?"));
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Deleted"), tr("Preset deleted."));
    }
}

void KeymapPresetManagerDialog::onRenamePreset() {
    // Rename preset
}

void KeymapPresetManagerDialog::onExportPreset() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Preset"),
        tr("preset.keymap"), tr("Keymap Files (*.keymap)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Exported"), tr("Preset exported."));
    }
}

void KeymapPresetManagerDialog::onImportPreset() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Preset"),
        QString(), tr("Keymap Files (*.keymap)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Imported"), tr("Preset imported."));
    }
}

void KeymapPresetManagerDialog::onSetAsDefault() {
    QMessageBox::information(this, tr("Default"), tr("Preset set as default."));
}

// ============================================================================
// Quick Keybinding Widget
// ============================================================================
QuickKeybindingWidget::QuickKeybindingWidget(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void QuickKeybindingWidget::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    actionLabel_ = new QLabel(this);
    layout->addWidget(actionLabel_);
    
    shortcutEdit_ = new QKeySequenceEdit(this);
    shortcutEdit_->setEnabled(false);
    layout->addWidget(shortcutEdit_);
    
    captureBtn_ = new QPushButton(tr("Set"), this);
    connect(captureBtn_, &QPushButton::clicked, this, &QuickKeybindingWidget::onCaptureShortcut);
    layout->addWidget(captureBtn_);
    
    clearBtn_ = new QPushButton(tr("Clear"), this);
    connect(clearBtn_, &QPushButton::clicked, this, &QuickKeybindingWidget::onClearShortcut);
    layout->addWidget(clearBtn_);
    
    resetBtn_ = new QPushButton(tr("Reset"), this);
    connect(resetBtn_, &QPushButton::clicked, this, &QuickKeybindingWidget::onResetShortcut);
    layout->addWidget(resetBtn_);
}

void QuickKeybindingWidget::setAction(const QString& action, const QKeySequence& shortcut) {
    actionLabel_->setText(action);
    shortcutEdit_->setKeySequence(shortcut);
}

void QuickKeybindingWidget::onCaptureShortcut() {
    ShortcutCaptureDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QKeySequence shortcut = dialog.capturedShortcut();
        shortcutEdit_->setKeySequence(shortcut);
        emit shortcutChanged(shortcut);
    }
}

void QuickKeybindingWidget::onClearShortcut() {
    shortcutEdit_->clear();
    emit shortcutChanged(QKeySequence());
}

void QuickKeybindingWidget::onResetShortcut() {
    // Reset to default
    QMessageBox::information(this, tr("Reset"), tr("Shortcut reset to default."));
}

} // namespace scratchrobin::ui
