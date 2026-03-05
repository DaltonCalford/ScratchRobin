/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/object_metadata_dialog.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

namespace scratchrobin::ui {

namespace {

constexpr int kPropertyName = 0;
constexpr int kPropertyType = 1;
constexpr int kPropertyValue = 2;
constexpr int kPropertyNotes = 3;

struct KindFieldSpec {
  std::string property_name;
  std::string property_type;
  std::string default_value;
  std::string notes;
  bool required{false};
  bool is_boolean{false};
  std::vector<std::string> enum_values;
};

struct KindProfile {
  std::string kind;
  std::string title;
  std::string summary;
  std::vector<KindFieldSpec> fields;
};

std::string ToLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

wxString DisplayLabelFromToken(const std::string& token) {
  wxString text = wxString::FromUTF8(token);
  text.Replace("_", " ");

  bool capitalize_next = true;
  for (size_t i = 0; i < text.length(); ++i) {
    const wxChar ch = text[i];
    if (ch == ' ') {
      capitalize_next = true;
      continue;
    }
    if (capitalize_next) {
      text[i] = static_cast<wxChar>(wxToupper(ch));
      capitalize_next = false;
    }
  }
  return text;
}

KindFieldSpec MakeField(const std::string& name, const std::string& type,
                        const std::string& default_value,
                        const std::string& notes, const bool required,
                        const bool is_boolean,
                        const std::vector<std::string>& enum_values = {}) {
  KindFieldSpec field;
  field.property_name = name;
  field.property_type = type;
  field.default_value = default_value;
  field.notes = notes;
  field.required = required;
  field.is_boolean = is_boolean;
  field.enum_values = enum_values;
  return field;
}

std::vector<KindFieldSpec> MakeIdentityFields(const std::string& id_name,
                                              const std::string& name_name) {
  return {
      MakeField(id_name, "UUID", "", "system identity", true, false),
      MakeField(name_name, "TEXT", "", "logical name", true, false),
      MakeField("owner_id", "UUID", "", "owning principal", false, false),
      MakeField("is_valid", "BOOLEAN", "true", "soft delete flag", false, true),
  };
}

KindProfile MakeGenericProfile(const std::string& kind) {
  KindProfile profile;
  profile.kind = kind;
  profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
  profile.summary =
      "Kind-specific field set for " + kind +
      ". Add extra fields in the table below for ScratchBird preview design.";
  profile.fields = MakeIdentityFields(kind + "_id", kind + "_name");
  return profile;
}

const std::vector<std::string>& SupportedKinds() {
  static const std::vector<std::string> kinds{
      "table",            "view",             "procedure",
      "function",         "sequence",         "trigger",
      "domain",           "package",          "job",
      "schedule",         "user",             "role",
      "group",            "schema",           "column",
      "index",            "constraint",       "composite_type",
      "tablespace",       "database",         "emulation_type",
      "emulation_server", "emulated_database", "charset",
      "collation",        "timezone",         "udr",
      "udr_engine",       "udr_module",       "exception",
      "comment",          "dependency",       "permission",
      "statistic",        "extension",        "foreign_server",
      "foreign_table",    "user_mapping",     "server_registry",
      "cluster",          "synonym",          "policy",
      "object_definition", "prepared_transaction", "audit_log",
      "migration_history", "other",
  };
  return kinds;
}

KindProfile BuildKindProfile(const std::string& kind_in) {
  const std::string kind = ToLowerCopy(kind_in);

  if (kind == "table") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Table Metadata";
    profile.summary =
        "Table editing stays in the dedicated table editor. These fields summarize identity.";
    profile.fields = {
        MakeField("table_id", "UUID", "", "table UUID", true, false),
        MakeField("table_name", "TEXT", "", "table name", true, false),
        MakeField("schema_id", "UUID", "", "owning schema", true, false),
        MakeField("table_type", "ENUM", "HEAP", "table storage class", false, false,
                  {"HEAP", "INDEX", "TEMPORARY", "MATERIALIZED_VIEW", "TOAST"}),
    };
    return profile;
  }

  if (kind == "view") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "View Metadata";
    profile.summary = "View/materialized view definition controls.";
    profile.fields = {
        MakeField("view_id", "UUID", "", "view UUID", true, false),
        MakeField("view_name", "TEXT", "", "view name", true, false),
        MakeField("schema_id", "UUID", "", "schema", true, false),
        MakeField("is_materialized", "BOOLEAN", "false", "materialized view", false,
                  true),
        MakeField("refresh_strategy", "ENUM", "ON_DEMAND", "refresh semantics", false,
                  false, {"ON_DEMAND", "ON_COMMIT", "INCREMENTAL"}),
        MakeField("columns", "TEXT", "", "comma/semicolon/newline-separated column names",
                  false, false),
        MakeField("triggers", "TEXT", "",
                  "comma/semicolon/newline-separated trigger names", false, false),
    };
    return profile;
  }

