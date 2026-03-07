#include "ui/monitoring_panels.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QTabWidget>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QProgressBar>
#include <QGroupBox>
#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>

namespace scratchrobin::ui {

// ============================================================================
// ConnectionMonitorPanel
// ============================================================================

ConnectionMonitorPanel::ConnectionMonitorPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
    loadConnections();
}

void ConnectionMonitorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    refreshBtn_ = new QPushButton(tr("&Refresh"), this);
    toolbarLayout->addWidget(refreshBtn_);
    
    toolbarLayout->addSpacing(16);
    
    toolbarLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterCombo_ = new QComboBox(this);
    filterCombo_->setEditable(true);
    filterCombo_->addItem(tr("All"));
    filterCombo_->addItem(tr("Active"));
    filterCombo_->addItem(tr("Idle"));
    filterCombo_->addItem(tr("Idle in Transaction"));
    toolbarLayout->addWidget(filterCombo_);
    
    toolbarLayout->addStretch();
    
    autoRefreshCheck_ = new QCheckBox(tr("Auto-refresh (5s)"), this);
    toolbarLayout->addWidget(autoRefreshCheck_);
    
    terminateBtn_ = new QPushButton(tr("&Terminate"), this);
    terminateBtn_->setEnabled(false);
    toolbarLayout->addWidget(terminateBtn_);
    
    auto* detailsBtn = new QPushButton(tr("&Details"), this);
    toolbarLayout->addWidget(detailsBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(8);
    table_->setHorizontalHeaderLabels({
        tr("PID"), tr("User"), tr("Database"), tr("Client"),
        tr("State"), tr("Query"), tr("Duration"), tr("Connected")
    });
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setAlternatingRowColors(true);
    mainLayout->addWidget(table_);
    
    // Status bar
    statusLabel_ = new QLabel(tr("Total: 0 | Active: 0 | Idle: 0"), this);
    mainLayout->addWidget(statusLabel_);
    
    // Connections
    connect(refreshBtn_, &QPushButton::clicked, this, &ConnectionMonitorPanel::onRefresh);
    connect(terminateBtn_, &QPushButton::clicked, this, &ConnectionMonitorPanel::onTerminateConnection);
    connect(detailsBtn, &QPushButton::clicked, this, &ConnectionMonitorPanel::onShowDetails);
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int) { onFilterChanged(filterCombo_->currentText()); });
    connect(autoRefreshCheck_, &QCheckBox::toggled, [this](bool checked) {
        setAutoRefresh(checked, 5000);
    });
    connect(table_, &QTableWidget::itemSelectionChanged, [this]() {
        terminateBtn_->setEnabled(!table_->selectedItems().isEmpty());
    });
    
    // Refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &ConnectionMonitorPanel::onRefresh);
}

void ConnectionMonitorPanel::refresh() {
    loadConnections();
}

void ConnectionMonitorPanel::setAutoRefresh(bool enabled, int intervalMs) {
    if (enabled) {
        refreshTimer_->start(intervalMs);
    } else {
        refreshTimer_->stop();
    }
}

void ConnectionMonitorPanel::onRefresh() {
    loadConnections();
}

void ConnectionMonitorPanel::loadConnections() {
    currentConnections_ = fetchConnections();
    populateTable(currentConnections_);
}

QList<DbConnectionInfo> ConnectionMonitorPanel::fetchConnections() {
    QList<DbConnectionInfo> connections;
    
    // Mock data - would query database in real implementation
    DbConnectionInfo conn1;
    conn1.id = 1;
    conn1.pid = 12345;
    conn1.username = "admin";
    conn1.database = "production";
    conn1.clientAddress = "192.168.1.100:54321";
    conn1.state = "active";
    conn1.currentQuery = "SELECT * FROM orders WHERE created_at > '2026-01-01'";
    conn1.queryDurationMs = 1500;
    conn1.connectedSince = QDateTime::currentDateTime().addSecs(-3600);
    conn1.applicationName = "ScratchRobin";
    connections.append(conn1);
    
    DbConnectionInfo conn2;
    conn2.id = 2;
    conn2.pid = 12346;
    conn2.username = "app_user";
    conn2.database = "production";
    conn2.clientAddress = "192.168.1.101:54322";
    conn2.state = "idle";
    conn2.connectedSince = QDateTime::currentDateTime().addSecs(-7200);
    conn2.applicationName = "WebApp";
    connections.append(conn2);
    
    DbConnectionInfo conn3;
    conn3.id = 3;
    conn3.pid = 12347;
    conn3.username = "reporting";
    conn3.database = "warehouse";
    conn3.clientAddress = "192.168.1.102:54323";
    conn3.state = "idle in transaction";
    conn3.queryDurationMs = 300000;  // 5 minutes - long running
    conn3.connectedSince = QDateTime::currentDateTime().addSecs(-18000);
    conn3.applicationName = "ReportGenerator";
    connections.append(conn3);
    
    return connections;
}

