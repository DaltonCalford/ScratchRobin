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
#include <QTableWidget>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QSettings>
#include <QMap>
#include <QVariant>
#include <QTimer>
#include <QProgressBar>
#include <QScrollArea>
#include <QSplitter>
#include <QAction>
#include <QMenu>

#include "database/database_driver_manager.h"

namespace scratchrobin {

struct ColumnDefinition {
    QString name;
    QString dataType;
    int length = 0;
    int precision = 0;
    int scale = 0;
    bool isNullable = true;
    QString defaultValue;
    bool isPrimaryKey = false;
    bool isUnique = false;
    bool isAutoIncrement = false;
    QString comment;
    QMap<QString, QVariant> additionalProperties;
};

struct IndexDefinition {
    QString name;
    QString type; // "PRIMARY", "UNIQUE", "INDEX", "FULLTEXT", etc.
    QList<QString> columns;
    QString method; // "BTREE", "HASH", etc.
    bool isUnique = false;
};

struct ForeignKeyDefinition {
    QString name;
    QString column;
    QString referencedTable;
    QString referencedColumn;
    QString onDelete; // "CASCADE", "SET NULL", "RESTRICT", etc.
    QString onUpdate;
};

struct ConstraintDefinition {
    QString name;
    QString type; // "PRIMARY KEY", "FOREIGN KEY", "UNIQUE", "CHECK"
    QString expression;
    QList<QString> columns;
};

struct TableDefinition {
    QString name;
    QString schema;
    QString engine; // MySQL specific
    QString charset;
    QString collation;
    QString comment;
    QList<ColumnDefinition> columns;
    QList<IndexDefinition> indexes;
    QList<ForeignKeyDefinition> foreignKeys;
    QList<ConstraintDefinition> constraints;
    QMap<QString, QVariant> tableOptions;
};

class TableDesignerDialog : public QDialog {
    Q_OBJECT

public:
    explicit TableDesignerDialog(QWidget* parent = nullptr);
    ~TableDesignerDialog() override = default;

    // Public interface
    void setTableDefinition(const TableDefinition& definition);
    TableDefinition getTableDefinition() const;
    void setEditMode(bool isEdit);
    void setDatabaseType(DatabaseType type);
    void loadExistingTable(const QString& schema, const QString& tableName);

signals:
    void tableSaved(const TableDefinition& definition);
    void tableCreated(const QString& sql);
    void tableAltered(const QString& sql);

public slots:
    void accept() override;
    void reject() override;

private slots:
    // Column management
    void onAddColumn();
    void onEditColumn();
    void onDeleteColumn();
    void onMoveColumnUp();
    void onMoveColumnDown();
    void onColumnSelectionChanged();

    // Index management
    void onAddIndex();
    void onEditIndex();
    void onDeleteIndex();

    // Constraint management
    void onAddConstraint();
    void onEditConstraint();
    void onDeleteConstraint();

    // Foreign key management
    void onAddForeignKey();
    void onEditForeignKey();
    void onDeleteForeignKey();

    // Table properties
    void onTableNameChanged(const QString& name);
    void onEngineChanged(int index);
    void onCharsetChanged(int index);

    // Actions
    void onGenerateSQL();
    void onPreviewSQL();
    void onValidateTable();
    void onImportColumns();
    void onExportColumns();

private:
    void setupUI();
    void setupBasicTab();
    void setupColumnsTab();
    void setupIndexesTab();
    void setupConstraintsTab();
    void setupForeignKeysTab();
    void setupAdvancedTab();
    void setupSQLTab();

    void setupColumnTable();
    void setupIndexTable();
    void setupConstraintTable();
    void setupForeignKeyTable();

    void updateColumnTable();
    void updateIndexTable();
    void updateConstraintTable();
    void updateForeignKeyTable();

    void populateDataTypes();
    void populateEngines();
    void populateCharsets();
    void populateCollations();

    bool validateTable();
    QString generateCreateSQL() const;
    QString generateAlterSQL() const;
    QString generateDropSQL() const;

    void loadColumnToDialog(int row);
    void saveColumnFromDialog();
    void clearColumnDialog();

    void updateButtonStates();

    // UI Components
    QVBoxLayout* mainLayout_;
    QTabWidget* tabWidget_;

    // Basic tab
    QWidget* basicTab_;
    QFormLayout* basicLayout_;
    QLineEdit* tableNameEdit_;
    QLineEdit* schemaEdit_;
    QComboBox* engineCombo_;
    QComboBox* charsetCombo_;
    QComboBox* collationCombo_;
    QTextEdit* commentEdit_;

    // Columns tab
    QWidget* columnsTab_;
    QVBoxLayout* columnsLayout_;
    QTableWidget* columnsTable_;
    QHBoxLayout* columnsButtonLayout_;
    QPushButton* addColumnButton_;
    QPushButton* editColumnButton_;
    QPushButton* deleteColumnButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;

    // Column edit dialog (embedded)
    QGroupBox* columnGroup_;
    QFormLayout* columnLayout_;
    QLineEdit* columnNameEdit_;
    QComboBox* dataTypeCombo_;
    QSpinBox* lengthSpin_;
    QSpinBox* precisionSpin_;
    QSpinBox* scaleSpin_;
    QCheckBox* nullableCheck_;
    QLineEdit* defaultValueEdit_;
    QCheckBox* primaryKeyCheck_;
    QCheckBox* uniqueCheck_;
    QCheckBox* autoIncrementCheck_;
    QTextEdit* columnCommentEdit_;

    // Indexes tab
    QWidget* indexesTab_;
    QVBoxLayout* indexesLayout_;
    QTableWidget* indexesTable_;
    QHBoxLayout* indexesButtonLayout_;
    QPushButton* addIndexButton_;
    QPushButton* editIndexButton_;
    QPushButton* deleteIndexButton_;

    // Constraints tab
    QWidget* constraintsTab_;
    QVBoxLayout* constraintsLayout_;
    QTableWidget* constraintsTable_;
    QHBoxLayout* constraintsButtonLayout_;
    QPushButton* addConstraintButton_;
    QPushButton* editConstraintButton_;
    QPushButton* deleteConstraintButton_;

    // Foreign Keys tab
    QWidget* foreignKeysTab_;
    QVBoxLayout* foreignKeysLayout_;
    QTableWidget* foreignKeysTable_;
    QHBoxLayout* foreignKeysButtonLayout_;
    QPushButton* addForeignKeyButton_;
    QPushButton* editForeignKeyButton_;
    QPushButton* deleteForeignKeyButton_;

    // Advanced tab
    QWidget* advancedTab_;
    QVBoxLayout* advancedLayout_;
    QGroupBox* optionsGroup_;
    QFormLayout* optionsLayout_;

    // SQL tab
    QWidget* sqlTab_;
    QVBoxLayout* sqlLayout_;
    QTextEdit* sqlPreviewEdit_;
    QPushButton* generateSqlButton_;
    QPushButton* validateButton_;

    // Dialog buttons
    QDialogButtonBox* dialogButtons_;

    // Current state
    TableDefinition currentDefinition_;
    DatabaseType currentDatabaseType_;
    bool isEditMode_;
    QString originalTableName_;
    QString originalSchema_;

    // Database driver manager
    DatabaseDriverManager* driverManager_;
};

} // namespace scratchrobin
