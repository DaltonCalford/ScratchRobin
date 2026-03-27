#include "ui/csv_import_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>

namespace scratchrobin::ui {

CsvImportDialog::CsvImportDialog(QWidget* parent)
    : QDialog(parent)
    , preview_table_(nullptr)
    , table_name_edit_(nullptr)
    , delimiter_edit_(nullptr)
    , quote_edit_(nullptr)
    , has_header_check_(nullptr)
    , create_table_check_(nullptr)
    , batch_size_spin_(nullptr)
    , status_label_(nullptr)
    , import_btn_(nullptr)
    , cancel_btn_(nullptr) {
  setupUi();
}

CsvImportDialog::~CsvImportDialog() = default;

void CsvImportDialog::setupUi() {
  setWindowTitle(tr("Import CSV"));
  setMinimumSize(700, 500);

  auto* main_layout = new QVBoxLayout(this);

  // Options section
  auto* options_group = new QGridLayout();
  
  options_group->addWidget(new QLabel(tr("Table Name:")), 0, 0);
  table_name_edit_ = new QLineEdit(this);
  table_name_edit_->setPlaceholderText(tr("Enter table name"));
  connect(table_name_edit_, &QLineEdit::textChanged, this, &CsvImportDialog::onTableNameChanged);
  options_group->addWidget(table_name_edit_, 0, 1);

  options_group->addWidget(new QLabel(tr("Delimiter:")), 1, 0);
  delimiter_edit_ = new QLineEdit(",", this);
  delimiter_edit_->setMaximumWidth(50);
  connect(delimiter_edit_, &QLineEdit::textChanged, this, &CsvImportDialog::onDelimiterChanged);
  options_group->addWidget(delimiter_edit_, 1, 1, Qt::AlignLeft);

  options_group->addWidget(new QLabel(tr("Quote Char:")), 2, 0);
  quote_edit_ = new QLineEdit("\"", this);
  quote_edit_->setMaximumWidth(50);
  connect(quote_edit_, &QLineEdit::textChanged, this, &CsvImportDialog::updatePreview);
  options_group->addWidget(quote_edit_, 2, 1, Qt::AlignLeft);

  has_header_check_ = new QCheckBox(tr("First row is header"), this);
  has_header_check_->setChecked(true);
  connect(has_header_check_, &QCheckBox::toggled, this, &CsvImportDialog::onHasHeaderToggled);
  options_group->addWidget(has_header_check_, 3, 0, 1, 2);

  create_table_check_ = new QCheckBox(tr("Create table if not exists"), this);
  create_table_check_->setChecked(true);
  options_group->addWidget(create_table_check_, 4, 0, 1, 2);

  options_group->addWidget(new QLabel(tr("Batch Size:")), 5, 0);
  batch_size_spin_ = new QSpinBox(this);
  batch_size_spin_->setRange(1, 10000);
  batch_size_spin_->setValue(1000);
  batch_size_spin_->setSingleStep(100);
  options_group->addWidget(batch_size_spin_, 5, 1, Qt::AlignLeft);

  main_layout->addLayout(options_group);

  // Preview section
  main_layout->addWidget(new QLabel(tr("Preview (first 10 rows):")));
  
  preview_table_ = new QTableWidget(this);
  preview_table_->setAlternatingRowColors(true);
  preview_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  preview_table_->horizontalHeader()->setStretchLastSection(true);
  preview_table_->setMaximumHeight(250);
  main_layout->addWidget(preview_table_);

  // Status
  status_label_ = new QLabel(tr("No file loaded"), this);
  main_layout->addWidget(status_label_);

  // Buttons
  auto* button_layout = new QHBoxLayout();
  button_layout->addStretch();
  
  cancel_btn_ = new QPushButton(tr("Cancel"), this);
  connect(cancel_btn_, &QPushButton::clicked, this, &CsvImportDialog::onCancelClicked);
  button_layout->addWidget(cancel_btn_);

  import_btn_ = new QPushButton(tr("Import"), this);
  import_btn_->setEnabled(false);
  import_btn_->setDefault(true);
  connect(import_btn_, &QPushButton::clicked, this, &CsvImportDialog::onImportClicked);
  button_layout->addWidget(import_btn_);

  main_layout->addLayout(button_layout);
}

bool CsvImportDialog::loadCsv(const QString& filename) {
  filename_ = filename;
  
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, tr("Error"), 
                         tr("Cannot open file: %1").arg(filename));
    return false;
  }

  QTextStream stream(&file);
  stream.setEncoding(QStringConverter::Utf8);
  file_content_ = stream.readAll();
  file.close();

  // Extract table name from filename
  QFileInfo fileInfo(filename);
  QString baseName = fileInfo.baseName();
  // Sanitize for SQL identifier
  baseName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
  if (baseName.at(0).isDigit()) {
    baseName = "table_" + baseName;
  }
  table_name_edit_->setText(baseName.toLower());

  updatePreview();
  return true;
}

