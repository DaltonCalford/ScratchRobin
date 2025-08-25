#include "schema_manager_dialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QDateTime>

namespace scratchrobin {

SchemaManagerDialog::SchemaManagerDialog(QWidget* parent)
    : QDialog(parent)
    , mainLayout_(nullptr)
    , mainSplitter_(nullptr)
    , leftWidget_(nullptr)
    , leftLayout_(nullptr)
    , toolbarLayout_(nullptr)
    , createSchemaButton_(nullptr)
    , refreshButton_(nullptr)
    , exportButton_(nullptr)
    , importButton_(nullptr)
    , schemaTree_(nullptr)
    , contextMenu_(nullptr)
    , rightWidget_(nullptr)
    , rightLayout_(nullptr)
    , objectNameLabel_(nullptr)
    , objectTypeLabel_(nullptr)
    , createdLabel_(nullptr)
    , modifiedLabel_(nullptr)
    , definitionEdit_(nullptr)
    , commentEdit_(nullptr)
    , editButton_(nullptr)
    , deleteButton_(nullptr)
    , dialogButtons_(nullptr)
    , currentDatabaseType_(DatabaseType::POSTGRESQL)
    , driverManager_(&DatabaseDriverManager::instance())
{
    setupUI();
    setWindowTitle("Schema Manager");
    setModal(true);
    resize(1000, 700);
}

void SchemaManagerDialog::setupUI() {
    mainLayout_ = new QVBoxLayout(this);

    // Main splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);

    // Setup left side (schema tree)
    setupTreeView();

    // Setup right side (properties panel)
    setupPropertyPanel();

    mainLayout_->addWidget(mainSplitter_);

    // Dialog buttons
    dialogButtons_ = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(dialogButtons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout_->addWidget(dialogButtons_);

    // Set splitter proportions
    mainSplitter_->setStretchFactor(0, 1);
    mainSplitter_->setStretchFactor(1, 2);
}

void SchemaManagerDialog::setupTreeView() {
    leftWidget_ = new QWidget();
    leftLayout_ = new QVBoxLayout(leftWidget_);

    // Toolbar
    setupToolbar();
    leftLayout_->addLayout(toolbarLayout_);

    // Schema tree
    schemaTree_ = new QTreeWidget(leftWidget_);
    schemaTree_->setHeaderLabel("Database Objects");
    schemaTree_->setContextMenuPolicy(Qt::CustomContextMenu);

    leftLayout_->addWidget(schemaTree_);

    // Set up context menu
    setupContextMenu();

    // Connect signals
    connect(schemaTree_, &QTreeWidget::itemSelectionChanged, this, &SchemaManagerDialog::onObjectSelected);
    connect(schemaTree_, &QTreeWidget::itemDoubleClicked, this, &SchemaManagerDialog::onObjectDoubleClicked);
    connect(schemaTree_, &QTreeWidget::customContextMenuRequested, this, &SchemaManagerDialog::onContextMenuRequested);
    connect(createSchemaButton_, &QPushButton::clicked, this, &SchemaManagerDialog::onCreateSchema);
    connect(refreshButton_, &QPushButton::clicked, this, &SchemaManagerDialog::onRefreshSchemas);
    connect(exportButton_, &QPushButton::clicked, this, &SchemaManagerDialog::onExportSchema);
    connect(importButton_, &QPushButton::clicked, this, &SchemaManagerDialog::onImportSchema);

    mainSplitter_->addWidget(leftWidget_);
}

void SchemaManagerDialog::setupToolbar() {
    toolbarLayout_ = new QHBoxLayout();

    createSchemaButton_ = new QPushButton("Create Schema", this);
    refreshButton_ = new QPushButton("Refresh", this);
    exportButton_ = new QPushButton("Export", this);
    importButton_ = new QPushButton("Import", this);

    toolbarLayout_->addWidget(createSchemaButton_);
    toolbarLayout_->addWidget(refreshButton_);
    toolbarLayout_->addStretch();
    toolbarLayout_->addWidget(exportButton_);
    toolbarLayout_->addWidget(importButton_);
}

void SchemaManagerDialog::setupContextMenu() {
    contextMenu_ = new QMenu(this);

    QAction* createTableAction = contextMenu_->addAction("Create Table");
    QAction* createViewAction = contextMenu_->addAction("Create View");
    QAction* createProcedureAction = contextMenu_->addAction("Create Procedure");
    QAction* createFunctionAction = contextMenu_->addAction("Create Function");
    QAction* createIndexAction = contextMenu_->addAction("Create Index");
    QAction* createTriggerAction = contextMenu_->addAction("Create Trigger");
    contextMenu_->addSeparator();
    QAction* editAction = contextMenu_->addAction("Edit");
    QAction* deleteAction = contextMenu_->addAction("Delete");
    QAction* refreshAction = contextMenu_->addAction("Refresh");

    // Connect actions
    connect(createTableAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateTable);
    connect(createViewAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateView);
    connect(createProcedureAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateProcedure);
    connect(createFunctionAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateFunction);
    connect(createIndexAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateIndex);
    connect(createTriggerAction, &QAction::triggered, this, &SchemaManagerDialog::onCreateTrigger);
    connect(editAction, &QAction::triggered, this, [this]() {
        if (editButton_) editButton_->click();
    });
    connect(deleteAction, &QAction::triggered, this, [this]() {
        if (deleteButton_) deleteButton_->click();
    });
    connect(refreshAction, &QAction::triggered, this, &SchemaManagerDialog::onRefreshObjects);
}

void SchemaManagerDialog::setupPropertyPanel() {
    rightWidget_ = new QWidget();
    rightLayout_ = new QVBoxLayout(rightWidget_);

    // Object info
    objectNameLabel_ = new QLabel("No object selected", rightWidget_);
    objectNameLabel_->setStyleSheet("font-weight: bold; font-size: 14px;");

    objectTypeLabel_ = new QLabel("", rightWidget_);
    createdLabel_ = new QLabel("", rightWidget_);
    modifiedLabel_ = new QLabel("", rightWidget_);

    rightLayout_->addWidget(objectNameLabel_);
    rightLayout_->addWidget(objectTypeLabel_);
    rightLayout_->addWidget(createdLabel_);
    rightLayout_->addWidget(modifiedLabel_);
    rightLayout_->addSpacing(10);

    // Definition
    QLabel* definitionLabel = new QLabel("Definition:", rightWidget_);
    rightLayout_->addWidget(definitionLabel);

    definitionEdit_ = new QTextEdit(rightWidget_);
    definitionEdit_->setReadOnly(true);
    definitionEdit_->setMaximumHeight(200);
    rightLayout_->addWidget(definitionEdit_);

    // Comment
    QLabel* commentLabel = new QLabel("Comment:", rightWidget_);
    rightLayout_->addWidget(commentLabel);

    commentEdit_ = new QTextEdit(rightWidget_);
    commentEdit_->setReadOnly(true);
    commentEdit_->setMaximumHeight(100);
    rightLayout_->addWidget(commentEdit_);

    rightLayout_->addStretch();

    // Action buttons
    editButton_ = new QPushButton("Edit", rightWidget_);
    deleteButton_ = new QPushButton("Delete", rightWidget_);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(editButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addStretch();

    rightLayout_->addLayout(buttonLayout);

    // Initially disable buttons
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);

    connect(editButton_, &QPushButton::clicked, this, [this]() {
        // TODO: Open appropriate editor dialog based on object type
        QMessageBox::information(this, "Edit", "Object editing will be implemented when the appropriate dialog is available.");
    });

    connect(deleteButton_, &QPushButton::clicked, this, [this]() {
        // TODO: Delete object
        QMessageBox::information(this, "Delete", "Object deletion will be implemented in the next update.");
    });

    mainSplitter_->addWidget(rightWidget_);
}

void SchemaManagerDialog::setDatabaseType(DatabaseType type) {
    currentDatabaseType_ = type;
    refreshSchemaList();
}

void SchemaManagerDialog::setCurrentSchema(const QString& schemaName) {
    currentSchema_ = schemaName;
    populateSchemaTree();
}

void SchemaManagerDialog::refreshSchemaList() {
    // TODO: Load schemas from database
    // For now, create sample schemas
    schemas_.clear();

    SchemaDefinition sampleSchema;
    sampleSchema.name = "public";
    sampleSchema.owner = "postgres";
    sampleSchema.charset = "UTF8";
    sampleSchema.collation = "en_US.UTF-8";
    sampleSchema.comment = "Standard public schema";
    schemas_.append(sampleSchema);

    sampleSchema.name = "accounting";
    sampleSchema.owner = "accountant";
    sampleSchema.charset = "UTF8";
    sampleSchema.collation = "en_US.UTF-8";
    sampleSchema.comment = "Accounting department schema";
    schemas_.append(sampleSchema);

    populateSchemaTree();
}

void SchemaManagerDialog::populateSchemaTree() {
    schemaTree_->clear();

    for (const SchemaDefinition& schema : schemas_) {
        QTreeWidgetItem* schemaItem = new QTreeWidgetItem(schemaTree_);
        schemaItem->setText(0, schema.name);
        schemaItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));

        // Add object types
        QStringList objectTypes = {"Tables", "Views", "Procedures", "Functions", "Indexes", "Triggers"};

        for (const QString& type : objectTypes) {
            QTreeWidgetItem* typeItem = new QTreeWidgetItem(schemaItem);
            typeItem->setText(0, type);
            typeItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));

