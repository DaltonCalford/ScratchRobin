#pragma once
#include <QDialog>
#include <QTreeWidget>
#include <memory>
#include <QHash>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QTextEdit;
class QTableWidget;
class QTabWidget;
class QSplitter;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Query Execution Plan Viewer
 * 
 * Implements FORM_SPECIFICATION.md Explain Plan section:
 * - Query plan capture
 * - Plan visualization (tree view)
 * - Cost analysis
 * - Index usage analysis
 */

// ============================================================================
// Query Plan Node
// ============================================================================
struct QueryPlanNode {
    QString operation;       // Seq Scan, Index Scan, Hash Join, etc.
    QString tableName;
    QString indexName;
    QString condition;
    double cost = 0.0;       // Total cost
    double startupCost = 0.0;
    double actualTime = 0.0;
    long long actualRows = 0;
    long long estimatedRows = 0;
    int width = 0;           // Row width in bytes
    QString output;
    QString filter;
    QString hashCond;
    QString joinCond;
    QString sortKey;
    QString indexCond;
    QList<QueryPlanNode> children;
    
    // Analysis flags
    bool isExpensive = false;
    bool isSeqScan = false;
    bool isIndexScan = false;
    bool usesIndex = false;
};

// ============================================================================
// Query Plan
// ============================================================================
struct QueryPlan {
    QString query;
    double totalCost = 0.0;
    double startupCost = 0.0;
    long long totalRows = 0;
    int planWidth = 0;
    QList<QueryPlanNode> rootNodes;
    QString planningTime;
    QString executionTime;
    bool isAnalyzed = false;
    
    // Warnings
    QStringList warnings;
    QStringList suggestions;
};

// ============================================================================
// Explain Plan Viewer Dialog
// ============================================================================
class ExplainPlanDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExplainPlanDialog(backend::SessionClient* client, QWidget* parent = nullptr);

    void analyzeQuery(const QString& sql);

public slots:
    void refresh();
    void exportPlan();

private slots:
    void onAnalyzeClicked();
    void onFormatChanged(int index);
    void onNodeSelected();
    void onToggleCosts(bool show);
    void onComparePlans();

private:
    void setupUi();
    void displayPlan(const QueryPlan& plan);
    void populateTree(QTreeWidgetItem* parent, const QueryPlanNode& node);
    void analyzePlanIssues(const QueryPlan& plan);
    void updateStatistics(const QueryPlan& plan);
    
    QueryPlan parseExplainOutput(const QString& output);
    QueryPlan parseJsonExplain(const QString& json);
    QueryPlan parseTextExplain(const QString& text);

    backend::SessionClient* client_ = nullptr;
    QueryPlan currentPlan_;
    QList<QueryPlan> planHistory_;
    
    // UI
    QComboBox* formatCombo_ = nullptr;
    QComboBox* analyzeCombo_ = nullptr;
    QPushButton* analyzeBtn_ = nullptr;
    QTextEdit* queryEdit_ = nullptr;
    QTreeWidget* planTree_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
    QTableWidget* statsTable_ = nullptr;
    QTextEdit* warningsEdit_ = nullptr;
    QTabWidget* tabWidget_ = nullptr;
};

// ============================================================================
// Plan Tree Widget (custom tree with cost bars)
// ============================================================================
class PlanTreeWidget : public QTreeWidget {
    Q_OBJECT

public:
    explicit PlanTreeWidget(QWidget* parent = nullptr);

    void setShowCosts(bool show);
    void setHighlightExpensive(bool highlight);

protected:
    void drawRow(QPainter* painter, const QStyleOptionViewItem& options,
                 const QModelIndex& index) const override;

private:
    bool showCosts_ = true;
    bool highlightExpensive_ = true;
    double maxCost_ = 0.0;
};

// ============================================================================
// Plan Comparison Dialog
// ============================================================================
class PlanComparisonDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlanComparisonDialog(const QList<QueryPlan>& plans, QWidget* parent = nullptr);

private:
    void setupUi();
    void createComparisonTable();
    void createCostChart();
    void createImprovementSummary();

    QList<QueryPlan> plans_;
    QTabWidget* tabWidget_ = nullptr;
};

// ============================================================================
// Index Advisor (suggests missing indexes)
// ============================================================================
class IndexAdvisor : public QObject {
    Q_OBJECT

public:
    explicit IndexAdvisor(QObject* parent = nullptr);

    struct IndexSuggestion {
        QString tableName;
        QStringList columns;
        QString indexType;  // B-tree, Hash, GIN, etc.
        double estimatedImprovement;
        QString reason;
        QString suggestedSql;
    };

    QList<IndexSuggestion> analyzePlan(const QueryPlan& plan);
    QList<IndexSuggestion> analyzeSlowQueries(const QStringList& queries);

private:
    QList<IndexSuggestion> detectMissingIndexes(const QueryPlanNode& node);
    void detectMissingIndexesRecursive(const QueryPlanNode& node, QList<IndexSuggestion>& suggestions);
    QList<IndexSuggestion> detectCompositeIndexOpportunities(const QueryPlan& plan);
    double estimateImprovement(const QString& table, const QStringList& columns);
};

// ============================================================================
// Query Optimizer Hints
// ============================================================================
class QueryOptimizerHints : public QObject {
    Q_OBJECT

public:
    static QStringList getHints(const QueryPlan& plan);
    static QStringList getRewriteSuggestions(const QString& query, const QueryPlan& plan);

private:
    static bool hasSequentialScans(const QueryPlan& plan);
    static bool hasHighCostNodes(const QueryPlan& plan, double threshold);
    static bool hasSuboptimalJoins(const QueryPlan& plan);
};

} // namespace scratchrobin::ui
