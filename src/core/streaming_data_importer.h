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

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/result_set.h"
#include "core/status.h"

namespace scratchrobin::core {

// Forward declarations
class Connection;

// Import data formats
enum class ImportFormat {
  kCSV,
  kJSON,
  kXML,
  kParquet,
  kExcel,
  kFixedWidth,
  kCustom
};

// Import mode
enum class ImportMode {
  kInsert,
  kUpdate,
  kUpsert,
  kReplace,
  kAppend
};

// CSV parsing options
struct CSVOptions {
  char delimiter{','};
  char quote{'"'};
  char escape{'\\'};
  bool has_header{true};
  bool trim_whitespace{false};
  std::string null_value;
  std::string encoding{"UTF-8"};
  int skip_lines{0};
  std::vector<int> ignore_columns;
};

// JSON parsing options
struct JSONOptions {
  std::string root_path;
  std::string array_path;
  bool flatten_nested{true};
  std::string date_format;
  std::string encoding{"UTF-8"};
};

// Column mapping
struct ColumnMapping {
  std::string source_column;
  std::string target_column;
  std::string data_type;
  std::string transform_expression;
  bool nullable{true};
  std::optional<std::string> default_value;
};

// Import configuration
struct ImportConfig {
  std::string import_id;
  std::string source_path;
  ImportFormat format{ImportFormat::kCSV};
  ImportMode mode{ImportMode::kInsert};
  std::string target_table;
  std::string target_schema;
  std::shared_ptr<Connection> connection;
  
  // Format-specific options
  CSVOptions csv_options;
  JSONOptions json_options;
  std::map<std::string, std::string> custom_options;
  
  // Column mappings
  std::vector<ColumnMapping> column_mappings;
  bool auto_map_columns{true};
  
  // Performance options
  int batch_size{1000};
  int commit_frequency{10000};
  bool use_transactions{true};
  int max_errors{100};
  int64_t max_rows{0};  // 0 = unlimited
  int64_t skip_rows{0};
  int64_t limit_rows{0};  // 0 = unlimited
  
  // Validation
  bool validate_data{true};
  bool abort_on_error{false};
  bool continue_on_error{true};
};

// Import progress
struct ImportProgress {
  std::string import_id;
  int64_t bytes_read{0};
  int64_t total_bytes{0};
  int64_t rows_read{0};
  int64_t rows_processed{0};
  int64_t rows_inserted{0};
  int64_t rows_updated{0};
  int64_t rows_skipped{0};
  int64_t rows_failed{0};
  int64_t current_batch{0};
  double percentage_complete{0.0};
  std::chrono::milliseconds elapsed_time{0};
  std::chrono::milliseconds estimated_remaining{0};
  double rows_per_second{0.0};
  std::string current_operation;
  std::string status_message;
};

// Import result
struct ImportResult {
  Status status;
  std::string import_id;
  int64_t total_rows_read{0};
  int64_t total_rows_processed{0};
  int64_t total_rows_inserted{0};
  int64_t total_rows_updated{0};
  int64_t total_rows_skipped{0};
  int64_t total_rows_failed{0};
  std::chrono::milliseconds total_duration{0};
  double average_rows_per_second{0.0};
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::map<std::string, int64_t> column_statistics;
};

// Data validation error
struct ValidationError {
  int64_t row_number{0};
  std::string column_name;
  std::string error_type;
  std::string error_message;
  std::string value;
};

// ============================================================================
// StreamingDataImporter
// ============================================================================

class StreamingDataImporter {
 public:
  using ProgressCallback = std::function<void(const ImportProgress&)>;
  using ErrorCallback = std::function<bool(const ValidationError&)>;
  using RowCallback = std::function<Status(const std::map<std::string, std::string>&)>;
  using CompletionCallback = std::function<void(const ImportResult&)>;

  StreamingDataImporter();
  ~StreamingDataImporter();

  // Disable copy
  StreamingDataImporter(const StreamingDataImporter&) = delete;
  StreamingDataImporter& operator=(const StreamingDataImporter&) = delete;

  // Import operations
  ImportResult ExecuteImport(const ImportConfig& config);
  void ExecuteImportAsync(const ImportConfig& config);
  
  // Streaming import with callbacks
  Status StreamImport(const ImportConfig& config,
                      RowCallback row_callback,
                      CompletionCallback completion_callback = nullptr);
  
  // Import control
  void CancelImport(const std::string& import_id);
  void PauseImport(const std::string& import_id);
  void ResumeImport(const std::string& import_id);
  bool IsImportRunning(const std::string& import_id) const;
  
  // Progress and results
  ImportProgress GetProgress(const std::string& import_id) const;
  std::optional<ImportResult> GetResult(const std::string& import_id);
  std::vector<ValidationError> GetValidationErrors(const std::string& import_id) const;
  
  // Callbacks
  void SetProgressCallback(ProgressCallback callback);
  void SetErrorCallback(ErrorCallback callback);
  void SetCompletionCallback(CompletionCallback callback);
  
  // Schema detection
  Status DetectSchema(const std::string& source_path,
                      ImportFormat format,
                      const CSVOptions& csv_opts,
                      const JSONOptions& json_opts,
                      std::vector<ColumnMapping>* detected_columns);
  
  // Preview data
  Status PreviewData(const std::string& source_path,
                     ImportFormat format,
                     int max_rows,
                     ResultSet* preview_data);
  
  // Column auto-mapping
  std::vector<ColumnMapping> AutoMapColumns(
      const std::vector<std::string>& source_columns,
      const std::vector<std::string>& target_columns);
  
  // Validation
  std::vector<ValidationError> ValidateImportConfig(const ImportConfig& config);
  
  // Cleanup
  void ClearCompletedImports();
  void ClearAllImports();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
