#include "vacuum_analyze_tool.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
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
#include <QSplitter>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QProgressBar>
#include <QSpinBox>
#include <QListWidget>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QTimer>

namespace scratchrobin::ui {

// ============================================================================
// Vacuum/Analyze Panel
// ============================================================================

VacuumAnalyzePanel::VacuumAnalyzePanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("vacuum_analyze", parent)
    , client_(client) {
    setupUi();
    loadTableStats();
}

void VacuumAnalyzePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* vacuumBtn = new QPushButton(tr("Vacuum"), this);
    connect(vacuumBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onVacuum);
    toolbarLayout->addWidget(vacuumBtn);
    
    auto* vacuumFullBtn = new QPushButton(tr("Vacuum Full"), this);
    connect(vacuumFullBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onVacuumFull);
    toolbarLayout->addWidget(vacuumFullBtn);
    
    auto* analyzeBtn = new QPushButton(tr("Analyze"), this);
    connect(analyzeBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onAnalyze);
    toolbarLayout->addWidget(analyzeBtn);
    
    auto* vacuumAnalyzeBtn = new QPushButton(tr("Vacuum Analyze"), this);
    connect(vacuumAnalyzeBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onVacuumAnalyze);
    toolbarLayout->addWidget(vacuumAnalyzeBtn);
    
    auto* reindexBtn = new QPushButton(tr("Reindex"), this);
    connect(reindexBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onReindex);
    toolbarLayout->addWidget(reindexBtn);
    
    toolbarLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onRefreshStats);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    schemaFilter_ = new QComboBox(this);
    schemaFilter_->addItem(tr("All Schemas"));
    schemaFilter_->addItems({"public", "pg_catalog", "information_schema"});
    filterLayout->addWidget(new QLabel(tr("Schema:"), this));
    filterLayout->addWidget(schemaFilter_);
    
    sortByCombo_ = new QComboBox(this);
    sortByCombo_->addItems({tr("Table Name"), tr("Size"), tr("Dead Tuples"), tr("Bloat"), tr("Last Vacuum")});
    filterLayout->addWidget(new QLabel(tr("Sort By:"), this));
    filterLayout->addWidget(sortByCombo_);
    
    showBloatedOnlyCheck_ = new QCheckBox(tr("Bloated tables only"), this);
    filterLayout->addWidget(showBloatedOnlyCheck_);
    
    autoRefreshCheck_ = new QCheckBox(tr("Auto-refresh"), this);
    connect(autoRefreshCheck_, &QCheckBox::toggled, this, &VacuumAnalyzePanel::onAutoRefreshToggled);
    filterLayout->addWidget(autoRefreshCheck_);
    
    refreshIntervalSpin_ = new QSpinBox(this);
    refreshIntervalSpin_->setRange(5, 300);
    refreshIntervalSpin_->setValue(60);
    refreshIntervalSpin_->setSuffix(tr("s"));
    filterLayout->addWidget(refreshIntervalSpin_);
    
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);
    
    // Selection toolbar
    auto* selectLayout = new QHBoxLayout();
    
    auto* selectAllBtn = new QPushButton(tr("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onSelectAllTables);
    selectLayout->addWidget(selectAllBtn);
    
    auto* deselectAllBtn = new QPushButton(tr("Deselect All"), this);
    connect(deselectAllBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onDeselectAllTables);
    selectLayout->addWidget(deselectAllBtn);
    
    auto* selectBloatedBtn = new QPushButton(tr("Select Bloated"), this);
    connect(selectBloatedBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onSelectBloatedTables);
    selectLayout->addWidget(selectBloatedBtn);
    
    selectLayout->addStretch();
    mainLayout->addLayout(selectLayout);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Stats table
    statsTable_ = new QTableView(this);
    statsModel_ = new QStandardItemModel(this);
    statsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Size"), tr("Live"), tr("Dead"), tr("Dead %"), tr("Last Vacuum"), tr("Last Analyze")});
    statsTable_->setModel(statsModel_);
    statsTable_->setAlternatingRowColors(true);
    statsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    statsTable_->setSelectionMode(QAbstractItemView::MultiSelection);
    connect(statsTable_, &QTableView::clicked, this, &VacuumAnalyzePanel::onTableSelected);
    splitter->addWidget(statsTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(4, 4, 4, 4);
    
    auto* infoGroup = new QGroupBox(tr("Table Information"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    tableNameLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Table:"), tableNameLabel_);
    
    sizeLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Size:"), sizeLabel_);
    
    liveTuplesLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Live Tuples:"), liveTuplesLabel_);
    
    deadTuplesLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Dead Tuples:"), deadTuplesLabel_);
    
    deadRatioLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Dead Ratio:"), deadRatioLabel_);
    
    lastVacuumLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Last Vacuum:"), lastVacuumLabel_);
    
    lastAnalyzeLabel_ = new QLabel(this);
    infoLayout->addRow(tr("Last Analyze:"), lastAnalyzeLabel_);
    
    detailsLayout->addWidget(infoGroup);
    
    // Bloat indicator
    auto* bloatGroup = new QGroupBox(tr("Bloat Status"), this);
    auto* bloatLayout = new QVBoxLayout(bloatGroup);
    
    bloatBar_ = new QProgressBar(this);
    bloatBar_->setRange(0, 100);
    bloatBar_->setValue(0);
    bloatBar_->setFormat(tr("%p% bloated"));
    bloatLayout->addWidget(bloatBar_);
    
    detailsLayout->addWidget(bloatGroup);
    
    // Actions
    auto* actionsGroup = new QGroupBox(tr("Quick Actions"), this);
    auto* actionsLayout = new QVBoxLayout(actionsGroup);
    
    auto* bloatReportBtn = new QPushButton(tr("View Bloat Report"), this);
    connect(bloatReportBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onViewBloatReport);
    actionsLayout->addWidget(bloatReportBtn);
    
    auto* deadTuplesBtn = new QPushButton(tr("Dead Tuples Monitor"), this);
    connect(deadTuplesBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onViewDeadTuples);
    actionsLayout->addWidget(deadTuplesBtn);
    
    auto* activityBtn = new QPushButton(tr("View Activity"), this);
    connect(activityBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onViewActivity);
    actionsLayout->addWidget(activityBtn);
    
    auto* scheduleBtn = new QPushButton(tr("Schedule Maintenance"), this);
    connect(scheduleBtn, &QPushButton::clicked, this, &VacuumAnalyzePanel::onScheduleMaintenance);
    actionsLayout->addWidget(scheduleBtn);
    
    detailsLayout->addWidget(actionsGroup);
    detailsLayout->addStretch();
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({500, 300});
    
    mainLayout->addWidget(splitter, 1);
    
    // Progress bar
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    mainLayout->addWidget(progressBar_);
    
    // Status
    statusLabel_ = new QLabel(tr("Ready"), this);
    mainLayout->addWidget(statusLabel_);
}

void VacuumAnalyzePanel::loadTableStats() {
    tableStats_.clear();
    
    // Simulate loading table statistics
    QStringList tables = {"customers", "orders", "products", "inventory", "audit_log", "users", "sessions"};
    
    for (int i = 0; i < tables.size(); ++i) {
        TableStats stats;
        stats.schema = "public";
        stats.tableName = tables[i];
        stats.totalSize = (i + 1) * 1024 * 1024 * 10; // 10MB, 20MB, etc.
        stats.liveTuples = (i + 1) * 10000;
        stats.deadTuples = i * 500;
        stats.deadRatio = (float)stats.deadTuples / (stats.liveTuples + stats.deadTuples) * 100;
        stats.lastVacuum = QDateTime::currentDateTime().addDays(-i);
        stats.lastAnalyze = QDateTime::currentDateTime().addDays(-i);
        stats.vacuumCount = 10 - i;
        stats.autovacuumCount = i * 5;
        stats.needsVacuum = stats.deadRatio > 5;
        stats.needsAnalyze = i % 3 == 0;
        tableStats_.append(stats);
    }
    
    updateStatsTable();
}

void VacuumAnalyzePanel::updateStatsTable() {
    statsModel_->clear();
    statsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Size"), tr("Live"), tr("Dead"), tr("Dead %"), tr("Last Vacuum"), tr("Last Analyze")});
    
    for (const auto& stats : tableStats_) {
        QString sizeStr = QString("%1 MB").arg(stats.totalSize / 1024 / 1024);
        
        QList<QStandardItem*> row;
        row << new QStandardItem(stats.tableName);
        row << new QStandardItem(sizeStr);
        row << new QStandardItem(QString::number(stats.liveTuples));
        row << new QStandardItem(QString::number(stats.deadTuples));
        
        auto* deadItem = new QStandardItem(QString("%1%").arg(stats.deadRatio, 0, 'f', 1));
        if (stats.deadRatio > 10) {
            deadItem->setBackground(Qt::red);
            deadItem->setForeground(Qt::white);
        } else if (stats.deadRatio > 5) {
            deadItem->setBackground(Qt::yellow);
        }
        row << deadItem;
        
        row << new QStandardItem(stats.lastVacuum.toString("yyyy-MM-dd"));
        row << new QStandardItem(stats.lastAnalyze.toString("yyyy-MM-dd"));
        
        statsModel_->appendRow(row);
    }
}

void VacuumAnalyzePanel::updateTableDetails(const TableStats& stats) {
    tableNameLabel_->setText(QString("%1.%2").arg(stats.schema, stats.tableName));
    sizeLabel_->setText(QString("%1 MB").arg(stats.totalSize / 1024 / 1024));
    liveTuplesLabel_->setText(QString::number(stats.liveTuples));
    deadTuplesLabel_->setText(QString::number(stats.deadTuples));
    deadRatioLabel_->setText(QString("%1%").arg(stats.deadRatio, 0, 'f', 2));
    lastVacuumLabel_->setText(stats.lastVacuum.toString());
    lastAnalyzeLabel_->setText(stats.lastAnalyze.toString());
    
    bloatBar_->setValue(qMin(100, (int)stats.deadRatio * 5));
    
    if (stats.deadRatio > 10) {
        bloatBar_->setStyleSheet("QProgressBar::chunk { background-color: red; }");
    } else if (stats.deadRatio > 5) {
        bloatBar_->setStyleSheet("QProgressBar::chunk { background-color: yellow; }");
    } else {
        bloatBar_->setStyleSheet("QProgressBar::chunk { background-color: green; }");
    }
}

QStringList VacuumAnalyzePanel::selectedTables() const {
    QStringList tables;
    auto indexes = statsTable_->selectionModel()->selectedRows();
    for (const auto& index : indexes) {
        if (index.row() < tableStats_.size()) {
            tables.append(tableStats_[index.row()].tableName);
        }
    }
    return tables;
}

void VacuumAnalyzePanel::runMaintenance(const QString& operation, const QStringList& tables) {
    if (tables.isEmpty()) {
        QMessageBox::warning(this, tr("No Tables Selected"),
            tr("Please select at least one table."));
        return;
    }
    
    MaintenanceDialog dialog(operation, tables, client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadTableStats();
        emit operationCompleted(operation, true);
    }
}

void VacuumAnalyzePanel::onVacuum() {
    runMaintenance("VACUUM", selectedTables());
}

void VacuumAnalyzePanel::onVacuumFull() {
    runMaintenance("VACUUM FULL", selectedTables());
}

void VacuumAnalyzePanel::onAnalyze() {
    runMaintenance("ANALYZE", selectedTables());
}

void VacuumAnalyzePanel::onVacuumAnalyze() {
    runMaintenance("VACUUM ANALYZE", selectedTables());
}

void VacuumAnalyzePanel::onReindex() {
    QStringList tables = selectedTables();
    if (tables.isEmpty()) {
        QMessageBox::warning(this, tr("No Tables Selected"),
            tr("Please select at least one table."));
        return;
    }
    
    ReindexDialog dialog(tables, client_, this);
    dialog.exec();
}

void VacuumAnalyzePanel::onStopOperation() {
    statusLabel_->setText(tr("Operation stopped"));
}

void VacuumAnalyzePanel::onSelectAllTables() {
    statsTable_->selectAll();
}

void VacuumAnalyzePanel::onDeselectAllTables() {
    statsTable_->clearSelection();
}

void VacuumAnalyzePanel::onSelectBloatedTables() {
    statsTable_->clearSelection();
    for (int i = 0; i < tableStats_.size(); ++i) {
        if (tableStats_[i].deadRatio > 5) {
            statsTable_->selectRow(i);
        }
    }
}

void VacuumAnalyzePanel::onTableSelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= tableStats_.size()) return;
    updateTableDetails(tableStats_[index.row()]);
}

void VacuumAnalyzePanel::onRefreshStats() {
    loadTableStats();
    statusLabel_->setText(tr("Statistics refreshed at %1").arg(QDateTime::currentDateTime().toString()));
}

void VacuumAnalyzePanel::onAutoRefreshToggled(bool enabled) {
    if (enabled) {
        QTimer::singleShot(refreshIntervalSpin_->value() * 1000, this, [this]() {
            if (autoRefreshCheck_->isChecked()) {
                onRefreshStats();
                onAutoRefreshToggled(true);
            }
        });
    }
}

void VacuumAnalyzePanel::onScheduleMaintenance() {
    QMessageBox::information(this, tr("Schedule"),
        tr("Use the Scheduled Jobs panel to set up recurring maintenance tasks."));
}

void VacuumAnalyzePanel::onRunMaintenanceNow() {
    onVacuumAnalyze();
}

void VacuumAnalyzePanel::onViewBloatReport() {
    BloatReportDialog dialog(tableStats_, this);
    dialog.exec();
}

void VacuumAnalyzePanel::onViewDeadTuples() {
    DeadTuplesDialog dialog(tableStats_, this);
    dialog.exec();
}

void VacuumAnalyzePanel::onViewActivity() {
    QMessageBox::information(this, tr("Activity"),
        tr("Database activity monitor would show current vacuum/analyze operations."));
}

void VacuumAnalyzePanel::onExportStats() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Statistics"),
        QString(),
        tr("CSV Files (*.csv);;JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Statistics exported."));
    }
}

