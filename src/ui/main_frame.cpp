/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/main_frame.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

#include <wx/artprov.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>

#include "backend/preview_object_metadata_store.h"
#include "backend/preview_metadata_store.h"
#include "backend/scratchbird_catalog_preview.h"
#include "core/settings.h"
#include "res/art_provider.h"
#include "res/tree_image_list.h"
#include "ui/object_metadata_dialog.h"
#include "ui/settings_dialog.h"
#include "ui/sql_editor_frame.h"
#include "ui/table_metadata_dialog.h"

namespace scratchrobin::ui {

namespace {
enum class SqlObjectKind {
  kTable,
  kView,
  kProcedure,
  kFunction,
  kGenerator,
  kTrigger,
  kDomain,
  kPackage,
  kJob,
  kSchedule,
  kUser,
  kRole,
  kGroup,
  kSchema,
  kColumn,
  kIndex,
  kConstraint,
  kCompositeType,
  kTablespace,
  kDatabase,
  kEmulationType,
  kEmulationServer,
  kEmulatedDatabase,
  kCharset,
  kCollation,
  kTimezone,
  kUdr,
  kUdrEngine,
  kUdrModule,
  kException,
  kComment,
  kDependency,
  kPermission,
  kStatistic,
  kExtension,
  kForeignServer,
  kForeignTable,
  kUserMapping,
  kServerRegistry,
  kCluster,
  kSynonym,
  kPolicy,
  kObjectDefinition,
  kPreparedTransaction,
  kAuditLog,
  kMigrationHistory,
  kOther,
};

struct SqlObjectNode {
  wxString name;
  SqlObjectKind kind{SqlObjectKind::kOther};
  std::string schema_path;
  std::string object_name;
};

struct SchemaNode {
  wxString name;
  std::string full_path;
  std::vector<SchemaNode> children;
  std::vector<SqlObjectNode> objects;
};

struct DatabaseNode {
  wxString name;
  std::vector<SchemaNode> root_schemas;
};

struct ServerNode {
  wxString name;
  std::vector<DatabaseNode> databases;
};

struct TreeIcons {
  int server{-1};
  int database{-1};
  int folder{-1};
  int schema{-1};
  int table{-1};
  int view{-1};
  int procedure{-1};
  int function{-1};
  int generator{-1};
  int trigger{-1};
  int domain{-1};
  int package{-1};
  int job{-1};
  int schedule{-1};
  int user{-1};
  int role{-1};
  int group{-1};
  int branch_columns{-1};
  int branch_indexes{-1};
  int branch_triggers{-1};
  int column{-1};
  int column_key{-1};
  int index{-1};
  int table_trigger{-1};
};

enum class TreeNodeType {
  kUnknown = 0,
  kServer = 1,
  kDatabase = 2,
  kSchema = 3,
  kObject = 4,
  kObjectBranch = 5,
  kObjectLeaf = 6,
};

struct TreeNodeData final : public wxTreeItemData {
  TreeNodeType type{TreeNodeType::kUnknown};
  std::string schema_path;
  std::string object_name;
  SqlObjectKind object_kind{SqlObjectKind::kOther};
};

struct PreviewCatalogBuildResult {
  std::vector<ServerNode> servers;
  std::vector<backend::PreviewTableMetadata> table_metadata;
  std::vector<backend::PreviewObjectMetadata> object_metadata;
};

wxString ToWxString(const std::string& value) {
  return wxString::FromUTF8(value.c_str());
}

std::string ToLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

wxString ToTitleCase(const std::string& value) {
  wxString text = ToWxString(value);
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

wxString FriendlySchemaLabel(const std::string& segment,
                             const std::string& full_path) {
  const std::string lower = ToLowerCopy(segment);
  if (full_path == "root.sys") {
    return "System";
  }
  if (full_path == "root.users") {
    return "User Schemas";
  }
  if (full_path == "root.remote") {
    return "Remote";
  }
  if (full_path == "root.local") {
    return "Local";
  }
  if (full_path == "root.nosql") {
    return "NoSQL";
  }
  if (lower == "information") {
    return "Metadata";
  }
  if (lower == "monitor") {
    return "Monitoring";
  }
  if (lower == "config") {
    return "Configuration";
  }
  if (lower == "system") {
    return "System Objects";
  }
  return ToTitleCase(segment);
}

std::string TrimCopy(const std::string& value) {
  std::size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  std::size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

std::string MakeTableMetadataKey(const std::string& schema_path,
                                 const std::string& table_name) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(table_name);
}

std::string MakeObjectMetadataKey(const std::string& schema_path,
                                  const std::string& object_name,
                                  const std::string& object_kind) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(object_name) + "|" +
         ToLowerCopy(object_kind);
}

std::string ObjectKindName(const SqlObjectKind kind) {
  switch (kind) {
    case SqlObjectKind::kTable:
      return "table";
    case SqlObjectKind::kView:
      return "view";
    case SqlObjectKind::kProcedure:
      return "procedure";
    case SqlObjectKind::kFunction:
      return "function";
    case SqlObjectKind::kGenerator:
      return "sequence";
    case SqlObjectKind::kTrigger:
      return "trigger";
    case SqlObjectKind::kDomain:
      return "domain";
    case SqlObjectKind::kPackage:
      return "package";
    case SqlObjectKind::kJob:
      return "job";
    case SqlObjectKind::kSchedule:
      return "schedule";
    case SqlObjectKind::kUser:
      return "user";
    case SqlObjectKind::kRole:
      return "role";
    case SqlObjectKind::kGroup:
      return "group";
    case SqlObjectKind::kSchema:
      return "schema";
    case SqlObjectKind::kColumn:
      return "column";
    case SqlObjectKind::kIndex:
      return "index";
    case SqlObjectKind::kConstraint:
      return "constraint";
    case SqlObjectKind::kCompositeType:
      return "composite_type";
    case SqlObjectKind::kTablespace:
      return "tablespace";
    case SqlObjectKind::kDatabase:
      return "database";
    case SqlObjectKind::kEmulationType:
      return "emulation_type";
    case SqlObjectKind::kEmulationServer:
      return "emulation_server";
    case SqlObjectKind::kEmulatedDatabase:
      return "emulated_database";
    case SqlObjectKind::kCharset:
      return "charset";
    case SqlObjectKind::kCollation:
      return "collation";
    case SqlObjectKind::kTimezone:
      return "timezone";
    case SqlObjectKind::kUdr:
      return "udr";
    case SqlObjectKind::kUdrEngine:
      return "udr_engine";
    case SqlObjectKind::kUdrModule:
      return "udr_module";
    case SqlObjectKind::kException:
      return "exception";
    case SqlObjectKind::kComment:
      return "comment";
    case SqlObjectKind::kDependency:
      return "dependency";
    case SqlObjectKind::kPermission:
      return "permission";
    case SqlObjectKind::kStatistic:
      return "statistic";
    case SqlObjectKind::kExtension:
      return "extension";
    case SqlObjectKind::kForeignServer:
      return "foreign_server";
    case SqlObjectKind::kForeignTable:
      return "foreign_table";
    case SqlObjectKind::kUserMapping:
      return "user_mapping";
    case SqlObjectKind::kServerRegistry:
      return "server_registry";
    case SqlObjectKind::kCluster:
      return "cluster";
    case SqlObjectKind::kSynonym:
      return "synonym";
    case SqlObjectKind::kPolicy:
      return "policy";
    case SqlObjectKind::kObjectDefinition:
      return "object_definition";
    case SqlObjectKind::kPreparedTransaction:
      return "prepared_transaction";
    case SqlObjectKind::kAuditLog:
      return "audit_log";
    case SqlObjectKind::kMigrationHistory:
      return "migration_history";
    case SqlObjectKind::kOther:
    default:
      return "other";
  }
}

bool IsTableKind(const SqlObjectKind kind) {
  return kind == SqlObjectKind::kTable;
}

const backend::PreviewTableMetadata* FindTableMetadata(
    const std::unordered_map<std::string, backend::PreviewTableMetadata>& metadata_by_key,
    const std::string& schema_path, const std::string& table_name) {
  const auto it = metadata_by_key.find(MakeTableMetadataKey(schema_path, table_name));
  if (it == metadata_by_key.end()) {
    return nullptr;
  }
  return &it->second;
}

const backend::PreviewObjectMetadata* FindObjectMetadata(
    const std::unordered_map<std::string, backend::PreviewObjectMetadata>& metadata_by_key,
    const std::string& schema_path, const std::string& object_name,
    const std::string& object_kind) {
  const auto it =
      metadata_by_key.find(MakeObjectMetadataKey(schema_path, object_name, object_kind));
  if (it == metadata_by_key.end()) {
    return nullptr;
  }
  return &it->second;
}

wxString BuildColumnLabel(const backend::PreviewColumnMetadata& column) {
  wxString label = ToWxString(column.column_name);
  wxString key_flags;
  if (column.is_primary_key) {
    key_flags += "PK";
  }
  if (column.is_foreign_key) {
    if (!key_flags.IsEmpty()) {
      key_flags += ",";
    }
    key_flags += "FK";
  }

  if (!key_flags.IsEmpty()) {
    label += " [" + key_flags + "]";
  }

  const wxString domain_or_type = !column.domain_name.empty()
                                      ? ToWxString(column.domain_name)
                                      : ToWxString(column.data_type);
  if (!domain_or_type.IsEmpty()) {
    label += " : " + domain_or_type;
  }

  return label;
}

wxString BuildIndexLabel(const backend::PreviewIndexMetadata& index) {
  wxString label = ToWxString(index.index_name);
  if (index.unique) {
    label += " [UNIQUE]";
  }
  if (!index.method.empty()) {
    label += " (" + ToWxString(index.method) + ")";
  }
  return label;
}

wxString BuildTriggerLabel(const backend::PreviewTriggerMetadata& trigger) {
  wxString label = ToWxString(trigger.trigger_name);
  wxString timing_event;
  if (!trigger.timing.empty()) {
    timing_event += ToWxString(trigger.timing);
  }
  if (!trigger.event.empty()) {
    if (!timing_event.IsEmpty()) {
      timing_event += " ";
    }
    timing_event += ToWxString(trigger.event);
  }
  if (!timing_event.IsEmpty()) {
    label += " [" + timing_event + "]";
  }
  return label;
}

std::vector<wxString> ParseNameList(const std::string& raw) {
  std::vector<wxString> out;
  std::string token;
  auto flush = [&]() {
    const std::string trimmed = TrimCopy(token);
    if (!trimmed.empty()) {
      out.push_back(ToWxString(trimmed));
    }
    token.clear();
  };

  for (const char c : raw) {
    if (c == ',' || c == ';' || c == '\n' || c == '\r') {
      flush();
      continue;
    }
    token.push_back(c);
  }
  flush();
  return out;
}

void AppendUniqueNames(std::vector<wxString>* names,
                       const std::vector<wxString>& candidates) {
  if (!names) {
    return;
  }
  std::unordered_set<std::string> seen;
  seen.reserve(names->size() + candidates.size());
  for (const auto& name : *names) {
    seen.insert(ToLowerCopy(name.ToStdString()));
  }
  for (const auto& candidate : candidates) {
    wxString trimmed = candidate;
    trimmed = trimmed.Trim().Trim(false);
    if (trimmed.IsEmpty()) {
      continue;
    }
    const std::string key = ToLowerCopy(trimmed.ToStdString());
    if (seen.contains(key)) {
      continue;
    }
    seen.insert(key);
    names->push_back(trimmed);
  }
}

std::vector<wxString> CollectPropertyNameList(
    const backend::PreviewObjectMetadata* metadata,
    const std::vector<std::string>& property_names) {
  if (!metadata) {
    return {};
  }

  std::unordered_set<std::string> expected;
  expected.reserve(property_names.size());
  for (const auto& name : property_names) {
    expected.insert(ToLowerCopy(name));
  }

  std::vector<wxString> names;
  for (const auto& property : metadata->properties) {
    const std::string property_name = ToLowerCopy(property.property_name);
    if (!expected.contains(property_name)) {
      continue;
    }

    const std::string raw_value = !property.property_value.empty()
                                      ? property.property_value
                                      : property.notes;
    AppendUniqueNames(&names, ParseNameList(raw_value));
  }

  return names;
}

std::vector<wxString> CollectPrefixedPropertyNames(
    const backend::PreviewObjectMetadata* metadata, const std::string& prefix) {
  if (!metadata) {
    return {};
  }

  const std::string normalized_prefix = ToLowerCopy(prefix) + ":";
  std::vector<wxString> names;
  for (const auto& property : metadata->properties) {
    const std::string property_name = ToLowerCopy(property.property_name);
    if (!property_name.starts_with(normalized_prefix)) {
      continue;
    }

    const std::string suffix = TrimCopy(property.property_name.substr(prefix.size() + 1));
    if (!suffix.empty()) {
      AppendUniqueNames(&names, {ToWxString(suffix)});
      continue;
    }

    const std::string raw_value = !property.property_value.empty()
                                      ? property.property_value
                                      : property.notes;
    AppendUniqueNames(&names, ParseNameList(raw_value));
  }

  return names;
}

std::vector<wxString> CollectSchemaObjectNamesByKind(const SchemaNode& schema,
                                                     const SqlObjectKind kind,
                                                     const std::string& exclude_name) {
  std::vector<wxString> names;
  for (const auto& object : schema.objects) {
    if (object.kind != kind) {
      continue;
    }
    if (ToLowerCopy(object.object_name) == ToLowerCopy(exclude_name)) {
      continue;
    }
    names.push_back(object.name);
  }
  std::sort(names.begin(), names.end(),
            [](const wxString& lhs, const wxString& rhs) {
              return lhs.Lower() < rhs.Lower();
            });
  return names;
}

wxTreeItemId AppendSubtypeBranch(wxTreeCtrlBase* tree, const wxTreeItemId& parent,
                                 const wxString& label, const int branch_icon,
                                 const int leaf_icon, const SqlObjectNode& owner,
                                 const std::vector<wxString>& leaf_names) {
  auto* branch_data = new TreeNodeData();
  branch_data->type = TreeNodeType::kObjectBranch;
  branch_data->schema_path = owner.schema_path;
  branch_data->object_name = owner.object_name;
  branch_data->object_kind = owner.kind;
  const wxTreeItemId branch_item =
      tree->AppendItem(parent, label, branch_icon, branch_icon, branch_data);

  for (const auto& leaf_name : leaf_names) {
    wxString trimmed = leaf_name;
    trimmed = trimmed.Trim().Trim(false);
    if (trimmed.IsEmpty()) {
      continue;
    }

    auto* leaf_data = new TreeNodeData();
    leaf_data->type = TreeNodeType::kObjectLeaf;
    leaf_data->schema_path = owner.schema_path;
    leaf_data->object_name = owner.object_name;
    leaf_data->object_kind = owner.kind;
    tree->AppendItem(branch_item, trimmed, leaf_icon, leaf_icon, leaf_data);
  }

  return branch_item;
}

wxString TreePath(wxTreeCtrlBase* tree, wxTreeItemId item) {
  wxArrayString parts;
  while (item.IsOk()) {
    const wxString label = tree->GetItemText(item);
    if (!label.IsEmpty() && label != "root") {
      parts.Add(label);
    }
    item = tree->GetItemParent(item);
  }

  wxString out;
  for (int i = static_cast<int>(parts.size()) - 1; i >= 0; --i) {
    if (!out.IsEmpty()) {
      out += " > ";
    }
    out += parts[i];
  }
  return out;
}

bool ContainsCaseInsensitive(const wxString& haystack, const wxString& needle) {
  const wxString h = haystack.Lower();
  const wxString n = needle.Lower();
  return !n.IsEmpty() && h.Find(n) != wxNOT_FOUND;
}

bool FindFirstMatch(wxTreeCtrlBase* tree, wxTreeItemId parent, const wxString& query,
                    wxTreeItemId* out) {
  wxTreeItemIdValue cookie;
  wxTreeItemId child = tree->GetFirstChild(parent, cookie);
  while (child.IsOk()) {
    if (ContainsCaseInsensitive(tree->GetItemText(child), query)) {
      *out = child;
      return true;
    }
    if (FindFirstMatch(tree, child, query, out)) {
      return true;
    }
    child = tree->GetNextChild(parent, cookie);
  }
  return false;
}

std::vector<std::string> SplitPath(const std::string& path) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start < path.size()) {
    const std::size_t dot = path.find('.', start);
    const std::size_t end = (dot == std::string::npos) ? path.size() : dot;
    if (end > start) {
      parts.push_back(path.substr(start, end - start));
    }
    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }
  return parts;
}

