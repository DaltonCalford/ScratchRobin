#include "query_favorites.h"
#include "query_history.h"
#include <backend/session_client.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QColorDialog>
#include <QMenu>
#include <QToolBar>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDrag>
#include <QMimeData>
#include <QCheckBox>

namespace scratchrobin::ui {

// ============================================================================
// Query Favorites Manager Panel
// ============================================================================
QueryFavoritesManagerPanel::QueryFavoritesManagerPanel(QueryHistoryStorage* storage,
                                         backend::SessionClient* client,
                                         QWidget* parent)
    : DockPanel("query_favorites_manager", parent)
    , storage_(storage)
    , client_(client) {
    setupUi();
    loadFolders();
    loadQueries();
}

void QueryFavoritesManagerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    auto* addBtn = new QPushButton(tr("+ Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onAddFavorite);
    toolbarLayout->addWidget(addBtn);
    
    auto* folderBtn = new QPushButton(tr("New Folder"), this);
    connect(folderBtn, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onNewFolder);
    toolbarLayout->addWidget(folderBtn);
    
    toolbarLayout->addStretch();
    
    auto* importBtn = new QPushButton(tr("Import"), this);
    connect(importBtn, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onImportFavorites);
    toolbarLayout->addWidget(importBtn);
    
    auto* exportBtn = new QPushButton(tr("Export"), this);
    connect(exportBtn, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onExportFavorites);
    toolbarLayout->addWidget(exportBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Search
    auto* searchLayout = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search favorites..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &QueryFavoritesManagerPanel::onSearchTextChanged);
    searchLayout->addWidget(searchEdit_);
    
    tagFilter_ = new QComboBox(this);
    tagFilter_->addItem(tr("All Tags"));
    connect(tagFilter_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) { onFilterByTag(tagFilter_->itemText(index)); });
    searchLayout->addWidget(tagFilter_);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter
    splitter_ = new QSplitter(Qt::Horizontal, this);
    
    // Folder tree
    folderTree_ = new QTreeWidget(this);
    folderTree_->setHeaderLabel(tr("Folders"));
    folderTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(folderTree_, &QTreeWidget::itemClicked, this, &QueryFavoritesManagerPanel::onFolderSelected);
    connect(folderTree_, &QTreeWidget::customContextMenuRequested, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(tr("New Folder"), this, &QueryFavoritesManagerPanel::onNewFolder);
        menu.addAction(tr("Rename"), this, &QueryFavoritesManagerPanel::onRenameFolder);
        menu.addAction(tr("Delete"), this, &QueryFavoritesManagerPanel::onDeleteFolder);
        menu.addSeparator();
        menu.addAction(tr("Organize Folders..."), this, &QueryFavoritesManagerPanel::onOrganizeFolders);
        menu.exec(folderTree_->mapToGlobal(pos));
    });
    splitter_->addWidget(folderTree_);
    