  if (kind == "procedure" || kind == "function") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = (kind == "procedure") ? "Procedure Metadata" : "Function Metadata";
    profile.summary = "Stored code object metadata and runtime signature controls.";
    const std::string id_name = (kind == "procedure") ? "procedure_id" : "function_id";
    const std::string object_name =
        (kind == "procedure") ? "procedure_name" : "function_name";
    const std::string argument_count_name =
        (kind == "procedure") ? "parameter_count" : "argument_count";
    profile.fields = {
        MakeField(id_name, "UUID", "", "stored code UUID", true, false),
        MakeField(object_name, "TEXT", "", "object name", true, false),
        MakeField("schema_id", "UUID", "", "schema", true, false),
        MakeField("procedure_type", "ENUM", (kind == "procedure") ? "PROCEDURE"
                                                                      : "FUNCTION",
                  "stored code type", true, false, {"PROCEDURE", "FUNCTION"}),
        MakeField("language", "ENUM", "PSQL", "source language", false, false,
                  {"PSQL", "SQL", "UDR", "PLPGSQL"}),
        MakeField(argument_count_name, "UINT32", "0", "declared parameters", false,
                  false),
    };
    return profile;
  }

  if (kind == "sequence") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Sequence Metadata";
    profile.summary = "Sequence state and range controls.";
    profile.fields = {
        MakeField("sequence_id", "UUID", "", "sequence UUID", true, false),
        MakeField("sequence_name", "TEXT", "", "sequence name", true, false),
        MakeField("current_value", "INT64", "0", "current value", false, false),
        MakeField("increment_by", "INT64", "1", "step", false, false),
        MakeField("min_value", "INT64", "1", "minimum", false, false),
        MakeField("max_value", "INT64", "9223372036854775807", "maximum", false,
                  false),
        MakeField("cycle", "BOOLEAN", "false", "wrap at max", false, true),
    };
    return profile;
  }

  if (kind == "trigger") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Trigger Metadata";
    profile.summary = "Trigger timing, scope, and activation metadata.";
    profile.fields = {
        MakeField("trigger_id", "UUID", "", "trigger UUID", true, false),
        MakeField("trigger_name", "TEXT", "", "trigger name", true, false),
        MakeField("table_id", "UUID", "", "target table UUID", false, false),
        MakeField("trigger_timing", "ENUM", "BEFORE", "timing", false, false,
                  {"BEFORE", "AFTER"}),
        MakeField("trigger_event", "TEXT", "INSERT", "event mask", false, false),
        MakeField("enabled", "BOOLEAN", "true", "enabled flag", false, true),
    };
    return profile;
  }

  if (kind == "domain") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Domain Metadata";
    profile.summary = "Domain definition and inheritance controls.";
    profile.fields = {
        MakeField("domain_id", "UUID", "", "domain UUID", true, false),
        MakeField("domain_name", "TEXT", "", "domain name", true, false),
        MakeField("schema_id", "UUID", "", "schema", true, false),
        MakeField("base_type_name", "TEXT", "", "base data type", false, false),
        MakeField("parent_domain_id", "UUID", "", "parent domain", false, false),
        MakeField("is_nullable", "BOOLEAN", "true", "nullable default", false, true),
    };
    return profile;
  }

  if (kind == "package") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Package Metadata";
    profile.summary = "Stored package header/body references and ownership.";
    profile.fields = {
        MakeField("package_id", "UUID", "", "package UUID", true, false),
        MakeField("package_name", "TEXT", "", "package name", true, false),
        MakeField("schema_id", "UUID", "", "schema", true, false),
        MakeField("package_header_oid", "UUID", "", "TOAST header ref", false, false),
        MakeField("package_body_oid", "UUID", "", "TOAST body ref", false, false),
        MakeField("procedures", "TEXT", "",
                  "comma/semicolon/newline-separated procedure names", false, false),
        MakeField("functions", "TEXT", "",
                  "comma/semicolon/newline-separated function names", false, false),
    };
    return profile;
  }

  if (kind == "job") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Job Metadata";
    profile.summary = "Scheduler job behavior and execution controls.";
    profile.fields = {
        MakeField("job_id", "UUID", "", "job UUID", true, false),
        MakeField("job_name", "TEXT", "", "job name", true, false),
        MakeField("job_type", "ENUM", "SQL", "execution type", false, false,
                  {"SQL", "PROCEDURE", "EXTERNAL"}),
        MakeField("state", "ENUM", "ENABLED", "scheduler state", false, false,
                  {"ENABLED", "DISABLED", "PAUSED", "RUNNING"}),
        MakeField("schedule_kind", "ENUM", "CRON", "schedule mode", false, false,
                  {"CRON", "EVERY", "AT"}),
        MakeField("next_run_time", "INT64", "0", "next run epoch micros", false,
                  false),
    };
    return profile;
  }

  if (kind == "schedule") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Schedule Metadata";
    profile.summary = "Job schedule expression controls.";
    profile.fields = {
        MakeField("schedule_id", "UUID", "", "schedule UUID", true, false),
        MakeField("schedule_kind", "ENUM", "CRON", "schedule mode", true, false,
                  {"CRON", "EVERY", "AT"}),
        MakeField("cron_expression", "TEXT", "", "cron expression", false, false),
        MakeField("interval_ms", "INT64", "0", "interval milliseconds", false,
                  false),
        MakeField("start_time", "INT64", "0", "start time", false, false),
        MakeField("end_time", "INT64", "0", "end time", false, false),
    };
    return profile;
  }

  if (kind == "user" || kind == "role" || kind == "group") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Security principal metadata and lifecycle flags.";
    const std::string id_name = kind + "_id";
    const std::string name_name = kind + "_name";
    profile.fields = {
        MakeField(id_name, "UUID", "", "principal UUID", true, false),
        MakeField(name_name, "TEXT", "", "principal name", true, false),
        MakeField("owner_id", "UUID", "", "owner", false, false),
        MakeField("is_system", "BOOLEAN", "false", "system-managed principal", false,
                  true),
        MakeField("is_locked", "BOOLEAN", "false", "lock state", false, true),
    };
    return profile;
  }

  if (kind == "schema") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Schema Metadata";
    profile.summary = "Recursive schema node definition controls.";
    profile.fields = {
        MakeField("schema_id", "UUID", "", "schema UUID", true, false),
        MakeField("schema_name", "TEXT", "", "schema name", true, false),
        MakeField("parent_schema_id", "UUID", "", "parent schema", false, false),
        MakeField("schema_type", "ENUM", "APPLICATION", "schema class", false, false,
                  {"SYSTEM", "USER_HOME", "APPLICATION"}),
    };
    return profile;
  }

  if (kind == "column") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Column Metadata";
    profile.summary = "Column definition controls.";
    profile.fields = {
        MakeField("column_id", "UUID", "", "column UUID", true, false),
        MakeField("table_id", "UUID", "", "table UUID", true, false),
        MakeField("column_name", "TEXT", "", "column name", true, false),
        MakeField("data_type_name", "TEXT", "", "base type", true, false),
        MakeField("domain_id", "UUID", "", "domain UUID", false, false),
        MakeField("is_nullable", "BOOLEAN", "true", "nullable flag", false, true),
    };
    return profile;
  }

  if (kind == "index") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Index Metadata";
    profile.summary = "Index state and access method controls.";
    profile.fields = {
        MakeField("index_id", "UUID", "", "index UUID", true, false),
        MakeField("table_id", "UUID", "", "table UUID", true, false),
        MakeField("index_name", "TEXT", "", "index name", true, false),
        MakeField("index_type", "ENUM", "BTREE", "index type", false, false,
                  {"BTREE", "HASH", "GIN", "GIST", "SPGIST"}),
        MakeField("is_unique", "BOOLEAN", "false", "unique flag", false, true),
        MakeField("state", "ENUM", "ACTIVE", "index lifecycle", false, false,
                  {"BUILDING", "ACTIVE", "RETIRED", "FAILED", "INACTIVE"}),
    };
    return profile;
  }

  if (kind == "constraint") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Constraint Metadata";
    profile.summary = "Constraint semantics and enforcement controls.";
    profile.fields = {
        MakeField("constraint_id", "UUID", "", "constraint UUID", true, false),
        MakeField("table_id", "UUID", "", "table UUID", true, false),
        MakeField("constraint_name", "TEXT", "", "constraint name", true, false),
        MakeField("constraint_type", "ENUM", "CHECK", "constraint type", false, false,
                  {"PRIMARY_KEY", "FOREIGN_KEY", "UNIQUE", "CHECK", "NOT_NULL",
                   "EXCLUSION"}),
        MakeField("is_enabled", "BOOLEAN", "true", "enforcement flag", false, true),
    };
    return profile;
  }

  if (kind == "composite_type") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Composite Type Metadata";
    profile.summary = "Composite type catalog controls.";
    profile.fields = {
        MakeField("type_id", "UUID", "", "type UUID", true, false),
        MakeField("schema_id", "UUID", "", "schema UUID", true, false),
        MakeField("type_name", "TEXT", "", "type name", true, false),
        MakeField("attribute_spec", "TEXT", "", "attribute definition", false,
                  false),
    };
    return profile;
  }

  if (kind == "tablespace" || kind == "database") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Storage/root object metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("owner_id", "UUID", "", "owner", false, false),
        MakeField("default_charset", "UUID", "", "default charset", false, false),
    };
    return profile;
  }

  if (kind == "emulation_type" || kind == "emulation_server" ||
      kind == "emulated_database") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "ScratchBird emulation catalog object metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("state", "ENUM", "ACTIVE", "state", false, false,
                  {"ACTIVE", "DISABLED"}),
        MakeField("options_json", "TEXT", "", "serialized options", false, false),
    };
    return profile;
  }

  if (kind == "charset" || kind == "collation" || kind == "timezone") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Localization and encoding metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("locale", "TEXT", "", "locale/config", false, false),
        MakeField("is_default", "BOOLEAN", "false", "default flag", false, true),
    };
    return profile;
  }

  if (kind == "udr" || kind == "udr_engine" || kind == "udr_module") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "External code execution metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("library_path", "TEXT", "", "library path", false, false),
        MakeField("entry_point", "TEXT", "", "symbol name", false, false),
        MakeField("is_enabled", "BOOLEAN", "true", "enabled", false, true),
    };
    return profile;
  }

  if (kind == "exception") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Exception Metadata";
    profile.summary = "Firebird-style exception metadata.";
    profile.fields = {
        MakeField("exception_id", "UUID", "", "exception UUID", true, false),
        MakeField("exception_name", "TEXT", "", "exception name", true, false),
        MakeField("schema_id", "UUID", "", "schema", true, false),
        MakeField("message", "TEXT", "", "message text", false, false),
    };
    return profile;
  }

  if (kind == "comment" || kind == "dependency" || kind == "permission" ||
      kind == "policy") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Cross-object governance metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField("object_id", "UUID", "", "target object UUID", true, false),
        MakeField("object_type", "TEXT", "", "target type", true, false),
        MakeField("owner_id", "UUID", "", "owner/authorizer", false, false),
        MakeField("is_enabled", "BOOLEAN", "true", "active flag", false, true),
    };
    return profile;
  }

  if (kind == "statistic") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Statistic Metadata";
    profile.summary = "Optimizer statistics metadata.";
    profile.fields = {
        MakeField("statistic_id", "UUID", "", "stat UUID", true, false),
        MakeField("table_id", "UUID", "", "table UUID", true, false),
        MakeField("column_id", "UUID", "", "column UUID", false, false),
        MakeField("num_rows", "UINT64", "0", "analyzed row count", false, false),
        MakeField("num_distinct", "UINT64", "0", "distinct count", false, false),
    };
    return profile;
  }

  if (kind == "extension" || kind == "cluster" || kind == "server_registry") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "System extension/distributed control-plane metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("state", "ENUM", "ACTIVE", "state", false, false,
                  {"ACTIVE", "DISABLED", "FAILED"}),
        MakeField("config_json", "TEXT", "", "configuration", false, false),
    };
    return profile;
  }

  if (kind == "foreign_server" || kind == "foreign_table" || kind == "user_mapping" ||
      kind == "synonym") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Federation/indirection metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField(kind + "_name", "TEXT", "", "name", true, false),
        MakeField("target_path", "TEXT", "", "target reference", false, false),
        MakeField("options_json", "TEXT", "", "connection/options", false, false),
    };
    return profile;
  }

  if (kind == "object_definition") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Object Definition Metadata";
    profile.summary = "DDL and bytecode storage metadata.";
    profile.fields = {
        MakeField("object_id", "UUID", "", "target object UUID", true, false),
        MakeField("object_type", "TEXT", "", "target object type", true, false),
        MakeField("ddl_text", "TEXT", "", "source DDL", false, false),
        MakeField("bytecode_length", "UINT32", "0", "compiled bytes", false,
                  false),
    };
    return profile;
  }

  if (kind == "prepared_transaction") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = "Prepared Transaction Metadata";
    profile.summary = "Prepared transaction durability metadata.";
    profile.fields = {
        MakeField("transaction_id", "UUID", "", "transaction UUID", true, false),
        MakeField("owner_id", "UUID", "", "owner principal", false, false),
        MakeField("prepared_time", "INT64", "0", "epoch micros", false, false),
        MakeField("state", "ENUM", "PREPARED", "state", false, false,
                  {"PREPARED", "COMMITTED", "ROLLED_BACK"}),
    };
    return profile;
  }

  if (kind == "audit_log" || kind == "migration_history") {
    KindProfile profile;
    profile.kind = kind;
    profile.title = DisplayLabelFromToken(kind).ToStdString() + " Metadata";
    profile.summary = "Operational history object metadata.";
    profile.fields = {
        MakeField(kind + "_id", "UUID", "", "identity", true, false),
        MakeField("event_type", "TEXT", "", "event classification", true, false),
        MakeField("actor_id", "UUID", "", "actor", false, false),
        MakeField("event_time", "INT64", "0", "epoch micros", false, false),
    };
    return profile;
  }

  if (kind == "other") {
    KindProfile profile = MakeGenericProfile(kind);
    profile.summary =
        "Generic object metadata. Choose a specific object kind for specialized controls.";
    return profile;
  }

  return MakeGenericProfile(kind);
}

