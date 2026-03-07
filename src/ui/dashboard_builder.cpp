#include "ui/dashboard_builder.h"
#include "backend/session_client.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsProxyWidget>
#include <QTreeView>
#include <QTableView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QTimer>
#include <QScrollBar>
#include <QPainter>
#include <QMimeData>

namespace scratchrobin::ui {

// ============================================================================
// DashboardCanvasItem
// ============================================================================

DashboardCanvasItem::DashboardCanvasItem(const DashboardWidget& widget, QGraphicsItem* parent)
    : QObject(nullptr), widget_(widget) {
    setupWidget();
}

void DashboardCanvasItem::setupWidget() {
    // Create container
    item_ = new QGraphicsRectItem();
    
    // Border
    border_ = new QGraphicsRectItem(0, 0, 200, 150, item_);
    border_->setPen(QPen(QColor(200, 200, 200), 1));
    border_->setBrush(QBrush(Qt::white));
    
    // Title
    title_ = new QGraphicsTextItem(widget_.title, item_);
    title_->setPos(5, 5);
    QFont titleFont = title_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(9);
    title_->setFont(titleFont);
    
    // Content based on type
    switch (widget_.type) {
        case DashboardWidgetType::Chart:
            renderChart();
            break;
        case DashboardWidgetType::Table:
            renderTable();
            break;
        case DashboardWidgetType::KPI:
            renderKPI();
            break;
        case DashboardWidgetType::Gauge:
            renderGauge();
            break;
        case DashboardWidgetType::Text:
            renderText();
            break;
        case DashboardWidgetType::Filter:
            renderFilter();
            break;
        default:
            break;
    }
}

void DashboardCanvasItem::renderChart() {
    // Simple bar chart representation
    int barWidth = 20;
    int spacing = 10;
    int baseline = 100;
    
    QList<int> values = {30, 60, 45, 80, 55};
    
    for (int i = 0; i < values.size(); ++i) {
        auto* bar = new QGraphicsRectItem(20 + i * (barWidth + spacing), 
                                          baseline - values[i], 
                                          barWidth, values[i], item_);
        bar->setBrush(QBrush(QColor(66, 133, 244)));
        bar->setPen(Qt::NoPen);
    }
    
    // Axis
    auto* xAxis = new QGraphicsLineItem(10, baseline, 160, baseline, item_);
    xAxis->setPen(QPen(Qt::black, 1));
}

void DashboardCanvasItem::renderTable() {
    // Table header
    auto* header = new QGraphicsRectItem(10, 30, 180, 20, item_);
    header->setBrush(QBrush(QColor(240, 240, 240)));
    
    auto* headerText = new QGraphicsTextItem("Col1    Col2    Col3", item_);
    headerText->setPos(15, 30);
    
    // Rows
    for (int i = 0; i < 3; ++i) {
        auto* rowText = new QGraphicsTextItem(
            QString("Row%1    %2    %3").arg(i+1).arg(i*10).arg(i*100), item_);
        rowText->setPos(15, 55 + i * 18);
    }
}

void DashboardCanvasItem::renderKPI() {
    auto* valueText = new QGraphicsTextItem("1,234", item_);
    QFont valueFont = valueText->font();
    valueFont.setPointSize(24);
    valueFont.setBold(true);
    valueText->setFont(valueFont);
    valueText->setDefaultTextColor(QColor(66, 133, 244));
    valueText->setPos(50, 50);
    
    auto* changeText = new QGraphicsTextItem("+12.5% ▲", item_);
    changeText->setDefaultTextColor(QColor(76, 175, 80));
    changeText->setPos(60, 85);
}

void DashboardCanvasItem::renderGauge() {
    // Simple gauge arc
    QPainterPath path;
    path.arcMoveTo(20, 40, 120, 80, 210);
    path.arcTo(20, 40, 120, 80, 210, 120);
    
    auto* arc = new QGraphicsPathItem(path, item_);
    arc->setPen(QPen(QColor(200, 200, 200), 8));
    
    // Value arc
    QPainterPath valuePath;
    valuePath.arcMoveTo(20, 40, 120, 80, 210);
    valuePath.arcTo(20, 40, 120, 80, 210, 80);
    
    auto* valueArc = new QGraphicsPathItem(valuePath, item_);
    valueArc->setPen(QPen(QColor(66, 133, 244), 8));
    
    auto* valueText = new QGraphicsTextItem("67%", item_);
    QFont valueFont = valueText->font();
    valueFont.setPointSize(16);
    valueFont.setBold(true);
    valueText->setFont(valueFont);
    valueText->setPos(65, 65);
}

void DashboardCanvasItem::renderText() {
    auto* text = new QGraphicsTextItem(
        "<b>Summary</b><br>"
        "Total sales increased by 15% this month.<br>"
        "Customer satisfaction is at 94%.", item_);
    text->setTextWidth(180);
    text->setPos(10, 35);
}

void DashboardCanvasItem::renderFilter() {
    // Visual representation of filter controls
    auto* label1 = new QGraphicsTextItem("Date Range:", item_);
    label1->setPos(10, 35);
    
    auto* rect1 = new QGraphicsRectItem(10, 55, 80, 20, item_);
    rect1->setBrush(QBrush(Qt::white));
    rect1->setPen(QPen(QColor(200, 200, 200)));
    
    auto* label2 = new QGraphicsTextItem("Region:", item_);
    label2->setPos(10, 85);
    
    auto* rect2 = new QGraphicsRectItem(10, 105, 80, 20, item_);
    rect2->setBrush(QBrush(Qt::white));
    rect2->setPen(QPen(QColor(200, 200, 200)));
}

void DashboardCanvasItem::setSelected(bool selected) {
    if (border_) {
        border_->setPen(QPen(selected ? QColor(66, 133, 244) : QColor(200, 200, 200), 
                             selected ? 2 : 1));
    }
}

void DashboardCanvasItem::updateData(const QVariant& data) {
    Q_UNUSED(data)
    // Update widget content with new data
}

void DashboardCanvasItem::refresh() {
    // Trigger data refresh
}

// ============================================================================
// DashboardCanvas
// ============================================================================

DashboardCanvas::DashboardCanvas(QWidget* parent) : QGraphicsView(parent) {
    setupScene();
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setAcceptDrops(true);
}

void DashboardCanvas::setupScene() {
    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(0, 0, 1000, 800);
    setScene(scene_);
    
    // Background
    scene_->setBackgroundBrush(QBrush(QColor(245, 245, 245)));
    
    if (editMode_) {
        drawGrid();
    }
}

void DashboardCanvas::drawGrid() {
    // Add grid lines
    QPen gridPen(QColor(220, 220, 220), 1, Qt::DotLine);
    
    for (qreal x = 0; x <= scene_->width(); x += GRID_SIZE) {
        scene_->addLine(x, 0, x, scene_->height(), gridPen);
    }
    for (qreal y = 0; y <= scene_->height(); y += GRID_SIZE) {
        scene_->addLine(0, y, scene_->width(), y, gridPen);
    }
}

void DashboardCanvas::setEditMode(bool editMode) {
    editMode_ = editMode;
    scene_->clear();
    if (editMode_) {
        drawGrid();
    }
    
    // Re-add widgets
    for (auto* item : items_) {
        scene_->addItem(item->graphicsItem());
    }
}

void DashboardCanvas::addWidget(const DashboardWidget& widget) {
    auto* item = new DashboardCanvasItem(widget);
    items_[widget.id] = item;
    
    auto* graphicsItem = item->graphicsItem();
    graphicsItem->setPos(widget.geometry.x(), widget.geometry.y());
    graphicsItem->setFlag(QGraphicsItem::ItemIsMovable, editMode_);
    graphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, editMode_);
    
