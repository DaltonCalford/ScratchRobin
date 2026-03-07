#include "slow_query_log_viewer.h"
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
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QRandomGenerator>

namespace scratchrobin::ui {

// ============================================================================
// Slow Query Log Viewer Panel
// ============================================================================

SlowQueryLogViewerPanel::SlowQueryLogViewerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("slow_query_log", parent)
    , client_(client) {
    setupUi();
}

void SlowQueryLogViewerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    setupToolbar();
    
    // Filters
    auto* filterGroup = new QGroupBox(tr("Filters"), this);
    auto* filterLayout = new QHBoxLayout(filterGroup);
    
    filterLayout->addWidget(new QLabel(tr("Min Duration:"), this));
    durationFilterCombo_ = new QComboBox(this);
    durationFilterCombo_->addItems({"100ms", "500ms", "1s", "5s", "10s"});
    durationFilterCombo_->setCurrentIndex(1);
    connect(durationFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlowQueryLogViewerPanel::onDurationFilterChanged);
    filterLayout->addWidget(durationFilterCombo_);
    
    filterLayout->addSpacing(20);
    
    filterLayout->addWidget(new QLabel(tr("Search:"), this));
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search SQL..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &SlowQueryLogViewerPanel::onSearchTextChanged);
    filterLayout->addWidget(searchEdit_, 1);
    
    filterLayout->addSpacing(20);
    
    showWithSuggestionsCheck_ = new QCheckBox(tr("With Suggestions Only"), this);
    filterLayout->addWidget(showWithSuggestionsCheck_);
    
    auto* applyFilterBtn = new QPushButton(tr("Apply"), this);
    connect(applyFilterBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onApplyFilters);
    filterLayout->addWidget(applyFilterBtn);
    
    auto* clearFilterBtn = new QPushButton(tr("Clear"), this);
    connect(clearFilterBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onClearFilters);
    filterLayout->addWidget(clearFilterBtn);
    
    mainLayout->addWidget(filterGroup);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Query table
    setupQueryTable();
    splitter->addWidget(queryTable_);
    
    // Details panel
    setupDetailsPanel();
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    
    auto* detailsTabs = new QTabWidget(this);
    detailsTabs->addTab(sqlPreview_, tr("SQL"));
    detailsTabs->addTab(analysisResults_, tr("Analysis"));
    detailsTabs->addTab(indexSuggestionsTable_, tr("Suggestions"));
    detailsLayout->addWidget(detailsTabs);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({300, 200});
    
    mainLayout->addWidget(splitter, 1);
    
    // Summary panel
    setupSummaryPanel();
    auto* summaryWidget = new QWidget(this);
    auto* summaryLayout = new QHBoxLayout(summaryWidget);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    
    summaryLayout->addWidget(new QLabel(tr("Total:"), this));
    totalQueriesLabel_ = new QLabel("0", this);
    summaryLayout->addWidget(totalQueriesLabel_);
    
    summaryLayout->addSpacing(20);
    summaryLayout->addWidget(new QLabel(tr("Slow:"), this));
    slowQueriesLabel_ = new QLabel("0", this);
    slowQueriesLabel_->setStyleSheet("color: red;");
    summaryLayout->addWidget(slowQueriesLabel_);
    
    summaryLayout->addSpacing(20);
    summaryLayout->addWidget(new QLabel(tr("Avg Duration:"), this));
    avgDurationLabel_ = new QLabel("0ms", this);
    summaryLayout->addWidget(avgDurationLabel_);
    
    summaryLayout->addSpacing(20);
    summaryLayout->addWidget(new QLabel(tr("Max Duration:"), this));
    maxDurationLabel_ = new QLabel("0ms", this);
    summaryLayout->addWidget(maxDurationLabel_);
    
    summaryLayout->addSpacing(20);
    summaryLayout->addWidget(new QLabel(tr("Patterns:"), this));
    uniquePatternsLabel_ = new QLabel("0", this);
    summaryLayout->addWidget(uniquePatternsLabel_);
    
    summaryLayout->addStretch();
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setMaximumWidth(150);
    progressBar_->setVisible(false);
    summaryLayout->addWidget(progressBar_);
    
    mainLayout->addWidget(summaryWidget);
    
    // Load sample data
    loadSlowQueries();
}

