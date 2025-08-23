#ifndef SCRATCHROBIN_PROPERTY_VIEWER_H
#define SCRATCHROBIN_PROPERTY_VIEWER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QScrollArea>
#include <QLineEdit>
#include <QComboBox>
#include "metadata/schema_collector.h"

namespace scratchrobin {

class IMetadataManager;

enum class PropertyDataType {
    STRING,
    INTEGER,
    DECIMAL,
    BOOLEAN,
    DATE_TIME,
    LIST,
    SIZE,
    DURATION,
    PERCENTAGE,
    IDENTIFIER,
    SQL_TYPE,
    FILE_PATH,
    URL,
    EMAIL,
    PHONE,
    CURRENCY,
    OBJECT_REFERENCE,
    CUSTOM
};

enum class PropertyCategory {
    GENERAL,
    PHYSICAL,
    PERFORMANCE,
    SECURITY,
    RELATIONSHIPS,
    DEPENDENCIES,
    STATISTICS,
    LOGICAL,
    STORAGE,
    EXTENDED,
    CUSTOM
};

struct PropertyDefinition {
    std::string id;
    std::string name;
    std::string displayName;
    std::string description;
    PropertyDataType dataType;
    PropertyCategory category;
    std::string currentValue;
    std::string defaultValue;
    std::string validationPattern;
    bool isReadOnly = false;
    bool isRequired = false;
    bool isAdvanced = false;
    std::vector<std::string> allowedValues;
    std::unordered_map<std::string, std::string> metadata;
};

struct PropertyGroup {
    std::string id;
    std::string name;
    std::string description;
    PropertyCategory category;
    std::vector<PropertyDefinition> properties;
    bool isExpanded = true;
    bool isVisible = true;
};

// Forward declarations
class IObjectHierarchy;
class ICacheManager;
class SchemaObject;

enum class PropertyDisplayMode {
    GRID,
    FORM,
    TREE,
    TEXT,
    CUSTOM
};

struct PropertyDisplayOptions {
    PropertyDisplayMode mode = PropertyDisplayMode::GRID;
    bool showAdvanced = false;
    bool showCategories = true;
    bool compactMode = false;
    std::vector<PropertyCategory> visibleCategories;
};

struct PropertySearchOptions {
    std::string searchText;
    std::string pattern;
    std::vector<PropertyCategory> categories;
    bool caseSensitive = false;
    bool regexMode = false;
    bool searchInNames = true;
    bool searchInValues = true;
};

using PropertyGroupChangedCallback = std::function<void(const std::string&, const PropertyGroup&)>;

class PropertyViewer : public QWidget {
    Q_OBJECT

public:
    explicit PropertyViewer(QWidget* parent = nullptr);
    ~PropertyViewer() override;

    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager);

    // Property management
    void setObject(const std::string& objectId, const std::string& objectType);
    void setProperties(const std::vector<PropertyGroup>& propertyGroups);
    void addPropertyGroup(const PropertyGroup& group);
    void removePropertyGroup(const std::string& groupId);
    void updatePropertyGroup(const PropertyGroup& group);

    // Property editing
    bool isModified() const;
    std::unordered_map<std::string, std::string> getModifiedProperties() const;
    void applyChanges();
    void revertChanges();
    void resetToDefaults();

    // UI customization
    void setReadOnly(bool readOnly);
    void setShowAdvanced(bool showAdvanced);
    void setShowCategories(bool showCategories);
    void setCompactMode(bool compact);

    // Search and filter
    void setFilterText(const std::string& filterText);
    void setCategoryFilter(const std::vector<PropertyCategory>& categories);

    // Export/Import
    void exportProperties(const std::string& filePath);
    void exportProperties(const std::string& filePath, const std::string& format);
    void importProperties(const std::string& filePath);

    // UI setup
    void setupUI();
    void setupConnections();

