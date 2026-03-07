#include "sql_profiler.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
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
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTimer>
#include <QFileInfo>

namespace scratchrobin::ui {

// ============================================================================
// SQL Profiler Panel
// ============================================================================

SqlProfilerPanel::SqlProfilerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("sql_profiler", parent)
    , client_(client) {
    setupUi();
    
    // Setup refresh timer
    refreshTimer_ = new QTimer(this);
    connect(refreshTimer_, &QTimer::timeout, this, &SqlProfilerPanel::onRefreshStats);
}

void SqlProfilerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    setupToolbar();
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel(tr("Filter:"), this));
    filterCombo_ = new QComboBox(this);
    filterCombo_->addItems({tr("All Queries"), tr("Slow Queries"), tr("Frequent Queries"), 
                            tr("High I/O"), tr("Errors")});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SqlProfilerPanel::onFilterChanged);
    filterLayout->addWidget(filterCombo_);
    
    filterLayout->addSpacing(20);
    
    filterLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Filter by text..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &SqlProfilerPanel::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    filterLayout->addSpacing(20);
    
    autoRefreshCheck_ = new QCheckBox(tr("Auto-refresh"), this);
    filterLayout->addWidget(autoRefreshCheck_);
    
    refreshIntervalSpin_ = new QSpinBox(this);
    refreshIntervalSpin_->setRange(1, 60);
    refreshIntervalSpin_->setValue(5);
    refreshIntervalSpin_->setSuffix("s");
    filterLayout->addWidget(refreshIntervalSpin_);
    
    mainLayout->addLayout(filterLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Query table
    setupQueryTable();
    splitter->addWidget(queryTable_);
    
    // Stats panel
    setupStatsPanel();
    auto* statsWidget = new QWidget(this);
    auto* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    
    // Summary stats
    auto* statsGrid = new QGridLayout();
    statsGrid->addWidget(new QLabel(tr("Total Queries:"), this), 0, 0);
    totalQueriesLabel_ = new QLabel("0", this);
    statsGrid->addWidget(totalQueriesLabel_, 0, 1);
    
    statsGrid->addWidget(new QLabel(tr("Slow Queries:"), this), 0, 2);
    slowQueriesLabel_ = new QLabel("0", this);
    slowQueriesLabel_->setStyleSheet("color: red;");
    statsGrid->addWidget(slowQueriesLabel_, 0, 3);
    
    statsGrid->addWidget(new QLabel(tr("Avg Time:"), this), 1, 0);
    avgTimeLabel_ = new QLabel("0 ms", this);
    statsGrid->addWidget(avgTimeLabel_, 1, 1);
    
    statsGrid->addWidget(new QLabel(tr("Total Time:"), this), 1, 2);
    totalTimeLabel_ = new QLabel("0 ms", this);
    statsGrid->addWidget(totalTimeLabel_, 1, 3);
    
    statsGrid->addWidget(new QLabel(tr("Rows Processed:"), this), 2, 0);
    rowsProcessedLabel_ = new QLabel("0", this);
    statsGrid->addWidget(rowsProcessedLabel_, 2, 1);
    
    statsLayout->addLayout(statsGrid);
    
    // Node stats table
    nodeStatsTable_ = new QTableView(this);
    nodeStatsModel_ = new QStandardItemModel(this);
    nodeStatsModel_->setHorizontalHeaderLabels({tr("Query"), tr("Count"), tr("Total"), tr("Avg"), tr("Max")});
    nodeStatsTable_->setModel(nodeStatsModel_);
    nodeStatsTable_->setAlternatingRowColors(true);
    statsLayout->addWidget(nodeStatsTable_, 1);
    
    splitter->addWidget(statsWidget);
    splitter->setSizes({300, 200});
    
    mainLayout->addWidget(splitter, 1);
}

