/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "core/query_payload.h"

namespace scratchrobin::backend {

/**
 * Query parameter types for prepared statement binding
 */
enum class QueryParameterType {
  kNull,
  kBool,
  kInt32,
  kInt64,
  kFloat,
  kDouble,
  kString,
  kBytes,
  kTimestamp,
  kDate,
  kTime,
  kUuid
};

/**
 * Query parameter for prepared statement binding
 */
struct QueryParameter {
  QueryParameterType type{QueryParameterType::kNull};
  std::variant<std::monostate, bool, int32_t, int64_t, float, double,
               std::string, std::vector<uint8_t>>
      value;

  // Constructors for convenience
  QueryParameter() = default;
  explicit QueryParameter(std::nullptr_t) : type(QueryParameterType::kNull) {}
  explicit QueryParameter(bool v) : type(QueryParameterType::kBool), value(v) {}
  explicit QueryParameter(int32_t v) : type(QueryParameterType::kInt32), value(v) {}
  explicit QueryParameter(int64_t v) : type(QueryParameterType::kInt64), value(v) {}
  explicit QueryParameter(float v) : type(QueryParameterType::kFloat), value(v) {}
  explicit QueryParameter(double v) : type(QueryParameterType::kDouble), value(v) {}
  explicit QueryParameter(const std::string& v)
      : type(QueryParameterType::kString), value(v) {}
  explicit QueryParameter(std::string_view v)
      : type(QueryParameterType::kString), value(std::string(v)) {}
  explicit QueryParameter(const char* v)
      : type(QueryParameterType::kString), value(std::string(v)) {}
  explicit QueryParameter(const std::vector<uint8_t>& v)
      : type(QueryParameterType::kBytes), value(v) {}

  // Check if null
  bool isNull() const { return type == QueryParameterType::kNull; }

  // Get string representation for debugging
  std::string toString() const;
};

/**
 * Query flags for execution control
 */
enum class QueryFlags : uint32_t {
  kNone = 0,
  kBytecode = 1 << 0,        // Require bytecode execution
  kReadOnly = 1 << 1,        // Read-only query hint
  kAutoCommit = 1 << 2,      // Auto-commit after execution
  kCacheResult = 1 << 3,     // Cache result set
  kStreaming = 1 << 4,       // Stream results (don't buffer all)
};

inline QueryFlags operator|(QueryFlags a, QueryFlags b) {
  return static_cast<QueryFlags>(static_cast<uint32_t>(a) |
                                  static_cast<uint32_t>(b));
}

inline bool hasFlag(QueryFlags flags, QueryFlags flag) {
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * QueryRequest - Full query execution request
 *
 * Includes SQL, parameters, flags, and execution context
 */
struct QueryRequest {
  // Core query
  std::uint16_t port{0};
  std::string dialect;
  std::string sql;
  core::QueryPayload payload;

  // Parameter binding
  std::vector<QueryParameter> parameters;

  // Execution control
  QueryFlags flags{QueryFlags::kNone};
  std::uint32_t timeout_ms{30000};

  // Metadata
  std::string client_info;
  std::map<std::string, std::string> attributes;

  // Helpers
  bool bytecodeFlag() const {
    return hasFlag(flags, QueryFlags::kBytecode);
  }

  void setBytecodeFlag(bool enable) {
    if (enable) {
      flags = flags | QueryFlags::kBytecode;
    }
  }

  void addParameter(const QueryParameter& param) {
    parameters.push_back(param);
  }

  void clearParameters() { parameters.clear(); }
};

/**
 * Parse SQL to find parameter placeholders ($1, $2, ?, :name)
 * @param sql The SQL statement
 * @return Vector of placeholder positions and names
 */
std::vector<std::pair<size_t, std::string>> ParseParameterPlaceholders(
    const std::string& sql);

/**
 * Replace positional parameters with bound values
 * @param sql SQL with placeholders
 * @param parameters Bound parameter values
 * @return SQL with placeholders replaced (for databases without prepared statements)
 */
std::string SubstituteParameters(const std::string& sql,
                                  const std::vector<QueryParameter>& parameters);

}  // namespace scratchrobin::backend
