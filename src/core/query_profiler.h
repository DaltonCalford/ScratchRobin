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

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin::core {

// ============================================================================
// Query Timing
// ============================================================================

struct QueryTiming {
  std::chrono::microseconds parse_time{0};
  std::chrono::microseconds plan_time{0};
  std::chrono::microseconds execution_time{0};
  std::chrono::microseconds fetch_time{0};
  std::chrono::microseconds total_time{0};

  std::chrono::microseconds GetTotalClientTime() const {
    return parse_time + plan_time + execution_time + fetch_time;
  }
};

// ============================================================================
// Query Plan Node
// ============================================================================

struct QueryPlanNode {
  std::string operation;
  std::string object_name;
  std::string object_type;
  double cost{0.0};
  double estimated_rows{0.0};
  int estimated_width{0};
  std::string condition;
  std::string index_name;
  std::vector<std::shared_ptr<QueryPlanNode>> children;

  // Actual statistics (if EXPLAIN ANALYZE)
  std::chrono::microseconds actual_time{0};
  int64_t actual_rows{0};
  int64_t actual_loops{0};
};

// ============================================================================
// Query Plan
// ============================================================================

struct QueryPlan {
  std::vector<std::shared_ptr<QueryPlanNode>> nodes;
  double total_cost{0.0};
  double startup_cost{0.0};
  std::string planning_time;
  std::string execution_time;
  std::string output_format;  // text, json, xml, yaml
};

// ============================================================================
// Query Statistics
// ============================================================================

struct QueryStatistics {
  int64_t rows_affected{0};
  int64_t rows_returned{0};
  int64_t buffer_hits{0};
  int64_t buffer_reads{0};
  int64_t temp_files{0};
  int64_t temp_bytes{0};
  int64_t sort_operations{0};
  int64_t sort_space_used{0};
  int64_t index_scans{0};
  int64_t sequential_scans{0};
  int64_t index_fetches{0};
};

// ============================================================================
// Profile Result
// ============================================================================

struct ProfileResult {
  std::string query_text;
  QueryTiming timing;
  QueryPlan plan;
  QueryStatistics statistics;
  bool success{true};
  std::string error_message;
  std::chrono::system_clock::time_point profiled_at;
};

// ============================================================================
// Profile Comparison
// ============================================================================

struct ProfileComparison {
  ProfileResult baseline;
  ProfileResult current;
  double time_change_percent{0.0};
  double cost_change_percent{0.0};
  bool is_improvement{false};
  std::string summary;
};

// ============================================================================
// Query Profiler
// ============================================================================

class QueryProfiler {
 public:
  static QueryProfiler& Instance();

  // Profile a query
  ProfileResult Profile(const std::string& query);
  ProfileResult ProfileWithAnalyze(const std::string& query);

  // Get query plan only
  QueryPlan GetQueryPlan(const std::string& query);

  // Get timing only
  QueryTiming TimeQuery(const std::string& query);

  // Compare two profile results
  ProfileComparison Compare(const ProfileResult& baseline,
                            const ProfileResult& current);

  // Profile history
  void SaveProfileResult(const ProfileResult& result);
  std::vector<ProfileResult> GetProfileHistory(const std::string& query_hash);
  ProfileResult GetBestProfile(const std::string& query_hash);
  ProfileResult GetWorstProfile(const std::string& query_hash);

  // Clear history
  void ClearHistory();
  void ClearHistoryForQuery(const std::string& query_hash);

  // Export
  bool ExportProfileToJson(const ProfileResult& result,
                           const std::string& filepath);
  bool ExportProfileToText(const ProfileResult& result,
                           const std::string& filepath);

  // Visualization helpers
  std::string GeneratePlanVisualization(const QueryPlan& plan);
  std::string GenerateFlameGraph(const ProfileResult& result);
  std::string GenerateTimeline(const ProfileResult& result);

  // Suggestions
  std::vector<std::string> GetOptimizationSuggestions(const ProfileResult& result);

  // Settings
  void SetMaxHistorySize(int size) { max_history_size_ = size; }
  int GetMaxHistorySize() const { return max_history_size_; }

  void SetAutoAnalyze(bool enabled) { auto_analyze_ = enabled; }
  bool GetAutoAnalyze() const { return auto_analyze_; }

  void SetTimeoutSeconds(int seconds) { timeout_seconds_ = seconds; }
  int GetTimeoutSeconds() const { return timeout_seconds_; }

 private:
  QueryProfiler() = default;
  ~QueryProfiler() = default;

  QueryProfiler(const QueryProfiler&) = delete;
  QueryProfiler& operator=(const QueryProfiler&) = delete;

  std::vector<ProfileResult> history_;
  int max_history_size_{100};
  bool auto_analyze_{true};
  int timeout_seconds_{60};

  std::string ComputeQueryHash(const std::string& query);
  QueryPlan ParsePlanText(const std::string& plan_text);
  void TrimHistory();
};

}  // namespace scratchrobin::core
