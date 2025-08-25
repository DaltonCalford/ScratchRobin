#include "dynamic_connection_dialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QApplication>
#include <QStyle>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QThread>
#include <QMetaType>

namespace scratchrobin {

DynamicConnectionDialog::DynamicConnectionDialog(QWidget* parent)
    : QDialog(parent), settings_(new QSettings("ScratchRobin", "Connections", this)),
      driverManager_(&DatabaseDriverManager::instance()) {
    setWindowTitle("Database Connection");
    setModal(true);
    setMinimumSize(600, 500);
    resize(800, 600);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Initialize with default values
    currentDatabaseType_ = DatabaseType::POSTGRESQL;
    currentConfig_.databaseType = currentDatabaseType_;
    currentConfig_.port = 5432;
    currentConfig_.timeout = 30;
    currentConfig_.sslMode = "prefer";
    currentConfig_.charset = "UTF-8";

    setupUI();
    loadSavedConnections();

    // Connect to driver manager
    connect(driverManager_, &DatabaseDriverManager::connectionTestCompleted,
            this, &DynamicConnectionDialog::connectionTested);
}

void DynamicConnectionDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);
    mainLayout_->setContentsMargins(15, 15, 15, 15);

    // Create main splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal);
    mainLayout_->addWidget(mainSplitter);

    // Left side - connection configuration
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);

    // Create tab widget for connection configuration
    tabWidget_ = new QTabWidget();
    leftLayout->addWidget(tabWidget_);

    // Setup tabs
    setupBasicTab();
    setupAdvancedTab();
    setupSecurityTab();
    setupTestingTab();

    mainSplitter->addWidget(leftWidget);

    // Right side - saved connections
    setupSavedConnections();
    mainSplitter->addWidget(savedConnectionsWidget_);

    // Set splitter sizes
    mainSplitter->setSizes({600, 200});

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &DynamicConnectionDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &DynamicConnectionDialog::reject);
    connect(dialogButtons_->button(QDialogButtonBox::Help), &QPushButton::clicked, [this]() {
        QMessageBox::information(this, "Connection Help",
                               "Select your database type and fill in the connection parameters.\n\n"
                               "Use the 'Test Connection' button to verify your settings.\n\n"
                               "Advanced parameters are available in the Advanced tab.");
    });

    mainLayout_->addWidget(dialogButtons_);

    // Update UI with current configuration
    updateConnectionParameters();
}