    // Query table
    queryTable_ = new QTableView(this);
    queryModel_ = new QStandardItemModel(this);
    queryModel_->setHorizontalHeaderLabels({tr("Name"), tr("Tags"), tr("Modified")});
    queryTable_->setModel(queryModel_);
    queryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queryTable_->setAlternatingRowColors(true);
    queryTable_->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(queryTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QueryFavoritesManagerPanel::onQuerySelected);
    connect(queryTable_, &QTableView::doubleClicked, this, &QueryFavoritesManagerPanel::onExecuteFavorite);
    connect(queryTable_, &QTableView::customContextMenuRequested, [this](const QPoint& pos) {
        QMenu menu(this);
        menu.addAction(tr("Execute"), this, &QueryFavoritesManagerPanel::onExecuteFavorite);
        menu.addAction(tr("Copy to Editor"), this, &QueryFavoritesManagerPanel::onCopyToEditor);
        menu.addSeparator();
        menu.addAction(tr("Edit..."), this, &QueryFavoritesManagerPanel::onEditFavorite);
        menu.addAction(tr("Duplicate"), this, &QueryFavoritesManagerPanel::onDuplicateFavorite);
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &QueryFavoritesManagerPanel::onDeleteFavorite);
        menu.exec(queryTable_->mapToGlobal(pos));
    });
    
    splitter_->addWidget(queryTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    queryNameLabel_ = new QLabel(tr("Select a query"), this);
    QFont titleFont = queryNameLabel_->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    queryNameLabel_->setFont(titleFont);
    detailsLayout->addWidget(queryNameLabel_);
    
    querySqlEdit_ = new QTextEdit(this);
    querySqlEdit_->setReadOnly(true);
    querySqlEdit_->setMaximumHeight(150);
    detailsLayout->addWidget(querySqlEdit_);
    
    detailsLayout->addWidget(new QLabel(tr("Tags:"), this));
    tagsList_ = new QListWidget(this);
    tagsList_->setMaximumHeight(60);
    tagsList_->setFlow(QListView::LeftToRight);
    detailsLayout->addWidget(tagsList_);
    
    statsLabel_ = new QLabel(this);
    detailsLayout->addWidget(statsLabel_);
    
    // Action buttons
    auto* btnLayout = new QHBoxLayout();
    
    executeBtn_ = new QPushButton(tr("Execute"), this);
    executeBtn_->setEnabled(false);
    connect(executeBtn_, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onExecuteFavorite);
    btnLayout->addWidget(executeBtn_);
    
    editBtn_ = new QPushButton(tr("Edit"), this);
    editBtn_->setEnabled(false);
    connect(editBtn_, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onEditFavorite);
    btnLayout->addWidget(editBtn_);
    
    deleteBtn_ = new QPushButton(tr("Delete"), this);
    deleteBtn_->setEnabled(false);
    connect(deleteBtn_, &QPushButton::clicked, this, &QueryFavoritesManagerPanel::onDeleteFavorite);
    btnLayout->addWidget(deleteBtn_);
    
    btnLayout->addStretch();
    
    detailsLayout->addLayout(btnLayout);
    detailsLayout->addStretch();
    
    splitter_->addWidget(detailsWidget);
    splitter_->setSizes({150, 250, 200});
    
    mainLayout->addWidget(splitter_, 1);
}

void QueryFavoritesManagerPanel::loadFolders() {
    folders_.clear();
    
    // Add default folders
    FavoriteFolder uncategorized;
    uncategorized.id = "";
    uncategorized.name = tr("Uncategorized");
    uncategorized.sortOrder = 0;
    folders_.append(uncategorized);
    
    FavoriteFolder reports;
    reports.id = "folder_reports";
    reports.name = tr("Reports");
    reports.sortOrder = 1;
    folders_.append(reports);
    
    FavoriteFolder maintenance;
    maintenance.id = "folder_maintenance";
    maintenance.name = tr("Maintenance");
    maintenance.sortOrder = 2;
    folders_.append(maintenance);
    
    updateFolderTree();
}

void QueryFavoritesManagerPanel::loadQueries() {
    queries_.clear();
    
    // Sample favorites
    FavoriteQuery q1;
    q1.id = "fav1";
    q1.name = tr("Daily Sales Report");
    q1.sql = "SELECT * FROM sales WHERE date = CURRENT_DATE";
    q1.folderId = "folder_reports";
    q1.tags = QStringList({"daily", "sales", "report"});
    q1.modifiedAt = QDateTime::currentDateTime().addDays(-1);
    q1.executionCount = 25;
    queries_.append(q1);
    
    FavoriteQuery q2;
    q2.id = "fav2";
    q2.name = tr("Active Users");
    q2.sql = "SELECT * FROM users WHERE active = 1";
    q2.folderId = "";
    q2.tags = QStringList({"users", "active"});
    q2.modifiedAt = QDateTime::currentDateTime().addDays(-3);
    q2.executionCount = 12;
    queries_.append(q2);
    
    FavoriteQuery q3;
    q3.id = "fav3";
    q3.name = tr("Database Backup");
    q3.sql = "BACKUP DATABASE";
    q3.folderId = "folder_maintenance";
    q3.tags = QStringList({"backup", "maintenance"});
    q3.modifiedAt = QDateTime::currentDateTime().addDays(-7);
    q3.executionCount = 4;
    queries_.append(q3);
    
    updateQueryList();
}

void QueryFavoritesManagerPanel::updateFolderTree() {
    folderTree_->clear();
    
    // Add "All Favorites" root
    auto* allItem = new QTreeWidgetItem(folderTree_, QStringList(tr("All Favorites")));
    allItem->setData(0, Qt::UserRole, QString("__all__"));
    allItem->setIcon(0, QIcon::fromTheme("folder-favorites"));
    
    // Build folder hierarchy
    populateFolderTree(allItem, "");
    
    folderTree_->expandAll();
}