void ConnectionMonitorPanel::populateTable(const QList<DbConnectionInfo>& connections) {
    table_->setRowCount(0);
    
    int activeCount = 0;
    int idleCount = 0;
    
    for (const auto& conn : connections) {
        // Apply filter
        QString filter = filterCombo_->currentText().toLower();
        if (filter != "all") {
            if (!conn.state.toLower().contains(filter)) continue;
        }
        
        int row = table_->rowCount();
        table_->insertRow(row);
        
        table_->setItem(row, 0, new QTableWidgetItem(QString::number(conn.pid)));
        table_->setItem(row, 1, new QTableWidgetItem(conn.username));
        table_->setItem(row, 2, new QTableWidgetItem(conn.database));
        table_->setItem(row, 3, new QTableWidgetItem(conn.clientAddress));
        table_->setItem(row, 4, new QTableWidgetItem(conn.state));
        table_->setItem(row, 5, new QTableWidgetItem(conn.currentQuery.left(50)));
        table_->setItem(row, 6, new QTableWidgetItem(QString("%1 ms").arg(conn.queryDurationMs)));
        table_->setItem(row, 7, new QTableWidgetItem(conn.connectedSince.toString("hh:mm:ss")));
        
        // Color coding
        if (conn.state == "active") {
            table_->item(row, 4)->setBackground(QColor(200, 255, 200));
            activeCount++;
        } else if (conn.state == "idle in transaction") {
            table_->item(row, 4)->setBackground(QColor(255, 255, 200));
        } else {
            idleCount++;
        }
        
        // Highlight long-running queries
        if (conn.queryDurationMs > 60000) {
            table_->item(row, 6)->setBackground(QColor(255, 200, 200));
        }
        
        // Store connection ID
        table_->item(row, 0)->setData(Qt::UserRole, conn.id);
    }
    
    statusLabel_->setText(tr("Total: %1 | Active: %2 | Idle: %3")
                         .arg(connections.size())
                         .arg(activeCount)
                         .arg(idleCount));
}

void ConnectionMonitorPanel::onTerminateConnection() {
    auto items = table_->selectedItems();
    if (items.isEmpty()) return;
    
    int row = items.first()->row();
    int connId = table_->item(row, 0)->data(Qt::UserRole).toInt();
    int pid = table_->item(row, 0)->text().toInt();
    
    auto reply = QMessageBox::warning(this, tr("Terminate Connection"),
        tr("Are you sure you want to terminate connection %1 (PID: %2)?\n"
           "This will abort any running queries.")
        .arg(connId).arg(pid),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Would execute: SELECT pg_terminate_backend(pid)
        emit terminateRequested(connId);
        refresh();
    }
}

void ConnectionMonitorPanel::onFilterChanged(const QString& filter) {
    Q_UNUSED(filter)
    populateTable(currentConnections_);
}

void ConnectionMonitorPanel::onShowDetails() {
    auto items = table_->selectedItems();
    if (items.isEmpty()) return;
    
    int row = items.first()->row();
    int connId = table_->item(row, 0)->data(Qt::UserRole).toInt();
    
    for (const auto& conn : currentConnections_) {
        if (conn.id == connId) {
            emit connectionSelected(conn);
            break;
        }
    }
}

void ConnectionMonitorPanel::updateDisplay() {
    populateTable(currentConnections_);
}

// ============================================================================
// TransactionMonitorPanel
// ============================================================================

TransactionMonitorPanel::TransactionMonitorPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
}

void TransactionMonitorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("&Refresh"), this);
    toolbarLayout->addWidget(refreshBtn);
    
    toolbarLayout->addStretch();
    
    activeCountLabel_ = new QLabel(tr("Active: 0"), this);
    toolbarLayout->addWidget(activeCountLabel_);
    
    idleCountLabel_ = new QLabel(tr("Idle: 0"), this);
    toolbarLayout->addWidget(idleCountLabel_);
    
    longRunningLabel_ = new QLabel(tr("Long Running: 0"), this);
    toolbarLayout->addWidget(longRunningLabel_);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({
        tr("ID"), tr("PID"), tr("Database"), tr("Start Time"), 
        tr("Duration"), tr("Statements")
    });
    mainLayout->addWidget(table_);
    
    // Details
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(150);
    mainLayout->addWidget(detailsEdit_);
    
    connect(refreshBtn, &QPushButton::clicked, this, &TransactionMonitorPanel::onRefresh);
}

void TransactionMonitorPanel::refresh() {
    loadTransactions();
}

void TransactionMonitorPanel::loadTransactions() {
    // Mock transaction data
    table_->setRowCount(0);
    
    // Would fetch from database
    activeCountLabel_->setText(tr("Active: %1").arg(1));
    idleCountLabel_->setText(tr("Idle: %1").arg(2));
    longRunningLabel_->setText(tr("Long Running: %1").arg(0));
}

void TransactionMonitorPanel::onRefresh() {
    loadTransactions();
}

void TransactionMonitorPanel::onViewDetails() {
    // Show transaction details
}

void TransactionMonitorPanel::onRollbackTransaction() {
    // Rollback selected transaction
}

// ============================================================================
// LockMonitorPanel
// ============================================================================

LockMonitorPanel::LockMonitorPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
}

void LockMonitorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("&Refresh"), this);
    toolbarLayout->addWidget(refreshBtn);
    
    toolbarLayout->addStretch();
    
    blockedCountLabel_ = new QLabel(tr("Blocked: 0"), this);
    toolbarLayout->addWidget(blockedCountLabel_);
    
    deadlockLabel_ = new QLabel(tr("Deadlocks: 0"), this);
    toolbarLayout->addWidget(deadlockLabel_);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Splitter for table and graph
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Locks table
    table_ = new QTableWidget(this);
    table_->setColumnCount(8);
    table_->setHorizontalHeaderLabels({
        tr("Lock ID"), tr("PID"), tr("Mode"), tr("Type"),
        tr("Table"), tr("Status"), tr("Blocked By"), tr("Duration")
    });
    splitter->addWidget(table_);
    
    // Lock graph
    lockGraph_ = new QTreeWidget(this);
    lockGraph_->setHeaderLabel(tr("Lock Dependency Graph"));
    splitter->addWidget(lockGraph_);
    
    mainLayout->addWidget(splitter);
    
    connect(refreshBtn, &QPushButton::clicked, this, &LockMonitorPanel::onRefresh);
}

void LockMonitorPanel::refresh() {
    loadLocks();
}

void LockMonitorPanel::loadLocks() {
    currentLocks_ = fetchLocks();
    populateTable(currentLocks_);
    detectDeadlocks();
}

QList<LockInfo> LockMonitorPanel::fetchLocks() {
    QList<LockInfo> locks;
    
    // Mock lock data
    LockInfo lock1;
    lock1.lockId = 1;
    lock1.processId = 12345;
    lock1.lockMode = "EXCLUSIVE";
    lock1.lockType = "relation";
    lock1.tableName = "orders";
    lock1.status = "granted";
    locks.append(lock1);
    
    LockInfo lock2;
    lock2.lockId = 2;
    lock2.processId = 12346;
    lock2.lockMode = "SHARE";
    lock2.lockType = "relation";
    lock2.tableName = "orders";
    lock2.status = "waiting";
    lock2.blockedProcessId = 12345;
    lock2.isBlocking = true;
    locks.append(lock2);
    
    return locks;
}