QList<QStringList> CsvImportDialog::parseCsv(const QString& content) {
  QList<QStringList> result;
  QChar delimiter = delimiter_edit_->text().isEmpty() ? ',' : delimiter_edit_->text().at(0);
  QChar quote = quote_edit_->text().isEmpty() ? '"' : quote_edit_->text().at(0);

  QStringList currentRow;
  QString currentField;
  bool inQuotes = false;

  for (int i = 0; i < content.length(); ++i) {
    QChar c = content.at(i);

    if (inQuotes) {
      if (c == quote) {
        // Check for escaped quote (double quote)
        if (i + 1 < content.length() && content.at(i + 1) == quote) {
          currentField.append(quote);
          ++i;  // Skip next quote
        } else {
          inQuotes = false;
        }
      } else {
        currentField.append(c);
      }
    } else {
      if (c == quote) {
        inQuotes = true;
      } else if (c == delimiter) {
        currentRow.append(currentField);
        currentField.clear();
      } else if (c == '\n' || c == '\r') {
        if (!currentField.isEmpty() || !currentRow.isEmpty()) {
          currentRow.append(currentField);
          result.append(currentRow);
          currentRow.clear();
          currentField.clear();
        }
        // Skip \r\n sequence
        if (c == '\r' && i + 1 < content.length() && content.at(i + 1) == '\n') {
          ++i;
        }
      } else {
        currentField.append(c);
      }
    }
  }

  // Don't forget the last field/row
  if (!currentField.isEmpty() || !currentRow.isEmpty()) {
    currentRow.append(currentField);
    result.append(currentRow);
  }

  return result;
}

void CsvImportDialog::updatePreview() {
  if (file_content_.isEmpty()) return;

  QList<QStringList> parsed = parseCsv(file_content_);
  if (parsed.isEmpty()) {
    status_label_->setText(tr("No data found in file"));
    return;
  }

  data_rows_ = parsed;

  // Handle headers
  if (has_header_check_->isChecked() && !data_rows_.isEmpty()) {
    headers_ = data_rows_.first();
    data_rows_.removeFirst();
  } else {
    // Generate column names
    headers_.clear();
    if (!data_rows_.isEmpty()) {
      for (int i = 0; i < data_rows_.first().size(); ++i) {
        headers_.append(QString("column_%1").arg(i + 1));
      }
    }
  }

  // Update preview table (show first 10 rows)
  preview_table_->clear();
  preview_table_->setColumnCount(headers_.size());
  preview_table_->setHorizontalHeaderLabels(headers_);
  
  int previewRows = qMin(data_rows_.size(), 10);
  preview_table_->setRowCount(previewRows);

  for (int row = 0; row < previewRows; ++row) {
    const QStringList& rowData = data_rows_.at(row);
    for (int col = 0; col < qMin(rowData.size(), headers_.size()); ++col) {
      preview_table_->setItem(row, col, new QTableWidgetItem(rowData.at(col)));
    }
  }

  status_label_->setText(tr("Total rows: %1, Columns: %2").arg(data_rows_.size()).arg(headers_.size()));
  import_btn_->setEnabled(!table_name_edit_->text().isEmpty() && !data_rows_.isEmpty());
}

CsvImportOptions CsvImportDialog::options() const {
  CsvImportOptions opts;
  opts.table_name = table_name_edit_->text();
  opts.has_header = has_header_check_->isChecked();
  opts.create_table = create_table_check_->isChecked();
  opts.delimiter = delimiter_edit_->text().isEmpty() ? ',' : delimiter_edit_->text().at(0);
  opts.quote = quote_edit_->text().isEmpty() ? '"' : quote_edit_->text().at(0);
  opts.batch_size = batch_size_spin_->value();
  return opts;
}

QStringList CsvImportDialog::headers() const {
  return headers_;
}

QList<QStringList> CsvImportDialog::dataRows() const {
  return data_rows_;
}

QString CsvImportDialog::escapeSqlIdentifier(const QString& name) const {
  // Simple escaping - in production, use proper SQL escaping
  QString escaped = name;
  escaped.replace("\"", "\"\"");
  return "\"" + escaped + "\"";
}

QString CsvImportDialog::inferSqlType(const QStringList& values) const {
  bool allInt = true;
  bool allDouble = true;
  bool allBool = true;

  for (const QString& val : values) {
    QString trimmed = val.trimmed();
    if (trimmed.isEmpty()) continue;

    // Check boolean
    QString lower = trimmed.toLower();
    if (lower != "true" && lower != "false" && lower != "1" && lower != "0" && 
        lower != "yes" && lower != "no" && lower != "t" && lower != "f") {
      allBool = false;
    }

    // Check integer
    bool intOk;
    trimmed.toLongLong(&intOk);
    if (!intOk) allInt = false;

    // Check double
    bool doubleOk;
    trimmed.toDouble(&doubleOk);
    if (!doubleOk) allDouble = false;
  }

  if (allBool) return "BOOLEAN";
  if (allInt) return "BIGINT";
  if (allDouble) return "DOUBLE PRECISION";
  
  // Check if it looks like a date
  int dateCount = 0;
  QRegularExpression dateRegex("^\\d{4}[-/]\\d{1,2}[-/]\\d{1,2}$");
  for (const QString& val : values) {
    if (dateRegex.match(val.trimmed()).hasMatch()) dateCount++;
  }
  if (dateCount > values.size() / 2) return "DATE";

  // Default to text with reasonable length
  int maxLen = 0;
  for (const QString& val : values) {
    maxLen = qMax(maxLen, val.length());
  }
  if (maxLen <= 255) return "VARCHAR(255)";
  return "TEXT";
}

void CsvImportDialog::onTableNameChanged(const QString& text) {
  import_btn_->setEnabled(!text.isEmpty() && !data_rows_.isEmpty());
}

void CsvImportDialog::onDelimiterChanged(const QString& text) {
  updatePreview();
}

void CsvImportDialog::onHasHeaderToggled(bool checked) {
  updatePreview();
}

void CsvImportDialog::onImportClicked() {
  if (table_name_edit_->text().isEmpty()) {
    QMessageBox::warning(this, tr("Warning"), tr("Please enter a table name"));
    return;
  }
  accept();
}

void CsvImportDialog::onCancelClicked() {
  reject();
}

}  // namespace scratchrobin::ui