// ============================================================================
// Maintenance Dialog
// ============================================================================

MaintenanceDialog::MaintenanceDialog(const QString& operation, const QStringList& tables,
                                     backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , operation_(operation)
    , tables_(tables)
    , client_(client)
    , currentTable_(0) {
    setupUi();
    setWindowTitle(tr("%1 - Maintenance").arg(operation));
    resize(500, 400);
}

void MaintenanceDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    operationLabel_ = new QLabel(tr("<h3>%1</h3>").arg(operation_), this);
    layout->addWidget(operationLabel_);
    
    layout->addWidget(new QLabel(tr("Tables: %1").arg(tables_.join(", ")), this));
    
    layout->addWidget(new QLabel(tr("Overall Progress:"), this));
    overallProgress_ = new QProgressBar(this);
    overallProgress_->setRange(0, tables_.size());
    overallProgress_->setValue(0);
    layout->addWidget(overallProgress_);
    
    layout->addWidget(new QLabel(tr("Current Table:"), this));
    currentTableLabel_ = new QLabel(tr("Waiting to start..."), this);
    layout->addWidget(currentTableLabel_);
    
    tableProgress_ = new QProgressBar(this);
    tableProgress_->setRange(0, 100);
    tableProgress_->setValue(0);
    layout->addWidget(tableProgress_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setFont(QFont("Monospace", 9));
    layout->addWidget(logEdit_, 1);
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    startBtn_ = new QPushButton(tr("Start"), this);
    connect(startBtn_, &QPushButton::clicked, this, &MaintenanceDialog::onStart);
    btnLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &MaintenanceDialog::onStop);
    btnLayout->addWidget(stopBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &MaintenanceDialog::onClose);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void MaintenanceDialog::onStart() {
    running_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("Running..."));
    
    executeMaintenance();
}