    scene_->addItem(graphicsItem);
}

void DashboardCanvas::removeWidget(const QString& widgetId) {
    if (items_.contains(widgetId)) {
        scene_->removeItem(items_[widgetId]->graphicsItem());
        delete items_[widgetId];
        items_.remove(widgetId);
    }
}

void DashboardCanvas::updateWidget(const QString& widgetId, const DashboardWidget& widget) {
    removeWidget(widgetId);
    addWidget(widget);
}

void DashboardCanvas::clear() {
    for (auto* item : items_) {
        scene_->removeItem(item->graphicsItem());
        delete item;
    }
    items_.clear();
}

void DashboardCanvas::refreshAll() {
    for (auto* item : items_) {
        item->refresh();
    }
}

QList<DashboardWidget> DashboardCanvas::widgets() const {
    QList<DashboardWidget> result;
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        result.append(it.value()->widget());
    }
    return result;
}

DashboardWidget DashboardCanvas::selectedWidget() const {
    if (selectedWidgetId_.isEmpty() || !items_.contains(selectedWidgetId_)) {
        return DashboardWidget();
    }
    return items_[selectedWidgetId_]->widget();
}

void DashboardCanvas::mousePressEvent(QMouseEvent* event) {
    auto scenePos = mapToScene(event->pos());
    
    // Find clicked widget
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it.value()->graphicsItem()->contains(it.value()->graphicsItem()->mapFromScene(scenePos))) {
            selectedWidgetId_ = it.key();
            it.value()->setSelected(true);
            dragging_ = true;
            dragStart_ = scenePos;
            emit widgetSelected(selectedWidgetId_);
            
            // Deselect others
            for (auto it2 = items_.begin(); it2 != items_.end(); ++it2) {
                if (it2.key() != selectedWidgetId_) {
                    it2.value()->setSelected(false);
                }
            }
            break;
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void DashboardCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_ && editMode_ && !selectedWidgetId_.isEmpty()) {
        auto scenePos = mapToScene(event->pos());
        auto delta = scenePos - dragStart_;
        
        if (items_.contains(selectedWidgetId_)) {
            auto* item = items_[selectedWidgetId_]->graphicsItem();
            auto newPos = item->pos() + delta;
            
            // Snap to grid
            newPos.setX(qRound(newPos.x() / GRID_SIZE) * GRID_SIZE);
            newPos.setY(qRound(newPos.y() / GRID_SIZE) * GRID_SIZE);
            
            item->setPos(newPos);
            dragStart_ = scenePos;
            emit widgetMoved(selectedWidgetId_, newPos);
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void DashboardCanvas::mouseReleaseEvent(QMouseEvent* event) {
    dragging_ = false;
    QGraphicsView::mouseReleaseEvent(event);
}

void DashboardCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (!selectedWidgetId_.isEmpty()) {
        emit widgetDoubleClicked(selectedWidgetId_);
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void DashboardCanvas::contextMenuEvent(QContextMenuEvent* event) {
    emit canvasContextMenu(event->pos());
}

void DashboardCanvas::dragEnterEvent(QDragEnterEvent* event) {
    if (editMode_) {
        event->acceptProposedAction();
    }
}

void DashboardCanvas::dragMoveEvent(QDragMoveEvent* event) {
    if (editMode_) {
        event->acceptProposedAction();
    }
}

void DashboardCanvas::dropEvent(QDropEvent* event) {
    if (!editMode_) return;
    
    // Handle widget drop from palette
    QString widgetType = event->mimeData()->text();
    
    DashboardWidget widget;
    widget.id = QUuid::createUuid().toString();
    widget.type = DashboardWidgetType::Chart;  // Default
    widget.title = tr("New Widget");
    widget.geometry = QRectF(mapToScene(event->pos()).x(), 
                             mapToScene(event->pos()).y(), 
                             200, 150);
    
    if (widgetType == "chart") {
        widget.type = DashboardWidgetType::Chart;
        widget.title = tr("Chart");
    } else if (widgetType == "table") {
        widget.type = DashboardWidgetType::Table;
        widget.title = tr("Data Table");
    } else if (widgetType == "kpi") {
        widget.type = DashboardWidgetType::KPI;
        widget.title = tr("KPI");
    } else if (widgetType == "gauge") {
        widget.type = DashboardWidgetType::Gauge;
        widget.title = tr("Gauge");
    } else if (widgetType == "text") {
        widget.type = DashboardWidgetType::Text;
        widget.title = tr("Text");
    } else if (widgetType == "filter") {
        widget.type = DashboardWidgetType::Filter;
        widget.title = tr("Filters");
    }
    
    addWidget(widget);
    event->acceptProposedAction();
}

void DashboardCanvas::alignWidgets() {
    // Align selected widgets
}

void DashboardCanvas::distributeWidgets() {
    // Distribute selected widgets evenly
}

// ============================================================================
// DashboardDesignerDialog
// ============================================================================

DashboardDesignerDialog::DashboardDesignerDialog(backend::SessionClient* client,
                                                const QString& dashboardId,
                                                QWidget* parent)
    : QDialog(parent), client_(client), dashboardId_(dashboardId) {
    
    setWindowTitle(dashboardId.isEmpty() ? tr("New Dashboard") : tr("Edit Dashboard"));
    setMinimumSize(1200, 800);
    
    setupUi();
    
    if (!dashboardId.isEmpty()) {
        loadDashboard(dashboardId);
    }
}

void DashboardDesignerDialog::setupUi() {
    auto* mainLayout = new QHBoxLayout(this);
    
    // Left panel - Widget palette
    setupWidgetPalette();
    mainLayout->addWidget(widgetPalette_, 1);
    
    // Center - Canvas
    canvas_ = new DashboardCanvas(this);
    mainLayout->addWidget(canvas_, 6);
    
    // Right panel - Properties
    setupPropertiesPanel();
    mainLayout->addWidget(propertiesPanel_, 2);
    
    // Toolbar
    setupToolbar();
    
    // Connect canvas signals
    connect(canvas_, &DashboardCanvas::widgetSelected,
            this, &DashboardDesignerDialog::onWidgetSelected);
    connect(canvas_, &DashboardCanvas::widgetDoubleClicked,
            this, &DashboardDesignerDialog::onWidgetDoubleClicked);
}

void DashboardDesignerDialog::setupToolbar() {
    auto* toolbar = new QToolBar(this);
    
    auto* saveAction = toolbar->addAction(tr("Save"));
    auto* previewAction = toolbar->addAction(tr("Preview"));
    toolbar->addSeparator();
    auto* alignLeftAction = toolbar->addAction(tr("Align Left"));
    auto* alignCenterAction = toolbar->addAction(tr("Align Center"));
    auto* alignRightAction = toolbar->addAction(tr("Align Right"));
    toolbar->addSeparator();
    auto* gridAction = toolbar->addAction(tr("Show Grid"));
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    
    layout()->setMenuBar(toolbar);
    
    connect(saveAction, &QAction::triggered, this, &DashboardDesignerDialog::onSave);
    connect(previewAction, &QAction::triggered, this, &DashboardDesignerDialog::onPreview);
    connect(alignLeftAction, &QAction::triggered, this, &DashboardDesignerDialog::onAlignLeft);
    connect(alignCenterAction, &QAction::triggered, this, &DashboardDesignerDialog::onAlignCenter);
    connect(alignRightAction, &QAction::triggered, this, &DashboardDesignerDialog::onAlignRight);
    connect(gridAction, &QAction::toggled, this, &DashboardDesignerDialog::onToggleGrid);
}

void DashboardDesignerDialog::setupWidgetPalette() {
    widgetPalette_ = new QListWidget(this);
    widgetPalette_->setDragEnabled(true);
    widgetPalette_->setMaximumWidth(150);
    
    // Add widget types
    auto* chartItem = new QListWidgetItem(tr("📊 Chart"), widgetPalette_);
    chartItem->setData(Qt::UserRole, "chart");
    
    auto* tableItem = new QListWidgetItem(tr("📋 Table"), widgetPalette_);
    tableItem->setData(Qt::UserRole, "table");
    
    auto* kpiItem = new QListWidgetItem(tr("🔢 KPI"), widgetPalette_);
    kpiItem->setData(Qt::UserRole, "kpi");
    
    auto* gaugeItem = new QListWidgetItem(tr("🎚️ Gauge"), widgetPalette_);
    gaugeItem->setData(Qt::UserRole, "gauge");
    
    auto* textItem = new QListWidgetItem(tr("📝 Text"), widgetPalette_);
    textItem->setData(Qt::UserRole, "text");
    
    auto* filterItem = new QListWidgetItem(tr("🔍 Filter"), widgetPalette_);
    filterItem->setData(Qt::UserRole, "filter");
    
    // Buttons
    auto* container = new QWidget(this);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    
    layout->addWidget(new QLabel(tr("Widget Palette"), this));
    layout->addWidget(widgetPalette_);
    
    addWidgetBtn_ = new QPushButton(tr("Add Widget"), this);
    editWidgetBtn_ = new QPushButton(tr("Edit Widget"), this);
    removeWidgetBtn_ = new QPushButton(tr("Remove Widget"), this);
    
    layout->addWidget(addWidgetBtn_);
    layout->addWidget(editWidgetBtn_);
    layout->addWidget(removeWidgetBtn_);
    layout->addStretch();
    
    connect(addWidgetBtn_, &QPushButton::clicked, this, &DashboardDesignerDialog::onAddWidget);
    connect(editWidgetBtn_, &QPushButton::clicked, this, &DashboardDesignerDialog::onEditWidget);
    connect(removeWidgetBtn_, &QPushButton::clicked, this, &DashboardDesignerDialog::onRemoveWidget);
    
    widgetPalette_ = container->findChild<QListWidget*>();
}

void DashboardDesignerDialog::setupPropertiesPanel() {
    propertiesPanel_ = new QGroupBox(tr("Properties"), this);
    propertiesLayout_ = new QFormLayout(propertiesPanel_);
    
    propertiesLayout_->addRow(new QLabel(tr("Select a widget to edit properties"), this));
}

void DashboardDesignerDialog::loadDashboard(const QString& dashboardId) {
    Q_UNUSED(dashboardId)
    
    // Mock dashboard
    dashboard_.name = "Sales Dashboard";
    
    DashboardWidget widget1;
    widget1.id = "widget1";
    widget1.type = DashboardWidgetType::KPI;
    widget1.title = "Total Sales";
    widget1.geometry = QRectF(50, 50, 200, 120);
    canvas_->addWidget(widget1);
    
    DashboardWidget widget2;
    widget2.id = "widget2";
    widget2.type = DashboardWidgetType::Chart;
    widget2.title = "Monthly Sales";
    widget2.geometry = QRectF(270, 50, 400, 250);
    canvas_->addWidget(widget2);
    
    DashboardWidget widget3;
    widget3.id = "widget3";
    widget3.type = DashboardWidgetType::Table;
    widget3.title = "Top Customers";
    widget3.geometry = QRectF(50, 190, 200, 200);
    canvas_->addWidget(widget3);
}

void DashboardDesignerDialog::updatePropertiesPanel() {
    // Clear existing
    while (propertiesLayout_->rowCount() > 0) {
        propertiesLayout_->removeRow(0);
    }
    
    auto widget = canvas_->selectedWidget();
    if (widget.id.isEmpty()) {
        propertiesLayout_->addRow(new QLabel(tr("Select a widget to edit properties"), this));
        return;
    }
    
    // Add property fields
    auto* titleEdit = new QLineEdit(widget.title, this);
    propertiesLayout_->addRow(tr("Title:"), titleEdit);
    
    auto* xSpin = new QSpinBox(this);
    xSpin->setRange(0, 1000);
    xSpin->setValue(widget.geometry.x());
    propertiesLayout_->addRow(tr("X:"), xSpin);
    
    auto* ySpin = new QSpinBox(this);
    ySpin->setRange(0, 800);
    ySpin->setValue(widget.geometry.y());
    propertiesLayout_->addRow(tr("Y:"), ySpin);
    
    auto* wSpin = new QSpinBox(this);
    wSpin->setRange(50, 500);
    wSpin->setValue(widget.geometry.width());
    propertiesLayout_->addRow(tr("Width:"), wSpin);
    
    auto* hSpin = new QSpinBox(this);
    hSpin->setRange(50, 400);
    hSpin->setValue(widget.geometry.height());
    propertiesLayout_->addRow(tr("Height:"), hSpin);
    
    auto* refreshSpin = new QSpinBox(this);
    refreshSpin->setRange(0, 3600);
    refreshSpin->setValue(widget.refreshInterval);
    refreshSpin->setSuffix(tr(" s"));
    propertiesLayout_->addRow(tr("Refresh:"), refreshSpin);
}

void DashboardDesignerDialog::onSave() {
    if (dashboard_.name.isEmpty()) {
        bool ok;
        QString name = QInputDialog::getText(this, tr("Dashboard Name"),
            tr("Enter dashboard name:"), QLineEdit::Normal, QString(), &ok);
        if (ok && !name.isEmpty()) {
            dashboard_.name = name;
        }
    }
    accept();
}

void DashboardDesignerDialog::onPreview() {
    canvas_->setEditMode(false);
    
    QMessageBox::information(this, tr("Preview"), 
        tr("Dashboard is now in preview mode. Close this dialog to return to editing."));
    
    canvas_->setEditMode(true);
}

void DashboardDesignerDialog::onAddWidget() {
    WidgetConfigDialog dlg(DashboardWidget(), client_, this);
    if (dlg.exec() == QDialog::Accepted) {
        auto widget = dlg.widget();
        widget.id = QUuid::createUuid().toString();
        widget.geometry = QRectF(100, 100, 200, 150);
        canvas_->addWidget(widget);
    }
}

void DashboardDesignerDialog::onEditWidget() {
    auto widget = canvas_->selectedWidget();
    if (widget.id.isEmpty()) {
        QMessageBox::information(this, tr("Edit Widget"), tr("Please select a widget to edit."));
        return;
    }
    
    WidgetConfigDialog dlg(widget, client_, this);
    if (dlg.exec() == QDialog::Accepted) {
        canvas_->updateWidget(widget.id, dlg.widget());
    }
}

void DashboardDesignerDialog::onRemoveWidget() {
    auto widget = canvas_->selectedWidget();
    if (widget.id.isEmpty()) {
        QMessageBox::information(this, tr("Remove Widget"), tr("Please select a widget to remove."));
        return;
    }
    
    auto reply = QMessageBox::question(this, tr("Remove Widget"),
        tr("Remove widget '%1'?").arg(widget.title));
    
    if (reply == QMessageBox::Yes) {
        canvas_->removeWidget(widget.id);
    }
}

void DashboardDesignerDialog::onWidgetSelected(const QString& widgetId) {
    Q_UNUSED(widgetId)
    updatePropertiesPanel();
}

void DashboardDesignerDialog::onWidgetDoubleClicked(const QString& widgetId) {
    Q_UNUSED(widgetId)
    onEditWidget();
}

void DashboardDesignerDialog::onAlignLeft() { canvas_->alignWidgets(); }
void DashboardDesignerDialog::onAlignCenter() { canvas_->alignWidgets(); }
void DashboardDesignerDialog::onAlignRight() { canvas_->alignWidgets(); }
void DashboardDesignerDialog::onDistributeH() { canvas_->distributeWidgets(); }
void DashboardDesignerDialog::onDistributeV() { canvas_->distributeWidgets(); }

void DashboardDesignerDialog::onToggleGrid(bool show) {
    if (!show) {
        canvas_->scene()->clear();
    }
}

void DashboardDesignerDialog::onSetBackground() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Background"),
        QString(), tr("Images (*.png *.jpg *.bmp)"));
    
    if (!fileName.isEmpty()) {
        // Set background image
    }
}

void DashboardDesignerDialog::onManageDataSources() {
    QMessageBox::information(this, tr("Data Sources"), tr("Manage data sources..."));
}

// ============================================================================
// WidgetConfigDialog
// ============================================================================

WidgetConfigDialog::WidgetConfigDialog(const DashboardWidget& widget,
                                       backend::SessionClient* client,
                                       QWidget* parent)
    : QDialog(parent), widget_(widget), client_(client) {
    
    setWindowTitle(widget.id.isEmpty() ? tr("New Widget") : tr("Edit Widget"));
    setMinimumSize(500, 400);
    
    setupUi();
}

void WidgetConfigDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Basic properties
    auto* basicGroup = new QGroupBox(tr("Basic"), this);
    auto* basicLayout = new QFormLayout(basicGroup);
    
    titleEdit_ = new QLineEdit(widget_.title, this);
    basicLayout->addRow(tr("Title:"), titleEdit_);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({tr("Chart"), tr("Table"), tr("KPI"), tr("Gauge"), 
                          tr("Text"), tr("Filter")});
    basicLayout->addRow(tr("Type:"), typeCombo_);
    
    dataSourceCombo_ = new QComboBox(this);
    dataSourceCombo_->setEditable(true);
    basicLayout->addRow(tr("Data Source:"), dataSourceCombo_);
    
    refreshSpin_ = new QSpinBox(this);
    refreshSpin_->setRange(0, 3600);
    refreshSpin_->setSuffix(tr(" seconds"));
    basicLayout->addRow(tr("Refresh Interval:"), refreshSpin_);
    
    mainLayout->addWidget(basicGroup);
    
    // Type-specific config
    typeConfigWidget_ = new QGroupBox(tr("Configuration"), this);
    typeConfigLayout_ = new QFormLayout(typeConfigWidget_);
    mainLayout->addWidget(typeConfigWidget_);
    
    setupTypeSpecificConfig();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    auto* okBtn = new QPushButton(tr("OK"), this);
    okBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    
    btnLayout->addWidget(previewBtn);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
    
    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WidgetConfigDialog::onTypeChanged);
    connect(previewBtn, &QPushButton::clicked, this, &WidgetConfigDialog::onPreview);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    loadDataSources();
}

