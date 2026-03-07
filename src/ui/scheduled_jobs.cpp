#include "scheduled_jobs.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QProgressBar>
#include <QSpinBox>
#include <QTimeEdit>
#include <QListWidget>
#include <QCalendarWidget>
#include <QStackedWidget>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Scheduled Jobs Panel
// ============================================================================

ScheduledJobsPanel::ScheduledJobsPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("scheduled_jobs", parent)
    , client_(client) {
    setupUi();
    loadJobs();
    loadHistory();
}

void ScheduledJobsPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* createBtn = new QPushButton(tr("New Job..."), this);
    connect(createBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onCreateJob);
    toolbarLayout->addWidget(createBtn);
    
    auto* runBtn = new QPushButton(tr("Run Now"), this);
    connect(runBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onRunJobNow);
    toolbarLayout->addWidget(runBtn);
    
    auto* enableBtn = new QPushButton(tr("Enable"), this);
    connect(enableBtn, &QPushButton::clicked, this, [this]() { onEnableJob(true); });
    toolbarLayout->addWidget(enableBtn);
    
    auto* disableBtn = new QPushButton(tr("Disable"), this);
    connect(disableBtn, &QPushButton::clicked, this, [this]() { onEnableJob(false); });
    toolbarLayout->addWidget(disableBtn);
    
    toolbarLayout->addStretch();
    
    statusFilter_ = new QComboBox(this);
    statusFilter_->addItems({tr("All Status"), tr("Enabled"), tr("Disabled"), tr("Running"), tr("Failed")});
    connect(statusFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &ScheduledJobsPanel::onFilterByStatus);
    toolbarLayout->addWidget(statusFilter_);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search jobs..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &ScheduledJobsPanel::onFilterJobs);
    toolbarLayout->addWidget(searchEdit_);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onRefreshJobs);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Tab widget
    tabWidget_ = new QTabWidget(this);
    
    // Jobs tab
    auto* jobsWidget = new QWidget(this);
    auto* jobsLayout = new QVBoxLayout(jobsWidget);
    jobsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    jobsTable_ = new QTableView(this);
    jobsModel_ = new QStandardItemModel(this);
    jobsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Schedule"), tr("Database"), tr("Next Run"), tr("Status")});
    jobsTable_->setModel(jobsModel_);
    jobsTable_->setAlternatingRowColors(true);
    jobsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(jobsTable_, &QTableView::clicked, this, &ScheduledJobsPanel::onJobSelected);
    splitter->addWidget(jobsTable_);
    
    // Job details
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QFormLayout(detailsWidget);
    
    nameLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Name:"), nameLabel_);
    
    typeLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Type:"), typeLabel_);
    
    scheduleLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Schedule:"), scheduleLabel_);
    
    databaseLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Database:"), databaseLabel_);
    
    statusLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Status:"), statusLabel_);
    
    lastRunLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Last Run:"), lastRunLabel_);
    
    nextRunLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Next Run:"), nextRunLabel_);
    
    statsLabel_ = new QLabel(this);
    detailsLayout->addRow(tr("Statistics:"), statsLabel_);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({400, 250});
    
    jobsLayout->addWidget(splitter, 1);
    
    tabWidget_->addTab(jobsWidget, tr("Jobs"));
    
    // History tab
    auto* historyWidget = new QWidget(this);
    auto* historyLayout = new QVBoxLayout(historyWidget);
    historyLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* historyToolbar = new QHBoxLayout();
    auto* viewDetailsBtn = new QPushButton(tr("View Details"), this);
    connect(viewDetailsBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onViewHistory);
    historyToolbar->addWidget(viewDetailsBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear History"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onClearHistory);
    historyToolbar->addWidget(clearBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onExportHistory);
    historyToolbar->addWidget(exportBtn);
    
    historyToolbar->addStretch();
    historyLayout->addLayout(historyToolbar);
    
    historyTable_ = new QTableView(this);
    historyModel_ = new QStandardItemModel(this);
    historyModel_->setHorizontalHeaderLabels({tr("Job"), tr("Start Time"), tr("Duration"), tr("Status"), tr("Output")});
    historyTable_->setModel(historyModel_);
    historyTable_->setAlternatingRowColors(true);
    historyLayout->addWidget(historyTable_, 1);
    
    tabWidget_->addTab(historyWidget, tr("History"));
    
    // Schedule tab
    auto* scheduleWidget = new QWidget(this);
    auto* scheduleLayout = new QVBoxLayout(scheduleWidget);
    scheduleLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* scheduleInfo = new QLabel(tr("<h3>Schedule Overview</h3>"
                                       "<p>View upcoming scheduled jobs and maintenance windows.</p>"), this);
    scheduleInfo->setWordWrap(true);
    scheduleLayout->addWidget(scheduleInfo);
    
    auto* maintenanceBtn = new QPushButton(tr("Run Maintenance Jobs Now"), this);
    connect(maintenanceBtn, &QPushButton::clicked, this, &ScheduledJobsPanel::onRunMaintenanceJobs);
    scheduleLayout->addWidget(maintenanceBtn);
    
    scheduleLayout->addStretch();
    
    tabWidget_->addTab(scheduleWidget, tr("Schedule"));
    
    mainLayout->addWidget(tabWidget_, 1);
}

