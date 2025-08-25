#include "mssql_connection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QProcess>
#include <QDir>
#include <QDebug>

namespace scratchrobin {

// MSSQL Connection Parameters Implementation

bool MSSQLConnectionParameters::validateParameters(QString& errorMessage) const {
    // Validate server
    if (server.isEmpty()) {
        errorMessage = "Server name cannot be empty";
        return false;
    }

    // Validate port
    if (port < 1 || port > 65535) {
        errorMessage = "Port must be between 1 and 65535";
        return false;
    }

    // Validate authentication
    if (!useWindowsAuth && !useSQLAuth) {
        errorMessage = "At least one authentication method must be enabled";
        return false;
    }

    if (useSQLAuth) {
        if (username.isEmpty()) {
            errorMessage = "Username is required for SQL Server authentication";
            return false;
        }
        if (password.isEmpty()) {
            errorMessage = "Password is required for SQL Server authentication";
            return false;
        }
    }

    // Validate timeouts
    if (timeout < 0) {
        errorMessage = "Connection timeout cannot be negative";
        return false;
    }

    if (commandTimeout < 0) {
        errorMessage = "Command timeout cannot be negative";
        return false;
    }

    // Validate ODBC driver
    if (odbcDriver.isEmpty()) {
        errorMessage = "ODBC driver name cannot be empty";
        return false;
    }

    return true;
}

QString MSSQLConnectionParameters::generateConnectionString() const {
    if (useDSN && !dsn.isEmpty()) {
        return generateODBCConnectionString();
    }

    QStringList parts;

    // Server and port
    if (!server.isEmpty()) {
        if (port != 1433) {
            parts << QString("Server=%1,%2").arg(server, QString::number(port));
        } else {
            parts << QString("Server=%1").arg(server);
        }
    }

    // Database
    if (!database.isEmpty()) {
        parts << QString("Database=%1").arg(database);
    }

    // Authentication
    if (useWindowsAuth) {
        parts << "Trusted_Connection=Yes";
    } else if (useSQLAuth) {
        if (!username.isEmpty()) {
            parts << QString("UID=%1").arg(username);
        }
        if (!password.isEmpty()) {
            parts << QString("PWD=%1").arg(password);
        }
    }

    // Driver
    parts << QString("Driver={%1}").arg(odbcDriver);

    // Additional options
    if (encrypt) {
        parts << "Encrypt=Yes";
        if (trustServerCert) {
            parts << "TrustServerCertificate=Yes";
        }
    }

    if (timeout > 0) {
        parts << QString("Connection Timeout=%1").arg(timeout);
    }

    if (!applicationName.isEmpty()) {
        parts << QString("Application Name=%1").arg(applicationName);
    }

    if (!workstationId.isEmpty()) {
        parts << QString("Workstation ID=%1").arg(workstationId);
    }

    if (multipleActiveResultSets) {
        parts << "MultipleActiveResultSets=True";
    }

    if (!failoverPartner.isEmpty()) {
        parts << QString("Failover Partner=%1").arg(failoverPartner);
    }

    // Add any additional parameters
    for (auto it = additionalParams.begin(); it != additionalParams.end(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }

    return parts.join(";");
}

QString MSSQLConnectionParameters::generateODBCConnectionString() const {
    QStringList parts;

    if (!dsn.isEmpty()) {
        parts << QString("DSN=%1").arg(dsn);
    }

    // Database override
    if (!database.isEmpty()) {
        parts << QString("Database=%1").arg(database);
    }

    // Authentication
    if (useWindowsAuth) {
        parts << "Trusted_Connection=Yes";
    } else if (useSQLAuth) {
        if (!username.isEmpty()) {
            parts << QString("UID=%1").arg(username);
        }
        if (!password.isEmpty()) {
            parts << QString("PWD=%1").arg(password);
        }
    }

    return parts.join(";");
}

MSSQLConnectionParameters MSSQLConnectionParameters::fromConnectionString(const QString& connectionString) {
    MSSQLConnectionParameters params;
    QStringList pairs = connectionString.split(";", Qt::SkipEmptyParts);

    for (const QString& pair : pairs) {
        QStringList keyValue = pair.split("=");
        if (keyValue.size() == 2) {
            QString key = keyValue[0].trimmed().toLower();
            QString value = keyValue[1].trimmed();

            if (key == "server") {
                // Handle server,port format
                if (value.contains(",")) {
                    QStringList serverPort = value.split(",");
                    params.server = serverPort[0];
                    params.port = serverPort[1].toInt();
                } else {
                    params.server = value;
                }
            } else if (key == "database") {
                params.database = value;
            } else if (key == "uid") {
                params.username = value;
                params.useSQLAuth = true;
            } else if (key == "pwd") {
                params.password = value;
                params.useSQLAuth = true;
            } else if (key == "trusted_connection") {
                params.useWindowsAuth = (value.toLower() == "yes");
            } else if (key == "driver") {
                params.odbcDriver = value;
            } else if (key == "dsn") {
                params.dsn = value;
                params.useDSN = true;
            } else if (key == "encrypt") {
                params.encrypt = (value.toLower() == "yes");
            } else if (key == "trustservercertificate") {
                params.trustServerCert = (value.toLower() == "yes");
            } else if (key == "connection timeout") {
                params.timeout = value.toInt();
            } else if (key == "application name") {
                params.applicationName = value;
            } else if (key == "workstation id") {
                params.workstationId = value;
            } else if (key == "multipleactiveresultsets") {
                params.multipleActiveResultSets = (value.toLower() == "true");
            } else if (key == "failover partner") {
                params.failoverPartner = value;
            } else {
                // Store additional parameters
                params.additionalParams[keyValue[0].trimmed()] = value;
            }
        }
    }

    return params;
}

// MSSQL Connection Tester Implementation

bool MSSQLConnectionTester::testBasicConnection(const MSSQLConnectionParameters& params, QString& errorMessage) {
    QString validationError;
    if (!params.validateParameters(validationError)) {
        errorMessage = QString("Parameter validation failed: %1").arg(validationError);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_basic");
    db.setDatabaseName(params.generateConnectionString());

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_basic");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_basic");
    return true;
}

bool MSSQLConnectionTester::testDatabaseAccess(const MSSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_db_access");
    db.setDatabaseName(params.generateConnectionString());

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_db_access");
        return false;
    }

    // Test basic query execution
    QSqlQuery query(db);
    if (!query.exec("SELECT @@VERSION")) {
        errorMessage = QString("Query execution failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mssql_db_access");
        return false;
    }

    // Test database selection if specified
    if (!params.database.isEmpty()) {
        if (!query.exec(QString("USE [%1]").arg(params.database))) {
            errorMessage = QString("Database selection failed: %1").arg(query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mssql_db_access");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_db_access");
    return true;
}

bool MSSQLConnectionTester::testPermissions(const MSSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_permissions");
    db.setDatabaseName(params.generateConnectionString());

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_permissions");
        return false;
    }

    QSqlQuery query(db);
    QStringList testQueries = {
        "SELECT * FROM sys.databases",
        "SELECT * FROM INFORMATION_SCHEMA.TABLES",
        "SELECT * FROM sys.objects WHERE type IN ('U', 'V')"
    };

    for (const QString& sql : testQueries) {
        if (!query.exec(sql)) {
            errorMessage = QString("Permission test failed for query '%1': %2").arg(sql, query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mssql_permissions");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_permissions");
    return true;
}

bool MSSQLConnectionTester::testServerFeatures(const MSSQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_features");
    db.setDatabaseName(params.generateConnectionString());

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_features");
        return false;
    }

    QSqlQuery query(db);

    // Test basic features
    if (query.exec("SELECT @@VERSION")) {
        supportedFeatures << "BASIC_CONNECTIVITY";
    }

    // Test JSON support (SQL Server 2016+)
    if (query.exec("SELECT ISJSON('{}')")) {
        supportedFeatures << "JSON_SUPPORT";
    }

    // Test OFFSET FETCH (SQL Server 2012+)
    if (query.exec("SELECT * FROM (SELECT 1 as id) t ORDER BY id OFFSET 0 ROWS FETCH NEXT 1 ROWS ONLY")) {
        supportedFeatures << "OFFSET_FETCH";
    }

    // Test spatial data
    if (query.exec("SELECT geometry::STGeomFromText('POINT(0 0)', 4326)")) {
        supportedFeatures << "SPATIAL_SUPPORT";
    }

    // Test XML support
    if (query.exec("SELECT CAST('<root/>' as XML)")) {
        supportedFeatures << "XML_SUPPORT";
    }

    // Test sequences (SQL Server 2012+)
    if (query.exec("CREATE SEQUENCE #test_seq AS INT START WITH 1 INCREMENT BY 1")) {
        query.exec("DROP SEQUENCE #test_seq");
        supportedFeatures << "SEQUENCES";
    }

    // Test STRING_AGG (SQL Server 2017+)
    if (query.exec("SELECT STRING_AGG('test', ',')")) {
        supportedFeatures << "STRING_AGG";
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_features");
    return true;
}

bool MSSQLConnectionTester::testHighAvailability(const MSSQLConnectionParameters& params, QString& errorMessage) {
    // Test AlwaysOn availability groups and failover
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_ha");
    db.setDatabaseName(params.generateConnectionString());

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_ha");
        return false;
    }

    QSqlQuery query(db);

    // Check if AlwaysOn is enabled
    if (query.exec("SELECT SERVERPROPERTY('IsHadrEnabled') as HadrEnabled")) {
        if (query.next()) {
            int hadrEnabled = query.value(0).toInt();
            if (hadrEnabled == 1) {
                // Test availability groups
                if (!query.exec("SELECT * FROM sys.availability_groups")) {
                    errorMessage = "Failed to query availability groups";
                    db.close();
                    QSqlDatabase::removeDatabase("test_mssql_ha");
                    return false;
                }
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_ha");
    return true;
}

bool MSSQLConnectionTester::testEncryption(const MSSQLConnectionParameters& params, QString& errorMessage) {
    // Test SSL/TLS encryption
    MSSQLConnectionParameters testParams = params;
    testParams.encrypt = true;

    return testBasicConnection(testParams, errorMessage);
}

bool MSSQLConnectionTester::testPerformance(const MSSQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_mssql_performance");
    db.setDatabaseName(params.generateConnectionString());

    QElapsedTimer timer;
    timer.start();

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mssql_performance");
        return false;
    }

    qint64 connectionTime = timer.elapsed();

    QSqlQuery query(db);

    // Test simple query performance
    timer.restart();
    if (!query.exec("SELECT GETDATE()")) {
        errorMessage = "Simple query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mssql_performance");
        return false;
    }
    qint64 simpleQueryTime = timer.elapsed();

    // Test more complex query
    timer.restart();
    if (!query.exec("SELECT * FROM sys.databases WHERE database_id <= 10")) {
        errorMessage = "Complex query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mssql_performance");
        return false;
    }
    qint64 complexQueryTime = timer.elapsed();

    db.close();
    QSqlDatabase::removeDatabase("test_mssql_performance");

    // Store metrics
    metrics["connection_time_ms"] = connectionTime;
    metrics["simple_query_time_ms"] = simpleQueryTime;
    metrics["complex_query_time_ms"] = complexQueryTime;

    return true;
}

// MSSQL Connection Manager Implementation

std::unique_ptr<MSSQLConnectionManager> MSSQLConnectionManager::instance_ = nullptr;

MSSQLConnectionManager& MSSQLConnectionManager::instance() {
    if (!instance_) {
        instance_.reset(new MSSQLConnectionManager());
    }
    return *instance_;
}

bool MSSQLConnectionManager::connect(const MSSQLConnectionParameters& params, QString& errorMessage) {
    if (isConnected()) {
        disconnect();
    }

    currentParams_ = params;

    if (!initializeDatabase(params)) {
        errorMessage = "Failed to initialize database connection";
        return false;
    }

    if (!configureDatabase(params)) {
        errorMessage = "Failed to configure database connection";
        return false;
    }

    if (!database_.open()) {
        errorMessage = QString("Database connection failed: %1").arg(database_.lastError().text());
        return false;
    }

    // Test the connection
    if (!testConnection(errorMessage)) {
        disconnect();
        return false;
    }

    return true;
}

bool MSSQLConnectionManager::disconnect() {
    if (database_.isOpen()) {
        database_.close();
    }
    if (!database_.connectionName().isEmpty()) {
        QSqlDatabase::removeDatabase(database_.connectionName());
    }
    return true;
}

bool MSSQLConnectionManager::isConnected() const {
    return database_.isValid() && database_.isOpen();
}

QSqlDatabase MSSQLConnectionManager::getDatabase() {
    return database_;
}

bool MSSQLConnectionManager::getServerInfo(MSSQLServerInfo& info, QString& errorMessage) {
    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT @@VERSION as version, SERVERPROPERTY('Edition') as edition, "
                   "SERVERPROPERTY('ProductLevel') as product_level, "
                   "SERVERPROPERTY('ProductUpdateLevel') as product_update_level, "
                   "SERVERPROPERTY('MachineName') as machine_name, "
                   "SERVERPROPERTY('InstanceName') as instance_name, "
                   "SERVERPROPERTY('ServerName') as server_name, "
                   "SERVERPROPERTY('Collation') as collation, "
                   "SERVERPROPERTY('IsIntegratedSecurityOnly') as integrated_security_only, "
                   "SERVERPROPERTY('ProductVersion') as product_version")) {
        errorMessage = QString("Failed to get server info: %1").arg(query.lastError().text());
        return false;
    }

    if (query.next()) {
        info.version = query.value("version").toString();
        info.edition = query.value("edition").toString();
        info.productLevel = query.value("product_level").toString();
        info.productUpdateLevel = query.value("product_update_level").toString();
        info.machineName = query.value("machine_name").toString();
        info.instanceName = query.value("instance_name").toString();
        info.serverName = query.value("server_name").toString();
        info.collation = query.value("collation").toString();
        info.isIntegratedSecurityOnly = query.value("integrated_security_only").toBool();

        // Parse version number
        QString productVersion = query.value("product_version").toString();
        QStringList versionParts = productVersion.split(".");
        if (versionParts.size() >= 3) {
            info.majorVersion = versionParts[0].toInt();
            info.minorVersion = versionParts[1].toInt();
            info.buildNumber = versionParts[2].toInt();
        }

        serverInfo_ = info;
        return true;
    }

    errorMessage = "No server information returned";
    return false;
}

QStringList MSSQLConnectionManager::getAvailableDatabases(QString& errorMessage) {
    QStringList databases;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return databases;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT name FROM sys.databases WHERE database_id > 4 ORDER BY name")) {
        errorMessage = QString("Failed to get databases: %1").arg(query.lastError().text());
        return databases;
    }