void QueryFavoritesManagerPanel::populateFolderTree(QTreeWidgetItem* parent, const QString& parentId) {
    for (const auto& folder : folders_) {
        if (folder.parentId == parentId && !folder.id.isEmpty()) {
            auto* item = new QTreeWidgetItem(parent, QStringList(folder.name));
            item->setData(0, Qt::UserRole, folder.id);
            item->setIcon(0, QIcon::fromTheme("folder"));
            
            // Recursively add subfolders
            populateFolderTree(item, folder.id);
        }
    }
}

void QueryFavoritesManagerPanel::updateQueryList() {
    queryModel_->clear();
    queryModel_->setHorizontalHeaderLabels({tr("Name"), tr("Tags"), tr("Modified")});
    
    QString searchText = searchEdit_->text().toLower();
    QString tagFilter = tagFilter_->currentText();
    
    for (const auto& query : queries_) {
        // Filter by folder
        if (currentFolderId_ != "__all__" && query.folderId != currentFolderId_) {
            continue;
        }
        
        // Filter by search text
        if (!searchText.isEmpty() &&
            !query.name.toLower().contains(searchText) &&
            !query.sql.toLower().contains(searchText)) {
            continue;
        }
        
        // Filter by tag
        if (tagFilter != tr("All Tags") && !query.tags.contains(tagFilter)) {
            continue;
        }
        
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(query.name)
             << new QStandardItem(query.tags.join(", "))
             << new QStandardItem(query.modifiedAt.toString("yyyy-MM-dd"));
        
        (*row)[0]->setData(query.id, Qt::UserRole);
        queryModel_->appendRow(*row);
    }
    
    queryTable_->resizeColumnsToContents();
}

void QueryFavoritesManagerPanel::updateDetails(const FavoriteQuery& query) {
    queryNameLabel_->setText(query.name);
    querySqlEdit_->setText(query.sql);
    
    tagsList_->clear();
    for (const auto& tag : query.tags) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
    }
    
    statsLabel_->setText(tr("Executed %1 times | Last modified: %2")
        .arg(query.executionCount)
        .arg(query.modifiedAt.toString("yyyy-MM-dd hh:mm")));
    
    executeBtn_->setEnabled(true);
    editBtn_->setEnabled(true);
    deleteBtn_->setEnabled(true);
}

void QueryFavoritesManagerPanel::onFolderSelected(QTreeWidgetItem* item) {
    if (!item) return;
    
    currentFolderId_ = item->data(0, Qt::UserRole).toString();
    updateQueryList();
}

void QueryFavoritesManagerPanel::onQuerySelected(const QModelIndex& index) {
    if (!index.isValid()) {
        executeBtn_->setEnabled(false);
        editBtn_->setEnabled(false);
        deleteBtn_->setEnabled(false);
        return;
    }
    
    QString queryId = queryModel_->item(index.row(), 0)->data(Qt::UserRole).toString();
    
    for (const auto& query : queries_) {
        if (query.id == queryId) {
            currentQueryId_ = queryId;
            updateDetails(query);
            break;
        }
    }
}

void QueryFavoritesManagerPanel::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text)
    updateQueryList();
}

void QueryFavoritesManagerPanel::onFilterByTag(const QString& tag) {
    Q_UNUSED(tag)
    updateQueryList();
}

void QueryFavoritesManagerPanel::onAddFavorite() {
    FavoriteQuery newQuery;
    newQuery.id = "fav_" + QString::number(queries_.size() + 1);
    newQuery.folderId = currentFolderId_ == "__all__" ? "" : currentFolderId_;
    newQuery.createdAt = QDateTime::currentDateTime();
    newQuery.modifiedAt = QDateTime::currentDateTime();
    
    EditFavoriteDialog dialog(&newQuery, folders_, this);
    if (dialog.exec() == QDialog::Accepted) {
        queries_.append(dialog.query());
        updateQueryList();
    }
}

void QueryFavoritesManagerPanel::onEditFavorite() {
    if (currentQueryId_.isEmpty()) return;
    
    for (auto& query : queries_) {
        if (query.id == currentQueryId_) {
            EditFavoriteDialog dialog(&query, folders_, this);
            if (dialog.exec() == QDialog::Accepted) {
                query = dialog.query();
                query.modifiedAt = QDateTime::currentDateTime();
                updateQueryList();
            }
            break;
        }
    }
}

