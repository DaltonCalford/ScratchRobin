#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QAbstractTableModel>

QT_BEGIN_NAMESPACE
class QTreeView;
class QTableView;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QListView;
class QSplitter;
class QPushButton;
class QTabWidget;
class QGraphicsView;
class QGraphicsScene;
class QLabel;
class QCheckBox;
class QGroupBox;
class QStandardItemModel;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Visual Query Builder - Drag-and-drop query construction
 * 
 * Build SQL queries visually without writing code:
 * - Table drag-and-drop with relationship auto-detection
 * - Column selection with aggregates
 * - Join configuration
 * - WHERE clause builder
 * - GROUP BY / ORDER BY
 * - Visual to SQL conversion
 */

// ============================================================================
// Query Component Types
// ============================================================================
enum class QueryComponentType {
    Table,
    Column,
    Join,
    WhereCondition,
    OrderBy,
    GroupBy,
    Aggregate
};

struct QueryTable {
    QString id;
    QString name;
    QString alias;
    QPointF position;
    QList<QString> selectedColumns;
    bool isSelected = false;
};

struct QueryJoin {
    QString id;
    QString leftTable;
    QString rightTable;
    QString leftColumn;
    QString rightColumn;
    QString joinType; // INNER, LEFT, RIGHT, FULL, CROSS
};

struct QueryColumn {
    QString name;
    QString tableAlias;
    QString aggregate; // SUM, AVG, COUNT, MIN, MAX
    QString alias;
    bool isVisible = true;
    bool isDistinct = false;
    QString sortOrder; // ASC, DESC
    int sortPriority = 0;
};

struct QueryCondition {
    QString id;
    QString column;
    QString op; // =, <>, <, >, <=, >=, LIKE, IN, BETWEEN, IS NULL
    QString value;
    QString logicOp; // AND, OR
    int parenthesisGroup = 0;
};

struct VisualQuery {
    QString queryType; // SELECT, INSERT, UPDATE, DELETE
    QList<QueryTable> tables;
    QList<QueryJoin> joins;
    QList<QueryColumn> columns;
    QList<QueryCondition> whereConditions;
    QList<QueryColumn> groupByColumns;
    QList<QueryColumn> orderByColumns;
    bool isDistinct = false;
    int limit = 0;
    int offset = 0;
    
    QString toSql() const;
    void clear();
};

// ============================================================================
// Table Graphics Item for Query Builder
// ============================================================================
class QueryTableGraphicsItem : public QObject {
    Q_OBJECT
public:
    explicit QueryTableGraphicsItem(const QueryTable& table, QObject* parent = nullptr);
    
    QRectF boundingRect() const;
    void paint(QPainter* painter, const QRectF& rect);
    
    QueryTable table;
    bool isSelected = false;
    
signals:
    void columnToggled(const QString& column, bool checked);
    void tableMoved(const QPointF& pos);
};

// ============================================================================
// Query Builder Panel
// ============================================================================
class QueryBuilderPanel : public DockPanel {
    Q_OBJECT

public:
    explicit QueryBuilderPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Query Builder"); }
    QString panelCategory() const override { return "query"; }

public slots:
    // Canvas operations
    void onAddTable();
    void onRemoveTable();
    void onAddJoin();
    void onRemoveJoin();
    void onTableSelected(const QString& tableId);
    void onCanvasContextMenu(const QPoint& pos);
    
    // Column operations
    void onAddColumn();
    void onRemoveColumn();
    void onColumnMoved(int from, int to);
    
    // Condition operations
    void onAddWhereCondition();
    void onEditCondition();
    void onRemoveCondition();
    
    // Query operations
    void onQueryTypeChanged(const QString& type);
    void onGenerateSql();
    void onExecuteQuery();
    void onSaveQuery();
    void onLoadQuery();
    void onClearQuery();
    
    // View options
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onToggleGrid(bool show);

signals:
    void sqlGenerated(const QString& sql);
    void queryExecuted(const QString& sql);

private:
    void setupUi();
    void setupCanvas();
    void setupTablesPanel();
    void setupColumnsPanel();
    void setupConditionsPanel();
    void refreshCanvas();
    void updateSqlPreview();
    
    backend::SessionClient* client_;
    VisualQuery currentQuery_;
    
    // Visual canvas
    QGraphicsView* canvas_ = nullptr;
    QGraphicsScene* scene_ = nullptr;
    qreal zoomLevel_ = 1.0;
    bool showGrid_ = true;
    
    // Table browser
    QTreeView* tableBrowser_ = nullptr;
    QStandardItemModel* tableModel_ = nullptr;
    
    // Columns panel
    QTableView* columnsTable_ = nullptr;
    QStandardItemModel* columnsModel_ = nullptr;
    
    // Conditions panel
    QTableView* conditionsTable_ = nullptr;
    QStandardItemModel* conditionsModel_ = nullptr;
    
    // SQL preview
    QTextEdit* sqlPreview_ = nullptr;
    
    // Options
    QComboBox* queryTypeCombo_ = nullptr;
    QCheckBox* distinctCheck_ = nullptr;
    QLineEdit* limitEdit_ = nullptr;
};

// ============================================================================
// Join Configuration Dialog
// ============================================================================
class JoinConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit JoinConfigDialog(QueryJoin* join, 
                             const QList<QueryTable>& tables,
                             QWidget* parent = nullptr);

public slots:
    void onLeftTableChanged(int index);
    void onRightTableChanged(int index);
    void onAccept();

private:
    void setupUi();
    void loadColumns(const QString& tableId, QComboBox* combo);
    
    QueryJoin* join_;
    QList<QueryTable> tables_;
    
    QComboBox* leftTableCombo_ = nullptr;
    QComboBox* rightTableCombo_ = nullptr;
    QComboBox* leftColumnCombo_ = nullptr;
    QComboBox* rightColumnCombo_ = nullptr;
    QComboBox* joinTypeCombo_ = nullptr;
};

// ============================================================================
// Where Condition Dialog
// ============================================================================
class WhereConditionDialog : public QDialog {
    Q_OBJECT

public:
    explicit WhereConditionDialog(QueryCondition* condition,
                                 const VisualQuery& query,
                                 QWidget* parent = nullptr);

public slots:
    void onColumnChanged(int index);
    void onOperatorChanged(int index);
    void onAccept();

private:
    void setupUi();
    void updateValueWidget();
    
    QueryCondition* condition_;
    VisualQuery query_;
    
    QComboBox* columnCombo_ = nullptr;
    QComboBox* operatorCombo_ = nullptr;
    QWidget* valueWidget_ = nullptr;
    QLineEdit* valueEdit_ = nullptr;
    QComboBox* valueCombo_ = nullptr;
    QComboBox* logicOpCombo_ = nullptr;
};

} // namespace scratchrobin::ui
