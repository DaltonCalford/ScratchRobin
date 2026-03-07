#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QDateTime>
#include <QMetaType>
#include <QFormLayout>

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
class QSpinBox;
class QTimeEdit;
class QListWidget;
class QCalendarWidget;
class QStackedWidget;
class QRadioButton;
class QProgressBar;
class QFormLayout;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Scheduled Jobs Manager - Cron-like scheduler for database tasks
 * 
 * Schedule and manage automated database tasks:
 * - Backup scheduling (full, incremental)
 * - Vacuum/Analyze scheduling
 * - Custom SQL script execution
 * - Maintenance job scheduling
 * - Job execution history and notifications
 */

// ============================================================================
// Schedule Types
// ============================================================================
enum class ScheduleType {
    Once,
    Daily,
    Weekly,
    Monthly,
    Cron
};

enum class JobType {
    Backup,
    Vacuum,
    Analyze,
    VacuumAnalyze,
    Reindex,
    CustomSql,
    ShellCommand
};

enum class JobStatus {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled,
    Disabled
};

// ============================================================================
// Scheduled Job
// ============================================================================
struct ScheduledJob {
    QString id;
    QString name;
    QString description;
    JobType jobType;
    ScheduleType scheduleType;
    QString database;
    
    // Schedule settings
    QTime time;
    int dayOfWeek = 1; // 1 = Monday for weekly
    int dayOfMonth = 1; // 1-31 for monthly
    QString cronExpression; // For custom cron
    
    // Job parameters
    QString backupPath;
    QString backupType;
    QString customSql;
    QString shellCommand;
    QString targetSchema;
    QStringList targetTables;
    bool vacuumFull = false;
    bool vacuumFreeze = false;
    bool analyzeVerbose = false;
    
    // Status
    JobStatus status = JobStatus::Pending;
    QDateTime lastRun;
    QDateTime nextRun;
    int runCount = 0;
    int successCount = 0;
    int failureCount = 0;
    QString lastError;
    bool enabled = true;
    
    // Notification
    bool notifyOnSuccess = false;
    bool notifyOnFailure = true;
    QString notificationEmail;
};

struct JobExecution {
    QString id;
    QString jobId;
    QString jobName;
    QDateTime startTime;
    QDateTime endTime;
    int duration = 0; // seconds
    JobStatus status;
    QString output;
    QString error;
    int rowsAffected = 0;
};

// ============================================================================
// Scheduled Jobs Panel
// ============================================================================
class ScheduledJobsPanel : public DockPanel {
    Q_OBJECT

public:
    explicit ScheduledJobsPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Scheduled Jobs"); }
    QString panelCategory() const override { return "maintenance"; }

public slots:
    // Job management
    void onCreateJob();
    void onEditJob();
    void onDeleteJob();
    void onCloneJob();
    void onRunJobNow();
    void onEnableJob(bool enabled);
    void onStopJob();
    
    // Job list
    void onRefreshJobs();
    void onJobSelected(const QModelIndex& index);
    void onFilterJobs(const QString& text);
    void onFilterByStatus(int index);
    
    // History
    void onViewHistory();
    void onClearHistory();
    void onExportHistory();
    
    // Actions
    void onToggleAllJobs(bool enabled);
    void onRunMaintenanceJobs();

signals:
    void jobCreated(const QString& jobId);
    void jobModified(const QString& jobId);
    void jobDeleted(const QString& jobId);
    void jobExecuted(const QString& jobId, bool success);

private:
    void setupUi();
    void setupJobsList();
    void setupHistoryPanel();
    void setupSchedulePanel();
    void loadJobs();
    void loadHistory();
    void updateJobsTable();
    void updateHistoryTable();
    void updateJobDetails(const ScheduledJob& job);
    void calculateNextRun(ScheduledJob& job);
    
    backend::SessionClient* client_;
    QList<ScheduledJob> jobs_;
    QList<JobExecution> history_;
    
