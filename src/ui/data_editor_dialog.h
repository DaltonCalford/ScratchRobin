#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QProgressBar>
#include <QStatusBar>

namespace scratchrobin {

struct TableInfo {
    QString name;
    QString schema;
    QStringList columnNames;
    QStringList columnTypes;
    QList<bool> nullableColumns;
    QStringList primaryKeyColumns;
};

struct DataFilter {
    QString columnName;
    QString operator_; // =, !=, <, >, <=, >=, LIKE, ILIKE, IN, IS NULL, IS NOT NULL
    QString value;
    bool caseSensitive = false;
};

struct DataSort {
    QString columnName;
    Qt::SortOrder order = Qt::AscendingOrder;
};

class DataEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit DataEditorDialog(QWidget* parent = nullptr);
    ~DataEditorDialog() override = default;

    void setTableInfo(const TableInfo& info);
    void setTableData(const QList<QList<QVariant>>& data);
    void setCurrentDatabase(const QString& dbName);

    QList<QList<QVariant>> getModifiedData() const;
    QList<QList<QVariant>> getNewRows() const;
    QList<int> getDeletedRowIds() const;

    static bool showDataEditor(QWidget* parent,
                              const TableInfo& tableInfo,
                              const QList<QList<QVariant>>& initialData,
                              const QString& dbName = QString());

signals:
    void dataChanged();
    void rowAdded(int rowIndex);
    void rowDeleted(int rowIndex);
    void cellValueChanged(int row, int column, const QVariant& oldValue, const QVariant& newValue);
    void filterApplied(const QList<DataFilter>& filters);
    void sortApplied(const QList<DataSort>& sorting);

private slots:
    void onAddRow();
    void onDeleteRow();
    void onDuplicateRow();
    void onRefreshData();
    void onApplyFilter();
    void onClearFilter();
    void onSearchTextChanged(const QString& text);
    void onColumnFilterChanged(int column, const QString& filterText);
    void onCellChanged(int row, int column);
    void onSelectionChanged();
    void onCommitChanges();
    void onRollbackChanges();
    void onExportData();
    void onImportData();
    void onShowSQLPreview();
    void onGoToRow();
    void onPageChanged(int page);
    void onPageSizeChanged(int size);
    void onSortColumn(int column);

private:
    void setupUI();
    void setupToolbar();
    void setupTable();
    void setupFilters();
    void setupPagination();
    void setupSQLPreview();
    void setupSearchBar();
    void setupStatusBar();

    void loadTableData();
    void applyFilters();
    void applySorting();
    void updateTableDisplay();
    void updateRowCount();
    void updatePagination();
    void validateCellValue(int row, int column, const QVariant& value);
    QString generateSQLForChanges() const;
    void showContextMenu(const QPoint& pos);

    // UI Components
    QVBoxLayout* mainLayout_;

    // Toolbar
    QHBoxLayout* toolbarLayout_;
    QPushButton* addRowButton_;
    QPushButton* deleteRowButton_;
    QPushButton* duplicateRowButton_;
    QPushButton* refreshButton_;
    QPushButton* commitButton_;
    QPushButton* rollbackButton_;
    QPushButton* exportButton_;
    QPushButton* importButton_;

    // Search and filter
    QHBoxLayout* searchLayout_;
    QLineEdit* searchEdit_;
    QComboBox* searchColumnCombo_;
    QPushButton* filterButton_;
    QPushButton* clearFilterButton_;

    // Main table
    QTableWidget* dataTable_;
    QSplitter* mainSplitter_;

    // Filters panel
    QWidget* filtersWidget_;
    QVBoxLayout* filtersLayout_;
    QList<QLineEdit*> columnFilters_;

    // Pagination
    QHBoxLayout* paginationLayout_;
    QLabel* rowCountLabel_;
    QLabel* pageInfoLabel_;
    QPushButton* firstPageButton_;
    QPushButton* prevPageButton_;
    QSpinBox* pageSpinBox_;
    QPushButton* nextPageButton_;
    QPushButton* lastPageButton_;
    QComboBox* pageSizeCombo_;

    // SQL Preview
    QWidget* sqlPreviewWidget_;
    QTextEdit* sqlPreviewText_;
    QPushButton* showSQLButton_;

    // Status bar
    QStatusBar* statusBar_;
    QLabel* statusLabel_;
    QProgressBar* progressBar_;

    // Current state
    TableInfo tableInfo_;
    QList<QList<QVariant>> originalData_;
    QList<QList<QVariant>> currentData_;
    QList<QList<QVariant>> newRows_;
    QList<int> deletedRowIds_;
    QList<QPair<int, int>> modifiedCells_; // row, column pairs

    QList<DataFilter> activeFilters_;
    QList<DataSort> activeSorting_;
    QString currentSearchText_;
    QString currentDatabase_;

    // Pagination state
    int currentPage_ = 1;
    int pageSize_ = 100;
    int totalRows_ = 0;
    int totalPages_ = 0;

    // Selection state
    QList<int> selectedRows_;
    int lastEditedRow_ = -1;
    int lastEditedColumn_ = -1;
};

} // namespace scratchrobin
