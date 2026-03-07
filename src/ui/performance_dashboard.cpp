#include "performance_dashboard.h"
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
#include <QProgressBar>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTimer>
#include <QRandomGenerator>

namespace scratchrobin::ui {

// ============================================================================
// Performance Dashboard Panel
// ============================================================================

PerformanceDashboardPanel::PerformanceDashboardPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("performance_dashboard", parent)
    , client_(client) {
    setupUi();
    
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &PerformanceDashboardPanel::updateMetrics);
}

PerformanceDashboardPanel::~PerformanceDashboardPanel() {
    if (refreshTimer_) {
        refreshTimer_->stop();
    }
}

void PerformanceDashboardPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &PerformanceDashboardPanel::onStartMonitoring);
    toolbarLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &PerformanceDashboardPanel::onStopMonitoring);
    toolbarLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    pauseBtn_->setEnabled(false);
    connect(pauseBtn_, &QPushButton::clicked, this, &PerformanceDashboardPanel::onPauseMonitoring);
    toolbarLayout->addWidget(pauseBtn_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &PerformanceDashboardPanel::onRefreshNow);
    toolbarLayout->addWidget(refreshBtn);
    
    toolbarLayout->addSpacing(20);
    
    auto* configBtn = new QPushButton(tr("Configure"), this);
    connect(configBtn, &QPushButton::clicked, this, &PerformanceDashboardPanel::onConfigureDashboard);
    toolbarLayout->addWidget(configBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &PerformanceDashboardPanel::onExportMetrics);
    toolbarLayout->addWidget(exportBtn);
    
    toolbarLayout->addStretch();
    
    statusLabel_ = new QLabel(tr("Not monitoring"), this);
    toolbarLayout->addWidget(statusLabel_);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main tabs
    tabs_ = new QTabWidget(this);
    
    setupOverviewTab();
    setupConnectionsTab();
    setupQueryStatsTab();
    setupCacheTab();
    setupIOTab();
    
    mainLayout->addWidget(tabs_, 1);
}

void PerformanceDashboardPanel::setupOverviewTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);
    
    // Key metrics cards
    auto* cardsLayout = new QHBoxLayout();
    
    auto createCard = [this](const QString& title, QLabel** valueLabel) -> QGroupBox* {
        auto* group = new QGroupBox(title, this);
        auto* layout = new QVBoxLayout(group);
        *valueLabel = new QLabel("-", this);
        (*valueLabel)->setAlignment(Qt::AlignCenter);
        (*valueLabel)->setStyleSheet("font-size: 24px; font-weight: bold;");
        layout->addWidget(*valueLabel);
        return group;
    };
    
    cardsLayout->addWidget(createCard(tr("QPS"), &qpsLabel_));
    cardsLayout->addWidget(createCard(tr("TPS"), &tpsLabel_));
    cardsLayout->addWidget(createCard(tr("Connections"), &connectionsLabel_));
    cardsLayout->addWidget(createCard(tr("Cache Hit %"), &cacheHitLabel_));
    cardsLayout->addWidget(createCard(tr("Active Queries"), &activeQueriesLabel_));
    cardsLayout->addWidget(createCard(tr("Slow Queries"), &slowQueriesLabel_));
    
    layout->addLayout(cardsLayout);
    
    // Resource bars
    auto* resourcesGroup = new QGroupBox(tr("Resource Utilization"), this);
    auto* resourcesLayout = new QFormLayout(resourcesGroup);
    
    cpuBar_ = new QProgressBar(this);
    cpuBar_->setRange(0, 100);
    resourcesLayout->addRow(tr("CPU:"), cpuBar_);
    
    memoryBar_ = new QProgressBar(this);
    memoryBar_->setRange(0, 100);
    resourcesLayout->addRow(tr("Memory:"), memoryBar_);
    
    diskBar_ = new QProgressBar(this);
    diskBar_->setRange(0, 100);
    resourcesLayout->addRow(tr("Disk I/O:"), diskBar_);
    
    layout->addWidget(resourcesGroup);
    
    // Charts placeholder
    auto* chartsGroup = new QGroupBox(tr("Trends"), this);
    auto* chartsLayout = new QHBoxLayout(chartsGroup);
    
    qpsChart_ = new QLabel(tr("QPS Chart"), this);
    qpsChart_->setMinimumHeight(150);
    qpsChart_->setStyleSheet("background: #f0f0f0; border: 1px solid #ccc;");
    qpsChart_->setAlignment(Qt::AlignCenter);
    chartsLayout->addWidget(qpsChart_);
    
    connChart_ = new QLabel(tr("Connections Chart"), this);
    connChart_->setMinimumHeight(150);
    connChart_->setStyleSheet("background: #f0f0f0; border: 1px solid #ccc;");
    connChart_->setAlignment(Qt::AlignCenter);
    chartsLayout->addWidget(connChart_);
    
    layout->addWidget(chartsGroup, 1);
    
    tabs_->addTab(widget, tr("Overview"));
}

