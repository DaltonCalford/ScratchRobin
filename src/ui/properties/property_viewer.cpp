#include "ui/properties/property_viewer.h"
#include "metadata/metadata_manager.h"
#include "metadata/object_hierarchy.h"
#include "metadata/cache_manager.h"
#include "metadata/schema_collector.h"
#include <QApplication>
#include <QHeaderView>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QClipboard>
#include <QRegularExpressionValidator>
#include <QDebug>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <functional>

namespace scratchrobin {

class PropertyViewer::Impl {
public:
    Impl(PropertyViewer* parent)
        : parent_(parent) {}

    // Property change callback
    std::function<void(const std::string&, const std::string&, const std::string&)> propertyChangedCallback_;

    // Property groups and display options
    std::vector<PropertyGroup> propertyGroups_;
    PropertyDisplayOptions displayOptions_;
    std::vector<PropertyDefinition> searchResults_;

    // UI Widgets
    QTableWidget* gridView_;
    QScrollArea* formView_;
    QTreeWidget* treeView_;
    QTextEdit* textView_;

    std::shared_ptr<QWidget> createPropertyEditor(const PropertyDefinition& property) {
        switch (property.dataType) {
            case PropertyDataType::STRING:
                return createStringEditor(property);
            case PropertyDataType::INTEGER:
                return createIntegerEditor(property);
            case PropertyDataType::DECIMAL:
                return createDecimalEditor(property);
            case PropertyDataType::BOOLEAN:
                return createBooleanEditor(property);
            case PropertyDataType::DATE_TIME:
                return createDateTimeEditor(property);
            case PropertyDataType::LIST:
                return createListEditor(property);
            default:
                return createCustomEditor(property);
        }
    }

    std::shared_ptr<QWidget> createStringEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QLineEdit>();
        editor->setText(QString::fromStdString(property.currentValue));
        editor->setReadOnly(property.isReadOnly);
        editor->setPlaceholderText(QString::fromStdString(property.defaultValue));

        if (!property.validationPattern.empty()) {
            QRegularExpression regex(QString::fromStdString(property.validationPattern));
            auto validator = new QRegularExpressionValidator(regex, editor.get());
            editor->setValidator(validator);
        }

        connect(editor.get(), &QLineEdit::textChanged, [this, property](const QString& text) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue, text.toStdString());
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createIntegerEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QSpinBox>();
        editor->setValue(std::stoi(property.currentValue));
        editor->setReadOnly(property.isReadOnly);
        editor->setMinimum(std::numeric_limits<int>::min());
        editor->setMaximum(std::numeric_limits<int>::max());

        connect(editor.get(), QOverload<int>::of(&QSpinBox::valueChanged),
                [this, property](int value) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue, std::to_string(value));
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createDecimalEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QDoubleSpinBox>();
        editor->setValue(std::stod(property.currentValue));
        editor->setReadOnly(property.isReadOnly);
        editor->setDecimals(6);
        editor->setMinimum(std::numeric_limits<double>::lowest());
        editor->setMaximum(std::numeric_limits<double>::max());

        connect(editor.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                [this, property](double value) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue, std::to_string(value));
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createBooleanEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QCheckBox>();
        editor->setChecked(property.currentValue == "true");
        editor->setEnabled(!property.isReadOnly);

        connect(editor.get(), &QCheckBox::toggled, [this, property](bool checked) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue, checked ? "true" : "false");
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createDateTimeEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QDateTimeEdit>();
        if (!property.currentValue.empty()) {
            editor->setDateTime(QDateTime::fromString(QString::fromStdString(property.currentValue)));
        }
        editor->setReadOnly(property.isReadOnly);
        editor->setCalendarPopup(true);

