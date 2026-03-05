/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/query_profiler.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

QueryProfiler& QueryProfiler::Instance() {
  static QueryProfiler instance;
  return instance;
}

// ============================================================================
// Profile a Query
// ============================================================================

ProfileResult QueryProfiler::Profile(const std::string& query) {
  ProfileResult result;
  result.query_text = query;
  result.profiled_at = std::chrono::system_clock::now();
  
  // Measure parse time
  auto start = std::chrono::high_resolution_clock::now();
  // Parse would happen here
  auto end = std::chrono::high_resolution_clock::now();
  result.timing.parse_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  // Get query plan
  result.plan = GetQueryPlan(query);
  
  // Simulate execution timing
  result.timing.total_time = result.timing.parse_time;
  result.success = true;
  
  SaveProfileResult(result);
  return result;
}

ProfileResult QueryProfiler::ProfileWithAnalyze(const std::string& query) {
  ProfileResult result = Profile(query);
  
  // Would run EXPLAIN ANALYZE equivalent
  result.plan.execution_time = "Execution time would be populated here";
  
  return result;
}

// ============================================================================
// Query Plan
// ============================================================================

QueryPlan QueryProfiler::GetQueryPlan(const std::string& query) {
  QueryPlan plan;
  plan.output_format = "text";
  
  // Would execute EXPLAIN and parse results
  // This is a stub implementation
  auto root = std::make_shared<QueryPlanNode>();
  root->operation = "Seq Scan";
  plan.nodes.push_back(root);
  
  return plan;
}

// ============================================================================
// Timing
// ============================================================================

QueryTiming QueryProfiler::TimeQuery(const std::string& query) {
  auto start = std::chrono::high_resolution_clock::now();
  
  // Would execute query here
  
  auto end = std::chrono::high_resolution_clock::now();
  
  QueryTiming timing;
  timing.total_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  return timing;
}

// ============================================================================
// Comparison
// ============================================================================

ProfileComparison QueryProfiler::Compare(const ProfileResult& baseline,
                                         const ProfileResult& current) {
  ProfileComparison comparison;
  comparison.baseline = baseline;
  comparison.current = current;
  
  auto baseline_total = baseline.timing.GetTotalClientTime().count();
  auto current_total = current.timing.GetTotalClientTime().count();
  
  if (baseline_total > 0) {
    comparison.time_change_percent = 
        ((static_cast<double>(current_total) - baseline_total) / baseline_total) * 100.0;
  }
  
  if (baseline.plan.total_cost > 0) {
    comparison.cost_change_percent = 
        ((current.plan.total_cost - baseline.plan.total_cost) / baseline.plan.total_cost) * 100.0;
  }
  
  comparison.is_improvement = comparison.time_change_percent < 0;
  
  std::ostringstream summary;
  summary << "Time change: " << std::fixed << std::setprecision(2) 
          << comparison.time_change_percent << "%";
  comparison.summary = summary.str();
  
  return comparison;
}

// ============================================================================
// Profile History
// ============================================================================

void QueryProfiler::SaveProfileResult(const ProfileResult& result) {
  history_.push_back(result);
  TrimHistory();
}

std::vector<ProfileResult> QueryProfiler::GetProfileHistory(
    const std::string& query_hash) {
  std::vector<ProfileResult> results;
  for (const auto& result : history_) {
    if (ComputeQueryHash(result.query_text) == query_hash) {
      results.push_back(result);
    }
  }
  return results;
}

ProfileResult QueryProfiler::GetBestProfile(const std::string& query_hash) {
  auto history = GetProfileHistory(query_hash);
  if (history.empty()) {
    return ProfileResult{};
  }
  
  auto best = history[0];
  auto best_time = best.timing.GetTotalClientTime();
  
  for (const auto& result : history) {
    auto time = result.timing.GetTotalClientTime();
    if (time < best_time) {
      best = result;
      best_time = time;
    }
  }
  
  return best;
}

ProfileResult QueryProfiler::GetWorstProfile(const std::string& query_hash) {
  auto history = GetProfileHistory(query_hash);
  if (history.empty()) {
    return ProfileResult{};
  }
  
  auto worst = history[0];
  auto worst_time = worst.timing.GetTotalClientTime();
  
  for (const auto& result : history) {
    auto time = result.timing.GetTotalClientTime();
    if (time > worst_time) {
      worst = result;
      worst_time = time;
    }
  }
  
  return worst;
}

