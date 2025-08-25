#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>
#include "database_driver_manager.h"
#include "mysql_features.h"

namespace scratchrobin {

// MySQL Connection Parameters and Configuration
struct MySQLConnectionParameters {
    // Basic connection parameters
    QString host;              // Server host (default: localhost)
    int port = 3306;          // Server port (default: 3306)
    QString database;         // Default database name
    QString username;         // Username
    QString password;         // Password

    // Authentication options
    bool useSSL = false;      // Use SSL/TLS encryption
    QString sslCA;            // SSL CA certificate file
    QString sslCert;          // SSL client certificate file
    QString sslKey;           // SSL client key file
    QString sslCipher;        // SSL cipher list

    // Connection options
    bool compress = false;    // Use compression
    QString charset = "utf8mb4"; // Connection character set
    QString collation = "utf8mb4_general_ci"; // Connection collation
    int timeout = 30;         // Connection timeout in seconds
    int commandTimeout = 0;   // Command timeout in seconds (0 = no limit)

    // Advanced options
    QString initCommand;      // Initial command to execute
    QString unixSocket;       // Unix socket path (for local connections)
    QString namedPipe;        // Named pipe (for Windows local connections)
    bool useNamedPipe = false; // Use named pipe instead of TCP
    QString applicationName;  // Application name for identification
    bool autoReconnect = true; // Auto-reconnect on connection loss

    // Pooling options
    bool connectionPooling = true;
    int minPoolSize = 1;
    int maxPoolSize = 10;
    int connectionLifetime = 0;

    // MySQL-specific options
    bool useMySQLClientLibrary = true; // Use MySQL client library
    QString pluginDir;       // Plugin directory
    bool allowLocalInfile = false; // Allow LOAD DATA LOCAL INFILE
    bool allowMultipleStatements = false; // Allow multiple statements per query
    int maxAllowedPacket = 1048576; // Maximum packet size (1MB default)

    // MySQL 5.7+ specific options
    bool usePerformanceSchema = true; // Enable Performance Schema monitoring
    bool useSysSchema = true;         // Enable sys schema for monitoring

    // MySQL 8.0+ specific options
    bool useMySQLX = false;           // Use MySQL X Protocol
    int mysqlxPort = 33060;          // MySQL X Protocol port

    // Additional connection string parameters
    QMap<QString, QString> additionalParams;

    // Validate the connection parameters
    bool validateParameters(QString& errorMessage) const;

    // Generate connection string
    QString generateConnectionString() const;

    // Generate ODBC connection string (for compatibility)
    QString generateODBCConnectionString() const;