    while (query.next()) {
        databases << query.value(0).toString();
    }

    return databases;
}

QStringList MSSQLConnectionManager::getDatabaseSchemas(const QString& database, QString& errorMessage) {
    QStringList schemas;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return schemas;
    }

    QSqlQuery query(database_);
    QString sql = QString("USE [%1]; SELECT name FROM sys.schemas ORDER BY name").arg(database);

    if (!query.exec(sql)) {
        errorMessage = QString("Failed to get schemas: %1").arg(query.lastError().text());
        return schemas;
    }

    while (query.next()) {
        schemas << query.value(0).toString();
    }

    return schemas;
}

bool MSSQLConnectionManager::detectServerCapabilities(MSSQLServerInfo& info, QString& errorMessage) {
    if (!getServerInfo(info, errorMessage)) {
        return false;
    }

    // Detect feature support based on version
    info.supportsJSON = info.isVersionAtLeast(13); // SQL Server 2016
    info.supportsSpatial = info.isVersionAtLeast(10); // SQL Server 2008
    info.supportsXML = info.isVersionAtLeast(9); // SQL Server 2005
    info.supportsSequences = info.isVersionAtLeast(11); // SQL Server 2012
    info.supportsOffsetFetch = info.isVersionAtLeast(11); // SQL Server 2012
    info.supportsStringAgg = info.isVersionAtLeast(14); // SQL Server 2017

    return true;
}