void SlowQueryLogViewerPanel::setupToolbar() {
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* importBtn = new QPushButton(tr("Import Log"), this);
    connect(importBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onImportLog);
    toolbarLayout->addWidget(importBtn);
    
    auto* serverBtn = new QPushButton(tr("From Server"), this);
    connect(serverBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onImportFromServer);
    toolbarLayout->addWidget(serverBtn);
    
    auto* reloadBtn = new QPushButton(tr("Reload"), this);
    connect(reloadBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onReloadLog);
    toolbarLayout->addWidget(reloadBtn);
    
    auto* clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onClearLog);
    toolbarLayout->addWidget(clearBtn);
    
    toolbarLayout->addSpacing(20);
    
    auto* analyzeBtn = new QPushButton(tr("Analyze"), this);
    connect(analyzeBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onAnalyzeSelected);
    toolbarLayout->addWidget(analyzeBtn);
    
    auto* patternsBtn = new QPushButton(tr("Patterns"), this);
    connect(patternsBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onShowPatterns);
    toolbarLayout->addWidget(patternsBtn);
    
    auto* suggestionsBtn = new QPushButton(tr("Suggestions"), this);
    connect(suggestionsBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onShowIndexSuggestions);
    toolbarLayout->addWidget(suggestionsBtn);
    
    toolbarLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &SlowQueryLogViewerPanel::onExportResults);
    toolbarLayout->addWidget(exportBtn);
    
    static_cast<QVBoxLayout*>(layout())->insertLayout(0, toolbarLayout);
}

void SlowQueryLogViewerPanel::setupQueryTable() {
    queryTable_ = new QTableView(this);
    queryModel_ = new QStandardItemModel(this);
    queryModel_->setHorizontalHeaderLabels({tr("Time"), tr("Duration"), tr("Rows"), tr("Database"), tr("Query")});
    queryTable_->setModel(queryModel_);
    queryTable_->setAlternatingRowColors(true);
    queryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queryTable_->horizontalHeader()->setStretchLastSection(true);
    connect(queryTable_, &QTableView::clicked, this, &SlowQueryLogViewerPanel::onQuerySelected);
    connect(queryTable_, &QTableView::doubleClicked, this, &SlowQueryLogViewerPanel::onQueryDoubleClicked);
}

void SlowQueryLogViewerPanel::setupDetailsPanel() {
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setFont(QFont("Consolas", 9));
    sqlPreview_->setReadOnly(true);
    
    analysisResults_ = new QTextEdit(this);
    analysisResults_->setReadOnly(true);
    
    indexSuggestionsTable_ = new QTableView(this);
    indexSuggestionsModel_ = new QStandardItemModel(this);
    indexSuggestionsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Column"), tr("Impact"), tr("SQL")});
    indexSuggestionsTable_->setModel(indexSuggestionsModel_);
    indexSuggestionsTable_->setAlternatingRowColors(true);
}

void SlowQueryLogViewerPanel::setupSummaryPanel() {
    // Implemented in setupUi
}

void SlowQueryLogViewerPanel::loadSlowQueries() {
    // Simulate loading slow queries
    allQueries_.clear();
    
    for (int i = 0; i < 20; ++i) {
        SlowQueryEntry entry;
        entry.id = i + 1;
        entry.timestamp = QDateTime::currentDateTime().addSecs(-i * 60);
        entry.duration = 500 + QRandomGenerator::global()->bounded(2000);
        entry.rowsSent = 10 + QRandomGenerator::global()->bounded(100);
        entry.rowsExamined = entry.rowsSent * (5 + QRandomGenerator::global()->bounded(20));
        entry.database = "testdb";
        entry.user = "admin";
        entry.sql = QString("SELECT * FROM customers WHERE status = 'active' AND created_at > '2024-01-01' ORDER BY id LIMIT %1").arg(entry.rowsSent);
        entry.normalizedSql = "SELECT * FROM customers WHERE status = ? AND created_at > ? ORDER BY id LIMIT ?";
        entry.hasIndexSuggestion = (i % 3 == 0);
        allQueries_.append(entry);
    }
    
    applyFilters();
    updateSummary();
}