void WidgetConfigDialog::setupTypeSpecificConfig() {
    // Clear existing
    while (typeConfigLayout_->rowCount() > 0) {
        typeConfigLayout_->removeRow(0);
    }
    
    int typeIndex = typeCombo_->currentIndex();
    
    switch (typeIndex) {
        case 0: // Chart
            chartTypeCombo_ = new QComboBox(this);
            chartTypeCombo_->addItems({tr("Bar"), tr("Line"), tr("Pie"), tr("Area"),
                                       tr("Scatter"), tr("Donut")});
            typeConfigLayout_->addRow(tr("Chart Type:"), chartTypeCombo_);
            
            xAxisCombo_ = new QComboBox(this);
            typeConfigLayout_->addRow(tr("X Axis:"), xAxisCombo_);
            
            yAxisCombo_ = new QComboBox(this);
            typeConfigLayout_->addRow(tr("Y Axis:"), yAxisCombo_);
            break;
            
        case 2: // KPI
            kpiFormatEdit_ = new QLineEdit(this);
            typeConfigLayout_->addRow(tr("Number Format:"), kpiFormatEdit_);
            
            kpiTrendCombo_ = new QComboBox(this);
            kpiTrendCombo_->addItems({tr("None"), tr("Up is Good"), tr("Down is Good")});
            typeConfigLayout_->addRow(tr("Trend:"), kpiTrendCombo_);
            break;
            
        default:
            typeConfigLayout_->addRow(new QLabel(tr("No additional configuration needed."), this));
            break;
    }
}

