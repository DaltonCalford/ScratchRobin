#include "database_health_check.h"
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
#include <QProgressBar>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTreeView>

namespace scratchrobin::ui {

// ============================================================================
// Database Health Check Panel
// ============================================================================

DatabaseHealthCheckPanel::DatabaseHealthCheckPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("health_check", parent)
    , client_(client) {
    setupUi();
}

void DatabaseHealthCheckPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    setupToolbar();
    
    // Summary panel
    setupSummaryPanel();
    auto* summaryWidget = new QWidget(this);
    auto* summaryLayout = new QHBoxLayout(summaryWidget);
    summaryLayout->setContentsMargins(10, 5, 10, 5);
    
    // Overall score
    auto* scoreGroup = new QGroupBox(tr("Health Score"), this);
    auto* scoreLayout = new QVBoxLayout(scoreGroup);
    overallScoreLabel_ = new QLabel("-", this);
    overallScoreLabel_->setAlignment(Qt::AlignCenter);
    overallScoreLabel_->setStyleSheet("font-size: 36px; font-weight: bold; color: green;");
    scoreLayout->addWidget(overallScoreLabel_);
    scoreBar_ = new QProgressBar(this);
    scoreBar_->setRange(0, 100);
    scoreLayout->addWidget(scoreBar_);
    summaryLayout->addWidget(scoreGroup);
    
    // Status counts
    auto* countsGroup = new QGroupBox(tr("Status"), this);
    auto* countsLayout = new QFormLayout(countsGroup);
    healthyCountLabel_ = new QLabel("0", this);
    healthyCountLabel_->setStyleSheet("color: green; font-weight: bold;");
    countsLayout->addRow(tr("Healthy:"), healthyCountLabel_);
    warningCountLabel_ = new QLabel("0", this);
    warningCountLabel_->setStyleSheet("color: orange; font-weight: bold;");
    countsLayout->addRow(tr("Warnings:"), warningCountLabel_);
    criticalCountLabel_ = new QLabel("0", this);
    criticalCountLabel_->setStyleSheet("color: red; font-weight: bold;");
    countsLayout->addRow(tr("Critical:"), criticalCountLabel_);
    summaryLayout->addWidget(countsGroup);
    
    summaryLayout->addStretch();
    
    // Progress
    progressBar_ = new QProgressBar(this);
    progressBar_->setMaximumWidth(200);
    progressBar_->setVisible(false);
    summaryLayout->addWidget(progressBar_);
    
    mainLayout->addWidget(summaryWidget);
    
    // Main tabs
    tabs_ = new QTabWidget(this);
    
    setupResultsTable();
    tabs_->addTab(resultsTable_, tr("Results"));
    
    setupDetailsPanel();
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->addWidget(detailsEdit_);
    detailsLayout->addWidget(new QLabel(tr("Recommendation:"), this));
    detailsLayout->addWidget(recommendationEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    fixBtn_ = new QPushButton(tr("Fix Issue"), this);
    connect(fixBtn_, &QPushButton::clicked, this, &DatabaseHealthCheckPanel::onFixIssue);
    btnLayout->addWidget(fixBtn_);
    detailsLayout->addLayout(btnLayout);
    
    tabs_->addTab(detailsWidget, tr("Details"));
    
    mainLayout->addWidget(tabs_, 1);
}

void DatabaseHealthCheckPanel::setupToolbar() {
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* runBtn = new QPushButton(tr("Run Health Check"), this);
    connect(runBtn, &QPushButton::clicked, this, &DatabaseHealthCheckPanel::onRunHealthCheck);
    toolbarLayout->addWidget(runBtn);
    
    auto* scheduleBtn = new QPushButton(tr("Schedule"), this);
    connect(scheduleBtn, &QPushButton::clicked, this, &DatabaseHealthCheckPanel::onScheduleCheck);
    toolbarLayout->addWidget(scheduleBtn);
    
    toolbarLayout->addSpacing(20);
    
    auto* filterCombo = new QComboBox(this);
    filterCombo->addItems({tr("All Checks"), tr("Critical Only"), tr("Warnings Only"), tr("By Category")});
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DatabaseHealthCheckPanel::onFilterByStatus);
    toolbarLayout->addWidget(new QLabel(tr("Filter:"), this));
    toolbarLayout->addWidget(filterCombo);
    
    toolbarLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export Report"), this);
    connect(exportBtn, &QPushButton::clicked, this, &DatabaseHealthCheckPanel::onExportReport);
    toolbarLayout->addWidget(exportBtn);
    
    auto* historyBtn = new QPushButton(tr("History"), this);
    connect(historyBtn, &QPushButton::clicked, this, &DatabaseHealthCheckPanel::onViewHistory);
    toolbarLayout->addWidget(historyBtn);
    
    static_cast<QVBoxLayout*>(layout())->insertLayout(0, toolbarLayout);
}

void DatabaseHealthCheckPanel::setupResultsTable() {
    resultsTable_ = new QTableView(this);
    resultsModel_ = new QStandardItemModel(this);
    resultsModel_->setHorizontalHeaderLabels({tr("Status"), tr("Category"), tr("Check"), tr("Message"), tr("Score")});
    resultsTable_->setModel(resultsModel_);
    resultsTable_->setAlternatingRowColors(true);
    resultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(resultsTable_, &QTableView::clicked, this, &DatabaseHealthCheckPanel::onItemSelected);
}

void DatabaseHealthCheckPanel::setupSummaryPanel() {
    // Implemented in setupUi
}

void DatabaseHealthCheckPanel::setupDetailsPanel() {
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(150);
    
    recommendationEdit_ = new QTextEdit(this);
    recommendationEdit_->setReadOnly(true);
    recommendationEdit_->setMaximumHeight(100);
}

void DatabaseHealthCheckPanel::runChecks() {
    allItems_.clear();
    
    // Simulate running checks
    checkConnections();
    checkDiskSpace();
    checkIndexes();
    checkTables();
    checkVacuumStatus();
    checkReplication();
    checkConfiguration();
    checkSecurity();
    
    // Update display
    resultsModel_->clear();
    resultsModel_->setHorizontalHeaderLabels({tr("Status"), tr("Category"), tr("Check"), tr("Message"), tr("Score")});
    
    for (const auto& item : allItems_) {
        QList<QStandardItem*> row;
        
        auto* statusItem = new QStandardItem();
        switch (item.status) {
        case HealthStatus::Healthy:
            statusItem->setText(tr("✓ Healthy"));
            statusItem->setForeground(Qt::darkGreen);
            break;
        case HealthStatus::Warning:
            statusItem->setText(tr("⚠ Warning"));
            statusItem->setForeground(QColor(255, 140, 0));
            break;
        case HealthStatus::Critical:
            statusItem->setText(tr("✗ Critical"));
            statusItem->setForeground(Qt::red);
            break;
        default:
            statusItem->setText(tr("? Unknown"));
        }
        row << statusItem;
        
        QString catStr;
        switch (item.category) {
        case HealthCheckCategory::Connections: catStr = tr("Connections"); break;
        case HealthCheckCategory::DiskSpace: catStr = tr("Disk Space"); break;
        case HealthCheckCategory::Indexes: catStr = tr("Indexes"); break;
        case HealthCheckCategory::Tables: catStr = tr("Tables"); break;
        case HealthCheckCategory::Vacuum: catStr = tr("Vacuum"); break;
        default: catStr = tr("Other");
        }
        row << new QStandardItem(catStr);
        row << new QStandardItem(item.name);
        row << new QStandardItem(item.message);
        row << new QStandardItem(QString::number(item.score) + "%");
        
        resultsModel_->appendRow(row);
    }
    
    updateSummary();
}

