#include "ui/explain_plan_viewer.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QCheckBox>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

namespace scratchrobin::ui {

// ============================================================================
// ExplainPlanDialog
// ============================================================================

ExplainPlanDialog::ExplainPlanDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setWindowTitle(tr("Query Execution Plan"));
    setMinimumSize(1000, 700);
    resize(1200, 800);
    
    setupUi();
}

void ExplainPlanDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    toolbarLayout->addWidget(new QLabel(tr("Format:"), this));
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItem(tr("Text"), "text");
    formatCombo_->addItem(tr("JSON"), "json");
    formatCombo_->addItem(tr("XML"), "xml");
    toolbarLayout->addWidget(formatCombo_);
    
    toolbarLayout->addWidget(new QLabel(tr("Mode:"), this));
    analyzeCombo_ = new QComboBox(this);
    analyzeCombo_->addItem(tr("EXPLAIN"), false);
    analyzeCombo_->addItem(tr("EXPLAIN ANALYZE"), true);
    toolbarLayout->addWidget(analyzeCombo_);
    
    analyzeBtn_ = new QPushButton(tr("&Analyze"), this);
    analyzeBtn_->setDefault(true);
    toolbarLayout->addWidget(analyzeBtn_);
    
    toolbarLayout->addSpacing(20);
    
    auto* exportBtn = new QPushButton(tr("&Export..."), this);
    toolbarLayout->addWidget(exportBtn);
    
    auto* compareBtn = new QPushButton(tr("&Compare..."), this);
    toolbarLayout->addWidget(compareBtn);
    
    toolbarLayout->addStretch();
    
    auto* costsCheck = new QCheckBox(tr("Show &Costs"), this);
    costsCheck->setChecked(true);
    toolbarLayout->addWidget(costsCheck);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Splitter for query and results
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Query editor
    queryEdit_ = new QTextEdit(this);
    queryEdit_->setPlaceholderText(tr("Enter SQL query to analyze..."));
    queryEdit_->setFont(QFont("Consolas", 10));
    queryEdit_->setMaximumHeight(150);
    splitter->addWidget(queryEdit_);
    
    // Results tabs
    tabWidget_ = new QTabWidget(this);
    
    // Plan tree tab
    planTree_ = new PlanTreeWidget(this);
    planTree_->setHeaderLabels({tr("Operation"), tr("Cost"), tr("Rows"), tr("Time"), tr("Table/Index")});
    planTree_->header()->setStretchLastSection(true);
    planTree_->setColumnWidth(0, 300);
    planTree_->setColumnWidth(1, 100);
    planTree_->setColumnWidth(2, 80);
    planTree_->setColumnWidth(3, 80);
    tabWidget_->addTab(planTree_, tr("Execution Plan"));
    
    // Details tab
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setFont(QFont("Consolas", 9));
    tabWidget_->addTab(detailsEdit_, tr("Details"));
    
    // Statistics tab
    statsTable_ = new QTableWidget(this);
    statsTable_->setColumnCount(2);
    statsTable_->setHorizontalHeaderLabels({tr("Metric"), tr("Value")});
    statsTable_->horizontalHeader()->setStretchLastSection(true);
    tabWidget_->addTab(statsTable_, tr("Statistics"));
    
    // Warnings tab
    warningsEdit_ = new QTextEdit(this);
    warningsEdit_->setReadOnly(true);
    tabWidget_->addTab(warningsEdit_, tr("Warnings & Suggestions"));
    
    splitter->addWidget(tabWidget_);
    splitter->setSizes({150, 500});
    mainLayout->addWidget(splitter, 1);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    statusLayout->addWidget(new QLabel(tr("Total Cost:"), this));
    auto* costLabel = new QLabel(tr("N/A"), this);
    statusLayout->addWidget(costLabel);
    
    statusLayout->addSpacing(20);
    statusLayout->addWidget(new QLabel(tr("Est. Rows:"), this));
    auto* rowsLabel = new QLabel(tr("N/A"), this);
    statusLayout->addWidget(rowsLabel);
    
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
    
    // Connections
    connect(analyzeBtn_, &QPushButton::clicked, this, &ExplainPlanDialog::onAnalyzeClicked);
    connect(exportBtn, &QPushButton::clicked, this, &ExplainPlanDialog::exportPlan);
    connect(compareBtn, &QPushButton::clicked, this, &ExplainPlanDialog::onComparePlans);
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExplainPlanDialog::onFormatChanged);
    connect(planTree_, &QTreeWidget::itemClicked, this, &ExplainPlanDialog::onNodeSelected);
    connect(costsCheck, &QCheckBox::toggled, this, &ExplainPlanDialog::onToggleCosts);
}