void WidgetConfigDialog::loadDataSources() {
    dataSourceCombo_->addItems({
        "sales_by_month",
        "top_customers",
        "inventory_levels",
        "query_performance",
        "user_activity"
    });
}

DashboardWidget WidgetConfigDialog::widget() const {
    DashboardWidget result = widget_;
    result.title = titleEdit_->text();
    result.type = static_cast<DashboardWidgetType>(typeCombo_->currentIndex());
    result.dataSource = dataSourceCombo_->currentText();
    result.refreshInterval = refreshSpin_->value();
    return result;
}

void WidgetConfigDialog::onTypeChanged(int index) {
    Q_UNUSED(index)
    setupTypeSpecificConfig();
}

void WidgetConfigDialog::onDataSourceChanged(const QString& source) {
    Q_UNUSED(source)
}

void WidgetConfigDialog::onPreview() {
    QMessageBox::information(this, tr("Preview"), tr("Widget preview..."));
}

// ============================================================================
// DashboardManagerPanel
// ============================================================================

DashboardManagerPanel::DashboardManagerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("dashboard_manager", parent), client_(client) {
    setupUi();
}

void DashboardManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    auto* newBtn = new QPushButton(tr("New"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    auto* viewBtn = new QPushButton(tr("View"), this);
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    
    toolbar->addWidget(newBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(viewBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addStretch();
    toolbar->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbar);
    
    // Tree
    dashboardTree_ = new QTreeView(this);
    dashboardTree_->setHeaderHidden(false);
    dashboardTree_->setAlternatingRowColors(true);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Dashboard"), tr("Category"), tr("Modified")});
    dashboardTree_->setModel(model_);
    
    mainLayout->addWidget(dashboardTree_);
    
    loadDashboards();
    
    connect(newBtn, &QPushButton::clicked, this, &DashboardManagerPanel::onNewDashboard);
    connect(editBtn, &QPushButton::clicked, this, &DashboardManagerPanel::onEditDashboard);
    connect(viewBtn, &QPushButton::clicked, this, [this]() {
        auto index = dashboardTree_->currentIndex();
        if (index.isValid()) {
            emit dashboardOpenRequested(index.data().toString());
        }
    });
    connect(deleteBtn, &QPushButton::clicked, this, &DashboardManagerPanel::onDeleteDashboard);
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardManagerPanel::refresh);
}

void DashboardManagerPanel::loadDashboards() {
    model_->removeRows(0, model_->rowCount());
    
    // Sales
    auto* salesCat = new QStandardItem(tr("Sales"));
    salesCat->appendRow({new QStandardItem(tr("Sales Overview")),
                         new QStandardItem(tr("Sales")),
                         new QStandardItem(tr("2026-03-05"))});
    salesCat->appendRow({new QStandardItem(tr("Regional Performance")),
                         new QStandardItem(tr("Sales")),
                         new QStandardItem(tr("2026-03-04"))});
    model_->appendRow(salesCat);
    
    // System
    auto* sysCat = new QStandardItem(tr("System"));
    sysCat->appendRow({new QStandardItem(tr("Database Health")),
                       new QStandardItem(tr("System")),
                       new QStandardItem(tr("2026-03-06"))});
    sysCat->appendRow({new QStandardItem(tr("Query Performance")),
                       new QStandardItem(tr("System")),
                       new QStandardItem(tr("2026-03-06"))});
    model_->appendRow(sysCat);
    
    dashboardTree_->expandAll();
    dashboardTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void DashboardManagerPanel::refresh() {
    loadDashboards();
}

void DashboardManagerPanel::panelActivated() {
    refresh();
}

void DashboardManagerPanel::onNewDashboard() {
    DashboardDesignerDialog dlg(client_, QString(), this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void DashboardManagerPanel::onEditDashboard() {
    auto index = dashboardTree_->currentIndex();
    if (!index.isValid()) return;
    
    DashboardDesignerDialog dlg(client_, "dashboard_id", this);
    dlg.exec();
}

void DashboardManagerPanel::onDeleteDashboard() {
    auto reply = QMessageBox::question(this, tr("Delete Dashboard"),
        tr("Are you sure you want to delete this dashboard?"));
    
    if (reply == QMessageBox::Yes) {
        refresh();
    }
}

void DashboardManagerPanel::onDuplicateDashboard() {
    // Duplicate selected dashboard
}

void DashboardManagerPanel::onShareDashboard() {
    QMessageBox::information(this, tr("Share"), tr("Dashboard sharing options..."));
}

void DashboardManagerPanel::onSetAsHome() {
    QMessageBox::information(this, tr("Set as Home"), 
        tr("This dashboard will be shown on startup."));
}

void DashboardManagerPanel::onExportDashboard() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Dashboard"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Dashboard exported."));
    }
}

