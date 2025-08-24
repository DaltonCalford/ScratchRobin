#include "favorites_manager_dialog.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QApplication>
#include <QStyle>
#include <QTreeWidgetItem>
#include <QListWidgetItem>
#include <QUuid>

namespace scratchrobin {

QJsonObject FavoriteQuery::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["category"] = category;
    json["queryText"] = queryText;
    json["connectionName"] = connectionName;
    json["databaseName"] = databaseName;
    json["createdDate"] = createdDate.toString(Qt::ISODate);
    json["modifiedDate"] = modifiedDate.toString(Qt::ISODate);
    json["lastUsed"] = lastUsed.toString(Qt::ISODate);
    json["usageCount"] = usageCount;
    json["isDefault"] = isDefault;

    QJsonArray tagsArray;
    for (const QString& tag : tags) {
        tagsArray.append(tag);
    }
    json["tags"] = tagsArray;

    json["metadata"] = metadata;
    return json;
}

FavoriteQuery FavoriteQuery::fromJson(const QJsonObject& json) {
    FavoriteQuery favorite;
    favorite.id = json["id"].toString();
    favorite.name = json["name"].toString();
    favorite.description = json["description"].toString();
    favorite.category = json["category"].toString();
    favorite.queryText = json["queryText"].toString();
    favorite.connectionName = json["connectionName"].toString();
    favorite.databaseName = json["databaseName"].toString();
    favorite.createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    favorite.modifiedDate = QDateTime::fromString(json["modifiedDate"].toString(), Qt::ISODate);
    favorite.lastUsed = QDateTime::fromString(json["lastUsed"].toString(), Qt::ISODate);
    favorite.usageCount = json["usageCount"].toInt();
    favorite.isDefault = json["isDefault"].toBool();

    QJsonArray tagsArray = json["tags"].toArray();
    for (const QJsonValue& tagValue : tagsArray) {
        favorite.tags.append(tagValue.toString());
    }

    favorite.metadata = json["metadata"].toObject();
    return favorite;
}

QJsonObject FavoriteCategory::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["parentId"] = parentId;
    json["color"] = color;
    json["iconName"] = iconName;
    json["sortOrder"] = sortOrder;
    json["createdDate"] = createdDate.toString(Qt::ISODate);

    QJsonArray favoritesArray;
    for (const QString& favoriteId : favoriteIds) {
        favoritesArray.append(favoriteId);
    }
    json["favoriteIds"] = favoritesArray;

    return json;
}

FavoriteCategory FavoriteCategory::fromJson(const QJsonObject& json) {
    FavoriteCategory category;
    category.id = json["id"].toString();
    category.name = json["name"].toString();
    category.description = json["description"].toString();
    category.parentId = json["parentId"].toString();
    category.color = json["color"].toString();
    category.iconName = json["iconName"].toString();
    category.sortOrder = json["sortOrder"].toInt();
    category.createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);

    QJsonArray favoritesArray = json["favoriteIds"].toArray();
    for (const QJsonValue& favoriteValue : favoritesArray) {
        category.favoriteIds.append(favoriteValue.toString());
    }

    return category;
}

FavoritesManagerDialog::FavoritesManagerDialog(QWidget* parent)
    : QDialog(parent), settings_(new QSettings("ScratchRobin", "Favorites", this)) {
    setWindowTitle("Query Favorites Manager");
    setModal(true);
    setMinimumSize(900, 600);
    resize(1100, 700);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupUI();
    loadCategories();
    loadFavorites();
}

void FavoritesManagerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(10);
    mainLayout_->setContentsMargins(15, 15, 15, 15);

    // Create main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal);
    mainLayout_->addWidget(mainSplitter_);

    // Create tab widget
    tabWidget_ = new QTabWidget();
    mainSplitter_->addWidget(tabWidget_);

    // Setup tabs
    setupFavoritesTab();
    setupCategoriesTab();
    setupSettingsTab();

    // Setup preview panel
    setupPreviewPanel();

    // Set splitter sizes
    mainSplitter_->setSizes({700, 300});

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(dialogButtons_, &QDialogButtonBox::accepted, this, &FavoritesManagerDialog::accept);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &FavoritesManagerDialog::reject);

    mainLayout_->addWidget(dialogButtons_);
}