// ============================================================================
// Clear History
// ============================================================================

void QueryProfiler::ClearHistory() {
  history_.clear();
}

void QueryProfiler::ClearHistoryForQuery(const std::string& query_hash) {
  history_.erase(
      std::remove_if(history_.begin(), history_.end(),
                     [&query_hash, this](const ProfileResult& r) {
                       return ComputeQueryHash(r.query_text) == query_hash;
                     }),
      history_.end());
}

// ============================================================================
// Export
// ============================================================================

bool QueryProfiler::ExportProfileToJson(const ProfileResult& result,
                                        const std::string& filepath) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  file << "{\n";
  file << "  \"query\": \"" << result.query_text << "\",\n";
  file << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
  file << "  \"timing\": {\n";
  file << "    \"parse_us\": " << result.timing.parse_time.count() << ",\n";
  file << "    \"plan_us\": " << result.timing.plan_time.count() << ",\n";
  file << "    \"execution_us\": " << result.timing.execution_time.count() << ",\n";
  file << "    \"total_us\": " << result.timing.total_time.count() << "\n";
  file << "  }\n";
  file << "}\n";
  
  return true;
}

bool QueryProfiler::ExportProfileToText(const ProfileResult& result,
                                        const std::string& filepath) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return false;
  }
  
  file << "Query Profile\n";
  file << "=============\n\n";
  file << "Query: " << result.query_text << "\n\n";
  file << "Timing:\n";
  file << "  Parse:     " << result.timing.parse_time.count() << " us\n";
  file << "  Plan:      " << result.timing.plan_time.count() << " us\n";
  file << "  Execution: " << result.timing.execution_time.count() << " us\n";
  file << "  Total:     " << result.timing.total_time.count() << " us\n";
  
  return true;
}

// ============================================================================
// Visualization
// ============================================================================

std::string QueryProfiler::GeneratePlanVisualization(const QueryPlan& plan) {
  std::ostringstream viz;
  
  viz << "Query Plan Visualization\n";
  viz << "========================\n\n";
  
  for (const auto& node : plan.nodes) {
    viz << node->operation;
    if (!node->object_name.empty()) {
      viz << " on " << node->object_name;
    }
    viz << " (cost=" << node->cost << ")\n";
  }
  
  return viz.str();
}

std::string QueryProfiler::GenerateFlameGraph(const ProfileResult& result) {
  // Would generate flame graph data format
  return "Flame graph data would be generated here";
}

std::string QueryProfiler::GenerateTimeline(const ProfileResult& result) {
  std::ostringstream timeline;
  
  timeline << "Query Timeline:\n";
  timeline << "  Parse:     [" << std::string(result.timing.parse_time.count() / 1000 + 1, '=') << "]\n";
  timeline << "  Plan:      [" << std::string(result.timing.plan_time.count() / 1000 + 1, '=') << "]\n";
  timeline << "  Execution: [" << std::string(result.timing.execution_time.count() / 1000 + 1, '=') << "]\n";
  
  return timeline.str();
}

// ============================================================================
// Suggestions
// ============================================================================

std::vector<std::string> QueryProfiler::GetOptimizationSuggestions(
    const ProfileResult& result) {
  std::vector<std::string> suggestions;
  
  // Analyze plan for optimization opportunities
  for (const auto& node : result.plan.nodes) {
    if (node->operation == "Seq Scan") {
      suggestions.push_back("Consider adding an index to avoid sequential scan");
    }
    if (node->estimated_rows > 10000 && node->index_name.empty()) {
      suggestions.push_back("Large table scan without index on " + node->object_name);
    }
  }
  
  if (result.timing.total_time.count() > 1000000) {  // > 1 second
    suggestions.push_back("Query execution time exceeds 1 second - consider optimization");
  }
  
  return suggestions;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string QueryProfiler::ComputeQueryHash(const std::string& query) {
  // Simple hash implementation
  std::hash<std::string> hasher;
  return std::to_string(hasher(query));
}

QueryPlan QueryProfiler::ParsePlanText(const std::string& plan_text) {
  QueryPlan plan;
  // Would parse EXPLAIN output format
  return plan;
}

void QueryProfiler::TrimHistory() {
  while (static_cast<int>(history_.size()) > max_history_size_) {
    history_.erase(history_.begin());
  }
}

}  // namespace scratchrobin::core