void LockMonitorPanel::populateTable(const QList<LockInfo>& locks) {
    table_->setRowCount(0);
    
    int blockedCount = 0;
    
    for (const auto& lock : locks) {
        int row = table_->rowCount();
        table_->insertRow(row);
        
        table_->setItem(row, 0, new QTableWidgetItem(QString::number(lock.lockId)));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(lock.processId)));
        table_->setItem(row, 2, new QTableWidgetItem(lock.lockMode));
        table_->setItem(row, 3, new QTableWidgetItem(lock.lockType));
        table_->setItem(row, 4, new QTableWidgetItem(lock.tableName));
        table_->setItem(row, 5, new QTableWidgetItem(lock.status));
        table_->setItem(row, 6, new QTableWidgetItem(
            lock.blockedProcessId > 0 ? QString::number(lock.blockedProcessId) : "-"));
        
        if (lock.status == "waiting") {
            table_->item(row, 5)->setBackground(QColor(255, 200, 200));
            blockedCount++;
        }
    }
    
    blockedCountLabel_->setText(tr("Blocked: %1").arg(blockedCount));
}

void LockMonitorPanel::detectDeadlocks() {
    // Analyze lock graph for cycles
    deadlockLabel_->setText(tr("Deadlocks: %1").arg(0));
}

void LockMonitorPanel::onRefresh() {
    loadLocks();
}

void LockMonitorPanel::onKillBlockingProcess() {
    // Kill the blocking process
}

void LockMonitorPanel::onViewLockGraph() {
    // Show lock dependency graph
}

// ============================================================================
// PerformanceMonitorPanel
// ============================================================================

PerformanceMonitorPanel::PerformanceMonitorPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
    
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &PerformanceMonitorPanel::onRefresh);
}

void PerformanceMonitorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("&Refresh"), this);
    toolbarLayout->addWidget(refreshBtn);
    
    recordBtn_ = new QPushButton(tr("&Record"), this);
    recordBtn_->setCheckable(true);
    toolbarLayout->addWidget(recordBtn_);
    
    exportBtn_ = new QPushButton(tr("&Export..."), this);
    exportBtn_->setEnabled(false);
    toolbarLayout->addWidget(exportBtn_);
    
    toolbarLayout->addStretch();
    
    auto* alertsBtn = new QPushButton(tr("&Configure Alerts..."), this);
    toolbarLayout->addWidget(alertsBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Metrics grid
    auto* metricsGroup = new QGroupBox(tr("Current Metrics"), this);
    auto* metricsLayout = new QGridLayout(metricsGroup);
    
    int row = 0;
    auto addMetric = [&](const QString& label, QLabel*& valueLabel, int r, int c) {
        metricsLayout->addWidget(new QLabel(label + ":", this), r, c * 2);
        valueLabel = new QLabel("0", this);
        valueLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2196F3;");
        metricsLayout->addWidget(valueLabel, r, c * 2 + 1);
    };
    
    addMetric(tr("Queries/sec"), qpsLabel_, row, 0);
    addMetric(tr("Trans/sec"), tpsLabel_, row++, 1);
    addMetric(tr("Cache Hit %"), cacheHitLabel_, row, 0);
    addMetric(tr("Connections"), connectionsLabel_, row++, 1);
    addMetric(tr("Avg Query Time"), avgQueryTimeLabel_, row, 0);
    addMetric(tr("CPU %"), cpuLabel_, row++, 1);
    addMetric(tr("Memory"), memoryLabel_, row, 0);
    addMetric(tr("Disk I/O"), diskIOLabel_, row++, 1);
    
    mainLayout->addWidget(metricsGroup);
    
    // Charts placeholder
    mainLayout->addWidget(new QLabel(tr("Performance Charts (placeholder)"), this));
    
    // History table
    auto* historyGroup = new QGroupBox(tr("Metrics History"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);
    
    // Would add chart widget here
    historyLayout->addWidget(new QLabel(tr("History chart would be displayed here"), this));
    
    mainLayout->addWidget(historyGroup);
    
    // Connections
    connect(refreshBtn, &QPushButton::clicked, this, &PerformanceMonitorPanel::onRefresh);
    connect(recordBtn_, &QPushButton::toggled, this, &PerformanceMonitorPanel::onToggleRecording);
    connect(exportBtn_, &QPushButton::clicked, this, &PerformanceMonitorPanel::onExport);
    connect(alertsBtn, &QPushButton::clicked, this, &PerformanceMonitorPanel::onConfigureAlerts);
}

void PerformanceMonitorPanel::refresh() {
    collectMetrics();
}

void PerformanceMonitorPanel::setAutoRefresh(bool enabled, int intervalMs) {
    if (enabled) {
        refreshTimer_->start(intervalMs);
    } else {
        refreshTimer_->stop();
    }
}

void PerformanceMonitorPanel::startRecording() {
    isRecording_ = true;
    metricsHistory_.clear();
    recordBtn_->setChecked(true);
    recordBtn_->setText(tr("&Stop"));
    exportBtn_->setEnabled(true);
}

void PerformanceMonitorPanel::stopRecording() {
    isRecording_ = false;
    recordBtn_->setChecked(false);
    recordBtn_->setText(tr("&Record"));
    exportBtn_->setEnabled(!metricsHistory_.isEmpty());
}

void PerformanceMonitorPanel::exportData(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;
    
    QTextStream stream(&file);
    stream << "timestamp,qps,tps,cache_hit,connections,avg_query_time\n";
    
    for (const auto& m : metricsHistory_) {
        stream << m.timestamp.toString(Qt::ISODate) << ","
               << m.queriesPerSecond << ","
               << m.transactionsPerSecond << ","
               << m.cacheHitRatio << ","
               << m.activeConnections << ","
               << m.avgQueryTime << "\n";
    }
    
    file.close();
}

void PerformanceMonitorPanel::onRefresh() {
    collectMetrics();
}

void PerformanceMonitorPanel::onToggleRecording(bool checked) {
    if (checked) {
        startRecording();
    } else {
        stopRecording();
    }
}

void PerformanceMonitorPanel::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Metrics"),
        QString(), tr("CSV (*.csv)"));
    if (!fileName.isEmpty()) {
        exportData(fileName);
    }
}

