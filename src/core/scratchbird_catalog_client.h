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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "scratchbird_types.h"
#include "status.h"

namespace scratchrobin {
namespace core {

// ============================================================================
// Connection Configuration
// ============================================================================

struct ConnectionConfig {
  std::string host = "localhost";
  int port = 5432;
  std::string database;
  std::string username;
  std::string password;
  std::string application_name = "robin-migrate";
  int connect_timeout_ms = 30000;
  bool ssl_mode = false;
};

// ============================================================================
// CatalogObject (Base)
// ============================================================================

struct CatalogObject {
  std::string name;
  std::string type;
  std::string schema;
  std::string description;
  std::string owner;
  
  CatalogObject();
  CatalogObject(const std::string& name_val, const std::string& type_val,
                const std::string& schema_val);
  virtual ~CatalogObject();
  
  bool IsValid() const;
  std::string GetQualifiedName() const;
};

// ============================================================================
// ColumnInfo
// ============================================================================

struct ColumnInfo {
  std::string name;
  std::string data_type;
  bool nullable = true;
  std::string default_value;
  int32_t max_length = 0;
  int32_t precision = 0;
  int32_t scale = 0;
  int32_t ordinal_position = 0;
  std::string collation;
  bool is_primary_key = false;
  bool is_foreign_key = false;
  std::string foreign_key_table;
  std::string foreign_key_column;
  
  ColumnInfo();
  ColumnInfo(const std::string& column_name, const std::string& data_type_val,
             bool nullable_val);
  ~ColumnInfo();
  
  std::string GetQualifiedName(const std::string& table_name,
                               const std::string& schema_name) const;
  bool IsPrimaryKey() const;
  bool IsForeignKey() const;
};

// ============================================================================
// TableInfo
// ============================================================================

struct TableInfo : public CatalogObject {
  std::vector<ColumnInfo> columns;
  std::vector<std::string> primary_key_columns;
  int64_t row_count = -1;  // -1 means unknown
  int64_t size_bytes = -1;
  bool is_temporary = false;
  std::string tablespace;
  
  TableInfo();
  TableInfo(const std::string& table_name, const std::string& table_schema);
  ~TableInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true) const;
  std::optional<ColumnInfo> GetColumn(const std::string& column_name) const;
  bool HasColumn(const std::string& column_name) const;
};

// ============================================================================
// IndexColumnInfo
// ============================================================================

struct IndexColumnInfo {
  std::string name;
  std::string sort_order = "ASC";
  std::string collation;
  bool is_expression = false;
  std::string expression;
};

// ============================================================================
// IndexInfo
// ============================================================================

struct IndexInfo : public CatalogObject {
  std::string table_name;
  std::string table_schema;
  std::string index_type = "btree";
  bool is_unique = false;
  bool is_primary = false;
  std::vector<IndexColumnInfo> columns;
  std::string filter_condition;
  std::string tablespace;
  
  IndexInfo();
  IndexInfo(const std::string& index_name, const std::string& table_name,
            const std::string& schema_name);
  ~IndexInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true) const;
};

// ============================================================================
// ConstraintInfo
// ============================================================================

struct ConstraintInfo : public CatalogObject {
  std::string table_name;
  std::string constraint_type;  // PRIMARY KEY, UNIQUE, FOREIGN KEY, CHECK
  std::vector<std::string> columns;
  std::string check_expression;
  std::string reference_schema;
  std::string reference_table;
  std::vector<std::string> reference_columns;
  std::string update_rule;
  std::string delete_rule;
  bool is_deferrable = false;
  bool initially_deferred = false;
  
  ConstraintInfo();
  ConstraintInfo(const std::string& constraint_name, const std::string& table_name,
                 const std::string& constraint_type_val,
                 const std::string& schema_name);
  ~ConstraintInfo();
  
  std::string GetAddSql() const;
  std::string GetDropSql() const;
};

// ============================================================================
// ViewInfo
// ============================================================================

struct ViewInfo : public CatalogObject {
  std::string definition;
  std::vector<ColumnInfo> columns;
  std::vector<std::string> dependencies;
  bool is_materialized = false;
  bool is_updatable = false;
  
  ViewInfo();
  ViewInfo(const std::string& view_name, const std::string& view_schema);
  ~ViewInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true, bool cascade = false) const;
};

// ============================================================================
// FunctionArgument
// ============================================================================

struct FunctionArgument {
  std::string name;
  std::string data_type;
  std::string mode = "IN";  // IN, OUT, INOUT, VARIADIC
  std::string default_value;
};

// ============================================================================
// FunctionInfo
// ============================================================================

struct FunctionInfo : public CatalogObject {
  std::string return_type;
  std::vector<FunctionArgument> arguments;
  std::vector<std::string> argument_types;
  std::string language;
  std::string definition;
  bool is_aggregate = false;
  bool is_window = false;
  bool is_volatile = true;
  std::string source_code;
  
  FunctionInfo();
  FunctionInfo(const std::string& function_name, const std::string& function_schema);
  ~FunctionInfo();
  
  std::string GetSignature() const;
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true) const;
};