void DashboardManagerPanel::onImportDashboard() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Dashboard"),
        QString(), tr("JSON Files (*.json)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Import"), tr("Dashboard imported."));
        refresh();
    }
}

// ============================================================================
// DashboardViewer
// ============================================================================

DashboardViewer::DashboardViewer(backend::SessionClient* client,
                                const DashboardDefinition& dashboard,
                                QWidget* parent)
    : QDialog(parent), client_(client), dashboard_(dashboard) {
    
    setWindowTitle(dashboard.name);
    setMinimumSize(1200, 800);
    
    setupUi();
    startAutoRefresh();
}

void DashboardViewer::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    auto* filterBtn = new QPushButton(tr("Filters"), this);
    auto* exportBtn = new QPushButton(tr("Export"), this);
    auto* printBtn = new QPushButton(tr("Print"), this);
    auto* editBtn = new QPushButton(tr("Edit"), this);
    auto* fullscreenBtn = new QPushButton(tr("Fullscreen"), this);
    
    toolbar->addWidget(refreshBtn);
    toolbar->addWidget(filterBtn);
    toolbar->addStretch();
    toolbar->addWidget(exportBtn);
    toolbar->addWidget(printBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(fullscreenBtn);
    
    mainLayout->addLayout(toolbar);
    
    // Canvas
    canvas_ = new DashboardCanvas(this);
    canvas_->setEditMode(false);
    
    // Load widgets
    for (const auto& widget : dashboard_.widgets) {
        canvas_->addWidget(widget);
    }
    
    mainLayout->addWidget(canvas_);
    
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardViewer::onRefresh);
    connect(filterBtn, &QPushButton::clicked, this, &DashboardViewer::onConfigureFilters);
    connect(exportBtn, &QPushButton::clicked, this, &DashboardViewer::onExport);
    connect(printBtn, &QPushButton::clicked, this, &DashboardViewer::onPrint);
    connect(editBtn, &QPushButton::clicked, this, &DashboardViewer::onEdit);
    connect(fullscreenBtn, &QPushButton::clicked, this, &DashboardViewer::onFullscreen);
}

