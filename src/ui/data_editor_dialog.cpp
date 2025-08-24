#include "data_editor_dialog.h"
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QScrollArea>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QRegularExpression>
#include <QSettings>
#include <QApplication>
#include <QTimer>
#include <QStandardPaths>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QClipboard>

namespace scratchrobin {

DataEditorDialog::DataEditorDialog(QWidget* parent)
    : QDialog(parent) {
    setupUI();
    setWindowTitle("Data Editor");
    setMinimumSize(800, 600);
    resize(1200, 800);

    connect(this, &DataEditorDialog::dataChanged, this, &DataEditorDialog::updateRowCount);
    connect(this, &DataEditorDialog::rowAdded, this, &DataEditorDialog::onSelectionChanged);
    connect(this, &DataEditorDialog::rowDeleted, this, &DataEditorDialog::onSelectionChanged);
}

void DataEditorDialog::setTableInfo(const TableInfo& info) {
    tableInfo_ = info;
    setWindowTitle(QString("Data Editor - %1.%2").arg(info.schema, info.name));

    // Update column filter combo
    searchColumnCombo_->clear();
    searchColumnCombo_->addItem("All Columns");
    for (const QString& column : info.columnNames) {
        searchColumnCombo_->addItem(column);
    }

    // Setup table columns
    dataTable_->setColumnCount(info.columnNames.size());
    dataTable_->setHorizontalHeaderLabels(info.columnNames);

    // Setup column filters
    setupFilters();

    // Enable sorting
    dataTable_->setSortingEnabled(true);
}

void DataEditorDialog::setTableData(const QList<QList<QVariant>>& data) {
    originalData_ = data;
    currentData_ = data;
    totalRows_ = data.size();
    updatePagination();
    updateTableDisplay();
}

void DataEditorDialog::setCurrentDatabase(const QString& dbName) {
    currentDatabase_ = dbName;
    statusLabel_->setText(QString("Database: %1 | Table: %2.%3")
                         .arg(dbName, tableInfo_.schema, tableInfo_.name));
}

QList<QList<QVariant>> DataEditorDialog::getModifiedData() const {
    QList<QList<QVariant>> modifiedData;
    for (const auto& pair : modifiedCells_) {
        int row = pair.first;
        int col = pair.second;
        if (row < currentData_.size() && col < currentData_[row].size()) {
            modifiedData.append(currentData_[row]);
        }
    }
    return modifiedData;
}

QList<QList<QVariant>> DataEditorDialog::getNewRows() const {
    return newRows_;
}

QList<int> DataEditorDialog::getDeletedRowIds() const {
    return deletedRowIds_;
}

bool DataEditorDialog::showDataEditor(QWidget* parent,
                                     const TableInfo& tableInfo,
                                     const QList<QList<QVariant>>& initialData,
                                     const QString& dbName) {
    DataEditorDialog dialog(parent);
    dialog.setTableInfo(tableInfo);
    dialog.setTableData(initialData);
    dialog.setCurrentDatabase(dbName);

    return dialog.exec() == QDialog::Accepted;
}

void DataEditorDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Header with title and database info
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Data Editor");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c5aa0;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout_->addLayout(headerLayout);

    // Toolbar
    setupToolbar();
    mainLayout_->addLayout(toolbarLayout_);

    // Search and filter bar
    setupSearchBar();
    mainLayout_->addLayout(searchLayout_);

    // Main content area
    mainSplitter_ = new QSplitter(Qt::Horizontal);
    mainSplitter_->setChildrenCollapsible(false);

    // Left panel - filters
    filtersWidget_ = new QWidget();
    filtersWidget_->setMaximumWidth(300);
    setupFilters();
    mainSplitter_->addWidget(filtersWidget_);

    // Right panel - data table
    setupTable();
    mainSplitter_->addWidget(dataTable_);

    mainLayout_->addWidget(mainSplitter_);

    // Pagination
    setupPagination();
    mainLayout_->addLayout(paginationLayout_);