        connect(editor.get(), &QDateTimeEdit::dateTimeChanged,
                [this, property](const QDateTime& dateTime) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue,
                                       dateTime.toString().toStdString());
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createListEditor(const PropertyDefinition& property) {
        auto editor = std::make_shared<QComboBox>();
        for (const auto& value : property.allowedValues) {
            editor->addItem(QString::fromStdString(value));
        }
        editor->setCurrentText(QString::fromStdString(property.currentValue));
        editor->setEnabled(!property.isReadOnly);

        connect(editor.get(), QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, property, editor](int index) {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue,
                                       editor->currentText().toStdString());
            }
        });

        return editor;
    }

    std::shared_ptr<QWidget> createCustomEditor(const PropertyDefinition& property) {
        // Default to text editor for custom types
        auto editor = std::make_shared<QTextEdit>();
        editor->setPlainText(QString::fromStdString(property.currentValue));
        editor->setReadOnly(property.isReadOnly);
        editor->setMaximumHeight(100);

        connect(editor.get(), &QTextEdit::textChanged, [this, property, editor]() {
            if (propertyChangedCallback_) {
                propertyChangedCallback_(property.id, property.currentValue,
                                       editor->toPlainText().toStdString());
            }
        });

        return editor;
    }

    void setupGridView() {
        gridView_ = new QTableWidget();
        gridView_->setColumnCount(4);
        gridView_->setHorizontalHeaderLabels({"Property", "Value", "Type", "Category"});
        gridView_->horizontalHeader()->setStretchLastSection(true);
        gridView_->setAlternatingRowColors(true);
        gridView_->setSelectionBehavior(QAbstractItemView::SelectRows);
        gridView_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        gridView_->verticalHeader()->setVisible(false);

        // Connect to property change signals
        connect(gridView_, &QTableWidget::cellChanged, [this](int row, int column) {
            if (column == 1) { // Value column
                auto item = gridView_->item(row, 0); // Property name
                auto valueItem = gridView_->item(row, 1); // Property value
                if (item && valueItem && propertyChangedCallback_) {
                    propertyChangedCallback_(item->text().toStdString(),
                                           valueItem->text().toStdString(),
                                           valueItem->text().toStdString());
                }
            }
        });
    }

    void setupFormView() {
        formView_ = new QScrollArea();
        formView_->setWidgetResizable(true);
        formView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        formView_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    void setupTreeView() {
        treeView_ = new QTreeWidget();
        treeView_->setHeaderLabels({"Property", "Value", "Type"});
        treeView_->setAlternatingRowColors(true);
        treeView_->setRootIsDecorated(true);
        treeView_->setUniformRowHeights(false);
        treeView_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        connect(treeView_, &QTreeWidget::itemChanged, [this](QTreeWidgetItem* item, int column) {
            if (column == 1 && item->parent()) { // Value column, not a category
                if (propertyChangedCallback_) {
                    auto propertyItem = item->parent()->child(item->parent()->indexOfChild(item));
                    propertyChangedCallback_(item->text(0).toStdString(),
                                           item->text(1).toStdString(),
                                           item->text(1).toStdString());
                }
            }
        });
    }

    void setupTextView() {
        textView_ = new QTextEdit();
        textView_->setReadOnly(true);
        textView_->setFont(QFont("Courier New", 10));
    }

    void populateGridView() {
        gridView_->setRowCount(0);

        for (const auto& group : propertyGroups_) {
            for (const auto& property : group.properties) {
                int row = gridView_->rowCount();
                gridView_->insertRow(row);

                // Property name
                auto nameItem = new QTableWidgetItem(QString::fromStdString(property.name));
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                gridView_->setItem(row, 0, nameItem);

                // Property value
                auto valueItem = new QTableWidgetItem(QString::fromStdString(property.currentValue));
                if (property.isReadOnly) {
                    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
                }
                valueItem->setBackground(QBrush(getPropertyColor(property.category)));
                gridView_->setItem(row, 1, valueItem);

                // Property type
                auto typeItem = new QTableWidgetItem(QString::fromStdString(toString(property.dataType)));
                typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
                gridView_->setItem(row, 2, typeItem);

                // Property category
                auto categoryItem = new QTableWidgetItem(QString::fromStdString(toString(property.category)));
                categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
                gridView_->setItem(row, 3, categoryItem);
            }
        }

        gridView_->resizeColumnsToContents();
    }

    void populateFormView() {
        auto widget = new QWidget();
        auto layout = new QVBoxLayout(widget);

        for (const auto& group : propertyGroups_) {
            auto groupBox = new QGroupBox(QString::fromStdString(group.name));
            auto formLayout = new QFormLayout(groupBox);

            for (const auto& property : group.properties) {
                auto label = new QLabel(QString::fromStdString(property.name));
                label->setToolTip(QString::fromStdString(property.description));

                auto editor = createPropertyEditor(property);
                editor->setProperty("propertyId", QString::fromStdString(property.id));

                formLayout->addRow(label, editor.get());
                editorWidgets_[property.id] = editor;
            }

            layout->addWidget(groupBox);
        }

        layout->addStretch();
        formView_->setWidget(widget);
    }

    void populateTreeView() {
        treeView_->clear();

        for (const auto& group : propertyGroups_) {
            auto groupItem = new QTreeWidgetItem(treeView_);
            groupItem->setText(0, QString::fromStdString(group.name));
            groupItem->setText(1, "");
            groupItem->setText(2, "Category");
            groupItem->setBackground(0, QBrush(getPropertyColor(group.category)));
            groupItem->setExpanded(group.isExpanded);

            for (const auto& property : group.properties) {
                auto propertyItem = new QTreeWidgetItem(groupItem);
                propertyItem->setText(0, QString::fromStdString(property.name));
                propertyItem->setText(1, QString::fromStdString(property.currentValue));
                propertyItem->setText(2, QString::fromStdString(toString(property.dataType)));

                if (property.isReadOnly) {
                    propertyItem->setFlags(propertyItem->flags() & ~Qt::ItemIsEditable);
                }

                propertyItem->setBackground(1, QBrush(getPropertyColor(property.category)));
                propertyItem->setToolTip(0, QString::fromStdString(property.description));
            }
        }

        treeView_->resizeColumnToContents(0);
        treeView_->resizeColumnToContents(1);
        treeView_->resizeColumnToContents(2);
    }

    void populateTextView() {
        QString text;

        for (const auto& group : propertyGroups_) {
            text += QString("== %1 ==\n").arg(QString::fromStdString(group.name));

            for (const auto& property : group.properties) {
                text += QString("%1: %2 (%3)\n")
                    .arg(QString::fromStdString(property.name))
                    .arg(QString::fromStdString(property.currentValue))
                    .arg(QString::fromStdString(toString(property.dataType)));
            }

            text += "\n";
        }

        textView_->setPlainText(text);
    }

    void createPropertyGroups(const SchemaObject& object) {
        propertyGroups_.clear();

        // General Properties
        PropertyGroup generalGroup;
        generalGroup.id = "general";
        generalGroup.name = "General";
        generalGroup.category = PropertyCategory::GENERAL;
        generalGroup.isExpanded = true;
        createGeneralProperties(object, generalGroup);
        propertyGroups_.push_back(generalGroup);

        // Physical Properties
        PropertyGroup physicalGroup;
        physicalGroup.id = "physical";
        physicalGroup.name = "Physical Properties";
        physicalGroup.category = PropertyCategory::PHYSICAL;
        physicalGroup.isExpanded = false;
        createPhysicalProperties(object, physicalGroup);
        propertyGroups_.push_back(physicalGroup);

        // Logical Properties
        PropertyGroup logicalGroup;
        logicalGroup.id = "logical";
        logicalGroup.name = "Logical Properties";
        logicalGroup.category = PropertyCategory::LOGICAL;
        logicalGroup.isExpanded = false;
        createLogicalProperties(object, logicalGroup);
        propertyGroups_.push_back(logicalGroup);

        // Security Properties
        PropertyGroup securityGroup;
        securityGroup.id = "security";
        securityGroup.name = "Security";
        securityGroup.category = PropertyCategory::SECURITY;
        securityGroup.isExpanded = false;
        createSecurityProperties(object, securityGroup);
        propertyGroups_.push_back(securityGroup);

        // Performance Properties
        PropertyGroup performanceGroup;
        performanceGroup.id = "performance";
        performanceGroup.name = "Performance";
        performanceGroup.category = PropertyCategory::PERFORMANCE;
        performanceGroup.isExpanded = false;
        createPerformanceProperties(object, performanceGroup);
        propertyGroups_.push_back(performanceGroup);

        // Storage Properties
        PropertyGroup storageGroup;
        storageGroup.id = "storage";
        storageGroup.name = "Storage";
        storageGroup.category = PropertyCategory::STORAGE;
        storageGroup.isExpanded = false;
        createStorageProperties(object, storageGroup);
        propertyGroups_.push_back(storageGroup);

        // Relationship Properties
        PropertyGroup relationshipGroup;
        relationshipGroup.id = "relationships";
        relationshipGroup.name = "Relationships";
        relationshipGroup.category = PropertyCategory::RELATIONSHIPS;
        relationshipGroup.isExpanded = false;
        createRelationshipProperties(object, relationshipGroup);
        propertyGroups_.push_back(relationshipGroup);

        // Extended Properties
        if (!object.properties.empty()) {
            PropertyGroup extendedGroup;
            extendedGroup.id = "extended";
            extendedGroup.name = "Extended Properties";
            extendedGroup.category = PropertyCategory::EXTENDED;
            extendedGroup.isExpanded = false;
            createExtendedProperties(object, extendedGroup);
            propertyGroups_.push_back(extendedGroup);
        }
    }

    void createGeneralProperties(const SchemaObject& object, PropertyGroup& group) {
        // Name
        PropertyDefinition nameProp;
        nameProp.id = "name";
        nameProp.name = "Name";
        nameProp.description = "Object name";
        nameProp.dataType = PropertyDataType::STRING;
        nameProp.currentValue = object.name;
        nameProp.isReadOnly = true;
        nameProp.category = PropertyCategory::GENERAL;
        group.properties.push_back(nameProp);

        // Schema
        PropertyDefinition schemaProp;
        schemaProp.id = "schema";
        schemaProp.name = "Schema";
        schemaProp.description = "Schema name";
        schemaProp.dataType = PropertyDataType::STRING;
        schemaProp.currentValue = object.schema;
        schemaProp.isReadOnly = true;
        schemaProp.category = PropertyCategory::GENERAL;
        group.properties.push_back(schemaProp);

        // Type
        PropertyDefinition typeProp;
        typeProp.id = "type";
        typeProp.name = "Type";
        typeProp.description = "Object type";
        typeProp.dataType = PropertyDataType::STRING;
        typeProp.currentValue = toString(object.type);
        typeProp.isReadOnly = true;
        typeProp.category = PropertyCategory::GENERAL;
        group.properties.push_back(typeProp);

        // Owner
        if (!object.owner.empty()) {
            PropertyDefinition ownerProp;
            ownerProp.id = "owner";
            ownerProp.name = "Owner";
            ownerProp.description = "Object owner";
            ownerProp.dataType = PropertyDataType::STRING;
            ownerProp.currentValue = object.owner;
            ownerProp.isReadOnly = true;
            ownerProp.category = PropertyCategory::GENERAL;
            group.properties.push_back(ownerProp);
        }

        // Created At
        if (object.createdAt != std::chrono::system_clock::time_point()) {
            PropertyDefinition createdProp;
            createdProp.id = "created_at";
            createdProp.name = "Created At";
            createdProp.description = "Creation timestamp";
            createdProp.dataType = PropertyDataType::DATE_TIME;
            createdProp.currentValue = formatTimestamp(object.createdAt);
            createdProp.isReadOnly = true;
            createdProp.category = PropertyCategory::GENERAL;
            group.properties.push_back(createdProp);
        }

        // Modified At
        if (object.modifiedAt != std::chrono::system_clock::time_point()) {
            PropertyDefinition modifiedProp;
            modifiedProp.id = "modified_at";
            modifiedProp.name = "Modified At";
            modifiedProp.description = "Last modification timestamp";
            modifiedProp.dataType = PropertyDataType::DATE_TIME;
            modifiedProp.currentValue = formatTimestamp(object.modifiedAt);
            modifiedProp.isReadOnly = true;
            modifiedProp.category = PropertyCategory::GENERAL;
            group.properties.push_back(modifiedProp);
        }
    }

    void createPhysicalProperties(const SchemaObject& object, PropertyGroup& group) {
        // System Object
        PropertyDefinition systemProp;
        systemProp.id = "is_system_object";
        systemProp.name = "System Object";
        systemProp.description = "Whether this is a system object";
        systemProp.dataType = PropertyDataType::BOOLEAN;
        systemProp.currentValue = object.isSystemObject ? "true" : "false";
        systemProp.isReadOnly = true;
        systemProp.category = PropertyCategory::PHYSICAL;
        group.properties.push_back(systemProp);

        // Temporary Object
        PropertyDefinition tempProp;
        tempProp.id = "is_temporary";
        tempProp.name = "Temporary Object";
        tempProp.description = "Whether this is a temporary object";
        tempProp.dataType = PropertyDataType::BOOLEAN;
        tempProp.currentValue = object.isTemporary ? "true" : "false";
        tempProp.isReadOnly = true;
        tempProp.category = PropertyCategory::PHYSICAL;
        group.properties.push_back(tempProp);
    }

    void createLogicalProperties(const SchemaObject& object, PropertyGroup& group) {
        // This would be populated based on object-specific logical properties
        // For example, for tables: row count, column count, etc.
        // For indexes: index type, columns, etc.
        // Implementation would vary based on object type
    }

    void createSecurityProperties(const SchemaObject& object, PropertyGroup& group) {
        // Security-related properties would be populated here
        // For example: permissions, grants, security policies, etc.
    }

    void createPerformanceProperties(const SchemaObject& object, PropertyGroup& group) {
        // Performance-related properties would be populated here
        // For example: size, access patterns, performance metrics, etc.
    }

    void createStorageProperties(const SchemaObject& object, PropertyGroup& group) {
        // Storage-related properties would be populated here
        // For example: tablespace, storage parameters, etc.
    }

    void createRelationshipProperties(const SchemaObject& object, PropertyGroup& group) {
        // Relationship properties would be populated here
        // For example: dependencies, dependents, foreign keys, etc.
    }

    void createExtendedProperties(const SchemaObject& object, PropertyGroup& group) {
        // Extended properties from the object's properties map
        for (const auto& [key, value] : object.properties) {
            PropertyDefinition prop;
            prop.id = "ext_" + key;
            prop.name = key;
            prop.description = "Extended property: " + key;
            prop.dataType = inferDataType(value);
            prop.currentValue = value;
            prop.isReadOnly = true; // Extended properties are typically read-only
            prop.category = PropertyCategory::EXTENDED;
            group.properties.push_back(prop);
        }
    }

    std::string toString(PropertyDataType type) {
        switch (type) {
            case PropertyDataType::STRING: return "String";
            case PropertyDataType::INTEGER: return "Integer";
            case PropertyDataType::DECIMAL: return "Decimal";
            case PropertyDataType::BOOLEAN: return "Boolean";
            case PropertyDataType::DATE_TIME: return "Date/Time";
            case PropertyDataType::SIZE: return "Size";
            case PropertyDataType::DURATION: return "Duration";
            case PropertyDataType::PERCENTAGE: return "Percentage";
            case PropertyDataType::IDENTIFIER: return "Identifier";
            case PropertyDataType::SQL_TYPE: return "SQL Type";
            case PropertyDataType::FILE_PATH: return "File Path";
            case PropertyDataType::URL: return "URL";
            case PropertyDataType::EMAIL: return "Email";
            case PropertyDataType::PHONE: return "Phone";
            case PropertyDataType::CURRENCY: return "Currency";
            case PropertyDataType::LIST: return "List";
            case PropertyDataType::OBJECT_REFERENCE: return "Object Reference";
            default: return "Unknown";
        }
    }

    std::string toString(PropertyCategory category) {
        switch (category) {
            case PropertyCategory::GENERAL: return "General";
            case PropertyCategory::PHYSICAL: return "Physical";
            case PropertyCategory::LOGICAL: return "Logical";
            case PropertyCategory::SECURITY: return "Security";
            case PropertyCategory::PERFORMANCE: return "Performance";
            case PropertyCategory::STORAGE: return "Storage";
            case PropertyCategory::RELATIONSHIPS: return "Relationships";
            case PropertyCategory::EXTENDED: return "Extended";
            default: return "Unknown";
        }
    }

    PropertyDataType inferDataType(const std::string& value) {
        // Simple data type inference
        if (value == "true" || value == "false") {
            return PropertyDataType::BOOLEAN;
        }

        if (value.find_first_not_of("0123456789") == std::string::npos) {
            return PropertyDataType::INTEGER;
        }

        if (value.find_first_not_of("0123456789.") == std::string::npos && value.find('.') != std::string::npos) {
            return PropertyDataType::DECIMAL;
        }

        return PropertyDataType::STRING;
    }

    QColor getPropertyColor(PropertyCategory category) {
        switch (category) {
            case PropertyCategory::GENERAL: return QColor(240, 240, 240);
            case PropertyCategory::PHYSICAL: return QColor(255, 255, 200);
            case PropertyCategory::LOGICAL: return QColor(200, 255, 200);
            case PropertyCategory::SECURITY: return QColor(255, 200, 200);
            case PropertyCategory::PERFORMANCE: return QColor(200, 200, 255);
            case PropertyCategory::STORAGE: return QColor(255, 200, 255);
            case PropertyCategory::RELATIONSHIPS: return QColor(200, 255, 255);
            case PropertyCategory::EXTENDED: return QColor(255, 220, 180);
            default: return QColor(240, 240, 240);
        }
    }

    std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    PropertyViewer* parent_;
    std::unordered_map<std::string, std::shared_ptr<QWidget>> editorWidgets_;
};

