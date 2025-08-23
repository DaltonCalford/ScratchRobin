#ifndef SCRATCHROBIN_TABLE_DESIGNER_H
#define SCRATCHROBIN_TABLE_DESIGNER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>

namespace scratchrobin {

class IMetadataManager;
class ISQLExecutor;
class IConnectionManager;

enum class ColumnType {
    TEXT,
    INTEGER,
    BIGINT,
    FLOAT,
    DOUBLE,
    DECIMAL,
    BOOLEAN,
    DATE,
    TIME,
    DATETIME,
    TIMESTAMP,
    BLOB,
    CLOB,
    BINARY,
    UUID,
    JSON,
    XML,
    ARRAY,
    CUSTOM
};

enum class ConstraintType {
    PRIMARY_KEY,
    FOREIGN_KEY,
    UNIQUE,
    CHECK,
    NOT_NULL,
    DEFAULT_VALUE,
    AUTO_INCREMENT
};

enum class IndexType {
    BTREE,
    HASH,
    GIN,
    GIST,
    SPGIST,
    BRIN,
    UNIQUE,
    PARTIAL
};

enum class TableStorageType {
    REGULAR,
    TEMPORARY,
    UNLOGGED,
    INHERITED,
    PARTITIONED
};

struct ColumnDefinition {
    std::string name;
    std::string originalName;
    ColumnType type = ColumnType::TEXT;
    std::string typeName;
    int length = 0;
    int precision = 0;
    int scale = 0;
    bool isNullable = true;
    std::string defaultValue;
    bool isAutoIncrement = false;
    bool isPrimaryKey = false;
    bool isUnique = false;
    std::string comment;
    std::string collation;
    std::unordered_map<std::string, std::string> properties;
};

struct ConstraintDefinition {
    std::string name;
    ConstraintType type = ConstraintType::PRIMARY_KEY;
    std::vector<std::string> columns;
    std::string referenceTable;
    std::vector<std::string> referenceColumns;
    std::string checkExpression;
    std::string onDeleteAction;
    std::string onUpdateAction;
    bool isDeferrable = false;
    bool isInitiallyDeferred = false;
    std::unordered_map<std::string, std::string> properties;
};

struct IndexDefinition {
    std::string name;
    IndexType type = IndexType::BTREE;
    std::vector<std::string> columns;
    std::string expression;
    std::string tablespace;
    bool isUnique = false;
    std::string whereClause;
    std::unordered_map<std::string, std::string> properties;
};

struct TableDefinition {
    std::string name;
    std::string schema;
    std::string database;
    TableStorageType storageType = TableStorageType::REGULAR;
    std::vector<ColumnDefinition> columns;
    std::vector<ConstraintDefinition> constraints;
    std::vector<IndexDefinition> indexes;
    std::string tablespace;
    std::string comment;
    bool hasOids = false;
    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> inheritsFrom;
    std::string partitionBy;
    std::vector<std::string> partitionValues;
};

struct TableDesignOptions {
    bool enableAutoNaming = true;
    bool enableDragAndDrop = true;
    bool enableContextMenus = true;
    bool enableValidation = true;
    bool enablePreview = true;
    bool enableTemplates = true;
    bool enableImport = true;
    bool enableExport = true;
    bool showAdvancedOptions = false;
    bool autoGenerateConstraints = true;
    bool autoGenerateIndexes = true;
    int maxColumns = 100;
    int maxIndexes = 10;
    int maxConstraints = 20;
};

class ITableDesigner {
public:
    virtual ~ITableDesigner() = default;

    virtual void initialize(const TableDesignOptions& options) = 0;
    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;

    virtual bool createTable(const TableDefinition& definition, const std::string& connectionId) = 0;
    virtual bool modifyTable(const std::string& tableName, const TableDefinition& definition,
                           const std::string& connectionId) = 0;
    virtual bool dropTable(const std::string& tableName, const std::string& connectionId, bool cascade = false) = 0;

