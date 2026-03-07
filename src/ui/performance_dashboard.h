#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QLabel;
class QProgressBar;
class QSplitter;
class QTimer;
class QGroupBox;
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Performance Dashboard - Real-time database metrics visualization
 * 
 * Monitor database performance in real-time:
 * - Connection count and activity
 * - Query throughput (QPS)
 * - Cache hit ratios
 * - I/O statistics
 * - Lock waits
 * - Resource utilization charts
 */

// ============================================================================
// Metric Types
// ============================================================================
enum class MetricType {
    Connections,
    QueriesPerSecond,
    TransactionsPerSecond,
    CacheHitRatio,
    DiskIO,
    LockWaits,
    ActiveQueries,
    IdleConnections,
    TempFileUsage,
    CheckpointActivity
};

struct MetricValue {
    MetricType type;
    QString name;
    double current = 0;
    double min = 0;
    double max = 0;
    double avg = 0;
    QString unit;
    QDateTime timestamp;
    QList<QPair<QDateTime, double>> history;
};

struct ConnectionStats {
    int total = 0;
    int active = 0;
    int idle = 0;
    int idleInTransaction = 0;
    int waiting = 0;
};

struct QueryStats {
    double queriesPerSecond = 0;
    double transactionsPerSecond = 0;
    double avgQueryTime = 0;
    int slowQueries = 0;
    int failedQueries = 0;
};

struct CacheStats {
    double bufferHitRatio = 0;
    double indexHitRatio = 0;
    int blocksRead = 0;
    int blocksHit = 0;
};

// ============================================================================
// Performance Dashboard Panel
// ============================================================================
class PerformanceDashboardPanel : public DockPanel {
    Q_OBJECT

public:
    explicit PerformanceDashboardPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    ~PerformanceDashboardPanel() override;
    
    QString panelTitle() const override { return tr("Performance Dashboard"); }
    QString panelCategory() const override { return "performance"; }

public slots:
    void onStartMonitoring();
    void onStopMonitoring();
    void onPauseMonitoring();
    void onRefreshNow();
    void onClearHistory();
    
    // Metric selection
    void onToggleMetric(int metricId, bool enabled);
    void onSelectAllMetrics();
    void onSelectNoMetrics();
    
    // Export
    void onExportMetrics();
    void onSaveSnapshot();
    void onCompareSnapshots();
    
    // Configuration
    void onConfigureDashboard();
    void onSetRefreshInterval(int seconds);

signals:
    void metricsUpdated(const QList<MetricValue>& metrics);
    void alertTriggered(const QString& alertType, const QString& message);

private:
    void setupUi();
    void setupOverviewTab();
    void setupConnectionsTab();
    void setupQueryStatsTab();
    void setupCacheTab();
    void setupIOTab();
    void setupCharts();
    void updateMetrics();
    void updateOverviewCards();
    void updateConnectionStats();
    void updateQueryStats();
    void updateCacheStats();
    void updateIOStats();
    void checkAlerts();
    
    backend::SessionClient* client_;
    QTimer* refreshTimer_ = nullptr;
    bool isMonitoring_ = false;
    int refreshInterval_ = 5; // seconds
    
    // Metric storage
    QList<MetricValue> metrics_;
    ConnectionStats connStats_;
    QueryStats queryStats_;
    CacheStats cacheStats_;
    
    // UI Components
    QTabWidget* tabs_ = nullptr;
    
    // Overview tab
    QLabel* qpsLabel_ = nullptr;
    QLabel* tpsLabel_ = nullptr;
    QLabel* connectionsLabel_ = nullptr;
    QLabel* cacheHitLabel_ = nullptr;
    QLabel* activeQueriesLabel_ = nullptr;
    QLabel* slowQueriesLabel_ = nullptr;
    
    QProgressBar* cpuBar_ = nullptr;
    QProgressBar* memoryBar_ = nullptr;
    QProgressBar* diskBar_ = nullptr;
    
    // Connections tab
    QTableView* connectionsTable_ = nullptr;
    QStandardItemModel* connectionsModel_ = nullptr;
    
    // Query stats tab
    QTableView* queryStatsTable_ = nullptr;
    QStandardItemModel* queryStatsModel_ = nullptr;
    
    // Charts (simplified as QLabel placeholders for now)
    QLabel* qpsChart_ = nullptr;
    QLabel* connChart_ = nullptr;
    QLabel* cacheChart_ = nullptr;
    
    // Toolbar
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Metric Configuration Dialog
// ============================================================================
class MetricConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit MetricConfigDialog(QWidget* parent = nullptr);

public slots:
    void onSave();
    void onReset();

private:
    void setupUi();
    
    QCheckBox* showConnectionsCheck_ = nullptr;
    QCheckBox* showQPSCheck_ = nullptr;
    QCheckBox* showTPSCheck_ = nullptr;
    QCheckBox* showCacheCheck_ = nullptr;
    QCheckBox* showIOCheck_ = nullptr;
    QCheckBox* showLocksCheck_ = nullptr;
    
    QSpinBox* historySizeSpin_ = nullptr;
    QSpinBox* alertThresholdSpin_ = nullptr;
};

// ============================================================================
// Snapshot Comparison Dialog
// ============================================================================
class SnapshotComparisonDialog : public QDialog {
    Q_OBJECT

public:
    explicit SnapshotComparisonDialog(QWidget* parent = nullptr);

public slots:
    void onLoadSnapshot1();
    void onLoadSnapshot2();
    void onExportComparison();

private:
    void setupUi();
    void compare();
    
    QTableView* comparisonTable_ = nullptr;
    QStandardItemModel* comparisonModel_ = nullptr;
    QTextEdit* summaryEdit_ = nullptr;
};

} // namespace scratchrobin::ui
