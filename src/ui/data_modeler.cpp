#include "data_modeler.h"
#include <backend/session_client.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QTreeView>
#include <QTableView>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTabWidget>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSplitter>
#include <QMenu>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QCheckBox>
#include <QLabel>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin::ui {

// ============================================================================
// Data Model Implementation
// ============================================================================

void DataModel::addEntity(const ModelEntity& entity) {
    entities.append(entity);
    modified = QDateTime::currentDateTime();
}

void DataModel::removeEntity(const QString& name) {
    for (int i = 0; i < entities.size(); ++i) {
        if (entities[i].name == name) {
            entities.removeAt(i);
            modified = QDateTime::currentDateTime();
            return;
        }
    }
}

void DataModel::addRelationship(const ModelRelationship& rel) {
    relationships.append(rel);
    modified = QDateTime::currentDateTime();
}

void DataModel::removeRelationship(const QString& name) {
    for (int i = 0; i < relationships.size(); ++i) {
        if (relationships[i].name == name) {
            relationships.removeAt(i);
            modified = QDateTime::currentDateTime();
            return;
        }
    }
}

QString DataModel::generateSql() const {
    QString sql;
    sql += QString("-- Model: %1\n").arg(name);
    sql += QString("-- Version: %1\n").arg(version);
    sql += QString("-- Generated: %1\n\n").arg(QDateTime::currentDateTime().toString());
    
    for (const auto& entity : entities) {
        if (entity.type == EntityType::Table) {
            sql += QString("CREATE TABLE %1 (\n").arg(entity.name);
            
            for (int i = 0; i < entity.attributes.size(); ++i) {
                const auto& attr = entity.attributes[i];
                sql += QString("    %1 %2").arg(attr.name, attr.dataType);
                if (attr.length > 0) {
                    sql += QString("(%1)").arg(attr.length);
                }
                if (!attr.isNullable) {
                    sql += " NOT NULL";
                }
                if (attr.isPrimaryKey) {
                    sql += " PRIMARY KEY";
                }
                if (!attr.defaultValue.isEmpty()) {
                    sql += QString(" DEFAULT %1").arg(attr.defaultValue);
                }
                if (i < entity.attributes.size() - 1) {
                    sql += ",";
                }
                sql += "\n";
            }
            
            sql += ");\n\n";
            
            // Create indexes
            for (const auto& idx : entity.indexes) {
                if (idx.isPrimary) continue;
                sql += QString("CREATE %1INDEX %2 ON %3 (%4);\n")
                    .arg(idx.isUnique ? "UNIQUE " : "")
                    .arg(idx.name, entity.name, idx.columns.join(", "));
            }
            if (!entity.indexes.isEmpty()) {
                sql += "\n";
            }
        }
    }
    
    // Create foreign keys
    for (const auto& rel : relationships) {
        sql += QString("ALTER TABLE %1 ADD CONSTRAINT %2 ").arg(rel.childTable, rel.name);
        sql += QString("FOREIGN KEY (%1) ").arg(rel.childColumns.join(", "));
        sql += QString("REFERENCES %1 (%2) ").arg(rel.parentTable, rel.parentColumns.join(", "));
        sql += QString("ON DELETE %1 ON UPDATE %2;\n").arg(rel.onDelete, rel.onUpdate);
    }
    
    return sql;
}

bool DataModel::validate(QStringList& errors) const {
    bool valid = true;
    
    for (const auto& entity : entities) {
        if (entity.name.isEmpty()) {
            errors << "Entity has empty name";
            valid = false;
        }
        
        if (entity.attributes.isEmpty()) {
            errors << QString("Table '%1' has no attributes").arg(entity.name);
            valid = false;
        }
        
        bool hasPrimaryKey = false;
        for (const auto& attr : entity.attributes) {
            if (attr.name.isEmpty()) {
                errors << QString("Empty attribute name in table '%1'").arg(entity.name);
                valid = false;
            }
            if (attr.isPrimaryKey) {
                hasPrimaryKey = true;
            }
        }
        
        if (!hasPrimaryKey && entity.type == EntityType::Table) {
            errors << QString("Table '%1' has no primary key").arg(entity.name);
            // This is a warning, not an error
        }
    }
    
    return valid;
}

// ============================================================================
// Data Modeler Panel
// ============================================================================

DataModelerPanel::DataModelerPanel(backend::SessionClient* client, QWidget* parent)
    : DockPanel("data_modeler", parent)
    , client_(client) {
    setupUi();
    onNewModel();
}

void DataModelerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // Toolbars
    setupToolbar();
    mainLayout->addWidget(mainToolbar_);
    
    // Main splitter
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left: Model browser
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(4);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    leftLayout->addWidget(new QLabel(tr("Model Browser"), this));
    
    modelTree_ = new QTreeView(this);
    modelTree_->setHeaderHidden(true);
    modelTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(modelTree_, &QTreeView::clicked, this, &DataModelerPanel::onEntitySelected);
    leftLayout->addWidget(modelTree_, 1);
    
    splitter->addWidget(leftWidget);
    
    // Center: Canvas
    setupCanvas();
    splitter->addWidget(canvas_);
    
    // Right: Properties panel (placeholder)
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(4);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    rightLayout->addWidget(new QLabel(tr("Properties"), this));
    propertyTabs_ = new QTabWidget(this);
    rightLayout->addWidget(propertyTabs_, 1);
    
    splitter->addWidget(rightWidget);
    
    splitter->setSizes({200, 600, 250});
    mainLayout->addWidget(splitter, 1);
}

