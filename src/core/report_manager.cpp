/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/report_manager.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

ReportManager& ReportManager::Instance() {
  static ReportManager instance;
  return instance;
}

ReportManager::ReportManager() {
  InitializeBuiltinReports();
}

// ============================================================================
// Report Definitions
// ============================================================================

std::vector<ReportDefinition> ReportManager::GetAvailableReports() {
  std::vector<ReportDefinition> all = builtin_reports_;
  all.insert(all.end(), custom_reports_.begin(), custom_reports_.end());
  return all;
}

std::vector<ReportDefinition> ReportManager::GetReportsByCategory(
    const std::string& category) {
  std::vector<ReportDefinition> result;
  auto all = GetAvailableReports();
  std::copy_if(all.begin(), all.end(), std::back_inserter(result),
               [&category](const ReportDefinition& r) {
                 return r.category == category;
               });
  return result;
}

std::optional<ReportDefinition> ReportManager::GetReportDefinition(
    const std::string& report_id) {
  auto all = GetAvailableReports();
  for (const auto& report : all) {
    if (report.id == report_id) {
      return report;
    }
  }
  return std::nullopt;
}

// ============================================================================
// Report Registration
// ============================================================================

void ReportManager::RegisterBuiltinReport(const ReportDefinition& report) {
  builtin_reports_.push_back(report);
}

void ReportManager::UnregisterReport(const std::string& report_id) {
  custom_reports_.erase(
      std::remove_if(custom_reports_.begin(), custom_reports_.end(),
                     [&report_id](const ReportDefinition& r) {
                       return r.id == report_id;
                     }),
      custom_reports_.end());
}

// ============================================================================
// Custom Reports
// ============================================================================

Status ReportManager::SaveCustomReport(const ReportDefinition& report) {
  // Check if already exists
  auto it = std::find_if(custom_reports_.begin(), custom_reports_.end(),
                         [&report](const ReportDefinition& r) {
                           return r.id == report.id;
                         });
  if (it != custom_reports_.end()) {
    *it = report;
  } else {
    custom_reports_.push_back(report);
  }
  return Status::Ok();
}

Status ReportManager::DeleteCustomReport(const std::string& report_id) {
  auto it = std::find_if(custom_reports_.begin(), custom_reports_.end(),
                         [&report_id](const ReportDefinition& r) {
                           return r.id == report_id;
                         });
  if (it != custom_reports_.end()) {
    custom_reports_.erase(it);
    return Status::Ok();
  }
  return Status::Error("Report not found: " + report_id);
}

std::vector<ReportDefinition> ReportManager::GetCustomReports() {
  return custom_reports_;
}

// ============================================================================
// Report Generation
// ============================================================================

ReportData ReportManager::GenerateReport(
    const std::string& report_id,
    const std::vector<std::pair<std::string, std::string>>& parameters) {
  ReportData data;
  
  auto def = GetReportDefinition(report_id);
  if (!def.has_value()) {
    data.summary = "Report not found: " + report_id;
    return data;
  }
  
  // Replace parameters in SQL template
  std::string sql = ReplaceParameters(def->sql_template, parameters);
  
  // Execute and populate data
  data = ExecuteSql(sql);
  data.columns = def->columns;
  
  auto now = std::chrono::system_clock::now();
  data.generated_at = now;
  
  return data;
}

ReportData ReportManager::GenerateReportFromSql(
    const std::string& sql,
    const std::vector<ReportColumn>& columns) {
  ReportData data = ExecuteSql(sql);
  data.columns = columns;
  data.generated_at = std::chrono::system_clock::now();
  return data;
}

// ============================================================================
// Export
// ============================================================================

Status ReportManager::ExportReport(
    const ReportData& data,
    const std::string& filepath,
    ReportFormat format) {
  std::string content = ExportReportToString(data, format);
  
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return Status::Error("Failed to open file: " + filepath);
  }
  
  file << content;
  return Status::Ok();
}

