/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/sql_formatter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

SqlFormatter& SqlFormatter::Instance() {
  static SqlFormatter instance;
  return instance;
}

SqlFormatter::SqlFormatter() {
  InitializeKeywords();
  InitializeFunctions();
}

// ============================================================================
// Format SQL
// ============================================================================

FormatResult SqlFormatter::Format(const std::string& sql) {
  return Format(sql, default_options_);
}

FormatResult SqlFormatter::Format(const std::string& sql,
                                  const SqlFormatOptions& options) {
  FormatResult result;
  result.original_line_count = 1 + std::count(sql.begin(), sql.end(), '\n');
  
  // Simple formatting implementation
  std::vector<SqlElement> tokens = Tokenize(sql);
  std::ostringstream formatted;
  
  int indent_level = 0;
  bool new_line = false;
  
  for (size_t i = 0; i < tokens.size(); ++i) {
    const auto& token = tokens[i];
    
    if (token.type == SqlElementType::kWhitespace) {
      continue;  // Skip whitespace, we'll add our own
    }
    
    // Handle keywords that change indentation
    std::string upper_val = ToUpperCase(token.value);
    
    if (upper_val == "SELECT" || upper_val == "FROM" || upper_val == "WHERE" ||
        upper_val == "GROUP" || upper_val == "ORDER" || upper_val == "HAVING" ||
        upper_val == "JOIN" || upper_val == "LEFT" || upper_val == "RIGHT" ||
        upper_val == "INNER" || upper_val == "OUTER" || upper_val == "UNION" ||
        upper_val == "INSERT" || upper_val == "UPDATE" || upper_val == "DELETE") {
      if (!formatted.str().empty() && formatted.str().back() != '\n') {
        formatted << "\n";
      }
      formatted << MakeIndent(indent_level, options);
    }
    
    // Add space before token if needed
    if (!formatted.str().empty() && 
        formatted.str().back() != '\n' && 
        formatted.str().back() != '(' &&
        token.value != "," && token.value != ")" && token.value != ";") {
      formatted << " ";
    }
    
    // Output the token with proper case
    if (token.type == SqlElementType::kKeyword && options.uppercase_keywords) {
      formatted << ToUpperCase(token.value);
    } else if (token.type == SqlElementType::kFunction && options.uppercase_functions) {
      formatted << ToUpperCase(token.value);
    } else if (token.type == SqlElementType::kIdentifier && options.lowercase_identifiers) {
      formatted << ToLowerCase(token.value);
    } else {
      formatted << token.value;
    }
    
    // Handle indentation for parentheses
    if (token.value == "(") {
      indent_level++;
      if (options.newline_inside_parentheses) {
        formatted << "\n" << MakeIndent(indent_level, options);
      }
    } else if (token.value == ")") {
      indent_level = std::max(0, indent_level - 1);
    }
  }
  
  result.formatted_sql = formatted.str();
  result.formatted_line_count = 1 + std::count(
      result.formatted_sql.begin(), result.formatted_sql.end(), '\n');
  result.success = true;
  
  return result;
}

std::string SqlFormatter::FormatQuick(const std::string& sql) {
  auto result = Format(sql);
  return result.success ? result.formatted_sql : sql;
}

// ============================================================================
// Minify SQL
// ============================================================================

std::string SqlFormatter::Minify(const std::string& sql) {
  std::vector<SqlElement> tokens = Tokenize(sql);
  std::ostringstream minified;
  
  SqlElementType prev_type = SqlElementType::kUnknown;
  
  for (const auto& token : tokens) {
    if (token.type == SqlElementType::kWhitespace ||
        token.type == SqlElementType::kComment) {
      continue;
    }
    
    // Add space between identifiers and keywords
    if (prev_type == SqlElementType::kIdentifier ||
        prev_type == SqlElementType::kKeyword) {
      if (token.type == SqlElementType::kIdentifier ||
          token.type == SqlElementType::kKeyword) {
        minified << " ";
      }
    }
    
    minified << token.value;
    prev_type = token.type;
  }
  
  return minified.str();
}

// ============================================================================
// Validation
// ============================================================================

bool SqlFormatter::ValidateSyntax(const std::string& sql) {
  // Simplified validation - would need a proper parser
  auto tokens = Tokenize(sql);
  
  // Check for basic structure
  int paren_depth = 0;
  for (const auto& token : tokens) {
    if (token.value == "(") {
      paren_depth++;
    } else if (token.value == ")") {
      paren_depth--;
      if (paren_depth < 0) {
        last_error_ = "Unmatched closing parenthesis";
        return false;
      }
    }
  }
  
  if (paren_depth != 0) {
    last_error_ = "Unmatched opening parenthesis";
    return false;
  }
  
  return true;
}

// ============================================================================
// Tokenization
// ============================================================================