void FavoritesManagerDialog::setupFavoritesTab() {
    favoritesTab_ = new QWidget();
    favoritesLayout_ = new QHBoxLayout(favoritesTab_);

    // Left panel - controls
    leftPanel_ = new QWidget();
    leftLayout_ = new QVBoxLayout(leftPanel_);

    // Category selection
    QGroupBox* categoryGroup = new QGroupBox("Category");
    QFormLayout* categoryLayout = new QFormLayout(categoryGroup);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItem("All Categories", "");
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FavoritesManagerDialog::onCategoryChanged);
    categoryLayout->addRow("Filter by:", categoryCombo_);
    leftLayout_->addWidget(categoryGroup);

    // Search
    QGroupBox* searchGroup = new QGroupBox("Search");
    QFormLayout* searchLayout = new QFormLayout(searchGroup);
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search favorites...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &FavoritesManagerDialog::onSearchTextChanged);
    searchLayout->addRow("Search:", searchEdit_);
    leftLayout_->addWidget(searchGroup);

    // Filters
    filterGroup_ = new QGroupBox("Filters");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterGroup_);

    showAllCategoriesCheck_ = new QCheckBox("Show from all categories");
    showAllCategoriesCheck_->setChecked(showAllCategories_);
    connect(showAllCategoriesCheck_, &QCheckBox::toggled, this, &FavoritesManagerDialog::onFilterChanged);
    filterLayout->addWidget(showAllCategoriesCheck_);

    showRecentlyUsedCheck_ = new QCheckBox("Show recently used");
    showRecentlyUsedCheck_->setChecked(showRecentlyUsed_);
    connect(showRecentlyUsedCheck_, &QCheckBox::toggled, this, &FavoritesManagerDialog::onFilterChanged);
    filterLayout->addWidget(showRecentlyUsedCheck_);

    // Sort by
    QLabel* sortLabel = new QLabel("Sort by:");
    sortByCombo_ = new QComboBox();
    sortByCombo_->addItems({"Name", "Date Created", "Date Modified", "Last Used", "Usage Count"});
    connect(sortByCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FavoritesManagerDialog::onFilterChanged);
    filterLayout->addWidget(sortLabel);
    filterLayout->addWidget(sortByCombo_);

    leftLayout_->addWidget(filterGroup_);
    leftLayout_->addStretch();

    // Refresh button
    QPushButton* refreshButton = new QPushButton("Refresh");
    connect(refreshButton, &QPushButton::clicked, this, &FavoritesManagerDialog::onRefreshList);
    leftLayout_->addWidget(refreshButton);

    favoritesLayout_->addWidget(leftPanel_, 1);

    // Center - favorites list
    QGroupBox* favoritesGroup = new QGroupBox("Query Favorites");
    QVBoxLayout* favoritesGroupLayout = new QVBoxLayout(favoritesGroup);

    favoritesList_ = new QListWidget();
    favoritesList_->setAlternatingRowColors(true);
    favoritesList_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(favoritesList_, &QListWidget::itemSelectionChanged,
            this, &FavoritesManagerDialog::onFavoriteSelectionChanged);

    favoritesGroupLayout->addWidget(favoritesList_);
    favoritesLayout_->addWidget(favoritesGroup, 2);

    // Action buttons
    QWidget* buttonPanel = new QWidget();
    buttonLayout_ = new QHBoxLayout(buttonPanel);

    addButton_ = new QPushButton("Add");
    addButton_->setIcon(QIcon(":/icons/add.png"));
    connect(addButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onAddFavorite);
    buttonLayout_->addWidget(addButton_);

    editButton_ = new QPushButton("Edit");
    editButton_->setIcon(QIcon(":/icons/edit.png"));
    connect(editButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onEditFavorite);
    buttonLayout_->addWidget(editButton_);

    deleteButton_ = new QPushButton("Delete");
    deleteButton_->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onDeleteFavorite);
    buttonLayout_->addWidget(deleteButton_);

    duplicateButton_ = new QPushButton("Duplicate");
    duplicateButton_->setIcon(QIcon(":/icons/duplicate.png"));
    connect(duplicateButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onDuplicateFavorite);
    buttonLayout_->addWidget(duplicateButton_);

    executeButton_ = new QPushButton("Execute");
    executeButton_->setIcon(QIcon(":/icons/execute.png"));
    connect(executeButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onExecuteFavorite);
    buttonLayout_->addWidget(executeButton_);

    setDefaultButton_ = new QPushButton("Set Default");
    setDefaultButton_->setIcon(QIcon(":/icons/default.png"));
    connect(setDefaultButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onSetAsDefault);
    buttonLayout_->addWidget(setDefaultButton_);

    favoritesGroupLayout->addWidget(buttonPanel);

    tabWidget_->addTab(favoritesTab_, "Favorites");
}

