#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QTabWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct ConnectionInfo {
  QString name;
  QString host;
  int port;
  QString database;
  QString username;
  QString password;
  bool savePassword;
  QString dialect;
};

class ConnectionDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ConnectionDialog(QWidget* parent = nullptr);
  ~ConnectionDialog() override;

  ConnectionInfo connectionInfo() const;
  void setConnectionInfo(const ConnectionInfo& info);

 private slots:
  void onTestConnection();
  void onConnect();
  void onDialectChanged(int index);
  void togglePasswordVisibility();

 private:
  void setupUi();
  void setupConnections();
  void updateDefaultPort();

  // General tab
  QLineEdit* name_edit_;
  QComboBox* dialect_combo_;
  QLineEdit* host_edit_;
  QSpinBox* port_spin_;
  QLineEdit* database_edit_;
  
  // Authentication tab
  QLineEdit* username_edit_;
  QLineEdit* password_edit_;
  QCheckBox* save_password_check_;
  QPushButton* toggle_password_btn_;
  
  // Buttons
  QPushButton* test_btn_;
  QPushButton* connect_btn_;
  QPushButton* cancel_btn_;
  
  QTabWidget* tab_widget_;
  QLabel* status_label_;
};

}  // namespace scratchrobin::ui
