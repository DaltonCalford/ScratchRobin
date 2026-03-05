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

#include "status.h"

namespace scratchrobin::core {

// ============================================================================
// Report Format
// ============================================================================

enum class ReportFormat {
  kHtml,
  kPdf,
  kCsv,
  kExcel,
  kJson,
  kXml,
  kText
};

// ============================================================================
// Report Parameter
// ============================================================================

struct ReportParameter {
  std::string name;
  std::string display_name;
  std::string data_type;  // string, int, date, boolean, etc.
  std::string default_value;
  bool required{false};
  std::vector<std::string> allowed_values;
  std::string description;
};

// ============================================================================
// Report Column
// ============================================================================

struct ReportColumn {
  std::string name;
  std::string display_name;
  std::string data_type;
  int width{100};
  bool visible{true};
  bool sortable{true};
  bool filterable{true};
  std::string format;  // number format, date format, etc.
  std::string alignment;  // left, center, right
};

// ============================================================================
// Report Data
// ============================================================================

struct ReportData {
  std::vector<ReportColumn> columns;
  std::vector<std::vector<std::string>> rows;
  int total_row_count{0};
  std::string summary;
  std::chrono::system_clock::time_point generated_at;
};

// ============================================================================
// Report Definition
// ============================================================================

struct ReportDefinition {
  std::string id;
  std::string name;
  std::string description;
  std::string category;
  std::vector<ReportParameter> parameters;
  std::vector<ReportColumn> columns;
  std::string sql_template;
  std::string custom_script;
  bool is_builtin{true};
  std::string icon_name;
};

// ============================================================================
// Scheduled Report
// ============================================================================

struct ScheduledReport {
  int64_t id{0};
  std::string report_id;
  std::string name;
  std::string schedule;  // cron expression
  std::vector<std::pair<std::string, std::string>> parameters;
  ReportFormat output_format{ReportFormat::kPdf};
  std::string output_path;
  std::string email_recipients;
  bool enabled{true};
  std::chrono::system_clock::time_point last_run;
  std::chrono::system_clock::time_point next_run;
};

// ============================================================================
// Report Manager
// ============================================================================

class ReportManager {
 public:
  static ReportManager& Instance();

  // Report definitions
  std::vector<ReportDefinition> GetAvailableReports();
  std::vector<ReportDefinition> GetReportsByCategory(const std::string& category);
  std::optional<ReportDefinition> GetReportDefinition(const std::string& report_id);

  // Built-in reports
  void RegisterBuiltinReport(const ReportDefinition& report);
  void UnregisterReport(const std::string& report_id);

  // Custom reports
  Status SaveCustomReport(const ReportDefinition& report);
  Status DeleteCustomReport(const std::string& report_id);
  std::vector<ReportDefinition> GetCustomReports();

  // Generate reports
  ReportData GenerateReport(
      const std::string& report_id,
      const std::vector<std::pair<std::string, std::string>>& parameters);

  ReportData GenerateReportFromSql(
      const std::string& sql,
      const std::vector<ReportColumn>& columns);

  // Export
  Status ExportReport(
      const ReportData& data,
      const std::string& filepath,
      ReportFormat format);

  std::string ExportReportToString(
      const ReportData& data,
      ReportFormat format);

  // Format-specific generation
  std::string GenerateHtmlReport(const ReportData& data);
  std::string GenerateCsvReport(const ReportData& data);
  std::string GenerateJsonReport(const ReportData& data);
  std::string GenerateXmlReport(const ReportData& data);
  std::string GenerateTextReport(const ReportData& data);

  // Scheduled reports
  int64_t ScheduleReport(const ScheduledReport& scheduled);
  bool CancelScheduledReport(int64_t schedule_id);
  std::vector<ScheduledReport> GetScheduledReports();
  std::optional<ScheduledReport> GetScheduledReport(int64_t schedule_id);
  void RunScheduledReportNow(int64_t schedule_id);

  // Report categories
  std::vector<std::string> GetReportCategories();
  void AddCategory(const std::string& category);
  void RemoveCategory(const std::string& category);

  // Preview
  ReportData PreviewReport(
      const std::string& report_id,
      const std::vector<std::pair<std::string, std::string>>& parameters,
      int max_rows = 100);

  // Validation
  Status ValidateReportParameters(
      const ReportDefinition& report,
      const std::vector<std::pair<std::string, std::string>>& parameters);

  // Persistence
  bool LoadReportsFromFile(const std::string& filepath);
  bool SaveReportsToFile(const std::string& filepath);

 private:
  ReportManager();
  ~ReportManager() = default;

  ReportManager(const ReportManager&) = delete;
  ReportManager& operator=(const ReportManager&) = delete;

  std::vector<ReportDefinition> builtin_reports_;
  std::vector<ReportDefinition> custom_reports_;
  std::vector<ScheduledReport> scheduled_reports_;
  std::vector<std::string> categories_;
  int64_t next_schedule_id_{1};

  void InitializeBuiltinReports();
  ReportData ExecuteSql(const std::string& sql);
  std::string ReplaceParameters(
      const std::string& sql_template,
      const std::vector<std::pair<std::string, std::string>>& parameters);
};

}  // namespace scratchrobin::core