void FavoritesManagerDialog::setupCategoriesTab() {
    categoriesTab_ = new QWidget();
    categoriesLayout_ = new QVBoxLayout(categoriesTab_);

    // Categories tree
    QGroupBox* treeGroup = new QGroupBox("Categories");
    QVBoxLayout* treeLayout = new QVBoxLayout(treeGroup);

    categoriesTree_ = new QTreeWidget();
    categoriesTree_->setHeaderLabel("Categories");
    categoriesTree_->setAlternatingRowColors(true);
    categoriesTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(categoriesTree_, &QTreeWidget::itemSelectionChanged,
            this, &FavoritesManagerDialog::onCategorySelectionChanged);

    treeLayout->addWidget(categoriesTree_);

    // Category action buttons
    categoryButtonLayout_ = new QHBoxLayout();

    addCategoryButton_ = new QPushButton("Add");
    addCategoryButton_->setIcon(QIcon(":/icons/add.png"));
    connect(addCategoryButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onAddCategory);
    categoryButtonLayout_->addWidget(addCategoryButton_);

    editCategoryButton_ = new QPushButton("Edit");
    editCategoryButton_->setIcon(QIcon(":/icons/edit.png"));
    connect(editCategoryButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onEditCategory);
    categoryButtonLayout_->addWidget(editCategoryButton_);

    deleteCategoryButton_ = new QPushButton("Delete");
    deleteCategoryButton_->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteCategoryButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onDeleteCategory);
    categoryButtonLayout_->addWidget(deleteCategoryButton_);

    treeLayout->addLayout(categoryButtonLayout_);

    categoriesLayout_->addWidget(treeGroup);
    categoriesLayout_->addStretch();

    tabWidget_->addTab(categoriesTab_, "Categories");
}

void FavoritesManagerDialog::setupSettingsTab() {
    settingsTab_ = new QWidget();
    settingsLayout_ = new QVBoxLayout(settingsTab_);

    // General settings
    QGroupBox* generalGroup = new QGroupBox("General Settings");
    QVBoxLayout* generalLayout = new QVBoxLayout(generalGroup);

    autoSaveCheck_ = new QCheckBox("Auto-save favorites when modified");
    autoSaveCheck_->setChecked(true);
    generalLayout->addWidget(autoSaveCheck_);

    confirmDeleteCheck_ = new QCheckBox("Confirm before deleting favorites");
    confirmDeleteCheck_->setChecked(confirmDelete_);
    generalLayout->addWidget(confirmDeleteCheck_);

    showPreviewCheck_ = new QCheckBox("Show query preview in details panel");
    showPreviewCheck_->setChecked(showPreview_);
    generalLayout->addWidget(showPreviewCheck_);

    autoExecuteCheck_ = new QCheckBox("Auto-execute favorite when double-clicked");
    autoExecuteCheck_->setChecked(autoExecute_);
    generalLayout->addWidget(autoExecuteCheck_);

    settingsLayout_->addWidget(generalGroup);

    // Import/Export
    QGroupBox* importExportGroup = new QGroupBox("Import/Export");
    QHBoxLayout* importExportLayout = new QHBoxLayout(importExportGroup);

    importButton_ = new QPushButton("Import Favorites");
    importButton_->setIcon(QIcon(":/icons/import.png"));
    connect(importButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onImportFavorites);
    importExportLayout->addWidget(importButton_);

    exportButton_ = new QPushButton("Export Favorites");
    exportButton_->setIcon(QIcon(":/icons/export.png"));
    connect(exportButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onExportFavorites);
    importExportLayout->addWidget(exportButton_);

    settingsLayout_->addWidget(importExportGroup);

    // Backup/Restore
    QGroupBox* backupGroup = new QGroupBox("Backup & Restore");
    QHBoxLayout* backupLayout = new QHBoxLayout(backupGroup);

    backupButton_ = new QPushButton("Create Backup");
    backupButton_->setIcon(QIcon(":/icons/backup.png"));
    // connect(backupButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onCreateBackup);
    backupLayout->addWidget(backupButton_);

    restoreButton_ = new QPushButton("Restore from Backup");
    restoreButton_->setIcon(QIcon(":/icons/restore.png"));
    // connect(restoreButton_, &QPushButton::clicked, this, &FavoritesManagerDialog::onRestoreFromBackup);
    backupLayout->addWidget(restoreButton_);

    settingsLayout_->addWidget(backupGroup);
    settingsLayout_->addStretch();

    tabWidget_->addTab(settingsTab_, "Settings");
}

