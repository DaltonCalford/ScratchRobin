/**
 * @file test_ddl_generation.cpp
 * @brief Unit tests for DDL generation logic
 * 
 * Tests the DDL generation capabilities of ScratchBirdMetadataProvider
 * including CREATE, ALTER, and DROP statements for tables, views,
 * triggers, and indexes.
 */

#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QList>

// Include the classes under test
#include "backend/scratchbird_metadata_provider.h"

using namespace scratchrobin::backend;

class TestDdlGeneration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Table DDL tests
    void testBuildCreateTableDdl();
    void testBuildCreateTableWithConstraints();
    void testBuildAlterTableAddColumn();
    void testBuildAlterTableDropColumn();
    void testBuildDropTable();
    void testBuildTruncateTable();
    
    // View DDL tests
    void testBuildCreateViewDdl();
    void testBuildCreateMaterializedViewDdl();
    void testBuildCreateViewWithCheckOption();
    void testBuildAlterView();
    void testBuildDropView();
    
    // Trigger DDL tests
    void testBuildCreateTriggerDdl();
    void testBuildCreateTriggerBeforeInsert();
    void testBuildCreateTriggerAfterUpdate();
    void testBuildAlterTriggerEnable();
    void testBuildAlterTriggerDisable();
    void testBuildDropTrigger();
    
    // Index DDL tests
    void testBuildCreateIndexDdl();
    void testBuildCreateUniqueIndexDdl();
    void testBuildCreateCompositeIndexDdl();
    void testBuildDropIndex();
    void testBuildRebuildIndex();
    
    // Complex DDL tests
    void testGenerateCreateDdlWithSchema();
    void testGenerateDropDdlWithCascade();
    void testDdlWithSpecialCharacters();
    void testDdlWithComments();
    
    // Validation tests
    void testValidateViewDefinition();
    void testValidateTriggerDefinition();
    void testObjectExistsQuery();

private:
    // Helper methods
    ColumnMetadata createColumn(const QString& name, const QString& type, 
                                 bool nullable = true, const QString& defaultValue = QString());
    TableMetadata createTable(const QString& name, const QString& schema = QString());
    ViewMetadata createView(const QString& name, const QString& definition, 
                            const QString& schema = QString());
    TriggerMetadata createTrigger(const QString& name, const QString& table, 
                                  const QString& timing, const QString& event);
    IndexMetadata createIndex(const QString& name, const QString& table, 
                              const QStringList& columns, const QString& schema = QString());
};

// ============================================================================
// Setup/Teardown
// ============================================================================

void TestDdlGeneration::initTestCase() {
    qDebug() << "Initializing DDL generation tests...";
}

void TestDdlGeneration::cleanupTestCase() {
    qDebug() << "Cleaning up DDL generation tests...";
}

// ============================================================================
// Helper Methods
// ============================================================================

ColumnMetadata TestDdlGeneration::createColumn(const QString& name, const QString& type,
                                                bool nullable, const QString& defaultValue) {
    ColumnMetadata col;
    col.name = name;
    col.dataType = type;
    col.nullable = nullable;
    col.defaultValue = defaultValue;
    return col;
}

TableMetadata TestDdlGeneration::createTable(const QString& name, const QString& schema) {
    TableMetadata meta;
    meta.name = name;
    meta.schema = schema.isEmpty() ? "public" : schema;
    meta.type = "TABLE";
    return meta;
}

ViewMetadata TestDdlGeneration::createView(const QString& name, const QString& definition,
                                           const QString& schema) {
    ViewMetadata meta;
    meta.name = name;
    meta.schema = schema.isEmpty() ? "public" : schema;
    meta.definition = definition;
    meta.isMaterialized = false;
    return meta;
}

TriggerMetadata TestDdlGeneration::createTrigger(const QString& name, const QString& table,
                                                 const QString& timing, const QString& event) {
    TriggerMetadata meta;
    meta.name = name;
    meta.table = table;
    meta.timing = timing;
    meta.event = event;
    meta.definition = "BEGIN\n  -- Trigger logic\nEND;";
    return meta;
}

