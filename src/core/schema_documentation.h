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
// Documentation Format
// ============================================================================

enum class DocumentationFormat {
  kHtml,
  kMarkdown,
  kPdf,
  kDocx,
  kText
};

// ============================================================================
// Column Documentation
// ============================================================================

struct ColumnDocumentation {
  std::string name;
  std::string data_type;
  bool nullable{true};
  std::string default_value;
  std::string description;
  bool is_primary_key{false};
  bool is_foreign_key{false};
  bool is_indexed{false};
  bool is_unique{false};
  std::string referenced_table;
  std::string referenced_column;
};

// ============================================================================
// Index Documentation
// ============================================================================

struct IndexDocumentation {
  std::string name;
  std::vector<std::string> columns;
  bool is_unique{false};
  bool is_primary{false};
  std::string index_type;
  std::string description;
};

// ============================================================================
// Table Documentation
// ============================================================================

struct TableDocumentation {
  std::string name;
  std::string schema;
  std::string description;
  std::vector<ColumnDocumentation> columns;
  std::vector<IndexDocumentation> indexes;
  std::vector<std::string> triggers;
  int64_t estimated_row_count{0};
  int64_t size_bytes{0};
  std::string created_date;
  std::string last_modified;
};

// ============================================================================
// Relationship Documentation
// ============================================================================

struct RelationshipDocumentation {
  std::string name;
  std::string from_table;
  std::string from_column;
  std::string to_table;
  std::string to_column;
  std::string relationship_type;  // ONE_TO_ONE, ONE_TO_MANY, MANY_TO_MANY
  std::string on_update;
  std::string on_delete;
};

// ============================================================================
// Schema Documentation
// ============================================================================

struct SchemaDoc {
  std::string database_name;
  std::string schema_name;
  std::string generated_at;
  std::string version;
  std::vector<TableDocumentation> tables;
  std::vector<RelationshipDocumentation> relationships;
  std::vector<std::string> procedures;
  std::vector<std::string> functions;
  std::vector<std::string> views;
  std::string notes;
};

// ============================================================================
// Documentation Options
// ============================================================================

struct DocumentationOptions {
  DocumentationFormat format{DocumentationFormat::kHtml};
  bool include_tables{true};
  bool include_columns{true};
  bool include_indexes{true};
  bool include_relationships{true};
  bool include_triggers{true};
  bool include_procedures{true};
  bool include_functions{true};
  bool include_views{true};
  bool include_statistics{true};
  bool include_erd{false};  // Entity Relationship Diagram
  std::string template_path;  // Custom template
  std::string title;
  std::string author;
  std::string company;
  std::string logo_path;
  std::string notes;
};

// ============================================================================
// Schema Documentation Generator
// ============================================================================

class SchemaDocumentation {
 public:
  static SchemaDocumentation& Instance();

  // Generate documentation for entire schema
  SchemaDoc GenerateSchemaDocumentation(
      const std::string& database_name,
      const std::string& schema_name = "public");

  // Generate documentation for specific tables
  SchemaDoc GenerateTableDocumentation(
      const std::vector<std::string>& table_names);

  // Generate documentation as string
  std::string GenerateDocumentation(
      const SchemaDoc& schema_doc,
      const DocumentationOptions& options);

  // Export to file
  Status ExportToFile(
      const SchemaDoc& schema_doc,
      const std::string& filepath,
      const DocumentationOptions& options);

  // HTML generation
  std::string GenerateHtml(const SchemaDoc& schema_doc,
                           const DocumentationOptions& options);

  // Markdown generation
  std::string GenerateMarkdown(const SchemaDoc& schema_doc,
                               const DocumentationOptions& options);

  // Text generation
  std::string GenerateText(const SchemaDoc& schema_doc,
                           const DocumentationOptions& options);

  // Template support
  void LoadTemplate(const std::string& template_path);
  std::string ApplyTemplate(const SchemaDoc& schema_doc,
                            const std::string& custom_template);

  // Entity Relationship Diagram generation (DOT format)
  std::string GenerateErdDot(const SchemaDoc& schema_doc);

  // Update table/column descriptions
  Status UpdateTableDescription(const std::string& table_name,
                                const std::string& description);
  Status UpdateColumnDescription(const std::string& table_name,
                                 const std::string& column_name,
                                 const std::string& description);

  // Search documentation
  std::vector<TableDocumentation> SearchDocumentation(
      const SchemaDoc& schema_doc,
      const std::string& search_term);

 private:
  SchemaDocumentation() = default;
  ~SchemaDocumentation() = default;

  SchemaDocumentation(const SchemaDocumentation&) = delete;
  SchemaDocumentation& operator=(const SchemaDocumentation&) = delete;

  std::string default_template_;

  std::string EscapeHtml(const std::string& text);
  std::string EscapeMarkdown(const std::string& text);
  std::string FormatDataType(const std::string& type, int precision, int scale);
};

}  // namespace scratchrobin::core
