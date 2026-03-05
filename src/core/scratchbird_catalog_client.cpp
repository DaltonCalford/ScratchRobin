/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "scratchbird_catalog_client.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace scratchrobin {
namespace core {

namespace {

// Normalize string for comparison
std::string NormalizeString(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

// Case-insensitive comparison
bool IEquals(const std::string& a, const std::string& b) {
  return NormalizeString(a) == NormalizeString(b);
}

// Quote identifier for SQL
std::string QuoteIdentifier(const std::string& name) {
  if (name.find('"') != std::string::npos) {
    std::string result = "\"";
    for (char c : name) {
      if (c == '"') {
        result += "\"\"";
      } else {
        result += c;
      }
    }
    result += "\"";
    return result;
  }
  return "\"" + name + "\"";
}

// Build fully qualified name
std::string BuildQualifiedName(const std::string& catalog,
                               const std::string& schema,
                               const std::string& name) {
  std::string result;
  if (!catalog.empty()) {
    result += QuoteIdentifier(catalog) + ".";
  }
  if (!schema.empty()) {
    result += QuoteIdentifier(schema) + ".";
  }
  result += QuoteIdentifier(name);
  return result;
}

}  // anonymous namespace

// ============================================================================
// CatalogObject Implementation
// ============================================================================

CatalogObject::CatalogObject() = default;

CatalogObject::CatalogObject(const std::string& name_val,
                             const std::string& type_val,
                             const std::string& schema_val)
    : name(name_val),
      type(type_val),
      schema(schema_val) {}

CatalogObject::~CatalogObject() = default;

bool CatalogObject::IsValid() const {
  return !name.empty() && !type.empty();
}

std::string CatalogObject::GetQualifiedName() const {
  return BuildQualifiedName("", schema, name);
}

// ============================================================================
// TableInfo Implementation
// ============================================================================

TableInfo::TableInfo() = default;

TableInfo::TableInfo(const std::string& table_name,
                     const std::string& table_schema)
    : CatalogObject(table_name, "table", table_schema) {}

TableInfo::~TableInfo() = default;

std::string TableInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE TABLE " << GetQualifiedName() << " (\n";
  
  for (size_t i = 0; i < columns.size(); ++i) {
    if (i > 0) sql << ",\n";
    const auto& col = columns[i];
    sql << "  " << QuoteIdentifier(col.name) << " " << col.data_type;
    if (!col.nullable) {
      sql << " NOT NULL";
    }
    if (!col.default_value.empty()) {
      sql << " DEFAULT " << col.default_value;
    }
  }
  
  // Add primary key constraint if defined
  if (!primary_key_columns.empty()) {
    sql << ",\n  CONSTRAINT " << QuoteIdentifier("pk_" + name) << " PRIMARY KEY (";
    for (size_t i = 0; i < primary_key_columns.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << QuoteIdentifier(primary_key_columns[i]);
    }
    sql << ")";
  }
  
  sql << "\n)";
  return sql.str();
}

std::string TableInfo::GetDropSql(bool if_exists) const {
  std::ostringstream sql;
  sql << "DROP TABLE ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << GetQualifiedName();
  return sql.str();
}

std::optional<ColumnInfo> TableInfo::GetColumn(const std::string& column_name) const {
  for (const auto& col : columns) {
    if (IEquals(col.name, column_name)) {
      return col;
    }
  }
  return std::nullopt;
}

bool TableInfo::HasColumn(const std::string& column_name) const {
  return GetColumn(column_name).has_value();
}

// ============================================================================
// ColumnInfo Implementation
// ============================================================================

ColumnInfo::ColumnInfo() = default;

ColumnInfo::ColumnInfo(const std::string& column_name,
                       const std::string& data_type_val,
                       bool nullable_val)
    : name(column_name),
      data_type(data_type_val),
      nullable(nullable_val) {}

ColumnInfo::~ColumnInfo() = default;

std::string ColumnInfo::GetQualifiedName(const std::string& table_name,
                                         const std::string& schema_name) const {
  return BuildQualifiedName("", schema_name, table_name) + "." + QuoteIdentifier(name);
}

bool ColumnInfo::IsPrimaryKey() const {
  return is_primary_key;
}

bool ColumnInfo::IsForeignKey() const {
  return is_foreign_key;
}

// ============================================================================
// IndexInfo Implementation
// ============================================================================

IndexInfo::IndexInfo() = default;

IndexInfo::IndexInfo(const std::string& index_name,
                     const std::string& table_name,
                     const std::string& schema_name)
    : CatalogObject(index_name, "index", schema_name),
      table_name(table_name) {}

IndexInfo::~IndexInfo() = default;

std::string IndexInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE ";
  if (is_unique) {
    sql << "UNIQUE ";
  }
  sql << "INDEX " << QuoteIdentifier(name) << " ON "
      << BuildQualifiedName("", schema, table_name);
  
