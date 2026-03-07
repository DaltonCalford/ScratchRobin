#pragma once
#include <QDialog>
#include <QDateTime>
#include <QHash>
#include <memory>

QT_BEGIN_NAMESPACE
class QTableView;
class QStandardItemModel;
class QLineEdit;
class QComboBox;
class QDateTimeEdit;
class QCheckBox;
class QPushButton;
class QTextEdit;
class QLabel;
class QSplitter;
class QTreeWidget;
QT_END_NAMESPACE

namespace scratchrobin::ui {

/**
 * @brief Query History Management System
 * 
 * Implements FORM_SPECIFICATION.md Query History section:
 * - Automatic query logging
 * - History browser with search/filter
 * - Favorites/bookmarks
 * - Execution statistics
 * - Re-execution support
 */

// ============================================================================
// Query History Entry
// ============================================================================
struct QueryHistoryEntry {
    qint64 id = 0;
    QString sql;
    QString normalizedSql;  // For deduplication
    QDateTime timestamp;
    int executionTimeMs = 0;  // How long query took
    int rowCount = -1;        // Result row count (-1 = unknown)
    bool success = true;
    QString errorMessage;
    QString databaseName;
    QString connectionName;
    bool isFavorite = false;
    QString tags;  // Comma-separated tags
    int executionCount = 1;  // How many times this query was run
};

// ============================================================================
// Query History Storage (persistent)
// ============================================================================
class QueryHistoryStorage : public QObject {
    Q_OBJECT

public:
    explicit QueryHistoryStorage(QObject* parent = nullptr);
    ~QueryHistoryStorage() override;

    // Storage operations
    qint64 addEntry(const QueryHistoryEntry& entry);
    bool updateEntry(const QueryHistoryEntry& entry);
    bool deleteEntry(qint64 id);
    bool clearHistory(int daysToKeep = 0);  // 0 = clear all
    
    // Retrieval
    QList<QueryHistoryEntry> getEntries(int limit = 100, int offset = 0) const;
    QList<QueryHistoryEntry> search(const QString& query, 
                                    const QDateTime& from = QDateTime(),
                                    const QDateTime& to = QDateTime(),
                                    bool favoritesOnly = false,
                                    const QString& tagFilter = QString()) const;
    QueryHistoryEntry getEntry(qint64 id) const;
    
    // Statistics
    int getTotalCount() const;
    int getFavoriteCount() const;
    QHash<QString, int> getTagCounts() const;
    
    // Deduplication
    qint64 findDuplicate(const QString& normalizedSql, int withinMinutes = 5) const;

signals:
    void entryAdded(const QueryHistoryEntry& entry);
    void entryUpdated(const QueryHistoryEntry& entry);
    void entryDeleted(qint64 id);

private:
    void initStorage();
    QString normalizeSql(const QString& sql) const;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Query History Dialog
// ============================================================================
class QueryHistoryDialog : public QDialog {
    Q_OBJECT

public:
    explicit QueryHistoryDialog(QueryHistoryStorage* storage, QWidget* parent = nullptr);
    ~QueryHistoryDialog() override;

    QString selectedQuery() const { return selectedQuery_; }
    bool shouldExecute() const { return shouldExecute_; }

public slots:
    void refresh();
    void setFilter(const QString& filter);

signals:
    void querySelected(const QString& sql);
    void queryExecuteRequested(const QString& sql);

private slots:
    void onSearch();
    void onFilterChanged();
    void onSelectionChanged();
    void onDoubleClicked(const QModelIndex& index);
    void onToggleFavorite();
    void onDeleteEntry();
    void onUseQuery();
    void onExecuteQuery();
    void onAddTag();
    void onExportHistory();

private:
    void setupUi();
    void loadEntries();
    void updatePreview();
    void applyFilters();
    
    QueryHistoryStorage* storage_ = nullptr;
    
    // UI
    QTableView* historyTable_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QTextEdit* previewEdit_ = nullptr;
    
    // Filters
    QLineEdit* searchEdit_ = nullptr;
    QDateTimeEdit* fromDateEdit_ = nullptr;
    QDateTimeEdit* toDateEdit_ = nullptr;
    QComboBox* tagFilterCombo_ = nullptr;
    QCheckBox* favoritesOnlyCheck_ = nullptr;
    QCheckBox* successfulOnlyCheck_ = nullptr;
    
    // Buttons
    QPushButton* useBtn_ = nullptr;
    QPushButton* executeBtn_ = nullptr;
    QPushButton* favoriteBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;
    
    // State
    QString selectedQuery_;
    bool shouldExecute_ = false;
    qint64 selectedId_ = 0;
};

// ============================================================================
// Query History Manager (singleton-like access)
// ============================================================================
class QueryHistoryManager : public QObject {
    Q_OBJECT

public:
    static QueryHistoryManager* instance();
    
    void initialize(QWidget* parent = nullptr);
    void setStorage(QueryHistoryStorage* storage);
    
    // Recording
    void recordQuery(const QString& sql, bool success = true, 
                     int executionTimeMs = 0, int rowCount = -1,
                     const QString& errorMessage = QString());
    
    // UI
    void showHistoryDialog();
    bool hasDialog() const { return dialog_ != nullptr; }

signals:
    void querySelected(const QString& sql);
    void queryExecuteRequested(const QString& sql);

private:
    QueryHistoryManager(QObject* parent = nullptr);
    ~QueryHistoryManager() override;
    
    QueryHistoryStorage* storage_ = nullptr;
    QueryHistoryDialog* dialog_ = nullptr;
    QWidget* parentWidget_ = nullptr;
};

// ============================================================================
// Query Favorites Panel (sidebar widget)
// ============================================================================
class QueryFavoritesPanel : public QWidget {
    Q_OBJECT

public:
    explicit QueryFavoritesPanel(QueryHistoryStorage* storage, QWidget* parent = nullptr);

signals:
    void querySelected(const QString& sql);
    void queryExecuteRequested(const QString& sql);

private slots:
    void onItemClicked();
    void onItemDoubleClicked();
    void onAddFavorite();
    void onRemoveFavorite();
    void onOrganizeFolders();
    void refresh();

private:
    void setupUi();
    void loadFavorites();
    
    QueryHistoryStorage* storage_ = nullptr;
    QTreeWidget* treeWidget_ = nullptr;
};

} // namespace scratchrobin::ui
