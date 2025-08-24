#include "query_history_dialog.h"
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QSettings>
#include <QTimer>
#include <QApplication>
#include <algorithm>
#include <chrono>

namespace scratchrobin {

QueryHistoryDialog::QueryHistoryDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Query History");
    setMinimumSize(800, 600);
    resize(1000, 700);

    // Load some sample data for demonstration
    loadSampleData();
}

void QueryHistoryDialog::setQueryHistory(const QList<QueryHistoryEntry>& history) {
    allHistory_ = history;
    filteredHistory_ = history;
    populateTable();
}

void QueryHistoryDialog::addQueryEntry(const QueryHistoryEntry& entry) {
    allHistory_.prepend(entry); // Add to beginning
    filteredHistory_ = allHistory_;
    populateTable();
}

void QueryHistoryDialog::clearHistory() {
    allHistory_.clear();
    filteredHistory_.clear();
    populateTable();
}

void QueryHistoryDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Query History");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Create main splitter
    QSplitter* mainSplitter = new QSplitter(Qt::Vertical);

    // Top section: Query Table
    QWidget* topWidget = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);

    // Search box
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search:");
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Enter keywords to search in queries...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &QueryHistoryDialog::onSearchTextChanged);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(searchEdit_);
    topLayout->addLayout(searchLayout);

    // Query table
    setupQueryTable();
    topLayout->addWidget(queryTable_);

    mainSplitter->addWidget(topWidget);

    // Bottom section: Query details
    QWidget* bottomWidget = new QWidget();
    QVBoxLayout* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* detailsLabel = new QLabel("Query Details");
    detailsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    bottomLayout->addWidget(detailsLabel);

    // Query text
    QGroupBox* queryGroup = new QGroupBox("SQL Query");
    QVBoxLayout* queryLayout = new QVBoxLayout(queryGroup);
    queryTextEdit_ = new QTextEdit();
    queryTextEdit_->setMaximumHeight(150);
    queryTextEdit_->setFontFamily("monospace");
    queryTextEdit_->setReadOnly(true);
    queryLayout->addWidget(queryTextEdit_);
    bottomLayout->addWidget(queryGroup);

    // Query metadata
    QGroupBox* metadataGroup = new QGroupBox("Query Information");
    QFormLayout* metadataLayout = new QFormLayout(metadataGroup);

    executionTimeLabel_ = new QLabel();
    statusLabel_ = new QLabel();
    rowsAffectedLabel_ = new QLabel();
    timestampLabel_ = new QLabel();

    metadataLayout->addRow("Execution Time:", executionTimeLabel_);
    metadataLayout->addRow("Status:", statusLabel_);
    metadataLayout->addRow("Rows Affected:", rowsAffectedLabel_);
    metadataLayout->addRow("Timestamp:", timestampLabel_);

    bottomLayout->addWidget(metadataGroup);

    // Error details (if any)
    QGroupBox* errorGroup = new QGroupBox("Error Details");
    QVBoxLayout* errorLayout = new QVBoxLayout(errorGroup);
    errorTextEdit_ = new QTextEdit();
    errorTextEdit_->setMaximumHeight(100);
    errorTextEdit_->setFontFamily("monospace");
    errorTextEdit_->setReadOnly(true);
    errorTextEdit_->setVisible(false);
    errorLayout->addWidget(errorTextEdit_);
    bottomLayout->addWidget(errorGroup);

    mainSplitter->addWidget(bottomWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter);

    // Buttons
    setupButtons();
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(reRunButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addWidget(exportButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton_);
    mainLayout->addLayout(buttonLayout);
}

void QueryHistoryDialog::setupQueryTable() {
    queryTable_ = new QTableWidget();
    queryTable_->setColumnCount(4);
    queryTable_->setHorizontalHeaderLabels({
        "Time", "Query", "Status", "Duration"
    });

    queryTable_->horizontalHeader()->setStretchLastSection(true);
    queryTable_->verticalHeader()->setVisible(false);
    queryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queryTable_->setAlternatingRowColors(true);

    connect(queryTable_, &QTableWidget::itemSelectionChanged, this, &QueryHistoryDialog::onQuerySelected);
}

