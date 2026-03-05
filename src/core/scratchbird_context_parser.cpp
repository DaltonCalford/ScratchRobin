/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "scratchbird_context_parser.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stack>
#include <unordered_set>

namespace scratchrobin {
namespace core {

namespace {

// Normalize string for comparison
std::string NormalizeString(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

// Check if character is valid for SQL identifier start
bool IsIdentifierStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

// Check if character is valid for SQL identifier continuation
bool IsIdentifierPart(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// Trim whitespace from both ends
std::string Trim(const std::string& str) {
  size_t start = 0;
  while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
    ++start;
  }
  size_t end = str.size();
  while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
    --end;
  }
  return str.substr(start, end - start);
}

// SQL keyword set
const std::unordered_set<std::string>& GetSqlKeywords() {
  static const std::unordered_set<std::string> keywords = {
    "select", "from", "where", "insert", "update", "delete", "create", "drop",
    "alter", "table", "index", "view", "trigger", "function", "procedure",
    "join", "inner", "outer", "left", "right", "full", "cross", "on", "using",
    "and", "or", "not", "null", "is", "in", "exists", "between", "like",
    "group", "by", "order", "having", "limit", "offset", "union", "intersect",
    "except", "distinct", "all", "as", "case", "when", "then", "else", "end",
    "cast", "coalesce", "nullif", "with", "recursive", "values", "returning",
    "begin", "commit", "rollback", "savepoint", "transaction", "primary",
    "key", "foreign", "references", "unique", "check", "constraint", "default",
    "auto_increment", "serial", "nextval", "currval", "true", "false",
    "integer", "int", "bigint", "smallint", "decimal", "numeric", "real",
    "float", "double", "char", "varchar", "text", "date", "time", "timestamp",
    "interval", "boolean", "bytea", "uuid", "json", "jsonb", "xml"
  };
  return keywords;
}

bool IsKeyword(const std::string& word) {
  return GetSqlKeywords().find(NormalizeString(word)) != GetSqlKeywords().end();
}

}  // anonymous namespace

// ============================================================================
// ParseContext Implementation
// ============================================================================

ParseContext::ParseContext() = default;

ParseContext::ParseContext(const std::string& sql_text,
                           size_t cursor_position)
    : sql(sql_text),
      cursor_pos(cursor_position) {}

ParseContext::~ParseContext() = default;

bool ParseContext::IsEmpty() const {
  return sql.empty();
}

bool ParseContext::IsCursorAtEnd() const {
  return cursor_pos >= sql.size();
}

size_t ParseContext::GetLineNumber() const {
  size_t line = 1;
  for (size_t i = 0; i < cursor_pos && i < sql.size(); ++i) {
    if (sql[i] == '\n') {
      ++line;
    }
  }
  return line;
}

size_t ParseContext::GetColumnNumber() const {
  size_t col = 1;
  for (size_t i = 0; i < cursor_pos && i < sql.size(); ++i) {
    if (sql[i] == '\n') {
      col = 1;
    } else {
      ++col;
    }
  }
  return col;
}

// ============================================================================
// Token Implementation
// ============================================================================

Token::Token() = default;

Token::Token(TokenType type_val, const std::string& text_val, size_t pos_val)
    : type(type_val),
      text(text_val),
      position(pos_val) {}

Token::~Token() = default;

bool Token::IsKeyword() const {
  return type == TokenType::kKeyword;
}

bool Token::IsIdentifier() const {
  return type == TokenType::kIdentifier;
}

bool Token::IsLiteral() const {
  return type == TokenType::kLiteral;
}

bool Token::IsOperator() const {
  return type == TokenType::kOperator;
}

bool Token::IsPunctuation() const {
  return type == TokenType::kPunctuation;
}

// ============================================================================
// Tokenizer Implementation
// ============================================================================

Tokenizer::Tokenizer() = default;

Tokenizer::~Tokenizer() = default;

std::vector<Token> Tokenizer::Tokenize(const std::string& sql) {
  std::vector<Token> tokens;
  size_t pos = 0;
  
  while (pos < sql.size()) {
    char c = sql[pos];
    
    // Skip whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
      ++pos;
      continue;
    }
    
    // Line comment (-- ...)
    if (c == '-' && pos + 1 < sql.size() && sql[pos + 1] == '-') {
      size_t start = pos;
      pos += 2;
      while (pos < sql.size() && sql[pos] != '\n') {
        ++pos;
      }
      tokens.emplace_back(TokenType::kComment, sql.substr(start, pos - start), start);
      continue;
    }
    
    // Block comment (/* ... */)
    if (c == '/' && pos + 1 < sql.size() && sql[pos + 1] == '*') {
      size_t start = pos;
      pos += 2;
      while (pos + 1 < sql.size() && !(sql[pos] == '*' && sql[pos + 1] == '/')) {
        ++pos;
      }
      if (pos + 1 < sql.size()) {
        pos += 2;
      }
      tokens.emplace_back(TokenType::kComment, sql.substr(start, pos - start), start);
      continue;
    }
    
    // String literal
    if (c == '\'') {
      size_t start = pos;
      ++pos;
      while (pos < sql.size()) {
        if (sql[pos] == '\'' && pos + 1 < sql.size() && sql[pos + 1] == '\'') {
          pos += 2;  // Escaped quote
        } else if (sql[pos] == '\'') {
          ++pos;
          break;
        } else {
          ++pos;
        }
      }
      tokens.emplace_back(TokenType::kLiteral, sql.substr(start, pos - start), start);
      continue;
    }
    
    // Quoted identifier
    if (c == '"') {
      size_t start = pos;
      ++pos;
      while (pos < sql.size() && sql[pos] != '"') {
        ++pos;
      }
      if (pos < sql.size()) {
        ++pos;
      }
      tokens.emplace_back(TokenType::kIdentifier, sql.substr(start, pos - start), start);
      continue;
    }
    
    // Number literal
    if (std::isdigit(static_cast<unsigned char>(c)) || 
        (c == '.' && pos + 1 < sql.size() && std::isdigit(static_cast<unsigned char>(sql[pos + 1])))) {
      size_t start = pos;
      bool has_dot = (c == '.');
      ++pos;
      while (pos < sql.size()) {
        if (sql[pos] == '.' && !has_dot) {
          has_dot = true;
          ++pos;
        } else if (std::isdigit(static_cast<unsigned char>(sql[pos]))) {
          ++pos;
        } else if (sql[pos] == 'e' || sql[pos] == 'E') {
          ++pos;
          if (pos < sql.size() && (sql[pos] == '+' || sql[pos] == '-')) {
            ++pos;
          }
        } else {
          break;
        }
      }
      tokens.emplace_back(TokenType::kLiteral, sql.substr(start, pos - start), start);
      continue;
    }
    
    // Identifier or keyword
    if (IsIdentifierStart(c)) {
      size_t start = pos;
      ++pos;
      while (pos < sql.size() && IsIdentifierPart(sql[pos])) {
        ++pos;
      }
      std::string word = sql.substr(start, pos - start);
      TokenType type = IsKeyword(word) ? TokenType::kKeyword : TokenType::kIdentifier;
      tokens.emplace_back(type, word, start);
      continue;
    }
    
    // Operators (multi-char first)
    if (pos + 1 < sql.size()) {
      std::string two_char = sql.substr(pos, 2);
      if (two_char == "<=" || two_char == ">=" || two_char == "<>" || 
          two_char == "!=" || two_char == "||" || two_char == "->" ||
          two_char == "->>" || two_char == "=>" || two_char == "::") {
        tokens.emplace_back(TokenType::kOperator, two_char, pos);
        pos += 2;
        continue;
      }
    }
    
    // Single char operators and punctuation
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
        c == '=' || c == '<' || c == '>' || c == '~' || c == '!' ||
        c == '&' || c == '|' || c == '^' || c == '#') {
      tokens.emplace_back(TokenType::kOperator, std::string(1, c), pos);
      ++pos;
      continue;
    }
    
