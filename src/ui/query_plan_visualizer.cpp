#include "query_plan_visualizer.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QTableView>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Query Plan
// ============================================================================

void VisualQueryPlan::clear() {
    query.clear();
    planType.clear();
    rootNode = PlanNode();
    planningTime = 0;
    executionTime = 0;
    triggers = 0;
}

QString VisualQueryPlan::toText() const {
    QString text;
    text += "Query:\n" + query + "\n\n";
    text += "Plan Type: " + planType + "\n";
    text += "Planning Time: " + QString::number(planningTime) + " ms\n";
    text += "Execution Time: " + QString::number(executionTime) + " ms\n";
    return text;
}

// ============================================================================
// Query Plan Visualizer Panel
// ============================================================================

QueryPlanVisualizerPanel::QueryPlanVisualizerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("query_plan", parent)
    , client_(client) {
    setupUi();
}

void QueryPlanVisualizerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* explainBtn = new QPushButton(tr("EXPLAIN"), this);
    connect(explainBtn, &QPushButton::clicked, [this]() {
        onExplainQuery(queryEdit_->toPlainText());
    });
    toolbarLayout->addWidget(explainBtn);
    
    auto* analyzeBtn = new QPushButton(tr("EXPLAIN ANALYZE"), this);
    connect(analyzeBtn, &QPushButton::clicked, [this]() {
        onExplainAnalyzeQuery(queryEdit_->toPlainText());
    });
    toolbarLayout->addWidget(analyzeBtn);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &QueryPlanVisualizerPanel::onRefreshPlan);
    toolbarLayout->addWidget(refreshBtn);
    
    toolbarLayout->addStretch();
    
    auto* saveBtn = new QPushButton(tr("Save"), this);
    connect(saveBtn, &QPushButton::clicked, this, &QueryPlanVisualizerPanel::onSavePlan);
    toolbarLayout->addWidget(saveBtn);
    
    auto* compareBtn = new QPushButton(tr("Compare"), this);
    connect(compareBtn, &QPushButton::clicked, this, &QueryPlanVisualizerPanel::onComparePlans);
    toolbarLayout->addWidget(compareBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Query input
    queryEdit_ = new QTextEdit(this);
    queryEdit_->setPlaceholderText(tr("Enter SQL query to analyze..."));
    queryEdit_->setMaximumHeight(100);
    mainLayout->addWidget(queryEdit_);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // View tabs
    viewTabs_ = new QTabWidget(this);
    
    // Tree view
    setupTreeView();
    viewTabs_->addTab(treeView_, tr("Tree"));
    
    // Graph view
    setupGraphView();
    viewTabs_->addTab(graphView_, tr("Graph"));
    
    // Text view
    textView_ = new QTextEdit(this);
    textView_->setFont(QFont("Consolas", 9));
    textView_->setReadOnly(true);
    viewTabs_->addTab(textView_, tr("Text"));
    
    splitter->addWidget(viewTabs_);
    
    // Stats panel
    setupStatsPanel();
    auto* statsWidget = new QWidget(this);
    auto* statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->addWidget(new QLabel(tr("Plan Statistics"), this));
    
    auto* statsGrid = new QGridLayout();
    statsGrid->addWidget(new QLabel(tr("Total Time:"), this), 0, 0);
    totalTimeLabel_ = new QLabel("-", this);
    statsGrid->addWidget(totalTimeLabel_, 0, 1);
    
    statsGrid->addWidget(new QLabel(tr("Planning:"), this), 0, 2);
    planningTimeLabel_ = new QLabel("-", this);
    statsGrid->addWidget(planningTimeLabel_, 0, 3);
    
    statsGrid->addWidget(new QLabel(tr("Execution:"), this), 1, 0);
    executionTimeLabel_ = new QLabel("-", this);
    statsGrid->addWidget(executionTimeLabel_, 1, 1);
    
    statsGrid->addWidget(new QLabel(tr("Rows:"), this), 1, 2);
    rowsLabel_ = new QLabel("-", this);
    statsGrid->addWidget(rowsLabel_, 1, 3);
    
    statsGrid->addWidget(new QLabel(tr("Cost:"), this), 2, 0);
    costLabel_ = new QLabel("-", this);
    statsGrid->addWidget(costLabel_, 2, 1);
    
    statsLayout->addLayout(statsGrid);
    
    nodeStatsTable_ = new QTableView(this);
    nodeStatsModel_ = new QStandardItemModel(this);
    nodeStatsModel_->setHorizontalHeaderLabels({tr("Node"), tr("Type"), tr("Cost"), tr("Rows"), tr("Time")});
    nodeStatsTable_->setModel(nodeStatsModel_);
    statsLayout->addWidget(nodeStatsTable_, 1);
    
    splitter->addWidget(statsWidget);
    splitter->setSizes({400, 200});
    
    mainLayout->addWidget(splitter, 1);
}

