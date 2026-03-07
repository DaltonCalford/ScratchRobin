#include "ui/project_navigator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>

namespace scratchrobin::ui {

// DraggableTreeWidget implementation
DraggableTreeWidget::DraggableTreeWidget(QWidget* parent) 
    : QTreeWidget(parent), drag_item_(nullptr) {
  setDragEnabled(true);
}

void DraggableTreeWidget::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    drag_start_pos_ = event->pos();
    drag_item_ = itemAt(event->pos());
  }
  QTreeWidget::mousePressEvent(event);
}

void DraggableTreeWidget::mouseMoveEvent(QMouseEvent* event) {
  if (!(event->buttons() & Qt::LeftButton)) {
    QTreeWidget::mouseMoveEvent(event);
    return;
  }
  
  if ((event->pos() - drag_start_pos_).manhattanLength() < QApplication::startDragDistance()) {
    QTreeWidget::mouseMoveEvent(event);
    return;
  }
  
  if (drag_item_) {
    startDrag();
  }
}

void DraggableTreeWidget::startDrag() {
  if (!drag_item_) return;
  
  QString text = drag_item_->text(0);
  // For tables, include fully qualified name
  if (drag_item_->parent() && drag_item_->parent()->text(0) == "Tables") {
    QTreeWidgetItem* dbItem = drag_item_->parent()->parent();
    QTreeWidgetItem* serverItem = dbItem ? dbItem->parent() : nullptr;
    if (serverItem) {
      text = dbItem->text(0) + "." + text;
    }
  }
  
  emit itemDragStarted(text);
  
  auto* drag = new QDrag(this);
  auto* mimeData = new QMimeData;
  mimeData->setText(text);
  drag->setMimeData(mimeData);
  
  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  Q_UNUSED(dropAction)
  
  drag_item_ = nullptr;
}

// ProjectNavigator implementation
ProjectNavigator::ProjectNavigator(QWidget* parent)
    : QWidget(parent)
    , filter_edit_(nullptr)
    , tree_widget_(nullptr)
    , expand_btn_(nullptr)
    , collapse_btn_(nullptr)
    , refresh_btn_(nullptr)
    , server_menu_(nullptr)
    , database_menu_(nullptr)
    , table_menu_(nullptr)
    , column_menu_(nullptr)
    , general_menu_(nullptr)
    , context_item_(nullptr) {
  setupUi();
  createNodes();
  createContextMenus();
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::setupUi() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  // Filter bar
  auto* filter_layout = new QHBoxLayout();
  
  filter_edit_ = new QLineEdit(this);
  filter_edit_->setPlaceholderText(tr("Filter..."));
  filter_edit_->setClearButtonEnabled(true);
  filter_layout->addWidget(filter_edit_, 1);
  
  expand_btn_ = new QToolButton(this);
  expand_btn_->setText(tr("+"));
  expand_btn_->setToolTip(tr("Expand All"));
  filter_layout->addWidget(expand_btn_);
  
  collapse_btn_ = new QToolButton(this);
  collapse_btn_->setText(tr("-"));
  collapse_btn_->setToolTip(tr("Collapse All"));
  filter_layout->addWidget(collapse_btn_);
  
  refresh_btn_ = new QToolButton(this);
  refresh_btn_->setText(tr("↻"));
  refresh_btn_->setToolTip(tr("Refresh"));
  filter_layout->addWidget(refresh_btn_);
  
  layout->addLayout(filter_layout);

  // Tree widget
  tree_widget_ = new DraggableTreeWidget(this);
  tree_widget_->setHeaderHidden(true);
  tree_widget_->setColumnCount(1);
  tree_widget_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  tree_widget_->setAlternatingRowColors(true);
  tree_widget_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_widget_->setAcceptDrops(false);
  
  layout->addWidget(tree_widget_);

  // Connections
  connect(filter_edit_, &QLineEdit::textChanged,
          this, &ProjectNavigator::onFilterTextChanged);
  connect(expand_btn_, &QToolButton::clicked,
          this, &ProjectNavigator::onExpandAll);
  connect(collapse_btn_, &QToolButton::clicked,
          this, &ProjectNavigator::onCollapseAll);
  connect(refresh_btn_, &QToolButton::clicked,
          this, &ProjectNavigator::onRefresh);
  connect(tree_widget_, &QTreeWidget::itemDoubleClicked,
          this, &ProjectNavigator::onItemDoubleClicked);
  connect(tree_widget_, &QTreeWidget::customContextMenuRequested,
          this, &ProjectNavigator::onContextMenu);
  connect(tree_widget_, &DraggableTreeWidget::itemDragStarted,
          this, &ProjectNavigator::onItemDragged);
}

