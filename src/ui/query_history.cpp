#include "ui/query_history.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTreeWidget>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>

namespace scratchrobin::ui {

// ============================================================================
// QueryHistoryStorage Implementation
// ============================================================================

struct QueryHistoryStorage::Impl {
    // In-memory storage (will be SQLite/QSettings in production)
    QList<QueryHistoryEntry> entries;
    qint64 nextId = 1;
    QString storagePath;
    
    void saveToDisk() {
        QSettings settings;
        settings.beginWriteArray("QueryHistory", entries.size());
        for (int i = 0; i < entries.size(); ++i) {
            settings.setArrayIndex(i);
            const auto& e = entries[i];
            settings.setValue("id", e.id);
            settings.setValue("sql", e.sql);
            settings.setValue("timestamp", e.timestamp);
            settings.setValue("executionTime", e.executionTimeMs);
            settings.setValue("rowCount", e.rowCount);
            settings.setValue("success", e.success);
            settings.setValue("isFavorite", e.isFavorite);
            settings.setValue("tags", e.tags);
            settings.setValue("executionCount", e.executionCount);
        }
        settings.endArray();
    }
    
    void loadFromDisk() {
        entries.clear();
        QSettings settings;
        int size = settings.beginReadArray("QueryHistory");
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            QueryHistoryEntry e;
            e.id = settings.value("id").toLongLong();
            e.sql = settings.value("sql").toString();
            e.timestamp = settings.value("timestamp").toDateTime();
            e.executionTimeMs = settings.value("executionTime").toInt();
            e.rowCount = settings.value("rowCount").toInt();
            e.success = settings.value("success").toBool();
            e.isFavorite = settings.value("isFavorite").toBool();
            e.tags = settings.value("tags").toString();
            e.executionCount = settings.value("executionCount").toInt();
            entries.append(e);
            if (e.id >= nextId) nextId = e.id + 1;
        }
        settings.endArray();
        
        // Sort by timestamp descending
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });
    }
};

QueryHistoryStorage::QueryHistoryStorage(QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>()) {
    initStorage();
}

QueryHistoryStorage::~QueryHistoryStorage() = default;

void QueryHistoryStorage::initStorage() {
    impl_->loadFromDisk();
}

qint64 QueryHistoryStorage::addEntry(const QueryHistoryEntry& entry) {
    QueryHistoryEntry e = entry;
    e.id = impl_->nextId++;
    e.normalizedSql = normalizeSql(entry.sql);
    
    if (e.timestamp.isNull()) {
        e.timestamp = QDateTime::currentDateTime();
    }
    
    impl_->entries.prepend(e);
    impl_->saveToDisk();
    
    emit entryAdded(e);
    return e.id;
}

bool QueryHistoryStorage::updateEntry(const QueryHistoryEntry& entry) {
    for (auto& e : impl_->entries) {
        if (e.id == entry.id) {
            e = entry;
            impl_->saveToDisk();
            emit entryUpdated(e);
            return true;
        }
    }
    return false;
}

bool QueryHistoryStorage::deleteEntry(qint64 id) {
    for (int i = 0; i < impl_->entries.size(); ++i) {
        if (impl_->entries[i].id == id) {
            impl_->entries.removeAt(i);
            impl_->saveToDisk();
            emit entryDeleted(id);
            return true;
        }
    }
    return false;
}

bool QueryHistoryStorage::clearHistory(int daysToKeep) {
    if (daysToKeep <= 0) {
        impl_->entries.clear();
    } else {
        QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysToKeep);
        impl_->entries.erase(
            std::remove_if(impl_->entries.begin(), impl_->entries.end(),
                [&cutoff](const auto& e) { return e.timestamp < cutoff; }),
            impl_->entries.end());
    }
    impl_->saveToDisk();
    return true;
}

QList<QueryHistoryEntry> QueryHistoryStorage::getEntries(int limit, int offset) const {
    QList<QueryHistoryEntry> result;
    int end = qMin(offset + limit, impl_->entries.size());
    for (int i = offset; i < end; ++i) {
        result.append(impl_->entries[i]);
    }
    return result;
}