  if (!index_type.empty()) {
    sql << " USING " << index_type;
  }
  
  sql << " (";
  for (size_t i = 0; i < columns.size(); ++i) {
    if (i > 0) sql << ", ";
    const auto& col = columns[i];
    sql << QuoteIdentifier(col.name);
    if (!col.collation.empty()) {
      sql << " COLLATE " << QuoteIdentifier(col.collation);
    }
    if (!col.sort_order.empty()) {
      sql << " " << col.sort_order;
    }
  }
  sql << ")";
  
  return sql.str();
}

std::string IndexInfo::GetDropSql(bool if_exists) const {
  std::ostringstream sql;
  sql << "DROP INDEX ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << GetQualifiedName();
  return sql.str();
}

// ============================================================================
// ConstraintInfo Implementation
// ============================================================================

ConstraintInfo::ConstraintInfo() = default;

ConstraintInfo::ConstraintInfo(const std::string& constraint_name,
                               const std::string& table_name,
                               const std::string& constraint_type_val,
                               const std::string& schema_name)
    : CatalogObject(constraint_name, "constraint", schema_name),
      table_name(table_name),
      constraint_type(constraint_type_val) {}

ConstraintInfo::~ConstraintInfo() = default;

std::string ConstraintInfo::GetAddSql() const {
  std::ostringstream sql;
  sql << "ALTER TABLE " << BuildQualifiedName("", schema, table_name)
      << " ADD CONSTRAINT " << QuoteIdentifier(name);
  
  if (constraint_type == "PRIMARY KEY") {
    sql << " PRIMARY KEY (";
    for (size_t i = 0; i < columns.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << QuoteIdentifier(columns[i]);
    }
    sql << ")";
  } else if (constraint_type == "UNIQUE") {
    sql << " UNIQUE (";
    for (size_t i = 0; i < columns.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << QuoteIdentifier(columns[i]);
    }
    sql << ")";
  } else if (constraint_type == "FOREIGN KEY") {
    sql << " FOREIGN KEY (";
    for (size_t i = 0; i < columns.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << QuoteIdentifier(columns[i]);
    }
    sql << ") REFERENCES ";
    if (!reference_table.empty()) {
      sql << BuildQualifiedName("", reference_schema, reference_table);
      if (!reference_columns.empty()) {
        sql << " (";
        for (size_t i = 0; i < reference_columns.size(); ++i) {
          if (i > 0) sql << ", ";
          sql << QuoteIdentifier(reference_columns[i]);
        }
        sql << ")";
      }
    }
  } else if (constraint_type == "CHECK") {
    sql << " CHECK (" << check_expression << ")";
  }
  
  return sql.str();
}

std::string ConstraintInfo::GetDropSql() const {
  std::ostringstream sql;
  sql << "ALTER TABLE " << BuildQualifiedName("", schema, table_name)
      << " DROP CONSTRAINT " << QuoteIdentifier(name);
  return sql.str();
}

// ============================================================================
// ViewInfo Implementation
// ============================================================================

ViewInfo::ViewInfo() = default;

ViewInfo::ViewInfo(const std::string& view_name,
                   const std::string& view_schema)
    : CatalogObject(view_name, "view", view_schema) {}

ViewInfo::~ViewInfo() = default;

std::string ViewInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE ";
  if (is_materialized) {
    sql << "MATERIALIZED ";
  }
  sql << "VIEW " << GetQualifiedName();
  
  if (!columns.empty()) {
    sql << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
      if (i > 0) sql << ", ";
      sql << QuoteIdentifier(columns[i].name);
    }
    sql << ")";
  }
  
  sql << " AS\n" << definition;
  return sql.str();
}

std::string ViewInfo::GetDropSql(bool if_exists, bool cascade) const {
  std::ostringstream sql;
  sql << "DROP ";
  if (is_materialized) {
    sql << "MATERIALIZED ";
  }
  sql << "VIEW ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << GetQualifiedName();
  if (cascade) {
    sql << " CASCADE";
  }
  return sql.str();
}

// ============================================================================
// FunctionInfo Implementation
// ============================================================================

FunctionInfo::FunctionInfo() = default;

