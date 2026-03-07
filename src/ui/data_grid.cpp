#include "ui/data_grid.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

namespace scratchrobin::ui {

DataGrid::DataGrid(QWidget* parent)
    : QWidget(parent)
    , table_view_(nullptr)
    , model_(nullptr)
    , proxy_model_(nullptr)
    , filter_edit_(nullptr)
    , export_btn_(nullptr)
    , refresh_btn_(nullptr)
    , status_label_(nullptr)
    , total_rows_(0) {
  setupUi();
}

DataGrid::~DataGrid() = default;

void DataGrid::setupUi() {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  // Toolbar
  auto* toolbar = new QHBoxLayout();
  
  filter_edit_ = new QLineEdit(this);
  filter_edit_->setPlaceholderText(tr("Filter rows..."));
  filter_edit_->setClearButtonEnabled(true);
  toolbar->addWidget(filter_edit_, 1);
  
  refresh_btn_ = new QPushButton(tr("Refresh"), this);
  toolbar->addWidget(refresh_btn_);
  
  export_btn_ = new QPushButton(tr("Export CSV"), this);
  toolbar->addWidget(export_btn_);
  
  layout->addLayout(toolbar);

  // Table
  table_view_ = new QTableView(this);
  table_view_->setAlternatingRowColors(true);
  table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  table_view_->horizontalHeader()->setStretchLastSection(true);
  table_view_->horizontalHeader()->setSectionsMovable(true);
  table_view_->verticalHeader()->setVisible(false);
  table_view_->setSortingEnabled(true);
  
  model_ = new QStandardItemModel(this);
  proxy_model_ = new QSortFilterProxyModel(this);
  proxy_model_->setSourceModel(model_);
  proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy_model_->setFilterKeyColumn(-1);  // Search all columns
  
  table_view_->setModel(proxy_model_);
  layout->addWidget(table_view_);

  // Status bar
  auto* status_layout = new QHBoxLayout();
  status_label_ = new QLabel(tr("No data"), this);
  status_layout->addWidget(status_label_);
  status_layout->addStretch();
  layout->addLayout(status_layout);

  // Connections
  connect(filter_edit_, &QLineEdit::textChanged,
          this, &DataGrid::onFilterTextChanged);
  connect(export_btn_, &QPushButton::clicked,
          this, &DataGrid::onExportClicked);
  connect(refresh_btn_, &QPushButton::clicked,
          this, &DataGrid::onRefreshClicked);
  connect(table_view_, &QTableView::doubleClicked,
          this, &DataGrid::onItemDoubleClicked);
  connect(table_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &DataGrid::selectionChanged);
}

void DataGrid::setData(const QList<QStringList>& data, const QStringList& headers) {
  model_->clear();
  model_->setHorizontalHeaderLabels(headers);
  
  total_rows_ = data.size();
  
  for (const auto& row : data) {
    QList<QStandardItem*> items;
    for (const auto& cell : row) {
      auto* item = new QStandardItem(cell);
      item->setEditable(false);
      items.append(item);
    }
    model_->appendRow(items);
  }
  
  status_label_->setText(tr("%1 rows, %2 columns").arg(total_rows_).arg(headers.size()));
}

void DataGrid::clear() {
  model_->clear();
  total_rows_ = 0;
  status_label_->setText(tr("No data"));
}

int DataGrid::rowCount() const {
  return total_rows_;
}

int DataGrid::columnCount() const {
  return model_->columnCount();
}

QString DataGrid::selectedCellValue() const {
  QModelIndex index = table_view_->currentIndex();
  if (index.isValid()) {
    return proxy_model_->data(index).toString();
  }
  return QString();
}

QStringList DataGrid::selectedRow() const {
  QStringList result;
  QModelIndex index = table_view_->currentIndex();
  if (index.isValid()) {
    int row = index.row();
    for (int col = 0; col < proxy_model_->columnCount(); ++col) {
      result.append(proxy_model_->index(row, col).data().toString());
    }
  }
  return result;
}

void DataGrid::refresh() {
  emit refresh_btn_->clicked();
}

void DataGrid::exportToCsv(const QString& filename) {
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::critical(this, tr("Error"), 
                          tr("Cannot open file for writing: %1").arg(filename));
    return;
  }
  
  QTextStream stream(&file);
  
  // Headers
  QStringList headers;
  for (int col = 0; col < model_->columnCount(); ++col) {
    headers.append(model_->headerData(col, Qt::Horizontal).toString());
  }
  stream << headers.join(",") << "\n";
  
  // Data
  for (int row = 0; row < proxy_model_->rowCount(); ++row) {
    QStringList row_data;
    for (int col = 0; col < proxy_model_->columnCount(); ++col) {
      QString cell = proxy_model_->index(row, col).data().toString();
      // Escape quotes and wrap in quotes if contains comma
      if (cell.contains(',') || cell.contains('"') || cell.contains('\n')) {
        cell.replace("\"", "\"\"");
        cell = "\"" + cell + "\"";
      }
      row_data.append(cell);
    }
    stream << row_data.join(",") << "\n";
  }
  
  file.close();
  status_label_->setText(tr("Exported to %1").arg(filename));
}

void DataGrid::filter(const QString& text) {
  filter_edit_->setText(text);
}

void DataGrid::onFilterTextChanged(const QString& text) {
  proxy_model_->setFilterFixedString(text);
  int visible = proxy_model_->rowCount();
  status_label_->setText(tr("%1 of %2 rows shown").arg(visible).arg(total_rows_));
}

void DataGrid::onItemDoubleClicked(const QModelIndex& index) {
  if (index.isValid()) {
    emit rowDoubleClicked(proxy_model_->mapToSource(index).row());
    emit cellClicked(proxy_model_->mapToSource(index).row(), index.column());
  }
}

void DataGrid::onExportClicked() {
  QString filename = QFileDialog::getSaveFileName(this, tr("Export to CSV"),
                                                   QString(),
                                                   tr("CSV Files (*.csv);;All Files (*)"));
  if (!filename.isEmpty()) {
    exportToCsv(filename);
  }
}

void DataGrid::onRefreshClicked() {
  emit refresh();
}

}  // namespace scratchrobin::ui