    // UI
    QTabWidget* tabWidget_ = nullptr;
    QTableView* jobsTable_ = nullptr;
    QStandardItemModel* jobsModel_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QComboBox* statusFilter_ = nullptr;
    
    // Details
    QLabel* nameLabel_ = nullptr;
    QLabel* typeLabel_ = nullptr;
    QLabel* scheduleLabel_ = nullptr;
    QLabel* databaseLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* lastRunLabel_ = nullptr;
    QLabel* nextRunLabel_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    
    // History table
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* historyModel_ = nullptr;
};

// ============================================================================
// Job Wizard
// ============================================================================
class JobWizard : public QDialog {
    Q_OBJECT

public:
    explicit JobWizard(ScheduledJob* job, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onNext();
    void onPrevious();
    void onJobTypeChanged(int index);
    void onScheduleTypeChanged(int index);
    void onUpdateSummary();
    void onFinish();
    void onCancel();
    void onTestJob();

private:
    void setupUi();
    void createTypePage();
    void createSchedulePage();
    void createParametersPage();
    void createNotificationsPage();
    void createSummaryPage();
    void updateParameterPage();
    void updateSummaryText();
    
    ScheduledJob* job_;
    backend::SessionClient* client_;
    
    QStackedWidget* pages_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QPushButton* prevBtn_ = nullptr;
    
    // Type page
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* descriptionEdit_ = nullptr;
    QComboBox* jobTypeCombo_ = nullptr;
    QComboBox* databaseCombo_ = nullptr;
    
    // Schedule page
    QComboBox* scheduleTypeCombo_ = nullptr;
    QTimeEdit* timeEdit_ = nullptr;
    QComboBox* dayOfWeekCombo_ = nullptr;
    QSpinBox* dayOfMonthSpin_ = nullptr;
    QLineEdit* cronEdit_ = nullptr;
    QLabel* cronHelpLabel_ = nullptr;
    
    // Parameters page (dynamic based on job type)
    QWidget* parametersPage_ = nullptr;
    QFormLayout* parametersLayout_ = nullptr;
    
    // Backup parameters
    QLineEdit* backupPathEdit_ = nullptr;
    QComboBox* backupTypeCombo_ = nullptr;
    
    // SQL parameters
    QTextEdit* sqlEdit_ = nullptr;
    
    // Shell parameters
    QLineEdit* shellCommandEdit_ = nullptr;
    
    // Vacuum parameters
    QCheckBox* vacuumFullCheck_ = nullptr;
    QCheckBox* vacuumFreezeCheck_ = nullptr;
    QComboBox* vacuumSchemaCombo_ = nullptr;
    QListWidget* vacuumTablesList_ = nullptr;
    
    // Notification page
    QCheckBox* notifySuccessCheck_ = nullptr;
    QCheckBox* notifyFailureCheck_ = nullptr;
    QLineEdit* emailEdit_ = nullptr;
    
    // Summary page
    QTextEdit* summaryEdit_ = nullptr;
};

// ============================================================================
// Job History Dialog
// ============================================================================
class JobHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit JobHistoryDialog(const QString& jobId, const QList<JobExecution>& history, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();
    void onViewDetails();
    void onClearHistory();

private:
    void setupUi();
    void loadHistory();
    
    QString jobId_;
    QList<JobExecution> history_;
    
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* historyModel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};

// ============================================================================
// Job Execution Dialog
// ============================================================================
class JobExecutionDialog : public QDialog {
    Q_OBJECT

public:
    explicit JobExecutionDialog(const ScheduledJob& job, QWidget* parent = nullptr);

public slots:
    void onStart();
    void onStop();
    void onClose();

private:
    void setupUi();
    void executeJob();
    
    ScheduledJob job_;
    bool running_ = false;
    
    QLabel* jobNameLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QTextEdit* outputEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* startBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
};

} // namespace scratchrobin::ui
