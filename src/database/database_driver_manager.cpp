#include "database_driver_manager.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace scratchrobin {

// Initialize static instance
std::unique_ptr<DatabaseDriverManager> DatabaseDriverManager::instance_ = nullptr;

DatabaseDriverManager& DatabaseDriverManager::instance() {
    if (!instance_) {
        instance_.reset(new DatabaseDriverManager());
    }
    return *instance_;
}

DatabaseDriverManager::DatabaseDriverManager(QObject* parent)
    : QObject(parent) {
    initializeDrivers();
    scanAvailableDrivers();
}

void DatabaseDriverManager::initializeDrivers() {
    setupPostgreSQLDriver();
    setupMySQLDriver();
    setupSQLiteDriver();
    setupOracleDriver();
    setupSQLServerDriver();
}

void DatabaseDriverManager::setupPostgreSQLDriver() {
    DatabaseDriver driver("postgresql", "PostgreSQL", "QPSQL");
    driver.description = "Advanced open source relational database";
    driver.connectionParameters = QStringList() << "host" << "port" << "database" << "username" << "password"
                                   << "sslmode" << "connect_timeout" << "options";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "PostgreSQL client library (libpq) must be installed.\n"
                              "Ubuntu/Debian: sudo apt-get install libpq-dev\n"
                              "CentOS/RHEL: sudo yum install postgresql-devel\n"
                              "macOS: brew install postgresql";
    driver.requiredLibraries = QStringList() << "libpq.so" << "libpq.dylib" << "libpq.dll";

    drivers_[DatabaseType::POSTGRESQL] = driver;

    // PostgreSQL connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("sslmode", "SSL Mode", "string"));
    params.append(ConnectionParameter("connect_timeout", "Timeout (s)", "int"));
    params.append(ConnectionParameter("options", "Additional Options", "string"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 5432; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[5].defaultValue = "prefer"; // sslmode
    params[6].defaultValue = 30; // timeout
    params[7].placeholder = "Additional connection options";

    connectionParameters_[DatabaseType::POSTGRESQL] = params;
}

void DatabaseDriverManager::setupMySQLDriver() {
    DatabaseDriver driver("mysql", "MySQL", "QMYSQL");
    driver.description = "Popular open source relational database";
    driver.connectionParameters = QStringList() <<"host", "port", "database", "username", "password",
                                   "unix_socket", "charset", "ssl_ca", "ssl_cert", "ssl_key";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "MySQL client library must be installed.\n"
                              "Ubuntu/Debian: sudo apt-get install libmysqlclient-dev\n"
                              "CentOS/RHEL: sudo yum install mysql-devel\n"
                              "macOS: brew install mysql";
    driver.requiredLibraries = QStringList() <<"libmysqlclient.so", "libmysqlclient.dylib", "libmysqlclient.dll";

    drivers_[DatabaseType::MYSQL] = driver;

    // MySQL connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("unix_socket", "Unix Socket", "file"));
    params.append(ConnectionParameter("charset", "Character Set", "string"));
    params.append(ConnectionParameter("ssl_ca", "SSL CA File", "file"));
    params.append(ConnectionParameter("ssl_cert", "SSL Cert File", "file"));
    params.append(ConnectionParameter("ssl_key", "SSL Key File", "file"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 3306; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[6].defaultValue = "utf8mb4"; // charset
    params[6].placeholder = "Path to Unix socket file";
    params[7].placeholder = "SSL CA certificate file";
    params[8].placeholder = "SSL certificate file";
    params[9].placeholder = "SSL key file";

    connectionParameters_[DatabaseType::MYSQL] = params;
}

void DatabaseDriverManager::setupMariaDBDriver() {
    DatabaseDriver driver("mariadb", "MariaDB", "QMARIADB");
    driver.description = "Community-developed fork of MySQL";
    driver.connectionParameters = QStringList() <<"host", "port", "database", "username", "password",
                                   "unix_socket", "charset", "ssl_ca", "ssl_cert", "ssl_key";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "MariaDB client library must be installed.\n"
                              "Ubuntu/Debian: sudo apt-get install libmariadb-dev\n"
                              "CentOS/RHEL: sudo yum install mariadb-devel\n"
                              "macOS: brew install mariadb";
    driver.requiredLibraries = QStringList() <<"libmariadb.so", "libmariadb.dylib", "libmariadb.dll";

    drivers_[DatabaseType::MariaDB] = driver;

    // MariaDB connection parameters (same as MySQL)
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("unix_socket", "Unix Socket", "file"));
    params.append(ConnectionParameter("charset", "Character Set", "string"));
    params.append(ConnectionParameter("ssl_ca", "SSL CA File", "file"));
    params.append(ConnectionParameter("ssl_cert", "SSL Cert File", "file"));
    params.append(ConnectionParameter("ssl_key", "SSL Key File", "file"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 3306; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[6].defaultValue = "utf8mb4"; // charset
    params[6].placeholder = "Path to Unix socket file";
    params[7].placeholder = "SSL CA certificate file";
    params[8].placeholder = "SSL certificate file";
    params[9].placeholder = "SSL key file";

    connectionParameters_[DatabaseType::MariaDB] = params;
}

void DatabaseDriverManager::setupMSSQLDriver() {
    DatabaseDriver driver("mssql", "Microsoft SQL Server", "QODBC");
    driver.description = "Microsoft's enterprise relational database";
    driver.connectionParameters = QStringList() <<"dsn", "host", "port", "database", "username", "password",
                                   "driver", "trusted_connection", "encrypt", "trust_server_certificate";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "Microsoft SQL Server ODBC driver must be installed.\n"
                              "Ubuntu/Debian: Install Microsoft ODBC driver from Microsoft repository\n"
                              "Windows: Built-in ODBC support\n"
                              "macOS: Install Microsoft ODBC driver";
    driver.requiredLibraries = QStringList() <<"libodbc.so", "odbc32.dll", "libodbc.dylib";

    drivers_[DatabaseType::MSSQL] = driver;

    // MSSQL connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("dsn", "DSN Name", "string"));
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("driver", "ODBC Driver", "string"));
    params.append(ConnectionParameter("trusted_connection", "Trusted Connection", "bool"));
    params.append(ConnectionParameter("encrypt", "Encrypt Connection", "bool"));
    params.append(ConnectionParameter("trust_server_certificate", "Trust Server Certificate", "bool"));

    // Set defaults and requirements
    params[0].placeholder = "ODBC Data Source Name";
    params[1].defaultValue = "localhost"; // host
    params[2].defaultValue = 1433; // port
    params[3].required = true; // database
    params[6].defaultValue = "ODBC Driver 17 for SQL Server"; // driver
    params[7].defaultValue = false; // trusted_connection
    params[8].defaultValue = true; // encrypt
    params[9].defaultValue = false; // trust_server_certificate

    connectionParameters_[DatabaseType::MSSQL] = params;
}

void DatabaseDriverManager::setupODBCDriver() {
    DatabaseDriver driver("odbc", "ODBC Generic", "QODBC");
    driver.description = "Open Database Connectivity - Generic database access";
    driver.connectionParameters = QStringList() <<"dsn", "driver", "host", "port", "database", "username", "password";
    driver.requiresAdditionalSetup = false;
    driver.setupInstructions = "ODBC driver manager must be installed.\n"
                              "Ubuntu/Debian: sudo apt-get install unixodbc-dev\n"
                              "CentOS/RHEL: sudo yum install unixODBC-devel\n"
                              "Windows: Built-in ODBC support\n"
                              "macOS: Install unixODBC";
    driver.requiredLibraries = QStringList() <<"libodbc.so", "odbc32.dll", "libodbc.dylib";

    drivers_[DatabaseType::ODBC] = driver;

    // ODBC connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("dsn", "DSN Name", "string"));
    params.append(ConnectionParameter("driver", "ODBC Driver", "string"));
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));

    // Set defaults and requirements
    params[0].placeholder = "ODBC Data Source Name";
    params[1].placeholder = "ODBC driver name";
    params[2].defaultValue = "localhost"; // host

    connectionParameters_[DatabaseType::ODBC] = params;
}

void DatabaseDriverManager::setupFirebirdSQLDriver() {
    DatabaseDriver driver("firebird", "FirebirdSQL", "QFIREBIRD");
    driver.description = "Open source SQL relational database";
    driver.connectionParameters = QStringList() <<"host", "port", "database", "username", "password",
                                   "role", "charset", "dialect", "page_size";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "Firebird client library must be installed.\n"
                              "Ubuntu/Debian: sudo apt-get install firebird-dev\n"
                              "CentOS/RHEL: sudo yum install firebird-devel\n"
                              "Windows: Install Firebird ODBC driver\n"
                              "macOS: brew install firebird";
    driver.requiredLibraries = QStringList() <<"libfbclient.so", "fbclient.dll", "libfbclient.dylib"};

    drivers_[DatabaseType::FirebirdSQL] = driver;

    // FirebirdSQL connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database Path", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("role", "Role", "string"));
    params.append(ConnectionParameter("charset", "Character Set", "string"));
    params.append(ConnectionParameter("dialect", "SQL Dialect", "int"));
    params.append(ConnectionParameter("page_size", "Page Size", "int"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 3050; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[6].defaultValue = "UTF8"; // charset
    params[7].defaultValue = 3; // dialect
    params[8].defaultValue = 4096; // page_size

    connectionParameters_[DatabaseType::FirebirdSQL] = params;
}

void DatabaseDriverManager::setupSQLiteDriver() {
    DatabaseDriver driver("sqlite", "SQLite", "QSQLITE");
    driver.description = "Lightweight embedded database";
    driver.connectionParameters = QStringList() <<"database", "pragma_foreign_keys", "pragma_journal_mode",
                                   "pragma_synchronous", "pragma_cache_size", "pragma_temp_store";
    driver.requiresAdditionalSetup = false;
    driver.setupInstructions = "SQLite support is built into Qt.";
    driver.requiredLibraries = QStringList() <<};

    drivers_[DatabaseType::SQLITE] = driver;

    // SQLite connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("database", "Database File", "file"));
    params.append(ConnectionParameter("pragma_foreign_keys", "Enable Foreign Keys", "bool"));
    params.append(ConnectionParameter("pragma_journal_mode", "Journal Mode", "string"));
    params.append(ConnectionParameter("pragma_synchronous", "Synchronous Mode", "string"));
    params.append(ConnectionParameter("pragma_cache_size", "Cache Size (KB)", "int"));
    params.append(ConnectionParameter("pragma_temp_store", "Temp Store Mode", "string"));

    // Set defaults and requirements
    params[0].required = true; // database
    params[1].defaultValue = true; // foreign keys
    params[2].defaultValue = "WAL"; // journal mode
    params[3].defaultValue = "NORMAL"; // synchronous
    params[4].defaultValue = 2000; // cache size
    params[5].defaultValue = "MEMORY"; // temp store

    connectionParameters_[DatabaseType::SQLITE] = params;
}

void DatabaseDriverManager::setupOracleDriver() {
    DatabaseDriver driver("oracle", "Oracle Database", "QOCI");
    driver.description = "Oracle's enterprise relational database";
    driver.connectionParameters = QStringList() <<"host", "port", "database", "username", "password",
                                   "service_name", "sid", "charset", "numeric_characters";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "Oracle Instant Client must be installed.\n"
                              "Download from Oracle Technology Network and set up environment variables.";
    driver.requiredLibraries = QStringList() <<"libclntsh.so", "oci.dll", "libclntsh.dylib"};

    drivers_[DatabaseType::ORACLE] = driver;

    // Oracle connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database/SID", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("service_name", "Service Name", "string"));
    params.append(ConnectionParameter("sid", "SID", "string"));
    params.append(ConnectionParameter("charset", "Character Set", "string"));
    params.append(ConnectionParameter("numeric_characters", "Numeric Characters", "string"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 1521; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[7].defaultValue = "AL32UTF8"; // charset

    connectionParameters_[DatabaseType::ORACLE] = params;
}

void DatabaseDriverManager::setupSQLServerDriver() {
    DatabaseDriver driver("sqlserver", "SQL Server", "QODBC");
    driver.description = "Microsoft SQL Server database";
    driver.connectionParameters = QStringList() <<"dsn", "host", "port", "database", "username", "password",
                                   "driver", "trusted_connection", "encrypt", "trust_server_certificate";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "Microsoft SQL Server ODBC driver must be installed.\n"
                              "Ubuntu/Debian: Install Microsoft ODBC driver from Microsoft repository\n"
                              "Windows: Built-in ODBC support\n"
                              "macOS: Install Microsoft ODBC driver";
    driver.requiredLibraries = QStringList() <<"libodbc.so", "odbc32.dll", "libodbc.dylib";

    drivers_[DatabaseType::SQLSERVER] = driver;

    // SQL Server connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("dsn", "DSN Name", "string"));
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("driver", "ODBC Driver", "string"));
    params.append(ConnectionParameter("trusted_connection", "Trusted Connection", "bool"));
    params.append(ConnectionParameter("encrypt", "Encrypt Connection", "bool"));
    params.append(ConnectionParameter("trust_server_certificate", "Trust Server Certificate", "bool"));

    // Set defaults and requirements
    params[0].placeholder = "ODBC Data Source Name";
    params[1].defaultValue = "localhost"; // host
    params[2].defaultValue = 1433; // port
    params[3].required = true; // database
    params[6].defaultValue = "ODBC Driver 17 for SQL Server"; // driver
    params[7].defaultValue = false; // trusted_connection
    params[8].defaultValue = true; // encrypt
    params[9].defaultValue = false; // trust_server_certificate

    connectionParameters_[DatabaseType::SQLSERVER] = params;
}

void DatabaseDriverManager::setupDB2Driver() {
    DatabaseDriver driver("db2", "IBM DB2", "QDB2");
    driver.description = "IBM's enterprise relational database";
    driver.connectionParameters = QStringList() <<"host", "port", "database", "username", "password",
                                   "protocol", "schema", "isolation_level";
    driver.requiresAdditionalSetup = true;
    driver.setupInstructions = "IBM DB2 client library must be installed.\n"
                              "Requires IBM DB2 Runtime Client or IBM Data Server Driver.";
    driver.requiredLibraries = QStringList() <<"libdb2.so", "db2cli.dll", "libdb2.dylib"};

    drivers_[DatabaseType::DB2] = driver;

    // DB2 connection parameters
    QList<ConnectionParameter> params;
    params.append(ConnectionParameter("host", "Host", "string"));
    params.append(ConnectionParameter("port", "Port", "port"));
    params.append(ConnectionParameter("database", "Database", "string"));
    params.append(ConnectionParameter("username", "Username", "string"));
    params.append(ConnectionParameter("password", "Password", "password"));
    params.append(ConnectionParameter("protocol", "Protocol", "string"));
    params.append(ConnectionParameter("schema", "Default Schema", "string"));
    params.append(ConnectionParameter("isolation_level", "Isolation Level", "string"));

    // Set defaults and requirements
    params[0].defaultValue = "localhost"; // host
    params[1].defaultValue = 50000; // port
    params[2].required = true; // database
    params[3].required = true; // username
    params[4].required = true; params[4].sensitive = true; // password
    params[5].defaultValue = "TCPIP"; // protocol
    params[7].defaultValue = "CS"; // isolation level

    connectionParameters_[DatabaseType::DB2] = params;
}

void DatabaseDriverManager::scanAvailableDrivers() {
    // Check PostgreSQL
    checkPostgreSQLAvailability();

    // Check MySQL
    checkMySQLAvailability();

    // Check SQLite
    checkSQLiteAvailability();

    // Check Oracle
    checkOracleAvailability();

    // Check SQL Server
    checkSQLServerAvailability();

    emit driverScanCompleted();
}

void DatabaseDriverManager::checkPostgreSQLAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::POSTGRESQL];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QPSQL")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::POSTGRESQL, driver.status);
}

