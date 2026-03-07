#pragma once
#include <QDialog>
#include <QKeySequence>
#include <QHash>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QListWidget;
class QKeySequenceEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Custom Keybindings - Full keyboard customization
 * 
 * Customize keyboard shortcuts:
 * - Modify existing shortcuts
 * - Create custom shortcuts
 * - Context-sensitive bindings
 * - Import/export keymaps
 * - Conflict detection
 * - Default reset
 */

// ============================================================================
// Keybinding Structures
// ============================================================================
struct Keybinding {
    QString id;
    QString action;
    QString description;
    QString category;
    QKeySequence primaryShortcut;
    QKeySequence secondaryShortcut;
    bool isCustom = false;
    bool isEnabled = true;
    bool isGlobal = false;
};

struct KeybindingCategory {
    QString id;
    QString name;
    QString description;
    QString icon;
    QList<Keybinding> bindings;
};

struct KeybindingContext {
    QString id;
    QString name;
    QString description;
    QList<Keybinding> bindings;
};

struct KeybindingConflict {
    Keybinding binding1;
    Keybinding binding2;
    QString context;
};

// ============================================================================
// Keybindings Editor Dialog
// ============================================================================
class KeybindingsEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeybindingsEditorDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    // Navigation
    void onCategorySelected(int index);
    void onSearchTextChanged(const QString& text);
    void onBindingSelected(const QModelIndex& index);
    
    // Editing
    void onEditShortcut();
    void onClearShortcut();
    void onSetSecondaryShortcut();
    void onResetToDefault();
    void onResetAll();
    
    // Import/Export
    void onImportKeymap();
    void onExportKeymap();
    void onLoadPreset();
    
    // Conflict resolution
    void onCheckConflicts();
    void onResolveConflict();
    
    void onValidate();
    void onApply();

private:
    void setupUi();
    void setupCategories();
    void setupBindingsList();
    void setupDetailsPanel();
    void loadKeybindings();
    void updateBindingsList();
    void updateBindingDetails(const Keybinding& binding);
    QList<KeybindingConflict> detectConflicts();
    
    backend::SessionClient* client_;
    QList<KeybindingCategory> categories_;
    QList<Keybinding> allBindings_;
    Keybinding currentBinding_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    
    // Categories
    QListWidget* categoryList_ = nullptr;
    
    // Bindings list
    QTableView* bindingsTable_ = nullptr;
    QStandardItemModel* bindingsModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    
    // Details panel
    QLabel* actionLabel_ = nullptr;
    QLabel* descriptionLabel_ = nullptr;
    QKeySequenceEdit* primaryShortcutEdit_ = nullptr;
    QKeySequenceEdit* secondaryShortcutEdit_ = nullptr;
    QCheckBox* enabledCheck_ = nullptr;
    QCheckBox* globalCheck_ = nullptr;
    
    QPushButton* editBtn_ = nullptr;
    QPushButton* clearBtn_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
};

// ============================================================================
// Shortcut Capture Dialog
// ============================================================================
class ShortcutCaptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutCaptureDialog(QWidget* parent = nullptr);
    
    QKeySequence capturedShortcut() const;

public slots:
    void onClear();
    void onCancel();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void updateDisplay();
    
    QKeySequence capturedShortcut_;
    QLabel* displayLabel_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
    
    Qt::KeyboardModifiers currentModifiers_;
    int currentKey_ = 0;
};

// ============================================================================
// Conflict Resolution Dialog
// ============================================================================
class ConflictResolutionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConflictResolutionDialog(const QList<KeybindingConflict>& conflicts,
                                      QWidget* parent = nullptr);

public slots:
    void onConflictSelected(int index);
    void onKeepFirst();
    void onKeepSecond();
    void onChangeFirst();
    void onChangeSecond();
    void onIgnoreConflict();
    void onAutoResolve();

private:
    void setupUi();
    void updateConflictDetails(int index);
    
    QList<KeybindingConflict> conflicts_;
    int currentConflictIndex_ = 0;
    
    QListWidget* conflictList_ = nullptr;
    QLabel* detailsLabel_ = nullptr;
    QLabel* suggestionLabel_ = nullptr;
};

// ============================================================================
// Keymap Preset Manager Dialog
// ============================================================================
class KeymapPresetManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeymapPresetManagerDialog(QWidget* parent = nullptr);

public slots:
    void onPresetSelected(const QModelIndex& index);
    void onLoadPreset();
    void onSavePreset();
    void onDeletePreset();
    void onRenamePreset();
    void onExportPreset();
    void onImportPreset();
    void onSetAsDefault();

private:
    void setupUi();
    void loadPresets();
    
    QTableView* presetTable_ = nullptr;
    QStandardItemModel* presetModel_ = nullptr;
    QString currentPresetId_;
};

// ============================================================================
// Quick Keybinding Widget
// ============================================================================
class QuickKeybindingWidget : public QWidget {
    Q_OBJECT

public:
    explicit QuickKeybindingWidget(QWidget* parent = nullptr);
    
    void setAction(const QString& action, const QKeySequence& shortcut);

public slots:
    void onCaptureShortcut();
    void onClearShortcut();
    void onResetShortcut();

signals:
    void shortcutChanged(const QKeySequence& shortcut);

private:
    void setupUi();
    
    QLabel* actionLabel_ = nullptr;
    QKeySequenceEdit* shortcutEdit_ = nullptr;
    QPushButton* captureBtn_ = nullptr;
    QPushButton* clearBtn_ = nullptr;
    QPushButton* resetBtn_ = nullptr;
};

} // namespace scratchrobin::ui
