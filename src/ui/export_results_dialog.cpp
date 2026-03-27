#include "ui/export_results_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QStackedWidget>
#include <QLabel>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamWriter>
#include <QDateTime>

#ifdef SCRATCHROBIN_WITH_QXLSX
#include "xlsxdocument.h"
#endif

namespace scratchrobin::ui {

ExportResultsDialog::ExportResultsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Export Results"));
    setMinimumSize(500, 400);
    resize(550, 450);
    
    setupUi();
    createFormatPages();
}

ExportResultsDialog::~ExportResultsDialog() = default;

void ExportResultsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Format selection
    auto* formatLayout = new QHBoxLayout();
    formatLayout->addWidget(new QLabel(tr("Format:"), this));
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItem(tr("CSV (Comma Separated)"), "csv");
    formatCombo_->addItem(tr("SQL INSERT statements"), "sql");
    formatCombo_->addItem(tr("JSON"), "json");
    formatCombo_->addItem(tr("Excel (.xlsx)"), "excel");
    formatCombo_->addItem(tr("XML"), "xml");
    formatCombo_->addItem(tr("HTML"), "html");
    formatLayout->addWidget(formatCombo_);
    formatLayout->addStretch();
    mainLayout->addLayout(formatLayout);
    
    // File selection
    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel(tr("File:"), this));
    fileEdit_ = new QLineEdit(this);
    fileEdit_->setPlaceholderText(tr("Select destination file..."));
    fileLayout->addWidget(fileEdit_);
    browseBtn_ = new QPushButton(tr("Browse..."), this);
    fileLayout->addWidget(browseBtn_);
    mainLayout->addLayout(fileLayout);
    
    // Options pages
    optionsStack_ = new QStackedWidget(this);
    mainLayout->addWidget(optionsStack_);
    
    // Scope selection
    auto* scopeGroup = new QHBoxLayout();
    allRowsRadio_ = new QRadioButton(tr("All rows"), this);
    selectedRowsRadio_ = new QRadioButton(tr("Selected rows only"), this);
    allRowsRadio_->setChecked(true);
    scopeGroup->addWidget(allRowsRadio_);
    scopeGroup->addWidget(selectedRowsRadio_);
    scopeGroup->addStretch();
    mainLayout->addLayout(scopeGroup);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    exportBtn_ = new QPushButton(tr("Export"), this);
    exportBtn_->setDefault(true);
    buttonLayout->addWidget(exportBtn_);
    
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(cancelBtn_);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportResultsDialog::onFormatChanged);
    connect(browseBtn_, &QPushButton::clicked, this, &ExportResultsDialog::onBrowseFile);
    connect(exportBtn_, &QPushButton::clicked, this, &ExportResultsDialog::onExport);
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
}

