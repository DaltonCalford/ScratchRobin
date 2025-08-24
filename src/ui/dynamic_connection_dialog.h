#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QTimer>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>

#include "database/database_driver_manager.h"

namespace scratchrobin {

class DynamicConnectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit DynamicConnectionDialog(QWidget* parent = nullptr);
    ~DynamicConnectionDialog() override = default;

    // Public interface
    DatabaseConnectionConfig getConnectionConfig() const;
    void setConnectionConfig(const DatabaseConnectionConfig& config);
    bool testConnection();
    void saveConnection();
    void loadConnection(const QString& connectionName);

signals:
    void connectionTested(bool success, const QString& message);
    void connectionSaved(const QString& connectionName);
    void connectionSelected(const DatabaseConnectionConfig& config);

public slots:
    void accept() override;
    void reject() override;

private slots:
    void onDatabaseTypeChanged(int index);
    void onParameterValueChanged();
    void onTestConnection();
    void onSaveConnection();
    void onLoadConnection();
    void onAdvancedToggled(bool checked);
    void onConnectionStringChanged();
    void onSSLModeChanged(int index);

private:
    void setupUI();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupSecurityTab();
    void setupTestingTab();
    void setupSavedConnections();

    void updateConnectionParameters();
    void createParameterWidget(const ConnectionParameter& param, QWidget* parent, QFormLayout* layout);
    void clearParameterWidgets();
    void updateParameterWidgets();

    QWidget* createStringParameterWidget(const ConnectionParameter& param);
    QWidget* createPasswordParameterWidget(const ConnectionParameter& param);
    QWidget* createIntParameterWidget(const ConnectionParameter& param);
    QWidget* createPortParameterWidget(const ConnectionParameter& param);
    QWidget* createBoolParameterWidget(const ConnectionParameter& param);
    QWidget* createFileParameterWidget(const ConnectionParameter& param);

    bool validateParameters();
    QString generateConnectionString() const;
    void parseConnectionString(const QString& connectionString);

    void loadSavedConnections();
    void updateSavedConnectionsList();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QComboBox* databaseTypeCombo_;
    QLineEdit* connectionNameEdit_;
    QLineEdit* hostEdit_;
    QSpinBox* portSpin_;
    QLineEdit* databaseEdit_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QCheckBox* savePasswordCheck_;
    QCheckBox* autoConnectCheck_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QCheckBox* showAdvancedCheck_;
    QScrollArea* parametersScrollArea_;
    QWidget* parametersWidget_;
    QFormLayout* parametersLayout_;
    QLineEdit* connectionStringEdit_;
    QTextEdit* connectionStringPreview_;

    // Security tab
    QWidget* securityTab_;
    QFormLayout* securityLayout_;
    QComboBox* sslModeCombo_;
    QLineEdit* sslCaEdit_;
    QLineEdit* sslCertEdit_;
    QLineEdit* sslKeyEdit_;
    QSpinBox* timeoutSpin_;
    QLineEdit* charsetEdit_;

    // Testing tab
    QWidget* testingTab_;
    QVBoxLayout* testingLayout_;
    QPushButton* testConnectionButton_;
    QProgressBar* testProgressBar_;
    QLabel* testResultLabel_;
    QTextEdit* testDetailsText_;

    // Saved connections
    QWidget* savedConnectionsWidget_;
    QListWidget* savedConnectionsList_;
    QPushButton* loadConnectionButton_;
    QPushButton* deleteConnectionButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    DatabaseType currentDatabaseType_;
    DatabaseConnectionConfig currentConfig_;
    QMap<QString, QWidget*> parameterWidgets_;
    QMap<QString, ConnectionParameter> currentParameters_;
    QList<DatabaseConnectionConfig> savedConnections_;

    // Settings
    QSettings* settings_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
