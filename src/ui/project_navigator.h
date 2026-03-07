#pragma once
#include <QTreeWidget>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QToolButton;
class QMenu;
QT_END_NAMESPACE

namespace scratchrobin::ui {

// Custom tree widget to support drag
class DraggableTreeWidget : public QTreeWidget {
  Q_OBJECT
 public:
  explicit DraggableTreeWidget(QWidget* parent = nullptr);

 signals:
  void itemDragStarted(const QString& text);

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void startDrag();

 private:
  QPoint drag_start_pos_;
  QTreeWidgetItem* drag_item_ = nullptr;
};

class ProjectNavigator : public QWidget {
  Q_OBJECT

 public:
  explicit ProjectNavigator(QWidget* parent = nullptr);
  ~ProjectNavigator() override;

  void refresh();
  void clear();
  void addServer(const QString& serverName, const QString& host = QString());
  void addDatabase(const QString& serverName, const QString& dbName);
  void addTable(const QString& serverName, const QString& dbName, const QString& tableName);

 signals:
  void serverSelected(const QString& serverName);
  void databaseSelected(const QString& serverName, const QString& dbName);
  void tableSelected(const QString& serverName, const QString& dbName, const QString& tableName);
  void tableDoubleClicked(const QString& serverName, const QString& dbName, const QString& tableName);
  
  // Context menu actions
  void queryTableRequested(const QString& serverName, const QString& dbName, const QString& tableName);
  void editTableRequested(const QString& serverName, const QString& dbName, const QString& tableName);
  void dropTableRequested(const QString& serverName, const QString& dbName, const QString& tableName);
  void copyNameRequested(const QString& name);
  
  // Drag drop
  void itemDragged(const QString& text);

 private slots:
  void onFilterTextChanged(const QString& text);
  void onExpandAll();
  void onCollapseAll();
  void onRefresh();
  void onItemDoubleClicked(QTreeWidgetItem* item, int column);
  void onContextMenu(const QPoint& pos);
  void onItemDragged(const QString& text);
  
  // Context menu handlers
  void onQueryTable();
  void onEditTable();
  void onDropTable();
  void onCopyName();
  void onRefreshItem();
  void onNewConnection();
  void onDisconnect();
  bool filterItem(QTreeWidgetItem* item, const QString& text);

 private:
  void setupUi();
  void createNodes();
  void createContextMenus();
  QTreeWidgetItem* findServerItem(const QString& serverName);
  QTreeWidgetItem* findOrCreateCategoryItem(QTreeWidgetItem* parent, const QString& category);
  QString getItemType(QTreeWidgetItem* item) const;
  QString getItemPath(QTreeWidgetItem* item) const;
  QTreeWidgetItem* context_item_ = nullptr;

  QLineEdit* filter_edit_;
  DraggableTreeWidget* tree_widget_;
  QToolButton* expand_btn_;
  QToolButton* collapse_btn_;
  QToolButton* refresh_btn_;
  
  // Context menus
  QMenu* server_menu_;
  QMenu* database_menu_;
  QMenu* table_menu_;
  QMenu* column_menu_;
  QMenu* general_menu_;
};

}  // namespace scratchrobin::ui
