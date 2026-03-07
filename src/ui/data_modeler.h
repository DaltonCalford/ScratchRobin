#pragma once
#include "ui/dock_workspace.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QGraphicsView;
class QGraphicsScene;
class QTreeView;
class QTableView;
class QComboBox;
class QLineEdit;
class QTextEdit;
class QTabWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class QStandardItemModel;
class QSplitter;
class QMenu;
class QToolBar;
QT_END_NAMESPACE

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::ui {

/**
 * @brief Data Modeler - Visual database schema design tool
 * 
 * Create and modify database schemas visually:
 * - Drag-and-drop table creation
 * - Relationship editor with visual connections
 * - Attribute/property panels
 * - Forward engineering (model to SQL)
 * - Reverse engineering (database to model)
 * - Multiple diagram support
 * - Model validation
 */

// ============================================================================
// Model Entity Types
// ============================================================================
enum class EntityType {
    Table,
    View,
    Index,
    Constraint,
    Relationship
};

struct ModelAttribute {
    QString name;
    QString dataType;
    int length = 0;
    int precision = 0;
    int scale = 0;
    bool isNullable = true;
    bool isPrimaryKey = false;
    bool isForeignKey = false;
    bool isUnique = false;
    bool isAutoIncrement = false;
    QString defaultValue;
    QString comment;
};

struct ModelIndex {
    QString name;
    QStringList columns;
    bool isUnique = false;
    bool isPrimary = false;
    QString type; // BTREE, HASH, etc.
};

struct ModelConstraint {
    QString name;
    QString type; // CHECK, UNIQUE, PRIMARY KEY, FOREIGN KEY
    QString definition;
};

struct ModelRelationship {
    QString name;
    QString parentTable;
    QString childTable;
    QStringList parentColumns;
    QStringList childColumns;
    QString cardinality; // 1:1, 1:N, N:M
    QString onDelete;    // CASCADE, SET NULL, RESTRICT, etc.
    QString onUpdate;
};

struct ModelEntity {
    QString name;
    EntityType type;
    QPointF position;
    QSizeF size;
    QColor color;
    QString comment;
    
    // For tables
    QList<ModelAttribute> attributes;
    QList<ModelIndex> indexes;
    QList<ModelConstraint> constraints;
};

// ============================================================================
// Data Model
// ============================================================================
class DataModel {
public:
    QString name;
    QString version;
    QString description;
    QString databaseType;
    QDateTime created;
    QDateTime modified;
    
    QList<ModelEntity> entities;
    QList<ModelRelationship> relationships;
    
    void addEntity(const ModelEntity& entity);
    void removeEntity(const QString& name);
    void addRelationship(const ModelRelationship& rel);
    void removeRelationship(const QString& name);
    
    QString generateSql() const;
    bool validate(QStringList& errors) const;
};

// ============================================================================
// Data Modeler Panel
// ============================================================================
class DataModelerPanel : public DockPanel {
    Q_OBJECT

public:
    explicit DataModelerPanel(backend::SessionClient* client, QWidget* parent = nullptr);
    
    QString panelTitle() const override { return tr("Data Modeler"); }
    QString panelCategory() const override { return "database"; }

public slots:
    // File operations
    void onNewModel();
    void onOpenModel();
    void onSaveModel();
    void onSaveModelAs();
    void onImportModel();
    void onExportModel();
    
    // Entity operations
    void onAddTable();
    void onAddView();
    void onAddRelationship();
    void onEditEntity();
    void onDeleteEntity();
    void onDuplicateEntity();
    
    // Tools
    void onReverseEngineer();
    void onForwardEngineer();
    void onValidateModel();
    void onCompareWithDatabase();
    void onAutoLayout();
    void onZoomFit();
    void onZoomIn();
    void onZoomOut();
    
    // Selection
    void onEntitySelected(const QModelIndex& index);
    void onCanvasSelectionChanged();
    void onContextMenuRequested(const QPoint& pos);

signals:
    void modelModified();
    void sqlGenerated(const QString& sql);

private:
    void setupUi();
    void setupToolbar();
    void setupCanvas();
    void setupModelBrowser();
    void setupPropertyPanel();
    void updatePropertyPanel();
    void refreshModelBrowser();
    void drawEntity(const ModelEntity& entity);
    void drawRelationship(const ModelRelationship& rel);
    void drawModel();
    
    backend::SessionClient* client_;
    DataModel* currentModel_ = nullptr;
    
    // Canvas
    QGraphicsView* canvas_ = nullptr;
    QGraphicsScene* scene_ = nullptr;
    qreal zoomLevel_ = 1.0;
    
    // Model browser
    QTreeView* modelTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    
    // Property panel
    QTabWidget* propertyTabs_ = nullptr;
    QWidget* entityProperties_ = nullptr;
    QWidget* attributeProperties_ = nullptr;
    QWidget* relationshipProperties_ = nullptr;
    
    // Toolbars
    QToolBar* mainToolbar_ = nullptr;
    QToolBar* diagramToolbar_ = nullptr;
};

// ============================================================================
// Entity Editor Dialog
// ============================================================================
class EntityEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit EntityEditorDialog(ModelEntity* entity, 
                                backend::SessionClient* client,
                                QWidget* parent = nullptr);

public slots:
    void onAddAttribute();
    void onEditAttribute();
    void onDeleteAttribute();
    void onMoveAttributeUp();
    void onMoveAttributeDown();
    void onAddIndex();
    void onEditIndex();
    void onDeleteIndex();
    void onAddConstraint();
    void onEditConstraint();
    void onDeleteConstraint();
    void onPreviewSql();
    void onAccept();

private:
    void setupUi();
    void setupAttributesTab();
    void setupIndexesTab();
    void setupConstraintsTab();
    void loadAttributes();
    void loadIndexes();
    void loadConstraints();
    
    ModelEntity* entity_;
    backend::SessionClient* client_;
    
    QLineEdit* nameEdit_ = nullptr;
    QTextEdit* commentEdit_ = nullptr;
    
    // Attributes
    QTableView* attrTable_ = nullptr;
    QStandardItemModel* attrModel_ = nullptr;
    
    // Indexes
    QTableView* indexTable_ = nullptr;
    QStandardItemModel* indexModel_ = nullptr;
    
    // Constraints
    QTableView* constraintTable_ = nullptr;
    QStandardItemModel* constraintModel_ = nullptr;
};