bool RowHasAnyValue(const wxGrid* grid, const int row, const int columns) {
  for (int col = 0; col < columns; ++col) {
    if (!grid->GetCellValue(row, col).Trim().Trim(false).IsEmpty()) {
      return true;
    }
  }
  return false;
}

void DeleteFocusedOrSelectedRows(wxGrid* grid) {
  if (!grid || grid->GetNumberRows() <= 0) {
    return;
  }

  wxArrayInt rows = grid->GetSelectedRows();
  if (rows.IsEmpty()) {
    int row = grid->GetGridCursorRow();
    if (row < 0 || row >= grid->GetNumberRows()) {
      row = grid->GetNumberRows() - 1;
    }
    grid->DeleteRows(row, 1);
    return;
  }

  std::vector<int> sorted_rows;
  sorted_rows.reserve(rows.size());
  for (const int row : rows) {
    if (row >= 0 && row < grid->GetNumberRows()) {
      sorted_rows.push_back(row);
    }
  }
  std::sort(sorted_rows.begin(), sorted_rows.end(), std::greater<>());
  for (const int row : sorted_rows) {
    if (row >= 0 && row < grid->GetNumberRows()) {
      grid->DeleteRows(row, 1);
    }
  }
}

}  // namespace

ObjectMetadataDialog::ObjectMetadataDialog(
    wxWindow* parent, const Mode mode,
    const backend::PreviewObjectMetadata& seed_metadata)
    : wxDialog(parent, wxID_ANY, "SQL Object Metadata", wxDefaultPosition,
               wxSize(980, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode),
      seed_(seed_metadata) {
  BuildUi();
  BindEvents();
  PopulateFromSeed();
  ApplyMode();
}

backend::PreviewObjectMetadata ObjectMetadataDialog::GetMetadata() const {
  backend::PreviewObjectMetadata metadata;
  metadata.schema_path = schema_path_ctrl_->GetValue().ToStdString();
  metadata.object_name = object_name_ctrl_->GetValue().ToStdString();
  metadata.object_kind = SelectedObjectKind();
  metadata.description = description_ctrl_->GetValue().ToStdString();

  std::unordered_map<std::string, std::size_t> property_index;
  auto upsert_property = [&](const backend::PreviewObjectPropertyMetadata& property) {
    const std::string key = ToLowerCopy(property.property_name);
    const auto it = property_index.find(key);
    if (it == property_index.end()) {
      property_index.emplace(key, metadata.properties.size());
      metadata.properties.push_back(property);
    } else {
      metadata.properties[it->second] = property;
    }
  };

  for (const auto& field : kind_fields_) {
    backend::PreviewObjectPropertyMetadata property;
    property.property_name = field.property_name;
    property.property_type = field.property_type;
    property.property_value = ReadKindFieldValue(field);
    property.notes = field.notes;

    if (!field.required && !field.is_boolean && property.property_value.empty()) {
      continue;
    }
    if (!field.required && field.is_boolean && property.property_value == "false") {
      continue;
    }

    upsert_property(property);
  }

  const int rows = properties_grid_->GetNumberRows();
  for (int row = 0; row < rows; ++row) {
    backend::PreviewObjectPropertyMetadata property;
    property.property_name =
        properties_grid_->GetCellValue(row, kPropertyName).ToStdString();
    property.property_type =
        properties_grid_->GetCellValue(row, kPropertyType).ToStdString();
    property.property_value =
        properties_grid_->GetCellValue(row, kPropertyValue).ToStdString();
    property.notes = properties_grid_->GetCellValue(row, kPropertyNotes).ToStdString();
    if (!property.property_name.empty()) {
      upsert_property(property);
    }
  }

  return metadata;
}

void ObjectMetadataDialog::BuildUi() {
  auto* root = new wxPanel(this, wxID_ANY);
  auto* root_sizer = new wxBoxSizer(wxVERTICAL);

  auto* form_grid = new wxFlexGridSizer(2, 8, 8);
  form_grid->AddGrowableCol(1, 1);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Schema Path"), 0,
                 wxALIGN_CENTER_VERTICAL);
  schema_path_ctrl_ = new wxTextCtrl(root, wxID_ANY);
  form_grid->Add(schema_path_ctrl_, 1, wxEXPAND);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Object Name"), 0,
                 wxALIGN_CENTER_VERTICAL);
  object_name_ctrl_ = new wxTextCtrl(root, wxID_ANY);
  form_grid->Add(object_name_ctrl_, 1, wxEXPAND);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Object Kind"), 0,
                 wxALIGN_CENTER_VERTICAL);
  object_kind_choice_ =
      new wxChoice(root, static_cast<int>(ControlId::kObjectKind));
  for (const auto& kind : SupportedKinds()) {
    object_kind_choice_->Append(wxString::FromUTF8(kind));
  }
  if (object_kind_choice_->GetCount() > 0) {
    object_kind_choice_->SetSelection(0);
  }
  form_grid->Add(object_kind_choice_, 1, wxEXPAND);

  form_grid->Add(new wxStaticText(root, wxID_ANY, "Description"), 0,
                 wxALIGN_CENTER_VERTICAL);
  description_ctrl_ = new wxTextCtrl(root, wxID_ANY, "", wxDefaultPosition,
                                     wxDefaultSize, wxTE_MULTILINE);
  form_grid->Add(description_ctrl_, 1, wxEXPAND);

  kind_summary_label_ = new wxStaticText(root, wxID_ANY, "");

  auto* kind_box = new wxStaticBoxSizer(
      wxVERTICAL, root,
      "Specialized Kind Fields (persisted as property_name/type/value/notes)");
  kind_fields_panel_ = new wxPanel(kind_box->GetStaticBox(), wxID_ANY);
  kind_fields_sizer_ = new wxFlexGridSizer(2, 6, 8);
  kind_fields_sizer_->AddGrowableCol(1, 1);
  kind_fields_panel_->SetSizer(kind_fields_sizer_);
  kind_box->Add(kind_fields_panel_, 1, wxEXPAND | wxALL, 6);

  auto* properties_label = new wxStaticText(
      root, wxID_ANY,
      "Additional/override fields (rows with same name override specialized fields)");
  properties_grid_ = new wxGrid(root, wxID_ANY);
  properties_grid_->CreateGrid(0, 4);
  properties_grid_->SetColLabelValue(kPropertyName, "Field");
  properties_grid_->SetColLabelValue(kPropertyType, "Type");
  properties_grid_->SetColLabelValue(kPropertyValue, "Value");
  properties_grid_->SetColLabelValue(kPropertyNotes, "Notes");
  properties_grid_->SetColSize(kPropertyName, 220);
  properties_grid_->SetColSize(kPropertyType, 170);
  properties_grid_->SetColSize(kPropertyValue, 210);
  properties_grid_->SetColSize(kPropertyNotes, 300);

  auto* button_row = new wxBoxSizer(wxHORIZONTAL);
  add_property_btn_ = new wxButton(root, static_cast<int>(ControlId::kAddProperty),
                                   "Add Field");
  remove_property_btn_ =
      new wxButton(root, static_cast<int>(ControlId::kRemoveProperty),
                   "Remove Selected");
  button_row->Add(add_property_btn_, 0, wxRIGHT, 6);
  button_row->Add(remove_property_btn_, 0);
  button_row->AddStretchSpacer(1);

  auto* dialog_buttons = new wxStdDialogButtonSizer();
  auto* ok_button = new wxButton(root, wxID_OK);
  auto* cancel_button = new wxButton(root, wxID_CANCEL);
  dialog_buttons->AddButton(ok_button);
  dialog_buttons->AddButton(cancel_button);
  dialog_buttons->Realize();

  root_sizer->Add(form_grid, 0, wxEXPAND | wxALL, 8);
  root_sizer->Add(kind_summary_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
  root_sizer->Add(kind_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
  root_sizer->Add(properties_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
  root_sizer->Add(properties_grid_, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
  root_sizer->Add(button_row, 0, wxEXPAND | wxALL, 8);
  root_sizer->Add(dialog_buttons, 0, wxEXPAND | wxALL, 8);

  root->SetSizer(root_sizer);
  RebuildKindFieldControls(false);
}

void ObjectMetadataDialog::BindEvents() {
  Bind(wxEVT_BUTTON, &ObjectMetadataDialog::OnAddProperty, this,
       static_cast<int>(ControlId::kAddProperty));
  Bind(wxEVT_BUTTON, &ObjectMetadataDialog::OnRemoveProperty, this,
       static_cast<int>(ControlId::kRemoveProperty));
  Bind(wxEVT_CHOICE, &ObjectMetadataDialog::OnObjectKindChanged, this,
       static_cast<int>(ControlId::kObjectKind));
  Bind(wxEVT_BUTTON, &ObjectMetadataDialog::OnOk, this, wxID_OK);
}

void ObjectMetadataDialog::ApplyMode() {
  const KindProfile profile = BuildKindProfile(SelectedObjectKind());
  switch (mode_) {
    case Mode::kCreate:
      SetTitle("Create " + profile.title);
      break;
    case Mode::kEdit:
      SetTitle("Edit " + profile.title);
      break;
    case Mode::kView:
      SetTitle("View " + profile.title);
      break;
  }

  if (mode_ != Mode::kView) {
    return;
  }

  schema_path_ctrl_->SetEditable(false);
  object_name_ctrl_->SetEditable(false);
  object_kind_choice_->Disable();
  description_ctrl_->SetEditable(false);
  properties_grid_->EnableEditing(false);
  add_property_btn_->Disable();
  remove_property_btn_->Disable();

  for (const auto& field : kind_fields_) {
    if (field.control) {
      field.control->Disable();
    }
  }

  if (wxWindow* ok = FindWindow(wxID_OK)) {
    ok->SetLabel("Close");
  }
  if (wxWindow* cancel = FindWindow(wxID_CANCEL)) {
    cancel->Hide();
  }
}

void ObjectMetadataDialog::PopulateFromSeed() {
  schema_path_ctrl_->SetValue(seed_.schema_path);
  object_name_ctrl_->SetValue(seed_.object_name);
  description_ctrl_->SetValue(seed_.description);

  const std::string seed_kind = seed_.object_kind.empty() ? "other" : seed_.object_kind;
  int kind_selection = object_kind_choice_->FindString(wxString::FromUTF8(seed_kind));
  if (kind_selection == wxNOT_FOUND) {
    object_kind_choice_->Append(wxString::FromUTF8(seed_kind));
    kind_selection = static_cast<int>(object_kind_choice_->GetCount()) - 1;
  }
  if (kind_selection != wxNOT_FOUND) {
    object_kind_choice_->SetSelection(kind_selection);
  }

  RebuildKindFieldControls(false);
  PopulateKindFieldControlsFromSeed();

  std::unordered_set<std::string> specialized_names;
  for (const auto& field : kind_fields_) {
    specialized_names.insert(ToLowerCopy(field.property_name));
  }
  std::vector<backend::PreviewObjectPropertyMetadata> extra_properties;
  for (const auto& property : seed_.properties) {
    if (specialized_names.contains(ToLowerCopy(property.property_name))) {
      continue;
    }
    extra_properties.push_back(property);
  }

  if (!extra_properties.empty()) {
    properties_grid_->AppendRows(static_cast<int>(extra_properties.size()));
  }
  for (int row = 0; row < static_cast<int>(extra_properties.size()); ++row) {
    const auto& property = extra_properties[row];
    properties_grid_->SetCellValue(row, kPropertyName, property.property_name);
    properties_grid_->SetCellValue(row, kPropertyType, property.property_type);
    properties_grid_->SetCellValue(row, kPropertyValue, property.property_value);
    properties_grid_->SetCellValue(row, kPropertyNotes, property.notes);
  }
}

bool ObjectMetadataDialog::ValidateInput(wxString* error_message) const {
  if (schema_path_ctrl_->GetValue().Trim().Trim(false).IsEmpty()) {
    if (error_message) {
      *error_message = "Schema path is required.";
    }
    return false;
  }
  if (object_name_ctrl_->GetValue().Trim().Trim(false).IsEmpty()) {
    if (error_message) {
      *error_message = "Object name is required.";
    }
    return false;
  }
  if (SelectedObjectKind().empty()) {
    if (error_message) {
      *error_message = "Object kind is required.";
    }
    return false;
  }

  for (const auto& field : kind_fields_) {
    if (!field.required) {
      continue;
    }
    const std::string value = ReadKindFieldValue(field);
    if (value.empty()) {
      if (error_message) {
        *error_message = "Required field is missing: " +
                         field.property_name;
      }
      return false;
    }
  }

  const int rows = properties_grid_->GetNumberRows();
  for (int row = 0; row < rows; ++row) {
    const wxString name =
        properties_grid_->GetCellValue(row, kPropertyName).Trim().Trim(false);
    if (name.IsEmpty() &&
        RowHasAnyValue(properties_grid_, row, properties_grid_->GetNumberCols())) {
      if (error_message) {
        *error_message = "Each populated row must include a field name.";
      }
      return false;
    }
  }

  return true;
}

void ObjectMetadataDialog::RebuildKindFieldControls(const bool preserve_values) {
  std::unordered_map<std::string, std::string> prior_values;
  if (preserve_values) {
    for (const auto& field : kind_fields_) {
      prior_values[ToLowerCopy(field.property_name)] = ReadKindFieldValue(field);
    }
  }

  if (kind_fields_sizer_) {
    kind_fields_sizer_->Clear(true);
  }
  kind_fields_.clear();

  const KindProfile profile = BuildKindProfile(SelectedObjectKind());
  if (kind_summary_label_) {
    kind_summary_label_->SetLabel(wxString::FromUTF8(profile.summary));
  }

  for (const auto& spec : profile.fields) {
    auto* label = new wxStaticText(kind_fields_panel_, wxID_ANY,
                                   DisplayLabelFromToken(spec.property_name));
    kind_fields_sizer_->Add(label, 0, wxALIGN_CENTER_VERTICAL);

    KindFieldControl control;
    control.property_name = spec.property_name;
    control.property_type = spec.property_type;
    control.notes = spec.notes;
    control.required = spec.required;
    control.is_boolean = spec.is_boolean;

    wxWindow* editor = nullptr;
    if (spec.is_boolean) {
      auto* checkbox = new wxCheckBox(kind_fields_panel_, wxID_ANY, "");
      editor = checkbox;
    } else if (!spec.enum_values.empty()) {
      auto* choice = new wxChoice(kind_fields_panel_, wxID_ANY);
      for (const auto& value : spec.enum_values) {
        const wxString option = wxString::FromUTF8(value);
        choice->Append(option);
        control.enum_values.push_back(option);
      }
      editor = choice;
    } else {
      editor = new wxTextCtrl(kind_fields_panel_, wxID_ANY);
    }
    control.control = editor;

    kind_fields_sizer_->Add(editor, 1, wxEXPAND);
    kind_fields_.push_back(control);

    const std::string key = ToLowerCopy(spec.property_name);
    std::string value;
    const auto prior_it = prior_values.find(key);
    if (prior_it != prior_values.end()) {
      value = prior_it->second;
    } else {
      value = spec.default_value;
    }
    WriteKindFieldValue(kind_fields_.back(), value);
  }

  if (kind_fields_panel_) {
    kind_fields_panel_->Layout();
  }
  Layout();
}

void ObjectMetadataDialog::PopulateKindFieldControlsFromSeed() {
  std::unordered_map<std::string, backend::PreviewObjectPropertyMetadata> by_name;
  for (const auto& property : seed_.properties) {
    by_name[ToLowerCopy(property.property_name)] = property;
  }

  for (const auto& field : kind_fields_) {
    const auto it = by_name.find(ToLowerCopy(field.property_name));
    if (it == by_name.end()) {
      continue;
    }

    std::string value = it->second.property_value;
    if (value.empty()) {
      value = it->second.notes;
    }
    WriteKindFieldValue(field, value);
  }
}

std::string ObjectMetadataDialog::SelectedObjectKind() const {
  if (!object_kind_choice_) {
    return {};
  }
  const int selection = object_kind_choice_->GetSelection();
  if (selection == wxNOT_FOUND) {
    return {};
  }
  return object_kind_choice_->GetString(selection).ToStdString();
}

std::string ObjectMetadataDialog::ReadKindFieldValue(
    const KindFieldControl& field) const {
  if (!field.control) {
    return {};
  }

  if (field.is_boolean) {
    if (const auto* checkbox = dynamic_cast<const wxCheckBox*>(field.control)) {
      return checkbox->GetValue() ? "true" : "false";
    }
  }

  if (!field.enum_values.empty()) {
    if (const auto* choice = dynamic_cast<const wxChoice*>(field.control)) {
      const int selection = choice->GetSelection();
      if (selection == wxNOT_FOUND) {
        return {};
      }
      return choice->GetString(selection).ToStdString();
    }
  }

  if (const auto* text = dynamic_cast<const wxTextCtrl*>(field.control)) {
    return text->GetValue().ToStdString();
  }

  return {};
}

void ObjectMetadataDialog::WriteKindFieldValue(
    const KindFieldControl& field, const std::string& value) {
  if (!field.control) {
    return;
  }

  if (field.is_boolean) {
    if (auto* checkbox = dynamic_cast<wxCheckBox*>(field.control)) {
      const std::string normalized = ToLowerCopy(value);
      checkbox->SetValue(normalized == "1" || normalized == "true" ||
                         normalized == "yes" || normalized == "y");
      return;
    }
  }

  if (!field.enum_values.empty()) {
    if (auto* choice = dynamic_cast<wxChoice*>(field.control)) {
      int selection = choice->FindString(wxString::FromUTF8(value));
      if (selection == wxNOT_FOUND && choice->GetCount() > 0) {
        selection = 0;
      }
      if (selection != wxNOT_FOUND) {
        choice->SetSelection(selection);
      }
      return;
    }
  }

  if (auto* text = dynamic_cast<wxTextCtrl*>(field.control)) {
    text->SetValue(wxString::FromUTF8(value));
  }
}

void ObjectMetadataDialog::OnAddProperty(wxCommandEvent&) {
  const int row = properties_grid_->GetNumberRows();
  properties_grid_->AppendRows(1);
  properties_grid_->SetGridCursor(row, kPropertyName);
  properties_grid_->MakeCellVisible(row, kPropertyName);
}

void ObjectMetadataDialog::OnRemoveProperty(wxCommandEvent&) {
  DeleteFocusedOrSelectedRows(properties_grid_);
}

void ObjectMetadataDialog::OnObjectKindChanged(wxCommandEvent&) {
  RebuildKindFieldControls(true);

  if (mode_ == Mode::kCreate) {
    const std::string kind = SelectedObjectKind();
    const wxString current_name = object_name_ctrl_->GetValue().Trim().Trim(false);
    if (current_name.IsEmpty() || current_name == "new_object" ||
        current_name.StartsWith("new_")) {
      object_name_ctrl_->SetValue("new_" + wxString::FromUTF8(kind));
    }
  }

  ApplyMode();
}

void ObjectMetadataDialog::OnOk(wxCommandEvent&) {
  if (mode_ == Mode::kView) {
    EndModal(wxID_OK);
    return;
  }

  wxString error;
  if (!ValidateInput(&error)) {
    wxMessageBox(error, "Invalid Metadata", wxOK | wxICON_WARNING, this);
    return;
  }

  EndModal(wxID_OK);
}

}  // namespace scratchrobin::ui