SchemaNode& EnsureSchemaComponent(std::vector<SchemaNode>* siblings,
                                  const std::string& component,
                                  const std::string& full_path) {
  const wxString name = ToWxString(component);
  for (auto& node : *siblings) {
    if (node.name == name) {
      return node;
    }
  }

  siblings->push_back(SchemaNode{name, full_path, {}, {}});
  return siblings->back();
}

void EnsureSchemaPath(DatabaseNode* database, const std::string& full_path) {
  if (!database) {
    return;
  }

  std::vector<SchemaNode>* level = &database->root_schemas;
  std::string current_path;
  for (const auto& component : SplitPath(full_path)) {
    if (!current_path.empty()) {
      current_path += ".";
    }
    current_path += component;
    SchemaNode& node = EnsureSchemaComponent(level, component, current_path);
    level = &node.children;
  }
}

SchemaNode* FindSchemaPath(DatabaseNode* database, const std::string& full_path) {
  if (!database) {
    return nullptr;
  }

  std::vector<SchemaNode>* level = &database->root_schemas;
  SchemaNode* current = nullptr;
  for (const auto& component : SplitPath(full_path)) {
    const wxString name = ToWxString(component);
    auto it = std::find_if(level->begin(), level->end(),
                           [&](const SchemaNode& node) { return node.name == name; });
    if (it == level->end()) {
      return nullptr;
    }
    current = &(*it);
    level = &current->children;
  }
  return current;
}

