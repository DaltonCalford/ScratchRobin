#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>
#include "database_driver_manager.h"
#include "mssql_features.h"

namespace scratchrobin {

// MSSQL Connection Parameters and Configuration
struct MSSQLConnectionParameters {
    // Basic connection parameters
    QString server;              // Server name or IP
    int port = 1433;            // Port number (default 1433)
    QString database;           // Database name
    QString username;           // Username
    QString password;           // Password

    // Authentication options
    bool useWindowsAuth = false; // Use Windows Authentication (SSPI)
    bool useSQLAuth = true;     // Use SQL Server Authentication

    // Connection options
    bool encrypt = false;       // Encrypt connection
    bool trustServerCert = false; // Trust server certificate
    int timeout = 30;           // Connection timeout in seconds
    int commandTimeout = 0;     // Command timeout in seconds (0 = no limit)

    // Advanced options
    QString applicationName;    // Application name for identification
    QString workstationId;      // Workstation ID
    bool multipleActiveResultSets = true; // MARS support
    QString failoverPartner;    // Failover partner server
    bool useMirroring = false;  // Database mirroring support

    // Pooling options
    bool connectionPooling = true;
    int minPoolSize = 1;
    int maxPoolSize = 100;
    int connectionLifetime = 0;

    // ODBC specific options
    QString odbcDriver = "ODBC Driver 17 for SQL Server";
    QString dsn;                // Data Source Name (optional)
    bool useDSN = false;        // Use DSN instead of connection string

    // Additional connection string parameters
    QMap<QString, QString> additionalParams;

    // Validate the connection parameters
    bool validateParameters(QString& errorMessage) const;

    // Generate connection string
    QString generateConnectionString() const;

    // Get ODBC connection string
    QString generateODBCConnectionString() const;

    // Parse connection string
    static MSSQLConnectionParameters fromConnectionString(const QString& connectionString);
};

// MSSQL Connection Tester
class MSSQLConnectionTester {
public:
    static bool testBasicConnection(const MSSQLConnectionParameters& params, QString& errorMessage);
    static bool testDatabaseAccess(const MSSQLConnectionParameters& params, QString& errorMessage);
    static bool testPermissions(const MSSQLConnectionParameters& params, QString& errorMessage);
    static bool testServerFeatures(const MSSQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage);

    // Advanced testing
    static bool testHighAvailability(const MSSQLConnectionParameters& params, QString& errorMessage);
    static bool testEncryption(const MSSQLConnectionParameters& params, QString& errorMessage);
    static bool testPerformance(const MSSQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage);
};

// MSSQL Connection Pool Manager
class MSSQLConnectionPool {
public:
    static MSSQLConnectionPool& instance();

    bool initializePool(const MSSQLConnectionParameters& params, int poolSize = 10);
    QSqlDatabase getConnection(const QString& connectionName = "");
    void releaseConnection(QSqlDatabase& db);
    void closeAllConnections();

    // Pool statistics
    int getActiveConnections() const;
    int getAvailableConnections() const;
    int getPoolSize() const;

    // Health monitoring
    bool isHealthy() const;
    QString getHealthStatus() const;

private:
    MSSQLConnectionPool() = default;
    ~MSSQLConnectionPool() { closeAllConnections(); }

    static std::unique_ptr<MSSQLConnectionPool> instance_;
    QMap<QString, QSqlDatabase> connections_;
    MSSQLConnectionParameters poolParams_;
    int maxPoolSize_ = 10;
};

// MSSQL Server Information
struct MSSQLServerInfo {
    QString version;
    QString edition;
    QString productLevel;
    QString productUpdateLevel;
    QString machineName;
    QString instanceName;
    QString serverName;
    QString collation;
    bool isIntegratedSecurityOnly = false;
    int maxConnections = 0;

    // Version-specific features
    int majorVersion = 0;
    int minorVersion = 0;
    int buildNumber = 0;

    // Capabilities
    bool supportsJSON = false;
    bool supportsSpatial = false;
    bool supportsXML = false;
    bool supportsSequences = false;
    bool supportsOffsetFetch = false;
    bool supportsStringAgg = false;

    // Get version info
    QString getFullVersionString() const;
    bool isVersionAtLeast(int major, int minor = 0, int build = 0) const;
};

// MSSQL Connection Manager
class MSSQLConnectionManager {
public:
    static MSSQLConnectionManager& instance();

    // Connection management
    bool connect(const MSSQLConnectionParameters& params, QString& errorMessage);
    bool disconnect();
    bool isConnected() const;
    QSqlDatabase getDatabase();

    // Server information
    bool getServerInfo(MSSQLServerInfo& info, QString& errorMessage);
    QStringList getAvailableDatabases(QString& errorMessage);
    QStringList getDatabaseSchemas(const QString& database, QString& errorMessage);

    // Feature detection
    bool detectServerCapabilities(MSSQLServerInfo& info, QString& errorMessage);
    QStringList getSupportedFeatures() const;

    // Connection status
    QString getConnectionStatus() const;
    QString getLastError() const;
    bool testConnection(QString& errorMessage);

    // Configuration
    void setConnectionTimeout(int seconds);
    void setCommandTimeout(int seconds);
    void enableConnectionPooling(bool enable);
    void setPoolSize(int minSize, int maxSize);

private:
    MSSQLConnectionManager() = default;
    ~MSSQLConnectionManager();

    static std::unique_ptr<MSSQLConnectionManager> instance_;
    QSqlDatabase database_;
    MSSQLConnectionParameters currentParams_;
    MSSQLServerInfo serverInfo_;
    QString lastError_;

    bool initializeDatabase(const MSSQLConnectionParameters& params);
    bool configureDatabase(const MSSQLConnectionParameters& params);
};

// MSSQL Authentication Helper
class MSSQLAuthenticationHelper {
public:
    // Authentication methods
    static QStringList getAvailableAuthenticationMethods();
    static bool isWindowsAuthenticationAvailable();
    static bool isSQLServerAuthenticationAvailable();

    // Credential validation
    static bool validateCredentials(const QString& server, const QString& username,
                                  const QString& password, QString& errorMessage);
    static bool validateWindowsCredentials(QString& errorMessage);

    // Security context
    static QString getCurrentWindowsUser();
    static QString getCurrentDomain();
    static bool isDomainUser();

    // Connection string builders
    static QString buildWindowsAuthConnectionString(const QString& server, int port = 1433,
                                                   const QString& database = "");
    static QString buildSQLAuthConnectionString(const QString& server, int port,
                                              const QString& database, const QString& username,
                                              const QString& password);

    // Security utilities
    static QString encryptPassword(const QString& password);
    static QString generateSecurePassword(int length = 12);
};

} // namespace scratchrobin
