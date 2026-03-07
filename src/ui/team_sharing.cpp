#include "team_sharing.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QTableView>
#include <QTreeView>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QStackedWidget>
#include <QRadioButton>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QDateTimeEdit>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>

namespace scratchrobin::ui {

// ============================================================================
// Team Sharing Panel
// ============================================================================
TeamSharingPanel::TeamSharingPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("team_sharing", parent), client_(client) {
    setupUi();
    loadItems();
}

void TeamSharingPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto* toolbarLayout = new QHBoxLayout();
    
    newBtn_ = new QPushButton(tr("New Item"), this);
    editBtn_ = new QPushButton(tr("Edit"), this);
    deleteBtn_ = new QPushButton(tr("Delete"), this);
    shareBtn_ = new QPushButton(tr("Share"), this);
    
    newBtn_->setEnabled(client_ != nullptr);
    editBtn_->setEnabled(false);
    deleteBtn_->setEnabled(false);
    shareBtn_->setEnabled(false);
    
    connect(newBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onNewItem);
    connect(editBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onEditItem);
    connect(deleteBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onDeleteItem);
    connect(shareBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onShareItem);
    
    toolbarLayout->addWidget(newBtn_);
    toolbarLayout->addWidget(editBtn_);
    toolbarLayout->addWidget(deleteBtn_);
    toolbarLayout->addWidget(shareBtn_);
    toolbarLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &TeamSharingPanel::onRefreshItems);
    toolbarLayout->addWidget(refreshBtn);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Search bar
    auto* searchLayout = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search shared items..."));
    searchEdit_->setClearButtonEnabled(true);
    connect(searchEdit_, &QLineEdit::textChanged, this, &TeamSharingPanel::onSearchTextChanged);
    
    filterCombo_ = new QComboBox(this);
    filterCombo_->addItem(tr("All Items"));
    filterCombo_->addItem(tr("My Items"));
    filterCombo_->addItem(tr("Favorites"));
    filterCombo_->addItem(tr("Recent"));
    filterCombo_->addItem(tr("Popular"));
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TeamSharingPanel::onCategoryChanged);
    
    searchLayout->addWidget(searchEdit_, 1);
    searchLayout->addWidget(filterCombo_);
    
    mainLayout->addLayout(searchLayout);
    
    // Splitter for list and details
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Items table
    itemsTable_ = new QTableView(this);
    itemsModel_ = new QStandardItemModel(this);
    itemsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Author"), tr("Updated"), tr("★")});
    itemsTable_->setModel(itemsModel_);
    itemsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    itemsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    itemsTable_->setAlternatingRowColors(true);
    itemsTable_->horizontalHeader()->setStretchLastSection(true);
    
    connect(itemsTable_->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TeamSharingPanel::onItemSelected);
    
    splitter->addWidget(itemsTable_);
    
    // Details panel
    auto* detailsWidget = new QWidget(this);
    auto* detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setContentsMargins(8, 8, 8, 8);
    
    itemNameLabel_ = new QLabel(tr("Select an item to view details"), this);
    QFont nameFont = itemNameLabel_->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 2);
    itemNameLabel_->setFont(nameFont);
    detailsLayout->addWidget(itemNameLabel_);
    
    itemAuthorLabel_ = new QLabel(this);
    detailsLayout->addWidget(itemAuthorLabel_);
    
    itemDateLabel_ = new QLabel(this);
    detailsLayout->addWidget(itemDateLabel_);
    
    itemStatsLabel_ = new QLabel(this);
    detailsLayout->addWidget(itemStatsLabel_);
    
    tagsList_ = new QListWidget(this);
    tagsList_->setMaximumHeight(60);
    tagsList_->setFlow(QListView::LeftToRight);
    tagsList_->setWrapping(true);
    tagsList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    detailsLayout->addWidget(new QLabel(tr("Tags:"), this));
    detailsLayout->addWidget(tagsList_);
    
    itemDescriptionEdit_ = new QTextEdit(this);
    itemDescriptionEdit_->setReadOnly(true);
    itemDescriptionEdit_->setMaximumHeight(80);
    itemDescriptionEdit_->setPlaceholderText(tr("No description"));
    detailsLayout->addWidget(new QLabel(tr("Description:"), this));
    detailsLayout->addWidget(itemDescriptionEdit_);
    
    itemContentEdit_ = new QPlainTextEdit(this);
    itemContentEdit_->setReadOnly(true);
    itemContentEdit_->setPlaceholderText(tr("Content preview..."));
    detailsLayout->addWidget(new QLabel(tr("Content:"), this));
    detailsLayout->addWidget(itemContentEdit_, 1);
    
    // Action buttons
    auto* actionLayout = new QHBoxLayout();
    
    favoriteBtn_ = new QPushButton(tr("☆ Favorite"), this);
    favoriteBtn_->setEnabled(false);
    connect(favoriteBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onToggleFavorite);
    actionLayout->addWidget(favoriteBtn_);
    
    auto* copyBtn = new QPushButton(tr("Copy"), this);
    copyBtn->setEnabled(false);
    connect(copyBtn, &QPushButton::clicked, this, &TeamSharingPanel::onCopyToClipboard);
    actionLayout->addWidget(copyBtn);
    
    insertBtn_ = new QPushButton(tr("Insert to Editor"), this);
    insertBtn_->setEnabled(false);
    connect(insertBtn_, &QPushButton::clicked, this, &TeamSharingPanel::onInsertIntoEditor);
    actionLayout->addWidget(insertBtn_);
    
    auto* downloadBtn = new QPushButton(tr("Download"), this);
    downloadBtn->setEnabled(false);
    connect(downloadBtn, &QPushButton::clicked, this, &TeamSharingPanel::onDownloadItem);
    actionLayout->addWidget(downloadBtn);
    
    actionLayout->addStretch();
    detailsLayout->addLayout(actionLayout);
    
    splitter->addWidget(detailsWidget);
    splitter->setSizes({400, 400});
    
    mainLayout->addWidget(splitter, 1);
    
    // Status bar
    auto* statusLayout = new QHBoxLayout();
    auto* statusLabel = new QLabel(tr("Ready"), this);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    auto* teamBtn = new QPushButton(tr("Team Members"), this);
    connect(teamBtn, &QPushButton::clicked, this, &TeamSharingPanel::onViewTeamMembers);
    statusLayout->addWidget(teamBtn);
    
    auto* activityBtn = new QPushButton(tr("Activity"), this);
    connect(activityBtn, &QPushButton::clicked, this, &TeamSharingPanel::onViewActivityFeed);
    statusLayout->addWidget(activityBtn);
    
    mainLayout->addLayout(statusLayout);
}

