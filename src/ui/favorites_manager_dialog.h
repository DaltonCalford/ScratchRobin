#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QSplitter>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QSet>
#include <QDateTime>
#include <QJsonObject>
#include <QListWidget>

namespace scratchrobin {

struct FavoriteQuery {
    QString id;
    QString name;
    QString description;
    QString category;
    QString queryText;
    QString connectionName;
    QString databaseName;
    QDateTime createdDate;
    QDateTime modifiedDate;
    QDateTime lastUsed;
    int usageCount = 0;
    bool isDefault = false;
    QStringList tags;
    QJsonObject metadata;

    FavoriteQuery() = default;
    FavoriteQuery(const QString& id, const QString& name, const QString& queryText)
        : id(id), name(name), queryText(queryText), createdDate(QDateTime::currentDateTime()),
          modifiedDate(QDateTime::currentDateTime()), lastUsed(QDateTime::currentDateTime()) {}

    QJsonObject toJson() const;
    static FavoriteQuery fromJson(const QJsonObject& json);
};

struct FavoriteCategory {
    QString id;
    QString name;
    QString description;
    QString parentId;
    QString color;
    QString iconName;
    int sortOrder = 0;
    QDateTime createdDate;
    QList<QString> favoriteIds;

    FavoriteCategory() = default;
    FavoriteCategory(const QString& id, const QString& name)
        : id(id), name(name), createdDate(QDateTime::currentDateTime()) {}

    QJsonObject toJson() const;
    static FavoriteCategory fromJson(const QJsonObject& json);
};

class FavoritesManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit FavoritesManagerDialog(QWidget* parent = nullptr);
    ~FavoritesManagerDialog() override = default;

    // Static convenience methods
    static void showFavoritesManager(QWidget* parent);
    static FavoriteQuery getFavoriteQuery(QWidget* parent, const QString& category = QString());
    static bool saveCurrentQueryAsFavorite(QWidget* parent, const QString& queryText,
                                          const QString& connectionName = QString(),
                                          const QString& databaseName = QString());

    // Favorite management
    void loadFavorites();
    void saveFavorites();
    bool addFavorite(const FavoriteQuery& favorite);
    bool updateFavorite(const QString& id, const FavoriteQuery& favorite);
    bool removeFavorite(const QString& id);
    QList<FavoriteQuery> getFavoritesByCategory(const QString& category) const;
    FavoriteQuery getFavoriteById(const QString& id) const;

    // Category management
    void loadCategories();
    void saveCategories();
    bool addCategory(const FavoriteCategory& category);
    bool updateCategory(const QString& id, const FavoriteCategory& category);
    bool removeCategory(const QString& id);

signals:
    void favoriteSelected(const FavoriteQuery& favorite);
    void favoriteExecuted(const FavoriteQuery& favorite);
    void favoritesChanged();
    void categoriesChanged();

public slots:
    void accept() override;
    void reject() override;

private slots:
    void onCategoryChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onFavoriteSelectionChanged();
    void onCategorySelectionChanged();
    void onAddFavorite();
    void onEditFavorite();
    void onDeleteFavorite();
    void onExecuteFavorite();
    void onDuplicateFavorite();
    void onSetAsDefault();
    void onAddCategory();
    void onEditCategory();
    void onDeleteCategory();
    void onImportFavorites();
    void onExportFavorites();
    void onRefreshList();
    void onFilterChanged();

private:
    void setupUI();
    void setupFavoritesTab();
    void setupCategoriesTab();
    void setupSettingsTab();
    void setupPreviewPanel();
    void populateFavoritesList();
    void populateCategoriesTree();
    void updateCategoryCombo();
    void updateFavoriteDetails(const FavoriteQuery& favorite);
    void clearFavoriteDetails();
    QString generateUniqueId() const;
    bool validateFavorite(const FavoriteQuery& favorite);
    void sortFavorites(QList<FavoriteQuery>& favorites, const QString& sortBy = "name") const;
    QList<FavoriteQuery> filterFavorites() const;
    QList<FavoriteQuery> searchFavorites(const QString& searchText) const;
    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;
    QSplitter* mainSplitter_;

    // Favorites tab
    QWidget* favoritesTab_;
    QHBoxLayout* favoritesLayout_;

    // Left panel - categories and search
    QWidget* leftPanel_;
    QVBoxLayout* leftLayout_;

    QLabel* categoryLabel_;
    QComboBox* categoryCombo_;
    QLabel* searchLabel_;
    QLineEdit* searchEdit_;
    QGroupBox* filterGroup_;
    QCheckBox* showAllCategoriesCheck_;
    QCheckBox* showRecentlyUsedCheck_;
    QComboBox* sortByCombo_;

    // Center panel - favorites list
    QListWidget* favoritesList_;

    // Right panel - preview/details
    QWidget* previewPanel_;
    QVBoxLayout* previewLayout_;
    QGroupBox* previewGroup_;
    QTextEdit* previewTextEdit_;
    QLabel* previewInfoLabel_;

    // Action buttons
    QHBoxLayout* buttonLayout_;
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;
    QPushButton* executeButton_;
    QPushButton* duplicateButton_;
    QPushButton* setDefaultButton_;

    // Categories tab
    QWidget* categoriesTab_;
    QVBoxLayout* categoriesLayout_;
    QTreeWidget* categoriesTree_;
    QHBoxLayout* categoryButtonLayout_;
    QPushButton* addCategoryButton_;
    QPushButton* editCategoryButton_;
    QPushButton* deleteCategoryButton_;

    // Settings tab
    QWidget* settingsTab_;
    QVBoxLayout* settingsLayout_;
    QCheckBox* autoSaveCheck_;
    QCheckBox* confirmDeleteCheck_;
    QCheckBox* showPreviewCheck_;
    QCheckBox* autoExecuteCheck_;
    QPushButton* importButton_;
    QPushButton* exportButton_;
    QPushButton* backupButton_;
    QPushButton* restoreButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    QMap<QString, FavoriteQuery> favorites_;
    QMap<QString, FavoriteCategory> categories_;
    QString currentCategory_;
    QString currentSearchText_;
    QString currentSortBy_;
    FavoriteQuery currentFavorite_;

    // Settings
    QSettings* settings_;
    bool showAllCategories_ = true;
    bool showRecentlyUsed_ = true;
    bool confirmDelete_ = true;
    bool showPreview_ = true;
    bool autoExecute_ = false;
};

} // namespace scratchrobin
