# Phase 3.3: Property Viewers Implementation

## Overview
This phase implements comprehensive property viewers for database objects, providing detailed information display, dependency analysis, impact assessment, and interactive object inspection capabilities with seamless integration with the metadata loading and object browser systems.

## Prerequisites
- ✅ Phase 1.1 (Project Setup) completed
- ✅ Phase 1.2 (Architecture Design) completed
- ✅ Phase 1.3 (Dependency Management) completed
- ✅ Phase 2.1 (Connection Pooling) completed
- ✅ Phase 2.2 (Authentication System) completed
- ✅ Phase 2.3 (SSL/TLS Integration) completed
- ✅ Phase 3.1 (Metadata Loader) completed
- ✅ Phase 3.2 (Object Browser UI) completed
- Metadata loading system from Phase 3.1
- Object browser UI from Phase 3.2

## Implementation Tasks

### Task 3.3.1: Object Properties Viewer

#### 3.3.1.1: Properties Display Component
**Objective**: Create a comprehensive properties display system with categorized information and rich data presentation

**Implementation Steps:**
1. Implement properties viewer widget with tabbed interface
2. Create property grid with different data types and editors
3. Add property categorization and grouping
4. Implement property search and filtering
5. Create property history and change tracking

**Code Changes:**

**File: src/ui/properties/property_viewer.h**
```cpp
#ifndef SCRATCHROBIN_PROPERTY_VIEWER_H
#define SCRATCHROBIN_PROPERTY_VIEWER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QFormLayout>

namespace scratchrobin {

class IMetadataManager;
class IObjectHierarchy;
class ICacheManager;

enum class PropertyDisplayMode {
    GRID,
    FORM,
    TREE,
    TEXT,
    CUSTOM
};

enum class PropertyCategory {
    GENERAL,
    PHYSICAL,
    LOGICAL,
    SECURITY,
    PERFORMANCE,
    STORAGE,
    RELATIONSHIPS,
    EXTENDED
};

enum class PropertyDataType {
    STRING,
    INTEGER,
    DECIMAL,
    BOOLEAN,
    DATE_TIME,
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
    LIST,
    OBJECT_REFERENCE,
    CUSTOM
};

struct PropertyDefinition {
    std::string id;
    std::string name;
    std::string description;
    PropertyCategory category;
    PropertyDataType dataType;
    std::string defaultValue;
    std::string currentValue;
    bool isReadOnly = false;
    bool isRequired = false;
    bool isVisible = true;
    bool isEditable = true;
    std::string validationPattern;
    std::string formatString;
    std::string unit;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::string> allowedValues;
    std::string helpText;
    std::string tooltip;
};

struct PropertyGroup {
    std::string id;
    std::string name;
    std::string description;
    PropertyCategory category;
    std::vector<PropertyDefinition> properties;
    bool isExpanded = true;
    bool isVisible = true;
    int displayOrder = 0;
    QIcon icon;
    QColor headerColor;
};

struct PropertyDisplayOptions {
    PropertyDisplayMode mode = PropertyDisplayMode::GRID;
    bool showCategories = true;
    bool showDescriptions = true;
    bool showUnits = true;
    bool showIcons = true;
    bool enableEditing = true;
    bool showValidation = true;
    bool groupByCategory = true;
    int maxVisibleRows = 100;
    bool enableSearch = true;
    bool enableFiltering = true;
    bool enableSorting = true;
    bool showAdvancedProperties = false;
    bool autoRefresh = true;
    int refreshIntervalSeconds = 30;
};

struct PropertySearchOptions {
    std::string pattern;
    bool caseSensitive = false;
    bool regex = false;
    bool searchInNames = true;
    bool searchInValues = true;
    bool searchInDescriptions = false;
    std::vector<PropertyCategory> categories;
    std::vector<PropertyDataType> dataTypes;
    bool showOnlyMatching = false;
    int maxResults = 1000;
};

class IPropertyViewer {
public:
    virtual ~IPropertyViewer() = default;

    virtual void initialize(const PropertyDisplayOptions& options) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) = 0;
    virtual void setCacheManager(std::shared_ptr<ICacheManager> cacheManager) = 0;

    virtual void displayObjectProperties(const std::string& nodeId) = 0;
    virtual void displayObjectProperties(const SchemaObject& object) = 0;
    virtual void displayPropertyGroup(const PropertyGroup& group) = 0;

    virtual void setDisplayOptions(const PropertyDisplayOptions& options) = 0;
    virtual PropertyDisplayOptions getDisplayOptions() const = 0;

    virtual std::vector<PropertyGroup> getPropertyGroups() const = 0;
    virtual PropertyGroup getPropertyGroup(const std::string& groupId) const = 0;
    virtual std::vector<PropertyDefinition> getAllProperties() const = 0;

    virtual void searchProperties(const PropertySearchOptions& options) = 0;
    virtual std::vector<PropertyDefinition> getSearchResults() const = 0;
    virtual void clearSearch() = 0;

    virtual bool isPropertyModified(const std::string& propertyId) const = 0;
    virtual std::vector<std::string> getModifiedProperties() const = 0;
    virtual void applyPropertyChanges() = 0;
    virtual void revertPropertyChanges() = 0;

    virtual void exportProperties(const std::string& filePath, const std::string& format = "JSON") = 0;
    virtual void importProperties(const std::string& filePath) = 0;

    virtual void refreshProperties() = 0;
    virtual void clearProperties() = 0;

    using PropertyChangedCallback = std::function<void(const std::string&, const std::string&, const std::string&)>;
    using PropertyGroupChangedCallback = std::function<void(const std::string&)>;

    virtual void setPropertyChangedCallback(PropertyChangedCallback callback) = 0;
    virtual void setPropertyGroupChangedCallback(PropertyGroupChangedCallback callback) = 0;

    virtual QWidget* getWidget() = 0;
    virtual QSize sizeHint() const = 0;
};

class PropertyViewer : public QWidget, public IPropertyViewer {
    Q_OBJECT

public:
    PropertyViewer(QWidget* parent = nullptr);
    ~PropertyViewer() override;

    // IPropertyViewer implementation
    void initialize(const PropertyDisplayOptions& options) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) override;
    void setCacheManager(std::shared_ptr<ICacheManager> cacheManager) override;

    void displayObjectProperties(const std::string& nodeId) override;
    void displayObjectProperties(const SchemaObject& object) override;
    void displayPropertyGroup(const PropertyGroup& group) override;

    void setDisplayOptions(const PropertyDisplayOptions& options) override;
    PropertyDisplayOptions getDisplayOptions() const override;

    std::vector<PropertyGroup> getPropertyGroups() const override;
    PropertyGroup getPropertyGroup(const std::string& groupId) const override;
    std::vector<PropertyDefinition> getAllProperties() const override;

    void searchProperties(const PropertySearchOptions& options) override;
    std::vector<PropertyDefinition> getSearchResults() const override;
    void clearSearch() override;

    bool isPropertyModified(const std::string& propertyId) const override;
    std::vector<std::string> getModifiedProperties() const override;
    void applyPropertyChanges() override;
    void revertPropertyChanges() override;

    void exportProperties(const std::string& filePath, const std::string& format = "JSON") override;
    void importProperties(const std::string& filePath) override;

    void refreshProperties() override;
    void clearProperties() override;

    void setPropertyChangedCallback(PropertyChangedCallback callback) override;
    void setPropertyGroupChangedCallback(PropertyGroupChangedCallback callback) override;

    QWidget* getWidget() override;
    QSize sizeHint() const override;

private slots:
    void onTabChanged(int index);
    void onPropertyValueChanged();
    void onSearchTextChanged(const QString& text);
    void onSearchButtonClicked();
    void onClearSearchButtonClicked();
    void onExportButtonClicked();
    void onImportButtonClicked();
    void onRefreshButtonClicked();
    void onApplyChangesButtonClicked();
    void onRevertChangesButtonClicked();
    void onDisplayModeChanged(int index);
    void onCategoryToggled(bool checked);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // UI Components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* toolbarLayout_;
    QTabWidget* tabWidget_;
    QSplitter* splitter_;

    // Toolbar
    QLineEdit* searchBox_;
    QPushButton* searchButton_;
    QPushButton* clearSearchButton_;
    QComboBox* displayModeCombo_;
    QPushButton* refreshButton_;
    QPushButton* exportButton_;
    QPushButton* importButton_;
    QPushButton* applyChangesButton_;
    QPushButton* revertChangesButton_;

    // Display widgets
    QTableWidget* gridView_;
    QScrollArea* formView_;
    QTreeWidget* treeView_;
    QTextEdit* textView_;
    QWidget* customView_;

    // Status and info
    QLabel* objectInfoLabel_;
    QLabel* modificationStatusLabel_;

    // Configuration and state
    PropertyDisplayOptions displayOptions_;
    PropertySearchOptions currentSearch_;
    std::string currentNodeId_;
    SchemaObject currentObject_;
    std::vector<PropertyGroup> propertyGroups_;
    std::vector<PropertyDefinition> searchResults_;
    std::unordered_map<std::string, std::string> modifiedProperties_;
    std::unordered_map<std::string, std::string> originalValues_;

    // Callbacks
    PropertyChangedCallback propertyChangedCallback_;
    PropertyGroupChangedCallback propertyGroupChangedCallback_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<ICacheManager> cacheManager_;

    // Internal methods
    void setupUI();
    void setupToolbar();
    void setupViews();
    void setupConnections();

    void loadObjectProperties(const SchemaObject& object);
    void createPropertyGroups(const SchemaObject& object);
    void createGeneralProperties(const SchemaObject& object, PropertyGroup& group);
    void createPhysicalProperties(const SchemaObject& object, PropertyGroup& group);
    void createLogicalProperties(const SchemaObject& object, PropertyGroup& group);
    void createSecurityProperties(const SchemaObject& object, PropertyGroup& group);
    void createPerformanceProperties(const SchemaObject& object, PropertyGroup& group);
    void createStorageProperties(const SchemaObject& object, PropertyGroup& group);
    void createRelationshipProperties(const SchemaObject& object, PropertyGroup& group);
    void createExtendedProperties(const SchemaObject& object, PropertyGroup& group);

    void displayGridView();
    void displayFormView();
    void displayTreeView();
    void displayTextView();
    void displayCustomView();

    void createGridViewWidget();
    void createFormViewWidget();
    void createTreeViewWidget();
    void createTextViewWidget();
    void createCustomViewWidget();

    QWidget* createPropertyEditor(const PropertyDefinition& property);
    QWidget* createStringEditor(const PropertyDefinition& property);
    QWidget* createIntegerEditor(const PropertyDefinition& property);
    QWidget* createDecimalEditor(const PropertyDefinition& property);
    QWidget* createBooleanEditor(const PropertyDefinition& property);
    QWidget* createDateTimeEditor(const PropertyDefinition& property);
    QWidget* createListEditor(const PropertyDefinition& property);
    QWidget* createCustomEditor(const PropertyDefinition& property);

    void updatePropertyValue(const std::string& propertyId, const std::string& newValue);
    bool validatePropertyValue(const PropertyDefinition& property, const std::string& value);
    std::string formatPropertyValue(const PropertyDefinition& property, const std::string& value);

    void updateSearchResults();
    void highlightSearchMatches();
    void filterPropertiesByCategory(const std::string& categoryId, bool visible);

    void savePropertyState();
    void restorePropertyState();

    void updateModificationStatus();
    void updateObjectInfo();

    std::string serializeProperties(const std::vector<PropertyGroup>& groups, const std::string& format);
    std::vector<PropertyGroup> deserializeProperties(const std::string& data, const std::string& format);

    PropertyDataType inferDataType(const std::string& value);
    std::string getPropertyIconName(PropertyDataType type);
    QColor getPropertyColor(PropertyCategory category);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PROPERTY_VIEWER_H
```