// PropertyViewer implementation

PropertyViewer::PropertyViewer(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>(this)) {

    setupUI();
}

PropertyViewer::~PropertyViewer() = default;

void PropertyViewer::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    toolbarLayout_ = new QHBoxLayout();

    // Search box
    searchBox_ = new QLineEdit();
    searchBox_->setPlaceholderText("Search properties...");
    searchBox_->setMaximumWidth(300);
    toolbarLayout_->addWidget(searchBox_);

    // Search buttons
    searchButton_ = new QPushButton("Search");
    searchButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(searchButton_);

    clearSearchButton_ = new QPushButton("Clear");
    clearSearchButton_->setMaximumWidth(60);
    toolbarLayout_->addWidget(clearSearchButton_);

    toolbarLayout_->addSpacing(10);

    // Display mode selector
    displayModeCombo_ = new QComboBox();
    displayModeCombo_->addItem("Grid View", static_cast<int>(PropertyDisplayMode::GRID));
    displayModeCombo_->addItem("Form View", static_cast<int>(PropertyDisplayMode::FORM));
    displayModeCombo_->addItem("Tree View", static_cast<int>(PropertyDisplayMode::TREE));
    displayModeCombo_->addItem("Text View", static_cast<int>(PropertyDisplayMode::TEXT));
    displayModeCombo_->setMaximumWidth(120);
    toolbarLayout_->addWidget(displayModeCombo_);

    toolbarLayout_->addSpacing(10);

    // Control buttons
    refreshButton_ = new QPushButton("Refresh");
    refreshButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(refreshButton_);

    exportButton_ = new QPushButton("Export");
    exportButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(exportButton_);

    importButton_ = new QPushButton("Import");
    importButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(importButton_);

    applyChangesButton_ = new QPushButton("Apply");
    applyChangesButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(applyChangesButton_);

    revertChangesButton_ = new QPushButton("Revert");
    revertChangesButton_->setMaximumWidth(80);
    toolbarLayout_->addWidget(revertChangesButton_);

    toolbarLayout_->addStretch();

    mainLayout_->addLayout(toolbarLayout_);

    // Tab widget for different views
    tabWidget_ = new QTabWidget();
    mainLayout_->addWidget(tabWidget_);

    // Status labels
    auto statusLayout = new QHBoxLayout();
    objectInfoLabel_ = new QLabel("No object selected");
    statusLayout->addWidget(objectInfoLabel_);

    modificationStatusLabel_ = new QLabel("");
    statusLayout->addStretch();
    statusLayout->addWidget(modificationStatusLabel_);

    mainLayout_->addLayout(statusLayout);

    // Setup views
    impl_->setupGridView();
    impl_->setupFormView();
    impl_->setupTreeView();
    impl_->setupTextView();

    // Add views to tabs
    tabWidget_->addTab(gridView_, "Grid");
    tabWidget_->addTab(formView_, "Form");
    tabWidget_->addTab(treeView_, "Tree");
    tabWidget_->addTab(textView_, "Text");

    setupConnections();
}

