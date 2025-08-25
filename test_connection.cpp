#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Set up database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("scratchrobin_test");
    db.setUserName("scratchuser");
    db.setPassword("scratchpass");

    qDebug() << "Attempting to connect to PostgreSQL...";

    if (db.open()) {
        qDebug() << "Connection successful!";

        // Test a simple query
        QSqlQuery query(db);
        if (query.exec("SELECT version()")) {
            qDebug() << "Query successful!";
            while (query.next()) {
                qDebug() << "PostgreSQL version:" << query.value(0).toString();
            }
        } else {
            qDebug() << "Query failed:" << query.lastError().text();
        }

        db.close();
        qDebug() << "Connection closed.";
    } else {
        qDebug() << "Connection failed:" << db.lastError().text();
    }

    return 0;
}
