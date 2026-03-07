#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>
#include <QList>
#include <QHash>
#include <QUuid>
#include <QRectF>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsItem;
class QGraphicsRectItem;
class QGraphicsTextItem;
class QTableView;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTabWidget;
class QListWidget;
class QLabel;
class QSplitter;
class QSpinBox;
class QFormLayout;
class QTreeView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Dashboard Builder - Interactive database dashboards
 * 
 * - Widget-based dashboard designer
 * - Real-time data refresh
 * - Interactive filters and drill-down
 * - Multiple layout configurations
 */

// ============================================================================
// Widget Types
// ============================================================================
enum class DashboardWidgetType {
    Chart,          // Bar, Line, Pie, Area charts
    Table,          // Data table
    KPI,            // Key performance indicator
    Gauge,          // Gauge/meter widget
    Text,           // Rich text/markdown
    Filter,         // Filter controls
    Timeline,       // Event timeline
    Map,            // Geographic map
    Custom          // Custom widget
};

// ============================================================================
// Dashboard Widget Definition
// ============================================================================
struct DashboardWidget {
    QString id;
    DashboardWidgetType type;
    QString title;
    QRectF geometry;  // x, y, width, height (0-100 percentages)
    QString dataSource;
    QHash<QString, QVariant> config;
    int refreshInterval = 0;  // seconds, 0 = manual
    bool autoRefresh = false;
};

// ============================================================================
// Dashboard Definition
// ============================================================================
struct DashboardDefinition {
    QString id;
    QString name;
    QString description;
    QString category;
    QList<DashboardWidget> widgets;
    QHash<QString, QVariant> globalFilters;
    QDateTime createdAt;
    QDateTime modifiedAt;
    QString createdBy;
    bool isShared = false;
};

// ============================================================================
// Dashboard Canvas Item
// ============================================================================
class DashboardCanvasItem : public QObject {
    Q_OBJECT

public:
    explicit DashboardCanvasItem(const DashboardWidget& widget, QGraphicsItem* parent = nullptr);
    
    QGraphicsItem* graphicsItem() const { return item_; }
    DashboardWidget widget() const { return widget_; }
    
    void setSelected(bool selected);
    void updateData(const QVariant& data);
    void refresh();

signals:
    void clicked();
    void doubleClicked();
    void geometryChanged();

private:
    void setupWidget();
    void renderChart();
    void renderTable();
    void renderKPI();
    void renderGauge();
    void renderText();
    void renderFilter();
    
    DashboardWidget widget_;
    QGraphicsItem* item_ = nullptr;
    QGraphicsRectItem* border_ = nullptr;
    QGraphicsTextItem* title_ = nullptr;
    QGraphicsItem* content_ = nullptr;
};

// ============================================================================
// Dashboard Canvas
// ============================================================================
class DashboardCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit DashboardCanvas(QWidget* parent = nullptr);
    
    void setEditMode(bool editMode);
    bool isEditMode() const { return editMode_; }
    
    void addWidget(const DashboardWidget& widget);
    void removeWidget(const QString& widgetId);
    void updateWidget(const QString& widgetId, const DashboardWidget& widget);
    
    QList<DashboardWidget> widgets() const;
    DashboardWidget selectedWidget() const;

public slots:
    void clear();
    void refreshAll();
    void alignWidgets();
    void distributeWidgets();

signals:
    void widgetSelected(const QString& widgetId);
    void widgetDoubleClicked(const QString& widgetId);
    void widgetMoved(const QString& widgetId, const QPointF& pos);
    void widgetResized(const QString& widgetId, const QSizeF& size);
    void canvasContextMenu(const QPoint& pos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void setupScene();
    void drawGrid();
    DashboardCanvasItem* itemAt(const QPointF& pos) const;
    
    QGraphicsScene* scene_ = nullptr;
    bool editMode_ = true;
    bool dragging_ = false;
    bool resizing_ = false;
    QPointF dragStart_;
    QString selectedWidgetId_;
    QHash<QString, DashboardCanvasItem*> items_;
    static constexpr qreal GRID_SIZE = 20.0;
};