void DynamicConnectionDialog::setupBasicTab() {
    basicTab_ = new QWidget();
    basicLayout_ = new QFormLayout(basicTab_);

    // Database type selection
    databaseTypeCombo_ = new QComboBox();
    QStringList databaseTypes = driverManager_->getDatabaseTypeList();
    databaseTypeCombo_->addItems(databaseTypes);
    databaseTypeCombo_->setCurrentIndex(0); // PostgreSQL by default
    connect(databaseTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DynamicConnectionDialog::onDatabaseTypeChanged);
    basicLayout_->addRow("Database Type:", databaseTypeCombo_);

    // Connection name
    connectionNameEdit_ = new QLineEdit();
    connectionNameEdit_->setPlaceholderText("Enter connection name");
    basicLayout_->addRow("Connection Name:", connectionNameEdit_);

    // Basic connection parameters
    hostEdit_ = new QLineEdit();
    hostEdit_->setText("localhost");
    connect(hostEdit_, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    basicLayout_->addRow("Host:", hostEdit_);

    portSpin_ = new QSpinBox();
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5432);
    connect(portSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DynamicConnectionDialog::onParameterValueChanged);
    basicLayout_->addRow("Port:", portSpin_);

    databaseEdit_ = new QLineEdit();
    databaseEdit_->setPlaceholderText("Enter database name");
    connect(databaseEdit_, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    basicLayout_->addRow("Database:", databaseEdit_);

    usernameEdit_ = new QLineEdit();
    usernameEdit_->setPlaceholderText("Enter username");
    connect(usernameEdit_, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    basicLayout_->addRow("Username:", usernameEdit_);

    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("Enter password");
    connect(passwordEdit_, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    basicLayout_->addRow("Password:", passwordEdit_);

    // Options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    savePasswordCheck_ = new QCheckBox("Save Password");
    savePasswordCheck_->setChecked(false);
    optionsLayout->addWidget(savePasswordCheck_);

    autoConnectCheck_ = new QCheckBox("Auto Connect");
    autoConnectCheck_->setChecked(false);
    optionsLayout->addWidget(autoConnectCheck_);
    optionsLayout->addStretch();

    basicLayout_->addRow("Options:", optionsLayout);

    tabWidget_->addTab(basicTab_, "Basic");
}

void DynamicConnectionDialog::setupAdvancedTab() {
    advancedTab_ = new QWidget();
    advancedLayout_ = new QVBoxLayout(advancedTab_);

    // Show advanced parameters checkbox
    showAdvancedCheck_ = new QCheckBox("Show Advanced Parameters");
    showAdvancedCheck_->setChecked(false);
    connect(showAdvancedCheck_, &QCheckBox::toggled, this, &DynamicConnectionDialog::onAdvancedToggled);
    advancedLayout_->addWidget(showAdvancedCheck_);

    // Parameters scroll area
    parametersScrollArea_ = new QScrollArea();
    parametersScrollArea_->setWidgetResizable(true);
    parametersScrollArea_->setVisible(false);

    parametersWidget_ = new QWidget();
    parametersLayout_ = new QFormLayout(parametersWidget_);
    parametersScrollArea_->setWidget(parametersWidget_);

    advancedLayout_->addWidget(parametersScrollArea_);

    // Connection string section
    QGroupBox* connectionStringGroup = new QGroupBox("Connection String");
    QVBoxLayout* connectionStringLayout = new QVBoxLayout(connectionStringGroup);

    connectionStringEdit_ = new QLineEdit();
    connectionStringEdit_->setPlaceholderText("Enter custom connection string");
    connect(connectionStringEdit_, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onConnectionStringChanged);
    connectionStringLayout->addWidget(connectionStringEdit_);

    connectionStringPreview_ = new QTextEdit();
    connectionStringPreview_->setMaximumHeight(100);
    connectionStringPreview_->setReadOnly(true);
    connectionStringLayout->addWidget(connectionStringPreview_);

    advancedLayout_->addWidget(connectionStringGroup);

    tabWidget_->addTab(advancedTab_, "Advanced");
}

void DynamicConnectionDialog::setupSecurityTab() {
    securityTab_ = new QWidget();
    securityLayout_ = new QFormLayout(securityTab_);

    // SSL/TLS settings
    sslModeCombo_ = new QComboBox();
    sslModeCombo_->addItems({"disable", "prefer", "require", "verify-ca", "verify-full"});
    sslModeCombo_->setCurrentText("prefer");
    connect(sslModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DynamicConnectionDialog::onSSLModeChanged);
    securityLayout_->addRow("SSL Mode:", sslModeCombo_);

    sslCaEdit_ = new QLineEdit();
    sslCaEdit_->setPlaceholderText("SSL CA certificate file path");
    securityLayout_->addRow("SSL CA File:", sslCaEdit_);

    sslCertEdit_ = new QLineEdit();
    sslCertEdit_->setPlaceholderText("SSL certificate file path");
    securityLayout_->addRow("SSL Certificate:", sslCertEdit_);

    sslKeyEdit_ = new QLineEdit();
    sslKeyEdit_->setPlaceholderText("SSL key file path");
    securityLayout_->addRow("SSL Key File:", sslKeyEdit_);

    // Connection settings
    timeoutSpin_ = new QSpinBox();
    timeoutSpin_->setRange(1, 300);
    timeoutSpin_->setValue(30);
    timeoutSpin_->setSuffix(" seconds");
    securityLayout_->addRow("Connection Timeout:", timeoutSpin_);

    charsetEdit_ = new QLineEdit();
    charsetEdit_->setText("UTF-8");
    charsetEdit_->setPlaceholderText("Database character set");
    securityLayout_->addRow("Character Set:", charsetEdit_);

    tabWidget_->addTab(securityTab_, "Security");
}

void DynamicConnectionDialog::setupTestingTab() {
    testingTab_ = new QWidget();
    testingLayout_ = new QVBoxLayout(testingTab_);

    // Test connection section
    QGroupBox* testGroup = new QGroupBox("Connection Test");
    QVBoxLayout* testLayout = new QVBoxLayout(testGroup);

    testConnectionButton_ = new QPushButton("Test Connection");
    testConnectionButton_->setIcon(QIcon(":/icons/test.png"));
    connect(testConnectionButton_, &QPushButton::clicked, this, &DynamicConnectionDialog::onTestConnection);
    testLayout->addWidget(testConnectionButton_);

    testProgressBar_ = new QProgressBar();
    testProgressBar_->setVisible(false);
    testLayout->addWidget(testProgressBar_);

    testResultLabel_ = new QLabel("Click 'Test Connection' to verify settings");
    testResultLabel_->setStyleSheet("font-style: italic;");
    testLayout->addWidget(testResultLabel_);

    testingLayout_->addWidget(testGroup);

    // Test details
    QGroupBox* detailsGroup = new QGroupBox("Test Details");
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsGroup);

    testDetailsText_ = new QTextEdit();
    testDetailsText_->setMaximumHeight(200);
    testDetailsText_->setReadOnly(true);
    detailsLayout->addWidget(testDetailsText_);

    testingLayout_->addWidget(detailsGroup);

    testingLayout_->addStretch();

    tabWidget_->addTab(testingTab_, "Testing");
}

void DynamicConnectionDialog::setupSavedConnections() {
    savedConnectionsWidget_ = new QWidget();
    QVBoxLayout* savedLayout = new QVBoxLayout(savedConnectionsWidget_);

    QLabel* savedLabel = new QLabel("Saved Connections");
    savedLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    savedLayout->addWidget(savedLabel);

    savedConnectionsList_ = new QListWidget();
    savedConnectionsList_->setMaximumWidth(200);
    connect(savedConnectionsList_, &QListWidget::itemDoubleClicked,
            [this](QListWidgetItem* item) {
                QString connectionName = item->text();
                loadConnection(connectionName);
            });
    savedLayout->addWidget(savedConnectionsList_);

    QHBoxLayout* savedButtonsLayout = new QHBoxLayout();
    loadConnectionButton_ = new QPushButton("Load");
    loadConnectionButton_->setIcon(QIcon(":/icons/load.png"));
    connect(loadConnectionButton_, &QPushButton::clicked, this, &DynamicConnectionDialog::onLoadConnection);
    savedButtonsLayout->addWidget(loadConnectionButton_);

    deleteConnectionButton_ = new QPushButton("Delete");
    deleteConnectionButton_->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteConnectionButton_, &QPushButton::clicked, [this]() {
        QMessageBox::information(this, "Delete Connection",
                               "Delete connection functionality not yet implemented.");
    });
    savedButtonsLayout->addWidget(deleteConnectionButton_);

    savedLayout->addLayout(savedButtonsLayout);
}

void DynamicConnectionDialog::onDatabaseTypeChanged(int index) {
    if (index < 0) return;

    QString databaseTypeName = databaseTypeCombo_->itemText(index);
    currentDatabaseType_ = driverManager_->stringToDatabaseType(databaseTypeName);
    currentConfig_.databaseType = currentDatabaseType_;

    // Update default port
    QString defaultPort = driverManager_->getDefaultPort(currentDatabaseType_);
    portSpin_->setValue(defaultPort.toInt());

    // Update connection parameters
    updateConnectionParameters();
}

void DynamicConnectionDialog::updateConnectionParameters() {
    clearParameterWidgets();

    QList<ConnectionParameter> parameters = driverManager_->getConnectionParameters(currentDatabaseType_);
    currentParameters_.clear();

    for (const ConnectionParameter& param : parameters) {
        currentParameters_[param.name] = param;
    }

    if (showAdvancedCheck_->isChecked()) {
        updateParameterWidgets();
    }

    // Update connection string preview
    onParameterValueChanged();
}

void DynamicConnectionDialog::createParameterWidget(const ConnectionParameter& param, QWidget* parent, QFormLayout* layout) {
    QWidget* widget = nullptr;

    if (param.dataType == "string") {
        widget = createStringParameterWidget(param);
    } else if (param.dataType == "password") {
        widget = createPasswordParameterWidget(param);
    } else if (param.dataType == "int") {
        widget = createIntParameterWidget(param);
    } else if (param.dataType == "port") {
        widget = createPortParameterWidget(param);
    } else if (param.dataType == "bool") {
        widget = createBoolParameterWidget(param);
    } else if (param.dataType == "file") {
        widget = createFileParameterWidget(param);
    }

    if (widget) {
        parameterWidgets_[param.name] = widget;
        layout->addRow(QString("%1:").arg(param.displayName), widget);
    }
}

void DynamicConnectionDialog::clearParameterWidgets() {
    // Clear existing parameter widgets
    QLayoutItem* child;
    while ((child = parametersLayout_->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    parameterWidgets_.clear();
}

void DynamicConnectionDialog::updateParameterWidgets() {
    clearParameterWidgets();

    for (const ConnectionParameter& param : currentParameters_) {
        createParameterWidget(param, parametersWidget_, parametersLayout_);
    }
}

QWidget* DynamicConnectionDialog::createStringParameterWidget(const ConnectionParameter& param) {
    QLineEdit* edit = new QLineEdit();
    if (param.defaultValue.isValid()) {
        edit->setText(param.defaultValue.toString());
    }
    if (!param.placeholder.isEmpty()) {
        edit->setPlaceholderText(param.placeholder);
    }
    connect(edit, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    return edit;
}

QWidget* DynamicConnectionDialog::createPasswordParameterWidget(const ConnectionParameter& param) {
    QLineEdit* edit = new QLineEdit();
    edit->setEchoMode(QLineEdit::Password);
    if (!param.placeholder.isEmpty()) {
        edit->setPlaceholderText(param.placeholder);
    }
    connect(edit, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);
    return edit;
}

QWidget* DynamicConnectionDialog::createIntParameterWidget(const ConnectionParameter& param) {
    QSpinBox* spin = new QSpinBox();
    spin->setRange(0, 999999);
    if (param.defaultValue.isValid()) {
        spin->setValue(param.defaultValue.toInt());
    }
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DynamicConnectionDialog::onParameterValueChanged);
    return spin;
}

QWidget* DynamicConnectionDialog::createPortParameterWidget(const ConnectionParameter& param) {
    QSpinBox* spin = new QSpinBox();
    spin->setRange(1, 65535);
    if (param.defaultValue.isValid()) {
        spin->setValue(param.defaultValue.toInt());
    } else {
        spin->setValue(5432); // Default PostgreSQL port
    }
    connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &DynamicConnectionDialog::onParameterValueChanged);
    return spin;
}

QWidget* DynamicConnectionDialog::createBoolParameterWidget(const ConnectionParameter& param) {
    QCheckBox* check = new QCheckBox();
    if (param.defaultValue.isValid()) {
        check->setChecked(param.defaultValue.toBool());
    }
    connect(check, &QCheckBox::toggled, this, &DynamicConnectionDialog::onParameterValueChanged);
    return check;
}

QWidget* DynamicConnectionDialog::createFileParameterWidget(const ConnectionParameter& param) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLineEdit* edit = new QLineEdit();
    if (!param.placeholder.isEmpty()) {
        edit->setPlaceholderText(param.placeholder);
    }
    layout->addWidget(edit);

    QPushButton* browseButton = new QPushButton("...");
    browseButton->setMaximumWidth(30);
    connect(browseButton, &QPushButton::clicked, [this, edit]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select File");
        if (!fileName.isEmpty()) {
            edit->setText(fileName);
        }
    });
    layout->addWidget(browseButton);

    connect(edit, &QLineEdit::textChanged, this, &DynamicConnectionDialog::onParameterValueChanged);

    return widget;
}

