#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QColorDialog>
#include <QSettings>

namespace scratchrobin {

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void applySettings();
    void resetToDefaults();
    void selectQueryFont();
    void selectBackgroundColor();
    void selectForegroundColor();

private:
    void setupUI();
    void setupGeneralTab();
    void setupEditorTab();
    void setupConnectionTab();
    void setupAdvancedTab();
    void loadSettings();
    void saveSettings();
    void updateFontColorButton();
    void updateBackgroundColorButton();

    QTabWidget* tabWidget_;
    QDialogButtonBox* buttonBox_;

    // General tab
    QComboBox* languageCombo_;
    QComboBox* themeCombo_;
    QCheckBox* showSplashCheck_;
    QCheckBox* autoSaveCheck_;
    QSpinBox* autoSaveIntervalSpin_;
    QCheckBox* confirmExitCheck_;

    // Editor tab
    QFontComboBox* fontCombo_;
    QSpinBox* fontSizeSpin_;
    QPushButton* fontColorButton_;
    QPushButton* backgroundColorButton_;
    QCheckBox* syntaxHighlightingCheck_;
    QCheckBox* lineNumbersCheck_;
    QCheckBox* autoCompletionCheck_;
    QCheckBox* wordWrapCheck_;
    QSpinBox* tabWidthSpin_;

    // Connection tab
    QLineEdit* defaultHostEdit_;
    QSpinBox* defaultPortSpin_;
    QComboBox* defaultDriverCombo_;
    QCheckBox* autoConnectCheck_;
    QCheckBox* savePasswordsCheck_;
    QSpinBox* connectionTimeoutSpin_;

    // Advanced tab
    QCheckBox* enableLoggingCheck_;
    QComboBox* logLevelCombo_;
    QCheckBox* enableBackupsCheck_;
    QLineEdit* backupDirectoryEdit_;
    QPushButton* selectBackupDirButton_;
    QSpinBox* maxHistorySizeSpin_;
    QCheckBox* enableUpdatesCheck_;

    // Colors
    QColor queryFontColor_;
    QColor queryBackgroundColor_;
};

} // namespace scratchrobin
