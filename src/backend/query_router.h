/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "backend/query_response.h"
#include "core/status.h"

namespace scratchrobin::backend {

// Forward declarations
class SessionClient;
class ScratchbirdConnection;
struct ScratchbirdRuntimeConfig;

/**
 * Query classification types for routing decisions
 */
enum class QueryType {
  kUnknown,
  // DDL - Data Definition Language (routed to direct SQL)
  kDdlCreate,
  kDdlAlter,
  kDdlDrop,
  kDdlTruncate,
  // DML - Data Manipulation Language (routed to native SBLR)
  kDmlSelect,
  kDmlInsert,
  kDmlUpdate,
  kDmlDelete,
  kDmlMerge,
  kDmlUpsert,
  // Transaction Control
  kTxnBegin,
  kTxnCommit,
  kTxnRollback,
  kTxnSavepoint,
  // Utility/Admin
  kUtilityShow,
  kUtilitySet,
  kUtilityExplain,
  kUtilityAnalyze,
  // Programmatic
  kCallProcedure,
  kExecuteBlock,
};

/**
 * Execution policy for query routing
 */
struct ExecutionPolicy {
  bool prefer_native = true;      // Prefer SBLR compilation when possible
  bool allow_direct_sql = true;   // Allow direct SQL for DDL/utility
  bool require_bytecode = false;  // Require SBLR bytecode (strict mode)
  uint32_t timeout_ms = 30000;    // Query timeout
};

/**
 * QueryRouter - Centralized query routing and execution
 *
 * Unifies the dual backend paths:
 * - DDL/Utility queries → Direct SQL (ScratchbirdConnection)
 * - DML queries → Native SBLR (SessionClient → QueryCompilerV3 → SBWP)
 *
 * This eliminates the architectural confusion where MainWindow used
 * db_connection_ directly while panels received session_client_.
 */
class QueryRouter {
 public:
  QueryRouter();
  ~QueryRouter();

  // Disable copy, enable move
  QueryRouter(const QueryRouter&) = delete;
  QueryRouter& operator=(const QueryRouter&) = delete;
  QueryRouter(QueryRouter&&) noexcept;
  QueryRouter& operator=(QueryRouter&&) noexcept;

  /**
   * Initialize with backend components
   */
  void setSessionClient(SessionClient* client);
  void setDirectConnection(ScratchbirdConnection* connection);
  void setRuntimeConfig(const ScratchbirdRuntimeConfig* config);

  /**
   * Main execution entry point - routes based on query type
   */
  QueryResponse execute(const std::string& sql,
                        const ExecutionPolicy& policy = {});

  /**
   * Execute with explicit routing (bypass auto-detection)
   */
  QueryResponse executeDirectSql(const std::string& sql);
  QueryResponse executeNative(const std::string& sql,
                              const ExecutionPolicy& policy = {});

  /**
   * Query classification (exposed for testing)
   */
  static QueryType classifyQuery(const std::string& sql);
  static bool isDdl(QueryType type);
  static bool isDml(QueryType type);
  static bool isTransactionControl(QueryType type);
  static const char* queryTypeToString(QueryType type);

  /**
   * Statistics
   */
  struct Statistics {
    uint64_t total_queries = 0;
    uint64_t native_queries = 0;
    uint64_t direct_sql_queries = 0;
    uint64_t compilation_errors = 0;
    uint64_t execution_errors = 0;
  };
  Statistics statistics() const { return stats_; }
  void resetStatistics() { stats_ = {}; }

  /**
   * Query callback for progress/monitoring
   */
  using ProgressCallback = std::function<void(const std::string& stage,
                                               const std::string& detail)>;
  void setProgressCallback(ProgressCallback cb) { progress_callback_ = cb; }

 private:
  void notifyProgress(const std::string& stage, const std::string& detail);

  SessionClient* session_client_ = nullptr;
  ScratchbirdConnection* direct_connection_ = nullptr;
  const ScratchbirdRuntimeConfig* runtime_config_ = nullptr;

  Statistics stats_;
  ProgressCallback progress_callback_;
};

}  // namespace scratchrobin::backend