void ExportResultsDialog::createFormatPages() {
    // CSV Options Page
    csvPage_ = new QWidget(this);
    auto* csvLayout = new QGridLayout(csvPage_);
    csvLayout->addWidget(new QLabel(tr("Delimiter:"), this), 0, 0);
    csvDelimiterCombo_ = new QComboBox(this);
    csvDelimiterCombo_->addItem(tr("Comma"), ",");
    csvDelimiterCombo_->addItem(tr("Tab"), "\t");
    csvDelimiterCombo_->addItem(tr("Semicolon"), ";");
    csvDelimiterCombo_->addItem(tr("Pipe"), "|");
    csvLayout->addWidget(csvDelimiterCombo_, 0, 1);
    csvLayout->addWidget(new QLabel(tr("Quote character:"), this), 1, 0);
    csvQuoteCombo_ = new QComboBox(this);
    csvQuoteCombo_->addItem(tr("Double quote (\"))"), "\"");
    csvQuoteCombo_->addItem(tr("Single quote (')"), "'");
    csvLayout->addWidget(csvQuoteCombo_, 1, 1);
    csvLayout->addWidget(new QLabel(tr("Encoding:"), this), 2, 0);
    csvEncodingCombo_ = new QComboBox(this);
    csvEncodingCombo_->addItem(tr("UTF-8"), "UTF-8");
    csvEncodingCombo_->addItem(tr("ASCII"), "ASCII");
    csvLayout->addWidget(csvEncodingCombo_, 2, 1);
    csvHeaderCheck_ = new QCheckBox(tr("Include header row"), this);
    csvHeaderCheck_->setChecked(true);
    csvLayout->addWidget(csvHeaderCheck_, 3, 0, 1, 2);
    optionsStack_->addWidget(csvPage_);
    
    // SQL Options Page
    sqlPage_ = new QWidget(this);
    auto* sqlLayout = new QGridLayout(sqlPage_);
    sqlLayout->addWidget(new QLabel(tr("Target table:"), this), 0, 0);
    sqlTableEdit_ = new QLineEdit(this);
    sqlTableEdit_->setPlaceholderText(tr("table_name"));
    sqlLayout->addWidget(sqlTableEdit_, 0, 1);
    sqlIncludeColumnsCheck_ = new QCheckBox(tr("Include column names"), this);
    sqlIncludeColumnsCheck_->setChecked(true);
    sqlLayout->addWidget(sqlIncludeColumnsCheck_, 1, 0, 1, 2);
    optionsStack_->addWidget(sqlPage_);
    
    // JSON Options Page
    jsonPage_ = new QWidget(this);
    auto* jsonLayout = new QGridLayout(jsonPage_);
    jsonLayout->addWidget(new QLabel(tr("Format:"), this), 0, 0);
    jsonFormatCombo_ = new QComboBox(this);
    jsonFormatCombo_->addItem(tr("Array of objects"), "array");
    jsonFormatCombo_->addItem(tr("Object of arrays"), "object");
    jsonLayout->addWidget(jsonFormatCombo_, 0, 1);
    jsonPrettyPrintCheck_ = new QCheckBox(tr("Pretty print"), this);
    jsonPrettyPrintCheck_->setChecked(true);
    jsonLayout->addWidget(jsonPrettyPrintCheck_, 1, 0, 1, 2);
    jsonIncludeMetadataCheck_ = new QCheckBox(tr("Include metadata"), this);
    jsonLayout->addWidget(jsonIncludeMetadataCheck_, 2, 0, 1, 2);
    optionsStack_->addWidget(jsonPage_);
    
    // Placeholder pages for other formats
    for (int i = 0; i < 3; ++i) {
        auto* placeholder = new QWidget(this);
        auto* layout = new QVBoxLayout(placeholder);
        layout->addWidget(new QLabel(tr("No additional options for this format."), this));
        optionsStack_->addWidget(placeholder);
    }
}

void ExportResultsDialog::setData(const QList<QStringList>& data, const QStringList& headers) {
    data_ = data;
    headers_ = headers;
}

void ExportResultsDialog::onFormatChanged(int index) {
    optionsStack_->setCurrentIndex(index);
    
    // Update default file extension
    QString currentFile = fileEdit_->text();
    if (!currentFile.isEmpty()) {
        QString newExt;
        switch (index) {
            case 0: newExt = ".csv"; break;
            case 1: newExt = ".sql"; break;
            case 2: newExt = ".json"; break;
            case 3: newExt = ".xlsx"; break;
            case 4: newExt = ".xml"; break;
            case 5: newExt = ".html"; break;
        }
        // Replace extension
        int dotPos = currentFile.lastIndexOf('.');
        if (dotPos > 0) {
            currentFile = currentFile.left(dotPos) + newExt;
        } else {
            currentFile += newExt;
        }
        fileEdit_->setText(currentFile);
    }
}