void PerformanceDashboardPanel::setupConnectionsTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    connectionsTable_ = new QTableView(this);
    connectionsModel_ = new QStandardItemModel(this);
    connectionsModel_->setHorizontalHeaderLabels({tr("PID"), tr("User"), tr("Database"), 
                                                   tr("State"), tr("Query"), tr("Duration")});
    connectionsTable_->setModel(connectionsModel_);
    connectionsTable_->setAlternatingRowColors(true);
    
    layout->addWidget(connectionsTable_);
    tabs_->addTab(widget, tr("Connections"));
}

void PerformanceDashboardPanel::setupQueryStatsTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    queryStatsTable_ = new QTableView(this);
    queryStatsModel_ = new QStandardItemModel(this);
    queryStatsModel_->setHorizontalHeaderLabels({tr("Query Type"), tr("Count"), tr("Total Time"), 
                                                  tr("Avg Time"), tr("Min Time"), tr("Max Time")});
    queryStatsTable_->setModel(queryStatsModel_);
    queryStatsTable_->setAlternatingRowColors(true);
    
    layout->addWidget(queryStatsTable_);
    tabs_->addTab(widget, tr("Query Stats"));
}

void PerformanceDashboardPanel::setupCacheTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    cacheChart_ = new QLabel(tr("Cache Hit Ratio Chart"), this);
    cacheChart_->setMinimumHeight(200);
    cacheChart_->setStyleSheet("background: #f0f0f0; border: 1px solid #ccc;");
    cacheChart_->setAlignment(Qt::AlignCenter);
    layout->addWidget(cacheChart_);
    
    tabs_->addTab(widget, tr("Cache"));
}

void PerformanceDashboardPanel::setupIOTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    auto* ioLabel = new QLabel(tr("I/O Statistics"), this);
    layout->addWidget(ioLabel);
    
    tabs_->addTab(widget, tr("I/O"));
}

void PerformanceDashboardPanel::setupCharts() {
    // Chart setup would go here
}

void PerformanceDashboardPanel::updateMetrics() {
    // Simulate metrics updates
    static int counter = 0;
    counter++;
    
    // Update QPS
    double qps = 150.0 + QRandomGenerator::global()->bounded(50);
    qpsLabel_->setText(QString::number(qps, 'f', 1));
    
    // Update TPS
    double tps = 25.0 + QRandomGenerator::global()->bounded(10);
    tpsLabel_->setText(QString::number(tps, 'f', 1));
    
    // Update connections
    int connections = 12 + QRandomGenerator::global()->bounded(5);
    connectionsLabel_->setText(QString::number(connections));
    
    // Update cache hit
    double cacheHit = 95.0 + QRandomGenerator::global()->bounded(5);
    cacheHitLabel_->setText(QString::number(cacheHit, 'f', 1) + "%");
    
    // Update active queries
    int active = 3 + QRandomGenerator::global()->bounded(3);
    activeQueriesLabel_->setText(QString::number(active));
    
    // Update slow queries
    int slow = QRandomGenerator::global()->bounded(3);
    slowQueriesLabel_->setText(QString::number(slow));
    if (slow > 0) {
        slowQueriesLabel_->setStyleSheet("font-size: 24px; font-weight: bold; color: red;");
    } else {
        slowQueriesLabel_->setStyleSheet("font-size: 24px; font-weight: bold;");
    }
    
    // Update progress bars
    cpuBar_->setValue(30 + QRandomGenerator::global()->bounded(40));
    memoryBar_->setValue(50 + QRandomGenerator::global()->bounded(30));
    diskBar_->setValue(20 + QRandomGenerator::global()->bounded(20));
    
    emit metricsUpdated(metrics_);
}