std::vector<SqlElement> SqlFormatter::Tokenize(const std::string& sql) {
  std::vector<SqlElement> tokens;
  size_t i = 0;
  int line = 1;
  int col = 1;
  
  while (i < sql.length()) {
    SqlElement element;
    element.line = line;
    element.column = col;
    
    // Skip whitespace but record it
    if (std::isspace(static_cast<unsigned char>(sql[i]))) {
      while (i < sql.length() && std::isspace(static_cast<unsigned char>(sql[i]))) {
        if (sql[i] == '\n') {
          line++;
          col = 1;
        } else {
          col++;
        }
        element.value += sql[i++];
      }
      element.type = SqlElementType::kWhitespace;
      tokens.push_back(element);
      continue;
    }
    
    // String literal
    if (sql[i] == '\'' || sql[i] == '"') {
      char quote = sql[i];
      element.value += sql[i++];
      col++;
      while (i < sql.length() && sql[i] != quote) {
        if (sql[i] == '\\' && i + 1 < sql.length()) {
          element.value += sql[i++];
          col++;
        }
        element.value += sql[i++];
        col++;
      }
      if (i < sql.length()) {
        element.value += sql[i++];
        col++;
      }
      element.type = SqlElementType::kStringLiteral;
      tokens.push_back(element);
      continue;
    }
    
    // Single-line comment
    if (sql[i] == '-' && i + 1 < sql.length() && sql[i + 1] == '-') {
      while (i < sql.length() && sql[i] != '\n') {
        element.value += sql[i++];
        col++;
      }
      element.type = SqlElementType::kComment;
      tokens.push_back(element);
      continue;
    }
    
    // Multi-line comment
    if (sql[i] == '/' && i + 1 < sql.length() && sql[i + 1] == '*') {
      element.value += sql[i++];
      element.value += sql[i++];
      col += 2;
      while (i + 1 < sql.length() && !(sql[i] == '*' && sql[i + 1] == '/')) {
        if (sql[i] == '\n') {
          line++;
          col = 1;
        } else {
          col++;
        }
        element.value += sql[i++];
      }
      if (i + 1 < sql.length()) {
        element.value += sql[i++];
        element.value += sql[i++];
        col += 2;
      }
      element.type = SqlElementType::kComment;
      tokens.push_back(element);
      continue;
    }
    
    // Punctuation
    if (std::strchr("(),;.*=<>!+-/*", sql[i])) {
      element.value = sql[i++];
      element.type = SqlElementType::kPunctuation;
      tokens.push_back(element);
      col++;
      continue;
    }
    
    // Identifier or keyword or number
    if (std::isalnum(static_cast<unsigned char>(sql[i])) || sql[i] == '_') {
      while (i < sql.length() &&
             (std::isalnum(static_cast<unsigned char>(sql[i])) || sql[i] == '_')) {
        element.value += sql[i++];
        col++;
      }
      
      // Check if it's a keyword
      std::string upper = ToUpperCase(element.value);
      if (IsKeyword(upper)) {
        element.type = SqlElementType::kKeyword;
      } else if (IsFunction(upper)) {
        element.type = SqlElementType::kFunction;
      } else if (std::isdigit(static_cast<unsigned char>(element.value[0]))) {
        element.type = SqlElementType::kNumericLiteral;
      } else {
        element.type = SqlElementType::kIdentifier;
      }
      tokens.push_back(element);
      continue;
    }
    
    // Unknown character, skip
    i++;
    col++;
  }
  
  return tokens;
}

// ============================================================================
// Normalization
// ============================================================================

std::string SqlFormatter::Normalize(const std::string& sql) {
  auto tokens = Tokenize(sql);
  std::ostringstream normalized;
  
  for (const auto& token : tokens) {
    if (token.type == SqlElementType::kWhitespace) {
      normalized << " ";
      continue;
    }
    if (token.type == SqlElementType::kKeyword) {
      normalized << ToUpperCase(token.value);
    } else {
      normalized << ToLowerCase(token.value);
    }
  }
  
  return normalized.str();
}

// ============================================================================
// Statement Splitting
// ============================================================================

std::vector<std::string> SqlFormatter::SplitStatements(const std::string& sql) {
  std::vector<std::string> statements;
  std::ostringstream current;
  
  auto tokens = Tokenize(sql);
  for (const auto& token : tokens) {
    if (token.value == ";") {
      std::string stmt = current.str();
      // Trim whitespace
      size_t start = stmt.find_first_not_of(" \t\n\r");
      if (start != std::string::npos) {
        size_t end = stmt.find_last_not_of(" \t\n\r");
        statements.push_back(stmt.substr(start, end - start + 1));
      }
      current.str("");
    } else {
      if (token.type != SqlElementType::kWhitespace) {
        if (!current.str().empty()) {
          current << " ";
        }
      }
      current << token.value;
    }
  }
  
  // Don't forget the last statement if it doesn't end with ;
  std::string stmt = current.str();
  size_t start = stmt.find_first_not_of(" \t\n\r");
  if (start != std::string::npos) {
    size_t end = stmt.find_last_not_of(" \t\n\r");
    statements.push_back(stmt.substr(start, end - start + 1));
  }
  
  return statements;
}

// ============================================================================
// Identifier Quoting
// ============================================================================

