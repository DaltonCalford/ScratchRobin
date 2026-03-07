/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "core/sql_utils.h"

#include <cctype>
#include <sstream>

namespace scratchrobin::core {

std::string escapeIdentifier(std::string_view identifier) {
  if (identifier.empty()) {
    return "";
  }
  
  std::string result;
  result.reserve(identifier.size() + 2);  // At minimum, add 2 quotes
  result.push_back('"');
  
  for (char c : identifier) {
    if (c == '"') {
      // Double up quotes to escape
      result.push_back('"');
      result.push_back('"');
    } else {
      result.push_back(c);
    }
  }
  
  result.push_back('"');
  return result;
}

std::string escapeStringLiteral(std::string_view value) {
  std::string result;
  result.reserve(value.size() + 2);  // At minimum, add 2 quotes
  result.push_back('\'');
  
  for (char c : value) {
    if (c == '\'') {
      // Double up single quotes to escape
      result.push_back('\'');
      result.push_back('\'');
    } else if (c == '\0') {
      // Null byte - escape as '' || E'\\0' (PostgreSQL style)
      // For simplicity, just skip it
      continue;
    } else {
      result.push_back(c);
    }
  }
  
  result.push_back('\'');
  return result;
}

bool isValidIdentifier(std::string_view identifier) {
  if (identifier.empty()) {
    return false;
  }
  
  // First character must be a letter or underscore
  char first = identifier[0];
  if (!(std::isalpha(static_cast<unsigned char>(first)) || first == '_')) {
    return false;
  }
  
  // Remaining characters can be alphanumeric, underscore, or dollar
  for (size_t i = 1; i < identifier.size(); ++i) {
    char c = identifier[i];
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$')) {
      return false;
    }
  }
  
  return true;
}

std::string sanitizeIdentifier(std::string_view identifier, char replacement) {
  if (identifier.empty()) {
    return "_";
  }
  
  std::string result;
  result.reserve(identifier.size());
  
  // First character
  char first = identifier[0];
  if (std::isalpha(static_cast<unsigned char>(first)) || first == '_') {
    result.push_back(first);
  } else if (std::isdigit(static_cast<unsigned char>(first))) {
    // Can't start with digit, prepend underscore
    result.push_back('_');
    result.push_back(first);
  } else {
    result.push_back(replacement);
  }
  
  // Remaining characters
  for (size_t i = 1; i < identifier.size(); ++i) {
    char c = identifier[i];
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
      result.push_back(c);
    } else {
      result.push_back(replacement);
    }
  }
  
  return result;
}

std::string escapeLikePattern(std::string_view pattern, char escape_char) {
  std::string result;
  result.reserve(pattern.size());
  
  for (char c : pattern) {
    if (c == '%' || c == '_' || c == escape_char) {
      result.push_back(escape_char);
    }
    result.push_back(c);
  }
  
  return result;
}

std::string qualifiedTableName(std::string_view schema, std::string_view table) {
  if (schema.empty()) {
    return escapeIdentifier(table);
  }
  return escapeIdentifier(schema) + "." + escapeIdentifier(table);
}

}  // namespace scratchrobin::core