void ScheduledJobsPanel::loadJobs() {
    jobs_.clear();
    
    // Simulate loading jobs
    QStringList jobNames = {"Daily Backup", "Weekly Vacuum", "Hourly Analyze", "Monthly Reindex"};
    QStringList types = {"Backup", "Vacuum", "Analyze", "Reindex"};
    QStringList schedules = {"Daily at 02:00", "Weekly on Sun at 03:00", "Hourly", "Monthly on 1st at 04:00"};
    QStringList databases = {"scratchbird", "scratchbird", "scratchbird", "scratchbird"};
    
    for (int i = 0; i < jobNames.size(); ++i) {
        ScheduledJob job;
        job.id = QString::number(i);
        job.name = jobNames[i];
        job.jobType = static_cast<JobType>(i);
        job.scheduleType = static_cast<ScheduleType>(i % 4);
        job.database = databases[i];
        job.enabled = true;
        job.status = JobStatus::Pending;
        job.lastRun = QDateTime::currentDateTime().addDays(-1);
        job.nextRun = QDateTime::currentDateTime().addDays(1);
        job.runCount = 10 + i * 5;
        job.successCount = job.runCount;
        job.failureCount = 0;
        jobs_.append(job);
    }
    
    updateJobsTable();
}

void ScheduledJobsPanel::loadHistory() {
    history_.clear();
    
    // Simulate loading history
    for (int i = 0; i < 10; ++i) {
        JobExecution exec;
        exec.id = QString::number(i);
        exec.jobId = QString::number(i % 4);
        exec.jobName = jobs_[i % 4].name;
        exec.startTime = QDateTime::currentDateTime().addSecs(-i * 3600);
        exec.endTime = exec.startTime.addSecs(300);
        exec.duration = 300;
        exec.status = (i == 5) ? JobStatus::Failed : JobStatus::Completed;
        exec.output = tr("Job completed successfully");
        history_.append(exec);
    }
    
    updateHistoryTable();
}

