#include "ui/preferences_dialog_tabs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QFontComboBox>
#include <QPushButton>
#include <QSettings>

namespace scratchrobin::ui {

// ============================================================================
// General Preferences Tab
// ============================================================================

GeneralPreferencesTab::GeneralPreferencesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void GeneralPreferencesTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // Language group
    auto* langGroup = new QGroupBox(tr("Language"), this);
    auto* langLayout = new QHBoxLayout(langGroup);
    langLayout->addWidget(new QLabel(tr("Interface language:"), this));
    languageCombo_ = new QComboBox(this);
    languageCombo_->addItem(tr("English"), "en");
    languageCombo_->addItem(tr("Spanish"), "es");
    languageCombo_->addItem(tr("French"), "fr");
    languageCombo_->addItem(tr("German"), "de");
    langLayout->addWidget(languageCombo_);
    langLayout->addStretch();
    layout->addWidget(langGroup);
    
    // Theme group
    auto* themeGroup = new QGroupBox(tr("Theme"), this);
    auto* themeLayout = new QHBoxLayout(themeGroup);
    themeLayout->addWidget(new QLabel(tr("Color theme:"), this));
    themeCombo_ = new QComboBox(this);
    themeCombo_->addItem(tr("System Default"), "system");
    themeCombo_->addItem(tr("Light"), "light");
    themeCombo_->addItem(tr("Dark"), "dark");
    themeCombo_->addItem(tr("Night (Low Blue Light)"), "night");
    themeCombo_->addItem(tr("High Contrast"), "highcontrast");
    themeLayout->addWidget(themeCombo_);
    themeLayout->addStretch();
    layout->addWidget(themeGroup);
    
    // Confirmations group
    auto* confirmGroup = new QGroupBox(tr("Confirmations"), this);
    auto* confirmLayout = new QVBoxLayout(confirmGroup);
    confirmDeleteCheck_ = new QCheckBox(tr("Confirm object deletion"), this);
    confirmExitCheck_ = new QCheckBox(tr("Confirm exit with unsaved changes"), this);
    confirmRollbackCheck_ = new QCheckBox(tr("Confirm transaction rollback"), this);
    confirmLayout->addWidget(confirmDeleteCheck_);
    confirmLayout->addWidget(confirmExitCheck_);
    confirmLayout->addWidget(confirmRollbackCheck_);
    layout->addWidget(confirmGroup);
    
    // Auto-save group
    auto* autoSaveGroup = new QGroupBox(tr("Auto-save"), this);
    auto* autoSaveLayout = new QVBoxLayout(autoSaveGroup);
    autoSaveCheck_ = new QCheckBox(tr("Auto-save SQL scripts"), this);
    autoSaveLayout->addWidget(autoSaveCheck_);
    auto* intervalLayout = new QHBoxLayout();
    intervalLayout->addWidget(new QLabel(tr("Interval:"), this));
    autoSaveIntervalSpin_ = new QSpinBox(this);
    autoSaveIntervalSpin_->setRange(1, 60);
    autoSaveIntervalSpin_->setSuffix(tr(" minutes"));
    intervalLayout->addWidget(autoSaveIntervalSpin_);
    intervalLayout->addStretch();
    autoSaveLayout->addLayout(intervalLayout);
    layout->addWidget(autoSaveGroup);
    
    layout->addStretch();
}

void GeneralPreferencesTab::loadSettings(QSettings& settings) {
    languageCombo_->setCurrentIndex(languageCombo_->findData(settings.value("language", "en").toString()));
    themeCombo_->setCurrentIndex(themeCombo_->findData(settings.value("theme", "system").toString()));
    confirmDeleteCheck_->setChecked(settings.value("confirmDelete", true).toBool());
    confirmExitCheck_->setChecked(settings.value("confirmExit", true).toBool());
    confirmRollbackCheck_->setChecked(settings.value("confirmRollback", true).toBool());
    autoSaveCheck_->setChecked(settings.value("autoSave", true).toBool());
    autoSaveIntervalSpin_->setValue(settings.value("autoSaveInterval", 5).toInt());
}