void DatabaseDriverManager::checkMySQLAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::MYSQL];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QMYSQL")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::MYSQL, driver.status);
}

void DatabaseDriverManager::checkMariaDBAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::MariaDB];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QMARIADB")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::MariaDB, driver.status);
}

void DatabaseDriverManager::checkMSSQLAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::MSSQL];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QODBC")) {
        // Additional check for MSSQL ODBC driver could be added here
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::MSSQL, driver.status);
}

void DatabaseDriverManager::checkODBCAAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::ODBC];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QODBC")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::ODBC, driver.status);
}

void DatabaseDriverManager::checkFirebirdSQLAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::FirebirdSQL];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QFIREBIRD")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::FirebirdSQL, driver.status);
}

void DatabaseDriverManager::checkSQLiteAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::SQLITE];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QSQLITE")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::SQLITE, driver.status);
}

void DatabaseDriverManager::checkOracleAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::ORACLE];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QOCI")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::ORACLE, driver.status);
}

void DatabaseDriverManager::checkSQLServerAvailability() {
    DatabaseDriver& driver = drivers_[DatabaseType::SQLSERVER];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QODBC")) {
        // Additional check for MSSQL ODBC driver could be added here
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::SQLSERVER, driver.status);
}

void DatabaseDriverManager::checkDB2Availability() {
    DatabaseDriver& driver = drivers_[DatabaseType::DB2];
    QStringList drivers = QSqlDatabase::drivers();

    if (drivers.contains("QDB2")) {
        driver.status = DriverStatus::Available;
    } else {
        driver.status = DriverStatus::NotAvailable;
    }

    emit driverStatusChanged(DatabaseType::DB2, driver.status);
}

