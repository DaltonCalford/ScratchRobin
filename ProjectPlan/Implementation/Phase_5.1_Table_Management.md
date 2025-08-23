# Phase 5.1: Table Management Implementation

## Overview
This phase implements comprehensive table management functionality for ScratchRobin, providing table creation wizards, modification interfaces, validation, data import/export, and visual schema editing capabilities with seamless integration with the existing metadata and execution systems.

## Prerequisites
- ✅ Phase 1.1 (Project Setup) completed
- ✅ Phase 1.2 (Architecture Design) completed
- ✅ Phase 1.3 (Dependency Management) completed
- ✅ Phase 2.1 (Connection Pooling) completed
- ✅ Phase 2.2 (Authentication System) completed
- ✅ Phase 2.3 (SSL/TLS Integration) completed
- ✅ Phase 3.1 (Metadata Loader) completed
- ✅ Phase 3.2 (Object Browser UI) completed
- ✅ Phase 3.3 (Property Viewers) completed
- ✅ Phase 3.4 (Search and Indexing) completed
- ✅ Phase 4.1 (Editor Component) completed
- ✅ Phase 4.2 (Query Execution) completed
- ✅ Phase 4.3 (Query Management) completed
- Metadata system from Phase 3.1
- Query execution system from Phase 4.2

## Implementation Tasks

### Task 5.1.1: Table Creation Wizard

#### 5.1.1.1: Table Designer Component
**Objective**: Create a comprehensive table creation wizard with visual design capabilities and schema validation

**Implementation Steps:**
1. Implement visual table designer with drag-and-drop column management
2. Add data type selection with database-specific type mappings
3. Create constraint definition with primary keys, foreign keys, and check constraints
4. Implement index creation and management within table design
5. Add table properties configuration (storage, partitioning, etc.)
6. Create DDL preview and validation
7. Implement table creation with rollback support

**Code Changes:**

**File: src/table/table_designer.h**
```cpp
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
class ITextEditor;
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

    // UI Components
    QDialog* dialog_;
    QWidget* mainWidget_;
    QVBoxLayout* mainLayout_;
    QHBoxLayout* buttonLayout_;

    // Table properties
    QGroupBox* tableGroup_;
    QFormLayout* tableLayout_;
    QLineEdit* tableNameEdit_;
    QComboBox* schemaCombo_;
    QComboBox* storageTypeCombo_;
    QLineEdit* tablespaceEdit_;
    QTextEdit* commentEdit_;

    // Columns tab
    QTabWidget* tabWidget_;
    QWidget* columnsTab_;
    QVBoxLayout* columnsLayout_;
    QTableWidget* columnsTable_;
    QHBoxLayout* columnButtonsLayout_;
    QPushButton* addColumnButton_;
    QPushButton* removeColumnButton_;
    QPushButton* moveUpButton_;
    QPushButton* moveDownButton_;

    // Constraints tab
    QWidget* constraintsTab_;
    QVBoxLayout* constraintsLayout_;
    QListWidget* constraintsList_;
    QHBoxLayout* constraintButtonsLayout_;
    QPushButton* addConstraintButton_;
    QPushButton* removeConstraintButton_;

    // Indexes tab
    QWidget* indexesTab_;
    QVBoxLayout* indexesLayout_;
    QListWidget* indexesList_;
    QHBoxLayout* indexButtonsLayout_;
    QPushButton* addIndexButton_;
    QPushButton* removeIndexButton_;

    // Preview tab
    QWidget* previewTab_;
    QVBoxLayout* previewLayout_;
    QTextEdit* ddlPreview_;
    QPushButton* refreshPreviewButton_;

    // Dialog buttons
    QPushButton* validateButton_;
    QPushButton* createButton_;
    QPushButton* cancelButton_;

    // Configuration and state
    TableDesignOptions options_;
    TableDefinition currentDefinition_;
    std::string currentConnectionId_;
    bool isModifyMode_ = false;
    std::string originalTableName_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Callbacks
    TableCreatedCallback tableCreatedCallback_;
    TableModifiedCallback tableModifiedCallback_;
    TableDroppedCallback tableDroppedCallback_;
    ValidationErrorCallback validationErrorCallback_;

    // Internal methods
    void setupUI();
    void setupTableProperties();
    void setupColumnsTab();
    void setupConstraintsTab();
    void setupIndexesTab();
    void setupPreviewTab();
    void setupButtons();

    void populateSchemas();
    void populateDataTypes();
    void populateStorageTypes();

    void addColumnToTable(const ColumnDefinition& column);
    void updateColumnInTable(int row, const ColumnDefinition& column);
    void removeColumnFromTable(int row);

    void addConstraintToList(const ConstraintDefinition& constraint);
    void updateConstraintInList(int row, const ConstraintDefinition& constraint);
    void removeConstraintFromList(int row);

    void addIndexToList(const IndexDefinition& index);
    void updateIndexInList(int row, const IndexDefinition& index);
    void removeIndexFromList(int row);

    ColumnDefinition getColumnFromTable(int row) const;
    ConstraintDefinition getConstraintFromList(int row) const;
    IndexDefinition getIndexFromList(int row) const;

    void updateDDLPreview();
    void updateValidation();

    bool validateColumn(const ColumnDefinition& column, std::vector<std::string>& errors);
    bool validateConstraint(const ConstraintDefinition& constraint, std::vector<std::string>& errors);
    bool validateIndex(const IndexDefinition& index, std::vector<std::string>& errors);

    void applyTemplate(const std::string& templateName);
    void loadTableTemplate(const std::string& templateName);
    void saveAsTemplate(const std::string& templateName);

    void showColumnDialog(int row = -1);
    void showConstraintDialog(int row = -1);
    void showIndexDialog(int row = -1);

    void loadTableDefinition(const std::string& tableName);
    void saveTableDefinition();

    std::string generateColumnDDL(const ColumnDefinition& column);
    std::string generateConstraintDDL(const ConstraintDefinition& constraint);
    std::string generateIndexDDL(const IndexDefinition& index);

    void emitTableCreated(const std::string& tableName, const std::string& connectionId);
    void emitTableModified(const std::string& tableName, const std::string& connectionId);
    void emitTableDropped(const std::string& tableName, const std::string& connectionId);
    void emitValidationError(const std::vector<std::string>& errors);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TABLE_DESIGNER_H
```

