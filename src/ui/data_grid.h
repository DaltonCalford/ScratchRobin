#pragma once
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QLabel;
class QSortFilterProxyModel;
QT_END_NAMESPACE

namespace scratchrobin::ui {

class DataGrid : public QWidget {
  Q_OBJECT

 public:
  explicit DataGrid(QWidget* parent = nullptr);
  ~DataGrid() override;

  void setData(const QList<QStringList>& data, const QStringList& headers);
  void clear();
  
  int rowCount() const;
  int columnCount() const;
  
  QString selectedCellValue() const;
  QStringList selectedRow() const;

 signals:
  void rowDoubleClicked(int row);
  void cellClicked(int row, int column);
  void selectionChanged();

 public slots:
  void refresh();
  void exportToCsv(const QString& filename);
  void filter(const QString& text);

 private slots:
  void onFilterTextChanged(const QString& text);
  void onItemDoubleClicked(const QModelIndex& index);
  void onExportClicked();
  void onRefreshClicked();

 private:
  void setupUi();

  QTableView* table_view_;
  QStandardItemModel* model_;
  QSortFilterProxyModel* proxy_model_;
  
  QLineEdit* filter_edit_;
  QPushButton* export_btn_;
  QPushButton* refresh_btn_;
  QLabel* status_label_;
  
  int total_rows_ = 0;
};

}  // namespace scratchrobin::ui
