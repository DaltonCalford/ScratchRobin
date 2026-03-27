#include "ui/session_manager.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
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
#include <QFileDialog>
#include <QDateTime>

namespace scratchrobin::ui {

// ============================================================================
// Session Manager Panel
// ============================================================================
SessionManagerPanel::SessionManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("session_manager", parent)
    , client_(client)
    , refreshTimer_(new QTimer(this))
{
    setupUi();
    setupModel();
    refresh();
}

SessionManagerPanel::~SessionManagerPanel()
{
    stopAutoRefresh();
}

void SessionManagerPanel::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* killBtn = new QPushButton(tr("Kill Session"), this);
    auto* killAllBtn = new QPushButton(tr("Kill All User Sessions"), this);
    auto* cancelBtn = new QPushButton(tr("Cancel Query"), this);
    auto* detailsBtn = new QPushButton(tr("View Details"), this);
    auto* historyBtn = new QPushButton(tr("History"), this);
    
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(killBtn);
    toolbar->addWidget(killAllBtn);
    toolbar->addWidget(cancelBtn);
    toolbar->addWidget(detailsBtn);
    toolbar->addWidget(historyBtn);
    
    connect(refreshBtn, &QPushButton::clicked, this, &SessionManagerPanel::refresh);
    connect(killBtn, &QPushButton::clicked, this, &SessionManagerPanel::onKillSession);
    connect(killAllBtn, &QPushButton::clicked, this, &SessionManagerPanel::onKillAllUserSessions);
    connect(cancelBtn, &QPushButton::clicked, this, &SessionManagerPanel::onCancelQuery);
    connect(detailsBtn, &QPushButton::clicked, this, &SessionManagerPanel::onViewQueryDetails);
    connect(historyBtn, &QPushButton::clicked, this, &SessionManagerPanel::onViewSessionHistory);
    
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
    
    connect(autoRefreshCheck, &QCheckBox::toggled, this, &SessionManagerPanel::onAutoRefreshToggled);
    connect(refreshInterval, &QSpinBox::valueChanged, this, &SessionManagerPanel::onRefreshIntervalChanged);
    connect(refreshTimer_, &QTimer::timeout, this, &SessionManagerPanel::refresh);
    
    mainLayout->addLayout(refreshLayout);
    
    // Stats panel
    auto* statsGroup = new QGroupBox(tr("Session Statistics"), this);
    auto* statsLayout = new QHBoxLayout(statsGroup);
    
    totalLabel_ = new QLabel(tr("Total: 0"), this);
    activeLabel_ = new QLabel(tr("Active: 0"), this);
    idleLabel_ = new QLabel(tr("Idle: 0"), this);
    waitingLabel_ = new QLabel(tr("Waiting: 0"), this);
    
    statsLayout->addWidget(totalLabel_);
    statsLayout->addWidget(activeLabel_);
    statsLayout->addWidget(idleLabel_);
    statsLayout->addWidget(waitingLabel_);
    statsLayout->addStretch();
    
    mainLayout->addWidget(statsGroup);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter sessions..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &SessionManagerPanel::onFilterChanged);
    filterLayout->addWidget(filterEdit_);
    
    userFilter_ = new QComboBox(this);
    userFilter_->setEditable(true);
    userFilter_->addItem(tr("(All Users)"));
    filterLayout->addWidget(userFilter_);
    
    databaseFilter_ = new QComboBox(this);
    databaseFilter_->setEditable(true);
    databaseFilter_->addItem(tr("(All Databases)"));
    filterLayout->addWidget(databaseFilter_);
    
    stateFilter_ = new QComboBox(this);
    stateFilter_->addItem(tr("(All States)"));
    stateFilter_->addItem("active");
    stateFilter_->addItem("idle");
    stateFilter_->addItem("idle in transaction");
    filterLayout->addWidget(stateFilter_);
    
    mainLayout->addLayout(filterLayout);
    
    // Sessions table
    sessionTable_ = new QTableView(this);
    sessionTable_->setAlternatingRowColors(true);
    sessionTable_->setSortingEnabled(true);
    sessionTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    mainLayout->addWidget(sessionTable_);
}