void ProjectNavigator::createNodes() {
  tree_widget_->clear();
  
  // Add some sample structure
  addServer("localhost", "127.0.0.1:3050");
  addDatabase("localhost", "scratchbird");
  addTable("localhost", "scratchbird", "users");
  addTable("localhost", "scratchbird", "products");
  addTable("localhost", "scratchbird", "orders");
  
  addDatabase("localhost", "testdb");
  addTable("localhost", "testdb", "test_table1");
  addTable("localhost", "testdb", "test_table2");
}

void ProjectNavigator::createContextMenus() {
  // Server menu
  server_menu_ = new QMenu(this);
  server_menu_->addAction(tr("New Connection..."), this, &ProjectNavigator::onNewConnection);
  server_menu_->addAction(tr("Disconnect"), this, &ProjectNavigator::onDisconnect);
  server_menu_->addSeparator();
  server_menu_->addAction(tr("New Database..."), this, [this]() {
    if (context_item_) emit serverSelected(context_item_->text(0));
  });
  server_menu_->addAction(tr("Refresh"), this, &ProjectNavigator::onRefreshItem);
  server_menu_->addSeparator();
  server_menu_->addAction(tr("Properties"), this, [this]() {
    if (context_item_) emit serverSelected(context_item_->text(0));
  });
  
  // Database menu
  database_menu_ = new QMenu(this);
  database_menu_->addAction(tr("New Table..."), this, [this]() {
    if (context_item_ && context_item_->parent()) {
      emit databaseSelected(context_item_->parent()->text(0), context_item_->text(0));
    }
  });
  database_menu_->addAction(tr("Query Database"), this, [this]() {
    if (context_item_ && context_item_->parent()) {
      emit databaseSelected(context_item_->parent()->text(0), context_item_->text(0));
    }
  });
  database_menu_->addSeparator();
  database_menu_->addAction(tr("Backup..."), this, [this]() {
    if (context_item_ && context_item_->parent()) {
      emit databaseSelected(context_item_->parent()->text(0), context_item_->text(0));
    }
  });
  database_menu_->addAction(tr("Restore..."), this, [this]() {
    if (context_item_ && context_item_->parent()) {
      emit databaseSelected(context_item_->parent()->text(0), context_item_->text(0));
    }
  });
  database_menu_->addSeparator();
  database_menu_->addAction(tr("Refresh"), this, &ProjectNavigator::onRefreshItem);
  database_menu_->addSeparator();
  database_menu_->addAction(tr("Copy Name"), this, &ProjectNavigator::onCopyName);
  
  // Table menu
  table_menu_ = new QMenu(this);
  table_menu_->addAction(tr("Query Table (SELECT *)"), this, &ProjectNavigator::onQueryTable);
  table_menu_->addAction(tr("Edit Table Data"), this, &ProjectNavigator::onEditTable);
  table_menu_->addSeparator();
  table_menu_->addAction(tr("Design Table"), this, [this]() {
    if (context_item_ && context_item_->parent() && context_item_->parent()->parent()) {
      QTreeWidgetItem* dbItem = context_item_->parent()->parent();
      QTreeWidgetItem* serverItem = dbItem->parent();
      emit editTableRequested(serverItem->text(0), dbItem->text(0), context_item_->text(0));
    }
  });
  table_menu_->addAction(tr("View Indexes"), this, [this]() {
    if (context_item_) emit tableSelected(QString(), QString(), context_item_->text(0));
  });
  table_menu_->addSeparator();
  table_menu_->addAction(tr("Truncate..."), this, [this]() {
    if (context_item_) emit tableSelected(QString(), QString(), context_item_->text(0));
  });
  table_menu_->addAction(tr("Drop..."), this, &ProjectNavigator::onDropTable);
  table_menu_->addSeparator();
  table_menu_->addAction(tr("Copy Name"), this, &ProjectNavigator::onCopyName);
  table_menu_->addAction(tr("Copy Qualified Name"), this, [this]() {
    if (context_item_ && context_item_->parent() && context_item_->parent()->parent()) {
      QTreeWidgetItem* dbItem = context_item_->parent()->parent();
      QString qualified = dbItem->text(0) + "." + context_item_->text(0);
      QApplication::clipboard()->setText(qualified);
      emit copyNameRequested(qualified);
    }
  });
  
  // Column menu
  column_menu_ = new QMenu(this);
  column_menu_->addAction(tr("Copy Name"), this, &ProjectNavigator::onCopyName);
  
  // General menu (when clicking on empty space)
  general_menu_ = new QMenu(this);
  general_menu_->addAction(tr("New Connection..."), this, &ProjectNavigator::onNewConnection);
  general_menu_->addSeparator();
  general_menu_->addAction(tr("Refresh All"), this, &ProjectNavigator::onRefresh);
  general_menu_->addAction(tr("Expand All"), this, &ProjectNavigator::onExpandAll);
  general_menu_->addAction(tr("Collapse All"), this, &ProjectNavigator::onCollapseAll);
}

