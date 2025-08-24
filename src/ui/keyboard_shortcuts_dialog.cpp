#include "keyboard_shortcuts_dialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QApplication>
#include <QStyle>

namespace scratchrobin {

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget* parent)
    : QDialog(parent), settings_(new QSettings("ScratchRobin", "KeyboardShortcuts", this)) {
    setWindowTitle("Keyboard Shortcuts");
    setModal(true);
    setMinimumSize(700, 500);
    resize(900, 600);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    loadDefaultShortcuts();
    setupUI();
}

void KeyboardShortcutsDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);
    mainLayout_->setContentsMargins(15, 15, 15, 15);

    // Create tab widget
    tabWidget_ = new QTabWidget();
    mainLayout_->addWidget(tabWidget_);

    // Setup tabs
    setupShortcutsTab();
    setupConflictsTab();
    setupSettingsTab();

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &KeyboardShortcutsDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &KeyboardShortcutsDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &KeyboardShortcutsDialog::saveShortcuts);

    mainLayout_->addWidget(dialogButtons_);

    // Load shortcuts and update UI
    loadShortcuts();
    populateShortcutsTable();
    updateButtonStates();
}

void KeyboardShortcutsDialog::setupShortcutsTab() {
    shortcutsTab_ = new QWidget();
    shortcutsLayout_ = new QHBoxLayout(shortcutsTab_);

    // Left side - search and filters
    QVBoxLayout* leftLayout = new QVBoxLayout();

    // Search
    QGroupBox* searchGroup = new QGroupBox("Search & Filter");
    QFormLayout* searchLayout = new QFormLayout(searchGroup);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search shortcuts...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &KeyboardShortcutsDialog::onSearchTextChanged);
    searchLayout->addRow("Search:", searchEdit_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItem("All Categories", "");
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &KeyboardShortcutsDialog::onCategoryChanged);
    searchLayout->addRow("Category:", categoryCombo_);

    leftLayout->addWidget(searchGroup);

    // Action buttons
    QGroupBox* actionsGroup = new QGroupBox("Actions");
    buttonLayout_ = new QVBoxLayout(actionsGroup);

    editButton_ = new QPushButton("Edit Shortcut...");
    editButton_->setIcon(QIcon(":/icons/edit.png"));
    connect(editButton_, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onEditShortcut);
    buttonLayout_->addWidget(editButton_);

    resetButton_ = new QPushButton("Reset Shortcut");
    resetButton_->setIcon(QIcon(":/icons/reset.png"));
    connect(resetButton_, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onResetShortcut);
    buttonLayout_->addWidget(resetButton_);

    resetAllButton_ = new QPushButton("Reset All");
    resetAllButton_->setIcon(QIcon(":/icons/reset_all.png"));
    connect(resetAllButton_, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onResetAllShortcuts);
    buttonLayout_->addWidget(resetAllButton_);

    buttonLayout_->addStretch();

    leftLayout->addWidget(actionsGroup);

    shortcutsLayout_->addLayout(leftLayout, 1);

    // Right side - shortcuts table
    QVBoxLayout* rightLayout = new QVBoxLayout();

    QLabel* tableLabel = new QLabel("Keyboard Shortcuts:");
    tableLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(tableLabel);

    shortcutsTable_ = new QTableWidget();
    shortcutsTable_->setColumnCount(4);
    shortcutsTable_->setHorizontalHeaderLabels({"Action", "Shortcut", "Default", "Description"});
    shortcutsTable_->setAlternatingRowColors(true);
    shortcutsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    shortcutsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    shortcutsTable_->horizontalHeader()->setStretchLastSection(true);
    shortcutsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    shortcutsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    shortcutsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    shortcutsTable_->verticalHeader()->setVisible(false);

    connect(shortcutsTable_, &QTableWidget::itemSelectionChanged,
            this, &KeyboardShortcutsDialog::onTableSelectionChanged);

    rightLayout->addWidget(shortcutsTable_);

    shortcutsLayout_->addLayout(rightLayout, 3);

    tabWidget_->addTab(shortcutsTab_, "Shortcuts");
}