void QueryFavoritesManagerPanel::onDeleteFavorite() {
    if (currentQueryId_.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Favorite"),
        tr("Are you sure you want to delete this favorite query?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        for (int i = 0; i < queries_.size(); ++i) {
            if (queries_[i].id == currentQueryId_) {
                queries_.removeAt(i);
                break;
            }
        }
        currentQueryId_ = "";
        updateQueryList();
    }
}

void QueryFavoritesManagerPanel::onDuplicateFavorite() {
    if (currentQueryId_.isEmpty()) return;
    
    for (const auto& query : queries_) {
        if (query.id == currentQueryId_) {
            FavoriteQuery duplicate = query;
            duplicate.id = "fav_" + QString::number(queries_.size() + 1);
            duplicate.name = tr("Copy of %1").arg(query.name);
            duplicate.createdAt = QDateTime::currentDateTime();
            duplicate.modifiedAt = QDateTime::currentDateTime();
            queries_.append(duplicate);
            updateQueryList();
            break;
        }
    }
}

void QueryFavoritesManagerPanel::onExecuteFavorite() {
    if (currentQueryId_.isEmpty()) return;
    
    for (auto& query : queries_) {
        if (query.id == currentQueryId_) {
            query.executionCount++;
            query.lastExecuted = QDateTime::currentDateTime();
            emit queryExecuteRequested(query.sql);
            break;
        }
    }
}

void QueryFavoritesManagerPanel::onCopyToEditor() {
    if (currentQueryId_.isEmpty()) return;
    
    for (const auto& query : queries_) {
        if (query.id == currentQueryId_) {
            emit querySelected(query.sql);
            break;
        }
    }
}

void QueryFavoritesManagerPanel::onNewFolder() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Folder"),
        tr("Folder name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        FavoriteFolder folder;
        folder.id = "folder_" + QString::number(folders_.size() + 1);
        folder.name = name;
        folder.parentId = currentFolderId_ == "__all__" ? "" : currentFolderId_;
        folder.createdAt = QDateTime::currentDateTime();
        folders_.append(folder);
        updateFolderTree();
    }
}

void QueryFavoritesManagerPanel::onRenameFolder() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    QString folderId = item->data(0, Qt::UserRole).toString();
    if (folderId.isEmpty() || folderId == "__all__") return;
    
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename Folder"),
        tr("New name:"), QLineEdit::Normal, item->text(0), &ok);
    
    if (ok && !name.isEmpty()) {
        for (auto& folder : folders_) {
            if (folder.id == folderId) {
                folder.name = name;
                folder.modifiedAt = QDateTime::currentDateTime();
                break;
            }
        }
        updateFolderTree();
    }
}

void QueryFavoritesManagerPanel::onDeleteFolder() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    QString folderId = item->data(0, Qt::UserRole).toString();
    if (folderId.isEmpty() || folderId == "__all__") return;
    
    auto reply = QMessageBox::question(this, tr("Delete Folder"),
        tr("Delete this folder? Favorites will be moved to Uncategorized."),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Move queries to uncategorized
        for (auto& query : queries_) {
            if (query.folderId == folderId) {
                query.folderId = "";
            }
        }
        
        // Remove folder
        for (int i = 0; i < folders_.size(); ++i) {
            if (folders_[i].id == folderId) {
                folders_.removeAt(i);
                break;
            }
        }
        
        updateFolderTree();
        updateQueryList();
    }
}

void QueryFavoritesManagerPanel::onOrganizeFolders() {
    FolderOrganizationDialog dialog(&folders_, this);
    if (dialog.exec() == QDialog::Accepted) {
        updateFolderTree();
    }
}

void QueryFavoritesManagerPanel::onAddTag() {
    // Add tag to current query
}

void QueryFavoritesManagerPanel::onRemoveTag() {
    // Remove tag from current query
}

void QueryFavoritesManagerPanel::onSetColor() {
    // Set color for current query
}

void QueryFavoritesManagerPanel::onToggleGlobal() {
    // Toggle global status
}