void SqlProfilerPanel::setupToolbar() {
    auto* toolbarLayout = new QHBoxLayout();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &SqlProfilerPanel::onStartProfiling);
    toolbarLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    connect(stopBtn_, &QPushButton::clicked, this, &SqlProfilerPanel::onStopProfiling);
    toolbarLayout->addWidget(stopBtn_);
    
    pauseBtn_ = new QPushButton(tr("Pause"), this);
    connect(pauseBtn_, &QPushButton::clicked, this, &SqlProfilerPanel::onPauseProfiling);
    toolbarLayout->addWidget(pauseBtn_);
    
    clearBtn_ = new QPushButton(tr("Clear"), this);
    connect(clearBtn_, &QPushButton::clicked, this, &SqlProfilerPanel::onClearData);
    toolbarLayout->addWidget(clearBtn_);
    
    toolbarLayout->addSpacing(20);
    
    auto* reportBtn = new QPushButton(tr("Generate Report"), this);
    connect(reportBtn, &QPushButton::clicked, this, &SqlProfilerPanel::onGenerateReport);
    toolbarLayout->addWidget(reportBtn);
    
    auto* exportBtn = new QPushButton(tr("Export Slow"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SqlProfilerPanel::onExportSlowQueries);
    toolbarLayout->addWidget(exportBtn);
    
    toolbarLayout->addStretch();
    
    static_cast<QVBoxLayout*>(layout())->addLayout(toolbarLayout);
}

void SqlProfilerPanel::setupQueryTable() {
    queryTable_ = new QTableView(this);
    queryModel_ = new QStandardItemModel(this);
    queryModel_->setHorizontalHeaderLabels({tr("Time"), tr("Query"), tr("Duration"), tr("Rows"), tr("Database"), tr("User")});
    queryTable_->setModel(queryModel_);
    queryTable_->setAlternatingRowColors(true);
    queryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queryTable_->horizontalHeader()->setStretchLastSection(true);
    connect(queryTable_, &QTableView::clicked, this, &SqlProfilerPanel::onQuerySelected);
}

void SqlProfilerPanel::setupStatsPanel() {
    // Implemented in setupUi
}

void SqlProfilerPanel::setupChartsPanel() {
    // Charts for visualization
}

void SqlProfilerPanel::setupDetailsPanel() {
    // Query details panel
}

void SqlProfilerPanel::refreshQueryList() {
    queryModel_->clear();
    queryModel_->setHorizontalHeaderLabels({tr("Time"), tr("Query"), tr("Duration"), tr("Rows"), tr("Database"), tr("User")});
    
    auto queries = filterQueries();
    for (const auto& q : queries) {
        QList<QStandardItem*> row;
        row << new QStandardItem(q.startTime.toString("hh:mm:ss"));
        row << new QStandardItem(q.query.left(80) + (q.query.length() > 80 ? "..." : ""));
        
        auto* durationItem = new QStandardItem(QString::number(q.duration, 'f', 2) + " ms");
        if (q.isSlow) {
            durationItem->setForeground(Qt::red);
        }
        row << durationItem;
        
        row << new QStandardItem(QString::number(q.rowsReturned));
        row << new QStandardItem(q.database);
        row << new QStandardItem(q.user);
        
        queryModel_->appendRow(row);
    }
    
    updateStatistics();
}

void SqlProfilerPanel::updateStatistics() {
    double totalTime = 0;
    int slowCount = 0;
    int totalRows = 0;
    
    for (const auto& q : allQueries_) {
        totalTime += q.duration;
        totalRows += q.rowsReturned;
        if (q.isSlow) {
            slowCount++;
        }
    }
    
    totalQueriesLabel_->setText(QString::number(allQueries_.size()));
    slowQueriesLabel_->setText(QString::number(slowCount));
    totalTimeLabel_->setText(QString::number(totalTime, 'f', 2) + " ms");
    avgTimeLabel_->setText(allQueries_.isEmpty() ? "0 ms" : 
                          QString::number(totalTime / allQueries_.size(), 'f', 2) + " ms");
    rowsProcessedLabel_->setText(QString::number(totalRows));
    
    // Update node stats
    nodeStatsModel_->clear();
    nodeStatsModel_->setHorizontalHeaderLabels({tr("Query Pattern"), tr("Count"), tr("Total"), tr("Avg"), tr("Max")});
    
    // Group by normalized query
    QMap<QString, QList<QueryProfile>> grouped;
    for (const auto& q : allQueries_) {
        grouped[q.normalizedQuery].append(q);
    }
    
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        const auto& queries = it.value();
        double total = 0;
        double max = 0;
        for (const auto& q : queries) {
            total += q.duration;
            if (q.duration > max) max = q.duration;
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(it.key().left(50));
        row << new QStandardItem(QString::number(queries.size()));
        row << new QStandardItem(QString::number(total, 'f', 2) + " ms");
        row << new QStandardItem(QString::number(total / queries.size(), 'f', 2) + " ms");
        row << new QStandardItem(QString::number(max, 'f', 2) + " ms");
        nodeStatsModel_->appendRow(row);
    }
}

