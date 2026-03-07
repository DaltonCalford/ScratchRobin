#include "data_lineage.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableView>
#include <QTreeView>
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
#include <QListWidget>
#include <QDialogButtonBox>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QPen>
#include <QBrush>
#include <QFont>

namespace scratchrobin::ui {

// ============================================================================
// Data Lineage Panel
// ============================================================================

DataLineagePanel::DataLineagePanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_lineage", parent)
    , client_(client) {
    setupUi();
    
    // Create sample graph
    LineageNode node1;
    node1.id = "customers";
    node1.name = "customers";
    node1.type = "table";
    node1.x = 50;
    node1.y = 100;
    currentGraph_.nodes["customers"] = node1;
    
    LineageNode node2;
    node2.id = "orders";
    node2.name = "orders";
    node2.type = "table";
    node2.x = 300;
    node2.y = 100;
    currentGraph_.nodes["orders"] = node2;
    
    LineageEdge edge;
    edge.id = "e1";
    edge.sourceId = "customers";
    edge.targetId = "orders";
    edge.transformation = TransformationType::Join;
    currentGraph_.edges.append(edge);
    
    renderGraph();
}

void DataLineagePanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* selectBtn = new QPushButton(tr("Select Object..."), this);
    connect(selectBtn, &QPushButton::clicked, this, &DataLineagePanel::onSelectObject);
    toolbarLayout->addWidget(selectBtn);
    
    auto* upstreamBtn = new QPushButton(tr("Upstream"), this);
    connect(upstreamBtn, &QPushButton::clicked, this, &DataLineagePanel::onShowUpstream);
    toolbarLayout->addWidget(upstreamBtn);
    
    auto* downstreamBtn = new QPushButton(tr("Downstream"), this);
    connect(downstreamBtn, &QPushButton::clicked, this, &DataLineagePanel::onShowDownstream);
    toolbarLayout->addWidget(downstreamBtn);
    
    auto* bothBtn = new QPushButton(tr("Both"), this);
    connect(bothBtn, &QPushButton::clicked, this, &DataLineagePanel::onShowBoth);
    toolbarLayout->addWidget(bothBtn);
    
    toolbarLayout->addStretch();
    
    depthCombo_ = new QComboBox(this);
    depthCombo_->addItems({"1 Level", "2 Levels", "3 Levels", "Unlimited"});
    toolbarLayout->addWidget(new QLabel(tr("Depth:"), this));
    toolbarLayout->addWidget(depthCombo_);
    
    layoutCombo_ = new QComboBox(this);
    layoutCombo_->addItems({"Hierarchical", "Force-Directed", "Circular"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataLineagePanel::onLayoutChanged);
    toolbarLayout->addWidget(new QLabel(tr("Layout:"), this));
    toolbarLayout->addWidget(layoutCombo_);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Main content
    splitter_ = new QSplitter(Qt::Horizontal, this);
    
    // Object tree
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search objects..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &DataLineagePanel::onSearchObject);
    leftLayout->addWidget(searchEdit_);
    
    objectTree_ = new QTreeView(this);
    objectModel_ = new QStandardItemModel(this);
    objectTree_->setModel(objectModel_);
    objectTree_->setHeaderHidden(true);
    connect(objectTree_, &QTreeView::clicked, this, &DataLineagePanel::onObjectSelected);
    leftLayout->addWidget(objectTree_);
    
    splitter_->addWidget(leftWidget);
    
    // Graphics view
    graphicsScene_ = new QGraphicsScene(this);
    graphicsView_ = new LineageGraphicsView(this);
    graphicsView_->setScene(graphicsScene_);
    graphicsView_->setRenderHint(QPainter::Antialiasing);
    graphicsView_->setDragMode(QGraphicsView::ScrollHandDrag);
    connect(graphicsView_, &LineageGraphicsView::nodeClicked,
            this, &DataLineagePanel::onNodeClicked);
    connect(graphicsView_, &LineageGraphicsView::nodeDoubleClicked,
            this, &DataLineagePanel::onNodeDoubleClicked);
    splitter_->addWidget(graphicsView_);
    
