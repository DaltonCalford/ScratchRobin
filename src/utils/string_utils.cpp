#include "string_utils.h"
#include <chrono>
#include <atomic>

namespace scratchrobin {

// Helper function to convert QueryType to string
std::string queryTypeToString(QueryType type) {
    switch (type) {
        case QueryType::SELECT: return "SELECT";
        case QueryType::INSERT: return "INSERT";
        case QueryType::UPDATE: return "UPDATE";
        case QueryType::DELETE: return "DELETE";
        case QueryType::CREATE: return "CREATE";
        case QueryType::ALTER: return "ALTER";
        case QueryType::DROP: return "DROP";
        case QueryType::UNKNOWN: return "UNKNOWN";
        case QueryType::COMMIT: return "COMMIT";
        case QueryType::ROLLBACK: return "ROLLBACK";
        default: return "UNKNOWN";
    }
}

std::string indexTypeToString(IndexType type) {
    switch (type) {
        case IndexType::BTREE: return "BTREE";
        case IndexType::HASH: return "HASH";
        case IndexType::GIN: return "GIN";
        case IndexType::GIST: return "GIST";
        case IndexType::SPGIST: return "SPGIST";
        case IndexType::BRIN: return "BRIN";
        case IndexType::UNIQUE: return "UNIQUE";
        case IndexType::PARTIAL: return "PARTIAL";
        case IndexType::EXPRESSION: return "EXPRESSION";
        case IndexType::COMPOSITE: return "COMPOSITE";
        default: return "BTREE";
    }
}

std::string constraintTypeToString(ConstraintType type) {
    switch (type) {
        case ConstraintType::PRIMARY_KEY: return "PRIMARY KEY";
        case ConstraintType::FOREIGN_KEY: return "FOREIGN KEY";
        case ConstraintType::UNIQUE: return "UNIQUE";
        case ConstraintType::CHECK: return "CHECK";
        case ConstraintType::NOT_NULL: return "NOT NULL";
        case ConstraintType::DEFAULT: return "DEFAULT";
        case ConstraintType::EXCLUDE: return "EXCLUDE";
        case ConstraintType::DOMAIN: return "DOMAIN";
        default: return "UNKNOWN";
    }
}

std::string generateOperationId() {
    static std::atomic<int> counter{0};
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "op_" + std::to_string(timestamp) + "_" + std::to_string(++counter);
}

} // namespace scratchrobin