SqlObjectKind ToUiObjectKind(const backend::PreviewObjectKind kind) {
  switch (kind) {
    case backend::PreviewObjectKind::kTable:
      return SqlObjectKind::kTable;
    case backend::PreviewObjectKind::kView:
      return SqlObjectKind::kView;
    case backend::PreviewObjectKind::kProcedure:
      return SqlObjectKind::kProcedure;
    case backend::PreviewObjectKind::kFunction:
      return SqlObjectKind::kFunction;
    case backend::PreviewObjectKind::kSequence:
      return SqlObjectKind::kGenerator;
    case backend::PreviewObjectKind::kTrigger:
      return SqlObjectKind::kTrigger;
    case backend::PreviewObjectKind::kDomain:
      return SqlObjectKind::kDomain;
    case backend::PreviewObjectKind::kPackage:
      return SqlObjectKind::kPackage;
    case backend::PreviewObjectKind::kJob:
      return SqlObjectKind::kJob;
    case backend::PreviewObjectKind::kSchedule:
      return SqlObjectKind::kSchedule;
    case backend::PreviewObjectKind::kUser:
      return SqlObjectKind::kUser;
    case backend::PreviewObjectKind::kRole:
      return SqlObjectKind::kRole;
    case backend::PreviewObjectKind::kGroup:
      return SqlObjectKind::kGroup;
    case backend::PreviewObjectKind::kSchema:
      return SqlObjectKind::kSchema;
    case backend::PreviewObjectKind::kColumn:
      return SqlObjectKind::kColumn;
    case backend::PreviewObjectKind::kIndex:
      return SqlObjectKind::kIndex;
    case backend::PreviewObjectKind::kConstraint:
      return SqlObjectKind::kConstraint;
    case backend::PreviewObjectKind::kCompositeType:
      return SqlObjectKind::kCompositeType;
    case backend::PreviewObjectKind::kTablespace:
      return SqlObjectKind::kTablespace;
    case backend::PreviewObjectKind::kDatabase:
      return SqlObjectKind::kDatabase;
    case backend::PreviewObjectKind::kEmulationType:
      return SqlObjectKind::kEmulationType;
    case backend::PreviewObjectKind::kEmulationServer:
      return SqlObjectKind::kEmulationServer;
    case backend::PreviewObjectKind::kEmulatedDatabase:
      return SqlObjectKind::kEmulatedDatabase;
    case backend::PreviewObjectKind::kCharset:
      return SqlObjectKind::kCharset;
    case backend::PreviewObjectKind::kCollation:
      return SqlObjectKind::kCollation;
    case backend::PreviewObjectKind::kTimezone:
      return SqlObjectKind::kTimezone;
    case backend::PreviewObjectKind::kUdr:
      return SqlObjectKind::kUdr;
    case backend::PreviewObjectKind::kUdrEngine:
      return SqlObjectKind::kUdrEngine;
    case backend::PreviewObjectKind::kUdrModule:
      return SqlObjectKind::kUdrModule;
    case backend::PreviewObjectKind::kException:
      return SqlObjectKind::kException;
    case backend::PreviewObjectKind::kComment:
      return SqlObjectKind::kComment;
    case backend::PreviewObjectKind::kDependency:
      return SqlObjectKind::kDependency;
    case backend::PreviewObjectKind::kPermission:
      return SqlObjectKind::kPermission;
    case backend::PreviewObjectKind::kStatistic:
      return SqlObjectKind::kStatistic;
    case backend::PreviewObjectKind::kExtension:
      return SqlObjectKind::kExtension;
    case backend::PreviewObjectKind::kForeignServer:
      return SqlObjectKind::kForeignServer;
    case backend::PreviewObjectKind::kForeignTable:
      return SqlObjectKind::kForeignTable;
    case backend::PreviewObjectKind::kUserMapping:
      return SqlObjectKind::kUserMapping;
    case backend::PreviewObjectKind::kServerRegistry:
      return SqlObjectKind::kServerRegistry;
    case backend::PreviewObjectKind::kCluster:
      return SqlObjectKind::kCluster;
    case backend::PreviewObjectKind::kSynonym:
      return SqlObjectKind::kSynonym;
    case backend::PreviewObjectKind::kPolicy:
      return SqlObjectKind::kPolicy;
    case backend::PreviewObjectKind::kObjectDefinition:
      return SqlObjectKind::kObjectDefinition;
    case backend::PreviewObjectKind::kPreparedTransaction:
      return SqlObjectKind::kPreparedTransaction;
    case backend::PreviewObjectKind::kAuditLog:
      return SqlObjectKind::kAuditLog;
    case backend::PreviewObjectKind::kMigrationHistory:
      return SqlObjectKind::kMigrationHistory;
    case backend::PreviewObjectKind::kOther:
    default:
      return SqlObjectKind::kOther;
  }
}

int IconForObjectKind(const SqlObjectKind kind, const TreeIcons& icons) {
  switch (kind) {
    case SqlObjectKind::kTable:
      return icons.table;
    case SqlObjectKind::kView:
      return icons.view;
    case SqlObjectKind::kProcedure:
      return icons.procedure;
    case SqlObjectKind::kFunction:
      return icons.function;
    case SqlObjectKind::kGenerator:
      return icons.generator;
    case SqlObjectKind::kTrigger:
      return icons.trigger;
    case SqlObjectKind::kDomain:
      return icons.domain;
    case SqlObjectKind::kPackage:
      return icons.package;
    case SqlObjectKind::kJob:
      return icons.job;
    case SqlObjectKind::kSchedule:
      return icons.schedule;
    case SqlObjectKind::kUser:
      return icons.user;
    case SqlObjectKind::kRole:
      return icons.role;
    case SqlObjectKind::kGroup:
      return icons.group;
    case SqlObjectKind::kSchema:
      return icons.schema;
    case SqlObjectKind::kColumn:
      return icons.column;
    case SqlObjectKind::kIndex:
      return icons.index;
    case SqlObjectKind::kConstraint:
      return icons.index;
    case SqlObjectKind::kCompositeType:
      return icons.domain;
    case SqlObjectKind::kTablespace:
      return icons.database;
    case SqlObjectKind::kDatabase:
      return icons.database;
    case SqlObjectKind::kEmulationType:
      return icons.package;
    case SqlObjectKind::kEmulationServer:
      return icons.server;
    case SqlObjectKind::kEmulatedDatabase:
      return icons.database;
    case SqlObjectKind::kCharset:
      return icons.domain;
    case SqlObjectKind::kCollation:
      return icons.domain;
    case SqlObjectKind::kTimezone:
      return icons.schedule;
    case SqlObjectKind::kUdr:
      return icons.function;
    case SqlObjectKind::kUdrEngine:
      return icons.package;
    case SqlObjectKind::kUdrModule:
      return icons.package;
    case SqlObjectKind::kException:
      return icons.trigger;
    case SqlObjectKind::kComment:
      return icons.column;
    case SqlObjectKind::kDependency:
      return icons.index;
    case SqlObjectKind::kPermission:
      return icons.role;
    case SqlObjectKind::kStatistic:
      return icons.index;
    case SqlObjectKind::kExtension:
      return icons.package;
    case SqlObjectKind::kForeignServer:
      return icons.server;
    case SqlObjectKind::kForeignTable:
      return icons.table;
    case SqlObjectKind::kUserMapping:
      return icons.user;
    case SqlObjectKind::kServerRegistry:
      return icons.server;
    case SqlObjectKind::kCluster:
      return icons.server;
    case SqlObjectKind::kSynonym:
      return icons.view;
    case SqlObjectKind::kPolicy:
      return icons.role;
    case SqlObjectKind::kObjectDefinition:
      return icons.column;
    case SqlObjectKind::kPreparedTransaction:
      return icons.job;
    case SqlObjectKind::kAuditLog:
      return icons.table_trigger;
    case SqlObjectKind::kMigrationHistory:
      return icons.job;
    case SqlObjectKind::kOther:
    default:
      return icons.folder;
  }
}