    // Punctuation
    if (c == '(' || c == ')' || c == ',' || c == ';' || c == '.' ||
        c == '[' || c == ']' || c == '{' || c == '}') {
      tokens.emplace_back(TokenType::kPunctuation, std::string(1, c), pos);
      ++pos;
      continue;
    }
    
    // Unknown character - skip it
    ++pos;
  }
  
  return tokens;
}

std::vector<Token> Tokenizer::TokenizeUpTo(const std::string& sql, size_t position) {
  std::vector<Token> tokens = Tokenize(sql);
  std::vector<Token> result;
  for (const auto& token : tokens) {
    if (token.position < position) {
      result.push_back(token);
    } else {
      break;
    }
  }
  return result;
}

// ============================================================================
// StatementTypeDetector Implementation
// ============================================================================

StatementTypeDetector::StatementTypeDetector() = default;

StatementTypeDetector::~StatementTypeDetector() = default;

StatementType StatementTypeDetector::DetectStatic(const std::string& sql) {
  std::string trimmed = Trim(sql);
  if (trimmed.empty()) {
    return StatementType::kUnknown;
  }
  
  // Get first word
  size_t pos = 0;
  while (pos < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
    ++pos;
  }
  size_t start = pos;
  while (pos < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[pos]))) {
    ++pos;
  }
  std::string first_word = NormalizeString(trimmed.substr(start, pos - start));
  
  if (first_word == "select") {
    return StatementType::kSelect;
  } else if (first_word == "insert") {
    return StatementType::kInsert;
  } else if (first_word == "update") {
    return StatementType::kUpdate;
  } else if (first_word == "delete") {
    return StatementType::kDelete;
  } else if (first_word == "create") {
    return StatementType::kCreate;
  } else if (first_word == "drop") {
    return StatementType::kDrop;
  } else if (first_word == "alter") {
    return StatementType::kAlter;
  } else if (first_word == "begin" || first_word == "start") {
    return StatementType::kBegin;
  } else if (first_word == "commit") {
    return StatementType::kCommit;
  } else if (first_word == "rollback" || first_word == "abort") {
    return StatementType::kRollback;
  } else if (first_word == "with") {
    return StatementType::kCte;
  } else if (first_word == "explain") {
    return StatementType::kExplain;
  } else if (first_word == "call" || first_word == "execute") {
    return StatementType::kCall;
  } else if (first_word == "set") {
    return StatementType::kSet;
  }
  
  return StatementType::kUnknown;
}

