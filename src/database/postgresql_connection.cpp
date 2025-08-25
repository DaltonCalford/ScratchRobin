#include "postgresql_connection.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QDateTime>

namespace scratchrobin {

// PostgreSQL Connection Parameters Implementation

bool PostgreSQLConnectionParameters::validateParameters(QString& errorMessage) const {
    // Validate host
    if (host.isEmpty() && !useSSL) {
        errorMessage = "Either host or SSL must be specified";
        return false;
    }

    // Validate port
    if (port < 1 || port > 65535) {
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

        // Validate SSL mode
        QStringList validModes = {"disable", "allow", "prefer", "require", "verify-ca", "verify-full"};
        if (!validModes.contains(sslMode, Qt::CaseInsensitive)) {
            errorMessage = "Invalid SSL mode. Must be one of: " + validModes.join(", ");
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

QString PostgreSQLConnectionParameters::generateConnectionString() const {
    QStringList parts;

    // Basic connection parameters
    if (!host.isEmpty()) {
        parts << QString("host=%1").arg(host);
        if (port != 5432) {
            parts << QString("port=%1").arg(port);
        }
    }

    if (!database.isEmpty()) {
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    // Character set and options
    if (!charset.isEmpty()) {
        parts << QString("client_encoding=%1").arg(charset);
    }

    // SSL parameters
    if (useSSL) {
        parts << QString("sslmode=%1").arg(sslMode);
        if (!sslCA.isEmpty()) {
            parts << QString("sslrootcert=%1").arg(sslCA);
        }
        if (!sslCert.isEmpty()) {
            parts << QString("sslcert=%1").arg(sslCert);
        }
        if (!sslKey.isEmpty()) {
            parts << QString("sslkey=%1").arg(sslKey);
        }
        if (!sslCRL.isEmpty()) {
            parts << QString("sslcrl=%1").arg(sslCRL);
        }
    }

    // Connection options
    if (timeout > 0) {
        parts << QString("connect_timeout=%1").arg(timeout);
    }

    if (!applicationName.isEmpty()) {
        parts << QString("application_name=%1").arg(applicationName);
    }

    if (!searchPath.isEmpty()) {
        parts << QString("search_path=%1").arg(searchPath);
    }

    if (!timezone.isEmpty()) {
        parts << QString("timezone=%1").arg(timezone);
    }

    if (!role.isEmpty()) {
        parts << QString("options=-c role=%1").arg(role);
    }

    if (!options.isEmpty()) {
        parts << QString("options=%1").arg(options);
    }

    if (keepalives) {
        parts << "keepalives=1";
        if (keepalivesIdle > 0) {
            parts << QString("keepalives_idle=%1").arg(keepalivesIdle);
        }
        if (keepalivesInterval > 0) {
            parts << QString("keepalives_interval=%1").arg(keepalivesInterval);
        }
        if (keepalivesCount > 0) {
            parts << QString("keepalives_count=%1").arg(keepalivesCount);
        }
    }

    if (!targetSessionAttrs.isEmpty() && targetSessionAttrs != "any") {
        parts << QString("target_session_attrs=%1").arg(targetSessionAttrs);
    }

    if (gssEncMode) {
        parts << "gssencmode=require";
    }

    if (!fallbackApplicationName.isEmpty()) {
        parts << QString("fallback_application_name=%1").arg(fallbackApplicationName);
    }

    // Add any additional parameters
    for (auto it = additionalParams.begin(); it != additionalParams.end(); ++it) {
        parts << QString("%1=%2").arg(it.key(), it.value());
    }

    return parts.join(" ");
}

QString PostgreSQLConnectionParameters::generateODBCConnectionString() const {
    QStringList parts;

    // Basic connection parameters
    if (!host.isEmpty()) {
        parts << QString("SERVER=%1").arg(host);
        if (port != 5432) {
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

PostgreSQLConnectionParameters PostgreSQLConnectionParameters::fromConnectionString(const QString& connectionString) {
    PostgreSQLConnectionParameters params;
    QStringList pairs = connectionString.split(" ", Qt::SkipEmptyParts);

    for (const QString& pair : pairs) {
        QStringList keyValue = pair.split("=");
        if (keyValue.size() == 2) {
            QString key = keyValue[0].trimmed().toLower();
            QString value = keyValue[1].trimmed();

            if (key == "host" || key == "server") {
                params.host = value;
            } else if (key == "port") {
                params.port = value.toInt();
            } else if (key == "dbname" || key == "database") {
                params.database = value;
            } else if (key == "user" || key == "username") {
                params.username = value;
            } else if (key == "password" || key == "pwd") {
                params.password = value;
            } else if (key == "client_encoding") {
                params.charset = value;
            } else if (key == "sslmode") {
                params.sslMode = value;
                params.useSSL = (value != "disable");
            } else if (key == "sslrootcert") {
                params.sslCA = value;
            } else if (key == "sslcert") {
                params.sslCert = value;
            } else if (key == "sslkey") {
                params.sslKey = value;
            } else if (key == "sslcrl") {
                params.sslCRL = value;
            } else if (key == "connect_timeout") {
                params.timeout = value.toInt();
            } else if (key == "application_name") {
                params.applicationName = value;
            } else if (key == "search_path") {
                params.searchPath = value;
            } else if (key == "timezone") {
                params.timezone = value;
            } else if (key == "keepalives") {
                params.keepalives = (value == "1" || value.toLower() == "true");
            } else if (key == "keepalives_idle") {
                params.keepalivesIdle = value.toInt();
            } else if (key == "keepalives_interval") {
                params.keepalivesInterval = value.toInt();
            } else if (key == "keepalives_count") {
                params.keepalivesCount = value.toInt();
            } else if (key == "target_session_attrs") {
                params.targetSessionAttrs = value;
            } else if (key == "gssencmode") {
                params.gssEncMode = (value == "require");
            } else if (key == "fallback_application_name") {
                params.fallbackApplicationName = value;
            } else if (key == "options") {
                params.options = value;
            } else {
                // Store additional parameters
                params.additionalParams[keyValue[0].trimmed()] = value;
            }
        }
    }

    return params;
}

// PostgreSQL Connection Tester Implementation

bool PostgreSQLConnectionTester::testBasicConnection(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QString validationError;
    if (!params.validateParameters(validationError)) {
        errorMessage = QString("Parameter validation failed: %1").arg(validationError);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_basic");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    // Set additional connection options
    if (params.useSSL) {
        QString sslOptions;
        if (!params.sslCA.isEmpty()) {
            sslOptions += "sslrootcert=" + params.sslCA + " ";
        }
        if (!params.sslCert.isEmpty()) {
            sslOptions += "sslcert=" + params.sslCert + " ";
        }
        if (!params.sslKey.isEmpty()) {
            sslOptions += "sslkey=" + params.sslKey + " ";
        }
        if (!params.sslCRL.isEmpty()) {
            sslOptions += "sslcrl=" + params.sslCRL + " ";
        }
        if (!sslOptions.isEmpty()) {
            db.setConnectOptions(sslOptions.trimmed());
        }
    }

    if (!params.charset.isEmpty()) {
        db.setConnectOptions("client_encoding=" + params.charset);
    }

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_basic");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_basic");
    return true;
}

bool PostgreSQLConnectionTester::testDatabaseAccess(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_db_access");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_db_access");
        return false;
    }

    QSqlQuery query(db);

    // Test basic queries
    if (!query.exec("SELECT version()")) {
        errorMessage = QString("Version query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_db_access");
        return false;
    }

    // Test database selection if specified
    if (!params.database.isEmpty()) {
        if (!query.exec(QString("SET search_path TO public"))) {
            errorMessage = QString("Search path setup failed: %1").arg(query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_postgresql_db_access");
            return false;
        }
    }

    // Test information_schema access
    if (!query.exec("SELECT COUNT(*) FROM information_schema.tables")) {
        errorMessage = QString("Information schema access failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_db_access");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_db_access");
    return true;
}

bool PostgreSQLConnectionTester::testPermissions(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_permissions");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_permissions");
        return false;
    }

    QSqlQuery query(db);
    QStringList testQueries = {
        "SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema' LIMIT 1",
        "SELECT * FROM information_schema.columns WHERE table_schema = 'information_schema' LIMIT 1",
        "SELECT * FROM pg_catalog.pg_database",
        "SELECT * FROM pg_catalog.pg_user",
        "SELECT current_database()",
        "SELECT current_user"
    };

    for (const QString& sql : testQueries) {
        if (!query.exec(sql)) {
            errorMessage = QString("Permission test failed for query '%1': %2").arg(sql, query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_postgresql_permissions");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_permissions");
    return true;
}

bool PostgreSQLConnectionTester::testServerFeatures(const PostgreSQLConnectionParameters& params, QStringList& supportedFeatures, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_features");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_features");
        return false;
    }

    QSqlQuery query(db);

    // Test basic features
    if (query.exec("SELECT version()")) {
        supportedFeatures << "BASIC_CONNECTIVITY";
    }

    // Test JSON support (PostgreSQL 9.2+)
    if (query.exec("SELECT json_build_object('key', 'value')")) {
        supportedFeatures << "JSON_SUPPORT";
    }

    // Test arrays support
    if (query.exec("SELECT ARRAY[1,2,3]")) {
        supportedFeatures << "ARRAYS";
    }

    // Test CTE support (PostgreSQL 8.4+)
    if (query.exec("WITH cte AS (SELECT 1 as n) SELECT * FROM cte")) {
        supportedFeatures << "CTE_SUPPORT";
    }

    // Test window functions (PostgreSQL 8.4+)
    if (query.exec("SELECT id, ROW_NUMBER() OVER (ORDER BY id) FROM information_schema.tables LIMIT 1")) {
        supportedFeatures << "WINDOW_FUNCTIONS";
    }

    // Test text search
    if (query.exec("SELECT to_tsvector('english', 'The quick brown fox')")) {
        supportedFeatures << "TEXT_SEARCH";
    }

    // Test geometry (PostGIS)
    if (query.exec("SELECT PostGIS_version()")) {
        supportedFeatures << "GEOMETRY";
    }

    // Test ranges (PostgreSQL 9.2+)
    if (query.exec("SELECT int4range(1,5)")) {
        supportedFeatures << "RANGES";
    }

    // Test UUID
    if (query.exec("SELECT gen_random_uuid()")) {
        supportedFeatures << "UUID";
    }

    // Test hstore
    if (query.exec("SELECT 'key=>value'::hstore")) {
        supportedFeatures << "HSTORE";
    }

    // Test inheritance
    if (query.exec("SELECT COUNT(*) FROM pg_inherits")) {
        supportedFeatures << "INHERITANCE";
    }

    // Test partitioning (PostgreSQL 10+)
    if (query.exec("SELECT * FROM pg_partitioned_table LIMIT 1")) {
        supportedFeatures << "PARTITIONING";
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_features");
    return true;
}

bool PostgreSQLConnectionTester::testReplication(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_replication");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_replication");
        return false;
    }

    QSqlQuery query(db);

    // Test replication status
    if (!query.exec("SELECT * FROM pg_stat_replication")) {
        errorMessage = QString("Replication status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_replication");
        return false;
    }

    // Test WAL status
    if (!query.exec("SELECT * FROM pg_stat_wal_receiver")) {
        errorMessage = QString("WAL receiver status check failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_replication");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_replication");
    return true;
}

bool PostgreSQLConnectionTester::testSSLConnection(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    PostgreSQLConnectionParameters testParams = params;
    testParams.useSSL = true;
    testParams.sslMode = "require";

    return testBasicConnection(testParams, errorMessage);
}

bool PostgreSQLConnectionTester::testPerformance(const PostgreSQLConnectionParameters& params, QMap<QString, QVariant>& metrics, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_performance");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    QElapsedTimer timer;
    timer.start();

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_performance");
        return false;
    }

    qint64 connectionTime = timer.elapsed();

    QSqlQuery query(db);

    // Test simple query performance
    timer.restart();
    if (!query.exec("SELECT version()")) {
        errorMessage = "Simple query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_performance");
        return false;
    }
    qint64 simpleQueryTime = timer.elapsed();

    // Test more complex query
    timer.restart();
    if (!query.exec("SELECT * FROM information_schema.tables WHERE table_schema = 'information_schema' LIMIT 100")) {
        errorMessage = "Complex query test failed";
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_performance");
        return false;
    }
    qint64 complexQueryTime = timer.elapsed();

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_performance");

    // Store metrics
    metrics["connection_time_ms"] = connectionTime;
    metrics["simple_query_time_ms"] = simpleQueryTime;
    metrics["complex_query_time_ms"] = complexQueryTime;

    return true;
}

bool PostgreSQLConnectionTester::testStorageEngines(const PostgreSQLConnectionParameters& params, QStringList& engines, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_engines");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_engines");
        return false;
    }

    QSqlQuery query(db);

    // PostgreSQL doesn't have "storage engines" like MySQL, but we can test different access methods
    if (!query.exec("SELECT amname FROM pg_am ORDER BY amname")) {
        errorMessage = QString("Access methods query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_engines");
        return false;
    }

    while (query.next()) {
        engines << query.value(0).toString();
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_engines");
    return true;
}

bool PostgreSQLConnectionTester::testExtensions(const PostgreSQLConnectionParameters& params, QStringList& extensions, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_extensions");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_extensions");
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec("SELECT extname FROM pg_extension ORDER BY extname")) {
        errorMessage = QString("Extensions query failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgresql_extensions");
        return false;
    }

    while (query.next()) {
        extensions << query.value(0).toString();
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_extensions");
    return true;
}

bool PostgreSQLConnectionTester::testPostGIS(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgis");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgis");
        return false;
    }

