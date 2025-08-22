#include "sql_executor.h"

namespace scratchrobin {

class SQLExecutor::Impl {};

SQLExecutor::SQLExecutor()
    : impl_(std::make_unique<Impl>()) {
}

SQLExecutor::~SQLExecutor() = default;

QueryResult SQLExecutor::executeQuery(const std::string& sql, const std::string& connectionId) {
    QueryResult result;
    result.success = false;
    result.errorMessage = "Not implemented";
    return result;
}

bool SQLExecutor::executeNonQuery(const std::string& sql, const std::string& connectionId) {
    return false; // TODO: Implement non-query execution
}

QueryPlan SQLExecutor::explainQuery(const std::string& sql, const std::string& connectionId) {
    return QueryPlan(); // TODO: Implement query explanation
}

} // namespace scratchrobin