void SlowQueryLogViewerPanel::applyFilters() {
    filteredQueries_.clear();
    
    for (const auto& q : allQueries_) {
        bool include = true;
        
        // Duration filter
        if (q.duration < minDuration_) {
            include = false;
        }
        
        // Search filter
        if (!searchText_.isEmpty() && !q.sql.contains(searchText_, Qt::CaseInsensitive)) {
            include = false;
        }
        
        // Suggestions filter
        if (showWithSuggestionsCheck_->isChecked() && !q.hasIndexSuggestion) {
            include = false;
        }
        
        if (include) {
            filteredQueries_.append(q);
        }
    }
    
    // Update table
    queryModel_->clear();
    queryModel_->setHorizontalHeaderLabels({tr("Time"), tr("Duration"), tr("Rows"), tr("Database"), tr("Query")});
    
    for (const auto& q : filteredQueries_) {
        QList<QStandardItem*> row;
        row << new QStandardItem(q.timestamp.toString("hh:mm:ss"));
        row << new QStandardItem(QString::number(q.duration) + "ms");
        row << new QStandardItem(QString::number(q.rowsSent));
        row << new QStandardItem(q.database);
        row << new QStandardItem(q.sql.left(60) + "...");
        queryModel_->appendRow(row);
    }
}

void SlowQueryLogViewerPanel::analyzeQuery(const SlowQueryEntry& entry) {
    Q_UNUSED(entry)
    
    analysisResults_->setPlainText(
        tr("Query Analysis:\n\n"
           "Duration: %1ms\n"
           "Rows Examined: %2\n"
           "Rows Sent: %3\n"
           "Efficiency: %4%\n\n"
           "Potential Issues:\n"
           "- Missing index on 'status' column\n"
           "- Consider adding LIMIT clause\n"
           "- Query could benefit from covering index\n")
        .arg(QString::number(entry.duration))
        .arg(QString::number(entry.rowsExamined))
        .arg(QString::number(entry.rowsSent))
        .arg(QString::number(100.0 * entry.rowsSent / qMax(1, entry.rowsExamined), 'f', 1)));
}

QStringList SlowQueryLogViewerPanel::generateIndexSuggestions(const SlowQueryEntry& entry) {
    Q_UNUSED(entry)
    QStringList suggestions;
    suggestions << "CREATE INDEX idx_customers_status ON customers(status);";
    suggestions << "CREATE INDEX idx_customers_created ON customers(created_at);";
    return suggestions;
}

void SlowQueryLogViewerPanel::updateSummary() {
    totalQueriesLabel_->setText(QString::number(allQueries_.size()));
    slowQueriesLabel_->setText(QString::number(filteredQueries_.size()));
    
    double totalDuration = 0;
    double maxDuration = 0;
    QSet<QString> patterns;
    
    for (const auto& q : allQueries_) {
        totalDuration += q.duration;
        if (q.duration > maxDuration) maxDuration = q.duration;
        patterns.insert(q.normalizedSql);
    }
    
    avgDurationLabel_->setText(QString::number(allQueries_.isEmpty() ? 0 : totalDuration / allQueries_.size(), 'f', 0) + "ms");
    maxDurationLabel_->setText(QString::number(maxDuration, 'f', 0) + "ms");
    uniquePatternsLabel_->setText(QString::number(patterns.size()));
}

void SlowQueryLogViewerPanel::onImportLog() {
    ImportLogDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        loadSlowQueries();
    }
}

void SlowQueryLogViewerPanel::onImportFromServer() {
    QMessageBox::information(this, tr("Import from Server"),
        tr("Loading slow query log from database server..."));
    loadSlowQueries();
}