    QSqlQuery query(db);

    // Test PostGIS functions
    QStringList postgisTests = {
        "SELECT PostGIS_version()",
        "SELECT ST_AsText(ST_GeomFromText('POINT(0 0)'))",
        "SELECT ST_Distance(ST_GeomFromText('POINT(0 0)'), ST_GeomFromText('POINT(1 1)'))"
    };

    for (const QString& sql : postgisTests) {
        if (!query.exec(sql)) {
            errorMessage = QString("PostGIS test failed: %1").arg(query.lastError().text());
            db.close();
            QSqlDatabase::removeDatabase("test_postgis");
            return false;
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgis");
    return true;
}

bool PostgreSQLConnectionTester::testPostgresFDW(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgres_fdw");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgres_fdw");
        return false;
    }

    QSqlQuery query(db);

    // Test postgres_fdw functions
    if (!query.exec("SELECT * FROM pg_foreign_server")) {
        errorMessage = QString("postgres_fdw test failed: %1").arg(query.lastError().text());
        db.close();
        QSqlDatabase::removeDatabase("test_postgres_fdw");
        return false;
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgres_fdw");
    return true;
}

bool PostgreSQLConnectionTester::testEnterpriseFeatures(const PostgreSQLConnectionParameters& params, QStringList& features, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "test_postgresql_enterprise");
    db.setHostName(params.host);
    db.setPort(params.port);
    db.setDatabaseName(params.database);
    db.setUserName(params.username);
    db.setPassword(params.password);