void QueryFavoritesManagerPanel::onExportFavorites() {
    FavoritesImportExportDialog dialog(FavoritesImportExportDialog::Export, 
                                       queries_, folders_, this);
    dialog.exec();
}

void QueryFavoritesManagerPanel::onImportFavorites() {
    FavoritesImportExportDialog dialog(FavoritesImportExportDialog::Import,
                                       queries_, folders_, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadQueries();
        loadFolders();
    }
}

void QueryFavoritesManagerPanel::onExportFolder() {
    // Export current folder
}

// ============================================================================
// Edit Favorite Dialog
// ============================================================================
EditFavoriteDialog::EditFavoriteDialog(FavoriteQuery* query,
                                       const QList<FavoriteFolder>& folders,
                                       QWidget* parent)
    : QDialog(parent), originalQuery_(query), folders_(folders) {
    setupUi();
    
    nameEdit_->setText(query->name);
    sqlEdit_->setText(query->sql);
    descriptionEdit_->setText(query->description);
    globalCheck_->setChecked(query->isGlobal);
    selectedColor_ = query->color;
    
    populateFolderCombo();
    
    // Populate tags
    for (const auto& tag : query->tags) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
    }
}

void EditFavoriteDialog::setupUi() {
    setWindowTitle(tr("Edit Favorite Query"));
    setMinimumSize(500, 450);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Enter query name..."));
    formLayout->addRow(tr("Name:"), nameEdit_);
    
    folderCombo_ = new QComboBox(this);
    formLayout->addRow(tr("Folder:"), folderCombo_);
    
    sqlEdit_ = new QTextEdit(this);
    sqlEdit_->setPlaceholderText(tr("Enter SQL query..."));
    formLayout->addRow(tr("SQL:"), sqlEdit_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setPlaceholderText(tr("Optional description..."));
    descriptionEdit_->setMaximumHeight(60);
    formLayout->addRow(tr("Description:"), descriptionEdit_);
    
    layout->addLayout(formLayout);
    
    // Tags
    auto* tagsLayout = new QHBoxLayout();
    tagsList_ = new QListWidget(this);
    tagsList_->setMaximumHeight(60);
    tagsList_->setFlow(QListView::LeftToRight);
    tagsLayout->addWidget(tagsList_, 1);
    
    newTagEdit_ = new QLineEdit(this);
    newTagEdit_->setPlaceholderText(tr("New tag..."));
    tagsLayout->addWidget(newTagEdit_);
    
    auto* addTagBtn = new QPushButton(tr("Add"), this);
    connect(addTagBtn, &QPushButton::clicked, this, &EditFavoriteDialog::onAddTag);
    tagsLayout->addWidget(addTagBtn);
    
    layout->addLayout(tagsLayout);
    
    // Options
    globalCheck_ = new QCheckBox(tr("Available in all connections"), this);
    layout->addWidget(globalCheck_);
    
    auto* optionsLayout = new QHBoxLayout();
    colorBtn_ = new QPushButton(tr("Pick Color"), this);
    connect(colorBtn_, &QPushButton::clicked, this, &EditFavoriteDialog::onPickColor);
    optionsLayout->addWidget(colorBtn_);
    optionsLayout->addStretch();
    layout->addLayout(optionsLayout);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditFavoriteDialog::onValidate);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

void EditFavoriteDialog::populateFolderCombo() {
    folderCombo_->addItem(tr("Uncategorized"), QString(""));
    for (const auto& folder : folders_) {
        if (!folder.id.isEmpty()) {
            folderCombo_->addItem(folder.name, folder.id);
        }
    }
    
    // Select current folder
    for (int i = 0; i < folderCombo_->count(); ++i) {
        if (folderCombo_->itemData(i).toString() == originalQuery_->folderId) {
            folderCombo_->setCurrentIndex(i);
            break;
        }
    }
}

FavoriteQuery EditFavoriteDialog::query() const {
    FavoriteQuery q = *originalQuery_;
    q.name = nameEdit_->text();
    q.sql = sqlEdit_->toPlainText();
    q.description = descriptionEdit_->toPlainText();
    q.folderId = folderCombo_->currentData().toString();
    q.isGlobal = globalCheck_->isChecked();
    q.color = selectedColor_;
    
    q.tags.clear();
    for (int i = 0; i < tagsList_->count(); ++i) {
        q.tags.append(tagsList_->item(i)->text());
    }
    
    return q;
}

void EditFavoriteDialog::onSelectFolder() {
    // Folder selection logic
}

void EditFavoriteDialog::onAddTag() {
    QString tag = newTagEdit_->text().trimmed();
    if (!tag.isEmpty()) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
        newTagEdit_->clear();
    }
}