void PerformanceDashboardPanel::updateOverviewCards() {
    // Update overview display
}

void PerformanceDashboardPanel::updateConnectionStats() {
    // Update connection table
}

void PerformanceDashboardPanel::updateQueryStats() {
    // Update query stats table
}

void PerformanceDashboardPanel::updateCacheStats() {
    // Update cache stats
}

void PerformanceDashboardPanel::updateIOStats() {
    // Update I/O stats
}

void PerformanceDashboardPanel::checkAlerts() {
    // Check for threshold violations
}

void PerformanceDashboardPanel::onStartMonitoring() {
    isMonitoring_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    pauseBtn_->setEnabled(true);
    statusLabel_->setText(tr("Monitoring..."));
    
    refreshTimer_->start(refreshInterval_ * 1000);
    updateMetrics();
}

void PerformanceDashboardPanel::onStopMonitoring() {
    isMonitoring_ = false;
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);
    statusLabel_->setText(tr("Not monitoring"));
    
    refreshTimer_->stop();
}

void PerformanceDashboardPanel::onPauseMonitoring() {
    isMonitoring_ = false;
    pauseBtn_->setEnabled(false);
    startBtn_->setEnabled(true);
    statusLabel_->setText(tr("Paused"));
    
    refreshTimer_->stop();
}

void PerformanceDashboardPanel::onRefreshNow() {
    updateMetrics();
}

void PerformanceDashboardPanel::onClearHistory() {
    // Clear metric history
}

void PerformanceDashboardPanel::onToggleMetric(int metricId, bool enabled) {
    Q_UNUSED(metricId)
    Q_UNUSED(enabled)
}

void PerformanceDashboardPanel::onSelectAllMetrics() {
    // Enable all metrics
}

void PerformanceDashboardPanel::onSelectNoMetrics() {
    // Disable all metrics
}

void PerformanceDashboardPanel::onExportMetrics() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Metrics"),
        QString(),
        tr("CSV (*.csv);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Metrics exported to %1").arg(fileName));
    }
}

void PerformanceDashboardPanel::onSaveSnapshot() {
    QMessageBox::information(this, tr("Snapshot"),
        tr("Current metrics saved as snapshot."));
}

void PerformanceDashboardPanel::onCompareSnapshots() {
    SnapshotComparisonDialog dialog(this);
    dialog.exec();
}

void PerformanceDashboardPanel::onConfigureDashboard() {
    MetricConfigDialog dialog(this);
    dialog.exec();
}

void PerformanceDashboardPanel::onSetRefreshInterval(int seconds) {
    refreshInterval_ = seconds;
    if (refreshTimer_->isActive()) {
        refreshTimer_->start(refreshInterval_ * 1000);
    }
}

// ============================================================================
// Metric Configuration Dialog
// ============================================================================

