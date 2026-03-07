#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <memory>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct TableNode {
    QString name;
    QStringList columns;
    QList<std::pair<QString, QString>> foreignKeys; // (column, refTable)
    QPointF position;
    QSizeF size;
};

struct Relationship {
    QString fromTable;
    QString fromColumn;
    QString toTable;
    QString toColumn;
    QString type; // "1:1", "1:N", "N:M"
};

class TableGraphicsItem : public QGraphicsItem {
public:
    TableGraphicsItem(const TableNode& table, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    QString tableName() const { return table_.name; }
    QPointF connectionPoint(int columnIndex) const;
    
private:
    TableNode table_;
    qreal headerHeight_ = 25;
    qreal rowHeight_ = 18;
    qreal padding_ = 8;
};

class RelationshipGraphicsItem : public QGraphicsItem {
public:
    RelationshipGraphicsItem(TableGraphicsItem* from, int fromCol, 
                             TableGraphicsItem* to, int toCol,
                             const QString& type,
                             QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
private:
    TableGraphicsItem* fromItem_;
    TableGraphicsItem* toItem_;
    int fromColumn_;
    int toColumn_;
    QString type_;
};

class ERDiagramWidget : public QWidget {
    Q_OBJECT

public:
    explicit ERDiagramWidget(QWidget* parent = nullptr);
    ~ERDiagramWidget() override;

    void loadSchema(const QString& schema);
    void addTable(const TableNode& table);
    void addRelationship(const Relationship& rel);
    void clear();

    void autoLayout();
    void exportToImage(const QString& filename);
    void exportToSvg(const QString& filename);  // Falls back to PNG if QtSvg not available
    void print();

public slots:
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onRefresh();
    void onAutoLayout();
    void onExport();
    void onPrint();
    void onSchemaChanged(const QString& schema);
    void onShowAllTables(bool show);

private:
    void setupUi();
    void createSampleDiagram();
    
    QGraphicsView* view_;
    QGraphicsScene* scene_;
    
    QComboBox* schema_combo_;
    QLineEdit* filter_edit_;
    QPushButton* refresh_btn_;
    QPushButton* layout_btn_;
    QPushButton* zoom_in_btn_;
    QPushButton* zoom_out_btn_;
    QPushButton* zoom_fit_btn_;
    QPushButton* export_btn_;
    QPushButton* print_btn_;
    QCheckBox* show_all_check_;
    
    QList<TableGraphicsItem*> tableItems_;
    QList<RelationshipGraphicsItem*> relationshipItems_;
    
    qreal currentZoom_ = 1.0;
};

} // namespace scratchrobin::ui
