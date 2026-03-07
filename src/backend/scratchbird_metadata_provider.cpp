#include "backend/scratchbird_metadata_provider.h"
#include "backend/session_client.h"
#include "backend/query_response.h"

#include <QDebug>

namespace scratchrobin::backend {

// ============================================================================
// Constructor
// ============================================================================

ScratchBirdMetadataProvider::ScratchBirdMetadataProvider(SessionClient* client, QObject* parent)
    : QObject(parent), client_(client) {
}

void ScratchBirdMetadataProvider::setSessionClient(SessionClient* client) {
    client_ = client;
}

bool ScratchBirdMetadataProvider::isConnected() const {
    return client_ != nullptr;
}

// ============================================================================
// Schema/Database Operations
// ============================================================================

QStringList ScratchBirdMetadataProvider::schemas() const {
    if (!isConnected()) return QStringList();
    
    QString sql = "SELECT DISTINCT schema_name FROM information_schema.schemas ORDER BY schema_name";
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QStringList schemas;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (!row.empty()) {
                schemas.append(QString::fromStdString(row[0]));
            }
        }
    }
    return schemas;
}

QString ScratchBirdMetadataProvider::currentSchema() const {
    if (!isConnected()) return QString();
    
    auto response = client_->ExecuteSql(0, "scratchbird", "SELECT CURRENT_SCHEMA()");
    if (response.status.ok && !response.result_set.rows.empty()) {
        return QString::fromStdString(response.result_set.rows[0][0]);
    }
    return currentSchema_;
}

bool ScratchBirdMetadataProvider::setSchema(const QString& schema) {
    if (!isConnected()) return false;
    
    auto response = client_->ExecuteSql(0, "scratchbird", QString("SET SCHEMA '%1'").arg(schema).toStdString());
    if (response.status.ok) {
        currentSchema_ = schema;
        return true;
    }
    return false;
}

// ============================================================================
// Table Operations
// ============================================================================

QList<TableMetadata> ScratchBirdMetadataProvider::tables(const QString& schema) const {
    if (!isConnected()) return QList<TableMetadata>();
    
    QString sql = buildShowTablesQuery(schema);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QList<TableMetadata> tables;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() >= 2) {
                TableMetadata meta;
                meta.name = QString::fromStdString(row[0]);
                meta.schema = QString::fromStdString(row[1]);
                meta.type = row.size() > 2 ? QString::fromStdString(row[2]) : "TABLE";
                tables.append(meta);
            }
        }
    }
    return tables;
}

TableMetadata ScratchBirdMetadataProvider::table(const QString& name, const QString& schema) const {
    TableMetadata meta;
    if (!isConnected()) return meta;
    
    QString sql = QString("SELECT * FROM information_schema.tables WHERE table_name = '%1'")
                  .arg(name);
    if (!schema.isEmpty()) {
        sql += QString(" AND table_schema = '%1'").arg(schema);
    }
    
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    if (response.status.ok && !response.result_set.rows.empty()) {
        const auto& row = response.result_set.rows[0];
        meta.name = name;
        meta.schema = schema.isEmpty() ? currentSchema() : schema;
        if (row.size() > 2) meta.type = QString::fromStdString(row[2]);
    }
    
    meta.ddl = tableDdl(name, schema);
    return meta;
}

QString ScratchBirdMetadataProvider::tableDdl(const QString& name, const QString& schema) const {
    if (!isConnected()) return QString();
    
    QString showCmd = QString("SHOW CREATE TABLE %1%2")
                      .arg(schema.isEmpty() ? "" : schema + ".")
                      .arg(name);
    
    auto response = client_->ExecuteSql(0, "scratchbird", showCmd.toStdString());
    if (response.status.ok && !response.result_set.rows.empty() && response.result_set.rows[0].size() > 1) {
        return QString::fromStdString(response.result_set.rows[0][1]);
    }
    
    return generateCreateDdl("TABLE", name, schema);
}

