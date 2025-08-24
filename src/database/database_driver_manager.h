#pragma once

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QStringList>
#include <memory>
#include <functional>
#include "types/database_types.h"

namespace scratchrobin {

// Use the existing DatabaseType from the types namespace
using scratchrobin::DatabaseType;

enum class DriverStatus {
    Available,
    NotAvailable,
    NeedsInstallation,
    Loading,
    Error
};

struct DatabaseDriver {
    QString name;
    QString displayName;
    QString description;
    QString driverName;
    QStringList connectionParameters;
    bool requiresAdditionalSetup = false;
    QString setupInstructions;
    QStringList requiredLibraries;
    DriverStatus status = DriverStatus::NotAvailable;

    DatabaseDriver() = default;
    DatabaseDriver(const QString& name, const QString& displayName, const QString& driverName)
        : name(name), displayName(displayName), driverName(driverName) {}
};

struct ConnectionParameter {
    QString name;
    QString displayName;
    QString description;
    QString dataType; // "string", "int", "bool", "password", "file", "port"
    QVariant defaultValue;
    bool required = false;
    bool sensitive = false; // For passwords
    QString validationPattern;
    QString placeholder;

    ConnectionParameter() = default;
    ConnectionParameter(const QString& name, const QString& displayName, const QString& dataType)
        : name(name), displayName(displayName), dataType(dataType) {}
};

struct DatabaseConnectionConfig {
    DatabaseType databaseType;
    QString host;
    int port = 0;
    QString database;
    QString username;
    QString password;
    QString connectionName;
    QMap<QString, QVariant> additionalParameters;
    bool savePassword = false;
    bool autoConnect = false;
    int timeout = 30; // seconds
    QString sslMode = "prefer"; // none, prefer, require, verify-ca, verify-full
    QString charset = "UTF-8";
    int maxConnections = 10;

    bool operator==(const DatabaseConnectionConfig& other) const {
        return connectionName == other.connectionName &&
               databaseType == other.databaseType &&
               host == other.host &&
               port == other.port &&
               database == other.database &&
               username == other.username;
    }
};

class DatabaseDriverManager : public QObject {
    Q_OBJECT

public:
    static DatabaseDriverManager& instance();
    ~DatabaseDriverManager() override = default;

    // Driver management
    void initializeDrivers();
    void scanAvailableDrivers();
    QList<DatabaseDriver> getAvailableDrivers() const;
    QList<DatabaseDriver> getAllDrivers() const;
    DatabaseDriver getDriver(DatabaseType type) const;
    DatabaseDriver getDriver(const QString& name) const;
    bool isDriverAvailable(DatabaseType type) const;
    bool isDriverAvailable(const QString& driverName) const;

    // Connection parameter management
    QList<ConnectionParameter> getConnectionParameters(DatabaseType type) const;
    QList<ConnectionParameter> getConnectionParameters(const QString& driverName) const;
    bool validateConnectionParameters(DatabaseType type, const QMap<QString, QVariant>& parameters) const;

    // Connection testing
    bool testConnection(const DatabaseConnectionConfig& config, QString& errorMessage);
    QString generateConnectionString(const DatabaseConnectionConfig& config) const;

    // Driver installation helpers
    QString getDriverInstallationInstructions(DatabaseType type) const;
    QStringList getRequiredLibraries(DatabaseType type) const;
    bool checkDriverDependencies(DatabaseType type);

    // Utility functions
    QString databaseTypeToString(DatabaseType type) const;
    DatabaseType stringToDatabaseType(const QString& str) const;
    QString getDefaultPort(DatabaseType type) const;
    QStringList getDatabaseTypeList() const;

signals:
    void driverStatusChanged(DatabaseType type, DriverStatus status);
    void driverScanCompleted();
    void connectionTestCompleted(bool success, const QString& message);

private:
    DatabaseDriverManager(QObject* parent = nullptr);
    DatabaseDriverManager(const DatabaseDriverManager&) = delete;
    DatabaseDriverManager& operator=(const DatabaseDriverManager&) = delete;

    void setupPostgreSQLDriver();
    void setupMySQLDriver();
    void setupMariaDBDriver();
    void setupMSSQLDriver();
    void setupODBCDriver();
    void setupFirebirdSQLDriver();
    void setupSQLiteDriver();
    void setupOracleDriver();
    void setupSQLServerDriver();
    void setupDB2Driver();

    void checkPostgreSQLAvailability();
    void checkMySQLAvailability();
    void checkMariaDBAvailability();
    void checkMSSQLAvailability();
    void checkODBCAAvailability();
    void checkFirebirdSQLAvailability();
    void checkSQLiteAvailability();
    void checkOracleAvailability();
    void checkSQLServerAvailability();
    void checkDB2Availability();

    QMap<DatabaseType, DatabaseDriver> drivers_;
    QMap<DatabaseType, QList<ConnectionParameter>> connectionParameters_;

    static std::unique_ptr<DatabaseDriverManager> instance_;
};

} // namespace scratchrobin
