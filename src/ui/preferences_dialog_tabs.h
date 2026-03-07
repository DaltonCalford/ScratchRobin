#pragma once

#include <QWidget>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QFontComboBox;
class QTabWidget;
class QVBoxLayout;
class QPushButton;
QT_END_NAMESPACE

namespace scratchrobin::ui {

// Forward declarations
class ThemeManager;

/**
 * @brief General preferences tab (Language, Theme, Confirmations, Auto-save)
 */
class GeneralPreferencesTab : public QWidget {
    Q_OBJECT
public:
    explicit GeneralPreferencesTab(QWidget* parent = nullptr);
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

private:
    void setupUi();
    
    QComboBox* languageCombo_ = nullptr;
    QComboBox* themeCombo_ = nullptr;
    QCheckBox* confirmDeleteCheck_ = nullptr;
    QCheckBox* confirmExitCheck_ = nullptr;
    QCheckBox* confirmRollbackCheck_ = nullptr;
    QCheckBox* autoSaveCheck_ = nullptr;
    QSpinBox* autoSaveIntervalSpin_ = nullptr;
};

/**
 * @brief Editor preferences tab (Font, Indentation, Display, Completion)
 */
class EditorPreferencesTab : public QWidget {
    Q_OBJECT
public:
    explicit EditorPreferencesTab(QWidget* parent = nullptr);
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

private:
    void setupUi();
    
    QFontComboBox* fontCombo_ = nullptr;
    QSpinBox* fontSizeSpin_ = nullptr;
    QCheckBox* useTabsCheck_ = nullptr;
    QSpinBox* tabSizeSpin_ = nullptr;
    QCheckBox* lineNumbersCheck_ = nullptr;
    QCheckBox* highlightCurrentLineCheck_ = nullptr;
    QCheckBox* wordWrapCheck_ = nullptr;
    QCheckBox* showWhitespaceCheck_ = nullptr;
    QCheckBox* enableCompletionCheck_ = nullptr;
    QCheckBox* autoActivateCheck_ = nullptr;
    QSpinBox* autoActivateCharsSpin_ = nullptr;
    QSpinBox* completionDelaySpin_ = nullptr;
};

/**
 * @brief Results/Grid preferences tab (View, Data Fetching, Display, Grid Options)
 */
class ResultsPreferencesTab : public QWidget {
    Q_OBJECT
public:
    explicit ResultsPreferencesTab(QWidget* parent = nullptr);
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

private:
    void setupUi();
    
    QComboBox* defaultViewCombo_ = nullptr;
    QSpinBox* fetchSizeSpin_ = nullptr;
    QSpinBox* maxRowsSpin_ = nullptr;
    QLineEdit* nullDisplayEdit_ = nullptr;
    QLineEdit* dateFormatEdit_ = nullptr;
    QLineEdit* decimalSeparatorEdit_ = nullptr;
    QCheckBox* showRowNumbersCheck_ = nullptr;
    QCheckBox* alternatingColorsCheck_ = nullptr;
    QCheckBox* showGridLinesCheck_ = nullptr;
};

/**
 * @brief Connection preferences tab (Defaults, Auto-connect, Security)
 */
class ConnectionPreferencesTab : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionPreferencesTab(QWidget* parent = nullptr);
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

private:
    void setupUi();
    
    QSpinBox* timeoutSpin_ = nullptr;
    QSpinBox* keepAliveSpin_ = nullptr;
    QCheckBox* autoConnectCheck_ = nullptr;
    QCheckBox* reconnectCheck_ = nullptr;
    QCheckBox* savePasswordsCheck_ = nullptr;
    QCheckBox* confirmDangerousCheck_ = nullptr;
};

/**
 * @brief Shortcuts preferences tab (Keyboard shortcut customization)
 */
class ShortcutsPreferencesTab : public QWidget {
    Q_OBJECT
public:
    explicit ShortcutsPreferencesTab(QWidget* parent = nullptr);
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

private:
    void setupUi();
    
    // Uses ShortcutsDialog internally
    QPushButton* editShortcutsBtn_ = nullptr;
};

} // namespace scratchrobin::ui