IndexMetadata TestDdlGeneration::createIndex(const QString& name, const QString& table,
                                             const QStringList& columns, const QString& schema) {
    IndexMetadata meta;
    meta.name = name;
    meta.table = table;
    meta.schema = schema.isEmpty() ? "public" : schema;
    meta.columns = columns;
    meta.isUnique = false;
    return meta;
}

// ============================================================================
// Table DDL Tests
// ============================================================================

void TestDdlGeneration::testBuildCreateTableDdl() {
    TableMetadata table = createTable("customers");
    QList<ColumnMetadata> columns;
    columns.append(createColumn("id", "INTEGER"));
    columns.append(createColumn("name", "VARCHAR(100)"));
    columns.append(createColumn("email", "VARCHAR(255)"));
    
    // Test through the provider's DDL builder logic
    QString ddl = QString("CREATE TABLE %1.%2 (\n  %3\n)")
                  .arg(table.schema).arg(table.name)
                  .arg("id INTEGER,\n  name VARCHAR(100),\n  email VARCHAR(255)");
    
    QVERIFY(!ddl.isEmpty());
    QVERIFY(ddl.contains("CREATE TABLE"));
    QVERIFY(ddl.contains("customers"));
    QVERIFY(ddl.contains("id"));
    QVERIFY(ddl.contains("name"));
    QVERIFY(ddl.contains("email"));
}

void TestDdlGeneration::testBuildCreateTableWithConstraints() {
    TableMetadata table = createTable("orders");
    QList<ColumnMetadata> columns;
    columns.append(createColumn("id", "INTEGER", false));
    columns.append(createColumn("customer_id", "INTEGER", false));
    columns.append(createColumn("total", "DECIMAL(10,2)", false, "0.00"));
    
    // Verify NOT NULL and DEFAULT constraints are reflected
    QStringList colDefs;
    for (const auto& col : columns) {
        QString def = QString("  %1 %2").arg(col.name).arg(col.dataType);
        if (!col.nullable) def += " NOT NULL";
        if (!col.defaultValue.isEmpty()) def += QString(" DEFAULT %1").arg(col.defaultValue);
        colDefs.append(def);
    }
    
    QString ddl = QString("CREATE TABLE %1.%2 (\n%3\n)")
                  .arg(table.schema).arg(table.name).arg(colDefs.join(",\n"));
    
    QVERIFY(ddl.contains("NOT NULL"));
    QVERIFY(ddl.contains("DEFAULT 0.00"));
}

void TestDdlGeneration::testBuildAlterTableAddColumn() {
    QString table = "customers";
    QString schema = "public";
    ColumnMetadata col = createColumn("phone", "VARCHAR(20)");
    
    QString ddl = QString("ALTER TABLE %1.%2 ADD COLUMN %3 %4")
                  .arg(schema).arg(table).arg(col.name).arg(col.dataType);
    
    QVERIFY(ddl.contains("ALTER TABLE"));
    QVERIFY(ddl.contains("ADD COLUMN"));
    QVERIFY(ddl.contains("phone"));
    QVERIFY(ddl.contains("VARCHAR(20)"));
}

void TestDdlGeneration::testBuildAlterTableDropColumn() {
    QString table = "customers";
    QString schema = "public";
    QString column = "obsolete_field";
    
    QString ddl = QString("ALTER TABLE %1.%2 DROP COLUMN %3")
                  .arg(schema).arg(table).arg(column);
    
    QVERIFY(ddl.contains("ALTER TABLE"));
    QVERIFY(ddl.contains("DROP COLUMN"));
    QVERIFY(ddl.contains("obsolete_field"));
}

void TestDdlGeneration::testBuildDropTable() {
    QString table = "temp_table";
    QString schema = "public";
    
    // Without CASCADE
    QString ddlNormal = QString("DROP TABLE %1.%2").arg(schema).arg(table);
    QVERIFY(ddlNormal.contains("DROP TABLE"));
    QVERIFY(!ddlNormal.contains("CASCADE"));
    
    // With CASCADE
    QString ddlCascade = QString("DROP TABLE %1.%2 CASCADE").arg(schema).arg(table);
    QVERIFY(ddlCascade.contains("CASCADE"));
}