// ============================================================================
// Dashboard Designer Dialog
// ============================================================================
class DashboardDesignerDialog : public QDialog {
    Q_OBJECT

public:
    explicit DashboardDesignerDialog(backend::SessionClient* client,
                                    const QString& dashboardId = QString(),
                                    QWidget* parent = nullptr);

public slots:
    void onSave();
    void onPreview();
    void onAddWidget();
    void onEditWidget();
    void onRemoveWidget();
    void onWidgetSelected(const QString& widgetId);
    void onWidgetDoubleClicked(const QString& widgetId);
    void onAlignLeft();
    void onAlignCenter();
    void onAlignRight();
    void onDistributeH();
    void onDistributeV();
    void onToggleGrid(bool show);
    void onSetBackground();
    void onManageDataSources();

private:
    void setupUi();
    void setupToolbar();
    void setupWidgetPalette();
    void setupPropertiesPanel();
    void loadDashboard(const QString& dashboardId);
    void updatePropertiesPanel();
    
    backend::SessionClient* client_;
    QString dashboardId_;
    DashboardDefinition dashboard_;
    
    // UI
    DashboardCanvas* canvas_ = nullptr;
    QListWidget* widgetPalette_ = nullptr;
    QFormLayout* propertiesLayout_ = nullptr;
    QWidget* propertiesPanel_ = nullptr;
    
    // Actions
    QPushButton* addWidgetBtn_ = nullptr;
    QPushButton* removeWidgetBtn_ = nullptr;
    QPushButton* editWidgetBtn_ = nullptr;
};

// ============================================================================
// Widget Configuration Dialog
// ============================================================================
class WidgetConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit WidgetConfigDialog(const DashboardWidget& widget,
                               backend::SessionClient* client,
                               QWidget* parent = nullptr);
    
    DashboardWidget widget() const;

public slots:
    void onTypeChanged(int index);
    void onDataSourceChanged(const QString& source);
    void onPreview();

private:
    void setupUi();
    void setupTypeSpecificConfig();
    void loadDataSources();
    
    DashboardWidget widget_;
    backend::SessionClient* client_;
    
    // Basic
    QLineEdit* titleEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QComboBox* dataSourceCombo_ = nullptr;
    QSpinBox* refreshSpin_ = nullptr;
    
    // Type-specific container
    QWidget* typeConfigWidget_ = nullptr;
    QFormLayout* typeConfigLayout_ = nullptr;
    
    // Chart config
    QComboBox* chartTypeCombo_ = nullptr;
    QComboBox* xAxisCombo_ = nullptr;
    QComboBox* yAxisCombo_ = nullptr;
    
    // KPI config
    QLineEdit* kpiFormatEdit_ = nullptr;
    QComboBox* kpiTrendCombo_ = nullptr;
    
    // Preview
    QGraphicsView* previewView_ = nullptr;
};

// ============================================================================
// Dashboard Manager Panel
// ============================================================================
class DashboardManagerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DashboardManagerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Dashboard Manager"); }
    QString panelCategory() const override { return "reporting"; }
    
    void refresh();

public slots:
    void onNewDashboard();
    void onEditDashboard();
    void onDeleteDashboard();
    void onDuplicateDashboard();
    void onShareDashboard();
    void onSetAsHome();
    void onExportDashboard();
    void onImportDashboard();

signals:
    void dashboardSelected(const QString& dashboardId);
    void dashboardOpenRequested(const QString& dashboardId);

protected:
    void panelActivated() override;

private:
    void setupUi();
    void loadDashboards();
    
    backend::SessionClient* client_;
    
    QTreeView* dashboardTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
};

// ============================================================================
// Dashboard Viewer
// ============================================================================
class DashboardViewer : public QDialog {
    Q_OBJECT

public:
    explicit DashboardViewer(backend::SessionClient* client,
                            const DashboardDefinition& dashboard,
                            QWidget* parent = nullptr);

public slots:
    void onRefresh();
    void onExport();
    void onPrint();
    void onEdit();
    void onFullscreen();
    void onConfigureFilters();

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    void setupUi();
    void applyGlobalFilters();
    void startAutoRefresh();
    void stopAutoRefresh();
    
    backend::SessionClient* client_;
    DashboardDefinition dashboard_;
    DashboardCanvas* canvas_ = nullptr;
    QHash<QString, QVariant> filterValues_;
    int refreshTimerId_ = 0;
};

} // namespace scratchrobin::ui