#### 5.1.1.2: Data Import/Export System
**Objective**: Implement comprehensive data import and export functionality with multiple formats and validation

**Implementation Steps:**
1. Create data export with multiple format support (CSV, JSON, XML, Excel, SQL)
2. Add data import with validation and error handling
3. Implement data mapping and transformation
4. Create bulk data operations with progress tracking
5. Add data preview and sampling capabilities
6. Implement data validation and integrity checks

**Code Changes:**

**File: src/table/data_import_export.h**
```cpp
#ifndef SCRATCHROBIN_DATA_IMPORT_EXPORT_H
#define SCRATCHROBIN_DATA_IMPORT_EXPORT_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <functional>
#include <mutex>
#include <atomic>
#include <QObject>
#include <QJsonObject>
#include <QXmlStreamWriter>
#include <QTextStream>
#include <QStandardItemModel>

namespace scratchrobin {

class IMetadataManager;
class ISQLExecutor;
class IConnectionManager;

enum class DataFormat {
    CSV,
    TSV,
    JSON,
    XML,
    SQL,
    EXCEL,
    FIXED_WIDTH,
    CUSTOM
};

enum class ImportMode {
    INSERT,
    UPDATE,
    UPSERT,
    REPLACE,
    IGNORE_DUPLICATES,
    CUSTOM
};

enum class ExportScope {
    ENTIRE_TABLE,
    QUERY_RESULT,
    SELECTED_ROWS,
    FILTERED_DATA,
    CUSTOM_QUERY
};

enum class DataTypeMapping {
    AUTOMATIC,
    MANUAL,
    CUSTOM
};

struct ColumnMapping {
    std::string sourceColumn;
    std::string targetColumn;
    DataTypeMapping mappingType = DataTypeMapping::AUTOMATIC;
    std::string dataType;
    std::string formatString;
    std::string defaultValue;
    bool isNullable = true;
    bool skipColumn = false;
    std::unordered_map<std::string, std::string> transformationRules;
};

struct ImportOptions {
    DataFormat format = DataFormat::CSV;
    std::string filePath;
    std::string tableName;
    ImportMode mode = ImportMode::INSERT;
    std::vector<ColumnMapping> columnMappings;
    bool hasHeaders = true;
    std::string delimiter = ",";
    std::string quoteCharacter = "\"";
    std::string escapeCharacter = "\\";
    std::string lineEnding = "\n";
    std::string encoding = "UTF-8";
    int skipRows = 0;
    int maxRows = -1; // -1 means all rows
    bool validateData = true;
    bool stopOnError = false;
    int batchSize = 1000;
    bool useTransactions = true;
    bool ignoreDuplicates = false;
    bool updateOnDuplicate = false;
    std::vector<std::string> conflictColumns;
    std::vector<std::string> updateColumns;
    std::unordered_map<std::string, std::string> defaultValues;
    std::unordered_map<std::string, std::string> transformationRules;
};

struct ExportOptions {
    DataFormat format = DataFormat::CSV;
    ExportScope scope = ExportScope::ENTIRE_TABLE;
    std::string tableName;
    std::string query;
    std::string filePath;
    std::vector<std::string> columns;
    std::string whereClause;
    std::string orderByClause;
    int maxRows = -1; // -1 means all rows
    bool includeHeaders = true;
    bool includeRowNumbers = false;
    std::string delimiter = ",";
    std::string quoteCharacter = "\"";
    std::string escapeCharacter = "\\";
    std::string lineEnding = "\n";
    std::string encoding = "UTF-8";
    std::string nullValue = "NULL";
    bool compressOutput = false;
    int batchSize = 10000;
    std::unordered_map<std::string, std::string> formatOptions;
};

struct ImportResult {
    bool success = false;
    int totalRows = 0;
    int importedRows = 0;
    int skippedRows = 0;
    int failedRows = 0;
    int updatedRows = 0;
    std::chrono::milliseconds importTime{0};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::vector<std::pair<int, std::string>> failedRowDetails;
    std::unordered_map<std::string, int> statistics;
};

struct ExportResult {
    bool success = false;
    int totalRows = 0;
    int exportedRows = 0;
    std::size_t fileSize = 0;
    std::chrono::milliseconds exportTime{0};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    std::unordered_map<std::string, int> statistics;
};

struct DataPreview {
    std::vector<std::string> columns;
    std::vector<std::vector<QVariant>> sampleRows;
    int totalRows = 0;
    bool hasMoreRows = false;
    std::unordered_map<std::string, std::string> columnTypes;
    std::vector<std::string> warnings;
};

struct DataValidationRule {
    std::string columnName;
    std::string ruleType; // "required", "unique", "range", "pattern", "custom"
    QVariant minValue;
    QVariant maxValue;
    std::string pattern;
    std::string customExpression;
    std::string errorMessage;
    bool isEnabled = true;
};

struct DataValidationResult {
    bool isValid = true;
    int totalRows = 0;
    int validRows = 0;
    int invalidRows = 0;
    std::vector<std::pair<int, std::string>> validationErrors;
    std::unordered_map<std::string, int> errorCounts;
};

class IDataImportExport {
public:
    virtual ~IDataImportExport() = default;

    virtual void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) = 0;
    virtual void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) = 0;
    virtual void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) = 0;

    virtual ImportResult importData(const ImportOptions& options, const std::string& connectionId) = 0;
    virtual void importDataAsync(const ImportOptions& options, const std::string& connectionId) = 0;

    virtual ExportResult exportData(const ExportOptions& options, const std::string& connectionId) = 0;
    virtual void exportDataAsync(const ExportOptions& options, const std::string& connectionId) = 0;

    virtual DataPreview previewData(const std::string& filePath, DataFormat format, int maxRows = 100) = 0;
    virtual DataValidationResult validateData(const std::string& filePath, DataFormat format,
                                            const std::vector<DataValidationRule>& rules) = 0;

    virtual std::vector<ColumnMapping> generateColumnMappings(const std::string& tableName,
                                                            const std::vector<std::string>& sourceColumns,
                                                            const std::string& connectionId) = 0;

    virtual std::vector<std::string> getSupportedFormats() const = 0;
    virtual std::vector<std::string> getFormatExtensions(DataFormat format) const = 0;
    virtual DataFormat detectFormatFromFile(const std::string& filePath) const = 0;

    virtual std::string generatePreview(const ImportOptions& options, int maxPreviewLines = 10) = 0;
    virtual std::unordered_map<std::string, std::string> validateImportOptions(const ImportOptions& options) = 0;
    virtual std::unordered_map<std::string, std::string> validateExportOptions(const ExportOptions& options) = 0;

    using ImportProgressCallback = std::function<void(int, int, const std::string&)>;
    using ExportProgressCallback = std::function<void(int, int, const std::string&)>;
    using ImportCompletedCallback = std::function<void(const ImportResult&)>;
    using ExportCompletedCallback = std::function<void(const ExportResult&)>;

    virtual void setImportProgressCallback(ImportProgressCallback callback) = 0;
    virtual void setExportProgressCallback(ExportProgressCallback callback) = 0;
    virtual void setImportCompletedCallback(ImportCompletedCallback callback) = 0;
    virtual void setExportCompletedCallback(ExportCompletedCallback callback) = 0;
};

class DataImportExport : public QObject, public IDataImportExport {
    Q_OBJECT

public:
    DataImportExport(QObject* parent = nullptr);
    ~DataImportExport() override;

    // IDataImportExport implementation
    void setMetadataManager(std::shared_ptr<IMetadataManager> metadataManager) override;
    void setSQLExecutor(std::shared_ptr<ISQLExecutor> sqlExecutor) override;
    void setConnectionManager(std::shared_ptr<IConnectionManager> connectionManager) override;

    ImportResult importData(const ImportOptions& options, const std::string& connectionId) override;
    void importDataAsync(const ImportOptions& options, const std::string& connectionId) override;

    ExportResult exportData(const ExportOptions& options, const std::string& connectionId) override;
    void exportDataAsync(const ExportOptions& options, const std::string& connectionId) override;

    DataPreview previewData(const std::string& filePath, DataFormat format, int maxRows = 100) override;
    DataValidationResult validateData(const std::string& filePath, DataFormat format,
                                    const std::vector<DataValidationRule>& rules) override;

    std::vector<ColumnMapping> generateColumnMappings(const std::string& tableName,
                                                    const std::vector<std::string>& sourceColumns,
                                                    const std::string& connectionId) override;

    std::vector<std::string> getSupportedFormats() const override;
    std::vector<std::string> getFormatExtensions(DataFormat format) const override;
    DataFormat detectFormatFromFile(const std::string& filePath) const override;

    std::string generatePreview(const ImportOptions& options, int maxPreviewLines = 10) override;
    std::unordered_map<std::string, std::string> validateImportOptions(const ImportOptions& options) override;
    std::unordered_map<std::string, std::string> validateExportOptions(const ExportOptions& options) override;

    void setImportProgressCallback(ImportProgressCallback callback) override;
    void setExportProgressCallback(ExportProgressCallback callback) override;
    void setImportCompletedCallback(ImportCompletedCallback callback) override;
    void setExportCompletedCallback(ExportCompletedCallback callback) override;

private slots:
    void onImportProgress(int current, int total, const std::string& message);
    void onExportProgress(int current, int total, const std::string& message);
    void onImportCompleted();
    void onExportCompleted();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Core components
    std::shared_ptr<IMetadataManager> metadataManager_;
    std::shared_ptr<ISQLExecutor> sqlExecutor_;
    std::shared_ptr<IConnectionManager> connectionManager_;

    // Callbacks
    ImportProgressCallback importProgressCallback_;
    ExportProgressCallback exportProgressCallback_;
    ImportCompletedCallback importCompletedCallback_;
    ExportCompletedCallback exportCompletedCallback_;

    // Internal methods
    ImportResult performImport(const ImportOptions& options, const std::string& connectionId);
    ExportResult performExport(const ExportOptions& options, const std::string& connectionId);

    ImportResult importFromCSV(const ImportOptions& options, const std::string& connectionId);
    ImportResult importFromJSON(const ImportOptions& options, const std::string& connectionId);
    ImportResult importFromXML(const ImportOptions& options, const std::string& connectionId);
    ImportResult importFromSQL(const ImportOptions& options, const std::string& connectionId);
    ImportResult importFromExcel(const ImportOptions& options, const std::string& connectionId);

    ExportResult exportToCSV(const ExportOptions& options, const std::string& connectionId);
    ExportResult exportToJSON(const ExportOptions& options, const std::string& connectionId);
    ExportResult exportToXML(const ExportOptions& options, const std::string& connectionId);
    ExportResult exportToSQL(const ExportOptions& options, const std::string& connectionId);
    ExportResult exportToExcel(const ExportOptions& options, const std::string& connectionId);

    DataPreview previewCSV(const std::string& filePath, int maxRows);
    DataPreview previewJSON(const std::string& filePath, int maxRows);
    DataPreview previewXML(const std::string& filePath, int maxRows);
    DataPreview previewExcel(const std::string& filePath, int maxRows);

    DataValidationResult validateCSV(const std::string& filePath, const std::vector<DataValidationRule>& rules);
    DataValidationResult validateJSON(const std::string& filePath, const std::vector<DataValidationRule>& rules);
    DataValidationResult validateXML(const std::string& filePath, const std::vector<DataValidationRule>& rules);

    std::vector<std::vector<QVariant>> readCSVData(const std::string& filePath, const ImportOptions& options);
    std::vector<std::vector<QVariant>> readJSONData(const std::string& filePath, const ImportOptions& options);
    std::vector<std::vector<QVariant>> readXMLData(const std::string& filePath, const ImportOptions& options);

    bool writeCSVData(const std::vector<std::vector<QVariant>>& data, const ExportOptions& options);
    bool writeJSONData(const std::vector<std::vector<QVariant>>& data, const ExportOptions& options);
    bool writeXMLData(const std::vector<std::vector<QVariant>>& data, const ExportOptions& options);
    bool writeSQLData(const std::vector<std::vector<QVariant>>& data, const ExportOptions& options);

    QVariant convertValue(const QVariant& value, const std::string& targetType, const ColumnMapping& mapping);
    bool validateValue(const QVariant& value, const DataValidationRule& rule);

    std::string escapeValue(const std::string& value, DataFormat format, const ExportOptions& options);
    std::string quoteValue(const std::string& value, DataFormat format, const ExportOptions& options);

    std::vector<std::string> getTableColumns(const std::string& tableName, const std::string& connectionId);
    std::unordered_map<std::string, std::string> getTableColumnTypes(const std::string& tableName, const std::string& connectionId);

    void generateInsertStatements(const std::vector<std::vector<QVariant>>& data,
                                const ExportOptions& options, QTextStream& stream);
    void generateUpdateStatements(const std::vector<std::vector<QVariant>>& data,
                                const ExportOptions& options, QTextStream& stream);

    bool executeBatchInsert(const std::vector<std::vector<QVariant>>& data,
                          const ImportOptions& options, const std::string& connectionId,
                          ImportResult& result);

    bool executeBatchUpdate(const std::vector<std::vector<QVariant>>& data,
                          const ImportOptions& options, const std::string& connectionId,
                          ImportResult& result);

    void updateImportProgress(int current, int total, const std::string& message);
    void updateExportProgress(int current, int total, const std::string& message);

    void emitImportProgress(int current, int total, const std::string& message);
    void emitExportProgress(int current, int total, const std::string& message);
    void emitImportCompleted(const ImportResult& result);
    void emitExportCompleted(const ExportResult& result);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_IMPORT_EXPORT_H
```

## Summary

This phase 5.1 implementation provides comprehensive table management functionality for ScratchRobin with table creation wizards, modification interfaces, validation, data import/export, and visual schema editing capabilities with seamless integration with the existing metadata and execution systems.

✅ **Table Designer**: Comprehensive table creation wizard with visual design capabilities and schema validation
✅ **Data Import/Export**: Multi-format data import/export with validation and error handling
✅ **Column Management**: Advanced column definition with data types, constraints, and properties
✅ **Constraint Management**: Primary keys, foreign keys, unique constraints, and check constraints
✅ **Index Management**: Index creation and management within table design
✅ **DDL Generation**: Automatic DDL generation with preview and validation
✅ **Schema Validation**: Real-time validation and error reporting
✅ **Template Support**: Table templates and reusable design patterns
✅ **Visual Design**: Drag-and-drop interface for intuitive table design

The table management system provides enterprise-grade functionality with comprehensive table design, data manipulation, and schema management capabilities comparable to commercial database management tools.

**Next Phase**: Phase 5.2 - Index Management Implementation
