#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QProgressBar;
class QCheckBox;
class QSpinBox;
class QTimer;
class QGroupBox;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Connection Pool Monitor
 * 
 * Monitors database connection pool statistics.
 * - View pool status (active/idle connections)
 * - Monitor wait queue
 * - View connection statistics
 * - Configure pool settings
 */

// ============================================================================
// Pool Info Structures
// ============================================================================
struct PoolStatistics {
    int totalConnections = 0;
    int activeConnections = 0;
    int idleConnections = 0;
    int waitingClients = 0;
    int maxConnections = 0;
    int minConnections = 0;
    qint64 totalQueries = 0;
    qint64 totalTransactions = 0;
    double avgQueryTime = 0.0;
    double avgWaitTime = 0.0;
    QDateTime lastReset;
};

struct PoolConnectionInfo {
    int pid = 0;
    QString application;
    QString clientHost;
    QDateTime connected;
    QDateTime lastActivity;
    QString currentQuery;
    QString state;
    int poolEntryId = 0;
};

// ============================================================================
// Connection Pool Monitor Panel
// ============================================================================
class ConnectionPoolMonitorPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ConnectionPoolMonitorPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~ConnectionPoolMonitorPanel() override;
    
    QString panelTitle() const override { return tr("Connection Pool Monitor"); }
    QString panelCategory() const override { return "monitoring"; }
    
    void refresh();

public slots:
    void onAutoRefreshToggled(bool enabled);
    void onRefreshIntervalChanged(int seconds);
    void onResetStatistics();
    void onConfigurePool();
    void onViewConnectionDetails();
    void onDisconnectConnection();
    void onFilterChanged(const QString& filter);

protected:
    void panelActivated() override;
    void panelDeactivated() override;

private:
    void setupUi();
    void setupModel();
    void loadPoolStatistics();
    void loadConnections();
    void startAutoRefresh();
    void stopAutoRefresh();
    
    backend::SessionClient* client_;
    QTimer* refreshTimer_ = nullptr;
    
    // Statistics panel
    QLabel* totalLabel_ = nullptr;
    QLabel* activeLabel_ = nullptr;
    QLabel* idleLabel_ = nullptr;
    QLabel* waitingLabel_ = nullptr;
    QLabel* maxLabel_ = nullptr;
    QProgressBar* usageBar_ = nullptr;
    
    // Connections table
    QTableView* connectionTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    QLineEdit* filterEdit_ = nullptr;
};

// ============================================================================
// Configure Pool Dialog
// ============================================================================
class ConfigurePoolDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigurePoolDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onSave();
    void onReset();

private:
    void setupUi();
    void loadCurrentSettings();
    
    backend::SessionClient* client_;
    
    QSpinBox* maxConnectionsSpin_ = nullptr;
    QSpinBox* minConnectionsSpin_ = nullptr;
    QSpinBox* idleTimeoutSpin_ = nullptr;
    QSpinBox* connectionTimeoutSpin_ = nullptr;
    QSpinBox* maxQueueSizeSpin_ = nullptr;
    QCheckBox* autoScaleCheck_ = nullptr;
};

} // namespace scratchrobin::ui