void ScheduledJobsPanel::updateJobsTable() {
    jobsModel_->clear();
    jobsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Schedule"), tr("Database"), tr("Next Run"), tr("Status")});
    
    for (const auto& job : jobs_) {
        QString typeStr;
        switch (job.jobType) {
            case JobType::Backup: typeStr = tr("Backup"); break;
            case JobType::Vacuum: typeStr = tr("Vacuum"); break;
            case JobType::Analyze: typeStr = tr("Analyze"); break;
            case JobType::VacuumAnalyze: typeStr = tr("Vacuum+Analyze"); break;
            case JobType::Reindex: typeStr = tr("Reindex"); break;
            case JobType::CustomSql: typeStr = tr("Custom SQL"); break;
            case JobType::ShellCommand: typeStr = tr("Shell"); break;
        }
        
        QString scheduleStr;
        switch (job.scheduleType) {
            case ScheduleType::Once: scheduleStr = tr("Once"); break;
            case ScheduleType::Daily: scheduleStr = tr("Daily %1").arg(job.time.toString("hh:mm")); break;
            case ScheduleType::Weekly: scheduleStr = tr("Weekly %1").arg(job.dayOfWeek); break;
            case ScheduleType::Monthly: scheduleStr = tr("Monthly %1").arg(job.dayOfMonth); break;
            case ScheduleType::Cron: scheduleStr = job.cronExpression; break;
        }
        
        QString statusStr;
        switch (job.status) {
            case JobStatus::Pending: statusStr = tr("Pending"); break;
            case JobStatus::Running: statusStr = tr("Running"); break;
            case JobStatus::Completed: statusStr = tr("Completed"); break;
            case JobStatus::Failed: statusStr = tr("Failed"); break;
            case JobStatus::Cancelled: statusStr = tr("Cancelled"); break;
            case JobStatus::Disabled: statusStr = tr("Disabled"); break;
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(job.name);
        row << new QStandardItem(typeStr);
        row << new QStandardItem(scheduleStr);
        row << new QStandardItem(job.database);
        row << new QStandardItem(job.nextRun.toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(statusStr);
        jobsModel_->appendRow(row);
    }
}

void ScheduledJobsPanel::updateHistoryTable() {
    historyModel_->clear();
    historyModel_->setHorizontalHeaderLabels({tr("Job"), tr("Start Time"), tr("Duration"), tr("Status"), tr("Output")});
    
    for (const auto& exec : history_) {
        QString statusStr;
        switch (exec.status) {
            case JobStatus::Completed: statusStr = tr("Success"); break;
            case JobStatus::Failed: statusStr = tr("Failed"); break;
            case JobStatus::Cancelled: statusStr = tr("Cancelled"); break;
            default: statusStr = tr("Unknown");
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(exec.jobName);
        row << new QStandardItem(exec.startTime.toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(QString("%1s").arg(exec.duration));
        row << new QStandardItem(statusStr);
        row << new QStandardItem(exec.output.left(50));
        historyModel_->appendRow(row);
    }
}

void ScheduledJobsPanel::updateJobDetails(const ScheduledJob& job) {
    nameLabel_->setText(job.name);
    
    QString typeStr;
    switch (job.jobType) {
        case JobType::Backup: typeStr = tr("Backup"); break;
        case JobType::Vacuum: typeStr = tr("Vacuum"); break;
        case JobType::Analyze: typeStr = tr("Analyze"); break;
        case JobType::VacuumAnalyze: typeStr = tr("Vacuum+Analyze"); break;
        case JobType::Reindex: typeStr = tr("Reindex"); break;
        case JobType::CustomSql: typeStr = tr("Custom SQL"); break;
        case JobType::ShellCommand: typeStr = tr("Shell Command"); break;
    }
    typeLabel_->setText(typeStr);
    
    QString scheduleStr;
    switch (job.scheduleType) {
        case ScheduleType::Daily: scheduleStr = tr("Daily at %1").arg(job.time.toString("hh:mm")); break;
        case ScheduleType::Weekly: scheduleStr = tr("Weekly on day %1 at %2").arg(job.dayOfWeek).arg(job.time.toString("hh:mm")); break;
        case ScheduleType::Monthly: scheduleStr = tr("Monthly on day %1 at %2").arg(job.dayOfMonth).arg(job.time.toString("hh:mm")); break;
        default: scheduleStr = tr("Custom");
    }
    scheduleLabel_->setText(scheduleStr);
    
    databaseLabel_->setText(job.database);
    
    QString statusStr = job.enabled ? tr("Enabled") : tr("Disabled");
    statusLabel_->setText(statusStr);
    
    lastRunLabel_->setText(job.lastRun.isValid() ? job.lastRun.toString() : tr("Never"));
    nextRunLabel_->setText(job.nextRun.isValid() ? job.nextRun.toString() : tr("Not scheduled"));
    
    statsLabel_->setText(tr("Runs: %1 | Success: %2 | Failures: %3")
                        .arg(job.runCount).arg(job.successCount).arg(job.failureCount));
}

void ScheduledJobsPanel::calculateNextRun(ScheduledJob& job) {
    QDateTime now = QDateTime::currentDateTime();
    QDateTime next = now;
    
    switch (job.scheduleType) {
        case ScheduleType::Daily:
            next.setTime(job.time);
            if (next <= now) {
                next = next.addDays(1);
            }
            break;
        case ScheduleType::Weekly:
            next.setTime(job.time);
            while (next.date().dayOfWeek() != job.dayOfWeek || next <= now) {
                next = next.addDays(1);
            }
            break;
        case ScheduleType::Monthly:
            next.setTime(job.time);
            next.setDate(QDate(now.date().year(), now.date().month(), job.dayOfMonth));
            if (next <= now) {
                next = next.addMonths(1);
            }
            break;
        default:
            break;
    }
    
    job.nextRun = next;
}

void ScheduledJobsPanel::onCreateJob() {
    ScheduledJob newJob;
    JobWizard wizard(&newJob, client_, this);
    if (wizard.exec() == QDialog::Accepted) {
        calculateNextRun(newJob);
        jobs_.append(newJob);
        updateJobsTable();
        emit jobCreated(newJob.id);
    }
}

void ScheduledJobsPanel::onEditJob() {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    JobWizard wizard(&jobs_[index.row()], client_, this);
    wizard.exec();
    calculateNextRun(jobs_[index.row()]);
    updateJobsTable();
    emit jobModified(jobs_[index.row()].id);
}

void ScheduledJobsPanel::onDeleteJob() {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    QString jobId = jobs_[index.row()].id;
    auto reply = QMessageBox::warning(this, tr("Delete Job"),
        tr("Are you sure you want to delete job '%1'?").arg(jobs_[index.row()].name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        jobs_.removeAt(index.row());
        updateJobsTable();
        emit jobDeleted(jobId);
    }
}

void ScheduledJobsPanel::onCloneJob() {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    ScheduledJob clone = jobs_[index.row()];
    clone.id = QString::number(jobs_.size());
    clone.name += " (Copy)";
    clone.runCount = 0;
    clone.successCount = 0;
    clone.failureCount = 0;
    
    JobWizard wizard(&clone, client_, this);
    if (wizard.exec() == QDialog::Accepted) {
        jobs_.append(clone);
        updateJobsTable();
        emit jobCreated(clone.id);
    }
}

void ScheduledJobsPanel::onRunJobNow() {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    JobExecutionDialog dialog(jobs_[index.row()], this);
    dialog.exec();
    
    // Update history
    loadHistory();
}

void ScheduledJobsPanel::onEnableJob(bool enabled) {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    jobs_[index.row()].enabled = enabled;
    jobs_[index.row()].status = enabled ? JobStatus::Pending : JobStatus::Disabled;
    updateJobsTable();
}

void ScheduledJobsPanel::onStopJob() {
    // Stop running job
}

void ScheduledJobsPanel::onRefreshJobs() {
    loadJobs();
}

void ScheduledJobsPanel::onJobSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    updateJobDetails(jobs_[index.row()]);
}

void ScheduledJobsPanel::onFilterJobs(const QString& text) {
    for (int i = 0; i < jobsModel_->rowCount(); ++i) {
        auto* item = jobsModel_->item(i, 0);
        bool match = item->text().contains(text, Qt::CaseInsensitive);
        jobsTable_->setRowHidden(i, !match);
    }
}

void ScheduledJobsPanel::onFilterByStatus(int index) {
    Q_UNUSED(index)
    // Apply status filter
}

void ScheduledJobsPanel::onViewHistory() {
    auto index = jobsTable_->currentIndex();
    if (!index.isValid() || index.row() >= jobs_.size()) return;
    
    QString jobId = jobs_[index.row()].id;
    QList<JobExecution> jobHistory;
    for (const auto& exec : history_) {
        if (exec.jobId == jobId) {
            jobHistory.append(exec);
        }
    }
    
    JobHistoryDialog dialog(jobId, jobHistory, this);
    dialog.exec();
}

void ScheduledJobsPanel::onClearHistory() {
    auto reply = QMessageBox::question(this, tr("Clear History"),
        tr("Clear all job history?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        history_.clear();
        updateHistoryTable();
    }
}

void ScheduledJobsPanel::onExportHistory() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export History"),
        QString(),
        tr("CSV Files (*.csv);;JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("History exported."));
    }
}

void ScheduledJobsPanel::onToggleAllJobs(bool enabled) {
    for (auto& job : jobs_) {
        job.enabled = enabled;
        job.status = enabled ? JobStatus::Pending : JobStatus::Disabled;
    }
    updateJobsTable();
}

void ScheduledJobsPanel::onRunMaintenanceJobs() {
    QMessageBox::information(this, tr("Maintenance"),
        tr("Maintenance jobs would be executed now."));
}

// ============================================================================
// Job Wizard
// ============================================================================

JobWizard::JobWizard(ScheduledJob* job, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , job_(job)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Job Wizard"));
    resize(550, 450);
}

void JobWizard::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    pages_ = new QStackedWidget(this);
    
    createTypePage();
    createSchedulePage();
    createParametersPage();
    createNotificationsPage();
    createSummaryPage();
    
    layout->addWidget(pages_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* testBtn = new QPushButton(tr("Test"), this);
    connect(testBtn, &QPushButton::clicked, this, &JobWizard::onTestJob);
    btnLayout->addWidget(testBtn);
    
    prevBtn_ = new QPushButton(tr("< Back"), this);
    connect(prevBtn_, &QPushButton::clicked, this, &JobWizard::onPrevious);
    btnLayout->addWidget(prevBtn_);
    
    nextBtn_ = new QPushButton(tr("Next >"), this);
    connect(nextBtn_, &QPushButton::clicked, this, &JobWizard::onNext);
    btnLayout->addWidget(nextBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &JobWizard::onCancel);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    prevBtn_->setEnabled(false);
}

void JobWizard::createTypePage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    nameEdit_ = new QLineEdit(job_->name, this);
    layout->addRow(tr("Job Name:"), nameEdit_);
    
    descriptionEdit_ = new QTextEdit(job_->description, this);
    descriptionEdit_->setMaximumHeight(60);
    layout->addRow(tr("Description:"), descriptionEdit_);
    
    jobTypeCombo_ = new QComboBox(this);
    jobTypeCombo_->addItem(tr("Backup"), static_cast<int>(JobType::Backup));
    jobTypeCombo_->addItem(tr("Vacuum"), static_cast<int>(JobType::Vacuum));
    jobTypeCombo_->addItem(tr("Analyze"), static_cast<int>(JobType::Analyze));
    jobTypeCombo_->addItem(tr("Vacuum + Analyze"), static_cast<int>(JobType::VacuumAnalyze));
    jobTypeCombo_->addItem(tr("Reindex"), static_cast<int>(JobType::Reindex));
    jobTypeCombo_->addItem(tr("Custom SQL"), static_cast<int>(JobType::CustomSql));
    jobTypeCombo_->addItem(tr("Shell Command"), static_cast<int>(JobType::ShellCommand));
    connect(jobTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JobWizard::onJobTypeChanged);
    layout->addRow(tr("Job Type:"), jobTypeCombo_);
    
    databaseCombo_ = new QComboBox(this);
    databaseCombo_->addItems({"scratchbird", "postgres", "template1"});
    layout->addRow(tr("Database:"), databaseCombo_);
    
    pages_->addWidget(page);
}

void JobWizard::createSchedulePage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    scheduleTypeCombo_ = new QComboBox(this);
    scheduleTypeCombo_->addItem(tr("Once"), static_cast<int>(ScheduleType::Once));
    scheduleTypeCombo_->addItem(tr("Daily"), static_cast<int>(ScheduleType::Daily));
    scheduleTypeCombo_->addItem(tr("Weekly"), static_cast<int>(ScheduleType::Weekly));
    scheduleTypeCombo_->addItem(tr("Monthly"), static_cast<int>(ScheduleType::Monthly));
    scheduleTypeCombo_->addItem(tr("Custom (Cron)"), static_cast<int>(ScheduleType::Cron));
    connect(scheduleTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JobWizard::onScheduleTypeChanged);
    layout->addRow(tr("Schedule Type:"), scheduleTypeCombo_);
    
    timeEdit_ = new QTimeEdit(this);
    timeEdit_->setTime(QTime(2, 0));
    layout->addRow(tr("Time:"), timeEdit_);
    
    dayOfWeekCombo_ = new QComboBox(this);
    dayOfWeekCombo_->addItems({tr("Monday"), tr("Tuesday"), tr("Wednesday"), tr("Thursday"), 
                               tr("Friday"), tr("Saturday"), tr("Sunday")});
    layout->addRow(tr("Day of Week:"), dayOfWeekCombo_);
    
    dayOfMonthSpin_ = new QSpinBox(this);
    dayOfMonthSpin_->setRange(1, 31);
    dayOfMonthSpin_->setValue(1);
    layout->addRow(tr("Day of Month:"), dayOfMonthSpin_);
    
    cronEdit_ = new QLineEdit(this);
    cronEdit_->setPlaceholderText(tr("0 2 * * *"));
    layout->addRow(tr("Cron Expression:"), cronEdit_);
    
    cronHelpLabel_ = new QLabel(tr("Format: minute hour day month weekday"), this);
    cronHelpLabel_->setStyleSheet("color: gray;");
    layout->addRow(cronHelpLabel_);
    
    pages_->addWidget(page);
}

void JobWizard::createParametersPage() {
    parametersPage_ = new QWidget(this);
    parametersLayout_ = new QFormLayout(parametersPage_);
    
    pages_->addWidget(parametersPage_);
    
    updateParameterPage();
}

void JobWizard::updateParameterPage() {
    // Clear existing widgets
    while (parametersLayout_->count() > 0) {
        delete parametersLayout_->takeAt(0)->widget();
    }
    
    JobType type = static_cast<JobType>(jobTypeCombo_->currentData().toInt());
    
    switch (type) {
        case JobType::Backup: {
            auto* pathLayout = new QHBoxLayout();
            backupPathEdit_ = new QLineEdit(this);
            pathLayout->addWidget(backupPathEdit_);
            
            auto* browseBtn = new QPushButton(tr("Browse..."), this);
            pathLayout->addWidget(browseBtn);
            
            auto* pathWidget = new QWidget();
            pathWidget->setLayout(pathLayout);
            parametersLayout_->addRow(tr("Backup Path:"), pathWidget);
            
            backupTypeCombo_ = new QComboBox(this);
            backupTypeCombo_->addItems({tr("Full"), tr("Incremental"), tr("Differential")});
            parametersLayout_->addRow(tr("Backup Type:"), backupTypeCombo_);
            break;
        }
        case JobType::Vacuum:
        case JobType::VacuumAnalyze: {
            vacuumFullCheck_ = new QCheckBox(tr("Full vacuum (rewrite entire table)"), this);
            parametersLayout_->addWidget(vacuumFullCheck_);
            
            vacuumFreezeCheck_ = new QCheckBox(tr("Freeze tuples"), this);
            parametersLayout_->addWidget(vacuumFreezeCheck_);
            
            vacuumTablesList_ = new QListWidget(this);
            vacuumTablesList_->addItems({"customers", "orders", "products"});
            vacuumTablesList_->setSelectionMode(QAbstractItemView::MultiSelection);
            parametersLayout_->addRow(tr("Tables (empty for all):"), vacuumTablesList_);
            break;
        }
        case JobType::CustomSql: {
            sqlEdit_ = new QTextEdit(this);
            sqlEdit_->setPlaceholderText(tr("Enter SQL commands here..."));
            parametersLayout_->addRow(tr("SQL Commands:"), sqlEdit_);
            break;
        }
        case JobType::ShellCommand: {
            shellCommandEdit_ = new QLineEdit(this);
            parametersLayout_->addRow(tr("Shell Command:"), shellCommandEdit_);
            break;
        }
        default:
            break;
    }
}

void JobWizard::createNotificationsPage() {
    auto* page = new QWidget(this);
    auto* layout = new QFormLayout(page);
    
    notifySuccessCheck_ = new QCheckBox(tr("Notify on success"), this);
    layout->addRow(notifySuccessCheck_);
    
    notifyFailureCheck_ = new QCheckBox(tr("Notify on failure"), this);
    notifyFailureCheck_->setChecked(true);
    layout->addRow(notifyFailureCheck_);
    
    emailEdit_ = new QLineEdit(this);
    emailEdit_->setPlaceholderText(tr("email@example.com"));
    layout->addRow(tr("Notification Email:"), emailEdit_);
    
    pages_->addWidget(page);
}

void JobWizard::createSummaryPage() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    
    layout->addWidget(new QLabel(tr("Job Summary:"), this));
    
    summaryEdit_ = new QTextEdit(this);
    summaryEdit_->setReadOnly(true);
    layout->addWidget(summaryEdit_, 1);
    
    pages_->addWidget(page);
}

void JobWizard::updateSummaryText() {
    if (!summaryEdit_) return;
    
    QString summary = tr("Job Name: %1\n").arg(nameEdit_->text());
    summary += tr("Type: %1\n").arg(jobTypeCombo_->currentText());
    summary += tr("Database: %1\n").arg(databaseCombo_->currentText());
    summary += tr("Schedule: %1 at %2\n")
        .arg(scheduleTypeCombo_->currentText())
        .arg(timeEdit_->time().toString());
    
    summaryEdit_->setText(summary);
}

void JobWizard::onJobTypeChanged(int index) {
    Q_UNUSED(index)
    updateParameterPage();
}

void JobWizard::onScheduleTypeChanged(int index) {
    ScheduleType type = static_cast<ScheduleType>(scheduleTypeCombo_->itemData(index).toInt());
    
    timeEdit_->setEnabled(type != ScheduleType::Once);
    dayOfWeekCombo_->setEnabled(type == ScheduleType::Weekly);
    dayOfMonthSpin_->setEnabled(type == ScheduleType::Monthly);
    cronEdit_->setEnabled(type == ScheduleType::Cron);
}

void JobWizard::onUpdateSummary() {
    updateSummaryText();
}

void JobWizard::onNext() {
    int current = pages_->currentIndex();
    if (current < pages_->count() - 1) {
        pages_->setCurrentIndex(current + 1);
        prevBtn_->setEnabled(true);
        
        if (current + 1 == pages_->count() - 1) {
            nextBtn_->setText(tr("Finish"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &JobWizard::onNext);
            connect(nextBtn_, &QPushButton::clicked, this, &JobWizard::onFinish);
            updateSummaryText();
        }
    }
}

void JobWizard::onPrevious() {
    int current = pages_->currentIndex();
    if (current > 0) {
        pages_->setCurrentIndex(current - 1);
        
        if (current == pages_->count() - 1) {
            nextBtn_->setText(tr("Next >"));
            disconnect(nextBtn_, &QPushButton::clicked, this, &JobWizard::onFinish);
            connect(nextBtn_, &QPushButton::clicked, this, &JobWizard::onNext);
        }
        
        if (current - 1 == 0) {
            prevBtn_->setEnabled(false);
        }
    }
}

void JobWizard::onFinish() {
    job_->name = nameEdit_->text();
    job_->description = descriptionEdit_->toPlainText();
    job_->jobType = static_cast<JobType>(jobTypeCombo_->currentData().toInt());
    job_->scheduleType = static_cast<ScheduleType>(scheduleTypeCombo_->currentData().toInt());
    job_->database = databaseCombo_->currentText();
    job_->time = timeEdit_->time();
    job_->dayOfWeek = dayOfWeekCombo_->currentIndex() + 1;
    job_->dayOfMonth = dayOfMonthSpin_->value();
    job_->cronExpression = cronEdit_->text();
    job_->notifyOnSuccess = notifySuccessCheck_->isChecked();
    job_->notifyOnFailure = notifyFailureCheck_->isChecked();
    job_->notificationEmail = emailEdit_->text();
    job_->enabled = true;
    
    accept();
}

void JobWizard::onCancel() {
    reject();
}

void JobWizard::onTestJob() {
    QMessageBox::information(this, tr("Test Job"),
        tr("Job test would be executed here.\nThis would run the job immediately without scheduling."));
}

// ============================================================================
// Job History Dialog
// ============================================================================

JobHistoryDialog::JobHistoryDialog(const QString& jobId, const QList<JobExecution>& history, QWidget* parent)
    : QDialog(parent)
    , jobId_(jobId)
    , history_(history) {
    setupUi();
    setWindowTitle(tr("Job History"));
    resize(600, 400);
}

void JobHistoryDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* toolbar = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &JobHistoryDialog::onRefresh);
    toolbar->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &JobHistoryDialog::onExport);
    toolbar->addWidget(exportBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &JobHistoryDialog::onClearHistory);
    toolbar->addWidget(clearBtn);
    
    toolbar->addStretch();
    layout->addLayout(toolbar);
    
    historyTable_ = new QTableView(this);
    historyModel_ = new QStandardItemModel(this);
    historyModel_->setHorizontalHeaderLabels({tr("Start Time"), tr("Duration"), tr("Status"), tr("Output")});
    historyTable_->setModel(historyModel_);
    historyTable_->setAlternatingRowColors(true);
    layout->addWidget(historyTable_, 1);
    
    layout->addWidget(new QLabel(tr("Details:"), this));
    
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(100);
    layout->addWidget(detailsEdit_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
    
    loadHistory();
}

