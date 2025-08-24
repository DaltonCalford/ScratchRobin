#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QSet>

namespace scratchrobin {

struct KeyboardShortcut {
    QString id;
    QString category;
    QString actionName;
    QString description;
    QKeySequence defaultShortcut;
    QKeySequence currentShortcut;
    bool isEditable = true;

    KeyboardShortcut() = default;
    KeyboardShortcut(const QString& id, const QString& category, const QString& actionName,
                     const QString& description, const QKeySequence& defaultShortcut, bool isEditable = true)
        : id(id), category(category), actionName(actionName), description(description),
          defaultShortcut(defaultShortcut), currentShortcut(defaultShortcut), isEditable(isEditable) {}
};

class KeyboardShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit KeyboardShortcutsDialog(QWidget* parent = nullptr);
    ~KeyboardShortcutsDialog() override = default;

    // Static convenience methods
    static void showKeyboardShortcuts(QWidget* parent);
    static bool configureShortcut(QWidget* parent, const QString& actionId, QKeySequence& shortcut);

    // Shortcut management
    void loadShortcuts();
    void saveShortcuts();
    void resetToDefaults();
    bool hasConflicts() const;

signals:
    void shortcutsChanged();
    void shortcutConflict(const QString& shortcut, const QStringList& conflictingActions);

public slots:
    void accept() override;
    void reject() override;

private slots:
    void onCategoryChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onShortcutChanged(const QKeySequence& sequence);
    void onEditShortcut();
    void onResetShortcut();
    void onResetAllShortcuts();
    void onImportShortcuts();
    void onExportShortcuts();
    void onShowConflicts();
    void onTableSelectionChanged();

private:
    void setupUI();
    void setupShortcutsTab();
    void setupConflictsTab();
    void setupSettingsTab();
    void populateShortcutsTable();
    void updateTableFilters();
    bool validateShortcut(const QString& actionId, const QKeySequence& sequence);
    QStringList findConflicts(const QKeySequence& sequence) const;
    void highlightConflicts();
    void updateButtonStates();
    KeyboardShortcut* findShortcutById(const QString& id);
    void loadDefaultShortcuts();
    void saveCustomShortcuts();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Shortcuts tab
    QWidget* shortcutsTab_;
    QHBoxLayout* shortcutsLayout_;

    // Search and filter
    QLineEdit* searchEdit_;
    QComboBox* categoryCombo_;

    // Shortcuts table
    QTableWidget* shortcutsTable_;

    // Action buttons
    QVBoxLayout* buttonLayout_;
    QPushButton* editButton_;
    QPushButton* resetButton_;
    QPushButton* resetAllButton_;

    // Conflicts tab
    QWidget* conflictsTab_;
    QVBoxLayout* conflictsLayout_;
    QListWidget* conflictsList_;
    QLabel* conflictsInfoLabel_;

    // Settings tab
    QWidget* settingsTab_;
    QVBoxLayout* settingsLayout_;
    QCheckBox* showAdvancedCheck_;
    QCheckBox* enableTooltipsCheck_;
    QCheckBox* autoSaveCheck_;
    QPushButton* importButton_;
    QPushButton* exportButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    QMap<QString, KeyboardShortcut> shortcuts_;
    QStringList categories_;
    QString currentCategory_;
    QString currentSearchText_;
    QString currentEditingAction_;
    QSet<QString> conflictingShortcuts_;

    // Settings
    QSettings* settings_;
};

} // namespace scratchrobin
