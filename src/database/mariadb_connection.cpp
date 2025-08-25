#include "mariadb_connection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QProcess>
#include <QDir>
#include <QDebug>

namespace scratchrobin {

// MariaDB Connection Parameters Implementation

bool MariaDBConnectionParameters::validateParameters(QString& errorMessage) const {
    // Validate host
    if (host.isEmpty() && unixSocket.isEmpty() && namedPipe.isEmpty()) {
        errorMessage = "Either host, Unix socket, or named pipe must be specified";
        return false;
    }

    // Validate port
    if (!unixSocket.isEmpty() && !namedPipe.isEmpty()) {
        // For local connections, port is not used
    } else if (port < 1 || port > 65535) {
        errorMessage = "Port must be between 1 and 65535";
        return false;
    }

    // Validate authentication
    if (username.isEmpty()) {
        errorMessage = "Username is required";
        return false;
    }

    if (password.isEmpty()) {
        errorMessage = "Password is required";
        return false;
    }

    // Validate SSL parameters
    if (useSSL) {
        if (!sslCA.isEmpty() && !QFile::exists(sslCA)) {
            errorMessage = "SSL CA certificate file does not exist";
            return false;
        }
        if (!sslCert.isEmpty() && !QFile::exists(sslCert)) {
            errorMessage = "SSL client certificate file does not exist";
            return false;
        }
        if (!sslKey.isEmpty() && !QFile::exists(sslKey)) {
            errorMessage = "SSL client key file does not exist";
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

    // Validate character set
    if (charset.isEmpty()) {
        errorMessage = "Character set cannot be empty";
        return false;
    }

    return true;
}

QString MariaDBConnectionParameters::generateConnectionString() const {
    QStringList parts;

    // Basic connection parameters
    if (!unixSocket.isEmpty()) {
        parts << QString("unix_socket=%1").arg(unixSocket);
    } else if (useNamedPipe && !namedPipe.isEmpty()) {
        parts << QString("named_pipe=%1").arg(namedPipe);
    } else if (!host.isEmpty()) {
        parts << QString("host=%1").arg(host);
        if (port != 3306) {
            parts << QString("port=%1").arg(port);
        }
    }

    if (!database.isEmpty()) {
        parts << QString("database=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    // Character set and collation
    if (!charset.isEmpty()) {
        parts << QString("charset=%1").arg(charset);
    }

    if (!collation.isEmpty()) {
        parts << QString("collation=%1").arg(collation);
    }

    // SSL parameters
    if (useSSL) {
        parts << "ssl=1";
        if (!sslCA.isEmpty()) {
            parts << QString("ssl_ca=%1").arg(sslCA);
        }
        if (!sslCert.isEmpty()) {
            parts << QString("ssl_cert=%1").arg(sslCert);
        }
        if (!sslKey.isEmpty()) {
            parts << QString("ssl_key=%1").arg(sslKey);
        }
        if (!sslCipher.isEmpty()) {
            parts << QString("ssl_cipher=%1").arg(sslCipher);
        }
    }

    // Connection options
    if (compress) {
        parts << "compress=1";
    }

    if (timeout > 0) {
        parts << QString("connect_timeout=%1").arg(timeout);
    }

    if (commandTimeout > 0) {
        parts << QString("read_timeout=%1").arg(commandTimeout);
    }

    if (!initCommand.isEmpty()) {
        parts << QString("init_command=%1").arg(initCommand);
    }

    if (!applicationName.isEmpty()) {
        parts << QString("program_name=%1").arg(applicationName);
    }

    if (autoReconnect) {
        parts << "auto_reconnect=1";
    }

    if (allowLocalInfile) {
        parts << "allow_local_infile=1";
    }

    if (allowMultipleStatements) {
        parts << "allow_multiple_statements=1";
    }

    if (maxAllowedPacket > 0) {
        parts << QString("max_allowed_packet=%1").arg(maxAllowedPacket);
    }

    // Add any additional parameters
    for (auto it = additionalParams.begin(); it != additionalParams.end(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }

    return parts.join(";");
}

QString MariaDBConnectionParameters::generateODBCConnectionString() const {
    QStringList parts;

    // Basic connection parameters
    if (!host.isEmpty()) {
        parts << QString("SERVER=%1").arg(host);
        if (port != 3306) {
            parts << QString("PORT=%1").arg(port);
        }
    }

    if (!database.isEmpty()) {
        parts << QString("DATABASE=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("UID=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("PWD=%1").arg(password);
    }

    // Character set
    if (!charset.isEmpty()) {
        parts << QString("CHARSET=%1").arg(charset);
    }

    // SSL parameters
    if (useSSL) {
        parts << "SSL=1";
        if (!sslCA.isEmpty()) {
            parts << QString("SSL_CA=%1").arg(sslCA);
        }
        if (!sslCert.isEmpty()) {
            parts << QString("SSL_CERT=%1").arg(sslCert);
        }
        if (!sslKey.isEmpty()) {
            parts << QString("SSL_KEY=%1").arg(sslKey);
        }
    }

    return parts.join(";");
}

MariaDBConnectionParameters MariaDBConnectionParameters::fromConnectionString(const QString& connectionString) {
    MariaDBConnectionParameters params;
    QStringList pairs = connectionString.split(";", Qt::SkipEmptyParts);

    for (const QString& pair : pairs) {
        QStringList keyValue = pair.split("=");
        if (keyValue.size() == 2) {
            QString key = keyValue[0].trimmed().toLower();
            QString value = keyValue[1].trimmed();

            if (key == "host" || key == "server") {
                params.host = value;
            } else if (key == "port") {
                params.port = value.toInt();
            } else if (key == "database" || key == "dbname") {
                params.database = value;
            } else if (key == "user" || key == "username") {
                params.username = value;
            } else if (key == "password" || key == "pwd") {
                params.password = value;
            } else if (key == "unix_socket") {
                params.unixSocket = value;
            } else if (key == "charset") {
                params.charset = value;
            } else if (key == "collation") {
                params.collation = value;
            } else if (key == "ssl" || key == "usessl") {
                params.useSSL = (value == "1" || value.toLower() == "true");
            } else if (key == "ssl_ca") {
                params.sslCA = value;
            } else if (key == "ssl_cert") {
                params.sslCert = value;
            } else if (key == "ssl_key") {
                params.sslKey = value;
            } else if (key == "compress") {
                params.compress = (value == "1" || value.toLower() == "true");
            } else if (key == "connect_timeout") {
                params.timeout = value.toInt();
            } else if (key == "read_timeout") {
                params.commandTimeout = value.toInt();
            } else if (key == "init_command") {
                params.initCommand = value;
            } else if (key == "program_name") {
                params.applicationName = value;
            } else if (key == "auto_reconnect") {
                params.autoReconnect = (value == "1" || value.toLower() == "true");
            } else if (key == "allow_local_infile") {
                params.allowLocalInfile = (value == "1" || value.toLower() == "true");
            } else if (key == "allow_multiple_statements") {
                params.allowMultipleStatements = (value == "1" || value.toLower() == "true");
            } else if (key == "max_allowed_packet") {
                params.maxAllowedPacket = value.toInt();
            } else {
                // Store additional parameters
                params.additionalParams[keyValue[0].trimmed()] = value;
            }
        }
    }

    return params;
}

// MariaDB Connection Tester Implementation

bool MariaDBConnectionTester::testBasicConnection(const MariaDBConnectionParameters& params, QString& errorMessage) {
    QString validationError;
    if (!params.validateParameters(validationError)) {
        errorMessage = QString("Parameter validation failed: %1").arg(validationError);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_basic");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    // Set additional connection options
    if (params.useSSL) {
        db.setConnectOptions("SSL_CA=" + params.sslCA +
                           ";SSL_CERT=" + params.sslCert +
                           ";SSL_KEY=" + params.sslKey);
    }

    if (params.compress) {
        db.setConnectOptions("CLIENT_COMPRESS=1");
    }

    if (!params.charset.isEmpty()) {
        db.setConnectOptions("CLIENT_CHARSET=" + params.charset);
    }

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_basic");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_basic");
    return true;
}

bool MariaDBConnectionTester::testDatabaseAccess(const MariaDBConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_db_access");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_db_access");
        return false;
    }

    QSqlQuery query(db);

    // Test basic queries
    if (!query.exec("SELECT VERSION()")) {
        errorMessage = QString("Version query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_db_access");
        return false;
    }

    // Test database selection if specified
    if (!params.database.isEmpty()) {
        if (!query.exec(QString("USE `%1`").arg(params.database))) {
            errorMessage = QString("Database selection failed: %1").arg(query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mariadb_db_access");
            return false;
        }
    }

    // Test information_schema access
    if (!query.exec("SELECT COUNT(*) FROM information_schema.tables")) {
        errorMessage = QString("Information schema access failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_db_access");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_db_access");
    return true;
}

bool MariaDBConnectionTester::testPermissions(const MariaDBConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_permissions");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_permissions");
        return false;
    }

    QSqlQuery query(db);
    QStringList testQueries = {
        "SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema'",
        "SELECT * FROM information_schema.columns WHERE table_schema = 'information_schema'",
        "SHOW DATABASES",
        "SHOW TABLES"
    };

    for (const QString& sql : testQueries) {
        if (!query.exec(sql)) {
            errorMessage = QString("Permission test failed for query '%1': %2").arg(sql, query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mariadb_permissions");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_permissions");
    return true;
}

bool MariaDBConnectionTester::testServerFeatures(const MariaDBConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_features");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_features");
        return false;
    }

    QSqlQuery query(db);

    // Test basic features
    if (query.exec("SELECT VERSION()")) {
        supportedFeatures << "BASIC_CONNECTIVITY";
    }

    // Test JSON support (MariaDB 10.2+)
    if (query.exec("SELECT JSON_EXTRACT('{\"key\": \"value\"}', '$.key')")) {
        supportedFeatures << "JSON_SUPPORT";
    }

    // Test window functions (MariaDB 10.2+)
    if (query.exec("SELECT id, ROW_NUMBER() OVER (ORDER BY id) FROM information_schema.tables LIMIT 1")) {
        supportedFeatures << "WINDOW_FUNCTIONS";
    }

    // Test CTE (MariaDB 10.2+)
    if (query.exec("WITH cte AS (SELECT 1 as n) SELECT * FROM cte")) {
        supportedFeatures << "CTE_SUPPORT";
    }

    // Test sequences (MariaDB 10.3+)
    if (query.exec("CREATE SEQUENCE IF NOT EXISTS test_seq; DROP SEQUENCE test_seq")) {
        supportedFeatures << "SEQUENCES";
    }

    // Test spatial data
    if (query.exec("SELECT ST_AsText(ST_GeomFromText('POINT(0 0)'))")) {
        supportedFeatures << "SPATIAL_SUPPORT";
    }

    // Test partitioning
    if (query.exec("SELECT * FROM information_schema.partitions LIMIT 1")) {
        supportedFeatures << "PARTITIONING";
    }

    // Test performance schema
    if (query.exec("SELECT * FROM performance_schema.global_status LIMIT 1")) {
        supportedFeatures << "PERFORMANCE_SCHEMA";
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_features");
    return true;
}

bool MariaDBConnectionTester::testReplication(const MariaDBConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_replication");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_replication");
        return false;
    }

    QSqlQuery query(db);

    // Test replication status
    if (!query.exec("SHOW SLAVE STATUS")) {
        errorMessage = QString("Replication status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_replication");
        return false;
    }

    // Test binary log status
    if (!query.exec("SHOW BINARY LOGS")) {
        errorMessage = QString("Binary log status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_replication");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_replication");
    return true;
}

bool MariaDBConnectionTester::testSSLConnection(const MariaDBConnectionParameters& params, QString& errorMessage) {
    MariaDBConnectionParameters testParams = params;
    testParams.useSSL = true;

    return testBasicConnection(testParams, errorMessage);
}

bool MariaDBConnectionTester::testPerformance(const MariaDBConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_performance");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    QElapsedTimer timer;
    timer.start();

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_performance");
        return false;
    }

    qint64 connectionTime = timer.elapsed();

    QSqlQuery query(db);

    // Test simple query performance
    timer.restart();
    if (!query.exec("SELECT @@VERSION")) {
        errorMessage = "Simple query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_performance");
        return false;
    }
    qint64 simpleQueryTime = timer.elapsed();

    // Test more complex query
    timer.restart();
    if (!query.exec("SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema' LIMIT 100")) {
        errorMessage = "Complex query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_performance");
        return false;
    }
    qint64 complexQueryTime = timer.elapsed();

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_performance");

    // Store metrics
    metrics["connection_time_ms"] = connectionTime;
    metrics["simple_query_time_ms"] = simpleQueryTime;
    metrics["complex_query_time_ms"] = complexQueryTime;

    return true;
}

bool MariaDBConnectionTester::testStorageEngines(const MariaDBConnectionParameters& params, QStringList& engines, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mariadb_engines");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mariadb_engines");
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("SHOW STORAGE ENGINES")) {
        errorMessage = QString("Storage engines query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mariadb_engines");
        return false;
    }

