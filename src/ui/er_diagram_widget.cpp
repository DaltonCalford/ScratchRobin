#include "ui/er_diagram_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPainter>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QPrinter>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>
#include <QStyle>
#include <cmath>

namespace scratchrobin::ui {

// TableGraphicsItem implementation
TableGraphicsItem::TableGraphicsItem(const TableNode& table, QGraphicsItem* parent)
    : QGraphicsItem(parent), table_(table) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    // Calculate size based on columns
    qreal width = 150;
    qreal height = headerHeight_ + (table_.columns.size() * rowHeight_) + padding_;
    table_.size = QSizeF(width, height);
}

QRectF TableGraphicsItem::boundingRect() const {
    return QRectF(-2, -2, table_.size.width() + 4, table_.size.height() + 4);
}

void TableGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // Draw shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 30));
    painter->drawRoundedRect(3, 3, table_.size.width(), table_.size.height(), 4, 4);
    
    // Draw table body
    painter->setPen(QPen(Qt::darkGray, 1));
    painter->setBrush(Qt::white);
    painter->drawRoundedRect(0, 0, table_.size.width(), table_.size.height(), 4, 4);
    
    // Draw header
    QColor headerColor = isSelected() ? QColor(0, 120, 215) : QColor(70, 130, 180);
    painter->setBrush(headerColor);
    painter->drawRoundedRect(0, 0, table_.size.width(), headerHeight_, 4, 4);
    painter->setPen(QPen(headerColor.darker(), 1));
    painter->drawLine(0, headerHeight_, table_.size.width(), headerHeight_);
    
    // Draw table name
    painter->setPen(Qt::white);
    painter->setFont(QFont("Arial", 10, QFont::Bold));
    painter->drawText(QRectF(padding_, 2, table_.size.width() - padding_ * 2, headerHeight_ - 4), 
                      Qt::AlignCenter, table_.name);
    
    // Draw columns
    painter->setPen(Qt::black);
    painter->setFont(QFont("Consolas", 9));
    for (int i = 0; i < table_.columns.size(); ++i) {
        qreal y = headerHeight_ + (i * rowHeight_);
        painter->drawText(QRectF(padding_, y, table_.size.width() - padding_ * 2, rowHeight_), 
                          Qt::AlignVCenter | Qt::AlignLeft, table_.columns[i]);
    }
    
    // Draw border if selected
    if (isSelected()) {
        painter->setPen(QPen(QColor(0, 120, 215), 2, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(0, 0, table_.size.width(), table_.size.height(), 4, 4);
    }
}

QPointF TableGraphicsItem::connectionPoint(int columnIndex) const {
    qreal x = (columnIndex < 0) ? 0 : table_.size.width();
    qreal y = headerHeight_ + (std::abs(columnIndex) * rowHeight_) + (rowHeight_ / 2);
    return QPointF(x, y);
}

// RelationshipGraphicsItem implementation
RelationshipGraphicsItem::RelationshipGraphicsItem(TableGraphicsItem* from, int fromCol,
                                                   TableGraphicsItem* to, int toCol,
                                                   const QString& type,
                                                   QGraphicsItem* parent)
    : QGraphicsItem(parent), fromItem_(from), toItem_(to), 
      fromColumn_(fromCol), toColumn_(toCol), type_(type) {
    setZValue(-1); // Draw behind tables
}

QRectF RelationshipGraphicsItem::boundingRect() const {
    if (!fromItem_ || !toItem_) return QRectF();
    
    QPointF start = fromItem_->mapToScene(fromItem_->connectionPoint(fromColumn_));
    QPointF end = toItem_->mapToScene(toItem_->connectionPoint(toColumn_));
    
    qreal x = std::min(start.x(), end.x()) - 20;
    qreal y = std::min(start.y(), end.y()) - 20;
    qreal w = std::abs(end.x() - start.x()) + 40;
    qreal h = std::abs(end.y() - start.y()) + 40;
    
    return QRectF(x, y, w, h);
}

void RelationshipGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    if (!fromItem_ || !toItem_) return;
    
    QPointF start = fromItem_->mapToScene(fromItem_->connectionPoint(fromColumn_)) - mapToScene(0, 0);
    QPointF end = toItem_->mapToScene(toItem_->connectionPoint(toColumn_)) - mapToScene(0, 0);
    
    // Determine color based on relationship type
    QColor color = Qt::darkGray;
    if (type_ == "1:N") color = QColor(0, 150, 0);
    else if (type_ == "N:1") color = QColor(150, 0, 0);
    else if (type_ == "N:M") color = QColor(150, 0, 150);
    
    painter->setPen(QPen(color, 1.5));
    
    // Draw line with slight curve
    QPainterPath path;
    path.moveTo(start);
    
    // Calculate control points for bezier curve
    qreal midX = (start.x() + end.x()) / 2;
    QPointF ctrl1(midX, start.y());
    QPointF ctrl2(midX, end.y());
    
    path.cubicTo(ctrl1, ctrl2, end);
    painter->drawPath(path);
    
    // Draw relationship indicators
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    
    // Draw diamond for N:M, circles for 1:N
    if (type_ == "N:M") {
        QPointF mid = path.pointAtPercent(0.5);
        QPointF diamond[4] = {
            QPointF(mid.x(), mid.y() - 6),
            QPointF(mid.x() + 6, mid.y()),
            QPointF(mid.x(), mid.y() + 6),
            QPointF(mid.x() - 6, mid.y())
        };
        painter->drawPolygon(diamond, 4);
    } else if (type_ == "1:N") {
        QPointF mid = path.pointAtPercent(0.7);
        painter->drawEllipse(mid, 4, 4);
    }
}