// Public interface methods
QList<DatabaseDriver> DatabaseDriverManager::getAvailableDrivers() const {
    QList<DatabaseDriver> available;
    for (const auto& pair : drivers_) {
        if (pair.status == DriverStatus::Available) {
            available.append(pair);
        }
    }
    return available;
}

QList<DatabaseDriver> DatabaseDriverManager::getAllDrivers() const {
    return drivers_.values();
}

DatabaseDriver DatabaseDriverManager::getDriver(DatabaseType type) const {
    return drivers_.value(type, DatabaseDriver());
}

DatabaseDriver DatabaseDriverManager::getDriver(const QString& name) const {
    for (const DatabaseDriver& driver : drivers_) {
        if (driver.name == name) {
            return driver;
        }
    }
    return DatabaseDriver();
}

bool DatabaseDriverManager::isDriverAvailable(DatabaseType type) const {
    return drivers_.value(type, DatabaseDriver()).status == DriverStatus::Available;
}

bool DatabaseDriverManager::isDriverAvailable(const QString& driverName) const {
    for (const DatabaseDriver& driver : drivers_) {
        if (driver.driverName == driverName) {
            return driver.status == DriverStatus::Available;
        }
    }
    return false;
}

QList<ConnectionParameter> DatabaseDriverManager::getConnectionParameters(DatabaseType type) const {
    return connectionParameters_.value(type);
}