void GeneralPreferencesTab::saveSettings(QSettings& settings) {
    settings.setValue("language", languageCombo_->currentData());
    settings.setValue("theme", themeCombo_->currentData());
    settings.setValue("confirmDelete", confirmDeleteCheck_->isChecked());
    settings.setValue("confirmExit", confirmExitCheck_->isChecked());
    settings.setValue("confirmRollback", confirmRollbackCheck_->isChecked());
    settings.setValue("autoSave", autoSaveCheck_->isChecked());
    settings.setValue("autoSaveInterval", autoSaveIntervalSpin_->value());
}

// ============================================================================
// Editor Preferences Tab
// ============================================================================

EditorPreferencesTab::EditorPreferencesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void EditorPreferencesTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // Font group
    auto* fontGroup = new QGroupBox(tr("Font"), this);
    auto* fontLayout = new QGridLayout(fontGroup);
    fontLayout->addWidget(new QLabel(tr("Font family:"), this), 0, 0);
    fontCombo_ = new QFontComboBox(this);
    fontLayout->addWidget(fontCombo_, 0, 1);
    fontLayout->addWidget(new QLabel(tr("Size:"), this), 1, 0);
    fontSizeSpin_ = new QSpinBox(this);
    fontSizeSpin_->setRange(6, 72);
    fontSizeSpin_->setSuffix(tr(" pt"));
    fontLayout->addWidget(fontSizeSpin_, 1, 1);
    fontLayout->setColumnStretch(1, 1);
    layout->addWidget(fontGroup);
    
    // Indentation group
    auto* indentGroup = new QGroupBox(tr("Indentation"), this);
    auto* indentLayout = new QVBoxLayout(indentGroup);
    useTabsCheck_ = new QCheckBox(tr("Use tabs instead of spaces"), this);
    indentLayout->addWidget(useTabsCheck_);
    auto* tabSizeLayout = new QHBoxLayout();
    tabSizeLayout->addWidget(new QLabel(tr("Tab size:"), this));
    tabSizeSpin_ = new QSpinBox(this);
    tabSizeSpin_->setRange(1, 8);
    tabSizeSpin_->setSuffix(tr(" spaces"));
    tabSizeLayout->addWidget(tabSizeSpin_);
    tabSizeLayout->addStretch();
    indentLayout->addLayout(tabSizeLayout);
    layout->addWidget(indentGroup);
    
    // Display group
    auto* displayGroup = new QGroupBox(tr("Display"), this);
    auto* displayLayout = new QVBoxLayout(displayGroup);
    lineNumbersCheck_ = new QCheckBox(tr("Show line numbers"), this);
    highlightCurrentLineCheck_ = new QCheckBox(tr("Highlight current line"), this);
    wordWrapCheck_ = new QCheckBox(tr("Word wrap"), this);
    showWhitespaceCheck_ = new QCheckBox(tr("Show whitespace characters"), this);
    displayLayout->addWidget(lineNumbersCheck_);
    displayLayout->addWidget(highlightCurrentLineCheck_);
    displayLayout->addWidget(wordWrapCheck_);
    displayLayout->addWidget(showWhitespaceCheck_);
    layout->addWidget(displayGroup);
    
    // Code Completion group
    auto* completionGroup = new QGroupBox(tr("Code Completion"), this);
    auto* completionLayout = new QVBoxLayout(completionGroup);
    enableCompletionCheck_ = new QCheckBox(tr("Enable code completion"), this);
    completionLayout->addWidget(enableCompletionCheck_);
    auto* autoActivateLayout = new QHBoxLayout();
    autoActivateCheck_ = new QCheckBox(tr("Auto-activate after"), this);
    autoActivateLayout->addWidget(autoActivateCheck_);
    autoActivateCharsSpin_ = new QSpinBox(this);
    autoActivateCharsSpin_->setRange(1, 10);
    autoActivateCharsSpin_->setSuffix(tr(" characters"));
    autoActivateLayout->addWidget(autoActivateCharsSpin_);
    autoActivateLayout->addStretch();
    completionLayout->addLayout(autoActivateLayout);
    auto* delayLayout = new QHBoxLayout();
    delayLayout->addWidget(new QLabel(tr("Delay:"), this));
    completionDelaySpin_ = new QSpinBox(this);
    completionDelaySpin_->setRange(0, 1000);
    completionDelaySpin_->setSingleStep(50);
    completionDelaySpin_->setSuffix(tr(" ms"));
    delayLayout->addWidget(completionDelaySpin_);
    delayLayout->addStretch();
    completionLayout->addLayout(delayLayout);
    layout->addWidget(completionGroup);
    
    layout->addStretch();
}