void PropertyViewer::setupConnections() {
    // Search connections
    connect(searchBox_, &QLineEdit::returnPressed, this, &PropertyViewer::onSearchButtonClicked);
    connect(searchButton_, &QPushButton::clicked, this, &PropertyViewer::onSearchButtonClicked);
    connect(clearSearchButton_, &QPushButton::clicked, this, &PropertyViewer::onClearSearchButtonClicked);
    connect(searchBox_, &QLineEdit::textChanged, this, &PropertyViewer::onSearchTextChanged);

    // Display mode connections
    connect(displayModeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PropertyViewer::onDisplayModeChanged);

    // Button connections
    connect(refreshButton_, &QPushButton::clicked, this, &PropertyViewer::onRefreshButtonClicked);
    connect(exportButton_, &QPushButton::clicked, this, &PropertyViewer::onExportButtonClicked);
    connect(importButton_, &QPushButton::clicked, this, &PropertyViewer::onImportButtonClicked);
    connect(applyChangesButton_, &QPushButton::clicked, this, &PropertyViewer::onApplyChangesButtonClicked);
    connect(revertChangesButton_, &QPushButton::clicked, this, &PropertyViewer::onRevertChangesButtonClicked);
}

void PropertyViewer::initialize(const PropertyDisplayOptions& options) {
    displayOptions_ = options;
    displayModeCombo_->setCurrentIndex(static_cast<int>(options.mode));
    updateModificationStatus();
}