QList<ConnectionParameter> DatabaseDriverManager::getConnectionParameters(const QString& driverName) const {
    for (const auto& pair : drivers_) {
        if (pair.driverName == driverName) {
            return connectionParameters_.value(pair.type);
        }
    }
    return QList<ConnectionParameter>();
}

bool DatabaseDriverManager::validateConnectionParameters(DatabaseType type,
                                                        const QMap<QString, QVariant>& parameters) const {
    QList<ConnectionParameter> requiredParams = getConnectionParameters(type);

    for (const ConnectionParameter& param : requiredParams) {
        if (param.required && !parameters.contains(param.name)) {
            return false;
        }

        if (parameters.contains(param.name)) {
            QVariant value = parameters.value(param.name);

            // Basic type validation
            if (param.dataType == "int" && !value.canConvert(QMetaType::Int)) {
                return false;
            }
            if (param.dataType == "port" && (!value.canConvert(QMetaType::Int) ||
                value.toInt() <= 0 || value.toInt() > 65535)) {
                return false;
            }
            if (param.dataType == "string" && value.toString().isEmpty()) {
                return false;
            }
        }
    }

    return true;
}

bool DatabaseDriverManager::testConnection(const DatabaseConnectionConfig& config, QString& errorMessage) {
    QSqlDatabase db = QSqlDatabase::addDatabase(config.type == DatabaseType::SQLite ? "QSQLITE" : "QODBC",
                                               "connection_test");

    // Set connection parameters based on database type
    switch (config.type) {
        case DatabaseType::PostgreSQL:
            db.setHostName(config.host);
            db.setPort(config.port);
            db.setDatabaseName(config.database);
            db.setUserName(config.username);
            db.setPassword(config.password);
            break;

        case DatabaseType::MySQL:
        case DatabaseType::MariaDB:
            db.setHostName(config.host);
            db.setPort(config.port);
            db.setDatabaseName(config.database);
            db.setUserName(config.username);
            db.setPassword(config.password);
            break;

        case DatabaseType::SQLite:
            db.setDatabaseName(config.database);
            break;

        case DatabaseType::ODBC:
        case DatabaseType::MSSQL:
            // For ODBC connections, we need to build the connection string
            {
                QString connectionString = generateConnectionString(config);
                db.setDatabaseName(connectionString);
            }
            break;

        default:
            errorMessage = "Database type not supported for connection testing";
            return false;
    }

    // Try to open the connection
    if (!db.open()) {
        errorMessage = db.lastError().text();
        QSqlDatabase::removeDatabase("connection_test");
        return false;
    }

    // Run a simple test query
    QSqlQuery query(db);
    if (!query.exec("SELECT 1")) {
        errorMessage = query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase("connection_test");
        return false;
    }

    // Clean up
    db.close();
    QSqlDatabase::removeDatabase("connection_test");

    errorMessage = "Connection successful";
    return true;
}