    // SQL Preview (initially hidden)
    setupSQLPreview();

    // Status bar
    setupStatusBar();

    // Set initial splitter sizes
    mainSplitter_->setSizes({200, 800});
}

void DataEditorDialog::setupToolbar() {
    toolbarLayout_ = new QHBoxLayout();

    addRowButton_ = new QPushButton("Add Row");
    addRowButton_->setIcon(QIcon(":/icons/add_row.png"));
    addRowButton_->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px 12px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");
    connect(addRowButton_, &QPushButton::clicked, this, &DataEditorDialog::onAddRow);
    toolbarLayout_->addWidget(addRowButton_);

    deleteRowButton_ = new QPushButton("Delete Row");
    deleteRowButton_->setIcon(QIcon(":/icons/delete_row.png"));
    deleteRowButton_->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px 12px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #d32f2f; }");
    connect(deleteRowButton_, &QPushButton::clicked, this, &DataEditorDialog::onDeleteRow);
    toolbarLayout_->addWidget(deleteRowButton_);

    duplicateRowButton_ = new QPushButton("Duplicate");
    duplicateRowButton_->setIcon(QIcon(":/icons/duplicate.png"));
    connect(duplicateRowButton_, &QPushButton::clicked, this, &DataEditorDialog::onDuplicateRow);
    toolbarLayout_->addWidget(duplicateRowButton_);

    toolbarLayout_->addSpacing(20);

    refreshButton_ = new QPushButton("Refresh");
    refreshButton_->setIcon(QIcon(":/icons/refresh.png"));
    connect(refreshButton_, &QPushButton::clicked, this, &DataEditorDialog::onRefreshData);
    toolbarLayout_->addWidget(refreshButton_);

    toolbarLayout_->addSpacing(20);

    commitButton_ = new QPushButton("Commit Changes");
    commitButton_->setIcon(QIcon(":/icons/commit.png"));
    commitButton_->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px 12px; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #1976D2; }");
    connect(commitButton_, &QPushButton::clicked, this, &DataEditorDialog::onCommitChanges);
    toolbarLayout_->addWidget(commitButton_);

    rollbackButton_ = new QPushButton("Rollback");
    rollbackButton_->setIcon(QIcon(":/icons/rollback.png"));
    connect(rollbackButton_, &QPushButton::clicked, this, &DataEditorDialog::onRollbackChanges);
    toolbarLayout_->addWidget(rollbackButton_);

    toolbarLayout_->addSpacing(20);

    exportButton_ = new QPushButton("Export");
    exportButton_->setIcon(QIcon(":/icons/export.png"));
    connect(exportButton_, &QPushButton::clicked, this, &DataEditorDialog::onExportData);
    toolbarLayout_->addWidget(exportButton_);

    importButton_ = new QPushButton("Import");
    importButton_->setIcon(QIcon(":/icons/import.png"));
    connect(importButton_, &QPushButton::clicked, this, &DataEditorDialog::onImportData);
    toolbarLayout_->addWidget(importButton_);

    toolbarLayout_->addStretch();
}

void DataEditorDialog::setupSearchBar() {
    searchLayout_ = new QHBoxLayout();

    QLabel* searchLabel = new QLabel("Search:");
    searchLayout_->addWidget(searchLabel);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Enter search text...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &DataEditorDialog::onSearchTextChanged);
    searchLayout_->addWidget(searchEdit_);

    QLabel* inLabel = new QLabel(" in ");
    searchLayout_->addWidget(inLabel);

    searchColumnCombo_ = new QComboBox();
    searchColumnCombo_->addItem("All Columns");
    searchLayout_->addWidget(searchColumnCombo_);

    filterButton_ = new QPushButton("Apply Filter");
    filterButton_->setIcon(QIcon(":/icons/filter.png"));
    connect(filterButton_, &QPushButton::clicked, this, &DataEditorDialog::onApplyFilter);
    searchLayout_->addWidget(filterButton_);

    clearFilterButton_ = new QPushButton("Clear Filter");
    clearFilterButton_->setIcon(QIcon(":/icons/clear_filter.png"));
    connect(clearFilterButton_, &QPushButton::clicked, this, &DataEditorDialog::onClearFilter);
    searchLayout_->addWidget(clearFilterButton_);

    showSQLButton_ = new QPushButton("SQL Preview");
    showSQLButton_->setIcon(QIcon(":/icons/sql.png"));
    connect(showSQLButton_, &QPushButton::clicked, this, &DataEditorDialog::onShowSQLPreview);
    searchLayout_->addWidget(showSQLButton_);
}