// ============================================================================
// TriggerInfo
// ============================================================================

struct TriggerInfo : public CatalogObject {
  std::string table_name;
  std::string table_schema;
  std::string function_name;
  std::string function_schema;
  std::vector<std::string> events;
  bool before = false;
  bool instead_of = false;
  bool on_insert = false;
  bool on_update = false;
  bool on_delete = false;
  bool on_truncate = false;
  std::vector<std::string> update_columns;
  bool for_each_row = true;
  std::string when_condition;
  bool is_enabled = true;
  bool is_constraint = false;
  std::string constraint_reference;
  bool deferrable = false;
  bool initially_deferred = false;
  
  TriggerInfo();
  TriggerInfo(const std::string& trigger_name, const std::string& table_name,
              const std::string& schema_name);
  ~TriggerInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql() const;
};

// ============================================================================
// SequenceInfo
// ============================================================================

struct SequenceInfo : public CatalogObject {
  int64_t start_value = 1;
  int64_t min_value = 1;
  int64_t max_value = 9223372036854775807LL;
  int64_t increment = 1;
  int64_t cache_size = 1;
  bool cycle = false;
  int64_t current_value = 0;
  std::string data_type = "bigint";
  
  SequenceInfo();
  SequenceInfo(const std::string& sequence_name, const std::string& sequence_schema);
  ~SequenceInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true) const;
  std::string GetNextvalSql() const;
  std::string GetCurrvalSql() const;
};

// ============================================================================
// SchemaInfo
// ============================================================================

struct SchemaInfo : public CatalogObject {
  std::vector<std::string> table_names;
  std::vector<std::string> view_names;
  std::vector<std::string> function_names;
  int64_t size_bytes = -1;
  
  SchemaInfo();
  SchemaInfo(const std::string& schema_name);
  ~SchemaInfo();
  
  std::string GetCreateSql() const;
  std::string GetDropSql(bool if_exists = true, bool cascade = false) const;
};

// ============================================================================
// CatalogQueryBuilder
// ============================================================================

class CatalogQueryBuilder {
 public:
  CatalogQueryBuilder();
  ~CatalogQueryBuilder();
  
  std::string BuildTableQuery(const std::string& schema = "",
                              const std::string& table_pattern = "");
  std::string BuildColumnQuery(const std::string& schema,
                               const std::string& table);
  std::string BuildIndexQuery(const std::string& schema,
                              const std::string& table);
  std::string BuildConstraintQuery(const std::string& schema,
                                   const std::string& table);
  std::string BuildViewQuery(const std::string& schema = "");
  std::string BuildFunctionQuery(const std::string& schema = "");
  std::string BuildSchemaQuery();
  
 private:
  static std::string QuoteString(const std::string& str);
};

// ============================================================================
// ScratchBirdCatalogClient
// ============================================================================

class ScratchBirdCatalogClient {
 public:
  ScratchBirdCatalogClient();
  ~ScratchBirdCatalogClient();
  
  // Connection management
  void SetConnection(std::shared_ptr<void> connection);
  bool IsConnected() const;
  Status Connect(const ConnectionConfig& config);
  void Disconnect();
  
  // Schema queries
  std::vector<SchemaInfo> GetSchemas(Status* status = nullptr);
  
  // Table queries
  std::vector<TableInfo> GetTables(const std::string& schema_pattern = "",
                                   Status* status = nullptr);
  std::optional<TableInfo> GetTable(const std::string& schema,
                                    const std::string& table,
                                    Status* status = nullptr);
  std::vector<ColumnInfo> GetColumns(const std::string& schema,
                                     const std::string& table,
                                     Status* status = nullptr);
  
  // Index queries
  std::vector<IndexInfo> GetIndexes(const std::string& schema,
                                    const std::string& table,
                                    Status* status = nullptr);
  
  // Constraint queries
  std::vector<ConstraintInfo> GetConstraints(const std::string& schema,
                                             const std::string& table,
                                             Status* status = nullptr);
  
  // View queries
  std::vector<ViewInfo> GetViews(const std::string& schema_pattern = "",
                                 Status* status = nullptr);
  std::optional<ViewInfo> GetView(const std::string& schema,
                                  const std::string& view,
                                  Status* status = nullptr);
  
  // Function queries
  std::vector<FunctionInfo> GetFunctions(const std::string& schema_pattern = "",
                                         Status* status = nullptr);
  std::optional<FunctionInfo> GetFunction(const std::string& schema,
                                          const std::string& function,
                                          Status* status = nullptr);
  
  // Trigger queries
  std::vector<TriggerInfo> GetTriggers(const std::string& schema,
                                       const std::string& table,
                                       Status* status = nullptr);
  
  // Sequence queries
  std::vector<SequenceInfo> GetSequences(const std::string& schema_pattern = "",
                                         Status* status = nullptr);
  
  // Cache management
  Status RefreshCache();
  void ClearCache();
  
 private:
  ConnectionConfig config_;
  std::shared_ptr<void> connection_;
};

}  // namespace core
}  // namespace scratchrobin
