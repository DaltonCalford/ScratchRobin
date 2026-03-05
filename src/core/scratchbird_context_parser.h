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

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace scratchrobin {
namespace core {

// ============================================================================
// Statement Types
// ============================================================================

enum class StatementType {
  kUnknown,
  kSelect,
  kInsert,
  kUpdate,
  kDelete,
  kCreate,
  kDrop,
  kAlter,
  kBegin,
  kCommit,
  kRollback,
  kCte,
  kExplain,
  kCall,
  kSet,
  kOther
};

// ============================================================================
// Context Types
// ============================================================================

enum class ContextType {
  kUnknown,
  kGeneral,
  kTableReference,     // After FROM, JOIN, INTO
  kColumnReference,    // After SELECT, WHERE, SET
  kKeyword,            // Expecting keyword
  kFunctionCall,       // Inside function call
  kExpression,         // Inside expression
  kComment             // Inside comment
};

// ============================================================================
// Token Types
// ============================================================================

enum class TokenType {
  kInvalid,
  kKeyword,
  kIdentifier,
  kLiteral,
  kOperator,
  kPunctuation,
  kComment,
  kWhitespace,
  kEnd
};

// ============================================================================
// Token
// ============================================================================

struct Token {
  TokenType type = TokenType::kInvalid;
  std::string text;
  size_t position = 0;
  size_t length = 0;
  int line = 0;
  int column = 0;
  
  Token();
  Token(TokenType type_val, const std::string& text_val, size_t pos_val);
  ~Token();
  
  bool IsKeyword() const;
  bool IsIdentifier() const;
  bool IsLiteral() const;
  bool IsOperator() const;
  bool IsPunctuation() const;
};

// ============================================================================
// ParseContext
// ============================================================================

struct ParseContext {
  std::string sql;
  size_t cursor_pos = 0;
  StatementType statement_type = StatementType::kUnknown;
  ContextType context_type = ContextType::kUnknown;
  std::string current_word;
  std::vector<Token> tokens;
  std::vector<std::string> table_references;
  std::vector<std::string> column_references;
  std::vector<std::string> local_variables;
  
  ParseContext();
  ParseContext(const std::string& sql_text, size_t cursor_position);
  ~ParseContext();
  
  bool IsEmpty() const;
  bool IsCursorAtEnd() const;
  size_t GetLineNumber() const;
  size_t GetColumnNumber() const;
};

// ============================================================================
// CompletionItem
// ============================================================================

struct CompletionItem {
  std::string label;
  std::string kind;       // keyword, table, column, function, etc.
  std::string insert_text;
  std::string detail;
  std::string documentation;
  int priority = 100;
};

// Forward declarations
class CompletionProvider;

// ============================================================================
// Tokenizer
// ============================================================================

class Tokenizer {
 public:
  Tokenizer();
  ~Tokenizer();
  
  std::vector<Token> Tokenize(const std::string& sql);
  std::vector<Token> TokenizeUpTo(const std::string& sql, size_t position);
};

// ============================================================================
// StatementTypeDetector
// ============================================================================

class StatementTypeDetector {
 public:
  StatementTypeDetector();
  ~StatementTypeDetector();
  
  StatementType Detect(const std::string& sql);
  StatementType Detect(const std::vector<Token>& tokens) const;
  
  static StatementType DetectStatic(const std::string& sql);
  static std::string ToString(StatementType type);
};

// ============================================================================
// ContextAnalyzer
// ============================================================================

class ContextAnalyzer {
 public:
  ContextAnalyzer();
  ~ContextAnalyzer();
  
  ParseContext Analyze(const std::string& sql, size_t cursor_pos);
  
  ContextType DetermineContextType(const ParseContext& context);
  std::string ExtractCurrentWord(const std::string& sql, size_t cursor_pos);
  std::vector<std::string> ExtractTableReferences(const std::vector<Token>& tokens);
  std::vector<std::string> ExtractColumnReferences(const std::vector<Token>& tokens);
};

// ============================================================================
// CompletionProvider
// ============================================================================

class CompletionProvider {
 public:
  CompletionProvider();
  ~CompletionProvider();
  
  void AddKeyword(const std::string& keyword);
  void AddKeywords(const std::vector<std::string>& keywords);
  
  void AddTable(const std::string& table);
  void AddTables(const std::vector<std::string>& tables);
  
  void AddColumn(const std::string& column);
  void AddColumns(const std::vector<std::string>& columns);
  
  void AddFunction(const std::string& function);
  void AddFunctions(const std::vector<std::string>& functions);
  
  std::vector<CompletionItem> GetCompletions(const ParseContext& context,
                                              const std::string& prefix = "");
  
  void Clear();
  
 private:
  bool MatchesPrefix(const std::string& text, const std::string& prefix);
  
  std::unordered_set<std::string> keywords_;
  std::unordered_set<std::string> tables_;
  std::unordered_set<std::string> columns_;
  std::unordered_set<std::string> functions_;
};

// ============================================================================
// ScratchBirdContextParser
// ============================================================================

class ScratchBirdContextParser {
 public:
  ScratchBirdContextParser();
  ~ScratchBirdContextParser();
  
  // Main parsing methods
  ParseContext Parse(const std::string& sql, size_t cursor_pos);
  std::vector<Token> Tokenize(const std::string& sql);
  StatementType DetectStatementType(const std::string& sql);
  
  // Completion methods
  std::vector<CompletionItem> GetCompletions(const std::string& sql,
                                             size_t cursor_pos,
                                             CompletionProvider* provider = nullptr);
  
  // Context query methods
  ContextType GetContextType(const std::string& sql, size_t cursor_pos);
  std::string GetCurrentWord(const std::string& sql, size_t cursor_pos);
  
  // Extraction methods
  std::vector<std::string> ExtractTables(const std::string& sql);
  std::vector<std::string> ExtractColumns(const std::string& sql);
  
 private:
  Tokenizer tokenizer_;
  ContextAnalyzer analyzer_;
};

}  // namespace core
}  // namespace scratchrobin