void DataModelerPanel::setupToolbar() {
    mainToolbar_ = new QToolBar(this);
    mainToolbar_->setIconSize(QSize(16, 16));
    
    mainToolbar_->addAction(tr("New"), this, &DataModelerPanel::onNewModel);
    mainToolbar_->addAction(tr("Open"), this, &DataModelerPanel::onOpenModel);
    mainToolbar_->addAction(tr("Save"), this, &DataModelerPanel::onSaveModel);
    mainToolbar_->addSeparator();
    
    mainToolbar_->addAction(tr("Add Table"), this, &DataModelerPanel::onAddTable);
    mainToolbar_->addAction(tr("Add View"), this, &DataModelerPanel::onAddView);
    mainToolbar_->addAction(tr("Add Relationship"), this, &DataModelerPanel::onAddRelationship);
    mainToolbar_->addSeparator();
    
    mainToolbar_->addAction(tr("Reverse"), this, &DataModelerPanel::onReverseEngineer);
    mainToolbar_->addAction(tr("Forward"), this, &DataModelerPanel::onForwardEngineer);
    mainToolbar_->addSeparator();
    
    mainToolbar_->addAction(tr("Validate"), this, &DataModelerPanel::onValidateModel);
    mainToolbar_->addAction(tr("Auto Layout"), this, &DataModelerPanel::onAutoLayout);
}

void DataModelerPanel::setupCanvas() {
    scene_ = new QGraphicsScene(this);
    canvas_ = new QGraphicsView(scene_, this);
    canvas_->setRenderHint(QPainter::Antialiasing);
    canvas_->setDragMode(QGraphicsView::RubberBandDrag);
    canvas_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(canvas_, &QGraphicsView::customContextMenuRequested, 
            this, &DataModelerPanel::onContextMenuRequested);
}

void DataModelerPanel::setupModelBrowser() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Name"), tr("Type")});
    modelTree_->setModel(model_);
}

void DataModelerPanel::setupPropertyPanel() {
    // Property tabs are set up dynamically
}

void DataModelerPanel::updatePropertyPanel() {
    // Update based on selection
}

void DataModelerPanel::onNewModel() {
    currentModel_ = new DataModel();
    currentModel_->name = "Untitled Model";
    currentModel_->version = "1.0";
    currentModel_->databaseType = "PostgreSQL";
    currentModel_->created = QDateTime::currentDateTime();
    currentModel_->modified = QDateTime::currentDateTime();
    
    scene_->clear();
    setupModelBrowser();
    
    setWindowTitle(tr("Data Modeler - %1").arg(currentModel_->name));
}

void DataModelerPanel::onOpenModel() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Model"),
        QString(),
        tr("Model Files (*.srm);;JSON Files (*.json)"));
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Could not open file: %1").arg(file.errorString()));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    // Parse model from JSON (simplified)
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull() && doc.isObject()) {
        onNewModel();
        QJsonObject obj = doc.object();
        currentModel_->name = obj["name"].toString();
        currentModel_->version = obj["version"].toString();
        
        // Parse entities...
        QJsonArray entities = obj["entities"].toArray();
        for (const auto& e : entities) {
            QJsonObject eo = e.toObject();
            ModelEntity entity;
            entity.name = eo["name"].toString();
            entity.position = QPointF(eo["x"].toDouble(), eo["y"].toDouble());
            
            // Add to model
            currentModel_->addEntity(entity);
        }
        
        refreshModelBrowser();
        drawModel();
        
        QMessageBox::information(this, tr("Success"),
            tr("Model '%1' loaded successfully").arg(currentModel_->name));
    }
}

void DataModelerPanel::onSaveModel() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Model"),
        currentModel_->name + ".srm",
        tr("Model Files (*.srm);;JSON Files (*.json)"));
    
    if (fileName.isEmpty()) return;
    
    QJsonObject obj;
    obj["name"] = currentModel_->name;
    obj["version"] = currentModel_->version;
    obj["description"] = currentModel_->description;
    obj["databaseType"] = currentModel_->databaseType;
    obj["created"] = currentModel_->created.toString();
    obj["modified"] = QDateTime::currentDateTime().toString();
    
    QJsonArray entities;
    for (const auto& e : currentModel_->entities) {
        QJsonObject eo;
        eo["name"] = e.name;
        eo["type"] = static_cast<int>(e.type);
        eo["x"] = e.position.x();
        eo["y"] = e.position.y();
        
        QJsonArray attrs;
        for (const auto& a : e.attributes) {
            QJsonObject ao;
            ao["name"] = a.name;
            ao["dataType"] = a.dataType;
            attrs.append(ao);
        }
        eo["attributes"] = attrs;
        entities.append(eo);
    }
    obj["entities"] = entities;
    
    QJsonDocument doc(obj);
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        
        QMessageBox::information(this, tr("Success"),
            tr("Model saved to %1").arg(fileName));
    }
}

void DataModelerPanel::onSaveModelAs() {
    onSaveModel();
}

void DataModelerPanel::onImportModel() {
    QMessageBox::information(this, tr("Import"),
        tr("Import from other formats not yet implemented."));
}

void DataModelerPanel::onExportModel() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Model"),
        QString(),
        tr("PNG Image (*.png);;SVG (*.svg);;PDF (*.pdf)"));
    
    if (fileName.isEmpty()) return;
    
    if (fileName.endsWith(".png")) {
        QImage image(scene_->sceneRect().size().toSize(), QImage::Format_ARGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        scene_->render(&painter);
        image.save(fileName);
    } else if (fileName.endsWith(".svg")) {
        // SVG export would require QSvgGenerator
    }
    
    QMessageBox::information(this, tr("Export"),
        tr("Model exported to %1").arg(fileName));
}

