#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QDateTime>
#include <QVariant>

namespace scratchrobin::backend {
class SessionClient;
}

namespace scratchrobin::backend {

/**
 * @brief ScratchBird Metadata Provider
 * 
 * Provides database metadata queries using ScratchBird's SHOW commands
 * and catalog views. This is the backend wire-up layer that connects
 * UI components to actual ScratchBird engine queries.
 */

// ============================================================================
// Metadata Structures
// ============================================================================
struct TableMetadata {
    QString name;
    QString schema;
    QString type;  // TABLE, VIEW, SYSTEM TABLE
    int columnCount = 0;
    qint64 rowCount = 0;
    QDateTime created;
    QDateTime modified;
    QString owner;
    QString comment;
    QString ddl;  // Full CREATE TABLE statement from SHOW CREATE TABLE
};

struct ColumnMetadata {
    QString name;
    QString dataType;
    int length = 0;
    int precision = 0;
    int scale = 0;
    bool nullable = true;
    QString defaultValue;
    bool isPrimaryKey = false;
    bool isForeignKey = false;
    bool isUnique = false;
    bool isIndexed = false;
    QString comment;
    int ordinalPosition = 0;
};

struct ViewMetadata {
    QString name;
    QString schema;
    QString definition;  // SELECT statement
    QString ddl;  // Full CREATE VIEW from SHOW CREATE VIEW
    bool isMaterialized = false;
    QString checkOption;
    bool isReadOnly = false;
    QDateTime created;
    QString owner;
    QString comment;
    QStringList columns;
    QStringList dependencies;  // Tables/views this view depends on
};

struct TriggerMetadata {
    QString name;
    QString table;
    QString schema;
    QString timing;  // BEFORE, AFTER, INSTEAD OF
    QString event;   // INSERT, UPDATE, DELETE
    QString definition;
    QString ddl;  // Full CREATE TRIGGER from SHOW TRIGGER
    bool isEnabled = true;
    bool isSystem = false;
    QDateTime created;
    QString owner;
    QString comment;
};

struct IndexMetadata {
    QString name;
    QString table;
    QString schema;
    bool isUnique = false;
    bool isPrimary = false;
    QString type;  // BTREE, HASH, etc.
    QStringList columns;
    QString ddl;
};

struct ConstraintMetadata {
    QString name;
    QString table;
    QString schema;
    QString type;  // PRIMARY KEY, FOREIGN KEY, UNIQUE, CHECK
    QStringList columns;
    QString referencedTable;  // For FK
    QStringList referencedColumns;  // For FK
    QString definition;  // For CHECK constraints
    QString ddl;
};

struct DependencyInfo {
    QString objectName;
    QString objectType;
    QString dependencyName;
    QString dependencyType;
    QString dependencyRelationship;
};

// ============================================================================
// ScratchBird Metadata Provider
// ============================================================================
class ScratchBirdMetadataProvider : public QObject {
    Q_OBJECT

public:
    explicit ScratchBirdMetadataProvider(SessionClient* client, QObject* parent = nullptr);
    
    // Connection
    void setSessionClient(SessionClient* client);
    bool isConnected() const;
    
    // Schema/Database operations
    QStringList schemas() const;
    QString currentSchema() const;
    bool setSchema(const QString& schema);
    
    // Table operations
    QList<TableMetadata> tables(const QString& schema = QString()) const;
    TableMetadata table(const QString& name, const QString& schema = QString()) const;
    QString tableDdl(const QString& name, const QString& schema = QString()) const;
    bool createTable(const TableMetadata& metadata, const QList<ColumnMetadata>& columns);
    bool alterTable(const QString& name, const QString& schema, const QString& alterStatement);
    bool dropTable(const QString& name, const QString& schema = QString(), bool cascade = false);
    bool truncateTable(const QString& name, const QString& schema = QString());
    
    // Column operations
    QList<ColumnMetadata> columns(const QString& table, const QString& schema = QString()) const;
    bool addColumn(const QString& table, const ColumnMetadata& column, const QString& schema = QString());
    bool alterColumn(const QString& table, const QString& column, const QString& newDefinition, 
                     const QString& schema = QString());
    bool dropColumn(const QString& table, const QString& column, const QString& schema = QString());
    