void ProjectNavigator::refresh() {
  onRefresh();
}

void ProjectNavigator::clear() {
  tree_widget_->clear();
}

void ProjectNavigator::addServer(const QString& serverName, const QString& host) {
  auto* serverItem = new QTreeWidgetItem(tree_widget_);
  serverItem->setText(0, serverName);
  serverItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
  serverItem->setToolTip(0, host.isEmpty() ? serverName : QString("%1 (%2)").arg(serverName, host));
  
  // Add placeholder categories
  auto* dbsItem = new QTreeWidgetItem(serverItem);
  dbsItem->setText(0, "Databases");
  dbsItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
}

void ProjectNavigator::addDatabase(const QString& serverName, const QString& dbName) {
  QTreeWidgetItem* serverItem = findServerItem(serverName);
  if (!serverItem) return;
  
  QTreeWidgetItem* dbsItem = nullptr;
  for (int i = 0; i < serverItem->childCount(); ++i) {
    if (serverItem->child(i)->text(0) == "Databases") {
      dbsItem = serverItem->child(i);
      break;
    }
  }
  
  if (!dbsItem) {
    dbsItem = new QTreeWidgetItem(serverItem);
    dbsItem->setText(0, "Databases");
    dbsItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  }
  
  auto* dbItem = new QTreeWidgetItem(dbsItem);
  dbItem->setText(0, dbName);
  dbItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon));
  
  // Add table placeholder
  auto* tablesItem = new QTreeWidgetItem(dbItem);
  tablesItem->setText(0, "Tables");
  tablesItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
}

void ProjectNavigator::addTable(const QString& serverName, const QString& dbName, const QString& tableName) {
  QTreeWidgetItem* serverItem = findServerItem(serverName);
  if (!serverItem) return;
  
  QTreeWidgetItem* dbItem = nullptr;
  for (int i = 0; i < serverItem->childCount(); ++i) {
    QTreeWidgetItem* dbsItem = serverItem->child(i);
    if (dbsItem->text(0) == "Databases") {
      for (int j = 0; j < dbsItem->childCount(); ++j) {
        if (dbsItem->child(j)->text(0) == dbName) {
          dbItem = dbsItem->child(j);
          break;
        }
      }
      break;
    }
  }
  
  if (!dbItem) return;
  
  QTreeWidgetItem* tablesItem = nullptr;
  for (int i = 0; i < dbItem->childCount(); ++i) {
    if (dbItem->child(i)->text(0) == "Tables") {
      tablesItem = dbItem->child(i);
      break;
    }
  }
  
  if (!tablesItem) {
    tablesItem = new QTreeWidgetItem(dbItem);
    tablesItem->setText(0, "Tables");
    tablesItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  }
  
  auto* tableItem = new QTreeWidgetItem(tablesItem);
  tableItem->setText(0, tableName);
  tableItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
}

QTreeWidgetItem* ProjectNavigator::findServerItem(const QString& serverName) {
  for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = tree_widget_->topLevelItem(i);
    if (item->text(0) == serverName) {
      return item;
    }
  }
  return nullptr;
}

QTreeWidgetItem* ProjectNavigator::findOrCreateCategoryItem(QTreeWidgetItem* parent, const QString& category) {
  for (int i = 0; i < parent->childCount(); ++i) {
    if (parent->child(i)->text(0) == category) {
      return parent->child(i);
    }
  }
  auto* item = new QTreeWidgetItem(parent);
  item->setText(0, category);
  return item;
}

QString ProjectNavigator::getItemType(QTreeWidgetItem* item) const {
  if (!item) return QString();
  if (!item->parent()) return "server";
  QString parentText = item->parent()->text(0);
  if (parentText == "Databases") return "database";
  if (parentText == "Tables") return "table";
  if (parentText == "Columns") return "column";
  return "category";
}

