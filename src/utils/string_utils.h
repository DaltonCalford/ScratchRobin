#pragma once

#include <string>
#include "types/database_types.h"
#include "types/query_types.h"
#include "index/index_manager.h"
#include "constraint/constraint_manager.h"

namespace scratchrobin {

// Query type utilities
std::string queryTypeToString(QueryType type);

// Index type utilities
std::string indexTypeToString(IndexType type);

// Constraint type utilities
std::string constraintTypeToString(ConstraintType type);

// General utilities
std::string generateOperationId();

} // namespace scratchrobin