QList<QueryHistoryEntry> QueryHistoryStorage::search(const QString& query, 
                                                     const QDateTime& from,
                                                     const QDateTime& to,
                                                     bool favoritesOnly,
                                                     const QString& tagFilter) const {
    QList<QueryHistoryEntry> result;
    
    for (const auto& e : impl_->entries) {
        // Text search
        if (!query.isEmpty()) {
            if (!e.sql.contains(query, Qt::CaseInsensitive) &&
                !e.tags.contains(query, Qt::CaseInsensitive)) {
                continue;
            }
        }
        
        // Date range
        if (from.isValid() && e.timestamp < from) continue;
        if (to.isValid() && e.timestamp > to) continue;
        
        // Favorites filter
        if (favoritesOnly && !e.isFavorite) continue;
        
        // Tag filter
        if (!tagFilter.isEmpty() && !e.tags.contains(tagFilter, Qt::CaseInsensitive)) continue;
        
        result.append(e);
    }
    
    return result;
}

QueryHistoryEntry QueryHistoryStorage::getEntry(qint64 id) const {
    for (const auto& e : impl_->entries) {
        if (e.id == id) return e;
    }
    return QueryHistoryEntry();
}

int QueryHistoryStorage::getTotalCount() const {
    return impl_->entries.size();
}

int QueryHistoryStorage::getFavoriteCount() const {
    int count = 0;
    for (const auto& e : impl_->entries) {
        if (e.isFavorite) ++count;
    }
    return count;
}

QHash<QString, int> QueryHistoryStorage::getTagCounts() const {
    QHash<QString, int> counts;
    for (const auto& e : impl_->entries) {
        QStringList tags = e.tags.split(",", Qt::SkipEmptyParts);
        for (auto& tag : tags) {
            tag = tag.trimmed();
            if (!tag.isEmpty()) {
                counts[tag]++;
            }
        }
    }
    return counts;
}

qint64 QueryHistoryStorage::findDuplicate(const QString& normalizedSql, int withinMinutes) const {
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-withinMinutes * 60);
    QString normalized = normalizeSql(normalizedSql);
    
    for (const auto& e : impl_->entries) {
        if (e.timestamp >= cutoff && e.normalizedSql == normalized) {
            return e.id;
        }
    }
    return 0;
}

QString QueryHistoryStorage::normalizeSql(const QString& sql) const {
    QString normalized = sql.toUpper().simplified();
    // Remove extra whitespace
    normalized.replace(QRegularExpression("\\s+"), " ");
    // Normalize parameter values (numbers, strings)
    normalized.replace(QRegularExpression("'[^']*'"), "'?'");
    normalized.replace(QRegularExpression("\\b\\d+\\b"), "?");
    return normalized;
}

// ============================================================================
// QueryHistoryDialog
// ============================================================================

QueryHistoryDialog::QueryHistoryDialog(QueryHistoryStorage* storage, QWidget* parent)
    : QDialog(parent), storage_(storage) {
    setWindowTitle(tr("Query History"));
    setMinimumSize(900, 600);
    resize(1000, 700);
    
    setupUi();
    loadEntries();
    
    if (storage_) {
        connect(storage_, &QueryHistoryStorage::entryAdded, this, &QueryHistoryDialog::refresh);
    }
}

QueryHistoryDialog::~QueryHistoryDialog() = default;

void QueryHistoryDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Filter section
    auto* filterGroup = new QGroupBox(tr("Filters"), this);
    auto* filterLayout = new QGridLayout(filterGroup);
    
    filterLayout->addWidget(new QLabel(tr("Search:"), this), 0, 0);
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search SQL or tags..."));
    filterLayout->addWidget(searchEdit_, 0, 1, 1, 3);
    
    filterLayout->addWidget(new QLabel(tr("From:"), this), 1, 0);
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime().addDays(-7));
    filterLayout->addWidget(fromDateEdit_, 1, 1);
    
    filterLayout->addWidget(new QLabel(tr("To:"), this), 1, 2);
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterLayout->addWidget(toDateEdit_, 1, 3);
    
    filterLayout->addWidget(new QLabel(tr("Tags:"), this), 2, 0);
    tagFilterCombo_ = new QComboBox(this);
    tagFilterCombo_->setEditable(true);
    tagFilterCombo_->addItem(tr("All Tags"), QString());
    filterLayout->addWidget(tagFilterCombo_, 2, 1);
    
    favoritesOnlyCheck_ = new QCheckBox(tr("Favorites only"), this);
    filterLayout->addWidget(favoritesOnlyCheck_, 2, 2);
    
    successfulOnlyCheck_ = new QCheckBox(tr("Successful only"), this);
    filterLayout->addWidget(successfulOnlyCheck_, 2, 3);
    
    auto* searchBtn = new QPushButton(tr("&Search"), this);
    filterLayout->addWidget(searchBtn, 0, 4);
    
    mainLayout->addWidget(filterGroup);
    
    // Splitter for table and preview
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // History table
    historyTable_ = new QTableView(this);
    historyTable_->setAlternatingRowColors(true);
    historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    historyTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(historyTable_);
    
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({
        tr("★"), tr("Time"), tr("SQL"), tr("Exec Time"), 
        tr("Rows"), tr("Status"), tr("Tags")
    });
    historyTable_->setModel(model_);
    historyTable_->horizontalHeader()->setStretchLastSection(true);
    historyTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    historyTable_->setColumnWidth(0, 30);
    historyTable_->setColumnWidth(1, 150);
    historyTable_->setColumnWidth(3, 80);
    historyTable_->setColumnWidth(4, 60);
    historyTable_->setColumnWidth(5, 70);
    
    // Preview
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 10));
    previewEdit_->setPlaceholderText(tr("Select a query to preview..."));
    splitter->addWidget(previewEdit_);
    
    splitter->setSizes({400, 200});
    mainLayout->addWidget(splitter, 1);
    
    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    
    useBtn_ = new QPushButton(tr("&Use Query"), this);
    useBtn_->setEnabled(false);
    buttonLayout->addWidget(useBtn_);
    
    executeBtn_ = new QPushButton(tr("&Execute"), this);
    executeBtn_->setEnabled(false);
    buttonLayout->addWidget(executeBtn_);
    
    buttonLayout->addSpacing(20);
    
    favoriteBtn_ = new QPushButton(tr("Toggle &Favorite"), this);
    favoriteBtn_->setEnabled(false);
    buttonLayout->addWidget(favoriteBtn_);
    
    deleteBtn_ = new QPushButton(tr("&Delete"), this);
    deleteBtn_->setEnabled(false);
    buttonLayout->addWidget(deleteBtn_);
    
    buttonLayout->addStretch();
    
    auto* exportBtn = new QPushButton(tr("&Export..."), this);
    buttonLayout->addWidget(exportBtn);
    
    auto* clearBtn = new QPushButton(tr("&Clear History..."), this);
    buttonLayout->addWidget(clearBtn);
    
    buttonLayout->addSpacing(20);
    
    auto* closeBtn = new QPushButton(tr("&Close"), this);
    buttonLayout->addWidget(closeBtn);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(searchBtn, &QPushButton::clicked, this, &QueryHistoryDialog::onSearch);
    connect(searchEdit_, &QLineEdit::returnPressed, this, &QueryHistoryDialog::onSearch);
    connect(historyTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QueryHistoryDialog::onSelectionChanged);
    connect(historyTable_, &QTableView::doubleClicked, this, &QueryHistoryDialog::onDoubleClicked);
    connect(useBtn_, &QPushButton::clicked, this, &QueryHistoryDialog::onUseQuery);
    connect(executeBtn_, &QPushButton::clicked, this, &QueryHistoryDialog::onExecuteQuery);
    connect(favoriteBtn_, &QPushButton::clicked, this, &QueryHistoryDialog::onToggleFavorite);
    connect(deleteBtn_, &QPushButton::clicked, this, &QueryHistoryDialog::onDeleteEntry);
    connect(exportBtn, &QPushButton::clicked, this, &QueryHistoryDialog::onExportHistory);
    connect(clearBtn, &QPushButton::clicked, [this]() {
        auto reply = QMessageBox::question(this, tr("Clear History"),
            tr("Are you sure you want to clear the query history?"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes && storage_) {
            storage_->clearHistory();
            refresh();
        }
    });
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void QueryHistoryDialog::loadEntries() {
    if (!storage_) return;
    
    model_->removeRows(0, model_->rowCount());
    auto entries = storage_->getEntries(100);
    
    for (const auto& e : entries) {
        QList<QStandardItem*> row;
        
        auto* favItem = new QStandardItem(e.isFavorite ? "★" : "☆");
        favItem->setTextAlignment(Qt::AlignCenter);
        row.append(favItem);
        
        row.append(new QStandardItem(e.timestamp.toString("yyyy-MM-dd hh:mm:ss")));
        
        QString sqlPreview = e.sql.simplified().left(100);
        if (e.sql.length() > 100) sqlPreview += "...";
        row.append(new QStandardItem(sqlPreview));
        
        row.append(new QStandardItem(e.executionTimeMs > 0 
            ? QString("%1 ms").arg(e.executionTimeMs) : "-"));
        
        row.append(new QStandardItem(e.rowCount >= 0 
            ? QString::number(e.rowCount) : "-"));
        
        row.append(new QStandardItem(e.success ? tr("OK") : tr("Error")));
        
        row.append(new QStandardItem(e.tags));
        
        // Store ID in first item
        favItem->setData(e.id, Qt::UserRole);
        
        model_->appendRow(row);
    }
}

void QueryHistoryDialog::refresh() {
    loadEntries();
}

void QueryHistoryDialog::onSearch() {
    applyFilters();
}

void QueryHistoryDialog::applyFilters() {
    if (!storage_) return;
    
    auto entries = storage_->search(
        searchEdit_->text(),
        fromDateEdit_->dateTime(),
        toDateEdit_->dateTime(),
        favoritesOnlyCheck_->isChecked()
    );
    
    model_->removeRows(0, model_->rowCount());
    for (const auto& e : entries) {
        QList<QStandardItem*> row;
        row.append(new QStandardItem(e.isFavorite ? "★" : "☆"));
        row.append(new QStandardItem(e.timestamp.toString("yyyy-MM-dd hh:mm:ss")));
        QString sqlPreview = e.sql.simplified().left(100);
        if (e.sql.length() > 100) sqlPreview += "...";
        row.append(new QStandardItem(sqlPreview));
        row.append(new QStandardItem(e.executionTimeMs > 0 ? QString("%1 ms").arg(e.executionTimeMs) : "-"));
        row.append(new QStandardItem(e.rowCount >= 0 ? QString::number(e.rowCount) : "-"));
        row.append(new QStandardItem(e.success ? tr("OK") : tr("Error")));
        row.append(new QStandardItem(e.tags));
        row[0]->setData(e.id, Qt::UserRole);
        model_->appendRow(row);
    }
}

void QueryHistoryDialog::onFilterChanged() {
    applyFilters();
}

void QueryHistoryDialog::onSelectionChanged() {
    auto selection = historyTable_->selectionModel()->selectedRows();
    bool hasSelection = !selection.isEmpty();
    
    useBtn_->setEnabled(hasSelection);
    executeBtn_->setEnabled(hasSelection);
    favoriteBtn_->setEnabled(hasSelection);
    deleteBtn_->setEnabled(hasSelection);
    
    if (hasSelection) {
        selectedId_ = selection.first().data(Qt::UserRole).toLongLong();
        updatePreview();
    }
}

void QueryHistoryDialog::updatePreview() {
    if (!storage_ || selectedId_ == 0) return;
    
    auto entry = storage_->getEntry(selectedId_);
    previewEdit_->setPlainText(entry.sql);
    selectedQuery_ = entry.sql;
}

void QueryHistoryDialog::onDoubleClicked(const QModelIndex& index) {
    Q_UNUSED(index)
    onUseQuery();
}

void QueryHistoryDialog::onToggleFavorite() {
    if (!storage_ || selectedId_ == 0) return;
    
    auto entry = storage_->getEntry(selectedId_);
    entry.isFavorite = !entry.isFavorite;
    storage_->updateEntry(entry);
    
    refresh();
}

void QueryHistoryDialog::onDeleteEntry() {
    if (!storage_ || selectedId_ == 0) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Entry"),
        tr("Are you sure you want to delete this history entry?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        storage_->deleteEntry(selectedId_);
        selectedId_ = 0;
        selectedQuery_.clear();
        refresh();
    }
}

void QueryHistoryDialog::onUseQuery() {
    if (!selectedQuery_.isEmpty()) {
        emit querySelected(selectedQuery_);
        shouldExecute_ = false;
        accept();
    }
}

void QueryHistoryDialog::onExecuteQuery() {
    if (!selectedQuery_.isEmpty()) {
        emit queryExecuteRequested(selectedQuery_);
        shouldExecute_ = true;
        accept();
    }
}

void QueryHistoryDialog::onAddTag() {
    // TODO: Implement tag adding
}

void QueryHistoryDialog::onExportHistory() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export History"),
        QString(), tr("JSON (*.json);;CSV (*.csv);;SQL (*.sql)"));
    
    if (fileName.isEmpty() || !storage_) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Export Error"),
            tr("Could not open file for writing."));
        return;
    }
    
    auto entries = storage_->getEntries(1000);
    
    if (fileName.endsWith(".json")) {
        QJsonArray array;
        for (const auto& e : entries) {
            QJsonObject obj;
            obj["timestamp"] = e.timestamp.toString(Qt::ISODate);
            obj["sql"] = e.sql;
            obj["executionTimeMs"] = e.executionTimeMs;
            obj["rowCount"] = e.rowCount;
            obj["success"] = e.success;
            obj["isFavorite"] = e.isFavorite;
            obj["tags"] = e.tags;
            array.append(obj);
        }
        QJsonDocument doc(array);
        file.write(doc.toJson());
    } else if (fileName.endsWith(".csv")) {
        // CSV export
        QTextStream stream(&file);
        stream << "Timestamp,SQL,Execution Time,Row Count,Success\n";
        for (const auto& e : entries) {
            stream << e.timestamp.toString("yyyy-MM-dd hh:mm:ss") << ",";
            QString escaped = e.sql;
            escaped.replace("\"", "\"\"");
            stream << "\"" << escaped.simplified().left(200) << "\",";
            stream << e.executionTimeMs << ",";
            stream << e.rowCount << ",";
            stream << (e.success ? "Yes" : "No") << "\n";
        }
    } else {
        // SQL export - create INSERT statements
        QTextStream stream(&file);
        stream << "-- Query History Export\n";
        stream << "-- Generated: " << QDateTime::currentDateTime().toString() << "\n\n";
        for (const auto& e : entries) {
            if (e.sql.trimmed().endsWith(";")) {
                stream << e.sql << "\n\n";
            } else {
                stream << e.sql << ";\n\n";
            }
        }
    }
    
    file.close();
    
    QMessageBox::information(this, tr("Export Complete"),
        tr("Exported %1 entries to:\n%2").arg(entries.size()).arg(fileName));
}