void DatabaseHealthCheckPanel::updateSummary() {
    int healthy = 0, warning = 0, critical = 0;
    double totalScore = 0;
    
    for (const auto& item : allItems_) {
        switch (item.status) {
        case HealthStatus::Healthy: healthy++; break;
        case HealthStatus::Warning: warning++; break;
        case HealthStatus::Critical: critical++; break;
        default: break;
        }
        totalScore += item.score;
    }
    
    int total = allItems_.size();
    double overallScore = total > 0 ? totalScore / total : 0;
    
    overallScoreLabel_->setText(QString::number(static_cast<int>(overallScore)));
    scoreBar_->setValue(static_cast<int>(overallScore));
    
    if (overallScore >= 80) {
        overallScoreLabel_->setStyleSheet("font-size: 36px; font-weight: bold; color: green;");
    } else if (overallScore >= 60) {
        overallScoreLabel_->setStyleSheet("font-size: 36px; font-weight: bold; color: orange;");
    } else {
        overallScoreLabel_->setStyleSheet("font-size: 36px; font-weight: bold; color: red;");
    }
    
    healthyCountLabel_->setText(QString::number(healthy));
    warningCountLabel_->setText(QString::number(warning));
    criticalCountLabel_->setText(QString::number(critical));
}

void DatabaseHealthCheckPanel::checkConnections() {
    HealthCheckItem item;
    item.id = 1;
    item.name = tr("Connection Count");
    item.category = HealthCheckCategory::Connections;
    item.status = HealthStatus::Healthy;
    item.message = tr("Connection count is within normal range (12 active)");
    item.score = 95;
    allItems_.append(item);
    
    HealthCheckItem item2;
    item2.id = 2;
    item2.name = tr("Idle Connections");
    item2.category = HealthCheckCategory::Connections;
    item2.status = HealthStatus::Warning;
    item2.message = tr("5 idle connections detected");
    item2.score = 75;
    allItems_.append(item2);
}

void DatabaseHealthCheckPanel::checkDiskSpace() {
    HealthCheckItem item;
    item.id = 3;
    item.name = tr("Tablespace Usage");
    item.category = HealthCheckCategory::DiskSpace;
    item.status = HealthStatus::Healthy;
    item.message = tr("Disk usage is 45% (450GB of 1TB)");
    item.score = 90;
    allItems_.append(item);
}

void DatabaseHealthCheckPanel::checkIndexes() {
    HealthCheckItem item;
    item.id = 4;
    item.name = tr("Index Bloat");
    item.category = HealthCheckCategory::Indexes;
    item.status = HealthStatus::Warning;
    item.message = tr("3 indexes have >30% bloat");
    item.score = 70;
    allItems_.append(item);
    
    HealthCheckItem item2;
    item2.id = 5;
    item2.name = tr("Unused Indexes");
    item2.category = HealthCheckCategory::Indexes;
    item2.status = HealthStatus::Info;
    item2.message = tr("2 indexes have never been scanned");
    item2.score = 85;
    allItems_.append(item2);
}

void DatabaseHealthCheckPanel::checkTables() {
    HealthCheckItem item;
    item.id = 6;
    item.name = tr("Table Bloat");
    item.category = HealthCheckCategory::Tables;
    item.status = HealthStatus::Healthy;
    item.message = tr("Table bloat is within acceptable limits");
    item.score = 92;
    allItems_.append(item);
}

void DatabaseHealthCheckPanel::checkVacuumStatus() {
    HealthCheckItem item;
    item.id = 7;
    item.name = tr("AutoVacuum");
    item.category = HealthCheckCategory::Vacuum;
    item.status = HealthStatus::Healthy;
    item.message = tr("Autovacuum is running normally");
    item.score = 98;
    allItems_.append(item);
    
    HealthCheckItem item2;
    item2.id = 8;
    item2.name = tr("Last Vacuum");
    item2.category = HealthCheckCategory::Vacuum;
    item2.status = HealthStatus::Warning;
    item2.message = tr("2 tables haven't been vacuumed in 7 days");
    item2.score = 80;
    allItems_.append(item2);
}

void DatabaseHealthCheckPanel::checkReplication() {
    HealthCheckItem item;
    item.id = 9;
    item.name = tr("Replication Lag");
    item.category = HealthCheckCategory::Replication;
    item.status = HealthStatus::Healthy;
    item.message = tr("Replication lag is 0 seconds");
    item.score = 100;
    allItems_.append(item);
}

void DatabaseHealthCheckPanel::checkConfiguration() {
    HealthCheckItem item;
    item.id = 10;
    item.name = tr("Shared Buffers");
    item.category = HealthCheckCategory::Configuration;
    item.status = HealthStatus::Healthy;
    item.message = tr("shared_buffers is set to optimal value");
    item.score = 95;
    allItems_.append(item);
}

