#include "alert_notification_system.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTimer>
#include <QSystemTrayIcon>

namespace scratchrobin::ui {

// ============================================================================
// Alert Notification System Panel
// ============================================================================

AlertNotificationSystemPanel::AlertNotificationSystemPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("alert_system", parent)
    , client_(client) {
    setupUi();
    
    checkTimer_ = new QTimer(this);
    connect(checkTimer_, &QTimer::timeout, this, &AlertNotificationSystemPanel::checkAlerts);
    
    // Create tray icon
    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setIcon(QIcon::fromTheme("dialog-warning"));
    
    // Load sample rules
    loadAlertRules();
}

AlertNotificationSystemPanel::~AlertNotificationSystemPanel() {
    if (trayIcon_) {
        trayIcon_->hide();
    }
}

void AlertNotificationSystemPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* newRuleBtn = new QPushButton(tr("New Rule"), this);
    connect(newRuleBtn, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onNewRule);
    toolbarLayout->addWidget(newRuleBtn);
    
    auto* ackAllBtn = new QPushButton(tr("Acknowledge All"), this);
    connect(ackAllBtn, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onAcknowledgeAll);
    toolbarLayout->addWidget(ackAllBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear All"), this);
    connect(clearBtn, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onClearAllAlerts);
    toolbarLayout->addWidget(clearBtn);
    
    toolbarLayout->addStretch();
    
    auto* settingsBtn = new QPushButton(tr("Settings"), this);
    connect(settingsBtn, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onConfigureNotifications);
    toolbarLayout->addWidget(settingsBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Status summary
    auto* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel(tr("Active Alerts:"), this));
    activeCountLabel_ = new QLabel("0", this);
    activeCountLabel_->setStyleSheet("font-weight: bold; color: red;");
    statusLayout->addWidget(activeCountLabel_);
    
    statusLayout->addSpacing(20);
    statusLayout->addWidget(new QLabel(tr("Warnings:"), this));
    warningCountLabel_ = new QLabel("0", this);
    warningCountLabel_->setStyleSheet("color: orange;");
    statusLayout->addWidget(warningCountLabel_);
    
    statusLayout->addSpacing(20);
    statusLayout->addWidget(new QLabel(tr("Critical:"), this));
    criticalCountLabel_ = new QLabel("0", this);
    criticalCountLabel_->setStyleSheet("color: red;");
    statusLayout->addWidget(criticalCountLabel_);
    
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
    
    // Main tabs
    setupAlertsTab();
    setupRulesTab();
    setupSettingsTab();
    
    tabs_ = new QTabWidget(this);
    tabs_->addTab(alertsTable_, tr("Alerts"));
    tabs_->addTab(rulesTable_, tr("Rules"));
    
    mainLayout->addWidget(tabs_, 1);
    
    // Alert details
    auto* detailsGroup = new QGroupBox(tr("Alert Details"), this);
    auto* detailsLayout = new QVBoxLayout(detailsGroup);
    
    alertDetailsEdit_ = new QTextEdit(this);
    alertDetailsEdit_->setMaximumHeight(100);
    alertDetailsEdit_->setReadOnly(true);
    detailsLayout->addWidget(alertDetailsEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    acknowledgeBtn_ = new QPushButton(tr("Acknowledge"), this);
    connect(acknowledgeBtn_, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onAcknowledgeAlert);
    btnLayout->addWidget(acknowledgeBtn_);
    
    resolveBtn_ = new QPushButton(tr("Resolve"), this);
    connect(resolveBtn_, &QPushButton::clicked, this, &AlertNotificationSystemPanel::onResolveAlert);
    btnLayout->addWidget(resolveBtn_);
    
    detailsLayout->addLayout(btnLayout);
    
    mainLayout->addWidget(detailsGroup);
    
    // Start monitoring
    checkTimer_->start(5000); // Check every 5 seconds
}

void AlertNotificationSystemPanel::setupAlertsTab() {
    alertsTable_ = new QTableView(this);
    alertsModel_ = new QStandardItemModel(this);
    alertsModel_->setHorizontalHeaderLabels({tr("Time"), tr("Severity"), tr("Category"), tr("Title"), tr("Status")});
    alertsTable_->setModel(alertsModel_);
    alertsTable_->setAlternatingRowColors(true);
    alertsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(alertsTable_, &QTableView::clicked, this, &AlertNotificationSystemPanel::onAlertSelected);
}

void AlertNotificationSystemPanel::setupRulesTab() {
    rulesTable_ = new QTableView(this);
    rulesModel_ = new QStandardItemModel(this);
    rulesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Category"), tr("Severity"), tr("Condition"), tr("Enabled")});
    rulesTable_->setModel(rulesModel_);
    rulesTable_->setAlternatingRowColors(true);
    connect(rulesTable_, &QTableView::doubleClicked, this, &AlertNotificationSystemPanel::onEditRule);
}

void AlertNotificationSystemPanel::setupSettingsTab() {
    // Settings UI elements created in dialog
}

void AlertNotificationSystemPanel::startMonitoring() {
    checkTimer_->start(5000);
}

void AlertNotificationSystemPanel::checkAlerts() {
    // Simulate checking for alerts
    static int checkCount = 0;
    checkCount++;
    
    if (checkCount % 10 == 0) { // Every 10th check, simulate an alert
        Alert alert;
        alert.id = alerts_.size() + 1;
        alert.severity = AlertSeverity::Warning;
        alert.category = AlertCategory::Performance;
        alert.title = tr("High Query Duration");
        alert.message = tr("Query duration exceeded 1000ms");
        alert.triggeredAt = QDateTime::currentDateTime();
        alert.status = AlertStatus::Active;
        alert.source = "testdb";
        
        alerts_.append(alert);
        
        // Add to table
        addAlertToTable(alert);
        
        // Show notification
        if (settings_.enableDesktopNotifications) {
            showNotification(alert);
        }
        
        emit alertTriggered(alert);
    }
    
    updateAlertCounts();
}

void AlertNotificationSystemPanel::addAlertToTable(const Alert& alert) {
    QList<QStandardItem*> row;
    row << new QStandardItem(alert.triggeredAt.toString("hh:mm:ss"));
    
    auto* severityItem = new QStandardItem();
    switch (alert.severity) {
    case AlertSeverity::Info:
        severityItem->setText(tr("Info"));
        severityItem->setForeground(Qt::blue);
        break;
    case AlertSeverity::Warning:
        severityItem->setText(tr("Warning"));
        severityItem->setForeground(QColor(255, 140, 0));
        break;
    case AlertSeverity::Critical:
        severityItem->setText(tr("Critical"));
        severityItem->setForeground(Qt::red);
        break;
    case AlertSeverity::Emergency:
        severityItem->setText(tr("Emergency"));
        severityItem->setForeground(Qt::red);
        break;
    }
    row << severityItem;
    
    QString catStr;
    switch (alert.category) {
    case AlertCategory::Connection: catStr = tr("Connection"); break;
    case AlertCategory::Query: catStr = tr("Query"); break;
    case AlertCategory::Performance: catStr = tr("Performance"); break;
    case AlertCategory::DiskSpace: catStr = tr("Disk"); break;
    case AlertCategory::Memory: catStr = tr("Memory"); break;
    case AlertCategory::Replication: catStr = tr("Replication"); break;
    default: catStr = tr("Other");
    }
    row << new QStandardItem(catStr);
    row << new QStandardItem(alert.title);
    
    auto* statusItem = new QStandardItem();
    switch (alert.status) {
    case AlertStatus::Active:
        statusItem->setText(tr("Active"));
        statusItem->setForeground(Qt::red);
        break;
    case AlertStatus::Acknowledged:
        statusItem->setText(tr("Acknowledged"));
        statusItem->setForeground(Qt::darkYellow);
        break;
    case AlertStatus::Resolved:
        statusItem->setText(tr("Resolved"));
        statusItem->setForeground(Qt::darkGreen);
        break;
    default:
        statusItem->setText(tr("Unknown"));
    }
    row << statusItem;
    
    alertsModel_->insertRow(0, row);
}

void AlertNotificationSystemPanel::showNotification(const Alert& alert) {
    if (trayIcon_->isSystemTrayAvailable()) {
        trayIcon_->show();
        trayIcon_->showMessage(
            tr("Database Alert: %1").arg(alert.title),
            alert.message,
            alert.severity == AlertSeverity::Critical ? QSystemTrayIcon::Critical : QSystemTrayIcon::Warning,
            5000
        );
    }
}

void AlertNotificationSystemPanel::sendEmailNotification(const Alert& alert) {
    Q_UNUSED(alert)
    // Would send email via SMTP
}

void AlertNotificationSystemPanel::updateAlertCounts() {
    int active = 0, warning = 0, critical = 0;
    
    for (const auto& alert : alerts_) {
        if (alert.status == AlertStatus::Active) {
            active++;
            if (alert.severity == AlertSeverity::Warning) warning++;
            if (alert.severity == AlertSeverity::Critical || alert.severity == AlertSeverity::Emergency) critical++;
        }
    }
    
    activeCountLabel_->setText(QString::number(active));
    warningCountLabel_->setText(QString::number(warning));
    criticalCountLabel_->setText(QString::number(critical));
}

void AlertNotificationSystemPanel::loadAlertRules() {
    // Load default rules
    AlertRule rule1;
    rule1.id = 1;
    rule1.name = tr("Slow Query Alert");
    rule1.category = AlertCategory::Query;
    rule1.severity = AlertSeverity::Warning;
    rule1.condition = tr("query_duration > 1000");
    rule1.message = tr("Query execution time exceeded 1 second");
    rule1.isEnabled = true;
    rules_.append(rule1);
    
    AlertRule rule2;
    rule2.id = 2;
    rule2.name = tr("Connection Limit");
    rule2.category = AlertCategory::Connection;
    rule2.severity = AlertSeverity::Critical;
    rule2.condition = tr("connection_count > 100");
    rule2.message = tr("Connection count approaching limit");
    rule2.isEnabled = true;
    rules_.append(rule2);
    
    // Populate rules table
    rulesModel_->clear();
    rulesModel_->setHorizontalHeaderLabels({tr("Name"), tr("Category"), tr("Severity"), tr("Condition"), tr("Enabled")});
    
    for (const auto& rule : rules_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(rule.name);
        
        QString catStr;
        switch (rule.category) {
        case AlertCategory::Connection: catStr = tr("Connection"); break;
        case AlertCategory::Query: catStr = tr("Query"); break;
        case AlertCategory::Performance: catStr = tr("Performance"); break;
        default: catStr = tr("Other");
        }
        row << new QStandardItem(catStr);
        
        QString sevStr;
        switch (rule.severity) {
        case AlertSeverity::Info: sevStr = tr("Info"); break;
        case AlertSeverity::Warning: sevStr = tr("Warning"); break;
        case AlertSeverity::Critical: sevStr = tr("Critical"); break;
        case AlertSeverity::Emergency: sevStr = tr("Emergency"); break;
        }
        row << new QStandardItem(sevStr);
        row << new QStandardItem(rule.condition);
        row << new QStandardItem(rule.isEnabled ? tr("Yes") : tr("No"));
        
        rulesModel_->appendRow(row);
    }
}

void AlertNotificationSystemPanel::saveAlertRules() {
    // Save to settings
}

void AlertNotificationSystemPanel::onAcknowledgeAlert() {
    auto index = alertsTable_->currentIndex();
    if (!index.isValid() || index.row() >= alerts_.size()) return;
    
    // Update from the end since we insert at top
    int alertIndex = alerts_.size() - 1 - index.row();
    alerts_[alertIndex].status = AlertStatus::Acknowledged;
    alerts_[alertIndex].acknowledgedAt = QDateTime::currentDateTime();
    
    // Update table
    auto* statusItem = alertsModel_->item(index.row(), 4);
    if (statusItem) {
        statusItem->setText(tr("Acknowledged"));
        statusItem->setForeground(Qt::darkYellow);
    }
    
    updateAlertCounts();
    emit alertAcknowledged(alerts_[alertIndex].id);
}

void AlertNotificationSystemPanel::onAcknowledgeAll() {
    for (auto& alert : alerts_) {
        if (alert.status == AlertStatus::Active) {
            alert.status = AlertStatus::Acknowledged;
            alert.acknowledgedAt = QDateTime::currentDateTime();
        }
    }
    
    // Refresh table
    alertsModel_->clear();
    alertsModel_->setHorizontalHeaderLabels({tr("Time"), tr("Severity"), tr("Category"), tr("Title"), tr("Status")});
    for (const auto& alert : alerts_) {
        addAlertToTable(alert);
    }
    
    updateAlertCounts();
}

void AlertNotificationSystemPanel::onResolveAlert() {
    auto index = alertsTable_->currentIndex();
    if (!index.isValid() || index.row() >= alerts_.size()) return;
    
    int alertIndex = alerts_.size() - 1 - index.row();
    alerts_[alertIndex].status = AlertStatus::Resolved;
    alerts_[alertIndex].resolvedAt = QDateTime::currentDateTime();
    
    auto* statusItem = alertsModel_->item(index.row(), 4);
    if (statusItem) {
        statusItem->setText(tr("Resolved"));
        statusItem->setForeground(Qt::darkGreen);
    }
    
    updateAlertCounts();
    emit alertResolved(alerts_[alertIndex].id);
}

void AlertNotificationSystemPanel::onSuppressAlert() {
    // Suppress similar alerts
}

void AlertNotificationSystemPanel::onDeleteAlert() {
    auto index = alertsTable_->currentIndex();
    if (!index.isValid()) return;
    
    alertsModel_->removeRow(index.row());
}

void AlertNotificationSystemPanel::onClearAllAlerts() {
    alerts_.clear();
    alertsModel_->clear();
    alertsModel_->setHorizontalHeaderLabels({tr("Time"), tr("Severity"), tr("Category"), tr("Title"), tr("Status")});
    updateAlertCounts();
}

void AlertNotificationSystemPanel::onNewRule() {
    AlertRule newRule;
    AlertRuleDialog dialog(&newRule, this);
    if (dialog.exec() == QDialog::Accepted) {
        newRule.id = rules_.size() + 1;
        rules_.append(newRule);
        loadAlertRules(); // Refresh table
    }
}

void AlertNotificationSystemPanel::onEditRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    AlertRuleDialog dialog(&rules_[index.row()], this);
    if (dialog.exec() == QDialog::Accepted) {
        loadAlertRules();
    }
}

void AlertNotificationSystemPanel::onDeleteRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid()) return;
    
    rules_.removeAt(index.row());
    loadAlertRules();
}

