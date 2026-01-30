/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
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