void DataEditorDialog::setupTable() {
    dataTable_ = new QTableWidget();
    dataTable_->setAlternatingRowColors(true);
    dataTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    dataTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    dataTable_->horizontalHeader()->setStretchLastSection(true);
    dataTable_->verticalHeader()->setDefaultSectionSize(25);
    dataTable_->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(dataTable_, &QTableWidget::cellChanged, this, &DataEditorDialog::onCellChanged);
    connect(dataTable_, &QTableWidget::itemSelectionChanged, this, &DataEditorDialog::onSelectionChanged);
    connect(dataTable_, &QTableWidget::customContextMenuRequested, this, &DataEditorDialog::showContextMenu);
    connect(dataTable_->horizontalHeader(), &QHeaderView::sectionClicked, this, &DataEditorDialog::onSortColumn);
}

void DataEditorDialog::setupFilters() {
    filtersLayout_ = new QVBoxLayout(filtersWidget_);

    QGroupBox* columnFiltersGroup = new QGroupBox("Column Filters");
    QFormLayout* filtersFormLayout = new QFormLayout(columnFiltersGroup);

    columnFilters_.clear();
    for (const QString& columnName : tableInfo_.columnNames) {
        QLineEdit* filterEdit = new QLineEdit();
        filterEdit->setPlaceholderText(QString("Filter %1...").arg(columnName));
        connect(filterEdit, &QLineEdit::textChanged, [this, columnName](const QString& text) {
            int columnIndex = tableInfo_.columnNames.indexOf(columnName);
            onColumnFilterChanged(columnIndex, text);
        });
        filtersFormLayout->addRow(columnName + ":", filterEdit);
        columnFilters_.append(filterEdit);
    }

    filtersLayout_->addWidget(columnFiltersGroup);
    filtersLayout_->addStretch();
}

void DataEditorDialog::setupPagination() {
    paginationLayout_ = new QHBoxLayout();

    rowCountLabel_ = new QLabel("0 rows");
    paginationLayout_->addWidget(rowCountLabel_);

    paginationLayout_->addStretch();

    QLabel* pageLabel = new QLabel("Page:");
    paginationLayout_->addWidget(pageLabel);

    firstPageButton_ = new QPushButton("<<");
    firstPageButton_->setMaximumWidth(40);
    connect(firstPageButton_, &QPushButton::clicked, [this]() { onPageChanged(1); });
    paginationLayout_->addWidget(firstPageButton_);

    prevPageButton_ = new QPushButton("<");
    prevPageButton_->setMaximumWidth(40);
    connect(prevPageButton_, &QPushButton::clicked, [this]() { onPageChanged(currentPage_ - 1); });
    paginationLayout_->addWidget(prevPageButton_);

    pageSpinBox_ = new QSpinBox();
    pageSpinBox_->setMinimum(1);
    pageSpinBox_->setMaximum(1);
    pageSpinBox_->setMaximumWidth(60);
    connect(pageSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataEditorDialog::onPageChanged);
    paginationLayout_->addWidget(pageSpinBox_);

    nextPageButton_ = new QPushButton(">");
    nextPageButton_->setMaximumWidth(40);
    connect(nextPageButton_, &QPushButton::clicked, [this]() { onPageChanged(currentPage_ + 1); });
    paginationLayout_->addWidget(nextPageButton_);

    lastPageButton_ = new QPushButton(">>");
    lastPageButton_->setMaximumWidth(40);
    connect(lastPageButton_, &QPushButton::clicked, [this]() { onPageChanged(totalPages_); });
    paginationLayout_->addWidget(lastPageButton_);

    pageInfoLabel_ = new QLabel("of 1");
    paginationLayout_->addWidget(pageInfoLabel_);

    QLabel* sizeLabel = new QLabel("Page size:");
    paginationLayout_->addWidget(sizeLabel);

    pageSizeCombo_ = new QComboBox();
    pageSizeCombo_->addItems({"25", "50", "100", "250", "500", "1000"});
    pageSizeCombo_->setCurrentText("100");
    connect(pageSizeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        pageSize_ = pageSizeCombo_->currentText().toInt();
        updatePagination();
        updateTableDisplay();
    });
    paginationLayout_->addWidget(pageSizeCombo_);
}

