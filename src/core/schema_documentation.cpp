/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/schema_documentation.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace scratchrobin::core {

// ============================================================================
// Singleton
// ============================================================================

SchemaDocumentation& SchemaDocumentation::Instance() {
  static SchemaDocumentation instance;
  return instance;
}

// ============================================================================
// Schema Documentation Generation
// ============================================================================

SchemaDoc SchemaDocumentation::GenerateSchemaDocumentation(
    const std::string& database_name,
    const std::string& schema_name) {
  SchemaDoc doc;
  doc.database_name = database_name;
  doc.schema_name = schema_name;
  
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
  doc.generated_at = ss.str();
  doc.version = "1.0";
  
  // Would query database catalog to populate tables, relationships, etc.
  
  return doc;
}

SchemaDoc SchemaDocumentation::GenerateTableDocumentation(
    const std::vector<std::string>& table_names) {
  SchemaDoc doc;
  
  for (const auto& table_name : table_names) {
    TableDocumentation table_doc;
    table_doc.name = table_name;
    // Would query database for table details
    doc.tables.push_back(table_doc);
  }
  
  return doc;
}

// ============================================================================
// Format Generation
// ============================================================================

std::string SchemaDocumentation::GenerateDocumentation(
    const SchemaDoc& schema_doc,
    const DocumentationOptions& options) {
  switch (options.format) {
    case DocumentationFormat::kHtml:
      return GenerateHtml(schema_doc, options);
    case DocumentationFormat::kMarkdown:
      return GenerateMarkdown(schema_doc, options);
    case DocumentationFormat::kText:
      return GenerateText(schema_doc, options);
    default:
      return GenerateHtml(schema_doc, options);
  }
}

Status SchemaDocumentation::ExportToFile(
    const SchemaDoc& schema_doc,
    const std::string& filepath,
    const DocumentationOptions& options) {
  std::ofstream file(filepath);
  if (!file.is_open()) {
    return Status::Error("Failed to open file: " + filepath);
  }
  
  std::string content = GenerateDocumentation(schema_doc, options);
  file << content;
  
  return Status::Ok();
}

// ============================================================================
// HTML Generation
// ============================================================================

std::string SchemaDocumentation::GenerateHtml(
    const SchemaDoc& schema_doc,
    const DocumentationOptions& options) {
  std::ostringstream html;
  
  html << "<!DOCTYPE html>\n<html>\n<head>\n";
  html << "<title>" << (options.title.empty() ? schema_doc.database_name : options.title) << " Schema Documentation</title>\n";
  html << "<style>\n";
  html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
  html << "h1 { color: #333; }\n";
  html << "table { border-collapse: collapse; width: 100%; margin: 20px 0; }\n";
  html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
  html << "th { background-color: #4CAF50; color: white; }\n";
  html << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
  html << "</style>\n</head>\n<body>\n";
  
  html << "<h1>" << schema_doc.database_name << " Schema Documentation</h1>\n";
  html << "<p>Generated: " << schema_doc.generated_at << "</p>\n";
  html << "<p>Version: " << schema_doc.version << "</p>\n";
  
  if (!options.notes.empty()) {
    html << "<h2>Notes</h2>\n<p>" << EscapeHtml(options.notes) << "</p>\n";
  }
  
  // Tables section
  if (options.include_tables && !schema_doc.tables.empty()) {
    html << "<h2>Tables</h2>\n";
    for (const auto& table : schema_doc.tables) {
      html << "<h3>" << table.name << "</h3>\n";
      if (!table.description.empty()) {
        html << "<p>" << EscapeHtml(table.description) << "</p>\n";
      }
      
      if (options.include_columns && !table.columns.empty()) {
        html << "<table>\n";
        html << "<tr><th>Column</th><th>Type</th><th>Nullable</th><th>Default</th><th>Description</th></tr>\n";
        for (const auto& col : table.columns) {
          html << "<tr>";
          html << "<td>" << col.name << (col.is_primary_key ? " (PK)" : "") << "</td>";
          html << "<td>" << col.data_type << "</td>";
          html << "<td>" << (col.nullable ? "Yes" : "No") << "</td>";
          html << "<td>" << col.default_value << "</td>";
          html << "<td>" << EscapeHtml(col.description) << "</td>";
          html << "</tr>\n";
        }
        html << "</table>\n";
      }
    }
  }
  
  html << "</body>\n</html>";
  
  return html.str();
}

// ============================================================================
// Markdown Generation
// ============================================================================

std::string SchemaDocumentation::GenerateMarkdown(
    const SchemaDoc& schema_doc,
    const DocumentationOptions& options) {
  std::ostringstream md;
  
  md << "# " << (options.title.empty() ? schema_doc.database_name : options.title) << "\n\n";
  md << "Generated: " << schema_doc.generated_at << "\n\n";
  md << "Version: " << schema_doc.version << "\n\n";
  
  if (!options.notes.empty()) {
    md << "## Notes\n\n" << options.notes << "\n\n";
  }
  
  if (options.include_tables && !schema_doc.tables.empty()) {
    md << "## Tables\n\n";
    for (const auto& table : schema_doc.tables) {
      md << "### " << table.name << "\n\n";
      if (!table.description.empty()) {
        md << table.description << "\n\n";
      }
      
      if (options.include_columns && !table.columns.empty()) {
        md << "| Column | Type | Nullable | Default | Description |\n";
        md << "|--------|------|----------|---------|-------------|\n";
        for (const auto& col : table.columns) {
          md << "| " << col.name << (col.is_primary_key ? " (PK)" : "");
          md << " | " << col.data_type;
          md << " | " << (col.nullable ? "Yes" : "No");
          md << " | " << col.default_value;
          md << " | " << col.description;
          md << " |\n";
        }
        md << "\n";
      }
    }
  }
  
  return md.str();
}

// ============================================================================
// Text Generation
// ============================================================================

std::string SchemaDocumentation::GenerateText(
    const SchemaDoc& schema_doc,
    const DocumentationOptions& options) {
  std::ostringstream text;
  
  text << "==================================================\n";
  text << "  " << (options.title.empty() ? schema_doc.database_name : options.title) << "\n";
  text << "==================================================\n\n";
  text << "Generated: " << schema_doc.generated_at << "\n";
  text << "Version: " << schema_doc.version << "\n\n";
  
  if (!options.notes.empty()) {
    text << "NOTES:\n" << options.notes << "\n\n";
  }
  
  if (options.include_tables && !schema_doc.tables.empty()) {
    text << "TABLES:\n";
    text << "--------------------------------------------------\n\n";
    for (const auto& table : schema_doc.tables) {
      text << "Table: " << table.name << "\n";
      if (!table.description.empty()) {
        text << "Description: " << table.description << "\n";
      }
      
      if (options.include_columns && !table.columns.empty()) {
        text << "\n  Columns:\n";
        for (const auto& col : table.columns) {
          text << "    - " << col.name;
          text << " (" << col.data_type << ")";
          if (col.is_primary_key) text << " [PK]";
          if (!col.nullable) text << " [NOT NULL]";
          text << "\n";
        }
      }
      text << "\n";
    }
  }
  
  return text.str();
}

// ============================================================================
// Template Support
// ============================================================================

void SchemaDocumentation::LoadTemplate(const std::string& template_path) {
  // Implementation would load custom template
}

std::string SchemaDocumentation::ApplyTemplate(
    const SchemaDoc& schema_doc,
    const std::string& custom_template) {
  // Implementation would apply template with variable substitution
  return GenerateHtml(schema_doc, DocumentationOptions{});
}

// ============================================================================
// ERD Generation
// ============================================================================

std::string SchemaDocumentation::GenerateErdDot(const SchemaDoc& schema_doc) {
  std::ostringstream dot;
  
  dot << "digraph Schema {\n";
  dot << "  rankdir=LR;\n";
  dot << "  node [shape=record];\n\n";
  
  // Define tables as nodes
  for (const auto& table : schema_doc.tables) {
    dot << "  \"" << table.name << "\" [label=\"{" << table.name << "|";
    for (size_t i = 0; i < table.columns.size(); ++i) {
      if (i > 0) dot << "\\l";
      dot << table.columns[i].name;
    }
    dot << "}\"];\n";
  }
  
  dot << "\n";
  
  // Define relationships
  for (const auto& rel : schema_doc.relationships) {
    dot << "  \"" << rel.from_table << "\" -> \"" << rel.to_table << "\";\n";
  }
  
  dot << "}\n";
  
  return dot.str();
}

// ============================================================================
// Description Updates
// ============================================================================

Status SchemaDocumentation::UpdateTableDescription(
    const std::string& table_name,
    const std::string& description) {
  // Would update database catalog with description
  return Status::Ok();
}

Status SchemaDocumentation::UpdateColumnDescription(
    const std::string& table_name,
    const std::string& column_name,
    const std::string& description) {
  // Would update database catalog with description
  return Status::Ok();
}

// ============================================================================
// Search
// ============================================================================

std::vector<TableDocumentation> SchemaDocumentation::SearchDocumentation(
    const SchemaDoc& schema_doc,
    const std::string& search_term) {
  std::vector<TableDocumentation> results;
  
  for (const auto& table : schema_doc.tables) {
    if (table.name.find(search_term) != std::string::npos ||
        table.description.find(search_term) != std::string::npos) {
      results.push_back(table);
    }
  }
  
  return results;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string SchemaDocumentation::EscapeHtml(const std::string& text) {
  std::string escaped;
  for (char c : text) {
    switch (c) {
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '&': escaped += "&amp;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&#39;"; break;
      default: escaped += c; break;
    }
  }
  return escaped;
}

std::string SchemaDocumentation::EscapeMarkdown(const std::string& text) {
  std::string escaped;
  for (char c : text) {
    if (c == '|' || c == '\\' || c == '*' || c == '_' || c == '`') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

std::string SchemaDocumentation::FormatDataType(const std::string& type,
                                                int precision,
                                                int scale) {
  if (precision > 0) {
    std::ostringstream oss;
    oss << type << "(" << precision;
    if (scale > 0) {
      oss << "," << scale;
    }
    oss << ")";
    return oss.str();
  }
  return type;
}

}  // namespace scratchrobin::core
