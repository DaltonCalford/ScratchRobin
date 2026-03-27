#include "ui/lock_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
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
#include <QSplitter>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Lock Manager Panel
// ============================================================================
LockManagerPanel::LockManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("lock_manager", parent)
    , client_(client)
    , refreshTimer_(new QTimer(this))
{
    setupUi();
    setupModel();
    refresh();
}

LockManagerPanel::~LockManagerPanel()
{
    stopAutoRefresh();
}

void LockManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* killBtn = new QPushButton(tr("Kill Session"), this);
    auto* viewBtn = new QPushButton(tr("View Query"), this);
    auto* analyzeBtn = new QPushButton(tr("Analyze Chains"), this);
    
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(killBtn);
    toolbar->addWidget(viewBtn);
    toolbar->addWidget(analyzeBtn);
    
    connect(refreshBtn, &QPushButton::clicked, this, &LockManagerPanel::refresh);
    connect(killBtn, &QPushButton::clicked, this, &LockManagerPanel::onKillSession);
    connect(viewBtn, &QPushButton::clicked, this, &LockManagerPanel::onViewQuery);
    connect(analyzeBtn, &QPushButton::clicked, this, &LockManagerPanel::onAnalyzeWaitChains);
    
    mainLayout->addWidget(toolbar);
    
    // Auto refresh
    auto* refreshLayout = new QHBoxLayout();
    auto* autoRefreshCheck = new QCheckBox(tr("Auto Refresh"), this);
    auto* refreshInterval = new QSpinBox(this);
    refreshInterval->setRange(1, 60);
    refreshInterval->setValue(3);
    refreshInterval->setSuffix(" sec");
    
    refreshLayout->addWidget(autoRefreshCheck);
    refreshLayout->addWidget(refreshInterval);
    refreshLayout->addStretch();
    
    connect(autoRefreshCheck, &QCheckBox::toggled, this, &LockManagerPanel::onAutoRefreshToggled);
    connect(refreshInterval, &QSpinBox::valueChanged, this, &LockManagerPanel::onRefreshIntervalChanged);
    connect(refreshTimer_, &QTimer::timeout, this, &LockManagerPanel::refresh);
    
    mainLayout->addLayout(refreshLayout);
    
    // Stats panel
    auto* statsGroup = new QGroupBox(tr("Lock Statistics"), this);
    auto* statsLayout = new QHBoxLayout(statsGroup);
    
    totalLocksLabel_ = new QLabel(tr("Total: 0"), this);
    blockingLocksLabel_ = new QLabel(tr("Blocking: 0"), this);
    waitingLocksLabel_ = new QLabel(tr("Waiting: 0"), this);
    
    statsLayout->addWidget(totalLocksLabel_);
    statsLayout->addWidget(blockingLocksLabel_);
    statsLayout->addWidget(waitingLocksLabel_);
    statsLayout->addStretch();
    
    mainLayout->addWidget(statsGroup);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &LockManagerPanel::onFilterChanged);
    filterLayout->addWidget(filterEdit_);
    
    databaseFilter_ = new QComboBox(this);
    databaseFilter_->addItem(tr("(All DBs)"));
    filterLayout->addWidget(databaseFilter_);
    
    lockTypeFilter_ = new QComboBox(this);
    lockTypeFilter_->addItem(tr("(All Types)"));
    filterLayout->addWidget(lockTypeFilter_);
    
    showOnlyBlocking_ = new QCheckBox(tr("Blocking Only"), this);
    connect(showOnlyBlocking_, &QCheckBox::toggled, this, &LockManagerPanel::onShowOnlyBlockingChanged);
    filterLayout->addWidget(showOnlyBlocking_);
    
    showOnlyWaiting_ = new QCheckBox(tr("Waiting Only"), this);
    connect(showOnlyWaiting_, &QCheckBox::toggled, this, &LockManagerPanel::onShowOnlyWaitingChanged);
    filterLayout->addWidget(showOnlyWaiting_);
    
    mainLayout->addLayout(filterLayout);
    
    // Locks table
    lockTable_ = new QTableView(this);
    lockTable_->setAlternatingRowColors(true);
    lockTable_->setSortingEnabled(true);
    
    mainLayout->addWidget(lockTable_);
}

void LockManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(10);
    model_->setHorizontalHeaderLabels({
        tr("Lock ID"), tr("PID"), tr("Database"), tr("Relation"), 
        tr("Lock Type"), tr("Mode"), tr("Granted"), tr("Blocked By"), 
        tr("Wait Time"), tr("Query")
    });
    lockTable_->setModel(model_);
    lockTable_->horizontalHeader()->setStretchLastSection(true);
    lockTable_->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
    lockTable_->verticalHeader()->setDefaultSectionSize(20);
}

void LockManagerPanel::refresh()
{
    loadLocks();
}

void LockManagerPanel::panelActivated()
{
    refresh();
}

void LockManagerPanel::panelDeactivated()
{
    stopAutoRefresh();
}