    if (!db.open()) {
        errorMessage = QString("Connection failed: %1").arg(db.lastError().text());
        QSqlDatabase::removeDatabase("test_postgresql_enterprise");
        return false;
    }

    QSqlQuery query(db);

    // Test EnterpriseDB features
    QStringList enterpriseTests = {
        "SELECT * FROM edb_toolkit.secure_dblink_connect", // EDB Secure Dblink
        "SELECT * FROM edb_toolkit.wait_states", // EDB Wait States
        "SELECT * FROM edb_toolkit.statement_log", // EDB Statement Log
        "SELECT * FROM edb_toolkit.error_log" // EDB Error Log
    };

    for (const QString& sql : enterpriseTests) {
        if (query.exec(sql)) {
            if (sql.contains("secure_dblink")) {
                features << "ENTERPRISE_SECURE_DBLINK";
            } else if (sql.contains("wait_states")) {
                features << "ENTERPRISE_WAIT_STATES";
            } else if (sql.contains("statement_log")) {
                features << "ENTERPRISE_STATEMENT_LOG";
            } else if (sql.contains("error_log")) {
                features << "ENTERPRISE_ERROR_LOG";
            }
        }
    }

    db.close();
    QSqlDatabase::removeDatabase("test_postgresql_enterprise");
    return true;
}

