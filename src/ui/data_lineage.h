#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>

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
class QTreeView;
class QProgressBar;
class QListWidget;
class QGraphicsItem;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

// Forward declarations
class LineageGraphicsView;

/**
 * @brief Data Lineage - Impact analysis and data flow visualization
 * 
 * Track data flow and dependencies:
 * - Visual data flow diagrams
 * - Column-level lineage tracking
 * - Impact analysis (what depends on this?)
 * - Root cause analysis (where does this come from?)
 * - Cross-database lineage
 * - ETL pipeline visualization
 */

// ============================================================================
// Lineage Types
// ============================================================================
enum class LineageDirection {
    Upstream,   // Sources (what feeds into this)
    Downstream, // Dependencies (what uses this)
    Both
};

enum class LineageLevel {
    Database,
    Schema,
    Table,
    Column
};

enum class TransformationType {
    Select,
    Join,
    Filter,
    Aggregate,
    Transform,
    Union,
    Insert,
    Update,
    Delete,
    Create,
    Drop
};

struct LineageNode {
    QString id;
    QString name;
    QString type; // database, schema, table, column, view, function
    QString fullPath;
    QString database;
    QString schema;
    QString table;
    QString column;
    int x = 0;
    int y = 0;
    QColor color;
    bool isSelected = false;
    bool isHighlighted = false;
    QString tooltip;
};

struct LineageEdge {
    QString id;
    QString sourceId;
    QString targetId;
    TransformationType transformation;
    QString transformationName;
    QString sql;
    int impactWeight = 1; // 1-10
    bool isActive = true;
};

struct LineageGraph {
    QString rootId;
    LineageDirection direction;
    int depth;
    QHash<QString, LineageNode> nodes;
    QList<LineageEdge> edges;
    QDateTime generatedAt;
};

struct ImpactAnalysis {
    QString targetObject;
    int totalDependencies = 0;
    int directDependencies = 0;
    int indirectDependencies = 0;
    QList<QString> affectedReports;
    QList<QString> affectedDashboards;
    QList<QString> affectedETLs;
    QStringList riskAssessment;
};

// ============================================================================
// Data Lineage Panel
// ============================================================================
class DataLineagePanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataLineagePanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Lineage"); }
    QString panelCategory() const override { return "analysis"; }

public slots:
    // Selection
    void onSelectObject();
    void onObjectSelected(const QModelIndex& index);
    void onSearchObject(const QString& text);
    
    // Visualization
    void onShowUpstream();
    void onShowDownstream();
    void onShowBoth();
    void onChangeDepth(int depth);
    void onRefreshLineage();
    
    // Analysis
    void onImpactAnalysis();
    void onRootCauseAnalysis();
    void onCompareVersions();
    void onExportDiagram();
    void onPrintDiagram();
    
    // Interactive
    void onNodeClicked(const QString& nodeId);
    void onNodeDoubleClicked(const QString& nodeId);
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onLayoutChanged(int index);

signals:
    void lineageGraphGenerated(const LineageGraph& graph);
    void impactAnalysisComplete(const ImpactAnalysis& analysis);
    void objectSelected(const QString& objectPath);

private:
    void setupUi();
    void setupObjectTree();
    void setupVisualization();
    void buildLineageGraph();
    void renderGraph();
    void clearGraph();
    
    backend::SessionClient* client_;
    LineageGraph currentGraph_;
    
    // UI
    QSplitter* splitter_ = nullptr;
    QTreeView* objectTree_ = nullptr;
    QStandardItemModel* objectModel_ = nullptr;
    
    // Graphics view
    LineageGraphicsView* graphicsView_ = nullptr;
    QGraphicsScene* graphicsScene_ = nullptr;
    
    // Controls
    QComboBox* depthCombo_ = nullptr;
    QComboBox* layoutCombo_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    
    // Zoom
    double zoomLevel_ = 1.0;
    
    // Selected object
    QString selectedObjectPath_;
};

// ============================================================================
// Lineage Graphics View
// ============================================================================
class LineageGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit LineageGraphicsView(QWidget* parent = nullptr);

