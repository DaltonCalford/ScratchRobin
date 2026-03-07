#include "audit_log_viewer.h"
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
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTimeEdit>
#include <QProgressBar>
#include <QRandomGenerator>

namespace scratchrobin::ui {

// ============================================================================
// Audit Log Viewer Panel
// ============================================================================

AuditLogViewerPanel::AuditLogViewerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("audit_log_viewer", parent)
    , client_(client) {
    setupUi();
    loadLogEntries();
}

void AuditLogViewerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Filter bar
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
    
    severityCombo_ = new QComboBox(this);
    severityCombo_->addItem(tr("All Severities"));
    severityCombo_->addItem(tr("INFO"));
    severityCombo_->addItem(tr("WARNING"));
    severityCombo_->addItem(tr("ERROR"));
    severityCombo_->addItem(tr("CRITICAL"));
    filterLayout->addWidget(severityCombo_);
    
    actionTypeCombo_ = new QComboBox(this);
    actionTypeCombo_->addItem(tr("All Actions"));
    actionTypeCombo_->addItem(tr("SELECT"));
    actionTypeCombo_->addItem(tr("INSERT"));
    actionTypeCombo_->addItem(tr("UPDATE"));
    actionTypeCombo_->addItem(tr("DELETE"));
    actionTypeCombo_->addItem(tr("DDL"));
    actionTypeCombo_->addItem(tr("LOGIN"));
    filterLayout->addWidget(actionTypeCombo_);
    
    auto* filterBtn = new QPushButton(tr("Apply Filter"), this);
    connect(filterBtn, &QPushButton::clicked, this, &AuditLogViewerPanel::onApplyFilter);
    filterLayout->addWidget(filterBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &AuditLogViewerPanel::onClearFilters);
    filterLayout->addWidget(clearBtn);
    
    filterLayout->addStretch();
    