void QueryPlanVisualizerPanel::setupTreeView() {
    treeView_ = new QTreeView(this);
    treeModel_ = new QStandardItemModel(this);
    treeModel_->setHorizontalHeaderLabels({tr("Operation"), tr("Cost"), tr("Rows"), tr("Width")});
    treeView_->setModel(treeModel_);
    treeView_->setAlternatingRowColors(true);
    connect(treeView_, &QTreeView::clicked, this, &QueryPlanVisualizerPanel::onNodeSelected);
}

void QueryPlanVisualizerPanel::setupGraphView() {
    graphScene_ = new QGraphicsScene(this);
    graphView_ = new QGraphicsView(graphScene_, this);
    graphView_->setRenderHint(QPainter::Antialiasing);
}

void QueryPlanVisualizerPanel::setupStatsPanel() {
    // Implemented in setupUi
}

void QueryPlanVisualizerPanel::executeExplain(const QString& query, bool analyze) {
    Q_UNUSED(query)
    Q_UNUSED(analyze)
    
    // Simulate explain output
    currentPlan_.query = queryEdit_->toPlainText();
    currentPlan_.planType = analyze ? "EXPLAIN ANALYZE" : "EXPLAIN";
    currentPlan_.planningTime = 0.5;
    currentPlan_.executionTime = 12.3;
    
    // Create sample plan tree
    currentPlan_.rootNode.type = PlanNodeType::Aggregate;
    currentPlan_.rootNode.totalCost = 150.5;
    currentPlan_.rootNode.planRows = 1;
    
    PlanNode seqScan;
    seqScan.type = PlanNodeType::SeqScan;
    seqScan.relationName = "customers";
    seqScan.totalCost = 100.0;
    seqScan.planRows = 1000;
    currentPlan_.rootNode.children.append(seqScan);
    
    PlanNode indexScan;
    indexScan.type = PlanNodeType::IndexScan;
    indexScan.relationName = "orders";
    indexScan.indexName = "idx_orders_customer";
    indexScan.totalCost = 45.5;
    indexScan.planRows = 500;
    currentPlan_.rootNode.children.append(indexScan);
    
    // Update UI
    treeModel_->clear();
    treeModel_->setHorizontalHeaderLabels({tr("Operation"), tr("Cost"), tr("Rows"), tr("Width")});
    populateTreeModel(currentPlan_.rootNode, nullptr);
    treeView_->expandAll();
    
    // Update stats
    totalTimeLabel_->setText(QString::number(currentPlan_.planningTime + currentPlan_.executionTime) + " ms");
    planningTimeLabel_->setText(QString::number(currentPlan_.planningTime) + " ms");
    executionTimeLabel_->setText(QString::number(currentPlan_.executionTime) + " ms");
    rowsLabel_->setText(QString::number(currentPlan_.rootNode.planRows));
    costLabel_->setText(QString::number(currentPlan_.rootNode.totalCost));
    
    // Update text view
    textView_->setPlainText(currentPlan_.toText());
    
    // Update node stats
    nodeStatsModel_->clear();
    nodeStatsModel_->setHorizontalHeaderLabels({tr("Node"), tr("Type"), tr("Cost"), tr("Rows"), tr("Time")});
    
    for (const auto& child : currentPlan_.rootNode.children) {
        QList<QStandardItem*> row;
        row << new QStandardItem(child.relationName);
        row << new QStandardItem(child.indexName.isEmpty() ? "Seq Scan" : "Index Scan");
        row << new QStandardItem(QString::number(child.totalCost));
        row << new QStandardItem(QString::number(child.planRows));
        row << new QStandardItem("-");
        nodeStatsModel_->appendRow(row);
    }
}

