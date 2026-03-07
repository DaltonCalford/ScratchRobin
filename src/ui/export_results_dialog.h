#pragma once
#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QStackedWidget;
class QSpinBox;
class QTableView;
class QLabel;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Export Results dialog for exporting query results
 * 
 * Implements FORM_SPECIFICATION.md Export Results section:
 * - Multiple formats: CSV, SQL, JSON, Excel, XML, HTML
 * - Format-specific options
 * - Scope selection (all/selected rows)
 */
class ExportResultsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportResultsDialog(QWidget* parent = nullptr);
    ~ExportResultsDialog() override;

    void setData(const QList<QStringList>& data, const QStringList& headers);

private slots:
    void onFormatChanged(int index);
    void onBrowseFile();
    void onExport();
    void onOptionsChanged();

private:
    void setupUi();
    void createFormatPages();
    void updateOptionsPage();
    bool validateInputs();
    bool exportToCsv();
    bool exportToSql();
    bool exportToJson();

    // Data
    QList<QStringList> data_;
    QStringList headers_;

    // UI
    QComboBox* formatCombo_ = nullptr;
    QLineEdit* fileEdit_ = nullptr;
    QPushButton* browseBtn_ = nullptr;
    QPushButton* exportBtn_ = nullptr;
    QPushButton* cancelBtn_ = nullptr;
    QRadioButton* allRowsRadio_ = nullptr;
    QRadioButton* selectedRowsRadio_ = nullptr;
    QLabel* previewLabel_ = nullptr;
    
    // Format options pages
    QStackedWidget* optionsStack_ = nullptr;
    QWidget* csvPage_ = nullptr;
    QWidget* sqlPage_ = nullptr;
    QWidget* jsonPage_ = nullptr;
    
    // CSV options
    QComboBox* csvDelimiterCombo_ = nullptr;
    QComboBox* csvQuoteCombo_ = nullptr;
    QComboBox* csvEscapeCombo_ = nullptr;
    QCheckBox* csvHeaderCheck_ = nullptr;
    QComboBox* csvEncodingCombo_ = nullptr;
    
    // SQL options
    QLineEdit* sqlTableEdit_ = nullptr;
    QCheckBox* sqlIncludeColumnsCheck_ = nullptr;
    QSpinBox* sqlRowsPerStatementSpin_ = nullptr;
    QComboBox* sqlStatementTypeCombo_ = nullptr;
    
    // JSON options
    QComboBox* jsonFormatCombo_ = nullptr;
    QCheckBox* jsonPrettyPrintCheck_ = nullptr;
    QCheckBox* jsonIncludeMetadataCheck_ = nullptr;
};

} // namespace scratchrobin::ui