    while (query.next()) {
        QString engine = query.value(0).toString();
        QString support = query.value(1).toString();

        if (support == "YES" || support == "DEFAULT") {
            engines << engine;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mariadb_engines");
    return true;
}

// MariaDB Connection Manager Implementation

std::unique_ptr<MariaDBConnectionManager> MariaDBConnectionManager::instance_ = nullptr;

MariaDBConnectionManager& MariaDBConnectionManager::instance() {
    if (!instance_) {
        instance_.reset(new MariaDBConnectionManager());
    }
    return *instance_;
}

bool MariaDBConnectionManager::connect(const MariaDBConnectionParameters& params, QString& errorMessage) {
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

bool MariaDBConnectionManager::disconnect() {
    if (database_.isOpen()) {
        database_.close();
    }
    if (!database_.connectionName().isEmpty()) {
        QSqlDatabase::removeDatabase(database_.connectionName());
    }
    return true;
}

bool MariaDBConnectionManager::isConnected() const {
    return database_.isValid() && database_.isOpen();
}

QSqlDatabase MariaDBConnectionManager::getDatabase() {
    return database_;
}

bool MariaDBConnectionManager::getServerInfo(MariaDBServerInfo& info, QString& errorMessage) {
    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT VERSION() as version_string, @@version_comment as version_comment, "
                   "@@version_compile_machine as compile_machine, @@version_compile_os as compile_os, "
                   "@@hostname as hostname, @@port as port, @@socket as socket, "
                   "@@basedir as basedir, @@datadir as datadir, @@tmpdir as tmpdir, "
                   "@@character_set_server as server_charset, @@collation_server as server_collation, "
                   "@@time_zone as time_zone, @@system_time_zone as system_time_zone, "
                   "@@max_connections as max_connections, @@max_user_connections as max_user_connections, "
                   "@@have_ssl as ssl_support, @@have_openssl as openssl_support")) {
        errorMessage = QString("Failed to get server info: %1").arg(query.lastError().text());
        return false;
    }

    if (query.next()) {
        info.version = query.value("version_string").toString();
        info.versionComment = query.value("version_comment").toString();
        info.compileMachine = query.value("compile_machine").toString();
        info.compileOS = query.value("compile_os").toString();
        info.hostname = query.value("hostname").toString();
        info.port = query.value("port").toInt();
        info.socket = query.value("socket").toString();
        info.basedir = query.value("basedir").toString();
        info.datadir = query.value("datadir").toString();
        info.tmpdir = query.value("tmpdir").toString();
        info.serverCharset = query.value("server_charset").toString();
        info.serverCollation = query.value("server_collation").toString();
        info.timeZone = query.value("time_zone").toString();
        info.systemTimeZone = query.value("system_time_zone").toString();
        info.maxConnections = query.value("max_connections").toInt();
        info.maxUserConnections = query.value("max_user_connections").toInt();
        info.supportsSSL = (query.value("ssl_support").toString() == "YES");
        info.supportsSSL = info.supportsSSL || (query.value("openssl_support").toString() == "YES");

        // Parse version number
        QStringList versionParts = info.version.split(".");
        if (versionParts.size() >= 3) {
            info.majorVersion = versionParts[0].toInt();
            info.minorVersion = versionParts[1].toInt();
            info.patchVersion = versionParts[2].toInt();
        }

        serverInfo_ = info;
        return true;
    }

    errorMessage = "No server information returned";
    return false;
}

QStringList MariaDBConnectionManager::getAvailableDatabases(QString& errorMessage) {
    QStringList databases;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return databases;
    }