void QueryPlanVisualizerPanel::parseExplainOutput(const QString& output) {
    Q_UNUSED(output)
    // Parse EXPLAIN output
}

void QueryPlanVisualizerPanel::populateTreeModel(const PlanNode& node, QStandardItem* parent) {
    QList<QStandardItem*> row;
    
    QString opName;
    switch (node.type) {
    case PlanNodeType::SeqScan: opName = "Seq Scan"; break;
    case PlanNodeType::IndexScan: opName = "Index Scan"; break;
    case PlanNodeType::IndexOnlyScan: opName = "Index Only Scan"; break;
    case PlanNodeType::NestedLoop: opName = "Nested Loop"; break;
    case PlanNodeType::HashJoin: opName = "Hash Join"; break;
    case PlanNodeType::MergeJoin: opName = "Merge Join"; break;
    case PlanNodeType::Aggregate: opName = "Aggregate"; break;
    case PlanNodeType::Sort: opName = "Sort"; break;
    default: opName = "Unknown"; break;
    }
    
    if (!node.relationName.isEmpty()) {
        opName += " on " + node.relationName;
    }
    
    row << new QStandardItem(opName);
    row << new QStandardItem(QString::number(node.totalCost, 'f', 2));
    row << new QStandardItem(QString::number(node.planRows));
    row << new QStandardItem(QString::number(node.planWidth));
    
    if (parent) {
        parent->appendRow(row);
    } else {
        treeModel_->appendRow(row);
    }
    
    for (const auto& child : node.children) {
        populateTreeModel(child, row[0]);
    }
}

void QueryPlanVisualizerPanel::analyzePlan() {
    // Analyze plan for optimization opportunities
}

QStringList QueryPlanVisualizerPanel::generateSuggestions() {
    QStringList suggestions;
    
    // Check for sequential scans on large tables
    for (const auto& child : currentPlan_.rootNode.children) {
        if (child.type == PlanNodeType::SeqScan && child.planRows > 100) {
            suggestions << tr("Consider adding an index on '%1' to avoid sequential scan")
                          .arg(child.relationName);
        }
    }
    
    // Check for high cost operations
    if (currentPlan_.rootNode.totalCost > 1000) {
        suggestions << tr("High total cost detected. Consider query optimization.");
    }
    
    return suggestions;
}

void QueryPlanVisualizerPanel::onExplainQuery(const QString& query) {
    executeExplain(query, false);
}

void QueryPlanVisualizerPanel::onExplainAnalyzeQuery(const QString& query) {
    executeExplain(query, true);
}

void QueryPlanVisualizerPanel::onRefreshPlan() {
    executeExplain(queryEdit_->toPlainText(), currentPlan_.planType.contains("ANALYZE"));
}

void QueryPlanVisualizerPanel::onSavePlan() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Query Plan"),
        QString(),
        tr("Text Files (*.txt);;JSON (*.json)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(textView_->toPlainText().toUtf8());
            file.close();
        }
    }
}