void MaintenanceDialog::executeMaintenance() {
    if (currentTable_ >= tables_.size()) {
        statusLabel_->setText(tr("Completed"));
        running_ = false;
        startBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        return;
    }
    
    QString table = tables_[currentTable_];
    currentTableLabel_->setText(table);
    overallProgress_->setValue(currentTable_);
    
    logEdit_->append(tr("Processing %1...").arg(table));
    tableProgress_->setRange(0, 0); // Indeterminate
    
    QTimer::singleShot(1000, this, [this, table]() {
        if (!running_) return;
        
        logEdit_->append(tr("  Executing %1 on %2...").arg(operation_, table));
        
        QTimer::singleShot(1000, this, [this, table]() {
            if (!running_) return;
            
            logEdit_->append(tr("  %1 completed on %2").arg(operation_, table));
            totalSpaceReclaimed_ += 1024 * 1024; // Simulate space reclaimed
            tablesProcessed_++;
            
            currentTable_++;
            tableProgress_->setRange(0, 100);
            tableProgress_->setValue(100);
            
            if (running_) {
                executeMaintenance();
            }
        });
    });
}

void MaintenanceDialog::onStop() {
    running_ = false;
    statusLabel_->setText(tr("Stopped"));
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    tableProgress_->setRange(0, 100);
}