wxTreeItemId AppendSchemaNode(wxTreeCtrlBase* tree, wxTreeItemId parent,
                              const SchemaNode& schema, const TreeIcons& icons,
                              const std::unordered_map<std::string,
                                                       backend::PreviewTableMetadata>&
                                  table_metadata_by_key,
                              const std::unordered_map<std::string,
                                                       backend::PreviewObjectMetadata>&
                                  object_metadata_by_key,
                              const bool friendly_labels) {
  auto* schema_data = new TreeNodeData();
  schema_data->type = TreeNodeType::kSchema;
  schema_data->schema_path = schema.full_path;
  const wxString schema_label = friendly_labels
                                    ? FriendlySchemaLabel(schema.name.ToStdString(),
                                                         schema.full_path)
                                    : schema.name;
  const wxTreeItemId schema_item =
      tree->AppendItem(parent, schema_label, icons.schema, icons.schema, schema_data);

  std::vector<const SchemaNode*> sorted_children;
  sorted_children.reserve(schema.children.size());
  for (const auto& child : schema.children) {
    sorted_children.push_back(&child);
  }
  std::sort(sorted_children.begin(), sorted_children.end(),
            [](const SchemaNode* lhs, const SchemaNode* rhs) {
              return lhs->name.Lower() < rhs->name.Lower();
            });

  for (const SchemaNode* child : sorted_children) {
    AppendSchemaNode(tree, schema_item, *child, icons, table_metadata_by_key,
                     object_metadata_by_key, friendly_labels);
  }

  std::vector<const SqlObjectNode*> sorted_objects;
  sorted_objects.reserve(schema.objects.size());
  for (const auto& object : schema.objects) {
    sorted_objects.push_back(&object);
  }
  std::sort(sorted_objects.begin(), sorted_objects.end(),
            [](const SqlObjectNode* lhs, const SqlObjectNode* rhs) {
              return lhs->name.Lower() < rhs->name.Lower();
            });

  for (const SqlObjectNode* object : sorted_objects) {
    const int object_icon = IconForObjectKind(object->kind, icons);
    auto* object_data = new TreeNodeData();
    object_data->type = TreeNodeType::kObject;
    object_data->schema_path = object->schema_path;
    object_data->object_name = object->object_name;
    object_data->object_kind = object->kind;
    const wxTreeItemId object_item =
        tree->AppendItem(schema_item, object->name, object_icon, object_icon, object_data);

    if (object->kind == SqlObjectKind::kTable) {
      const backend::PreviewTableMetadata* metadata =
          FindTableMetadata(table_metadata_by_key, object->schema_path, object->object_name);

      const wxTreeItemId triggers_branch =
          AppendSubtypeBranch(tree, object_item, "Triggers", icons.branch_triggers,
                              icons.table_trigger, *object, {});
      const wxTreeItemId columns_branch =
          AppendSubtypeBranch(tree, object_item, "Columns", icons.branch_columns,
                              icons.column, *object, {});
      const wxTreeItemId indexes_branch =
          AppendSubtypeBranch(tree, object_item, "Indexes", icons.branch_indexes,
                              icons.index, *object, {});

      if (!metadata) {
        continue;
      }

      for (const auto& trigger : metadata->triggers) {
        if (trigger.trigger_name.empty()) {
          continue;
        }
        auto* trigger_data = new TreeNodeData();
        trigger_data->type = TreeNodeType::kObjectLeaf;
        trigger_data->schema_path = object->schema_path;
        trigger_data->object_name = object->object_name;
        trigger_data->object_kind = SqlObjectKind::kTable;
        tree->AppendItem(triggers_branch, BuildTriggerLabel(trigger), icons.table_trigger,
                         icons.table_trigger, trigger_data);
      }

      for (const auto& column : metadata->columns) {
        if (column.column_name.empty()) {
          continue;
        }
        auto* column_data = new TreeNodeData();
        column_data->type = TreeNodeType::kObjectLeaf;
        column_data->schema_path = object->schema_path;
        column_data->object_name = object->object_name;
        column_data->object_kind = SqlObjectKind::kTable;
        const int icon = (column.is_primary_key || column.is_foreign_key) ? icons.column_key
                                                                           : icons.column;
        tree->AppendItem(columns_branch, BuildColumnLabel(column), icon, icon, column_data);
      }

      for (const auto& index : metadata->indexes) {
        if (index.index_name.empty()) {
          continue;
        }
        auto* index_data = new TreeNodeData();
        index_data->type = TreeNodeType::kObjectLeaf;
        index_data->schema_path = object->schema_path;
        index_data->object_name = object->object_name;
        index_data->object_kind = SqlObjectKind::kTable;
        tree->AppendItem(indexes_branch, BuildIndexLabel(index), icons.index, icons.index,
                         index_data);
      }
      continue;
    }

    const backend::PreviewObjectMetadata* object_metadata = FindObjectMetadata(
        object_metadata_by_key, object->schema_path, object->object_name,
        ObjectKindName(object->kind));

    if (object->kind == SqlObjectKind::kView) {
      std::vector<wxString> columns;
      std::vector<wxString> triggers;
      AppendUniqueNames(&columns, CollectPropertyNameList(
                                      object_metadata,
                                      {"columns", "column_list", "view_columns"}));
      AppendUniqueNames(&triggers, CollectPropertyNameList(
                                       object_metadata,
                                       {"triggers", "trigger_list", "view_triggers"}));
      AppendUniqueNames(&columns,
                        CollectPrefixedPropertyNames(object_metadata, "column"));
      AppendUniqueNames(&triggers,
                        CollectPrefixedPropertyNames(object_metadata, "trigger"));

      if (columns.empty()) {
        AppendUniqueNames(&columns, CollectSchemaObjectNamesByKind(
                                        schema, SqlObjectKind::kColumn,
                                        object->object_name));
      }
      if (triggers.empty()) {
        AppendUniqueNames(&triggers, CollectSchemaObjectNamesByKind(
                                         schema, SqlObjectKind::kTrigger,
                                         object->object_name));
      }

      AppendSubtypeBranch(tree, object_item, "Columns", icons.branch_columns,
                          icons.column, *object, columns);
      AppendSubtypeBranch(tree, object_item, "Triggers", icons.branch_triggers,
                          icons.table_trigger, *object, triggers);
      continue;
    }

    if (object->kind == SqlObjectKind::kPackage) {
      std::vector<wxString> procedures;
      std::vector<wxString> functions;
      AppendUniqueNames(&procedures, CollectPropertyNameList(
                                         object_metadata,
                                         {"procedures", "procedure_members",
                                          "package_procedures"}));
      AppendUniqueNames(&functions, CollectPropertyNameList(
                                        object_metadata,
                                        {"functions", "function_members",
                                         "package_functions"}));
      AppendUniqueNames(&procedures,
                        CollectPrefixedPropertyNames(object_metadata, "procedure"));
      AppendUniqueNames(&functions,
                        CollectPrefixedPropertyNames(object_metadata, "function"));

      if (procedures.empty()) {
        AppendUniqueNames(&procedures, CollectSchemaObjectNamesByKind(
                                           schema, SqlObjectKind::kProcedure,
                                           object->object_name));
      }
      if (functions.empty()) {
        AppendUniqueNames(&functions, CollectSchemaObjectNamesByKind(
                                          schema, SqlObjectKind::kFunction,
                                          object->object_name));
      }

      AppendSubtypeBranch(tree, object_item, "Procedures", icons.branch_indexes,
                          icons.procedure, *object, procedures);
      AppendSubtypeBranch(tree, object_item, "Functions", icons.branch_indexes,
                          icons.function, *object, functions);
      continue;
    }
  }

  return schema_item;
}

PreviewCatalogBuildResult BuildPreviewCatalog(
    const backend::ScratchbirdRuntimeConfig& runtime_config) {
  const backend::ScratchbirdCatalogPreviewSnapshot snapshot =
      backend::LoadScratchbirdCatalogPreview(runtime_config);

  PreviewCatalogBuildResult result;
  result.table_metadata = snapshot.table_metadata;
  result.object_metadata = snapshot.object_metadata;

  DatabaseNode database;
  const wxString database_name = ToWxString(snapshot.database_name);
  database.name =
      "ScratchBird (" + (database_name.IsEmpty() ? wxString("default") : database_name) + ")";

  for (const auto& schema_path : snapshot.schema_paths) {
    EnsureSchemaPath(&database, schema_path);
  }

  for (const auto& object : snapshot.objects) {
    SchemaNode* schema = FindSchemaPath(&database, object.schema_path);
    if (!schema) {
      EnsureSchemaPath(&database, object.schema_path);
      schema = FindSchemaPath(&database, object.schema_path);
    }
    if (!schema) {
      continue;
    }

    schema->objects.push_back(SqlObjectNode{
        ToWxString(object.name),
        ToUiObjectKind(object.kind),
        object.schema_path,
        object.name,
    });
  }

  if (database.root_schemas.empty()) {
    EnsureSchemaPath(&database, "root");
    EnsureSchemaPath(&database, "root.users");
    EnsureSchemaPath(&database, "root.users.public");
  }

  ServerNode server;
  const wxString server_name = ToWxString(snapshot.server_name);
  server.name = server_name.IsEmpty() ? wxString("localhost") : server_name;
  server.databases.push_back(std::move(database));

  result.servers.push_back(std::move(server));
  return result;
}
}  // namespace

MainFrame::MainFrame(backend::SessionClient* session_client)
    : wxFrame(nullptr, wxID_ANY, "FlameRobin Database Admin (ScratchBird)",
              wxDefaultPosition, wxSize(420, 660)),
      session_client_(session_client) {
  runtime_config_.mode = backend::TransportMode::kPreview;
  runtime_config_.host = "127.0.0.1";
  runtime_config_.port = 4044;
  runtime_config_.database = "default";

  BuildMainMenu();
  BuildMainLayout();
  PopulateTree();
  BindEvents();

  CreateStatusBar(2);
  UpdateStatusbarText();

  SetIcon(wxArtProvider::GetIcon(wxART_HARDDISK, wxART_FRAME_ICON));
}