    mainLayout->addLayout(filterLayout);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &AuditLogViewerPanel::onRefreshLogs);
    toolbarLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &AuditLogViewerPanel::onExportLogs);
    toolbarLayout->addWidget(exportBtn);
    
    auto* archiveBtn = new QPushButton(tr("Archive"), this);
    connect(archiveBtn, &QPushButton::clicked, this, &AuditLogViewerPanel::onArchiveOldLogs);
    toolbarLayout->addWidget(archiveBtn);
    
    toolbarLayout->addStretch();
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search in logs..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &AuditLogViewerPanel::onSearch);
    toolbarLayout->addWidget(searchEdit_);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Log entries table
    logTable_ = new QTableView(this);
    logModel_ = new QStandardItemModel(this);
    logModel_->setHorizontalHeaderLabels({tr("Timestamp"), tr("User"), tr("Action"), tr("Object"), tr("Result")});
    logTable_->setModel(logModel_);
    logTable_->setAlternatingRowColors(true);
    logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(logTable_, &QTableView::clicked, this, &AuditLogViewerPanel::onEntrySelected);
    splitter->addWidget(logTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* infoGroup = new QGroupBox(tr("Entry Details"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    timestampLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Timestamp:"), timestampLabel_);
    
    userLabel_ = new QLabel(this);
    infoLayout->addRow(tr("User:"), userLabel_);
    
    databaseLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Database:"), databaseLabel_);
    
    actionLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Action:"), actionLabel_);
    
    objectLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Object:"), objectLabel_);
    
    resultLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Result:"), resultLabel_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Command text
    auto* cmdGroup = new QGroupBox(tr("Command"), this);
    auto* cmdLayout = new QVBoxLayout(cmdGroup);
    
    commandEdit_ = new QTextEdit(this);
    commandEdit_->setReadOnly(true);
    commandEdit_->setMaximumHeight(100);
    cmdLayout->addWidget(commandEdit_);
    
    detailsLayout->addWidget(cmdGroup);
    
    // Full details
    auto* fullGroup = new QGroupBox(tr("Full Details"), this);
    auto* fullLayout = new QVBoxLayout(fullGroup);
    
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    fullLayout->addWidget(detailsEdit_);
    
    detailsLayout->addWidget(fullGroup);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({400, 350});
    
    mainLayout->addWidget(splitter, 1);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusLayout->addWidget(statusLabel_);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
}

void AuditLogViewerPanel::loadLogEntries() {
    entries_.clear();
    
    // Simulate audit log entries
    QStringList actions = {"SELECT", "INSERT", "UPDATE", "DELETE", "CREATE TABLE", "DROP TABLE"};
    QStringList users = {"postgres", "admin", "app_user", "backup_user"};
    QStringList objects = {"customers", "orders", "products", "audit_log"};
    
    for (int i = 0; i < 50; ++i) {
        AuditLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime().addSecs(-i * 300);
        entry.userName = users[QRandomGenerator::global()->bounded(users.size())];
        entry.action = actions[QRandomGenerator::global()->bounded(actions.size())];
        entry.objectName = objects[QRandomGenerator::global()->bounded(objects.size())];
        entry.databaseName = "scratchbird";
        entry.result = QRandomGenerator::global()->bounded(10) < 8 ? "SUCCESS" : "FAILED";
        entry.severity = entry.result == "FAILED" ? Severity::ERROR : Severity::INFO;
        entry.commandText = QString("%1 FROM %2").arg(entry.action).arg(entry.objectName);
        entries_.append(entry);
    }
    
    updateLogTable();
    statusLabel_->setText(tr("%1 entries loaded").arg(entries_.size()));
}

void AuditLogViewerPanel::updateLogTable() {
    logModel_->clear();
    logModel_->setHorizontalHeaderLabels({tr("Timestamp"), tr("User"), tr("Action"), tr("Object"), tr("Result")});
    
    for (const auto& entry : entries_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        row << new QStandardItem(entry.userName);
        row << new QStandardItem(entry.action);
        row << new QStandardItem(entry.objectName);
        row << new QStandardItem(entry.result);
        logModel_->appendRow(row);
    }
    
    // Color code results
    for (int i = 0; i < logModel_->rowCount(); ++i) {
        auto* resultItem = logModel_->item(i, 4);
        if (resultItem->text() == "FAILED") {
            resultItem->setForeground(Qt::red);
        } else {
            resultItem->setForeground(Qt::darkGreen);
        }
    }
}

void AuditLogViewerPanel::updateEntryDetails(const AuditLogEntry& entry) {
    timestampLabel_->setText(entry.timestamp.toString());
    userLabel_->setText(entry.userName);
    databaseLabel_->setText(entry.databaseName);
    actionLabel_->setText(entry.action);
    objectLabel_->setText(entry.objectName);
    resultLabel_->setText(entry.result);
    
    if (entry.result == "FAILED") {
        resultLabel_->setStyleSheet("color: red;");
    } else {
        resultLabel_->setStyleSheet("color: green;");
    }
    
    commandEdit_->setText(entry.commandText);
    
    QString details = tr("Session ID: %1\n").arg(entry.sessionId);
    details += tr("Client Address: %1\n").arg(entry.clientAddress);
    if (!entry.errorMessage.isEmpty()) {
        details += tr("\nError: %1").arg(entry.errorMessage);
    }
    detailsEdit_->setText(details);
}

void AuditLogViewerPanel::onRefreshLogs() {
    loadLogEntries();
}

void AuditLogViewerPanel::onApplyFilter() {
    loadLogEntries();
    
    // Apply filters
    QString severity = severityCombo_->currentText();
    QString actionType = actionTypeCombo_->currentText();
    
    QList<AuditLogEntry> filtered;
    for (const auto& entry : entries_) {
        bool match = true;
        
        if (severity != tr("All Severities") && entry.severity != severityFromString(severity)) {
            match = false;
        }
        
        if (actionType != tr("All Actions") && !entry.action.contains(actionType)) {
            match = false;
        }
        
        if (match) {
            filtered.append(entry);
        }
    }
    
    entries_ = filtered;
    updateLogTable();
    statusLabel_->setText(tr("%1 entries after filter").arg(entries_.size()));
}

void AuditLogViewerPanel::onClearFilters() {
    severityCombo_->setCurrentIndex(0);
    actionTypeCombo_->setCurrentIndex(0);
    searchEdit_->clear();
    loadLogEntries();
}

void AuditLogViewerPanel::onEntrySelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= entries_.size()) return;
    updateEntryDetails(entries_[index.row()]);
}