void DynamicConnectionDialog::onParameterValueChanged() {
    // Update current configuration
    currentConfig_.host = hostEdit_->text();
    currentConfig_.port = portSpin_->value();
    currentConfig_.database = databaseEdit_->text();
    currentConfig_.username = usernameEdit_->text();
    currentConfig_.password = passwordEdit_->text();
    currentConfig_.connectionName = connectionNameEdit_->text();
    currentConfig_.savePassword = savePasswordCheck_->isChecked();
    currentConfig_.autoConnect = autoConnectCheck_->isChecked();
    currentConfig_.timeout = timeoutSpin_->value();
    currentConfig_.sslMode = sslModeCombo_->currentText();
    currentConfig_.charset = charsetEdit_->text();

    // Update additional parameters from advanced widgets
    currentConfig_.additionalParameters.clear();
    for (auto it = parameterWidgets_.begin(); it != parameterWidgets_.end(); ++it) {
        QString paramName = it.key();
        QWidget* widget = it.value();

        if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
            currentConfig_.additionalParameters[paramName] = edit->text();
        } else if (QSpinBox* spin = qobject_cast<QSpinBox*>(widget)) {
            currentConfig_.additionalParameters[paramName] = spin->value();
        } else if (QCheckBox* check = qobject_cast<QCheckBox*>(widget)) {
            currentConfig_.additionalParameters[paramName] = check->isChecked();
        }
    }

    // Update connection string preview
    QString connectionString = generateConnectionString();
    connectionStringPreview_->setPlainText(connectionString);
}

