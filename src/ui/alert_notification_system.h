#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QCheckBox;
class QLineEdit;
class QSpinBox;
class QGroupBox;
class QSplitter;
class QTreeView;
class QListView;
class QDateTimeEdit;
class QTimer;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Alert/Notification System - Event notifications
 * 
 * Monitor and alert on database events:
 * - Connection events (connect/disconnect)
 * - Slow query alerts
 * - Error notifications
 * - Resource thresholds
 * - Custom alert rules
 * - Email/notification delivery
 */

// ============================================================================
// Alert Types
// ============================================================================
enum class AlertSeverity {
    Info,
    Warning,
    Critical,
    Emergency
};

enum class AlertCategory {
    Connection,
    Query,
    Performance,
    DiskSpace,
    Memory,
    Replication,
    Backup,
    Security,
    Custom
};

enum class AlertStatus {
    Active,
    Acknowledged,
    Resolved,
    Suppressed
};

struct AlertRule {
    int id = 0;
    QString name;
    AlertCategory category;
    AlertSeverity severity;
    QString condition; // e.g., "query_duration > 1000"
    QString message;
    bool isEnabled = true;
    int cooldownMinutes = 5;
    bool emailNotification = false;
    bool desktopNotification = true;
    QString emailRecipients;
};

struct Alert {
    int id = 0;
    int ruleId = 0;
    AlertSeverity severity;
    AlertCategory category;
    QString title;
    QString message;
    QString details;
    QDateTime triggeredAt;
    QDateTime acknowledgedAt;
    QDateTime resolvedAt;
    QString acknowledgedBy;
    AlertStatus status = AlertStatus::Active;
    QString source; // database, server, etc.
};

struct NotificationSettings {
    bool enableDesktopNotifications = true;
    bool enableEmailNotifications = false;
    bool enableSound = true;
    int maxNotifications = 50;
    int retentionDays = 30;
    QString smtpServer;
    int smtpPort = 587;
    QString smtpUsername;
    QString smtpPassword;
    QString fromEmail;
};

// ============================================================================
// Alert Notification System Panel
// ============================================================================
class AlertNotificationSystemPanel : public DockPanel {
    Q_OBJECT

public:
    explicit AlertNotificationSystemPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~AlertNotificationSystemPanel() override;
    
    QString panelTitle() const override { return tr("Alerts & Notifications"); }
    QString panelCategory() const override { return "monitoring"; }

public slots:
    // Alert management
    void onAcknowledgeAlert();
    void onAcknowledgeAll();
    void onResolveAlert();
    void onSuppressAlert();
    void onDeleteAlert();
    void onClearAllAlerts();
    
    // Rule management
    void onNewRule();
    void onEditRule();
    void onDeleteRule();
    void onDuplicateRule();
    void onTestRule();
    void onEnableDisableRule();
    
    // Filtering
    void onFilterBySeverity(int severity);
    void onFilterByCategory(int category);
    void onFilterByStatus(int status);
    void onSearchAlerts(const QString& text);
    void onTimeRangeFilter();
    
    // View options
    void onViewActiveAlerts();
    void onViewAllAlerts();
    void onViewAlertHistory();
    void onViewRules();
    
    // Settings
    void onConfigureNotifications();
    void onConfigureEmail();
    void onTestNotification();

signals:
    void alertTriggered(const Alert& alert);
    void alertAcknowledged(int alertId);
    void alertResolved(int alertId);
    void notificationSent(const QString& type, const QString& recipient);

private:
    void setupUi();
    void setupAlertsTab();
    void setupRulesTab();
    void setupSettingsTab();
    void startMonitoring();
    void checkAlerts();
    void addAlertToTable(const Alert& alert);
    void showNotification(const Alert& alert);
    void sendEmailNotification(const Alert& alert);
    void updateAlertCounts();
    void loadAlertRules();
    void saveAlertRules();
    void onAlertSelected(const QModelIndex& index);
    
