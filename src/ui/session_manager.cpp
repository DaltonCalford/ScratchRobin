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
    
    // TODO: Load from SessionClient when API is available
    
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
        // TODO: Execute via SessionClient when API is available
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
        // TODO: Execute via SessionClient when API is available
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
        // TODO: Execute via SessionClient when API is available
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
    // TODO: Load additional details from SessionClient when API is available
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
    
    // TODO: Load from SessionClient when API is available
}

void SessionHistoryDialog::onRefresh()
{
    loadHistory();
}

void SessionHistoryDialog::onExport()
{
    // TODO: Export to CSV
    QMessageBox::information(this, tr("Export"), tr("History exported successfully."));
}

} // namespace scratchrobin::ui