void PerformanceMonitorPanel::onConfigureAlerts() {
    QMessageBox::information(this, tr("Alerts"), 
        tr("Alert configuration dialog would open here."));
}

void PerformanceMonitorPanel::updateCharts() {
    // Update chart displays
}

void PerformanceMonitorPanel::collectMetrics() {
    auto metrics = fetchMetrics();
    metrics.timestamp = QDateTime::currentDateTime();
    
    if (isRecording_) {
        metricsHistory_.append(metrics);
    }
    
    updateMetricDisplays();
    checkAlerts();
}

void PerformanceMonitorPanel::updateMetricDisplays() {
    if (metricsHistory_.isEmpty()) return;
    
    const auto& latest = metricsHistory_.last();
    
    qpsLabel_->setText(QString::number(latest.queriesPerSecond, 'f', 1));
    tpsLabel_->setText(QString::number(latest.transactionsPerSecond, 'f', 1));
    cacheHitLabel_->setText(QString::number(latest.cacheHitRatio, 'f', 1) + "%");
    connectionsLabel_->setText(QString::number(latest.activeConnections, 'f', 0));
    avgQueryTimeLabel_->setText(QString::number(latest.avgQueryTime, 'f', 2) + " ms");
    cpuLabel_->setText(QString::number(latest.cpuUsage, 'f', 1) + "%");
    memoryLabel_->setText(QString::number(latest.memoryUsage, 'f', 1) + " MB");
    diskIOLabel_->setText(QString::number(latest.diskIO, 'f', 1) + " MB/s");
}

void PerformanceMonitorPanel::checkAlerts() {
    if (metricsHistory_.isEmpty()) return;
    
    const auto& latest = metricsHistory_.last();
    
    if (latest.queriesPerSecond > qpsThreshold_) {
        emit alertTriggered(tr("High query rate: %1 QPS").arg(latest.queriesPerSecond), "warning");
    }
    if (latest.activeConnections > connectionThreshold_) {
        emit alertTriggered(tr("High connection count: %1").arg(latest.activeConnections), "warning");
    }
    if (latest.cacheHitRatio < cacheHitThreshold_) {
        emit alertTriggered(tr("Low cache hit ratio: %1%").arg(latest.cacheHitRatio), "critical");
    }
}

PerformanceMetrics PerformanceMonitorPanel::fetchMetrics() {
    PerformanceMetrics metrics;
    
    // Mock metrics - would query database statistics
    metrics.queriesPerSecond = 150.5 + (QRandomGenerator::global()->bounded(50));
    metrics.transactionsPerSecond = 25.0 + (QRandomGenerator::global()->bounded(10));
    metrics.cacheHitRatio = 95.0 + (QRandomGenerator::global()->bounded(4));
    metrics.activeConnections = 15;
    metrics.avgQueryTime = 5.5 + (QRandomGenerator::global()->bounded(10));
    metrics.cpuUsage = 25.0 + (QRandomGenerator::global()->bounded(20));
    metrics.memoryUsage = 512.0 + (QRandomGenerator::global()->bounded(100));
    metrics.diskIO = 5.0 + (QRandomGenerator::global()->bounded(5));
    
    return metrics;
}

