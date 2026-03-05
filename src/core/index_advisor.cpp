/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/index_advisor.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

IndexAdvisor& IndexAdvisor::Instance() {
  static IndexAdvisor instance;
  return instance;
}

// ============================================================================
// Query Analysis
// ============================================================================

QueryAnalysisResult IndexAdvisor::AnalyzeQuery(const std::string& query_text) {
  QueryAnalysisResult result;
  result.query_text = query_text;
  
  // Extract tables accessed
  // This is a simplified implementation
  size_t from_pos = query_text.find("FROM");
  if (from_pos != std::string::npos) {
    // Parse table names after FROM
  }
  
  // Extract columns from WHERE clause
  size_t where_pos = query_text.find("WHERE");
  if (where_pos != std::string::npos) {
    std::string where_clause = query_text.substr(where_pos + 5);
    result.columns_filtered = ExtractColumnsFromWhereClause(where_clause);
  }
  
  return result;
}

IndexAnalysisReport IndexAdvisor::AnalyzeQueries(
    const std::vector<std::string>& queries) {
  IndexAnalysisReport report;
  
  for (const auto& query : queries) {
    report.analyzed_queries.push_back(AnalyzeQuery(query));
  }
  
  // Generate recommendations based on analysis
  report.recommendations = GenerateRecommendations(queries).recommendations;
  
  return report;
}

// ============================================================================
// Table Analysis
// ============================================================================

std::vector<IndexRecommendation> IndexAdvisor::AnalyzeTable(
    const std::string& table_name) {
  std::vector<IndexRecommendation> recommendations;
  
  // Would query database for column statistics and usage patterns
  // to generate recommendations
  
  return recommendations;
}

// ============================================================================
// Recommendations
// ============================================================================

IndexAnalysisReport IndexAdvisor::GenerateRecommendations(
    const std::vector<std::string>& frequent_queries) {
  IndexAnalysisReport report;
  
  // Analyze each query
  for (const auto& query : frequent_queries) {
    auto analysis = AnalyzeQuery(query);
    report.analyzed_queries.push_back(analysis);
    
    // Check if columns in WHERE clause are indexed
    for (const auto& column : analysis.columns_filtered) {
      if (!analysis.uses_index) {
        IndexRecommendation rec;
        rec.table_name = analysis.tables_accessed.empty() ? "" : analysis.tables_accessed[0];
        rec.columns.push_back(column);
        rec.index_type = "BTREE";
        rec.estimated_benefit = 70;
        rec.reasoning = "Column used in WHERE clause without index";
        rec.suggested_name = "idx_" + rec.table_name + "_" + column;
        report.recommendations.push_back(rec);
      }
    }
  }
  
  return report;
}

std::vector<IndexRecommendation> IndexAdvisor::FindMissingIndexes(
    const std::string& table_name) {
  std::vector<IndexRecommendation> recommendations;
  // Implementation would analyze table statistics and query patterns
  return recommendations;
}

std::vector<ExistingIndex> IndexAdvisor::FindRedundantIndexes(
    const std::string& table_name) {
  std::vector<ExistingIndex> redundant;
  // Implementation would compare existing indexes to find overlaps
  return redundant;
}

std::vector<ExistingIndex> IndexAdvisor::FindUnusedIndexes(
    int min_days_since_last_use) {
  std::vector<ExistingIndex> unused;
  // Implementation would query index usage statistics
  return unused;
}

// ============================================================================
// Impact Estimation
// ============================================================================

double IndexAdvisor::EstimateIndexBenefit(
    const std::string& table_name,
    const std::vector<std::string>& columns) {
  // Simplified estimation based on column selectivity
  double benefit = 0.0;
  for (const auto& column : columns) {
    benefit += CalculateSelectivity(table_name, column);
  }
  return std::min(benefit * 100.0, 100.0);
}

// ============================================================================
// SQL Generation
// ============================================================================

std::string IndexAdvisor::GenerateCreateIndexSql(
    const IndexRecommendation& recommendation) {
  std::ostringstream sql;
  sql << "CREATE ";
  if (recommendation.is_unique) {
    sql << "UNIQUE ";
  }
  sql << "INDEX " << recommendation.suggested_name;
  sql << " ON " << recommendation.table_name << " (";
  
  for (size_t i = 0; i < recommendation.columns.size(); ++i) {
    if (i > 0) {
      sql << ", ";
    }
    sql << recommendation.columns[i];
  }
  sql << ")";
  
  if (recommendation.is_partial && !recommendation.partial_condition.empty()) {
    sql << " WHERE " << recommendation.partial_condition;
  }
  
  sql << ";";
  return sql.str();
}

std::string IndexAdvisor::GenerateDropIndexSql(const ExistingIndex& index) {
  std::ostringstream sql;
  sql << "DROP INDEX " << index.index_name << ";";
  return sql.str();
}

// ============================================================================
// Helper Methods
// ============================================================================

std::vector<std::string> IndexAdvisor::ExtractColumnsFromWhereClause(
    const std::string& where_clause) {
  std::vector<std::string> columns;
  // Simplified extraction - would need proper SQL parsing
  return columns;
}

std::vector<std::string> IndexAdvisor::ExtractColumnsFromOrderBy(
    const std::string& order_by_clause) {
  std::vector<std::string> columns;
  // Simplified extraction
  return columns;
}

std::vector<std::string> IndexAdvisor::ExtractColumnsFromJoin(
    const std::string& join_condition) {
  std::vector<std::string> columns;
  // Simplified extraction
  return columns;
}

double IndexAdvisor::CalculateSelectivity(const std::string& table_name,
                                          const std::string& column_name) {
  // Would query database for actual selectivity statistics
  return 0.1;  // Default estimate
}

}  // namespace scratchrobin::core