void EditFavoriteDialog::onRemoveTag() {
    auto* item = tagsList_->currentItem();
    if (item) {
        delete tagsList_->takeItem(tagsList_->row(item));
    }
}

void EditFavoriteDialog::onPickColor() {
    QColor color = QColorDialog::getColor(selectedColor_, this, tr("Select Color"));
    if (color.isValid()) {
        selectedColor_ = color;
        colorBtn_->setStyleSheet(QString("background-color: %1").arg(color.name()));
    }
}

void EditFavoriteDialog::onValidate() {
    if (nameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter a name."));
        return;
    }
    
    if (sqlEdit_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter SQL."));
        return;
    }
    
    accept();
}

// ============================================================================
// Folder Organization Dialog
// ============================================================================
FolderOrganizationDialog::FolderOrganizationDialog(QList<FavoriteFolder>* folders,
                                                   QWidget* parent)
    : QDialog(parent), folders_(folders) {
    setupUi();
    updateFolderList();
}

void FolderOrganizationDialog::setupUi() {
    setWindowTitle(tr("Organize Folders"));
    setMinimumSize(400, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    folderTree_ = new QTreeWidget(this);
    folderTree_->setHeaderLabel(tr("Folders"));
    folderTree_->setDragEnabled(true);
    folderTree_->setAcceptDrops(true);
    folderTree_->setDropIndicatorShown(true);
    layout->addWidget(folderTree_, 1);
    
    auto* btnLayout = new QHBoxLayout();
    
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &FolderOrganizationDialog::onAddFolder);
    btnLayout->addWidget(addBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &FolderOrganizationDialog::onEditFolder);
    btnLayout->addWidget(editBtn);
    
    auto* deleteBtn = new QPushButton(tr("Delete"), this);
    connect(deleteBtn, &QPushButton::clicked, this, &FolderOrganizationDialog::onDeleteFolder);
    btnLayout->addWidget(deleteBtn);
    
    btnLayout->addStretch();
    
    upBtn_ = new QPushButton(tr("↑"), this);
    connect(upBtn_, &QPushButton::clicked, this, &FolderOrganizationDialog::onMoveUp);
    btnLayout->addWidget(upBtn_);
    
    downBtn_ = new QPushButton(tr("↓"), this);
    connect(downBtn_, &QPushButton::clicked, this, &FolderOrganizationDialog::onMoveDown);
    btnLayout->addWidget(downBtn_);
    
    layout->addLayout(btnLayout);
    
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

void FolderOrganizationDialog::updateFolderList() {
    folderTree_->clear();
    
    for (const auto& folder : *folders_) {
        if (folder.id.isEmpty()) continue;  // Skip uncategorized
        
        auto* item = new QTreeWidgetItem(folderTree_, QStringList(folder.name));
        item->setData(0, Qt::UserRole, folder.id);
    }
}

void FolderOrganizationDialog::onAddFolder() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Folder"),
        tr("Folder name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        FavoriteFolder folder;
        folder.id = "folder_" + QString::number(folders_->size() + 1);
        folder.name = name;
        folder.createdAt = QDateTime::currentDateTime();
        folders_->append(folder);
        updateFolderList();
    }
}

void FolderOrganizationDialog::onEditFolder() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    bool ok;
    QString name = QInputDialog::getText(this, tr("Edit Folder"),
        tr("Folder name:"), QLineEdit::Normal, item->text(0), &ok);
    
    if (ok && !name.isEmpty()) {
        item->setText(0, name);
    }
}

void FolderOrganizationDialog::onDeleteFolder() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    delete folderTree_->takeTopLevelItem(folderTree_->indexOfTopLevelItem(item));
}

void FolderOrganizationDialog::onMoveUp() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    int index = folderTree_->indexOfTopLevelItem(item);
    if (index > 0) {
        folderTree_->takeTopLevelItem(index);
        folderTree_->insertTopLevelItem(index - 1, item);
        folderTree_->setCurrentItem(item);
    }
}