void JobHistoryDialog::loadHistory() {
    historyModel_->clear();
    historyModel_->setHorizontalHeaderLabels({tr("Start Time"), tr("Duration"), tr("Status"), tr("Output")});
    
    for (const auto& exec : history_) {
        QString statusStr;
        switch (exec.status) {
            case JobStatus::Completed: statusStr = tr("Success"); break;
            case JobStatus::Failed: statusStr = tr("Failed"); break;
            case JobStatus::Cancelled: statusStr = tr("Cancelled"); break;
            default: statusStr = tr("Unknown");
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(exec.startTime.toString("yyyy-MM-dd hh:mm"));
        row << new QStandardItem(QString("%1s").arg(exec.duration));
        row << new QStandardItem(statusStr);
        row << new QStandardItem(exec.output);
        historyModel_->appendRow(row);
    }
}

void JobHistoryDialog::onRefresh() {
    loadHistory();
}

void JobHistoryDialog::onExport() {
    QMessageBox::information(this, tr("Export"), tr("History would be exported."));
}

void JobHistoryDialog::onViewDetails() {
    // View selected execution details
}

void JobHistoryDialog::onClearHistory() {
    auto reply = QMessageBox::question(this, tr("Clear History"),
        tr("Clear all history for this job?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        history_.clear();
        loadHistory();
    }
}

// ============================================================================
// Job Execution Dialog
// ============================================================================

JobExecutionDialog::JobExecutionDialog(const ScheduledJob& job, QWidget* parent)
    : QDialog(parent)
    , job_(job)
    , running_(false) {
    setupUi();
    setWindowTitle(tr("Execute Job - %1").arg(job.name));
    resize(500, 350);
}

void JobExecutionDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    jobNameLabel_ = new QLabel(tr("<h3>%1</h3>").arg(job_.name), this);
    layout->addWidget(jobNameLabel_);
    
    layout->addWidget(new QLabel(tr("Progress:"), this));
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    layout->addWidget(progressBar_);
    
    outputEdit_ = new QTextEdit(this);
    outputEdit_->setReadOnly(true);
    outputEdit_->setFont(QFont("Monospace", 9));
    layout->addWidget(outputEdit_, 1);
    
    statusLabel_ = new QLabel(tr("Ready to execute"), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &JobExecutionDialog::onStart);
    btnLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &JobExecutionDialog::onStop);
    btnLayout->addWidget(stopBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &JobExecutionDialog::onClose);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void JobExecutionDialog::onStart() {
    running_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("Running..."));
    progressBar_->setRange(0, 0); // Indeterminate
    
    outputEdit_->append(tr("Starting job execution at %1...").arg(QDateTime::currentDateTime().toString()));
    
    // Simulate execution
    QTimer::singleShot(1000, this, [this]() {
        if (!running_) return;
        outputEdit_->append(tr("Connecting to database..."));
        
        QTimer::singleShot(1000, this, [this]() {
            if (!running_) return;
            outputEdit_->append(tr("Executing job steps..."));
            progressBar_->setRange(0, 100);
            progressBar_->setValue(50);
            
            QTimer::singleShot(1000, this, [this]() {
                if (!running_) return;
                outputEdit_->append(tr("Finalizing..."));
                progressBar_->setValue(100);
                
                QTimer::singleShot(500, this, [this]() {
                    if (!running_) return;
                    outputEdit_->append(tr("Job completed successfully!"));
                    statusLabel_->setText(tr("Completed"));
                    running_ = false;
                    startBtn_->setEnabled(true);
                    stopBtn_->setEnabled(false);
                });
            });
        });
    });
}

void JobExecutionDialog::onStop() {
    running_ = false;
    outputEdit_->append(tr("Job stopped by user."));
    statusLabel_->setText(tr("Stopped"));
    progressBar_->setRange(0, 100);
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
}

void JobExecutionDialog::onClose() {
    if (running_) {
        auto reply = QMessageBox::question(this, tr("Job Running"),
            tr("Job is still running. Close anyway?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    accept();
}

} // namespace scratchrobin::ui