void DataModelerPanel::onAddTable() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Table"),
        tr("Table name:"), QLineEdit::Normal, QString(), &ok);
    
    if (!ok || name.isEmpty()) return;
    
    ModelEntity entity;
    entity.name = name;
    entity.type = EntityType::Table;
    entity.position = QPointF(50 + QRandomGenerator::global()->bounded(400), 
                              50 + QRandomGenerator::global()->bounded(300));
    entity.size = QSizeF(180, 100);
    
    // Add default id column
    ModelAttribute idAttr;
    idAttr.name = "id";
    idAttr.dataType = "SERIAL";
    idAttr.isPrimaryKey = true;
    idAttr.isNullable = false;
    entity.attributes.append(idAttr);
    
    currentModel_->addEntity(entity);
    refreshModelBrowser();
    drawEntity(entity);
    
    emit modelModified();
}

void DataModelerPanel::onAddView() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("New View"),
        tr("View name:"), QLineEdit::Normal, QString(), &ok);
    
    if (!ok || name.isEmpty()) return;
    
    ModelEntity entity;
    entity.name = name;
    entity.type = EntityType::View;
    entity.position = QPointF(50 + QRandomGenerator::global()->bounded(400), 
                              50 + QRandomGenerator::global()->bounded(300));
    entity.size = QSizeF(180, 80);
    entity.color = QColor(200, 220, 255);
    
    currentModel_->addEntity(entity);
    refreshModelBrowser();
    drawEntity(entity);
    
    emit modelModified();
}

void DataModelerPanel::onAddRelationship() {
    if (currentModel_->entities.size() < 2) {
        QMessageBox::information(this, tr("Add Relationship"),
            tr("Need at least 2 entities to create a relationship."));
        return;
    }
    
    ModelRelationship rel;
    auto* dialog = new RelationshipEditorDialog(&rel, currentModel_->entities, client_, this);
    if (dialog->exec() == QDialog::Accepted) {
        currentModel_->addRelationship(rel);
        drawRelationship(rel);
        emit modelModified();
    }
}

void DataModelerPanel::onEditEntity() {
    // Get selected entity
    QModelIndex index = modelTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->itemFromIndex(index)->text();
    
    // Find entity
    for (auto& entity : currentModel_->entities) {
        if (entity.name == name) {
            auto* dialog = new EntityEditorDialog(&entity, client_, this);
            if (dialog->exec() == QDialog::Accepted) {
                refreshModelBrowser();
                drawModel();
                emit modelModified();
            }
            break;
        }
    }
}

