#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QElapsedTimer>
#include <memory>
#include <vector>
#include "database_driver_manager.h"
#include "postgresql_features.h"

namespace scratchrobin {

// PostgreSQL Connection Parameters and Configuration
struct PostgreSQLConnectionParameters {
    // Basic connection parameters
    QString host;              // Server host (default: localhost)
    int port = 5432;          // Server port (default: 5432)
    QString database;         // Default database name
    QString username;         // Username
    QString password;         // Password

    // Authentication options
    bool useSSL = false;      // Use SSL/TLS encryption
    QString sslCA;            // SSL CA certificate file
    QString sslCert;          // SSL client certificate file
    QString sslKey;           // SSL client key file
    QString sslMode = "prefer"; // SSL mode (disable, allow, prefer, require, verify-ca, verify-full)
    QString sslCRL;           // SSL certificate revocation list

    // Connection options
    QString charset = "UTF8"; // Connection character set
    int timeout = 30;         // Connection timeout in seconds
    int commandTimeout = 0;   // Command timeout in seconds (0 = no limit)

    // Advanced options
    QString applicationName;  // Application name for identification
    bool autoReconnect = true; // Auto-reconnect on connection loss
    QString searchPath;       // Schema search path
    QString timezone;         // Time zone for connection
    QString role;             // Role to switch to after connection

    // Pooling options
    bool connectionPooling = true;
    int minPoolSize = 1;
    int maxPoolSize = 10;
    int connectionLifetime = 0;

    // PostgreSQL-specific options
    bool usePostgresLibrary = true; // Use PostgreSQL client library
    QString options;         // Additional connection options
    QString fallbackApplicationName; // Fallback application name
    bool keepalives = true;  // Enable TCP keepalives
    int keepalivesIdle = 0;  // TCP keepalives idle time
    int keepalivesInterval = 0; // TCP keepalives interval
    int keepalivesCount = 0; // TCP keepalives count

    // PostgreSQL 9.0+ specific options
    QString targetSessionAttrs = "any"; // Session attributes (any, read-write, read-only, primary, standby, prefer-standby)

    // PostgreSQL 12+ specific options
    bool gssEncMode = false;  // Enable GSS encryption

    // Additional connection string parameters
    QMap<QString, QString> additionalParams;

    // Validate the connection parameters
    bool validateParameters(QString& errorMessage) const;

    // Generate connection string
    QString generateConnectionString() const;

    // Generate ODBC connection string (for compatibility)
    QString generateODBCConnectionString() const;

