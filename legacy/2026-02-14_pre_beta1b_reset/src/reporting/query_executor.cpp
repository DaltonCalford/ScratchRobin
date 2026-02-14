// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "query_executor.h"

#include <chrono>
#include <sstream>

namespace scratchrobin {
namespace reporting {

QueryExecutor::QueryExecutor(ConnectionManager* connection_manager,
                              ReportCache* cache)
    : connection_manager_(connection_manager),
      cache_(cache) {}

ExecutionResult QueryExecutor::ExecuteQuestion(const Question& question,
                                                const ExecutionContext& context,
                                                const ExecutionOptions& options) {
    ExecutionResult result;
    
    auto start = std::chrono::steady_clock::now();
    
    // Generate SQL
    std::string sql;
    if (question.sql_mode && question.query.native_sql) {
        sql = *question.query.native_sql;
    } else if (question.query.builder_query) {
        sql = BuilderQueryToSql(*question.query.builder_query);
    } else {
        result.error_message = "Invalid query configuration";
        return result;
    }
    
    // Apply parameters
    sql = ApplyParameters(sql, question.parameters, context.parameters);
    
    // Execute
    result = ExecuteInternal(sql, context, options);
    
    auto end = std::chrono::steady_clock::now();
    result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result;
}

std::vector<ExecutionResult> QueryExecutor::ExecuteDashboard(const Dashboard& dashboard,
                                                              const ExecutionContext& context,
                                                              const ExecutionOptions& options) {
    std::vector<ExecutionResult> results;
    
    for (const auto& card : dashboard.cards) {
        // In real implementation, fetch question from repository
        // and execute it with dashboard filter overrides
        ExecutionContext card_context = context;
        // Apply dashboard filters to card context
        
        ExecutionResult result;
        result.success = false;
        result.error_message = "Dashboard execution not fully implemented";
        results.push_back(result);
    }
    
    return results;
}

ExecutionResult QueryExecutor::ExecuteSql(const std::string& sql,
                                           const ExecutionContext& context,
                                           const ExecutionOptions& options) {
    return ExecuteInternal(sql, context, options);
}

ExecutionResult QueryExecutor::ExecuteWithParameters(const Question& question,
                                                      const std::map<std::string, std::string>& param_values,
                                                      const ExecutionContext& context) {
    ExecutionContext ctx = context;
    ctx.parameters = param_values;
    return ExecuteQuestion(question, ctx, {});
}

bool QueryExecutor::ValidateQuestion(const Question& question, std::string* error) {
    if (question.name.empty()) {
        if (error) *error = "Question name is required";
        return false;
    }
    
    if (question.sql_mode) {
        if (!question.query.native_sql || question.query.native_sql->empty()) {
            if (error) *error = "SQL query is required";
            return false;
        }
    } else {
        if (!question.query.builder_query) {
            if (error) *error = "Builder query is required";
            return false;
        }
    }
    
    return true;
}

bool QueryExecutor::ValidateSql(const std::string& sql, 
                                 const std::string& connection_ref, 
                                 std::string* error) {
    // In real implementation, parse and validate SQL
    if (sql.empty()) {
        if (error) *error = "SQL is empty";
        return false;
    }
    return true;
}

std::string QueryExecutor::BuilderQueryToSql(const BuilderQuery& query,
                                              const std::optional<Model>& model) {
    std::ostringstream sql;
    
    sql << "SELECT ";
    
    // Aggregations
    if (!query.aggregations.empty()) {
        for (size_t i = 0; i < query.aggregations.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << query.aggregations[i].function << "(" << query.aggregations[i].column << ")";
            if (!query.aggregations[i].alias.empty()) {
                sql << " AS " << query.aggregations[i].alias;
            }
        }
    } else {
        sql << "*";
    }
    
    // Breakouts (GROUP BY)
    if (!query.breakouts.empty()) {
        sql << ", ";
        for (size_t i = 0; i < query.breakouts.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << query.breakouts[i].column;
        }
    }
    
    // FROM
    sql << " FROM " << query.source;
    
    // WHERE
    if (!query.filters.empty()) {
        sql << " WHERE ";
        for (size_t i = 0; i < query.filters.size(); ++i) {
            if (i > 0) sql << " AND ";
            sql << query.filters[i].column << " " << query.filters[i].operator_ << " ";
            // Simplified value handling
            sql << "?";
        }
    }
    
    // GROUP BY
    if (!query.breakouts.empty()) {
        sql << " GROUP BY ";
        for (size_t i = 0; i < query.breakouts.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << query.breakouts[i].column;
        }
    }
    
    // ORDER BY
    if (!query.order_by.empty()) {
        sql << " ORDER BY ";
        for (size_t i = 0; i < query.order_by.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << query.order_by[i];
        }
    }
    
    // LIMIT
    sql << " LIMIT " << query.limit;
    
    return sql.str();
}

void QueryExecutor::SetAuditCallback(AuditCallback callback) {
    audit_callback_ = callback;
}

void QueryExecutor::SetExecutionCallback(ExecutionCallback callback) {
    execution_callback_ = callback;
}

bool QueryExecutor::CanExecute(const std::string& collection_id,
                                const std::string& action,
                                const std::string& role) const {
    // In real implementation, check permissions
    return true;
}

ExecutionResult QueryExecutor::ExecuteInternal(const std::string& sql,
                                                const ExecutionContext& context,
                                                const ExecutionOptions& options) {
    ExecutionResult result;
    
    // Check cache if enabled
    if (context.use_cache && cache_ && !options.bypass_cache) {
        std::string cache_key = cache_->GenerateKey(sql, context.parameters, 
                                                     context.connection_ref, "");
        auto cached = cache_->Get(cache_key);
        if (cached) {
            result.result = *cached;
            result.success = true;
            result.from_cache = true;
            result.cache_key = cache_key;
            return result;
        }
    }
    
    // Execute query
    if (!connection_manager_) {
        result.error_message = "No connection manager available";
        return result;
    }
    
    // In real implementation, execute through connection manager
    // For now, return placeholder
    result.success = false;
    result.error_message = "Query execution requires connection manager integration";
    
    // Store in cache if successful
    if (result.success && cache_ && !options.bypass_cache) {
        std::string cache_key = cache_->GenerateKey(sql, context.parameters,
                                                     context.connection_ref, "");
        cache_->Put(cache_key, result.result);
        result.cache_key = cache_key;
    }
    
    return result;
}

void QueryExecutor::LogAudit(const ExecutionAuditEvent& event) {
    if (audit_callback_) {
        audit_callback_(event);
    }
}

std::string QueryExecutor::GenerateSql(const Question& question,
                                        const std::map<std::string, std::string>& param_values) {
    if (question.sql_mode && question.query.native_sql) {
        return ApplyParameters(*question.query.native_sql, question.parameters, param_values);
    } else if (question.query.builder_query) {
        return BuilderQueryToSql(*question.query.builder_query);
    }
    return "";
}

std::string QueryExecutor::ApplyParameters(const std::string& sql,
                                            const std::vector<Parameter>& parameters,
                                            const std::map<std::string, std::string>& values) {
    std::string result = sql;
    
    // Simple parameter substitution
    for (const auto& param : parameters) {
        auto it = values.find(param.id);
        if (it != values.end()) {
            std::string placeholder = "{{" + param.id + "}}";
            size_t pos = result.find(placeholder);
            if (pos != std::string::npos) {
                result.replace(pos, placeholder.length(), EscapeSqlLiteral(it->second));
            }
        }
    }
    
    return result;
}

std::string QueryExecutor::EscapeSqlLiteral(const std::string& value) {
    std::string result;
    result.reserve(value.size() + 2);
    result += "'";
    for (char c : value) {
        if (c == '\'') result += "''";
        else if (c == '\\') result += "\\\\";
        else result += c;
    }
    result += "'";
    return result;
}

// AsyncQueryExecutor implementation

AsyncQueryExecutor::AsyncQueryExecutor(QueryExecutor* executor)
    : executor_(executor) {}

std::future<ExecutionResult> AsyncQueryExecutor::ExecuteQuestionAsync(const Question& question,
                                                                       const ExecutionContext& context) {
    return std::async(std::launch::async, [this, question, context]() {
        return executor_->ExecuteQuestion(question, context);
    });
}

std::future<ExecutionResult> AsyncQueryExecutor::ExecuteSqlAsync(const std::string& sql,
                                                                  const ExecutionContext& context) {
    return std::async(std::launch::async, [this, sql, context]() {
        return executor_->ExecuteSql(sql, context);
    });
}

void AsyncQueryExecutor::Cancel(const std::string& execution_id) {
    // Implementation would signal cancellation to running query
}

void AsyncQueryExecutor::CancelAll() {
    // Cancel all running queries
}

} // namespace reporting
} // namespace scratchrobin