            // Add sample objects for demonstration
            if (type == "Tables") {
                QStringList tables = {"users", "products", "orders", "categories"};
                for (const QString& table : tables) {
                    QTreeWidgetItem* tableItem = new QTreeWidgetItem(typeItem);
                    tableItem->setText(0, table);
                    tableItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
                }
            }
            // TODO: Add other object types
        }

        schemaItem->setExpanded(true);
    }
}

void SchemaManagerDialog::populateSchemaObjects(const QString& schemaName) {
    // TODO: Load objects from database
    objects_.clear();

    // Add sample objects
    SchemaManagerObject sampleObj;
    sampleObj.name = "users";
    sampleObj.type = "TABLE";
    sampleObj.schema = schemaName;
    sampleObj.definition = "CREATE TABLE users (id SERIAL PRIMARY KEY, name VARCHAR(100), email VARCHAR(255));";
    sampleObj.created = QDateTime::currentDateTime();
    sampleObj.modified = QDateTime::currentDateTime();
    sampleObj.comment = "User accounts table";
    objects_.append(sampleObj);
}

void SchemaManagerDialog::updatePropertyPanel(const SchemaManagerObject& object) {
    objectNameLabel_->setText(object.name);
    objectTypeLabel_->setText("Type: " + object.type);
    createdLabel_->setText("Created: " + object.created.toString());
    modifiedLabel_->setText("Modified: " + object.modified.toString());
    definitionEdit_->setPlainText(object.definition);
    commentEdit_->setPlainText(object.comment);

    editButton_->setEnabled(true);
    deleteButton_->setEnabled(true);
}