    virtual TableDefinition getTableDefinition(const std::string& tableName, const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableDataTypes(const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableCollations(const std::string& connectionId) = 0;
    virtual std::vector<std::string> getAvailableTablespaces(const std::string& connectionId) = 0;

    virtual std::string generateDDL(const TableDefinition& definition, const std::string& connectionId) = 0;
    virtual std::vector<std::string> validateTable(const TableDefinition& definition, const std::string& connectionId) = 0;

    virtual TableDesignOptions getOptions() const = 0;
    virtual void updateOptions(const TableDesignOptions& options) = 0;

    using TableCreatedCallback = std::function<void(const std::string&, const std::string&)>;
    using TableModifiedCallback = std::function<void(const std::string&, const std::string&)>;
    using TableDroppedCallback = std::function<void(const std::string&, const std::string&)>;
    using ValidationErrorCallback = std::function<void(const std::vector<std::string>&)>;

    virtual void setTableCreatedCallback(TableCreatedCallback callback) = 0;
    virtual void setTableModifiedCallback(TableModifiedCallback callback) = 0;
    virtual void setTableDroppedCallback(TableDroppedCallback callback) = 0;
    virtual void setValidationErrorCallback(ValidationErrorCallback callback) = 0;

    virtual QWidget* getWidget() = 0;
    virtual QDialog* getDialog() = 0;
    virtual int exec() = 0;
};

class TableDesigner : public QObject, public ITableDesigner {
    Q_OBJECT

public:
    TableDesigner(QObject* parent = nullptr);
    ~TableDesigner() override;

    // ITableDesigner implementation
    void initialize(const TableDesignOptions& options) override;
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;

    bool createTable(const TableDefinition& definition, const std::string& connectionId) override;
    bool modifyTable(const std::string& tableName, const TableDefinition& definition,
                   const std::string& connectionId) override;
    bool dropTable(const std::string& tableName, const std::string& connectionId, bool cascade = false) override;

    TableDefinition getTableDefinition(const std::string& tableName, const std::string& connectionId) override;
    std::vector<std::string> getAvailableDataTypes(const std::string& connectionId) override;
    std::vector<std::string> getAvailableCollations(const std::string& connectionId) override;
    std::vector<std::string> getAvailableTablespaces(const std::string& connectionId) override;

    std::string generateDDL(const TableDefinition& definition, const std::string& connectionId) override;
    std::vector<std::string> validateTable(const TableDefinition& definition, const std::string& connectionId) override;

    TableDesignOptions getOptions() const override;
    void updateOptions(const TableDesignOptions& options) override;

    void setTableCreatedCallback(TableCreatedCallback callback) override;
    void setTableModifiedCallback(TableModifiedCallback callback) override;
    void setTableDroppedCallback(TableDroppedCallback callback) override;
    void setValidationErrorCallback(ValidationErrorCallback callback) override;

    QWidget* getWidget() override;
    QDialog* getDialog() override;
    int exec() override;

private slots:
    void onAddColumnClicked();
    void onRemoveColumnClicked();
    void onMoveColumnUpClicked();
    void onMoveColumnDownClicked();
    void onAddConstraintClicked();
    void onRemoveConstraintClicked();
    void onAddIndexClicked();
    void onRemoveIndexClicked();
    void onPreviewClicked();
    void onValidateClicked();
    void onCreateClicked();
    void onCancelClicked();
    void onColumnSelectionChanged();
    void onConstraintSelectionChanged();
    void onIndexSelectionChanged();
    void onTableNameChanged(const QString& text);
    void onColumnTypeChanged(int index);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Configuration and state
    TableDesignOptions options_;

    // Callbacks
    TableCreatedCallback tableCreatedCallback_;
    TableModifiedCallback tableModifiedCallback_;
    TableDroppedCallback tableDroppedCallback_;
    ValidationErrorCallback validationErrorCallback_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TABLE_DESIGNER_H