void SessionManagerPanel::setupModel()
{
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(10);
    model_->setHorizontalHeaderLabels({
        tr("PID"), tr("User"), tr("Database"), tr("Application"), 
        tr("Client"), tr("State"), tr("Wait Event"), tr("Duration"), 
        tr("Query Start"), tr("Query")
    });
    sessionTable_->setModel(model_);
    sessionTable_->horizontalHeader()->setStretchLastSection(true);
    sessionTable_->horizontalHeader()->setSectionResizeMode(9, QHeaderView::Stretch);
    sessionTable_->verticalHeader()->setDefaultSectionSize(20);
}

void SessionManagerPanel::refresh()
{
    loadSessions();
}

void SessionManagerPanel::panelActivated()
{
    refresh();
}

void SessionManagerPanel::panelDeactivated()
{
    stopAutoRefresh();
}

void SessionManagerPanel::loadSessions()
{
    model_->removeRows(0, model_->rowCount());
    
    if (client_) {
        // Query active sessions from database
        std::string sql = 
            "SELECT pid, usename, datname, client_addr, backend_start, "
            "state, wait_event_type, query_start, query "
            "FROM pg_stat_activity "
            "WHERE state IS NOT NULL "
            "ORDER BY backend_start DESC";
        
        auto response = client_->ExecuteSql(4044, "scratchbird", sql);
        
        if (response.status.ok) {
            for (const auto& row : response.result_set.rows) {
                if (row.size() < 9) continue;
                
                QList<QStandardItem*> items;
                items << new QStandardItem(QString::fromStdString(row[0])); // PID
                items << new QStandardItem(QString::fromStdString(row[1])); // Username
                items << new QStandardItem(QString::fromStdString(row[2])); // Database
                items << new QStandardItem(QString::fromStdString(row[3])); // Client
                items << new QStandardItem(QString::fromStdString(row[4])); // Connected
                items << new QStandardItem(QString::fromStdString(row[5])); // State
                items << new QStandardItem(QString::fromStdString(row[6])); // Wait Event
                items << new QStandardItem(QString::fromStdString(row[7])); // Query Start
                items << new QStandardItem(QString::fromStdString(row[8]).left(100)); // Query
                
                model_->appendRow(items);
            }
        }
    }
    
    // Update stats
    totalLabel_->setText(tr("Total: %1").arg(model_->rowCount()));
}

void SessionManagerPanel::startAutoRefresh()
{
    refreshTimer_->start();
}

void SessionManagerPanel::stopAutoRefresh()
{
    refreshTimer_->stop();
}

void SessionManagerPanel::onAutoRefreshToggled(bool enabled)
{
    if (enabled) {
        startAutoRefresh();
    } else {
        stopAutoRefresh();
    }
}

void SessionManagerPanel::onRefreshIntervalChanged(int seconds)
{
    refreshTimer_->setInterval(seconds * 1000);
}