FunctionInfo::FunctionInfo(const std::string& function_name,
                           const std::string& function_schema)
    : CatalogObject(function_name, "function", function_schema) {}

FunctionInfo::~FunctionInfo() = default;

std::string FunctionInfo::GetSignature() const {
  std::ostringstream sig;
  sig << name << "(";
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i > 0) sig << ", ";
    const auto& arg = arguments[i];
    sig << arg.name << " " << arg.data_type;
    if (!arg.default_value.empty()) {
      sig << " DEFAULT " << arg.default_value;
    }
  }
  sig << ")";
  return sig.str();
}

std::string FunctionInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE OR REPLACE FUNCTION " << GetQualifiedName() << "(";
  
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i > 0) sql << ", ";
    const auto& arg = arguments[i];
    sql << arg.name << " " << arg.data_type;
  }
  
  sql << ")\n";
  sql << "RETURNS " << return_type << "\n";
  sql << "LANGUAGE " << language << "\n";
  
  if (is_volatile) {
    sql << "VOLATILE\n";
  } else {
    sql << "STABLE\n";
  }
  
  sql << "AS $$\n";
  sql << definition;
  sql << "\n$$";
  
  return sql.str();
}

std::string FunctionInfo::GetDropSql(bool if_exists) const {
  std::ostringstream sql;
  sql << "DROP FUNCTION ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << GetQualifiedName() << "(";
  for (size_t i = 0; i < argument_types.size(); ++i) {
    if (i > 0) sql << ", ";
    sql << argument_types[i];
  }
  sql << ")";
  return sql.str();
}

// ============================================================================
// TriggerInfo Implementation
// ============================================================================

TriggerInfo::TriggerInfo() = default;

TriggerInfo::TriggerInfo(const std::string& trigger_name,
                         const std::string& table_name,
                         const std::string& schema_name)
    : CatalogObject(trigger_name, "trigger", schema_name),
      table_name(table_name) {}

TriggerInfo::~TriggerInfo() = default;

std::string TriggerInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE ";
  if (is_constraint) {
    sql << "CONSTRAINT ";
  }
  sql << "TRIGGER " << QuoteIdentifier(name) << "\n";
  
  // Timing and events
  if (before) {
    sql << "BEFORE ";
  } else if (instead_of) {
    sql << "INSTEAD OF ";
  } else {
    sql << "AFTER ";
  }
  
  bool first = true;
  if (on_insert) {
    sql << "INSERT";
    first = false;
  }
  if (on_update) {
    if (!first) sql << " OR ";
    sql << "UPDATE";
    if (!update_columns.empty()) {
      sql << " OF ";
      for (size_t i = 0; i < update_columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << QuoteIdentifier(update_columns[i]);
      }
    }
    first = false;
  }
  if (on_delete) {
    if (!first) sql << " OR ";
    sql << "DELETE";
    first = false;
  }
  if (on_truncate) {
    if (!first) sql << " OR ";
    sql << "TRUNCATE";
  }
  
  sql << "\nON " << BuildQualifiedName("", schema, table_name) << "\n";
  
  if (is_constraint && !constraint_reference.empty()) {
    sql << "FROM " << QuoteIdentifier(constraint_reference) << "\n";
  }
  
  if (deferrable) {
    sql << "DEFERRABLE ";
    if (initially_deferred) {
      sql << "INITIALLY DEFERRED\n";
    } else {
      sql << "INITIALLY IMMEDIATE\n";
    }
  }
  
  if (for_each_row) {
    sql << "FOR EACH ROW\n";
  } else {
    sql << "FOR EACH STATEMENT\n";
  }
  
  if (!when_condition.empty()) {
    sql << "WHEN (" << when_condition << ")\n";
  }
  
  sql << "EXECUTE FUNCTION " << QuoteIdentifier(function_name) << "()";
  
  return sql.str();
}

std::string TriggerInfo::GetDropSql() const {
  std::ostringstream sql;
  sql << "DROP TRIGGER " << QuoteIdentifier(name) << " ON "
      << BuildQualifiedName("", schema, table_name);
  return sql.str();
}

// ============================================================================
// SequenceInfo Implementation
// ============================================================================

SequenceInfo::SequenceInfo() = default;

SequenceInfo::SequenceInfo(const std::string& sequence_name,
                           const std::string& sequence_schema)
    : CatalogObject(sequence_name, "sequence", sequence_schema) {}

SequenceInfo::~SequenceInfo() = default;

