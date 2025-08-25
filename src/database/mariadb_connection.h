#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <memory>
#include <vector>
#include "database_driver_manager.h"
#include "mariadb_features.h"

namespace scratchrobin {

// MariaDB Connection Parameters and Configuration
struct MariaDBConnectionParameters {
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

    // MariaDB-specific options
    bool useMariaDBClientLibrary = true; // Use MariaDB client library
    QString pluginDir;       // Plugin directory
    bool allowLocalInfile = false; // Allow LOAD DATA LOCAL INFILE
    bool allowMultipleStatements = false; // Allow multiple statements per query
    int maxAllowedPacket = 1048576; // Maximum packet size (1MB default)

    // Additional connection string parameters
    QMap<QString, QString> additionalParams;

    // Validate the connection parameters
    bool validateParameters(QString& errorMessage) const;

    // Generate connection string
    QString generateConnectionString() const;

    // Generate ODBC connection string (for compatibility)
    QString generateODBCConnectionString() const;

    // Parse connection string
    static MariaDBConnectionParameters fromConnectionString(const QString& connectionString);
};

// MariaDB Connection Tester
class MariaDBConnectionTester {
public:
    static bool testBasicConnection(const MariaDBConnectionParameters& params, QString& errorMessage);
    static bool testDatabaseAccess(const MariaDBConnectionParameters& params, QString& errorMessage);
    static bool testPermissions(const MariaDBConnectionParameters& params, QString& errorMessage);
    static bool testServerFeatures(const MariaDBConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage);

    // Advanced testing
    static bool testReplication(const MariaDBConnectionParameters& params, QString& errorMessage);
    static bool testSSLConnection(const MariaDBConnectionParameters& params, QString& errorMessage);
    static bool testPerformance(const MariaDBConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage);
    static bool testStorageEngines(const MariaDBConnectionParameters& params, QStringList& engines, QString& errorMessage);
};

// MariaDB Connection Pool Manager
class MariaDBConnectionPool {
public:
    static MariaDBConnectionPool& instance();

    bool initializePool(const MariaDBConnectionParameters& params, int poolSize = 10);
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
    MariaDBConnectionPool() = default;
    ~MariaDBConnectionPool() { closeAllConnections(); }

    static std::unique_ptr<MariaDBConnectionPool> instance_;
    QMap<QString, QSqlDatabase> connections_;
    MariaDBConnectionParameters poolParams_;
    int maxPoolSize_ = 10;
};

// MariaDB Server Information
struct MariaDBServerInfo {
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

    // Capabilities
    bool supportsJSON = false;
    bool supportsSequences = false;
    bool supportsVirtualColumns = false;
    bool supportsDynamicColumns = false;
    bool supportsWindowFunctions = false;
    bool supportsCTEs = false;
    bool supportsSpatial = false;
    bool supportsPartitioning = false;
    bool supportsGTID = false;
    bool supportsPerformanceSchema = false;
    bool supportsReplication = false;
    bool supportsSSL = false;

    // Get version info
    QString getFullVersionString() const;
    bool isVersionAtLeast(int major, int minor = 0, int patch = 0) const;
};

// MariaDB Connection Manager
class MariaDBConnectionManager {
public:
    static MariaDBConnectionManager& instance();

    // Connection management
    bool connect(const MariaDBConnectionParameters& params, QString& errorMessage);
    bool disconnect();
    bool isConnected() const;
    QSqlDatabase getDatabase();

    // Server information
    bool getServerInfo(MariaDBServerInfo& info, QString& errorMessage);
    QStringList getAvailableDatabases(QString& errorMessage);
    QStringList getDatabaseSchemas(const QString& database, QString& errorMessage);
    QStringList getStorageEngines(QString& errorMessage);
    QStringList getAvailablePlugins(QString& errorMessage);

    // Feature detection
    bool detectServerCapabilities(MariaDBServerInfo& info, QString& errorMessage);
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

private:
    MariaDBConnectionManager() = default;
    ~MariaDBConnectionManager();

    static std::unique_ptr<MariaDBConnectionManager> instance_;
    QSqlDatabase database_;
    MariaDBConnectionParameters currentParams_;
    MariaDBServerInfo serverInfo_;
    QString lastError_;

    bool initializeDatabase(const MariaDBConnectionParameters& params);
    bool configureDatabase(const MariaDBConnectionParameters& params);
};

// MariaDB Authentication Helper
class MariaDBAuthenticationHelper {
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
};

// MariaDB SSL Configuration Helper
class MariaDBSSLHelper {
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

} // namespace scratchrobin