void SchemaManagerDialog::accept() {
    QDialog::accept();
}

void SchemaManagerDialog::reject() {
    QDialog::reject();
}

// Schema management slots
void SchemaManagerDialog::onCreateSchema() {
    createSchemaDialog();
}

void SchemaManagerDialog::onEditSchema() {
    QTreeWidgetItem* currentItem = schemaTree_->currentItem();
    if (!currentItem) return;

    QString schemaName = currentItem->text(0);
    createSchemaDialog(schemaName);
}

void SchemaManagerDialog::onDeleteSchema() {
    QTreeWidgetItem* currentItem = schemaTree_->currentItem();
    if (!currentItem) return;

    QString schemaName = currentItem->text(0);
    deleteSchemaDialog(schemaName);
}

void SchemaManagerDialog::onRefreshSchemas() {
    refreshSchemaList();
}

void SchemaManagerDialog::onRefreshObjects() {
    // Refresh current schema objects
    if (!currentSchema_.isEmpty()) {
        populateSchemaObjects(currentSchema_);
    }
}

// Object creation slots
void SchemaManagerDialog::onCreateTable() {
    emit objectCreated(SchemaManagerObject{"New Table", "TABLE", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create Table", "Table creation dialog will be opened when implemented.");
}

void SchemaManagerDialog::onCreateView() {
    emit objectCreated(SchemaManagerObject{"New View", "VIEW", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create View", "View creation dialog will be opened when implemented.");
}

void SchemaManagerDialog::onCreateProcedure() {
    emit objectCreated(SchemaManagerObject{"New Procedure", "PROCEDURE", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create Procedure", "Procedure creation dialog will be opened when implemented.");
}

void SchemaManagerDialog::onCreateFunction() {
    emit objectCreated(SchemaManagerObject{"New Function", "FUNCTION", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create Function", "Function creation dialog will be opened when implemented.");
}

void SchemaManagerDialog::onCreateIndex() {
    emit objectCreated(SchemaManagerObject{"New Index", "INDEX", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create Index", "Index creation dialog will be opened when implemented.");
}

void SchemaManagerDialog::onCreateTrigger() {
    emit objectCreated(SchemaManagerObject{"New Trigger", "TRIGGER", currentSchema_, "", QDateTime::currentDateTime(), QDateTime::currentDateTime(), ""});
    QMessageBox::information(this, "Create Trigger", "Trigger creation dialog will be opened when implemented.");
}

// Tree navigation slots
void SchemaManagerDialog::onSchemaSelected(QTreeWidgetItem* item) {
    if (!item) return;

    QString itemText = item->text(0);

    // Check if it's a schema
    for (const SchemaDefinition& schema : schemas_) {
        if (schema.name == itemText) {
            currentSchema_ = schema.name;
            populateSchemaObjects(schema.name);
            return;
        }
    }

    // Check if it's an object type
    QStringList objectTypes = {"Tables", "Views", "Procedures", "Functions", "Indexes", "Triggers"};
    if (objectTypes.contains(itemText)) {
        currentObject_ = itemText;
        return;
    }

    // Check if it's an individual object
    if (item->parent() && item->parent()->text(0) != currentSchema_) {
        // Find object by name
        for (const SchemaManagerObject& obj : objects_) {
            if (obj.name == itemText) {
                updatePropertyPanel(obj);
                return;
            }
        }
    }

    // Clear property panel if nothing selected
    objectNameLabel_->setText("No object selected");
    objectTypeLabel_->setText("");
    createdLabel_->setText("");
    modifiedLabel_->setText("");
    definitionEdit_->clear();
    commentEdit_->clear();
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
}

void SchemaManagerDialog::onObjectSelected() {
    QList<QTreeWidgetItem*> selectedItems = schemaTree_->selectedItems();
    if (!selectedItems.isEmpty()) {
        onSchemaSelected(selectedItems.first());
    }
}

void SchemaManagerDialog::onObjectDoubleClicked(QTreeWidgetItem* item) {
    if (!item) return;

    // Open editor for the object
    onSchemaSelected(item);
    if (editButton_->isEnabled()) {
        editButton_->click();
    }
}

void SchemaManagerDialog::onContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = schemaTree_->itemAt(pos);
    if (item) {
        schemaTree_->setCurrentItem(item);
        contextMenu_->exec(schemaTree_->mapToGlobal(pos));
    }
}

// Action slots
void SchemaManagerDialog::onExportSchema() {
    QMessageBox::information(this, "Export Schema", "Schema export will be implemented in the next update.");
}

void SchemaManagerDialog::onImportSchema() {
    QMessageBox::information(this, "Import Schema", "Schema import will be implemented in the next update.");
}

void SchemaManagerDialog::createSchemaDialog(const QString& schemaName) {
    bool isEdit = !schemaName.isEmpty();

    QString name = QInputDialog::getText(this,
        isEdit ? "Edit Schema" : "Create Schema",
        "Schema Name:",
        QLineEdit::Normal,
        schemaName);

    if (name.isEmpty()) return;

    QString owner = QInputDialog::getText(this,
        "Schema Owner",
        "Owner:",
        QLineEdit::Normal,
        isEdit ? "postgres" : "");

    if (isEdit) {
        emit schemaAltered(name);
    } else {
        emit schemaCreated(name);
    }

    QMessageBox::information(this, isEdit ? "Schema Updated" : "Schema Created",
        QString("Schema '%1' has been %2.").arg(name, isEdit ? "updated" : "created"));
}

void SchemaManagerDialog::deleteSchemaDialog(const QString& schemaName) {
    if (QMessageBox::question(this, "Delete Schema",
        QString("Are you sure you want to delete schema '%1'?").arg(schemaName),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        emit schemaDropped(schemaName);
        QMessageBox::information(this, "Schema Deleted",
            QString("Schema '%1' has been deleted.").arg(schemaName));
    }
}

void SchemaManagerDialog::showObjectProperties(const SchemaManagerObject& object) {
    updatePropertyPanel(object);
}

void SchemaManagerDialog::updateButtonStates() {
    // Update button states based on selection
    QTreeWidgetItem* currentItem = schemaTree_->currentItem();
    bool hasSelection = (currentItem != nullptr);

    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
}

} // namespace scratchrobin