    // Initialization and configuration
    void initialize(const PropertyDisplayOptions& options);
    void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy);
    void setCacheManager(std::shared_ptr<ICacheManager> cacheManager);

    // Property display
    void displayObjectProperties(const std::string& nodeId);
    void displayObjectProperties(const SchemaObject& object);
    void displayPropertyGroup(const PropertyGroup& group);

    // Display options
    void setDisplayOptions(const PropertyDisplayOptions& options);
    PropertyDisplayOptions getDisplayOptions() const;

    // Property groups management
    std::vector<PropertyGroup> getPropertyGroups() const;
    PropertyGroup getPropertyGroup(const std::string& groupId) const;

    // Search functionality
    void searchProperties(const PropertySearchOptions& options);
    std::vector<PropertyDefinition> getSearchResults() const;
    void clearSearch();

    // Property modification tracking
    bool isPropertyModified(const std::string& propertyId) const;
    void applyPropertyChanges();
    void revertPropertyChanges();

    // Refresh and clear
    void refreshProperties();
    void clearProperties();

    // Advanced callbacks
    void setPropertyGroupChangedCallback(PropertyGroupChangedCallback callback);

    // UI widgets access
    QWidget* getWidget();
    QSize sizeHint() const;

    // Display management
    void refreshDisplay();
    void highlightSearchResults();
    void updateObjectInfo();
    void updateModificationStatus();

    // Event callbacks
    using PropertyChangedCallback = std::function<void(const std::string&, const std::string&, const std::string&)>;
    using PropertiesAppliedCallback = std::function<void(const std::unordered_map<std::string, std::string>&)>;

    void setPropertyChangedCallback(PropertyChangedCallback callback);
    void setPropertiesAppliedCallback(PropertiesAppliedCallback callback);

signals:
    void propertyChanged(const std::string& propertyId, const std::string& oldValue, const std::string& newValue);
    void propertiesApplied(const std::unordered_map<std::string, std::string>& modifiedProperties);
    void objectChanged(const std::string& objectId, const std::string& objectType);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<ICacheManager> cacheManager_;

    // Callbacks
    PropertyChangedCallback propertyChangedCallback_;
    PropertiesAppliedCallback propertiesAppliedCallback_;
    PropertyGroupChangedCallback propertyGroupChangedCallback_;

    // Data members
    std::vector<PropertyGroup> propertyGroups_;
    PropertyDisplayOptions displayOptions_;
    std::vector<PropertyDefinition> searchResults_;
    std::unordered_map<std::string, std::string> modifiedProperties_;
    std::unordered_map<std::string, std::string> originalValues_;
    std::string currentNodeId_;
    std::shared_ptr<SchemaObject> currentObject_;
    PropertySearchOptions currentSearch_;

    // UI Widgets
    QVBoxLayout* mainLayout_;
    QHBoxLayout* toolbarLayout_;
    QLineEdit* searchBox_;
    QPushButton* searchButton_;
    QPushButton* clearSearchButton_;
    QComboBox* displayModeCombo_;
    QPushButton* refreshButton_;
    QPushButton* exportButton_;
    QPushButton* importButton_;
    QPushButton* applyChangesButton_;
    QPushButton* revertChangesButton_;
    QTabWidget* tabWidget_;
    QTableWidget* gridView_;
    QTreeWidget* treeView_;
    QTextEdit* textView_;
    QScrollArea* formView_;
    QLabel* objectInfoLabel_;
    QLabel* modificationStatusLabel_;

    // Helper methods
    std::string toLower(const std::string& str);

    // Missing method declarations
    std::vector<PropertyDefinition> getAllProperties() const;
    std::string toString(SchemaObjectType type) const;

private slots:
    // Qt slots for UI events
    void onSearchTextChanged(const QString& text);
    void onSearchButtonClicked();
    void onClearSearchButtonClicked();
    void onDisplayModeChanged(int index);
    void onRefreshButtonClicked();
    void onExportButtonClicked();
    void onImportButtonClicked();
    void onApplyChangesButtonClicked();
    void onRevertChangesButtonClicked();

private:
    // Disable copy and assignment
    PropertyViewer(const PropertyViewer&) = delete;
    PropertyViewer& operator=(const PropertyViewer&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PROPERTY_VIEWER_H