void FolderOrganizationDialog::onMoveDown() {
    auto* item = folderTree_->currentItem();
    if (!item) return;
    
    int index = folderTree_->indexOfTopLevelItem(item);
    if (index < folderTree_->topLevelItemCount() - 1) {
        folderTree_->takeTopLevelItem(index);
        folderTree_->insertTopLevelItem(index + 1, item);
        folderTree_->setCurrentItem(item);
    }
}

void FolderOrganizationDialog::onItemMoved() {
    // Handle drag-and-drop reordering
}

// ============================================================================
// Quick Favorites Widget
// ============================================================================
QuickFavoritesWidget::QuickFavoritesWidget(QueryHistoryStorage* storage, QWidget* parent)
    : QWidget(parent), storage_(storage) {
    setupUi();
    loadFavorites();
}

void QuickFavoritesWidget::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    favoritesCombo_ = new QComboBox(this);
    favoritesCombo_->setMinimumWidth(150);
    connect(favoritesCombo_, QOverload<int>::of(&QComboBox::activated),
            this, &QuickFavoritesWidget::onFavoriteSelected);
    layout->addWidget(favoritesCombo_);
    
    addBtn_ = new QPushButton(tr("+"), this);
    addBtn_->setMaximumWidth(30);
    connect(addBtn_, &QPushButton::clicked, this, &QuickFavoritesWidget::onAddCurrentQuery);
    layout->addWidget(addBtn_);
    
    manageBtn_ = new QPushButton(tr("..."), this);
    manageBtn_->setMaximumWidth(30);
    connect(manageBtn_, &QPushButton::clicked, this, &QuickFavoritesWidget::onManageFavorites);
    layout->addWidget(manageBtn_);
}

void QuickFavoritesWidget::loadFavorites() {
    favoritesCombo_->clear();
    favoritesCombo_->addItem(tr("Favorites..."));
    
    // TODO: Load from storage
    favoritesCombo_->addItem(tr("Daily Sales"), "fav1");
    favoritesCombo_->addItem(tr("Active Users"), "fav2");
}

void QuickFavoritesWidget::onFavoriteSelected(int index) {
    if (index <= 0) return;
    
    QString queryId = favoritesCombo_->itemData(index).toString();
    // TODO: Get SQL from storage and emit
    emit favoriteExecuteRequested("SELECT * FROM...");
}

void QuickFavoritesWidget::onAddCurrentQuery() {
    // Add current editor query to favorites
}

void QuickFavoritesWidget::onManageFavorites() {
    // Open favorites manager
}

// ============================================================================
// Favorites Import/Export Dialog
// ============================================================================
FavoritesImportExportDialog::FavoritesImportExportDialog(Mode mode,
                                                         const QList<FavoriteQuery>& queries,
                                                         const QList<FavoriteFolder>& folders,
                                                         QWidget* parent)
    : QDialog(parent), mode_(mode), queries_(queries), folders_(folders) {
    setupUi();
    populateTree();
}