std::string SequenceInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE SEQUENCE " << GetQualifiedName();
  
  if (increment != 1) {
    sql << " INCREMENT BY " << increment;
  }
  if (min_value != 1) {
    sql << " MINVALUE " << min_value;
  }
  if (max_value != std::numeric_limits<int64_t>::max()) {
    sql << " MAXVALUE " << max_value;
  }
  if (start_value != 1) {
    sql << " START WITH " << start_value;
  }
  if (cache_size != 1) {
    sql << " CACHE " << cache_size;
  }
  if (cycle) {
    sql << " CYCLE";
  }
  
  return sql.str();
}

std::string SequenceInfo::GetDropSql(bool if_exists) const {
  std::ostringstream sql;
  sql << "DROP SEQUENCE ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << GetQualifiedName();
  return sql.str();
}

std::string SequenceInfo::GetNextvalSql() const {
  return "nextval('" + GetQualifiedName() + "')";
}

std::string SequenceInfo::GetCurrvalSql() const {
  return "currval('" + GetQualifiedName() + "')";
}

// ============================================================================
// SchemaInfo Implementation
// ============================================================================

SchemaInfo::SchemaInfo() = default;

SchemaInfo::SchemaInfo(const std::string& schema_name)
    : CatalogObject(schema_name, "schema", "") {}

SchemaInfo::~SchemaInfo() = default;

std::string SchemaInfo::GetCreateSql() const {
  std::ostringstream sql;
  sql << "CREATE SCHEMA " << QuoteIdentifier(name);
  if (!owner.empty()) {
    sql << " AUTHORIZATION " << QuoteIdentifier(owner);
  }
  return sql.str();
}

std::string SchemaInfo::GetDropSql(bool if_exists, bool cascade) const {
  std::ostringstream sql;
  sql << "DROP SCHEMA ";
  if (if_exists) {
    sql << "IF EXISTS ";
  }
  sql << QuoteIdentifier(name);
  if (cascade) {
    sql << " CASCADE";
  }
  return sql.str();
}

// ============================================================================
// CatalogQueryBuilder Implementation
// ============================================================================

CatalogQueryBuilder::CatalogQueryBuilder() = default;

CatalogQueryBuilder::~CatalogQueryBuilder() = default;

std::string CatalogQueryBuilder::BuildTableQuery(const std::string& schema,
                                                  const std::string& table_pattern) {
  std::ostringstream sql;
  sql << "SELECT "
      << "table_schema, table_name, table_type "
      << "FROM information_schema.tables "
      << "WHERE table_schema NOT IN ('pg_catalog', 'information_schema') ";
  
  if (!schema.empty()) {
    sql << "AND table_schema = " << QuoteString(schema) << " ";
  }
  
  if (!table_pattern.empty()) {
    sql << "AND table_name LIKE " << QuoteString(table_pattern) << " ";
  }
  
  sql << "ORDER BY table_schema, table_name";
  
  return sql.str();
}

std::string CatalogQueryBuilder::BuildColumnQuery(const std::string& schema,
                                                   const std::string& table) {
  std::ostringstream sql;
  sql << "SELECT "
      << "column_name, data_type, is_nullable, column_default, "
      << "character_maximum_length, numeric_precision, numeric_scale, "
      << "ordinal_position "
      << "FROM information_schema.columns "
      << "WHERE table_schema = " << QuoteString(schema) << " "
      << "AND table_name = " << QuoteString(table) << " "
      << "ORDER BY ordinal_position";
  
  return sql.str();
}

std::string CatalogQueryBuilder::BuildIndexQuery(const std::string& schema,
                                                  const std::string& table) {
  return "SELECT indexname, indexdef FROM pg_indexes WHERE schemaname = " +
         QuoteString(schema) + " AND tablename = " + QuoteString(table);
}

std::string CatalogQueryBuilder::BuildConstraintQuery(const std::string& schema,
                                                       const std::string& table) {
  std::ostringstream sql;
  sql << "SELECT "
      << "tc.constraint_name, tc.constraint_type, "
      << "kcu.column_name, ccu.table_name AS foreign_table_name, "
      << "ccu.column_name AS foreign_column_name "
      << "FROM information_schema.table_constraints tc "
      << "LEFT JOIN information_schema.key_column_usage kcu "
      << "  ON tc.constraint_name = kcu.constraint_name "
      << "  AND tc.table_schema = kcu.table_schema "
      << "LEFT JOIN information_schema.constraint_column_usage ccu "
      << "  ON ccu.constraint_name = tc.constraint_name "
      << "  AND ccu.table_schema = tc.table_schema "
      << "WHERE tc.table_schema = " << QuoteString(schema) << " "
      << "AND tc.table_name = " << QuoteString(table);
  
  return sql.str();
}