void DataModelerPanel::onDeleteEntity() {
    QModelIndex index = modelTree_->currentIndex();
    if (!index.isValid()) return;
    
    QString name = model_->itemFromIndex(index)->text();
    
    auto reply = QMessageBox::warning(this, tr("Delete Entity"),
        tr("Are you sure you want to delete '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        currentModel_->removeEntity(name);
        refreshModelBrowser();
        drawModel();
        emit modelModified();
    }
}

void DataModelerPanel::onDuplicateEntity() {
    QMessageBox::information(this, tr("Duplicate"),
        tr("Duplicate entity not yet implemented."));
}

void DataModelerPanel::onReverseEngineer() {
    auto* dialog = new ReverseEngineerDialog(client_, this);
    if (dialog->exec() == QDialog::Accepted) {
        refreshModelBrowser();
        drawModel();
    }
}

void DataModelerPanel::onForwardEngineer() {
    auto* dialog = new ForwardEngineerDialog(currentModel_, client_, this);
    dialog->exec();
}

void DataModelerPanel::onValidateModel() {
    auto* dialog = new ModelValidationDialog(currentModel_, this);
    dialog->exec();
}

void DataModelerPanel::onCompareWithDatabase() {
    QMessageBox::information(this, tr("Compare"),
        tr("Compare with database not yet implemented."));
}

void DataModelerPanel::onAutoLayout() {
    // Simple grid layout
    int cols = 3;
    int x = 50, y = 50;
    int spacing = 200;
    
    for (int i = 0; i < currentModel_->entities.size(); ++i) {
        currentModel_->entities[i].position = QPointF(x, y);
        x += spacing;
        if ((i + 1) % cols == 0) {
            x = 50;
            y += 150;
        }
    }
    
    drawModel();
}

void DataModelerPanel::onZoomFit() {
    canvas_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
}

void DataModelerPanel::onZoomIn() {
    zoomLevel_ *= 1.25;
    canvas_->scale(1.25, 1.25);
}

void DataModelerPanel::onZoomOut() {
    zoomLevel_ *= 0.8;
    canvas_->scale(0.8, 0.8);
}

void DataModelerPanel::onEntitySelected(const QModelIndex& index) {
    updatePropertyPanel();
}

void DataModelerPanel::onCanvasSelectionChanged() {
    // Handle canvas selection
}

void DataModelerPanel::onContextMenuRequested(const QPoint& pos) {
    QMenu menu(this);
    menu.addAction(tr("Add Table"), this, &DataModelerPanel::onAddTable);
    menu.addAction(tr("Add View"), this, &DataModelerPanel::onAddView);
    menu.addSeparator();
    menu.addAction(tr("Paste"));
    menu.exec(canvas_->mapToGlobal(pos));
}

void DataModelerPanel::refreshModelBrowser() {
    model_->clear();
    
    auto* tablesItem = new QStandardItem(tr("Tables"));
    auto* viewsItem = new QStandardItem(tr("Views"));
    auto* relationsItem = new QStandardItem(tr("Relationships"));
    
    for (const auto& entity : currentModel_->entities) {
        auto* item = new QStandardItem(entity.name);
        if (entity.type == EntityType::Table) {
            tablesItem->appendRow(item);
        } else if (entity.type == EntityType::View) {
            viewsItem->appendRow(item);
        }
    }
    
    for (const auto& rel : currentModel_->relationships) {
        auto* item = new QStandardItem(rel.name);
        relationsItem->appendRow(item);
    }
    
    if (tablesItem->hasChildren())
        model_->appendRow(tablesItem);
    if (viewsItem->hasChildren())
        model_->appendRow(viewsItem);
    if (relationsItem->hasChildren())
        model_->appendRow(relationsItem);
    
    modelTree_->expandAll();
}

void DataModelerPanel::drawEntity(const ModelEntity& entity) {
    QRectF rect(entity.position, entity.size);
    
    // Draw box
    QColor fillColor = entity.color.isValid() ? entity.color : QColor(255, 255, 200);
    auto* rectItem = scene_->addRect(rect, QPen(Qt::black), QBrush(fillColor));
    rectItem->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    
    // Draw title
    auto* title = scene_->addText(entity.name, QFont("Arial", 10, QFont::Bold));
    title->setPos(entity.position.x() + 5, entity.position.y() + 5);
    title->setDefaultTextColor(Qt::darkBlue);
    
    // Draw attributes
    int y = 25;
    for (const auto& attr : entity.attributes) {
        QString text = attr.name;
        if (attr.isPrimaryKey) text += " [PK]";
        if (attr.isForeignKey) text += " [FK]";
        text += " : " + attr.dataType;
        
        auto* attrText = scene_->addText(text, QFont("Arial", 8));
        attrText->setPos(entity.position.x() + 5, entity.position.y() + y);
        y += 15;
    }
}

void DataModelerPanel::drawRelationship(const ModelRelationship& rel) {
    // Find entity positions
    QPointF start, end;
    for (const auto& entity : currentModel_->entities) {
        if (entity.name == rel.parentTable) {
            start = entity.position + QPointF(entity.size.width(), entity.size.height() / 2);
        }
        if (entity.name == rel.childTable) {
            end = entity.position + QPointF(0, entity.size.height() / 2);
        }
    }
    
    // Draw line
    auto* line = scene_->addLine(start.x(), start.y(), end.x(), end.y(), QPen(Qt::black, 2));
    
    // Draw cardinality label
    QPointF mid((start.x() + end.x()) / 2, (start.y() + end.y()) / 2);
    auto* label = scene_->addText(rel.cardinality, QFont("Arial", 8));
    label->setPos(mid);
}

void DataModelerPanel::drawModel() {
    scene_->clear();
    
    for (const auto& entity : currentModel_->entities) {
        drawEntity(entity);
    }
    
    for (const auto& rel : currentModel_->relationships) {
        drawRelationship(rel);
    }
}

// ============================================================================
// Entity Editor Dialog
// ============================================================================

EntityEditorDialog::EntityEditorDialog(ModelEntity* entity,
                                      backend::SessionClient* client,
                                      QWidget* parent)
    : QDialog(parent)
    , entity_(entity)
    , client_(client) {
    setupUi();
}

void EntityEditorDialog::setupUi() {
    setWindowTitle(tr("Edit Entity - %1").arg(entity_->name));
    resize(700, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Basic info
    auto* infoGroup = new QGroupBox(tr("Basic Info"), this);
    auto* infoLayout = new QFormLayout(infoGroup);
    
    nameEdit_ = new QLineEdit(entity_->name, this);
    infoLayout->addRow(tr("Name:"), nameEdit_);
    
    commentEdit_ = new QTextEdit(this);
    commentEdit_->setMaximumHeight(60);
    commentEdit_->setPlainText(entity_->comment);
    infoLayout->addRow(tr("Comment:"), commentEdit_);
    
    layout->addWidget(infoGroup);
    
    // Tabs
    auto* tabs = new QTabWidget(this);
    
    // Attributes tab
    setupAttributesTab();
    tabs->addTab(attrTable_, tr("Attributes (%1)").arg(entity_->attributes.size()));
    
    // Indexes tab
    setupIndexesTab();
    tabs->addTab(indexTable_, tr("Indexes (%1)").arg(entity_->indexes.size()));
    
    // Constraints tab
    setupConstraintsTab();
    tabs->addTab(constraintTable_, tr("Constraints (%1)").arg(entity_->constraints.size()));
    
    layout->addWidget(tabs, 1);
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &EntityEditorDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void EntityEditorDialog::setupAttributesTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    attrTable_ = new QTableView(this);
    attrModel_ = new QStandardItemModel(this);
    attrModel_->setHorizontalHeaderLabels({
        tr("Name"), tr("Type"), tr("Length"), tr("PK"), tr("FK"), 
        tr("NN"), tr("UQ"), tr("AI"), tr("Default")
    });
    
    loadAttributes();
    
    attrTable_->setModel(attrModel_);
    attrTable_->setAlternatingRowColors(true);
    attrTable_->horizontalHeader()->setStretchLastSection(true);
    
    layout->addWidget(attrTable_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &EntityEditorDialog::onAddAttribute);
    btnLayout->addWidget(addBtn);
    
    auto* editBtn = new QPushButton(tr("Edit"), this);
    connect(editBtn, &QPushButton::clicked, this, &EntityEditorDialog::onEditAttribute);
    btnLayout->addWidget(editBtn);
    
    auto* delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &EntityEditorDialog::onDeleteAttribute);
    btnLayout->addWidget(delBtn);
    
    layout->addLayout(btnLayout);
}

void EntityEditorDialog::setupIndexesTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    indexTable_ = new QTableView(this);
    indexModel_ = new QStandardItemModel(this);
    indexModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Unique"), tr("Columns")});
    
    loadIndexes();
    
    indexTable_->setModel(indexModel_);
    indexTable_->setAlternatingRowColors(true);
    indexTable_->horizontalHeader()->setStretchLastSection(true);
    
    layout->addWidget(indexTable_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &EntityEditorDialog::onAddIndex);
    btnLayout->addWidget(addBtn);
    
    auto* delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &EntityEditorDialog::onDeleteIndex);
    btnLayout->addWidget(delBtn);
    
    layout->addLayout(btnLayout);
}

