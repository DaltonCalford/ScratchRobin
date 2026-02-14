/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PREPARED_TYPES_H
#define SCRATCHROBIN_PREPARED_TYPES_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

enum class PreparedParamType {
    Null,
    Bool,
    Int64,
    Double,
    String,
    Bytes
};

struct PreparedParameter {
    PreparedParamType type = PreparedParamType::Null;
    bool boolValue = false;
    int64_t intValue = 0;
    double doubleValue = 0.0;
    std::string stringValue;
    std::vector<uint8_t> bytesValue;
};

struct PreparedStatementHandle {
    virtual ~PreparedStatementHandle() = default;
    std::string sql;
    size_t parameterCount = 0;
};

using PreparedStatementHandlePtr = std::shared_ptr<PreparedStatementHandle>;

} // namespace scratchrobin

#endif // SCRATCHROBIN_PREPARED_TYPES_H
