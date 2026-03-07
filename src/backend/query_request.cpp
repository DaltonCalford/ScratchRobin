/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "backend/query_request.h"

#include <iomanip>
#include <regex>
#include <sstream>

#include "core/sql_utils.h"

namespace scratchrobin::backend {

std::string QueryParameter::toString() const {
  switch (type) {
    case QueryParameterType::kNull:
      return "NULL";
    case QueryParameterType::kBool:
      return std::get<bool>(value) ? "TRUE" : "FALSE";
    case QueryParameterType::kInt32:
      return std::to_string(std::get<int32_t>(value));
    case QueryParameterType::kInt64:
      return std::to_string(std::get<int64_t>(value));
    case QueryParameterType::kFloat:
      return std::to_string(std::get<float>(value));
    case QueryParameterType::kDouble:
      return std::to_string(std::get<double>(value));
    case QueryParameterType::kString:
      return core::escapeStringLiteral(std::get<std::string>(value));
    case QueryParameterType::kBytes: {
      const auto& bytes = std::get<std::vector<uint8_t>>(value);
      std::ostringstream oss;
      oss << "'\\x";
      for (auto b : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
      }
      oss << "'";
      return oss.str();
    }
    default:
      return "?";
  }
}

std::vector<std::pair<size_t, std::string>> ParseParameterPlaceholders(
    const std::string& sql) {
  std::vector<std::pair<size_t, std::string>> placeholders;

  // Pattern 1: Positional parameters $1, $2, etc. (PostgreSQL style)
  std::regex positional_re(R"(\$(\d+))");
  auto pos_begin = std::sregex_iterator(sql.begin(), sql.end(), positional_re);
  auto pos_end = std::sregex_iterator();

  for (auto it = pos_begin; it != pos_end; ++it) {
    placeholders.push_back({static_cast<size_t>(it->position()), it->str()});
  }

  // Pattern 2: Positional parameters ? (ODBC/JDBC style)
  size_t q_pos = 0;
  int q_index = 1;
  while ((q_pos = sql.find('?', q_pos)) != std::string::npos) {
    // Check if escaped (??)
    if (q_pos + 1 < sql.size() && sql[q_pos + 1] == '?') {
      q_pos += 2;
      continue;
    }
    placeholders.push_back({q_pos, "?" + std::to_string(q_index++)});
    ++q_pos;
  }

  // Pattern 3: Named parameters :name (Oracle/MySQL style)
  std::regex named_re(R"(:([a-zA-Z_][a-zA-Z0-9_]*))");
  auto named_begin = std::sregex_iterator(sql.begin(), sql.end(), named_re);
  auto named_end = std::sregex_iterator();

  for (auto it = named_begin; it != named_end; ++it) {
    placeholders.push_back({static_cast<size_t>(it->position()), it->str()});
  }

  // Sort by position
  std::sort(placeholders.begin(), placeholders.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  return placeholders;
}

std::string SubstituteParameters(const std::string& sql,
                                  const std::vector<QueryParameter>& parameters) {
  auto placeholders = ParseParameterPlaceholders(sql);
  if (placeholders.empty()) {
    return sql;
  }

  std::string result;
  size_t last_pos = 0;

  for (const auto& [pos, placeholder] : placeholders) {
    // Append text before placeholder
    result += sql.substr(last_pos, pos - last_pos);

    // Extract parameter index
    int param_index = -1;
    if (placeholder[0] == '$' || (placeholder[0] == '?' && placeholder.size() > 1)) {
      // $N or ?N format
      std::string num_str = placeholder.substr(1);
      param_index = std::stoi(num_str) - 1;  // Convert to 0-based
    } else if (placeholder[0] == '?') {
      // Simple ? - use position in sequence
      param_index = static_cast<int>(&placeholder - &placeholders[0].second);
    }

    // Substitute parameter value
    if (param_index >= 0 && param_index < static_cast<int>(parameters.size())) {
      result += parameters[param_index].toString();
    } else {
      // Keep placeholder if parameter not provided
      result += placeholder;
    }

    last_pos = pos + placeholder.length();
  }

  // Append remaining text
  result += sql.substr(last_pos);

  return result;
}

}  // namespace scratchrobin::backend