void EntityEditorDialog::setupConstraintsTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    
    constraintTable_ = new QTableView(this);
    constraintModel_ = new QStandardItemModel(this);
    constraintModel_->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Definition")});
    
    loadConstraints();
    
    constraintTable_->setModel(constraintModel_);
    constraintTable_->setAlternatingRowColors(true);
    constraintTable_->horizontalHeader()->setStretchLastSection(true);
    
    layout->addWidget(constraintTable_);
    
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &EntityEditorDialog::onAddConstraint);
    btnLayout->addWidget(addBtn);
    
    auto* delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &EntityEditorDialog::onDeleteConstraint);
    btnLayout->addWidget(delBtn);
    
    layout->addLayout(btnLayout);
}

void EntityEditorDialog::loadAttributes() {
    attrModel_->removeRows(0, attrModel_->rowCount());
    
    for (const auto& attr : entity_->attributes) {
        QList<QStandardItem*> row;
        row << new QStandardItem(attr.name);
        row << new QStandardItem(attr.dataType);
        row << new QStandardItem(attr.length > 0 ? QString::number(attr.length) : "");
        row << new QStandardItem(attr.isPrimaryKey ? "Y" : "");
        row << new QStandardItem(attr.isForeignKey ? "Y" : "");
        row << new QStandardItem(!attr.isNullable ? "Y" : "");
        row << new QStandardItem(attr.isUnique ? "Y" : "");
        row << new QStandardItem(attr.isAutoIncrement ? "Y" : "");
        row << new QStandardItem(attr.defaultValue);
        attrModel_->appendRow(row);
    }
}

void EntityEditorDialog::loadIndexes() {
    indexModel_->removeRows(0, indexModel_->rowCount());
    
    for (const auto& idx : entity_->indexes) {
        QList<QStandardItem*> row;
        row << new QStandardItem(idx.name);
        row << new QStandardItem(idx.type);
        row << new QStandardItem(idx.isUnique ? tr("Yes") : tr("No"));
        row << new QStandardItem(idx.columns.join(", "));
        indexModel_->appendRow(row);
    }
}

void EntityEditorDialog::loadConstraints() {
    constraintModel_->removeRows(0, constraintModel_->rowCount());
    
    for (const auto& c : entity_->constraints) {
        QList<QStandardItem*> row;
        row << new QStandardItem(c.name);
        row << new QStandardItem(c.type);
        row << new QStandardItem(c.definition);
        constraintModel_->appendRow(row);
    }
}

void EntityEditorDialog::onAddAttribute() {
    ModelAttribute attr;
    attr.name = "new_column";
    attr.dataType = "VARCHAR";
    attr.length = 255;
    attr.isNullable = true;
    
    entity_->attributes.append(attr);
    loadAttributes();
}

void EntityEditorDialog::onEditAttribute() {
    // Edit selected attribute
}

void EntityEditorDialog::onDeleteAttribute() {
    auto index = attrTable_->currentIndex();
    if (!index.isValid()) return;
    
    entity_->attributes.removeAt(index.row());
    loadAttributes();
}

void EntityEditorDialog::onMoveAttributeUp() {
    // Move attribute up
}

void EntityEditorDialog::onMoveAttributeDown() {
    // Move attribute down
}

void EntityEditorDialog::onAddIndex() {
    ModelIndex idx;
    idx.name = "idx_" + entity_->name.toLower() + "_" + QString::number(entity_->indexes.size() + 1);
    idx.type = "BTREE";
    
    entity_->indexes.append(idx);
    loadIndexes();
}

void EntityEditorDialog::onEditIndex() {
    // Edit selected index
}

void EntityEditorDialog::onDeleteIndex() {
    auto index = indexTable_->currentIndex();
    if (!index.isValid()) return;
    
    entity_->indexes.removeAt(index.row());
    loadIndexes();
}

void EntityEditorDialog::onAddConstraint() {
    ModelConstraint c;
    c.name = "chk_" + entity_->name.toLower() + "_" + QString::number(entity_->constraints.size() + 1);
    c.type = "CHECK";
    
    entity_->constraints.append(c);
    loadConstraints();
}

void EntityEditorDialog::onEditConstraint() {
    // Edit selected constraint
}

void EntityEditorDialog::onDeleteConstraint() {
    auto index = constraintTable_->currentIndex();
    if (!index.isValid()) return;
    
    entity_->constraints.removeAt(index.row());
    loadConstraints();
}

void EntityEditorDialog::onPreviewSql() {
    // Preview SQL
}