void KeyboardShortcutsDialog::setupConflictsTab() {
    conflictsTab_ = new QWidget();
    conflictsLayout_ = new QVBoxLayout(conflictsTab_);

    QLabel* conflictsTitle = new QLabel("Shortcut Conflicts");
    conflictsTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    conflictsLayout_->addWidget(conflictsTitle);

    conflictsInfoLabel_ = new QLabel("No conflicts detected.");
    conflictsInfoLabel_->setStyleSheet("color: green; font-style: italic;");
    conflictsLayout_->addWidget(conflictsInfoLabel_);

    conflictsList_ = new QListWidget();
    conflictsList_->setMaximumHeight(200);
    conflictsLayout_->addWidget(conflictsList_);

    QPushButton* refreshButton = new QPushButton("Refresh Conflicts");
    connect(refreshButton, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onShowConflicts);
    conflictsLayout_->addWidget(refreshButton);

    conflictsLayout_->addStretch();

    tabWidget_->addTab(conflictsTab_, "Conflicts");
}

void KeyboardShortcutsDialog::setupSettingsTab() {
    settingsTab_ = new QWidget();
    settingsLayout_ = new QVBoxLayout(settingsTab_);

    QLabel* settingsTitle = new QLabel("Shortcut Settings");
    settingsTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    settingsLayout_->addWidget(settingsTitle);

    // General settings
    QGroupBox* generalGroup = new QGroupBox("General");
    QVBoxLayout* generalLayout = new QVBoxLayout(generalGroup);

    showAdvancedCheck_ = new QCheckBox("Show advanced shortcuts");
    showAdvancedCheck_->setChecked(true);
    generalLayout->addWidget(showAdvancedCheck_);

    enableTooltipsCheck_ = new QCheckBox("Show shortcut tooltips");
    enableTooltipsCheck_->setChecked(true);
    generalLayout->addWidget(enableTooltipsCheck_);

    autoSaveCheck_ = new QCheckBox("Auto-save shortcut changes");
    autoSaveCheck_->setChecked(false);
    generalLayout->addWidget(autoSaveCheck_);

    settingsLayout_->addWidget(generalGroup);

    // Import/Export
    QGroupBox* importExportGroup = new QGroupBox("Import/Export");
    QHBoxLayout* importExportLayout = new QHBoxLayout(importExportGroup);

    importButton_ = new QPushButton("Import Shortcuts");
    importButton_->setIcon(QIcon(":/icons/import.png"));
    connect(importButton_, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onImportShortcuts);
    importExportLayout->addWidget(importButton_);

    exportButton_ = new QPushButton("Export Shortcuts");
    exportButton_->setIcon(QIcon(":/icons/export.png"));
    connect(exportButton_, &QPushButton::clicked, this, &KeyboardShortcutsDialog::onExportShortcuts);
    importExportLayout->addWidget(exportButton_);

    settingsLayout_->addWidget(importExportGroup);

    settingsLayout_->addStretch();

    tabWidget_->addTab(settingsTab_, "Settings");
}