void TeamSharingPanel::loadItems() {
    // TODO: Load from backend
    items_.clear();
    
    // Sample data
    SharedItem item1;
    item1.id = "1";
    item1.name = tr("Common JOIN Pattern");
    item1.description = tr("Standard JOIN pattern for customer orders");
    item1.type = SharedItemType::SqlQuery;
    item1.content = "SELECT c.name, o.order_date, o.total\nFROM customers c\nJOIN orders o ON c.id = o.customer_id";
    item1.author = tr("John Doe");
    item1.authorId = "user1";
    item1.createdAt = QDateTime::currentDateTime().addDays(-30);
    item1.updatedAt = QDateTime::currentDateTime().addDays(-5);
    item1.tags = QStringList({"join", "customers", "orders"});
    item1.category = tr("Queries");
    item1.viewCount = 245;
    item1.favoriteCount = 12;
    item1.isFavorite = true;
    item1.myPermission = PermissionLevel::Editor;
    items_.append(item1);
    
    SharedItem item2;
    item2.id = "2";
    item2.name = tr("Monthly Sales Report");
    item2.description = tr("Generate monthly sales summary");
    item2.type = SharedItemType::SqlQuery;
    item2.content = "SELECT DATE_FORMAT(order_date, '%Y-%m') as month,\n       SUM(total) as total_sales\nFROM orders\nGROUP BY month";
    item2.author = tr("Jane Smith");
    item2.authorId = "user2";
    item2.createdAt = QDateTime::currentDateTime().addDays(-60);
    item2.updatedAt = QDateTime::currentDateTime().addDays(-10);
    item2.tags = QStringList({"report", "sales", "monthly"});
    item2.category = tr("Reports");
    item2.viewCount = 189;
    item2.favoriteCount = 8;
    item2.isFavorite = false;
    item2.myPermission = PermissionLevel::Viewer;
    items_.append(item2);
    
    SharedItem item3;
    item3.id = "3";
    item3.name = tr("Backup Script Template");
    item3.description = tr("Standard database backup procedure");
    item3.type = SharedItemType::Script;
    item3.content = "#!/bin/bash\n# Database backup script\nmysqldump -u root -p database > backup.sql";
    item3.author = tr("Admin");
    item3.authorId = "admin";
    item3.createdAt = QDateTime::currentDateTime().addDays(-90);
    item3.updatedAt = QDateTime::currentDateTime().addDays(-1);
    item3.tags = QStringList({"backup", "script", "maintenance"});
    item3.category = tr("Scripts");
    item3.viewCount = 523;
    item3.favoriteCount = 45;
    item3.isFavorite = true;
    item3.myPermission = PermissionLevel::Viewer;
    items_.append(item3);
    
    updateItemsList();
}