void QueryHistoryDialog::setFilter(const QString& filter) {
    searchEdit_->setText(filter);
    onSearch();
}

// ============================================================================
// QueryHistoryManager (Singleton)
// ============================================================================

static QueryHistoryManager* g_instance = nullptr;

QueryHistoryManager* QueryHistoryManager::instance() {
    if (!g_instance) {
        g_instance = new QueryHistoryManager();
    }
    return g_instance;
}

QueryHistoryManager::QueryHistoryManager(QObject* parent)
    : QObject(parent) {
    storage_ = new QueryHistoryStorage(this);
}

QueryHistoryManager::~QueryHistoryManager() {
    g_instance = nullptr;
}

void QueryHistoryManager::initialize(QWidget* parent) {
    parentWidget_ = parent;
}

void QueryHistoryManager::setStorage(QueryHistoryStorage* storage) {
    if (storage_ && storage_ != storage) {
        delete storage_;
    }
    storage_ = storage;
}

void QueryHistoryManager::recordQuery(const QString& sql, bool success,
                                      int executionTimeMs, int rowCount,
                                      const QString& errorMessage) {
    if (!storage_) return;
    
    // Check for duplicates
    qint64 dupId = storage_->findDuplicate(sql, 5);
    if (dupId > 0) {
        // Update existing entry
        auto entry = storage_->getEntry(dupId);
        entry.executionCount++;
        entry.timestamp = QDateTime::currentDateTime();
        entry.executionTimeMs = executionTimeMs;
        entry.rowCount = rowCount;
        entry.success = success;
        storage_->updateEntry(entry);
        return;
    }
    
    QueryHistoryEntry entry;
    entry.sql = sql;
    entry.success = success;
    entry.executionTimeMs = executionTimeMs;
    entry.rowCount = rowCount;
    entry.errorMessage = errorMessage;
    
    storage_->addEntry(entry);
}