std::string ReportManager::ExportReportToString(
    const ReportData& data,
    ReportFormat format) {
  switch (format) {
    case ReportFormat::kHtml:
      return GenerateHtmlReport(data);
    case ReportFormat::kCsv:
      return GenerateCsvReport(data);
    case ReportFormat::kJson:
      return GenerateJsonReport(data);
    case ReportFormat::kXml:
      return GenerateXmlReport(data);
    case ReportFormat::kText:
      return GenerateTextReport(data);
    default:
      return GenerateHtmlReport(data);
  }
}

// ============================================================================
// Format-Specific Generation
// ============================================================================

std::string ReportManager::GenerateHtmlReport(const ReportData& data) {
  std::ostringstream html;
  
  html << "<!DOCTYPE html>\n<html>\n<head>\n";
  html << "<title>Report</title>\n";
  html << "<style>\n";
  html << "table { border-collapse: collapse; width: 100%; }\n";
  html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
  html << "th { background-color: #4CAF50; color: white; }\n";
  html << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
  html << "</style>\n</head>\n<body>\n";
  
  html << "<table>\n<tr>";
  for (const auto& col : data.columns) {
    html << "<th>" << col.display_name << "</th>";
  }
  html << "</tr>\n";
  
  for (const auto& row : data.rows) {
    html << "<tr>";
    for (const auto& cell : row) {
      html << "<td>" << cell << "</td>";
    }
    html << "</tr>\n";
  }
  
  html << "</table>\n</body>\n</html>";
  
  return html.str();
}

std::string ReportManager::GenerateCsvReport(const ReportData& data) {
  std::ostringstream csv;
  
  // Header
  for (size_t i = 0; i < data.columns.size(); ++i) {
    if (i > 0) csv << ",";
    csv << "\"" << data.columns[i].display_name << "\"";
  }
  csv << "\n";
  
  // Data
  for (const auto& row : data.rows) {
    for (size_t i = 0; i < row.size(); ++i) {
      if (i > 0) csv << ",";
      csv << "\"" << row[i] << "\"";
    }
    csv << "\n";
  }
  
  return csv.str();
}

std::string ReportManager::GenerateJsonReport(const ReportData& data) {
  std::ostringstream json;
  
  json << "{\n  \"columns\": [";
  for (size_t i = 0; i < data.columns.size(); ++i) {
    if (i > 0) json << ", ";
    json << "\"" << data.columns[i].name << "\"";
  }
  json << "],\n  \"data\": [\n";
  
  for (size_t r = 0; r < data.rows.size(); ++r) {
    if (r > 0) json << ",\n";
    json << "    [";
    for (size_t c = 0; c < data.rows[r].size(); ++c) {
      if (c > 0) json << ", ";
      json << "\"" << data.rows[r][c] << "\"";
    }
    json << "]";
  }
  
  json << "\n  ]\n}";
  
  return json.str();
}

std::string ReportManager::GenerateXmlReport(const ReportData& data) {
  std::ostringstream xml;
  
  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml << "<report>\n";
  
  for (const auto& row : data.rows) {
    xml << "  <row>\n";
    for (size_t i = 0; i < row.size() && i < data.columns.size(); ++i) {
      xml << "    <" << data.columns[i].name << ">";
      xml << row[i];
      xml << "</" << data.columns[i].name << ">\n";
    }
    xml << "  </row>\n";
  }
  
  xml << "</report>";
  
  return xml.str();
}

std::string ReportManager::GenerateTextReport(const ReportData& data) {
  std::ostringstream text;
  
  // Calculate column widths
  std::vector<size_t> widths;
  for (const auto& col : data.columns) {
    widths.push_back(col.display_name.length());
  }
  for (const auto& row : data.rows) {
    for (size_t i = 0; i < row.size() && i < widths.size(); ++i) {
      widths[i] = std::max(widths[i], row[i].length());
    }
  }
  
  // Header
  for (size_t i = 0; i < data.columns.size(); ++i) {
    if (i > 0) text << " | ";
    text << std::left << std::setw(widths[i]) << data.columns[i].display_name;
  }
  text << "\n";
  
  // Separator
  for (size_t i = 0; i < data.columns.size(); ++i) {
    if (i > 0) text << "-+";
    text << std::string(widths[i], '-');
  }
  text << "\n";
  
  // Data
  for (const auto& row : data.rows) {
    for (size_t i = 0; i < row.size(); ++i) {
      if (i > 0) text << " | ";
      text << std::left << std::setw(widths[i]) << row[i];
    }
    text << "\n";
  }
  
  return text.str();
}