// ERDiagramWidget implementation
ERDiagramWidget::ERDiagramWidget(QWidget* parent)
    : QWidget(parent), currentZoom_(1.0) {
    setupUi();
    createSampleDiagram();
}

ERDiagramWidget::~ERDiagramWidget() = default;

void ERDiagramWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbar
    auto* toolbar = new QHBoxLayout();
    
    toolbar->addWidget(new QLabel(tr("Schema:")));
    schema_combo_ = new QComboBox(this);
    schema_combo_->addItem(tr("(All Schemas)"));
    schema_combo_->addItem("public");
    schema_combo_->addItem("scratchbird");
    toolbar->addWidget(schema_combo_);
    
    toolbar->addSpacing(10);
    
    filter_edit_ = new QLineEdit(this);
    filter_edit_->setPlaceholderText(tr("Filter tables..."));
    toolbar->addWidget(filter_edit_);
    
    toolbar->addStretch();
    
    refresh_btn_ = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refresh_btn_);
    
    layout_btn_ = new QPushButton(tr("Auto Layout"), this);
    toolbar->addWidget(layout_btn_);
    
    zoom_in_btn_ = new QPushButton(tr("+"), this);
    zoom_in_btn_->setMaximumWidth(30);
    toolbar->addWidget(zoom_in_btn_);
    
    zoom_out_btn_ = new QPushButton(tr("-"), this);
    zoom_out_btn_->setMaximumWidth(30);
    toolbar->addWidget(zoom_out_btn_);
    
    zoom_fit_btn_ = new QPushButton(tr("Fit"), this);
    toolbar->addWidget(zoom_fit_btn_);
    
    export_btn_ = new QPushButton(tr("Export"), this);
    toolbar->addWidget(export_btn_);
    
    print_btn_ = new QPushButton(tr("Print"), this);
    toolbar->addWidget(print_btn_);
    
    show_all_check_ = new QCheckBox(tr("Show all tables"), this);
    show_all_check_->setChecked(true);
    toolbar->addWidget(show_all_check_);
    
    layout->addLayout(toolbar);
    
    // Graphics view
    scene_ = new QGraphicsScene(this);
    view_ = new QGraphicsView(scene_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setDragMode(QGraphicsView::RubberBandDrag);
    view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view_->setBackgroundBrush(QColor(245, 245, 245));
    view_->setFrameShape(QFrame::NoFrame);
    layout->addWidget(view_);
    
    // Connections
    connect(zoom_in_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onZoomIn);
    connect(zoom_out_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onZoomOut);
    connect(zoom_fit_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onZoomFit);
    connect(refresh_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onRefresh);
    connect(layout_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onAutoLayout);
    connect(export_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onExport);
    connect(print_btn_, &QPushButton::clicked, this, &ERDiagramWidget::onPrint);
    connect(schema_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int) { onSchemaChanged(schema_combo_->currentText()); });
    connect(show_all_check_, &QCheckBox::toggled, this, &ERDiagramWidget::onShowAllTables);
}

void ERDiagramWidget::createSampleDiagram() {
    // Sample tables
    TableNode users;
    users.name = "users";
    users.columns = {"id PK", "username", "email", "created_at"};
    users.position = QPointF(50, 50);
    
    TableNode orders;
    orders.name = "orders";
    orders.columns = {"id PK", "user_id FK", "total", "status", "created_at"};
    orders.position = QPointF(300, 50);
    
    TableNode products;
    products.name = "products";
    products.columns = {"id PK", "name", "price", "stock"};
    products.position = QPointF(550, 50);
    
    TableNode order_items;
    order_items.name = "order_items";
    order_items.columns = {"id PK", "order_id FK", "product_id FK", "quantity", "price"};
    order_items.position = QPointF(300, 250);
    
    TableNode categories;
    categories.name = "categories";
    categories.columns = {"id PK", "name", "parent_id FK"};
    categories.position = QPointF(550, 250);
    
    addTable(users);
    addTable(orders);
    addTable(products);
    addTable(order_items);
    addTable(categories);
    
    // Sample relationships
    Relationship rel1{"users", "id", "orders", "user_id", "1:N"};
    Relationship rel2{"orders", "id", "order_items", "order_id", "1:N"};
    Relationship rel3{"products", "id", "order_items", "product_id", "1:N"};
    Relationship rel4{"categories", "id", "categories", "parent_id", "1:N"};
    
    addRelationship(rel1);
    addRelationship(rel2);
    addRelationship(rel3);
    addRelationship(rel4);
}

