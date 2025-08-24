#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QListWidget>

namespace scratchrobin {

struct ConnectionProfile {
    QString name;
    QString driver;
    QString host;
    int port;
    QString database;
    QString username;
    QString password;
    bool savePassword;
};

class ConnectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget* parent = nullptr);
    ~ConnectionDialog();

    ConnectionProfile getConnectionProfile() const;
    void setConnectionProfile(const ConnectionProfile& profile);

signals:
    void connectionTested(bool success, const QString& message);

public slots:
    void testConnection();
    void accept() override;

private slots:
    void onDriverChanged(const QString& driver);
    void onSavePasswordChanged(bool checked);

private:
    void setupUI();
    void setupBasicTab();
    void setupAdvancedTab();
    void setupProfilesTab();

    // UI components
    QTabWidget* tabWidget_;
    QDialogButtonBox* buttonBox_;

    // Basic tab
    QComboBox* driverCombo_;
    QLineEdit* hostEdit_;
    QSpinBox* portSpin_;
    QLineEdit* databaseEdit_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QCheckBox* savePasswordCheck_;
    QPushButton* testButton_;
    QProgressBar* testProgress_;

    // Advanced tab
    QLineEdit* connectionNameEdit_;
    QSpinBox* timeoutSpin_;
    QCheckBox* sslCheck_;
    QCheckBox* compressionCheck_;
    QLineEdit* charsetEdit_;

    // Profiles tab
    QListWidget* profilesList_;

    // Status
    QLabel* statusLabel_;
};

} // namespace scratchrobin