MetricConfigDialog::MetricConfigDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void MetricConfigDialog::setupUi() {
    setWindowTitle(tr("Dashboard Configuration"));
    resize(400, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* group = new QGroupBox(tr("Visible Metrics"), this);
    auto* groupLayout = new QVBoxLayout(group);
    
    showConnectionsCheck_ = new QCheckBox(tr("Connections"), this);
    showQPSCheck_ = new QCheckBox(tr("Queries Per Second"), this);
    showTPSCheck_ = new QCheckBox(tr("Transactions Per Second"), this);
    showCacheCheck_ = new QCheckBox(tr("Cache Hit Ratio"), this);
    showIOCheck_ = new QCheckBox(tr("I/O Statistics"), this);
    showLocksCheck_ = new QCheckBox(tr("Lock Waits"), this);
    
    showConnectionsCheck_->setChecked(true);
    showQPSCheck_->setChecked(true);
    showTPSCheck_->setChecked(true);
    showCacheCheck_->setChecked(true);
    showIOCheck_->setChecked(true);
    showLocksCheck_->setChecked(true);
    
    groupLayout->addWidget(showConnectionsCheck_);
    groupLayout->addWidget(showQPSCheck_);
    groupLayout->addWidget(showTPSCheck_);
    groupLayout->addWidget(showCacheCheck_);
    groupLayout->addWidget(showIOCheck_);
    groupLayout->addWidget(showLocksCheck_);
    
    layout->addWidget(group);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    
    historySizeSpin_ = new QSpinBox(this);
    historySizeSpin_->setRange(10, 1000);
    historySizeSpin_->setValue(100);
    optionsLayout->addRow(tr("History size:"), historySizeSpin_);
    
    alertThresholdSpin_ = new QSpinBox(this);
    alertThresholdSpin_->setRange(1, 60);
    alertThresholdSpin_->setValue(5);
    alertThresholdSpin_->setSuffix(tr(" seconds"));
    optionsLayout->addRow(tr("Alert threshold:"), alertThresholdSpin_);
    
    layout->addWidget(optionsGroup);
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &MetricConfigDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void MetricConfigDialog::onSave() {
    // Save configuration
    accept();
}

void MetricConfigDialog::onReset() {
    // Reset to defaults
}

// ============================================================================
// Snapshot Comparison Dialog
// ============================================================================

SnapshotComparisonDialog::SnapshotComparisonDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void SnapshotComparisonDialog::setupUi() {
    setWindowTitle(tr("Compare Snapshots"));
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // Snapshot selection
    auto* topLayout = new QHBoxLayout();
    
    auto* snapshot1Btn = new QPushButton(tr("Load Snapshot 1"), this);
    connect(snapshot1Btn, &QPushButton::clicked, this, &SnapshotComparisonDialog::onLoadSnapshot1);
    topLayout->addWidget(snapshot1Btn);
    
    auto* snapshot2Btn = new QPushButton(tr("Load Snapshot 2"), this);
    connect(snapshot2Btn, &QPushButton::clicked, this, &SnapshotComparisonDialog::onLoadSnapshot2);
    topLayout->addWidget(snapshot2Btn);
    
    topLayout->addStretch();
    layout->addLayout(topLayout);
    
    // Comparison table
    comparisonTable_ = new QTableView(this);
    comparisonModel_ = new QStandardItemModel(this);
    comparisonModel_->setHorizontalHeaderLabels({tr("Metric"), tr("Snapshot 1"), tr("Snapshot 2"), tr("Change")});
    comparisonTable_->setModel(comparisonModel_);
    comparisonTable_->setAlternatingRowColors(true);
    layout->addWidget(comparisonTable_, 1);
    
    // Summary
    summaryEdit_ = new QTextEdit(this);
    summaryEdit_->setMaximumHeight(100);
    summaryEdit_->setReadOnly(true);
    layout->addWidget(summaryEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SnapshotComparisonDialog::onExportComparison);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
    
    // Add sample data
    comparisonModel_->appendRow({
        new QStandardItem(tr("QPS")),
        new QStandardItem("150.5"),
        new QStandardItem("175.2"),
        new QStandardItem("+16.4%")
    });
}

void SnapshotComparisonDialog::compare() {
    // Compare two snapshots
}

void SnapshotComparisonDialog::onLoadSnapshot1() {
    QMessageBox::information(this, tr("Load"), tr("Load first snapshot"));
}

void SnapshotComparisonDialog::onLoadSnapshot2() {
    QMessageBox::information(this, tr("Load"), tr("Load second snapshot"));
}

void SnapshotComparisonDialog::onExportComparison() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Comparison"),
        QString(),
        tr("Text Files (*.txt);;CSV (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Comparison exported to %1").arg(fileName));
    }
}

} // namespace scratchrobin::ui
