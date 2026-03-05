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

#include <functional>
#include <string>
#include <vector>

#include "status.h"

namespace scratchrobin::core {

// ============================================================================
// Bulk Edit Operation Types
// ============================================================================

enum class BulkEditOperationType {
  kUpdate,
  kDelete,
  kInsert
};

// ============================================================================
// Bulk Edit Criteria
// ============================================================================

struct BulkEditCriteria {
  std::string column_name;
  std::string operator_type;  // =, !=, <, >, <=, >=, LIKE, IN, etc.
  std::string value;
  std::string logical_op;     // AND, OR (for combining criteria)
};

// ============================================================================
// Bulk Edit Change
// ============================================================================

struct BulkEditChange {
  std::string column_name;
  std::string new_value;
  bool is_expression{false};  // If true, new_value is a SQL expression
};

// ============================================================================
// Bulk Edit Preview
// ============================================================================

struct BulkEditPreview {
  int affected_rows{0};
  std::vector<std::vector<std::string>> sample_data;
  std::vector<std::string> columns;
  std::string generated_sql;
  bool has_errors{false};
  std::string error_message;
};

// ============================================================================
// Bulk Edit Result
// ============================================================================

struct BulkEditResult {
  bool success{false};
  int rows_affected{0};
  std::string message;
  std::vector<std::string> errors;
};

// ============================================================================
// Progress Callback
// ============================================================================

using BulkEditProgressCallback = std::function<void(int rows_processed, int total_rows)>;

// ============================================================================
// Bulk Edit Manager
// ============================================================================

class BulkEditManager {
 public:
  static BulkEditManager& Instance();

  // Configuration
  void SetBatchSize(int size) { batch_size_ = size; }
  int GetBatchSize() const { return batch_size_; }

  // Preview operations
  BulkEditPreview PreviewUpdate(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria,
      const std::vector<BulkEditChange>& changes,
      int sample_limit = 100);

  BulkEditPreview PreviewDelete(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria,
      int sample_limit = 100);

  // Execute operations
  BulkEditResult ExecuteUpdate(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria,
      const std::vector<BulkEditChange>& changes,
      BulkEditProgressCallback progress_cb = nullptr);

  BulkEditResult ExecuteDelete(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria,
      BulkEditProgressCallback progress_cb = nullptr);

  // Transaction support
  void BeginTransaction();
  Status CommitTransaction();
  Status RollbackTransaction();
  bool IsInTransaction() const { return in_transaction_; }

  // Safety settings
  void SetRequireCriteria(bool require) { require_criteria_ = require; }
  bool GetRequireCriteria() const { return require_criteria_; }

  void SetMaxAffectedRows(int max_rows) { max_affected_rows_ = max_rows; }
  int GetMaxAffectedRows() const { return max_affected_rows_; }

  // SQL generation
  std::string GenerateUpdateSql(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria,
      const std::vector<BulkEditChange>& changes);

  std::string GenerateDeleteSql(
      const std::string& table_name,
      const std::vector<BulkEditCriteria>& criteria);

  // Validation
  Status ValidateCriteria(const std::vector<BulkEditCriteria>& criteria);
  Status ValidateChanges(const std::vector<BulkEditChange>& changes);

 private:
  BulkEditManager() = default;
  ~BulkEditManager() = default;

  BulkEditManager(const BulkEditManager&) = delete;
  BulkEditManager& operator=(const BulkEditManager&) = delete;

  int batch_size_{1000};
  bool in_transaction_{false};
  bool require_criteria_{true};
  int max_affected_rows_{100000};

  std::string BuildWhereClause(const std::vector<BulkEditCriteria>& criteria);
  std::string EscapeSqlValue(const std::string& value);
};

}  // namespace scratchrobin::core