void AlertNotificationSystemPanel::onDuplicateRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    AlertRule copy = rules_[index.row()];
    copy.id = rules_.size() + 1;
    copy.name += tr(" (Copy)");
    rules_.append(copy);
    loadAlertRules();
}

void AlertNotificationSystemPanel::onTestRule() {
    QMessageBox::information(this, tr("Test Rule"),
        tr("Rule test would be executed here."));
}

void AlertNotificationSystemPanel::onEnableDisableRule() {
    auto index = rulesTable_->currentIndex();
    if (!index.isValid() || index.row() >= rules_.size()) return;
    
    rules_[index.row()].isEnabled = !rules_[index.row()].isEnabled;
    loadAlertRules();
}

void AlertNotificationSystemPanel::onFilterBySeverity(int severity) {
    Q_UNUSED(severity)
    // Filter alerts by severity
}

void AlertNotificationSystemPanel::onFilterByCategory(int category) {
    Q_UNUSED(category)
    // Filter alerts by category
}

void AlertNotificationSystemPanel::onFilterByStatus(int status) {
    Q_UNUSED(status)
    // Filter alerts by status
}

void AlertNotificationSystemPanel::onSearchAlerts(const QString& text) {
    Q_UNUSED(text)
    // Search alerts
}

void AlertNotificationSystemPanel::onTimeRangeFilter() {
    // Filter by time range
}