    backend::SessionClient* client_;
    QTimer* checkTimer_ = nullptr;
    QList<Alert> alerts_;
    QList<AlertRule> rules_;
    NotificationSettings settings_;
    QSystemTrayIcon* trayIcon_ = nullptr;
    
    // UI Components
    QTabWidget* tabs_ = nullptr;
    
    // Alerts tab
    QTableView* alertsTable_ = nullptr;
    QStandardItemModel* alertsModel_ = nullptr;
    QLabel* activeCountLabel_ = nullptr;
    QLabel* warningCountLabel_ = nullptr;
    QLabel* criticalCountLabel_ = nullptr;
    
    // Rules tab
    QTableView* rulesTable_ = nullptr;
    QStandardItemModel* rulesModel_ = nullptr;
    
    // Alert details
    QTextEdit* alertDetailsEdit_ = nullptr;
    QPushButton* acknowledgeBtn_ = nullptr;
    QPushButton* resolveBtn_ = nullptr;
};

// ============================================================================
// Alert Rule Dialog
// ============================================================================
class AlertRuleDialog : public QDialog {
    Q_OBJECT

public:
    explicit AlertRuleDialog(AlertRule* rule, QWidget* parent = nullptr);

public slots:
    void onCategoryChanged(int index);
    void onTestCondition();
    void onSave();

private:
    void setupUi();
    void loadTemplates();
    
    AlertRule* rule_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QComboBox* severityCombo_ = nullptr;
    QTextEdit* conditionEdit_ = nullptr;
    QTextEdit* messageEdit_ = nullptr;
    QCheckBox* enabledCheck_ = nullptr;
    QSpinBox* cooldownSpin_ = nullptr;
    QCheckBox* emailCheck_ = nullptr;
    QCheckBox* desktopCheck_ = nullptr;
    QLineEdit* emailRecipientsEdit_ = nullptr;
};

// ============================================================================
// Notification Settings Dialog
// ============================================================================
class NotificationSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit NotificationSettingsDialog(NotificationSettings* settings, QWidget* parent = nullptr);

public slots:
    void onTestEmail();
    void onSave();
    void onReset();

private:
    void setupUi();
    void loadSettings();
    
    NotificationSettings* settings_;
    
    QCheckBox* desktopCheck_ = nullptr;
    QCheckBox* emailCheck_ = nullptr;
    QCheckBox* soundCheck_ = nullptr;
    QSpinBox* maxNotificationsSpin_ = nullptr;
    QSpinBox* retentionSpin_ = nullptr;
    
    // SMTP settings
    QLineEdit* smtpServerEdit_ = nullptr;
    QSpinBox* smtpPortSpin_ = nullptr;
    QLineEdit* smtpUserEdit_ = nullptr;
    QLineEdit* smtpPassEdit_ = nullptr;
    QLineEdit* fromEmailEdit_ = nullptr;
};

// ============================================================================
// Alert Details Dialog
// ============================================================================
class AlertDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit AlertDetailsDialog(const Alert& alert, QWidget* parent = nullptr);

public slots:
    void onAcknowledge();
    void onResolve();
    void onViewRelated();

private:
    void setupUi();
    
    Alert alert_;
    
    QLabel* severityLabel_ = nullptr;
    QLabel* categoryLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QTextEdit* messageEdit_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};

// ============================================================================
// Alert History Dialog
// ============================================================================
class AlertHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit AlertHistoryDialog(QWidget* parent = nullptr);

public slots:
    void onFilterByDate();
    void onExportHistory();
    void onClearOldAlerts();
    void onViewAlertDetails();

private:
    void setupUi();
    void loadHistory();
    
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* historyModel_ = nullptr;
    QDateTimeEdit* fromDateEdit_ = nullptr;
    QDateTimeEdit* toDateEdit_ = nullptr;
};

} // namespace scratchrobin::ui
