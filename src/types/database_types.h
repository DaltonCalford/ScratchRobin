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
    SQLITE,
    ORACLE,
    SQLSERVER
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

struct QueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int rowCount;
    int affectedRows;
    std::string errorMessage;
    bool success;
};

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