QStringList MSSQLConnectionManager::getSupportedFeatures() const {
    QStringList features;

    if (serverInfo_.supportsJSON) features << "JSON";
    if (serverInfo_.supportsSpatial) features << "SPATIAL";
    if (serverInfo_.supportsXML) features << "XML";
    if (serverInfo_.supportsSequences) features << "SEQUENCES";
    if (serverInfo_.supportsOffsetFetch) features << "OFFSET_FETCH";
    if (serverInfo_.supportsStringAgg) features << "STRING_AGG";

    return features;
}

QString MSSQLConnectionManager::getConnectionStatus() const {
    if (!isConnected()) {
        return "Disconnected";
    }

    return QString("Connected to %1\\%2").arg(serverInfo_.serverName, serverInfo_.instanceName);
}

QString MSSQLConnectionManager::getLastError() const {
    return lastError_;
}

bool MSSQLConnectionManager::testConnection(QString& errorMessage) {
    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT 1 as test")) {
        errorMessage = QString("Connection test failed: %1").arg(query.lastError().text());
        lastError_ = errorMessage;
        return false;
    }

    return true;
}

void MSSQLConnectionManager::setConnectionTimeout(int seconds) {
    currentParams_.timeout = seconds;
    // Reconnect with new timeout if connected
    if (isConnected()) {
        QString error;
        connect(currentParams_, error);
    }
}

