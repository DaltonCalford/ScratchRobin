#include "mysql_connection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QProcess>
#include <QDir>
#include <QDebug>

namespace scratchrobin {

// MySQL Connection Parameters Implementation

bool MySQLConnectionParameters::validateParameters(QString& errorMessage) const {
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

QString MySQLConnectionParameters::generateConnectionString() const {
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

    // MySQL-specific options
    if (usePerformanceSchema) {
        parts << "use_performance_schema=1";
    }

    if (useSysSchema) {
        parts << "use_sys_schema=1";
    }

    if (useMySQLX) {
        parts << QString("mysqlx_port=%1").arg(mysqlxPort);
    }

    // Add any additional parameters
    for (auto it = additionalParams.begin(); it != additionalParams.end(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }

    return parts.join(";");
}

QString MySQLConnectionParameters::generateODBCConnectionString() const {
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

MySQLConnectionParameters MySQLConnectionParameters::fromConnectionString(const QString& connectionString) {
    MySQLConnectionParameters params;
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
            } else if (key == "use_performance_schema") {
                params.usePerformanceSchema = (value == "1" || value.toLower() == "true");
            } else if (key == "use_sys_schema") {
                params.useSysSchema = (value == "1" || value.toLower() == "true");
            } else if (key == "mysqlx_port") {
                params.mysqlxPort = value.toInt();
            } else {
                // Store additional parameters
                params.additionalParams[keyValue[0].trimmed()] = value;
            }
        }
    }

    return params;
}

// MySQL Connection Tester Implementation

bool MySQLConnectionTester::testBasicConnection(const MySQLConnectionParameters& params, QString& errorMessage) {
    QString validationError;
    if (!params.validateParameters(validationError)) {
        errorMessage = QString("Parameter validation failed: %1").arg(validationError);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_basic");
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
        QSqlDatabase::removeDatabase("test_mysql_basic");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_basic");
    return true;
}

bool MySQLConnectionTester::testDatabaseAccess(const MySQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_db_access");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_db_access");
        return false;
    }

    QSqlQuery query(db);

    // Test basic queries
    if (!query.exec("SELECT VERSION()")) {
        errorMessage = QString("Version query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_db_access");
        return false;
    }

    // Test database selection if specified
    if (!params.database.isEmpty()) {
        if (!query.exec(QString("USE `%1`").arg(params.database))) {
            errorMessage = QString("Database selection failed: %1").arg(query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mysql_db_access");
            return false;
        }
    }

    // Test information_schema access
    if (!query.exec("SELECT COUNT(*) FROM information_schema.tables")) {
        errorMessage = QString("Information schema access failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_db_access");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_db_access");
    return true;
}

bool MySQLConnectionTester::testPermissions(const MySQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_permissions");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_permissions");
        return false;
    }

    QSqlQuery query(db);
    QStringList testQueries = {
        "SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema'",
        "SELECT * FROM information_schema.columns WHERE table_schema = 'information_schema'",
        "SHOW DATABASES",
        "SHOW TABLES",
        "SHOW GLOBAL VARIABLES"
    };

    for (const QString& sql : testQueries) {
        if (!query.exec(sql)) {
            errorMessage = QString("Permission test failed for query '%1': %2").arg(sql, query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_mysql_permissions");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_permissions");
    return true;
}

bool MySQLConnectionTester::testServerFeatures(const MySQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_features");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_features");
        return false;
    }

    QSqlQuery query(db);

    // Test basic features
    if (query.exec("SELECT VERSION()")) {
        supportedFeatures << "BASIC_CONNECTIVITY";
    }

    // Test JSON support (MySQL 5.7+)
    if (query.exec("SELECT JSON_EXTRACT('{\"key\": \"value\"}', '$.key')")) {
        supportedFeatures << "JSON_SUPPORT";
    }

    // Test window functions (MySQL 8.0+)
    if (query.exec("SELECT id, ROW_NUMBER() OVER (ORDER BY id) FROM information_schema.tables LIMIT 1")) {
        supportedFeatures << "WINDOW_FUNCTIONS";
    }