void EditorPreferencesTab::loadSettings(QSettings& settings) {
    fontCombo_->setCurrentFont(QFont(settings.value("editor/font", "Monospace").toString()));
    fontSizeSpin_->setValue(settings.value("editor/fontSize", 10).toInt());
    useTabsCheck_->setChecked(settings.value("editor/useTabs", false).toBool());
    tabSizeSpin_->setValue(settings.value("editor/tabSize", 4).toInt());
    lineNumbersCheck_->setChecked(settings.value("editor/lineNumbers", true).toBool());
    highlightCurrentLineCheck_->setChecked(settings.value("editor/highlightCurrentLine", true).toBool());
    wordWrapCheck_->setChecked(settings.value("editor/wordWrap", false).toBool());
    showWhitespaceCheck_->setChecked(settings.value("editor/showWhitespace", false).toBool());
    enableCompletionCheck_->setChecked(settings.value("editor/enableCompletion", true).toBool());
    autoActivateCheck_->setChecked(settings.value("editor/autoActivate", true).toBool());
    autoActivateCharsSpin_->setValue(settings.value("editor/autoActivateChars", 3).toInt());
    completionDelaySpin_->setValue(settings.value("editor/completionDelay", 250).toInt());
}

void EditorPreferencesTab::saveSettings(QSettings& settings) {
    settings.setValue("editor/font", fontCombo_->currentFont().family());
    settings.setValue("editor/fontSize", fontSizeSpin_->value());
    settings.setValue("editor/useTabs", useTabsCheck_->isChecked());
    settings.setValue("editor/tabSize", tabSizeSpin_->value());
    settings.setValue("editor/lineNumbers", lineNumbersCheck_->isChecked());
    settings.setValue("editor/highlightCurrentLine", highlightCurrentLineCheck_->isChecked());
    settings.setValue("editor/wordWrap", wordWrapCheck_->isChecked());
    settings.setValue("editor/showWhitespace", showWhitespaceCheck_->isChecked());
    settings.setValue("editor/enableCompletion", enableCompletionCheck_->isChecked());
    settings.setValue("editor/autoActivate", autoActivateCheck_->isChecked());
    settings.setValue("editor/autoActivateChars", autoActivateCharsSpin_->value());
    settings.setValue("editor/completionDelay", completionDelaySpin_->value());
}

// ============================================================================
// Results Preferences Tab
// ============================================================================