    // Parse connection string
    static MySQLConnectionParameters fromConnectionString(const QString& connectionString);
};

// MySQL Connection Tester
class MySQLConnectionTester {
public:
    static bool testBasicConnection(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testDatabaseAccess(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testPermissions(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testServerFeatures(const MySQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage);

    // Advanced testing
    static bool testReplication(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testSSLConnection(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testPerformance(const MySQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage);
    static bool testStorageEngines(const MySQLConnectionParameters& params, QStringList& engines, QString& errorMessage);
    static bool testMySQLVersion(const MySQLConnectionParameters& params, QString& version, QString& errorMessage);

    // MySQL-specific testing
    static bool testPerformanceSchema(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testSysSchema(const MySQLConnectionParameters& params, QString& errorMessage);
    static bool testEnterpriseFeatures(const MySQLConnectionParameters& params, QStringList& features, QString& errorMessage);
};

// MySQL Connection Pool Manager
class MySQLConnectionPool {
public:
    static MySQLConnectionPool& instance();

    bool initializePool(const MySQLConnectionParameters& params, int poolSize = 10);
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
    MySQLConnectionPool() = default;
    ~MySQLConnectionPool() { closeAllConnections(); }

    static std::unique_ptr<MySQLConnectionPool> instance_;
    QMap<QString, QSqlDatabase> connections_;
    MySQLConnectionParameters poolParams_;
    int maxPoolSize_ = 10;
};

// MySQL Server Information
struct MySQLServerInfo {
    QString version;
    QString versionComment;
    QString compileMachine;
    QString compileOS;
    QString hostname;
    int port = 3306;
    QString socket;
    QString basedir;
    QString datadir;
    QString tmpdir;
    QString serverCharset;
    QString serverCollation;
    QString timeZone;
    QString systemTimeZone;
    int maxConnections = 0;
    int maxUserConnections = 0;

    // Version-specific features
    int majorVersion = 0;
    int minorVersion = 0;
    int patchVersion = 0;

    // MySQL-specific version info
    bool isPerconaServer = false;
    bool isMySQLCluster = false;
    bool isEnterprise = false;

    // Capabilities
    bool supportsJSON = false;
    bool supportsSequences = false;  // MySQL doesn't have sequences
    bool supportsVirtualColumns = false;
    bool supportsWindowFunctions = false;
    bool supportsCTEs = false;
    bool supportsSpatial = false;
    bool supportsPartitioning = false;
    bool supportsGTID = false;
    bool supportsPerformanceSchema = false;
    bool supportsReplication = false;
    bool supportsFulltextSearch = false;
    bool supportsInvisibleIndexes = false;
    bool supportsExpressionIndexes = false;
    bool supportsDescendingIndexes = false;
    bool supportsSSL = false;

    // MySQL-specific capabilities
    bool supportsMySQLX = false;
    bool supportsSysSchema = false;
    bool supportsEnterpriseFeatures = false;

    // Get version info
    QString getFullVersionString() const;
    bool isVersionAtLeast(int major, int minor = 0, int patch = 0) const;
    bool isMySQL5_5() const;
    bool isMySQL5_6() const;
    bool isMySQL5_7() const;
    bool isMySQL8_0() const;
    bool isMySQL8_1() const;
};

// MySQL Connection Manager
class MySQLConnectionManager {
public:
    static MySQLConnectionManager& instance();

    // Connection management
    bool connect(const MySQLConnectionParameters& params, QString& errorMessage);
    bool disconnect();
    bool isConnected() const;
    QSqlDatabase getDatabase();

    // Server information
    bool getServerInfo(MySQLServerInfo& info, QString& errorMessage);
    QStringList getAvailableDatabases(QString& errorMessage);
    QStringList getDatabaseSchemas(const QString& database, QString& errorMessage);
    QStringList getStorageEngines(QString& errorMessage);
    QStringList getAvailablePlugins(QString& errorMessage);
    QStringList getAvailableCharSets(QString& errorMessage);

    // Feature detection
    bool detectServerCapabilities(MySQLServerInfo& info, QString& errorMessage);
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

    // MySQL-specific configuration
    void enablePerformanceSchema(bool enable);
    void enableSysSchema(bool enable);

private:
    MySQLConnectionManager() = default;
    ~MySQLConnectionManager();

    static std::unique_ptr<MySQLConnectionManager> instance_;
    QSqlDatabase database_;
    MySQLConnectionParameters currentParams_;
    MySQLServerInfo serverInfo_;
    QString lastError_;

    bool initializeDatabase(const MySQLConnectionParameters& params);
    bool configureDatabase(const MySQLConnectionParameters& params);
};

// MySQL Authentication Helper
class MySQLAuthenticationHelper {
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
    static QString buildStandardConnectionString(const QString& host, int port = 3306,
                                               const QString& database = "",
                                               const QString& username = "",
                                               const QString& password = "");
    static QString buildSSLConnectionString(const QString& host, int port = 3306,
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

    // MySQL-specific authentication
    static QString buildMySQLXConnectionString(const QString& host, int port = 33060,
                                             const QString& database = "",
                                             const QString& username = "",
                                             const QString& password = "");
};

// MySQL SSL Configuration Helper
class MySQLSSLHelper {
public:
    // SSL certificate validation
    static bool validateCertificate(const QString& caCert, const QString& clientCert,
                                  const QString& clientKey, QString& errorMessage);

    // SSL cipher configuration
    static QStringList getSupportedSSLCiphers();
    static QString getRecommendedSSLCipher();

    // Certificate generation
    static bool generateSelfSignedCertificate(const QString& certFile, const QString& keyFile,
                                            const QString& subject, int days = 365, QString& errorMessage);

    // SSL connection testing
    static bool testSSLConnection(const QString& host, int port,
                                const QString& caCert, const QString& clientCert,
                                const QString& clientKey, QString& errorMessage);
};

// MySQL Version Helper
class MySQLVersionHelper {
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

    // MySQL-specific version checks
    static bool isMySQL5_5(const QString& versionString);
    static bool isMySQL5_6(const QString& versionString);
    static bool isMySQL5_7(const QString& versionString);
    static bool isMySQL8_0(const QString& versionString);
    static bool isMySQL8_1(const QString& versionString);
    static bool isPerconaServer(const QString& versionString);
    static bool isMySQLCluster(const QString& versionString);
};

} // namespace scratchrobin