// ============================================================================
// MonitoringDashboard
// ============================================================================

MonitoringDashboard::MonitoringDashboard(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setWindowTitle(tr("Database Monitoring Dashboard"));
    setMinimumSize(1000, 700);
    
    setupUi();
}

void MonitoringDashboard::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    connectionPanel_ = new ConnectionMonitorPanel(client_, this);
    tabWidget_->addTab(connectionPanel_, tr("&Connections"));
    
    transactionPanel_ = new TransactionMonitorPanel(client_, this);
    tabWidget_->addTab(transactionPanel_, tr("&Transactions"));
    
    lockPanel_ = new LockMonitorPanel(client_, this);
    tabWidget_->addTab(lockPanel_, tr("&Locks"));
    
    performancePanel_ = new PerformanceMonitorPanel(client_, this);
    tabWidget_->addTab(performancePanel_, tr("&Performance"));
    
    mainLayout->addWidget(tabWidget_);
    
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
    
    // Panel connections
    connect(connectionPanel_, &ConnectionMonitorPanel::connectionSelected,
            this, &MonitoringDashboard::onConnectionSelected);
    connect(lockPanel_, &LockMonitorPanel::lockConflictDetected,
            this, &MonitoringDashboard::onLockConflict);
    connect(performancePanel_, &PerformanceMonitorPanel::alertTriggered,
            this, &MonitoringDashboard::onPerformanceAlert);
}

void MonitoringDashboard::setActiveTab(int tabIndex) {
    tabWidget_->setCurrentIndex(tabIndex);
}

void MonitoringDashboard::onConnectionSelected(const DbConnectionInfo& info) {
    Q_UNUSED(info)
    // Handle connection selection
}

void MonitoringDashboard::onLockConflict(const LockInfo& blocked, const LockInfo& blocker) {
    Q_UNUSED(blocked)
    Q_UNUSED(blocker)
    // Handle lock conflict
}

void MonitoringDashboard::onPerformanceAlert(const QString& message, const QString& severity) {
    Q_UNUSED(severity)
    // Handle performance alert
    QMessageBox::warning(this, tr("Performance Alert"), message);
}

// ============================================================================
// QueryCancellationDialog
// ============================================================================

QueryCancellationDialog::QueryCancellationDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setWindowTitle(tr("Query Cancellation"));
    setMinimumSize(400, 200);
    
    setupUi();
}

void QueryCancellationDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    mainLayout->addWidget(new QLabel(tr("Query:"), this));
    queryLabel_ = new QLabel(this);
    queryLabel_->setWordWrap(true);
    queryLabel_->setStyleSheet("font-family: monospace; font-size: 12px;");
    mainLayout->addWidget(queryLabel_);
    
    mainLayout->addWidget(new QLabel(tr("Status:"), this));
    statusLabel_ = new QLabel(tr("Running..."), this);
    mainLayout->addWidget(statusLabel_);
    
    mainLayout->addWidget(new QLabel(tr("Elapsed Time:"), this));
    elapsedLabel_ = new QLabel(tr("0s"), this);
    mainLayout->addWidget(elapsedLabel_);
    
    auto* buttonLayout = new QHBoxLayout();
    
    cancelBtn_ = new QPushButton(tr("&Cancel Query"), this);
    buttonLayout->addWidget(cancelBtn_);
    
    forceBtn_ = new QPushButton(tr("&Force Cancel"), this);
    forceBtn_->setEnabled(false);
    buttonLayout->addWidget(forceBtn_);
    
    buttonLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(cancelBtn_, &QPushButton::clicked, this, &QueryCancellationDialog::onCancelQuery);
    connect(forceBtn_, &QPushButton::clicked, this, &QueryCancellationDialog::onForceCancel);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &QueryCancellationDialog::updateProgress);
}

void QueryCancellationDialog::monitorQuery(int queryId, const QString& queryText) {
    queryId_ = queryId;
    queryText_ = queryText;
    queryLabel_->setText(queryText.left(100) + (queryText.length() > 100 ? "..." : ""));
    startTime_ = QDateTime::currentDateTime();
    refreshTimer_->start(1000);
}