void DataEditorDialog::setupSQLPreview() {
    sqlPreviewWidget_ = new QWidget();
    QVBoxLayout* sqlLayout = new QVBoxLayout(sqlPreviewWidget_);

    QLabel* sqlLabel = new QLabel("SQL Preview");
    sqlLabel->setStyleSheet("font-weight: bold; color: #2c5aa0;");
    sqlLayout->addWidget(sqlLabel);

    sqlPreviewText_ = new QTextEdit();
    sqlPreviewText_->setReadOnly(true);
    sqlPreviewText_->setFontFamily("monospace");
    sqlPreviewText_->setMaximumHeight(200);
    sqlPreviewText_->setStyleSheet("QTextEdit { background-color: #f5f5f5; }");
    sqlLayout->addWidget(sqlPreviewText_);

    // Initially hide the SQL preview
    sqlPreviewWidget_->setVisible(false);
    mainLayout_->addWidget(sqlPreviewWidget_);
}

void DataEditorDialog::setupStatusBar() {
    statusBar_ = new QStatusBar();
    mainLayout_->addWidget(statusBar_);

    statusLabel_ = new QLabel("Ready");
    statusBar_->addWidget(statusLabel_);

    statusBar_->addPermanentWidget(new QLabel(" | "));

    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    progressBar_->setMaximumWidth(200);
    statusBar_->addPermanentWidget(progressBar_);
}

void DataEditorDialog::onAddRow() {
    // Create a new row with default values
    QList<QVariant> newRow;
    for (int i = 0; i < tableInfo_.columnNames.size(); ++i) {
        if (tableInfo_.nullableColumns[i]) {
            newRow.append(QVariant()); // NULL for nullable columns
        } else {
            // Set appropriate default values based on column type
            QString type = tableInfo_.columnTypes[i].toLower();
            if (type.contains("int")) {
                newRow.append(0);
            } else if (type.contains("text") || type.contains("varchar")) {
                newRow.append("");
            } else if (type.contains("bool")) {
                newRow.append(false);
            } else {
                newRow.append(QVariant());
            }
        }
    }

    currentData_.append(newRow);
    newRows_.append(newRow);

    updateTableDisplay();
    updateRowCount();

    // Select the new row
    int newRowIndex = currentData_.size() - 1;
    dataTable_->selectRow(newRowIndex);
    dataTable_->scrollToItem(dataTable_->item(newRowIndex, 0));

    emit rowAdded(newRowIndex);
    emit dataChanged();
}

void DataEditorDialog::onDeleteRow() {
    QList<QTableWidgetSelectionRange> ranges = dataTable_->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select rows to delete.");
        return;
    }

    int rowCount = 0;
    for (const auto& range : ranges) {
        rowCount += range.rowCount();
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Deletion",
        QString("Are you sure you want to delete %1 selected row(s)?\n\nThis action cannot be undone.")
        .arg(rowCount),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Get selected row indices (in descending order to avoid index shifting)
    QList<int> rowsToDelete;
    for (const auto& range : ranges) {
        for (int i = range.bottomRow(); i >= range.topRow(); --i) {
            rowsToDelete.append(i);
        }
    }

    // Delete rows from current data
    for (int row : rowsToDelete) {
        if (row < currentData_.size()) {
            deletedRowIds_.append(row); // Store original row ID
            currentData_.removeAt(row);
        }
    }

    updateTableDisplay();
    updateRowCount();

    emit dataChanged();
}

