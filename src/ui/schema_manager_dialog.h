#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QTimer>
#include <QProgressBar>
#include <QSplitter>
#include <QListWidget>

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct SchemaDefinition {
    QString name;
    QString owner;
    QString charset;
    QString collation;
    QString comment;
    QList<QString> permissions;
    QMap<QString, QVariant> options;
};

struct SchemaObject {
    QString name;
    QString type; // "TABLE", "VIEW", "PROCEDURE", "FUNCTION", "INDEX", "TRIGGER"
    QString schema;
    QString definition;
    QDateTime created;
    QDateTime modified;
    QString comment;
    QMap<QString, QVariant> properties;
};

class SchemaManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit SchemaManagerDialog(QWidget* parent = nullptr);
    ~SchemaManagerDialog() override = default;

    // Public interface
    void setDatabaseType(DatabaseType type);
    void setCurrentSchema(const QString& schemaName);
    void refreshSchemaList();

signals:
    void schemaCreated(const QString& schemaName);
    void schemaAltered(const QString& schemaName);
    void schemaDropped(const QString& schemaName);
    void objectCreated(const SchemaObject& object);
    void objectAltered(const SchemaObject& object);
    void objectDropped(const SchemaObject& object);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Schema management
    void onCreateSchema();
    void onEditSchema();
    void onDeleteSchema();
    void onRefreshSchemas();

    // Object management
    void onCreateTable();
    void onCreateView();
    void onCreateProcedure();
    void onCreateFunction();
    void onCreateIndex();
    void onCreateTrigger();

    // Tree navigation
    void onSchemaSelected(QTreeWidgetItem* item);
    void onObjectSelected(QTreeWidgetItem* item);
    void onObjectDoubleClicked(QTreeWidgetItem* item);

    // Context menu
    void onContextMenuRequested(const QPoint& pos);

    // Actions
    void onRefreshObjects();
    void onExportSchema();
    void onImportSchema();

private:
    void setupUI();
    void setupTreeView();
    void setupToolbar();
    void setupContextMenu();
    void setupPropertyPanel();

    void populateSchemaTree();
    void populateSchemaObjects(const QString& schemaName);
    void updatePropertyPanel(const SchemaObject& object);

    void createSchemaDialog(const QString& schemaName = QString());
    void deleteSchemaDialog(const QString& schemaName);
    void showObjectProperties(const SchemaObject& object);

    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QSplitter* mainSplitter_;

    // Left side - Schema tree
    QWidget* leftWidget_;
    QVBoxLayout* leftLayout_;
    QHBoxLayout* toolbarLayout_;

    // Toolbar
    QPushButton* createSchemaButton_;
    QPushButton* refreshButton_;
    QPushButton* exportButton_;
    QPushButton* importButton_;

    // Schema tree
    QTreeWidget* schemaTree_;
    QMenu* contextMenu_;

    // Right side - Properties panel
    QWidget* rightWidget_;
    QVBoxLayout* rightLayout_;
    QLabel* objectNameLabel_;
    QLabel* objectTypeLabel_;
    QLabel* createdLabel_;
    QLabel* modifiedLabel_;
    QTextEdit* definitionEdit_;
    QTextEdit* commentEdit_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    DatabaseType currentDatabaseType_;
    QString currentSchema_;
    QString currentObject_;
    QList<SchemaDefinition> schemas_;
    QList<SchemaObject> objects_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