void QueryCancellationDialog::updateProgress() {
    qint64 elapsed = startTime_.secsTo(QDateTime::currentDateTime());
    elapsedLabel_->setText(QString("%1s").arg(elapsed));
}

void QueryCancellationDialog::onCancelQuery() {
    // Send cancel request
    statusLabel_->setText(tr("Cancelling..."));
    forceBtn_->setEnabled(true);
}

void QueryCancellationDialog::onForceCancel() {
    // Send force cancel (terminate backend)
    statusLabel_->setText(tr("Force cancelling..."));
}

void QueryCancellationDialog::onRefreshStatus() {
    // Check query status
    checkQueryStatus();
}

void QueryCancellationDialog::checkQueryStatus() {
    // Query the database for the status of the query
}

// ============================================================================
// DDLGenerationDialog
// ============================================================================

DDLGenerationDialog::DDLGenerationDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setWindowTitle(tr("Generate DDL"));
    setMinimumSize(600, 500);
    
    setupUi();
}

void DDLGenerationDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Options
    auto* optionsLayout = new QGridLayout();
    
    optionsLayout->addWidget(new QLabel(tr("Object Type:"), this), 0, 0);
    objectTypeCombo_ = new QComboBox(this);
    objectTypeCombo_->addItems({tr("Table"), tr("Index"), tr("View"), 
                                tr("Procedure"), tr("Function"), tr("Schema (Full)")});
    optionsLayout->addWidget(objectTypeCombo_, 0, 1);
    
    optionsLayout->addWidget(new QLabel(tr("Object Name:"), this), 1, 0);
    objectNameCombo_ = new QComboBox(this);
    objectNameCombo_->setEditable(true);
    optionsLayout->addWidget(objectNameCombo_, 1, 1);
    
    includeDropCheck_ = new QCheckBox(tr("Include DROP statement"), this);
    optionsLayout->addWidget(includeDropCheck_, 2, 0, 1, 2);
    
    includeConstraintsCheck_ = new QCheckBox(tr("Include constraints"), this);
    includeConstraintsCheck_->setChecked(true);
    optionsLayout->addWidget(includeConstraintsCheck_, 3, 0, 1, 2);
    
    includeIndexesCheck_ = new QCheckBox(tr("Include indexes"), this);
    includeIndexesCheck_->setChecked(true);
    optionsLayout->addWidget(includeIndexesCheck_, 4, 0, 1, 2);
    
    mainLayout->addLayout(optionsLayout);
    
    // Output
    mainLayout->addWidget(new QLabel(tr("Generated DDL:"), this));
    outputEdit_ = new QTextEdit(this);
    outputEdit_->setFont(QFont("Consolas", 10));
    outputEdit_->setReadOnly(true);
    mainLayout->addWidget(outputEdit_, 1);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    auto* generateBtn = new QPushButton(tr("&Generate"), this);
    buttonLayout->addWidget(generateBtn);
    
    buttonLayout->addStretch();
    
    auto* copyBtn = new QPushButton(tr("&Copy"), this);
    buttonLayout->addWidget(copyBtn);
    
    auto* saveBtn = new QPushButton(tr("&Save..."), this);
    buttonLayout->addWidget(saveBtn);
    
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(generateBtn, &QPushButton::clicked, this, &DDLGenerationDialog::onGenerate);
    connect(copyBtn, &QPushButton::clicked, this, &DDLGenerationDialog::onCopyToClipboard);
    connect(saveBtn, &QPushButton::clicked, this, &DDLGenerationDialog::onSaveToFile);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(objectTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DDLGenerationDialog::onObjectTypeChanged);
}

void DDLGenerationDialog::setObject(const QString& objectName, const QString& objectType) {
    objectNameCombo_->setCurrentText(objectName);
    int typeIndex = objectTypeCombo_->findText(objectType, Qt::MatchContains);
    if (typeIndex >= 0) {
        objectTypeCombo_->setCurrentIndex(typeIndex);
    }
    onGenerate();
}

void DDLGenerationDialog::onGenerate() {
    QString objectName = objectNameCombo_->currentText();
    QString objectType = objectTypeCombo_->currentText();
    
    QString ddl = generateDDL(objectName, objectType);
    outputEdit_->setPlainText(ddl);
}

