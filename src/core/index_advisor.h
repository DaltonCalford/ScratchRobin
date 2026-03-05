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

#include "status.h"

namespace scratchrobin::core {

// ============================================================================
// Index Recommendation
// ============================================================================

struct IndexRecommendation {
  std::string table_name;
  std::vector<std::string> columns;
  std::string index_type;  // BTREE, HASH, GIN, GIST, etc.
  int estimated_benefit{0};  // 0-100 score
  std::string reasoning;
  std::string suggested_name;
  std::string create_sql;
  bool is_unique{false};
  bool is_partial{false};
  std::string partial_condition;
};

// ============================================================================
// Existing Index
// ============================================================================

struct ExistingIndex {
  std::string index_name;
  std::string table_name;
  std::vector<std::string> columns;
  bool is_unique{false};
  bool is_primary{false};
  int usage_count{0};
  std::string last_used;
};

// ============================================================================
// Query Analysis Result
// ============================================================================

struct QueryAnalysisResult {
  std::string query_text;
  std::vector<std::string> tables_accessed;
  std::vector<std::string> columns_filtered;
  std::vector<std::string> columns_sorted;
  std::vector<std::string> columns_joined;
  double estimated_cost{0.0};
  bool uses_index{false};
  std::string current_plan;
};

// ============================================================================
// Index Analysis Report
// ============================================================================

struct IndexAnalysisReport {
  std::vector<IndexRecommendation> recommendations;
  std::vector<ExistingIndex> redundant_indexes;
  std::vector<ExistingIndex> unused_indexes;
  std::vector<QueryAnalysisResult> analyzed_queries;
  int total_tables_analyzed{0};
  int total_indexes_existing{0};
  std::string summary;
};

// ============================================================================
// Index Advisor
// ============================================================================

class IndexAdvisor {
 public:
  static IndexAdvisor& Instance();

  // Analyze a single query
  QueryAnalysisResult AnalyzeQuery(const std::string& query_text);

  // Analyze multiple queries and generate recommendations
  IndexAnalysisReport AnalyzeQueries(const std::vector<std::string>& queries);

  // Analyze a specific table
  std::vector<IndexRecommendation> AnalyzeTable(const std::string& table_name);

  // Generate recommendations based on query history
  IndexAnalysisReport GenerateRecommendations(
      const std::vector<std::string>& frequent_queries);

  // Check for missing indexes
  std::vector<IndexRecommendation> FindMissingIndexes(
      const std::string& table_name);

  // Check for redundant indexes
  std::vector<ExistingIndex> FindRedundantIndexes(
      const std::string& table_name = "");

  // Check for unused indexes
  std::vector<ExistingIndex> FindUnusedIndexes(
      int min_days_since_last_use = 30);

  // Simulate index impact
  double EstimateIndexBenefit(
      const std::string& table_name,
      const std::vector<std::string>& columns);

  // Generate CREATE INDEX statements
  std::string GenerateCreateIndexSql(const IndexRecommendation& recommendation);

  // Generate DROP INDEX statements for unused/redundant indexes
  std::string GenerateDropIndexSql(const ExistingIndex& index);

  // Settings
  void SetMinSelectivityThreshold(double threshold) {
    min_selectivity_threshold_ = threshold;
  }
  double GetMinSelectivityThreshold() const {
    return min_selectivity_threshold_;
  }

  void SetMaxColumnsPerIndex(int max_cols) {
    max_columns_per_index_ = max_cols;
  }
  int GetMaxColumnsPerIndex() const {
    return max_columns_per_index_;
  }

 private:
  IndexAdvisor() = default;
  ~IndexAdvisor() = default;

  IndexAdvisor(const IndexAdvisor&) = delete;
  IndexAdvisor& operator=(const IndexAdvisor&) = delete;

  double min_selectivity_threshold_{0.1};  // 10%
  int max_columns_per_index_{5};

  std::vector<std::string> ExtractColumnsFromWhereClause(
      const std::string& where_clause);
  std::vector<std::string> ExtractColumnsFromOrderBy(
      const std::string& order_by_clause);
  std::vector<std::string> ExtractColumnsFromJoin(
      const std::string& join_condition);
  double CalculateSelectivity(const std::string& table_name,
                              const std::string& column_name);
};

}  // namespace scratchrobin::core