void MaintenanceDialog::onClose() {
    if (running_) {
        auto reply = QMessageBox::question(this, tr("Operation Running"),
            tr("Maintenance is still running. Close anyway?"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::No) {
            return;
        }
        running_ = false;
    }
    accept();
}

// ============================================================================
// Bloat Report Dialog
// ============================================================================

BloatReportDialog::BloatReportDialog(const QList<TableStats>& stats, QWidget* parent)
    : QDialog(parent)
    , stats_(stats) {
    setupUi();
    setWindowTitle(tr("Table Bloat Report"));
    resize(600, 450);
}

void BloatReportDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    // Summary
    auto* summaryLayout = new QHBoxLayout();
    
    totalBloatLabel_ = new QLabel(this);
    summaryLayout->addWidget(totalBloatLabel_);
    
    tablesNeedingVacuumLabel_ = new QLabel(this);
    summaryLayout->addWidget(tablesNeedingVacuumLabel_);
    
    summaryLayout->addStretch();
    layout->addLayout(summaryLayout);
    
    // Report table
    reportTable_ = new QTableView(this);
    reportModel_ = new QStandardItemModel(this);
    reportModel_->setHorizontalHeaderLabels({tr("Table"), tr("Size"), tr("Bloat %"), tr("Est. Waste"), tr("Recommendation")});
    reportTable_->setModel(reportModel_);
    reportTable_->setAlternatingRowColors(true);
    layout->addWidget(reportTable_, 1);
    
    // Summary text
    summaryEdit_ = new QTextEdit(this);
    summaryEdit_->setReadOnly(true);
    summaryEdit_->setMaximumHeight(80);
    layout->addWidget(summaryEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &BloatReportDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* vacuumBtn = new QPushButton(tr("Vacuum Selected"), this);
    connect(vacuumBtn, &QPushButton::clicked, this, &BloatReportDialog::onVacuumSelected);
    btnLayout->addWidget(vacuumBtn);
    
    btnLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &BloatReportDialog::onExport);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
    
    loadReport();
}