void SessionManagerPanel::onKillSession()
{
    auto index = sessionTable_->currentIndex();
    if (!index.isValid()) return;
    
    int pid = model_->item(index.row(), 0)->text().toInt();
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

void SessionManagerPanel::onKillAllUserSessions()
{
    auto index = sessionTable_->currentIndex();
    if (!index.isValid()) return;
    
    QString username = model_->item(index.row(), 1)->text();
    
    auto reply = QMessageBox::warning(this, tr("Kill All User Sessions"),
        tr("Kill ALL sessions for user '%1'?\n\nThis will terminate all active connections for this user.").arg(username),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Kill all sessions for this user except current
        if (client_) {
            std::string sql = QString(
                "SELECT pg_terminate_backend(pid) "
                "FROM pg_stat_activity "
                "WHERE usename = '%1' AND pid != pg_backend_pid()"
            ).arg(username).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        refresh();
    }
}

void SessionManagerPanel::onCancelQuery()
{
    auto index = sessionTable_->currentIndex();
    if (!index.isValid()) return;
    
    int pid = model_->item(index.row(), 0)->text().toInt();
    
    auto reply = QMessageBox::question(this, tr("Cancel Query"),
        tr("Cancel query for session %1?").arg(pid),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Cancel query by sending cancel signal (using pg_cancel_backend)
        if (client_) {
            std::string sql = QString("SELECT pg_cancel_backend(%1)").arg(pid).toStdString();
            client_->ExecuteSql(4044, "scratchbird", sql);
        }
        refresh();
    }
}

void SessionManagerPanel::onViewQueryDetails()
{
    auto index = sessionTable_->currentIndex();
    if (!index.isValid()) return;
    
    SessionInfo info;
    info.pid = model_->item(index.row(), 0)->text().toInt();
    info.username = model_->item(index.row(), 1)->text();
    info.database = model_->item(index.row(), 2)->text();
    info.state = model_->item(index.row(), 5)->text();
    info.currentQuery = model_->item(index.row(), 9)->text();
    
    SessionDetailsDialog dialog(client_, info, this);
    dialog.exec();
}

void SessionManagerPanel::onViewSessionHistory()
{
    auto index = sessionTable_->currentIndex();
    if (!index.isValid()) return;
    
    int pid = model_->item(index.row(), 0)->text().toInt();
    
    SessionHistoryDialog dialog(client_, pid, this);
    dialog.exec();
}

void SessionManagerPanel::onFilterChanged(const QString& filter)
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
        sessionTable_->setRowHidden(i, !match);
    }
}

// ============================================================================
// Session Details Dialog
// ============================================================================
SessionDetailsDialog::SessionDetailsDialog(backend::SessionClient* client,
                                          const SessionInfo& session,
                                          QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , session_(session)
{
    setWindowTitle(tr("Session Details - PID %1").arg(session.pid));
    setMinimumSize(600, 400);
    setupUi();
}

void SessionDetailsDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setFont(QFont("Monospace", 10));
    
    QString details;
    details += QString("PID: %1\n").arg(session_.pid);
    details += QString("User: %1\n").arg(session_.username);
    details += QString("Database: %1\n").arg(session_.database);
    details += QString("State: %1\n").arg(session_.state);
    details += QString("Query Duration: %1 ms\n").arg(session_.queryDuration);
    details += "\n";
    details += "Current Query:\n";
    details += "-" + QString(50, '-') + "\n";
    details += session_.currentQuery;
    
    detailsEdit_->setPlainText(details);
    mainLayout->addWidget(detailsEdit_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
}

void SessionDetailsDialog::loadSessionDetails()
{
    if (!client_) return;
    
    // Load additional session statistics
    std::string sql = QString(
        "SELECT backend_start, xact_start, query_start, state_change, "
        "wait_event_type, wait_event, backend_xid, backend_xmin "
        "FROM pg_stat_activity WHERE pid = %1"
    ).arg(session_.pid).toStdString();
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok && !response.result_set.rows.empty()) {
        const auto& row = response.result_set.rows[0];
        QString details = detailsEdit_->toPlainText();
        details += "\n\nAdditional Details:\n";
        details += "-" + QString(50, '-') + "\n";
        if (row.size() > 0) details += QString("Backend Start: %1\n").arg(QString::fromStdString(row[0]));
        if (row.size() > 1) details += QString("Transaction Start: %1\n").arg(QString::fromStdString(row[1]));
        if (row.size() > 2) details += QString("Query Start: %1\n").arg(QString::fromStdString(row[2]));
        if (row.size() > 3) details += QString("State Change: %1\n").arg(QString::fromStdString(row[3]));
        if (row.size() > 4 && !row[4].empty()) details += QString("Wait Event Type: %1\n").arg(QString::fromStdString(row[4]));
        if (row.size() > 5 && !row[5].empty()) details += QString("Wait Event: %1\n").arg(QString::fromStdString(row[5]));
        detailsEdit_->setPlainText(details);
    }
}

