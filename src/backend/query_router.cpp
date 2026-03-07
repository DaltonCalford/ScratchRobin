/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "backend/query_router.h"

#include <algorithm>
#include <cctype>
#include <regex>

#include "backend/scratchbird_connection.h"
#include "backend/session_client.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

// =============================================================================
// Construction / Destruction
// =============================================================================

QueryRouter::QueryRouter() = default;
QueryRouter::~QueryRouter() = default;

QueryRouter::QueryRouter(QueryRouter&&) noexcept = default;
QueryRouter& QueryRouter::operator=(QueryRouter&&) noexcept = default;

// =============================================================================
// Initialization
// =============================================================================

void QueryRouter::setSessionClient(SessionClient* client) {
  session_client_ = client;
}

void QueryRouter::setDirectConnection(ScratchbirdConnection* connection) {
  direct_connection_ = connection;
}

void QueryRouter::setRuntimeConfig(const ScratchbirdRuntimeConfig* config) {
  runtime_config_ = config;
}

// =============================================================================
// Query Classification
// =============================================================================

QueryType QueryRouter::classifyQuery(const std::string& sql) {
  // Extract first significant keyword (skip comments and whitespace)
  std::string normalized;
  normalized.reserve(sql.size());
  
  bool in_single_quote = false;
  bool in_double_quote = false;
  bool in_line_comment = false;
  bool in_block_comment = false;
  
  for (size_t i = 0; i < sql.size(); ++i) {
    char c = sql[i];
    char next = (i + 1 < sql.size()) ? sql[i + 1] : '\0';
    
    // Handle comments
    if (!in_single_quote && !in_double_quote) {
      if (!in_line_comment && !in_block_comment) {
        if (c == '-' && next == '-') {
          in_line_comment = true;
          continue;
        }
        if (c == '/' && next == '*') {
          in_block_comment = true;
          i++;  // Skip *
          continue;
        }
      } else if (in_line_comment) {
        if (c == '\n') {
          in_line_comment = false;
        }
        continue;
      } else if (in_block_comment) {
        if (c == '*' && next == '/') {
          in_block_comment = false;
          i++;  // Skip /
        }
        continue;
      }
    }
    
    // Handle string literals
    if (!in_line_comment && !in_block_comment) {
      if (c == '\'' && !in_double_quote) {
        in_single_quote = !in_single_quote;
        continue;
      }
      if (c == '"' && !in_single_quote) {
        in_double_quote = !in_double_quote;
        continue;
      }
    }
    
    // Collect non-comment, non-string content
    if (!in_single_quote && !in_double_quote && 
        !in_line_comment && !in_block_comment) {
      normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
  }
  
  // Trim leading whitespace
  size_t start = 0;
  while (start < normalized.size() && std::isspace(static_cast<unsigned char>(normalized[start]))) {
    ++start;
  }
  
  // Extract first word
  size_t end = start;
  while (end < normalized.size() && !std::isspace(static_cast<unsigned char>(normalized[end]))) {
    ++end;
  }
  
  std::string first_word = normalized.substr(start, end - start);
  
  // Also get second word for compound statements (CREATE TABLE, ALTER INDEX, etc.)
  size_t second_start = end;
  while (second_start < normalized.size() && 
         std::isspace(static_cast<unsigned char>(normalized[second_start]))) {
    ++second_start;
  }
  size_t second_end = second_start;
  while (second_end < normalized.size() && 
         !std::isspace(static_cast<unsigned char>(normalized[second_end]))) {
    ++second_end;
  }
  std::string second_word = normalized.substr(second_start, second_end - second_start);
  
  // Classify based on first keyword
  if (first_word == "SELECT" || first_word == "WITH" || first_word == "VALUES") {
    return QueryType::kDmlSelect;
  }
  if (first_word == "INSERT") {
    return QueryType::kDmlInsert;
  }
  if (first_word == "UPDATE") {
    return QueryType::kDmlUpdate;
  }
  if (first_word == "DELETE") {
    return QueryType::kDmlDelete;
  }
  if (first_word == "MERGE") {
    return QueryType::kDmlMerge;
  }
  
  if (first_word == "CREATE") {
    return QueryType::kDdlCreate;
  }
  if (first_word == "ALTER") {
    return QueryType::kDdlAlter;
  }
  if (first_word == "DROP") {
    return QueryType::kDdlDrop;
  }
  if (first_word == "TRUNCATE") {
    return QueryType::kDdlTruncate;
  }
  
  if (first_word == "BEGIN" || first_word == "START") {
    return QueryType::kTxnBegin;
  }
  if (first_word == "COMMIT") {
    return QueryType::kTxnCommit;
  }
  if (first_word == "ROLLBACK" || first_word == "ABORT") {
    return QueryType::kTxnRollback;
  }
  if (first_word == "SAVEPOINT") {
    return QueryType::kTxnSavepoint;
  }
  
  if (first_word == "SHOW") {
    return QueryType::kUtilityShow;
  }
  if (first_word == "SET") {
    return QueryType::kUtilitySet;
  }
  if (first_word == "EXPLAIN") {
    return QueryType::kUtilityExplain;
  }
  if (first_word == "ANALYZE") {
    return QueryType::kUtilityAnalyze;
  }
  
  if (first_word == "EXECUTE" || first_word == "CALL" || first_word == "EXEC") {
    return QueryType::kCallProcedure;
  }
  if (first_word == "EXECUTE" && second_word == "BLOCK") {
    return QueryType::kExecuteBlock;
  }
  
  return QueryType::kUnknown;
}

bool QueryRouter::isDdl(QueryType type) {
  switch (type) {
    case QueryType::kDdlCreate:
    case QueryType::kDdlAlter:
    case QueryType::kDdlDrop:
    case QueryType::kDdlTruncate:
      return true;
    default:
      return false;
  }
}

bool QueryRouter::isDml(QueryType type) {
  switch (type) {
    case QueryType::kDmlSelect:
    case QueryType::kDmlInsert:
    case QueryType::kDmlUpdate:
    case QueryType::kDmlDelete:
    case QueryType::kDmlMerge:
    case QueryType::kDmlUpsert:
      return true;
    default:
      return false;
  }
}

bool QueryRouter::isTransactionControl(QueryType type) {
  switch (type) {
    case QueryType::kTxnBegin:
    case QueryType::kTxnCommit:
    case QueryType::kTxnRollback:
    case QueryType::kTxnSavepoint:
      return true;
    default:
      return false;
  }
}

const char* QueryRouter::queryTypeToString(QueryType type) {
  switch (type) {
    case QueryType::kUnknown: return "UNKNOWN";
    case QueryType::kDdlCreate: return "DDL_CREATE";
    case QueryType::kDdlAlter: return "DDL_ALTER";
    case QueryType::kDdlDrop: return "DDL_DROP";
    case QueryType::kDdlTruncate: return "DDL_TRUNCATE";
    case QueryType::kDmlSelect: return "DML_SELECT";
    case QueryType::kDmlInsert: return "DML_INSERT";
    case QueryType::kDmlUpdate: return "DML_UPDATE";
    case QueryType::kDmlDelete: return "DML_DELETE";
    case QueryType::kDmlMerge: return "DML_MERGE";
    case QueryType::kDmlUpsert: return "DML_UPSERT";
    case QueryType::kTxnBegin: return "TXN_BEGIN";
    case QueryType::kTxnCommit: return "TXN_COMMIT";
    case QueryType::kTxnRollback: return "TXN_ROLLBACK";
    case QueryType::kTxnSavepoint: return "TXN_SAVEPOINT";
    case QueryType::kUtilityShow: return "UTILITY_SHOW";
    case QueryType::kUtilitySet: return "UTILITY_SET";
    case QueryType::kUtilityExplain: return "UTILITY_EXPLAIN";
    case QueryType::kUtilityAnalyze: return "UTILITY_ANALYZE";
    case QueryType::kCallProcedure: return "CALL_PROCEDURE";
    case QueryType::kExecuteBlock: return "EXECUTE_BLOCK";
  }
  return "INVALID";
}

// =============================================================================
// Execution
// =============================================================================

void QueryRouter::notifyProgress(const std::string& stage, 
                                  const std::string& detail) {
  if (progress_callback_) {
    progress_callback_(stage, detail);
  }
}

QueryResponse QueryRouter::execute(const std::string& sql,
                                   const ExecutionPolicy& policy) {
  ++stats_.total_queries;
  
  QueryType type = classifyQuery(sql);
  
  notifyProgress("classify", queryTypeToString(type));
  
  // Route based on query type and policy
  if (policy.require_bytecode) {
    // Strict mode - everything goes through native path
    return executeNative(sql, policy);
  }
  
  if (isDdl(type) || isTransactionControl(type) || type == QueryType::kUtilityShow) {
    // DDL and utility queries go through direct SQL
    if (policy.allow_direct_sql) {
      return executeDirectSql(sql);
    }
    // If direct SQL not allowed, try native (will likely fail for DDL)
    return executeNative(sql, policy);
  }
  
  if (isDml(type)) {
    // DML queries prefer native path when available
    if (policy.prefer_native && session_client_) {
      return executeNative(sql, policy);
    }
    // Fall back to direct SQL
    if (policy.allow_direct_sql) {
      return executeDirectSql(sql);
    }
  }
  
  // Unknown query type - try native first, then fall back
  if (session_client_) {
    return executeNative(sql, policy);
  }
  return executeDirectSql(sql);
}

QueryResponse QueryRouter::executeDirectSql(const std::string& sql) {
  ++stats_.direct_sql_queries;
  notifyProgress("execute", "direct_sql");
  
  if (!direct_connection_) {
    return QueryResponse{
      core::Status::Error("No direct connection available"),
      {},
      "query_router::no_direct_connection"
    };
  }
  
  auto result = direct_connection_->query(sql);
  
  QueryResponse response;
  response.status = result.success ? core::Status::Ok() 
                                   : core::Status::Error(result.error_message);
  response.execution_path = "query_router::direct_sql";
  
  // Convert QueryResult to ResultSet
  for (const auto& col : result.columns) {
    response.result_set.columns.push_back(col.name);
  }
  for (const auto& row : result.rows) {
    response.result_set.rows.push_back(row);
  }
  
  if (!result.success) {
    ++stats_.execution_errors;
  }
  
  return response;
}

QueryResponse QueryRouter::executeNative(const std::string& sql,
                                         const ExecutionPolicy& policy) {
  ++stats_.native_queries;
  notifyProgress("execute", "native_sblr");
  
  if (!session_client_) {
    return QueryResponse{
      core::Status::Error("No session client available for native execution"),
      {},
      "query_router::no_session_client"
    };
  }
  
  // Use parser port 4044 (scratchbird-native dialect)
  // Pass empty dialect to let parser auto-detect
  return session_client_->ExecuteSql(4044, "", sql);
}

}  // namespace scratchrobin::backend
