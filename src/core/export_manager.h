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

// Export format types
enum class ExportFormat {
  kCSV,
  kJSON,
  kXML,
  kExcel,
  kSQL,
  kParquet,
  kMarkdown,
  kHTML,
  kPDF,
  kText,
  kCustom
};

// Export destination
enum class ExportDestination {
  kFile,
  kClipboard,
  kMemory,
  kStream
};

// CSV export options
struct CSVExportOptions {
  char delimiter{','};
  char quote{'"'};
  char escape{'\\'};
  bool include_header{true};
  bool bom{false};
  std::string line_ending{"\n"};
  std::string null_value;
  std::string encoding{"UTF-8"};
  bool trim_strings{false};
};

// JSON export options
struct JSONExportOptions {
  bool pretty_print{true};
  int indent_size{2};
  bool use_arrays{true};
  std::string date_format{"ISO8601"};
  bool include_nulls{true};
  std::string null_value{"null"};
};

// Excel export options
struct ExcelExportOptions {
  std::string sheet_name{"Data"};
  bool auto_width{true};
  bool freeze_header{true};
  std::vector<std::string> header_colors;
  bool include_filters{true};
  int max_rows_per_sheet{1048576};  // Excel limit
};

// SQL export options
struct SQLExportOptions {
  std::string target_dialect;  // e.g., "postgresql", "mysql", "firebird"
  bool include_create_table{true};
  bool include_drop_table{false};
  bool use_bulk_insert{true};
  int bulk_insert_size{1000};
  bool include_transactions{true};
  bool quote_identifiers{true};
};

// Column export configuration
struct ExportColumn {
  std::string name;
  std::string alias;
  std::string data_type;
  std::string format_pattern;
  bool include{true};
  std::string transform_expression;
};

// Export configuration
struct ExportConfig {
  std::string export_id;
  std::string source_query;
  std::string source_table;
  std::shared_ptr<Connection> connection;
  ExportFormat format{ExportFormat::kCSV};
  ExportDestination destination{ExportDestination::kFile};
  std::string output_path;
  
  // Column selection
  std::vector<ExportColumn> columns;
  bool export_all_columns{true};
  
  // Format options
  CSVExportOptions csv_options;
  JSONExportOptions json_options;
  ExcelExportOptions excel_options;
  SQLExportOptions sql_options;
  std::map<std::string, std::string> custom_options;
  
  // Data options
  int64_t max_rows{0};  // 0 = unlimited
  int64_t skip_rows{0};
  std::string where_clause;
  std::string order_by;
  
  // Performance
  int batch_size{1000};
  bool compress_output{false};
  std::string compression_format;  // "gzip", "zip"
};

// Export progress
struct ExportProgress {
  std::string export_id;
  int64_t rows_processed{0};
  int64_t total_rows{0};
  int64_t bytes_written{0};
  double percentage_complete{0.0};
  std::chrono::milliseconds elapsed_time{0};
  std::chrono::milliseconds estimated_remaining{0};
  double rows_per_second{0.0};
  std::string current_operation;
  std::string status_message;
};

// Export result
struct ExportResult {
  Status status;
  std::string export_id;
  std::string output_path;
  int64_t total_rows{0};
  int64_t total_bytes{0};
  std::chrono::milliseconds duration{0};
  double average_rows_per_second{0.0};
  std::vector<std::string> warnings;
  std::vector<std::string> errors;
  std::map<std::string, std::string> metadata;
};

// Export template
struct ExportTemplate {
  std::string template_id;
  std::string template_name;
  std::string description;
  ExportConfig config;
  std::chrono::system_clock::time_point created_at;
  std::string created_by;
};

// ============================================================================
// ExportManager
// ============================================================================

class ExportManager {
 public:
  using ProgressCallback = std::function<void(const ExportProgress&)>;
  using CompletionCallback = std::function<void(const ExportResult&)>;
  using RowTransformCallback = 
      std::function<Status(const std::map<std::string, std::string>& row,
                           std::map<std::string, std::string>* transformed)>;

  ExportManager();
  ~ExportManager();

  // Disable copy
  ExportManager(const ExportManager&) = delete;
  ExportManager& operator=(const ExportManager&) = delete;

  // Export operations
  ExportResult ExecuteExport(const ExportConfig& config);
  void ExecuteExportAsync(const ExportConfig& config);
  
  // Export from result set (for already-executed queries)
  ExportResult ExportResultSet(const ResultSet& result_set,
                               const ExportConfig& config);
  
  // Export control
  void CancelExport(const std::string& export_id);
  void PauseExport(const std::string& export_id);
  void ResumeExport(const std::string& export_id);
  bool IsExportRunning(const std::string& export_id) const;
  
  // Progress and results
  ExportProgress GetProgress(const std::string& export_id) const;
  std::optional<ExportResult> GetResult(const std::string& export_id);
  
  // Callbacks
  void SetProgressCallback(ProgressCallback callback);
  void SetCompletionCallback(CompletionCallback callback);
  void SetRowTransformCallback(RowTransformCallback callback);
  
  // Templates
  Status SaveTemplate(const std::string& name,
                      const ExportConfig& config,
                      const std::string& description = "");
  Status DeleteTemplate(const std::string& template_id);
  std::vector<ExportTemplate> ListTemplates() const;
  std::optional<ExportTemplate> GetTemplate(const std::string& template_id) const;
  ExportResult ExportUsingTemplate(const std::string& template_id,
                                   const std::map<std::string, std::string>& params);
  
  // Quick export helpers
  ExportResult ExportToCSV(const std::string& query,
                           std::shared_ptr<Connection> connection,
                           const std::string& output_path,
                           const CSVExportOptions& options = {});
  ExportResult ExportToJSON(const std::string& query,
                            std::shared_ptr<Connection> connection,
                            const std::string& output_path,
                            const JSONExportOptions& options = {});
  ExportResult ExportToExcel(const std::string& query,
                             std::shared_ptr<Connection> connection,
                             const std::string& output_path,
                             const ExcelExportOptions& options = {});
  ExportResult ExportToSQL(const std::string& query,
                           std::shared_ptr<Connection> connection,
                           const std::string& output_path,
                           const SQLExportOptions& options = {});
  
  // Preview
  Status PreviewExport(const ExportConfig& config,
                       int max_rows,
                       std::string* preview_data);
  
  // Import/Export configuration
  Status ExportConfigToFile(const ExportConfig& config,
                            const std::string& file_path);
  Status ImportConfigFromFile(const std::string& file_path,
                              ExportConfig* config);
  
  // Cleanup
  void ClearCompletedExports();
  void ClearAllExports();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace scratchrobin::core