void PropertyViewer::setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) {
    metadataManager_ = metadataManager;
}

void PropertyViewer::setObjectHierarchy(std::shared_ptr<IObjectHierarchy> objectHierarchy) {
    objectHierarchy_ = objectHierarchy;
}

void PropertyViewer::setCacheManager(std::shared_ptr<ICacheManager> cacheManager) {
    cacheManager_ = cacheManager;
}

void PropertyViewer::displayObjectProperties(const std::string& nodeId) {
    currentNodeId_ = nodeId;

    // In a real implementation, this would retrieve the object from metadata manager
    // For now, create a placeholder object
    SchemaObject placeholder;
    placeholder.name = "example_table";
    placeholder.schema = "public";
    placeholder.database = "example_db";
    placeholder.type = SchemaObjectType::TABLE;
    placeholder.owner = "postgres";
    placeholder.properties["row_count"] = "1000";
    placeholder.properties["size"] = "8192";

    displayObjectProperties(placeholder);
}

void PropertyViewer::displayObjectProperties(const SchemaObject& object) {
    currentObject_ = std::make_shared<SchemaObject>(object);
    impl_->createPropertyGroups(object);
    refreshDisplay();
    updateObjectInfo();
}

void PropertyViewer::displayPropertyGroup(const PropertyGroup& group) {
    // Display a specific property group
    propertyGroups_.clear();
    propertyGroups_.push_back(group);
    refreshDisplay();
}