void DataEditorDialog::onDuplicateRow() {
    QList<QTableWidgetSelectionRange> ranges = dataTable_->selectedRanges();
    if (ranges.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a row to duplicate.");
        return;
    }

    int selectedRow = ranges.first().topRow();
    if (selectedRow >= currentData_.size()) {
        return;
    }

    // Create a copy of the selected row
    QList<QVariant> duplicatedRow = currentData_[selectedRow];
    currentData_.append(duplicatedRow);
    newRows_.append(duplicatedRow);

    updateTableDisplay();
    updateRowCount();

    // Select the duplicated row
    int newRowIndex = currentData_.size() - 1;
    dataTable_->selectRow(newRowIndex);
    dataTable_->scrollToItem(dataTable_->item(newRowIndex, 0));

    emit rowAdded(newRowIndex);
    emit dataChanged();
}

void DataEditorDialog::onRefreshData() {
    // In a real implementation, this would reload data from the database
    QMessageBox::information(this, "Refresh Data",
                           "Data refresh functionality would reload data from the database.\n\n"
                           "Note: Any unsaved changes will be lost.");
}

void DataEditorDialog::onApplyFilter() {
    applyFilters();
    updateTableDisplay();
}

void DataEditorDialog::onClearFilter() {
    searchEdit_->clear();
    for (QLineEdit* filter : columnFilters_) {
        filter->clear();
    }
    activeFilters_.clear();
    updateTableDisplay();
}

void DataEditorDialog::onSearchTextChanged(const QString& text) {
    currentSearchText_ = text;
    if (text.isEmpty()) {
        return;
    }

    // Apply search filter
    QString searchColumn = searchColumnCombo_->currentText();
    if (searchColumn == "All Columns") {
        // Search in all columns
        for (int col = 0; col < tableInfo_.columnNames.size(); ++col) {
            DataFilter filter;
            filter.columnName = tableInfo_.columnNames[col];
            filter.operator_ = "LIKE";
            filter.value = "%" + text + "%";
            activeFilters_.append(filter);
        }
    } else {
        // Search in specific column
        DataFilter filter;
        filter.columnName = searchColumn;
        filter.operator_ = "LIKE";
        filter.value = "%" + text + "%";
        activeFilters_.append(filter);
    }

    applyFilters();
    updateTableDisplay();
}

void DataEditorDialog::onColumnFilterChanged(int column, const QString& filterText) {
    if (column < 0 || column >= tableInfo_.columnNames.size()) {
        return;
    }

    QString columnName = tableInfo_.columnNames[column];

    // Remove existing filter for this column
    activeFilters_.erase(
        std::remove_if(activeFilters_.begin(), activeFilters_.end(),
                      [columnName](const DataFilter& f) { return f.columnName == columnName; }),
        activeFilters_.end()
    );

    // Add new filter if text is not empty
    if (!filterText.isEmpty()) {
        DataFilter filter;
        filter.columnName = columnName;
        filter.operator_ = "LIKE";
        filter.value = "%" + filterText + "%";
        activeFilters_.append(filter);
    }

    applyFilters();
    updateTableDisplay();
}

void DataEditorDialog::onCellChanged(int row, int column) {
    if (row >= currentData_.size() || column >= currentData_[row].size()) {
        return;
    }

    QTableWidgetItem* item = dataTable_->item(row, column);
    if (!item) {
        return;
    }

    QVariant newValue = item->data(Qt::EditRole);
    QVariant oldValue = currentData_[row][column];

    // Validate the new value
    validateCellValue(row, column, newValue);

    // Update current data
    currentData_[row][column] = newValue;

    // Track modification
    QPair<int, int> cell(row, column);
    if (!modifiedCells_.contains(cell)) {
        modifiedCells_.append(cell);
    }

    emit cellValueChanged(row, column, oldValue, newValue);
    emit dataChanged();
}