std::string CatalogQueryBuilder::BuildViewQuery(const std::string& schema) {
  std::ostringstream sql;
  sql << "SELECT "
      << "table_schema, table_name, view_definition "
      << "FROM information_schema.views "
      << "WHERE table_schema NOT IN ('pg_catalog', 'information_schema') ";
  
  if (!schema.empty()) {
    sql << "AND table_schema = " << QuoteString(schema) << " ";
  }
  
  sql << "ORDER BY table_schema, table_name";
  
  return sql.str();
}

std::string CatalogQueryBuilder::BuildFunctionQuery(const std::string& schema) {
  std::ostringstream sql;
  sql << "SELECT "
      << "routine_schema, routine_name, routine_type, "
      << "data_type, routine_definition "
      << "FROM information_schema.routines "
      << "WHERE routine_schema NOT IN ('pg_catalog', 'information_schema') ";
  
  if (!schema.empty()) {
    sql << "AND routine_schema = " << QuoteString(schema) << " ";
  }
  
  sql << "ORDER BY routine_schema, routine_name";
  
  return sql.str();
}

std::string CatalogQueryBuilder::BuildSchemaQuery() {
  return "SELECT schema_name FROM information_schema.schemata "
         "WHERE schema_name NOT IN ('pg_catalog', 'information_schema', 'pg_toast') "
         "ORDER BY schema_name";
}

std::string CatalogQueryBuilder::QuoteString(const std::string& str) {
  std::string result = "'";
  for (char c : str) {
    if (c == '\'') {
      result += "''";
    } else {
      result += c;
    }
  }
  result += "'";
  return result;
}

// ============================================================================
// ScratchBirdCatalogClient Implementation
// ============================================================================

ScratchBirdCatalogClient::ScratchBirdCatalogClient() = default;

ScratchBirdCatalogClient::~ScratchBirdCatalogClient() = default;

void ScratchBirdCatalogClient::SetConnection(std::shared_ptr<void> connection) {
  connection_ = connection;
}

bool ScratchBirdCatalogClient::IsConnected() const {
  return connection_ != nullptr;
}

Status ScratchBirdCatalogClient::Connect(const ConnectionConfig& config) {
  config_ = config;
  return Status::Ok();
}

void ScratchBirdCatalogClient::Disconnect() {
  connection_.reset();
}

std::vector<SchemaInfo> ScratchBirdCatalogClient::GetSchemas(Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::vector<TableInfo> ScratchBirdCatalogClient::GetTables(const std::string& schema_pattern,
                                                            Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::optional<TableInfo> ScratchBirdCatalogClient::GetTable(const std::string& schema,
                                                             const std::string& table,
                                                             Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return std::nullopt;
}

std::vector<ColumnInfo> ScratchBirdCatalogClient::GetColumns(const std::string& schema,
                                                              const std::string& table,
                                                              Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::vector<IndexInfo> ScratchBirdCatalogClient::GetIndexes(const std::string& schema,
                                                             const std::string& table,
                                                             Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::vector<ConstraintInfo> ScratchBirdCatalogClient::GetConstraints(
    const std::string& schema,
    const std::string& table,
    Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::vector<ViewInfo> ScratchBirdCatalogClient::GetViews(const std::string& schema_pattern,
                                                          Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::optional<ViewInfo> ScratchBirdCatalogClient::GetView(const std::string& schema,
                                                           const std::string& view,
                                                           Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return std::nullopt;
}

std::vector<FunctionInfo> ScratchBirdCatalogClient::GetFunctions(
    const std::string& schema_pattern,
    Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::optional<FunctionInfo> ScratchBirdCatalogClient::GetFunction(
    const std::string& schema,
    const std::string& function,
    Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return std::nullopt;
}

std::vector<TriggerInfo> ScratchBirdCatalogClient::GetTriggers(const std::string& schema,
                                                                const std::string& table,
                                                                Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

std::vector<SequenceInfo> ScratchBirdCatalogClient::GetSequences(
    const std::string& schema_pattern,
    Status* status) {
  if (status) {
    *status = Status::Ok();
  }
  return {};
}

Status ScratchBirdCatalogClient::RefreshCache() {
  return Status::Ok();
}

void ScratchBirdCatalogClient::ClearCache() {
  // Clear any cached catalog data
}

}  // namespace core
}  // namespace scratchrobin