void KeyboardShortcutsDialog::loadDefaultShortcuts() {
    // File menu shortcuts
    shortcuts_["file.new"] = KeyboardShortcut("file.new", "File", "New", "Create new connection or document",
                                             QKeySequence("Ctrl+N"));
    shortcuts_["file.open"] = KeyboardShortcut("file.open", "File", "Open", "Open existing file or connection",
                                              QKeySequence("Ctrl+O"));
    shortcuts_["file.save"] = KeyboardShortcut("file.save", "File", "Save", "Save current document",
                                              QKeySequence("Ctrl+S"));
    shortcuts_["file.exit"] = KeyboardShortcut("file.exit", "File", "Exit", "Exit application",
                                              QKeySequence("Ctrl+Q"));

    // Edit menu shortcuts
    shortcuts_["edit.undo"] = KeyboardShortcut("edit.undo", "Edit", "Undo", "Undo last action",
                                              QKeySequence("Ctrl+Z"));
    shortcuts_["edit.redo"] = KeyboardShortcut("edit.redo", "Edit", "Redo", "Redo last undone action",
                                              QKeySequence("Ctrl+Y"));
    shortcuts_["edit.cut"] = KeyboardShortcut("edit.cut", "Edit", "Cut", "Cut selected text",
                                             QKeySequence("Ctrl+X"));
    shortcuts_["edit.copy"] = KeyboardShortcut("edit.copy", "Edit", "Copy", "Copy selected text",
                                              QKeySequence("Ctrl+C"));
    shortcuts_["edit.paste"] = KeyboardShortcut("edit.paste", "Edit", "Paste", "Paste from clipboard",
                                               QKeySequence("Ctrl+V"));
    shortcuts_["edit.find"] = KeyboardShortcut("edit.find", "Edit", "Find", "Find text",
                                              QKeySequence("Ctrl+F"));
    shortcuts_["edit.replace"] = KeyboardShortcut("edit.replace", "Edit", "Replace", "Find and replace text",
                                                 QKeySequence("Ctrl+H"));
    shortcuts_["edit.select_all"] = KeyboardShortcut("edit.select_all", "Edit", "Select All", "Select all text",
                                                    QKeySequence("Ctrl+A"));

    // Database operations
    shortcuts_["db.connect"] = KeyboardShortcut("db.connect", "Database", "Connect", "Connect to database",
                                               QKeySequence("Ctrl+D"));
    shortcuts_["db.disconnect"] = KeyboardShortcut("db.disconnect", "Database", "Disconnect", "Disconnect from database",
                                                  QKeySequence("Ctrl+Shift+D"));
    shortcuts_["db.execute"] = KeyboardShortcut("db.execute", "Database", "Execute Query", "Execute current query",
                                               QKeySequence("F5"));
    shortcuts_["db.stop"] = KeyboardShortcut("db.stop", "Database", "Stop Execution", "Stop current query execution",
                                            QKeySequence("Ctrl+Break"));

    // View shortcuts
    shortcuts_["view.refresh"] = KeyboardShortcut("view.refresh", "View", "Refresh", "Refresh current view",
                                                 QKeySequence("F5"));
    shortcuts_["view.query_history"] = KeyboardShortcut("view.query_history", "View", "Query History", "Show query history",
                                                       QKeySequence("Ctrl+H"));
    shortcuts_["view.object_browser"] = KeyboardShortcut("view.object_browser", "View", "Object Browser", "Show object browser",
                                                        QKeySequence("Ctrl+B"));

    // Tools shortcuts
    shortcuts_["tools.backup"] = KeyboardShortcut("tools.backup", "Tools", "Backup", "Create database backup",
                                                 QKeySequence("Ctrl+B"));
    shortcuts_["tools.import"] = KeyboardShortcut("tools.import", "Tools", "Import", "Import data or schema",
                                                 QKeySequence("Ctrl+I"));
    shortcuts_["tools.export"] = KeyboardShortcut("tools.export", "Tools", "Export", "Export data or schema",
                                                 QKeySequence("Ctrl+E"));
    shortcuts_["tools.preferences"] = KeyboardShortcut("tools.preferences", "Tools", "Preferences", "Open preferences dialog",
                                                      QKeySequence("Ctrl+,"));
    shortcuts_["tools.shortcuts"] = KeyboardShortcut("tools.shortcuts", "Tools", "Keyboard Shortcuts", "Configure keyboard shortcuts",
                                                     QKeySequence("Ctrl+K"));

    // Help shortcuts
    shortcuts_["help.about"] = KeyboardShortcut("help.about", "Help", "About", "Show about dialog",
                                               QKeySequence("F1"));
    shortcuts_["help.update_check"] = KeyboardShortcut("help.update_check", "Help", "Check for Updates", "Check for software updates",
                                                      QKeySequence("Ctrl+U"));

    // Build categories list
    QSet<QString> categorySet;
    for (const KeyboardShortcut& shortcut : shortcuts_) {
        categorySet.insert(shortcut.category);
    }
    categories_ = categorySet.values();
    std::sort(categories_.begin(), categories_.end());
}

