#pragma once
#include <QDialog>
#include <QStringList>
#include <QList>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE

namespace scratchrobin::ui {

struct CsvImportOptions {
  QString table_name;
  bool has_header = true;
  bool create_table = true;
  QChar delimiter = ',';
  QChar quote = '"';
  QString encoding = "UTF-8";
  int batch_size = 1000;
};

class CsvImportDialog : public QDialog {
  Q_OBJECT

 public:
  explicit CsvImportDialog(QWidget* parent = nullptr);
  ~CsvImportDialog() override;

  bool loadCsv(const QString& filename);
  CsvImportOptions options() const;
  
  // Get the parsed data
  QStringList headers() const;
  QList<QStringList> dataRows() const;

 private slots:
  void onTableNameChanged(const QString& text);
  void onDelimiterChanged(const QString& text);
  void onHasHeaderToggled(bool checked);
  void updatePreview();
  void onImportClicked();
  void onCancelClicked();

 private:
  void setupUi();
  QList<QStringList> parseCsv(const QString& content);
  QString escapeSqlIdentifier(const QString& name) const;
  QString inferSqlType(const QStringList& values) const;

  QTableWidget* preview_table_;
  QLineEdit* table_name_edit_;
  QLineEdit* delimiter_edit_;
  QLineEdit* quote_edit_;
  QCheckBox* has_header_check_;
  QCheckBox* create_table_check_;
  QSpinBox* batch_size_spin_;
  QLabel* status_label_;
  QPushButton* import_btn_;
  QPushButton* cancel_btn_;

  QStringList headers_;
  QList<QStringList> data_rows_;
  QString filename_;
  QString file_content_;
};

}  // namespace scratchrobin::ui
