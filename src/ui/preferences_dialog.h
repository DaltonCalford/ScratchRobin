#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QListWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct Preferences {
  // General
  bool auto_save_enabled = true;
  int auto_save_interval = 5;  // minutes
  bool confirm_on_exit = true;
  bool restore_last_session = true;
  
  // Editor
  QString editor_font = "Consolas";
  int editor_font_size = 10;
  bool show_line_numbers = true;
  bool enable_syntax_highlighting = true;
  bool enable_auto_completion = true;
  bool use_spaces_for_tabs = true;
  int tab_width = 2;
  
  // Query
  int query_timeout = 30;  // seconds
  int max_rows_to_fetch = 1000;
  bool auto_limit_results = true;
  
  // Connection
  int connection_timeout = 10;
  int max_connections = 5;
  bool ssl_verify = true;
};

class PreferencesDialog : public QDialog {
  Q_OBJECT

 public:
  explicit PreferencesDialog(QWidget* parent = nullptr);
  ~PreferencesDialog() override;

  Preferences preferences() const;
  void setPreferences(const Preferences& prefs);

 signals:
  void preferencesChanged(const Preferences& prefs);

 private slots:
  void onSave();
  void onCancel();
  void onReset();
  void onFontChanged(int index);
  void onBrowseFont();

 private:
  void setupUi();
  void setupGeneralTab();
  void setupEditorTab();
  void setupQueryTab();
  void setupConnectionTab();
  
  void loadDefaults();
  void applyPreferences();

  QTabWidget* tab_widget_;
  
  // General tab
  QCheckBox* auto_save_check_;
  QSpinBox* auto_save_spin_;
  QCheckBox* confirm_exit_check_;
  QCheckBox* restore_session_check_;
  
  // Editor tab
  QComboBox* font_combo_;
  QSpinBox* font_size_spin_;
  QCheckBox* line_numbers_check_;
  QCheckBox* syntax_highlight_check_;
  QCheckBox* auto_complete_check_;
  QCheckBox* spaces_for_tabs_check_;
  QSpinBox* tab_width_spin_;
  
  // Query tab
  QSpinBox* query_timeout_spin_;
  QSpinBox* max_rows_spin_;
  QCheckBox* auto_limit_check_;
  
  // Connection tab
  QSpinBox* conn_timeout_spin_;
  QSpinBox* max_conn_spin_;
  QCheckBox* ssl_verify_check_;
  
  // Buttons
  QPushButton* save_btn_;
  QPushButton* cancel_btn_;
  QPushButton* reset_btn_;
  
  Preferences current_prefs_;
};

}  // namespace scratchrobin::ui
