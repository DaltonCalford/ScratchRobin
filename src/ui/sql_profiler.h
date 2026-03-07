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
class QLineEdit;
class QSplitter;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QDateTimeEdit;
class QChartView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief SQL Profiler - Performance analysis tool
 * 
 * Analyze SQL query performance:
 * - Real-time query monitoring
 * - Slow query identification
 * - Execution statistics
 * - Resource usage tracking
 * - Performance reports
 */

// ============================================================================
// Profile Data Types
// ============================================================================
struct QueryProfile {
    int id;
    QString query;
    QString normalizedQuery;
    QString database;
    QString user;
    QString client;
    QDateTime startTime;
    QDateTime endTime;
    double duration = 0; // milliseconds
    double planningTime = 0;
    double executionTime = 0;
    int rowsAffected = 0;
    int rowsReturned = 0;
    int calls = 0;
    double totalTime = 0;
    double avgTime = 0;
    double minTime = 0;
    double maxTime = 0;
    double stdDevTime = 0;
    int sharedBlksHit = 0;
    int sharedBlksRead = 0;
    int sharedBlksDirtied = 0;
    int sharedBlksWritten = 0;
    int tempBlksRead = 0;
    int tempBlksWritten = 0;
    bool isSlow = false;
};

struct ProfileSession {
    int sessionId = 0;
    QString name;
    QDateTime startTime;
    QDateTime endTime;
    int queryCount = 0;
    double totalDuration = 0;
    bool isRunning = false;
    QList<QueryProfile> queries;
};

enum class ProfileFilter {
    All,
    SlowQueries,
    FrequentQueries,
    HighIO,
    HighMemory,
    Errors
};

// ============================================================================
// SQL Profiler Panel
// ============================================================================
class SqlProfilerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit SqlProfilerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("SQL Profiler"); }
    QString panelCategory() const override { return "performance"; }

public slots:
    // Session control
    void onStartProfiling();
    void onStopProfiling();
    void onPauseProfiling();
    void onClearData();
    void onSaveSession();
    void onLoadSession();
    
    // Filtering
    void onFilterChanged(int filter);
    void onSearchTextChanged(const QString& text);
    void onTimeRangeChanged();
    void onApplyFilters();
    
    // Analysis
    void onQuerySelected(const QModelIndex& index);
    void onShowQueryDetails();
    void onShowQueryPlan();
    void onOptimizeQuery();
    void onExportSlowQueries();
    void onGenerateReport();
    
    // Statistics
    void onRefreshStats();
    void onViewTopQueries();
    void onViewTrends();
    void onViewResourceUsage();

signals:
    void profilingStarted();
    void profilingStopped();
    void slowQueryDetected(const QueryProfile& query);
    void querySelected(const QString& query);

private:
    void setupUi();
    void setupToolbar();
    void setupQueryTable();
    void setupStatsPanel();
    void setupChartsPanel();
    void setupDetailsPanel();
    void refreshQueryList();
    void updateStatistics();
    void detectSlowQueries();
    QList<QueryProfile> filterQueries();
    
    backend::SessionClient* client_;
    ProfileSession currentSession_;
    QList<QueryProfile> allQueries_;
    ProfileFilter currentFilter_ = ProfileFilter::All;
    QString searchText_;
    
    // UI components
    QTableView* queryTable_ = nullptr;
    QStandardItemModel* queryModel_ = nullptr;
    
    // Stats
    QLabel* totalQueriesLabel_ = nullptr;
    QLabel* slowQueriesLabel_ = nullptr;
    QLabel* avgTimeLabel_ = nullptr;
    QLabel* totalTimeLabel_ = nullptr;
    QLabel* rowsProcessedLabel_ = nullptr;
    QTableView* nodeStatsTable_ = nullptr;
    QStandardItemModel* nodeStatsModel_ = nullptr;
    
    // Charts
    QTabWidget* chartTabs_ = nullptr;
    QWidget* timeChart_ = nullptr;
    QWidget* frequencyChart_ = nullptr;
    QWidget* ioChart_ = nullptr;
    
    // Details
    QTextEdit* queryDetailsEdit_ = nullptr;
    QTableView* statsTable_ = nullptr;
    QStandardItemModel* statsModel_ = nullptr;
    
    // Filters
    QComboBox* filterCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QCheckBox* autoRefreshCheck_ = nullptr;
    QSpinBox* refreshIntervalSpin_ = nullptr;
    
    // Toolbar
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* pauseBtn_ = nullptr;
    QPushButton* clearBtn_ = nullptr;
    
    // Timer for auto-refresh
    class QTimer* refreshTimer_ = nullptr;
};

// ============================================================================
// Profile Report Dialog
// ============================================================================
class ProfileReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileReportDialog(const ProfileSession& session, QWidget* parent = nullptr);

public slots:
    void onGenerateHtml();
    void onGeneratePdf();
    void onExportCsv();
    void onPrint();
    void onSaveReport();

private:
    void setupUi();
    void generateSummary();
    void generateTopQueries();
    void generateTrends();
    
    ProfileSession session_;
    
    QTabWidget* reportTabs_ = nullptr;
    QTextEdit* summaryEdit_ = nullptr;
    QTableView* topQueriesTable_ = nullptr;
    QStandardItemModel* topQueriesModel_ = nullptr;
    QTextEdit* recommendationsEdit_ = nullptr;
};

// ============================================================================
// Query Optimization Suggestions Dialog
// ============================================================================
class OptimizationSuggestionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptimizationSuggestionsDialog(const QueryProfile& query,
                                          backend::SessionClient* client,
                                          QWidget* parent = nullptr);

public slots:
    void onApplySuggestion();
    void onViewQueryPlan();
    void onCreateIndex();

private:
    void setupUi();
    void analyzeQuery();
    void generateSuggestions();
    
    QueryProfile query_;
    backend::SessionClient* client_;
    
    QTextEdit* queryEdit_ = nullptr;
    QTableView* suggestionsTable_ = nullptr;
    QStandardItemModel* suggestionsModel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};

// ============================================================================
// Configure Profiling Dialog
// ============================================================================
class ConfigureProfilingDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureProfilingDialog(QWidget* parent = nullptr);

public slots:
    void onSave();
    void onReset();

private:
    void setupUi();
    void loadSettings();
    
    QCheckBox* trackPlanningTimeCheck_ = nullptr;
    QCheckBox* trackIoTimingCheck_ = nullptr;
    QCheckBox* trackRowsCheck_ = nullptr;
    QCheckBox* trackTempFilesCheck_ = nullptr;
    QSpinBox* slowQueryThresholdSpin_ = nullptr;
    QSpinBox* maxQueriesSpin_ = nullptr;
    QLineEdit* includeFilterEdit_ = nullptr;
    QLineEdit* excludeFilterEdit_ = nullptr;
};

} // namespace scratchrobin::ui