signals:
    void nodeClicked(const QString& nodeId);
    void nodeDoubleClicked(const QString& nodeId);
    void edgeClicked(const QString& edgeId);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
};

// ============================================================================
// Lineage Node Item
// ============================================================================
class LineageNodeItem : public QGraphicsRectItem {
public:
    explicit LineageNodeItem(const LineageNode& node, QGraphicsItem* parent = nullptr);
    
    void setHighlighted(bool highlighted);
    void setSelected(bool selected);
    QString nodeId() const { return node_.id; }
    
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    LineageNode node_;
    bool highlighted_ = false;
    QGraphicsTextItem* textItem_ = nullptr;
    QGraphicsTextItem* iconItem_ = nullptr;
};

// ============================================================================
// Lineage Edge Item
// ============================================================================
class LineageEdgeItem : public QGraphicsLineItem {
public:
    explicit LineageEdgeItem(const LineageEdge& edge, QGraphicsItem* parent = nullptr);
    
    void updatePosition(const QPointF& start, const QPointF& end);
    QString edgeId() const { return edge_.id; }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    LineageEdge edge_;
    QPolygonF arrowHead_;
};

// ============================================================================
// Impact Analysis Dialog
// ============================================================================
class ImpactAnalysisDialog : public QDialog {
    Q_OBJECT

public:
    explicit ImpactAnalysisDialog(const QString& objectPath, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExportReport();
    void onViewAffectedObjects();
    void onSimulateChange();

private:
    void setupUi();
    void performAnalysis();
    void calculateImpact();
    
    QString objectPath_;
    backend::SessionClient* client_;
    ImpactAnalysis analysis_;
    
    QLabel* targetLabel_ = nullptr;
    QLabel* totalDepsLabel_ = nullptr;
    QLabel* directDepsLabel_ = nullptr;
    QLabel* indirectDepsLabel_ = nullptr;
    
    QListWidget* reportsList_ = nullptr;
    QListWidget* dashboardsList_ = nullptr;
    QListWidget* etlsList_ = nullptr;
    QTextEdit* riskEdit_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
};

// ============================================================================
// Root Cause Analysis Dialog
// ============================================================================
class RootCauseAnalysisDialog : public QDialog {
    Q_OBJECT

public:
    explicit RootCauseAnalysisDialog(const QString& objectPath, backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onTraceBack();
    void onViewSource();
    void onExportPath();

private:
    void setupUi();
    void performTraceback();
    
    QString objectPath_;
    backend::SessionClient* client_;
    
    QTreeView* sourceTree_ = nullptr;
    QStandardItemModel* sourceModel_ = nullptr;
    QTextEdit* sqlEdit_ = nullptr;
    QLabel* pathDepthLabel_ = nullptr;
};

// ============================================================================
// Object Selection Dialog
// ============================================================================
class ObjectSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ObjectSelectionDialog(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString selectedObject() const;

public slots:
    void onDatabaseChanged(int index);
    void onSchemaChanged(int index);
    void onObjectSelected(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);

private:
    void setupUi();
    void loadDatabases();
    void loadSchemas(const QString& database);
    void loadObjects(const QString& database, const QString& schema);
    
    backend::SessionClient* client_;
    QString selectedObject_;
    
    QComboBox* databaseCombo_ = nullptr;
    QComboBox* schemaCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
    QTreeView* objectTree_ = nullptr;
    QStandardItemModel* objectModel_ = nullptr;
};

// ============================================================================
// Lineage Export Dialog
// ============================================================================
class LineageExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit LineageExportDialog(const LineageGraph& graph, QWidget* parent = nullptr);

public slots:
    void onExportImage();
    void onExportPDF();
    void onExportSVG();
    void onExportJSON();
    void onCopyToClipboard();

private:
    void setupUi();
    void renderToImage();
    
    LineageGraph graph_;
    QGraphicsScene* previewScene_ = nullptr;
    QGraphicsView* previewView_ = nullptr;
};

} // namespace scratchrobin::ui