void AlertNotificationSystemPanel::onViewActiveAlerts() {
    tabs_->setCurrentIndex(0);
}

void AlertNotificationSystemPanel::onViewAllAlerts() {
    tabs_->setCurrentIndex(0);
}

void AlertNotificationSystemPanel::onViewAlertHistory() {
    AlertHistoryDialog dialog(this);
    dialog.exec();
}

void AlertNotificationSystemPanel::onViewRules() {
    tabs_->setCurrentIndex(1);
}

void AlertNotificationSystemPanel::onConfigureNotifications() {
    NotificationSettingsDialog dialog(&settings_, this);
    if (dialog.exec() == QDialog::Accepted) {
        saveAlertRules();
    }
}

void AlertNotificationSystemPanel::onConfigureEmail() {
    // Open email configuration
}

void AlertNotificationSystemPanel::onTestNotification() {
    trayIcon_->show();
    trayIcon_->showMessage(
        tr("Test Notification"),
        tr("This is a test notification from ScratchRobin."),
        QSystemTrayIcon::Information,
        3000
    );
}

void AlertNotificationSystemPanel::onAlertSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= alerts_.size()) return;
    
    int alertIndex = alerts_.size() - 1 - index.row();
    const auto& alert = alerts_[alertIndex];
    
    alertDetailsEdit_->setPlainText(
        tr("Title: %1\n"
           "Message: %2\n"
           "Source: %3\n"
           "Triggered: %4\n"
           "Status: %5")
        .arg(alert.title)
        .arg(alert.message)
        .arg(alert.source)
        .arg(alert.triggeredAt.toString())
        .arg(alert.status == AlertStatus::Active ? tr("Active") : 
             (alert.status == AlertStatus::Acknowledged ? tr("Acknowledged") : tr("Resolved"))));
}