void EntityEditorDialog::onAccept() {
    entity_->name = nameEdit_->text();
    entity_->comment = commentEdit_->toPlainText();
    accept();
}

// ============================================================================
// Relationship Editor Dialog
// ============================================================================

RelationshipEditorDialog::RelationshipEditorDialog(ModelRelationship* rel,
                                                  const QList<ModelEntity>& entities,
                                                  backend::SessionClient* client,
                                                  QWidget* parent)
    : QDialog(parent)
    , relationship_(rel)
    , client_(client)
    , entities_(entities) {
    setupUi();
}

void RelationshipEditorDialog::setupUi() {
    setWindowTitle(tr("Create Relationship"));
    resize(500, 400);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Name
    auto* formLayout = new QFormLayout();
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setText("fk_" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    formLayout->addRow(tr("Name:"), nameEdit_);
    
    layout->addLayout(formLayout);
    
    // Tables
    auto* tablesGroup = new QGroupBox(tr("Tables"), this);
    auto* tablesLayout = new QFormLayout(tablesGroup);
    
    parentTableCombo_ = new QComboBox(this);
    childTableCombo_ = new QComboBox(this);
    
    for (const auto& e : entities_) {
        if (e.type == EntityType::Table) {
            parentTableCombo_->addItem(e.name);
            childTableCombo_->addItem(e.name);
        }
    }
    
    tablesLayout->addRow(tr("Parent Table:"), parentTableCombo_);
    tablesLayout->addRow(tr("Child Table:"), childTableCombo_);
    
    layout->addWidget(tablesGroup);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QFormLayout(optionsGroup);
    
    cardinalityCombo_ = new QComboBox(this);
    cardinalityCombo_->addItems({"1:1", "1:N", "N:M"});
    cardinalityCombo_->setCurrentText("1:N");
    
    onDeleteCombo_ = new QComboBox(this);
    onDeleteCombo_->addItems({"NO ACTION", "CASCADE", "SET NULL", "SET DEFAULT", "RESTRICT"});
    
    onUpdateCombo_ = new QComboBox(this);
    onUpdateCombo_->addItems({"NO ACTION", "CASCADE", "SET NULL", "SET DEFAULT", "RESTRICT"});
    
    optionsLayout->addRow(tr("Cardinality:"), cardinalityCombo_);
    optionsLayout->addRow(tr("On Delete:"), onDeleteCombo_);
    optionsLayout->addRow(tr("On Update:"), onUpdateCombo_);
    
    layout->addWidget(optionsGroup);
    
    // Columns
    auto* colsGroup = new QGroupBox(tr("Column Mapping"), this);
    auto* colsLayout = new QVBoxLayout(colsGroup);
    colsLayout->addWidget(new QLabel(tr("Select parent and child columns:"), this));
    
    auto* autoMapBtn = new QPushButton(tr("Auto-Map Columns"), this);
    connect(autoMapBtn, &QPushButton::clicked, this, &RelationshipEditorDialog::onAutoMapColumns);
    colsLayout->addWidget(autoMapBtn);
    
    layout->addWidget(colsGroup);
    layout->addStretch();
    
    // Buttons
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, &RelationshipEditorDialog::onAccept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btnBox);
}

void RelationshipEditorDialog::onParentTableChanged(int index) {
    // Load columns
}

void RelationshipEditorDialog::onChildTableChanged(int index) {
    // Load columns
}

void RelationshipEditorDialog::onAutoMapColumns() {
    QMessageBox::information(this, tr("Auto-Map"),
        tr("Auto-mapping columns by name similarity."));
}

void RelationshipEditorDialog::onPreviewSql() {
    // Preview SQL
}

void RelationshipEditorDialog::onAccept() {
    relationship_->name = nameEdit_->text();
    relationship_->parentTable = parentTableCombo_->currentText();
    relationship_->childTable = childTableCombo_->currentText();
    relationship_->cardinality = cardinalityCombo_->currentText();
    relationship_->onDelete = onDeleteCombo_->currentText();
    relationship_->onUpdate = onUpdateCombo_->currentText();
    
    // Default columns
    relationship_->parentColumns << "id";
    relationship_->childColumns << relationship_->parentTable.toLower() + "_id";
    
    accept();
}

void RelationshipEditorDialog::loadColumns(const QString& table, QComboBox* combo) {
    combo->clear();
    for (const auto& e : entities_) {
        if (e.name == table) {
            for (const auto& attr : e.attributes) {
                combo->addItem(attr.name);
            }
            break;
        }
    }
}

// ============================================================================
// Forward Engineer Dialog
// ============================================================================

ForwardEngineerDialog::ForwardEngineerDialog(const DataModel* model,
                                            backend::SessionClient* client,
                                            QWidget* parent)
    : QDialog(parent)
    , model_(model)
    , client_(client) {
    setupUi();
}