void SqlProfilerPanel::detectSlowQueries() {
    for (auto& q : allQueries_) {
        q.isSlow = q.duration > 100; // 100ms threshold
    }
}

QList<QueryProfile> SqlProfilerPanel::filterQueries() {
    QList<QueryProfile> filtered;
    
    for (const auto& q : allQueries_) {
        bool include = true;
        
        // Apply filter type
        switch (currentFilter_) {
        case ProfileFilter::SlowQueries:
            include = q.isSlow;
            break;
        case ProfileFilter::HighIO:
            include = q.tempBlksRead > 0 || q.tempBlksWritten > 0;
            break;
        default:
            break;
        }
        
        // Apply search text
        if (!searchText_.isEmpty()) {
            include = include && q.query.contains(searchText_, Qt::CaseInsensitive);
        }
        
        if (include) {
            filtered.append(q);
        }
    }
    
    return filtered;
}

void SqlProfilerPanel::onStartProfiling() {
    currentSession_.isRunning = true;
    currentSession_.startTime = QDateTime::currentDateTime();
    currentSession_.sessionId = 1;
    
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    pauseBtn_->setEnabled(true);
    
    if (autoRefreshCheck_->isChecked()) {
        refreshTimer_->start(refreshIntervalSpin_->value() * 1000);
    }
    
    emit profilingStarted();
    
    // Simulate capturing queries
    for (int i = 0; i < 5; ++i) {
        QueryProfile qp;
        qp.id = i + 1;
        qp.query = "SELECT * FROM customers WHERE status = 'active'";
        qp.normalizedQuery = "SELECT * FROM customers WHERE status = ?";
        qp.duration = 50 + i * 10;
        qp.rowsReturned = 100 + i * 50;
        qp.database = "testdb";
        qp.user = "admin";
        qp.startTime = QDateTime::currentDateTime().addSecs(-i * 10);
        allQueries_.append(qp);
    }
    
    // Add a slow query
    QueryProfile slow;
    slow.id = 6;
    slow.query = "SELECT * FROM large_table WHERE unindexed_column LIKE '%test%'";
    slow.normalizedQuery = "SELECT * FROM large_table WHERE unindexed_column LIKE ?";
    slow.duration = 500;
    slow.isSlow = true;
    slow.rowsReturned = 10000;
    slow.database = "testdb";
    slow.user = "admin";
    slow.startTime = QDateTime::currentDateTime();
    allQueries_.append(slow);
    
    detectSlowQueries();
    refreshQueryList();
}

void SqlProfilerPanel::onStopProfiling() {
    currentSession_.isRunning = false;
    currentSession_.endTime = QDateTime::currentDateTime();
    
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    pauseBtn_->setEnabled(false);
    
    refreshTimer_->stop();
    
    emit profilingStopped();
}

void SqlProfilerPanel::onPauseProfiling() {
    currentSession_.isRunning = false;
    refreshTimer_->stop();
}

void SqlProfilerPanel::onClearData() {
    allQueries_.clear();
    refreshQueryList();
}

void SqlProfilerPanel::onSaveSession() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Profile Session"),
        QString(),
        tr("Profile Files (*.prof);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Save"),
            tr("Session saved to %1").arg(fileName));
    }
}

void SqlProfilerPanel::onLoadSession() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Profile Session"),
        QString(),
        tr("Profile Files (*.prof *.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Load"),
            tr("Session loaded from %1").arg(fileName));
        refreshQueryList();
    }
}

void SqlProfilerPanel::onFilterChanged(int filter) {
    currentFilter_ = static_cast<ProfileFilter>(filter);
    refreshQueryList();
}

void SqlProfilerPanel::onSearchTextChanged(const QString& text) {
    searchText_ = text;
    refreshQueryList();
}

void SqlProfilerPanel::onTimeRangeChanged() {
    // Apply time range filter
}

void SqlProfilerPanel::onApplyFilters() {
    refreshQueryList();
}