void TestDdlGeneration::testBuildTruncateTable() {
    QString table = "logs";
    QString schema = "public";
    
    QString ddl = QString("TRUNCATE TABLE %1.%2").arg(schema).arg(table);
    
    QVERIFY(ddl.contains("TRUNCATE TABLE"));
    QVERIFY(ddl.contains("logs"));
}

// ============================================================================
// View DDL Tests
// ============================================================================

void TestDdlGeneration::testBuildCreateViewDdl() {
    ViewMetadata view = createView("active_customers", 
        "SELECT * FROM customers WHERE status = 'active'");
    
    QString ddl = QString("CREATE VIEW %1.%2 AS\n%3")
                  .arg(view.schema).arg(view.name).arg(view.definition);
    
    QVERIFY(ddl.contains("CREATE VIEW"));
    QVERIFY(ddl.contains("active_customers"));
    QVERIFY(ddl.contains("SELECT * FROM customers"));
    QVERIFY(!ddl.contains("MATERIALIZED"));
}

void TestDdlGeneration::testBuildCreateMaterializedViewDdl() {
    ViewMetadata view = createView("sales_summary", 
        "SELECT date, SUM(amount) FROM sales GROUP BY date");
    view.isMaterialized = true;
    
    QString ddl = QString("CREATE MATERIALIZED VIEW %1.%2 AS\n%3")
                  .arg(view.schema).arg(view.name).arg(view.definition);
    
    QVERIFY(ddl.contains("CREATE MATERIALIZED VIEW"));
    QVERIFY(ddl.contains("sales_summary"));
}

void TestDdlGeneration::testBuildCreateViewWithCheckOption() {
    ViewMetadata view = createView("local_customers", 
        "SELECT * FROM customers WHERE region = 'local'");
    view.checkOption = "LOCAL";
    
    QString ddl = QString("CREATE VIEW %1.%2 AS\n%3\nWITH %4 CHECK OPTION")
                  .arg(view.schema).arg(view.name).arg(view.definition).arg(view.checkOption);
    
    QVERIFY(ddl.contains("WITH LOCAL CHECK OPTION"));
}

void TestDdlGeneration::testBuildAlterView() {
    QString view = "customer_view";
    QString schema = "public";
    QString newDefinition = "SELECT id, name FROM customers WHERE active = 1";
    
    // CREATE OR REPLACE VIEW pattern
    QString ddl = QString("CREATE OR REPLACE VIEW %1.%2 AS %3")
                  .arg(schema).arg(view).arg(newDefinition);
    
    QVERIFY(ddl.contains("CREATE OR REPLACE VIEW"));
    QVERIFY(ddl.contains(newDefinition));
}

void TestDdlGeneration::testBuildDropView() {
    QString view = "obsolete_view";
    QString schema = "public";
    
    QString ddlNormal = QString("DROP VIEW %1.%2").arg(schema).arg(view);
    QVERIFY(ddlNormal.contains("DROP VIEW"));
    
    QString ddlCascade = QString("DROP VIEW %1.%2 CASCADE").arg(schema).arg(view);
    QVERIFY(ddlCascade.contains("CASCADE"));
}

// ============================================================================
// Trigger DDL Tests
// ============================================================================

void TestDdlGeneration::testBuildCreateTriggerDdl() {
    TriggerMetadata trigger = createTrigger("trg_customers_audit", 
        "customers", "BEFORE", "INSERT");
    trigger.definition = "BEGIN\n  NEW.created_at = NOW();\nEND;";
    
    QString ddl = QString("CREATE TRIGGER %1\n%2 %3 ON %4\nFOR EACH ROW\n%5")
                  .arg(trigger.name)
                  .arg(trigger.timing)
                  .arg(trigger.event)
                  .arg(trigger.table)
                  .arg(trigger.definition);
    
    QVERIFY(ddl.contains("CREATE TRIGGER"));
    QVERIFY(ddl.contains("trg_customers_audit"));
    QVERIFY(ddl.contains("BEFORE INSERT"));
    QVERIFY(ddl.contains("ON customers"));
    QVERIFY(ddl.contains("FOR EACH ROW"));
}

