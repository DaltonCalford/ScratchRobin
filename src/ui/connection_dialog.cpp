#include "connection_dialog.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QTimer>
#include <QIcon>

namespace scratchrobin {

ConnectionDialog::ConnectionDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Database Connection");
    setModal(true);
    setFixedSize(500, 400);

    // Set window icon
    setWindowIcon(QIcon(":/logos/Artwork/ScratchRobin.png"));

    setupUI();

    // Connect signals
    connect(testButton_, &QPushButton::clicked, this, &ConnectionDialog::testConnection);
    connect(driverCombo_, &QComboBox::currentTextChanged, this, &ConnectionDialog::onDriverChanged);
    connect(savePasswordCheck_, &QCheckBox::toggled, this, &ConnectionDialog::onSavePasswordChanged);

    // Initialize with defaults
    onDriverChanged(driverCombo_->currentText());
}

ConnectionDialog::~ConnectionDialog() {
}

void ConnectionDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QLabel* headerLabel = new QLabel("Connect to Database", this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(headerLabel);

    // Tab widget
    tabWidget_ = new QTabWidget(this);
    mainLayout->addWidget(tabWidget_);

    setupBasicTab();
    setupAdvancedTab();
    setupProfilesTab();

    // Status label
    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet("margin-top: 10px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #f9f9f9;");
    mainLayout->addWidget(statusLabel_);

    // Button box
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &ConnectionDialog::accept);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &ConnectionDialog::reject);
    mainLayout->addWidget(buttonBox_);
}

void ConnectionDialog::setupBasicTab() {
    QWidget* basicTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(basicTab);

    // Database type group
    QGroupBox* dbGroup = new QGroupBox("Database Type");
    QFormLayout* dbForm = new QFormLayout(dbGroup);

    driverCombo_ = new QComboBox();
    driverCombo_->addItems({"PostgreSQL", "MySQL", "SQLite", "Oracle", "SQL Server", "MariaDB"});
    dbForm->addRow("Driver:", driverCombo_);

    layout->addWidget(dbGroup);

    // Connection details group
    QGroupBox* connGroup = new QGroupBox("Connection Details");
    QFormLayout* connForm = new QFormLayout(connGroup);

    hostEdit_ = new QLineEdit("localhost");
    connForm->addRow("Host:", hostEdit_);

    portSpin_ = new QSpinBox();
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5432); // PostgreSQL default
    connForm->addRow("Port:", portSpin_);

    databaseEdit_ = new QLineEdit();
    databaseEdit_->setPlaceholderText("Enter database name...");
    connForm->addRow("Database:", databaseEdit_);

    layout->addWidget(connGroup);

    // Authentication group
    QGroupBox* authGroup = new QGroupBox("Authentication");
    QFormLayout* authForm = new QFormLayout(authGroup);

    usernameEdit_ = new QLineEdit();
    usernameEdit_->setPlaceholderText("Username...");
    authForm->addRow("Username:", usernameEdit_);

    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("Password...");
    authForm->addRow("Password:", passwordEdit_);

    savePasswordCheck_ = new QCheckBox("Save password");
    authForm->addRow("", savePasswordCheck_);

    layout->addWidget(authGroup);

    // Test connection button
    QWidget* testWidget = new QWidget();
    QHBoxLayout* testLayout = new QHBoxLayout(testWidget);

    testButton_ = new QPushButton("Test Connection");
    testButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px 16px; border: none; border-radius: 4px; } QPushButton:hover { background-color: #1976D2; }");
    testLayout->addWidget(testButton_);

    testProgress_ = new QProgressBar();
    testProgress_->setVisible(false);
    testProgress_->setMaximumWidth(150);
    testLayout->addWidget(testProgress_);

    testLayout->addStretch();
    layout->addWidget(testWidget);

    layout->addStretch();
    tabWidget_->addTab(basicTab, "Basic");
}

void ConnectionDialog::setupAdvancedTab() {
    QWidget* advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(advancedTab);

    // Connection name group
    QGroupBox* nameGroup = new QGroupBox("Connection Profile");
    QFormLayout* nameForm = new QFormLayout(nameGroup);

    connectionNameEdit_ = new QLineEdit();
    connectionNameEdit_->setPlaceholderText("Enter connection name...");
    nameForm->addRow("Name:", connectionNameEdit_);

    layout->addWidget(nameGroup);

    // Connection options group
    QGroupBox* optionsGroup = new QGroupBox("Connection Options");
    QFormLayout* optionsForm = new QFormLayout(optionsGroup);

    timeoutSpin_ = new QSpinBox();
    timeoutSpin_->setRange(1, 300);
    timeoutSpin_->setValue(30);
    timeoutSpin_->setSuffix(" seconds");
    optionsForm->addRow("Timeout:", timeoutSpin_);

    sslCheck_ = new QCheckBox("Use SSL connection");
    optionsForm->addRow("", sslCheck_);

    compressionCheck_ = new QCheckBox("Enable compression");
    optionsForm->addRow("", compressionCheck_);

    charsetEdit_ = new QLineEdit("UTF-8");
    optionsForm->addRow("Character Set:", charsetEdit_);

    layout->addWidget(optionsGroup);
    layout->addStretch();

    tabWidget_->addTab(advancedTab, "Advanced");
}