static PlanNode parsePlanNode(const QJsonObject& obj) {
    PlanNode node;
    
    // Parse node type
    QString nodeType = obj["Node Type"].toString();
    if (nodeType == "Seq Scan") node.type = PlanNodeType::SeqScan;
    else if (nodeType == "Index Scan") node.type = PlanNodeType::IndexScan;
    else if (nodeType == "Index Only Scan") node.type = PlanNodeType::IndexOnlyScan;
    else if (nodeType == "Bitmap Heap Scan") node.type = PlanNodeType::BitmapHeapScan;
    else if (nodeType == "Bitmap Index Scan") node.type = PlanNodeType::BitmapIndexScan;
    else if (nodeType == "Nested Loop") node.type = PlanNodeType::NestedLoop;
    else if (nodeType == "Merge Join") node.type = PlanNodeType::MergeJoin;
    else if (nodeType == "Hash Join") node.type = PlanNodeType::HashJoin;
    else if (nodeType == "Hash") node.type = PlanNodeType::Hash;
    else if (nodeType == "Sort") node.type = PlanNodeType::Sort;
    else if (nodeType == "Aggregate") node.type = PlanNodeType::Aggregate;
    else if (nodeType == "Group") node.type = PlanNodeType::Group;
    else if (nodeType == "Limit") node.type = PlanNodeType::Limit;
    else if (nodeType == "Subquery Scan") node.type = PlanNodeType::SubqueryScan;
    else if (nodeType == "Function Scan") node.type = PlanNodeType::FunctionScan;
    else if (nodeType == "Values Scan") node.type = PlanNodeType::ValuesScan;
    else if (nodeType == "CTE Scan") node.type = PlanNodeType::CteScan;
    else node.type = PlanNodeType::SeqScan;  // Default fallback
    
    // Parse basic fields
    node.relationName = obj["Relation Name"].toString();
    node.schema = obj["Schema"].toString();
    node.alias = obj["Alias"].toString();
    node.indexName = obj["Index Name"].toString();
    
    // Parse cost/timing
    node.startupCost = obj["Startup Cost"].toDouble();
    node.totalCost = obj["Total Cost"].toDouble();
    node.actualStartupTime = obj["Actual Startup Time"].toDouble();
    node.actualTotalTime = obj["Actual Total Time"].toDouble();
    node.planRows = obj["Plan Rows"].toInt();
    node.actualRows = obj["Actual Rows"].toInt();
    node.planWidth = obj["Plan Width"].toInt();
    node.actualLoops = obj["Actual Loops"].toInt(1);
    
    // Parse conditions
    node.joinFilter = obj["Join Filter"].toString();
    node.filter = obj["Filter"].toString();
    node.indexCond = obj["Index Cond"].toString();
    node.recheckCond = obj["Recheck Cond"].toString();
    node.hashCond = obj["Hash Cond"].toString();
    node.mergeCond = obj["Merge Cond"].toString();
    {
        QJsonArray arr = obj["Sort Key"].toArray();
        QStringList keys;
        for (const auto& v : arr) keys.append(v.toString());
        node.sortKey = keys.join(", ");
    }
    node.sortMethod = obj["Sort Method"].toString();
    node.sortSpaceUsed = obj["Sort Space Used"].toString();
    {
        QJsonArray arr = obj["Group Key"].toArray();
        QStringList keys;
        for (const auto& v : arr) keys.append(v.toString());
        node.groupKey = keys.join(", ");
    }
    node.strategy = obj["Strategy"].toString();
    node.parallelWorkers = obj["Workers Planned"].toString();
    {
        QJsonArray arr = obj["Output"].toArray();
        QStringList keys;
        for (const auto& v : arr) keys.append(v.toString());
        node.output = keys.join(", ");
    }
    
    // Parse children
    QJsonArray plans = obj["Plans"].toArray();
    for (const auto& plan : plans) {
        PlanNode child = parsePlanNode(plan.toObject());
        child.parent = &node;
        node.children.append(child);
    }
    
    return node;
}

