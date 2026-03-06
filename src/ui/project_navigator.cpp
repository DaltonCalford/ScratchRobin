#include "ui/project_navigator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolButton>
#include <QMenu>
#include <QHeaderView>

namespace scratchrobin::ui {

ProjectNavigator::ProjectNavigator(QWidget* parent) : QWidget(parent) {
  setupUi();
  createNodes();
}

ProjectNavigator::~ProjectNavigator() = default;

void ProjectNavigator::setupUi() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(2);

  auto* filter_layout = new QHBoxLayout();
  filter_layout->setSpacing(2);
  
  filter_edit_ = new QLineEdit(this);
  filter_edit_->setPlaceholderText(tr("Filter objects..."));
  filter_edit_->setClearButtonEnabled(true);
  filter_layout->addWidget(filter_edit_);

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

  tree_widget_ = new QTreeWidget(this);
  tree_widget_->setHeaderHidden(true);
  tree_widget_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
  tree_widget_->setAlternatingRowColors(true);
  layout->addWidget(tree_widget_);

  connect(filter_edit_, &QLineEdit::textChanged, this, &ProjectNavigator::onFilterTextChanged);
  connect(expand_btn_, &QToolButton::clicked, this, &ProjectNavigator::onExpandAll);
  connect(collapse_btn_, &QToolButton::clicked, this, &ProjectNavigator::onCollapseAll);
  connect(refresh_btn_, &QToolButton::clicked, this, &ProjectNavigator::onRefresh);
}

void ProjectNavigator::createNodes() {
  tree_widget_->clear();

  auto* servers_header = new QTreeWidgetItem(tree_widget_);
  servers_header->setText(0, tr("Servers"));
  servers_header->setExpanded(true);

  auto* localhost = new QTreeWidgetItem(servers_header);
  localhost->setText(0, tr("localhost"));
  localhost->setIcon(0, QIcon::fromTheme("network-server"));
  localhost->setExpanded(true);

  auto* main_db = new QTreeWidgetItem(localhost);
  main_db->setText(0, tr("main_db"));
  main_db->setIcon(0, QIcon::fromTheme("folder-database"));

  auto* tables_cat = new QTreeWidgetItem(main_db);
  tables_cat->setText(0, tr("Tables"));
  tables_cat->setIcon(0, QIcon::fromTheme("folder"));

  auto* users_table = new QTreeWidgetItem(tables_cat);
  users_table->setText(0, tr("users"));
  users_table->setIcon(0, QIcon::fromTheme("table"));

  auto* orders_table = new QTreeWidgetItem(tables_cat);
  orders_table->setText(0, tr("orders"));
  orders_table->setIcon(0, QIcon::fromTheme("table"));
}

void ProjectNavigator::refresh() { createNodes(); }
void ProjectNavigator::clear() { tree_widget_->clear(); }

void ProjectNavigator::addServer(const QString& serverName, const QString& host) {
  Q_UNUSED(host)
  auto* root = tree_widget_->topLevelItem(0);
  if (!root) { root = new QTreeWidgetItem(tree_widget_); root->setText(0, tr("Servers")); }
  auto* server = new QTreeWidgetItem(root);
  server->setText(0, serverName);
  server->setIcon(0, QIcon::fromTheme("network-server"));
}

void ProjectNavigator::addDatabase(const QString& serverName, const QString& dbName) {
  auto* server = findServerItem(serverName);
  if (!server) return;
  auto* db = new QTreeWidgetItem(server);
  db->setText(0, dbName);
  db->setIcon(0, QIcon::fromTheme("folder-database"));
}

void ProjectNavigator::addTable(const QString& serverName, const QString& dbName, const QString& tableName) {
  auto* server = findServerItem(serverName);
  if (!server) return;
  for (int i = 0; i < server->childCount(); ++i) {
    auto* db = server->child(i);
    if (db->text(0) == dbName) {
      auto* tables_cat = findOrCreateCategoryItem(db, tr("Tables"));
      auto* table = new QTreeWidgetItem(tables_cat);
      table->setText(0, tableName);
      table->setIcon(0, QIcon::fromTheme("table"));
      return;
    }
  }
}

QTreeWidgetItem* ProjectNavigator::findServerItem(const QString& serverName) {
  auto* root = tree_widget_->topLevelItem(0);
  if (!root) return nullptr;
  for (int i = 0; i < root->childCount(); ++i) {
    auto* child = root->child(i);
    if (child->text(0) == serverName) return child;
  }
  return nullptr;
}

QTreeWidgetItem* ProjectNavigator::findOrCreateCategoryItem(QTreeWidgetItem* parent, const QString& category) {
  if (!parent) return nullptr;
  for (int i = 0; i < parent->childCount(); ++i) {
    auto* child = parent->child(i);
    if (child->text(0) == category) return child;
  }
  auto* cat = new QTreeWidgetItem(parent);
  cat->setText(0, category);
  cat->setIcon(0, QIcon::fromTheme("folder"));
  return cat;
}

void ProjectNavigator::onFilterTextChanged(const QString& text) { Q_UNUSED(text) }
void ProjectNavigator::onExpandAll() { tree_widget_->expandAll(); }
void ProjectNavigator::onCollapseAll() { tree_widget_->collapseAll(); }
void ProjectNavigator::onRefresh() { refresh(); }

}  // namespace scratchrobin::ui