void MainFrame::BuildMainMenu() {
  menu_bar_ = new wxMenuBar();

  auto* database_menu = new wxMenu();
  database_menu->Append(static_cast<int>(ControlId::kMenuOpenSqlEditor),
                        "Open new Volatile &SQL Editor...\tCtrl+E");
  database_menu->Append(static_cast<int>(ControlId::kMenuRegisterDatabase),
                        "R&egister existing database...");
  database_menu->Append(static_cast<int>(ControlId::kMenuCreateDatabase),
                        "Create &new database...");
  database_menu->AppendSeparator();
  mode_preview_item_ =
      database_menu->AppendCheckItem(static_cast<int>(ControlId::kMenuModePreview),
                                     "Use &Preview Mode (No Connection)");
  mode_managed_item_ =
      database_menu->AppendCheckItem(static_cast<int>(ControlId::kMenuModeManaged),
                                     "Use &Managed ScratchBird Mode");
  mode_listener_only_item_ =
      database_menu->AppendCheckItem(static_cast<int>(ControlId::kMenuModeListenerOnly),
                                     "Use &Listener-Only ScratchBird Mode");
  mode_ipc_only_item_ =
      database_menu->AppendCheckItem(static_cast<int>(ControlId::kMenuModeIpcOnly),
                                     "Use &IPC-Only ScratchBird Mode");
  mode_embedded_item_ =
      database_menu->AppendCheckItem(static_cast<int>(ControlId::kMenuModeEmbedded),
                                     "Use &Embedded ScratchBird Mode");
  database_menu->AppendSeparator();
  database_menu->Append(wxID_EXIT, "&Quit");
  menu_bar_->Append(database_menu, "&Database");

  view_menu_ = new wxMenu();
  toggle_status_item_ = view_menu_->AppendCheckItem(
      static_cast<int>(ControlId::kMenuToggleStatusBar), "&Status bar");
  toggle_search_item_ = view_menu_->AppendCheckItem(
      static_cast<int>(ControlId::kMenuToggleSearchBar), "S&earch bar");
  toggle_debug_mode_item_ = view_menu_->AppendCheckItem(
      static_cast<int>(ControlId::kMenuToggleDebugMode),
      "&Debug mode (show internal tree)");
  menu_bar_->Append(view_menu_, "&View");

  auto* server_menu = new wxMenu();
  server_menu->Append(static_cast<int>(ControlId::kMenuRegisterServer),
                      "&Register server...");
  server_menu->Append(static_cast<int>(ControlId::kMenuServerProperties),
                      "Server registration &info");
  menu_bar_->Append(server_menu, "&Server");

  auto* object_menu = new wxMenu();
  auto* new_submenu = new wxMenu();
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewTable), "&Table...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewView), "&View...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewProcedure), "&Procedure...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewFunction), "&Function...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewSequence), "&Sequence...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewTrigger), "&Trigger...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewDomain), "&Domain...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewPackage), "&Package...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewJob), "&Job...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewSchedule), "Sc&hedule...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewUser), "&User...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewRole), "&Role...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewGroup), "&Group...");
  new_submenu->Append(static_cast<int>(ControlId::kMenuNewObject), "&Other Object...");
  object_menu->AppendSubMenu(new_submenu, "&New");
  object_menu->Append(static_cast<int>(ControlId::kMenuObjectProperties),
                      "Object &Properties...");
  menu_bar_->Append(object_menu, "&Object");

  auto* tools_menu = new wxMenu();
  tools_menu->Append(static_cast<int>(ControlId::kMenuSettings),
                     "&Settings...\tCtrl+Comma");
  menu_bar_->Append(tools_menu, "&Tools");

  auto* help_menu = new wxMenu();
  help_menu->Append(wxID_ABOUT, "&About");
  menu_bar_->Append(help_menu, "&Help");

  SetMenuBar(menu_bar_);
  
  // Load and apply settings
  core::Settings::get().load();
  ApplySettings();

  if (toggle_status_item_) {
    toggle_status_item_->Check(true);
  }
  if (toggle_search_item_) {
    toggle_search_item_->Check(true);
  }
  if (toggle_debug_mode_item_) {
    toggle_debug_mode_item_->Check(debug_mode_enabled_);
  }
  if (mode_preview_item_ && mode_managed_item_ && mode_listener_only_item_ &&
      mode_ipc_only_item_ && mode_embedded_item_) {
    mode_preview_item_->Check(true);
    mode_managed_item_->Check(false);
    mode_listener_only_item_->Check(false);
    mode_ipc_only_item_->Check(false);
    mode_embedded_item_->Check(false);
  }
}

void MainFrame::BuildMainLayout() {
  main_panel_ = new wxPanel(this, wxID_ANY);

  // Use wxGenericTreeCtrl for SVG/scalable icon support
  // wxGenericTreeCtrl supports wxBitmapBundle natively for high-DPI displays
  const long tree_style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT |
                          wxBORDER_THEME;
  object_tree_ = new wxGenericTreeCtrl(main_panel_, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, tree_style);
  BuildTreeImageList();

  search_panel_ = new wxPanel(main_panel_, wxID_ANY, wxDefaultPosition,
                              wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_THEME);

  wxArrayString choices;
  const long combo_style = wxCB_DROPDOWN | wxTE_PROCESS_ENTER;
  search_box_ = new wxComboBox(search_panel_, static_cast<int>(ControlId::kSearchBox),
                               wxEmptyString, wxDefaultPosition, wxDefaultSize,
                               choices, combo_style);

  const wxSize button_size(16, 16);
  button_prev_ = new wxBitmapButton(
      search_panel_, static_cast<int>(ControlId::kSearchPrev),
      wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR, button_size));
  button_next_ = new wxBitmapButton(
      search_panel_, static_cast<int>(ControlId::kSearchNext),
      wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR, button_size));
  button_advanced_ = new wxBitmapButton(
      search_panel_, static_cast<int>(ControlId::kSearchAdvanced),
      wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR, button_size));

  button_prev_->SetToolTip("Previous match");
  button_next_->SetToolTip("Next match");
  button_advanced_->SetToolTip("Advanced metadata search");

  auto* search_combo_sizer = new wxBoxSizer(wxVERTICAL);
  search_combo_sizer->AddStretchSpacer(1);
  search_combo_sizer->Add(search_box_, 0, wxEXPAND);
  search_combo_sizer->AddStretchSpacer(1);

  auto* search_row = new wxBoxSizer(wxHORIZONTAL);
  search_row->Add(search_combo_sizer, 1, wxEXPAND);
  search_row->Add(button_prev_, 0, wxLEFT, 2);
  search_row->Add(button_next_, 0, wxLEFT, 2);
  search_row->Add(button_advanced_, 0, wxLEFT, 2);
  search_panel_->SetSizer(search_row);

  search_panel_sizer_ = new wxBoxSizer(wxVERTICAL);
  search_panel_sizer_->Add(object_tree_, 1, wxEXPAND);
  search_panel_sizer_->Add(search_panel_, 0, wxEXPAND);
  main_panel_->SetSizer(search_panel_sizer_);

  auto* root_sizer = new wxBoxSizer(wxVERTICAL);
  root_sizer->Add(main_panel_, 1, wxEXPAND);
  SetSizer(root_sizer);
  Layout();
}

void MainFrame::BuildTreeImageList() {
  if (!object_tree_) {
    return;
  }

  // Initialize the art provider (registers custom SVG icons)
  scratchrobin::res::InitArtProvider();

  // Get reference to the scalable image list
  scratchrobin::res::TreeImageList& til = scratchrobin::res::TreeImageList::get();
  
  // Set up our icon indices using SVG icons
  icon_server_ = til.getServerIndex();
  icon_database_ = til.getDatabaseConnectedIndex();
  icon_folder_ = til.getFolderIndex();
  icon_schema_ = til.getFolderIndex();
  icon_table_ = til.getTableIndex();
  icon_view_ = til.getViewIndex();
  icon_procedure_ = til.getProcedureIndex();
  icon_function_ = til.getFunctionIndex();
  icon_generator_ = til.getGeneratorIndex();
  icon_trigger_ = til.getTriggerIndex();
  icon_domain_ = til.getDomainIndex();
  icon_package_ = til.getPackageIndex();
  icon_job_ = til.getUDFIndex();
  icon_schedule_ = til.getGeneratorIndex();
  icon_user_ = til.getUserIndex();
  icon_role_ = til.getRoleIndex();
  icon_group_ = til.getRolesIndex();
  icon_branch_columns_ = til.getFolderIndex();
  icon_branch_indexes_ = til.getIndicesIndex();
  icon_branch_triggers_ = til.getTriggersIndex();
  icon_column_ = til.getColumnIndex();
  icon_column_key_ = til.getColumnPKIndex();
  icon_index_ = til.getIndexIndex();
  icon_table_trigger_ = til.getTriggerIndex();

  // wxGenericTreeCtrl uses SetImages() with bitmap bundles for scalability
  const auto& bundles = til.getAllBundles();
  if (!bundles.empty()) {
    object_tree_->SetImages(bundles);
  }
}