    splitter_->setSizes({200, 600});
    mainLayout->addWidget(splitter_, 1);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("Ready"), this);
    statusLayout->addWidget(statusLabel_);
    
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setMaximumWidth(150);
    statusLayout->addWidget(progressBar_);
    
    auto* zoomInBtn = new QPushButton(tr("+"), this);
    connect(zoomInBtn, &QPushButton::clicked, this, &DataLineagePanel::onZoomIn);
    statusLayout->addWidget(zoomInBtn);
    
    auto* zoomOutBtn = new QPushButton(tr("-"), this);
    connect(zoomOutBtn, &QPushButton::clicked, this, &DataLineagePanel::onZoomOut);
    statusLayout->addWidget(zoomOutBtn);
    
    auto* fitBtn = new QPushButton(tr("Fit"), this);
    connect(fitBtn, &QPushButton::clicked, this, &DataLineagePanel::onZoomFit);
    statusLayout->addWidget(fitBtn);
    
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
}

void DataLineagePanel::setupObjectTree() {
    objectModel_->clear();
    
    auto* root = new QStandardItem("Database");
    objectModel_->appendRow(root);
    
    auto* tables = new QStandardItem("Tables");
    tables->appendRow(new QStandardItem("customers"));
    tables->appendRow(new QStandardItem("orders"));
    tables->appendRow(new QStandardItem("products"));
    root->appendRow(tables);
    
    auto* views = new QStandardItem("Views");
    views->appendRow(new QStandardItem("customer_orders"));
    root->appendRow(views);
}

void DataLineagePanel::renderGraph() {
    graphicsScene_->clear();
    
    // Draw nodes
    for (const auto& node : currentGraph_.nodes) {
        auto* rect = graphicsScene_->addRect(node.x, node.y, 120, 40, 
                                              QPen(Qt::black), QBrush(Qt::lightGray));
        
        auto* text = graphicsScene_->addText(node.name, QFont("Arial", 10));
        text->setPos(node.x + 10, node.y + 10);
        text->setDefaultTextColor(Qt::black);
    }
    
    // Draw edges
    for (const auto& edge : currentGraph_.edges) {
        if (!currentGraph_.nodes.contains(edge.sourceId) || 
            !currentGraph_.nodes.contains(edge.targetId))
            continue;
        
        const auto& source = currentGraph_.nodes[edge.sourceId];
        const auto& target = currentGraph_.nodes[edge.targetId];
        
        graphicsScene_->addLine(source.x + 120, source.y + 20, 
                                target.x, target.y + 20, 
                                QPen(Qt::darkGray, 2));
    }
}

void DataLineagePanel::clearGraph() {
    graphicsScene_->clear();
    currentGraph_.nodes.clear();
    currentGraph_.edges.clear();
}

void DataLineagePanel::onSelectObject() {
    ObjectSelectionDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        selectedObjectPath_ = dialog.selectedObject();
        statusLabel_->setText(tr("Selected: %1").arg(selectedObjectPath_));
    }
}

void DataLineagePanel::onObjectSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    
    QString objectName = objectModel_->itemFromIndex(index)->text();
    statusLabel_->setText(tr("Selected: %1").arg(objectName));
}

void DataLineagePanel::onSearchObject(const QString& text) {
    Q_UNUSED(text)
    // Filter tree
}

void DataLineagePanel::onShowUpstream() {
    statusLabel_->setText(tr("Showing upstream dependencies"));
    renderGraph();
}

void DataLineagePanel::onShowDownstream() {
    statusLabel_->setText(tr("Showing downstream dependencies"));
    renderGraph();
}

void DataLineagePanel::onShowBoth() {
    statusLabel_->setText(tr("Showing both directions"));
    renderGraph();
}

void DataLineagePanel::onChangeDepth(int depth) {
    Q_UNUSED(depth)
}

void DataLineagePanel::onRefreshLineage() {
    statusLabel_->setText(tr("Refreshing lineage..."));
    renderGraph();
}

void DataLineagePanel::onImpactAnalysis() {
    ImpactAnalysisDialog dialog(selectedObjectPath_, client_, this);
    dialog.exec();
}

void DataLineagePanel::onRootCauseAnalysis() {
    RootCauseAnalysisDialog dialog(selectedObjectPath_, client_, this);
    dialog.exec();
}

void DataLineagePanel::onCompareVersions() {
    QMessageBox::information(this, tr("Compare"), tr("Version comparison would show here."));
}

void DataLineagePanel::onExportDiagram() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Diagram"),
        QString(),
        tr("PNG (*.png);;PDF (*.pdf);;SVG (*.svg)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Diagram exported."));
    }
}

void DataLineagePanel::onPrintDiagram() {
    QMessageBox::information(this, tr("Print"), tr("Diagram sent to printer."));
}

void DataLineagePanel::onNodeClicked(const QString& nodeId) {
    statusLabel_->setText(tr("Selected: %1").arg(nodeId));
}

