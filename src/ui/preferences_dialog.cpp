#include "preferences_dialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>

namespace scratchrobin {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Preferences");
    setModal(true);
    setFixedSize(600, 500);
    resize(600, 500);

    setupUI();
    loadSettings();
}

PreferencesDialog::~PreferencesDialog() {
}

void PreferencesDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel("Application Preferences", this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(headerLabel);

    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    setupGeneralTab();
    setupEditorTab();
    setupConnectionTab();
    setupAdvancedTab();

    // Button box with Apply button
    buttonBox_ = new QDialogButtonBox(this);
    QPushButton* applyButton = buttonBox_->addButton("Apply", QDialogButtonBox::ApplyRole);
    QPushButton* resetButton = buttonBox_->addButton("Reset to Defaults", QDialogButtonBox::ResetRole);
    buttonBox_->addButton(QDialogButtonBox::Ok);
    buttonBox_->addButton(QDialogButtonBox::Cancel);

    connect(buttonBox_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &PreferencesDialog::applySettings);
    connect(resetButton, &QPushButton::clicked, this, &PreferencesDialog::resetToDefaults);

    mainLayout->addWidget(buttonBox_);
}

void PreferencesDialog::setupGeneralTab() {
    QWidget* generalTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(generalTab);

    // Appearance group
    QGroupBox* appearanceGroup = new QGroupBox("Appearance");
    QFormLayout* appearanceForm = new QFormLayout(appearanceGroup);

    themeCombo_ = new QComboBox();
    themeCombo_->addItems({"System Default", "Light", "Dark", "High Contrast"});
    appearanceForm->addRow("Theme:", themeCombo_);

    languageCombo_ = new QComboBox();
    languageCombo_->addItems({"English", "Spanish", "French", "German", "Chinese", "Japanese"});
    appearanceForm->addRow("Language:", languageCombo_);

    layout->addWidget(appearanceGroup);

    // Startup group
    QGroupBox* startupGroup = new QGroupBox("Startup Behavior");
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);

    showSplashCheck_ = new QCheckBox("Show splash screen on startup");
    showSplashCheck_->setChecked(true);
    startupLayout->addWidget(showSplashCheck_);

    confirmExitCheck_ = new QCheckBox("Confirm before exiting application");
    confirmExitCheck_->setChecked(true);
    startupLayout->addWidget(confirmExitCheck_);

    layout->addWidget(startupGroup);

    // Auto-save group
    QGroupBox* autosaveGroup = new QGroupBox("Auto-Save");
    QHBoxLayout* autosaveLayout = new QHBoxLayout(autosaveGroup);

    autoSaveCheck_ = new QCheckBox("Enable auto-save");
    autoSaveCheck_->setChecked(true);
    autosaveLayout->addWidget(autoSaveCheck_);

    autosaveLayout->addWidget(new QLabel("Interval:"));
    autoSaveIntervalSpin_ = new QSpinBox();
    autoSaveIntervalSpin_->setRange(1, 60);
    autoSaveIntervalSpin_->setValue(5);
    autoSaveIntervalSpin_->setSuffix(" minutes");
    autosaveLayout->addWidget(autoSaveIntervalSpin_);

    autosaveLayout->addStretch();
    layout->addWidget(autosaveGroup);

    layout->addStretch();
    tabWidget_->addTab(generalTab, "General");
}