void MainFrame::PopulateTree() {
  object_tree_->DeleteAllItems();
  tree_root_ = object_tree_->AddRoot("root", icon_folder_, icon_folder_);
  tree_database_ = wxTreeItemId();
  table_metadata_by_key_.clear();
  object_metadata_by_key_.clear();

  const TreeIcons icons{
      icon_server_,          icon_database_,       icon_folder_,
      icon_schema_,          icon_table_,          icon_view_,
      icon_procedure_,       icon_function_,       icon_generator_,
      icon_trigger_,         icon_domain_,         icon_package_,
      icon_job_,             icon_schedule_,       icon_user_,
      icon_role_,            icon_group_,          icon_branch_columns_,
      icon_branch_indexes_,  icon_branch_triggers_, icon_column_,
      icon_column_key_,      icon_index_,          icon_table_trigger_,
  };

  const PreviewCatalogBuildResult preview_catalog = BuildPreviewCatalog(runtime_config_);
  for (const auto& table_metadata : preview_catalog.table_metadata) {
    table_metadata_by_key_[MakeTableMetadataKey(table_metadata.schema_path,
                                                table_metadata.table_name)] =
        table_metadata;
  }
  for (const auto& object_metadata : preview_catalog.object_metadata) {
    object_metadata_by_key_[MakeObjectMetadataKey(object_metadata.schema_path,
                                                  object_metadata.object_name,
                                                  object_metadata.object_kind)] =
        object_metadata;
  }

  const std::vector<ServerNode>& servers = preview_catalog.servers;

  wxTreeItemId first_server;
  for (const auto& server : servers) {
    auto* server_data = new TreeNodeData();
    server_data->type = TreeNodeType::kServer;
    const wxTreeItemId server_item = object_tree_->AppendItem(
        tree_root_, server.name, icons.server, icons.server, server_data);
    if (!first_server.IsOk()) {
      first_server = server_item;
    }

    for (const auto& database : server.databases) {
      auto* database_data = new TreeNodeData();
      database_data->type = TreeNodeType::kDatabase;
      const wxTreeItemId database_item = object_tree_->AppendItem(
          server_item, database.name, icons.database, icons.database, database_data);
      if (!tree_database_.IsOk()) {
        tree_database_ = database_item;
      }

      auto sort_schema_nodes = [](std::vector<const SchemaNode*>* schemas) {
        std::sort(schemas->begin(), schemas->end(),
                  [](const SchemaNode* lhs, const SchemaNode* rhs) {
                    return lhs->name.Lower() < rhs->name.Lower();
                  });
      };

      auto append_schema_list =
          [&](const wxTreeItemId& parent_item,
              const std::vector<const SchemaNode*>& schemas,
              const bool friendly_labels) {
            for (const SchemaNode* schema : schemas) {
              const wxTreeItemId schema_item =
                  AppendSchemaNode(object_tree_, parent_item, *schema, icons,
                                   table_metadata_by_key_, object_metadata_by_key_,
                                   friendly_labels);
              object_tree_->Expand(schema_item);
            }
          };

      const auto root_it = std::find_if(
          database.root_schemas.begin(), database.root_schemas.end(),
          [](const SchemaNode& schema) { return schema.full_path == "root"; });

      std::vector<const SchemaNode*> working_schemas;
      if (root_it != database.root_schemas.end() && !root_it->children.empty()) {
        working_schemas.reserve(root_it->children.size());
        for (const auto& schema : root_it->children) {
          working_schemas.push_back(&schema);
        }
      } else {
        working_schemas.reserve(database.root_schemas.size());
        for (const auto& schema : database.root_schemas) {
          working_schemas.push_back(&schema);
        }
      }
      sort_schema_nodes(&working_schemas);

      const wxTreeItemId working_tree_item =
          object_tree_->AppendItem(database_item, "Working Layout", icons.folder,
                                   icons.folder);
      append_schema_list(working_tree_item, working_schemas, true);
      object_tree_->Expand(working_tree_item);

      if (debug_mode_enabled_) {
        std::vector<const SchemaNode*> debug_schemas;
        debug_schemas.reserve(database.root_schemas.size());
        for (const auto& schema : database.root_schemas) {
          debug_schemas.push_back(&schema);
        }
        sort_schema_nodes(&debug_schemas);

        const wxTreeItemId debug_tree_item =
            object_tree_->AppendItem(database_item, "Debug: Internal Schema Tree",
                                     icons.folder, icons.folder);
        append_schema_list(debug_tree_item, debug_schemas, false);
        object_tree_->Expand(debug_tree_item);
      }

      object_tree_->Expand(database_item);
    }

    object_tree_->Expand(server_item);
  }

  if (first_server.IsOk()) {
    object_tree_->SelectItem(first_server);
  }
}

void MainFrame::BindEvents() {
  // Register for settings changes
  core::Settings::get().addChangeCallback([this]() { OnSettingsChanged(); });
  
  Bind(wxEVT_MENU, &MainFrame::OnMenuOpenSqlEditor, this,
       static_cast<int>(ControlId::kMenuOpenSqlEditor));
  Bind(wxEVT_MENU, &MainFrame::OnMenuToggleStatusBar, this,
       static_cast<int>(ControlId::kMenuToggleStatusBar));
  Bind(wxEVT_MENU, &MainFrame::OnMenuToggleSearchBar, this,
       static_cast<int>(ControlId::kMenuToggleSearchBar));
  Bind(wxEVT_MENU, &MainFrame::OnMenuToggleDebugMode, this,
       static_cast<int>(ControlId::kMenuToggleDebugMode));
  Bind(wxEVT_MENU, &MainFrame::OnMenuModePreview, this,
       static_cast<int>(ControlId::kMenuModePreview));
  Bind(wxEVT_MENU, &MainFrame::OnMenuModeManaged, this,
       static_cast<int>(ControlId::kMenuModeManaged));
  Bind(wxEVT_MENU, &MainFrame::OnMenuModeListenerOnly, this,
       static_cast<int>(ControlId::kMenuModeListenerOnly));
  Bind(wxEVT_MENU, &MainFrame::OnMenuModeIpcOnly, this,
       static_cast<int>(ControlId::kMenuModeIpcOnly));
  Bind(wxEVT_MENU, &MainFrame::OnMenuModeEmbedded, this,
       static_cast<int>(ControlId::kMenuModeEmbedded));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewTable, this,
       static_cast<int>(ControlId::kMenuNewTable));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewView, this,
       static_cast<int>(ControlId::kMenuNewView));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewProcedure, this,
       static_cast<int>(ControlId::kMenuNewProcedure));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewFunction, this,
       static_cast<int>(ControlId::kMenuNewFunction));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewSequence, this,
       static_cast<int>(ControlId::kMenuNewSequence));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewTrigger, this,
       static_cast<int>(ControlId::kMenuNewTrigger));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewDomain, this,
       static_cast<int>(ControlId::kMenuNewDomain));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewPackage, this,
       static_cast<int>(ControlId::kMenuNewPackage));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewJob, this,
       static_cast<int>(ControlId::kMenuNewJob));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewSchedule, this,
       static_cast<int>(ControlId::kMenuNewSchedule));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewUser, this,
       static_cast<int>(ControlId::kMenuNewUser));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewRole, this,
       static_cast<int>(ControlId::kMenuNewRole));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewGroup, this,
       static_cast<int>(ControlId::kMenuNewGroup));
  Bind(wxEVT_MENU, &MainFrame::OnMenuNewObject, this,
       static_cast<int>(ControlId::kMenuNewObject));
  Bind(wxEVT_MENU, &MainFrame::OnMenuObjectProperties, this,
       static_cast<int>(ControlId::kMenuObjectProperties));

  Bind(wxEVT_MENU, &MainFrame::OnMenuPlaceholder, this,
       static_cast<int>(ControlId::kMenuRegisterDatabase));
  Bind(wxEVT_MENU, &MainFrame::OnMenuPlaceholder, this,
       static_cast<int>(ControlId::kMenuCreateDatabase));
  Bind(wxEVT_MENU, &MainFrame::OnMenuPlaceholder, this,
       static_cast<int>(ControlId::kMenuRegisterServer));
  Bind(wxEVT_MENU, &MainFrame::OnMenuPlaceholder, this,
       static_cast<int>(ControlId::kMenuServerProperties));

  Bind(wxEVT_MENU, &MainFrame::OnMenuSettings, this,
       static_cast<int>(ControlId::kMenuSettings));
  Bind(wxEVT_MENU, &MainFrame::OnMenuAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);

  object_tree_->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::OnTreeSelectionChanged, this);
  object_tree_->Bind(wxEVT_TREE_ITEM_ACTIVATED, &MainFrame::OnTreeItemActivated, this);

  Bind(wxEVT_TEXT, &MainFrame::OnSearchTextChange, this,
       static_cast<int>(ControlId::kSearchBox));
  Bind(wxEVT_TEXT_ENTER, &MainFrame::OnSearchEnter, this,
       static_cast<int>(ControlId::kSearchBox));
  Bind(wxEVT_BUTTON, &MainFrame::OnSearchPrev, this,
       static_cast<int>(ControlId::kSearchPrev));
  Bind(wxEVT_BUTTON, &MainFrame::OnSearchNext, this,
       static_cast<int>(ControlId::kSearchNext));
  Bind(wxEVT_BUTTON, &MainFrame::OnSearchAdvanced, this,
       static_cast<int>(ControlId::kSearchAdvanced));
}

void MainFrame::UpdateStatusbarText() {
  if (!GetStatusBar()) {
    return;
  }
  if (!GetStatusBar()->IsShown()) {
    return;
  }

  const wxTreeItemId selected = object_tree_->GetSelection();
  if (!selected.IsOk()) {
    GetStatusBar()->SetStatusText("[No database selected]", 0);
    GetStatusBar()->SetStatusText((std::string("mode: ") + ToString(runtime_config_.mode)), 1);
    return;
  }

  const wxString path = TreePath(object_tree_, selected);
  if (path.IsEmpty()) {
    GetStatusBar()->SetStatusText("[No database selected]", 0);
    GetStatusBar()->SetStatusText((std::string("mode: ") + ToString(runtime_config_.mode)), 1);
    return;
  }

  GetStatusBar()->SetStatusText(path, 0);
  GetStatusBar()->SetStatusText((std::string("mode: ") + ToString(runtime_config_.mode)), 1);
}