void QueryHistoryManager::showHistoryDialog() {
    if (!dialog_) {
        dialog_ = new QueryHistoryDialog(storage_, parentWidget_);
        connect(dialog_, &QueryHistoryDialog::querySelected,
                this, &QueryHistoryManager::querySelected);
        connect(dialog_, &QueryHistoryDialog::queryExecuteRequested,
                this, &QueryHistoryManager::queryExecuteRequested);
        connect(dialog_, &QDialog::finished, [this]() {
            dialog_->deleteLater();
            dialog_ = nullptr;
        });
    }
    dialog_->show();
    dialog_->raise();
    dialog_->activateWindow();
}

// ============================================================================
// QueryFavoritesPanel
// ============================================================================

QueryFavoritesPanel::QueryFavoritesPanel(QueryHistoryStorage* storage, QWidget* parent)
    : QWidget(parent), storage_(storage) {
    setupUi();
    loadFavorites();
}

void QueryFavoritesPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    layout->setContentsMargins(4, 4, 4, 4);
    
    auto* filterEdit = new QLineEdit(this);
    filterEdit->setPlaceholderText(tr("Filter favorites..."));
    layout->addWidget(filterEdit);
    
    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setHeaderHidden(true);
    layout->addWidget(treeWidget_);
    
    auto* buttonLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("+"), this);
    addBtn->setToolTip(tr("Add current query"));
    buttonLayout->addWidget(addBtn);
    
    auto* removeBtn = new QPushButton(tr("-"), this);
    removeBtn->setToolTip(tr("Remove favorite"));
    buttonLayout->addWidget(removeBtn);
    
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // Connections
    connect(treeWidget_, &QTreeWidget::itemClicked, this, &QueryFavoritesPanel::onItemClicked);
    connect(treeWidget_, &QTreeWidget::itemDoubleClicked, this, &QueryFavoritesPanel::onItemDoubleClicked);
    connect(addBtn, &QPushButton::clicked, this, &QueryFavoritesPanel::onAddFavorite);
    connect(removeBtn, &QPushButton::clicked, this, &QueryFavoritesPanel::onRemoveFavorite);
}