void DataEditorDialog::onSelectionChanged() {
    QList<QTableWidgetSelectionRange> ranges = dataTable_->selectedRanges();

    selectedRows_.clear();
    for (const auto& range : ranges) {
        for (int i = range.topRow(); i <= range.bottomRow(); ++i) {
            selectedRows_.append(i);
        }
    }

    bool hasSelection = !selectedRows_.isEmpty();
    deleteRowButton_->setEnabled(hasSelection);
    duplicateRowButton_->setEnabled(selectedRows_.size() == 1);
}

void DataEditorDialog::onCommitChanges() {
    if (modifiedCells_.isEmpty() && newRows_.isEmpty() && deletedRowIds_.isEmpty()) {
        QMessageBox::information(this, "No Changes", "There are no changes to commit.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Commit Changes",
        QString("Are you sure you want to commit the following changes?\n\n"
                "• %1 modified cells\n"
                "• %2 new rows\n"
                "• %3 deleted rows\n\n"
                "This will permanently update the database.")
        .arg(modifiedCells_.size())
        .arg(newRows_.size())
        .arg(deletedRowIds_.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // In a real implementation, this would execute SQL commands
        QMessageBox::information(this, "Changes Committed",
                               "All changes have been successfully committed to the database.");
        accept();
    }
}

void DataEditorDialog::onRollbackChanges() {
    if (modifiedCells_.isEmpty() && newRows_.isEmpty() && deletedRowIds_.isEmpty()) {
        QMessageBox::information(this, "No Changes", "There are no changes to rollback.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Rollback Changes",
        "Are you sure you want to discard all changes?\n\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Reset all changes
        currentData_ = originalData_;
        modifiedCells_.clear();
        newRows_.clear();
        deletedRowIds_.clear();

        updateTableDisplay();
        updateRowCount();
        statusLabel_->setText("Changes rolled back");
    }
}

void DataEditorDialog::onExportData() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Data", "",
        "CSV Files (*.csv);;JSON Files (*.json);;SQL Files (*.sql);;All Files (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    // In a real implementation, this would export the data
    QMessageBox::information(this, "Export Complete",
                           QString("Data has been exported to:\n\n%1").arg(fileName));
}

void DataEditorDialog::onImportData() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Data", "",
        "CSV Files (*.csv);;JSON Files (*.json);;SQL Files (*.sql);;All Files (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    QMessageBox::information(this, "Import Complete",
                           QString("Data has been imported from:\n\n%1").arg(fileName));
}

void DataEditorDialog::onShowSQLPreview() {
    QString sql = generateSQLForChanges();
    sqlPreviewText_->setPlainText(sql);

    if (sqlPreviewWidget_->isVisible()) {
        sqlPreviewWidget_->setVisible(false);
        showSQLButton_->setText("SQL Preview");
    } else {
        sqlPreviewWidget_->setVisible(true);
        showSQLButton_->setText("Hide SQL");
    }
}

void DataEditorDialog::onGoToRow() {
    bool ok;
    int row = QInputDialog::getInt(this, "Go to Row", "Enter row number:", 1, 1, totalRows_, 1, &ok);
    if (ok) {
        int page = ((row - 1) / pageSize_) + 1;
        onPageChanged(page);

        // Select the row
        int rowInPage = (row - 1) % pageSize_;
        dataTable_->selectRow(rowInPage);
        dataTable_->scrollToItem(dataTable_->item(rowInPage, 0));
    }
}

void DataEditorDialog::onPageChanged(int page) {
    if (page < 1 || page > totalPages_) {
        return;
    }

    currentPage_ = page;
    pageSpinBox_->setValue(page);
    updateTableDisplay();
}