QString ProjectNavigator::getItemPath(QTreeWidgetItem* item) const {
  if (!item) return QString();
  QStringList parts;
  QTreeWidgetItem* current = item;
  while (current) {
    parts.prepend(current->text(0));
    current = current->parent();
  }
  return parts.join(".");
}

void ProjectNavigator::onFilterTextChanged(const QString& text) {
  for (int i = 0; i < tree_widget_->topLevelItemCount(); ++i) {
    filterItem(tree_widget_->topLevelItem(i), text);
  }
}

bool ProjectNavigator::filterItem(QTreeWidgetItem* item, const QString& text) {
  bool hasVisibleChild = false;
  for (int i = 0; i < item->childCount(); ++i) {
    if (filterItem(item->child(i), text)) {
      hasVisibleChild = true;
    }
  }
  
  bool matches = item->text(0).contains(text, Qt::CaseInsensitive);
  bool visible = matches || hasVisibleChild;
  item->setHidden(!visible);
  
  if (matches && !text.isEmpty()) {
    item->setExpanded(true);
  }
  
  return visible;
}

void ProjectNavigator::onExpandAll() {
  tree_widget_->expandAll();
}

void ProjectNavigator::onCollapseAll() {
  tree_widget_->collapseAll();
}

void ProjectNavigator::onRefresh() {
  createNodes();
}

void ProjectNavigator::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
  Q_UNUSED(column)
  if (!item) return;
  
  QString type = getItemType(item);
  if (type == "table") {
    QTreeWidgetItem* dbItem = item->parent()->parent();
    QTreeWidgetItem* serverItem = dbItem->parent()->parent();
    emit tableDoubleClicked(serverItem->text(0), dbItem->text(0), item->text(0));
    emit queryTableRequested(serverItem->text(0), dbItem->text(0), item->text(0));
  } else if (type == "database") {
    QTreeWidgetItem* serverItem = item->parent()->parent();
    emit databaseSelected(serverItem->text(0), item->text(0));
  } else if (type == "server") {
    emit serverSelected(item->text(0));
  }
}

void ProjectNavigator::onContextMenu(const QPoint& pos) {
  context_item_ = tree_widget_->itemAt(pos);
  
  if (!context_item_) {
    general_menu_->popup(tree_widget_->mapToGlobal(pos));
    return;
  }
  
  QString type = getItemType(context_item_);
  if (type == "server") {
    server_menu_->popup(tree_widget_->mapToGlobal(pos));
  } else if (type == "database") {
    database_menu_->popup(tree_widget_->mapToGlobal(pos));
  } else if (type == "table") {
    table_menu_->popup(tree_widget_->mapToGlobal(pos));
  } else if (type == "column") {
    column_menu_->popup(tree_widget_->mapToGlobal(pos));
  }
}

void ProjectNavigator::onItemDragged(const QString& text) {
  emit itemDragged(text);
}

void ProjectNavigator::onQueryTable() {
  if (!context_item_) return;
  QTreeWidgetItem* dbItem = context_item_->parent()->parent();
  QTreeWidgetItem* serverItem = dbItem->parent()->parent();
  emit queryTableRequested(serverItem->text(0), dbItem->text(0), context_item_->text(0));
}

void ProjectNavigator::onEditTable() {
  if (!context_item_) return;
  QTreeWidgetItem* dbItem = context_item_->parent()->parent();
  QTreeWidgetItem* serverItem = dbItem->parent()->parent();
  emit editTableRequested(serverItem->text(0), dbItem->text(0), context_item_->text(0));
}

void ProjectNavigator::onDropTable() {
  if (!context_item_) return;
  QTreeWidgetItem* dbItem = context_item_->parent()->parent();
  QTreeWidgetItem* serverItem = dbItem->parent()->parent();
  emit dropTableRequested(serverItem->text(0), dbItem->text(0), context_item_->text(0));
}

void ProjectNavigator::onCopyName() {
  if (!context_item_) return;
  QString name = context_item_->text(0);
  QApplication::clipboard()->setText(name);
  emit copyNameRequested(name);
}

void ProjectNavigator::onRefreshItem() {
  if (!context_item_) return;
  // TODO: Refresh specific item
  refresh();
}

void ProjectNavigator::onNewConnection() {
  // TODO: Signal to open connection dialog
}

void ProjectNavigator::onDisconnect() {
  if (!context_item_) return;
  // TODO: Disconnect from server
}

}  // namespace scratchrobin::ui
