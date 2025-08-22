#ifndef SCRATCHROBIN_QUERY_TYPES_H
#define SCRATCHROBIN_QUERY_TYPES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace scratchrobin {

enum class QueryType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE,
    CREATE,
    DROP,
    ALTER,
    GRANT,
    REVOKE,
    BEGIN,
    COMMIT,
    ROLLBACK,
    UNKNOWN
};

enum class QueryState {
    PENDING,
    EXECUTING,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct QueryInfo {
    std::string id;
    std::string sql;
    QueryType type;
    QueryState state;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds executionTime;
    int rowsAffected;
    int rowsReturned;
    std::string errorMessage;
    std::unordered_map<std::string, std::string> parameters;
};

struct QueryPlanNode {
    std::string nodeType;
    std::string relationName;
    std::vector<std::string> outputColumns;
    double cost;
    double actualTime;
    int actualRows;
    std::vector<std::shared_ptr<QueryPlanNode>> children;
};

struct QueryPlan {
    std::shared_ptr<QueryPlanNode> root;
    double totalCost;
    double executionTime;
    std::string planText;
};

struct QueryHistoryEntry {
    std::string id;
    std::string sql;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds duration;
    int rowsAffected;
    bool success;
    std::string errorMessage;
    std::string connectionId;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_TYPES_H