bool ScratchBirdMetadataProvider::createTable(const TableMetadata& metadata, 
                                               const QList<ColumnMetadata>& columns) {
    if (!isConnected()) return false;
    
    QString ddl = buildCreateTableDdl(metadata, columns);
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    } else {
        emit errorOccurred(QString::fromStdString(response.status.message));
        return false;
    }
}

bool ScratchBirdMetadataProvider::alterTable(const QString& name, const QString& schema, 
                                              const QString& alterStatement) {
    if (!isConnected()) return false;
    
    QString fullTable = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("ALTER TABLE %1 %2").arg(fullTable).arg(alterStatement);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::dropTable(const QString& name, const QString& schema, bool cascade) {
    if (!isConnected()) return false;
    
    QString fullTable = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("DROP TABLE %1%2").arg(fullTable).arg(cascade ? " CASCADE" : "");
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::truncateTable(const QString& name, const QString& schema) {
    if (!isConnected()) return false;
    
    QString fullTable = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("TRUNCATE TABLE %1").arg(fullTable);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

// ============================================================================
// Column Operations
// ============================================================================

QList<ColumnMetadata> ScratchBirdMetadataProvider::columns(const QString& table, const QString& schema) const {
    if (!isConnected()) return QList<ColumnMetadata>();
    
    QString sql = buildShowColumnsQuery(table, schema);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QList<ColumnMetadata> cols;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            ColumnMetadata meta;
            if (row.size() > 0) meta.name = QString::fromStdString(row[0]);
            if (row.size() > 1) meta.dataType = QString::fromStdString(row[1]);
            if (row.size() > 2) meta.nullable = QString::fromStdString(row[2]).toUpper() == "YES";
            if (row.size() > 3) meta.defaultValue = QString::fromStdString(row[3]);
            if (row.size() > 4) meta.isPrimaryKey = QString::fromStdString(row[4]).toUpper() == "PRI";
            cols.append(meta);
        }
    }
    return cols;
}

bool ScratchBirdMetadataProvider::addColumn(const QString& table, const ColumnMetadata& column, 
                                            const QString& schema) {
    QString def = QString("%1 %2%3").arg(column.name).arg(column.dataType)
                  .arg(column.nullable ? "" : " NOT NULL");
    return alterTable(table, schema, QString("ADD COLUMN %1").arg(def));
}

bool ScratchBirdMetadataProvider::alterColumn(const QString& table, const QString& column, 
                                               const QString& newDefinition, const QString& schema) {
    return alterTable(table, schema, QString("ALTER COLUMN %1 SET %2").arg(column).arg(newDefinition));
}

bool ScratchBirdMetadataProvider::dropColumn(const QString& table, const QString& column, 
                                              const QString& schema) {
    return alterTable(table, schema, QString("DROP COLUMN %1").arg(column));
}

// ============================================================================
// View Operations
// ============================================================================

QList<ViewMetadata> ScratchBirdMetadataProvider::views(const QString& schema) const {
    if (!isConnected()) return QList<ViewMetadata>();
    
    QString sql = buildShowViewsQuery(schema);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QList<ViewMetadata> views;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            if (row.size() >= 2) {
                ViewMetadata meta;
                meta.name = QString::fromStdString(row[0]);
                meta.schema = QString::fromStdString(row[1]);
                views.append(meta);
            }
        }
    }
    return views;
}

ViewMetadata ScratchBirdMetadataProvider::view(const QString& name, const QString& schema) const {
    ViewMetadata meta;
    if (!isConnected()) return meta;
    
    QString sql = QString("SELECT * FROM information_schema.views WHERE table_name = '%1'")
                  .arg(name);
    if (!schema.isEmpty()) {
        sql += QString(" AND table_schema = '%1'").arg(schema);
    }
    
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    if (response.status.ok && !response.result_set.rows.empty()) {
        const auto& row = response.result_set.rows[0];
        meta.name = name;
        meta.schema = schema.isEmpty() ? currentSchema() : schema;
        if (row.size() > 2) meta.definition = QString::fromStdString(row[2]);
    }
    
    meta.ddl = viewDdl(name, schema);
    return meta;
}

