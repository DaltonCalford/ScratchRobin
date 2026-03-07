#include "ui/sql_completer.h"
#include "backend/session_client.h"

#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QLineEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <QPainter>
#include <QStandardItemModel>
#include <QMenu>
#include <QDebug>

namespace scratchrobin::ui {

// ============================================================================
// SQL Keywords Database
// ============================================================================
static const char* SQL_KEYWORDS[] = {
    // ANSI SQL
    "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE", "CREATE", "DROP", "ALTER",
    "TABLE", "INDEX", "VIEW", "TRIGGER", "PROCEDURE", "FUNCTION", "DATABASE", "SCHEMA",
    "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER", "CROSS", "NATURAL", "ON",
    "AND", "OR", "NOT", "NULL", "IS", "IN", "EXISTS", "BETWEEN", "LIKE", "SIMILAR",
    "ORDER", "BY", "GROUP", "HAVING", "LIMIT", "OFFSET", "UNION", "ALL", "DISTINCT",
    "ASC", "DESC", "AS", "CASE", "WHEN", "THEN", "ELSE", "END", "IF", "WHILE",
    "BEGIN", "COMMIT", "ROLLBACK", "TRANSACTION", "SAVEPOINT", "RELEASE",
    "PRIMARY", "KEY", "FOREIGN", "REFERENCES", "UNIQUE", "CHECK", "DEFAULT",
    "AUTO_INCREMENT", "IDENTITY", "NULLS", "FIRST", "LAST",
    // Data types
    "INT", "INTEGER", "BIGINT", "SMALLINT", "TINYINT", "BOOLEAN", "BOOL",
    "VARCHAR", "CHAR", "TEXT", "STRING", "CLOB", "BLOB", "BINARY", "VARBINARY",
    "DECIMAL", "NUMERIC", "FLOAT", "DOUBLE", "REAL", "MONEY", "CURRENCY",
    "DATE", "TIME", "DATETIME", "TIMESTAMP", "INTERVAL", "YEAR", "MONTH", "DAY",
    "JSON", "JSONB", "XML", "UUID", "ARRAY", "ENUM", "SET",
    // Functions
    "COUNT", "SUM", "AVG", "MIN", "MAX", "STDDEV", "VARIANCE",
    nullptr
};

static const char* SQL_FUNCTIONS[] = {
    // Aggregate
    "COUNT(*)", "COUNT(DISTINCT )", "SUM()", "AVG()", "MIN()", "MAX()",
    "STRING_AGG()", "ARRAY_AGG()", "JSON_AGG()", "XMLAGG()",
    // String
    "CONCAT()", "SUBSTRING()", "LEFT()", "RIGHT()", "TRIM()", "LTRIM()", "RTRIM()",
    "UPPER()", "LOWER()", "LENGTH()", "CHAR_LENGTH()", "REPLACE()", "POSITION()",
    "INSTR()", "SUBSTR()", "LPAD()", "RPAD()", "TRANSLATE()", "INITCAP()",
    // Date/Time
    "NOW()", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "LOCALTIME",
    "EXTRACT(YEAR FROM )", "EXTRACT(MONTH FROM )", "EXTRACT(DAY FROM )",
    "DATE_PART()", "DATE_TRUNC()", "AGE()", "DATE_FORMAT()", "STRFTIME()",
    // Math
    "ABS()", "ROUND()", "CEIL()", "FLOOR()", "POWER()", "SQRT()", "EXP()", "LN()",
    "LOG()", "MOD()", "SIGN()", "TRUNC()", "RANDOM()", "PI()",
    // Conversion
    "CAST( AS )", "CONVERT()", "TO_CHAR()", "TO_DATE()", "TO_NUMBER()", "TO_TIMESTAMP()",
    "COALESCE()", "NULLIF()", "GREATEST()", "LEAST()", "NVL()", "IFNULL()",
    // Conditional
    "CASE WHEN THEN END", "NULLIF()", "COALESCE()", "DECODE()", "IIF()",
    // Window
    "ROW_NUMBER() OVER ()", "RANK() OVER ()", "DENSE_RANK() OVER ()",
    "LEAD() OVER ()", "LAG() OVER ()", "FIRST_VALUE() OVER ()", "LAST_VALUE() OVER ()",
    nullptr
};

// ============================================================================
// SqlCompleterEngine
// ============================================================================

SqlCompleterEngine::SqlCompleterEngine(backend::SessionClient* client, QObject* parent)
    : QObject(parent), client_(client) {
    loadKeywords();
    loadFunctions();
}

void SqlCompleterEngine::loadKeywords() {
    for (int i = 0; SQL_KEYWORDS[i]; ++i) {
        CompletionItem item;
        item.text = SQL_KEYWORDS[i];
        item.type = "keyword";
        item.description = "SQL Keyword";
        item.priority = 100;
        keywords_.append(item);
    }
}

void SqlCompleterEngine::loadFunctions() {
    for (int i = 0; SQL_FUNCTIONS[i]; ++i) {
        CompletionItem item;
        item.text = SQL_FUNCTIONS[i];
        item.type = "function";
        item.description = "SQL Function";
        item.priority = 90;
        functions_.append(item);
    }
}

void SqlCompleterEngine::refreshSchemaCache() {
    schemaLoaded_ = false;
    loadSchemaObjects();
}

void SqlCompleterEngine::loadSchemaObjects() {
    if (!client_) return;
    
    // Mock data for now - in real implementation would query database
    tables_.clear();
    views_.clear();
    schemas_.clear();
    
    // Sample tables
    QStringList sampleTables = {"users", "orders", "products", "categories", 
                                "customers", "employees", "departments"};
    for (const auto& table : sampleTables) {
        CompletionItem item;
        item.text = table;
        item.type = "table";
        item.description = "Table";
        item.priority = 80;
        tables_.append(item);
    }
    
    schemaLoaded_ = true;
}

void SqlCompleterEngine::refreshTables() {
    loadSchemaObjects();
}

void SqlCompleterEngine::refreshColumns(const QString& tableName) {
    // Mock column data
    QHash<QString, QStringList> sampleColumns = {
        {"users", {"id", "username", "email", "created_at", "updated_at"}},
        {"orders", {"id", "user_id", "product_id", "quantity", "total", "created_at"}},
        {"products", {"id", "name", "description", "price", "category_id", "in_stock"}},
        {"categories", {"id", "name", "parent_id"}},
        {"customers", {"id", "company_name", "contact_name", "phone", "address"}},
        {"employees", {"id", "first_name", "last_name", "email", "department_id", "hire_date"}},
        {"departments", {"id", "name", "location"}}
    };
    
    QStringList columns = sampleColumns.value(tableName.toLower());
    QList<CompletionItem> columnItems;
    for (const auto& col : columns) {
        CompletionItem item;
        item.text = col;
        item.type = "column";
        item.description = "Column";
        item.detail = tableName;  // Source table
        item.priority = 85;
        columnItems.append(item);
    }
    tableColumns_[tableName.toLower()] = columnItems;
}

SqlCompleterEngine::CompletionContext SqlCompleterEngine::detectContext(
    const QString& text, int cursorPosition) const {
    
    QString beforeCursor = text.left(cursorPosition).toUpper();
    
    // Remove strings and comments for analysis
    QRegularExpression stringRegex("'[^']*'");
    beforeCursor.replace(stringRegex, "''");
    
    // Check for context keywords
    static const QHash<QString, CompletionContext> contextMap = {
        {"SELECT", CompletionContext::AfterSelect},
        {"FROM", CompletionContext::AfterFrom},
        {"JOIN", CompletionContext::AfterJoin},
        {"WHERE", CompletionContext::AfterWhere},
        {"ORDER BY", CompletionContext::AfterOrderBy},
        {"GROUP BY", CompletionContext::AfterGroupBy},
        {"INSERT INTO", CompletionContext::AfterInsert},
        {"UPDATE", CompletionContext::AfterUpdate},
        {"CREATE", CompletionContext::AfterCreate},
    };
    
    // Find the most recent context keyword
    int lastPos = -1;
    CompletionContext context = CompletionContext::General;
    
    for (auto it = contextMap.begin(); it != contextMap.end(); ++it) {
        int pos = beforeCursor.lastIndexOf(it.key());
        if (pos > lastPos) {
            lastPos = pos;
            context = it.value();
        }
    }
    
    // Check if we're in a function call
    if (beforeCursor.contains(QRegularExpression("\\w+\\s*\\([^)]*$"))) {
        return CompletionContext::InFunction;
    }
    
    return context;
}

QList<SqlCompleterEngine::CompletionItem> SqlCompleterEngine::getCompletions(
    const QString& text, int cursorPosition, const QString& currentWord) {
    
    QList<CompletionItem> allItems;
    
    // Always include keywords
    allItems.append(keywords_);
    
    // Add functions
    allItems.append(functions_);
    
    // Add schema objects if loaded
    if (!schemaLoaded_) {
        loadSchemaObjects();
    }
    
    // Context-aware filtering
    auto context = detectContext(text, cursorPosition);
    
    switch (context) {
        case CompletionContext::AfterFrom:
        case CompletionContext::AfterJoin:
        case CompletionContext::AfterUpdate:
            // Prioritize tables
            allItems.append(tables_);
            allItems.append(views_);
            break;
            
        case CompletionContext::AfterSelect:
        case CompletionContext::AfterWhere:
        case CompletionContext::AfterOrderBy:
        case CompletionContext::AfterGroupBy:
            // Add columns from tables mentioned in query
            allItems.append(tables_);  // Can also use table names in expressions
            // Try to extract table names from query
            {
                QRegularExpression fromRegex("FROM\\s+(\\w+)", QRegularExpression::CaseInsensitiveOption);
                auto match = fromRegex.match(text);
                if (match.hasMatch()) {
                    QString tableName = match.captured(1).toLower();
                    if (!tableColumns_.contains(tableName)) {
                        const_cast<SqlCompleterEngine*>(this)->refreshColumns(tableName);
                    }
                    allItems.append(tableColumns_.value(tableName));
                }
            }
            break;
            
        case CompletionContext::AfterInsert:
            allItems.append(tables_);
            break;
            
        default:
            allItems.append(tables_);
            allItems.append(views_);
            break;
    }
    
    // Filter by current word
    if (!currentWord.isEmpty()) {
        allItems = fuzzyMatch(allItems, currentWord.toUpper());
    }
    
    // Sort by priority
    std::sort(allItems.begin(), allItems.end(), 
              [](const CompletionItem& a, const CompletionItem& b) {
                  if (a.priority != b.priority) return a.priority > b.priority;
                  return a.text < b.text;
              });
    
    return allItems;
}

QList<SqlCompleterEngine::CompletionItem> SqlCompleterEngine::fuzzyMatch(
    const QList<CompletionItem>& items, const QString& pattern) {
    
    QList<QPair<CompletionItem, int>> scored;  // item, score
    
    for (const auto& item : items) {
        int score = calculateMatchScore(item.text, pattern);
        if (score > 0) {
            scored.append({item, score});
        }
    }
    
    // Sort by score descending
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    QList<CompletionItem> result;
    for (const auto& pair : scored) {
        result.append(pair.first);
    }
    
    return result;
}

int SqlCompleterEngine::calculateMatchScore(const QString& item, const QString& pattern) {
    QString itemUpper = item.toUpper();
    QString patternUpper = pattern.toUpper();
    
    // Exact match - highest score
    if (itemUpper == patternUpper) return 1000;
    
    // Starts with - high score
    if (itemUpper.startsWith(patternUpper)) return 800 - item.length();
    
    // Contains as word - good score
    if (itemUpper.contains(" " + patternUpper) || 
        itemUpper.contains("(" + patternUpper)) {
        return 600 - item.length();
    }
    
    // Contains anywhere - medium score
    if (itemUpper.contains(patternUpper)) {
        return 400 - item.length();
    }
    
    // Fuzzy match - calculate character sequence match
    int patternIdx = 0;
    int itemIdx = 0;
    int matchCount = 0;
    int lastMatchIdx = -1;
    int totalGap = 0;
    
    while (patternIdx < patternUpper.length() && itemIdx < itemUpper.length()) {
        if (patternUpper[patternIdx] == itemUpper[itemIdx]) {
            matchCount++;
            if (lastMatchIdx >= 0) {
                totalGap += itemIdx - lastMatchIdx - 1;
            }
            lastMatchIdx = itemIdx;
            patternIdx++;
        }
        itemIdx++;
    }
    
    // All characters matched
    if (matchCount == patternUpper.length()) {
        // Score based on gap size (smaller gaps = better)
        return 200 - (totalGap * 10) - item.length();
    }
    
    return 0;  // No match
}

// ============================================================================
// SqlCompleter
// ============================================================================

SqlCompleter::SqlCompleter(backend::SessionClient* client, QObject* parent)
    : QCompleter(parent), engine_(new SqlCompleterEngine(client, this)) {
    
    setupModel();
    
    // Configure popup
    setCompletionMode(QCompleter::PopupCompletion);
    setCaseSensitivity(Qt::CaseInsensitive);
    setWrapAround(false);
    setMaxVisibleItems(15);
    
    // Setup trigger timer for performance
    triggerTimer_ = new QTimer(this);
    triggerTimer_->setSingleShot(true);
    triggerTimer_->setInterval(TRIGGER_DELAY_MS);
    connect(triggerTimer_, &QTimer::timeout, this, &SqlCompleter::performCompletion);
}

void SqlCompleter::setupModel() {
    sourceModel_ = new QStringListModel(this);
    filterModel_ = new QSortFilterProxyModel(this);
    filterModel_->setSourceModel(sourceModel_);
    filterModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    setModel(filterModel_);
}

void SqlCompleter::setEditorText(const QString& text) {
    editorText_ = text;
}

void SqlCompleter::setCursorPosition(int pos) {
    cursorPosition_ = pos;
}

void SqlCompleter::triggerCompletion() {
    triggerTimer_->start();
}

void SqlCompleter::performCompletion() {
    // Extract current word
    QString textBeforeCursor = editorText_.left(cursorPosition_);
    
    // Find word start
    int wordStart = cursorPosition_ - 1;
    while (wordStart >= 0 && 
           (textBeforeCursor[wordStart].isLetterOrNumber() || 
            textBeforeCursor[wordStart] == '_')) {
        wordStart--;
    }
    wordStart++;
    
    QString currentWord = textBeforeCursor.mid(wordStart);
    
    // Get completions
    auto items = engine_->getCompletions(editorText_, cursorPosition_, currentWord);
    
    // Limit results
    if (items.size() > maxSuggestions_) {
        items = items.mid(0, maxSuggestions_);
    }
    
    // Update model
    updateModel(items);
    
    // Show completion popup if we have items
    if (!items.isEmpty() && !currentWord.isEmpty()) {
        setCompletionPrefix(currentWord);
        complete();
    }
}

void SqlCompleter::updateModel(const QList<SqlCompleterEngine::CompletionItem>& items) {
    QStringList strings;
    for (const auto& item : items) {
        strings.append(item.text);
    }
    sourceModel_->setStringList(strings);
}

void SqlCompleter::setFuzzyMatching(bool enabled) {
    fuzzyMatching_ = enabled;
}

void SqlCompleter::setCaseSensitive(bool sensitive) {
    setCaseSensitivity(sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void SqlCompleter::setMaxSuggestions(int max) {
    maxSuggestions_ = max;
}

void SqlCompleter::onTextChanged() {
    triggerCompletion();
}

// ============================================================================
// CompletionItemDelegate
// ============================================================================

CompletionItemDelegate::CompletionItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void CompletionItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
    QStyledItemDelegate::paint(painter, option, index);
    
    // Could add icon/type indicator here
    // For now, just using default rendering
}

QSize CompletionItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const {
    return QStyledItemDelegate::sizeHint(option, index);
}

void CompletionItemDelegate::drawIcon(QPainter* painter, const QRect& rect, 
                                      const QString& type) const {
    Q_UNUSED(painter)
    Q_UNUSED(rect)
    Q_UNUSED(type)
    // Placeholder for icon drawing
}

// ============================================================================
// SchemaBrowserPanel
// ============================================================================

class SchemaBrowserPanel::SchemaTreeModel : public QStandardItemModel {
public:
    explicit SchemaTreeModel(QObject* parent = nullptr) : QStandardItemModel(parent) {
        setHorizontalHeaderLabels({"Object", "Type"});
    }
};

SchemaBrowserPanel::SchemaBrowserPanel(backend::SessionClient* client, QWidget* parent)
    : QWidget(parent), client_(client) {
    setupUi();
    loadSchemaTree();
}

void SchemaBrowserPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    layout->setContentsMargins(4, 4, 4, 4);
    
    // Filter
    filterEdit_ = new QLineEdit(this);
    filterEdit_->setPlaceholderText(tr("Filter objects..."));
    layout->addWidget(filterEdit_);
    
    // Tree view
    treeView_ = new QTreeView(this);
    treeView_->setHeaderHidden(true);
    treeView_->setAlternatingRowColors(true);
    layout->addWidget(treeView_);
    
    treeModel_ = new SchemaTreeModel(this);
    treeView_->setModel(treeModel_);
    
    // Connections
    connect(filterEdit_, &QLineEdit::textChanged, this, &SchemaBrowserPanel::setFilter);
    connect(treeView_, &QTreeView::clicked, this, &SchemaBrowserPanel::onItemClicked);
    connect(treeView_, &QTreeView::doubleClicked, this, &SchemaBrowserPanel::onItemDoubleClicked);
    treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView_, &QTreeView::customContextMenuRequested, 
            this, &SchemaBrowserPanel::showContextMenu);
}