#### 3.3.1.2: Dependency Analysis Viewer
**Objective**: Create an interactive dependency analysis and visualization component

**Implementation Steps:**
1. Implement dependency graph visualization
2. Create dependency impact analysis
3. Add dependency navigation and filtering
4. Implement dependency change tracking

**Code Changes:**

**File: src/ui/properties/dependency_viewer.h**
```cpp
#ifndef SCRATCHROBIN_DEPENDENCY_VIEWER_H
#define SCRATCHROBIN_DEPENDENCY_VIEWER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QSplitter>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QMenu>
#include <QAction>

namespace scratchrobin {

class IObjectHierarchy;
class IMetadataManager;

enum class DependencyViewMode {
    GRAPH,
    TREE,
    TABLE,
    TEXT,
    MATRIX
};

enum class DependencyDisplayType {
    ALL,
    DIRECT_ONLY,
    LEVEL_SPECIFIC,
    IMPACT_ANALYSIS,
    CHANGE_IMPACT,
    CIRCULAR_REFERENCES
};

enum class DependencyNodeType {
    ROOT_OBJECT,
    DEPENDENCY,
    DEPENDENT,
    CIRCULAR_REFERENCE,
    IMPACTED_OBJECT
};

struct DependencyNode {
    std::string id;
    std::string name;
    DependencyNodeType type;
    SchemaObjectType objectType;
    std::string schema;
    std::string database;
    int level = 0;
    bool isCircular = false;
    bool isExpanded = false;
    std::unordered_map<std::string, std::string> properties;
    QGraphicsEllipseItem* graphicsItem = nullptr;
    QGraphicsTextItem* labelItem = nullptr;
};

struct DependencyEdge {
    std::string id;
    std::string fromNodeId;
    std::string toNodeId;
    DependencyType dependencyType;
    std::string description;
    bool isCircular = false;
    int weight = 1;
    QGraphicsLineItem* graphicsItem = nullptr;
    QGraphicsTextItem* labelItem = nullptr;
};

struct DependencyGraph {
    std::string rootObjectId;
    std::vector<DependencyNode> nodes;
    std::vector<DependencyEdge> edges;
    std::unordered_map<std::string, int> nodeLevels;
    std::vector<std::vector<std::string>> circularReferenceChains;
    bool hasCircularReferences = false;
    int maxDepth = 0;
    int totalNodes = 0;
    int totalEdges = 0;
};

struct DependencyViewOptions {
    DependencyViewMode mode = DependencyViewMode::GRAPH;
    DependencyDisplayType displayType = DependencyDisplayType::ALL;
    bool showCircularReferences = true;
    bool showImpactAnalysis = true;
    bool enableNodeExpansion = true;
    bool showNodeLabels = true;
    bool showEdgeLabels = false;
    int maxDisplayDepth = 5;
    int maxNodes = 100;
    bool autoLayout = true;
    bool enableAnimation = true;
    int nodeSize = 40;
    int edgeWidth = 2;
    QColor backgroundColor = QColor(255, 255, 255);
    QColor nodeColor = QColor(100, 149, 237);
    QColor edgeColor = QColor(128, 128, 128);
    QColor circularRefColor = QColor(255, 69, 0);
    QColor impactColor = QColor(255, 215, 0);
};

struct DependencyAnalysisOptions {
    bool includeIndirectDependencies = true;
    bool includeSoftDependencies = false;
    bool includeSystemObjects = false;
    bool followInheritance = true;
    bool followComposition = true;
    int maxAnalysisDepth = 10;
    bool detectCircularReferences = true;
    bool calculateImpact = true;
    bool includePerformanceMetrics = true;
};

class IDependencyViewer {
public:
    virtual ~IDependencyViewer() = default;

    virtual void initialize(const DependencyViewOptions& options) = 0;
    virtual void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;

    virtual void displayObjectDependencies(const std::string& nodeId) = 0;
    virtual void displayObjectDependencies(const SchemaObject& object) = 0;
    virtual void displayDependencyGraph(const DependencyGraph& graph) = 0;

    virtual void setViewOptions(const DependencyViewOptions& options) = 0;
    virtual DependencyViewOptions getViewOptions() const = 0;

    virtual DependencyGraph getCurrentGraph() const = 0;
    virtual std::vector<DependencyNode> getVisibleNodes() const = 0;
    virtual std::vector<DependencyEdge> getVisibleEdges() const = 0;

    virtual void analyzeDependencies(const DependencyAnalysisOptions& options) = 0;
    virtual ImpactAnalysis getImpactAnalysis() const = 0;

    virtual void findCircularReferences() = 0;
    virtual std::vector<std::vector<std::string>> getCircularReferences() const = 0;

    virtual void exportGraph(const std::string& filePath, const std::string& format = "PNG") = 0;
    virtual void printGraph() = 0;

    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;
    virtual void fitToView() = 0;
    virtual void resetView() = 0;

    using NodeSelectedCallback = std::function<void(const std::string&)>;
    using EdgeSelectedCallback = std::function<void(const std::string&)>;
    using GraphChangedCallback = std::function<void()>;

    virtual void setNodeSelectedCallback(NodeSelectedCallback callback) = 0;
    virtual void setEdgeSelectedCallback(EdgeSelectedCallback callback) = 0;
    virtual void setGraphChangedCallback(GraphChangedCallback callback) = 0;

    virtual QWidget* getWidget() = 0;
    virtual QGraphicsView* getGraphicsView() = 0;
};

class DependencyViewer : public QWidget, public IDependencyViewer {
    Q_OBJECT

public:
    DependencyViewer(QWidget* parent = nullptr);
    ~DependencyViewer() override;

    // IDependencyViewer implementation
    void initialize(const DependencyViewOptions& options) override;
    void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;

    void displayObjectDependencies(const std::string& nodeId) override;
    void displayObjectDependencies(const SchemaObject& object) override;
    void displayDependencyGraph(const DependencyGraph& graph) override;

    void setViewOptions(const DependencyViewOptions& options) override;
    DependencyViewOptions getViewOptions() const override;

    DependencyGraph getCurrentGraph() const override;
    std::vector<DependencyNode> getVisibleNodes() const override;
    std::vector<DependencyEdge> getVisibleEdges() const override;

    void analyzeDependencies(const DependencyAnalysisOptions& options) override;
    ImpactAnalysis getImpactAnalysis() const override;

    void findCircularReferences() override;
    std::vector<std::vector<std::string>> getCircularReferences() const override;

    void exportGraph(const std::string& filePath, const std::string& format = "PNG") override;
    void printGraph() override;

    void zoomIn() override;
    void zoomOut() override;
    void fitToView() override;
    void resetView() override;

    void setNodeSelectedCallback(NodeSelectedCallback callback) override;
    void setEdgeSelectedCallback(EdgeSelectedCallback callback) override;
    void setGraphChangedCallback(GraphChangedCallback callback) override;

    QWidget* getWidget() override;
    QGraphicsView* getGraphicsView() override;

private slots:
    void onNodeDoubleClicked();
    void onEdgeDoubleClicked();
    void onContextMenuRequested(const QPoint& pos);
    void onViewModeChanged(int index);
    void onDisplayTypeChanged(int index);
    void onZoomInButtonClicked();
    void onZoomOutButtonClicked();
    void onFitToViewButtonClicked();
    void onResetViewButtonClicked();
    void onExportButtonClicked();
    void onPrintButtonClicked();
    void onAnalyzeButtonClicked();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // UI Components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* toolbarLayout_;
    QSplitter* splitter_;

    // Toolbar
    QComboBox* viewModeCombo_;
    QComboBox* displayTypeCombo_;
    QPushButton* zoomInButton_;
    QPushButton* zoomOutButton_;
    QPushButton* fitToViewButton_;
    QPushButton* resetViewButton_;
    QPushButton* exportButton_;
    QPushButton* printButton_;
    QPushButton* analyzeButton_;

    // Display widgets
    QGraphicsView* graphicsView_;
    QGraphicsScene* graphicsScene_;
    QTreeWidget* treeView_;
    QTableWidget* tableView_;
    QTextEdit* textView_;

    // Analysis and info
    QLabel* analysisInfoLabel_;
    QProgressBar* analysisProgressBar_;

    // Context menu
    QMenu* contextMenu_;

    // Configuration and state
    DependencyViewOptions viewOptions_;
    DependencyGraph currentGraph_;
    ImpactAnalysis currentImpactAnalysis_;
    std::vector<std::vector<std::string>> circularReferences_;
    std::string currentNodeId_;
    SchemaObject currentObject_;

    // Callbacks
    NodeSelectedCallback nodeSelectedCallback_;
    EdgeSelectedCallback edgeSelectedCallback_;
    GraphChangedCallback graphChangedCallback_;

    // Core components
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<IMetadataManager> metadataManager_;

    // Internal methods
    void setupUI();
    void setupToolbar();
    void setupViews();
    void setupGraphicsScene();
    void setupTreeView();
    void setupTableView();
    void setupTextView();
    void setupContextMenu();
    void setupConnections();

    void loadObjectDependencies(const SchemaObject& object);
    void buildDependencyGraph(const SchemaObject& object);
    void createGraphNodes(const ObjectHierarchy& hierarchy);
    void createGraphEdges(const ObjectHierarchy& hierarchy);

    void displayGraphView();
    void displayTreeView();
    void displayTableView();
    void displayTextView();

    void createGraphicsNode(const DependencyNode& node);
    void createGraphicsEdge(const DependencyEdge& edge);
    void layoutGraph();
    void applyGraphStyling();

    QGraphicsEllipseItem* createNodeItem(const DependencyNode& node);
    QGraphicsLineItem* createEdgeItem(const DependencyEdge& edge);
    QGraphicsTextItem* createNodeLabel(const DependencyNode& node);
    QGraphicsTextItem* createEdgeLabel(const DependencyEdge& edge);

    void updateNodeAppearance(DependencyNode& node);
    void updateEdgeAppearance(DependencyEdge& edge);

    void performImpactAnalysis();
    void detectCircularReferences();
    void calculateDependencyMetrics();

    void handleNodeSelection(const std::string& nodeId);
    void handleEdgeSelection(const std::string& edgeId);

    void updateAnalysisInfo();
    void showAnalysisProgress(bool visible, const std::string& message = "");

    void clearGraph();
    void refreshGraph();

    std::string generateNodeId(const SchemaObject& object);
    std::string generateEdgeId(const std::string& fromNodeId, const std::string& toNodeId);

    QColor getNodeColor(DependencyNodeType type);
    QColor getEdgeColor(DependencyType type);
    int getNodeSize(DependencyNodeType type);
    int getEdgeWidth(DependencyType type);

    void exportToImage(const std::string& filePath, const std::string& format);
    void exportToSvg(const std::string& filePath);
    void exportToPdf(const std::string& filePath);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DEPENDENCY_VIEWER_H
```