void PreferencesDialog::setupEditorTab() {
    QWidget* editorTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(editorTab);

    // Font settings group
    QGroupBox* fontGroup = new QGroupBox("Font & Colors");
    QFormLayout* fontForm = new QFormLayout(fontGroup);

    QWidget* fontWidget = new QWidget();
    QHBoxLayout* fontLayout = new QHBoxLayout(fontWidget);

    fontCombo_ = new QFontComboBox();
    fontCombo_->setCurrentFont(QFont("Monaco"));
    fontLayout->addWidget(fontCombo_);

    fontSizeSpin_ = new QSpinBox();
    fontSizeSpin_->setRange(8, 24);
    fontSizeSpin_->setValue(12);
    fontLayout->addWidget(fontSizeSpin_);

    QPushButton* selectFontButton = new QPushButton("Select Font...");
    connect(selectFontButton, &QPushButton::clicked, this, &PreferencesDialog::selectQueryFont);
    fontLayout->addWidget(selectFontButton);

    fontForm->addRow("Font:", fontWidget);

    QWidget* colorWidget = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorWidget);

    fontColorButton_ = new QPushButton("Text Color");
    fontColorButton_->setStyleSheet("background-color: #000000; color: white;");
    connect(fontColorButton_, &QPushButton::clicked, this, &PreferencesDialog::selectForegroundColor);
    colorLayout->addWidget(fontColorButton_);

    backgroundColorButton_ = new QPushButton("Background Color");
    backgroundColorButton_->setStyleSheet("background-color: #ffffff; color: black;");
    connect(backgroundColorButton_, &QPushButton::clicked, this, &PreferencesDialog::selectBackgroundColor);
    colorLayout->addWidget(backgroundColorButton_);

    colorLayout->addStretch();
    fontForm->addRow("Colors:", colorWidget);

    layout->addWidget(fontGroup);

    // Editor features group
    QGroupBox* featuresGroup = new QGroupBox("Editor Features");
    QVBoxLayout* featuresLayout = new QVBoxLayout(featuresGroup);

    syntaxHighlightingCheck_ = new QCheckBox("Enable syntax highlighting");
    syntaxHighlightingCheck_->setChecked(true);
    featuresLayout->addWidget(syntaxHighlightingCheck_);

    lineNumbersCheck_ = new QCheckBox("Show line numbers");
    lineNumbersCheck_->setChecked(true);
    featuresLayout->addWidget(lineNumbersCheck_);

    autoCompletionCheck_ = new QCheckBox("Enable auto-completion");
    autoCompletionCheck_->setChecked(true);
    featuresLayout->addWidget(autoCompletionCheck_);

    wordWrapCheck_ = new QCheckBox("Enable word wrap");
    wordWrapCheck_->setChecked(false);
    featuresLayout->addWidget(wordWrapCheck_);

    QWidget* tabWidget = new QWidget();
    QHBoxLayout* tabLayout = new QHBoxLayout(tabWidget);
    tabLayout->addWidget(new QLabel("Tab width:"));
    tabWidthSpin_ = new QSpinBox();
    tabWidthSpin_->setRange(2, 8);
    tabWidthSpin_->setValue(4);
    tabLayout->addWidget(tabWidthSpin_);
    tabLayout->addWidget(new QLabel("spaces"));
    tabLayout->addStretch();
    featuresLayout->addWidget(tabWidget);

    layout->addWidget(featuresGroup);
    layout->addStretch();

    tabWidget_->addTab(editorTab, "Editor");
}

void PreferencesDialog::setupConnectionTab() {
    QWidget* connectionTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(connectionTab);

    // Default connection settings group
    QGroupBox* defaultsGroup = new QGroupBox("Default Connection Settings");
    QFormLayout* defaultsForm = new QFormLayout(defaultsGroup);

    defaultDriverCombo_ = new QComboBox();
    defaultDriverCombo_->addItems({"PostgreSQL", "MySQL", "SQLite", "Oracle", "SQL Server", "MariaDB"});
    defaultsForm->addRow("Default Driver:", defaultDriverCombo_);

    defaultHostEdit_ = new QLineEdit("localhost");
    defaultsForm->addRow("Default Host:", defaultHostEdit_);

    defaultPortSpin_ = new QSpinBox();
    defaultPortSpin_->setRange(1, 65535);
    defaultPortSpin_->setValue(5432);
    defaultsForm->addRow("Default Port:", defaultPortSpin_);

    layout->addWidget(defaultsGroup);

    // Connection behavior group
    QGroupBox* behaviorGroup = new QGroupBox("Connection Behavior");
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);

    autoConnectCheck_ = new QCheckBox("Automatically connect to last database on startup");
    autoConnectCheck_->setChecked(false);
    behaviorLayout->addWidget(autoConnectCheck_);

    savePasswordsCheck_ = new QCheckBox("Save passwords in connection profiles");
    savePasswordsCheck_->setChecked(false);
    behaviorLayout->addWidget(savePasswordsCheck_);

    QWidget* timeoutWidget = new QWidget();
    QHBoxLayout* timeoutLayout = new QHBoxLayout(timeoutWidget);
    timeoutLayout->addWidget(new QLabel("Connection timeout:"));
    connectionTimeoutSpin_ = new QSpinBox();
    connectionTimeoutSpin_->setRange(5, 300);
    connectionTimeoutSpin_->setValue(30);
    connectionTimeoutSpin_->setSuffix(" seconds");
    timeoutLayout->addWidget(connectionTimeoutSpin_);
    timeoutLayout->addStretch();
    behaviorLayout->addWidget(timeoutWidget);

    layout->addWidget(behaviorGroup);
    layout->addStretch();

    tabWidget_->addTab(connectionTab, "Connections");
}