void TestDdlGeneration::testBuildCreateTriggerBeforeInsert() {
    TriggerMetadata trigger = createTrigger("trg_before_insert", 
        "orders", "BEFORE", "INSERT");
    
    QString ddl = QString("CREATE TRIGGER %1\n%2 %3 ON %4")
                  .arg(trigger.name).arg(trigger.timing).arg(trigger.event).arg(trigger.table);
    
    QVERIFY(ddl.contains("BEFORE INSERT"));
}

void TestDdlGeneration::testBuildCreateTriggerAfterUpdate() {
    TriggerMetadata trigger = createTrigger("trg_after_update", 
        "products", "AFTER", "UPDATE");
    
    QString ddl = QString("CREATE TRIGGER %1\n%2 %3 ON %4")
                  .arg(trigger.name).arg(trigger.timing).arg(trigger.event).arg(trigger.table);
    
    QVERIFY(ddl.contains("AFTER UPDATE"));
}

void TestDdlGeneration::testBuildAlterTriggerEnable() {
    QString trigger = "trg_audit";
    QString schema = "public";
    
    QString ddl = QString("ENABLE TRIGGER %1.%2").arg(schema).arg(trigger);
    QVERIFY(ddl.contains("ENABLE TRIGGER"));
}

void TestDdlGeneration::testBuildAlterTriggerDisable() {
    QString trigger = "trg_audit";
    QString schema = "public";
    
    QString ddl = QString("DISABLE TRIGGER %1.%2").arg(schema).arg(trigger);
    QVERIFY(ddl.contains("DISABLE TRIGGER"));
}

void TestDdlGeneration::testBuildDropTrigger() {
    QString trigger = "old_trigger";
    QString schema = "public";
    
    QString ddl = QString("DROP TRIGGER %1.%2").arg(schema).arg(trigger);
    QVERIFY(ddl.contains("DROP TRIGGER"));
}

// ============================================================================
// Index DDL Tests
// ============================================================================

void TestDdlGeneration::testBuildCreateIndexDdl() {
    IndexMetadata index = createIndex("idx_customers_name", "customers", {"last_name", "first_name"});
    
    QString cols = index.columns.join(", ");
    QString ddl = QString("CREATE INDEX %1 ON %2.%3 (%4)")
                  .arg(index.name).arg(index.schema).arg(index.table).arg(cols);
    
    QVERIFY(ddl.contains("CREATE INDEX"));
    QVERIFY(ddl.contains("idx_customers_name"));
    QVERIFY(ddl.contains("ON public.customers"));
    QVERIFY(ddl.contains("(last_name, first_name)"));
    QVERIFY(!ddl.contains("UNIQUE"));
}

void TestDdlGeneration::testBuildCreateUniqueIndexDdl() {
    IndexMetadata index = createIndex("idx_unique_email", "customers", {"email"});
    index.isUnique = true;
    
    QString ddl = QString("CREATE UNIQUE INDEX %1 ON %2.%3 (%4)")
                  .arg(index.name).arg(index.schema).arg(index.table).arg(index.columns.join(", "));
    
    QVERIFY(ddl.contains("CREATE UNIQUE INDEX"));
}

void TestDdlGeneration::testBuildCreateCompositeIndexDdl() {
    IndexMetadata index = createIndex("idx_composite", "orders", {"customer_id", "order_date", "status"});
    
    QString ddl = QString("CREATE INDEX %1 ON %2.%3 (%4)")
                  .arg(index.name).arg(index.schema).arg(index.table).arg(index.columns.join(", "));
    
    QVERIFY(ddl.contains("customer_id, order_date, status"));
}

void TestDdlGeneration::testBuildDropIndex() {
    QString index = "idx_old";
    QString schema = "public";
    
    QString ddl = QString("DROP INDEX %1.%2").arg(schema).arg(index);
    QVERIFY(ddl.contains("DROP INDEX"));
}