ResultsPreferencesTab::ResultsPreferencesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ResultsPreferencesTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // Default view group
    auto* viewGroup = new QGroupBox(tr("Default View"), this);
    auto* viewLayout = new QHBoxLayout(viewGroup);
    viewLayout->addWidget(new QLabel(tr("Default result view:"), this));
    defaultViewCombo_ = new QComboBox(this);
    defaultViewCombo_->addItem(tr("Grid"), "grid");
    defaultViewCombo_->addItem(tr("Record"), "record");
    defaultViewCombo_->addItem(tr("Text"), "text");
    viewLayout->addWidget(defaultViewCombo_);
    viewLayout->addStretch();
    layout->addWidget(viewGroup);
    
    // Data fetching group
    auto* fetchGroup = new QGroupBox(tr("Data Fetching"), this);
    auto* fetchLayout = new QGridLayout(fetchGroup);
    fetchLayout->addWidget(new QLabel(tr("Fetch size:"), this), 0, 0);
    fetchSizeSpin_ = new QSpinBox(this);
    fetchSizeSpin_->setRange(100, 10000);
    fetchSizeSpin_->setSingleStep(100);
    fetchSizeSpin_->setSuffix(tr(" rows"));
    fetchLayout->addWidget(fetchSizeSpin_, 0, 1);
    fetchLayout->addWidget(new QLabel(tr("Maximum rows:"), this), 1, 0);
    maxRowsSpin_ = new QSpinBox(this);
    maxRowsSpin_->setRange(0, 1000000);
    maxRowsSpin_->setSingleStep(1000);
    maxRowsSpin_->setSpecialValueText(tr("Unlimited"));
    maxRowsSpin_->setSuffix(tr(" rows (0=unlimited)"));
    fetchLayout->addWidget(maxRowsSpin_, 1, 1);
    fetchLayout->setColumnStretch(1, 1);
    layout->addWidget(fetchGroup);
    
    // Display group
    auto* displayGroup = new QGroupBox(tr("Display"), this);
    auto* displayLayout = new QGridLayout(displayGroup);
    displayLayout->addWidget(new QLabel(tr("Null value display:"), this), 0, 0);
    nullDisplayEdit_ = new QLineEdit(this);
    nullDisplayEdit_->setPlaceholderText(tr("e.g., NULL, <null>, ∅"));
    displayLayout->addWidget(nullDisplayEdit_, 0, 1);
    displayLayout->addWidget(new QLabel(tr("Date/Time format:"), this), 1, 0);
    dateFormatEdit_ = new QLineEdit(this);
    dateFormatEdit_->setPlaceholderText(tr("e.g., YYYY-MM-DD HH:MM:SS"));
    displayLayout->addWidget(dateFormatEdit_, 1, 1);
    displayLayout->addWidget(new QLabel(tr("Decimal separator:"), this), 2, 0);
    decimalSeparatorEdit_ = new QLineEdit(this);
    decimalSeparatorEdit_->setMaxLength(1);
    displayLayout->addWidget(decimalSeparatorEdit_, 2, 1);
    displayLayout->setColumnStretch(1, 1);
    layout->addWidget(displayGroup);
    
    // Grid options group
    auto* gridGroup = new QGroupBox(tr("Grid Options"), this);
    auto* gridLayout = new QVBoxLayout(gridGroup);
    showRowNumbersCheck_ = new QCheckBox(tr("Show row numbers"), this);
    alternatingColorsCheck_ = new QCheckBox(tr("Alternating row colors"), this);
    showGridLinesCheck_ = new QCheckBox(tr("Show grid lines"), this);
    gridLayout->addWidget(showRowNumbersCheck_);
    gridLayout->addWidget(alternatingColorsCheck_);
    gridLayout->addWidget(showGridLinesCheck_);
    layout->addWidget(gridGroup);
    
    layout->addStretch();
}

void ResultsPreferencesTab::loadSettings(QSettings& settings) {
    defaultViewCombo_->setCurrentIndex(defaultViewCombo_->findData(settings.value("results/defaultView", "grid").toString()));
    fetchSizeSpin_->setValue(settings.value("results/fetchSize", 1000).toInt());
    maxRowsSpin_->setValue(settings.value("results/maxRows", 10000).toInt());
    nullDisplayEdit_->setText(settings.value("results/nullDisplay", "NULL").toString());
    dateFormatEdit_->setText(settings.value("results/dateFormat", "YYYY-MM-DD HH:MM:SS").toString());
    decimalSeparatorEdit_->setText(settings.value("results/decimalSeparator", ".").toString());
    showRowNumbersCheck_->setChecked(settings.value("results/showRowNumbers", true).toBool());
    alternatingColorsCheck_->setChecked(settings.value("results/alternatingColors", true).toBool());
    showGridLinesCheck_->setChecked(settings.value("results/showGridLines", false).toBool());
}

void ResultsPreferencesTab::saveSettings(QSettings& settings) {
    settings.setValue("results/defaultView", defaultViewCombo_->currentData());
    settings.setValue("results/fetchSize", fetchSizeSpin_->value());
    settings.setValue("results/maxRows", maxRowsSpin_->value());
    settings.setValue("results/nullDisplay", nullDisplayEdit_->text());
    settings.setValue("results/dateFormat", dateFormatEdit_->text());
    settings.setValue("results/decimalSeparator", decimalSeparatorEdit_->text());
    settings.setValue("results/showRowNumbers", showRowNumbersCheck_->isChecked());
    settings.setValue("results/alternatingColors", alternatingColorsCheck_->isChecked());
    settings.setValue("results/showGridLines", showGridLinesCheck_->isChecked());
}

// ============================================================================
// Connection Preferences Tab
// ============================================================================