void QueryHistoryDialog::setupButtons() {
    reRunButton_ = new QPushButton("Re-run Query");
    reRunButton_->setIcon(QIcon(":/icons/run.png"));
    connect(reRunButton_, &QPushButton::clicked, this, &QueryHistoryDialog::onReRunQuery);

    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteButton_, &QPushButton::clicked, this, &QueryHistoryDialog::onDeleteQuery);

    exportButton_ = new QPushButton("Export History");
    exportButton_->setIcon(QIcon(":/icons/export.png"));
    connect(exportButton_, &QPushButton::clicked, this, &QueryHistoryDialog::onExportHistory);

    closeButton_ = new QPushButton("Close");
    closeButton_->setDefault(true);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
}

void QueryHistoryDialog::loadSampleData() {
    QList<QueryHistoryEntry> sampleData;

    QueryHistoryEntry entry1;
    entry1.id = "1";
    entry1.sql = "SELECT * FROM users WHERE active = true ORDER BY created_at DESC LIMIT 100";
    entry1.timestamp = std::chrono::system_clock::now() - std::chrono::minutes(5);
    entry1.duration = std::chrono::milliseconds(1250);
    entry1.rowsAffected = 45;
    entry1.success = true;
    sampleData.append(entry1);

    QueryHistoryEntry entry2;
    entry2.id = "2";
    entry2.sql = "INSERT INTO products (name, price, category_id) VALUES ('New Product', 29.99, 5)";
    entry2.timestamp = std::chrono::system_clock::now() - std::chrono::minutes(15);
    entry2.duration = std::chrono::milliseconds(45);
    entry2.rowsAffected = 1;
    entry2.success = true;
    sampleData.append(entry2);

    QueryHistoryEntry entry3;
    entry3.id = "3";
    entry3.sql = "UPDATE users SET last_login = NOW() WHERE id = 123";
    entry3.timestamp = std::chrono::system_clock::now() - std::chrono::hours(2);
    entry3.duration = std::chrono::milliseconds(67);
    entry3.rowsAffected = 1;
    entry3.success = true;
    sampleData.append(entry3);

    QueryHistoryEntry entry4;
    entry4.id = "4";
    entry4.sql = "SELECT * FROM orders WHERE status = 'pending' AND created_at >= '2024-01-01'";
    entry4.timestamp = std::chrono::system_clock::now() - std::chrono::hours(4);
    entry4.duration = std::chrono::milliseconds(2340);
    entry4.rowsAffected = 0;
    entry4.success = false;
    entry4.errorMessage = "Table 'orders' does not exist";
    sampleData.append(entry4);

    setQueryHistory(sampleData);
}

void QueryHistoryDialog::populateTable() {
    queryTable_->setRowCount(0);

    for (int i = 0; i < filteredHistory_.size(); ++i) {
        const QueryHistoryEntry& entry = filteredHistory_[i];
        queryTable_->insertRow(i);

        // Time
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        QString timeStr = QString::fromStdString(std::ctime(&time_t)).trimmed();
        QTableWidgetItem* timeItem = new QTableWidgetItem(timeStr);
        timeItem->setData(Qt::UserRole, QString::fromStdString(entry.id));
        queryTable_->setItem(i, 0, timeItem);

        // Query (truncated)
        std::string queryText = entry.sql.length() > 50 ? entry.sql.substr(0, 50) + "..." : entry.sql;
        QTableWidgetItem* queryItem = new QTableWidgetItem(QString::fromStdString(queryText));
        queryItem->setToolTip(QString::fromStdString(entry.sql));
        queryTable_->setItem(i, 1, queryItem);

        // Status
        QString statusText = entry.success ? "Success" : "Error";
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        if (entry.success) {
            statusItem->setBackground(QColor(200, 255, 200));
        } else {
            statusItem->setBackground(QColor(255, 200, 200));
        }
        queryTable_->setItem(i, 2, statusItem);

        // Duration
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry.duration).count();
        QTableWidgetItem* durationItem = new QTableWidgetItem(QString::number(durationMs) + " ms");
        queryTable_->setItem(i, 3, durationItem);
    }

    queryTable_->resizeColumnsToContents();
    queryTable_->horizontalHeader()->setStretchLastSection(true);
}