StatementType StatementTypeDetector::Detect(const std::vector<Token>& tokens) const {
  for (const auto& token : tokens) {
    if (token.type == TokenType::kKeyword) {
      std::string word = NormalizeString(token.text);
      if (word == "select") return StatementType::kSelect;
      if (word == "insert") return StatementType::kInsert;
      if (word == "update") return StatementType::kUpdate;
      if (word == "delete") return StatementType::kDelete;
      if (word == "create") return StatementType::kCreate;
      if (word == "drop") return StatementType::kDrop;
      if (word == "alter") return StatementType::kAlter;
      if (word == "begin" || word == "start") return StatementType::kBegin;
      if (word == "commit") return StatementType::kCommit;
      if (word == "rollback" || word == "abort") return StatementType::kRollback;
      if (word == "with") return StatementType::kCte;
      if (word == "explain") return StatementType::kExplain;
      if (word == "call" || word == "execute") return StatementType::kCall;
      if (word == "set") return StatementType::kSet;
    }
  }
  return StatementType::kUnknown;
}

std::string StatementTypeDetector::ToString(StatementType type) {
  switch (type) {
    case StatementType::kSelect: return "SELECT";
    case StatementType::kInsert: return "INSERT";
    case StatementType::kUpdate: return "UPDATE";
    case StatementType::kDelete: return "DELETE";
    case StatementType::kCreate: return "CREATE";
    case StatementType::kDrop: return "DROP";
    case StatementType::kAlter: return "ALTER";
    case StatementType::kBegin: return "BEGIN";
    case StatementType::kCommit: return "COMMIT";
    case StatementType::kRollback: return "ROLLBACK";
    case StatementType::kCte: return "CTE";
    case StatementType::kExplain: return "EXPLAIN";
    case StatementType::kCall: return "CALL";
    case StatementType::kSet: return "SET";
    case StatementType::kUnknown: return "UNKNOWN";
    default: return "UNKNOWN";
  }
}

