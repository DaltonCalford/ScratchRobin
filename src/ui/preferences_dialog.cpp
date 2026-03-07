#include "ui/preferences_dialog.h"
#include "ui/preferences_dialog_tabs.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>

namespace scratchrobin::ui {

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(tr("Preferences"));
  setMinimumSize(600, 500);
  resize(700, 550);
  
  setupUi();
  loadDefaults();
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::setupUi() {
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setSpacing(12);
  
  // Tab widget
  tab_widget_ = new QTabWidget(this);
  
  // Create tabs
  auto* generalTab = new GeneralPreferencesTab(this);
  auto* editorTab = new EditorPreferencesTab(this);
  auto* resultsTab = new ResultsPreferencesTab(this);
  auto* connectionTab = new ConnectionPreferencesTab(this);
  
  tab_widget_->addTab(generalTab, tr("General"));
  tab_widget_->addTab(editorTab, tr("Editor"));
  tab_widget_->addTab(resultsTab, tr("Results"));
  tab_widget_->addTab(connectionTab, tr("Connection"));
  
  main_layout->addWidget(tab_widget_);
  
  // Buttons
  auto* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  
  reset_btn_ = new QPushButton(tr("Restore Defaults"), this);
  button_layout->addWidget(reset_btn_);
  
  cancel_btn_ = new QPushButton(tr("Cancel"), this);
  button_layout->addWidget(cancel_btn_);
  
  save_btn_ = new QPushButton(tr("OK"), this);
  save_btn_->setDefault(true);
  button_layout->addWidget(save_btn_);
  
  main_layout->addLayout(button_layout);
  
  // Connections
  connect(save_btn_, &QPushButton::clicked, this, &PreferencesDialog::onSave);
  connect(cancel_btn_, &QPushButton::clicked, this, &PreferencesDialog::onCancel);
  connect(reset_btn_, &QPushButton::clicked, this, &PreferencesDialog::onReset);
}

void PreferencesDialog::loadDefaults() {
  QSettings settings;
  
  // Load settings into each tab
  for (int i = 0; i < tab_widget_->count(); ++i) {
    if (auto* tab = qobject_cast<GeneralPreferencesTab*>(tab_widget_->widget(i))) {
      tab->loadSettings(settings);
    } else if (auto* tab = qobject_cast<EditorPreferencesTab*>(tab_widget_->widget(i))) {
      tab->loadSettings(settings);
    } else if (auto* tab = qobject_cast<ResultsPreferencesTab*>(tab_widget_->widget(i))) {
      tab->loadSettings(settings);
    } else if (auto* tab = qobject_cast<ConnectionPreferencesTab*>(tab_widget_->widget(i))) {
      tab->loadSettings(settings);
    }
  }
}

void PreferencesDialog::onSave() {
  QSettings settings;
  
  // Save settings from each tab
  for (int i = 0; i < tab_widget_->count(); ++i) {
    if (auto* tab = qobject_cast<GeneralPreferencesTab*>(tab_widget_->widget(i))) {
      tab->saveSettings(settings);
    } else if (auto* tab = qobject_cast<EditorPreferencesTab*>(tab_widget_->widget(i))) {
      tab->saveSettings(settings);
    } else if (auto* tab = qobject_cast<ResultsPreferencesTab*>(tab_widget_->widget(i))) {
      tab->saveSettings(settings);
    } else if (auto* tab = qobject_cast<ConnectionPreferencesTab*>(tab_widget_->widget(i))) {
      tab->saveSettings(settings);
    }
  }
  
  emit preferencesChanged(current_prefs_);
  accept();
}

void PreferencesDialog::onCancel() {
  reject();
}

void PreferencesDialog::onReset() {
  QMessageBox::StandardButton reply = QMessageBox::question(this, 
      tr("Restore Defaults"),
      tr("Are you sure you want to restore all preferences to default values?"),
      QMessageBox::Yes | QMessageBox::No);
  
  if (reply == QMessageBox::Yes) {
    QSettings settings;
    settings.clear();
    loadDefaults();
  }
}

Preferences PreferencesDialog::preferences() const {
  return current_prefs_;
}

void PreferencesDialog::setPreferences(const Preferences& prefs) {
  current_prefs_ = prefs;
  loadDefaults();
}

// Legacy methods - kept for compatibility
void PreferencesDialog::onFontChanged(int index) { Q_UNUSED(index) }
void PreferencesDialog::onBrowseFont() {}
void PreferencesDialog::applyPreferences() {}
void PreferencesDialog::setupGeneralTab() {}
void PreferencesDialog::setupEditorTab() {}
void PreferencesDialog::setupQueryTab() {}
void PreferencesDialog::setupConnectionTab() {}

}  // namespace scratchrobin::ui