    QSqlQuery query(database_);
    if (!query.exec("SHOW DATABASES")) {
        errorMessage = QString("Failed to get databases: %1").arg(query.lastError().text());
        return databases;
    }

    while (query.next()) {
        QString dbName = query.value(0).toString();
        if (dbName != "information_schema" && dbName != "mysql" &&
            dbName != "performance_schema" && dbName != "sys") {
            databases << dbName;
        }
    }

    return databases;
}

QStringList MariaDBConnectionManager::getDatabaseSchemas(const QString& database, QString& errorMessage) {
    QStringList schemas;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return schemas;
    }

    QSqlQuery query(database_);
    QString sql = QString("USE `%1`; SHOW TABLES").arg(database);

    if (!query.exec(sql)) {
        errorMessage = QString("Failed to get tables for database %1: %2").arg(database, query.lastError().text());
        return schemas;
    }

    while (query.next()) {
        schemas << query.value(0).toString();
    }

    return schemas;
}

QStringList MariaDBConnectionManager::getStorageEngines(QString& errorMessage) {
    QStringList engines;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return engines;
    }

    QSqlQuery query(database_);
    if (!query.exec("SHOW STORAGE ENGINES")) {
        errorMessage = QString("Failed to get storage engines: %1").arg(query.lastError().text());
        return engines;
    }

    while (query.next()) {
        QString engine = query.value(0).toString();
        QString support = query.value(1).toString();

        if (support == "YES" || support == "DEFAULT") {
            engines << engine;
        }
    }

    return engines;
}