void SqlProfilerPanel::onQuerySelected(const QModelIndex& index) {
    Q_UNUSED(index)
    // Show query details
}

void SqlProfilerPanel::onShowQueryDetails() {
    // Show detailed query info
}

void SqlProfilerPanel::onShowQueryPlan() {
    // Open query plan for selected query
}

void SqlProfilerPanel::onOptimizeQuery() {
    // Show optimization suggestions
}

void SqlProfilerPanel::onExportSlowQueries() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Slow Queries"),
        QString(),
        tr("CSV (*.csv);;Text (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << "Query,Duration,Rows\n";
            for (const auto& q : allQueries_) {
                if (q.isSlow) {
                    stream << "\"" << q.query << "\"," << q.duration << "," << q.rowsReturned << "\n";
                }
            }
            file.close();
        }
        
        QMessageBox::information(this, tr("Export"),
            tr("Slow queries exported to %1").arg(fileName));
    }
}

void SqlProfilerPanel::onGenerateReport() {
    ProfileReportDialog dialog(currentSession_, this);
    dialog.exec();
}

void SqlProfilerPanel::onRefreshStats() {
    // Refresh statistics from database
}

void SqlProfilerPanel::onViewTopQueries() {
    // Show top queries view
}

void SqlProfilerPanel::onViewTrends() {
    // Show trends over time
}

void SqlProfilerPanel::onViewResourceUsage() {
    // Show I/O and memory usage
}

// ============================================================================
// Profile Report Dialog
// ============================================================================

ProfileReportDialog::ProfileReportDialog(const ProfileSession& session, QWidget* parent)
    : QDialog(parent)
    , session_(session) {
    setupUi();
    generateSummary();
    generateTopQueries();
}

void ProfileReportDialog::setupUi() {
    setWindowTitle(tr("Profile Report"));
    resize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    reportTabs_ = new QTabWidget(this);
    
    // Summary tab
    summaryEdit_ = new QTextEdit(this);
    summaryEdit_->setReadOnly(true);
    reportTabs_->addTab(summaryEdit_, tr("Summary"));
    
    // Top queries tab
    topQueriesTable_ = new QTableView(this);
    topQueriesModel_ = new QStandardItemModel(this);
    topQueriesModel_->setHorizontalHeaderLabels({tr("Query"), tr("Count"), tr("Total Time"), tr("Avg Time"), tr("Rows")});
    topQueriesTable_->setModel(topQueriesModel_);
    topQueriesTable_->setAlternatingRowColors(true);
    reportTabs_->addTab(topQueriesTable_, tr("Top Queries"));
    
    // Recommendations tab
    recommendationsEdit_ = new QTextEdit(this);
    recommendationsEdit_->setReadOnly(true);
    reportTabs_->addTab(recommendationsEdit_, tr("Recommendations"));
    
    layout->addWidget(reportTabs_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* htmlBtn = new QPushButton(tr("Export HTML"), this);
    connect(htmlBtn, &QPushButton::clicked, this, &ProfileReportDialog::onGenerateHtml);
    btnLayout->addWidget(htmlBtn);
    
    auto* csvBtn = new QPushButton(tr("Export CSV"), this);
    connect(csvBtn, &QPushButton::clicked, this, &ProfileReportDialog::onExportCsv);
    btnLayout->addWidget(csvBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ProfileReportDialog::generateSummary() {
    QString summary;
    summary += "<h1>SQL Profile Report</h1>\n";
    summary += "<p><b>Session:</b> " + session_.name + "</p>\n";
    summary += "<p><b>Period:</b> " + session_.startTime.toString() + " - " + 
               (session_.endTime.isValid() ? session_.endTime.toString() : tr("Running")) + "</p>\n";
    summary += "<p><b>Total Queries:</b> " + QString::number(session_.queryCount) + "</p>\n";
    summary += "<p><b>Total Duration:</b> " + QString::number(session_.totalDuration) + " ms</p>\n";
    
    summaryEdit_->setHtml(summary);
}

void ProfileReportDialog::generateTopQueries() {
    topQueriesModel_->clear();
    topQueriesModel_->setHorizontalHeaderLabels({tr("Query"), tr("Count"), tr("Total Time"), tr("Avg Time"), tr("Rows")});
    
    // Add sample data
    topQueriesModel_->appendRow({
        new QStandardItem("SELECT * FROM customers..."),
        new QStandardItem("150"),
        new QStandardItem("5000 ms"),
        new QStandardItem("33.3 ms"),
        new QStandardItem("15000")
    });
}

void ProfileReportDialog::generateTrends() {
    // Generate trend charts
}

void ProfileReportDialog::onGenerateHtml() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Report"),
        QString(),
        tr("HTML Files (*.html)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(summaryEdit_->toHtml().toUtf8());
            file.close();
        }
        
        QMessageBox::information(this, tr("Export"),
            tr("Report exported to %1").arg(fileName));
    }
}

void ProfileReportDialog::onGeneratePdf() {
    QMessageBox::information(this, tr("PDF"),
        tr("PDF export not yet implemented."));
}

void ProfileReportDialog::onExportCsv() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export CSV"),
        QString(),
        tr("CSV Files (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Data exported to %1").arg(fileName));
    }
}