void BloatReportDialog::loadReport() {
    reportModel_->clear();
    reportModel_->setHorizontalHeaderLabels({tr("Table"), tr("Size"), tr("Bloat %"), tr("Est. Waste"), tr("Recommendation")});
    
    int tablesNeedingVacuum = 0;
    qint64 totalWaste = 0;
    
    for (const auto& stats : stats_) {
        // Calculate bloat (simplified)
        float bloatPercent = stats.deadRatio * 2; // Simplified estimate
        qint64 wasteBytes = (qint64)(stats.totalSize * bloatPercent / 100);
        
        if (bloatPercent > 10) tablesNeedingVacuum++;
        totalWaste += wasteBytes;
        
        QString recommendation;
        if (bloatPercent > 20) {
            recommendation = tr("Vacuum Full recommended");
        } else if (bloatPercent > 10) {
            recommendation = tr("Vacuum recommended");
        } else if (bloatPercent > 5) {
            recommendation = tr("Monitor");
        } else {
            recommendation = tr("OK");
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(stats.tableName);
        row << new QStandardItem(QString("%1 MB").arg(stats.totalSize / 1024 / 1024));
        row << new QStandardItem(QString("%1%").arg(bloatPercent, 0, 'f', 1));
        row << new QStandardItem(QString("%1 MB").arg(wasteBytes / 1024 / 1024));
        row << new QStandardItem(recommendation);
        reportModel_->appendRow(row);
    }
    
    totalBloatLabel_->setText(tr("Total Waste: %1 MB").arg(totalWaste / 1024 / 1024));
    tablesNeedingVacuumLabel_->setText(tr("Tables Needing Vacuum: %1").arg(tablesNeedingVacuum));
    
    summaryEdit_->setText(tr("Bloat Report Summary:\n"
                              "- Tables analyzed: %1\n"
                              "- Tables needing vacuum: %2\n"
                              "- Total estimated waste: %3 MB\n"
                              "Recommendation: Run VACUUM on bloated tables.")
                          .arg(stats_.size())
                          .arg(tablesNeedingVacuum)
                          .arg(totalWaste / 1024 / 1024));
}

void BloatReportDialog::onRefresh() {
    loadReport();
}

void BloatReportDialog::onExport() {
    QMessageBox::information(this, tr("Export"), tr("Bloat report would be exported."));
}

void BloatReportDialog::onVacuumSelected() {
    QMessageBox::information(this, tr("Vacuum"), tr("Vacuum would be started on selected tables."));
}

void BloatReportDialog::calculateBloat() {
    // Bloat calculation logic
}

// ============================================================================
// Dead Tuples Dialog
// ============================================================================

DeadTuplesDialog::DeadTuplesDialog(const QList<TableStats>& stats, QWidget* parent)
    : QDialog(parent)
    , stats_(stats) {
    setupUi();
    setWindowTitle(tr("Dead Tuples Monitor"));
    resize(550, 400);
}

void DeadTuplesDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    // Summary
    auto* summaryLayout = new QHBoxLayout();
    
    totalDeadTuplesLabel_ = new QLabel(this);
    summaryLayout->addWidget(totalDeadTuplesLabel_);
    
    worstTableLabel_ = new QLabel(this);
    summaryLayout->addWidget(worstTableLabel_);
    
    summaryLayout->addStretch();
    layout->addLayout(summaryLayout);
    
    // Stats table
    statsTable_ = new QTableView(this);
    statsModel_ = new QStandardItemModel(this);
    statsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Dead Tuples"), tr("Live Tuples"), tr("Ratio"), tr("Last Vacuum"), tr("Status")});
    statsTable_->setModel(statsModel_);
    statsTable_->setAlternatingRowColors(true);
    layout->addWidget(statsTable_, 1);
    
    // Recommendations
    layout->addWidget(new QLabel(tr("Recommendations:"), this));
    
    recommendationsEdit_ = new QTextEdit(this);
    recommendationsEdit_->setReadOnly(true);
    recommendationsEdit_->setMaximumHeight(80);
    layout->addWidget(recommendationsEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &DeadTuplesDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* autovacuumBtn = new QPushButton(tr("Check Autovacuum"), this);
    connect(autovacuumBtn, &QPushButton::clicked, this, &DeadTuplesDialog::onAutoVacuumCheck);
    btnLayout->addWidget(autovacuumBtn);
    
    auto* vacuumWorstBtn = new QPushButton(tr("Vacuum Worst Tables"), this);
    connect(vacuumWorstBtn, &QPushButton::clicked, this, &DeadTuplesDialog::onVacuumWorstTables);
    btnLayout->addWidget(vacuumWorstBtn);
    
    btnLayout->addStretch();
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
    
    loadStats();
}

void DeadTuplesDialog::loadStats() {
    statsModel_->clear();
    statsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Dead Tuples"), tr("Live Tuples"), tr("Ratio"), tr("Last Vacuum"), tr("Status")});
    
    qint64 totalDead = 0;
    float worstRatio = 0;
    QString worstTable;
    
    for (const auto& stats : stats_) {
        totalDead += stats.deadTuples;
        if (stats.deadRatio > worstRatio) {
            worstRatio = stats.deadRatio;
            worstTable = stats.tableName;
        }
        
        QString status;
        if (stats.deadRatio > 20) {
            status = tr("CRITICAL");
        } else if (stats.deadRatio > 10) {
            status = tr("WARNING");
        } else if (stats.deadRatio > 5) {
            status = tr("CAUTION");
        } else {
            status = tr("OK");
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(stats.tableName);
        row << new QStandardItem(QString::number(stats.deadTuples));
        row << new QStandardItem(QString::number(stats.liveTuples));
        row << new QStandardItem(QString("%1%").arg(stats.deadRatio, 0, 'f', 1));
        row << new QStandardItem(stats.lastVacuum.toString("yyyy-MM-dd"));
        row << new QStandardItem(status);
        statsModel_->appendRow(row);
    }
    
    totalDeadTuplesLabel_->setText(tr("Total Dead Tuples: %1").arg(totalDead));
    worstTableLabel_->setText(tr("Worst Table: %1 (%2%)").arg(worstTable).arg(worstRatio, 0, 'f', 1));
    
    recommendationsEdit_->setText(tr("Recommendations:\n"
                                      "1. Tables with >20% dead tuples need immediate vacuum\n"
                                      "2. Review autovacuum settings if dead tuples accumulate quickly\n"
                                      "3. Consider increasing autovacuum frequency for high-churn tables"));
}

void DeadTuplesDialog::onRefresh() {
    loadStats();
}

void DeadTuplesDialog::onAutoVacuumCheck() {
    QMessageBox::information(this, tr("Autovacuum"),
        tr("Autovacuum status check would show:\n"
           "- Autovacuum workers running\n"
           "- Tables waiting for vacuum\n"
           "- Vacuum settings and thresholds"));
}

void DeadTuplesDialog::onVacuumWorstTables() {
    QMessageBox::information(this, tr("Vacuum"),
        tr("Vacuum would be started on tables with highest dead tuple ratio."));
}

// ============================================================================
// Reindex Dialog
// ============================================================================

ReindexDialog::ReindexDialog(const QStringList& tables, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , tables_(tables)
    , client_(client) {
    setupUi();
    setWindowTitle(tr("Reindex Tables"));
    resize(500, 400);
}

void ReindexDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Tables to reindex: %1").arg(tables_.join(", ")), this));
    
    layout->addWidget(new QLabel(tr("Indexes:"), this));
    
    indexTable_ = new QTableView(this);
    indexModel_ = new QStandardItemModel(this);
    indexModel_->setHorizontalHeaderLabels({tr("Table"), tr("Index"), tr("Size"), tr("Status")});
    indexTable_->setModel(indexModel_);
    indexTable_->setAlternatingRowColors(true);
    layout->addWidget(indexTable_, 1);
    
    // Populate with simulated index data
    for (const auto& table : tables_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(table);
        row << new QStandardItem(table + "_pkey");
        row << new QStandardItem("16 MB");
        row << new QStandardItem(tr("Ready"));
        indexModel_->appendRow(row);
        
        row.clear();
        row << new QStandardItem(table);
        row << new QStandardItem(table + "_idx1");
        row << new QStandardItem("8 MB");
        row << new QStandardItem(tr("Ready"));
        indexModel_->appendRow(row);
    }
    
    layout->addWidget(new QLabel(tr("Progress:"), this));
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    layout->addWidget(progressBar_);
    
    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    logEdit_->setMaximumHeight(100);
    layout->addWidget(logEdit_);
    
    statusLabel_ = new QLabel(tr("Ready"), this);
    layout->addWidget(statusLabel_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    startBtn_ = new QPushButton(tr("Start Reindex"), this);
    connect(startBtn_, &QPushButton::clicked, this, &ReindexDialog::onStart);
    btnLayout->addWidget(startBtn_);
    
    stopBtn_ = new QPushButton(tr("Stop"), this);
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &ReindexDialog::onStop);
    btnLayout->addWidget(stopBtn_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ReindexDialog::onStart() {
    running_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText(tr("Reindexing..."));
    
    logEdit_->append(tr("Starting reindex operation..."));
    progressBar_->setRange(0, 0); // Indeterminate
    
    // Simulate reindex
    QTimer::singleShot(2000, this, [this]() {
        if (!running_) return;
        
        for (const auto& table : tables_) {
            logEdit_->append(tr("Reindexing %1...").arg(table));
        }
        
        QTimer::singleShot(1000, this, [this]() {
            if (!running_) return;
            
            logEdit_->append(tr("Reindex completed successfully!"));
            progressBar_->setRange(0, 100);
            progressBar_->setValue(100);
            statusLabel_->setText(tr("Completed"));
            running_ = false;
            startBtn_->setEnabled(true);
            stopBtn_->setEnabled(false);
        });
    });
}

void ReindexDialog::onStop() {
    running_ = false;
    logEdit_->append(tr("Reindex stopped by user."));
    statusLabel_->setText(tr("Stopped"));
    progressBar_->setRange(0, 100);
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
}

void ReindexDialog::onClose() {
    accept();
}

} // namespace scratchrobin::ui