void TeamSharingPanel::updateItemsList() {
    itemsModel_->clear();
    itemsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Author"), tr("Updated"), tr("★")});
    
    for (const auto& item : items_) {
        QString typeStr;
        switch (item.type) {
            case SharedItemType::SqlQuery: typeStr = tr("Query"); break;
            case SharedItemType::CodeSnippet: typeStr = tr("Snippet"); break;
            case SharedItemType::Template: typeStr = tr("Template"); break;
            case SharedItemType::Script: typeStr = tr("Script"); break;
            case SharedItemType::Documentation: typeStr = tr("Doc"); break;
        }
        
        auto* nameItem = new QStandardItem(item.name);
        nameItem->setData(item.id, Qt::UserRole);
        nameItem->setIcon(QIcon::fromTheme("document-text"));
        
        auto* row = new QList<QStandardItem*>();
        *row << nameItem
             << new QStandardItem(typeStr)
             << new QStandardItem(item.author)
             << new QStandardItem(item.updatedAt.toString("yyyy-MM-dd"))
             << new QStandardItem(QString::number(item.favoriteCount));
        
        itemsModel_->appendRow(*row);
    }
}

void TeamSharingPanel::onItemSelected(const QModelIndex& index) {
    if (!index.isValid()) {
        editBtn_->setEnabled(false);
        deleteBtn_->setEnabled(false);
        shareBtn_->setEnabled(false);
        favoriteBtn_->setEnabled(false);
        insertBtn_->setEnabled(false);
        return;
    }
    
    int row = index.row();
    if (row >= 0 && row < items_.size()) {
        currentItem_ = items_[row];
        updateItemDetails(currentItem_);
        
        bool canEdit = currentItem_.myPermission == PermissionLevel::Owner ||
                       currentItem_.myPermission == PermissionLevel::Editor;
        
        editBtn_->setEnabled(canEdit);
        deleteBtn_->setEnabled(currentItem_.myPermission == PermissionLevel::Owner);
        shareBtn_->setEnabled(true);
        favoriteBtn_->setEnabled(true);
        insertBtn_->setEnabled(true);
        
        favoriteBtn_->setText(currentItem_.isFavorite ? tr("★ Favorited") : tr("☆ Favorite"));
    }
}

void TeamSharingPanel::updateItemDetails(const SharedItem& item) {
    itemNameLabel_->setText(item.name);
    itemAuthorLabel_->setText(tr("By %1").arg(item.author));
    itemDateLabel_->setText(tr("Updated: %1 | Created: %2")
        .arg(item.updatedAt.toString("yyyy-MM-dd"))
        .arg(item.createdAt.toString("yyyy-MM-dd")));
    itemStatsLabel_->setText(tr("Views: %1 | Favorites: %2 | Version: %3")
        .arg(item.viewCount)
        .arg(item.favoriteCount)
        .arg(item.version));
    
    itemDescriptionEdit_->setText(item.description);
    itemContentEdit_->setPlainText(item.content);
    
    tagsList_->clear();
    for (const auto& tag : item.tags) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
    }
}

void TeamSharingPanel::onNewItem() {
    NewSharedItemDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        SharedItem newItem = dialog.item();
        newItem.id = QString::number(items_.size() + 1);
        newItem.createdAt = QDateTime::currentDateTime();
        newItem.updatedAt = QDateTime::currentDateTime();
        newItem.author = tr("Me");
        newItem.authorId = "me";
        newItem.myPermission = PermissionLevel::Owner;
        items_.append(newItem);
        updateItemsList();
    }
}

void TeamSharingPanel::onEditItem() {
    if (currentItem_.id.isEmpty()) return;
    
    // TODO: Open edit dialog
    QMessageBox::information(this, tr("Edit"), tr("Edit dialog would open for: %1").arg(currentItem_.name));
}