    // Test CTE (MySQL 8.0+)
    if (query.exec("WITH cte AS (SELECT 1 as n) SELECT * FROM cte")) {
        supportedFeatures << "CTE_SUPPORT";
    }

    // Test Performance Schema (MySQL 5.5+)
    if (query.exec("SELECT * FROM performance_schema.global_status LIMIT 1")) {
        supportedFeatures << "PERFORMANCE_SCHEMA";
    }

    // Test sys schema (MySQL 5.7+)
    if (query.exec("SELECT * FROM sys.version")) {
        supportedFeatures << "SYS_SCHEMA";
    }

    // Test spatial data
    if (query.exec("SELECT ST_AsText(ST_GeomFromText('POINT(0 0)'))")) {
        supportedFeatures << "SPATIAL_SUPPORT";
    }

    // Test partitioning
    if (query.exec("SELECT * FROM information_schema.partitions LIMIT 1")) {
        supportedFeatures << "PARTITIONING";
    }

    // Test full-text search
    if (query.exec("SELECT * FROM information_schema.statistics WHERE index_type = 'FULLTEXT' LIMIT 1")) {
        supportedFeatures << "FULLTEXT_SEARCH";
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_features");
    return true;
}

bool MySQLConnectionTester::testReplication(const MySQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_replication");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_replication");
        return false;
    }

    QSqlQuery query(db);

    // Test replication status
    if (!query.exec("SHOW SLAVE STATUS")) {
        errorMessage = QString("Replication status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_replication");
        return false;
    }

    // Test binary log status
    if (!query.exec("SHOW BINARY LOGS")) {
        errorMessage = QString("Binary log status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_replication");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_replication");
    return true;
}

bool MySQLConnectionTester::testSSLConnection(const MySQLConnectionParameters& params, QString& errorMessage) {
    MySQLConnectionParameters testParams = params;
    testParams.useSSL = true;

    return testBasicConnection(testParams, errorMessage);
}

bool MySQLConnectionTester::testPerformance(const MySQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_performance");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    QElapsedTimer timer;
    timer.start();

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_performance");
        return false;
    }

    qint64 connectionTime = timer.elapsed();

    QSqlQuery query(db);

    // Test simple query performance
    timer.restart();
    if (!query.exec("SELECT @@VERSION")) {
        errorMessage = "Simple query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_performance");
        return false;
    }
    qint64 simpleQueryTime = timer.elapsed();

    // Test more complex query
    timer.restart();
    if (!query.exec("SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema' LIMIT 100")) {
        errorMessage = "Complex query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_performance");
        return false;
    }
    qint64 complexQueryTime = timer.elapsed();

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_performance");

    // Store metrics
    metrics["connection_time_ms"] = connectionTime;
    metrics["simple_query_time_ms"] = simpleQueryTime;
    metrics["complex_query_time_ms"] = complexQueryTime;

    return true;
}

bool MySQLConnectionTester::testStorageEngines(const MySQLConnectionParameters& params, QStringList& engines, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_engines");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_engines");
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("SHOW STORAGE ENGINES")) {
        errorMessage = QString("Storage engines query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_engines");
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
    QSqlDatabase::removeDatabase("test_mysql_engines");
    return true;
}

bool MySQLConnectionTester::testMySQLVersion(const MySQLConnectionParameters& params, QString& version, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_version");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_version");
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("SELECT VERSION()")) {
        errorMessage = "Version query failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_version");
        return false;
    }

    if (query.next()) {
        version = query.value(0).toString();
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_version");
    return true;
}