void KeyboardShortcutsDialog::loadShortcuts() {
    // Load custom shortcuts from settings
    for (auto& shortcut : shortcuts_) {
        QString savedShortcut = settings_->value(QString("shortcuts/%1").arg(shortcut.id)).toString();
        if (!savedShortcut.isEmpty()) {
            shortcut.currentShortcut = QKeySequence(savedShortcut);
        }
    }
}

void KeyboardShortcutsDialog::saveShortcuts() {
    // Save current shortcuts to settings
    for (const KeyboardShortcut& shortcut : shortcuts_) {
        if (shortcut.currentShortcut != shortcut.defaultShortcut) {
            settings_->setValue(QString("shortcuts/%1").arg(shortcut.id),
                               shortcut.currentShortcut.toString());
        } else {
            settings_->remove(QString("shortcuts/%1").arg(shortcut.id));
        }
    }

    emit shortcutsChanged();
}

void KeyboardShortcutsDialog::populateShortcutsTable() {
    shortcutsTable_->setRowCount(0);

    for (const KeyboardShortcut& shortcut : shortcuts_) {
        // Apply filters
        if (!currentCategory_.isEmpty() && shortcut.category != currentCategory_) {
            continue;
        }

        if (!currentSearchText_.isEmpty()) {
            QString searchText = currentSearchText_.toLower();
            if (!shortcut.actionName.toLower().contains(searchText) &&
                !shortcut.description.toLower().contains(searchText)) {
                continue;
            }
        }

        int row = shortcutsTable_->rowCount();
        shortcutsTable_->insertRow(row);

        // Action name
        QTableWidgetItem* actionItem = new QTableWidgetItem(shortcut.actionName);
        actionItem->setData(Qt::UserRole, shortcut.id);
        shortcutsTable_->setItem(row, 0, actionItem);

        // Current shortcut
        QTableWidgetItem* shortcutItem = new QTableWidgetItem(shortcut.currentShortcut.toString());
        if (shortcut.currentShortcut != shortcut.defaultShortcut) {
            shortcutItem->setForeground(Qt::blue);
            shortcutItem->setFont(QFont("", -1, QFont::Bold));
        }
        shortcutsTable_->setItem(row, 1, shortcutItem);

        // Default shortcut
        QTableWidgetItem* defaultItem = new QTableWidgetItem(shortcut.defaultShortcut.toString());
        shortcutsTable_->setItem(row, 2, defaultItem);

        // Description
        QTableWidgetItem* descItem = new QTableWidgetItem(shortcut.description);
        shortcutsTable_->setItem(row, 3, descItem);

        // Set category as tooltip
        actionItem->setToolTip(QString("Category: %1").arg(shortcut.category));
    }

    shortcutsTable_->resizeColumnsToContents();
}

void KeyboardShortcutsDialog::updateTableFilters() {
    populateShortcutsTable();
}

bool KeyboardShortcutsDialog::validateShortcut(const QString& actionId, const QKeySequence& sequence) {
    if (sequence.isEmpty()) {
        return true; // Empty shortcuts are allowed
    }

    QStringList conflicts = findConflicts(sequence);
    if (!conflicts.isEmpty() && !conflicts.contains(actionId)) {
        emit shortcutConflict(sequence.toString(), conflicts);
        return false;
    }

    return true;
}

QStringList KeyboardShortcutsDialog::findConflicts(const QKeySequence& sequence) const {
    QStringList conflicts;

    for (const KeyboardShortcut& shortcut : shortcuts_) {
        if (shortcut.currentShortcut == sequence) {
            conflicts.append(shortcut.actionName);
        }
    }

    return conflicts;
}