void TeamSharingPanel::onDeleteItem() {
    if (currentItem_.id.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, tr("Delete Item"),
        tr("Are you sure you want to delete '%1'?").arg(currentItem_.name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        items_.removeAll(currentItem_);
        currentItem_ = SharedItem();
        updateItemsList();
    }
}

void TeamSharingPanel::onShareItem() {
    if (currentItem_.id.isEmpty()) return;
    
    ShareItemDialog dialog(currentItem_, client_, this);
    dialog.exec();
}

void TeamSharingPanel::onToggleFavorite() {
    if (currentItem_.id.isEmpty()) return;
    
    currentItem_.isFavorite = !currentItem_.isFavorite;
    if (currentItem_.isFavorite) {
        currentItem_.favoriteCount++;
    } else {
        currentItem_.favoriteCount--;
    }
    
    // Update in list
    for (auto& item : items_) {
        if (item.id == currentItem_.id) {
            item = currentItem_;
            break;
        }
    }
    
    favoriteBtn_->setText(currentItem_.isFavorite ? tr("★ Favorited") : tr("☆ Favorite"));
    updateItemsList();
}

void TeamSharingPanel::onCopyToClipboard() {
    if (currentItem_.id.isEmpty()) return;
    
    QApplication::clipboard()->setText(currentItem_.content);
    QMessageBox::information(this, tr("Copied"), tr("Item content copied to clipboard."));
}

void TeamSharingPanel::onInsertIntoEditor() {
    if (currentItem_.id.isEmpty()) return;
    
    emit itemInserted(currentItem_.content);
    QMessageBox::information(this, tr("Inserted"), tr("Item inserted into editor."));
}

void TeamSharingPanel::onDownloadItem() {
    if (currentItem_.id.isEmpty()) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Download Item"),
        currentItem_.name + ".sql", tr("SQL Files (*.sql);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        // TODO: Save file
        QMessageBox::information(this, tr("Downloaded"), tr("Item saved to: %1").arg(fileName));
    }
}

void TeamSharingPanel::onRefreshItems() {
    loadItems();
}

void TeamSharingPanel::onViewTeamMembers() {
    TeamMembersDialog dialog(client_, this);
    dialog.exec();
}

void TeamSharingPanel::onViewActivityFeed() {
    ActivityFeedDialog dialog(client_, this);
    dialog.exec();
}

void TeamSharingPanel::onCategoryChanged(int index) {
    Q_UNUSED(index)
    filterItems();
}

void TeamSharingPanel::onSearchTextChanged(const QString& text) {
    currentFilter_ = text;
    filterItems();
}

void TeamSharingPanel::onTagFilterChanged(const QString& tag) {
    Q_UNUSED(tag)
    // TODO: Filter by tag
}

void TeamSharingPanel::filterItems() {
    // TODO: Implement filtering logic
    updateItemsList();
}

void TeamSharingPanel::onDuplicateItem() {
    if (currentItem_.id.isEmpty()) return;
    
    SharedItem duplicated = currentItem_;
    duplicated.id = QString::number(items_.size() + 1);
    duplicated.name = tr("Copy of %1").arg(duplicated.name);
    duplicated.author = tr("Me");
    duplicated.authorId = "me";
    duplicated.createdAt = QDateTime::currentDateTime();
    duplicated.updatedAt = QDateTime::currentDateTime();
    duplicated.viewCount = 0;
    duplicated.favoriteCount = 0;
    duplicated.myPermission = PermissionLevel::Owner;
    
    items_.append(duplicated);
    updateItemsList();
}

// ============================================================================
// New Shared Item Dialog
// ============================================================================
NewSharedItemDialog::NewSharedItemDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
}

void NewSharedItemDialog::setupUi() {
    setWindowTitle(tr("New Shared Item"));
    setMinimumSize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    auto* formLayout = new QFormLayout();
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("Enter item name..."));
    formLayout->addRow(tr("Name:"), nameEdit_);
    
    descriptionEdit_ = new QTextEdit(this);
    descriptionEdit_->setPlaceholderText(tr("Enter description..."));
    descriptionEdit_->setMaximumHeight(80);
    formLayout->addRow(tr("Description:"), descriptionEdit_);
    
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem(tr("SQL Query"), static_cast<int>(SharedItemType::SqlQuery));
    typeCombo_->addItem(tr("Code Snippet"), static_cast<int>(SharedItemType::CodeSnippet));
    typeCombo_->addItem(tr("Template"), static_cast<int>(SharedItemType::Template));
    typeCombo_->addItem(tr("Script"), static_cast<int>(SharedItemType::Script));
    typeCombo_->addItem(tr("Documentation"), static_cast<int>(SharedItemType::Documentation));
    formLayout->addRow(tr("Type:"), typeCombo_);
    
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->setEditable(true);
    categoryCombo_->addItems({tr("Queries"), tr("Snippets"), tr("Reports"), tr("Scripts"), tr("Documentation")});
    formLayout->addRow(tr("Category:"), categoryCombo_);
    
    layout->addLayout(formLayout);
    
    // Tags
    auto* tagsLayout = new QHBoxLayout();
    newTagEdit_ = new QLineEdit(this);
    newTagEdit_->setPlaceholderText(tr("Add tag..."));
    auto* addTagBtn = new QPushButton(tr("Add"), this);
    connect(addTagBtn, &QPushButton::clicked, this, &NewSharedItemDialog::onAddTag);
    tagsLayout->addWidget(newTagEdit_);
    tagsLayout->addWidget(addTagBtn);
    layout->addLayout(tagsLayout);
    
    tagsList_ = new QListWidget(this);
    tagsList_->setMaximumHeight(60);
    tagsList_->setFlow(QListView::LeftToRight);
    layout->addWidget(tagsList_);
    
    // Content
    layout->addWidget(new QLabel(tr("Content:"), this));
    contentEdit_ = new QPlainTextEdit(this);
    contentEdit_->setPlaceholderText(tr("Enter content here..."));
    layout->addWidget(contentEdit_, 1);
    
    // Options
    publicCheck_ = new QCheckBox(tr("Make public (visible to all team members)"), this);
    publicCheck_->setChecked(true);
    layout->addWidget(publicCheck_);
    
    // Buttons
    buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox_, &QDialogButtonBox::accepted, this, &NewSharedItemDialog::onValidate);
    connect(buttonBox_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox_);
}