void QueryFavoritesPanel::loadFavorites() {
    if (!storage_) return;
    
    treeWidget_->clear();
    
    auto entries = storage_->search(QString(), QDateTime(), QDateTime(), true);
    
    for (const auto& e : entries) {
        auto* item = new QTreeWidgetItem(treeWidget_);
        QString label = e.sql.simplified().left(40);
        if (e.sql.length() > 40) label += "...";
        item->setText(0, label);
        item->setData(0, Qt::UserRole, e.id);
        item->setToolTip(0, e.sql);
    }
}

void QueryFavoritesPanel::refresh() {
    loadFavorites();
}

void QueryFavoritesPanel::onItemClicked() {
    auto* item = treeWidget_->currentItem();
    if (!item) return;
    
    qint64 id = item->data(0, Qt::UserRole).toLongLong();
    if (storage_) {
        auto entry = storage_->getEntry(id);
        emit querySelected(entry.sql);
    }
}

void QueryFavoritesPanel::onItemDoubleClicked() {
    auto* item = treeWidget_->currentItem();
    if (!item) return;
    
    qint64 id = item->data(0, Qt::UserRole).toLongLong();
    if (storage_) {
        auto entry = storage_->getEntry(id);
        emit queryExecuteRequested(entry.sql);
    }
}

void QueryFavoritesPanel::onAddFavorite() {
    // Would be called with current SQL from editor
}

void QueryFavoritesPanel::onRemoveFavorite() {
    auto* item = treeWidget_->currentItem();
    if (!item || !storage_) return;
    
    qint64 id = item->data(0, Qt::UserRole).toLongLong();
    auto entry = storage_->getEntry(id);
    entry.isFavorite = false;
    storage_->updateEntry(entry);
    
    refresh();
}

void QueryFavoritesPanel::onOrganizeFolders() {
    // TODO: Implement folder organization
}

} // namespace scratchrobin::ui