QString DatabaseDriverManager::generateConnectionString(const DatabaseConnectionConfig& config) const {
    QStringList parts;

    switch (config.type) {
        case DatabaseType::MSSQL:
            parts.append("Driver={ODBC Driver 17 for SQL Server}");
            if (!config.host.isEmpty()) {
                parts.append(QString("Server=%1,%2").arg(config.host).arg(config.port));
            }
            if (!config.database.isEmpty()) {
                parts.append(QString("Database=%1").arg(config.database));
            }
            if (!config.username.isEmpty()) {
                parts.append(QString("Uid=%1").arg(config.username));
            }
            if (!config.password.isEmpty()) {
                parts.append(QString("Pwd=%1").arg(config.password));
            }
            break;

        case DatabaseType::ODBC:
            if (!config.additionalParameters.value("dsn").toString().isEmpty()) {
                parts.append(QString("DSN=%1").arg(config.additionalParameters.value("dsn").toString()));
            } else if (!config.additionalParameters.value("driver").toString().isEmpty()) {
                parts.append(QString("Driver={%1}").arg(config.additionalParameters.value("driver").toString()));
                if (!config.host.isEmpty()) {
                    parts.append(QString("Server=%1").arg(config.host));
                }
                if (config.port > 0) {
                    parts.append(QString("Port=%1").arg(config.port));
                }
            }
            if (!config.database.isEmpty()) {
                parts.append(QString("Database=%1").arg(config.database));
            }
            if (!config.username.isEmpty()) {
                parts.append(QString("Uid=%1").arg(config.username));
            }
            if (!config.password.isEmpty()) {
                parts.append(QString("Pwd=%1").arg(config.password));
            }
            break;

        default:
            // For other databases, return empty string
            break;
    }

    return parts.join(";");
}

