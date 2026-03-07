#pragma once
#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QDateTime>
#include <QHash>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QTreeWidget;
class QTextEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QLabel;
class QTabWidget;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Database Monitoring System
 * 
 * Implements FORM_SPECIFICATION.md Monitoring section:
 * - Connection monitor
 * - Transaction monitor  
 * - Lock monitor
 * - Performance monitor
 */

// ============================================================================
// Connection Information (for monitoring)
// ============================================================================
struct DbConnectionInfo {
    int id = 0;
    QString clientAddress;
    QString database;
    QString username;
    QDateTime connectedSince;
    QString state;  // active, idle, idle in transaction
    QString currentQuery;
    qint64 queryDurationMs = 0;
    int pid = 0;
    QString applicationName;
};

// ============================================================================
// Lock Information
// ============================================================================
struct LockInfo {
    int lockId = 0;
    int transactionId = 0;
    int processId = 0;
    QString lockMode;  // SHARE, EXCLUSIVE, ACCESS SHARE, etc.
    QString lockType;  // relation, tuple, transactionid, etc.
    QString tableName;
    QString status;  // granted, waiting
    QDateTime grantedSince;
    int blockedProcessId = 0;
    bool isBlocking = false;
};

// ============================================================================
// Performance Metrics
// ============================================================================
struct PerformanceMetrics {
    double queriesPerSecond = 0.0;
    double transactionsPerSecond = 0.0;
    double cacheHitRatio = 0.0;
    double activeConnections = 0;
    double idleConnections = 0;
    double blockedConnections = 0;
    double avgQueryTime = 0.0;
    double cpuUsage = 0.0;
    double memoryUsage = 0.0;
    double diskIO = 0.0;
    QDateTime timestamp;
};

// ============================================================================
// Connection Monitor Panel
// ============================================================================
class ConnectionMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit ConnectionMonitorPanel(backend::SessionClient* client, QWidget* parent = nullptr);

    void refresh();
    void setAutoRefresh(bool enabled, int intervalMs = 5000);

signals:
    void connectionSelected(const DbConnectionInfo& info);
    void terminateRequested(int connectionId);

private slots:
    void onRefresh();
    void onTerminateConnection();
    void onFilterChanged(const QString& filter);
    void onShowDetails();
    void updateDisplay();

private:
    void setupUi();
    void loadConnections();
    QList<DbConnectionInfo> fetchConnections();
    void populateTable(const QList<DbConnectionInfo>& connections);

    backend::SessionClient* client_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    
    QTableWidget* table_ = nullptr;
    QPushButton* refreshBtn_ = nullptr;
    QPushButton* terminateBtn_ = nullptr;
    QCheckBox* autoRefreshCheck_ = nullptr;
    QComboBox* filterCombo_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    
    QList<DbConnectionInfo> currentConnections_;
};

// ============================================================================
// Transaction Monitor Panel
// ============================================================================
class TransactionMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit TransactionMonitorPanel(backend::SessionClient* client, QWidget* parent = nullptr);

    void refresh();

signals:
    void transactionSelected(int transactionId);

private slots:
    void onRefresh();
    void onViewDetails();
    void onRollbackTransaction();

private:
    void setupUi();
    void loadTransactions();

    backend::SessionClient* client_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    
    QTableWidget* table_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
    QLabel* activeCountLabel_ = nullptr;
    QLabel* idleCountLabel_ = nullptr;
    QLabel* longRunningLabel_ = nullptr;
};

// ============================================================================
// Lock Monitor Panel
// ============================================================================
class LockMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit LockMonitorPanel(backend::SessionClient* client, QWidget* parent = nullptr);

    void refresh();

signals:
    void lockConflictDetected(const LockInfo& blocked, const LockInfo& blocker);

private slots:
    void onRefresh();
    void onKillBlockingProcess();
    void onViewLockGraph();

private:
    void setupUi();
    void loadLocks();
    QList<LockInfo> fetchLocks();
    void detectDeadlocks();
    void populateTable(const QList<LockInfo>& locks);

    backend::SessionClient* client_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    
    QTableWidget* table_ = nullptr;
    QTreeWidget* lockGraph_ = nullptr;
    QLabel* blockedCountLabel_ = nullptr;
    QLabel* deadlockLabel_ = nullptr;
    
    QList<LockInfo> currentLocks_;
};

// ============================================================================
// Performance Monitor Panel
// ============================================================================
class PerformanceMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit PerformanceMonitorPanel(backend::SessionClient* client, QWidget* parent = nullptr);

    void refresh();
    void setAutoRefresh(bool enabled, int intervalMs = 2000);
    void startRecording();
    void stopRecording();
    void exportData(const QString& fileName);

signals:
    void alertTriggered(const QString& message, const QString& severity);

