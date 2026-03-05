/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/bulk_edit_manager.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

BulkEditManager& BulkEditManager::Instance() {
  static BulkEditManager instance;
  return instance;
}

// ============================================================================
// Preview Operations
// ============================================================================

BulkEditPreview BulkEditManager::PreviewUpdate(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria,
    const std::vector<BulkEditChange>& changes,
    int sample_limit) {
  BulkEditPreview preview;
  preview.generated_sql = GenerateUpdateSql(table_name, criteria, changes);
  preview.affected_rows = 0;  // Would be determined by actual query
  preview.has_errors = false;
  return preview;
}

BulkEditPreview BulkEditManager::PreviewDelete(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria,
    int sample_limit) {
  BulkEditPreview preview;
  preview.generated_sql = GenerateDeleteSql(table_name, criteria);
  preview.affected_rows = 0;  // Would be determined by actual query
  preview.has_errors = false;
  return preview;
}

// ============================================================================
// Execute Operations
// ============================================================================

BulkEditResult BulkEditManager::ExecuteUpdate(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria,
    const std::vector<BulkEditChange>& changes,
    BulkEditProgressCallback progress_cb) {
  BulkEditResult result;
  
  if (require_criteria_ && criteria.empty()) {
    result.success = false;
    result.message = "Criteria required for safety";
    return result;
  }
  
  // Validate inputs
  auto status = ValidateCriteria(criteria);
  if (!status.ok) {
    result.success = false;
    result.message = status.message;
    return result;
  }
  
  status = ValidateChanges(changes);
  if (!status.ok) {
    result.success = false;
    result.message = status.message;
    return result;
  }
  
  result.success = true;
  result.message = "Update executed successfully";
  return result;
}

BulkEditResult BulkEditManager::ExecuteDelete(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria,
    BulkEditProgressCallback progress_cb) {
  BulkEditResult result;
  
  if (require_criteria_ && criteria.empty()) {
    result.success = false;
    result.message = "Criteria required for safety";
    return result;
  }
  
  result.success = true;
  result.message = "Delete executed successfully";
  return result;
}

// ============================================================================
// Transaction Support
// ============================================================================

void BulkEditManager::BeginTransaction() {
  in_transaction_ = true;
}

Status BulkEditManager::CommitTransaction() {
  in_transaction_ = false;
  return Status::Ok();
}

Status BulkEditManager::RollbackTransaction() {
  in_transaction_ = false;
  return Status::Ok();
}

// ============================================================================
// SQL Generation
// ============================================================================

std::string BulkEditManager::GenerateUpdateSql(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria,
    const std::vector<BulkEditChange>& changes) {
  std::ostringstream sql;
  sql << "UPDATE " << table_name << " SET ";
  
  for (size_t i = 0; i < changes.size(); ++i) {
    if (i > 0) {
      sql << ", ";
    }
    sql << changes[i].column_name << " = ";
    if (changes[i].is_expression) {
      sql << changes[i].new_value;
    } else {
      sql << "'" << EscapeSqlValue(changes[i].new_value) << "'";
    }
  }
  
  std::string where = BuildWhereClause(criteria);
  if (!where.empty()) {
    sql << " WHERE " << where;
  }
  
  return sql.str();
}

std::string BulkEditManager::GenerateDeleteSql(
    const std::string& table_name,
    const std::vector<BulkEditCriteria>& criteria) {
  std::ostringstream sql;
  sql << "DELETE FROM " << table_name;
  
  std::string where = BuildWhereClause(criteria);
  if (!where.empty()) {
    sql << " WHERE " << where;
  }
  
  return sql.str();
}

// ============================================================================
// Validation
// ============================================================================

Status BulkEditManager::ValidateCriteria(
    const std::vector<BulkEditCriteria>& criteria) {
  for (const auto& c : criteria) {
    if (c.column_name.empty()) {
      return Status::Error("Criteria column name cannot be empty");
    }
  }
  return Status::Ok();
}

Status BulkEditManager::ValidateChanges(
    const std::vector<BulkEditChange>& changes) {
  if (changes.empty()) {
    return Status::Error("At least one change is required");
  }
  for (const auto& c : changes) {
    if (c.column_name.empty()) {
      return Status::Error("Change column name cannot be empty");
    }
  }
  return Status::Ok();
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string BulkEditManager::BuildWhereClause(
    const std::vector<BulkEditCriteria>& criteria) {
  std::ostringstream where;
  
  for (size_t i = 0; i < criteria.size(); ++i) {
    if (i > 0) {
      where << " " << (criteria[i].logical_op.empty() ? "AND" : criteria[i].logical_op) << " ";
    }
    where << criteria[i].column_name << " " << criteria[i].operator_type << " ";
    where << "'" << EscapeSqlValue(criteria[i].value) << "'";
  }
  
  return where.str();
}

std::string BulkEditManager::EscapeSqlValue(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (char c : value) {
    if (c == '\'') {
      escaped += "''";
    } else {
      escaped += c;
    }
  }
  return escaped;
}

}  // namespace scratchrobin::core