bool MySQLConnectionTester::testPerformanceSchema(const MySQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_performance_schema");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_performance_schema");
        return false;
    }

    QSqlQuery query(db);

    // Test Performance Schema access
    if (!query.exec("SELECT * FROM performance_schema.global_status LIMIT 1")) {
        errorMessage = "Performance Schema access failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_performance_schema");
        return false;
    }

    // Test some Performance Schema tables
    QStringList perfTables = {
        "performance_schema.events_statements_current",
        "performance_schema.events_statements_summary_by_digest",
        "performance_schema.table_io_waits_summary_by_table"
    };

    for (const QString& table : perfTables) {
        if (!query.exec(QString("SELECT COUNT(*) FROM %1 LIMIT 1").arg(table))) {
            errorMessage = QString("Performance Schema table %1 access failed").arg(table);
            db.close();
            QSqlDatabase::removeDatabase("test_mysql_performance_schema");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_performance_schema");
    return true;
}

bool MySQLConnectionTester::testSysSchema(const MySQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_sys_schema");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_sys_schema");
        return false;
    }

    QSqlQuery query(db);

    // Test sys schema access
    if (!query.exec("SELECT * FROM sys.version")) {
        errorMessage = "sys schema access failed";
        db.close();
        QSqlDatabase::removeDatabase("test_mysql_sys_schema");
        return false;
    }

    // Test some sys schema views
    QStringList sysViews = {
        "sys.host_summary",
        "sys.user_summary",
        "sys.statement_analysis"
    };

    for (const QString& view : sysViews) {
        if (!query.exec(QString("SELECT COUNT(*) FROM %1 LIMIT 1").arg(view))) {
            errorMessage = QString("sys schema view %1 access failed").arg(view);
            db.close();
            QSqlDatabase::removeDatabase("test_mysql_sys_schema");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_sys_schema");
    return true;
}

bool MySQLConnectionTester::testEnterpriseFeatures(const MySQLConnectionParameters& params, QStringList& features, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "test_mysql_enterprise");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_mysql_enterprise");
        return false;
    }

    QSqlQuery query(db);

    // Test Enterprise features
    QStringList enterpriseFeatures = {
        "SELECT * FROM mysql.audit_log", // MySQL Enterprise Audit
        "SELECT * FROM mysql.firewall_users", // MySQL Enterprise Firewall
        "SELECT * FROM mysql.query_rewrite", // MySQL Enterprise Query Rewrite
        "SELECT * FROM mysql.tde_status" // MySQL Enterprise Transparent Data Encryption
    };

    for (const QString& sql : enterpriseFeatures) {
        if (query.exec(sql)) {
            if (sql.contains("audit_log")) {
                features << "ENTERPRISE_AUDIT";
            } else if (sql.contains("firewall_users")) {
                features << "ENTERPRISE_FIREWALL";
            } else if (sql.contains("query_rewrite")) {
                features << "ENTERPRISE_QUERY_REWRITE";
            } else if (sql.contains("tde_status")) {
                features << "ENTERPRISE_TDE";
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_mysql_enterprise");
    return true;
}

// MySQL Connection Manager Implementation

std::unique_ptr<MySQLConnectionManager> MySQLConnectionManager::instance_ = nullptr;

MySQLConnectionManager& MySQLConnectionManager::instance() {
    if (!instance_) {
        instance_.reset(new MySQLConnectionManager());
    }
    return *instance_;
}

bool MySQLConnectionManager::connect(const MySQLConnectionParameters& params, QString& errorMessage) {
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

bool MySQLConnectionManager::disconnect() {
    if (database_.isOpen()) {
        database_.close();
    }
    if (!database_.connectionName().isEmpty()) {
        QSqlDatabase::removeDatabase(database_.connectionName());
    }
    return true;
}

bool MySQLConnectionManager::isConnected() const {
    return database_.isValid() && database_.isOpen();
}

QSqlDatabase MySQLConnectionManager::getDatabase() {
    return database_;
}

bool MySQLConnectionManager::getServerInfo(MySQLServerInfo& info, QString& errorMessage) {
    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT "
                   "VERSION() as version_string, "
                   "@@version_comment as version_comment, "
                   "@@version_compile_machine as compile_machine, "
                   "@@version_compile_os as compile_os, "
                   "@@hostname as hostname, "
                   "@@port as port, "
                   "@@socket as socket, "
                   "@@basedir as basedir, "
                   "@@datadir as datadir, "
                   "@@tmpdir as tmpdir, "
                   "@@character_set_server as server_charset, "
                   "@@collation_server as server_collation, "
                   "@@time_zone as time_zone, "
                   "@@system_time_zone as system_time_zone, "
                   "@@max_connections as max_connections, "
                   "@@max_user_connections as max_user_connections, "
                   "@@have_ssl as ssl_support, "
                   "@@have_openssl as openssl_support")) {
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

        // Check for special versions
        info.isPerconaServer = MySQLVersionHelper::isPerconaServer(info.version);
        info.isMySQLCluster = MySQLVersionHelper::isMySQLCluster(info.version);

        serverInfo_ = info;
        return true;
    }

    errorMessage = "No server information returned";
    return false;
}

QStringList MySQLConnectionManager::getAvailableDatabases(QString& errorMessage) {
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
            dbName != "performance_schema" && dbName != "sys" && dbName != "test") {
            databases << dbName;
        }
    }

    return databases;
}