    // View operations
    QList<ViewMetadata> views(const QString& schema = QString()) const;
    ViewMetadata view(const QString& name, const QString& schema = QString()) const;
    QString viewDdl(const QString& name, const QString& schema = QString()) const;
    bool createView(const ViewMetadata& view);
    bool alterView(const QString& name, const QString& schema, const QString& newDefinition);
    bool dropView(const QString& name, const QString& schema = QString(), bool cascade = false);
    bool refreshMaterializedView(const QString& name, const QString& schema = QString());
    
    // Trigger operations
    QList<TriggerMetadata> triggers(const QString& table = QString(), const QString& schema = QString()) const;
    TriggerMetadata trigger(const QString& name, const QString& schema = QString()) const;
    QString triggerDdl(const QString& name, const QString& schema = QString()) const;
    bool createTrigger(const TriggerMetadata& trigger);
    bool alterTrigger(const QString& name, const QString& schema, bool enable);
    bool dropTrigger(const QString& name, const QString& schema = QString());
    
    // Index operations
    QList<IndexMetadata> indexes(const QString& table = QString(), const QString& schema = QString()) const;
    IndexMetadata index(const QString& name, const QString& schema = QString()) const;
    QString indexDdl(const QString& name, const QString& schema = QString()) const;
    bool createIndex(const IndexMetadata& index);
    bool dropIndex(const QString& name, const QString& schema = QString());
    bool rebuildIndex(const QString& name, const QString& schema = QString());
    
    // Constraint operations
    QList<ConstraintMetadata> constraints(const QString& table = QString(), const QString& schema = QString()) const;
    bool addConstraint(const QString& table, const ConstraintMetadata& constraint, const QString& schema = QString());
    bool dropConstraint(const QString& table, const QString& constraint, const QString& schema = QString());
    
    // Dependency tracking
    QList<DependencyInfo> dependencies(const QString& objectName, const QString& objectType, 
                                       const QString& schema = QString()) const;
    QList<DependencyInfo> dependents(const QString& objectName, const QString& objectType,
                                     const QString& schema = QString()) const;
    
    // Statistics
    qint64 tableRowCount(const QString& table, const QString& schema = QString()) const;
    qint64 tableSizeBytes(const QString& table, const QString& schema = QString()) const;
    QHash<QString, QVariant> tableStatistics(const QString& table, const QString& schema = QString()) const;
    
    // DDL Generation (using ScratchBird SHOW commands)
    QString generateCreateDdl(const QString& objectType, const QString& name, 
                              const QString& schema = QString()) const;
    QString generateAlterDdl(const QString& objectType, const QString& name,
                             const QString& schema = QString()) const;
    QString generateDropDdl(const QString& objectType, const QString& name,
                            const QString& schema = QString(), bool cascade = false) const;
    
    // Validation
    bool validateViewDefinition(const QString& sql, QString* errorMessage = nullptr) const;
    bool validateTriggerDefinition(const QString& sql, QString* errorMessage = nullptr) const;
    bool objectExists(const QString& objectType, const QString& name, 
                      const QString& schema = QString()) const;

signals:
    void queryExecuted(const QString& sql, int rowsAffected);
    void errorOccurred(const QString& error);
    void metadataRefreshed();

private:
    // Raw query execution
    QVariant executeQuery(const QString& sql) const;
    QVariant executeShowCommand(const QString& showCommand) const;
    
    // Query builders
    QString buildShowTablesQuery(const QString& schema) const;
    QString buildShowViewsQuery(const QString& schema) const;
    QString buildShowTriggersQuery(const QString& table, const QString& schema) const;
    QString buildShowIndexesQuery(const QString& table, const QString& schema) const;
    QString buildShowColumnsQuery(const QString& table, const QString& schema) const;
    
    // DDL builders
    QString buildCreateTableDdl(const TableMetadata& metadata, const QList<ColumnMetadata>& columns) const;
    QString buildCreateViewDdl(const ViewMetadata& view) const;
    QString buildCreateTriggerDdl(const TriggerMetadata& trigger) const;
    QString buildCreateIndexDdl(const IndexMetadata& index) const;
    
    SessionClient* client_ = nullptr;
    mutable QString currentSchema_;
};

} // namespace scratchrobin::backend
