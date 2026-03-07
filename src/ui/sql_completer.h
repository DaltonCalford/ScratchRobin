#pragma once
#include <QCompleter>
#include <QHash>
#include <QTimer>
#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QStringListModel;
class QSortFilterProxyModel;
class QTreeView;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Enhanced SQL code completion engine
 * 
 * Implements FORM_SPECIFICATION.md Code Completion section:
 * - Keyword completion (all SQL keywords)
 * - Schema object completion (tables, views, procedures)
 * - Table/column completion with context awareness
 * - Fuzzy matching for partial input
 * - Performance < 500ms response time
 */
class SqlCompleterEngine : public QObject {
    Q_OBJECT

public:
    struct CompletionItem {
        QString text;
        QString type;  // keyword, table, column, function, schema
        QString description;
        QString detail;  // Additional info (data type, table source)
        int priority = 0;  // Higher = shown first
    };

    explicit SqlCompleterEngine(backend::SessionClient* client, QObject* parent = nullptr);

    // Schema caching
    void refreshSchemaCache();
    void refreshTables();
    void refreshColumns(const QString& tableName);
    
    // Completion queries
    QList<CompletionItem> getCompletions(const QString& text, int cursorPosition, const QString& currentWord);
    
    // Context-aware completion
    enum class CompletionContext {
        General,
        AfterSelect,      // Column names, *
        AfterFrom,        // Table names
        AfterWhere,       // Column names
        AfterJoin,        // Table names
        AfterOrderBy,     // Column names
        AfterGroupBy,     // Column names
        AfterInsert,      // Table names
        AfterUpdate,      // Table names
        AfterCreate,      // Schema objects
        InFunction,       // Function names
    };

    CompletionContext detectContext(const QString& text, int cursorPosition) const;

private:
    void loadKeywords();
    void loadFunctions();
    void loadSchemaObjects();
    
    QList<CompletionItem> filterByContext(const QList<CompletionItem>& items, CompletionContext context);
    QList<CompletionItem> fuzzyMatch(const QList<CompletionItem>& items, const QString& pattern);
    int calculateMatchScore(const QString& item, const QString& pattern);

    backend::SessionClient* client_ = nullptr;
    
    // Cached data
    QHash<QString, QList<CompletionItem>> tableColumns_;  // table -> columns
    QList<CompletionItem> keywords_;
    QList<CompletionItem> functions_;
    QList<CompletionItem> tables_;
    QList<CompletionItem> views_;
    QList<CompletionItem> schemas_;
    
    bool schemaLoaded_ = false;
};

// ============================================================================
// Enhanced SQL Completer Widget
// ============================================================================
class SqlCompleter : public QCompleter {
    Q_OBJECT

public:
    explicit SqlCompleter(backend::SessionClient* client, QObject* parent = nullptr);

    void setEditorText(const QString& text);
    void setCursorPosition(int pos);
    
    // Trigger completion at current position
    void triggerCompletion();
    
    // Configure behavior
    void setFuzzyMatching(bool enabled);
    void setCaseSensitive(bool sensitive);
    void setMaxSuggestions(int max);

signals:
    void completionRequested();

private slots:
    void onTextChanged();
    void performCompletion();

private:
    void setupModel();
    void updateModel(const QList<SqlCompleterEngine::CompletionItem>& items);

    SqlCompleterEngine* engine_ = nullptr;
    QSortFilterProxyModel* filterModel_ = nullptr;
    QStringListModel* sourceModel_ = nullptr;
    QTimer* triggerTimer_ = nullptr;
    
    QString editorText_;
    int cursorPosition_ = 0;
    
    bool fuzzyMatching_ = true;
    int maxSuggestions_ = 50;
    static constexpr int TRIGGER_DELAY_MS = 150;
};

// ============================================================================
// Completion Popup Delegate (for rich rendering)
// ============================================================================
class CompletionItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit CompletionItemDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, 
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, 
                   const QModelIndex& index) const override;

private:
    void drawIcon(QPainter* painter, const QRect& rect, const QString& type) const;
};

// ============================================================================
// Schema Browser Panel (for sidebar)
// ============================================================================
class SchemaBrowserPanel : public QWidget {
    Q_OBJECT

public:
    explicit SchemaBrowserPanel(backend::SessionClient* client, QWidget* parent = nullptr);

    void refresh();
    void setFilter(const QString& filter);

signals:
    void objectSelected(const QString& name, const QString& type);
    void objectDoubleClicked(const QString& name, const QString& type);

private:
    void setupUi();
    void loadSchemaTree();
    void onItemClicked();
    void onItemDoubleClicked();
    void showContextMenu(const QPoint& pos);

    backend::SessionClient* client_ = nullptr;
    
    class SchemaTreeModel;
    SchemaTreeModel* treeModel_ = nullptr;
    QTreeView* treeView_ = nullptr;
    QLineEdit* filterEdit_ = nullptr;
};

} // namespace scratchrobin::ui