void DynamicConnectionDialog::onTestConnection() {
    if (!validateParameters()) {
        return;
    }

    testConnectionButton_->setEnabled(false);
    testProgressBar_->setVisible(true);
    testProgressBar_->setRange(0, 0); // Indeterminate progress
    testResultLabel_->setText("Testing connection...");
    testResultLabel_->setStyleSheet("color: blue;");

    // Test connection in a separate thread
    QTimer::singleShot(100, [this]() {
        QString errorMessage;
        bool success = driverManager_->testConnection(currentConfig_, errorMessage);

        testConnectionButton_->setEnabled(true);
        testProgressBar_->setVisible(false);

        if (success) {
            testResultLabel_->setText("Connection successful!");
            testResultLabel_->setStyleSheet("color: green; font-weight: bold;");
            testDetailsText_->setPlainText("Connection test completed successfully.\n\n" + errorMessage);
        } else {
            testResultLabel_->setText("Connection failed!");
            testResultLabel_->setStyleSheet("color: red; font-weight: bold;");
            testDetailsText_->setPlainText("Connection test failed:\n\n" + errorMessage);
        }

        emit connectionTested(success, errorMessage);
    });
}

void DynamicConnectionDialog::onSaveConnection() {
    if (currentConfig_.connectionName.isEmpty()) {
        QMessageBox::warning(this, "Save Connection", "Please enter a connection name.");
        return;
    }

    saveConnection();
    updateSavedConnectionsList();

    QMessageBox::information(this, "Connection Saved",
                           QString("Connection '%1' has been saved.").arg(currentConfig_.connectionName));

    emit connectionSaved(currentConfig_.connectionName);
}