// PostgreSQL Connection Manager Implementation

std::unique_ptr<PostgreSQLConnectionManager> PostgreSQLConnectionManager::instance_ = nullptr;

PostgreSQLConnectionManager& PostgreSQLConnectionManager::instance() {
    if (!instance_) {
        instance_.reset(new PostgreSQLConnectionManager());
    }
    return *instance_;
}

bool PostgreSQLConnectionManager::connect(const PostgreSQLConnectionParameters& params, QString& errorMessage) {
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

bool PostgreSQLConnectionManager::disconnect() {
    if (database_.isOpen()) {
        database_.close();
    }
    if (!database_.connectionName().isEmpty()) {
        QSqlDatabase::removeDatabase(database_.connectionName());
    }
    return true;
}

bool PostgreSQLConnectionManager::isConnected() const {
    return database_.isValid() && database_.isOpen();
}

QSqlDatabase PostgreSQLConnectionManager::getDatabase() {
    return database_;
}

bool PostgreSQLConnectionManager::getServerInfo(PostgreSQLServerInfo& info, QString& errorMessage) {
    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT "
                   "version() as version_string, "
                   "inet_server_addr() as server_address, "
                   "inet_server_port() as server_port, "
                   "current_setting('server_version') as server_version, "
                   "current_setting('server_version_num') as server_version_num, "
                   "current_setting('server_encoding') as server_encoding, "
                   "current_setting('client_encoding') as client_encoding, "
                   "current_setting('lc_collate') as lc_collate, "
                   "current_setting('lc_ctype') as lc_ctype, "
                   "current_setting('timezone') as timezone, "
                   "current_setting('shared_buffers') as shared_buffers, "
                   "current_setting('work_mem') as work_mem, "
                   "current_setting('maintenance_work_mem') as maintenance_work_mem, "
                   "current_setting('effective_cache_size') as effective_cache_size, "
                   "current_setting('max_connections') as max_connections, "
                   "current_setting('autovacuum') as autovacuum_enabled, "
                   "current_setting('log_statement') as log_statement, "
                   "current_setting('log_duration') as log_duration, "
                   "pg_size_pretty(pg_database_size(current_database())) as database_size")) {
        errorMessage = QString("Failed to get server info: %1").arg(query.lastError().text());
        return false;
    }

    if (query.next()) {
        info.version = query.value("version_string").toString();
        info.serverAddress = query.value("server_address").toString();
        info.serverPort = query.value("server_port").toInt();
        info.serverEncoding = query.value("server_encoding").toString();
        info.clientEncoding = query.value("client_encoding").toString();
        info.lcCollate = query.value("lc_collate").toString();
        info.lcCtype = query.value("lc_ctype").toString();
        info.timezone = query.value("timezone").toString();
        info.sharedBuffers = query.value("shared_buffers").toString();
        info.workMem = query.value("work_mem").toString();
        info.maintenanceWorkMem = query.value("maintenance_work_mem").toString();
        info.effectiveCacheSize = query.value("effective_cache_size").toString();
        info.maxConnections = query.value("max_connections").toInt();
        info.autovacuumEnabled = (query.value("autovacuum_enabled").toString() == "on");
        info.logStatement = query.value("log_statement").toString();
        info.logDuration = query.value("log_duration").toString();
        info.databaseSize = query.value("database_size").toString();

        // Parse version number
        QStringList versionParts = info.version.split(".");
        if (versionParts.size() >= 2) {
            info.majorVersion = versionParts[0].toInt();
            info.minorVersion = versionParts[1].toInt();
            if (versionParts.size() >= 3) {
                info.patchVersion = versionParts[2].toInt();
            }
        }

        // Check for special versions
        info.isEnterpriseDB = info.version.contains("EnterpriseDB");
        info.isPostgresPlus = info.version.contains("Postgres Plus");
        info.isGreenplum = info.version.contains("Greenplum");

        serverInfo_ = info;
        return true;
    }

    errorMessage = "No server information returned";
    return false;
}

QStringList PostgreSQLConnectionManager::getAvailableDatabases(QString& errorMessage) {
    QStringList databases;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return databases;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT datname FROM pg_database WHERE datistemplate = false ORDER BY datname")) {
        errorMessage = QString("Failed to get databases: %1").arg(query.lastError().text());
        return databases;
    }

    while (query.next()) {
        QString dbName = query.value(0).toString();
        if (dbName != "postgres" && dbName != "template0" && dbName != "template1") {
            databases << dbName;
        }
    }

    return databases;
}

QStringList PostgreSQLConnectionManager::getDatabaseSchemas(const QString& database, QString& errorMessage) {
    QStringList schemas;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return schemas;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT nspname FROM pg_namespace WHERE nspname NOT IN ('pg_catalog', 'information_schema', 'pg_toast') AND nspname NOT LIKE 'pg_temp_%' ORDER BY nspname")) {
        errorMessage = QString("Failed to get schemas: %1").arg(query.lastError().text());
        return schemas;
    }

    while (query.next()) {
        schemas << query.value(0).toString();
    }

    return schemas;
}

