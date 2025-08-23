#ifndef SCRATCHROBIN_SQL_EXECUTOR_H
#define SCRATCHROBIN_SQL_EXECUTOR_H

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include "types/database_types.h"
#include "types/query_types.h"

namespace scratchrobin {

// Simple QueryResult for components - avoiding circular include with execution/sql_executor.h
struct QueryResult {
    std::string queryId;
    std::string queryText;
    QueryType queryType;
    std::string connectionId;
    std::string databaseName;
    std::string userName;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds executionTime;
    bool success = false;
    int rowsAffected = 0;
    std::vector<std::string> columnNames;
    std::vector<std::vector<std::string>> rows;
    std::string errorMessage;
};

class SQLExecutor {
public:
    SQLExecutor();
    ~SQLExecutor();

    QueryResult executeQuery(const std::string& sql, const std::string& connectionId);
    bool executeNonQuery(const std::string& sql, const std::string& connectionId);
    QueryPlan explainQuery(const std::string& sql, const std::string& connectionId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SQL_EXECUTOR_H