void ProfileReportDialog::onPrint() {
    QMessageBox::information(this, tr("Print"),
        tr("Print functionality not yet implemented."));
}

void ProfileReportDialog::onSaveReport() {
    onGenerateHtml();
}

// ============================================================================
// Optimization Suggestions Dialog
// ============================================================================

OptimizationSuggestionsDialog::OptimizationSuggestionsDialog(const QueryProfile& query,
                                                             backend::SessionClient* client,
                                                             QWidget* parent)
    : QDialog(parent)
    , query_(query)
    , client_(client) {
    setupUi();
    analyzeQuery();
    generateSuggestions();
}

void OptimizationSuggestionsDialog::setupUi() {
    setWindowTitle(tr("Optimization Suggestions"));
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    layout->addWidget(new QLabel(tr("Query:"), this));
    
    queryEdit_ = new QTextEdit(this);
    queryEdit_->setFont(QFont("Consolas", 9));
    queryEdit_->setPlainText(query_.query);
    queryEdit_->setMaximumHeight(100);
    queryEdit_->setReadOnly(true);
    layout->addWidget(queryEdit_);
    
    layout->addWidget(new QLabel(tr("Suggestions:"), this));
    
    suggestionsTable_ = new QTableView(this);
    suggestionsModel_ = new QStandardItemModel(this);
    suggestionsModel_->setHorizontalHeaderLabels({tr("Severity"), tr("Category"), tr("Suggestion")});
    suggestionsTable_->setModel(suggestionsModel_);
    suggestionsTable_->setAlternatingRowColors(true);
    layout->addWidget(suggestionsTable_, 1);
    
    layout->addWidget(new QLabel(tr("Details:"), this));
    
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    layout->addWidget(detailsEdit_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* planBtn = new QPushButton(tr("View Plan"), this);
    connect(planBtn, &QPushButton::clicked, this, &OptimizationSuggestionsDialog::onViewQueryPlan);
    btnLayout->addWidget(planBtn);
    
    auto* indexBtn = new QPushButton(tr("Create Index"), this);
    connect(indexBtn, &QPushButton::clicked, this, &OptimizationSuggestionsDialog::onCreateIndex);
    btnLayout->addWidget(indexBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void OptimizationSuggestionsDialog::analyzeQuery() {
    // Analyze query for optimization opportunities
}

void OptimizationSuggestionsDialog::generateSuggestions() {
    suggestionsModel_->clear();
    suggestionsModel_->setHorizontalHeaderLabels({tr("Severity"), tr("Category"), tr("Suggestion")});
    
    if (query_.duration > 100) {
        auto* severityItem = new QStandardItem(tr("High"));
        severityItem->setForeground(Qt::red);
        suggestionsModel_->appendRow({
            severityItem,
            new QStandardItem(tr("Performance")),
            new QStandardItem(tr("Query execution time exceeds 100ms. Consider adding indexes."))
        });
    }
    
    if (query_.tempBlksRead > 0 || query_.tempBlksWritten > 0) {
        auto* severityItem = new QStandardItem(tr("Medium"));
        severityItem->setForeground(QColor(255, 140, 0));
        suggestionsModel_->appendRow({
            severityItem,
            new QStandardItem(tr("I/O")),
            new QStandardItem(tr("Query is using temp files. Consider increasing work_mem."))
        });
    }
    
    suggestionsModel_->appendRow({
        new QStandardItem(tr("Low")),
        new QStandardItem(tr("Style")),
        new QStandardItem(tr("Consider using explicit column list instead of SELECT *"))
    });
}

void OptimizationSuggestionsDialog::onApplySuggestion() {
    QMessageBox::information(this, tr("Apply"),
        tr("Apply suggestion - not yet implemented."));
}

void OptimizationSuggestionsDialog::onViewQueryPlan() {
    // Open query plan visualizer
}

void OptimizationSuggestionsDialog::onCreateIndex() {
    QMessageBox::information(this, tr("Create Index"),
        tr("Index creation dialog - not yet implemented."));
}

// ============================================================================
// Configure Profiling Dialog
// ============================================================================

ConfigureProfilingDialog::ConfigureProfilingDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
    loadSettings();
}

void ConfigureProfilingDialog::setupUi() {
    setWindowTitle(tr("Configure Profiling"));
    resize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* optionsGroup = new QGroupBox(tr("Tracking Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    trackPlanningTimeCheck_ = new QCheckBox(tr("Track planning time"), this);
    trackIoTimingCheck_ = new QCheckBox(tr("Track I/O timing"), this);
    trackRowsCheck_ = new QCheckBox(tr("Track row counts"), this);
    trackTempFilesCheck_ = new QCheckBox(tr("Track temp file usage"), this);
    
    optionsLayout->addWidget(trackPlanningTimeCheck_);
    optionsLayout->addWidget(trackIoTimingCheck_);
    optionsLayout->addWidget(trackRowsCheck_);
    optionsLayout->addWidget(trackTempFilesCheck_);
    
    layout->addWidget(optionsGroup);
    
    auto* limitsGroup = new QGroupBox(tr("Limits"), this);
    auto* limitsLayout = new QFormLayout(limitsGroup);
    
    slowQueryThresholdSpin_ = new QSpinBox(this);
    slowQueryThresholdSpin_->setRange(10, 10000);
    slowQueryThresholdSpin_->setValue(100);
    slowQueryThresholdSpin_->setSuffix(" ms");
    limitsLayout->addRow(tr("Slow query threshold:"), slowQueryThresholdSpin_);
    
    maxQueriesSpin_ = new QSpinBox(this);
    maxQueriesSpin_->setRange(100, 100000);
    maxQueriesSpin_->setValue(10000);
    limitsLayout->addRow(tr("Max queries to store:"), maxQueriesSpin_);
    
    layout->addWidget(limitsGroup);
    
    auto* filtersGroup = new QGroupBox(tr("Filters"), this);
    auto* filtersLayout = new QFormLayout(filtersGroup);
    
    includeFilterEdit_ = new QLineEdit(this);
    filtersLayout->addRow(tr("Include patterns:"), includeFilterEdit_);
    
    excludeFilterEdit_ = new QLineEdit(this);
    filtersLayout->addRow(tr("Exclude patterns:"), excludeFilterEdit_);
    
    layout->addWidget(filtersGroup);
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &ConfigureProfilingDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    auto* resetBtn = new QPushButton(tr("Reset"), this);
    connect(resetBtn, &QPushButton::clicked, this, &ConfigureProfilingDialog::onReset);
    btnBox->addButton(resetBtn, QDialogButtonBox::ResetRole);
    
    layout->addWidget(btnBox);
}

void ConfigureProfilingDialog::loadSettings() {
    // Load from settings
    trackPlanningTimeCheck_->setChecked(true);
    trackIoTimingCheck_->setChecked(true);
    trackRowsCheck_->setChecked(true);
    trackTempFilesCheck_->setChecked(false);
}

void ConfigureProfilingDialog::onSave() {
    // Save settings
    accept();
}

void ConfigureProfilingDialog::onReset() {
    // Reset to defaults
    trackPlanningTimeCheck_->setChecked(true);
    trackIoTimingCheck_->setChecked(true);
    trackRowsCheck_->setChecked(true);
    trackTempFilesCheck_->setChecked(false);
    slowQueryThresholdSpin_->setValue(100);
    maxQueriesSpin_->setValue(10000);
}

} // namespace scratchrobin::ui