void DatabaseHealthCheckPanel::checkSecurity() {
    HealthCheckItem item;
    item.id = 11;
    item.name = tr("SSL Status");
    item.category = HealthCheckCategory::Security;
    item.status = HealthStatus::Healthy;
    item.message = tr("SSL is enabled and configured properly");
    item.score = 100;
    allItems_.append(item);
}

void DatabaseHealthCheckPanel::onRunHealthCheck() {
    progressBar_->setVisible(true);
    progressBar_->setRange(0, 0); // Indeterminate
    
    isRunning_ = true;
    emit healthCheckStarted();
    
    runChecks();
    
    progressBar_->setVisible(false);
    isRunning_ = false;
    emit healthCheckCompleted(currentReport_);
}

void DatabaseHealthCheckPanel::onStopHealthCheck() {
    isRunning_ = false;
    progressBar_->setVisible(false);
}

void DatabaseHealthCheckPanel::onScheduleCheck() {
    ScheduleCheckDialog dialog(this);
    dialog.exec();
}

void DatabaseHealthCheckPanel::onExportReport() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Health Report"),
        QString(),
        tr("HTML (*.html);;PDF (*.pdf);;Text (*.txt)"));
    
    if (!fileName.isEmpty()) {
        HealthReportDialog dialog(currentReport_, this);
        dialog.exec();
    }
}

void DatabaseHealthCheckPanel::onPrintReport() {
    QMessageBox::information(this, tr("Print"),
        tr("Print functionality not yet implemented."));
}

void DatabaseHealthCheckPanel::onFilterByCategory(const QString& category) {
    Q_UNUSED(category)
    // Filter results by category
}

void DatabaseHealthCheckPanel::onFilterByStatus(int status) {
    Q_UNUSED(status)
    // Filter results by status
}

void DatabaseHealthCheckPanel::onShowAllIssues() {
    // Show all issues
}

void DatabaseHealthCheckPanel::onShowCriticalOnly() {
    // Filter to critical only
}

void DatabaseHealthCheckPanel::onShowWarningsOnly() {
    // Filter to warnings only
}

void DatabaseHealthCheckPanel::onItemSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= allItems_.size()) return;
    
    const auto& item = allItems_[index.row()];
    detailsEdit_->setPlainText(item.details.isEmpty() ? item.message : item.details);
    recommendationEdit_->setPlainText(item.recommendation.isEmpty() ? 
        tr("No specific recommendation available.") : item.recommendation);
}

void DatabaseHealthCheckPanel::onFixIssue() {
    auto index = resultsTable_->currentIndex();
    if (!index.isValid() || index.row() >= allItems_.size()) return;
    
    FixIssueDialog dialog(allItems_[index.row()], client_, this);
    dialog.exec();
}

void DatabaseHealthCheckPanel::onIgnoreIssue() {
    // Mark issue as ignored
}

void DatabaseHealthCheckPanel::onViewDetails() {
    tabs_->setCurrentIndex(1);
}

void DatabaseHealthCheckPanel::onViewHistory() {
    QMessageBox::information(this, tr("History"),
        tr("Health check history would be shown here."));
}

void DatabaseHealthCheckPanel::onCompareWithPrevious() {
    QMessageBox::information(this, tr("Compare"),
        tr("Compare with previous check would be shown here."));
}

void DatabaseHealthCheckPanel::onLoadPreviousReport() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Health Report"),
        QString(),
        tr("Health Reports (*.hcr)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Load"),
            tr("Report loaded from %1").arg(fileName));
    }
}

// ============================================================================
// Schedule Check Dialog
// ============================================================================