void ExplainPlanDialog::analyzeQuery(const QString& sql) {
    queryEdit_->setPlainText(sql);
    onAnalyzeClicked();
}

void ExplainPlanDialog::onAnalyzeClicked() {
    QString sql = queryEdit_->toPlainText().trimmed();
    if (sql.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Query"), tr("Please enter a SQL query to analyze."));
        return;
    }
    
    bool useAnalyze = analyzeCombo_->currentData().toBool();
    QString format = formatCombo_->currentData().toString();
    
    // Build EXPLAIN statement
    QString explainSql = useAnalyze ? "EXPLAIN ANALYZE " : "EXPLAIN ";
    if (format == "json") explainSql += "(FORMAT JSON) ";
    else if (format == "xml") explainSql += "(FORMAT XML) ";
    explainSql += sql;
    
    // Mock execution - would use client_->execute() in real implementation
    // For now, generate a mock plan
    QueryPlan mockPlan;
    mockPlan.query = sql;
    mockPlan.isAnalyzed = useAnalyze;
    mockPlan.totalCost = 1500.50;
    mockPlan.totalRows = 5000;
    
    // Create mock plan tree
    QueryPlanNode root;
    root.operation = "Hash Join";
    root.cost = 1500.50;
    root.startupCost = 100.0;
    root.estimatedRows = 5000;
    root.actualTime = 125.5;
    root.hashCond = "(orders.user_id = users.id)";
    
    QueryPlanNode child1;
    child1.operation = "Seq Scan";
    child1.tableName = "orders";
    child1.cost = 800.0;
    child1.estimatedRows = 10000;
    child1.isSeqScan = true;
    root.children.append(child1);
    
    QueryPlanNode child2;
    child2.operation = "Index Scan";
    child2.tableName = "users";
    child2.indexName = "users_pkey";
    child2.cost = 200.0;
    child2.estimatedRows = 1000;
    child2.isIndexScan = true;
    child2.usesIndex = true;
    root.children.append(child2);
    
    mockPlan.rootNodes.append(root);
    
    // Add warnings
    if (child1.isSeqScan && child1.estimatedRows > 1000) {
        mockPlan.warnings.append(tr("Sequential scan on 'orders' with %1 rows - consider adding an index")
                                .arg(child1.estimatedRows));
        mockPlan.suggestions.append(tr("CREATE INDEX idx_orders_user_id ON orders(user_id);"));
    }
    
    currentPlan_ = mockPlan;
    planHistory_.append(mockPlan);
    
    displayPlan(mockPlan);
}