void QueryHistoryDialog::showQueryDetails(const QueryHistoryEntry& entry) {
    queryTextEdit_->setText(QString::fromStdString(entry.sql));

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(entry.duration).count();
    executionTimeLabel_->setText(QString::number(durationMs) + " ms");

    statusLabel_->setText(entry.success ? "Success" : "Error");
    rowsAffectedLabel_->setText(QString::number(entry.rowsAffected));

    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    QString timeStr = QString::fromStdString(std::ctime(&time_t)).trimmed();
    timestampLabel_->setText(timeStr);

    if (!entry.errorMessage.empty()) {
        errorTextEdit_->setText(QString::fromStdString(entry.errorMessage));
        errorTextEdit_->parentWidget()->setVisible(true);
    } else {
        errorTextEdit_->parentWidget()->setVisible(false);
    }
}

void QueryHistoryDialog::applyFilters() {
    filteredHistory_.clear();

    for (const QueryHistoryEntry& entry : allHistory_) {
        bool matches = true;

        // Search text filter
        if (!currentSearchText_.isEmpty()) {
            std::string sqlLower = entry.sql;
            std::transform(sqlLower.begin(), sqlLower.end(), sqlLower.begin(), ::tolower);
            std::string searchLower = currentSearchText_.toStdString();
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

            if (sqlLower.find(searchLower) == std::string::npos) {
                matches = false;
            }
        }

        if (matches) {
            filteredHistory_.append(entry);
        }
    }

    populateTable();
}

void QueryHistoryDialog::onSearchTextChanged(const QString& text) {
    currentSearchText_ = text;
    applyFilters();
}

void QueryHistoryDialog::onQuerySelected() {
    QList<QTableWidgetItem*> selectedItems = queryTable_->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    int row = selectedItems.first()->row();
    if (row >= 0 && row < filteredHistory_.size()) {
        currentEntry_ = filteredHistory_[row];
        showQueryDetails(currentEntry_);

        // Update button states
        reRunButton_->setEnabled(true);
        deleteButton_->setEnabled(true);
    }
}

void QueryHistoryDialog::onReRunQuery() {
    if (!currentEntry_.sql.empty()) {
        emit queryReRun(currentEntry_.sql);
        QMessageBox::information(this, "Query Re-run", "Query has been sent to the query editor.");
    }
}

void QueryHistoryDialog::onDeleteQuery() {
    if (currentEntry_.id.empty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Query",
        "Are you sure you want to delete this query from history?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit queryDeleted(currentEntry_.id);

        // Remove from local lists
        for (int i = allHistory_.size() - 1; i >= 0; --i) {
            if (allHistory_[i].id == currentEntry_.id) {
                allHistory_.removeAt(i);
                break;
            }
        }
        applyFilters();
    }
}

void QueryHistoryDialog::onExportHistory() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Query History",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "JSON Files (*.json);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        emit historyExported(fileName);
        QMessageBox::information(this, "Export Complete",
                               "Query history has been exported successfully.");
    }
}