// ============================================================================
// Session History Dialog
// ============================================================================
SessionHistoryDialog::SessionHistoryDialog(backend::SessionClient* client,
                                          int pid,
                                          QWidget* parent)
    : QDialog(parent)
    , client_(client)
    , pid_(pid)
{
    setWindowTitle(tr("Session History - PID %1").arg(pid));
    setMinimumSize(700, 500);
    setupUi();
    loadHistory();
}

void SessionHistoryDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    
    auto* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel(tr("Limit:")));
    
    limitSpin_ = new QSpinBox(this);
    limitSpin_->setRange(10, 1000);
    limitSpin_->setValue(100);
    controlLayout->addWidget(limitSpin_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &SessionHistoryDialog::onRefresh);
    controlLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SessionHistoryDialog::onExport);
    controlLayout->addWidget(exportBtn);
    
    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);
    
    historyTable_ = new QTableView(this);
    historyTable_->setAlternatingRowColors(true);
    
    model_ = new QStandardItemModel(this);
    model_->setColumnCount(5);
    model_->setHorizontalHeaderLabels({
        tr("Timestamp"), tr("Duration (ms)"), tr("Rows"), tr("Status"), tr("Query")
    });
    historyTable_->setModel(model_);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    historyTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    
    mainLayout->addWidget(historyTable_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
}

void SessionHistoryDialog::loadHistory()
{
    model_->removeRows(0, model_->rowCount());
    
    if (!client_) return;
    
    int limit = limitSpin_->value();
    
    // Query statement statistics from pg_stat_statements if available
    std::string sql = QString(
        "SELECT queryid, calls, total_exec_time, rows, "
        "shared_blks_hit, shared_blks_read, query "
        "FROM pg_stat_statements "
        "WHERE query NOT LIKE '%%pg_stat%%' "
        "ORDER BY total_exec_time DESC "
        "LIMIT %1"
    ).arg(limit).toStdString();
    
    auto response = client_->ExecuteSql(4044, "scratchbird", sql);
    
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() < 7) continue;
            
            QList<QStandardItem*> items;
            items << new QStandardItem("N/A"); // Timestamp (not available in pg_stat_statements)
            items << new QStandardItem(QString::fromStdString(row[2])); // Duration
            items << new QStandardItem(QString::fromStdString(row[3])); // Rows
            items << new QStandardItem(QString::fromStdString(row[1])); // Calls (as status)
            items << new QStandardItem(QString::fromStdString(row[6]).left(200)); // Query
            
            model_->appendRow(items);
        }
    } else {
        // pg_stat_statements not available, show sample data
        QList<QStandardItem*> items;
        items << new QStandardItem(QDateTime::currentDateTime().toString());
        items << new QStandardItem("0");
        items << new QStandardItem("0");
        items << new QStandardItem("N/A");
        items << new QStandardItem("Statistics extension not available");
        model_->appendRow(items);
    }
}

void SessionHistoryDialog::onRefresh()
{
    loadHistory();
}

void SessionHistoryDialog::onExport()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Session History"),
                                                    QString(), tr("CSV Files (*.csv)"));
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), 
                             tr("Could not open file for writing: %1").arg(fileName));
        return;
    }
    
    QTextStream stream(&file);
    
    // Headers
    QStringList headers;
    for (int col = 0; col < model_->columnCount(); ++col) {
        if (col > 0) stream << ",";
        stream << "\"" << model_->headerData(col, Qt::Horizontal).toString() << "\"";
    }
    stream << "\n";
    
    // Data
    for (int row = 0; row < model_->rowCount(); ++row) {
        for (int col = 0; col < model_->columnCount(); ++col) {
            if (col > 0) stream << ",";
            QString value = model_->item(row, col)->text();
            value.replace("\"", "\"\""); // Escape quotes
            stream << "\"" << value << "\"";
        }
        stream << "\n";
    }
    
    file.close();
    QMessageBox::information(this, tr("Export"), 
                             tr("History exported successfully to %1").arg(fileName));
}

} // namespace scratchrobin::ui