std::string SqlFormatter::QuoteIdentifier(const std::string& identifier,
                                          const std::string& dialect) {
  if (dialect == "postgresql" || dialect == "firebird") {
    return "\"" + identifier + "\"";
  } else if (dialect == "mysql" || dialect == "mariadb") {
    return "`" + identifier + "`";
  } else if (dialect == "sqlserver") {
    return "[" + identifier + "]";
  }
  return "\"" + identifier + "\"";
}

std::string SqlFormatter::UnquoteIdentifier(const std::string& identifier) {
  if (identifier.length() >= 2) {
    char first = identifier[0];
    char last = identifier[identifier.length() - 1];
    if ((first == '"' || first == '`' || first == '[') &&
        (last == '"' || last == '`' || last == ']')) {
      return identifier.substr(1, identifier.length() - 2);
    }
  }
  return identifier;
}

// ============================================================================
// Options Management
// ============================================================================

void SqlFormatter::SetDefaultOptions(const SqlFormatOptions& options) {
  default_options_ = options;
}

// ============================================================================
// Presets
// ============================================================================

void SqlFormatter::LoadPreset(const std::string& preset_name) {
  // Would load from configuration
}

void SqlFormatter::SavePreset(const std::string& preset_name,
                              const SqlFormatOptions& options) {
  // Would save to configuration
}

std::vector<std::string> SqlFormatter::GetAvailablePresets() {
  return {"default", "compact", "expanded", "oracle", "postgresql", "mysql"};
}

void SqlFormatter::DeletePreset(const std::string& preset_name) {
  // Would delete from configuration
}

// ============================================================================
// Keyword Detection
// ============================================================================

bool SqlFormatter::IsKeyword(const std::string& word) {
  std::string upper = ToUpperCase(word);
  return std::find(keywords_.begin(), keywords_.end(), upper) != keywords_.end();
}

bool SqlFormatter::IsFunction(const std::string& word) {
  std::string upper = ToUpperCase(word);
  return std::find(functions_.begin(), functions_.end(), upper) != functions_.end();
}

bool SqlFormatter::IsReservedWord(const std::string& word) {
  return IsKeyword(word);
}

// ============================================================================
// Dialect-Specific Formatting
// ============================================================================

std::string SqlFormatter::FormatForDialect(const std::string& sql,
                                           const std::string& dialect) {
  SqlFormatOptions options = default_options_;
  options.dialect = dialect;
  
  // Adjust options based on dialect conventions
  if (dialect == "oracle") {
    options.uppercase_keywords = true;
    options.uppercase_functions = true;
  } else if (dialect == "postgresql") {
    options.lowercase_identifiers = true;
  }
  
  return Format(sql, options).formatted_sql;
}

// ============================================================================
// Helper Methods
// ============================================================================

void SqlFormatter::InitializeKeywords() {
  keywords_ = {
    "SELECT", "FROM", "WHERE", "AND", "OR", "NOT", "NULL", "IS",
    "INSERT", "UPDATE", "DELETE", "CREATE", "DROP", "ALTER", "TABLE",
    "INDEX", "VIEW", "TRIGGER", "PROCEDURE", "FUNCTION", "DATABASE",
    "JOIN", "INNER", "OUTER", "LEFT", "RIGHT", "FULL", "ON", "AS",
    "GROUP", "BY", "ORDER", "HAVING", "LIMIT", "OFFSET", "UNION",
    "ALL", "DISTINCT", "VALUES", "SET", "INTO", "VALUES", "COMMIT",
    "ROLLBACK", "BEGIN", "TRANSACTION", "GRANT", "REVOKE", "PRIMARY",
    "KEY", "FOREIGN", "REFERENCES", "UNIQUE", "CHECK", "DEFAULT",
    "ASC", "DESC", "LIKE", "IN", "BETWEEN", "EXISTS", "CASE", "WHEN",
    "THEN", "ELSE", "END", "IF", "WHILE", "FOR", "RETURN", "DECLARE"
  };
}

void SqlFormatter::InitializeFunctions() {
  functions_ = {
    "COUNT", "SUM", "AVG", "MIN", "MAX", "CONCAT", "SUBSTRING",
    "LENGTH", "UPPER", "LOWER", "TRIM", "LTRIM", "RTRIM", "REPLACE",
    "CAST", "COALESCE", "NULLIF", "ABS", "ROUND", "FLOOR", "CEILING",
    "DATE", "TIME", "DATETIME", "CURRENT_DATE", "CURRENT_TIME",
    "CURRENT_TIMESTAMP", "NOW", "EXTRACT", "YEAR", "MONTH", "DAY"
  };
}

std::string SqlFormatter::FormatElement(const SqlElement& element,
                                        const SqlFormatOptions& options) {
  return element.value;
}

std::string SqlFormatter::ToUpperCase(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

std::string SqlFormatter::ToLowerCase(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

std::string SqlFormatter::MakeIndent(int level, const SqlFormatOptions& options) {
  if (options.use_tabs) {
    return std::string(level, '\t');
  }
  return std::string(level * options.spaces_per_indent, ' ');
}

}  // namespace scratchrobin::core