void ExportResultsDialog::onBrowseFile() {
    QString filter;
    QString format = formatCombo_->currentData().toString();
    
    if (format == "csv") filter = tr("CSV Files (*.csv);;All Files (*)");
    else if (format == "sql") filter = tr("SQL Files (*.sql);;All Files (*)");
    else if (format == "json") filter = tr("JSON Files (*.json);;All Files (*)");
    else if (format == "excel") filter = tr("Excel Files (*.xlsx);;All Files (*)");
    else if (format == "xml") filter = tr("XML Files (*.xml);;All Files (*)");
    else if (format == "html") filter = tr("HTML Files (*.html);;All Files (*)");
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Results"), 
                                                    QString(), filter);
    if (!fileName.isEmpty()) {
        fileEdit_->setText(fileName);
    }
}

bool ExportResultsDialog::validateInputs() {
    if (fileEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Export Error"), 
            tr("Please select a destination file."));
        return false;
    }
    
    if (data_.isEmpty()) {
        QMessageBox::warning(this, tr("Export Error"), 
            tr("No data to export."));
        return false;
    }
    
    return true;
}

void ExportResultsDialog::onExport() {
    if (!validateInputs()) return;
    
    QString format = formatCombo_->currentData().toString();
    bool success = false;
    
    if (format == "csv") {
        success = exportToCsv();
    } else if (format == "sql") {
        success = exportToSql();
    } else if (format == "json") {
        success = exportToJson();
    } else if (format == "excel") {
        success = exportToExcel();
    } else if (format == "xml") {
        success = exportToXml();
    } else if (format == "html") {
        success = exportToHtml();
    } else {
        QMessageBox::warning(this, tr("Unknown Format"), 
            tr("Unknown export format: %1").arg(format.toUpper()));
        return;
    }
    
    if (success) {
        QMessageBox::information(this, tr("Export Complete"), 
            tr("Data exported successfully to:\n%1").arg(fileEdit_->text()));
        accept();
    }
}

bool ExportResultsDialog::exportToCsv() {
    QFile file(fileEdit_->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"), 
            tr("Cannot open file for writing."));
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    
    QString delimiter = csvDelimiterCombo_->currentData().toString();
    QString quote = csvQuoteCombo_->currentData().toString();
    
    // Write header
    if (csvHeaderCheck_->isChecked()) {
        for (int i = 0; i < headers_.size(); ++i) {
            if (i > 0) stream << delimiter;
            stream << quote << headers_[i] << quote;
        }
        stream << "\n";
    }
    
    // Write data
    for (const auto& row : data_) {
        for (int i = 0; i < row.size(); ++i) {
            if (i > 0) stream << delimiter;
            stream << quote << row[i] << quote;
        }
        stream << "\n";
    }
    
    file.close();
    return true;
}

bool ExportResultsDialog::exportToSql() {
    QFile file(fileEdit_->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"), 
            tr("Cannot open file for writing."));
        return false;
    }
    
    QTextStream stream(&file);
    QString tableName = sqlTableEdit_->text().isEmpty() ? "table_name" : sqlTableEdit_->text();
    
    stream << "-- Exported from ScratchRobin\n";
    stream << "-- Table: " << tableName << "\n\n";
    
    for (const auto& row : data_) {
        stream << "INSERT INTO " << tableName;
        
        if (sqlIncludeColumnsCheck_->isChecked() && !headers_.isEmpty()) {
            stream << " (";
            for (int i = 0; i < headers_.size(); ++i) {
                if (i > 0) stream << ", ";
                stream << headers_[i];
            }
            stream << ")";
        }
        
        stream << " VALUES (";
        for (int i = 0; i < row.size(); ++i) {
            if (i > 0) stream << ", ";
            // Simple quoting - in production would need proper escaping
            stream << "'" << row[i] << "'";
        }
        stream << ");\n";
    }
    
    file.close();
    return true;
}

