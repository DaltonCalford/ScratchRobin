#include "ui/connection_pool_monitor.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QToolBar>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QSplitter>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Connection Pool Monitor Panel
// ============================================================================
ConnectionPoolMonitorPanel::ConnectionPoolMonitorPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("connection_pool_monitor", parent)
    , client_(client)
    , refreshTimer_(new QTimer(this))
{
    setupUi();
    setupModel();
    refresh();
}

ConnectionPoolMonitorPanel::~ConnectionPoolMonitorPanel()
{
    stopAutoRefresh();
}

void ConnectionPoolMonitorPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* resetBtn = new QPushButton(tr("Reset Stats"), this);
    auto* configureBtn = new QPushButton(tr("Configure"), this);
    auto* disconnectBtn = new QPushButton(tr("Disconnect"), this);
    
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(resetBtn);
    toolbar->addWidget(configureBtn);
    toolbar->addWidget(disconnectBtn);
    
    connect(refreshBtn, &QPushButton::clicked, this, &ConnectionPoolMonitorPanel::refresh);
    connect(resetBtn, &QPushButton::clicked, this, &ConnectionPoolMonitorPanel::onResetStatistics);
    connect(configureBtn, &QPushButton::clicked, this, &ConnectionPoolMonitorPanel::onConfigurePool);
    
    mainLayout->addWidget(toolbar);
    
    // Auto refresh
    auto* refreshLayout = new QHBoxLayout();
    auto* autoRefreshCheck = new QCheckBox(tr("Auto Refresh"), this);
    auto* refreshInterval = new QSpinBox(this);
    refreshInterval->setRange(1, 60);
    refreshInterval->setValue(5);
    refreshInterval->setSuffix(" sec");
    
    refreshLayout->addWidget(autoRefreshCheck);
    refreshLayout->addWidget(refreshInterval);
    refreshLayout->addStretch();
    
    connect(autoRefreshCheck, &QCheckBox::toggled, this, &ConnectionPoolMonitorPanel::onAutoRefreshToggled);
    connect(refreshInterval, &QSpinBox::valueChanged, this, &ConnectionPoolMonitorPanel::onRefreshIntervalChanged);
    connect(refreshTimer_, &QTimer::timeout, this, &ConnectionPoolMonitorPanel::refresh);
    
    mainLayout->addLayout(refreshLayout);
    
    // Statistics panel
    auto* statsGroup = new QGroupBox(tr("Pool Statistics"), this);
    auto* statsLayout = new QGridLayout(statsGroup);
    
    totalLabel_ = new QLabel(tr("Total Connections: 0"), this);
    activeLabel_ = new QLabel(tr("Active: 0"), this);
    idleLabel_ = new QLabel(tr("Idle: 0"), this);
    waitingLabel_ = new QLabel(tr("Waiting: 0"), this);
    maxLabel_ = new QLabel(tr("Max: 0"), this);
    
    usageBar_ = new QProgressBar(this);
    usageBar_->setRange(0, 100);
    usageBar_->setValue(0);
    usageBar_->setTextVisible(true);
    
    statsLayout->addWidget(totalLabel_, 0, 0);
    statsLayout->addWidget(activeLabel_, 0, 1);
    statsLayout->addWidget(idleLabel_, 0, 2);
    statsLayout->addWidget(waitingLabel_, 0, 3);
    statsLayout->addWidget(maxLabel_, 0, 4);
    statsLayout->addWidget(usageBar_, 1, 0, 1, 5);
    
    mainLayout->addWidget(statsGroup);
    
    // Filter
    auto* filterLayout = new QHBoxLayout();
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter connections..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ConnectionPoolMonitorPanel::onFilterChanged);
    filterLayout->addWidget(filterEdit_);
    mainLayout->addLayout(filterLayout);
    
    // Connections table
    connectionTable_ = new QTableView(this);
    connectionTable_->setAlternatingRowColors(true);
    connectionTable_->setSortingEnabled(true);
    
    mainLayout->addWidget(connectionTable_);
}

void ConnectionPoolMonitorPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(8);
    model_->setHorizontalHeaderLabels({
        tr("PID"), tr("Application"), tr("Client Host"), tr("Connected"), 
        tr("Last Activity"), tr("State"), tr("Pool Entry"), tr("Current Query")
    });
    connectionTable_->setModel(model_);
    connectionTable_->horizontalHeader()->setStretchLastSection(true);
    connectionTable_->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    connectionTable_->verticalHeader()->setDefaultSectionSize(20);
}

void ConnectionPoolMonitorPanel::refresh()
{
    loadPoolStatistics();
    loadConnections();
}

void ConnectionPoolMonitorPanel::panelActivated()
{
    refresh();
}

void ConnectionPoolMonitorPanel::panelDeactivated()
{
    stopAutoRefresh();
}

