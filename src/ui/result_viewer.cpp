#include "result_viewer.h"
#include "execution/sql_executor.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>

namespace scratchrobin {

class ResultViewer::Impl : public QWidget {
public:
    Impl() {
        setupUI();
    }

    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        // Info label
        infoLabel_ = new QLabel("No results to display");
        infoLabel_->setAlignment(Qt::AlignCenter);
        layout->addWidget(infoLabel_);

        // Results table
        resultsTable_ = new QTableWidget();
        resultsTable_->setAlternatingRowColors(true);
        resultsTable_->horizontalHeader()->setStretchLastSection(true);
        resultsTable_->setVisible(false);
        layout->addWidget(resultsTable_);
    }

    void setResults(const QueryResult& results) {
        if (results.columnNames.empty()) {
            infoLabel_->setText("No results to display");
            resultsTable_->setVisible(false);
            return;
        }

        // Set up table columns
        resultsTable_->setColumnCount(results.columnNames.size());
        QStringList headerLabels;
        for (const auto& columnName : results.columnNames) {
            headerLabels << QString::fromStdString(columnName);
        }
        resultsTable_->setHorizontalHeaderLabels(headerLabels);

        // Set up table rows
        resultsTable_->setRowCount(results.rows.size());
        for (size_t row = 0; row < results.rows.size(); ++row) {
            for (size_t col = 0; col < results.rows[row].size(); ++col) {
                QString cellValue = results.rows[row][col].toString();
                resultsTable_->setItem(row, col, new QTableWidgetItem(cellValue));
            }
        }

        infoLabel_->setText(QString("Query executed successfully - %1 rows returned").arg(results.rows.size()));
        resultsTable_->setVisible(true);
    }

    void clearResults() {
        resultsTable_->clear();
        resultsTable_->setRowCount(0);
        resultsTable_->setColumnCount(0);
        resultsTable_->setVisible(false);
        infoLabel_->setText("No results to display");
    }

private:
    QLabel* infoLabel_;
    QTableWidget* resultsTable_;
};

ResultViewer::ResultViewer()
    : impl_(std::make_unique<Impl>()) {
}

ResultViewer::~ResultViewer() = default;

void ResultViewer::setResults(const QueryResult& results) {
    impl_->setResults(results);
}

void ResultViewer::clearResults() {
    impl_->clearResults();
}

void ResultViewer::exportResults(const std::string& format, const std::string& filename) {
    // TODO: Implement result export
}

QWidget* ResultViewer::getWidget() {
    return impl_.get();
}

} // namespace scratchrobin