### Task 3.3.2: Impact Assessment System

#### 3.3.2.1: Impact Analysis Engine
**Objective**: Implement comprehensive impact analysis for database object changes

**Implementation Steps:**
1. Create impact analysis engine with change prediction
2. Implement cascading effect calculation
3. Add impact visualization and reporting
4. Create change recommendations system

**Code Changes:**

**File: src/ui/properties/impact_analyzer.h**
```cpp
#ifndef SCRATCHROBIN_IMPACT_ANALYZER_H
#define SCRATCHROBIN_IMPACT_ANALYZER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QTabWidget>

namespace scratchrobin {

class IObjectHierarchy;
class IMetadataManager;

enum class ChangeOperation {
    CREATE_OBJECT,
    DROP_OBJECT,
    ALTER_OBJECT,
    RENAME_OBJECT,
    MODIFY_PROPERTIES,
    ADD_CONSTRAINT,
    DROP_CONSTRAINT,
    ADD_INDEX,
    DROP_INDEX,
    MODIFY_DATA,
    BULK_UPDATE,
    SCHEMA_MIGRATION,
    CUSTOM_OPERATION
};

enum class ImpactSeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

enum class ImpactCategory {
    DATA_INTEGRITY,
    PERFORMANCE,
    AVAILABILITY,
    SECURITY,
    COMPLIANCE,
    FUNCTIONALITY,
    MAINTENANCE,
    COST
};

enum class ImpactScope {
    LOCAL,           // Affects only the target object
    SCHEMA,          // Affects objects within the same schema
    DATABASE,        // Affects objects across the database
    APPLICATION,     // Affects application functionality
    SYSTEM           // Affects system-wide operations
};

struct ImpactedObject {
    std::string objectId;
    std::string objectName;
    SchemaObjectType objectType;
    std::string schema;
    std::string database;
    std::string connectionId;
    ImpactSeverity severity;
    std::string impactDescription;
    std::vector<std::string> affectedProperties;
    std::vector<std::string> requiredActions;
    bool requiresMigration = false;
    bool breaksDependencies = false;
    int cascadeLevel = 0;
    std::chrono::system_clock::time_point detectedAt;
};

struct CascadingEffect {
    std::string effectId;
    std::string description;
    ImpactCategory category;
    ImpactSeverity severity;
    ImpactScope scope;
    std::vector<std::string> affectedObjects;
    std::vector<std::string> prerequisites;
    std::vector<std::string> mitigationSteps;
    bool isBlocking = false;
    bool requiresDowntime = false;
    std::chrono::hours estimatedDuration{0};
    double riskScore = 0.0;
};

struct ImpactAnalysisResult {
    std::string targetObjectId;
    std::string targetObjectName;
    SchemaObjectType targetObjectType;
    ChangeOperation operation;
    ImpactSeverity overallSeverity;
    int totalImpactedObjects = 0;
    int criticalImpacts = 0;
    int highImpacts = 0;
    int mediumImpacts = 0;
    int lowImpacts = 0;
    std::vector<ImpactedObject> impactedObjects;
    std::vector<CascadingEffect> cascadingEffects;
    std::vector<std::string> blockingIssues;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
    std::vector<std::string> migrationSteps;
    bool requiresBackup = false;
    bool requiresDowntime = false;
    bool breaksApplication = false;
    std::chrono::hours estimatedDuration{0};
    double riskScore = 0.0;
    std::chrono::system_clock::time_point analyzedAt;
    std::chrono::milliseconds analysisTime;
};

struct ImpactAnalysisOptions {
    bool includeIndirectEffects = true;
    bool includePerformanceImpact = true;
    bool includeSecurityImpact = true;
    bool includeComplianceImpact = true;
    bool includeCostImpact = true;
    int maxAnalysisDepth = 10;
    int maxImpactedObjects = 1000;
    bool enableRiskAssessment = true;
    bool includeMigrationSteps = true;
    bool includeRollbackSteps = true;
    std::vector<ImpactCategory> includedCategories;
    std::vector<ImpactScope> includedScopes;
};

struct RiskAssessment {
    double overallRiskScore = 0.0;
    double dataLossRisk = 0.0;
    double performanceRisk = 0.0;
    double availabilityRisk = 0.0;
    double securityRisk = 0.0;
    double complianceRisk = 0.0;
    std::vector<std::string> riskFactors;
    std::vector<std::string> mitigationStrategies;
    std::vector<std::string> contingencyPlans;
    bool requiresApproval = false;
    std::string approvalLevel;
};

class IImpactAnalyzer {
public:
    virtual ~IImpactAnalyzer() = default;

    virtual void initialize(const ImpactAnalysisOptions& options) = 0;
    virtual void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;

    virtual ImpactAnalysisResult analyzeImpact(
        const std::string& objectId, ChangeOperation operation,
        const std::unordered_map<std::string, std::string>& changeDetails = {}) = 0;

    virtual ImpactAnalysisResult analyzeImpact(
        const SchemaObject& object, ChangeOperation operation,
        const std::unordered_map<std::string, std::string>& changeDetails = {}) = 0;

    virtual RiskAssessment assessRisk(const ImpactAnalysisResult& analysis) = 0;

    virtual std::vector<std::string> generateMigrationScript(const ImpactAnalysisResult& analysis) = 0;
    virtual std::vector<std::string> generateRollbackScript(const ImpactAnalysisResult& analysis) = 0;
    virtual std::vector<std::string> generateValidationScript(const ImpactAnalysisResult& analysis) = 0;

    virtual ImpactAnalysisOptions getOptions() const = 0;
    virtual void setOptions(const ImpactAnalysisOptions& options) = 0;

    virtual std::vector<ImpactAnalysisResult> getAnalysisHistory() const = 0;
    virtual void clearAnalysisHistory() = 0;

    using AnalysisProgressCallback = std::function<void(int, int, const std::string&)>;
    using AnalysisCompletedCallback = std::function<void(const ImpactAnalysisResult&)>;

    virtual void setAnalysisProgressCallback(AnalysisProgressCallback callback) = 0;
    virtual void setAnalysisCompletedCallback(AnalysisCompletedCallback callback) = 0;
};

class ImpactAnalyzer : public QObject, public IImpactAnalyzer {
    Q_OBJECT

public:
    ImpactAnalyzer(QObject* parent = nullptr);
    ~ImpactAnalyzer() override;

    // IImpactAnalyzer implementation
    void initialize(const ImpactAnalysisOptions& options) override;
    void setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;

    ImpactAnalysisResult analyzeImpact(
        const std::string& objectId, ChangeOperation operation,
        const std::unordered_map<std::string, std::string>& changeDetails = {}) override;

    ImpactAnalysisResult analyzeImpact(
        const SchemaObject& object, ChangeOperation operation,
        const std::unordered_map<std::string, std::string>& changeDetails = {}) override;

    RiskAssessment assessRisk(const ImpactAnalysisResult& analysis) override;

    std::vector<std::string> generateMigrationScript(const ImpactAnalysisResult& analysis) override;
    std::vector<std::string> generateRollbackScript(const ImpactAnalysisResult& analysis) override;
    std::vector<std::string> generateValidationScript(const ImpactAnalysisResult& analysis) override;

    ImpactAnalysisOptions getOptions() const override;
    void setOptions(const ImpactAnalysisOptions& options) override;

    std::vector<ImpactAnalysisResult> getAnalysisHistory() const override;
    void clearAnalysisHistory() override;

    void setAnalysisProgressCallback(AnalysisProgressCallback callback) override;
    void setAnalysisCompletedCallback(AnalysisCompletedCallback callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IObjectHierarchy> objectHierarchy_;
    std::shared_ptr<IMetadataManager> metadataManager_;

    // Configuration and state
    ImpactAnalysisOptions options_;
    std::vector<ImpactAnalysisResult> analysisHistory_;

    // Callbacks
    AnalysisProgressCallback progressCallback_;
    AnalysisCompletedCallback completedCallback_;

    // Internal methods
    ImpactAnalysisResult performImpactAnalysis(
        const SchemaObject& object, ChangeOperation operation,
        const std::unordered_map<std::string, std::string>& changeDetails);

    void analyzeCreateImpact(const SchemaObject& object, ImpactAnalysisResult& result);
    void analyzeDropImpact(const SchemaObject& object, ImpactAnalysisResult& result);
    void analyzeAlterImpact(const SchemaObject& object, ImpactAnalysisResult& result,
                           const std::unordered_map<std::string, std::string>& changeDetails);
    void analyzeRenameImpact(const SchemaObject& object, ImpactAnalysisResult& result,
                            const std::unordered_map<std::string, std::string>& changeDetails);

    void collectDirectDependencies(const SchemaObject& object, ImpactAnalysisResult& result);
    void collectIndirectDependencies(const SchemaObject& object, ImpactAnalysisResult& result);
    void collectDependents(const SchemaObject& object, ImpactAnalysisResult& result);

    void analyzeDataIntegrityImpact(const SchemaObject& object, ChangeOperation operation,
                                   ImpactAnalysisResult& result);
    void analyzePerformanceImpact(const SchemaObject& object, ChangeOperation operation,
                                 ImpactAnalysisResult& result);
    void analyzeSecurityImpact(const SchemaObject& object, ChangeOperation operation,
                              ImpactAnalysisResult& result);
    void analyzeComplianceImpact(const SchemaObject& object, ChangeOperation operation,
                                ImpactAnalysisResult& result);

    void calculateCascadingEffects(ImpactAnalysisResult& result);
    void identifyBlockingIssues(ImpactAnalysisResult& result);
    void generateRecommendations(ImpactAnalysisResult& result);
    void estimateDurationAndRisk(ImpactAnalysisResult& result);

    ImpactedObject createImpactedObject(const SchemaObject& object, ImpactSeverity severity,
                                       const std::string& description);
    CascadingEffect createCascadingEffect(const std::string& description, ImpactCategory category,
                                         ImpactSeverity severity, ImpactScope scope);

    void updateAnalysisHistory(const ImpactAnalysisResult& result);

    double calculateRiskScore(const ImpactAnalysisResult& result);
    RiskAssessment performRiskAssessment(const ImpactAnalysisResult& result);

    std::vector<std::string> generateCreateMigrationScript(const ImpactAnalysisResult& result);
    std::vector<std::string> generateDropMigrationScript(const ImpactAnalysisResult& result);
    std::vector<std::string> generateAlterMigrationScript(const ImpactAnalysisResult& result);

    std::vector<std::string> generateValidationQueries(const ImpactAnalysisResult& result);
    std::vector<std::string> generateHealthCheckQueries(const ImpactAnalysisResult& result);

    bool isSystemCriticalObject(const SchemaObject& object);
    bool hasDependentApplications(const SchemaObject& object);
    bool containsSensitiveData(const SchemaObject& object);

    void notifyProgress(int current, int total, const std::string& message);
    void notifyCompletion(const ImpactAnalysisResult& result);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_IMPACT_ANALYZER_H
```