void NewSharedItemDialog::onTypeChanged(int index) {
    Q_UNUSED(index)
}

void NewSharedItemDialog::onCategoryChanged(const QString& category) {
    Q_UNUSED(category)
}

void NewSharedItemDialog::onAddTag() {
    QString tag = newTagEdit_->text().trimmed();
    if (!tag.isEmpty()) {
        auto* item = new QListWidgetItem(tag);
        item->setBackground(QBrush(QColor(200, 220, 255)));
        tagsList_->addItem(item);
        newTagEdit_->clear();
    }
}

void NewSharedItemDialog::onRemoveTag() {
    auto* item = tagsList_->currentItem();
    if (item) {
        delete tagsList_->takeItem(tagsList_->row(item));
    }
}

void NewSharedItemDialog::onValidate() {
    if (nameEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter a name."));
        return;
    }
    
    if (contentEdit_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Validation"), tr("Please enter content."));
        return;
    }
    
    accept();
}

SharedItem NewSharedItemDialog::item() const {
    SharedItem item;
    item.name = nameEdit_->text().trimmed();
    item.description = descriptionEdit_->toPlainText().trimmed();
    item.type = static_cast<SharedItemType>(typeCombo_->currentData().toInt());
    item.content = contentEdit_->toPlainText();
    item.category = categoryCombo_->currentText();
    item.isPublic = publicCheck_->isChecked();
    
    for (int i = 0; i < tagsList_->count(); ++i) {
        item.tags.append(tagsList_->item(i)->text());
    }
    
    return item;
}

// ============================================================================
// Share Item Dialog
// ============================================================================
ShareItemDialog::ShareItemDialog(const SharedItem& item, backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), item_(item), client_(client) {
    setupUi();
    loadCurrentPermissions();
}

void ShareItemDialog::setupUi() {
    setWindowTitle(tr("Share: %1").arg(item_.name));
    setMinimumSize(450, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    // Item info
    itemNameLabel_ = new QLabel(tr("Sharing: <b>%1</b>").arg(item_.name), this);
    layout->addWidget(itemNameLabel_);
    
    // Public link
    auto* linkLayout = new QHBoxLayout();
    linkEdit_ = new QLineEdit(this);
    linkEdit_->setReadOnly(true);
    linkEdit_->setText(tr("https://scratchrobin.local/share/%1").arg(item_.id));
    linkLayout->addWidget(new QLabel(tr("Link:"), this));
    linkLayout->addWidget(linkEdit_);
    
    auto* copyLinkBtn = new QPushButton(tr("Copy"), this);
    connect(copyLinkBtn, &QPushButton::clicked, this, &ShareItemDialog::onCopyLink);
    linkLayout->addWidget(copyLinkBtn);
    layout->addLayout(linkLayout);
    
    // Public toggle
    publicCheck_ = new QCheckBox(tr("Allow anyone with the link to view"), this);
    publicCheck_->setChecked(item_.isPublic);
    connect(publicCheck_, &QCheckBox::toggled, this, &ShareItemDialog::onMakePublic);
    layout->addWidget(publicCheck_);
    
    layout->addWidget(new QLabel(tr("People with access:"), this));
    
    // Permissions list
    permissionsList_ = new QListWidget(this);
    layout->addWidget(permissionsList_);
    
    // Add user
    auto* addLayout = new QHBoxLayout();
    userCombo_ = new QComboBox(this);
    userCombo_->setEditable(true);
    userCombo_->addItems({tr("Alice"), tr("Bob"), tr("Charlie")});
    addLayout->addWidget(userCombo_);
    
    permissionCombo_ = new QComboBox(this);
    permissionCombo_->addItem(tr("Viewer"), static_cast<int>(PermissionLevel::Viewer));
    permissionCombo_->addItem(tr("Editor"), static_cast<int>(PermissionLevel::Editor));
    addLayout->addWidget(permissionCombo_);
    
    addUserBtn_ = new QPushButton(tr("Add"), this);
    connect(addUserBtn_, &QPushButton::clicked, this, &ShareItemDialog::onAddUser);
    addLayout->addWidget(addUserBtn_);
    
    layout->addLayout(addLayout);
    
    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttonBox);
}

