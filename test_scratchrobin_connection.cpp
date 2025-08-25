#include <QCoreApplication>
#include <QDebug>
#include <vector>
#include "database/database_driver_manager.h"
#include "types/database_types.h"

using namespace scratchrobin;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Initialize the database driver manager
    DatabaseDriverManager& manager = DatabaseDriverManager::instance();
    manager.initializeDrivers();

    qDebug() << "=== ScratchRobin Database Connection Test ===";

    // Check available drivers
    qDebug() << "Available database types:";

    // Test specific database types
    std::vector<DatabaseType> testTypes = {
        DatabaseType::POSTGRESQL,
        DatabaseType::MYSQL,
        DatabaseType::MARIADB,
        DatabaseType::SQLITE,
        DatabaseType::MSSQL
    };

    for (auto type : testTypes) {
        DatabaseDriver driver = manager.getDriver(type);
        if (!driver.name.isEmpty()) {
            qDebug() << " - " << driver.name << "(" << driver.displayName << "):" << (manager.isDriverAvailable(type) ? "Available" : "Not Available");
        }
    }

    // Test PostgreSQL connection
    qDebug() << "\n=== Testing PostgreSQL Connection ===";
    DatabaseConnectionConfig config;
    config.databaseType = DatabaseType::POSTGRESQL;
    config.host = "localhost";
    config.port = 5432;
    config.database = "scratchrobin_test";
    config.username = "scratchuser";
    config.password = "scratchpass";
    config.timeout = 30;

    QString errorMessage;
    bool success = manager.testConnection(config, errorMessage);

    if (success) {
        qDebug() << "✓ PostgreSQL connection test PASSED";
        qDebug() << "  Connection details: localhost:5432/scratchrobin_test as scratchuser";
    } else {
        qDebug() << "✗ PostgreSQL connection test FAILED";
        qDebug() << "  Error:" << errorMessage;
    }

    return success ? 0 : 1;
}