QStringList MariaDBConnectionManager::getAvailablePlugins(QString& errorMessage) {
    QStringList plugins;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return plugins;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT plugin_name FROM information_schema.plugins ORDER BY plugin_name")) {
        errorMessage = QString("Failed to get plugins: %1").arg(query.lastError().text());
        return plugins;
    }

    while (query.next()) {
        plugins << query.value(0).toString();
    }

    return plugins;
}

bool MariaDBConnectionManager::detectServerCapabilities(MariaDBServerInfo& info, QString& errorMessage) {
    if (!getServerInfo(info, errorMessage)) {
        return false;
    }

    // Detect feature support based on version
    info.supportsJSON = info.isVersionAtLeast(10, 2);
    info.supportsSequences = info.isVersionAtLeast(10, 3);
    info.supportsVirtualColumns = info.isVersionAtLeast(5, 2);
    info.supportsDynamicColumns = info.isVersionAtLeast(5, 5);
    info.supportsWindowFunctions = info.isVersionAtLeast(10, 2);
    info.supportsCTEs = info.isVersionAtLeast(10, 2);
    info.supportsSpatial = info.isVersionAtLeast(5, 5);
    info.supportsPartitioning = info.isVersionAtLeast(5, 1);
    info.supportsGTID = info.isVersionAtLeast(10, 0);
    info.supportsPerformanceSchema = info.isVersionAtLeast(5, 5);
    info.supportsReplication = true; // Available in all versions

    return true;
}