void ShareItemDialog::loadCurrentPermissions() {
    permissions_.clear();
    
    // Add owner
    SharePermission ownerPerm;
    ownerPerm.userId = item_.authorId;
    ownerPerm.userName = item_.author;
    ownerPerm.level = PermissionLevel::Owner;
    ownerPerm.grantedAt = item_.createdAt;
    permissions_.append(ownerPerm);
    
    updatePermissionsList();
}

void ShareItemDialog::updatePermissionsList() {
    permissionsList_->clear();
    
    for (const auto& perm : permissions_) {
        QString levelStr;
        switch (perm.level) {
            case PermissionLevel::Owner: levelStr = tr("Owner"); break;
            case PermissionLevel::Editor: levelStr = tr("Editor"); break;
            case PermissionLevel::Viewer: levelStr = tr("Viewer"); break;
        }
        
        auto* item = new QListWidgetItem(tr("%1 - %2").arg(perm.userName).arg(levelStr));
        permissionsList_->addItem(item);
    }
}

void ShareItemDialog::onAddUser() {
    QString userName = userCombo_->currentText().trimmed();
    if (userName.isEmpty()) return;
    
    SharePermission perm;
    perm.userId = userName.toLower();
    perm.userName = userName;
    perm.level = static_cast<PermissionLevel>(permissionCombo_->currentData().toInt());
    perm.grantedAt = QDateTime::currentDateTime();
    perm.grantedBy = tr("Me");
    
    permissions_.append(perm);
    updatePermissionsList();
    
    userCombo_->setCurrentIndex(-1);
}

void ShareItemDialog::onRemoveUser() {
    int row = permissionsList_->currentRow();
    if (row > 0) { // Don't remove owner
        permissions_.removeAt(row);
        updatePermissionsList();
    }
}

void ShareItemDialog::onPermissionChanged(const QString& userId, PermissionLevel level) {
    Q_UNUSED(userId)
    Q_UNUSED(level)
    // TODO: Update permission
}

void ShareItemDialog::onMakePublic(bool checked) {
    item_.isPublic = checked;
}

void ShareItemDialog::onCopyLink() {
    QApplication::clipboard()->setText(linkEdit_->text());
    QMessageBox::information(this, tr("Copied"), tr("Link copied to clipboard."));
}

void ShareItemDialog::onSendInvite() {
    // TODO: Send invite
    QMessageBox::information(this, tr("Invite"), tr("Invitation sent."));
}

// ============================================================================
// Team Members Dialog
// ============================================================================
TeamMembersDialog::TeamMembersDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadMembers();
}

void TeamMembersDialog::setupUi() {
    setWindowTitle(tr("Team Members"));
    setMinimumSize(500, 350);
    
    auto* layout = new QVBoxLayout(this);
    
    // Stats
    teamStatsLabel_ = new QLabel(tr("Loading team statistics..."), this);
    layout->addWidget(teamStatsLabel_);
    
    // Members table
    membersTable_ = new QTableView(this);
    membersModel_ = new QStandardItemModel(this);
    membersModel_->setHorizontalHeaderLabels({tr("Name"), tr("Email"), tr("Role"), tr("Joined"), tr("Status")});
    membersTable_->setModel(membersModel_);
    membersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    membersTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(membersTable_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    
    inviteBtn_ = new QPushButton(tr("Invite Member"), this);
    connect(inviteBtn_, &QPushButton::clicked, this, &TeamMembersDialog::onInviteMember);
    btnLayout->addWidget(inviteBtn_);
    
    auto* removeBtn = new QPushButton(tr("Remove"), this);
    connect(removeBtn, &QPushButton::clicked, this, &TeamMembersDialog::onRemoveMember);
    btnLayout->addWidget(removeBtn);
    
    auto* messageBtn = new QPushButton(tr("Send Message"), this);
    connect(messageBtn, &QPushButton::clicked, this, &TeamMembersDialog::onSendMessage);
    btnLayout->addWidget(messageBtn);
    
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &TeamMembersDialog::onRefreshMembers);
    btnLayout->addWidget(refreshBtn);
    
    layout->addLayout(btnLayout);
}