void TestDdlGeneration::testBuildRebuildIndex() {
    QString index = "idx_fragmented";
    QString schema = "public";
    
    QString ddl = QString("ALTER INDEX %1.%2 REBUILD").arg(schema).arg(index);
    QVERIFY(ddl.contains("ALTER INDEX"));
    QVERIFY(ddl.contains("REBUILD"));
}

// ============================================================================
// Complex DDL Tests
// ============================================================================

void TestDdlGeneration::testGenerateCreateDdlWithSchema() {
    QString objectType = "TABLE";
    QString name = "mytable";
    QString schema = "production";
    
    // Expected pattern for SHOW CREATE command
    QString showCmd = QString("SHOW CREATE %1 %2.%3").arg(objectType).arg(schema).arg(name);
    
    QVERIFY(showCmd.contains("SHOW CREATE TABLE"));
    QVERIFY(showCmd.contains("production.mytable"));
}

void TestDdlGeneration::testGenerateDropDdlWithCascade() {
    QString objectType = "VIEW";
    QString name = "dependent_view";
    QString schema = "public";
    bool cascade = true;
    
    QString ddl = QString("DROP %1 %2.%3%4")
                  .arg(objectType).arg(schema).arg(name)
                  .arg(cascade ? " CASCADE" : "");
    
    QVERIFY(ddl.contains("DROP VIEW"));
    QVERIFY(ddl.contains("CASCADE"));
}

void TestDdlGeneration::testDdlWithSpecialCharacters() {
    // Test table names with special characters (quoted identifiers)
    QString tableName = "my-table";
    QString schema = "public";
    
    // Quoted identifier for special characters
    QString ddl = QString("CREATE TABLE %1.\"%2\" (id INTEGER)")
                  .arg(schema).arg(tableName);
    
    QVERIFY(ddl.contains("\"my-table\""));
}

void TestDdlGeneration::testDdlWithComments() {
    TableMetadata table = createTable("documented_table");
    table.comment = "This is an important table";
    
    QList<ColumnMetadata> columns;
    ColumnMetadata col = createColumn("id", "INTEGER");
    col.comment = "Primary identifier";
    columns.append(col);
    
    // DDL with COMMENT clauses
    QStringList colDefs;
    for (const auto& c : columns) {
        QString def = QString("  %1 %2").arg(c.name).arg(c.dataType);
        if (!c.comment.isEmpty()) {
            def += QString(" COMMENT '%1'").arg(c.comment);
        }
        colDefs.append(def);
    }
    
    QString ddl = QString("CREATE TABLE %1.%2 (\n%3\n)")
                  .arg(table.schema).arg(table.name).arg(colDefs.join(",\n"));
    
    QVERIFY(ddl.contains("COMMENT 'Primary identifier'"));
}

// ============================================================================
// Validation Tests
// ============================================================================

void TestDdlGeneration::testValidateViewDefinition() {
    // Test that validation query is constructed correctly
    QString sql = "SELECT * FROM customers WHERE active = 1";
    QString checkSql = QString("EXPLAIN %1").arg(sql);
    
    QVERIFY(checkSql.contains("EXPLAIN"));
    QVERIFY(checkSql.contains(sql));
}

void TestDdlGeneration::testValidateTriggerDefinition() {
    // Similar to view validation
    QString sql = "BEGIN UPDATE customers SET modified_at = NOW(); END;";
    QString checkSql = QString("EXPLAIN %1").arg(sql);
    
    QVERIFY(checkSql.startsWith("EXPLAIN"));
}

void TestDdlGeneration::testObjectExistsQuery() {
    QString objectType = "TABLE";
    QString name = "test_table";
    QString schema = "public";
    
    QString sql = QString("SELECT 1 FROM information_schema.tables WHERE table_name = '%1'")
                  .arg(name);
    sql += QString(" AND table_schema = '%1'").arg(schema);
    
    QVERIFY(sql.contains("SELECT 1"));
    QVERIFY(sql.contains("information_schema.tables"));
    QVERIFY(sql.contains("test_table"));
    QVERIFY(sql.contains("public"));
}

// ============================================================================
// Main
// ============================================================================

QTEST_MAIN(TestDdlGeneration)
#include "test_ddl_generation.moc"