QStringList MariaDBConnectionManager::getSupportedFeatures() const {
    QStringList features;

    if (serverInfo_.supportsJSON) features << "JSON";
    if (serverInfo_.supportsSequences) features << "SEQUENCES";
    if (serverInfo_.supportsVirtualColumns) features << "VIRTUAL_COLUMNS";
    if (serverInfo_.supportsDynamicColumns) features << "DYNAMIC_COLUMNS";
    if (serverInfo_.supportsWindowFunctions) features << "WINDOW_FUNCTIONS";
    if (serverInfo_.supportsCTEs) features << "CTE";
    if (serverInfo_.supportsSpatial) features << "SPATIAL";
    if (serverInfo_.supportsPartitioning) features << "PARTITIONING";
    if (serverInfo_.supportsGTID) features << "GTID";
    if (serverInfo_.supportsPerformanceSchema) features << "PERFORMANCE_SCHEMA";
    if (serverInfo_.supportsReplication) features << "REPLICATION";
    if (serverInfo_.supportsSSL) features << "SSL";

    return features;
}

QString MariaDBConnectionManager::getConnectionStatus() const {
    if (!isConnected()) {
        return "Disconnected";
    }

    return QString("Connected to %1:%2").arg(currentParams_.host, QString::number(currentParams_.port));
}