void ConnectionDialog::setupProfilesTab() {
    QWidget* profilesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(profilesTab);

    QLabel* infoLabel = new QLabel("Saved connection profiles:");
    layout->addWidget(infoLabel);

    profilesList_ = new QListWidget();
    layout->addWidget(profilesList_);

    // Add some sample profiles
    profilesList_->addItem("Development DB");
    profilesList_->addItem("Production DB");
    profilesList_->addItem("Test Environment");

    layout->addStretch();
    tabWidget_->addTab(profilesTab, "Profiles");
}

void ConnectionDialog::onDriverChanged(const QString& driver) {
    // Set default ports for different drivers
    if (driver == "PostgreSQL") {
        portSpin_->setValue(5432);
    } else if (driver == "MySQL" || driver == "MariaDB") {
        portSpin_->setValue(3306);
    } else if (driver == "Oracle") {
        portSpin_->setValue(1521);
    } else if (driver == "SQL Server") {
        portSpin_->setValue(1433);
    } else if (driver == "SQLite") {
        portSpin_->setValue(0);
        hostEdit_->setText("");
        databaseEdit_->setPlaceholderText("Enter database file path...");
    }
}

void ConnectionDialog::onSavePasswordChanged(bool checked) {
    if (checked) {
        QMessageBox::warning(this, "Security Warning",
                           "Saving passwords can be a security risk. "
                           "Make sure your computer is secure.");
    }
}

void ConnectionDialog::testConnection() {
    testButton_->setEnabled(false);
    testProgress_->setVisible(true);
    testProgress_->setValue(0);
    statusLabel_->setText("Testing connection...");
    statusLabel_->setStyleSheet("margin-top: 10px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #fff3cd; color: #856404;");

    // Simulate connection test with progress
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this, timer]() {
        static int progress = 0;
        progress += 10;
        testProgress_->setValue(progress);

        if (progress >= 100) {
            timer->stop();
            timer->deleteLater();

            // Simulate connection result
            bool success = !hostEdit_->text().isEmpty() && !databaseEdit_->text().isEmpty();
            QString message = success ? "Connection successful!" : "Connection failed. Please check your settings.";

            testButton_->setEnabled(true);
            testProgress_->setVisible(false);

            if (success) {
                statusLabel_->setText(message);
                statusLabel_->setStyleSheet("margin-top: 10px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #d4edda; color: #155724;");
            } else {
                statusLabel_->setText(message);
                statusLabel_->setStyleSheet("margin-top: 10px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; background-color: #f8d7da; color: #721c24;");
            }

            emit connectionTested(success, message);
        }
    });
    timer->start(100);
}

ConnectionProfile ConnectionDialog::getConnectionProfile() const {
    ConnectionProfile profile;
    profile.driver = driverCombo_->currentText();
    profile.host = hostEdit_->text();
    profile.port = portSpin_->value();
    profile.database = databaseEdit_->text();
    profile.username = usernameEdit_->text();
    profile.password = passwordEdit_->text();
    profile.savePassword = savePasswordCheck_->isChecked();
    profile.name = connectionNameEdit_->text();
    return profile;
}

void ConnectionDialog::setConnectionProfile(const ConnectionProfile& profile) {
    driverCombo_->setCurrentText(profile.driver);
    hostEdit_->setText(profile.host);
    portSpin_->setValue(profile.port);
    databaseEdit_->setText(profile.database);
    usernameEdit_->setText(profile.username);
    passwordEdit_->setText(profile.password);
    savePasswordCheck_->setChecked(profile.savePassword);
    connectionNameEdit_->setText(profile.name);
}

void ConnectionDialog::accept() {
    // Validate required fields
    if (hostEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Connection Error", "Please enter a host address.");
        tabWidget_->setCurrentIndex(0);
        hostEdit_->setFocus();
        return;
    }

    if (databaseEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "Connection Error", "Please enter a database name.");
        tabWidget_->setCurrentIndex(0);
        databaseEdit_->setFocus();
        return;
    }

    QDialog::accept();
}

} // namespace scratchrobin