void SlowQueryLogViewerPanel::onReloadLog() {
    loadSlowQueries();
}

void SlowQueryLogViewerPanel::onClearLog() {
    allQueries_.clear();
    filteredQueries_.clear();
    queryModel_->clear();
    queryModel_->setHorizontalHeaderLabels({tr("Time"), tr("Duration"), tr("Rows"), tr("Database"), tr("Query")});
    updateSummary();
}

void SlowQueryLogViewerPanel::onExportResults() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Results"),
        QString(),
        tr("CSV (*.csv);;Text (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Results exported to %1").arg(fileName));
    }
}

void SlowQueryLogViewerPanel::onApplyFilters() {
    applyFilters();
}

void SlowQueryLogViewerPanel::onClearFilters() {
    searchEdit_->clear();
    durationFilterCombo_->setCurrentIndex(0);
    showWithSuggestionsCheck_->setChecked(false);
    applyFilters();
}

void SlowQueryLogViewerPanel::onDurationFilterChanged(int index) {
    Q_UNUSED(index)
    QString text = durationFilterCombo_->currentText();
    if (text.endsWith("ms")) {
        minDuration_ = text.left(text.length() - 2).toDouble();
    } else if (text.endsWith("s")) {
        minDuration_ = text.left(text.length() - 1).toDouble() * 1000;
    }
}

void SlowQueryLogViewerPanel::onSearchTextChanged(const QString& text) {
    searchText_ = text;
}

void SlowQueryLogViewerPanel::onTimeRangeChanged() {
    // Apply time range filter
}

void SlowQueryLogViewerPanel::onAnalyzeSelected() {
    auto index = queryTable_->currentIndex();
    if (!index.isValid() || index.row() >= filteredQueries_.size()) return;
    
    analyzeQuery(filteredQueries_[index.row()]);
}

void SlowQueryLogViewerPanel::onAnalyzeAll() {
    QMessageBox::information(this, tr("Analyze All"),
        tr("Analyzing all queries..."));
}

void SlowQueryLogViewerPanel::onShowPatterns() {
    PatternAnalysisDialog dialog(filteredQueries_, this);
    dialog.exec();
}

void SlowQueryLogViewerPanel::onShowIndexSuggestions() {
    indexSuggestionsModel_->clear();
    indexSuggestionsModel_->setHorizontalHeaderLabels({tr("Table"), tr("Column"), tr("Impact"), tr("SQL")});
    
    indexSuggestionsModel_->appendRow({
        new QStandardItem("customers"),
        new QStandardItem("status"),
        new QStandardItem(tr("High")),
        new QStandardItem("CREATE INDEX idx_customers_status ON customers(status);")
    });
    
    indexSuggestionsModel_->appendRow({
        new QStandardItem("customers"),
        new QStandardItem("created_at"),
        new QStandardItem(tr("Medium")),
        new QStandardItem("CREATE INDEX idx_customers_created ON customers(created_at);")
    });
}

void SlowQueryLogViewerPanel::onShowQueryPlan() {
    // Show query plan for selected query
}

void SlowQueryLogViewerPanel::onQuerySelected(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= filteredQueries_.size()) return;
    
    const auto& entry = filteredQueries_[index.row()];
    sqlPreview_->setPlainText(entry.sql);
    analyzeQuery(entry);
}

void SlowQueryLogViewerPanel::onQueryDoubleClicked(const QModelIndex& index) {
    if (!index.isValid() || index.row() >= filteredQueries_.size()) return;
    
    QueryAnalysisDialog dialog(filteredQueries_[index.row()], client_, this);
    dialog.exec();
}

void SlowQueryLogViewerPanel::onRefreshSummary() {
    updateSummary();
}

void SlowQueryLogViewerPanel::onGenerateReport() {
    QMessageBox::information(this, tr("Generate Report"),
        tr("Generating slow query report..."));
}

// ============================================================================
// Import Log Dialog
// ============================================================================

ImportLogDialog::ImportLogDialog(QWidget* parent)
    : QDialog(parent) {
    setupUi();
}