void PreferencesDialog::setupAdvancedTab() {
    QWidget* advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);

    // Logging group
    QGroupBox* loggingGroup = new QGroupBox("Logging");
    QHBoxLayout* loggingLayout = new QHBoxLayout(loggingGroup);

    enableLoggingCheck_ = new QCheckBox("Enable logging");
    enableLoggingCheck_->setChecked(true);
    loggingLayout->addWidget(enableLoggingCheck_);

    loggingLayout->addWidget(new QLabel("Log level:"));
    logLevelCombo_ = new QComboBox();
    logLevelCombo_->addItems({"Debug", "Info", "Warning", "Error", "Critical"});
    logLevelCombo_->setCurrentText("Info");
    loggingLayout->addWidget(logLevelCombo_);

    loggingLayout->addStretch();
    layout->addWidget(loggingGroup);

    // Backup group
    QGroupBox* backupGroup = new QGroupBox("Backup Settings");
    QVBoxLayout* backupLayout = new QVBoxLayout(backupGroup);

    enableBackupsCheck_ = new QCheckBox("Enable automatic backups");
    enableBackupsCheck_->setChecked(true);
    backupLayout->addWidget(enableBackupsCheck_);

    QWidget* backupDirWidget = new QWidget();
    QHBoxLayout* backupDirLayout = new QHBoxLayout(backupDirWidget);
    backupDirLayout->addWidget(new QLabel("Backup directory:"));

    backupDirectoryEdit_ = new QLineEdit();
    backupDirectoryEdit_->setText(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups");
    backupDirLayout->addWidget(backupDirectoryEdit_);

    selectBackupDirButton_ = new QPushButton("Browse...");
    connect(selectBackupDirButton_, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Directory",
                                                       backupDirectoryEdit_->text());
        if (!dir.isEmpty()) {
            backupDirectoryEdit_->setText(dir);
        }
    });
    backupDirLayout->addWidget(selectBackupDirButton_);

    backupLayout->addWidget(backupDirWidget);
    layout->addWidget(backupGroup);

    // History group
    QGroupBox* historyGroup = new QGroupBox("Query History");
    QHBoxLayout* historyLayout = new QHBoxLayout(historyGroup);

    historyLayout->addWidget(new QLabel("Maximum history size:"));
    maxHistorySizeSpin_ = new QSpinBox();
    maxHistorySizeSpin_->setRange(100, 10000);
    maxHistorySizeSpin_->setValue(1000);
    maxHistorySizeSpin_->setSingleStep(100);
    maxHistorySizeSpin_->setSuffix(" queries");
    historyLayout->addWidget(maxHistorySizeSpin_);

    historyLayout->addStretch();
    layout->addWidget(historyGroup);

    // Updates group
    QGroupBox* updatesGroup = new QGroupBox("Updates");
    QVBoxLayout* updatesLayout = new QVBoxLayout(updatesGroup);

    enableUpdatesCheck_ = new QCheckBox("Automatically check for updates");
    enableUpdatesCheck_->setChecked(true);
    updatesLayout->addWidget(enableUpdatesCheck_);

    layout->addWidget(updatesGroup);
    layout->addStretch();

    tabWidget_->addTab(advancedTab, "Advanced");
}