ConnectionPreferencesTab::ConnectionPreferencesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ConnectionPreferencesTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    
    // Defaults group
    auto* defaultsGroup = new QGroupBox(tr("Defaults"), this);
    auto* defaultsLayout = new QGridLayout(defaultsGroup);
    defaultsLayout->addWidget(new QLabel(tr("Connection timeout:"), this), 0, 0);
    timeoutSpin_ = new QSpinBox(this);
    timeoutSpin_->setRange(1, 300);
    timeoutSpin_->setSuffix(tr(" seconds"));
    defaultsLayout->addWidget(timeoutSpin_, 0, 1);
    defaultsLayout->addWidget(new QLabel(tr("Keep-alive interval:"), this), 1, 0);
    keepAliveSpin_ = new QSpinBox(this);
    keepAliveSpin_->setRange(0, 3600);
    keepAliveSpin_->setSpecialValueText(tr("Disabled"));
    keepAliveSpin_->setSuffix(tr(" seconds (0=disabled)"));
    defaultsLayout->addWidget(keepAliveSpin_, 1, 1);
    defaultsLayout->setColumnStretch(1, 1);
    layout->addWidget(defaultsGroup);
    
    // Auto-connect group
    auto* autoConnectGroup = new QGroupBox(tr("Auto-connect"), this);
    auto* autoConnectLayout = new QVBoxLayout(autoConnectGroup);
    autoConnectCheck_ = new QCheckBox(tr("Connect on startup"), this);
    reconnectCheck_ = new QCheckBox(tr("Reconnect on connection loss"), this);
    autoConnectLayout->addWidget(autoConnectCheck_);
    autoConnectLayout->addWidget(reconnectCheck_);
    layout->addWidget(autoConnectGroup);
    
    // Security group
    auto* securityGroup = new QGroupBox(tr("Security"), this);
    auto* securityLayout = new QVBoxLayout(securityGroup);
    savePasswordsCheck_ = new QCheckBox(tr("Save passwords (encrypted)"), this);
    confirmDangerousCheck_ = new QCheckBox(tr("Confirm before executing dangerous operations (DELETE without WHERE, DROP, etc.)"), this);
    securityLayout->addWidget(savePasswordsCheck_);
    securityLayout->addWidget(confirmDangerousCheck_);
    layout->addWidget(securityGroup);
    
    layout->addStretch();
}

void ConnectionPreferencesTab::loadSettings(QSettings& settings) {
    timeoutSpin_->setValue(settings.value("connection/timeout", 30).toInt());
    keepAliveSpin_->setValue(settings.value("connection/keepAlive", 0).toInt());
    autoConnectCheck_->setChecked(settings.value("connection/autoConnect", false).toBool());
    reconnectCheck_->setChecked(settings.value("connection/reconnect", false).toBool());
    savePasswordsCheck_->setChecked(settings.value("connection/savePasswords", true).toBool());
    confirmDangerousCheck_->setChecked(settings.value("connection/confirmDangerous", true).toBool());
}

void ConnectionPreferencesTab::saveSettings(QSettings& settings) {
    settings.setValue("connection/timeout", timeoutSpin_->value());
    settings.setValue("connection/keepAlive", keepAliveSpin_->value());
    settings.setValue("connection/autoConnect", autoConnectCheck_->isChecked());
    settings.setValue("connection/reconnect", reconnectCheck_->isChecked());
    settings.setValue("connection/savePasswords", savePasswordsCheck_->isChecked());
    settings.setValue("connection/confirmDangerous", confirmDangerousCheck_->isChecked());
}

// ============================================================================
// Shortcuts Preferences Tab
// ============================================================================

ShortcutsPreferencesTab::ShortcutsPreferencesTab(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void ShortcutsPreferencesTab::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* infoLabel = new QLabel(tr("Keyboard shortcuts can be customized here."), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    editShortcutsBtn_ = new QPushButton(tr("Edit Shortcuts..."), this);
    layout->addWidget(editShortcutsBtn_);
    
    layout->addStretch();
}

void ShortcutsPreferencesTab::loadSettings(QSettings& settings) {
    Q_UNUSED(settings)
}

void ShortcutsPreferencesTab::saveSettings(QSettings& settings) {
    Q_UNUSED(settings)
}

} // namespace scratchrobin::ui