void PropertyViewer::setDisplayOptions(const PropertyDisplayOptions& options) {
    displayOptions_ = options;
    displayModeCombo_->setCurrentIndex(static_cast<int>(options.mode));
    refreshDisplay();
}

PropertyDisplayOptions PropertyViewer::getDisplayOptions() const {
    return displayOptions_;
}

std::vector<PropertyGroup> PropertyViewer::getPropertyGroups() const {
    return propertyGroups_;
}

PropertyGroup PropertyViewer::getPropertyGroup(const std::string& groupId) const {
    for (const auto& group : propertyGroups_) {
        if (group.id == groupId) {
            return group;
        }
    }
    return PropertyGroup();
}

std::vector<PropertyDefinition> PropertyViewer::getAllProperties() const {
    std::vector<PropertyDefinition> allProperties;
    for (const auto& group : propertyGroups_) {
        allProperties.insert(allProperties.end(), group.properties.begin(), group.properties.end());
    }
    return allProperties;
}

void PropertyViewer::searchProperties(const PropertySearchOptions& options) {
    currentSearch_ = options;

    // Filter properties based on search criteria
    searchResults_.clear();

    for (const auto& group : propertyGroups_) {
        for (const auto& property : group.properties) {
            bool matches = true;

            // Check name
            if (options.searchInNames) {
                std::string searchText = options.caseSensitive ? property.name : toLower(property.name);
                std::string pattern = options.caseSensitive ? options.pattern : toLower(options.pattern);

                if (options.regexMode) {
                    try {
                        std::regex regexPattern(pattern);
                        matches = std::regex_search(searchText, regexPattern);
                    } catch (const std::regex_error&) {
                        matches = searchText.find(pattern) != std::string::npos;
                    }
                } else {
                    matches = searchText.find(pattern) != std::string::npos;
                }
            }

            if (matches && options.searchInValues) {
                std::string searchText = options.caseSensitive ? property.currentValue : toLower(property.currentValue);
                std::string pattern = options.caseSensitive ? options.pattern : toLower(options.pattern);
                matches = searchText.find(pattern) != std::string::npos;
            }

            if (matches) {
                searchResults_.push_back(property);
            }
        }
    }

    highlightSearchResults();
}