void PreferencesDialog::loadSettings() {
    QSettings settings("ScratchRobin", "Preferences");

    // General settings
    themeCombo_->setCurrentText(settings.value("theme", "System Default").toString());
    languageCombo_->setCurrentText(settings.value("language", "English").toString());
    showSplashCheck_->setChecked(settings.value("showSplash", true).toBool());
    confirmExitCheck_->setChecked(settings.value("confirmExit", true).toBool());
    autoSaveCheck_->setChecked(settings.value("autoSave", true).toBool());
    autoSaveIntervalSpin_->setValue(settings.value("autoSaveInterval", 5).toInt());

    // Editor settings
    fontCombo_->setCurrentFont(settings.value("editorFont", QFont("Monaco")).value<QFont>());
    fontSizeSpin_->setValue(settings.value("editorFontSize", 12).toInt());

    queryFontColor_ = settings.value("queryFontColor", QColor(Qt::black)).value<QColor>();
    updateFontColorButton();

    queryBackgroundColor_ = settings.value("queryBackgroundColor", QColor(Qt::white)).value<QColor>();
    updateBackgroundColorButton();

    syntaxHighlightingCheck_->setChecked(settings.value("syntaxHighlighting", true).toBool());
    lineNumbersCheck_->setChecked(settings.value("lineNumbers", true).toBool());
    autoCompletionCheck_->setChecked(settings.value("autoCompletion", true).toBool());
    wordWrapCheck_->setChecked(settings.value("wordWrap", false).toBool());
    tabWidthSpin_->setValue(settings.value("tabWidth", 4).toInt());

    // Connection settings
    defaultDriverCombo_->setCurrentText(settings.value("defaultDriver", "PostgreSQL").toString());
    defaultHostEdit_->setText(settings.value("defaultHost", "localhost").toString());
    defaultPortSpin_->setValue(settings.value("defaultPort", 5432).toInt());
    autoConnectCheck_->setChecked(settings.value("autoConnect", false).toBool());
    savePasswordsCheck_->setChecked(settings.value("savePasswords", false).toBool());
    connectionTimeoutSpin_->setValue(settings.value("connectionTimeout", 30).toInt());

    // Advanced settings
    enableLoggingCheck_->setChecked(settings.value("enableLogging", true).toBool());
    logLevelCombo_->setCurrentText(settings.value("logLevel", "Info").toString());
    enableBackupsCheck_->setChecked(settings.value("enableBackups", true).toBool());
    backupDirectoryEdit_->setText(settings.value("backupDirectory",
                               QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups").toString());
    maxHistorySizeSpin_->setValue(settings.value("maxHistorySize", 1000).toInt());
    enableUpdatesCheck_->setChecked(settings.value("enableUpdates", true).toBool());
}

void PreferencesDialog::saveSettings() {
    QSettings settings("ScratchRobin", "Preferences");

    // General settings
    settings.setValue("theme", themeCombo_->currentText());
    settings.setValue("language", languageCombo_->currentText());
    settings.setValue("showSplash", showSplashCheck_->isChecked());
    settings.setValue("confirmExit", confirmExitCheck_->isChecked());
    settings.setValue("autoSave", autoSaveCheck_->isChecked());
    settings.setValue("autoSaveInterval", autoSaveIntervalSpin_->value());

    // Editor settings
    settings.setValue("editorFont", fontCombo_->currentFont());
    settings.setValue("editorFontSize", fontSizeSpin_->value());
    settings.setValue("queryFontColor", queryFontColor_);
    settings.setValue("queryBackgroundColor", queryBackgroundColor_);
    settings.setValue("syntaxHighlighting", syntaxHighlightingCheck_->isChecked());
    settings.setValue("lineNumbers", lineNumbersCheck_->isChecked());
    settings.setValue("autoCompletion", autoCompletionCheck_->isChecked());
    settings.setValue("wordWrap", wordWrapCheck_->isChecked());
    settings.setValue("tabWidth", tabWidthSpin_->value());

    // Connection settings
    settings.setValue("defaultDriver", defaultDriverCombo_->currentText());
    settings.setValue("defaultHost", defaultHostEdit_->text());
    settings.setValue("defaultPort", defaultPortSpin_->value());
    settings.setValue("autoConnect", autoConnectCheck_->isChecked());
    settings.setValue("savePasswords", savePasswordsCheck_->isChecked());
    settings.setValue("connectionTimeout", connectionTimeoutSpin_->value());

    // Advanced settings
    settings.setValue("enableLogging", enableLoggingCheck_->isChecked());
    settings.setValue("logLevel", logLevelCombo_->currentText());
    settings.setValue("enableBackups", enableBackupsCheck_->isChecked());
    settings.setValue("backupDirectory", backupDirectoryEdit_->text());
    settings.setValue("maxHistorySize", maxHistorySizeSpin_->value());
    settings.setValue("enableUpdates", enableUpdatesCheck_->isChecked());
}

void PreferencesDialog::updateFontColorButton() {
    QString style = QString("background-color: %1; color: %2; border: 1px solid #ccc; padding: 5px;")
                   .arg(queryFontColor_.name())
                   .arg(queryFontColor_.lightness() > 128 ? "black" : "white");
    fontColorButton_->setStyleSheet(style);
    fontColorButton_->setText("Text Color");
}

void PreferencesDialog::updateBackgroundColorButton() {
    QString style = QString("background-color: %1; color: %2; border: 1px solid #ccc; padding: 5px;")
                   .arg(queryBackgroundColor_.name())
                   .arg(queryBackgroundColor_.lightness() > 128 ? "black" : "white");
    backgroundColorButton_->setStyleSheet(style);
    backgroundColorButton_->setText("Background Color");
}

void PreferencesDialog::selectQueryFont() {
    bool ok;
    QFont font = QFontDialog::getFont(&ok, fontCombo_->currentFont(), this);
    if (ok) {
        fontCombo_->setCurrentFont(font);
    }
}

void PreferencesDialog::selectForegroundColor() {
    QColor color = QColorDialog::getColor(queryFontColor_, this, "Select Text Color");
    if (color.isValid()) {
        queryFontColor_ = color;
        updateFontColorButton();
    }
}

void PreferencesDialog::selectBackgroundColor() {
    QColor color = QColorDialog::getColor(queryBackgroundColor_, this, "Select Background Color");
    if (color.isValid()) {
        queryBackgroundColor_ = color;
        updateBackgroundColorButton();
    }
}

void PreferencesDialog::applySettings() {
    saveSettings();
    QMessageBox::information(this, "Settings Applied", "Your preferences have been saved and applied.");
}

void PreferencesDialog::resetToDefaults() {
    if (QMessageBox::question(this, "Reset Settings",
                             "Are you sure you want to reset all settings to their defaults?") == QMessageBox::Yes) {
        // Reset all controls to defaults
        themeCombo_->setCurrentText("System Default");
        languageCombo_->setCurrentText("English");
        showSplashCheck_->setChecked(true);
        confirmExitCheck_->setChecked(true);
        autoSaveCheck_->setChecked(true);
        autoSaveIntervalSpin_->setValue(5);

        fontCombo_->setCurrentFont(QFont("Monaco"));
        fontSizeSpin_->setValue(12);
        queryFontColor_ = QColor(Qt::black);
        queryBackgroundColor_ = QColor(Qt::white);
        updateFontColorButton();
        updateBackgroundColorButton();
        syntaxHighlightingCheck_->setChecked(true);
        lineNumbersCheck_->setChecked(true);
        autoCompletionCheck_->setChecked(true);
        wordWrapCheck_->setChecked(false);
        tabWidthSpin_->setValue(4);

        defaultDriverCombo_->setCurrentText("PostgreSQL");
        defaultHostEdit_->setText("localhost");
        defaultPortSpin_->setValue(5432);
        autoConnectCheck_->setChecked(false);
        savePasswordsCheck_->setChecked(false);
        connectionTimeoutSpin_->setValue(30);

        enableLoggingCheck_->setChecked(true);
        logLevelCombo_->setCurrentText("Info");
        enableBackupsCheck_->setChecked(true);
        backupDirectoryEdit_->setText(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups");
        maxHistorySizeSpin_->setValue(1000);
        enableUpdatesCheck_->setChecked(true);
    }
}

void PreferencesDialog::accept() {
    saveSettings();
    QDialog::accept();
}

void PreferencesDialog::reject() {
    // Reload original settings
    loadSettings();
    QDialog::reject();
}

} // namespace scratchrobin