void QueryHistoryDialog::setupQueryDetails() {
    // Query details panel
    queryDetailsWidget_ = new QWidget();
    queryDetailsLayout_ = new QVBoxLayout(queryDetailsWidget_);

    // Query text display
    QGroupBox* queryGroup = new QGroupBox("Query");
    QVBoxLayout* queryLayout = new QVBoxLayout(queryGroup);
    queryTextEdit_ = new QTextEdit();
    queryTextEdit_->setReadOnly(true);
    queryTextEdit_->setMaximumHeight(150);
    queryLayout->addWidget(queryTextEdit_);
    queryDetailsLayout_->addWidget(queryGroup);

    // Statistics
    QGroupBox* statsGroup = new QGroupBox("Statistics");
    QGridLayout* statsLayout = new QGridLayout(statsGroup);

    QLabel* execTimeLabel = new QLabel("Execution Time:");
    executionTimeLabel_ = new QLabel("N/A");
    statsLayout->addWidget(execTimeLabel, 0, 0);
    statsLayout->addWidget(executionTimeLabel_, 0, 1);

    QLabel* statusLabel = new QLabel("Status:");
    statusLabel_ = new QLabel("N/A");
    statsLayout->addWidget(statusLabel, 1, 0);
    statsLayout->addWidget(statusLabel_, 1, 1);

    QLabel* rowsLabel = new QLabel("Rows Affected:");
    rowsAffectedLabel_ = new QLabel("N/A");
    statsLayout->addWidget(rowsLabel, 2, 0);
    statsLayout->addWidget(rowsAffectedLabel_, 2, 1);

    QLabel* timestampLabel = new QLabel("Timestamp:");
    timestampLabel_ = new QLabel("N/A");
    statsLayout->addWidget(timestampLabel, 3, 0);
    statsLayout->addWidget(timestampLabel_, 3, 1);

    queryDetailsLayout_->addWidget(statsGroup);

    // Error details (if any)
    errorTextEdit_ = new QTextEdit();
    errorTextEdit_->setReadOnly(true);
    errorTextEdit_->setMaximumHeight(100);
    errorTextEdit_->setVisible(false);
    queryDetailsLayout_->addWidget(errorTextEdit_);

    // Add to details tab
    detailsTab_->layout()->addWidget(queryDetailsWidget_);
}

void QueryHistoryDialog::setupStatisticsTab() {
    statisticsTab_ = new QWidget();
    QVBoxLayout* statsLayout = new QVBoxLayout(statisticsTab_);

    // Statistics widgets
    QGroupBox* generalStats = new QGroupBox("General Statistics");
    QFormLayout* generalLayout = new QFormLayout(generalStats);

    totalQueriesLabel_ = new QLabel("0");
    generalLayout->addRow("Total Queries:", totalQueriesLabel_);

    successRateLabel_ = new QLabel("0%");
    generalLayout->addRow("Success Rate:", successRateLabel_);

    avgExecutionTimeLabel_ = new QLabel("0 ms");
    generalLayout->addRow("Average Execution Time:", avgExecutionTimeLabel_);

    mostFrequentTypeLabel_ = new QLabel("N/A");
    generalLayout->addRow("Most Frequent Type:", mostFrequentTypeLabel_);

    statsLayout->addWidget(generalStats);

    // Time-based statistics
    QGroupBox* timeStats = new QGroupBox("Time-based Statistics");
    QFormLayout* timeLayout = new QFormLayout(timeStats);

    queriesTodayLabel_ = new QLabel("0");
    timeLayout->addRow("Queries Today:", queriesTodayLabel_);

    queriesThisWeekLabel_ = new QLabel("0");
    timeLayout->addRow("Queries This Week:", queriesThisWeekLabel_);

    statsLayout->addWidget(timeStats);
    statsLayout->addStretch();

    // Add to main tab widget
    tabWidget_->addTab(statisticsTab_, "Statistics");
}

void QueryHistoryDialog::onFilterChanged() {
    currentFavoritesOnly_ = favoritesOnlyCheck_->isChecked();
    applyFilters();
}

void QueryHistoryDialog::onDateFilterChanged() {
    currentStartDate_ = startDateEdit_->dateTime();
    currentEndDate_ = endDateEdit_->dateTime();
    applyFilters();
}

void QueryHistoryDialog::onStatusFilterChanged() {
    currentStatusFilter_ = statusFilter_->currentText();
    applyFilters();
}

void QueryHistoryDialog::onTypeFilterChanged() {
    currentTypeFilter_ = typeFilter_->currentText();
    applyFilters();
}

} // namespace scratchrobin