bool ExportResultsDialog::exportToJson() {
    QFile file(fileEdit_->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"), 
            tr("Cannot open file for writing."));
        return false;
    }
    
    QJsonArray jsonArray;
    
    for (const auto& row : data_) {
        QJsonObject obj;
        for (int i = 0; i < row.size() && i < headers_.size(); ++i) {
            obj[headers_[i]] = row[i];
        }
        jsonArray.append(obj);
    }
    
    QJsonDocument doc(jsonArray);
    QByteArray jsonData = jsonPrettyPrintCheck_->isChecked() 
        ? doc.toJson(QJsonDocument::Indented) 
        : doc.toJson(QJsonDocument::Compact);
    
    file.write(jsonData);
    file.close();
    return true;
}

bool ExportResultsDialog::exportToExcel() {
#ifdef SCRATCHROBIN_WITH_QXLSX
    QXlsx::Document xlsx;
    
    // Write header row
    for (int i = 0; i < headers_.size(); ++i) {
        xlsx.write(1, i + 1, headers_[i]);
    }
    
    // Write data rows
    int rowNum = 2;
    for (const auto& row : data_) {
        for (int i = 0; i < row.size(); ++i) {
            xlsx.write(rowNum, i + 1, row[i]);
        }
        rowNum++;
    }
    
    // Auto-size columns
    for (int i = 0; i < headers_.size(); ++i) {
        xlsx.setColumnWidth(i + 1, 15);
    }
    
    return xlsx.saveAs(fileEdit_->text());
#else
    QMessageBox::information(this, tr("Not Available"), 
        tr("Excel export requires QXlsx library which is not available.\n"
           "Please export to CSV and open in Excel."));
    return false;
#endif
}

bool ExportResultsDialog::exportToXml() {
    QFile file(fileEdit_->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"), 
            tr("Cannot open file for writing."));
        return false;
    }
    
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);
    
    writer.writeStartDocument();
    writer.writeStartElement("data");
    writer.writeAttribute("exported", QDateTime::currentDateTime().toString(Qt::ISODate));
    
    for (const auto& row : data_) {
        writer.writeStartElement("row");
        for (int i = 0; i < row.size() && i < headers_.size(); ++i) {
            writer.writeTextElement(headers_[i], row[i]);
        }
        writer.writeEndElement(); // row
    }
    
    writer.writeEndElement(); // data
    writer.writeEndDocument();
    
    file.close();
    return true;
}

bool ExportResultsDialog::exportToHtml() {
    QFile file(fileEdit_->text());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Error"), 
            tr("Cannot open file for writing."));
        return false;
    }
    
    QTextStream stream(&file);
    
    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n<head>\n";
    stream << "<meta charset=\"UTF-8\">\n";
    stream << "<title>Exported Data</title>\n";
    stream << "<style>\n";
    stream << "  body { font-family: Arial, sans-serif; margin: 20px; }\n";
    stream << "  table { border-collapse: collapse; width: 100%; }\n";
    stream << "  th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    stream << "  th { background-color: #4CAF50; color: white; }\n";
    stream << "  tr:nth-child(even) { background-color: #f2f2f2; }\n";
    stream << "</style>\n";
    stream << "</head>\n<body>\n";
    stream << "<h1>Exported Data</h1>\n";
    stream << "<table>\n";
    
    // Header
    stream << "  <tr>\n";
    for (const auto& header : headers_) {
        stream << "    <th>" << header << "</th>\n";
    }
    stream << "  </tr>\n";
    
    // Data
    for (const auto& row : data_) {
        stream << "  <tr>\n";
        for (const auto& cell : row) {
            stream << "    <td>" << cell << "</td>\n";
        }
        stream << "  </tr>\n";
    }
    
    stream << "</table>\n";
    stream << "<p><em>Exported from ScratchRobin on " 
           << QDateTime::currentDateTime().toString() << "</em></p>\n";
    stream << "</body>\n</html>\n";
    
    file.close();
    return true;
}

void ExportResultsDialog::onOptionsChanged() {
    // Options changed - could update preview here
}

} // namespace scratchrobin::ui