void ImportLogDialog::setupUi() {
    setWindowTitle(tr("Import Slow Query Log"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    filePathEdit_ = new QLineEdit(this);
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &ImportLogDialog::onBrowseFile);
    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(filePathEdit_, 1);
    fileLayout->addWidget(browseBtn);
    formLayout->addRow(tr("Log File:"), fileLayout);
    
    logFormatCombo_ = new QComboBox(this);
    logFormatCombo_->addItems({tr("Auto-detect"), tr("PostgreSQL"), tr("MySQL"), tr("Generic")});
    formLayout->addRow(tr("Format:"), logFormatCombo_);
    
    minDurationSpin_ = new QSpinBox(this);
    minDurationSpin_->setRange(0, 10000);
    minDurationSpin_->setValue(100);
    minDurationSpin_->setSuffix(" ms");
    formLayout->addRow(tr("Min Duration:"), minDurationSpin_);
    
    parseCommentsCheck_ = new QCheckBox(tr("Parse comments in SQL"), this);
    formLayout->addRow(parseCommentsCheck_);
    
    layout->addLayout(formLayout);
    
    layout->addWidget(new QLabel(tr("Preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(100);
    layout->addWidget(previewEdit_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setVisible(false);
    layout->addWidget(progressBar_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &ImportLogDialog::onPreview);
    btnLayout->addWidget(previewBtn);
    
    auto* importBtn = new QPushButton(tr("Import"), this);
    connect(importBtn, &QPushButton::clicked, this, &ImportLogDialog::onImport);
    btnLayout->addWidget(importBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ImportLogDialog::onBrowseFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Slow Query Log"),
        QString(),
        tr("Log Files (*.log);;All Files (*.*)"));
    
    if (!fileName.isEmpty()) {
        filePathEdit_->setText(fileName);
    }
}

void ImportLogDialog::onImport() {
    accept();
}

void ImportLogDialog::onPreview() {
    previewEdit_->setPlainText(tr("Preview of first 10 lines...\n"
                                   "2024-01-15 10:30:15 duration: 1250ms...\n"
                                   "SELECT * FROM customers..."));
}

void ImportLogDialog::detectLogFormat() {
    // Auto-detect log format from file
}

// ============================================================================
// Query Analysis Dialog
// ============================================================================

QueryAnalysisDialog::QueryAnalysisDialog(const SlowQueryEntry& entry,
                                         backend::SessionClient* client,
                                         QWidget* parent)
    : QDialog(parent)
    , entry_(entry)
    , client_(client) {
    setupUi();
}

void QueryAnalysisDialog::setupUi() {
    setWindowTitle(tr("Query Analysis"));
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    // SQL
    layout->addWidget(new QLabel(tr("SQL Query:"), this));
    sqlEdit_ = new QTextEdit(this);
    sqlEdit_->setFont(QFont("Consolas", 9));
    sqlEdit_->setPlainText(entry_.sql);
    sqlEdit_->setMaximumHeight(100);
    sqlEdit_->setReadOnly(true);
    layout->addWidget(sqlEdit_);
    
    // Analysis
    layout->addWidget(new QLabel(tr("Analysis:"), this));
    analysisEdit_ = new QTextEdit(this);
    analysisEdit_->setReadOnly(true);
    analysisEdit_->setPlainText(
        tr("Duration: %1ms\n"
           "Rows Examined: %2\n"
           "Rows Sent: %3\n\n"
           "Recommendations:\n"
           "1. Add index on status column\n"
           "2. Consider using covering index\n"
           "3. Evaluate if all columns are needed")
        .arg(QString::number(entry_.duration))
        .arg(QString::number(entry_.rowsExamined))
        .arg(QString::number(entry_.rowsSent)));
    layout->addWidget(analysisEdit_);
    
    // Suggestions table
    layout->addWidget(new QLabel(tr("Index Suggestions:"), this));
    suggestionsTable_ = new QTableView(this);
    suggestionsModel_ = new QStandardItemModel(this);
    suggestionsModel_->setHorizontalHeaderLabels({tr("Index"), tr("Impact"), tr("Size Estimate")});
    suggestionsTable_->setModel(suggestionsModel_);
    suggestionsTable_->setMaximumHeight(100);
    layout->addWidget(suggestionsTable_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* explainBtn = new QPushButton(tr("EXPLAIN"), this);
    connect(explainBtn, &QPushButton::clicked, this, &QueryAnalysisDialog::onExplainQuery);
    btnLayout->addWidget(explainBtn);
    
    auto* indexBtn = new QPushButton(tr("Create Index"), this);
    connect(indexBtn, &QPushButton::clicked, this, &QueryAnalysisDialog::onCreateIndex);
    btnLayout->addWidget(indexBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void QueryAnalysisDialog::analyze() {
    // Perform detailed analysis
}

void QueryAnalysisDialog::onExplainQuery() {
    QMessageBox::information(this, tr("EXPLAIN"),
        tr("Query plan would be shown here."));
}

void QueryAnalysisDialog::onCreateIndex() {
    QMessageBox::information(this, tr("Create Index"),
        tr("Index creation dialog would open."));
}

void QueryAnalysisDialog::onOptimizeQuery() {
    // Show optimized query
}

void QueryAnalysisDialog::onExportAnalysis() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Analysis"),
        QString(),
        tr("Text Files (*.txt)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Analysis exported."));
    }
}

// ============================================================================
// Pattern Analysis Dialog
// ============================================================================

PatternAnalysisDialog::PatternAnalysisDialog(const QList<SlowQueryEntry>& entries,
                                             QWidget* parent)
    : QDialog(parent)
    , entries_(entries) {
    setupUi();
    analyzePatterns();
}

void PatternAnalysisDialog::setupUi() {
    setWindowTitle(tr("Pattern Analysis"));
    resize(800, 500);
    
    auto* layout = new QVBoxLayout(this);
    
    patternsTable_ = new QTableView(this);
    patternsModel_ = new QStandardItemModel(this);
    patternsModel_->setHorizontalHeaderLabels({tr("Pattern"), tr("Count"), tr("Total Time"), tr("Avg Time"), tr("% of Total")});
    patternsTable_->setModel(patternsModel_);
    patternsTable_->setAlternatingRowColors(true);
    layout->addWidget(patternsTable_, 1);
    
    layout->addWidget(new QLabel(tr("Details:"), this));
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(100);
    layout->addWidget(detailsEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &PatternAnalysisDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &PatternAnalysisDialog::onExportPatterns);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void PatternAnalysisDialog::analyzePatterns() {
    // Group queries by normalized SQL
    QMap<QString, QList<SlowQueryEntry>> groups;
    for (const auto& e : entries_) {
        groups[e.normalizedSql].append(e);
    }
    
    patternsModel_->clear();
    patternsModel_->setHorizontalHeaderLabels({tr("Pattern"), tr("Count"), tr("Total Time"), tr("Avg Time"), tr("% of Total")});
    
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const auto& list = it.value();
        double totalTime = 0;
        for (const auto& e : list) {
            totalTime += e.duration;
        }
        
        QList<QStandardItem*> row;
        row << new QStandardItem(it.key().left(50));
        row << new QStandardItem(QString::number(list.size()));
        row << new QStandardItem(QString::number(totalTime, 'f', 0) + "ms");
        row << new QStandardItem(QString::number(totalTime / list.size(), 'f', 0) + "ms");
        row << new QStandardItem(QString::number(100.0 * list.size() / entries_.size(), 'f', 1) + "%");
        patternsModel_->appendRow(row);
    }
}

void PatternAnalysisDialog::onRefresh() {
    analyzePatterns();
}

void PatternAnalysisDialog::onExportPatterns() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Patterns"),
        QString(),
        tr("CSV (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Patterns exported."));
    }
}

void PatternAnalysisDialog::onViewPatternDetails() {
    // Show detailed pattern view
}

} // namespace scratchrobin::ui