QStringList MySQLConnectionManager::getDatabaseSchemas(const QString& database, QString& errorMessage) {
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

QStringList MySQLConnectionManager::getStorageEngines(QString& errorMessage) {
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

QStringList MySQLConnectionManager::getAvailablePlugins(QString& errorMessage) {
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

QStringList MySQLConnectionManager::getAvailableCharSets(QString& errorMessage) {
    QStringList charsets;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return charsets;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT character_set_name FROM information_schema.character_sets ORDER BY character_set_name")) {
        errorMessage = QString("Failed to get character sets: %1").arg(query.lastError().text());
        return charsets;
    }

    while (query.next()) {
        charsets << query.value(0).toString();
    }

    return charsets;
}

bool MySQLConnectionManager::detectServerCapabilities(MySQLServerInfo& info, QString& errorMessage) {
    if (!getServerInfo(info, errorMessage)) {
        return false;
    }

    // Detect feature support based on version
    info.supportsJSON = info.isVersionAtLeast(5, 7);
    info.supportsSequences = false; // MySQL doesn't have built-in sequences
    info.supportsVirtualColumns = info.isVersionAtLeast(5, 7);
    info.supportsWindowFunctions = info.isVersionAtLeast(8, 0);
    info.supportsCTEs = info.isVersionAtLeast(8, 0);
    info.supportsSpatial = info.isVersionAtLeast(5, 5);
    info.supportsPartitioning = info.isVersionAtLeast(5, 1);
    info.supportsGTID = info.isVersionAtLeast(5, 6);
    info.supportsPerformanceSchema = info.isVersionAtLeast(5, 5);
    info.supportsReplication = true; // Available in all versions
    info.supportsFulltextSearch = info.isVersionAtLeast(3, 23, 23);
    info.supportsInvisibleIndexes = info.isVersionAtLeast(8, 0);
    info.supportsExpressionIndexes = info.isVersionAtLeast(8, 0);
    info.supportsDescendingIndexes = info.isVersionAtLeast(8, 0);
    info.supportsSysSchema = info.isVersionAtLeast(5, 7);
    info.supportsMySQLX = info.isVersionAtLeast(8, 0);
    info.supportsEnterpriseFeatures = info.isMySQL8_0() || info.isPerconaServer;

    return true;
}

QStringList MySQLConnectionManager::getSupportedFeatures() const {
    QStringList features;

    if (serverInfo_.supportsJSON) features << "JSON";
    if (serverInfo_.supportsVirtualColumns) features << "VIRTUAL_COLUMNS";
    if (serverInfo_.supportsWindowFunctions) features << "WINDOW_FUNCTIONS";
    if (serverInfo_.supportsCTEs) features << "CTE";
    if (serverInfo_.supportsSpatial) features << "SPATIAL";
    if (serverInfo_.supportsPartitioning) features << "PARTITIONING";
    if (serverInfo_.supportsGTID) features << "GTID";
    if (serverInfo_.supportsPerformanceSchema) features << "PERFORMANCE_SCHEMA";
    if (serverInfo_.supportsReplication) features << "REPLICATION";
    if (serverInfo_.supportsFulltextSearch) features << "FULLTEXT_SEARCH";
    if (serverInfo_.supportsInvisibleIndexes) features << "INVISIBLE_INDEXES";
    if (serverInfo_.supportsExpressionIndexes) features << "EXPRESSION_INDEXES";
    if (serverInfo_.supportsDescendingIndexes) features << "DESCENDING_INDEXES";
    if (serverInfo_.supportsSysSchema) features << "SYS_SCHEMA";
    if (serverInfo_.supportsMySQLX) features << "MYSQLX";
    if (serverInfo_.supportsEnterpriseFeatures) features << "ENTERPRISE_FEATURES";

    return features;
}

QString MySQLConnectionManager::getConnectionStatus() const {
    if (!isConnected()) {
        return "Disconnected";
    }

    return QString("Connected to %1:%2").arg(currentParams_.host, QString::number(currentParams_.port));
}

QString MySQLConnectionManager::getLastError() const {
    return lastError_;
}

bool MySQLConnectionManager::testConnection(QString& errorMessage) {
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

void MySQLConnectionManager::setConnectionTimeout(int seconds) {
    currentParams_.timeout = seconds;
    // Reconnect with new timeout if connected
    if (isConnected()) {
        QString error;
        connect(currentParams_, error);
    }
}

void MySQLConnectionManager::setCommandTimeout(int seconds) {
    currentParams_.commandTimeout = seconds;
}

void MySQLConnectionManager::enableConnectionPooling(bool enable) {
    currentParams_.connectionPooling = enable;
}

void MySQLConnectionManager::setPoolSize(int minSize, int maxSize) {
    currentParams_.minPoolSize = minSize;
    currentParams_.maxPoolSize = maxSize;
}

bool MySQLConnectionManager::configureSSL(const QString& caCert, const QString& clientCert,
                                         const QString& clientKey, QString& errorMessage) {
    currentParams_.useSSL = true;
    currentParams_.sslCA = caCert;
    currentParams_.sslCert = clientCert;
    currentParams_.sslKey = clientKey;

    return MySQLSSLHelper::validateCertificate(caCert, clientCert, clientKey, errorMessage);
}

void MySQLConnectionManager::enablePerformanceSchema(bool enable) {
    currentParams_.usePerformanceSchema = enable;
}

void MySQLConnectionManager::enableSysSchema(bool enable) {
    currentParams_.useSysSchema = enable;
}

bool MySQLConnectionManager::initializeDatabase(const MySQLConnectionParameters& params) {
    QString connectionName = QString("mysql_connection_%1").arg(QDateTime::currentMSecsSinceEpoch());
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

    if (params.allowLocalInfile) {
        database_.setConnectOptions("CLIENT_LOCAL_FILES=1");
    }

    if (params.allowMultipleStatements) {
        database_.setConnectOptions("CLIENT_MULTI_STATEMENTS=1");
    }

    if (params.usePerformanceSchema) {
        database_.setConnectOptions("CLIENT_PS_MULTI_RESULTS=1");
    }

    return database_.isValid();
}

bool MySQLConnectionManager::configureDatabase(const MySQLConnectionParameters& params) {
    // Additional configuration can be done here
    if (!params.database.isEmpty()) {
        // The database is already specified in the connection
        // No additional configuration needed
    }

    return true;
}

// MySQL Server Info Implementation

QString MySQLServerInfo::getFullVersionString() const {
    return QString("%1.%2.%3").arg(majorVersion).arg(minorVersion).arg(patchVersion);
}

bool MySQLServerInfo::isVersionAtLeast(int major, int minor, int patch) const {
    if (majorVersion > major) return true;
    if (majorVersion < major) return false;
    if (minorVersion > minor) return true;
    if (minorVersion < minor) return false;
    return patchVersion >= patch;
}

bool MySQLServerInfo::isMySQL5_5() const {
    return majorVersion == 5 && minorVersion == 5;
}

bool MySQLServerInfo::isMySQL5_6() const {
    return majorVersion == 5 && minorVersion == 6;
}

bool MySQLServerInfo::isMySQL5_7() const {
    return majorVersion == 5 && minorVersion == 7;
}

bool MySQLServerInfo::isMySQL8_0() const {
    return majorVersion == 8 && minorVersion == 0;
}

bool MySQLServerInfo::isMySQL8_1() const {
    return majorVersion == 8 && minorVersion == 1;
}

// MySQL Authentication Helper Implementation

QStringList MySQLAuthenticationHelper::getAvailableAuthenticationMethods() {
    QStringList methods;
    methods << "MySQL Native Authentication" << "SSL Authentication";
    return methods;
}

bool MySQLAuthenticationHelper::isSSLSupported() {
    return true; // SSL is supported in MySQL
}

bool MySQLAuthenticationHelper::isCompressionSupported() {
    return true; // Compression is supported in MySQL
}

bool MySQLAuthenticationHelper::validateCredentials(const QString& host, int port,
                                                   const QString& username, const QString& password,
                                                   QString& errorMessage) {
    MySQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.username = username;
    params.password = password;

    return MySQLConnectionTester::testBasicConnection(params, errorMessage);
}

bool MySQLAuthenticationHelper::validateSSLConnection(const QString& host, int port,
                                                      const QString& caCert, const QString& clientCert,
                                                      const QString& clientKey, QString& errorMessage) {
    MySQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.useSSL = true;
    params.sslCA = caCert;
    params.sslCert = clientCert;
    params.sslKey = clientKey;

    return MySQLConnectionTester::testSSLConnection(params, errorMessage);
}

QString MySQLAuthenticationHelper::generateSecurePassword(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;

    for (int i = 0; i < length; ++i) {
        int index = qrand() % chars.length();
        password.append(chars.at(index));
    }

    return password;
}

bool MySQLAuthenticationHelper::isPasswordStrong(const QString& password) {
    if (password.length() < 8) return false;
    if (!password.contains(QRegularExpression("[A-Z]"))) return false;
    if (!password.contains(QRegularExpression("[a-z]"))) return false;
    if (!password.contains(QRegularExpression("[0-9]"))) return false;
    if (!password.contains(QRegularExpression("[!@#$%^&*]"))) return false;
    return true;
}

QString MySQLAuthenticationHelper::buildStandardConnectionString(const QString& host, int port,
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

QString MySQLAuthenticationHelper::buildSSLConnectionString(const QString& host, int port,
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

QString MySQLAuthenticationHelper::buildSocketConnectionString(const QString& socketPath,
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

QString MySQLAuthenticationHelper::buildMySQLXConnectionString(const QString& host, int port,
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

    parts << "mysqlx=1";

    return parts.join(";");
}

// MySQL SSL Helper Implementation

bool MySQLSSLHelper::validateCertificate(const QString& caCert, const QString& clientCert,
                                       const QString& clientKey, QString& errorMessage) {
    // Check if files exist and are readable
    if (!caCert.isEmpty() && !QFile::exists(caCert)) {
        errorMessage = "SSL CA certificate file does not exist";
        return false;
    }

    if (!clientCert.isEmpty() && !QFile::exists(clientCert)) {
        errorMessage = "SSL client certificate file does not exist";
        return false;
    }

    if (!clientKey.isEmpty() && !QFile::exists(clientKey)) {
        errorMessage = "SSL client key file does not exist";
        return false;
    }

    // Basic validation - could be enhanced with OpenSSL certificate validation
    return true;
}

QStringList MySQLSSLHelper::getSupportedSSLCiphers() {
    QStringList ciphers;
    ciphers << "AES128-SHA" << "AES256-SHA" << "AES128-SHA256" << "AES256-SHA256"
            << "DHE-RSA-AES128-SHA" << "DHE-RSA-AES256-SHA" << "ECDHE-RSA-AES128-SHA"
            << "ECDHE-RSA-AES256-SHA" << "ECDHE-RSA-AES128-SHA256" << "ECDHE-RSA-AES256-SHA384";
    return ciphers;
}

QString MySQLSSLHelper::getRecommendedSSLCipher() {
    return "ECDHE-RSA-AES256-SHA384";
}

bool MySQLSSLHelper::generateSelfSignedCertificate(const QString& certFile, const QString& keyFile,
                                                  const QString& subject, int days, QString& errorMessage) {
    // This would require OpenSSL integration - for now, return false
    errorMessage = "Certificate generation requires OpenSSL integration";
    return false;
}

bool MySQLSSLHelper::testSSLConnection(const QString& host, int port,
                                     const QString& caCert, const QString& clientCert,
                                     const QString& clientKey, QString& errorMessage) {
    MySQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.useSSL = true;
    params.sslCA = caCert;
    params.sslCert = clientCert;
    params.sslKey = clientKey;

    return MySQLConnectionTester::testSSLConnection(params, errorMessage);
}

// MySQL Version Helper Implementation

bool MySQLVersionHelper::parseVersion(const QString& versionString, int& major, int& minor, int& patch) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(versionString);

    if (match.hasMatch()) {
        major = match.captured(1).toInt();
        minor = match.captured(2).toInt();
        patch = match.captured(3).toInt();
        return true;
    }

    return false;
}

QString MySQLVersionHelper::getVersionFamily(const QString& versionString) {
    int major, minor, patch;
    if (parseVersion(versionString, major, minor, patch)) {
        if (major == 5) {
            if (minor == 5) return "MySQL 5.5";
            if (minor == 6) return "MySQL 5.6";
            if (minor == 7) return "MySQL 5.7";
        } else if (major == 8) {
            if (minor == 0) return "MySQL 8.0";
            if (minor == 1) return "MySQL 8.1";
        }
    }

    return "Unknown";
}

bool MySQLVersionHelper::supportsFeature(const QString& versionString, const QString& feature) {
    int major, minor, patch;
    if (!parseVersion(versionString, major, minor, patch)) {
        return false;
    }

    QString minVersion = getMinimumVersionForFeature(feature);
    if (minVersion.isEmpty()) {
        return true; // Feature supported in all versions
    }

    int minMajor, minMinor, minPatch;
    if (parseVersion(minVersion, minMajor, minMinor, minPatch)) {
        return (major > minMajor) ||
               (major == minMajor && minor > minMinor) ||
               (major == minMajor && minor == minMinor && patch >= minPatch);
    }

    return false;
}

QString MySQLVersionHelper::getMinimumVersionForFeature(const QString& feature) {
    static QMap<QString, QString> featureVersions = {
        {"JSON", "5.7.0"},
        {"CTE", "8.0.0"},
        {"WINDOW_FUNCTIONS", "8.0.0"},
        {"INVISIBLE_INDEXES", "8.0.0"},
        {"EXPRESSION_INDEXES", "8.0.0"},
        {"DESCENDING_INDEXES", "8.0.0"},
        {"PERFORMANCE_SCHEMA", "5.5.0"},
        {"SYS_SCHEMA", "5.7.0"},
        {"MYSQLX", "8.0.0"},
        {"PARTITIONING", "5.1.0"},
        {"FULLTEXT", "3.23.23"}
    };

    return featureVersions.value(feature.toUpper(), "");
}

int MySQLVersionHelper::compareVersions(const QString& version1, const QString& version2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    if (!parseVersion(version1, major1, minor1, patch1) ||
        !parseVersion(version2, major2, minor2, patch2)) {
        return 0; // Can't compare
    }

    if (major1 != major2) return major1 > major2 ? 1 : -1;
    if (minor1 != minor2) return minor1 > minor2 ? 1 : -1;
    if (patch1 != patch2) return patch1 > patch2 ? 1 : -1;

    return 0; // Equal
}

bool MySQLVersionHelper::isVersionInRange(const QString& version, const QString& minVersion, const QString& maxVersion) {
    if (!minVersion.isEmpty() && compareVersions(version, minVersion) < 0) {
        return false;
    }

    if (!maxVersion.isEmpty() && compareVersions(version, maxVersion) > 0) {
        return false;
    }

    return true;
}

bool MySQLVersionHelper::isMySQL5_5(const QString& versionString) {
    return versionString.startsWith("5.5");
}

bool MySQLVersionHelper::isMySQL5_6(const QString& versionString) {
    return versionString.startsWith("5.6");
}

bool MySQLVersionHelper::isMySQL5_7(const QString& versionString) {
    return versionString.startsWith("5.7");
}

bool MySQLVersionHelper::isMySQL8_0(const QString& versionString) {
    return versionString.startsWith("8.0");
}

bool MySQLVersionHelper::isMySQL8_1(const QString& versionString) {
    return versionString.startsWith("8.1");
}

bool MySQLVersionHelper::isPerconaServer(const QString& versionString) {
    return versionString.contains("Percona");
}

bool MySQLVersionHelper::isMySQLCluster(const QString& versionString) {
    return versionString.contains("Cluster");
}

MySQLConnectionManager::~MySQLConnectionManager() {
    disconnect();
}

} // namespace scratchrobin