private slots:
    void onRefresh();
    void onToggleRecording(bool checked);
    void onExport();
    void onConfigureAlerts();
    void updateCharts();

private:
    void setupUi();
    void collectMetrics();
    void updateMetricDisplays();
    void checkAlerts();
    PerformanceMetrics fetchMetrics();

    backend::SessionClient* client_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    bool isRecording_ = false;
    QList<PerformanceMetrics> metricsHistory_;
    
    // Metric labels
    QLabel* qpsLabel_ = nullptr;
    QLabel* tpsLabel_ = nullptr;
    QLabel* cacheHitLabel_ = nullptr;
    QLabel* connectionsLabel_ = nullptr;
    QLabel* avgQueryTimeLabel_ = nullptr;
    QLabel* cpuLabel_ = nullptr;
    QLabel* memoryLabel_ = nullptr;
    QLabel* diskIOLabel_ = nullptr;
    
    // Charts placeholder
    QWidget* qpsChart_ = nullptr;
    QWidget* connectionsChart_ = nullptr;
    
    QPushButton* recordBtn_ = nullptr;
    QPushButton* exportBtn_ = nullptr;
    
    // Alert thresholds
    double qpsThreshold_ = 1000.0;
    double connectionThreshold_ = 100.0;
    double cacheHitThreshold_ = 90.0;
};

// ============================================================================
// Combined Monitoring Dashboard
// ============================================================================
class MonitoringDashboard : public QDialog {
    Q_OBJECT

public:
    explicit MonitoringDashboard(backend::SessionClient* client, QWidget* parent = nullptr);

    void setActiveTab(int tabIndex);

private slots:
    void onConnectionSelected(const DbConnectionInfo& info);
    void onLockConflict(const LockInfo& blocked, const LockInfo& blocker);
    void onPerformanceAlert(const QString& message, const QString& severity);

private:
    void setupUi();

    backend::SessionClient* client_ = nullptr;
    
    QTabWidget* tabWidget_ = nullptr;
    ConnectionMonitorPanel* connectionPanel_ = nullptr;
    TransactionMonitorPanel* transactionPanel_ = nullptr;
    LockMonitorPanel* lockPanel_ = nullptr;
    PerformanceMonitorPanel* performancePanel_ = nullptr;
};

// ============================================================================
// Query Cancellation Dialog
// ============================================================================
class QueryCancellationDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryCancellationDialog(backend::SessionClient* client, QWidget* parent = nullptr);

    void monitorQuery(int queryId, const QString& queryText);

private slots:
    void onCancelQuery();
    void onForceCancel();
    void onRefreshStatus();
    void updateProgress();

private:
    void setupUi();
    void checkQueryStatus();

    backend::SessionClient* client_ = nullptr;
    int queryId_ = 0;
    QString queryText_;
    
    QLabel* queryLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* elapsedLabel_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QPushButton* forceBtn_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    QDateTime startTime_;
};

// ============================================================================
// DDL Generation Dialog
// ============================================================================
class DDLGenerationDialog : public QDialog {
    Q_OBJECT

public:
    explicit DDLGenerationDialog(backend::SessionClient* client, QWidget* parent = nullptr);

    void setObject(const QString& objectName, const QString& objectType);

private slots:
    void onGenerate();
    void onCopyToClipboard();
    void onSaveToFile();
    void onObjectTypeChanged();

private:
    void setupUi();
    QString generateDDL(const QString& objectName, const QString& objectType);
    QString generateTableDDL(const QString& tableName);
    QString generateIndexDDL(const QString& indexName);
    QString generateViewDDL(const QString& viewName);
    QString generateProcedureDDL(const QString& procName);
    QString generateFullSchemaDDL();

    backend::SessionClient* client_ = nullptr;
    
    QComboBox* objectTypeCombo_ = nullptr;
    QComboBox* objectNameCombo_ = nullptr;
    QCheckBox* includeDropCheck_ = nullptr;
    QCheckBox* includeConstraintsCheck_ = nullptr;
    QCheckBox* includeIndexesCheck_ = nullptr;
    QTextEdit* outputEdit_ = nullptr;
};

// ============================================================================
// Session Manager (manages multiple connections)
// ============================================================================
class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject* parent = nullptr);

    void addSession(backend::SessionClient* client, const QString& name);
    void removeSession(backend::SessionClient* client);
    backend::SessionClient* currentSession() const;
    void setCurrentSession(backend::SessionClient* client);
    
    QList<backend::SessionClient*> sessions() const;
    QString sessionName(backend::SessionClient* client) const;

signals:
    void sessionAdded(backend::SessionClient* client);
    void sessionRemoved(backend::SessionClient* client);
    void currentSessionChanged(backend::SessionClient* client);

private:
    QHash<backend::SessionClient*, QString> sessions_;
    backend::SessionClient* currentSession_ = nullptr;
};

} // namespace scratchrobin::ui
