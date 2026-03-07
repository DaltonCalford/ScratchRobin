#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QTableView;
class QTextEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QLabel;
class QLineEdit;
class QCheckBox;
class QGroupBox;
class QTabWidget;
class QSplitter;
class QProgressBar;
class QSpinBox;
class QListWidget;
class QTreeView;
class QProgressDialog;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Vacuum/Analyze Maintenance Tool
 * 
 * Database maintenance operations:
 * - Vacuum tables to reclaim storage
 * - Analyze tables to update statistics
 * - Vacuum Analyze for both operations
 * - Reindex for index optimization
 * - Visual table bloat analysis
 * - Dead tuple monitoring
 */

// ============================================================================
// Table Statistics
// ============================================================================
struct TableStats {
    QString schema;
    QString tableName;
    qint64 totalSize = 0;       // Total table size
    qint64 liveTuples = 0;      // Estimated live tuples
    qint64 deadTuples = 0;      // Estimated dead tuples
    float deadRatio = 0;        // Dead tuple ratio
    QDateTime lastVacuum;       // Last vacuum time
    QDateTime lastAutovacuum;   // Last autovacuum time
    QDateTime lastAnalyze;      // Last analyze time
    QDateTime lastAutoanalyze;  // Last autoanalyze time
    int vacuumCount = 0;        // Manual vacuum count
    int autovacuumCount = 0;    // Autovacuum count
    bool needsVacuum = false;   // Flag if vacuum is recommended
    bool needsAnalyze = false;  // Flag if analyze is recommended
};

struct MaintenanceOperation {
    QString id;
    QString type; // vacuum, analyze, reindex, vacuum_analyze
    QString target; // table name or "all"
    QString schema;
    QDateTime startTime;
    QDateTime endTime;
    int duration = 0; // seconds
    QString status; // pending, running, completed, failed
    QString errorMessage;
    qint64 tuplesProcessed = 0;
    qint64 spaceReclaimed = 0; // bytes
};

// ============================================================================
// Vacuum/Analyze Tool Panel
// ============================================================================
class VacuumAnalyzePanel : public DockPanel {
    Q_OBJECT

public:
    explicit VacuumAnalyzePanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Vacuum/Analyze"); }
    QString panelCategory() const override { return "maintenance"; }

public slots:
    // Operations
    void onVacuum();
    void onVacuumFull();
    void onAnalyze();
    void onVacuumAnalyze();
    void onReindex();
    void onStopOperation();
    
    // Selection
    void onSelectAllTables();
    void onDeselectAllTables();
    void onSelectBloatedTables();
    void onTableSelected(const QModelIndex& index);
    
    // Refresh
    void onRefreshStats();
    void onAutoRefreshToggled(bool enabled);
    
    // Scheduling
    void onScheduleMaintenance();
    void onRunMaintenanceNow();
    
    // Analysis
    void onViewBloatReport();
    void onViewDeadTuples();
    void onViewActivity();
    void onExportStats();

signals:
    void operationStarted(const QString& operationId);
    void operationCompleted(const QString& operationId, bool success);
    void tableBloatDetected(const QString& tableName, float bloatRatio);

private:
    void setupUi();
    void setupStatsTable();
    void setupOperationsPanel();
    void loadTableStats();
    void updateStatsTable();
    void updateTableDetails(const TableStats& stats);
    void runMaintenance(const QString& operation, const QStringList& tables);
    QStringList selectedTables() const;
    
    backend::SessionClient* client_;
    QList<TableStats> tableStats_;
    QList<MaintenanceOperation> operations_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTableView* statsTable_ = nullptr;
    QStandardItemModel* statsModel_ = nullptr;
    
    // Filters
    QComboBox* schemaFilter_ = nullptr;
    QComboBox* sortByCombo_ = nullptr;
    QCheckBox* showBloatedOnlyCheck_ = nullptr;
    QCheckBox* autoRefreshCheck_ = nullptr;
    QSpinBox* refreshIntervalSpin_ = nullptr;
    
    // Details
    QLabel* tableNameLabel_ = nullptr;
    QLabel* sizeLabel_ = nullptr;
    QLabel* liveTuplesLabel_ = nullptr;
    QLabel* deadTuplesLabel_ = nullptr;
    QLabel* deadRatioLabel_ = nullptr;
    QLabel* lastVacuumLabel_ = nullptr;
    QLabel* lastAnalyzeLabel_ = nullptr;
    QProgressBar* bloatBar_ = nullptr;
    
    // Operations log
    QTextEdit* logEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

// ============================================================================
// Maintenance Dialog
// ============================================================================
class MaintenanceDialog : public QDialog {
    Q_OBJECT

public:
    explicit MaintenanceDialog(const QString& operation, const QStringList& tables, 
                               backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onStart();
    void onStop();
    void onClose();

private:
    void setupUi();
    void executeMaintenance();
    void updateProgress();
    
    QString operation_;
    QStringList tables_;
    backend::SessionClient* client_;
    bool running_ = false;
    int currentTable_ = 0;
    
    QLabel* operationLabel_ = nullptr;
    QProgressBar* overallProgress_ = nullptr;
    QProgressBar* tableProgress_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QLabel* currentTableLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    
    qint64 totalSpaceReclaimed_ = 0;
    int tablesProcessed_ = 0;
};

// ============================================================================
// Bloat Report Dialog
// ============================================================================
class BloatReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit BloatReportDialog(const QList<TableStats>& stats, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();
    void onVacuumSelected();

private:
    void setupUi();
    void loadReport();
    void calculateBloat();
    
    QList<TableStats> stats_;
    
    QTableView* reportTable_ = nullptr;
    QStandardItemModel* reportModel_ = nullptr;
    QTextEdit* summaryEdit_ = nullptr;
    QLabel* totalBloatLabel_ = nullptr;
    QLabel* tablesNeedingVacuumLabel_ = nullptr;
};

// ============================================================================
// Dead Tuples Monitor
// ============================================================================
class DeadTuplesDialog : public QDialog {
    Q_OBJECT

public:
    explicit DeadTuplesDialog(const QList<TableStats>& stats, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onAutoVacuumCheck();
    void onVacuumWorstTables();

private:
    void setupUi();
    void loadStats();
    
    QList<TableStats> stats_;
    
    QTableView* statsTable_ = nullptr;
    QStandardItemModel* statsModel_ = nullptr;
    QTextEdit* recommendationsEdit_ = nullptr;
    QLabel* totalDeadTuplesLabel_ = nullptr;
    QLabel* worstTableLabel_ = nullptr;
};

// ============================================================================
// Reindex Dialog
// ============================================================================
class ReindexDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReindexDialog(const QStringList& tables, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onStart();
    void onStop();
    void onClose();

private:
    void setupUi();
    void executeReindex();
    
    QStringList tables_;
    backend::SessionClient* client_;
    bool running_ = false;
    
    QTableView* indexTable_ = nullptr;
    QStandardItemModel* indexModel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* logEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
};

} // namespace scratchrobin::ui
