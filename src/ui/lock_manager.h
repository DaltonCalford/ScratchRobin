#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTreeView;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QComboBox;
class QCheckBox;
class QTimer;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Lock Manager
 * 
 * Views and manages database locks.
 * - View current locks
 * - Detect lock conflicts
 * - Kill blocking sessions
 * - Analyze lock wait chains
 */

// ============================================================================
// Lock Info Structures
// ============================================================================
struct LockDetails {
    int lockId = 0;
    int pid = 0;
    QString database;
    QString relation;
    QString relationType;
    QString lockType;
    QString lockMode;
    bool granted = false;
    int blockedBy = 0;
    QDateTime queryStart;
    QString query;
    int waitTime = 0;  // milliseconds
};

struct LockWaitChainInfo {
    int blockerPid = 0;
    QString blockerQuery;
    QList<int> blockedPids;
    int totalWaitTime = 0;
};

// ============================================================================
// Lock Manager Panel
// ============================================================================
class LockManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit LockManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~LockManagerPanel() override;
    
    QString panelTitle() const override { return tr("Lock Manager"); }
    QString panelCategory() const override { return "monitoring"; }
    
    void refresh();

public slots:
    void onAutoRefreshToggled(bool enabled);
    void onRefreshIntervalChanged(int seconds);
    void onKillSession();
    void onViewQuery();
    void onAnalyzeWaitChains();
    void onFilterChanged(const QString& filter);
    void onShowOnlyBlockingChanged(bool checked);
    void onShowOnlyWaitingChanged(bool checked);

protected:
    void panelActivated() override;
    void panelDeactivated() override;

private:
    void setupUi();
    void setupModel();
    void loadLocks();
    void startAutoRefresh();
    void stopAutoRefresh();
    
    backend::SessionClient* client_;
    QTimer* refreshTimer_ = nullptr;
    
    QTableView* lockTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    // Filters
    QLineEdit* filterEdit_ = nullptr;
    QComboBox* databaseFilter_ = nullptr;
    QComboBox* lockTypeFilter_ = nullptr;
    QCheckBox* showOnlyBlocking_ = nullptr;
    QCheckBox* showOnlyWaiting_ = nullptr;
    
    // Stats
    QLabel* totalLocksLabel_ = nullptr;
    QLabel* blockingLocksLabel_ = nullptr;
    QLabel* waitingLocksLabel_ = nullptr;
};

// ============================================================================
// Lock Wait Chain Dialog
// ============================================================================
class LockWaitChainDialog : public QDialog {
    Q_OBJECT

public:
    explicit LockWaitChainDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onKillBlocker();
    void onExportReport();

private:
    void setupUi();
    void loadWaitChains();
    
    backend::SessionClient* client_;
    
    QTreeView* chainTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Confirm Kill Session Dialog
// ============================================================================
class ConfirmKillSessionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfirmKillSessionDialog(int pid, const QString& query, QWidget* parent = nullptr);

private:
    void setupUi();
    
    int pid_;
    QString query_;
};

} // namespace scratchrobin::ui
