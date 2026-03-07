#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QTextEdit;
class QComboBox;
class QSplitter;
class QStandardItemModel;
class QStandardItem;
class QGraphicsView;
class QGraphicsScene;
class QTabWidget;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Query Plan Visualizer - Visual EXPLAIN output
 * 
 * Visualize database query execution plans:
 * - Tree/graph view of plan nodes
 * - Cost and timing information
 * - Index usage visualization
 * - Sequential scan warnings
 * - Plan comparison
 */

// ============================================================================
// Plan Node Types
// ============================================================================
enum class PlanNodeType {
    SeqScan,
    IndexScan,
    IndexOnlyScan,
    BitmapHeapScan,
    BitmapIndexScan,
    NestedLoop,
    MergeJoin,
    HashJoin,
    Hash,
    Sort,
    Aggregate,
    Group,
    Limit,
    SubqueryScan,
    FunctionScan,
    ValuesScan,
    CteScan,
    WorkTableScan,
    ForeignScan,
    CustomScan
};

struct PlanNode {
    int id = 0;
    PlanNodeType type;
    QString relationName;
    QString schema;
    QString alias;
    QString indexName;
    double startupCost = 0;
    double totalCost = 0;
    double actualStartupTime = 0;
    double actualTotalTime = 0;
    int planRows = 0;
    int actualRows = 0;
    int planWidth = 0;
    int actualLoops = 1;
    QString joinFilter;
    QString filter;
    QString indexCond;
    QString recheckCond;
    QString hashCond;
    QString mergeCond;
    QString sortKey;
    QString sortMethod;
    QString sortSpaceUsed;
    QString groupKey;
    QString strategy;
    bool partialMode = false;
    QString parallelWorkers;
    QString output;
    
    QList<PlanNode> children;
    PlanNode* parent = nullptr;
};

struct VisualQueryPlan {
    QString query;
    QString planType; // EXPLAIN, EXPLAIN ANALYZE, EXPLAIN VERBOSE
    PlanNode rootNode;
    double planningTime = 0;
    double executionTime = 0;
    int triggers = 0;
    
    void clear();
    QString toText() const;
};

// ============================================================================
// Query Plan Visualizer Panel
// ============================================================================
class QueryPlanVisualizerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit QueryPlanVisualizerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Query Plan"); }
    QString panelCategory() const override { return "query"; }

public slots:
    void onExplainQuery(const QString& query);
    void onExplainAnalyzeQuery(const QString& query);
    void onRefreshPlan();
    void onSavePlan();
    void onLoadPlan();
    void onExportPlan();
    void onComparePlans();
    
    // View modes
    void onViewTree();
    void onViewGraph();
    void onViewText();
    void onViewFlame();
    
    // Node details
    void onNodeSelected(const QModelIndex& index);
    void onHighlightSlowNodes();
    void onHighlightSeqScans();

signals:
    void planLoaded(const VisualQueryPlan& plan);
    void optimizationSuggestions(const QStringList& suggestions);

private:
    void setupUi();
    void setupTreeView();
    void setupGraphView();
    void setupTextView();
    void setupStatsPanel();
    void executeExplain(const QString& query, bool analyze);
    void parseExplainOutput(const QString& output);
    void populateTreeModel(const PlanNode& node, QStandardItem* parent);
    void analyzePlan();
    QStringList generateSuggestions();
    
    backend::SessionClient* client_;
    VisualQueryPlan currentPlan_;
    
    // Views
    QTabWidget* viewTabs_ = nullptr;
    QTreeView* treeView_ = nullptr;
    QStandardItemModel* treeModel_ = nullptr;
    QGraphicsView* graphView_ = nullptr;
    QGraphicsScene* graphScene_ = nullptr;
    QTextEdit* textView_ = nullptr;
    
    // Stats panel
    QLabel* totalTimeLabel_ = nullptr;
    QLabel* planningTimeLabel_ = nullptr;
    QLabel* executionTimeLabel_ = nullptr;
    QLabel* rowsLabel_ = nullptr;
    QLabel* costLabel_ = nullptr;
    QTableView* nodeStatsTable_ = nullptr;
    QStandardItemModel* nodeStatsModel_ = nullptr;
    
    // Current query
    QTextEdit* queryEdit_ = nullptr;
};

// ============================================================================
// Plan Comparison Dialog
// ============================================================================
class VisualPlanComparisonDialog : public QDialog {
    Q_OBJECT

public:
    explicit VisualPlanComparisonDialog(const VisualQueryPlan& plan1, 
                                 const VisualQueryPlan& plan2,
                                 QWidget* parent = nullptr);

public slots:
    void onSwitchViews();
    void onExportComparison();

private:
    void setupUi();
    void comparePlans();
    
    VisualQueryPlan plan1_;
    VisualQueryPlan plan2_;
    
    QTreeView* tree1_ = nullptr;
    QTreeView* tree2_ = nullptr;
    QTextEdit* diffEdit_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
};

// ============================================================================
// Plan Node Details Dialog
// ============================================================================
class PlanNodeDetailsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlanNodeDetailsDialog(const PlanNode& node, QWidget* parent = nullptr);

private:
    void setupUi();
    
    PlanNode node_;
    
    QTextEdit* detailsEdit_ = nullptr;
    QTableView* statsTable_ = nullptr;
    QStandardItemModel* statsModel_ = nullptr;
};

} // namespace scratchrobin::ui