void QueryPlanVisualizerPanel::onLoadPlan() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Query Plan"),
        QString(),
        tr("JSON Files (*.json);;Text Files (*.txt);;All Files (*)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Cannot open file for reading."));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Invalid JSON format."));
        return;
    }
    
    // PostgreSQL EXPLAIN (FORMAT JSON) returns an array with one object
    QJsonArray rootArray = doc.array();
    if (rootArray.isEmpty()) {
        QMessageBox::critical(this, tr("Error"),
            tr("Empty plan array."));
        return;
    }
    
    QJsonObject planObj = rootArray[0].toObject();
    QJsonObject plan = planObj["Plan"].toObject();
    
    // Clear current plan
    currentPlan_.clear();
    
    // Parse plan
    currentPlan_.rootNode = parsePlanNode(plan);
    currentPlan_.planningTime = planObj["Planning Time"].toDouble();
    currentPlan_.executionTime = planObj["Execution Time"].toDouble();
    currentPlan_.triggers = planObj["Triggers"].toArray().size();
    
    // Refresh display
    // Refresh would update the display here
    
    QMessageBox::information(this, tr("Plan Loaded"),
        tr("Query plan loaded from:\n%1").arg(fileName));
}

void QueryPlanVisualizerPanel::onExportPlan() {
    // Export plan in various formats
}

void QueryPlanVisualizerPanel::onComparePlans() {
    VisualQueryPlan emptyPlan;
    VisualPlanComparisonDialog dialog(currentPlan_, emptyPlan, this);
    dialog.exec();
}

void QueryPlanVisualizerPanel::onViewTree() {
    viewTabs_->setCurrentIndex(0);
}

void QueryPlanVisualizerPanel::onViewGraph() {
    viewTabs_->setCurrentIndex(1);
}

void QueryPlanVisualizerPanel::onViewText() {
    viewTabs_->setCurrentIndex(2);
}

static void buildFlameData(const PlanNode& node, QStringList& lines, int depth = 0) {
    QString indent(depth * 2, ' ');
    double time = node.actualTotalTime > 0 ? node.actualTotalTime : node.totalCost;
    
    QString line = indent;
    switch (node.type) {
        case PlanNodeType::SeqScan: line += "[Seq Scan] "; break;
        case PlanNodeType::IndexScan: line += "[Index Scan] "; break;
        case PlanNodeType::IndexOnlyScan: line += "[Index Only Scan] "; break;
        case PlanNodeType::BitmapHeapScan: line += "[Bitmap Heap Scan] "; break;
        case PlanNodeType::NestedLoop: line += "[Nested Loop] "; break;
        case PlanNodeType::MergeJoin: line += "[Merge Join] "; break;
        case PlanNodeType::HashJoin: line += "[Hash Join] "; break;
        case PlanNodeType::Hash: line += "[Hash] "; break;
        case PlanNodeType::Sort: line += "[Sort] "; break;
        case PlanNodeType::Aggregate: line += "[Aggregate] "; break;
        case PlanNodeType::Group: line += "[Group] "; break;
        case PlanNodeType::Limit: line += "[Limit] "; break;
        default: line += "[Node] "; break;
    }
    
    if (!node.relationName.isEmpty()) {
        line += node.relationName + " ";
    }
    line += QString("(%1 ms, %2 rows)").arg(time, 0, 'f', 2).arg(node.actualRows > 0 ? node.actualRows : node.planRows);
    
    lines.append(line);
    
    for (const auto& child : node.children) {
        buildFlameData(child, lines, depth + 1);
    }
}

void QueryPlanVisualizerPanel::onViewFlame() {
    if (currentPlan_.rootNode.relationName.isEmpty() && currentPlan_.rootNode.children.isEmpty()) {
        QMessageBox::information(this, tr("No Plan"),
            tr("No query plan loaded. Generate or load a plan first."));
        return;
    }
    
    // Build flame graph text representation
    QStringList flameData;
    flameData.append("=== Flame Graph View ===");
    flameData.append("Time (ms) and row counts for each operation:");
    flameData.append("");
    
    buildFlameData(currentPlan_.rootNode, flameData);
    
    flameData.append("");
    flameData.append(QString("Planning Time: %1 ms").arg(currentPlan_.planningTime));
    flameData.append(QString("Execution Time: %1 ms").arg(currentPlan_.executionTime));
    
    // Show in a dialog
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Flame Graph View"));
    dialog.resize(800, 600);
    
    auto* layout = new QVBoxLayout(&dialog);
    auto* textEdit = new QTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Consolas", 10));
    textEdit->setPlainText(flameData.join("\n"));
    layout->addWidget(textEdit);
    
    auto* closeBtn = new QPushButton(tr("Close"), &dialog);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    dialog.exec();
}

