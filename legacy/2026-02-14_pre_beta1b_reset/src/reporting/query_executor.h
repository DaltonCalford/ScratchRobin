// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "report_cache.h"
#include "report_types.h"
#include "../core/query_types.h"
#include "../core/connection_manager.h"

#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {
namespace reporting {

/**
 * @brief Query execution context
 */
struct ExecutionContext {
    std::string connection_ref;
    std::string role_context;
    std::string environment_id;
    std::map<std::string, std::string> parameters;
    bool use_cache = true;
    int row_limit = 10000;
};

/**
 * @brief Query execution result with metadata
 */
struct ExecutionResult {
    QueryResult result;
    bool from_cache = false;
    std::chrono::milliseconds execution_time{0};
    std::optional<std::string> cache_key;
    std::optional<std::string> error_message;
    bool success = false;
};

/**
 * @brief Audit event for query execution
 */
struct ExecutionAuditEvent {
    std::string event_id;
    std::time_t timestamp;
    std::string actor;
    std::string action;  // run, refresh, alert_eval, subscription
    std::string target_id;  // question_id, dashboard_id, etc.
    std::string connection_ref;
    std::string status;  // success, fail
    std::optional<std::string> error_message;
    std::map<std::string, std::string> metadata;
};

/**
 * @brief Query execution options
 */
struct ExecutionOptions {
    bool validate_only = false;  // Just validate, don't execute
    bool dry_run = false;        // Return generated SQL without running
    bool bypass_cache = false;
    std::optional<int> timeout_seconds;
};

/**
 * @brief Callback for execution events
 */
using ExecutionCallback = std::function<void(const ExecutionResult&)>;
using AuditCallback = std::function<void(const ExecutionAuditEvent&)>;

/**
 * @brief Query executor for reporting
 * 
 * Handles query execution, caching, and audit logging for reporting artifacts.
 */
class QueryExecutor {
public:
    QueryExecutor(ConnectionManager* connection_manager,
                  ReportCache* cache);
    
    // Execute a question
    ExecutionResult ExecuteQuestion(const Question& question,
                                    const ExecutionContext& context,
                                    const ExecutionOptions& options = {});
    
    // Execute a dashboard (all cards)
    std::vector<ExecutionResult> ExecuteDashboard(const Dashboard& dashboard,
                                                  const ExecutionContext& context,
                                                  const ExecutionOptions& options = {});
    
    // Execute raw SQL
    ExecutionResult ExecuteSql(const std::string& sql,
                               const ExecutionContext& context,
                               const ExecutionOptions& options = {});
    
    // Execute with parameters
    ExecutionResult ExecuteWithParameters(const Question& question,
                                          const std::map<std::string, std::string>& param_values,
                                          const ExecutionContext& context);
    
    // Validate a question/query
    bool ValidateQuestion(const Question& question, std::string* error);
    bool ValidateSql(const std::string& sql, const std::string& connection_ref, std::string* error);
    
    // Builder query to SQL conversion
    std::string BuilderQueryToSql(const BuilderQuery& query,
                                  const std::optional<Model>& model = std::nullopt);
    
    // Set callbacks
    void SetAuditCallback(AuditCallback callback);
    void SetExecutionCallback(ExecutionCallback callback);
    
    // Permission check
    bool CanExecute(const std::string& collection_id,
                    const std::string& action,
                    const std::string& role) const;
    
private:
    ConnectionManager* connection_manager_;
    ReportCache* cache_;
    AuditCallback audit_callback_;
    ExecutionCallback execution_callback_;
    
    ExecutionResult ExecuteInternal(const std::string& sql,
                                    const ExecutionContext& context,
                                    const ExecutionOptions& options);
    
    void LogAudit(const ExecutionAuditEvent& event);
    
    std::string GenerateSql(const Question& question,
                            const std::map<std::string, std::string>& param_values);
    
    std::string ApplyParameters(const std::string& sql,
                                const std::vector<Parameter>& parameters,
                                const std::map<std::string, std::string>& values);
    
    std::string EscapeSqlLiteral(const std::string& value);
};

/**
 * @brief Async query execution support
 */
class AsyncQueryExecutor {
public:
    explicit AsyncQueryExecutor(QueryExecutor* executor);
    
    // Execute asynchronously
    std::future<ExecutionResult> ExecuteQuestionAsync(const Question& question,
                                                       const ExecutionContext& context);
    
    std::future<ExecutionResult> ExecuteSqlAsync(const std::string& sql,
                                                  const ExecutionContext& context);
    
    // Cancel running queries
    void Cancel(const std::string& execution_id);
    void CancelAll();
    
private:
    QueryExecutor* executor_;
    std::map<std::string, std::shared_ptr<std::atomic<bool>>> cancel_tokens_;
    mutable std::mutex mutex_;
};

} // namespace reporting
} // namespace scratchrobin