void TeamMembersDialog::loadMembers() {
    members_.clear();
    
    // Sample data
    TeamMember m1;
    m1.id = "1";
    m1.name = tr("John Doe");
    m1.email = "john@example.com";
    m1.role = tr("Admin");
    m1.joinedAt = QDateTime::currentDateTime().addDays(-365);
    m1.isOnline = true;
    members_.append(m1);
    
    TeamMember m2;
    m2.id = "2";
    m2.name = tr("Jane Smith");
    m2.email = "jane@example.com";
    m2.role = tr("Developer");
    m2.joinedAt = QDateTime::currentDateTime().addDays(-180);
    m2.isOnline = true;
    members_.append(m2);
    
    TeamMember m3;
    m3.id = "3";
    m3.name = tr("Bob Wilson");
    m3.email = "bob@example.com";
    m3.role = tr("Analyst");
    m3.joinedAt = QDateTime::currentDateTime().addDays(-90);
    m3.isOnline = false;
    members_.append(m3);
    
    updateMembersList();
}

void TeamMembersDialog::updateMembersList() {
    membersModel_->clear();
    membersModel_->setHorizontalHeaderLabels({tr("Name"), tr("Email"), tr("Role"), tr("Joined"), tr("Status")});
    
    for (const auto& member : members_) {
        auto* row = new QList<QStandardItem*>();
        *row << new QStandardItem(member.name)
             << new QStandardItem(member.email)
             << new QStandardItem(member.role)
             << new QStandardItem(member.joinedAt.toString("yyyy-MM-dd"))
             << new QStandardItem(member.isOnline ? tr("● Online") : tr("○ Offline"));
        
        (*row)[4]->setForeground(QBrush(member.isOnline ? QColor(0, 150, 0) : QColor(150, 150, 150)));
        
        membersModel_->appendRow(*row);
    }
    
    teamStatsLabel_->setText(tr("%1 team members (%2 online)")
        .arg(members_.size())
        .arg(std::count_if(members_.begin(), members_.end(), 
            [](const TeamMember& m) { return m.isOnline; })));
}

void TeamMembersDialog::onInviteMember() {
    bool ok;
    QString email = QInputDialog::getText(this, tr("Invite Member"),
        tr("Enter email address:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !email.isEmpty()) {
        QMessageBox::information(this, tr("Invite Sent"),
            tr("Invitation sent to %1").arg(email));
    }
}

void TeamMembersDialog::onRemoveMember() {
    auto index = membersTable_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, tr("Remove"), tr("Please select a member to remove."));
        return;
    }
    
    auto reply = QMessageBox::question(this, tr("Remove Member"),
        tr("Are you sure you want to remove this member?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: Remove member
        QMessageBox::information(this, tr("Removed"), tr("Member removed."));
    }
}

void TeamMembersDialog::onChangeRole() {
    // TODO: Change member role
}

void TeamMembersDialog::onSendMessage() {
    auto index = membersTable_->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, tr("Message"), tr("Please select a member."));
        return;
    }
    
    bool ok;
    QString message = QInputDialog::getMultiLineText(this, tr("Send Message"),
        tr("Enter your message:"), QString(), &ok);
    
    if (ok && !message.isEmpty()) {
        QMessageBox::information(this, tr("Sent"), tr("Message sent."));
    }
}

void TeamMembersDialog::onRefreshMembers() {
    loadMembers();
}

// ============================================================================
// Activity Feed Dialog
// ============================================================================
ActivityFeedDialog::ActivityFeedDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
    loadActivities();
}