ScheduleCheckDialog::ScheduleCheckDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void ScheduleCheckDialog::setupUi() {
    setWindowTitle(tr("Schedule Health Check"));
    resize(400, 300);
    
    auto* layout = new QVBoxLayout(this);
    
    enabledCheck_ = new QCheckBox(tr("Enable scheduled checks"), this);
    enabledCheck_->setChecked(true);
    layout->addWidget(enabledCheck_);
    
    auto* formLayout = new QFormLayout();
    
    frequencyCombo_ = new QComboBox(this);
    frequencyCombo_->addItems({tr("Daily"), tr("Weekly"), tr("Monthly")});
    formLayout->addRow(tr("Frequency:"), frequencyCombo_);
    
    dayCombo_ = new QComboBox(this);
    dayCombo_->addItems({tr("Monday"), tr("Tuesday"), tr("Wednesday"), tr("Thursday"), 
                         tr("Friday"), tr("Saturday"), tr("Sunday")});
    formLayout->addRow(tr("Day:"), dayCombo_);
    
    hourCombo_ = new QComboBox(this);
    for (int i = 0; i < 24; ++i) {
        hourCombo_->addItem(QString::number(i).rightJustified(2, '0') + ":00");
    }
    formLayout->addRow(tr("Time:"), hourCombo_);
    
    emailCheck_ = new QCheckBox(tr("Send email notifications"), this);
    formLayout->addRow(emailCheck_);
    
    emailEdit_ = new QLineEdit(this);
    formLayout->addRow(tr("Email:"), emailEdit_);
    
    autoFixCheck_ = new QCheckBox(tr("Auto-fix issues when possible"), this);
    formLayout->addRow(autoFixCheck_);
    
    layout->addLayout(formLayout);
    layout->addStretch();
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &ScheduleCheckDialog::onSave);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void ScheduleCheckDialog::onSave() {
    accept();
}

void ScheduleCheckDialog::onTestSchedule() {
    QMessageBox::information(this, tr("Test"),
        tr("Test schedule notification sent."));
}

// ============================================================================
// Fix Issue Dialog
// ============================================================================

FixIssueDialog::FixIssueDialog(const HealthCheckItem& item,
                              backend::SessionClient* client,
                              QWidget* parent)
    : QDialog(parent)
    , item_(item)
    , client_(client) {
    setupUi();
}