void SchemaBrowserPanel::loadSchemaTree() {
    treeModel_->clear();
    
    // Tables category
    auto* tablesItem = new QStandardItem(tr("Tables"));
    tablesItem->setIcon(QIcon::fromTheme("folder"));
    treeModel_->appendRow(tablesItem);
    
    QStringList tables = {"users", "orders", "products", "categories", 
                          "customers", "employees", "departments"};
    for (const auto& table : tables) {
        auto* item = new QStandardItem(table);
        item->setData("table", Qt::UserRole);
        tablesItem->appendRow(item);
    }
    tablesItem->setRowCount(tables.size());
    
    // Views category
    auto* viewsItem = new QStandardItem(tr("Views"));
    viewsItem->setIcon(QIcon::fromTheme("folder"));
    treeModel_->appendRow(viewsItem);
    
    QStringList views = {"active_users", "sales_summary", "inventory_status"};
    for (const auto& view : views) {
        auto* item = new QStandardItem(view);
        item->setData("view", Qt::UserRole);
        viewsItem->appendRow(item);
    }
    
    // Procedures category
    auto* procsItem = new QStandardItem(tr("Procedures"));
    procsItem->setIcon(QIcon::fromTheme("folder"));
    treeModel_->appendRow(procsItem);
    
    treeView_->expandAll();
}

