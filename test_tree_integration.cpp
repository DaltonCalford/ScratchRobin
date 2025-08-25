#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "database/postgresql_catalog.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "=== Testing PostgreSQL Tree Integration ===";

    // Test PostgreSQL catalog queries
    qDebug() << "Testing PostgreSQL catalog queries...";

    // Test schemas query
    QString schemasQuery = scratchrobin::PostgreSQLCatalog::getSchemasQuery();
    qDebug() << "Schemas query:" << schemasQuery;

    // Test tables query
    QString tablesQuery = scratchrobin::PostgreSQLCatalog::getTablesQuery("public");
    qDebug() << "Tables query:" << tablesQuery;

    // Test views query
    QString viewsQuery = scratchrobin::PostgreSQLCatalog::getViewsQuery("public");
    qDebug() << "Views query:" << viewsQuery;

    // Test functions query
    QString functionsQuery = scratchrobin::PostgreSQLCatalog::getFunctionsQuery("public");
    qDebug() << "Functions query:" << functionsQuery;

    // Test connection and real queries
    qDebug() << "\n=== Testing Real Database Connection ===";

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "tree_test");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("scratchrobin_test");
    db.setUserName("scratchuser");
    db.setPassword("scratchpass");

    if (!db.open()) {
        qDebug() << "Failed to open database:" << db.lastError().text();
        return 1;
    }

    qDebug() << "✓ Database connection successful";

    // Test schema query
    QSqlQuery query(db);
    if (query.exec(schemasQuery)) {
        qDebug() << "✓ Schemas query executed successfully";
        int count = 0;
        while (query.next() && count < 5) { // Show first 5 results
            QString schemaName = query.value("schema_name").toString();
            qDebug() << "  - Schema:" << schemaName;
            count++;
        }
        if (query.next()) {
            qDebug() << "  ... (more schemas available)";
        }
    } else {
        qDebug() << "✗ Schemas query failed:" << query.lastError().text();
    }

    // Test tables query for public schema
    if (query.exec(tablesQuery)) {
        qDebug() << "✓ Tables query executed successfully";
        int count = 0;
        while (query.next() && count < 5) { // Show first 5 results
            QString tableName = query.value("table_name").toString();
            QString tableType = query.value("table_type").toString();
            qDebug() << "  - Table:" << tableName << "(" << tableType << ")";
            count++;
        }
        if (query.next()) {
            qDebug() << "  ... (more tables available)";
        }
    } else {
        qDebug() << "✗ Tables query failed:" << query.lastError().text();
    }

    db.close();
    QSqlDatabase::removeDatabase("tree_test");

    qDebug() << "✓ Database connection closed";
    qDebug() << "=== Tree Integration Test Complete ===";

    return 0;
}