void ActivityFeedDialog::setupUi() {
    setWindowTitle(tr("Activity Feed"));
    setMinimumSize(600, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Filters
    auto* filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel(tr("Type:"), this));
    typeFilterCombo_ = new QComboBox(this);
    typeFilterCombo_->addItems({tr("All"), tr("Created"), tr("Modified"), tr("Shared"), tr("Commented")});
    connect(typeFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ActivityFeedDialog::onFilterByType);
    filterLayout->addWidget(typeFilterCombo_);
    
    filterLayout->addWidget(new QLabel(tr("User:"), this));
    userFilterCombo_ = new QComboBox(this);
    userFilterCombo_->addItems({tr("All"), tr("Me"), tr("John"), tr("Jane")});
    connect(userFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                onFilterByUser(userFilterCombo_->itemText(index));
            });
    filterLayout->addWidget(userFilterCombo_);
    
    filterLayout->addStretch();
    
    auto* clearBtn = new QPushButton(tr("Clear Filter"), this);
    connect(clearBtn, &QPushButton::clicked, this, &ActivityFeedDialog::onClearFilter);
    filterLayout->addWidget(clearBtn);
    
    layout->addLayout(filterLayout);
    
    // Activity table
    activityTable_ = new QTableView(this);
    activityModel_ = new QStandardItemModel(this);
    activityModel_->setHorizontalHeaderLabels({tr("Time"), tr("User"), tr("Action"), tr("Item")});
    activityTable_->setModel(activityModel_);
    activityTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    activityTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(activityTable_, 1);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &ActivityFeedDialog::onRefresh);
    btnLayout->addWidget(refreshBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ActivityFeedDialog::loadActivities() {
    // TODO: Load from backend
}

void ActivityFeedDialog::updateActivityList() {
    activityModel_->clear();
    activityModel_->setHorizontalHeaderLabels({tr("Time"), tr("User"), tr("Action"), tr("Item")});
    
    // Sample data
    auto* row1 = new QList<QStandardItem*>();
    *row1 << new QStandardItem(QDateTime::currentDateTime().addSecs(-300).toString("yyyy-MM-dd hh:mm"))
          << new QStandardItem(tr("John Doe"))
          << new QStandardItem(tr("Created"))
          << new QStandardItem(tr("Common JOIN Pattern"));
    activityModel_->appendRow(*row1);
    
    auto* row2 = new QList<QStandardItem*>();
    *row2 << new QStandardItem(QDateTime::currentDateTime().addSecs(-900).toString("yyyy-MM-dd hh:mm"))
          << new QStandardItem(tr("Jane Smith"))
          << new QStandardItem(tr("Modified"))
          << new QStandardItem(tr("Monthly Sales Report"));
    activityModel_->appendRow(*row2);
    
    auto* row3 = new QList<QStandardItem*>();
    *row3 << new QStandardItem(QDateTime::currentDateTime().addSecs(-1800).toString("yyyy-MM-dd hh:mm"))
          << new QStandardItem(tr("Bob Wilson"))
          << new QStandardItem(tr("Shared"))
          << new QStandardItem(tr("Backup Script"));
    activityModel_->appendRow(*row3);
}

void ActivityFeedDialog::onFilterByType(int type) {
    Q_UNUSED(type)
    updateActivityList();
}

void ActivityFeedDialog::onFilterByUser(const QString& user) {
    Q_UNUSED(user)
    updateActivityList();
}

void ActivityFeedDialog::onClearFilter() {
    typeFilterCombo_->setCurrentIndex(0);
    userFilterCombo_->setCurrentIndex(0);
    updateActivityList();
}

void ActivityFeedDialog::onRefresh() {
    updateActivityList();
}

void ActivityFeedDialog::onViewActivityDetails() {
    // TODO: Show activity details
}

// ============================================================================
// Import Shared Item Dialog
// ============================================================================
ImportSharedItemDialog::ImportSharedItemDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUi();
}

void ImportSharedItemDialog::setupUi() {
    setWindowTitle(tr("Import Shared Item"));
    setMinimumSize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    
    // Search
    auto* searchLayout = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("Search for items..."));
    searchLayout->addWidget(searchEdit_);
    
    auto* searchBtn = new QPushButton(tr("Search"), this);
    connect(searchBtn, &QPushButton::clicked, this, &ImportSharedItemDialog::onSearch);
    searchLayout->addWidget(searchBtn);
    
    layout->addLayout(searchLayout);
    
    // Results table
    resultsTable_ = new QTableView(this);
    resultsModel_ = new QStandardItemModel(this);
    resultsModel_->setHorizontalHeaderLabels({tr("Name"), tr("Author"), tr("Type"), tr("Rating")});
    resultsTable_->setModel(resultsModel_);
    resultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(resultsTable_);
    
    // Preview
    layout->addWidget(new QLabel(tr("Preview:"), this));
    previewEdit_ = new QTextEdit(this);
    previewEdit_->setReadOnly(true);
    previewEdit_->setMaximumHeight(100);
    layout->addWidget(previewEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* previewBtn = new QPushButton(tr("Preview"), this);
    connect(previewBtn, &QPushButton::clicked, this, &ImportSharedItemDialog::onPreviewItem);
    btnLayout->addWidget(previewBtn);
    
    importBtn_ = new QPushButton(tr("Import"), this);
    importBtn_->setEnabled(false);
    connect(importBtn_, &QPushButton::clicked, this, &ImportSharedItemDialog::onImport);
    btnLayout->addWidget(importBtn_);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
}

void ImportSharedItemDialog::searchItems(const QString& query) {
    Q_UNUSED(query)
    // TODO: Search backend
}

void ImportSharedItemDialog::onSearch() {
    searchItems(searchEdit_->text());
}

void ImportSharedItemDialog::onItemSelected(const QModelIndex& index) {
    Q_UNUSED(index)
    importBtn_->setEnabled(true);
}

void ImportSharedItemDialog::onPreviewItem() {
    previewEdit_->setText(tr("Item preview would be shown here."));
}

void ImportSharedItemDialog::onImport() {
    accept();
}

QString ImportSharedItemDialog::selectedItemId() const {
    return selectedItemId_;
}

} // namespace scratchrobin::ui