void ExplainPlanDialog::displayPlan(const QueryPlan& plan) {
    // Clear existing
    planTree_->clear();
    
    // Populate tree
    for (const auto& node : plan.rootNodes) {
        auto* item = new QTreeWidgetItem(planTree_);
        populateTree(item, node);
        planTree_->addTopLevelItem(item);
        item->setExpanded(true);
    }
    
    // Update statistics
    statsTable_->setRowCount(0);
    int row = 0;
    auto addStat = [&](const QString& name, const QString& value) {
        statsTable_->insertRow(row);
        statsTable_->setItem(row, 0, new QTableWidgetItem(name));
        statsTable_->setItem(row, 1, new QTableWidgetItem(value));
        row++;
    };
    
    addStat(tr("Total Cost"), QString::number(plan.totalCost, 'f', 2));
    addStat(tr("Startup Cost"), QString::number(plan.startupCost, 'f', 2));
    addStat(tr("Estimated Rows"), QString::number(plan.totalRows));
    addStat(tr("Plan Width"), QString::number(plan.planWidth));
    if (plan.isAnalyzed) {
        addStat(tr("Planning Time"), plan.planningTime);
        addStat(tr("Execution Time"), plan.executionTime);
    }
    
    // Update warnings
    warningsEdit_->clear();
    if (!plan.warnings.isEmpty()) {
        warningsEdit_->append(tr("<b>Warnings:</b>"));
        for (const auto& warning : plan.warnings) {
            warningsEdit_->append(tr("⚠ %1").arg(warning));
        }
    }
    if (!plan.suggestions.isEmpty()) {
        warningsEdit_->append(tr("\n<b>Suggestions:</b>"));
        for (const auto& suggestion : plan.suggestions) {
            warningsEdit_->append(tr("💡 %1").arg(suggestion));
        }
    }
    
    // Show details
    detailsEdit_->setPlainText(tr("Query:\n%1\n\nTotal Cost: %2\nEstimated Rows: %3")
                               .arg(plan.query)
                               .arg(plan.totalCost)
                               .arg(plan.totalRows));
}

void ExplainPlanDialog::populateTree(QTreeWidgetItem* parent, const QueryPlanNode& node) {
    parent->setText(0, node.operation);
    parent->setText(1, QString::number(node.cost, 'f', 2));
    parent->setText(2, QString::number(node.estimatedRows));
    parent->setText(3, node.actualTime > 0 ? QString::number(node.actualTime, 'f', 2) : "-");
    
    QString tableInfo;
    if (!node.tableName.isEmpty()) {
        tableInfo = node.tableName;
        if (!node.indexName.isEmpty()) {
            tableInfo += tr(" (using %1)").arg(node.indexName);
        }
    }
    parent->setText(4, tableInfo);
    
    // Color coding
    if (node.isSeqScan && node.estimatedRows > 1000) {
        parent->setBackground(0, QColor(255, 200, 200));  // Red-ish for expensive seq scan
    } else if (node.usesIndex) {
        parent->setBackground(0, QColor(200, 255, 200));  // Green for index usage
    }
    
    // Add children
    for (const auto& child : node.children) {
        auto* childItem = new QTreeWidgetItem(parent);
        populateTree(childItem, child);
    }
}

void ExplainPlanDialog::onNodeSelected() {
    auto* item = planTree_->currentItem();
    if (!item) return;
    
    // Show details for selected node
    QString details = tr("Operation: %1\n").arg(item->text(0));
    details += tr("Cost: %1\n").arg(item->text(1));
    details += tr("Rows: %1\n").arg(item->text(2));
    
    detailsEdit_->setPlainText(details);
}

void ExplainPlanDialog::onFormatChanged(int index) {
    Q_UNUSED(index)
    // Would update display format
}

void ExplainPlanDialog::onToggleCosts(bool show) {
    auto* planTree = qobject_cast<PlanTreeWidget*>(planTree_);
    if (planTree) {
        planTree->setShowCosts(show);
    }
}

void ExplainPlanDialog::onComparePlans() {
    if (planHistory_.size() < 2) {
        QMessageBox::information(this, tr("Compare Plans"),
            tr("Need at least 2 plans to compare. Analyze multiple queries first."));
        return;
    }
    
    PlanComparisonDialog dlg(planHistory_, this);
    dlg.exec();
}

void ExplainPlanDialog::refresh() {
    // Refresh current plan display
}

void ExplainPlanDialog::exportPlan() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Plan"),
        QString(), tr("JSON (*.json);;Text (*.txt);;HTML (*.html)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Export Error"),
            tr("Could not open file for writing."));
        return;
    }
    
    if (fileName.endsWith(".json")) {
        QJsonObject root;
        root["query"] = currentPlan_.query;
        root["totalCost"] = currentPlan_.totalCost;
        root["totalRows"] = static_cast<qint64>(currentPlan_.totalRows);
        
        QJsonDocument doc(root);
        file.write(doc.toJson());
    } else if (fileName.endsWith(".html")) {
        QTextStream stream(&file);
        stream << "<html><body>\n";
        stream << "<h1>Query Execution Plan</h1>\n";
        stream << "<pre>" << currentPlan_.query << "</pre>\n";
        stream << "<p>Total Cost: " << currentPlan_.totalCost << "</p>\n";
        stream << "</body></html>\n";
    } else {
        QTextStream stream(&file);
        stream << "Query: " << currentPlan_.query << "\n\n";
        stream << "Total Cost: " << currentPlan_.totalCost << "\n";
        stream << "Estimated Rows: " << currentPlan_.totalRows << "\n";
    }
    
    file.close();
    
    QMessageBox::information(this, tr("Export Complete"),
        tr("Plan exported to:\n%1").arg(fileName));
}