// ============================================================================
// Relationship Editor Dialog
// ============================================================================
class RelationshipEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit RelationshipEditorDialog(ModelRelationship* rel,
                                     const QList<ModelEntity>& entities,
                                     backend::SessionClient* client,
                                     QWidget* parent = nullptr);

public slots:
    void onParentTableChanged(int index);
    void onChildTableChanged(int index);
    void onAutoMapColumns();
    void onPreviewSql();
    void onAccept();

private:
    void setupUi();
    void loadColumns(const QString& table, QComboBox* combo);
    
    ModelRelationship* relationship_;
    backend::SessionClient* client_;
    QList<ModelEntity> entities_;
    
    QLineEdit* nameEdit_ = nullptr;
    QComboBox* parentTableCombo_ = nullptr;
    QComboBox* childTableCombo_ = nullptr;
    QComboBox* cardinalityCombo_ = nullptr;
    QComboBox* onDeleteCombo_ = nullptr;
    QComboBox* onUpdateCombo_ = nullptr;
    
    QList<QComboBox*> parentColumnCombos_;
    QList<QComboBox*> childColumnCombos_;
};

// ============================================================================
// Forward Engineer Dialog
// ============================================================================
class ForwardEngineerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ForwardEngineerDialog(const DataModel* model,
                                  backend::SessionClient* client,
                                  QWidget* parent = nullptr);

public slots:
    void onGenerate();
    void onPreview();
    void onExecute();
    void onExportSql();
    void onCopySql();

private:
    void setupUi();
    void generateSql();
    
    const DataModel* model_;
    backend::SessionClient* client_;
    
    QCheckBox* includeDropCheck_ = nullptr;
    QCheckBox* includeCommentsCheck_ = nullptr;
    QCheckBox* quoteIdentifiersCheck_ = nullptr;
    QCheckBox* createIndexesCheck_ = nullptr;
    QCheckBox* createFkCheck_ = nullptr;
    QTextEdit* sqlPreview_ = nullptr;
};

// ============================================================================
// Reverse Engineer Dialog
// ============================================================================
class ReverseEngineerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ReverseEngineerDialog(backend::SessionClient* client, QWidget* parent = nullptr);

public slots:
    void onRefreshObjects();
    void onSelectAll();
    void onDeselectAll();
    void onImport();
    void onPreview();

private:
    void setupUi();
    void loadDatabaseObjects();
    
    backend::SessionClient* client_;
    
    QComboBox* schemaCombo_ = nullptr;
    QTreeView* objectTree_ = nullptr;
    QStandardItemModel* model_ = nullptr;
    QCheckBox* includeIndexesCheck_ = nullptr;
    QCheckBox* includeConstraintsCheck_ = nullptr;
    QCheckBox* includeRelationshipsCheck_ = nullptr;
};

// ============================================================================
// Model Validation Dialog
// ============================================================================
class ModelValidationDialog : public QDialog {
    Q_OBJECT

public:
    explicit ModelValidationDialog(const DataModel* model, QWidget* parent = nullptr);

public slots:
    void onRunValidation();
    void onFixIssue();
    void onExportReport();

private:
    void setupUi();
    void validateModel();
    
    const DataModel* model_;
    
    QTableView* issuesTable_ = nullptr;
    QStandardItemModel* issuesModel_ = nullptr;
    QTextEdit* detailsEdit_ = nullptr;
};

} // namespace scratchrobin::ui
