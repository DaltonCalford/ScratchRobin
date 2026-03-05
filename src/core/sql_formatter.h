/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Formatting Options
// ============================================================================

struct SqlFormatOptions {
  // Indentation
  bool use_tabs{false};
  int spaces_per_indent{4};
  int max_line_length{120};

  // Case
  bool uppercase_keywords{true};
  bool uppercase_functions{false};
  bool lowercase_identifiers{true};

  // Spacing
  bool space_after_comma{true};
  bool space_around_operators{true};
  bool compact_lists{false};

  // Alignment
  bool align_keywords{false};
  bool align_columns{false};

  // Newlines
  bool newline_before_select_list{false};
  bool newline_before_from{true};
  bool newline_before_where{true};
  bool newline_before_join{true};
  bool newline_before_group_by{true};
  bool newline_before_order_by{true};
  bool newline_before_having{true};

  // Parentheses
  bool compact_parentheses{false};
  bool newline_inside_parentheses{true};

  // Comments
  bool preserve_comments{true};
  bool align_comments{false};

  // Dialect
  std::string dialect{"generic"};  // firebird, postgresql, mysql, etc.
};

// ============================================================================
// Format Result
// ============================================================================

struct FormatResult {
  bool success{false};
  std::string formatted_sql;
  std::string error_message;
  int original_line_count{0};
  int formatted_line_count{0};
};

// ============================================================================
// SQL Element
// ============================================================================

enum class SqlElementType {
  kKeyword,
  kIdentifier,
  kStringLiteral,
  kNumericLiteral,
  kOperator,
  kPunctuation,
  kComment,
  kWhitespace,
  kFunction,
  kUnknown
};

struct SqlElement {
  SqlElementType type{SqlElementType::kUnknown};
  std::string value;
  int line{0};
  int column{0};
};

// ============================================================================
// SQL Formatter
// ============================================================================

class SqlFormatter {
 public:
  static SqlFormatter& Instance();

  // Format SQL
  FormatResult Format(const std::string& sql);
  FormatResult Format(const std::string& sql, const SqlFormatOptions& options);

  // Quick format with defaults
  std::string FormatQuick(const std::string& sql);

  // Minify SQL (remove unnecessary whitespace)
  std::string Minify(const std::string& sql);

  // Validate SQL syntax
  bool ValidateSyntax(const std::string& sql);
  std::string GetLastError() const { return last_error_; }

  // Tokenize SQL
  std::vector<SqlElement> Tokenize(const std::string& sql);

  // Normalize SQL (lowercase keywords, standardize spacing)
  std::string Normalize(const std::string& sql);

  // Extract statements from a batch
  std::vector<std::string> SplitStatements(const std::string& sql);

  // Quote identifiers
  std::string QuoteIdentifier(const std::string& identifier,
                              const std::string& dialect = "generic");
  std::string UnquoteIdentifier(const std::string& identifier);

  // Options management
  void SetDefaultOptions(const SqlFormatOptions& options);
  SqlFormatOptions GetDefaultOptions() const { return default_options_; }

  // Presets
  void LoadPreset(const std::string& preset_name);
  void SavePreset(const std::string& preset_name, const SqlFormatOptions& options);
  std::vector<std::string> GetAvailablePresets();
  void DeletePreset(const std::string& preset_name);

  // Keyword detection
  bool IsKeyword(const std::string& word);
  bool IsFunction(const std::string& word);
  bool IsReservedWord(const std::string& word);

  // Dialect-specific formatting
  std::string FormatForDialect(const std::string& sql,
                               const std::string& dialect);

 private:
  SqlFormatter();
  ~SqlFormatter() = default;

  SqlFormatter(const SqlFormatter&) = delete;
  SqlFormatter& operator=(const SqlFormatter&) = delete;

  SqlFormatOptions default_options_;
  std::string last_error_;

  void InitializeKeywords();
  void InitializeFunctions();

  std::vector<std::string> keywords_;
  std::vector<std::string> functions_;

  std::string FormatElement(const SqlElement& element,
                            const SqlFormatOptions& options);
  std::string ToUpperCase(const std::string& str);
  std::string ToLowerCase(const std::string& str);
  std::string MakeIndent(int level, const SqlFormatOptions& options);
};

}  // namespace scratchrobin::core