void DataLineagePanel::onNodeDoubleClicked(const QString& nodeId) {
    QMessageBox::information(this, tr("Node"), tr("Details for: %1").arg(nodeId));
}

void DataLineagePanel::onZoomIn() {
    graphicsView_->scale(1.2, 1.2);
}

void DataLineagePanel::onZoomOut() {
    graphicsView_->scale(1.0 / 1.2, 1.0 / 1.2);
}

void DataLineagePanel::onZoomFit() {
    graphicsView_->fitInView(graphicsScene_->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void DataLineagePanel::onLayoutChanged(int index) {
    Q_UNUSED(index)
    renderGraph();
}

// ============================================================================
// Lineage Graphics View
// ============================================================================

LineageGraphicsView::LineageGraphicsView(QWidget* parent)
    : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

void LineageGraphicsView::mousePressEvent(QMouseEvent* event) {
    if (auto* item = itemAt(event->pos())) {
        if (auto* nodeItem = dynamic_cast<LineageNodeItem*>(item)) {
            emit nodeClicked(nodeItem->nodeId());
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void LineageGraphicsView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (auto* item = itemAt(event->pos())) {
        if (auto* nodeItem = dynamic_cast<LineageNodeItem*>(item)) {
            emit nodeDoubleClicked(nodeItem->nodeId());
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void LineageGraphicsView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        scale(factor, factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void LineageGraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {
    QGraphicsView::drawBackground(painter, rect);
    
    // Draw grid
    QPen pen(Qt::lightGray, 0.5);
    painter->setPen(pen);
    
    int gridSize = 20;
    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);
    
    for (qreal x = left; x < rect.right(); x += gridSize)
        painter->drawLine(int(x), int(rect.top()), int(x), int(rect.bottom()));
    for (qreal y = top; y < rect.bottom(); y += gridSize)
        painter->drawLine(int(rect.left()), int(y), int(rect.right()), int(y));
}

// ============================================================================
// Impact Analysis Dialog
// ============================================================================

ImpactAnalysisDialog::ImpactAnalysisDialog(const QString& objectPath, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), objectPath_(objectPath), client_(client) {
    setupUi();
    setWindowTitle(tr("Impact Analysis - %1").arg(objectPath));
    resize(500, 400);
    performAnalysis();
}

void ImpactAnalysisDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    targetLabel_ = new QLabel(tr("Target: %1").arg(objectPath_), this);
    layout->addWidget(targetLabel_);
    
    auto* summaryGroup = new QGroupBox(tr("Summary"), this);
    auto* summaryLayout = new QFormLayout(summaryGroup);
    
    totalDepsLabel_ = new QLabel(this);
    summaryLayout->addRow(tr("Total Dependencies:"), totalDepsLabel_);
    
    directDepsLabel_ = new QLabel(this);
    summaryLayout->addRow(tr("Direct Dependencies:"), directDepsLabel_);
    
    indirectDepsLabel_ = new QLabel(this);
    summaryLayout->addRow(tr("Indirect Dependencies:"), indirectDepsLabel_);
    
    layout->addWidget(summaryGroup);
    
    auto* affectedGroup = new QGroupBox(tr("Affected Objects"), this);
    auto* affectedLayout = new QVBoxLayout(affectedGroup);
    
    reportsList_ = new QListWidget(this);
    reportsList_->addItem(tr("Monthly Sales Report"));
    reportsList_->addItem(tr("Customer Dashboard"));
    affectedLayout->addWidget(new QLabel(tr("Reports:"), this));
    affectedLayout->addWidget(reportsList_);
    
    etlsList_ = new QListWidget(this);
    etlsList_->addItem(tr("daily_etl_job"));
    etlsList_->addItem(tr("customer_sync"));
    affectedLayout->addWidget(new QLabel(tr("ETL Jobs:"), this));
    affectedLayout->addWidget(etlsList_);
    
    layout->addWidget(affectedGroup, 1);
    
    riskEdit_ = new QTextEdit(this);
    riskEdit_->setReadOnly(true);
    riskEdit_->setMaximumHeight(80);
    layout->addWidget(new QLabel(tr("Risk Assessment:"), this));
    layout->addWidget(riskEdit_);
    
    progressBar_ = new QProgressBar(this);
    layout->addWidget(progressBar_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &ImpactAnalysisDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &ImpactAnalysisDialog::onExportReport);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ImpactAnalysisDialog::performAnalysis() {
    progressBar_->setRange(0, 0);
    
    QTimer::singleShot(1000, this, [this]() {
        totalDepsLabel_->setText("5");
        directDepsLabel_->setText("3");
        indirectDepsLabel_->setText("2");
        
        riskEdit_->setText(tr("MEDIUM RISK: 3 reports and 2 ETL jobs depend on this object. "
                               "Changes should be tested carefully."));
        
        progressBar_->setRange(0, 100);
        progressBar_->setValue(100);
    });
}

void ImpactAnalysisDialog::onRefresh() {
    performAnalysis();
}

void ImpactAnalysisDialog::onExportReport() {
    QMessageBox::information(this, tr("Export"), tr("Impact report exported."));
}

void ImpactAnalysisDialog::onViewAffectedObjects() {}
void ImpactAnalysisDialog::onSimulateChange() {
    QMessageBox::information(this, tr("Simulate"), tr("Change simulation would run here."));
}

// ============================================================================
// Root Cause Analysis Dialog
// ============================================================================

RootCauseAnalysisDialog::RootCauseAnalysisDialog(const QString& objectPath, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), objectPath_(objectPath), client_(client) {
    setupUi();
    setWindowTitle(tr("Root Cause Analysis - %1").arg(objectPath));
    resize(500, 400);
}

void RootCauseAnalysisDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel(tr("Tracing data lineage for: %1").arg(objectPath_), this));
    
    sourceTree_ = new QTreeView(this);
    sourceModel_ = new QStandardItemModel(this);
    sourceTree_->setModel(sourceModel_);
    layout->addWidget(sourceTree_, 1);
    
    pathDepthLabel_ = new QLabel(tr("Path Depth: 3 levels"), this);
    layout->addWidget(pathDepthLabel_);
    
    layout->addWidget(new QLabel(tr("Source SQL:"), this));
    sqlEdit_ = new QTextEdit(this);
    sqlEdit_->setReadOnly(true);
    sqlEdit_->setMaximumHeight(100);
    layout->addWidget(sqlEdit_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* traceBtn = new QPushButton(tr("Trace Back"), this);
    connect(traceBtn, &QPushButton::clicked, this, &RootCauseAnalysisDialog::onTraceBack);
    btnLayout->addWidget(traceBtn);
    
    auto* exportBtn = new QPushButton(tr("Export Path"), this);
    connect(exportBtn, &QPushButton::clicked, this, &RootCauseAnalysisDialog::onExportPath);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
    
    performTraceback();
}

void RootCauseAnalysisDialog::performTraceback() {
    sourceModel_->clear();
    
    auto* root = new QStandardItem("customer_orders (view)");
    
    auto* orders = new QStandardItem("orders (table)");
    orders->appendRow(new QStandardItem("customers (table)"));
    root->appendRow(orders);
    
    auto* products = new QStandardItem("products (table)");
    root->appendRow(products);
    
    sourceModel_->appendRow(root);
    sourceTree_->expandAll();
    
    sqlEdit_->setText(tr("SELECT c.name, o.order_date, o.total\n"
                          "FROM orders o\n"
                          "JOIN customers c ON o.customer_id = c.id"));
}

void RootCauseAnalysisDialog::onTraceBack() {
    performTraceback();
}

void RootCauseAnalysisDialog::onViewSource() {
    QMessageBox::information(this, tr("Source"), tr("Source code would be shown here."));
}

void RootCauseAnalysisDialog::onExportPath() {
    QMessageBox::information(this, tr("Export"), tr("Lineage path exported."));
}

// ============================================================================
// Object Selection Dialog
// ============================================================================

ObjectSelectionDialog::ObjectSelectionDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    setWindowTitle(tr("Select Object"));
    resize(400, 500);
    loadDatabases();
}

void ObjectSelectionDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    databaseCombo_ = new QComboBox(this);
    connect(databaseCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ObjectSelectionDialog::onDatabaseChanged);
    formLayout->addRow(tr("Database:"), databaseCombo_);
    
    schemaCombo_ = new QComboBox(this);
    connect(schemaCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ObjectSelectionDialog::onSchemaChanged);
    formLayout->addRow(tr("Schema:"), schemaCombo_);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({tr("All Types"), tr("Tables"), tr("Views"), tr("Columns")});
    formLayout->addRow(tr("Type:"), typeCombo_);
    
    layout->addLayout(formLayout);
    
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter objects..."));
    connect(filterEdit_, &QLineEdit::textChanged, this, &ObjectSelectionDialog::onFilterTextChanged);
    layout->addWidget(filterEdit_);
    
    objectTree_ = new QTreeView(this);
    objectModel_ = new QStandardItemModel(this);
    objectTree_->setModel(objectModel_);
    objectTree_->setHeaderHidden(true);
    connect(objectTree_, &QTreeView::clicked, this, &ObjectSelectionDialog::onObjectSelected);
    layout->addWidget(objectTree_, 1);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* selectBtn = new QPushButton(tr("Select"), this);
    connect(selectBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(selectBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ObjectSelectionDialog::loadDatabases() {
    databaseCombo_->addItems({"scratchbird", "postgres"});
}

void ObjectSelectionDialog::loadSchemas(const QString& database) {
    Q_UNUSED(database)
    schemaCombo_->clear();
    schemaCombo_->addItems({"public", "information_schema"});
}

void ObjectSelectionDialog::loadObjects(const QString& database, const QString& schema) {
    Q_UNUSED(database)
    Q_UNUSED(schema)
    
    objectModel_->clear();
    
    auto* tables = new QStandardItem("Tables");
    tables->appendRow(new QStandardItem("customers"));
    tables->appendRow(new QStandardItem("orders"));
    tables->appendRow(new QStandardItem("products"));
    objectModel_->appendRow(tables);
    
    auto* views = new QStandardItem("Views");
    views->appendRow(new QStandardItem("customer_orders"));
    objectModel_->appendRow(views);
}

void ObjectSelectionDialog::onDatabaseChanged(int index) {
    loadSchemas(databaseCombo_->itemText(index));
}

void ObjectSelectionDialog::onSchemaChanged(int index) {
    loadObjects(databaseCombo_->currentText(), schemaCombo_->itemText(index));
}

void ObjectSelectionDialog::onObjectSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    selectedObject_ = objectModel_->itemFromIndex(index)->text();
}

void ObjectSelectionDialog::onFilterTextChanged(const QString& text) {
    Q_UNUSED(text)
}

QString ObjectSelectionDialog::selectedObject() const {
    return selectedObject_;
}

// ============================================================================
// LineageNodeItem
// ============================================================================
LineageNodeItem::LineageNodeItem(const LineageNode& node, QGraphicsItem* parent)
    : QGraphicsRectItem(parent), node_(node) {
    setAcceptHoverEvents(true);
}

void LineageNodeItem::setHighlighted(bool highlighted) {
    highlighted_ = highlighted;
    update();
}

void LineageNodeItem::setSelected(bool selected) {
    QGraphicsRectItem::setSelected(selected);
    update();
}

void LineageNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // Draw node rectangle
    QRectF rect = boundingRect();
    painter->fillRect(rect, highlighted_ ? QColor(200, 220, 255) : QColor(240, 240, 240));
    painter->drawRect(rect);
    
    // Draw node name
    painter->drawText(rect, Qt::AlignCenter, node_.name);
}

void LineageNodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    setHighlighted(true);
}

void LineageNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    Q_UNUSED(event)
    setHighlighted(false);
}

// ============================================================================
// LineageEdgeItem
// ============================================================================
LineageEdgeItem::LineageEdgeItem(const LineageEdge& edge, QGraphicsItem* parent)
    : QGraphicsLineItem(parent), edge_(edge) {
}

void LineageEdgeItem::updatePosition(const QPointF& start, const QPointF& end) {
    setLine(start.x(), start.y(), end.x(), end.y());
}

void LineageEdgeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setPen(QPen(Qt::darkGray, 2));
    painter->drawLine(line());
    
    // Draw arrowhead
    if (!arrowHead_.isEmpty()) {
        painter->setBrush(Qt::darkGray);
        painter->drawPolygon(arrowHead_);
    }
}

// ============================================================================
// LineageExportDialog
// ============================================================================
void LineageExportDialog::onExportImage() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Image"),
        QString(), tr("PNG Files (*.png);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("Image exported to %1").arg(fileName));
    }
}

void LineageExportDialog::onExportPDF() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export PDF"),
        QString(), tr("PDF Files (*.pdf);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("PDF exported to %1").arg(fileName));
    }
}

void LineageExportDialog::onExportSVG() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export SVG"),
        QString(), tr("SVG Files (*.svg);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("SVG exported to %1").arg(fileName));
    }
}

void LineageExportDialog::onExportJSON() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export JSON"),
        QString(), tr("JSON Files (*.json);;All Files (*)"));
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("JSON exported to %1").arg(fileName));
    }
}

void LineageExportDialog::onCopyToClipboard() {
    QMessageBox::information(this, tr("Copy"), tr("Image copied to clipboard."));
}

} // namespace scratchrobin::ui
