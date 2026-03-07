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
class QProgressBar;
class QSplitter;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QTreeView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Database Health Check - Diagnostic reports
 * 
 * Comprehensive database health diagnostics:
 * - Connection health
 * - Disk space usage
 * - Index health (bloat, unused indexes)
 * - Table bloat detection
 * - Vacuum/Analyze status
 * - Replication lag
 * - Configuration review
 */

// ============================================================================
// Health Check Types
// ============================================================================
enum class HealthCheckCategory {
    Connections,
    DiskSpace,
    Memory,
    Indexes,
    Tables,
    Vacuum,
    Replication,
    Configuration,
    Security,
    Performance
};

enum class HealthStatus {
    Healthy,
    Warning,
    Critical,
    Info,
    Unknown
};

struct HealthCheckItem {
    int id;
    QString name;
    HealthCheckCategory category;
    HealthStatus status;
    QString message;
    QString details;
    QString recommendation;
    double score = 0; // 0-100
    bool isChecked = true;
};

struct HealthReport {
    QDateTime timestamp;
    int totalChecks = 0;
    int healthyCount = 0;
    int warningCount = 0;
    int criticalCount = 0;
    double overallScore = 0;
    QList<HealthCheckItem> items;
};

struct DiskSpaceInfo {
    QString tablespace;
    double totalSize = 0;
    double usedSize = 0;
    double freeSize = 0;
    double usagePercent = 0;
};

struct IndexHealthInfo {
    QString schema;
    QString table;
    QString index;
    double bloatPercent = 0;
    bool isUnused = false;
    int scanCount = 0;
    double size = 0;
};

// ============================================================================
// Database Health Check Panel
// ============================================================================
class DatabaseHealthCheckPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DatabaseHealthCheckPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Health Check"); }
    QString panelCategory() const override { return "maintenance"; }

public slots:
    // Main actions
    void onRunHealthCheck();
    void onStopHealthCheck();
    void onScheduleCheck();
    void onExportReport();
    void onPrintReport();
    
    // Filtering
    void onFilterByCategory(const QString& category);
    void onFilterByStatus(int status);
    void onShowAllIssues();
    void onShowCriticalOnly();
    void onShowWarningsOnly();
    
    // Item actions
    void onItemSelected(const QModelIndex& index);
    void onFixIssue();
    void onIgnoreIssue();
    void onViewDetails();
    
    // History
    void onViewHistory();
    void onCompareWithPrevious();
    void onLoadPreviousReport();

signals:
    void healthCheckStarted();
    void healthCheckCompleted(const HealthReport& report);
    void issueFound(const HealthCheckItem& item);

private:
    void setupUi();
    void setupToolbar();
    void setupResultsTable();
    void setupSummaryPanel();
    void setupDetailsPanel();
    void runChecks();
    void updateSummary();
    void checkConnections();
    void checkDiskSpace();
    void checkIndexes();
    void checkTables();
    void checkVacuumStatus();
    void checkReplication();
    void checkConfiguration();
    void checkSecurity();
    
    backend::SessionClient* client_;
    HealthReport currentReport_;
    QList<HealthCheckItem> allItems_;
    bool isRunning_ = false;
    
    // UI Components
    QTabWidget* tabs_ = nullptr;
    
    // Summary
    QLabel* overallScoreLabel_ = nullptr;
    QProgressBar* scoreBar_ = nullptr;
    QLabel* statusSummaryLabel_ = nullptr;
    QLabel* healthyCountLabel_ = nullptr;
    QLabel* warningCountLabel_ = nullptr;
    QLabel* criticalCountLabel_ = nullptr;
    
    // Results table
    QTableView* resultsTable_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
    
    // Details
    QTextEdit* detailsEdit_ = nullptr;
    QTextEdit* recommendationEdit_ = nullptr;
    QPushButton* fixBtn_ = nullptr;
    
    // Progress
    QProgressBar* progressBar_ = nullptr;
    QLabel* progressLabel_ = nullptr;
};

// ============================================================================
// Schedule Check Dialog
// ============================================================================
class ScheduleCheckDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScheduleCheckDialog(QWidget* parent = nullptr);

public slots:
    void onSave();
    void onTestSchedule();

private:
    void setupUi();
    
    QCheckBox* enabledCheck_ = nullptr;
    QComboBox* frequencyCombo_ = nullptr;
    QComboBox* dayCombo_ = nullptr;
    QComboBox* hourCombo_ = nullptr;
    QComboBox* minuteCombo_ = nullptr;
    QCheckBox* emailCheck_ = nullptr;
    QLineEdit* emailEdit_ = nullptr;
    QCheckBox* autoFixCheck_ = nullptr;
};

// ============================================================================
// Fix Issue Dialog
// ============================================================================
class FixIssueDialog : public QDialog {
    Q_OBJECT

public:
    explicit FixIssueDialog(const HealthCheckItem& item,
                           backend::SessionClient* client,
                           QWidget* parent = nullptr);

public slots:
    void onPreviewFix();
    void onApplyFix();
    void onScheduleFix();

private:
    void setupUi();
    void generateFixScript();
    
    HealthCheckItem item_;
    backend::SessionClient* client_;
    
    QTextEdit* issueEdit_ = nullptr;
    QTextEdit* fixScriptEdit_ = nullptr;
    QCheckBox* backupCheck_ = nullptr;
    QCheckBox* verifyCheck_ = nullptr;
};

// ============================================================================
// Health Report Dialog
// ============================================================================
class HealthReportDialog : public QDialog {
    Q_OBJECT

public:
    explicit HealthReportDialog(const HealthReport& report, QWidget* parent = nullptr);

public slots:
    void onExportHtml();
    void onExportPdf();
    void onPrint();
    void onEmail();

private:
    void setupUi();
    void generateReport();
    
    HealthReport report_;
    
    QTextEdit* reportEdit_ = nullptr;
    QTableView* summaryTable_ = nullptr;
    QStandardItemModel* summaryModel_ = nullptr;
};

// ============================================================================
// Detailed Analysis Dialog
// ============================================================================
class DetailedAnalysisDialog : public QDialog {
    Q_OBJECT

public:
    explicit DetailedAnalysisDialog(HealthCheckCategory category,
                                   backend::SessionClient* client,
                                   QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExportDetails();

private:
    void setupUi();
    void loadDetails();
    
    HealthCheckCategory category_;
    backend::SessionClient* client_;
    
    QTableView* detailsTable_ = nullptr;
    QStandardItemModel* detailsModel_ = nullptr;
    QTextEdit* analysisEdit_ = nullptr;
};

} // namespace scratchrobin::ui