QStringList PostgreSQLConnectionManager::getStorageEngines(QString& errorMessage) {
    QStringList engines;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return engines;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT amname FROM pg_am ORDER BY amname")) {
        errorMessage = QString("Failed to get access methods: %1").arg(query.lastError().text());
        return engines;
    }

    while (query.next()) {
        engines << query.value(0).toString();
    }

    return engines;
}

QStringList PostgreSQLConnectionManager::getAvailableExtensions(QString& errorMessage) {
    QStringList extensions;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return extensions;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT name FROM pg_available_extensions ORDER BY name")) {
        errorMessage = QString("Failed to get extensions: %1").arg(query.lastError().text());
        return extensions;
    }

    while (query.next()) {
        extensions << query.value(0).toString();
    }

    return extensions;
}

QStringList PostgreSQLConnectionManager::getAvailableCharSets(QString& errorMessage) {
    QStringList charsets;

    if (!isConnected()) {
        errorMessage = "Not connected to database";
        return charsets;
    }

    QSqlQuery query(database_);
    if (!query.exec("SELECT encoding, name FROM pg_encoding ORDER BY encoding")) {
        errorMessage = QString("Failed to get encodings: %1").arg(query.lastError().text());
        return charsets;
    }

    while (query.next()) {
        charsets << query.value(1).toString();
    }

    return charsets;
}

bool PostgreSQLConnectionManager::detectServerCapabilities(PostgreSQLServerInfo& info, QString& errorMessage) {
    if (!getServerInfo(info, errorMessage)) {
        return false;
    }

    // Detect feature support based on version
    info.supportsJSON = info.isVersionAtLeast(9, 2);
    info.supportsArrays = info.isVersionAtLeast(8, 0);
    info.supportsHStore = info.isVersionAtLeast(8, 4);
    info.supportsGeometry = info.isVersionAtLeast(8, 0); // PostGIS extension
    info.supportsTextSearch = info.isVersionAtLeast(8, 3);
    info.supportsRanges = info.isVersionAtLeast(9, 2);
    info.supportsCTEs = info.isVersionAtLeast(8, 4);
    info.supportsWindowFunctions = info.isVersionAtLeast(8, 4);
    info.supportsInheritance = info.isVersionAtLeast(7, 4);
    info.supportsPartitioning = info.isVersionAtLeast(10, 0);
    info.supportsSSL = true; // PostgreSQL always supports SSL
    info.supportsReplication = info.isVersionAtLeast(9, 0);
    info.supportsPostGIS = true; // Check if PostGIS is available
    info.supportsPostgresFDW = info.isVersionAtLeast(9, 1);
    info.supportsEnterpriseFeatures = info.isEnterpriseDB;

    return true;
}

QStringList PostgreSQLConnectionManager::getSupportedFeatures() const {
    QStringList features;

    if (serverInfo_.supportsJSON) features << "JSON";
    if (serverInfo_.supportsArrays) features << "ARRAYS";
    if (serverInfo_.supportsHStore) features << "HSTORE";
    if (serverInfo_.supportsGeometry) features << "GEOMETRY";
    if (serverInfo_.supportsTextSearch) features << "TEXT_SEARCH";
    if (serverInfo_.supportsRanges) features << "RANGES";
    if (serverInfo_.supportsCTEs) features << "CTE";
    if (serverInfo_.supportsWindowFunctions) features << "WINDOW_FUNCTIONS";
    if (serverInfo_.supportsInheritance) features << "INHERITANCE";
    if (serverInfo_.supportsPartitioning) features << "PARTITIONING";
    if (serverInfo_.supportsSSL) features << "SSL";
    if (serverInfo_.supportsReplication) features << "REPLICATION";
    if (serverInfo_.supportsPostGIS) features << "POSTGIS";
    if (serverInfo_.supportsPostgresFDW) features << "POSTGRES_FDW";
    if (serverInfo_.supportsEnterpriseFeatures) features << "ENTERPRISE_FEATURES";

    return features;
}

QString PostgreSQLConnectionManager::getConnectionStatus() const {
    if (!isConnected()) {
        return "Disconnected";
    }

    return QString("Connected to %1:%2").arg(currentParams_.host, QString::number(currentParams_.port));
}

QString PostgreSQLConnectionManager::getLastError() const {
    return lastError_;
}