QString DDLGenerationDialog::generateDDL(const QString& objectName, const QString& objectType) {
    if (objectType == tr("Table")) {
        return generateTableDDL(objectName);
    } else if (objectType == tr("Index")) {
        return generateIndexDDL(objectName);
    } else if (objectType == tr("View")) {
        return generateViewDDL(objectName);
    } else if (objectType == tr("Schema (Full)")) {
        return generateFullSchemaDDL();
    }
    return tr("-- DDL generation not implemented for this object type");
}

QString DDLGenerationDialog::generateTableDDL(const QString& tableName) {
    QString ddl;
    
    if (includeDropCheck_->isChecked()) {
        ddl += QString("DROP TABLE IF EXISTS %1;\n\n").arg(tableName);
    }
    
    ddl += QString("CREATE TABLE %1 (\n").arg(tableName);
    ddl += "    id INT PRIMARY KEY,\n";
    ddl += "    name VARCHAR(255),\n";
    ddl += "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n";
    ddl += ");\n";
    
    if (includeConstraintsCheck_->isChecked()) {
        ddl += QString("\n-- Constraints for %1\n").arg(tableName);
        ddl += QString("ALTER TABLE %1 ADD CONSTRAINT chk_name CHECK (name IS NOT NULL);\n").arg(tableName);
    }
    
    if (includeIndexesCheck_->isChecked()) {
        ddl += QString("\n-- Indexes for %1\n").arg(tableName);
        ddl += QString("CREATE INDEX idx_%1_name ON %1(name);\n").arg(tableName);
    }
    
    return ddl;
}

QString DDLGenerationDialog::generateIndexDDL(const QString& indexName) {
    return QString("-- Index: %1\n"
                   "-- Would generate CREATE INDEX statement here\n")
                   .arg(indexName);
}

QString DDLGenerationDialog::generateViewDDL(const QString& viewName) {
    return QString("-- View: %1\n"
                   "DROP VIEW IF EXISTS %1;\n\n"
                   "CREATE VIEW %1 AS\n"
                   "SELECT * FROM some_table;\n")
                   .arg(viewName);
}

QString DDLGenerationDialog::generateProcedureDDL(const QString& procName) {
    return QString("-- Procedure: %1\n").arg(procName);
}

QString DDLGenerationDialog::generateFullSchemaDDL() {
    return tr("-- Full schema DDL would be generated here\n"
              "-- This would include all tables, indexes, views, etc.\n");
}

void DDLGenerationDialog::onCopyToClipboard() {
    QApplication::clipboard()->setText(outputEdit_->toPlainText());
}

void DDLGenerationDialog::onSaveToFile() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save DDL"),
        QString(), tr("SQL Files (*.sql);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(outputEdit_->toPlainText().toUtf8());
        file.close();
        QMessageBox::information(this, tr("Saved"), tr("DDL saved to %1").arg(fileName));
    }
}

void DDLGenerationDialog::onObjectTypeChanged() {
    // Update available object names based on type
}

// ============================================================================
// SessionManager
// ============================================================================

SessionManager::SessionManager(QObject* parent)
    : QObject(parent) {
}

void SessionManager::addSession(backend::SessionClient* client, const QString& name) {
    sessions_[client] = name;
    if (!currentSession_) {
        currentSession_ = client;
    }
    emit sessionAdded(client);
}

void SessionManager::removeSession(backend::SessionClient* client) {
    sessions_.remove(client);
    if (currentSession_ == client) {
        currentSession_ = sessions_.isEmpty() ? nullptr : sessions_.keys().first();
        emit currentSessionChanged(currentSession_);
    }
    emit sessionRemoved(client);
}

backend::SessionClient* SessionManager::currentSession() const {
    return currentSession_;
}

void SessionManager::setCurrentSession(backend::SessionClient* client) {
    if (sessions_.contains(client) && currentSession_ != client) {
        currentSession_ = client;
        emit currentSessionChanged(client);
    }
}

QList<backend::SessionClient*> SessionManager::sessions() const {
    return sessions_.keys();
}

QString SessionManager::sessionName(backend::SessionClient* client) const {
    return sessions_.value(client);
}

} // namespace scratchrobin::ui