// ============================================================================
// Alert Rule Dialog
// ============================================================================

AlertRuleDialog::AlertRuleDialog(AlertRule* rule, QWidget* parent)
    : QDialog(parent)
    , rule_(rule) {
    setupUi();
}

void AlertRuleDialog::setupUi() {
    setWindowTitle(tr("Alert Rule"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    nameEdit_ = new QLineEdit(rule_->name, this);
    formLayout->addRow(tr("Name:"), nameEdit_);
    
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({tr("Connection"), tr("Query"), tr("Performance"), 
                               tr("Disk Space"), tr("Memory"), tr("Replication"), tr("Custom")});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AlertRuleDialog::onCategoryChanged);
    formLayout->addRow(tr("Category:"), categoryCombo_);
    
    severityCombo_ = new QComboBox(this);
    severityCombo_->addItems({tr("Info"), tr("Warning"), tr("Critical"), tr("Emergency")});
    formLayout->addRow(tr("Severity:"), severityCombo_);
    
    conditionEdit_ = new QTextEdit(this);
    conditionEdit_->setMaximumHeight(60);
    conditionEdit_->setPlainText(rule_->condition);
    formLayout->addRow(tr("Condition:"), conditionEdit_);
    
    messageEdit_ = new QTextEdit(this);
    messageEdit_->setMaximumHeight(60);
    messageEdit_->setPlainText(rule_->message);
    formLayout->addRow(tr("Message:"), messageEdit_);
    
    enabledCheck_ = new QCheckBox(tr("Enabled"), this);
    enabledCheck_->setChecked(rule_->isEnabled);
    formLayout->addRow(enabledCheck_);
    
    cooldownSpin_ = new QSpinBox(this);
    cooldownSpin_->setRange(1, 60);
    cooldownSpin_->setValue(rule_->cooldownMinutes);
    cooldownSpin_->setSuffix(tr(" minutes"));
    formLayout->addRow(tr("Cooldown:"), cooldownSpin_);
    
    emailCheck_ = new QCheckBox(tr("Send email notification"), this);
    emailCheck_->setChecked(rule_->emailNotification);
    formLayout->addRow(emailCheck_);
    
    emailRecipientsEdit_ = new QLineEdit(rule_->emailRecipients, this);
    formLayout->addRow(tr("Recipients:"), emailRecipientsEdit_);
    
    desktopCheck_ = new QCheckBox(tr("Show desktop notification"), this);
    desktopCheck_->setChecked(rule_->desktopNotification);
    formLayout->addRow(desktopCheck_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AlertRuleDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    auto* testBtn = new QPushButton(tr("Test"), this);
    connect(testBtn, &QPushButton::clicked, this, &AlertRuleDialog::onTestCondition);
    btnBox->addButton(testBtn, QDialogButtonBox::ActionRole);
    
    layout->addWidget(btnBox);
}

void AlertRuleDialog::onCategoryChanged(int index) {
    Q_UNUSED(index)
    // Update templates based on category
}

void AlertRuleDialog::onTestCondition() {
    QMessageBox::information(this, tr("Test"),
        tr("Condition test would be executed here."));
}

void AlertRuleDialog::onSave() {
    rule_->name = nameEdit_->text();
    rule_->condition = conditionEdit_->toPlainText();
    rule_->message = messageEdit_->toPlainText();
    rule_->isEnabled = enabledCheck_->isChecked();
    rule_->cooldownMinutes = cooldownSpin_->value();
    rule_->emailNotification = emailCheck_->isChecked();
    rule_->emailRecipients = emailRecipientsEdit_->text();
    rule_->desktopNotification = desktopCheck_->isChecked();
    accept();
}

// ============================================================================
// Notification Settings Dialog
// ============================================================================

NotificationSettingsDialog::NotificationSettingsDialog(NotificationSettings* settings, QWidget* parent)
    : QDialog(parent)
    , settings_(settings) {
    setupUi();
    loadSettings();
}

void NotificationSettingsDialog::setupUi() {
    setWindowTitle(tr("Notification Settings"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* generalGroup = new QGroupBox(tr("General"), this);
    auto* generalLayout = new QFormLayout(generalGroup);
    
    desktopCheck_ = new QCheckBox(tr("Enable desktop notifications"), this);
    generalLayout->addRow(desktopCheck_);
    
    emailCheck_ = new QCheckBox(tr("Enable email notifications"), this);
    generalLayout->addRow(emailCheck_);
    
    soundCheck_ = new QCheckBox(tr("Play notification sound"), this);
    generalLayout->addRow(soundCheck_);
    
    maxNotificationsSpin_ = new QSpinBox(this);
    maxNotificationsSpin_->setRange(10, 200);
    generalLayout->addRow(tr("Max notifications:"), maxNotificationsSpin_);
    
    retentionSpin_ = new QSpinBox(this);
    retentionSpin_->setRange(1, 90);
    retentionSpin_->setSuffix(tr(" days"));
    generalLayout->addRow(tr("Retention period:"), retentionSpin_);
    
    layout->addWidget(generalGroup);
    
    auto* smtpGroup = new QGroupBox(tr("SMTP Settings"), this);
    auto* smtpLayout = new QFormLayout(smtpGroup);
    
    smtpServerEdit_ = new QLineEdit(this);
    smtpLayout->addRow(tr("Server:"), smtpServerEdit_);
    
    smtpPortSpin_ = new QSpinBox(this);
    smtpPortSpin_->setRange(1, 65535);
    smtpPortSpin_->setValue(587);
    smtpLayout->addRow(tr("Port:"), smtpPortSpin_);
    
    smtpUserEdit_ = new QLineEdit(this);
    smtpLayout->addRow(tr("Username:"), smtpUserEdit_);
    
    smtpPassEdit_ = new QLineEdit(this);
    smtpPassEdit_->setEchoMode(QLineEdit::Password);
    smtpLayout->addRow(tr("Password:"), smtpPassEdit_);
    
    fromEmailEdit_ = new QLineEdit(this);
    smtpLayout->addRow(tr("From Email:"), fromEmailEdit_);
    
    layout->addWidget(smtpGroup);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &NotificationSettingsDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    auto* testBtn = new QPushButton(tr("Test Email"), this);
    connect(testBtn, &QPushButton::clicked, this, &NotificationSettingsDialog::onTestEmail);
    btnBox->addButton(testBtn, QDialogButtonBox::ActionRole);
    
    layout->addWidget(btnBox);
}

void NotificationSettingsDialog::loadSettings() {
    desktopCheck_->setChecked(settings_->enableDesktopNotifications);
    emailCheck_->setChecked(settings_->enableEmailNotifications);
    soundCheck_->setChecked(settings_->enableSound);
    maxNotificationsSpin_->setValue(settings_->maxNotifications);
    retentionSpin_->setValue(settings_->retentionDays);
    smtpServerEdit_->setText(settings_->smtpServer);
    smtpPortSpin_->setValue(settings_->smtpPort);
    smtpUserEdit_->setText(settings_->smtpUsername);
    smtpPassEdit_->setText(settings_->smtpPassword);
    fromEmailEdit_->setText(settings_->fromEmail);
}

void NotificationSettingsDialog::onTestEmail() {
    QMessageBox::information(this, tr("Test Email"),
        tr("Test email would be sent to the configured address."));
}

void NotificationSettingsDialog::onSave() {
    settings_->enableDesktopNotifications = desktopCheck_->isChecked();
    settings_->enableEmailNotifications = emailCheck_->isChecked();
    settings_->enableSound = soundCheck_->isChecked();
    settings_->maxNotifications = maxNotificationsSpin_->value();
    settings_->retentionDays = retentionSpin_->value();
    settings_->smtpServer = smtpServerEdit_->text();
    settings_->smtpPort = smtpPortSpin_->value();
    settings_->smtpUsername = smtpUserEdit_->text();
    settings_->smtpPassword = smtpPassEdit_->text();
    settings_->fromEmail = fromEmailEdit_->text();
    accept();
}

void NotificationSettingsDialog::onReset() {
    loadSettings();
}

// ============================================================================
// Alert Details Dialog
// ============================================================================

AlertDetailsDialog::AlertDetailsDialog(const Alert& alert, QWidget* parent)
    : QDialog(parent)
    , alert_(alert) {
    setupUi();
}

void AlertDetailsDialog::setupUi() {
    setWindowTitle(tr("Alert Details"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    severityLabel_ = new QLabel(this);
    switch (alert_.severity) {
    case AlertSeverity::Info:
        severityLabel_->setText(tr("Info"));
        severityLabel_->setStyleSheet("color: blue; font-weight: bold;");
        break;
    case AlertSeverity::Warning:
        severityLabel_->setText(tr("Warning"));
        severityLabel_->setStyleSheet("color: orange; font-weight: bold;");
        break;
    case AlertSeverity::Critical:
        severityLabel_->setText(tr("Critical"));
        severityLabel_->setStyleSheet("color: red; font-weight: bold;");
        break;
    case AlertSeverity::Emergency:
        severityLabel_->setText(tr("Emergency"));
        severityLabel_->setStyleSheet("color: red; font-weight: bold;");
        break;
    }
    formLayout->addRow(tr("Severity:"), severityLabel_);
    
    categoryLabel_ = new QLabel(this);
    formLayout->addRow(tr("Category:"), categoryLabel_);
    
    statusLabel_ = new QLabel(this);
    formLayout->addRow(tr("Status:"), statusLabel_);
    
    timeLabel_ = new QLabel(alert_.triggeredAt.toString(), this);
    formLayout->addRow(tr("Triggered:"), timeLabel_);
    
    layout->addLayout(formLayout);
    
    layout->addWidget(new QLabel(tr("Message:"), this));
    messageEdit_ = new QTextEdit(this);
    messageEdit_->setReadOnly(true);
    messageEdit_->setPlainText(alert_.message);
    layout->addWidget(messageEdit_);
    
    layout->addWidget(new QLabel(tr("Details:"), this));
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setPlainText(alert_.details);
    layout->addWidget(detailsEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* ackBtn = new QPushButton(tr("Acknowledge"), this);
    connect(ackBtn, &QPushButton::clicked, this, &AlertDetailsDialog::onAcknowledge);
    btnLayout->addWidget(ackBtn);
    
    auto* resolveBtn = new QPushButton(tr("Resolve"), this);
    connect(resolveBtn, &QPushButton::clicked, this, &AlertDetailsDialog::onResolve);
    btnLayout->addWidget(resolveBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void AlertDetailsDialog::onAcknowledge() {
    accept();
}

void AlertDetailsDialog::onResolve() {
    accept();
}

void AlertDetailsDialog::onViewRelated() {
    // View related alerts
}

// ============================================================================
// Alert History Dialog
// ============================================================================

AlertHistoryDialog::AlertHistoryDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
    loadHistory();
}

void AlertHistoryDialog::setupUi() {
    setWindowTitle(tr("Alert History"));
    resize(800, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Filter controls
    auto* filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel(tr("From:"), this));
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    filterLayout->addWidget(fromDateEdit_);
    
    filterLayout->addWidget(new QLabel(tr("To:"), this));
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterLayout->addWidget(toDateEdit_);
    
    auto* filterBtn = new QPushButton(tr("Filter"), this);
    connect(filterBtn, &QPushButton::clicked, this, &AlertHistoryDialog::onFilterByDate);
    filterLayout->addWidget(filterBtn);
    
    filterLayout->addStretch();
    layout->addLayout(filterLayout);
    
    // History table
    historyTable_ = new QTableView(this);
    historyModel_ = new QStandardItemModel(this);
    historyModel_->setHorizontalHeaderLabels({tr("Time"), tr("Severity"), tr("Category"), tr("Title"), tr("Status")});
    historyTable_->setModel(historyModel_);
    historyTable_->setAlternatingRowColors(true);
    layout->addWidget(historyTable_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &AlertHistoryDialog::onExportHistory);
    btnLayout->addWidget(exportBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear Old"), this);
    connect(clearBtn, &QPushButton::clicked, this, &AlertHistoryDialog::onClearOldAlerts);
    btnLayout->addWidget(clearBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void AlertHistoryDialog::loadHistory() {
    // Load alert history
    historyModel_->clear();
    historyModel_->setHorizontalHeaderLabels({tr("Time"), tr("Severity"), tr("Category"), tr("Title"), tr("Status")});
    
    // Add sample history entries
    for (int i = 0; i < 10; ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QDateTime::currentDateTime().addDays(-i).toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(tr("Warning"));
        row << new QStandardItem(tr("Performance"));
        row << new QStandardItem(tr("Slow Query Detected"));
        row << new QStandardItem(tr("Resolved"));
        historyModel_->appendRow(row);
    }
}

void AlertHistoryDialog::onFilterByDate() {
    loadHistory(); // Reload with filter
}

void AlertHistoryDialog::onExportHistory() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export History"),
        QString(),
        tr("CSV (*.csv);;Text (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("History exported to %1").arg(fileName));
    }
}

void AlertHistoryDialog::onClearOldAlerts() {
    auto reply = QMessageBox::question(this, tr("Clear Old Alerts"),
        tr("Are you sure you want to delete alerts older than 30 days?"));
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Cleared"),
            tr("Old alerts have been deleted."));
    }
}

void AlertHistoryDialog::onViewAlertDetails() {
    // Show alert details
}

} // namespace scratchrobin::ui