// ============================================================================
// ContextAnalyzer Implementation
// ============================================================================

ContextAnalyzer::ContextAnalyzer() = default;

ContextAnalyzer::~ContextAnalyzer() = default;

ParseContext ContextAnalyzer::Analyze(const std::string& sql, size_t cursor_pos) {
  ParseContext context(sql, cursor_pos);
  
  // Tokenize up to cursor position
  context.tokens = Tokenizer().TokenizeUpTo(sql, cursor_pos);
  
  // Detect statement type
  StatementTypeDetector detector;
  context.statement_type = detector.Detect(context.tokens);
  
  // Determine context type
  context.context_type = DetermineContextType(context);
  
  // Extract current word at cursor
  context.current_word = ExtractCurrentWord(sql, cursor_pos);
  
  // Extract table references
  context.table_references = ExtractTableReferences(context.tokens);
  
  // Extract column references
  context.column_references = ExtractColumnReferences(context.tokens);
  
  return context;
}

ContextType ContextAnalyzer::DetermineContextType(const ParseContext& context) {
  if (context.tokens.empty()) {
    return ContextType::kUnknown;
  }
  
  // Look at recent tokens to determine context
  size_t n = context.tokens.size();
  
  // Check last few tokens for context clues
  for (int i = static_cast<int>(n) - 1; i >= 0 && i >= static_cast<int>(n) - 5; --i) {
    const auto& token = context.tokens[i];
    if (token.type != TokenType::kKeyword) {
      continue;
    }
    
    std::string kw = NormalizeString(token.text);
    
    // After FROM, JOIN - expecting table
    if (kw == "from" || kw == "join" || kw == "into" || kw == "update" ||
        kw == "table" || kw == "index" || kw == "on") {
      return ContextType::kTableReference;
    }
    
    // After SELECT, WHERE, SET, VALUES - expecting column/expression
    if (kw == "select" || kw == "where" || kw == "set" || kw == "values" ||
        kw == "having" || kw == "group" || kw == "order" || kw == "by") {
      return ContextType::kColumnReference;
    }
    
    // After CREATE - expecting object type
    if (kw == "create") {
      return ContextType::kKeyword;
    }
    
    // After ALTER - expecting object type or column
    if (kw == "alter") {
      return ContextType::kKeyword;
    }
    
    // After ORDER BY, GROUP BY - expecting column
    if ((kw == "order" || kw == "group") && i + 1 < static_cast<int>(n) &&
        NormalizeString(context.tokens[i + 1].text) == "by") {
      return ContextType::kColumnReference;
    }
  }
  
  return ContextType::kGeneral;
}

std::string ContextAnalyzer::ExtractCurrentWord(const std::string& sql, size_t cursor_pos) {
  if (cursor_pos > sql.size()) {
    cursor_pos = sql.size();
  }
  
  // Find word boundaries around cursor
  size_t start = cursor_pos;
  while (start > 0 && IsIdentifierPart(sql[start - 1])) {
    --start;
  }
  
  size_t end = cursor_pos;
  while (end < sql.size() && IsIdentifierPart(sql[end])) {
    ++end;
  }
  
  return sql.substr(start, end - start);
}