void MSSQLConnectionManager::setCommandTimeout(int seconds) {
    currentParams_.commandTimeout = seconds;
}

void MSSQLConnectionManager::enableConnectionPooling(bool enable) {
    currentParams_.connectionPooling = enable;
}

void MSSQLConnectionManager::setPoolSize(int minSize, int maxSize) {
    currentParams_.minPoolSize = minSize;
    currentParams_.maxPoolSize = maxSize;
}

bool MSSQLConnectionManager::initializeDatabase(const MSSQLConnectionParameters& params) {
    QString connectionName = QString("mssql_connection_%1").arg(QDateTime::currentMSecsSinceEpoch());
    database_ = QSqlDatabase::addDatabase("QODBC", connectionName);
    database_.setDatabaseName(params.generateConnectionString());

    // Set additional connection options
    if (params.timeout > 0) {
        database_.setConnectOptions(QString("SQL_ATTR_CONNECTION_TIMEOUT=%1").arg(params.timeout));
    }

    if (params.commandTimeout > 0) {
        database_.setConnectOptions(QString("SQL_ATTR_QUERY_TIMEOUT=%1").arg(params.commandTimeout));
    }

    return database_.isValid();
}

bool MSSQLConnectionManager::configureDatabase(const MSSQLConnectionParameters& params) {
    // Set database-specific configuration
    if (!params.database.isEmpty()) {
        // The database is already specified in the connection string
        // No additional configuration needed
    }

    return true;
}