void FavoritesManagerDialog::setupPreviewPanel() {
    previewPanel_ = new QWidget();
    previewLayout_ = new QVBoxLayout(previewPanel_);

    previewGroup_ = new QGroupBox("Query Preview");
    QVBoxLayout* previewGroupLayout = new QVBoxLayout(previewGroup_);

    previewInfoLabel_ = new QLabel("Select a favorite to preview");
    previewInfoLabel_->setStyleSheet("font-style: italic; color: gray;");
    previewGroupLayout->addWidget(previewInfoLabel_);

    previewTextEdit_ = new QTextEdit();
    previewTextEdit_->setReadOnly(true);
    previewTextEdit_->setMaximumHeight(300);
    previewGroupLayout->addWidget(previewTextEdit_);

    previewLayout_->addWidget(previewGroup_);

    mainSplitter_->addWidget(previewPanel_);
}

void FavoritesManagerDialog::loadFavorites() {
    favorites_.clear();

    // Load from settings
    int size = settings_->beginReadArray("favorites");
    for (int i = 0; i < size; ++i) {
        settings_->setArrayIndex(i);
        QJsonObject json;
        for (const QString& key : settings_->childKeys()) {
            json[key] = settings_->value(key).toString();
        }
        FavoriteQuery favorite = FavoriteQuery::fromJson(json);
        favorites_[favorite.id] = favorite;
    }
    settings_->endArray();

    populateFavoritesList();
    updateButtonStates();
}

void FavoritesManagerDialog::saveFavorites() {
    // Clear existing favorites
    settings_->remove("favorites");

    // Save current favorites
    settings_->beginWriteArray("favorites");
    int index = 0;
    for (const FavoriteQuery& favorite : favorites_) {
        settings_->setArrayIndex(index);
        QJsonObject json = favorite.toJson();
        for (auto it = json.begin(); it != json.end(); ++it) {
            settings_->setValue(it.key(), it.value().toVariant());
        }
        index++;
    }
    settings_->endArray();
}

void FavoritesManagerDialog::loadCategories() {
    categories_.clear();

    // Add default categories
    categories_["general"] = FavoriteCategory("general", "General");
    categories_["reports"] = FavoriteCategory("reports", "Reports");
    categories_["maintenance"] = FavoriteCategory("maintenance", "Maintenance");
    categories_["analysis"] = FavoriteCategory("analysis", "Data Analysis");

    // Load from settings
    int size = settings_->beginReadArray("categories");
    for (int i = 0; i < size; ++i) {
        settings_->setArrayIndex(i);
        QJsonObject json;
        for (const QString& key : settings_->childKeys()) {
            json[key] = settings_->value(key).toString();
        }
        FavoriteCategory category = FavoriteCategory::fromJson(json);
        categories_[category.id] = category;
    }
    settings_->endArray();

    populateCategoriesTree();
    updateCategoryCombo();
}

void FavoritesManagerDialog::saveCategories() {
    // Clear existing categories
    settings_->remove("categories");

    // Save current categories
    settings_->beginWriteArray("categories");
    int index = 0;
    for (const FavoriteCategory& category : categories_) {
        settings_->setArrayIndex(index);
        QJsonObject json = category.toJson();
        for (auto it = json.begin(); it != json.end(); ++it) {
            settings_->setValue(it.key(), it.value().toVariant());
        }
        index++;
    }
    settings_->endArray();
}