std::vector<std::string> ContextAnalyzer::ExtractTableReferences(
    const std::vector<Token>& tokens) {
  std::vector<std::string> tables;
  
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i].type != TokenType::kKeyword) {
      continue;
    }
    
    std::string kw = NormalizeString(tokens[i].text);
    
    // Look for table names after FROM, JOIN, INTO, UPDATE
    if ((kw == "from" || kw == "join" || kw == "into" || kw == "update") &&
        i + 1 < tokens.size()) {
      // Skip opening parenthesis (subquery)
      size_t j = i + 1;
      while (j < tokens.size() && 
             tokens[j].type == TokenType::kPunctuation &&
             tokens[j].text == "(") {
        ++j;
        // Skip to matching )
        int depth = 1;
        while (j < tokens.size() && depth > 0) {
          if (tokens[j].text == "(") ++depth;
          if (tokens[j].text == ")") --depth;
          ++j;
        }
      }
      
      // Get table name
      if (j < tokens.size() && tokens[j].type == TokenType::kIdentifier) {
        tables.push_back(tokens[j].text);
      }
    }
  }
  
  return tables;
}

std::vector<std::string> ContextAnalyzer::ExtractColumnReferences(
    const std::vector<Token>& tokens) {
  std::vector<std::string> columns;
  std::unordered_set<std::string> seen;
  
  for (const auto& token : tokens) {
    if (token.type == TokenType::kIdentifier) {
      std::string col = NormalizeString(token.text);
      if (seen.insert(col).second) {
        columns.push_back(token.text);
      }
    }
  }
  
  return columns;
}

// ============================================================================
// CompletionProvider Implementation
// ============================================================================

CompletionProvider::CompletionProvider() = default;

CompletionProvider::~CompletionProvider() = default;

void CompletionProvider::AddKeyword(const std::string& keyword) {
  keywords_.insert(keyword);
}

void CompletionProvider::AddKeywords(const std::vector<std::string>& keywords) {
  for (const auto& kw : keywords) {
    keywords_.insert(kw);
  }
}

void CompletionProvider::AddTable(const std::string& table) {
  tables_.insert(table);
}

void CompletionProvider::AddTables(const std::vector<std::string>& tables) {
  for (const auto& t : tables) {
    tables_.insert(t);
  }
}

void CompletionProvider::AddColumn(const std::string& column) {
  columns_.insert(column);
}

void CompletionProvider::AddColumns(const std::vector<std::string>& columns) {
  for (const auto& c : columns) {
    columns_.insert(c);
  }
}

void CompletionProvider::AddFunction(const std::string& function) {
  functions_.insert(function);
}

void CompletionProvider::AddFunctions(const std::vector<std::string>& functions) {
  for (const auto& f : functions) {
    functions_.insert(f);
  }
}

