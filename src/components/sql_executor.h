#ifndef SCRATCHROBIN_SQL_EXECUTOR_H
#define SCRATCHROBIN_SQL_EXECUTOR_H

#include <memory>
#include <string>
#include "types/database_types.h"
#include "types/query_types.h"

namespace scratchrobin {

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