void ExplainPlanDialog::analyzePlanIssues(const QueryPlan& plan) {
    Q_UNUSED(plan)
    // Analyze plan for common issues
}

void ExplainPlanDialog::updateStatistics(const QueryPlan& plan) {
    Q_UNUSED(plan)
    // Update statistics display
}

QueryPlan ExplainPlanDialog::parseExplainOutput(const QString& output) {
    // Parse database-specific EXPLAIN output
    Q_UNUSED(output)
    return QueryPlan();
}

QueryPlan ExplainPlanDialog::parseJsonExplain(const QString& json) {
    Q_UNUSED(json)
    return QueryPlan();
}

QueryPlan ExplainPlanDialog::parseTextExplain(const QString& text) {
    Q_UNUSED(text)
    return QueryPlan();
}

// ============================================================================
// PlanTreeWidget
// ============================================================================

PlanTreeWidget::PlanTreeWidget(QWidget* parent)
    : QTreeWidget(parent) {
}

void PlanTreeWidget::setShowCosts(bool show) {
    showCosts_ = show;
    setColumnHidden(1, !show);
    setColumnHidden(2, !show);
    setColumnHidden(3, !show);
}

void PlanTreeWidget::setHighlightExpensive(bool highlight) {
    highlightExpensive_ = highlight;
    viewport()->update();
}

void PlanTreeWidget::drawRow(QPainter* painter, const QStyleOptionViewItem& options,
                             const QModelIndex& index) const {
    QTreeWidget::drawRow(painter, options, index);
    
    if (!showCosts_) return;
    
    // Draw cost bar in background
    double cost = index.sibling(index.row(), 1).data().toDouble();
    if (cost > 0 && maxCost_ > 0) {
        int width = static_cast<int>((cost / maxCost_) * 100);
        QRect rect = options.rect;
        rect.setWidth(width);
        rect.setHeight(3);
        rect.moveTop(rect.bottom() - 3);
        
        painter->fillRect(rect, cost > maxCost_ * 0.5 ? Qt::red : Qt::green);
    }
}

// ============================================================================
// PlanComparisonDialog
// ============================================================================

PlanComparisonDialog::PlanComparisonDialog(const QList<QueryPlan>& plans, QWidget* parent)
    : QDialog(parent), plans_(plans) {
    setWindowTitle(tr("Compare Query Plans"));
    setMinimumSize(900, 600);
    
    setupUi();
}

void PlanComparisonDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    tabWidget_ = new QTabWidget(this);
    
    // Comparison table
    auto* tableWidget = new QWidget(this);
    auto* tableLayout = new QVBoxLayout(tableWidget);
    createComparisonTable();
    tableLayout->addWidget(new QLabel(tr("Plan Comparison"), this));
    tabWidget_->addTab(tableWidget, tr("Table"));
    
    // Cost chart
    auto* chartWidget = new QWidget(this);
    auto* chartLayout = new QVBoxLayout(chartWidget);
    chartLayout->addWidget(new QLabel(tr("Cost Comparison Chart"), this));
    tabWidget_->addTab(chartWidget, tr("Chart"));
    
    // Improvements
    auto* improveWidget = new QWidget(this);
    auto* improveLayout = new QVBoxLayout(improveWidget);
    createImprovementSummary();
    improveLayout->addWidget(new QLabel(tr("Performance Improvements"), this));
    tabWidget_->addTab(improveWidget, tr("Improvements"));
    
    mainLayout->addWidget(tabWidget_);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn);
}