// MSSQL Server Info Implementation

QString MSSQLServerInfo::getFullVersionString() const {
    return QString("%1.%2.%3").arg(majorVersion).arg(minorVersion).arg(buildNumber);
}

bool MSSQLServerInfo::isVersionAtLeast(int major, int minor, int build) const {
    if (majorVersion > major) return true;
    if (majorVersion < major) return false;
    if (minorVersion > minor) return true;
    if (minorVersion < minor) return false;
    return buildNumber >= build;
}

// MSSQL Authentication Helper Implementation

QStringList MSSQLAuthenticationHelper::getAvailableAuthenticationMethods() {
    QStringList methods;
    methods << "SQL Server Authentication";

    if (isWindowsAuthenticationAvailable()) {
        methods << "Windows Authentication";
    }

    return methods;
}

bool MSSQLAuthenticationHelper::isWindowsAuthenticationAvailable() {
    // Check if running on Windows
    #ifdef Q_OS_WIN
    return true;
    #else
    return false;
    #endif
}

bool MSSQLAuthenticationHelper::isSQLServerAuthenticationAvailable() {
    return true; // Always available
}

bool MSSQLAuthenticationHelper::validateCredentials(const QString& server, const QString& username,
                                                   const QString& password, QString& errorMessage) {
    MSSQLConnectionParameters params;
    params.server = server;
    params.username = username;
    params.password = password;
    params.useSQLAuth = true;
    params.useWindowsAuth = false;

    return MSSQLConnectionTester::testBasicConnection(params, errorMessage);
}