void KeyboardShortcutsDialog::highlightConflicts() {
    conflictingShortcuts_.clear();

    // Check for conflicts
    QMap<QKeySequence, QStringList> conflicts;
    for (const KeyboardShortcut& shortcut : shortcuts_) {
        if (!shortcut.currentShortcut.isEmpty()) {
            conflicts[shortcut.currentShortcut].append(shortcut.actionName);
        }
    }

    // Highlight conflicting items
    for (int row = 0; row < shortcutsTable_->rowCount(); ++row) {
        QTableWidgetItem* actionItem = shortcutsTable_->item(row, 0);
        if (actionItem) {
            QString actionId = actionItem->data(Qt::UserRole).toString();
            QTableWidgetItem* shortcutItem = shortcutsTable_->item(row, 1);

            if (shortcuts_.contains(actionId)) {
                const KeyboardShortcut& shortcut = shortcuts_[actionId];
                QStringList conflicts = findConflicts(shortcut.currentShortcut);

                if (conflicts.size() > 1) {
                    conflictingShortcuts_.insert(shortcut.currentShortcut.toString());
                    if (shortcutItem) {
                        shortcutItem->setBackground(Qt::red);
                        shortcutItem->setForeground(Qt::white);
                    }
                } else {
                    if (shortcutItem) {
                        shortcutItem->setBackground(Qt::white);
                        shortcutItem->setForeground(Qt::black);
                    }
                }
            }
        }
    }

    // Update conflicts tab
    onShowConflicts();
}

void KeyboardShortcutsDialog::updateButtonStates() {
    bool hasSelection = !shortcutsTable_->selectedItems().isEmpty();
    editButton_->setEnabled(hasSelection);
    resetButton_->setEnabled(hasSelection);

    // Check for conflicts
    bool hasConflicts = !conflictingShortcuts_.isEmpty();
    if (hasConflicts) {
        dialogButtons_->button(QDialogButtonBox::Ok)->setEnabled(false);
        dialogButtons_->button(QDialogButtonBox::Ok)->setToolTip("Please resolve shortcut conflicts first");
    } else {
        dialogButtons_->button(QDialogButtonBox::Ok)->setEnabled(true);
        dialogButtons_->button(QDialogButtonBox::Ok)->setToolTip("");
    }
}

// Event handlers
void KeyboardShortcutsDialog::onCategoryChanged(int index) {
    if (index >= 0 && index < categoryCombo_->count()) {
        currentCategory_ = categoryCombo_->itemData(index).toString();
        updateTableFilters();
    }
}

void KeyboardShortcutsDialog::onSearchTextChanged(const QString& text) {
    currentSearchText_ = text;
    updateTableFilters();
}

void KeyboardShortcutsDialog::onShortcutChanged(const QKeySequence& sequence) {
    // This would be called when editing a shortcut
    Q_UNUSED(sequence)
}

void KeyboardShortcutsDialog::onEditShortcut() {
    QList<QTableWidgetItem*> selectedItems = shortcutsTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    int row = selectedItems.first()->row();
    QTableWidgetItem* actionItem = shortcutsTable_->item(row, 0);
    if (!actionItem) {
        return;
    }

    QString actionId = actionItem->data(Qt::UserRole).toString();
    KeyboardShortcut* shortcut = findShortcutById(actionId);
    if (!shortcut || !shortcut->isEditable) {
        QMessageBox::information(this, "Not Editable",
                               "This shortcut cannot be modified.");
        return;
    }

    bool ok;
    QString currentShortcutStr = shortcut->currentShortcut.toString();
    QKeySequence newShortcut = QKeySequence(QInputDialog::getText(this, "Edit Shortcut",
                                                                "Enter new shortcut:",
                                                                QLineEdit::Normal,
                                                                currentShortcutStr, &ok));

    if (ok && validateShortcut(actionId, newShortcut)) {
        shortcut->currentShortcut = newShortcut;
        populateShortcutsTable();
        highlightConflicts();
        updateButtonStates();

        if (autoSaveCheck_->isChecked()) {
            saveShortcuts();
        }
    }
}

void KeyboardShortcutsDialog::onResetShortcut() {
    QList<QTableWidgetItem*> selectedItems = shortcutsTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    int row = selectedItems.first()->row();
    QTableWidgetItem* actionItem = shortcutsTable_->item(row, 0);
    if (!actionItem) {
        return;
    }

    QString actionId = actionItem->data(Qt::UserRole).toString();
    KeyboardShortcut* shortcut = findShortcutById(actionId);
    if (!shortcut) {
        return;
    }

    shortcut->currentShortcut = shortcut->defaultShortcut;
    populateShortcutsTable();
    highlightConflicts();
    updateButtonStates();

    if (autoSaveCheck_->isChecked()) {
        saveShortcuts();
    }
}