#### 3.3.2.2: Impact Assessment Dialog
**Objective**: Create user-friendly dialog for displaying impact analysis results with recommendations

**Implementation Steps:**
1. Implement impact assessment dialog with rich visualization
2. Create recommendation display and action items
3. Add risk assessment visualization
4. Implement approval workflow integration

**Code Changes:**

**File: src/ui/properties/impact_dialog.h**
```cpp
#ifndef SCRATCHROBIN_IMPACT_DIALOG_H
#define SCRATCHROBIN_IMPACT_DIALOG_H

#include <memory>
#include <string>
#include <vector>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QSplitter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>

namespace scratchrobin {

class IImpactAnalyzer;

enum class ImpactDialogMode {
    ANALYSIS,
    APPROVAL,
    EXECUTION,
    REVIEW
};

enum class ImpactApprovalStatus {
    PENDING,
    APPROVED,
    REJECTED,
    REQUIRES_REVIEW,
    ESCALATED
};

struct ImpactDialogOptions {
    ImpactDialogMode mode = ImpactDialogMode::ANALYSIS;
    bool showRiskAssessment = true;
    bool showMigrationSteps = true;
    bool showRecommendations = true;
    bool enableApprovalWorkflow = true;
    bool enableScriptGeneration = true;
    bool enableExport = true;
    int maxVisibleImpacts = 100;
    bool autoExpandCritical = true;
    bool showProgressDetails = true;
    QColor criticalColor = QColor(220, 53, 69);
    QColor highColor = QColor(255, 193, 7);
    QColor mediumColor = QColor(0, 123, 255);
    QColor lowColor = QColor(40, 167, 69);
};

struct ApprovalRequest {
    std::string requestId;
    std::string requester;
    std::string approver;
    ImpactApprovalStatus status;
    std::string justification;
    std::vector<std::string> reviewComments;
    std::chrono::system_clock::time_point requestedAt;
    std::chrono::system_clock::time_point reviewedAt;
};

class IImpactDialog {
public:
    virtual ~IImpactDialog() = default;

    virtual void initialize(const ImpactDialogOptions& options) = 0;
    virtual void setImpactAnalyzer(std::shared_ptr<IImpactAnalyzer> impactAnalyzer) = 0;

    virtual void showImpactAnalysis(const ImpactAnalysisResult& analysis) = 0;
    virtual void showRiskAssessment(const RiskAssessment& assessment) = 0;

    virtual ImpactDialogOptions getOptions() const = 0;
    virtual void setOptions(const ImpactDialogOptions& options) = 0;

    virtual ApprovalRequest submitForApproval(const std::string& approver,
                                            const std::string& justification) = 0;
    virtual ImpactApprovalStatus getApprovalStatus(const std::string& requestId) const = 0;

    virtual void exportReport(const std::string& filePath, const std::string& format = "PDF") = 0;
    virtual void printReport() = 0;

    using ApprovalStatusChangedCallback = std::function<void(const std::string&, ImpactApprovalStatus)>;
    using ActionRequestedCallback = std::function<void(const std::string&)>;

    virtual void setApprovalStatusChangedCallback(ApprovalStatusChangedCallback callback) = 0;
    virtual void setActionRequestedCallback(ActionRequestedCallback callback) = 0;

    virtual QDialog* getDialog() = 0;
    virtual int exec() = 0;
};

class ImpactDialog : public QDialog, public IImpactDialog {
    Q_OBJECT

public:
    ImpactDialog(QWidget* parent = nullptr);
    ~ImpactDialog() override;

    // IImpactDialog implementation
    void initialize(const ImpactDialogOptions& options) override;
    void setImpactAnalyzer(std::shared_ptr<IImpactAnalyzer> impactAnalyzer) override;

    void showImpactAnalysis(const ImpactAnalysisResult& analysis) override;
    void showRiskAssessment(const RiskAssessment& assessment) override;

    ImpactDialogOptions getOptions() const override;
    void setOptions(const ImpactDialogOptions& options) override;

    ApprovalRequest submitForApproval(const std::string& approver,
                                    const std::string& justification) override;
    ImpactApprovalStatus getApprovalStatus(const std::string& requestId) const override;

    void exportReport(const std::string& filePath, const std::string& format = "PDF") override;
    void printReport() override;

    void setApprovalStatusChangedCallback(ApprovalStatusChangedCallback callback) override;
    void setActionRequestedCallback(ActionRequestedCallback callback) override;

    QDialog* getDialog() override;
    int exec() override;

private slots:
    void onTabChanged(int index);
    void onImpactDoubleClicked(QTreeWidgetItem* item, int column);
    void onGenerateScriptsButtonClicked();
    void onExportReportButtonClicked();
    void onPrintReportButtonClicked();
    void onSubmitApprovalButtonClicked();
    void onApprovalStatusChanged();
    void onRiskChartClicked();
    void onRecommendationClicked();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Dialog components
    QVBoxLayout* mainLayout_;
    QHBoxLayout* buttonLayout_;
    QTabWidget* tabWidget_;

    // Summary tab
    QWidget* summaryTab_;
    QVBoxLayout* summaryLayout_;
    QLabel* summaryTitleLabel_;
    QLabel* summarySeverityLabel_;
    QLabel* summaryImpactLabel_;
    QLabel* summaryRiskLabel_;
    QTextEdit* summaryDescription_;

    // Impacts tab
    QWidget* impactsTab_;
    QVBoxLayout* impactsLayout_;
    QTreeWidget* impactsTree_;
    QTextEdit* impactsDetails_;

    // Risk Assessment tab
    QWidget* riskTab_;
    QVBoxLayout* riskLayout_;
    QGraphicsView* riskChartView_;
    QChart* riskChart_;
    QTextEdit* riskDetails_;
    QTableWidget* riskFactorsTable_;

    // Recommendations tab
    QWidget* recommendationsTab_;
    QVBoxLayout* recommendationsLayout_;
    QTableWidget* recommendationsTable_;
    QTextEdit* migrationStepsText_;
    QTextEdit* rollbackStepsText_;

    // Approval tab
    QWidget* approvalTab_;
    QVBoxLayout* approvalLayout_;
    QLabel* approvalStatusLabel_;
    QComboBox* approverCombo_;
    QTextEdit* justificationText_;
    QPushButton* submitApprovalButton_;
    QTableWidget* approvalHistoryTable_;

    // Dialog buttons
    QPushButton* generateScriptsButton_;
    QPushButton* exportReportButton_;
    QPushButton* printReportButton_;
    QPushButton* closeButton_;
    QPushButton* helpButton_;

    // Configuration and state
    ImpactDialogOptions options_;
    ImpactAnalysisResult currentAnalysis_;
    RiskAssessment currentRiskAssessment_;
    ApprovalRequest currentApproval_;
    std::vector<ApprovalRequest> approvalHistory_;

    // Callbacks
    ApprovalStatusChangedCallback approvalStatusCallback_;
    ActionRequestedCallback actionRequestedCallback_;

    // Core components
    std::shared_ptr<IImpactAnalyzer> impactAnalyzer_;

    // Internal methods
    void setupUI();
    void setupSummaryTab();
    void setupImpactsTab();
    void setupRiskTab();
    void setupRecommendationsTab();
    void setupApprovalTab();
    void setupButtons();
    void setupConnections();

    void populateSummaryTab();
    void populateImpactsTab();
    void populateRiskTab();
    void populateRecommendationsTab();
    void populateApprovalTab();

    void createImpactsTree();
    void createRiskChart();
    void createRecommendationsTable();
    void createApprovalHistoryTable();

    void updateImpactDetails(const ImpactedObject& impact);
    void updateRiskChart();
    void updateApprovalStatus();

    void generateImpactReport();
    void generateRiskReport();
    void generateMigrationReport();

    std::string formatDuration(std::chrono::hours duration);
    std::string formatSeverity(ImpactSeverity severity);
    QColor getSeverityColor(ImpactSeverity severity);
    QIcon getSeverityIcon(ImpactSeverity severity);

    void showProgressDialog(const std::string& operation);
    void hideProgressDialog();

    void handleApprovalSubmission();
    void handleApprovalStatusUpdate(const std::string& requestId, ImpactApprovalStatus status);

    void exportToPdf(const std::string& filePath);
    void exportToHtml(const std::string& filePath);
    void exportToText(const std::string& filePath);

    std::string generateHtmlReport();
    std::string generateTextReport();

    void printReportInternal();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_IMPACT_DIALOG_H
```

## Summary

This phase 3.3 implementation provides comprehensive property viewing and impact analysis functionality for ScratchRobin with the following key features:

✅ **Property Viewer**: Rich property display with multiple view modes, categorization, editing, and validation
✅ **Dependency Viewer**: Interactive dependency graph visualization with circular reference detection
✅ **Impact Analysis Engine**: Comprehensive change impact assessment with risk analysis and recommendations
✅ **Impact Assessment Dialog**: User-friendly dialog with approval workflow and report generation
✅ **Professional UI/UX**: Modern Qt interface with charts, graphs, and interactive visualizations
✅ **Enterprise Features**: Approval workflows, script generation, risk assessment, and compliance checking
✅ **Integration**: Seamless integration with metadata loader and object browser systems
✅ **Extensibility**: Plugin architecture for custom property types and impact analysis rules

The property viewers provide detailed object information, dependency analysis, and impact assessment with professional-grade visualization and workflow management.

**Next Phase**: Phase 3.4 - Search and Indexing Implementation