void MainFrame::EnsureSqlEditor() {
  if (!sql_editor_frame_) {
    sql_editor_frame_ = new SqlEditorFrame(session_client_, &runtime_config_);
    sql_editor_frame_->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
      sql_editor_frame_ = nullptr;
      event.Skip();
    });
  }

  sql_editor_frame_->Show();
  sql_editor_frame_->Raise();
}

std::string MainFrame::MakeTableMetadataKey(const std::string& schema_path,
                                            const std::string& table_name) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(table_name);
}

std::string MainFrame::MakeObjectMetadataKey(const std::string& schema_path,
                                             const std::string& object_name,
                                             const std::string& object_kind) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(object_name) + "|" +
         ToLowerCopy(object_kind);
}

bool MainFrame::GetSelectedTableIdentity(std::string* schema_path,
                                         std::string* table_name) const {
  const wxTreeItemId selected = object_tree_ ? object_tree_->GetSelection() : wxTreeItemId{};
  if (!selected.IsOk()) {
    return false;
  }

  return GetTableIdentityForItem(selected, schema_path, table_name);
}

bool MainFrame::GetTableIdentityForItem(const wxTreeItemId& item,
                                        std::string* schema_path,
                                        std::string* table_name) const {
  if (!object_tree_ || !item.IsOk()) {
    return false;
  }

  auto* node_data = dynamic_cast<TreeNodeData*>(object_tree_->GetItemData(item));
  if (!node_data || node_data->schema_path.empty() || node_data->object_name.empty()) {
    return false;
  }

  const bool is_table_node = (node_data->type == TreeNodeType::kObject &&
                              node_data->object_kind == SqlObjectKind::kTable) ||
                             ((node_data->type == TreeNodeType::kObjectBranch ||
                               node_data->type == TreeNodeType::kObjectLeaf) &&
                              node_data->object_kind == SqlObjectKind::kTable);
  if (!is_table_node) {
    return false;
  }

  if (schema_path) {
    *schema_path = node_data->schema_path;
  }
  if (table_name) {
    *table_name = node_data->object_name;
  }
  return true;
}

bool MainFrame::GetSelectedObjectIdentity(std::string* schema_path,
                                          std::string* object_name,
                                          std::string* object_kind) const {
  const wxTreeItemId selected = object_tree_ ? object_tree_->GetSelection() : wxTreeItemId{};
  if (!selected.IsOk() || !object_tree_) {
    return false;
  }

  auto* node_data = dynamic_cast<TreeNodeData*>(object_tree_->GetItemData(selected));
  if (!node_data || node_data->schema_path.empty() || node_data->object_name.empty()) {
    return false;
  }

  SqlObjectKind kind = SqlObjectKind::kOther;
  if (node_data->type == TreeNodeType::kObject ||
      node_data->type == TreeNodeType::kObjectBranch ||
      node_data->type == TreeNodeType::kObjectLeaf) {
    kind = node_data->object_kind;
  } else {
    return false;
  }

  if (schema_path) {
    *schema_path = node_data->schema_path;
  }
  if (object_name) {
    *object_name = node_data->object_name;
  }
  if (object_kind) {
    *object_kind = ObjectKindName(kind);
  }
  return true;
}

std::string MainFrame::GetSelectedSchemaPath() const {
  const wxTreeItemId selected = object_tree_ ? object_tree_->GetSelection() : wxTreeItemId{};
  if (!selected.IsOk()) {
    return "root.users.public";
  }

  auto* node_data = dynamic_cast<TreeNodeData*>(object_tree_->GetItemData(selected));
  if (!node_data) {
    return "root.users.public";
  }

  if (!node_data->schema_path.empty()) {
    return node_data->schema_path;
  }

  return "root.users.public";
}

bool MainFrame::SavePreviewTableMetadataRecord(
    const backend::PreviewTableMetadata& metadata) {
  std::string load_error;
  std::vector<backend::PreviewTableMetadata> tables =
      backend::LoadPreviewTableMetadata(backend::DefaultPreviewMetadataPath(), &load_error);
  backend::UpsertPreviewTableMetadata(&tables, metadata);

  std::string save_error;
  const bool saved = backend::SavePreviewTableMetadata(
      backend::DefaultPreviewMetadataPath(), tables, &save_error);
  if (!saved) {
    wxMessageBox("Failed to save preview metadata:\n" +
                     wxString::FromUTF8(save_error),
                 "Save Error", wxOK | wxICON_ERROR, this);
    return false;
  }
  return true;
}

bool MainFrame::SavePreviewObjectMetadataRecord(
    const backend::PreviewObjectMetadata& metadata) {
  std::string load_error;
  std::vector<backend::PreviewObjectMetadata> objects =
      backend::LoadPreviewObjectMetadata(
          backend::DefaultPreviewObjectMetadataPath(), &load_error);
  backend::UpsertPreviewObjectMetadata(&objects, metadata);

  std::string save_error;
  const bool saved = backend::SavePreviewObjectMetadata(
      backend::DefaultPreviewObjectMetadataPath(), objects, &save_error);
  if (!saved) {
    wxMessageBox("Failed to save object metadata:\n" +
                     wxString::FromUTF8(save_error),
                 "Save Error", wxOK | wxICON_ERROR, this);
    return false;
  }
  return true;
}

void MainFrame::OpenTableMetadataDialog(const bool create_new, const bool view_only) {
  backend::PreviewTableMetadata seed;
  if (create_new) {
    seed.schema_path = GetSelectedSchemaPath();
    seed.table_name = "new_table";
  } else {
    std::string schema_path;
    std::string table_name;
    if (!GetSelectedTableIdentity(&schema_path, &table_name)) {
      wxMessageBox("Select a table object in the tree first.",
                   "No Table Selected", wxOK | wxICON_INFORMATION, this);
      return;
    }
    seed.schema_path = schema_path;
    seed.table_name = table_name;

    const auto it = table_metadata_by_key_.find(
        MakeTableMetadataKey(schema_path, table_name));
    if (it != table_metadata_by_key_.end()) {
      seed = it->second;
    }
  }

  const TableMetadataDialog::Mode mode =
      create_new ? TableMetadataDialog::Mode::kCreate
                 : (view_only ? TableMetadataDialog::Mode::kView
                              : TableMetadataDialog::Mode::kEdit);

  TableMetadataDialog dialog(this, mode, seed);
  if (dialog.ShowModal() != wxID_OK || view_only) {
    return;
  }

  const backend::PreviewTableMetadata metadata = dialog.GetMetadata();
  if (!SavePreviewTableMetadataRecord(metadata)) {
    return;
  }

  PopulateTree();
  search_box_->SetValue(wxString::FromUTF8(metadata.table_name));
  ApplySearchFilter();
}

void MainFrame::OpenObjectMetadataDialog(const bool create_new,
                                         const bool view_only,
                                         const std::string& create_kind) {
  backend::PreviewObjectMetadata seed;
  if (create_new) {
    seed.schema_path = GetSelectedSchemaPath();
    seed.object_kind = create_kind.empty() ? "other" : create_kind;
    if (seed.object_kind == "view") {
      seed.object_name = "new_view";
    } else if (seed.object_kind == "procedure") {
      seed.object_name = "new_procedure";
    } else if (seed.object_kind == "function") {
      seed.object_name = "new_function";
    } else if (seed.object_kind == "sequence") {
      seed.object_name = "new_sequence";
    } else if (seed.object_kind == "trigger") {
      seed.object_name = "new_trigger";
    } else if (seed.object_kind == "domain") {
      seed.object_name = "new_domain";
    } else if (seed.object_kind == "package") {
      seed.object_name = "new_package";
    } else if (seed.object_kind == "job") {
      seed.object_name = "new_job";
    } else if (seed.object_kind == "schedule") {
      seed.object_name = "new_schedule";
    } else if (seed.object_kind == "user") {
      seed.object_name = "new_user";
    } else if (seed.object_kind == "role") {
      seed.object_name = "new_role";
    } else if (seed.object_kind == "group") {
      seed.object_name = "new_group";
    } else {
      seed.object_name = "new_object";
    }
  } else {
    std::string schema_path;
    std::string object_name;
    std::string object_kind;
    if (!GetSelectedObjectIdentity(&schema_path, &object_name, &object_kind)) {
      wxMessageBox("Select an object node in the tree first.",
                   "No Object Selected", wxOK | wxICON_INFORMATION, this);
      return;
    }

    if (object_kind == "table") {
      OpenTableMetadataDialog(false, view_only);
      return;
    }

    seed.schema_path = schema_path;
    seed.object_name = object_name;
    seed.object_kind = object_kind;
    const auto it = object_metadata_by_key_.find(
        MakeObjectMetadataKey(schema_path, object_name, object_kind));
    if (it != object_metadata_by_key_.end()) {
      seed = it->second;
    }
  }

  const ObjectMetadataDialog::Mode mode =
      create_new ? ObjectMetadataDialog::Mode::kCreate
                 : (view_only ? ObjectMetadataDialog::Mode::kView
                              : ObjectMetadataDialog::Mode::kEdit);

  ObjectMetadataDialog dialog(this, mode, seed);
  if (dialog.ShowModal() != wxID_OK || view_only) {
    return;
  }

  const backend::PreviewObjectMetadata metadata = dialog.GetMetadata();
  if (!SavePreviewObjectMetadataRecord(metadata)) {
    return;
  }

  PopulateTree();
  search_box_->SetValue(wxString::FromUTF8(metadata.object_name));
  ApplySearchFilter();
}