std::vector<PropertyDefinition> PropertyViewer::getSearchResults() const {
    return searchResults_;
}

void PropertyViewer::clearSearch() {
    currentSearch_ = PropertySearchOptions();
    searchBox_->clear();
    searchResults_.clear();
    refreshDisplay();
}

bool PropertyViewer::isPropertyModified(const std::string& propertyId) const {
    return modifiedProperties_.count(propertyId) > 0;
}

std::vector<std::string> PropertyViewer::getModifiedProperties() const {
    std::vector<std::string> modified;
    for (const auto& [id, value] : modifiedProperties_) {
        modified.push_back(id);
    }
    return modified;
}

void PropertyViewer::applyPropertyChanges() {
    // In a real implementation, this would apply changes to the database
    // For now, just clear the modified state
    modifiedProperties_.clear();
    originalValues_.clear();
    updateModificationStatus();
}

void PropertyViewer::revertPropertyChanges() {
    // Revert all modified properties to their original values
    for (const auto& [propertyId, originalValue] : originalValues_) {
        // Update the property value in the UI
        // This would need to be implemented for each view type
    }

    modifiedProperties_.clear();
    originalValues_.clear();
    updateModificationStatus();
}

void PropertyViewer::exportProperties(const std::string& filePath, const std::string& format) {
    // Implementation would export properties to file
    qDebug() << "Exporting properties to" << QString::fromStdString(filePath);
}

void PropertyViewer::importProperties(const std::string& filePath) {
    // Implementation would import properties from file
    qDebug() << "Importing properties from" << QString::fromStdString(filePath);
}

void PropertyViewer::refreshProperties() {
    if (!currentNodeId_.empty()) {
        displayObjectProperties(currentNodeId_);
    }
}

void PropertyViewer::clearProperties() {
    currentNodeId_.clear();
    currentObject_ = std::make_shared<SchemaObject>();
    propertyGroups_.clear();
    searchResults_.clear();
    modifiedProperties_.clear();
    originalValues_.clear();

    gridView_->setRowCount(0);
    treeView_->clear();
    textView_->clear();

    // Clear form view
    if (formView_->widget()) {
        delete formView_->widget();
        formView_->setWidget(new QWidget());
    }

    updateObjectInfo();
    updateModificationStatus();
}

void PropertyViewer::setPropertyChangedCallback(PropertyChangedCallback callback) {
    propertyChangedCallback_ = callback;
}