void PlanComparisonDialog::createComparisonTable() {
    // Create table comparing plans
}

void PlanComparisonDialog::createCostChart() {
    // Create cost comparison chart
}

void PlanComparisonDialog::createImprovementSummary() {
    // Create improvement summary
}

// ============================================================================
// IndexAdvisor
// ============================================================================

IndexAdvisor::IndexAdvisor(QObject* parent)
    : QObject(parent) {
}

QList<IndexAdvisor::IndexSuggestion> IndexAdvisor::analyzePlan(const QueryPlan& plan) {
    QList<IndexSuggestion> suggestions;
    
    for (const auto& node : plan.rootNodes) {
        detectMissingIndexesRecursive(node, suggestions);
    }
    
    return suggestions;
}

void IndexAdvisor::detectMissingIndexesRecursive(const QueryPlanNode& node, QList<IndexSuggestion>& suggestions) {
    auto nodeSuggestions = detectMissingIndexes(node);
    suggestions.append(nodeSuggestions);
    
    for (const auto& child : node.children) {
        detectMissingIndexesRecursive(child, suggestions);
    }
}

QList<IndexAdvisor::IndexSuggestion> IndexAdvisor::detectMissingIndexes(const QueryPlanNode& node) {
    QList<IndexSuggestion> suggestions;
    
    // Detect sequential scans on large tables
    if (node.isSeqScan && node.estimatedRows > 1000 && !node.tableName.isEmpty()) {
        IndexSuggestion suggestion;
        suggestion.tableName = node.tableName;
        suggestion.indexType = "B-tree";
        suggestion.estimatedImprovement = 50.0;  // 50% improvement estimate
        suggestion.reason = tr("Sequential scan on table with %1 rows")
                           .arg(node.estimatedRows);
        suggestion.suggestedSql = tr("CREATE INDEX idx_%1_%2 ON %1(%2);")
                                 .arg(node.tableName)
                                 .arg("column");  // Would extract from condition
        suggestions.append(suggestion);
    }
    
    return suggestions;
}

QList<IndexAdvisor::IndexSuggestion> IndexAdvisor::detectCompositeIndexOpportunities(const QueryPlan& plan) {
    Q_UNUSED(plan)
    return QList<IndexSuggestion>();
}

double IndexAdvisor::estimateImprovement(const QString& table, const QStringList& columns) {
    Q_UNUSED(table)
    Q_UNUSED(columns)
    return 0.0;
}

QList<IndexAdvisor::IndexSuggestion> IndexAdvisor::analyzeSlowQueries(const QStringList& queries) {
    Q_UNUSED(queries)
    return QList<IndexSuggestion>();
}

// ============================================================================
// QueryOptimizerHints
// ============================================================================

QStringList QueryOptimizerHints::getHints(const QueryPlan& plan) {
    QStringList hints;
    
    if (hasSequentialScans(plan)) {
        hints.append(tr("Consider adding indexes to avoid sequential scans"));
    }
    
    if (hasHighCostNodes(plan, 1000.0)) {
        hints.append(tr("High cost operations detected - consider query rewriting"));
    }
    
    if (hasSuboptimalJoins(plan)) {
        hints.append(tr("Review join order for better performance"));
    }
    
    return hints;
}

QStringList QueryOptimizerHints::getRewriteSuggestions(const QString& query, const QueryPlan& plan) {
    Q_UNUSED(query)
    QStringList suggestions;
    
    if (hasSequentialScans(plan)) {
        suggestions.append(tr("Add WHERE clause filters to enable index usage"));
    }
    
    return suggestions;
}

bool QueryOptimizerHints::hasSequentialScans(const QueryPlan& plan) {
    for (const auto& node : plan.rootNodes) {
        if (node.isSeqScan) return true;
    }
    return false;
}

bool QueryOptimizerHints::hasHighCostNodes(const QueryPlan& plan, double threshold) {
    return plan.totalCost > threshold;
}

bool QueryOptimizerHints::hasSuboptimalJoins(const QueryPlan& plan) {
    Q_UNUSED(plan)
    // Would analyze join order
    return false;
}

} // namespace scratchrobin::ui