bool MSSQLAuthenticationHelper::validateWindowsCredentials(QString& errorMessage) {
    #ifdef Q_OS_WIN
    MSSQLConnectionParameters params;
    params.server = "localhost";
    params.useSQLAuth = false;
    params.useWindowsAuth = true;

    return MSSQLConnectionTester::testBasicConnection(params, errorMessage);
    #else
    errorMessage = "Windows Authentication is not available on this platform";
    return false;
    #endif
}

QString MSSQLAuthenticationHelper::getCurrentWindowsUser() {
    #ifdef Q_OS_WIN
    return qgetenv("USERNAME");
    #else
    return QString();
    #endif
}

QString MSSQLAuthenticationHelper::getCurrentDomain() {
    #ifdef Q_OS_WIN
    return qgetenv("USERDOMAIN");
    #else
    return QString();
    #endif
}

bool MSSQLAuthenticationHelper::isDomainUser() {
    #ifdef Q_OS_WIN
    QString domain = getCurrentDomain();
    return !domain.isEmpty() && domain != ".";
    #else
    return false;
    #endif
}

QString MSSQLAuthenticationHelper::buildWindowsAuthConnectionString(const QString& server, int port,
                                                                   const QString& database) {
    QStringList parts;
    parts << QString("Driver={ODBC Driver 17 for SQL Server}");
    parts << QString("Server=%1").arg(port != 1433 ? QString("%1,%2").arg(server, QString::number(port)) : server);
    if (!database.isEmpty()) {
        parts << QString("Database=%1").arg(database);
    }
    parts << "Trusted_Connection=Yes";

    return parts.join(";");
}

QString MSSQLAuthenticationHelper::buildSQLAuthConnectionString(const QString& server, int port,
                                                              const QString& database, const QString& username,
                                                              const QString& password) {
    QStringList parts;
    parts << QString("Driver={ODBC Driver 17 for SQL Server}");
    parts << QString("Server=%1").arg(port != 1433 ? QString("%1,%2").arg(server, QString::number(port)) : server);
    if (!database.isEmpty()) {
        parts << QString("Database=%1").arg(database);
    }
    parts << QString("UID=%1").arg(username);
    parts << QString("PWD=%1").arg(password);

    return parts.join(";");
}

QString MSSQLAuthenticationHelper::encryptPassword(const QString& password) {
    // Basic obfuscation - in production, use proper encryption
    return password.toUtf8().toBase64();
}

QString MSSQLAuthenticationHelper::generateSecurePassword(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;

    for (int i = 0; i < length; ++i) {
        int index = qrand() % chars.length();
        password.append(chars.at(index));
    }

    return password;
}

MSSQLConnectionManager::~MSSQLConnectionManager() {
    disconnect();
}

} // namespace scratchrobin
