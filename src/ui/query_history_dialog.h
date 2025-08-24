#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QClipboard>
#include "types/query_types.h"

namespace scratchrobin {

class QueryHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryHistoryDialog(QWidget* parent = nullptr);
    ~QueryHistoryDialog() override = default;

    void setQueryHistory(const QList<QueryHistoryEntry>& history);
    void addQueryEntry(const QueryHistoryEntry& entry);
    void clearHistory();

signals:
    void querySelected(const QueryHistoryEntry& entry);
    void queryReRun(const std::string& query);
    void queryDeleted(const std::string& id);
    void historyExported(const QString& filePath);
    void historyImported(const QString& filePath);

private slots:
    void onSearchTextChanged(const QString& text);
    void onQuerySelected();
    void onReRunQuery();
    void onDeleteQuery();
    void onExportHistory();

private:
    void loadSampleData();

private:
    void setupUI();
    void setupQueryTable();
    void setupButtons();

    void populateTable();
    void showQueryDetails(const QueryHistoryEntry& entry);
    void applyFilters();

    // UI Components
    // Search
    QLineEdit* searchEdit_;
    QCheckBox* favoritesOnlyCheck_;

    // Filters
    QDateTimeEdit* startDateEdit_;
    QDateTimeEdit* endDateEdit_;
    QComboBox* statusFilter_;
    QComboBox* typeFilter_;

    // Query Table
    QTableWidget* queryTable_;
    QTabWidget* tabWidget_;
    QWidget* detailsTab_;
    QWidget* statisticsTab_;

    // Query Details
    QTextEdit* queryTextEdit_;
    QLabel* executionTimeLabel_;
    QLabel* statusLabel_;
    QLabel* rowsAffectedLabel_;
    QLabel* timestampLabel_;
    QTextEdit* errorTextEdit_;

    // Buttons
    QPushButton* reRunButton_;
    QPushButton* deleteButton_;
    QPushButton* exportButton_;
    QPushButton* closeButton_;

    // Additional UI components for details and statistics
    QWidget* queryDetailsWidget_;
    QVBoxLayout* queryDetailsLayout_;

    QLabel* totalQueriesLabel_;
    QLabel* successRateLabel_;
    QLabel* avgExecutionTimeLabel_;
    QLabel* mostFrequentTypeLabel_;
    QLabel* queriesTodayLabel_;
    QLabel* queriesThisWeekLabel_;

    // Data
    QList<QueryHistoryEntry> allHistory_;
    QList<QueryHistoryEntry> filteredHistory_;
    QueryHistoryEntry currentEntry_;

    // Settings
    QString currentSearchText_;
    bool currentFavoritesOnly_ = false;
    QDateTime currentStartDate_;
    QDateTime currentEndDate_;
    QString currentStatusFilter_;
    QString currentTypeFilter_;

private:
    void setupQueryDetails();
    void setupStatisticsTab();

private slots:
    void onFilterChanged();
    void onDateFilterChanged();
    void onStatusFilterChanged();
    void onTypeFilterChanged();
};

} // namespace scratchrobin