void FavoritesManagerDialog::populateFavoritesList() {
    favoritesList_->clear();

    QList<FavoriteQuery> filteredFavorites = filterFavorites();

    for (const FavoriteQuery& favorite : filteredFavorites) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(favorite.name);
        item->setToolTip(favorite.description);
        item->setData(Qt::UserRole, favorite.id);

        // Set icon based on category
        if (favorite.isDefault) {
            item->setIcon(QIcon(":/icons/default.png"));
            item->setFont(QFont("", -1, QFont::Bold));
        } else {
            item->setIcon(QIcon(":/icons/favorite.png"));
        }

        favoritesList_->addItem(item);
    }
}

void FavoritesManagerDialog::populateCategoriesTree() {
    categoriesTree_->clear();

    for (const FavoriteCategory& category : categories_) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, category.name);
        item->setToolTip(0, category.description);
        item->setData(0, Qt::UserRole, category.id);
        categoriesTree_->addTopLevelItem(item);
    }
}

void FavoritesManagerDialog::updateCategoryCombo() {
    categoryCombo_->clear();
    categoryCombo_->addItem("All Categories", "");

    for (const FavoriteCategory& category : categories_) {
        categoryCombo_->addItem(category.name, category.id);
    }
}

void FavoritesManagerDialog::updateFavoriteDetails(const FavoriteQuery& favorite) {
    previewTextEdit_->setPlainText(favorite.queryText);

    QString info = QString(
        "Name: %1\n"
        "Category: %2\n"
        "Description: %3\n"
        "Connection: %4\n"
        "Database: %5\n"
        "Created: %6\n"
        "Modified: %7\n"
        "Last Used: %8\n"
        "Usage Count: %9\n"
        "Tags: %10"
    )
    .arg(favorite.name)
    .arg(favorite.category.isEmpty() ? "None" : favorite.category)
    .arg(favorite.description.isEmpty() ? "None" : favorite.description)
    .arg(favorite.connectionName.isEmpty() ? "Any" : favorite.connectionName)
    .arg(favorite.databaseName.isEmpty() ? "Any" : favorite.databaseName)
    .arg(favorite.createdDate.toString("yyyy-MM-dd hh:mm"))
    .arg(favorite.modifiedDate.toString("yyyy-MM-dd hh:mm"))
    .arg(favorite.lastUsed.toString("yyyy-MM-dd hh:mm"))
    .arg(favorite.usageCount)
    .arg(favorite.tags.isEmpty() ? "None" : favorite.tags.join(", "));

    previewInfoLabel_->setText(info);
}

void FavoritesManagerDialog::clearFavoriteDetails() {
    previewTextEdit_->clear();
    previewInfoLabel_->setText("Select a favorite to preview");
}

QList<FavoriteQuery> FavoritesManagerDialog::filterFavorites() const {
    QList<FavoriteQuery> result;

    for (const FavoriteQuery& favorite : favorites_) {
        // Category filter
        if (!currentCategory_.isEmpty() && favorite.category != currentCategory_) {
            if (!showAllCategories_) {
                continue;
            }
        }

        // Search filter
        if (!currentSearchText_.isEmpty()) {
            QString searchText = currentSearchText_.toLower();
            if (!favorite.name.toLower().contains(searchText) &&
                !favorite.description.toLower().contains(searchText) &&
                !favorite.queryText.toLower().contains(searchText)) {
                continue;
            }
        }

        result.append(favorite);
    }

    // Sort results
    sortFavorites(result, currentSortBy_);
    return result;
}

QList<FavoriteQuery> FavoritesManagerDialog::searchFavorites(const QString& searchText) const {
    QList<FavoriteQuery> result;

    for (const FavoriteQuery& favorite : favorites_) {
        if (favorite.name.toLower().contains(searchText.toLower()) ||
            favorite.description.toLower().contains(searchText.toLower()) ||
            favorite.queryText.toLower().contains(searchText.toLower())) {
            result.append(favorite);
        }
    }

    return result;
}