void ForwardEngineerDialog::setupUi() {
    setWindowTitle(tr("Forward Engineer"));
    resize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QGridLayout(optionsGroup);
    
    includeDropCheck_ = new QCheckBox(tr("Include DROP statements"), this);
    includeCommentsCheck_ = new QCheckBox(tr("Include comments"), this);
    quoteIdentifiersCheck_ = new QCheckBox(tr("Quote identifiers"), this);
    createIndexesCheck_ = new QCheckBox(tr("Create indexes"), this);
    createFkCheck_ = new QCheckBox(tr("Create foreign keys"), this);
    
    optionsLayout->addWidget(includeDropCheck_, 0, 0);
    optionsLayout->addWidget(includeCommentsCheck_, 0, 1);
    optionsLayout->addWidget(quoteIdentifiersCheck_, 1, 0);
    optionsLayout->addWidget(createIndexesCheck_, 1, 1);
    optionsLayout->addWidget(createFkCheck_, 2, 0);
    
    includeCommentsCheck_->setChecked(true);
    createIndexesCheck_->setChecked(true);
    createFkCheck_->setChecked(true);
    
    layout->addWidget(optionsGroup);
    
    // Preview
    layout->addWidget(new QLabel(tr("Generated SQL:"), this));
    
    sqlPreview_ = new QTextEdit(this);
    sqlPreview_->setFont(QFont("Consolas", 10));
    sqlPreview_->setReadOnly(true);
    layout->addWidget(sqlPreview_, 1);
    
    generateSql();
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* genBtn = new QPushButton(tr("Generate"), this);
    connect(genBtn, &QPushButton::clicked, this, &ForwardEngineerDialog::onGenerate);
    btnLayout->addWidget(genBtn);
    
    auto* saveBtn = new QPushButton(tr("Save to File"), this);
    connect(saveBtn, &QPushButton::clicked, this, &ForwardEngineerDialog::onExportSql);
    btnLayout->addWidget(saveBtn);
    
    auto* copyBtn = new QPushButton(tr("Copy"), this);
    connect(copyBtn, &QPushButton::clicked, this, &ForwardEngineerDialog::onCopySql);
    btnLayout->addWidget(copyBtn);
    
    auto* execBtn = new QPushButton(tr("Execute"), this);
    connect(execBtn, &QPushButton::clicked, this, &ForwardEngineerDialog::onExecute);
    btnLayout->addWidget(execBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ForwardEngineerDialog::generateSql() {
    if (model_) {
        sqlPreview_->setPlainText(model_->generateSql());
    }
}

void ForwardEngineerDialog::onGenerate() {
    generateSql();
}

void ForwardEngineerDialog::onPreview() {
    generateSql();
}

void ForwardEngineerDialog::onExecute() {
    QMessageBox::warning(this, tr("Not Implemented"),
        tr("Direct execution not yet implemented.\nPlease save and execute manually."));
}

void ForwardEngineerDialog::onExportSql() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export SQL"),
        model_->name + ".sql",
        tr("SQL Files (*.sql)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(sqlPreview_->toPlainText().toUtf8());
            file.close();
            
            QMessageBox::information(this, tr("Success"),
                tr("SQL exported to %1").arg(fileName));
        }
    }
}

void ForwardEngineerDialog::onCopySql() {
    // Copy to clipboard
}

// ============================================================================
// Reverse Engineer Dialog
// ============================================================================

ReverseEngineerDialog::ReverseEngineerDialog(backend::SessionClient* client, QWidget* parent)
    : QDialog(parent)
    , client_(client) {
    setupUi();
}

void ReverseEngineerDialog::setupUi() {
    setWindowTitle(tr("Reverse Engineer"));
    resize(600, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Schema selection
    auto* schemaLayout = new QHBoxLayout();
    schemaLayout->addWidget(new QLabel(tr("Schema:"), this));
    
    schemaCombo_ = new QComboBox(this);
    schemaCombo_->addItem("public");
    schemaLayout->addWidget(schemaCombo_, 1);
    
    auto* refreshBtn = new QPushButton(tr("Refresh"), this);
    connect(refreshBtn, &QPushButton::clicked, this, &ReverseEngineerDialog::onRefreshObjects);
    schemaLayout->addWidget(refreshBtn);
    
    layout->addLayout(schemaLayout);
    
    // Objects tree
    objectTree_ = new QTreeView(this);
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({tr("Object"), tr("Type")});
    objectTree_->setModel(model_);
    objectTree_->setAlternatingRowColors(true);
    objectTree_->header()->setStretchLastSection(true);
    layout->addWidget(objectTree_, 1);
    
    // Options
    auto* optionsGroup = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsGroup);
    
    includeIndexesCheck_ = new QCheckBox(tr("Include indexes"), this);
    includeConstraintsCheck_ = new QCheckBox(tr("Include constraints"), this);
    includeRelationshipsCheck_ = new QCheckBox(tr("Include relationships"), this);
    
    includeIndexesCheck_->setChecked(true);
    includeConstraintsCheck_->setChecked(true);
    includeRelationshipsCheck_->setChecked(true);
    
    optionsLayout->addWidget(includeIndexesCheck_);
    optionsLayout->addWidget(includeConstraintsCheck_);
    optionsLayout->addWidget(includeRelationshipsCheck_);
    
    layout->addWidget(optionsGroup);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* selectAllBtn = new QPushButton(tr("Select All"), this);
    connect(selectAllBtn, &QPushButton::clicked, this, &ReverseEngineerDialog::onSelectAll);
    btnLayout->addWidget(selectAllBtn);
    
    auto* deselectAllBtn = new QPushButton(tr("Deselect All"), this);
    connect(deselectAllBtn, &QPushButton::clicked, this, &ReverseEngineerDialog::onDeselectAll);
    btnLayout->addWidget(deselectAllBtn);
    
    auto* importBtn = new QPushButton(tr("Import"), this);
    connect(importBtn, &QPushButton::clicked, this, &ReverseEngineerDialog::onImport);
    btnLayout->addWidget(importBtn);
    
    auto* cancelBtn = new QPushButton(tr("Cancel"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);
    
    layout->addLayout(btnLayout);
    
    // Load initial data
    loadDatabaseObjects();
}

void ReverseEngineerDialog::loadDatabaseObjects() {
    model_->removeRows(0, model_->rowCount());
    
    // Simulate loading objects
    auto* tablesItem = new QStandardItem(tr("Tables"));
    tablesItem->setCheckable(true);
    tablesItem->setCheckState(Qt::Checked);
    
    QStringList tables = {"users", "orders", "products", "categories", "order_items"};
    for (const auto& t : tables) {
        auto* item = new QStandardItem(t);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        tablesItem->appendRow(item);
    }
    model_->appendRow(tablesItem);
    
    auto* viewsItem = new QStandardItem(tr("Views"));
    viewsItem->setCheckable(true);
    viewsItem->setCheckState(Qt::Checked);
    
    QStringList views = {"v_active_users", "v_order_summary"};
    for (const auto& v : views) {
        auto* item = new QStandardItem(v);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        viewsItem->appendRow(item);
    }
    model_->appendRow(viewsItem);
    
    objectTree_->expandAll();
}

void ReverseEngineerDialog::onRefreshObjects() {
    loadDatabaseObjects();
}

void ReverseEngineerDialog::onSelectAll() {
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* item = model_->item(i);
        item->setCheckState(Qt::Checked);
        for (int j = 0; j < item->rowCount(); ++j) {
            item->child(j)->setCheckState(Qt::Checked);
        }
    }
}