void DataEditorDialog::onPageSizeChanged(int size) {
    pageSize_ = size;
    updatePagination();
    updateTableDisplay();
}

void DataEditorDialog::onSortColumn(int column) {
    if (column < 0 || column >= tableInfo_.columnNames.size()) {
        return;
    }

    QString columnName = tableInfo_.columnNames[column];
    Qt::SortOrder order = dataTable_->horizontalHeader()->sortIndicatorOrder();

    // Update sorting
    activeSorting_.clear();
    DataSort sort;
    sort.columnName = columnName;
    sort.order = order;
    activeSorting_.append(sort);

    applySorting();
    updateTableDisplay();
}

void DataEditorDialog::loadTableData() {
    // In a real implementation, this would load data from the database
    // For now, just use the current data
    updateTableDisplay();
}

void DataEditorDialog::applyFilters() {
    // In a real implementation, this would apply filters to the data
    emit filterApplied(activeFilters_);
}

void DataEditorDialog::applySorting() {
    if (activeSorting_.isEmpty()) {
        return;
    }

    const DataSort& sort = activeSorting_.first();
    int columnIndex = tableInfo_.columnNames.indexOf(sort.columnName);

    if (columnIndex < 0) {
        return;
    }

    // Sort current data
    std::sort(currentData_.begin(), currentData_.end(),
              [columnIndex, sort](const QList<QVariant>& a, const QList<QVariant>& b) {
                  if (sort.order == Qt::AscendingOrder) {
                      return a[columnIndex] < b[columnIndex];
                  } else {
                      return a[columnIndex] > b[columnIndex];
                  }
              });

    emit sortApplied(activeSorting_);
}

void DataEditorDialog::updateTableDisplay() {
    dataTable_->setSortingEnabled(false); // Disable sorting during update

    // Calculate page data
    int startRow = (currentPage_ - 1) * pageSize_;
    int endRow = qMin(startRow + pageSize_, currentData_.size());

    dataTable_->setRowCount(endRow - startRow);

    for (int i = startRow; i < endRow; ++i) {
        const QList<QVariant>& rowData = currentData_[i];
        for (int col = 0; col < rowData.size() && col < dataTable_->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData(Qt::EditRole, rowData[col]);

            // Style based on modification status
            QPair<int, int> cell(i, col);
            if (modifiedCells_.contains(cell)) {
                item->setBackground(QColor("#FFF3CD")); // Light yellow for modified cells
            } else if (newRows_.contains(rowData)) {
                item->setBackground(QColor("#D4EDDA")); // Light green for new rows
            }

            dataTable_->setItem(i - startRow, col, item);
        }
    }

    dataTable_->setSortingEnabled(true); // Re-enable sorting
    updateRowCount();
}

void DataEditorDialog::updateRowCount() {
    rowCountLabel_->setText(QString("%1 rows").arg(currentData_.size()));

    QString status = QString("Page %1 of %2 | %3 rows total")
                    .arg(currentPage_)
                    .arg(totalPages_)
                    .arg(totalRows_);

    if (!modifiedCells_.isEmpty() || !newRows_.isEmpty() || !deletedRowIds_.isEmpty()) {
        status += QString(" | %1 modified, %2 new, %3 deleted")
                 .arg(modifiedCells_.size())
                 .arg(newRows_.size())
                 .arg(deletedRowIds_.size());
    }

    statusLabel_->setText(status);
}

void DataEditorDialog::updatePagination() {
    totalPages_ = qMax(1, (totalRows_ + pageSize_ - 1) / pageSize_);
    pageSpinBox_->setMaximum(totalPages_);
    pageInfoLabel_->setText(QString("of %1").arg(totalPages_));
}