void DynamicConnectionDialog::onLoadConnection() {
    QListWidgetItem* currentItem = savedConnectionsList_->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, "Load Connection", "Please select a connection to load.");
        return;
    }

    QString connectionName = currentItem->text();
    loadConnection(connectionName);
}

void DynamicConnectionDialog::onAdvancedToggled(bool checked) {
    parametersScrollArea_->setVisible(checked);
    if (checked) {
        updateParameterWidgets();
    } else {
        clearParameterWidgets();
    }
}

void DynamicConnectionDialog::onConnectionStringChanged() {
    QString connectionString = connectionStringEdit_->text();
    if (!connectionString.isEmpty()) {
        parseConnectionString(connectionString);
    }
}

void DynamicConnectionDialog::onSSLModeChanged(int index) {
    Q_UNUSED(index)
    onParameterValueChanged();
}

bool DynamicConnectionDialog::validateParameters() {
    if (currentConfig_.database.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Database name is required.");
        return false;
    }

    if (currentConfig_.username.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Username is required.");
        return false;
    }

    return driverManager_->validateConnectionParameters(currentDatabaseType_,
                                                       currentConfig_.additionalParameters);
}

QString DynamicConnectionDialog::generateConnectionString() const {
    return driverManager_->generateConnectionString(currentConfig_);
}