void MainFrame::ApplySearchNavigation(const bool next) {
  const wxString query = search_box_->GetValue().Trim().Trim(false);
  if (query.IsEmpty()) {
    return;
  }

  wxTreeItemId match;
  if (FindFirstMatch(object_tree_, tree_root_, query, &match)) {
    object_tree_->SelectItem(match);
    object_tree_->EnsureVisible(match);
    return;
  }

  wxMessageBox(next ? "No next match found." : "No previous match found.",
               "Search", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::ApplySearchFilter() {
  const wxString query = search_box_->GetValue().Trim().Trim(false);
  if (query.IsEmpty()) {
    return;
  }

  wxTreeItemId match;
  if (FindFirstMatch(object_tree_, tree_root_, query, &match)) {
    object_tree_->SelectItem(match);
    object_tree_->EnsureVisible(match);
  }
}

void MainFrame::OnMenuOpenSqlEditor(wxCommandEvent&) {
  EnsureSqlEditor();
}

void MainFrame::ApplyModeSelection(backend::TransportMode mode) {
  runtime_config_.mode = mode;
  if (mode_preview_item_) {
    mode_preview_item_->Check(mode == backend::TransportMode::kPreview);
  }
  if (mode_managed_item_) {
    mode_managed_item_->Check(mode == backend::TransportMode::kManaged);
  }
  if (mode_listener_only_item_) {
    mode_listener_only_item_->Check(mode == backend::TransportMode::kListenerOnly);
  }
  if (mode_ipc_only_item_) {
    mode_ipc_only_item_->Check(mode == backend::TransportMode::kIpcOnly);
  }
  if (mode_embedded_item_) {
    mode_embedded_item_->Check(mode == backend::TransportMode::kEmbedded);
  }
  UpdateStatusbarText();
}

void MainFrame::OnMenuToggleStatusBar(wxCommandEvent&) {
  if (GetStatusBar()) {
    GetStatusBar()->Destroy();
    return;
  }

  CreateStatusBar(2);
  UpdateStatusbarText();
}

void MainFrame::OnMenuToggleSearchBar(wxCommandEvent&) {
  if (!search_panel_ || !search_panel_sizer_) {
    return;
  }

  const bool shown = search_panel_->IsShown();
  search_panel_sizer_->Show(search_panel_, !shown, true);
  search_panel_sizer_->Layout();
  main_panel_->Layout();
}

void MainFrame::OnMenuToggleDebugMode(wxCommandEvent&) {
  debug_mode_enabled_ =
      toggle_debug_mode_item_ ? toggle_debug_mode_item_->IsChecked()
                              : !debug_mode_enabled_;
  PopulateTree();
}

void MainFrame::OnMenuModePreview(wxCommandEvent&) {
  ApplyModeSelection(backend::TransportMode::kPreview);
}

void MainFrame::OnMenuModeManaged(wxCommandEvent&) {
  ApplyModeSelection(backend::TransportMode::kManaged);
}

void MainFrame::OnMenuModeListenerOnly(wxCommandEvent&) {
  ApplyModeSelection(backend::TransportMode::kListenerOnly);
}

void MainFrame::OnMenuModeIpcOnly(wxCommandEvent&) {
  ApplyModeSelection(backend::TransportMode::kIpcOnly);
}

void MainFrame::OnMenuModeEmbedded(wxCommandEvent&) {
  ApplyModeSelection(backend::TransportMode::kEmbedded);
}

void MainFrame::OnMenuNewTable(wxCommandEvent&) {
  OpenTableMetadataDialog(true, false);
}

void MainFrame::OnMenuNewView(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "view");
}

void MainFrame::OnMenuNewProcedure(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "procedure");
}

void MainFrame::OnMenuNewFunction(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "function");
}

void MainFrame::OnMenuNewSequence(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "sequence");
}

void MainFrame::OnMenuNewTrigger(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "trigger");
}

void MainFrame::OnMenuNewDomain(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "domain");
}

void MainFrame::OnMenuNewPackage(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "package");
}

void MainFrame::OnMenuNewJob(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "job");
}

void MainFrame::OnMenuNewSchedule(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "schedule");
}

void MainFrame::OnMenuNewUser(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "user");
}

void MainFrame::OnMenuNewRole(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "role");
}

void MainFrame::OnMenuNewGroup(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false, "group");
}

void MainFrame::OnMenuNewObject(wxCommandEvent&) {
  OpenObjectMetadataDialog(true, false);
}

void MainFrame::OnMenuObjectProperties(wxCommandEvent&) {
  std::string schema_path;
  std::string object_name;
  std::string object_kind;
  if (!GetSelectedObjectIdentity(&schema_path, &object_name, &object_kind)) {
    wxMessageBox("Select an object in the tree first.",
                 "No Object Selected", wxOK | wxICON_INFORMATION, this);
    return;
  }
  if (object_kind == "table") {
    OpenTableMetadataDialog(false, false);
    return;
  }
  OpenObjectMetadataDialog(false, false);
}

void MainFrame::OnMenuPlaceholder(wxCommandEvent&) {
  wxMessageBox("This action is scaffolded for ScratchBird migration and will be wired next.",
               "Feature Scaffold", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnMenuSettings(wxCommandEvent&) {
  SettingsDialog dlg(this);
  dlg.ShowWithLivePreview();
}

void MainFrame::ApplySettings() {
  const core::Settings& settings = core::Settings::get();
  const core::ScaleSettings& scales = settings.getScaleSettings();
  
  // Apply theme
  const core::ThemeColors colors = settings.getThemeColors();
  
  // Apply to this window
  SetBackgroundColour(colors.background);
  SetForegroundColour(colors.foreground);
  
  // Apply to menu bar
  if (menu_bar_) {
    wxFont menu_font = menu_bar_->GetFont();
    menu_font.SetPointSize(scales.getFontSize(9, scales.menu_font_scale));
    menu_bar_->SetFont(menu_font);
  }
  
  // Apply to main panel
  if (main_panel_) {
    main_panel_->SetBackgroundColour(colors.background);
    main_panel_->SetForegroundColour(colors.foreground);
  }
  
  // Apply to tree
  if (object_tree_) {
    object_tree_->SetBackgroundColour(colors.tree_bg);
    object_tree_->SetForegroundColour(colors.tree_fg);
    
    // Tree font
    wxFont tree_font = object_tree_->GetFont();
    tree_font.SetPointSize(scales.getFontSize(9, scales.tree_text_scale));
    object_tree_->SetFont(tree_font);
    
    // Rebuild tree images with new icon size
    BuildTreeImageList();
  }
  
  // Apply to status bar
  wxStatusBar* status = GetStatusBar();
  if (status) {
    status->SetBackgroundColour(colors.status_bg);
    status->SetForegroundColour(colors.status_fg);
    wxFont status_font = status->GetFont();
    status_font.SetPointSize(scales.getFontSize(9, scales.status_bar_scale));
    status->SetFont(status_font);
  }
  
  // Refresh layout
  Layout();
  Refresh();
}

void MainFrame::OnSettingsChanged() {
  ApplySettings();
}

void MainFrame::OnMenuAbout(wxCommandEvent&) {
  wxMessageBox(
      "robin-migrate\n\nFlameRobin-style client scaffold for ScratchBird.\n"
      "Execution path remains parser/native-adapter -> ServerSession boundary.",
      "About robin-migrate", wxOK | wxICON_INFORMATION, this);
}

void MainFrame::OnTreeSelectionChanged(wxTreeEvent&) {
  UpdateStatusbarText();
}

void MainFrame::OnTreeItemActivated(wxTreeEvent& event) {
  const wxTreeItemId item = event.GetItem();
  if (!item.IsOk()) {
    return;
  }

  object_tree_->SelectItem(item);

  std::string schema_path;
  std::string object_name;
  std::string object_kind;
  if (GetSelectedObjectIdentity(&schema_path, &object_name, &object_kind)) {
    if (object_kind == "table") {
      OpenTableMetadataDialog(false, true);
    } else {
      OpenObjectMetadataDialog(false, true);
    }
    return;
  }

  if (object_tree_->IsExpanded(item)) {
    object_tree_->Collapse(item);
  } else {
    object_tree_->Expand(item);
  }
}

void MainFrame::OnSearchTextChange(wxCommandEvent&) {
  ApplySearchFilter();
}

void MainFrame::OnSearchEnter(wxCommandEvent&) {
  ApplySearchNavigation(true);
}

void MainFrame::OnSearchPrev(wxCommandEvent&) {
  ApplySearchNavigation(false);
}

void MainFrame::OnSearchNext(wxCommandEvent&) {
  ApplySearchNavigation(true);
}

void MainFrame::OnSearchAdvanced(wxCommandEvent&) {
  wxMessageBox("Advanced metadata search dialog is not wired yet.",
               "Advanced Search", wxOK | wxICON_INFORMATION, this);
}

}  // namespace scratchrobin::ui