bool PostgreSQLConnectionManager::testConnection(QString& errorMessage) {
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

void PostgreSQLConnectionManager::setConnectionTimeout(int seconds) {
    currentParams_.timeout = seconds;
    // Reconnect with new timeout if connected
    if (isConnected()) {
        QString error;
        connect(currentParams_, error);
    }
}

void PostgreSQLConnectionManager::setCommandTimeout(int seconds) {
    currentParams_.commandTimeout = seconds;
}

void PostgreSQLConnectionManager::enableConnectionPooling(bool enable) {
    currentParams_.connectionPooling = enable;
}

void PostgreSQLConnectionManager::setPoolSize(int minSize, int maxSize) {
    currentParams_.minPoolSize = minSize;
    currentParams_.maxPoolSize = maxSize;
}

bool PostgreSQLConnectionManager::configureSSL(const QString& caCert, const QString& clientCert,
                                             const QString& clientKey, QString& errorMessage) {
    currentParams_.useSSL = true;
    currentParams_.sslMode = "require";
    currentParams_.sslCA = caCert;
    currentParams_.sslCert = clientCert;
    currentParams_.sslKey = clientKey;

    return PostgreSQLSSLHelper::validateCertificate(caCert, clientCert, clientKey, errorMessage);
}

void PostgreSQLConnectionManager::setSearchPath(const QString& searchPath) {
    currentParams_.searchPath = searchPath;
}

void PostgreSQLConnectionManager::setTimeZone(const QString& timezone) {
    currentParams_.timezone = timezone;
}

void PostgreSQLConnectionManager::setApplicationName(const QString& appName) {
    currentParams_.applicationName = appName;
}

bool PostgreSQLConnectionManager::initializeDatabase(const PostgreSQLConnectionParameters& params) {
    QString connectionName = QString("postgresql_connection_%1").arg(QDateTime::currentMSecsSinceEpoch());
    database_ = QSqlDatabase::addDatabase("QPSQL", connectionName);
    database_.setHostName(params.host);
    database_.setPort(params.port);
    database_.setDatabaseName(params.database);
    database_.setUserName(params.username);
    database_.setPassword(params.password);

    // Set additional connection options
    QStringList options;

    if (!params.charset.isEmpty()) {
        options << "client_encoding=" + params.charset;
    }

    if (params.useSSL) {
        options << "sslmode=" + params.sslMode;
        if (!params.sslCA.isEmpty()) {
            options << "sslrootcert=" + params.sslCA;
        }
        if (!params.sslCert.isEmpty()) {
            options << "sslcert=" + params.sslCert;
        }
        if (!params.sslKey.isEmpty()) {
            options << "sslkey=" + params.sslKey;
        }
        if (!params.sslCRL.isEmpty()) {
            options << "sslcrl=" + params.sslCRL;
        }
    }

    if (params.timeout > 0) {
        options << "connect_timeout=" + QString::number(params.timeout);
    }

    if (!params.applicationName.isEmpty()) {
        options << "application_name=" + params.applicationName;
    }

    if (!params.searchPath.isEmpty()) {
        options << "search_path=" + params.searchPath;
    }

    if (!params.timezone.isEmpty()) {
        options << "timezone=" + params.timezone;
    }

    if (params.keepalives) {
        options << "keepalives=1";
        if (params.keepalivesIdle > 0) {
            options << "keepalives_idle=" + QString::number(params.keepalivesIdle);
        }
        if (params.keepalivesInterval > 0) {
            options << "keepalives_interval=" + QString::number(params.keepalivesInterval);
        }
        if (params.keepalivesCount > 0) {
            options << "keepalives_count=" + QString::number(params.keepalivesCount);
        }
    }

    if (!params.targetSessionAttrs.isEmpty() && params.targetSessionAttrs != "any") {
        options << "target_session_attrs=" + params.targetSessionAttrs;
    }

    if (params.gssEncMode) {
        options << "gssencmode=require";
    }

    if (!params.fallbackApplicationName.isEmpty()) {
        options << "fallback_application_name=" + params.fallbackApplicationName;
    }

    if (!params.options.isEmpty()) {
        options << params.options;
    }

    // Add any additional parameters
    for (auto it = params.additionalParams.begin(); it != params.additionalParams.end(); ++it) {
        options << it.key() + "=" + it.value();
    }

    if (!options.isEmpty()) {
        database_.setConnectOptions(options.join(" "));
    }

    return database_.isValid();
}

bool PostgreSQLConnectionManager::configureDatabase(const PostgreSQLConnectionParameters& params) {
    // Additional configuration can be done here
    if (!params.database.isEmpty()) {
        // The database is already specified in the connection
        // No additional configuration needed for PostgreSQL
    }

    return true;
}

// PostgreSQL Server Info Implementation

QString PostgreSQLServerInfo::getFullVersionString() const {
    return QString("%1.%2.%3").arg(majorVersion).arg(minorVersion).arg(patchVersion);
}

bool PostgreSQLServerInfo::isVersionAtLeast(int major, int minor, int patch) const {
    if (majorVersion > major) return true;
    if (majorVersion < major) return false;
    if (minorVersion > minor) return true;
    if (minorVersion < minor) return false;
    return patchVersion >= patch;
}

bool PostgreSQLServerInfo::isPostgreSQL9_0() const {
    return majorVersion == 9 && minorVersion == 0;
}

bool PostgreSQLServerInfo::isPostgreSQL9_1() const {
    return majorVersion == 9 && minorVersion == 1;
}

bool PostgreSQLServerInfo::isPostgreSQL9_2() const {
    return majorVersion == 9 && minorVersion == 2;
}

bool PostgreSQLServerInfo::isPostgreSQL9_3() const {
    return majorVersion == 9 && minorVersion == 3;
}

bool PostgreSQLServerInfo::isPostgreSQL9_4() const {
    return majorVersion == 9 && minorVersion == 4;
}

bool PostgreSQLServerInfo::isPostgreSQL9_5() const {
    return majorVersion == 9 && minorVersion == 5;
}

bool PostgreSQLServerInfo::isPostgreSQL9_6() const {
    return majorVersion == 9 && minorVersion == 6;
}

bool PostgreSQLServerInfo::isPostgreSQL10() const {
    return majorVersion == 10;
}

bool PostgreSQLServerInfo::isPostgreSQL11() const {
    return majorVersion == 11;
}

bool PostgreSQLServerInfo::isPostgreSQL12() const {
    return majorVersion == 12;
}

bool PostgreSQLServerInfo::isPostgreSQL13() const {
    return majorVersion == 13;
}

bool PostgreSQLServerInfo::isPostgreSQL14() const {
    return majorVersion == 14;
}

bool PostgreSQLServerInfo::isPostgreSQL15() const {
    return majorVersion == 15;
}

bool PostgreSQLServerInfo::isPostgreSQL16() const {
    return majorVersion == 16;
}

// PostgreSQL Authentication Helper Implementation

QStringList PostgreSQLAuthenticationHelper::getAvailableAuthenticationMethods() {
    QStringList methods;
    methods << "PostgreSQL MD5 Authentication" << "PostgreSQL SCRAM Authentication"
            << "SSL Certificate Authentication" << "Kerberos Authentication"
            << "LDAP Authentication" << "RADIUS Authentication";
    return methods;
}

bool PostgreSQLAuthenticationHelper::isSSLSupported() {
    return true; // PostgreSQL always supports SSL
}

bool PostgreSQLAuthenticationHelper::isCompressionSupported() {
    return true; // PostgreSQL supports compression
}

bool PostgreSQLAuthenticationHelper::validateCredentials(const QString& host, int port,
                                                       const QString& username, const QString& password,
                                                       QString& errorMessage) {
    PostgreSQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.username = username;
    params.password = password;

    return PostgreSQLConnectionTester::testBasicConnection(params, errorMessage);
}

bool PostgreSQLAuthenticationHelper::validateSSLConnection(const QString& host, int port,
                                                         const QString& caCert, const QString& clientCert,
                                                         const QString& clientKey, QString& errorMessage) {
    PostgreSQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.useSSL = true;
    params.sslMode = "require";
    params.sslCA = caCert;
    params.sslCert = clientCert;
    params.sslKey = clientKey;

    return PostgreSQLConnectionTester::testSSLConnection(params, errorMessage);
}

QString PostgreSQLAuthenticationHelper::generateSecurePassword(int length) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString password;

    for (int i = 0; i < length; ++i) {
        int index = qrand() % chars.length();
        password.append(chars.at(index));
    }

    return password;
}