void DashboardViewer::applyGlobalFilters() {
    // Apply filter values to all widgets
}

void DashboardViewer::startAutoRefresh() {
    // Start timer for auto-refresh
    for (const auto& widget : dashboard_.widgets) {
        if (widget.autoRefresh && widget.refreshInterval > 0) {
            refreshTimerId_ = startTimer(widget.refreshInterval * 1000);
            break;
        }
    }
}

void DashboardViewer::stopAutoRefresh() {
    if (refreshTimerId_) {
        killTimer(refreshTimerId_);
        refreshTimerId_ = 0;
    }
}

void DashboardViewer::timerEvent(QTimerEvent* event) {
    if (event->timerId() == refreshTimerId_) {
        canvas_->refreshAll();
    }
}

void DashboardViewer::onRefresh() {
    canvas_->refreshAll();
}

void DashboardViewer::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Dashboard"),
        QString(), tr("PDF (*.pdf);;PNG (*.png);;HTML (*.html)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Dashboard exported."));
    }
}

void DashboardViewer::onPrint() {
    QMessageBox::information(this, tr("Print"), tr("Printing dashboard..."));
}

void DashboardViewer::onEdit() {
    DashboardDesignerDialog dlg(client_, dashboard_.id, this);
    dlg.exec();
}

void DashboardViewer::onFullscreen() {
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void DashboardViewer::onConfigureFilters() {
    QMessageBox::information(this, tr("Filters"), tr("Configure dashboard filters..."));
}

} // namespace scratchrobin::ui