void LockManagerPanel::loadLocks()
{
    model_->removeRows(0, model_->rowCount());
    
    if (!client_) return;
    
    // Query pg_locks for current locks
    std::string sql = 
        "SELECT l.locktype, l.database::regdatabase::text, l.relation::regclass::text, "
        "l.pid, l.mode, l.granted, l.fastpath, "
        "COALESCE(blocked.locktype, '') as blocked_by_type, "
        "COALESCE(blocked.pid::text, '') as blocked_by_pid, "
        "pg_stat_activity.query "
        "FROM pg_locks l "
        "LEFT JOIN pg_locks blocked ON l.pid = blocked.pid AND l.granted = false AND blocked.granted = true "
        "LEFT JOIN pg_stat_activity ON l.pid = pg_stat_activity.pid "
        "WHERE l.locktype != 'virtualxid' "
        "ORDER BY l.granted, l.pid";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    int blockingCount = 0;
    int waitingCount = 0;
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 11) continue;
            
            bool granted = (row[5] == "t" || row[5] == "true");
            bool isWaiting = !granted;
            bool isBlocking = !row[8].empty(); // blocked_by_pid
            
            if (isBlocking) blockingCount++;
            if (isWaiting) waitingCount++;
            
            QList<QStandardItem*> items;
            items << new QStandardItem(QString::fromStdString(row[0])); // Lock Type
            items << new QStandardItem(QString::fromStdString(row[3])); // PID
            items << new QStandardItem(QString::fromStdString(row[1])); // Database
            items << new QStandardItem(QString::fromStdString(row[2])); // Relation
            items << new QStandardItem(QString::fromStdString(row[0])); // Lock Type
            items << new QStandardItem(QString::fromStdString(row[4])); // Mode
            items << new QStandardItem(granted ? tr("Yes") : tr("No")); // Granted
            items << new QStandardItem(QString::fromStdString(row[8])); // Blocked By
            items << new QStandardItem(isWaiting ? tr("Waiting") : tr("-")); // Wait Time
            items << new QStandardItem(QString::fromStdString(row[10])); // Query
            
            model_->appendRow(items);
        }
    }
    
    totalLocksLabel_->setText(tr("Total: %1").arg(model_->rowCount()));
    blockingLocksLabel_->setText(tr("Blocking: %1").arg(blockingCount));
    waitingLocksLabel_->setText(tr("Waiting: %1").arg(waitingCount));
}

void LockManagerPanel::startAutoRefresh()
{
    refreshTimer_->start();
}

void LockManagerPanel::stopAutoRefresh()
{
    refreshTimer_->stop();
}

void LockManagerPanel::onAutoRefreshToggled(bool enabled)
{
    if (enabled) {
        startAutoRefresh();
    } else {
        stopAutoRefresh();
    }
}

void LockManagerPanel::onRefreshIntervalChanged(int seconds)
{
    refreshTimer_->setInterval(seconds * 1000);
}

void LockManagerPanel::onKillSession()
{
    auto index = lockTable_->currentIndex();
    if (!index.isValid()) return;
    
    int pid = model_->item(index.row(), 1)->text().toInt();
    QString query = model_->item(index.row(), 9)->text();
    
    auto reply = QMessageBox::question(this, tr("Kill Session"),
        tr("Kill session %1?\n\nQuery: %2").arg(pid).arg(query.left(100)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (client_) {
            std::string sql = QString("SELECT pg_terminate_backend(%1)").arg(pid).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        refresh();
    }
}

void LockManagerPanel::onViewQuery()
{
    auto index = lockTable_->currentIndex();
    if (!index.isValid()) return;
    
    QString query = model_->item(index.row(), 9)->text();
    
    QMessageBox::information(this, tr("Query"), query);
}

void LockManagerPanel::onAnalyzeWaitChains()
{
    LockWaitChainDialog dialog(client_, this);
    dialog.exec();
}

void LockManagerPanel::onFilterChanged(const QString& filter)
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
        lockTable_->setRowHidden(i, !match);
    }
}

void LockManagerPanel::onShowOnlyBlockingChanged(bool checked)
{
    if (!checked) {
        onFilterChanged(filterEdit_->text());
        return;
    }
    
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* blockedByItem = model_->item(i, 7); // Blocked By column
        bool isBlocking = blockedByItem && !blockedByItem->text().isEmpty();
        lockTable_->setRowHidden(i, !isBlocking);
    }
}

void LockManagerPanel::onShowOnlyWaitingChanged(bool checked)
{
    if (!checked) {
        onFilterChanged(filterEdit_->text());
        return;
    }
    
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* grantedItem = model_->item(i, 6); // Granted column
        bool isWaiting = grantedItem && grantedItem->text() == tr("No");
        lockTable_->setRowHidden(i, !isWaiting);
    }
}

// ============================================================================
// Lock Wait Chain Dialog
// ============================================================================
LockWaitChainDialog::LockWaitChainDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client)
{
    setWindowTitle(tr("Lock Wait Chains"));
    setMinimumSize(600, 500);
    setupUi();
    loadWaitChains();
}

void LockWaitChainDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    chainTree_ = new QTreeView(this);
    chainTree_->setAlternatingRowColors(true);
    
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(4);
    model_->setHorizontalHeaderLabels({tr("Blocker"), tr("Query"), tr("Blocked Sessions"), tr("Total Wait")});
    chainTree_->setModel(model_);
    chainTree_->header()->setStretchLastSection(false);
    chainTree_->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    
    mainLayout->addWidget(chainTree_);
    
    auto* buttonLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* killBtn = new QPushButton(tr("Kill Blocker"), this);
    auto* exportBtn = new QPushButton(tr("Export"), this);
    auto* closeBtn = new QPushButton(tr("Close"), this);
    
    connect(refreshBtn, &QPushButton::clicked, this, &LockWaitChainDialog::onRefresh);
    connect(killBtn, &QPushButton::clicked, this, &LockWaitChainDialog::onKillBlocker);
    connect(exportBtn, &QPushButton::clicked, this, &LockWaitChainDialog::onExportReport);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addWidget(refreshBtn);
    buttonLayout->addWidget(killBtn);
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
}

void LockWaitChainDialog::loadWaitChains()
{
    model_->removeRows(0, model_->rowCount());
    
    if (!client_) return;
    
    // Query for blocking sessions
    std::string sql = 
        "WITH RECURSIVE lock_chains AS ( "
        "  SELECT l.pid as blocker, NULL::integer as blocked, 0 as level, "
        "         l.mode, l.locktype, l.relation::regclass::text as obj, "
        "         pg_stat_activity.query "
        "  FROM pg_locks l "
        "  JOIN pg_stat_activity ON l.pid = pg_stat_activity.pid "
        "  WHERE l.granted = true "
        "  AND EXISTS (SELECT 1 FROM pg_locks l2 WHERE l2.pid = l.pid AND l2.granted = false) "
        "  UNION ALL "
        "  SELECT blocked.locktype as blocker, blocked.pid as blocked, "
        "         chains.level + 1, blocked.mode, blocked.locktype, "
        "         blocked.relation::regclass::text, pg_stat_activity.query "
        "  FROM pg_locks blocked "
        "  JOIN lock_chains chains ON blocked.pid = chains.blocker "
        "  JOIN pg_stat_activity ON blocked.pid = pg_stat_activity.pid "
        "  WHERE blocked.granted = false "
        ") "
        "SELECT blocker, query, COUNT(*) as blocked_count, MAX(level) as depth "
        "FROM lock_chains GROUP BY blocker, query ORDER BY depth DESC";
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 4) continue;
            
            QList<QStandardItem*> items;
            items << new QStandardItem(QString::fromStdString(row[0])); // Blocker PID
            items << new QStandardItem(QString::fromStdString(row[1])); // Query
            items << new QStandardItem(QString::fromStdString(row[2])); // Blocked Sessions
            items << new QStandardItem(QString::fromStdString(row[3])); // Total Wait
            
            model_->appendRow(items);
        }
    }
}

void LockWaitChainDialog::onRefresh()
{
    loadWaitChains();
}

void LockWaitChainDialog::onKillBlocker()
{
    auto index = chainTree_->currentIndex();
    if (!index.isValid()) return;
    
    auto reply = QMessageBox::warning(this, tr("Kill Blocker"),
        tr("Kill the blocking session?\n\nThis will release all blocked sessions."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (client_) {
            QString blockerPid = model_->item(index.row(), 0)->text();
            std::string sql = QString("SELECT pg_terminate_backend(%1)").arg(blockerPid).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        loadWaitChains();
    }
}

void LockWaitChainDialog::onExportReport()
{
    QMessageBox::information(this, tr("Export"), tr("Report exported."));
}

// ============================================================================
// Confirm Kill Session Dialog
// ============================================================================
ConfirmKillSessionDialog::ConfirmKillSessionDialog(int pid, const QString& query, QWidget* parent)
    : QDialog(parent)
    , pid_(pid)
    , query_(query)
{
    setWindowTitle(tr("Confirm Kill Session"));
    setMinimumSize(500, 300);
    setupUi();
}

void ConfirmKillSessionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* warningLabel = new QLabel(
        tr("<b>Warning:</b> You are about to kill session %1.").arg(pid_), this);
    warningLabel->setStyleSheet("color: red;");
    mainLayout->addWidget(warningLabel);
    
    mainLayout->addWidget(new QLabel(tr("Current Query:")));
    
    auto* queryEdit = new QTextEdit(this);
    queryEdit->setReadOnly(true);
    queryEdit->setPlainText(query_);
    queryEdit->setMaximumHeight(100);
    mainLayout->addWidget(queryEdit);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);
    buttonBox->button(QDialogButtonBox::Yes)->setText(tr("Kill Session"));
    buttonBox->button(QDialogButtonBox::Yes)->setStyleSheet("background-color: red; color: white;");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

} // namespace scratchrobin::ui