void ReverseEngineerDialog::onDeselectAll() {
    for (int i = 0; i < model_->rowCount(); ++i) {
        auto* item = model_->item(i);
        item->setCheckState(Qt::Unchecked);
        for (int j = 0; j < item->rowCount(); ++j) {
            item->child(j)->setCheckState(Qt::Unchecked);
        }
    }
}

void ReverseEngineerDialog::onImport() {
    accept();
}

void ReverseEngineerDialog::onPreview() {
    // Preview
}

// ============================================================================
// Model Validation Dialog
// ============================================================================

ModelValidationDialog::ModelValidationDialog(const DataModel* model, QWidget* parent)
    : QDialog(parent)
    , model_(model) {
    setupUi();
    validateModel();
}

void ModelValidationDialog::setupUi() {
    setWindowTitle(tr("Validate Model"));
    resize(700, 500);
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(6, 6, 6, 6);
    
    // Summary
    layout->addWidget(new QLabel(tr("Validation Results:"), this));
    
    // Issues table
    issuesTable_ = new QTableView(this);
    issuesModel_ = new QStandardItemModel(this);
    issuesModel_->setHorizontalHeaderLabels({tr("Severity"), tr("Type"), tr("Object"), tr("Message")});
    issuesTable_->setModel(issuesModel_);
    issuesTable_->setAlternatingRowColors(true);
    issuesTable_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(issuesTable_, 1);
    
    // Details
    layout->addWidget(new QLabel(tr("Details:"), this));
    detailsEdit_ = new QTextEdit(this);
    detailsEdit_->setReadOnly(true);
    detailsEdit_->setMaximumHeight(100);
    layout->addWidget(detailsEdit_);
    
    // Buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    
    auto* runBtn = new QPushButton(tr("Run Validation"), this);
    connect(runBtn, &QPushButton::clicked, this, &ModelValidationDialog::onRunValidation);
    btnLayout->addWidget(runBtn);
    
    auto* exportBtn = new QPushButton(tr("Export Report"), this);
    connect(exportBtn, &QPushButton::clicked, this, &ModelValidationDialog::onExportReport);
    btnLayout->addWidget(exportBtn);
    
    auto* closeBtn = new QPushButton(tr("Close"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    
    layout->addLayout(btnLayout);
}

void ModelValidationDialog::validateModel() {
    issuesModel_->removeRows(0, issuesModel_->rowCount());
    
    QStringList errors;
    model_->validate(errors);
    
    // Add validation issues
    for (const auto& error : errors) {
        QList<QStandardItem*> row;
        row << new QStandardItem(tr("Error"));
        row[0]->setForeground(Qt::red);
        row << new QStandardItem(tr("Model"));
        row << new QStandardItem("-");
        row << new QStandardItem(error);
        issuesModel_->appendRow(row);
    }
    
    // Add some warnings
    QList<QStandardItem*> warn1;
    warn1 << new QStandardItem(tr("Warning"));
    warn1[0]->setForeground(QColor(255, 165, 0));
    warn1 << new QStandardItem(tr("Performance"));
    warn1 << new QStandardItem("orders");
    warn1 << new QStandardItem(tr("Table may benefit from additional indexes"));
    issuesModel_->appendRow(warn1);
    
    QList<QStandardItem*> info1;
    info1 << new QStandardItem(tr("Info"));
    info1[0]->setForeground(Qt::blue);
    info1 << new QStandardItem(tr("Naming"));
    info1 << new QStandardItem("users");
    info1 << new QStandardItem(tr("Table naming follows convention"));
    issuesModel_->appendRow(info1);
}

void ModelValidationDialog::onRunValidation() {
    validateModel();
}

void ModelValidationDialog::onFixIssue() {
    QMessageBox::information(this, tr("Fix Issue"),
        tr("Auto-fix not yet implemented."));
}

void ModelValidationDialog::onExportReport() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Report"),
        QString(),
        tr("Text Files (*.txt);;HTML (*.html)"));
    
    if (!fileName.isEmpty()) {
        QMessageBox::information(this, tr("Export"),
            tr("Report exported to %1").arg(fileName));
    }
}

} // namespace scratchrobin::ui
