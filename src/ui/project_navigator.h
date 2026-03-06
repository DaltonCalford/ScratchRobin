#pragma once
#include <QTreeWidget>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QToolButton;
QT_END_NAMESPACE

namespace scratchrobin::ui {

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

 private slots:
  void onFilterTextChanged(const QString& text);
  void onExpandAll();
  void onCollapseAll();
  void onRefresh();

 private:
  void setupUi();
  void createNodes();
  QTreeWidgetItem* findServerItem(const QString& serverName);
  QTreeWidgetItem* findOrCreateCategoryItem(QTreeWidgetItem* parent, const QString& category);

  QLineEdit* filter_edit_;
  QTreeWidget* tree_widget_;
  QToolButton* expand_btn_;
  QToolButton* collapse_btn_;
  QToolButton* refresh_btn_;
};

}  // namespace scratchrobin::ui