QString MariaDBConnectionManager::getLastError() const {
    return lastError_;
}

bool MariaDBConnectionManager::testConnection(QString& errorMessage) {
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

void MariaDBConnectionManager::setConnectionTimeout(int seconds) {
    currentParams_.timeout = seconds;
    // Reconnect with new timeout if connected
    if (isConnected()) {
        QString error;
        connect(currentParams_, error);
    }
}

void MariaDBConnectionManager::setCommandTimeout(int seconds) {
    currentParams_.commandTimeout = seconds;
}

void MariaDBConnectionManager::enableConnectionPooling(bool enable) {
    currentParams_.connectionPooling = enable;
}

void MariaDBConnectionManager::setPoolSize(int minSize, int maxSize) {
    currentParams_.minPoolSize = minSize;
    currentParams_.maxPoolSize = maxSize;
}

bool MariaDBConnectionManager::configureSSL(const QString& caCert, const QString& clientCert,
                                           const QString& clientKey, QString& errorMessage) {
    currentParams_.useSSL = true;
    currentParams_.sslCA = caCert;
    currentParams_.sslCert = clientCert;
    currentParams_.sslKey = clientKey;

    return MariaDBSSLHelper::validateCertificate(caCert, clientCert, clientKey, errorMessage);
}

bool MariaDBConnectionManager::initializeDatabase(const MariaDBConnectionParameters& params) {
    QString connectionName = QString("mariadb_connection_%1").arg(QDateTime::currentMSecsSinceEpoch());
    database_ = QSqlDatabase::addDatabase("QMYSQL", connectionName);
    database_.setHostName(params.host);
    database_.setPort(params.port);
    database_.setDatabaseName(params.database);
    database_.setUserName(params.username);
    database_.setPassword(params.password);

    // Set additional connection options
    QStringList options;

    if (!params.charset.isEmpty()) {
        database_.setConnectOptions("CLIENT_CHARSET=" + params.charset);
    }

    if (params.useSSL) {
        QString sslOptions = "SSL_CA=" + params.sslCA;
        if (!params.sslCert.isEmpty()) {
            sslOptions += ";SSL_CERT=" + params.sslCert;
        }
        if (!params.sslKey.isEmpty()) {
            sslOptions += ";SSL_KEY=" + params.sslKey;
        }
        database_.setConnectOptions(sslOptions);
    }

    if (params.compress) {
        database_.setConnectOptions("CLIENT_COMPRESS=1");
    }

    if (params.autoReconnect) {
        database_.setConnectOptions("CLIENT_RECONNECT=1");
    }

    return database_.isValid();
}

bool MariaDBConnectionManager::configureDatabase(const MariaDBConnectionParameters& params) {
    // Additional configuration can be done here
    if (!params.database.isEmpty()) {
        // The database is already specified in the connection
        // No additional configuration needed
    }

    return true;
}

// MariaDB Server Info Implementation

QString MariaDBServerInfo::getFullVersionString() const {
    return QString("%1.%2.%3").arg(majorVersion).arg(minorVersion).arg(patchVersion);
}

bool MariaDBServerInfo::isVersionAtLeast(int major, int minor, int patch) const {
    if (majorVersion > major) return true;
    if (majorVersion < major) return false;
    if (minorVersion > minor) return true;
    if (minorVersion < minor) return false;
    return patchVersion >= patch;
}

// MariaDB Authentication Helper Implementation

QStringList MariaDBAuthenticationHelper::getAvailableAuthenticationMethods() {
    QStringList methods;
    methods << "MySQL Native Authentication" << "SSL Authentication";
    return methods;
}

bool MariaDBAuthenticationHelper::isSSLSupported() {
    return true; // SSL is supported in MariaDB
}

bool MariaDBAuthenticationHelper::isCompressionSupported() {
    return true; // Compression is supported in MariaDB
}

bool MariaDBAuthenticationHelper::validateCredentials(const QString& host, int port,
                                                     const QString& username, const QString& password,
                                                     QString& errorMessage) {
    MariaDBConnectionParameters params;
    params.host = host;
    params.port = port;
    params.username = username;
    params.password = password;

    return MariaDBConnectionTester::testBasicConnection(params, errorMessage);
}

bool MariaDBAuthenticationHelper::validateSSLConnection(const QString& host, int port,
                                                       const QString& caCert, const QString& clientCert,
                                                       const QString& clientKey, QString& errorMessage) {
    MariaDBConnectionParameters params;
    params.host = host;
    params.port = port;
    params.useSSL = true;
    params.sslCA = caCert;
    params.sslCert = clientCert;
    params.sslKey = clientKey;

    return MariaDBConnectionTester::testSSLConnection(params, errorMessage);
}

QString MariaDBAuthenticationHelper::generateSecurePassword(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;

    for (int i = 0; i < length; ++i) {
        int index = qrand() % chars.length();
        password.append(chars.at(index));
    }

    return password;
}

bool MariaDBAuthenticationHelper::isPasswordStrong(const QString& password) {
    if (password.length() < 8) return false;
    if (!password.contains(QRegularExpression("[A-Z]"))) return false;
    if (!password.contains(QRegularExpression("[a-z]"))) return false;
    if (!password.contains(QRegularExpression("[0-9]"))) return false;
    if (!password.contains(QRegularExpression("[!@#$%^&*]"))) return false;
    return true;
}

QString MariaDBAuthenticationHelper::buildStandardConnectionString(const QString& host, int port,
                                                                  const QString& database,
                                                                  const QString& username,
                                                                  const QString& password) {
    QStringList parts;
    parts << QString("host=%1").arg(host);
    parts << QString("port=%1").arg(port);

    if (!database.isEmpty()) {
        parts << QString("database=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    return parts.join(";");
}

QString MariaDBAuthenticationHelper::buildSSLConnectionString(const QString& host, int port,
                                                             const QString& database,
                                                             const QString& username,
                                                             const QString& password,
                                                             const QString& caCert,
                                                             const QString& clientCert,
                                                             const QString& clientKey) {
    QStringList parts;
    parts << QString("host=%1").arg(host);
    parts << QString("port=%1").arg(port);

    if (!database.isEmpty()) {
        parts << QString("database=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    parts << "ssl=1";

    if (!caCert.isEmpty()) {
        parts << QString("ssl_ca=%1").arg(caCert);
    }

    if (!clientCert.isEmpty()) {
        parts << QString("ssl_cert=%1").arg(clientCert);
    }

    if (!clientKey.isEmpty()) {
        parts << QString("ssl_key=%1").arg(clientKey);
    }

    return parts.join(";");
}

QString MariaDBAuthenticationHelper::buildSocketConnectionString(const QString& socketPath,
                                                                const QString& database,
                                                                const QString& username,
                                                                const QString& password) {
    QStringList parts;
    parts << QString("unix_socket=%1").arg(socketPath);

    if (!database.isEmpty()) {
        parts << QString("database=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    return parts.join(";");
}

MariaDBConnectionManager::~MariaDBConnectionManager() {
    disconnect();
}

} // namespace scratchrobin