void PropertyViewer::setPropertyGroupChangedCallback(PropertyGroupChangedCallback callback) {
    propertyGroupChangedCallback_ = callback;
}

QWidget* PropertyViewer::getWidget() {
    return this;
}

QSize PropertyViewer::sizeHint() const {
    return QSize(600, 400);
}

void PropertyViewer::refreshDisplay() {
    switch (displayOptions_.mode) {
        case PropertyDisplayMode::GRID:
            impl_->populateGridView();
            break;
        case PropertyDisplayMode::FORM:
            impl_->populateFormView();
            break;
        case PropertyDisplayMode::TREE:
            impl_->populateTreeView();
            break;
        case PropertyDisplayMode::TEXT:
            impl_->populateTextView();
            break;
        case PropertyDisplayMode::CUSTOM:
            // Custom view implementation
            break;
    }
}

void PropertyViewer::highlightSearchResults() {
    // Implementation would highlight search results in the current view
    // This is view-specific and would need to be implemented for each view type
}

void PropertyViewer::updateObjectInfo() {
    if (currentObject_.name.empty()) {
        objectInfoLabel_->setText("No object selected");
    } else {
        QString typeStr;
        switch (currentObject_.type) {
            case SchemaObjectType::TABLE: typeStr = "Table"; break;
            case SchemaObjectType::VIEW: typeStr = "View"; break;
            case SchemaObjectType::COLUMN: typeStr = "Column"; break;
            case SchemaObjectType::INDEX: typeStr = "Index"; break;
            case SchemaObjectType::CONSTRAINT: typeStr = "Constraint"; break;
            case SchemaObjectType::TRIGGER: typeStr = "Trigger"; break;
            case SchemaObjectType::FUNCTION: typeStr = "Function"; break;
            case SchemaObjectType::PROCEDURE: typeStr = "Procedure"; break;
            case SchemaObjectType::SEQUENCE: typeStr = "Sequence"; break;
            case SchemaObjectType::DOMAIN: typeStr = "Domain"; break;
            case SchemaObjectType::TYPE: typeStr = "Type"; break;
            case SchemaObjectType::RULE: typeStr = "Rule"; break;
            default: typeStr = "Unknown"; break;
        }

        if (currentObject_) {
            QString info = QString("Object: %1.%2 (%3)")
                .arg(QString::fromStdString(currentObject_->schema))
                .arg(QString::fromStdString(currentObject_->name))
                .arg(typeStr);
            objectInfoLabel_->setText(info);
        } else {
            objectInfoLabel_->setText("No object selected");
        }
    }
}

void PropertyViewer::updateModificationStatus() {
    if (modifiedProperties_.empty()) {
        modificationStatusLabel_->setText("");
        applyChangesButton_->setEnabled(false);
        revertChangesButton_->setEnabled(false);
    } else {
        QString status = QString("%1 properties modified").arg(modifiedProperties_.size());
        modificationStatusLabel_->setText(status);
        applyChangesButton_->setEnabled(true);
        revertChangesButton_->setEnabled(true);
    }
}

std::string PropertyViewer::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Private slot implementations

void PropertyViewer::onSearchTextChanged(const QString& text) {
    if (displayOptions_.mode == PropertyDisplayMode::GRID) {
        // Real-time search for grid view
        PropertySearchOptions options;
        options.pattern = text.toStdString();
        searchProperties(options);
    }
}

void PropertyViewer::onSearchButtonClicked() {
    PropertySearchOptions options;
    options.pattern = searchBox_->text().toStdString();
    options.caseSensitive = false;
    options.regexMode = false;
    searchProperties(options);
}

void PropertyViewer::onClearSearchButtonClicked() {
    clearSearch();
}

void PropertyViewer::onDisplayModeChanged(int index) {
    PropertyDisplayMode mode = static_cast<PropertyDisplayMode>(
        displayModeCombo_->itemData(index).toInt());
    displayOptions_.mode = mode;
    refreshDisplay();
}

void PropertyViewer::onRefreshButtonClicked() {
    refreshProperties();
}

void PropertyViewer::onExportButtonClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Properties",
        "properties.json", "JSON Files (*.json);;XML Files (*.xml);;Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        exportProperties(fileName.toStdString(), "JSON");
    }
}

void PropertyViewer::onImportButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Properties",
        "", "JSON Files (*.json);;XML Files (*.xml);;Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        importProperties(fileName.toStdString());
    }
}

void PropertyViewer::onApplyChangesButtonClicked() {
    applyPropertyChanges();
}

void PropertyViewer::onRevertChangesButtonClicked() {
    revertPropertyChanges();
}

} // namespace scratchrobin