void SchemaBrowserPanel::refresh() {
    loadSchemaTree();
}

void SchemaBrowserPanel::setFilter(const QString& filter) {
    // Simple filter implementation
    for (int i = 0; i < treeModel_->rowCount(); ++i) {
        auto* parent = treeModel_->item(i);
        bool parentVisible = false;
        
        for (int j = 0; j < parent->rowCount(); ++j) {
            auto* child = parent->child(j);
            bool match = filter.isEmpty() || 
                        child->text().contains(filter, Qt::CaseInsensitive);
            treeView_->setRowHidden(j, parent->index(), !match);
            if (match) parentVisible = true;
        }
        
        treeView_->setRowHidden(i, QModelIndex(), !parentVisible && !filter.isEmpty());
    }
}

void SchemaBrowserPanel::onItemClicked() {
    auto index = treeView_->currentIndex();
    if (!index.isValid()) return;
    
    auto* item = treeModel_->itemFromIndex(index);
    if (!item) return;
    
    QString type = item->data(Qt::UserRole).toString();
    if (!type.isEmpty()) {
        emit objectSelected(item->text(), type);
    }
}

void SchemaBrowserPanel::onItemDoubleClicked() {
    auto index = treeView_->currentIndex();
    if (!index.isValid()) return;
    
    auto* item = treeModel_->itemFromIndex(index);
    if (!item) return;
    
    QString type = item->data(Qt::UserRole).toString();
    if (!type.isEmpty()) {
        emit objectDoubleClicked(item->text(), type);
    }
}

void SchemaBrowserPanel::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    
    menu.addAction(tr("Refresh"), this, &SchemaBrowserPanel::refresh);
    menu.addSeparator();
    menu.addAction(tr("Generate SELECT"));
    menu.addAction(tr("Generate INSERT"));
    menu.addAction(tr("View Data"));
    
    menu.exec(treeView_->mapToGlobal(pos));
}

} // namespace scratchrobin::ui