void FavoritesManagerDialog::sortFavorites(QList<FavoriteQuery>& favorites, const QString& sortBy) const {
    if (sortBy == "name") {
        std::sort(favorites.begin(), favorites.end(),
                 [](const FavoriteQuery& a, const FavoriteQuery& b) {
                     return a.name < b.name;
                 });
    } else if (sortBy == "createdDate") {
        std::sort(favorites.begin(), favorites.end(),
                 [](const FavoriteQuery& a, const FavoriteQuery& b) {
                     return a.createdDate < b.createdDate;
                 });
    } else if (sortBy == "lastUsed") {
        std::sort(favorites.begin(), favorites.end(),
                 [](const FavoriteQuery& a, const FavoriteQuery& b) {
                     return a.lastUsed < b.lastUsed;
                 });
    } else if (sortBy == "usageCount") {
        std::sort(favorites.begin(), favorites.end(),
                 [](const FavoriteQuery& a, const FavoriteQuery& b) {
                     return a.usageCount > b.usageCount; // Descending order
                 });
    }
}

QString FavoritesManagerDialog::generateUniqueId() const {
    return QUuid::createUuid().toString().remove("{").remove("}").remove("-");
}

// Event handlers
void FavoritesManagerDialog::onCategoryChanged(int index) {
    if (index >= 0 && index < categoryCombo_->count()) {
        currentCategory_ = categoryCombo_->itemData(index).toString();
        populateFavoritesList();
    }
}

void FavoritesManagerDialog::onSearchTextChanged(const QString& text) {
    currentSearchText_ = text;
    populateFavoritesList();
}

void FavoritesManagerDialog::onFavoriteSelectionChanged() {
    QList<QListWidgetItem*> selectedItems = favoritesList_->selectedItems();
    if (selectedItems.isEmpty()) {
        clearFavoriteDetails();
        currentFavorite_ = FavoriteQuery();
    } else {
        QListWidgetItem* item = selectedItems.first();
        QString favoriteId = item->data(Qt::UserRole).toString();
        currentFavorite_ = favorites_[favoriteId];
        updateFavoriteDetails(currentFavorite_);
    }

    updateButtonStates();
}

void FavoritesManagerDialog::onCategorySelectionChanged() {
    // Handle category selection
}

void FavoritesManagerDialog::onAddFavorite() {
    // TODO: Implement add favorite dialog
    QMessageBox::information(this, "Add Favorite", "Add favorite functionality not yet implemented.");
}

void FavoritesManagerDialog::onEditFavorite() {
    if (currentFavorite_.id.isEmpty()) {
        return;
    }

    // TODO: Implement edit favorite dialog
    QMessageBox::information(this, "Edit Favorite", "Edit favorite functionality not yet implemented.");
}

void FavoritesManagerDialog::onDeleteFavorite() {
    if (currentFavorite_.id.isEmpty()) {
        return;
    }

    if (confirmDeleteCheck_->isChecked()) {
        if (QMessageBox::question(this, "Delete Favorite",
                                QString("Are you sure you want to delete '%1'?").arg(currentFavorite_.name),
                                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
    }

    favorites_.remove(currentFavorite_.id);
    populateFavoritesList();
    clearFavoriteDetails();
    saveFavorites();

    QMessageBox::information(this, "Deleted", "Favorite has been deleted.");
}

void FavoritesManagerDialog::onExecuteFavorite() {
    if (currentFavorite_.id.isEmpty()) {
        return;
    }

    // Update usage statistics
    FavoriteQuery updated = currentFavorite_;
    updated.lastUsed = QDateTime::currentDateTime();
    updated.usageCount++;
    favorites_[updated.id] = updated;

    emit favoriteExecuted(currentFavorite_);
    saveFavorites();
}

void FavoritesManagerDialog::onDuplicateFavorite() {
    if (currentFavorite_.id.isEmpty()) {
        return;
    }

    FavoriteQuery duplicate = currentFavorite_;
    duplicate.id = generateUniqueId();
    duplicate.name = QString("%1 (Copy)").arg(duplicate.name);
    duplicate.createdDate = QDateTime::currentDateTime();
    duplicate.modifiedDate = QDateTime::currentDateTime();
    duplicate.usageCount = 0;
    duplicate.isDefault = false;

    favorites_[duplicate.id] = duplicate;
    populateFavoritesList();
    saveFavorites();
}

void FavoritesManagerDialog::onSetAsDefault() {
    if (currentFavorite_.id.isEmpty()) {
        return;
    }

    // Clear default flag from all favorites
    for (FavoriteQuery& favorite : favorites_) {
        favorite.isDefault = false;
    }

    // Set current favorite as default
    FavoriteQuery updated = currentFavorite_;
    updated.isDefault = true;
    favorites_[updated.id] = updated;

    populateFavoritesList();
    saveFavorites();

    QMessageBox::information(this, "Set as Default",
                           QString("'%1' has been set as the default favorite.").arg(updated.name));
}

void FavoritesManagerDialog::onAddCategory() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Category", "Category name:", QLineEdit::Normal, "", &ok);

    if (ok && !name.isEmpty()) {
        FavoriteCategory category;
        category.id = generateUniqueId();
        category.name = name;
        category.createdDate = QDateTime::currentDateTime();

        categories_[category.id] = category;
        populateCategoriesTree();
        updateCategoryCombo();
        saveCategories();
    }
}

void FavoritesManagerDialog::onEditCategory() {
    // TODO: Implement edit category
    QMessageBox::information(this, "Edit Category", "Edit category functionality not yet implemented.");
}

void FavoritesManagerDialog::onDeleteCategory() {
    // TODO: Implement delete category
    QMessageBox::information(this, "Delete Category", "Delete category functionality not yet implemented.");
}

void FavoritesManagerDialog::onImportFavorites() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Favorites",
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                   "Favorites Files (*.json);;All Files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    // TODO: Implement import functionality
    QMessageBox::information(this, "Import", "Import functionality not yet implemented.");
}