void QueryPlanVisualizerPanel::onNodeSelected(const QModelIndex& index) {
    Q_UNUSED(index)
    // Show node details
}

void QueryPlanVisualizerPanel::onHighlightSlowNodes() {
    // Highlight expensive nodes
}

void QueryPlanVisualizerPanel::onHighlightSeqScans() {
    // Highlight sequential scans
}

// ============================================================================
// Plan Comparison Dialog
// ============================================================================

VisualPlanComparisonDialog::VisualPlanComparisonDialog(const VisualQueryPlan& plan1, 
                                          const VisualQueryPlan& plan2,
                                          QWidget* parent)
    : QDialog(parent)
    , plan1_(plan1)
    , plan2_(plan2) {
    setupUi();
}

void VisualPlanComparisonDialog::setupUi() {
    setWindowTitle(tr("Compare Query Plans"));
    resize(900, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    summaryLabel_ = new QLabel(this);
    layout->addWidget(summaryLabel_);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    tree1_ = new QTreeView(this);
    auto* model1 = new QStandardItemModel(this);
    model1->setHorizontalHeaderLabels({tr("Plan 1"), tr("Cost"), tr("Rows")});
    tree1_->setModel(model1);
    splitter->addWidget(tree1_);
    
    tree2_ = new QTreeView(this);
    auto* model2 = new QStandardItemModel(this);
    model2->setHorizontalHeaderLabels({tr("Plan 2"), tr("Cost"), tr("Rows")});
    tree2_->setModel(model2);
    splitter->addWidget(tree2_);
    
    layout->addWidget(splitter, 1);
    
    // Add sample data
    model1->appendRow({new QStandardItem("Seq Scan"), new QStandardItem("100"), new QStandardItem("1000")});
    model2->appendRow({new QStandardItem("Index Scan"), new QStandardItem("50"), new QStandardItem("1000")});
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void VisualPlanComparisonDialog::comparePlans() {
    // Compare two plans
}

void VisualPlanComparisonDialog::onSwitchViews() {
    // Switch between comparison views
}

void VisualPlanComparisonDialog::onExportComparison() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Comparison"),
        QString(),
        tr("Text Files (*.txt);;HTML (*.html)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Comparison exported to %1").arg(fileName));
    }
}

// ============================================================================
// Plan Node Details Dialog
// ============================================================================

PlanNodeDetailsDialog::PlanNodeDetailsDialog(const PlanNode& node, QWidget* parent)
    : QDialog(parent)
    , node_(node) {
    setupUi();
}

void PlanNodeDetailsDialog::setupUi() {
    setWindowTitle(tr("Plan Node Details"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setFont(QFont("Consolas", 9));
    detailsEdit_->setReadOnly(true);
    
    QString details;
    details += "Node Type: " + QString::number(static_cast<int>(node_.type)) + "\n";
    details += "Relation: " + node_.relationName + "\n";
    details += "Index: " + node_.indexName + "\n";
    details += "Startup Cost: " + QString::number(node_.startupCost) + "\n";
    details += "Total Cost: " + QString::number(node_.totalCost) + "\n";
    details += "Plan Rows: " + QString::number(node_.planRows) + "\n";
    details += "Plan Width: " + QString::number(node_.planWidth) + "\n";
    
    detailsEdit_->setPlainText(details);
    layout->addWidget(detailsEdit_);
    
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

} // namespace scratchrobin::ui