void AuditLogViewerPanel::onSearch(const QString& text) {
    for (int i = 0; i < logModel_->rowCount(); ++i) {
        bool match = false;
        for (int j = 0; j < logModel_->columnCount(); ++j) {
            auto* item = logModel_->item(i, j);
            if (item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        logTable_->setRowHidden(i, !match);
    }
}

void AuditLogViewerPanel::onExportLogs() {
    QMessageBox::information(this, tr("Export"),
        tr("Audit log export functionality would save to file."));
}

void AuditLogViewerPanel::onArchiveOldLogs() {
    auto reply = QMessageBox::question(this, tr("Archive Old Logs"),
        tr("Archive logs older than 30 days?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QMessageBox::information(this, tr("Archive"),
            tr("Old logs have been archived."));
    }
}

void AuditLogViewerPanel::onViewStatistics() {
    QMessageBox::information(this, tr("Statistics"),
        tr("Audit log statistics would display here."));
}

void AuditLogViewerPanel::onConfigureRetention() {
    QMessageBox::information(this, tr("Retention"),
        tr("Log retention policy configuration would display here."));
}

void AuditLogViewerPanel::onEnableDisable() {
    QMessageBox::information(this, tr("Audit Logging"),
        tr("Audit logging would be enabled/disabled."));
}

void AuditLogViewerPanel::onFilterChanged(int index) {
    Q_UNUSED(index)
    // Filter change handler
}

Severity AuditLogViewerPanel::severityFromString(const QString& str) {
    if (str == "INFO") return Severity::INFO;
    if (str == "WARNING") return Severity::WARNING;
    if (str == "ERROR") return Severity::ERROR;
    if (str == "CRITICAL") return Severity::CRITICAL;
    return Severity::INFO;
}

// ============================================================================
// Audit Filter Dialog
// ============================================================================

AuditFilterDialog::AuditFilterDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void AuditFilterDialog::setupUi() {
    setWindowTitle(tr("Filter Audit Log"));
    resize(400, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    formLayout->addRow(tr("From:"), fromDateEdit_);
    
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
    formLayout->addRow(tr("To:"), toDateEdit_);
    
    severityCombo_ = new QComboBox(this);
    severityCombo_->addItems({tr("All"), "INFO", "WARNING", "ERROR", "CRITICAL"});
    formLayout->addRow(tr("Severity:"), severityCombo_);
    
    userEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("User:"), userEdit_);
    
    actionCombo_ = new QComboBox(this);
    actionCombo_->addItems({tr("All"), "SELECT", "INSERT", "UPDATE", "DELETE", "DDL"});
    formLayout->addRow(tr("Action:"), actionCombo_);
    
    objectEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Object:"), objectEdit_);
    
    layout->addLayout(formLayout);
    
    // Presets
    auto* presetGroup = new QGroupBox(tr("Presets"), this);
    auto* presetLayout = new QVBoxLayout(presetGroup);
    
    auto* todayBtn = new QPushButton(tr("Today"), this);
    connect(todayBtn, &QPushButton::clicked, this, &AuditFilterDialog::onPresetToday);
    presetLayout->addWidget(todayBtn);
    
    auto* weekBtn = new QPushButton(tr("Last 7 Days"), this);
    connect(weekBtn, &QPushButton::clicked, this, &AuditFilterDialog::onPresetWeek);
    presetLayout->addWidget(weekBtn);
    
    auto* monthBtn = new QPushButton(tr("Last 30 Days"), this);
    connect(monthBtn, &QPushButton::clicked, this, &AuditFilterDialog::onPresetMonth);
    presetLayout->addWidget(monthBtn);
    
    layout->addWidget(presetGroup);
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void AuditFilterDialog::onPresetToday() {
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-1));
}

void AuditFilterDialog::onPresetWeek() {
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
}

void AuditFilterDialog::onPresetMonth() {
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-30));
}

void AuditFilterDialog::onSaveFilter() {
    accept();
}

// ============================================================================
// Audit Statistics Dialog
// ============================================================================

AuditStatisticsDialog::AuditStatisticsDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void AuditStatisticsDialog::setupUi() {
    setWindowTitle(tr("Audit Log Statistics"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Statistics summary
    auto* summaryGroup = new QGroupBox(tr("Summary"), this);
    auto* summaryLayout = new QFormLayout(summaryGroup);
    
    summaryLayout->addRow(tr("Total Entries:"), new QLabel("1,234", this));
    summaryLayout->addRow(tr("Date Range:"), new QLabel("2024-01-01 to 2024-12-31", this));
    summaryLayout->addRow(tr("Failed Actions:"), new QLabel("45", this));
    
    layout->addWidget(summaryGroup);
    
    // Action breakdown
    auto* actionGroup = new QGroupBox(tr("Action Breakdown"), this);
    auto* actionLayout = new QVBoxLayout(actionGroup);
    
    auto* actionTable = new QTableView(this);
    auto* actionModel = new QStandardItemModel(this);
    actionModel->setHorizontalHeaderLabels({tr("Action"), tr("Count"), tr("%")});
    actionModel->appendRow({new QStandardItem("SELECT"), new QStandardItem("800"), new QStandardItem("65%")});
    actionModel->appendRow({new QStandardItem("INSERT"), new QStandardItem("200"), new QStandardItem("16%")});
    actionModel->appendRow({new QStandardItem("UPDATE"), new QStandardItem("150"), new QStandardItem("12%")});
    actionModel->appendRow({new QStandardItem("DELETE"), new QStandardItem("50"), new QStandardItem("4%")});
    actionModel->appendRow({new QStandardItem("DDL"), new QStandardItem("34"), new QStandardItem("3%")});
    actionTable->setModel(actionModel);
    actionLayout->addWidget(actionTable);
    
    layout->addWidget(actionGroup);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* exportBtn = new QPushButton(tr("Export Report"), this);
    btnLayout->addWidget(exportBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

} // namespace scratchrobin::ui