void FavoritesImportExportDialog::setupUi() {
    setWindowTitle(mode_ == Export ? tr("Export Favorites") : tr("Import Favorites"));
    setMinimumSize(450, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // File selection
    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel(tr("File:"), this));
    fileEdit_ = new QLineEdit(this);
    fileLayout->addWidget(fileEdit_, 1);
    
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    connect(browseBtn, &QPushButton::clicked, this, &FavoritesImportExportDialog::onBrowseFile);
    fileLayout->addWidget(browseBtn);
    
    layout->addLayout(fileLayout);
    
    // Format
    auto* formatLayout = new QHBoxLayout();
    formatLayout->addWidget(new QLabel(tr("Format:"), this));
    formatCombo_ = new QComboBox(this);
    formatCombo_->addItems({tr("JSON"), tr("SQL"), tr("XML")});
    formatLayout->addWidget(formatCombo_);
    formatLayout->addStretch();
    layout->addLayout(formatLayout);
    
    // Items tree
    itemTree_ = new QTreeWidget(this);
    itemTree_->setHeaderLabel(mode_ == Export ? tr("Items to Export") : tr("Items to Import"));
    layout->addWidget(itemTree_, 1);
    
    // Options
    includeFoldersCheck_ = new QCheckBox(tr("Include folder structure"), this);
    includeFoldersCheck_->setChecked(true);
    layout->addWidget(includeFoldersCheck_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    auto* selectAllBtn = new QPushButton(tr("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &FavoritesImportExportDialog::onSelectAll);
    btnLayout->addWidget(selectAllBtn);
    
    auto* selectNoneBtn = new QPushButton(tr("Select None"), this);
    connect(selectNoneBtn, &QPushButton::clicked, this, &FavoritesImportExportDialog::onSelectNone);
    btnLayout->addWidget(selectNoneBtn);
    
    btnLayout->addStretch();
    
    executeBtn_ = new QPushButton(mode_ == Export ? tr("Export") : tr("Import"), this);
    executeBtn_->setEnabled(false);
    connect(executeBtn_, &QPushButton::clicked, this, &FavoritesImportExportDialog::onExecute);
    btnLayout->addWidget(executeBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void FavoritesImportExportDialog::populateTree() {
    itemTree_->clear();
    
    // Group by folder
    QHash<QString, QTreeWidgetItem*> folderItems;
    
    // Add folders
    for (const auto& folder : folders_) {
        if (folder.id.isEmpty()) continue;
        
        auto* folderItem = new QTreeWidgetItem(itemTree_, QStringList(folder.name));
        folderItem->setCheckState(0, Qt::Checked);
        folderItem->setData(0, Qt::UserRole, folder.id);
        folderItems[folder.id] = folderItem;
    }
    
    // Add queries
    for (const auto& query : queries_) {
        QTreeWidgetItem* parent = folderItems.value(query.folderId, itemTree_->invisibleRootItem());
        auto* queryItem = new QTreeWidgetItem(parent, QStringList(query.name));
        queryItem->setCheckState(0, Qt::Checked);
        queryItem->setData(0, Qt::UserRole, query.id);
    }
    
    itemTree_->expandAll();
}

void FavoritesImportExportDialog::onBrowseFile() {
    QString fileName;
    if (mode_ == Export) {
        fileName = QFileDialog::getSaveFileName(this, tr("Export Favorites"),
            tr("favorites.json"), tr("JSON Files (*.json);;SQL Files (*.sql)"));
    } else {
        fileName = QFileDialog::getOpenFileName(this, tr("Import Favorites"),
            QString(), tr("JSON Files (*.json);;SQL Files (*.sql)"));
    }
    
    if (!fileName.isEmpty()) {
        fileEdit_->setText(fileName);
        executeBtn_->setEnabled(true);
    }
}

void FavoritesImportExportDialog::onSelectAll() {
    QTreeWidgetItemIterator it(itemTree_);
    while (*it) {
        (*it)->setCheckState(0, Qt::Checked);
        ++it;
    }
}

void FavoritesImportExportDialog::onSelectNone() {
    QTreeWidgetItemIterator it(itemTree_);
    while (*it) {
        (*it)->setCheckState(0, Qt::Unchecked);
        ++it;
    }
}

void FavoritesImportExportDialog::onToggleFolder(int state) {
    Q_UNUSED(state)
    // Toggle all children
}

void FavoritesImportExportDialog::onExecute() {
    QString fileName = fileEdit_->text();
    if (fileName.isEmpty()) return;
    
    if (mode_ == Export) {
        exportToFile(fileName);
        QMessageBox::information(this, tr("Export Complete"),
            tr("Favorites exported successfully."));
    } else {
        importFromFile(fileName);
        QMessageBox::information(this, tr("Import Complete"),
            tr("Favorites imported successfully."));
    }
    
    accept();
}

void FavoritesImportExportDialog::exportToFile(const QString& fileName) {
    QJsonArray queriesArray;
    
    for (const auto& query : queries_) {
        QJsonObject obj;
        obj["name"] = query.name;
        obj["sql"] = query.sql;
        obj["description"] = query.description;
        obj["folderId"] = query.folderId;
        obj["tags"] = QJsonArray::fromStringList(query.tags);
        queriesArray.append(obj);
    }
    
    QJsonObject root;
    root["version"] = "1.0";
    root["queries"] = queriesArray;
    
    QJsonDocument doc(root);
    
    QFile file(fileName);
    if (file.open(QFile::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void FavoritesImportExportDialog::importFromFile(const QString& fileName) {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) return;
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    
    // TODO: Parse and add to favorites
    Q_UNUSED(root)
}

} // namespace scratchrobin::ui