// ============================================================================
// Scheduled Reports
// ============================================================================

int64_t ReportManager::ScheduleReport(const ScheduledReport& scheduled) {
  ScheduledReport new_schedule = scheduled;
  new_schedule.id = next_schedule_id_++;
  scheduled_reports_.push_back(new_schedule);
  return new_schedule.id;
}

bool ReportManager::CancelScheduledReport(int64_t schedule_id) {
  auto it = std::remove_if(scheduled_reports_.begin(), scheduled_reports_.end(),
                           [schedule_id](const ScheduledReport& s) {
                             return s.id == schedule_id;
                           });
  if (it != scheduled_reports_.end()) {
    scheduled_reports_.erase(it, scheduled_reports_.end());
    return true;
  }
  return false;
}

std::vector<ScheduledReport> ReportManager::GetScheduledReports() {
  return scheduled_reports_;
}

std::optional<ScheduledReport> ReportManager::GetScheduledReport(int64_t schedule_id) {
  for (const auto& schedule : scheduled_reports_) {
    if (schedule.id == schedule_id) {
      return schedule;
    }
  }
  return std::nullopt;
}

void ReportManager::RunScheduledReportNow(int64_t schedule_id) {
  // Implementation would execute the scheduled report immediately
}

// ============================================================================
// Categories
// ============================================================================

std::vector<std::string> ReportManager::GetReportCategories() {
  return categories_;
}

void ReportManager::AddCategory(const std::string& category) {
  if (std::find(categories_.begin(), categories_.end(), category) == categories_.end()) {
    categories_.push_back(category);
  }
}

void ReportManager::RemoveCategory(const std::string& category) {
  categories_.erase(
      std::remove(categories_.begin(), categories_.end(), category),
      categories_.end());
}

// ============================================================================
// Preview
// ============================================================================

ReportData ReportManager::PreviewReport(
    const std::string& report_id,
    const std::vector<std::pair<std::string, std::string>>& parameters,
    int max_rows) {
  auto data = GenerateReport(report_id, parameters);
  if (static_cast<int>(data.rows.size()) > max_rows) {
    data.rows.resize(max_rows);
  }
  data.summary = "Preview (limited to " + std::to_string(max_rows) + " rows)";
  return data;
}

// ============================================================================
// Validation
// ============================================================================

Status ReportManager::ValidateReportParameters(
    const ReportDefinition& report,
    const std::vector<std::pair<std::string, std::string>>& parameters) {
  for (const auto& param : report.parameters) {
    if (param.required) {
      auto it = std::find_if(parameters.begin(), parameters.end(),
                             [&param](const auto& p) {
                               return p.first == param.name;
                             });
      if (it == parameters.end() || it->second.empty()) {
        return Status::Error("Required parameter missing: " + param.name);
      }
    }
  }
  return Status::Ok();
}

// ============================================================================
// Persistence
// ============================================================================

bool ReportManager::LoadReportsFromFile(const std::string& filepath) {
  // Implementation would load from JSON or XML
  return true;
}

bool ReportManager::SaveReportsToFile(const std::string& filepath) {
  // Implementation would save to JSON or XML
  return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void ReportManager::InitializeBuiltinReports() {
  // Add some default report categories
  categories_ = {
    "Database",
    "Performance",
    "Security",
    "Schema",
    "Custom"
  };
  
  // Built-in reports would be registered here
}

ReportData ReportManager::ExecuteSql(const std::string& sql) {
  ReportData data;
  // Implementation would execute SQL against database
  return data;
}

std::string ReportManager::ReplaceParameters(
    const std::string& sql_template,
    const std::vector<std::pair<std::string, std::string>>& parameters) {
  std::string result = sql_template;
  for (const auto& param : parameters) {
    std::string placeholder = "{{" + param.first + "}}";
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
      result.replace(pos, placeholder.length(), param.second);
      pos += param.second.length();
    }
  }
  return result;
}

}  // namespace scratchrobin::core