void DynamicConnectionDialog::parseConnectionString(const QString& connectionString) {
    // Basic parsing for common connection string formats
    QStringList parts = connectionString.split(";");

    for (const QString& part : parts) {
        QString trimmed = part.trimmed();
        if (trimmed.contains("=")) {
            QStringList keyValue = trimmed.split("=");
            if (keyValue.size() == 2) {
                QString key = keyValue[0].toLower();
                QString value = keyValue[1];

                if (key == "server" || key == "host") {
                    hostEdit_->setText(value);
                } else if (key == "port") {
                    portSpin_->setValue(value.toInt());
                } else if (key == "database" || key == "db") {
                    databaseEdit_->setText(value);
                } else if (key == "uid" || key == "user") {
                    usernameEdit_->setText(value);
                } else if (key == "pwd" || key == "password") {
                    passwordEdit_->setText(value);
                }
            }
        }
    }
}

void DynamicConnectionDialog::saveConnection() {
    QJsonObject json;
    json["type"] = driverManager_->databaseTypeToString(currentConfig_.databaseType);
    json["connectionName"] = currentConfig_.connectionName;
    json["host"] = currentConfig_.host;
    json["port"] = currentConfig_.port;
    json["database"] = currentConfig_.database;
    json["username"] = currentConfig_.username;
    json["savePassword"] = currentConfig_.savePassword;
    json["autoConnect"] = currentConfig_.autoConnect;
    json["timeout"] = currentConfig_.timeout;
    json["sslMode"] = currentConfig_.sslMode;
    json["charset"] = currentConfig_.charset;

    if (currentConfig_.savePassword) {
        json["password"] = currentConfig_.password;
    }

    // Save additional parameters
    QJsonObject additionalParams;
    for (auto it = currentConfig_.additionalParameters.begin();
         it != currentConfig_.additionalParameters.end(); ++it) {
        additionalParams[it.key()] = QJsonValue::fromVariant(it.value());
    }
    json["additionalParameters"] = additionalParams;

    // Save to settings
    QString connectionKey = QString("connections/%1").arg(currentConfig_.connectionName);
    settings_->setValue(connectionKey, QJsonDocument(json).toJson());

    // Add to saved connections list if not already there
    if (!savedConnections_.contains(currentConfig_)) {
        savedConnections_.append(currentConfig_);
    }
}