QString DatabaseDriverManager::getDriverInstallationInstructions(DatabaseType type) const {
    return drivers_.value(type).setupInstructions;
}

QStringList DatabaseDriverManager::getRequiredLibraries(DatabaseType type) const {
    return drivers_.value(type).requiredLibraries;
}

bool DatabaseDriverManager::checkDriverDependencies(DatabaseType type) {
    // This would check if required libraries are available
    // For now, just return true
    return true;
}

QString DatabaseDriverManager::databaseTypeToString(DatabaseType type) const {
    switch (type) {
        case DatabaseType::POSTGRESQL: return "PostgreSQL";
        case DatabaseType::MYSQL: return "MySQL";
        case DatabaseType::SQLITE: return "SQLite";
        case DatabaseType::ORACLE: return "Oracle";
        case DatabaseType::SQLSERVER: return "SQL Server";
        case DatabaseType::SCRATCHBIRD: return "ScratchBird";
        default: return "Unknown";
    }
}

DatabaseType DatabaseDriverManager::stringToDatabaseType(const QString& str) const {
    QString lower = str.toLower();
    if (lower == "postgresql") return DatabaseType::POSTGRESQL;
    if (lower == "mysql") return DatabaseType::MYSQL;
    if (lower == "sqlite") return DatabaseType::SQLITE;
    if (lower == "oracle") return DatabaseType::ORACLE;
    if (lower == "sql server" || lower == "mssql") return DatabaseType::SQLSERVER;
    if (lower == "scratchbird") return DatabaseType::SCRATCHBIRD;
    return DatabaseType::POSTGRESQL; // default
}

QString DatabaseDriverManager::getDefaultPort(DatabaseType type) const {
    switch (type) {
        case DatabaseType::POSTGRESQL: return "5432";
        case DatabaseType::MYSQL: return "3306";
        case DatabaseType::ORACLE: return "1521";
        case DatabaseType::SQLSERVER: return "1433";
        case DatabaseType::SQLITE: return "0"; // No port for SQLite
        case DatabaseType::SCRATCHBIRD: return "5432"; // Default to PostgreSQL port
        default: return "0";
    }
}

QStringList DatabaseDriverManager::getDatabaseTypeList() const {
    return QStringList() << "PostgreSQL" << "MySQL" << "SQLite" << "Oracle" << "SQL Server" << "ScratchBird";
}

} // namespace scratchrobin