void KeyboardShortcutsDialog::onResetAllShortcuts() {
    if (QMessageBox::question(this, "Reset All Shortcuts",
                            "Are you sure you want to reset all shortcuts to their defaults?") != QMessageBox::Yes) {
        return;
    }

    for (KeyboardShortcut& shortcut : shortcuts_) {
        shortcut.currentShortcut = shortcut.defaultShortcut;
    }

    populateShortcutsTable();
    highlightConflicts();
    updateButtonStates();

    if (autoSaveCheck_->isChecked()) {
        saveShortcuts();
    }
}

void KeyboardShortcutsDialog::onImportShortcuts() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Shortcuts",
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                   "Shortcut Files (*.json);;All Files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    // TODO: Implement import functionality
    QMessageBox::information(this, "Import", "Import functionality not yet implemented.");
}

void KeyboardShortcutsDialog::onExportShortcuts() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Shortcuts",
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                   "Shortcut Files (*.json);;All Files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    // TODO: Implement export functionality
    QMessageBox::information(this, "Export", "Export functionality not yet implemented.");
}

void KeyboardShortcutsDialog::onShowConflicts() {
    conflictsList_->clear();

    if (conflictingShortcuts_.isEmpty()) {
        conflictsInfoLabel_->setText("No conflicts detected.");
        conflictsInfoLabel_->setStyleSheet("color: green; font-style: italic;");
        return;
    }

    conflictsInfoLabel_->setText(QString("Found %1 shortcut conflicts:").arg(conflictingShortcuts_.size()));
    conflictsInfoLabel_->setStyleSheet("color: red; font-weight: bold;");

    for (const QString& shortcutStr : conflictingShortcuts_) {
        QKeySequence sequence(shortcutStr);
        QStringList actions = findConflicts(sequence);

        QListWidgetItem* item = new QListWidgetItem();
        item->setText(QString("%1: %2").arg(shortcutStr, actions.join(", ")));
        item->setForeground(Qt::red);
        conflictsList_->addItem(item);
    }
}

void KeyboardShortcutsDialog::onTableSelectionChanged() {
    updateButtonStates();
}

KeyboardShortcut* KeyboardShortcutsDialog::findShortcutById(const QString& id) {
    if (shortcuts_.contains(id)) {
        return &shortcuts_[id];
    }
    return nullptr;
}

void KeyboardShortcutsDialog::accept() {
    if (hasConflicts()) {
        QMessageBox::warning(this, "Shortcut Conflicts",
                           "Please resolve all shortcut conflicts before saving.");
        tabWidget_->setCurrentIndex(1); // Switch to conflicts tab
        return;
    }

    saveShortcuts();
    QDialog::accept();
}

void KeyboardShortcutsDialog::reject() {
    // Reload original shortcuts
    loadShortcuts();
    QDialog::reject();
}

bool KeyboardShortcutsDialog::hasConflicts() const {
    for (const KeyboardShortcut& shortcut : shortcuts_) {
        if (!shortcut.currentShortcut.isEmpty()) {
            QStringList conflicts = findConflicts(shortcut.currentShortcut);
            if (conflicts.size() > 1) {
                return true;
            }
        }
    }
    return false;
}

// Static convenience methods
void KeyboardShortcutsDialog::showKeyboardShortcuts(QWidget* parent) {
    KeyboardShortcutsDialog dialog(parent);
    dialog.exec();
}

bool KeyboardShortcutsDialog::configureShortcut(QWidget* parent, const QString& actionId, QKeySequence& shortcut) {
    // This method would be used to configure individual shortcuts
    Q_UNUSED(parent)
    Q_UNUSED(actionId)
    Q_UNUSED(shortcut)
    return false; // Not implemented yet
}

} // namespace scratchrobin
