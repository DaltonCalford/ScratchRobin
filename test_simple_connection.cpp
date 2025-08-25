#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "=== Simple PostgreSQL Connection Test ===";

    // Test direct Qt PostgreSQL connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("scratchrobin_test");
    db.setUserName("scratchuser");
    db.setPassword("scratchpass");

    qDebug() << "Attempting to connect to PostgreSQL...";

    if (db.open()) {
        qDebug() << "✓ Connection successful!";

        // Test a simple query
        QSqlQuery query(db);
        if (query.exec("SELECT version()")) {
            qDebug() << "✓ Query successful!";
            while (query.next()) {
                qDebug() << "PostgreSQL version:" << query.value(0).toString();
            }
        } else {
            qDebug() << "✗ Query failed:" << query.lastError().text();
        }

        // Test getting database list
        if (query.exec("SELECT datname FROM pg_database WHERE datistemplate = false")) {
            qDebug() << "✓ Database list query successful!";
            qDebug() << "Available databases:";
            while (query.next()) {
                qDebug() << " - " << query.value(0).toString();
            }
        } else {
            qDebug() << "✗ Database list query failed:" << query.lastError().text();
        }

        db.close();
        qDebug() << "✓ Connection closed successfully.";
        return 0;
    } else {
        qDebug() << "✗ Connection failed:" << db.lastError().text();
        return 1;
    }
}
