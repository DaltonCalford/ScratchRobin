#ifndef SCRATCHROBIN_QUERY_TYPES_H
#define SCRATCHROBIN_QUERY_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace scratchrobin {

struct QueryColumn {
    std::string name;
    std::string type;
};

struct QueryMessage {
    std::string severity;
    std::string message;
    std::string detail;
};

struct QueryStats {
    double elapsedMs = 0.0;
    int64_t rowsReturned = 0;
};

struct QueryValue {
    bool isNull = true;
    std::string text;
    std::vector<uint8_t> raw;
};

struct QueryResult {
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> rows;
    int64_t rowsAffected = 0;
    std::string commandTag;
    std::vector<QueryMessage> messages;
    std::vector<std::string> errorStack;
    QueryStats stats;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_TYPES_H