bool PostgreSQLAuthenticationHelper::isPasswordStrong(const QString& password) {
    if (password.length() < 8) return false;
    if (!password.contains(QRegularExpression("[A-Z]"))) return false;
    if (!password.contains(QRegularExpression("[a-z]"))) return false;
    if (!password.contains(QRegularExpression("[0-9]"))) return false;
    if (!password.contains(QRegularExpression("[!@#$%^&*]"))) return false;
    return true;
}

QString PostgreSQLAuthenticationHelper::buildStandardConnectionString(const QString& host, int port,
                                                                     const QString& database,
                                                                     const QString& username,
                                                                     const QString& password) {
    QStringList parts;
    parts << QString("host=%1").arg(host);
    parts << QString("port=%1").arg(port);

    if (!database.isEmpty()) {
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    return parts.join(" ");
}

QString PostgreSQLAuthenticationHelper::buildSSLConnectionString(const QString& host, int port,
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
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    parts << "sslmode=require";

    if (!caCert.isEmpty()) {
        parts << QString("sslrootcert=%1").arg(caCert);
    }

    if (!clientCert.isEmpty()) {
        parts << QString("sslcert=%1").arg(clientCert);
    }

    if (!clientKey.isEmpty()) {
        parts << QString("sslkey=%1").arg(clientKey);
    }

    return parts.join(" ");
}

QString PostgreSQLAuthenticationHelper::buildSocketConnectionString(const QString& socketPath,
                                                                   const QString& database,
                                                                   const QString& username,
                                                                   const QString& password) {
    QStringList parts;
    parts << QString("host=%1").arg(socketPath);

    if (!database.isEmpty()) {
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    if (!password.isEmpty()) {
        parts << QString("password=%1").arg(password);
    }

    return parts.join(" ");
}

QString PostgreSQLAuthenticationHelper::buildKerberosConnectionString(const QString& host, int port,
                                                                     const QString& database,
                                                                     const QString& username) {
    QStringList parts;
    parts << QString("host=%1").arg(host);
    parts << QString("port=%1").arg(port);

    if (!database.isEmpty()) {
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    parts << "gssencmode=require";

    return parts.join(" ");
}

QString PostgreSQLAuthenticationHelper::buildLDAPConnectionString(const QString& host, int port,
                                                                 const QString& database,
                                                                 const QString& username) {
    QStringList parts;
    parts << QString("host=%1").arg(host);
    parts << QString("port=%1").arg(port);

    if (!database.isEmpty()) {
        parts << QString("dbname=%1").arg(database);
    }

    if (!username.isEmpty()) {
        parts << QString("user=%1").arg(username);
    }

    parts << "ldapserver=your-ldap-server";

    return parts.join(" ");
}

// PostgreSQL SSL Helper Implementation

bool PostgreSQLSSLHelper::validateCertificate(const QString& caCert, const QString& clientCert,
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

QStringList PostgreSQLSSLHelper::getSupportedSSLCiphers() {
    QStringList ciphers;
    ciphers << "AES128-SHA" << "AES256-SHA" << "AES128-SHA256" << "AES256-SHA256"
            << "DHE-RSA-AES128-SHA" << "DHE-RSA-AES256-SHA" << "ECDHE-RSA-AES128-SHA"
            << "ECDHE-RSA-AES256-SHA" << "ECDHE-RSA-AES128-SHA256" << "ECDHE-RSA-AES256-SHA384";
    return ciphers;
}

QString PostgreSQLSSLHelper::getRecommendedSSLCipher() {
    return "ECDHE-RSA-AES256-SHA384";
}

bool PostgreSQLSSLHelper::generateSelfSignedCertificate(const QString& certFile, const QString& keyFile,
                                                        const QString& subject, int days, QString& errorMessage) {
    if (days <= 0) days = 365; // Set default if invalid
    // This would require OpenSSL integration - for now, return false
    errorMessage = "Certificate generation requires OpenSSL integration";
    return false;
}

bool PostgreSQLSSLHelper::testSSLConnection(const QString& host, int port,
                                          const QString& caCert, const QString& clientCert,
                                          const QString& clientKey, QString& errorMessage) {
    PostgreSQLConnectionParameters params;
    params.host = host;
    params.port = port;
    params.useSSL = true;
    params.sslMode = "require";
    params.sslCA = caCert;
    params.sslCert = clientCert;
    params.sslKey = clientKey;

    return PostgreSQLConnectionTester::testSSLConnection(params, errorMessage);
}

// PostgreSQL Version Helper Implementation

bool PostgreSQLVersionHelper::parseVersion(const QString& versionString, int& major, int& minor, int& patch) {
    QRegularExpression versionRegex("(\\d+)\\.(\\d+)");
    QRegularExpressionMatch match = versionRegex.match(versionString);

    if (match.hasMatch()) {
        major = match.captured(1).toInt();
        minor = match.captured(2).toInt();
        patch = 0; // PostgreSQL doesn't use patch versions in the same way
        return true;
    }

    return false;
}

QString PostgreSQLVersionHelper::getVersionFamily(const QString& versionString) {
    int major, minor, patch;
    if (parseVersion(versionString, major, minor, patch)) {
        if (major == 9) {
            if (minor == 0) return "PostgreSQL 9.0";
            if (minor == 1) return "PostgreSQL 9.1";
            if (minor == 2) return "PostgreSQL 9.2";
            if (minor == 3) return "PostgreSQL 9.3";
            if (minor == 4) return "PostgreSQL 9.4";
            if (minor == 5) return "PostgreSQL 9.5";
            if (minor == 6) return "PostgreSQL 9.6";
        } else if (major >= 10) {
            return QString("PostgreSQL %1").arg(major);
        }
    }

    return "Unknown";
}

bool PostgreSQLVersionHelper::supportsFeature(const QString& versionString, const QString& feature) {
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
        return (major > minMajor) || (major == minMajor && minor >= minMinor);
    }

    return false;
}

QString PostgreSQLVersionHelper::getMinimumVersionForFeature(const QString& feature) {
    static QMap<QString, QString> featureVersions = {
        {"JSON", "9.2"},
        {"CTE", "8.4"},
        {"WINDOW_FUNCTIONS", "8.4"},
        {"ARRAYS", "8.0"},
        {"GEOMETRY", "8.0"},
        {"TEXT_SEARCH", "8.3"},
        {"RANGES", "9.2"},
        {"UUID", "8.3"},
        {"HSTORE", "8.4"},
        {"INHERITANCE", "7.4"},
        {"PARTITIONING", "10.0"}
    };

    return featureVersions.value(feature.toUpper(), "");
}

int PostgreSQLVersionHelper::compareVersions(const QString& version1, const QString& version2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    if (!parseVersion(version1, major1, minor1, patch1) ||
        !parseVersion(version2, major2, minor2, patch2)) {
        return 0; // Can't compare
    }

    if (major1 != major2) return major1 > major2 ? 1 : -1;
    if (minor1 != minor2) return minor1 > minor2 ? 1 : -1;

    return 0; // Equal
}

bool PostgreSQLVersionHelper::isVersionInRange(const QString& version, const QString& minVersion, const QString& maxVersion) {
    if (!minVersion.isEmpty() && compareVersions(version, minVersion) < 0) {
        return false;
    }

    if (!maxVersion.isEmpty() && compareVersions(version, maxVersion) > 0) {
        return false;
    }

    return true;
}

bool PostgreSQLVersionHelper::isPostgreSQL9_0(const QString& versionString) {
    return versionString.startsWith("9.0");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_1(const QString& versionString) {
    return versionString.startsWith("9.1");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_2(const QString& versionString) {
    return versionString.startsWith("9.2");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_3(const QString& versionString) {
    return versionString.startsWith("9.3");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_4(const QString& versionString) {
    return versionString.startsWith("9.4");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_5(const QString& versionString) {
    return versionString.startsWith("9.5");
}

bool PostgreSQLVersionHelper::isPostgreSQL9_6(const QString& versionString) {
    return versionString.startsWith("9.6");
}

bool PostgreSQLVersionHelper::isPostgreSQL10(const QString& versionString) {
    return versionString.startsWith("10.");
}

bool PostgreSQLVersionHelper::isPostgreSQL11(const QString& versionString) {
    return versionString.startsWith("11.");
}

bool PostgreSQLVersionHelper::isPostgreSQL12(const QString& versionString) {
    return versionString.startsWith("12.");
}

bool PostgreSQLVersionHelper::isPostgreSQL13(const QString& versionString) {
    return versionString.startsWith("13.");
}

bool PostgreSQLVersionHelper::isPostgreSQL14(const QString& versionString) {
    return versionString.startsWith("14.");
}

bool PostgreSQLVersionHelper::isPostgreSQL15(const QString& versionString) {
    return versionString.startsWith("15.");
}

bool PostgreSQLVersionHelper::isPostgreSQL16(const QString& versionString) {
    return versionString.startsWith("16.");
}

PostgreSQLConnectionManager::~PostgreSQLConnectionManager() {
    disconnect();
}

} // namespace scratchrobin