std::vector<CompletionItem> CompletionProvider::GetCompletions(
    const ParseContext& context,
    const std::string& prefix) {
  std::vector<CompletionItem> completions;
  std::string lower_prefix = NormalizeString(prefix);
  
  switch (context.context_type) {
    case ContextType::kTableReference:
      // Suggest tables
      for (const auto& table : tables_) {
        if (MatchesPrefix(table, lower_prefix)) {
          completions.push_back(CompletionItem{table, "table", table, "", "", 100});
        }
      }
      break;
      
    case ContextType::kColumnReference:
      // Suggest columns
      for (const auto& col : columns_) {
        if (MatchesPrefix(col, lower_prefix)) {
          completions.push_back(CompletionItem{col, "column", col, "", "", 100});
        }
      }
      // Also suggest functions
      for (const auto& fn : functions_) {
        if (MatchesPrefix(fn, lower_prefix)) {
          completions.push_back(CompletionItem{fn, "function", fn + "()", "", "", 80});
        }
      }
      break;
      
    case ContextType::kKeyword:
      // Suggest keywords
      for (const auto& kw : keywords_) {
        if (MatchesPrefix(kw, lower_prefix)) {
          completions.push_back(CompletionItem{kw, "keyword", kw, "", "", 90});
        }
      }
      break;
      
    case ContextType::kFunctionCall:
      // Suggest functions
      for (const auto& fn : functions_) {
        if (MatchesPrefix(fn, lower_prefix)) {
          completions.push_back(CompletionItem{fn, "function", fn + "()", "", "", 100});
        }
      }
      break;
      
    default:
      // General context - suggest everything
      for (const auto& kw : keywords_) {
        if (MatchesPrefix(kw, lower_prefix)) {
          completions.push_back(CompletionItem{kw, "keyword", kw, "", "", 90});
        }
      }
      for (const auto& table : tables_) {
        if (MatchesPrefix(table, lower_prefix)) {
          completions.push_back(CompletionItem{table, "table", table, "", "", 80});
        }
      }
      for (const auto& fn : functions_) {
        if (MatchesPrefix(fn, lower_prefix)) {
          completions.push_back(CompletionItem{fn, "function", fn + "()", "", "", 70});
        }
      }
      break;
  }
  
  // Sort by priority (descending)
  std::sort(completions.begin(), completions.end(),
            [](const CompletionItem& a, const CompletionItem& b) {
              return a.priority > b.priority;
            });
  
  return completions;
}

bool CompletionProvider::MatchesPrefix(const std::string& text,
                                        const std::string& prefix) {
  if (prefix.empty()) {
    return true;
  }
  std::string lower_text = NormalizeString(text);
  return lower_text.find(prefix) == 0;
}

void CompletionProvider::Clear() {
  keywords_.clear();
  tables_.clear();
  columns_.clear();
  functions_.clear();
}

// ============================================================================
// ScratchBirdContextParser Implementation
// ============================================================================

ScratchBirdContextParser::ScratchBirdContextParser() = default;

ScratchBirdContextParser::~ScratchBirdContextParser() = default;

ParseContext ScratchBirdContextParser::Parse(const std::string& sql, 
                                              size_t cursor_pos) {
  return ContextAnalyzer().Analyze(sql, cursor_pos);
}

std::vector<Token> ScratchBirdContextParser::Tokenize(const std::string& sql) {
  return Tokenizer().Tokenize(sql);
}

StatementType ScratchBirdContextParser::DetectStatementType(const std::string& sql) {
  return StatementTypeDetector::DetectStatic(sql);
}

std::vector<CompletionItem> ScratchBirdContextParser::GetCompletions(
    const std::string& sql,
    size_t cursor_pos,
    CompletionProvider* provider) {
  
  ParseContext context = Parse(sql, cursor_pos);
  
  if (provider) {
    return provider->GetCompletions(context, context.current_word);
  }
  
  return {};
}

ContextType ScratchBirdContextParser::GetContextType(const std::string& sql,
                                                      size_t cursor_pos) {
  ParseContext context = Parse(sql, cursor_pos);
  return context.context_type;
}

std::string ScratchBirdContextParser::GetCurrentWord(const std::string& sql,
                                                      size_t cursor_pos) {
  return ContextAnalyzer().ExtractCurrentWord(sql, cursor_pos);
}

std::vector<std::string> ScratchBirdContextParser::ExtractTables(
    const std::string& sql) {
  auto tokens = Tokenizer().Tokenize(sql);
  return ContextAnalyzer().ExtractTableReferences(tokens);
}

std::vector<std::string> ScratchBirdContextParser::ExtractColumns(
    const std::string& sql) {
  auto tokens = Tokenizer().Tokenize(sql);
  return ContextAnalyzer().ExtractColumnReferences(tokens);
}

}  // namespace core
}  // namespace scratchrobin
