#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "Available Qt SQL drivers:";
    QStringList drivers = QSqlDatabase::drivers();
    for (const QString& driver : drivers) {
        qDebug() << " - " << driver;
    }

    // Check specifically for PostgreSQL driver
    if (QSqlDatabase::isDriverAvailable("QPSQL")) {
        qDebug() << "PostgreSQL driver (QPSQL) is available!";

        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
        if (db.driver() && db.driver()->hasFeature(QSqlDriver::Transactions)) {
            qDebug() << "PostgreSQL driver supports transactions.";
        }
    } else {
        qDebug() << "PostgreSQL driver (QPSQL) is NOT available.";
    }

    return 0;
}