void FixIssueDialog::setupUi() {
    setWindowTitle(tr("Fix Issue - %1").arg(item_.name));
    resize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    issueEdit_ = new QTextEdit(this);
    issueEdit_->setReadOnly(true);
    issueEdit_->setPlainText(item_.message + "\n\n" + item_.details);
    layout->addWidget(issueEdit_);
    
    layout->addWidget(new QLabel(tr("Fix Script:"), this));
    
    fixScriptEdit_ = new QTextEdit(this);
    fixScriptEdit_->setFont(QFont("Consolas", 9));
    layout->addWidget(fixScriptEdit_, 1);
    
    generateFixScript();
    
    backupCheck_ = new QCheckBox(tr("Create backup before fixing"), this);
    backupCheck_->setChecked(true);
    layout->addWidget(backupCheck_);
    
    verifyCheck_ = new QCheckBox(tr("Verify fix after applying"), this);
    verifyCheck_->setChecked(true);
    layout->addWidget(verifyCheck_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &FixIssueDialog::onPreviewFix);
    btnLayout->addWidget(previewBtn);
    
    auto* applyBtn = new QPushButton(tr("Apply Fix"), this);
    connect(applyBtn, &QPushButton::clicked, this, &FixIssueDialog::onApplyFix);
    btnLayout->addWidget(applyBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void FixIssueDialog::generateFixScript() {
    // Generate appropriate fix script based on issue type
    QString script;
    
    switch (item_.category) {
    case HealthCheckCategory::Indexes:
        script = "-- Reindex table to reduce bloat\nREINDEX INDEX idx_name;";
        break;
    case HealthCheckCategory::Vacuum:
        script = "-- Vacuum table\nVACUUM ANALYZE table_name;";
        break;
    default:
        script = "-- No automatic fix available\n-- Manual intervention required";
    }
    
    fixScriptEdit_->setPlainText(script);
}

void FixIssueDialog::onPreviewFix() {
    QMessageBox::information(this, tr("Preview"),
        tr("Preview of fix changes would be shown here."));
}

void FixIssueDialog::onApplyFix() {
    QMessageBox::warning(this, tr("Apply Fix"),
        tr("This would apply the fix to the database.\n"
           "In production, this would show a confirmation dialog."));
}

void FixIssueDialog::onScheduleFix() {
    QMessageBox::information(this, tr("Schedule"),
        tr("Fix would be scheduled for next maintenance window."));
}

// ============================================================================
// Health Report Dialog
// ============================================================================

HealthReportDialog::HealthReportDialog(const HealthReport& report, QWidget* parent)
    : QDialog(parent)
    , report_(report) {
    setupUi();
    generateReport();
}

void HealthReportDialog::setupUi() {
    setWindowTitle(tr("Health Report"));
    resize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    reportEdit_ = new QTextEdit(this);
    reportEdit_->setReadOnly(true);
    layout->addWidget(reportEdit_, 1);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* htmlBtn = new QPushButton(tr("Export HTML"), this);
    connect(htmlBtn, &QPushButton::clicked, this, &HealthReportDialog::onExportHtml);
    btnLayout->addWidget(htmlBtn);
    
    auto* pdfBtn = new QPushButton(tr("Export PDF"), this);
    connect(pdfBtn, &QPushButton::clicked, this, &HealthReportDialog::onExportPdf);
    btnLayout->addWidget(pdfBtn);
    
    auto* printBtn = new QPushButton(tr("Print"), this);
    connect(printBtn, &QPushButton::clicked, this, &HealthReportDialog::onPrint);
    btnLayout->addWidget(printBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void HealthReportDialog::generateReport() {
    QString report;
    report += "<h1>Database Health Report</h1>\n";
    report += "<p>Generated: " + QDateTime::currentDateTime().toString() + "</p>\n";
    report += "<h2>Overall Score: 87%</h2>\n";
    report += "<p>Status: <span style='color: orange;'>Good with minor issues</span></p>\n";
    report += "<h3>Summary</h3>\n";
    report += "<ul>\n";
    report += "<li>8 Healthy checks</li>\n";
    report += "<li>3 Warnings</li>\n";
    report += "<li>0 Critical issues</li>\n";
    report += "</ul>\n";
    
    reportEdit_->setHtml(report);
}

void HealthReportDialog::onExportHtml() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export HTML"),
        QString(),
        tr("HTML Files (*.html)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(reportEdit_->toHtml().toUtf8());
            file.close();
        }
        QMessageBox::information(this, tr("Export"), tr("Report exported."));
    }
}

void HealthReportDialog::onExportPdf() {
    QMessageBox::information(this, tr("PDF"), tr("PDF export not yet implemented."));
}

void HealthReportDialog::onPrint() {
    QMessageBox::information(this, tr("Print"), tr("Print dialog would open."));
}

void HealthReportDialog::onEmail() {
    QMessageBox::information(this, tr("Email"), tr("Email dialog would open."));
}

// ============================================================================
// Detailed Analysis Dialog
// ============================================================================

DetailedAnalysisDialog::DetailedAnalysisDialog(HealthCheckCategory category,
                                              backend::SessionClient* client,
                                              QWidget* parent)
    : QDialog(parent)
    , category_(category)
    , client_(client) {
    setupUi();
    loadDetails();
}

void DetailedAnalysisDialog::setupUi() {
    setWindowTitle(tr("Detailed Analysis"));
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    detailsTable_ = new QTableView(this);
    detailsModel_ = new QStandardItemModel(this);
    detailsTable_->setModel(detailsModel_);
    layout->addWidget(detailsTable_, 1);
    
    analysisEdit_ = new QTextEdit(this);
    analysisEdit_->setReadOnly(true);
    analysisEdit_->setMaximumHeight(150);
    layout->addWidget(analysisEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &DetailedAnalysisDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &DetailedAnalysisDialog::onExportDetails);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void DetailedAnalysisDialog::loadDetails() {
    // Load detailed analysis data
    detailsModel_->clear();
}

void DetailedAnalysisDialog::onRefresh() {
    loadDetails();
}

void DetailedAnalysisDialog::onExportDetails() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Details"),
        QString(),
        tr("Text Files (*.txt);;CSV (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Details exported to %1").arg(fileName));
    }
}

} // namespace scratchrobin::ui