void DynamicConnectionDialog::loadConnection(const QString& connectionName) {
    QString connectionKey = QString("connections/%1").arg(connectionName);
    QByteArray jsonData = settings_->value(connectionKey).toByteArray();

    if (jsonData.isEmpty()) {
        QMessageBox::warning(this, "Load Connection", "Connection not found.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonObject json = doc.object();

    currentConfig_.connectionName = json["connectionName"].toString();
    currentConfig_.databaseType = driverManager_->stringToDatabaseType(json["type"].toString());
    currentConfig_.host = json["host"].toString();
    currentConfig_.port = json["port"].toInt();
    currentConfig_.database = json["database"].toString();
    currentConfig_.username = json["username"].toString();
    currentConfig_.password = json["password"].toString();
    currentConfig_.savePassword = json["savePassword"].toBool();
    currentConfig_.autoConnect = json["autoConnect"].toBool();
    currentConfig_.timeout = json["timeout"].toInt(30);
    currentConfig_.sslMode = json["sslMode"].toString("prefer");
    currentConfig_.charset = json["charset"].toString("UTF-8");

    // Load additional parameters
    QJsonObject additionalParams = json["additionalParameters"].toObject();
    for (auto it = additionalParams.begin(); it != additionalParams.end(); ++it) {
        currentConfig_.additionalParameters[it.key()] = it.value().toVariant();
    }

    // Update UI
    connectionNameEdit_->setText(currentConfig_.connectionName);
    databaseTypeCombo_->setCurrentText(driverManager_->databaseTypeToString(currentConfig_.databaseType));
    hostEdit_->setText(currentConfig_.host);
    portSpin_->setValue(currentConfig_.port);
    databaseEdit_->setText(currentConfig_.database);
    usernameEdit_->setText(currentConfig_.username);
    passwordEdit_->setText(currentConfig_.password);
    savePasswordCheck_->setChecked(currentConfig_.savePassword);
    autoConnectCheck_->setChecked(currentConfig_.autoConnect);
    sslModeCombo_->setCurrentText(currentConfig_.sslMode);
    timeoutSpin_->setValue(currentConfig_.timeout);
    charsetEdit_->setText(currentConfig_.charset);

    emit connectionSelected(currentConfig_);
}

void DynamicConnectionDialog::loadSavedConnections() {
    savedConnections_.clear();

    // Get all connection keys
    settings_->beginGroup("connections");
    QStringList connectionNames = settings_->childKeys();
    settings_->endGroup();

    // Load each connection
    for (const QString& connectionName : connectionNames) {
        loadConnection(connectionName);
    }

    updateSavedConnectionsList();
}

void DynamicConnectionDialog::updateSavedConnectionsList() {
    savedConnectionsList_->clear();

    for (const DatabaseConnectionConfig& config : savedConnections_) {
        QListWidgetItem* item = new QListWidgetItem(config.connectionName);
        item->setIcon(QIcon(":/icons/database.png"));
        savedConnectionsList_->addItem(item);
    }
}

DatabaseConnectionConfig DynamicConnectionDialog::getConnectionConfig() const {
    return currentConfig_;
}

void DynamicConnectionDialog::setConnectionConfig(const DatabaseConnectionConfig& config) {
    currentConfig_ = config;

    // Update UI with the new configuration
    connectionNameEdit_->setText(config.connectionName);
    databaseTypeCombo_->setCurrentText(driverManager_->databaseTypeToString(config.databaseType));
    hostEdit_->setText(config.host);
    portSpin_->setValue(config.port);
    databaseEdit_->setText(config.database);
    usernameEdit_->setText(config.username);
    passwordEdit_->setText(config.password);
    savePasswordCheck_->setChecked(config.savePassword);
    autoConnectCheck_->setChecked(config.autoConnect);
    sslModeCombo_->setCurrentText(config.sslMode);
    timeoutSpin_->setValue(config.timeout);
    charsetEdit_->setText(config.charset);

    onDatabaseTypeChanged(databaseTypeCombo_->currentIndex());
}

bool DynamicConnectionDialog::testConnection() {
    return onTestConnection(), true; // This will trigger the test
}

void DynamicConnectionDialog::accept() {
    if (!validateParameters()) {
        return;
    }

    QDialog::accept();
}

void DynamicConnectionDialog::reject() {
    QDialog::reject();
}

} // namespace scratchrobin
