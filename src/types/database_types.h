#ifndef SCRATCHROBIN_DATABASE_TYPES_H
#define SCRATCHROBIN_DATABASE_TYPES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace scratchrobin {

enum class DatabaseType {
    SCRATCHBIRD,
    POSTGRESQL,
    MYSQL,
    MARIADB,
    SQLITE,
    ORACLE,
    SQLSERVER,
    MSSQL,
    ODBC,
    FIREBIRD,
    DB2
};

struct DatabaseConnection {
    std::string id;
    std::string name;
    DatabaseType type;
    std::string host;
    int port;
    std::string database;
    std::string username;
    std::string password;
    std::unordered_map<std::string, std::string> options;
};

// QueryResult is defined in execution/sql_executor.h

struct DatabaseInfo {
    std::string name;
    std::string version;
    DatabaseType type;
    std::string serverName;
    std::string userName;
    std::vector<std::string> databases;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATABASE_TYPES_H
