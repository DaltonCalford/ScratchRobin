#include "ui/connection_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTabWidget>
#include <QGroupBox>
#include <QMessageBox>
#include <QTimer>

namespace scratchrobin::ui {

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent) {
  setupUi();
  setupConnections();
  setWindowTitle(tr("Database Connection"));
  resize(450, 350);
}

ConnectionDialog::~ConnectionDialog() = default;

void ConnectionDialog::setupUi() {
  auto* main_layout = new QVBoxLayout(this);
  
  // Tab widget
  tab_widget_ = new QTabWidget(this);
  main_layout->addWidget(tab_widget_);
  
  // General tab
  auto* general_tab = new QWidget();
  auto* general_layout = new QGridLayout(general_tab);
  
  // Connection name
  general_layout->addWidget(new QLabel(tr("Connection Name:")), 0, 0);
  name_edit_ = new QLineEdit(this);
  name_edit_->setPlaceholderText(tr("My Database"));
  general_layout->addWidget(name_edit_, 0, 1);
  
  // Dialect
  general_layout->addWidget(new QLabel(tr("Database Type:")), 1, 0);
  dialect_combo_ = new QComboBox(this);
  dialect_combo_->addItem(tr("ScratchBird Native"), "scratchbird-native");
  dialect_combo_->addItem(tr("Firebird"), "firebird");
  dialect_combo_->addItem(tr("PostgreSQL"), "postgresql");
  dialect_combo_->addItem(tr("MySQL"), "mysql");
  dialect_combo_->addItem(tr("SQLite"), "sqlite");
  general_layout->addWidget(dialect_combo_, 1, 1);
  
  // Host
  general_layout->addWidget(new QLabel(tr("Host:")), 2, 0);
  host_edit_ = new QLineEdit(this);
  host_edit_->setText("localhost");
  general_layout->addWidget(host_edit_, 2, 1);
  
  // Port
  general_layout->addWidget(new QLabel(tr("Port:")), 3, 0);
  port_spin_ = new QSpinBox(this);
  port_spin_->setRange(1, 65535);
  port_spin_->setValue(4044);
  general_layout->addWidget(port_spin_, 3, 1);
  
  // Database
  general_layout->addWidget(new QLabel(tr("Database:"), this), 4, 0);
  database_edit_ = new QLineEdit(this);
  database_edit_->setPlaceholderText(tr("database name or path"));
  general_layout->addWidget(database_edit_, 4, 1);
  
  general_layout->setColumnStretch(1, 1);
  tab_widget_->addTab(general_tab, tr("General"));
  
  // Authentication tab
  auto* auth_tab = new QWidget();
  auto* auth_layout = new QGridLayout(auth_tab);
  
  // Username
  auth_layout->addWidget(new QLabel(tr("Username:")), 0, 0);
  username_edit_ = new QLineEdit(this);
  auth_layout->addWidget(username_edit_, 0, 1);
  
  // Password
  auth_layout->addWidget(new QLabel(tr("Password:")), 1, 0);
  auto* password_layout = new QHBoxLayout();
  password_edit_ = new QLineEdit(this);
  password_edit_->setEchoMode(QLineEdit::Password);
  password_layout->addWidget(password_edit_);
  toggle_password_btn_ = new QPushButton(tr("Show"), this);
  toggle_password_btn_->setCheckable(true);
  password_layout->addWidget(toggle_password_btn_);
  auth_layout->addLayout(password_layout, 1, 1);
  
  // Save password
  save_password_check_ = new QCheckBox(tr("Save password"), this);
  auth_layout->addWidget(save_password_check_, 2, 0, 1, 2);
  
  auth_layout->setColumnStretch(1, 1);
  tab_widget_->addTab(auth_tab, tr("Authentication"));
  
  // Status label
  status_label_ = new QLabel(this);
  status_label_->setWordWrap(true);
  main_layout->addWidget(status_label_);
  
  // Buttons
  auto* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  
  test_btn_ = new QPushButton(tr("&Test Connection"), this);
  button_layout->addWidget(test_btn_);
  
  connect_btn_ = new QPushButton(tr("&Connect"), this);
  connect_btn_->setDefault(true);
  button_layout->addWidget(connect_btn_);
  
  cancel_btn_ = new QPushButton(tr("&Cancel"), this);
  button_layout->addWidget(cancel_btn_);
  
  main_layout->addLayout(button_layout);
}

void ConnectionDialog::setupConnections() {
  connect(test_btn_, &QPushButton::clicked, this, &ConnectionDialog::onTestConnection);
  connect(connect_btn_, &QPushButton::clicked, this, &ConnectionDialog::onConnect);
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
  connect(dialect_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ConnectionDialog::onDialectChanged);
  connect(toggle_password_btn_, &QPushButton::toggled, this, &ConnectionDialog::togglePasswordVisibility);
}

void ConnectionDialog::updateDefaultPort() {
  QString dialect = dialect_combo_->currentData().toString();
  if (dialect == "scratchbird-native" || dialect == "firebird") {
    port_spin_->setValue(3050);
  } else if (dialect == "postgresql") {
    port_spin_->setValue(5432);
  } else if (dialect == "mysql") {
    port_spin_->setValue(3306);
  } else if (dialect == "sqlite") {
    port_spin_->setValue(0);
  }
}

void ConnectionDialog::onDialectChanged(int) {
  updateDefaultPort();
}

void ConnectionDialog::togglePasswordVisibility() {
  if (toggle_password_btn_->isChecked()) {
    password_edit_->setEchoMode(QLineEdit::Normal);
    toggle_password_btn_->setText(tr("Hide"));
  } else {
    password_edit_->setEchoMode(QLineEdit::Password);
    toggle_password_btn_->setText(tr("Show"));
  }
}

void ConnectionDialog::onTestConnection() {
  // Simulate connection test
  status_label_->setText(tr("Testing connection to %1...").arg(host_edit_->text()));
  
  // In real implementation, would attempt actual connection here
  QTimer::singleShot(1000, this, [this]() {
    status_label_->setText(tr("<span style='color: green;'>✓ Connection successful!</span>"));
  });
}

void ConnectionDialog::onConnect() {
  if (name_edit_->text().isEmpty()) {
    QMessageBox::warning(this, tr("Validation Error"), tr("Please enter a connection name."));
    return;
  }
  if (host_edit_->text().isEmpty()) {
    QMessageBox::warning(this, tr("Validation Error"), tr("Please enter a host."));
    return;
  }
  accept();
}

ConnectionInfo ConnectionDialog::connectionInfo() const {
  ConnectionInfo info;
  info.name = name_edit_->text();
  info.host = host_edit_->text();
  info.port = port_spin_->value();
  info.database = database_edit_->text();
  info.username = username_edit_->text();
  info.password = password_edit_->text();
  info.savePassword = save_password_check_->isChecked();
  info.dialect = dialect_combo_->currentData().toString();
  return info;
}

void ConnectionDialog::setConnectionInfo(const ConnectionInfo& info) {
  name_edit_->setText(info.name);
  host_edit_->setText(info.host);
  port_spin_->setValue(info.port);
  database_edit_->setText(info.database);
  username_edit_->setText(info.username);
  password_edit_->setText(info.password);
  save_password_check_->setChecked(info.savePassword);
  
  int index = dialect_combo_->findData(info.dialect);
  if (index >= 0) {
    dialect_combo_->setCurrentIndex(index);
  }
}

}  // namespace scratchrobin::ui