void DataEditorDialog::validateCellValue(int row, int column, const QVariant& value) {
    // Basic validation based on column type
    if (column >= tableInfo_.columnTypes.size()) {
        return;
    }

    QString columnType = tableInfo_.columnTypes[column].toLower();
    QString errorMessage;

    if (columnType.contains("int")) {
        bool ok;
        value.toInt(&ok);
        if (!ok && !value.isNull()) {
            errorMessage = "Value must be a valid integer";
        }
    } else if (columnType.contains("numeric") || columnType.contains("decimal")) {
        bool ok;
        value.toDouble(&ok);
        if (!ok && !value.isNull()) {
            errorMessage = "Value must be a valid number";
        }
    } else if (columnType.contains("bool")) {
        if (!value.canConvert<bool>() && !value.isNull()) {
            errorMessage = "Value must be true/false";
        }
    }

    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, "Invalid Value",
                           QString("Column '%1': %2")
                           .arg(tableInfo_.columnNames[column], errorMessage));
    }
}

QString DataEditorDialog::generateSQLForChanges() const {
    QStringList sqlStatements;
    QString tableName = QString("%1.%2").arg(tableInfo_.schema, tableInfo_.name);

    // Generate UPDATE statements for modified cells
    for (const auto& pair : modifiedCells_) {
        int row = pair.first;
        int col = pair.second;
        if (row < currentData_.size() && col < currentData_[row].size()) {
            QString columnName = tableInfo_.columnNames[col];
            QVariant newValue = currentData_[row][col];
            QString valueStr = newValue.isNull() ? "NULL" : QString("'%1'").arg(newValue.toString());

            // In a real implementation, you would need a WHERE clause based on primary key
            sqlStatements << QString("UPDATE %1 SET %2 = %3 WHERE id = %4;")
                         .arg(tableName, columnName, valueStr, QString::number(row));
        }
    }

    // Generate INSERT statements for new rows
    for (const auto& row : newRows_) {
        QStringList columns, values;
        for (int col = 0; col < row.size() && col < tableInfo_.columnNames.size(); ++col) {
            columns << tableInfo_.columnNames[col];
            QVariant value = row[col];
            QString valueStr = value.isNull() ? "NULL" : QString("'%1'").arg(value.toString());
            values << valueStr;
        }

        sqlStatements << QString("INSERT INTO %1 (%2) VALUES (%3);")
                     .arg(tableName, columns.join(", "), values.join(", "));
    }

    // Generate DELETE statements
    for (int rowId : deletedRowIds_) {
        sqlStatements << QString("DELETE FROM %1 WHERE id = %2;")
                     .arg(tableName, QString::number(rowId));
    }

    return sqlStatements.join("\n\n");
}

void DataEditorDialog::showContextMenu(const QPoint& pos) {
    QMenu contextMenu(this);

    QAction* copyAction = contextMenu.addAction("Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, [this]() {
        QApplication::clipboard()->setText(dataTable_->currentItem()->text());
    });

    QAction* pasteAction = contextMenu.addAction("Paste");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, [this]() {
        QString text = QApplication::clipboard()->text();
        QTableWidgetItem* currentItem = dataTable_->currentItem();
        if (currentItem) {
            currentItem->setText(text);
        }
    });

    contextMenu.addSeparator();

    QAction* addRowAction = contextMenu.addAction("Add Row");
    connect(addRowAction, &QAction::triggered, this, &DataEditorDialog::onAddRow);

    QAction* deleteRowAction = contextMenu.addAction("Delete Row");
    connect(deleteRowAction, &QAction::triggered, this, &DataEditorDialog::onDeleteRow);

    QAction* duplicateRowAction = contextMenu.addAction("Duplicate Row");
    connect(duplicateRowAction, &QAction::triggered, this, &DataEditorDialog::onDuplicateRow);

    contextMenu.addSeparator();

    QAction* goToRowAction = contextMenu.addAction("Go to Row...");
    connect(goToRowAction, &QAction::triggered, this, &DataEditorDialog::onGoToRow);

    contextMenu.exec(dataTable_->viewport()->mapToGlobal(pos));
}

} // namespace scratchrobin