QString ScratchBirdMetadataProvider::viewDdl(const QString& name, const QString& schema) const {
    if (!isConnected()) return QString();
    
    QString showCmd = QString("SHOW CREATE VIEW %1%2")
                      .arg(schema.isEmpty() ? "" : schema + ".")
                      .arg(name);
    
    auto response = client_->ExecuteSql(0, "scratchbird", showCmd.toStdString());
    if (response.status.ok && !response.result_set.rows.empty() && response.result_set.rows[0].size() > 1) {
        return QString::fromStdString(response.result_set.rows[0][1]);
    }
    return QString();
}

bool ScratchBirdMetadataProvider::createView(const ViewMetadata& view) {
    if (!isConnected()) return false;
    
    QString ddl = buildCreateViewDdl(view);
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::alterView(const QString& name, const QString& schema, 
                                            const QString& newDefinition) {
    if (!isConnected()) return false;
    
    QString fullView = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("CREATE OR REPLACE VIEW %1 AS %2").arg(fullView).arg(newDefinition);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::dropView(const QString& name, const QString& schema, bool cascade) {
    if (!isConnected()) return false;
    
    QString fullView = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("DROP VIEW %1%2").arg(fullView).arg(cascade ? " CASCADE" : "");
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::refreshMaterializedView(const QString& name, const QString& schema) {
    if (!isConnected()) return false;
    
    QString fullView = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("REFRESH MATERIALIZED VIEW %1").arg(fullView);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

// ============================================================================
// Trigger Operations
// ============================================================================

QList<TriggerMetadata> ScratchBirdMetadataProvider::triggers(const QString& table, const QString& schema) const {
    if (!isConnected()) return QList<TriggerMetadata>();
    
    QString sql = buildShowTriggersQuery(table, schema);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QList<TriggerMetadata> triggers;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            TriggerMetadata meta;
            if (row.size() > 0) meta.name = QString::fromStdString(row[0]);
            if (row.size() > 1) meta.table = QString::fromStdString(row[1]);
            if (row.size() > 2) meta.timing = QString::fromStdString(row[2]);
            if (row.size() > 3) meta.event = QString::fromStdString(row[3]);
            if (row.size() > 4) meta.isEnabled = QString::fromStdString(row[4]).toUpper() != "DISABLED";
            triggers.append(meta);
        }
    }
    return triggers;
}

TriggerMetadata ScratchBirdMetadataProvider::trigger(const QString& name, const QString& schema) const {
    TriggerMetadata meta;
    if (!isConnected()) return meta;
    
    meta.name = name;
    meta.schema = schema.isEmpty() ? currentSchema() : schema;
    meta.ddl = triggerDdl(name, schema);
    return meta;
}

QString ScratchBirdMetadataProvider::triggerDdl(const QString& name, const QString& schema) const {
    if (!isConnected()) return QString();
    
    QString showCmd = QString("SHOW CREATE TRIGGER %1%2")
                      .arg(schema.isEmpty() ? "" : schema + ".")
                      .arg(name);
    
    auto response = client_->ExecuteSql(0, "scratchbird", showCmd.toStdString());
    if (response.status.ok && !response.result_set.rows.empty() && response.result_set.rows[0].size() > 1) {
        return QString::fromStdString(response.result_set.rows[0][1]);
    }
    return QString();
}

bool ScratchBirdMetadataProvider::createTrigger(const TriggerMetadata& trigger) {
    if (!isConnected()) return false;
    
    QString ddl = buildCreateTriggerDdl(trigger);
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::alterTrigger(const QString& name, const QString& schema, bool enable) {
    if (!isConnected()) return false;
    
    QString fullTrigger = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("%1 TRIGGER %2").arg(enable ? "ENABLE" : "DISABLE").arg(fullTrigger);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::dropTrigger(const QString& name, const QString& schema) {
    if (!isConnected()) return false;
    
    QString fullTrigger = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("DROP TRIGGER %1").arg(fullTrigger);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

// ============================================================================
// Index Operations
// ============================================================================

QList<IndexMetadata> ScratchBirdMetadataProvider::indexes(const QString& table, const QString& schema) const {
    if (!isConnected()) return QList<IndexMetadata>();
    
    QString sql = buildShowIndexesQuery(table, schema);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    QList<IndexMetadata> indexes;
    if (response.status.ok) {
        for (const auto& row : response.result_set.rows) {
            IndexMetadata meta;
            if (row.size() > 0) meta.name = QString::fromStdString(row[0]);
            if (row.size() > 1) meta.table = QString::fromStdString(row[1]);
            if (row.size() > 2) meta.isUnique = QString::fromStdString(row[2]).toUpper() == "YES";
            indexes.append(meta);
        }
    }
    return indexes;
}

IndexMetadata ScratchBirdMetadataProvider::index(const QString& name, const QString& schema) const {
    IndexMetadata meta;
    meta.name = name;
    meta.schema = schema;
    meta.ddl = indexDdl(name, schema);
    return meta;
}

QString ScratchBirdMetadataProvider::indexDdl(const QString& name, const QString& schema) const {
    if (!isConnected()) return QString();
    
    QString showCmd = QString("SHOW CREATE INDEX %1%2")
                      .arg(schema.isEmpty() ? "" : schema + ".")
                      .arg(name);
    
    auto response = client_->ExecuteSql(0, "scratchbird", showCmd.toStdString());
    if (response.status.ok && !response.result_set.rows.empty() && response.result_set.rows[0].size() > 1) {
        return QString::fromStdString(response.result_set.rows[0][1]);
    }
    return QString();
}

bool ScratchBirdMetadataProvider::createIndex(const IndexMetadata& index) {
    if (!isConnected()) return false;
    
    QString ddl = buildCreateIndexDdl(index);
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::dropIndex(const QString& name, const QString& schema) {
    if (!isConnected()) return false;
    
    QString fullIndex = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("DROP INDEX %1").arg(fullIndex);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

bool ScratchBirdMetadataProvider::rebuildIndex(const QString& name, const QString& schema) {
    if (!isConnected()) return false;
    
    QString fullIndex = schema.isEmpty() ? name : schema + "." + name;
    QString ddl = QString("ALTER INDEX %1 REBUILD").arg(fullIndex);
    
    auto response = client_->ExecuteSql(0, "scratchbird", ddl.toStdString());
    if (response.status.ok) {
        emit queryExecuted(ddl, 0);
        return true;
    }
    emit errorOccurred(QString::fromStdString(response.status.message));
    return false;
}

// ============================================================================
// DDL Generation
// ============================================================================

QString ScratchBirdMetadataProvider::generateCreateDdl(const QString& objectType, 
                                                        const QString& name,
                                                        const QString& schema) const {
    if (!isConnected()) return QString();
    
    QString fullName = schema.isEmpty() ? name : schema + "." + name;
    QString showCmd = QString("SHOW CREATE %1 %2").arg(objectType).arg(fullName);
    
    auto response = client_->ExecuteSql(0, "scratchbird", showCmd.toStdString());
    if (response.status.ok && !response.result_set.rows.empty() && response.result_set.rows[0].size() > 1) {
        return QString::fromStdString(response.result_set.rows[0][1]);
    }
    
    return QString("-- Unable to generate DDL for %1 %2").arg(objectType).arg(fullName);
}

QString ScratchBirdMetadataProvider::generateAlterDdl(const QString& objectType,
                                                       const QString& name,
                                                       const QString& schema) const {
    return QString("-- ALTER %1 %2.%3").arg(objectType).arg(schema).arg(name);
}

QString ScratchBirdMetadataProvider::generateDropDdl(const QString& objectType,
                                                      const QString& name,
                                                      const QString& schema,
                                                      bool cascade) const {
    QString fullName = schema.isEmpty() ? name : schema + "." + name;
    return QString("DROP %1 %2%3").arg(objectType).arg(fullName)
                                  .arg(cascade ? " CASCADE" : "");
}

// ============================================================================
// Query Builders
// ============================================================================

QString ScratchBirdMetadataProvider::buildShowTablesQuery(const QString& schema) const {
    QString sql = "SELECT table_name, table_schema, table_type FROM information_schema.tables";
    if (!schema.isEmpty()) {
        sql += QString(" WHERE table_schema = '%1'").arg(schema);
    }
    sql += " ORDER BY table_name";
    return sql;
}

QString ScratchBirdMetadataProvider::buildShowViewsQuery(const QString& schema) const {
    QString sql = "SELECT table_name, table_schema FROM information_schema.views";
    if (!schema.isEmpty()) {
        sql += QString(" WHERE table_schema = '%1'").arg(schema);
    }
    sql += " ORDER BY table_name";
    return sql;
}

QString ScratchBirdMetadataProvider::buildShowTriggersQuery(const QString& table, 
                                                            const QString& schema) const {
    QString sql = "SELECT trigger_name, event_object_table, action_timing, event_manipulation, status "
                  "FROM information_schema.triggers";
    if (!table.isEmpty()) {
        sql += QString(" WHERE event_object_table = '%1'").arg(table);
    }
    if (!schema.isEmpty()) {
        sql += table.isEmpty() ? " WHERE" : " AND";
        sql += QString(" trigger_schema = '%1'").arg(schema);
    }
    sql += " ORDER BY trigger_name";
    return sql;
}

QString ScratchBirdMetadataProvider::buildShowIndexesQuery(const QString& table, 
                                                           const QString& schema) const {
    QString sql = "SELECT index_name, table_name, is_unique FROM information_schema.statistics";
    if (!table.isEmpty()) {
        sql += QString(" WHERE table_name = '%1'").arg(table);
    }
    if (!schema.isEmpty()) {
        sql += table.isEmpty() ? " WHERE" : " AND";
        sql += QString(" index_schema = '%1'").arg(schema);
    }
    sql += " ORDER BY index_name";
    return sql;
}

QString ScratchBirdMetadataProvider::buildShowColumnsQuery(const QString& table, 
                                                           const QString& schema) const {
    QString fullTable = schema.isEmpty() ? table : schema + "." + table;
    return QString("SHOW COLUMNS FROM %1").arg(fullTable);
}

// ============================================================================
// DDL Builders
// ============================================================================

QString ScratchBirdMetadataProvider::buildCreateTableDdl(const TableMetadata& metadata,
                                                          const QList<ColumnMetadata>& columns) const {
    QStringList colDefs;
    for (const auto& col : columns) {
        QString def = QString("  %1 %2").arg(col.name).arg(col.dataType);
        if (!col.nullable) def += " NOT NULL";
        if (!col.defaultValue.isEmpty()) def += QString(" DEFAULT %1").arg(col.defaultValue);
        if (!col.comment.isEmpty()) def += QString(" COMMENT '%1'").arg(col.comment);
        colDefs.append(def);
    }
    
    return QString("CREATE TABLE %1.%2 (\n%3\n)")
           .arg(metadata.schema).arg(metadata.name)
           .arg(colDefs.join(",\n"));
}

QString ScratchBirdMetadataProvider::buildCreateViewDdl(const ViewMetadata& view) const {
    QString ddl = QString("CREATE %1VIEW %2.%3 AS\n%4")
                  .arg(view.isMaterialized ? "MATERIALIZED " : "")
                  .arg(view.schema).arg(view.name)
                  .arg(view.definition);
    
    if (!view.checkOption.isEmpty()) {
        ddl += QString("\nWITH %1 CHECK OPTION").arg(view.checkOption);
    }
    return ddl;
}

QString ScratchBirdMetadataProvider::buildCreateTriggerDdl(const TriggerMetadata& trigger) const {
    return QString("CREATE TRIGGER %1\n%2 %3 ON %4\nFOR EACH ROW\n%5")
           .arg(trigger.name)
           .arg(trigger.timing)
           .arg(trigger.event)
           .arg(trigger.table)
           .arg(trigger.definition);
}

QString ScratchBirdMetadataProvider::buildCreateIndexDdl(const IndexMetadata& index) const {
    QString unique = index.isUnique ? "UNIQUE " : "";
    QString cols = index.columns.join(", ");
    return QString("CREATE %1INDEX %2 ON %3.%4 (%5)")
           .arg(unique).arg(index.name).arg(index.schema).arg(index.table).arg(cols);
}

// ============================================================================
// Validation
// ============================================================================

bool ScratchBirdMetadataProvider::validateViewDefinition(const QString& sql, QString* errorMessage) const {
    if (!isConnected()) {
        if (errorMessage) *errorMessage = tr("Not connected to database");
        return false;
    }
    
    QString checkSql = QString("EXPLAIN %1").arg(sql);
    auto response = client_->ExecuteSql(0, "scratchbird", checkSql.toStdString());
    
    if (!response.status.ok && errorMessage) {
        *errorMessage = QString::fromStdString(response.status.message);
    }
    return response.status.ok;
}

bool ScratchBirdMetadataProvider::validateTriggerDefinition(const QString& sql, QString* errorMessage) const {
    return validateViewDefinition(sql, errorMessage);
}

bool ScratchBirdMetadataProvider::objectExists(const QString& objectType, const QString& name,
                                                const QString& schema) const {
    if (!isConnected()) return false;
    
    QString sql;
    if (objectType.toUpper() == "TABLE") {
        sql = QString("SELECT 1 FROM information_schema.tables WHERE table_name = '%1'").arg(name);
    } else if (objectType.toUpper() == "VIEW") {
        sql = QString("SELECT 1 FROM information_schema.views WHERE table_name = '%1'").arg(name);
    } else if (objectType.toUpper() == "TRIGGER") {
        sql = QString("SELECT 1 FROM information_schema.triggers WHERE trigger_name = '%1'").arg(name);
    } else {
        return false;
    }
    
    if (!schema.isEmpty()) {
        sql += QString(" AND table_schema = '%1'").arg(schema);
    }
    
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    return response.status.ok && !response.result_set.rows.empty();
}

// ============================================================================
// Statistics
// ============================================================================

qint64 ScratchBirdMetadataProvider::tableRowCount(const QString& table, const QString& schema) const {
    if (!isConnected()) return -1;
    
    QString fullTable = schema.isEmpty() ? table : schema + "." + table;
    auto response = client_->ExecuteSql(0, "scratchbird", QString("SELECT COUNT(*) FROM %1").arg(fullTable).toStdString());
    
    if (response.status.ok && !response.result_set.rows.empty()) {
        return QString::fromStdString(response.result_set.rows[0][0]).toLongLong();
    }
    return -1;
}

qint64 ScratchBirdMetadataProvider::tableSizeBytes(const QString& table, const QString& schema) const {
    if (!isConnected()) return -1;
    
    QString sql = QString("SELECT pg_total_relation_size('%1')")
                  .arg(schema.isEmpty() ? table : schema + "." + table);
    auto response = client_->ExecuteSql(0, "scratchbird", sql.toStdString());
    
    if (response.status.ok && !response.result_set.rows.empty()) {
        return QString::fromStdString(response.result_set.rows[0][0]).toLongLong();
    }
    return -1;
}

QHash<QString, QVariant> ScratchBirdMetadataProvider::tableStatistics(const QString& table, 
                                                                       const QString& schema) const {
    QHash<QString, QVariant> stats;
    stats["row_count"] = tableRowCount(table, schema);
    stats["size_bytes"] = tableSizeBytes(table, schema);
    return stats;
}

} // namespace scratchrobin::backend