void ERDiagramWidget::loadSchema(const QString& schema) {
    // TODO: Load tables from database schema
    clear();
    createSampleDiagram();
}

void ERDiagramWidget::addTable(const TableNode& table) {
    auto* item = new TableGraphicsItem(table);
    item->setPos(table.position);
    scene_->addItem(item);
    tableItems_.append(item);
}

void ERDiagramWidget::addRelationship(const Relationship& rel) {
    TableGraphicsItem* fromItem = nullptr;
    TableGraphicsItem* toItem = nullptr;
    
    for (auto* item : tableItems_) {
        if (item->tableName() == rel.fromTable) fromItem = item;
        if (item->tableName() == rel.toTable) toItem = item;
    }
    
    if (fromItem && toItem) {
        // Find column indices (simplified - assumes column 1 is FK)
        int fromCol = (rel.type == "1:N") ? 1 : -1;
        int toCol = (rel.type == "1:N") ? 1 : -1;
        
        auto* relItem = new RelationshipGraphicsItem(fromItem, fromCol, toItem, toCol, rel.type);
        scene_->addItem(relItem);
        relationshipItems_.append(relItem);
    }
}

void ERDiagramWidget::clear() {
    for (auto* item : tableItems_) {
        scene_->removeItem(item);
        delete item;
    }
    tableItems_.clear();
    
    for (auto* item : relationshipItems_) {
        scene_->removeItem(item);
        delete item;
    }
    relationshipItems_.clear();
}

void ERDiagramWidget::autoLayout() {
    // Simple grid layout
    int cols = 3;
    qreal spacing = 50;
    qreal x = spacing;
    qreal y = spacing;
    int col = 0;
    
    for (auto* item : tableItems_) {
        item->setPos(x, y);
        
        col++;
        if (col >= cols) {
            col = 0;
            x = spacing;
            y += 200;
        } else {
            x += 250;
        }
    }
    
    scene_->update();
}

void ERDiagramWidget::exportToImage(const QString& filename) {
    QImage image(scene_->sceneRect().size().toSize() * 2, QImage::Format_ARGB32);
    image.fill(Qt::white);
    
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(2, 2);
    scene_->render(&painter);
    painter.end();
    
    image.save(filename);
}

void ERDiagramWidget::exportToSvg(const QString& filename) {
    // SVG export requires Qt6::Svg module which may not be available
    // For now, export as high-resolution PNG instead
    QString pngFilename = filename;
    pngFilename.replace(".svg", ".png");
    exportToImage(pngFilename);
}

void ERDiagramWidget::print() {
    QPrinter printer(QPrinter::HighResolution);
    // Printer dialog would be shown here
    
    QPainter painter(&printer);
    scene_->render(&painter);
    painter.end();
}

void ERDiagramWidget::onZoomIn() {
    currentZoom_ *= 1.25;
    view_->setTransform(QTransform::fromScale(currentZoom_, currentZoom_));
}

void ERDiagramWidget::onZoomOut() {
    currentZoom_ /= 1.25;
    view_->setTransform(QTransform::fromScale(currentZoom_, currentZoom_));
}

void ERDiagramWidget::onZoomFit() {
    view_->fitInView(scene_->itemsBoundingRect(), Qt::KeepAspectRatio);
    currentZoom_ = view_->transform().m11();
}

void ERDiagramWidget::onRefresh() {
    loadSchema(schema_combo_->currentText());
}

void ERDiagramWidget::onAutoLayout() {
    autoLayout();
}

void ERDiagramWidget::onExport() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Export ER Diagram"),
                                                    QString(),
                                                    tr("PNG Image (*.png);;SVG Image (*.svg);;PDF Document (*.pdf)"));
    if (filename.isEmpty()) return;
    
    if (filename.endsWith(".svg")) {
        exportToSvg(filename);
    } else if (filename.endsWith(".pdf")) {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        QPainter painter(&printer);
        scene_->render(&painter);
    } else {
        exportToImage(filename);
    }
}

void ERDiagramWidget::onPrint() {
    print();
}

void ERDiagramWidget::onSchemaChanged(const QString& schema) {
    loadSchema(schema);
}

void ERDiagramWidget::onShowAllTables(bool show) {
    // TODO: Filter based on check state
    for (auto* item : tableItems_) {
        item->setVisible(show);
    }
    scene_->update();
}

} // namespace scratchrobin::ui