void ConnectionPoolMonitorPanel::loadPoolStatistics()
{
    // TODO: Load from SessionClient when API is available
    
    PoolStatistics stats;
    stats.totalConnections = 10;
    stats.activeConnections = 3;
    stats.idleConnections = 7;
    stats.waitingClients = 0;
    stats.maxConnections = 100;
    
    totalLabel_->setText(tr("Total Connections: %1").arg(stats.totalConnections));
    activeLabel_->setText(tr("Active: %1").arg(stats.activeConnections));
    idleLabel_->setText(tr("Idle: %1").arg(stats.idleConnections));
    waitingLabel_->setText(tr("Waiting: %1").arg(stats.waitingClients));
    maxLabel_->setText(tr("Max: %1").arg(stats.maxConnections));
    
    int usage = (stats.totalConnections * 100) / stats.maxConnections;
    usageBar_->setValue(usage);
    usageBar_->setFormat(QString("%1%").arg(usage));
    
    if (usage > 90) {
        usageBar_->setStyleSheet("QProgressBar::chunk { background-color: red; }");
    } else if (usage > 70) {
        usageBar_->setStyleSheet("QProgressBar::chunk { background-color: orange; }");
    } else {
        usageBar_->setStyleSheet("QProgressBar::chunk { background-color: green; }");
    }
}

void ConnectionPoolMonitorPanel::loadConnections()
{
    model_->removeRows(0, model_->rowCount());
    
    // TODO: Load from SessionClient when API is available
}

void ConnectionPoolMonitorPanel::startAutoRefresh()
{
    refreshTimer_->start();
}

void ConnectionPoolMonitorPanel::stopAutoRefresh()
{
    refreshTimer_->stop();
}

void ConnectionPoolMonitorPanel::onAutoRefreshToggled(bool enabled)
{
    if (enabled) {
        startAutoRefresh();
    } else {
        stopAutoRefresh();
    }
}

void ConnectionPoolMonitorPanel::onRefreshIntervalChanged(int seconds)
{
    refreshTimer_->setInterval(seconds * 1000);
}

void ConnectionPoolMonitorPanel::onResetStatistics()
{
    // TODO: Reset statistics via SessionClient
    QMessageBox::information(this, tr("Reset Statistics"),
        tr("Pool statistics have been reset."));
    refresh();
}

void ConnectionPoolMonitorPanel::onConfigurePool()
{
    ConfigurePoolDialog dialog(client_, this);
    dialog.exec();
}

void ConnectionPoolMonitorPanel::onViewConnectionDetails()
{
    // TODO: Show connection details dialog
}

void ConnectionPoolMonitorPanel::onDisconnectConnection()
{
    auto index = connectionTable_->currentIndex();
    if (!index.isValid()) return;
    
    int pid = model_->item(index.row(), 0)->text().toInt();
    
    auto reply = QMessageBox::question(this, tr("Disconnect"),
        tr("Disconnect connection %1?").arg(pid),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Execute via SessionClient
        refresh();
    }
}

void ConnectionPoolMonitorPanel::onFilterChanged(const QString& filter)
{
    for (int i = 0; i < model_->rowCount(); ++i) {
        bool match = filter.isEmpty();
        if (!match) {
            for (int j = 0; j < model_->columnCount(); ++j) {
                auto* item = model_->item(i, j);
                if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        connectionTable_->setRowHidden(i, !match);
    }
}

// ============================================================================
// Configure Pool Dialog
// ============================================================================
ConfigurePoolDialog::ConfigurePoolDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Configure Connection Pool"));
    setMinimumSize(400, 300);
    setupUi();
    loadCurrentSettings();
}

void ConfigurePoolDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    maxConnectionsSpin_ = new QSpinBox(this);
    maxConnectionsSpin_->setRange(1, 10000);
    maxConnectionsSpin_->setValue(100);
    formLayout->addRow(tr("Max Connections:"), maxConnectionsSpin_);
    
    minConnectionsSpin_ = new QSpinBox(this);
    minConnectionsSpin_->setRange(0, 100);
    minConnectionsSpin_->setValue(10);
    formLayout->addRow(tr("Min Connections:"), minConnectionsSpin_);
    
    idleTimeoutSpin_ = new QSpinBox(this);
    idleTimeoutSpin_->setRange(0, 3600);
    idleTimeoutSpin_->setValue(300);
    idleTimeoutSpin_->setSuffix(" sec");
    formLayout->addRow(tr("Idle Timeout:"), idleTimeoutSpin_);
    
    connectionTimeoutSpin_ = new QSpinBox(this);
    connectionTimeoutSpin_->setRange(1, 300);
    connectionTimeoutSpin_->setValue(30);
    connectionTimeoutSpin_->setSuffix(" sec");
    formLayout->addRow(tr("Connection Timeout:"), connectionTimeoutSpin_);
    
    maxQueueSizeSpin_ = new QSpinBox(this);
    maxQueueSizeSpin_->setRange(0, 10000);
    maxQueueSizeSpin_->setValue(100);
    formLayout->addRow(tr("Max Queue Size:"), maxQueueSizeSpin_);
    
    autoScaleCheck_ = new QCheckBox(tr("Enable auto-scaling"), this);
    formLayout->addRow(QString(), autoScaleCheck_);
    
    mainLayout->addLayout(formLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigurePoolDialog::onSave);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ConfigurePoolDialog::loadCurrentSettings()
{
    // TODO: Load from SessionClient when API is available
}

void ConfigurePoolDialog::onSave()
{
    // TODO: Save via SessionClient when API is available
    accept();
}

void ConfigurePoolDialog::onReset()
{
    loadCurrentSettings();
}

} // namespace scratchrobin::ui