void FavoritesManagerDialog::onExportFavorites() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Favorites",
                                                   QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                   "Favorites Files (*.json);;All Files (*.*)");

    if (fileName.isEmpty()) {
        return;
    }

    // TODO: Implement export functionality
    QMessageBox::information(this, "Export", "Export functionality not yet implemented.");
}

void FavoritesManagerDialog::onRefreshList() {
    populateFavoritesList();
}

void FavoritesManagerDialog::onFilterChanged() {
    showAllCategories_ = showAllCategoriesCheck_->isChecked();
    showRecentlyUsed_ = showRecentlyUsedCheck_->isChecked();

    QStringList sortOptions = {"name", "createdDate", "modifiedDate", "lastUsed", "usageCount"};
    currentSortBy_ = sortOptions[sortByCombo_->currentIndex()];

    populateFavoritesList();
}

void FavoritesManagerDialog::updateButtonStates() {
    bool hasSelection = !favoritesList_->selectedItems().isEmpty();
    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
    executeButton_->setEnabled(hasSelection);
    duplicateButton_->setEnabled(hasSelection);
    setDefaultButton_->setEnabled(hasSelection);
}

void FavoritesManagerDialog::accept() {
    saveFavorites();
    saveCategories();
    emit favoritesChanged();
    QDialog::accept();
}

void FavoritesManagerDialog::reject() {
    QDialog::reject();
}

// Static convenience methods
void FavoritesManagerDialog::showFavoritesManager(QWidget* parent) {
    FavoritesManagerDialog dialog(parent);
    dialog.exec();
}

FavoriteQuery FavoritesManagerDialog::getFavoriteQuery(QWidget* parent, const QString& category) {
    FavoritesManagerDialog dialog(parent);
    if (!category.isEmpty()) {
        dialog.currentCategory_ = category;
    }

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.currentFavorite_;
    }

    return FavoriteQuery();
}

bool FavoritesManagerDialog::saveCurrentQueryAsFavorite(QWidget* parent, const QString& queryText,
                                                       const QString& connectionName,
                                                       const QString& databaseName) {
    FavoritesManagerDialog dialog(parent);

    FavoriteQuery favorite;
    favorite.id = dialog.generateUniqueId();
    favorite.name = QString("Query %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    favorite.queryText = queryText;
    favorite.connectionName = connectionName;
    favorite.databaseName = databaseName;
    favorite.createdDate = QDateTime::currentDateTime();
    favorite.modifiedDate = QDateTime::currentDateTime();

    dialog.favorites_[favorite.id] = favorite;
    dialog.saveFavorites();

    QMessageBox::information(parent, "Saved",
                           QString("Query has been saved as favorite '%1'").arg(favorite.name));

    return true;
}

} // namespace scratchrobin