    // Parse connection string
    static PostgreSQLConnectionParameters fromConnectionString(const QString& connectionString);
};

// PostgreSQL Connection Tester
class PostgreSQLConnectionTester {
public:
    static bool testBasicConnection(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testDatabaseAccess(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testPermissions(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testServerFeatures(const PostgreSQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage);

    // Advanced testing
    static bool testReplication(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testSSLConnection(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testPerformance(const PostgreSQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage);
    static bool testStorageEngines(const PostgreSQLConnectionParameters& params, QStringList& engines, QString& errorMessage);
    static bool testExtensions(const PostgreSQLConnectionParameters& params, QStringList& extensions, QString& errorMessage);

    // PostgreSQL-specific testing
    static bool testPostGIS(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testPostgresFDW(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    static bool testEnterpriseFeatures(const PostgreSQLConnectionParameters& params, QStringList& features, QString& errorMessage);
};

// PostgreSQL Connection Pool Manager
class PostgreSQLConnectionPool {
public:
    static PostgreSQLConnectionPool& instance();

    bool initializePool(const PostgreSQLConnectionParameters& params, int poolSize = 10);
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
    PostgreSQLConnectionPool() = default;
    ~PostgreSQLConnectionPool() { closeAllConnections(); }

    static std::unique_ptr<PostgreSQLConnectionPool> instance_;
    QMap<QString, QSqlDatabase> connections_;
    PostgreSQLConnectionParameters poolParams_;
    int maxPoolSize_ = 10;
};

// PostgreSQL Server Information
struct PostgreSQLServerInfo {
    QString version;
    QString versionComment;
    QString compileMachine;
    QString compileOS;
    QString serverAddress;
    int serverPort = 5432;
    QString serverEncoding;
    QString clientEncoding;
    QString lcCollate;
    QString lcCtype;
    QString timezone;
    QString sharedBuffers;
    QString workMem;
    QString maintenanceWorkMem;
    QString effectiveCacheSize;
    int maxConnections = 0;
    bool autovacuumEnabled = false;
    QString logStatement;
    QString logDuration;
    QString databaseSize;

    // Version-specific features
    int majorVersion = 0;
    int minorVersion = 0;
    int patchVersion = 0;

    // PostgreSQL-specific version info
    bool isEnterpriseDB = false;
    bool isPostgresPlus = false;
    bool isGreenplum = false;

    // Capabilities
    bool supportsJSON = false;
    bool supportsArrays = false;
    bool supportsHStore = false;
    bool supportsGeometry = false;
    bool supportsTextSearch = false;
    bool supportsRanges = false;
    bool supportsCTEs = false;
    bool supportsWindowFunctions = false;
    bool supportsInheritance = false;
    bool supportsPartitioning = false;
    bool supportsSSL = false;
    bool supportsReplication = false;

    // PostgreSQL-specific capabilities
    bool supportsPostGIS = false;
    bool supportsPostgresFDW = false;
    bool supportsEnterpriseFeatures = false;

    // Get version info
    QString getFullVersionString() const;
    bool isVersionAtLeast(int major, int minor = 0, int patch = 0) const;
    bool isPostgreSQL9_0() const;
    bool isPostgreSQL9_1() const;
    bool isPostgreSQL9_2() const;
    bool isPostgreSQL9_3() const;
    bool isPostgreSQL9_4() const;
    bool isPostgreSQL9_5() const;
    bool isPostgreSQL9_6() const;
    bool isPostgreSQL10() const;
    bool isPostgreSQL11() const;
    bool isPostgreSQL12() const;
    bool isPostgreSQL13() const;
    bool isPostgreSQL14() const;
    bool isPostgreSQL15() const;
    bool isPostgreSQL16() const;
};

// PostgreSQL Connection Manager
class PostgreSQLConnectionManager {
public:
    static PostgreSQLConnectionManager& instance();

    // Connection management
    bool connect(const PostgreSQLConnectionParameters& params, QString& errorMessage);
    bool disconnect();
    bool isConnected() const;
    QSqlDatabase getDatabase();

    // Server information
    bool getServerInfo(PostgreSQLServerInfo& info, QString& errorMessage);
    QStringList getAvailableDatabases(QString& errorMessage);
    QStringList getDatabaseSchemas(const QString& database, QString& errorMessage);
    QStringList getStorageEngines(QString& errorMessage);
    QStringList getAvailableExtensions(QString& errorMessage);
    QStringList getAvailableCharSets(QString& errorMessage);

    // Feature detection
    bool detectServerCapabilities(PostgreSQLServerInfo& info, QString& errorMessage);
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

    // SSL configuration
    bool configureSSL(const QString& caCert, const QString& clientCert,
                     const QString& clientKey, QString& errorMessage);

    // PostgreSQL-specific configuration
    void setSearchPath(const QString& searchPath);
    void setTimeZone(const QString& timezone);
    void setApplicationName(const QString& appName);

private:
    PostgreSQLConnectionManager() = default;
public:
    ~PostgreSQLConnectionManager();

    static std::unique_ptr<PostgreSQLConnectionManager> instance_;
    QSqlDatabase database_;
    PostgreSQLConnectionParameters currentParams_;
    PostgreSQLServerInfo serverInfo_;
    QString lastError_;

    bool initializeDatabase(const PostgreSQLConnectionParameters& params);
    bool configureDatabase(const PostgreSQLConnectionParameters& params);
};

// PostgreSQL Authentication Helper
class PostgreSQLAuthenticationHelper {
public:
    // Authentication methods
    static QStringList getAvailableAuthenticationMethods();
    static bool isSSLSupported();
    static bool isCompressionSupported();

    // Credential validation
    static bool validateCredentials(const QString& host, int port,
                                  const QString& username, const QString& password,
                                  QString& errorMessage);
    static bool validateSSLConnection(const QString& host, int port,
                                    const QString& caCert, const QString& clientCert,
                                    const QString& clientKey, QString& errorMessage);

    // Security utilities
    static QString generateSecurePassword(int length = 12);
    static bool isPasswordStrong(const QString& password);

    // Connection string builders
    static QString buildStandardConnectionString(const QString& host, int port = 5432,
                                               const QString& database = "",
                                               const QString& username = "",
                                               const QString& password = "");
    static QString buildSSLConnectionString(const QString& host, int port = 5432,
                                          const QString& database = "",
                                          const QString& username = "",
                                          const QString& password = "",
                                          const QString& caCert = "",
                                          const QString& clientCert = "",
                                          const QString& clientKey = "");
    static QString buildSocketConnectionString(const QString& socketPath,
                                             const QString& database = "",
                                             const QString& username = "",
                                             const QString& password = "");

    // PostgreSQL-specific authentication
    static QString buildKerberosConnectionString(const QString& host, int port = 5432,
                                                const QString& database = "",
                                                const QString& username = "");
    static QString buildLDAPConnectionString(const QString& host, int port = 5432,
                                           const QString& database = "",
                                           const QString& username = "");
};

// PostgreSQL SSL Configuration Helper
class PostgreSQLSSLHelper {
public:
    // SSL certificate validation
    static bool validateCertificate(const QString& caCert, const QString& clientCert,
                                  const QString& clientKey, QString& errorMessage);

    // SSL cipher configuration
    static QStringList getSupportedSSLCiphers();
    static QString getRecommendedSSLCipher();

    // Certificate generation
    static bool generateSelfSignedCertificate(const QString& certFile, const QString& keyFile,
                                                                                         const QString& subject, int days, QString& errorMessage);

    // SSL connection testing
    static bool testSSLConnection(const QString& host, int port,
                                const QString& caCert, const QString& clientCert,
                                const QString& clientKey, QString& errorMessage);
};

// PostgreSQL Version Helper
class PostgreSQLVersionHelper {
public:
    // Version parsing
    static bool parseVersion(const QString& versionString, int& major, int& minor, int& patch);
    static QString getVersionFamily(const QString& versionString);

    // Feature support by version
    static bool supportsFeature(const QString& versionString, const QString& feature);
    static QString getMinimumVersionForFeature(const QString& feature);

    // Version comparison
    static int compareVersions(const QString& version1, const QString& version2);
    static bool isVersionInRange(const QString& version, const QString& minVersion, const QString& maxVersion = "");

    // PostgreSQL-specific version checks
    static bool isPostgreSQL9_0(const QString& versionString);
    static bool isPostgreSQL9_1(const QString& versionString);
    static bool isPostgreSQL9_2(const QString& versionString);
    static bool isPostgreSQL9_3(const QString& versionString);
    static bool isPostgreSQL9_4(const QString& versionString);
    static bool isPostgreSQL9_5(const QString& versionString);
    static bool isPostgreSQL9_6(const QString& versionString);
    static bool isPostgreSQL10(const QString& versionString);
    static bool isPostgreSQL11(const QString& versionString);
    static bool isPostgreSQL12(const QString& versionString);
    static bool isPostgreSQL13(const QString& versionString);
    static bool isPostgreSQL14(const QString& versionString);
    static bool isPostgreSQL15(const QString& versionString);
    static bool isPostgreSQL16(const QString& versionString);
};

} // namespace scratchrobin
