/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "backend/scratchbird_catalog_preview.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace scratchrobin::backend {

namespace {

bool StartsWith(const std::string& value, const std::string& prefix) {
  return value.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), value.begin());
}

std::string ToLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string ToUpperCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return value;
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

std::string ReadTextFile(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::in | std::ios::binary);
  if (!stream.good()) {
    return {};
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

std::string SliceInitializerBlock(const std::string& source, const std::string& marker) {
  const std::size_t marker_pos = source.find(marker);
  if (marker_pos == std::string::npos) {
    return {};
  }

  const std::size_t brace_start = source.find('{', marker_pos);
  if (brace_start == std::string::npos) {
    return {};
  }

  int depth = 0;
  for (std::size_t i = brace_start; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}') {
      --depth;
      if (depth == 0) {
        return source.substr(brace_start, (i - brace_start) + 1);
      }
    }
  }

  return {};
}

std::string NormalizeSchemaPath(const std::string& raw_path) {
  const std::string trimmed = TrimCopy(raw_path);
  if (trimmed.empty()) {
    return {};
  }
  if (trimmed == "root" || StartsWith(trimmed, "root.")) {
    return trimmed;
  }
  return "root." + trimmed;
}

std::vector<std::string> SplitPath(const std::string& path) {
  std::vector<std::string> components;
  std::size_t start = 0;
  while (start < path.size()) {
    const std::size_t dot = path.find('.', start);
    const std::size_t end = (dot == std::string::npos) ? path.size() : dot;
    if (end > start) {
      components.push_back(path.substr(start, end - start));
    }
    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }
  return components;
}

std::string JoinPathComponents(const std::vector<std::string>& components,
                               const std::size_t start, const std::size_t end) {
  if (start >= end || end > components.size()) {
    return {};
  }

  std::string out;
  for (std::size_t i = start; i < end; ++i) {
    if (!out.empty()) {
      out.push_back('.');
    }
    out += components[i];
  }
  return out;
}

std::vector<std::string> ExtractBootstrapSchemaPaths(const std::string& catalog_source) {
  std::string block = SliceInitializerBlock(catalog_source, "kBootstrapSchemas");
  if (block.empty()) {
    block = SliceInitializerBlock(catalog_source, "kCanonicalNodes");
  }
  if (block.empty()) {
    return {};
  }

  static const std::regex kNodeRegex(
      R"re(\{\s*"([^"]+)"\s*,\s*"[^"]+"\s*,\s*(?:nullptr|"[^"]+")\s*,\s*(?:true|false|nullptr|"[^"]+")\s*\})re");

  std::set<std::string> dedup;
  for (std::sregex_iterator it(block.begin(), block.end(), kNodeRegex), end; it != end; ++it) {
    const std::string normalized = NormalizeSchemaPath((*it)[1].str());
    if (!normalized.empty()) {
      dedup.insert(normalized);
    }
  }

  if (dedup.empty()) {
    return {};
  }
  dedup.insert("root");
  return {dedup.begin(), dedup.end()};
}

std::vector<std::pair<std::string, std::string>> ExtractAliasMapEntries(
    const std::string& catalog_source) {
  const std::string block = SliceInitializerBlock(catalog_source, "kSystemTableAliasMap");
  if (block.empty()) {
    return {};
  }

  static const std::regex kAliasRegex(
      R"re(\{\s*"([^"]+)"\s*,\s*"sys\.([^"]+)"\s*\})re");
  std::vector<std::pair<std::string, std::string>> entries;
  for (std::sregex_iterator it(block.begin(), block.end(), kAliasRegex), end; it != end; ++it) {
    entries.emplace_back((*it)[1].str(), (*it)[2].str());
  }
  return entries;
}

std::vector<std::string> ExtractSysVirtualTableNames(const std::string& sys_catalog_source) {
  const std::string block =
      SliceInitializerBlock(sys_catalog_source, "void SysCatalogHandler::initializeTableNames()");
  if (block.empty()) {
    return {};
  }

  static const std::regex kNameRegex("\"([a-z0-9_]+)\"");
  std::vector<std::string> names;
  for (std::sregex_iterator it(block.begin(), block.end(), kNameRegex), end; it != end; ++it) {
    names.push_back((*it)[1].str());
  }
  return names;
}

std::unordered_map<std::string, std::string> ExtractSysDispatchQueryFunctions(
    const std::string& sys_catalog_source) {
  static const std::regex kDispatchRegex(
      R"re(equalsCaseInsensitive\(table_name,\s*"([a-z0-9_]+)"\)\)\s*\{\s*return\s+([A-Za-z0-9_]+)\s*\()re");
  std::unordered_map<std::string, std::string> dispatch;
  for (std::sregex_iterator it(sys_catalog_source.begin(), sys_catalog_source.end(),
                               kDispatchRegex),
       end;
       it != end; ++it) {
    dispatch.emplace((*it)[1].str(), (*it)[2].str());
  }
  return dispatch;
}

std::unordered_map<std::string, std::string> ExtractSysColumnDefSymbolsByTable(
    const std::string& sys_catalog_source) {
  static const std::regex kMappingRegex(
      R"re(equalsCaseInsensitive\(table_name,\s*"([a-z0-9_]+)"\)\)\s*\{\s*return\s*&([A-Za-z0-9_]+)\s*;)re");

  std::unordered_map<std::string, std::string> mapping;
  for (std::sregex_iterator it(sys_catalog_source.begin(), sys_catalog_source.end(),
                               kMappingRegex),
       end;
       it != end; ++it) {
    mapping.emplace((*it)[1].str(), (*it)[2].str());
  }
  return mapping;
}

struct ParsedRecordField {
  std::string name;
  std::string cpp_type;
  bool is_array{false};
};

std::string NormalizeRecordKey(const std::string& value) {
  std::string normalized;
  normalized.reserve(value.size());
  for (const unsigned char c : value) {
    if (std::isalnum(c)) {
      normalized.push_back(static_cast<char>(std::tolower(c)));
    }
  }
  return normalized;
}

std::unordered_map<std::string, std::string> ExtractSystemDomainByColumn(
    const std::string& catalog_source) {
  const std::string block = SliceInitializerBlock(catalog_source, "kSystemDomainByColumn");
  if (block.empty()) {
    return {};
  }

  static const std::regex kEntryRegex(
      R"re(\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\})re");
  std::unordered_map<std::string, std::string> domains;
  for (std::sregex_iterator it(block.begin(), block.end(), kEntryRegex), end; it != end; ++it) {
    domains.emplace(ToLowerCopy((*it)[1].str()), (*it)[2].str());
  }
  return domains;
}

std::unordered_map<std::string, std::string> ExtractSystemDomainByTableColumn(
    const std::string& catalog_source) {
  const std::string block =
      SliceInitializerBlock(catalog_source, "kSystemDomainByTableColumn");
  if (block.empty()) {
    return {};
  }

  static const std::regex kEntryRegex(
      R"re(\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\})re");
  std::unordered_map<std::string, std::string> domains;
  for (std::sregex_iterator it(block.begin(), block.end(), kEntryRegex), end; it != end; ++it) {
    domains.emplace(ToLowerCopy((*it)[1].str()), (*it)[2].str());
  }
  return domains;
}

std::unordered_map<std::string, std::vector<ParsedRecordField>> ExtractRecordStructFields(
    const std::string& catalog_source) {
  static const std::regex kStructRegex(R"re(struct\s+([A-Za-z0-9_]+)\s*\{)re");
  static const std::regex kFieldRegex(
      R"re(^\s*([^;{}()]+?)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(\[[^\]]+\])?\s*;\s*$)re");

  std::unordered_map<std::string, std::vector<ParsedRecordField>> fields_by_record;
  for (std::sregex_iterator it(catalog_source.begin(), catalog_source.end(), kStructRegex), end;
       it != end; ++it) {
    const std::string struct_name = (*it)[1].str();
    const std::size_t struct_start = static_cast<std::size_t>((*it).position(0));
    const std::size_t brace_start = catalog_source.find('{', struct_start);
    if (brace_start == std::string::npos) {
      continue;
    }

    int depth = 0;
    std::size_t brace_end = std::string::npos;
    for (std::size_t i = brace_start; i < catalog_source.size(); ++i) {
      if (catalog_source[i] == '{') {
        ++depth;
      } else if (catalog_source[i] == '}') {
        --depth;
        if (depth == 0) {
          brace_end = i;
          break;
        }
      }
    }
    if (brace_end == std::string::npos || brace_end <= brace_start) {
      continue;
    }

    const std::string block = catalog_source.substr(brace_start + 1, brace_end - brace_start - 1);
    std::vector<ParsedRecordField> parsed_fields;
    std::istringstream lines(block);
    std::string line;
    while (std::getline(lines, line)) {
      const std::size_t comment = line.find("//");
      if (comment != std::string::npos) {
        line = line.substr(0, comment);
      }
      line = TrimCopy(line);
      if (line.empty()) {
        continue;
      }

      std::smatch match;
      if (!std::regex_match(line, match, kFieldRegex)) {
        continue;
      }

      ParsedRecordField field;
      field.cpp_type = TrimCopy(match[1].str());
      field.name = match[2].str();
      field.is_array = match.size() > 3 && match[3].matched;
      if (field.cpp_type.empty() || field.name.empty()) {
        continue;
      }

      const std::string lower_name = ToLowerCopy(field.name);
      if (StartsWith(lower_name, "reserved") || StartsWith(lower_name, "padding")) {
        continue;
      }
      parsed_fields.push_back(std::move(field));
    }

    if (!parsed_fields.empty()) {
      fields_by_record[NormalizeRecordKey(struct_name)] = std::move(parsed_fields);
    }
  }

  return fields_by_record;
}

std::string MapCppTypeToPreviewType(const std::string& cpp_type, const bool is_array) {
  const std::string type = ToLowerCopy(TrimCopy(cpp_type));
  if (type == "id") {
    return "UUID";
  }
  if (type.find("uint8_t") != std::string::npos) {
    return is_array ? "BLOB" : "UINT8";
  }
  if (type.find("uint16_t") != std::string::npos) {
    return "UINT16";
  }
  if (type.find("uint32_t") != std::string::npos) {
    return "UINT32";
  }
  if (type.find("uint64_t") != std::string::npos) {
    return "UINT64";
  }
  if (type.find("int8_t") != std::string::npos) {
    return "INT8";
  }
  if (type.find("int16_t") != std::string::npos) {
    return "INT16";
  }
  if (type.find("int32_t") != std::string::npos) {
    return "INT32";
  }
  if (type.find("int64_t") != std::string::npos) {
    return "INT64";
  }
  if (type.find("bool") != std::string::npos) {
    return "BOOLEAN";
  }
  if (type.find("float") != std::string::npos) {
    return "FLOAT32";
  }
  if (type.find("double") != std::string::npos) {
    return "FLOAT64";
  }
  if (type.find("char") != std::string::npos && is_array) {
    return "TEXT";
  }
  if (type.find("string") != std::string::npos) {
    return "TEXT";
  }
  return "TEXT";
}

std::vector<PreviewColumnMetadata> ParseSysColumnDefsForTable(
    const std::string& sys_catalog_source, const std::string& symbol_name) {
  const std::string block = SliceInitializerBlock(sys_catalog_source, symbol_name);
  if (block.empty()) {
    return {};
  }

  static const std::regex kEntryRegex(
      R"re(\{\s*"([^"]+)"\s*,\s*DataType::([A-Za-z0-9_]+)\s*,\s*(true|false)\s*\})re");

  std::vector<PreviewColumnMetadata> columns;
  for (std::sregex_iterator it(block.begin(), block.end(), kEntryRegex), end; it != end; ++it) {
    PreviewColumnMetadata column;
    column.column_name = ToLowerCopy((*it)[1].str());
    column.data_type = ToUpperCopy((*it)[2].str());
    column.nullable = ((*it)[3].str() == "true");
    columns.push_back(std::move(column));
  }
  return columns;
}

std::string SelectSchemaForAlias(const std::string& alias_name,
                                 const std::string& record_type,
                                 const std::unordered_set<std::string>& known_schemas);
std::string SelectSchemaForSysQueryFunction(const std::string& query_function_name);
std::string SelectSchemaForSysVirtualTable(const std::string& table_name);
PreviewObjectKind InferObjectKind(const std::string& name);
PreviewObjectKind InferSysVirtualObjectKind(const std::string& table_name);

std::vector<PreviewTableMetadata> BuildSourcePreviewTableMetadata(
    const std::string& catalog_source, const std::string& sys_catalog_source,
    const std::vector<std::pair<std::string, std::string>>& alias_entries,
    const std::unordered_set<std::string>& known_schemas) {
  std::vector<PreviewTableMetadata> tables;

  const auto sys_table_to_symbol = ExtractSysColumnDefSymbolsByTable(sys_catalog_source);
  const auto dispatch_by_table = ExtractSysDispatchQueryFunctions(sys_catalog_source);
  for (const auto& [table_name, symbol_name] : sys_table_to_symbol) {
    if (InferSysVirtualObjectKind(table_name) != PreviewObjectKind::kTable) {
      continue;
    }
    PreviewTableMetadata metadata;
    const auto dispatch_it = dispatch_by_table.find(table_name);
    metadata.schema_path =
        (dispatch_it != dispatch_by_table.end())
            ? SelectSchemaForSysQueryFunction(dispatch_it->second)
            : SelectSchemaForSysVirtualTable(table_name);
    metadata.table_name = table_name;
    metadata.columns = ParseSysColumnDefsForTable(sys_catalog_source, symbol_name);
    if (!metadata.schema_path.empty() && !metadata.table_name.empty() &&
        !metadata.columns.empty()) {
      UpsertPreviewTableMetadata(&tables, metadata);
    }
  }

  const auto fields_by_record = ExtractRecordStructFields(catalog_source);
  const auto domain_by_column = ExtractSystemDomainByColumn(catalog_source);
  const auto domain_by_table_column = ExtractSystemDomainByTableColumn(catalog_source);
  for (const auto& [alias_name, record_type] : alias_entries) {
    if (InferObjectKind(alias_name) != PreviewObjectKind::kTable) {
      continue;
    }
    const auto it = fields_by_record.find(NormalizeRecordKey(record_type));
    if (it == fields_by_record.end()) {
      continue;
    }

    PreviewTableMetadata metadata;
    metadata.schema_path = SelectSchemaForAlias(alias_name, record_type, known_schemas);
    metadata.table_name = alias_name;

    bool assigned_pk = false;
    std::vector<std::string> pk_columns;
    std::vector<std::string> fk_columns;
    for (const auto& field : it->second) {
      PreviewColumnMetadata column;
      column.column_name = ToLowerCopy(field.name);
      column.data_type = MapCppTypeToPreviewType(field.cpp_type, field.is_array);
      column.nullable = true;

      const std::string table_column_key =
          "sys." + ToLowerCopy(record_type) + "." + column.column_name;
      const auto table_domain_it = domain_by_table_column.find(table_column_key);
      if (table_domain_it != domain_by_table_column.end()) {
        column.domain_name = table_domain_it->second;
      } else {
        const auto column_domain_it = domain_by_column.find(column.column_name);
        if (column_domain_it != domain_by_column.end()) {
          column.domain_name = column_domain_it->second;
        }
      }

      if (!assigned_pk &&
          (column.column_name == "id" ||
           (column.column_name.size() > 3 &&
            column.column_name.substr(column.column_name.size() - 3) == "_id"))) {
        column.is_primary_key = true;
        column.nullable = false;
        assigned_pk = true;
        pk_columns.push_back(column.column_name);
      } else if (column.column_name.size() > 3 &&
                 column.column_name.substr(column.column_name.size() - 3) == "_id") {
        column.is_foreign_key = true;
        fk_columns.push_back(column.column_name);
      }

      metadata.columns.push_back(std::move(column));
    }

    if (!pk_columns.empty()) {
      PreviewIndexMetadata pk_index;
      pk_index.index_name = "pk_" + metadata.table_name;
      pk_index.unique = true;
      pk_index.method = "BTREE";
      for (std::size_t i = 0; i < pk_columns.size(); ++i) {
        if (i > 0) {
          pk_index.target += ",";
        }
        pk_index.target += pk_columns[i];
      }
      metadata.indexes.push_back(std::move(pk_index));
    }
    for (const auto& fk_column : fk_columns) {
      PreviewIndexMetadata fk_index;
      fk_index.index_name = "idx_" + metadata.table_name + "_" + fk_column;
      fk_index.unique = false;
      fk_index.method = "BTREE";
      fk_index.target = fk_column;
      metadata.indexes.push_back(std::move(fk_index));
    }

    if (!metadata.schema_path.empty() && !metadata.table_name.empty() &&
        !metadata.columns.empty()) {
      UpsertPreviewTableMetadata(&tables, metadata);
    }
  }

  return tables;
}

std::string ObjectKindName(const PreviewObjectKind kind) {
  switch (kind) {
    case PreviewObjectKind::kTable:
      return "table";
    case PreviewObjectKind::kView:
      return "view";
    case PreviewObjectKind::kProcedure:
      return "procedure";
    case PreviewObjectKind::kFunction:
      return "function";
    case PreviewObjectKind::kSequence:
      return "sequence";
    case PreviewObjectKind::kTrigger:
      return "trigger";
    case PreviewObjectKind::kDomain:
      return "domain";
    case PreviewObjectKind::kPackage:
      return "package";
    case PreviewObjectKind::kJob:
      return "job";
    case PreviewObjectKind::kSchedule:
      return "schedule";
    case PreviewObjectKind::kUser:
      return "user";
    case PreviewObjectKind::kRole:
      return "role";
    case PreviewObjectKind::kGroup:
      return "group";
    case PreviewObjectKind::kSchema:
      return "schema";
    case PreviewObjectKind::kColumn:
      return "column";
    case PreviewObjectKind::kIndex:
      return "index";
    case PreviewObjectKind::kConstraint:
      return "constraint";
    case PreviewObjectKind::kCompositeType:
      return "composite_type";
    case PreviewObjectKind::kTablespace:
      return "tablespace";
    case PreviewObjectKind::kDatabase:
      return "database";
    case PreviewObjectKind::kEmulationType:
      return "emulation_type";
    case PreviewObjectKind::kEmulationServer:
      return "emulation_server";
    case PreviewObjectKind::kEmulatedDatabase:
      return "emulated_database";
    case PreviewObjectKind::kCharset:
      return "charset";
    case PreviewObjectKind::kCollation:
      return "collation";
    case PreviewObjectKind::kTimezone:
      return "timezone";
    case PreviewObjectKind::kUdr:
      return "udr";
    case PreviewObjectKind::kUdrEngine:
      return "udr_engine";
    case PreviewObjectKind::kUdrModule:
      return "udr_module";
    case PreviewObjectKind::kException:
      return "exception";
    case PreviewObjectKind::kComment:
      return "comment";
    case PreviewObjectKind::kDependency:
      return "dependency";
    case PreviewObjectKind::kPermission:
      return "permission";
    case PreviewObjectKind::kStatistic:
      return "statistic";
    case PreviewObjectKind::kExtension:
      return "extension";
    case PreviewObjectKind::kForeignServer:
      return "foreign_server";
    case PreviewObjectKind::kForeignTable:
      return "foreign_table";
    case PreviewObjectKind::kUserMapping:
      return "user_mapping";
    case PreviewObjectKind::kServerRegistry:
      return "server_registry";
    case PreviewObjectKind::kCluster:
      return "cluster";
    case PreviewObjectKind::kSynonym:
      return "synonym";
    case PreviewObjectKind::kPolicy:
      return "policy";
    case PreviewObjectKind::kObjectDefinition:
      return "object_definition";
    case PreviewObjectKind::kPreparedTransaction:
      return "prepared_transaction";
    case PreviewObjectKind::kAuditLog:
      return "audit_log";
    case PreviewObjectKind::kMigrationHistory:
      return "migration_history";
    case PreviewObjectKind::kOther:
    default:
      return "other";
  }
}

std::string MakeObjectMetadataKey(const std::string& schema_path,
                                  const std::string& object_name,
                                  const std::string& object_kind) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(object_name) + "|" +
         ToLowerCopy(object_kind);
}

void UpsertObjectMetadata(std::vector<PreviewObjectMetadata>* objects,
                          const PreviewObjectMetadata& metadata) {
  if (!objects || metadata.schema_path.empty() || metadata.object_name.empty() ||
      metadata.object_kind.empty()) {
    return;
  }
  const std::string key =
      MakeObjectMetadataKey(metadata.schema_path, metadata.object_name, metadata.object_kind);
  for (auto& existing : *objects) {
    if (MakeObjectMetadataKey(existing.schema_path, existing.object_name,
                              existing.object_kind) == key) {
      existing = metadata;
      return;
    }
  }
  objects->push_back(metadata);
}

std::vector<PreviewObjectMetadata> BuildSourcePreviewObjectMetadata(
    const std::string& catalog_source, const std::string& sys_catalog_source,
    const std::vector<std::pair<std::string, std::string>>& alias_entries,
    const std::unordered_set<std::string>& known_schemas) {
  std::vector<PreviewObjectMetadata> objects;

  const auto fields_by_record = ExtractRecordStructFields(catalog_source);
  const auto domain_by_column = ExtractSystemDomainByColumn(catalog_source);
  const auto domain_by_table_column = ExtractSystemDomainByTableColumn(catalog_source);
  const auto sys_table_to_symbol = ExtractSysColumnDefSymbolsByTable(sys_catalog_source);

  auto append_fields = [&](const std::string& record_type,
                           const std::vector<ParsedRecordField>& fields,
                           std::vector<PreviewObjectPropertyMetadata>* out) {
    if (!out) {
      return;
    }
    for (const auto& field : fields) {
      PreviewObjectPropertyMetadata property;
      property.property_name = ToLowerCopy(field.name);
      property.property_type = MapCppTypeToPreviewType(field.cpp_type, field.is_array);

      const std::string table_column_key =
          "sys." + ToLowerCopy(record_type) + "." + property.property_name;
      const auto table_domain_it = domain_by_table_column.find(table_column_key);
      if (table_domain_it != domain_by_table_column.end()) {
        property.notes = "domain=" + table_domain_it->second;
      } else {
        const auto column_domain_it = domain_by_column.find(property.property_name);
        if (column_domain_it != domain_by_column.end()) {
          property.notes = "domain=" + column_domain_it->second;
        }
      }

      out->push_back(std::move(property));
    }
  };

  auto append_sys_columns = [&](const std::string& table_name,
                                std::vector<PreviewObjectPropertyMetadata>* out) {
    if (!out) {
      return;
    }
    const auto symbol_it = sys_table_to_symbol.find(ToLowerCopy(table_name));
    if (symbol_it == sys_table_to_symbol.end()) {
      return;
    }
    for (const auto& column :
         ParseSysColumnDefsForTable(sys_catalog_source, symbol_it->second)) {
      PreviewObjectPropertyMetadata property;
      property.property_name = column.column_name;
      property.property_type = column.data_type;
      if (!column.nullable) {
        property.notes = "not null";
      }
      const auto column_domain_it = domain_by_column.find(property.property_name);
      if (column_domain_it != domain_by_column.end()) {
        if (!property.notes.empty()) {
          property.notes += "; ";
        }
        property.notes += "domain=" + column_domain_it->second;
      }
      out->push_back(std::move(property));
    }
  };

  for (const auto& [alias_name, record_type] : alias_entries) {
    const PreviewObjectKind inferred_kind = InferObjectKind(alias_name);
    if (inferred_kind == PreviewObjectKind::kTable) {
      continue;
    }

    PreviewObjectMetadata metadata;
    metadata.schema_path = SelectSchemaForAlias(alias_name, record_type, known_schemas);
    metadata.object_name = alias_name;
    metadata.object_kind = ObjectKindName(inferred_kind);
    metadata.description = "Source-derived definition from " + record_type;

    const auto fields_it = fields_by_record.find(NormalizeRecordKey(record_type));
    if (fields_it != fields_by_record.end()) {
      append_fields(record_type, fields_it->second, &metadata.properties);
    }
    if (metadata.properties.empty()) {
      append_sys_columns(alias_name, &metadata.properties);
    }

    if (!metadata.schema_path.empty() && !metadata.object_name.empty() &&
        !metadata.object_kind.empty()) {
      UpsertObjectMetadata(&objects, metadata);
    }

    if (ToLowerCopy(alias_name) == "procedures") {
      PreviewObjectMetadata function_metadata = metadata;
      function_metadata.object_name = "functions";
      function_metadata.object_kind = "function";
      function_metadata.description =
          "Source-derived definition from " + record_type +
          " (procedure_type=function)";
      UpsertObjectMetadata(&objects, function_metadata);
    }
  }

  for (const auto& [table_name, symbol_name] : sys_table_to_symbol) {
    (void)symbol_name;
    const PreviewObjectKind kind = InferSysVirtualObjectKind(table_name);
    if (kind == PreviewObjectKind::kTable) {
      continue;
    }

    PreviewObjectMetadata metadata;
    metadata.schema_path = SelectSchemaForSysVirtualTable(table_name);
    metadata.object_name = table_name;
    metadata.object_kind = ObjectKindName(kind);
    metadata.description = "Source-derived definition from sys." + table_name;
    append_sys_columns(table_name, &metadata.properties);
    UpsertObjectMetadata(&objects, metadata);
  }

  struct SyntheticObjectMapping {
    const char* object_name;
    const char* record_type;
    PreviewObjectKind kind;
    const char* schema_path;
  };
  static constexpr SyntheticObjectMapping kSyntheticObjects[] = {
      {"job_types", "JobTypeCatalogRecord", PreviewObjectKind::kJob, "root.sys.jobs"},
      {"job_type_params", "JobTypeParamCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"job_params", "JobParamCatalogRecord", PreviewObjectKind::kJob, "root.sys.jobs"},
      {"job_schedules", "JobScheduleCatalogRecord", PreviewObjectKind::kSchedule,
       "root.sys.jobs"},
      {"job_type_policies", "JobTypePolicyCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"package_members", "PackageMemberRecord", PreviewObjectKind::kPackage,
       "root.sys.system"},
      {"domain_param_keys", "DomainParamKeyRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_parameters", "DomainParameterRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_constraints", "DomainConstraintCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_security", "DomainSecurityCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_validation", "DomainValidationCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_integrity", "DomainIntegrityCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
  };

  for (const auto& synthetic : kSyntheticObjects) {
    const auto fields_it =
        fields_by_record.find(NormalizeRecordKey(synthetic.record_type));
    if (fields_it == fields_by_record.end()) {
      continue;
    }

    PreviewObjectMetadata metadata;
    metadata.schema_path = synthetic.schema_path;
    metadata.object_name = synthetic.object_name;
    metadata.object_kind = ObjectKindName(synthetic.kind);
    metadata.description =
        "Source-derived definition from " + std::string(synthetic.record_type);
    append_fields(synthetic.record_type, fields_it->second, &metadata.properties);
    UpsertObjectMetadata(&objects, metadata);
  }

  return objects;
}

std::string ResolveSourceDirectory(const std::string& requested) {
  if (!requested.empty()) {
    return requested;
  }

  if (const char* env = std::getenv("ROBIN_MIGRATE_SCRATCHBIRD_SOURCE_DIR")) {
    if (env[0] != '\0') {
      return env;
    }
  }

#ifdef ROBIN_MIGRATE_DEFAULT_SCRATCHBIRD_SOURCE_DIR
  return ROBIN_MIGRATE_DEFAULT_SCRATCHBIRD_SOURCE_DIR;
#else
  return "/home/dcalford/CliWork/ScratchBird";
#endif
}

std::string NormalizeServerName(const std::string& host) {
  const std::string trimmed = TrimCopy(host);
  if (trimmed.empty()) {
    return "localhost";
  }
  if (trimmed == "127.0.0.1" || trimmed == "::1") {
    return "localhost";
  }
  return trimmed;
}

std::string NormalizeDatabaseName(const std::string& database) {
  const std::string trimmed = TrimCopy(database);
  if (trimmed.empty()) {
    return "default";
  }
  return trimmed;
}

PreviewObjectKind InferObjectKind(const std::string& name) {
  const std::string lower = ToLowerCopy(name);
  static const std::unordered_map<std::string, PreviewObjectKind> kAliasKinds{
      {"audit_log", PreviewObjectKind::kAuditLog},
      {"authkeys", PreviewObjectKind::kUser},
      {"charsets", PreviewObjectKind::kCharset},
      {"clusters", PreviewObjectKind::kCluster},
      {"collations", PreviewObjectKind::kCollation},
      {"columns", PreviewObjectKind::kColumn},
      {"comments", PreviewObjectKind::kComment},
      {"composite_types", PreviewObjectKind::kCompositeType},
      {"constraints", PreviewObjectKind::kConstraint},
      {"databases", PreviewObjectKind::kDatabase},
      {"dependencies", PreviewObjectKind::kDependency},
      {"domains", PreviewObjectKind::kDomain},
      {"dormant_transactions", PreviewObjectKind::kPreparedTransaction},
      {"emulated_dbs", PreviewObjectKind::kEmulatedDatabase},
      {"emulated_databases", PreviewObjectKind::kEmulatedDatabase},
      {"emulation_servers", PreviewObjectKind::kEmulationServer},
      {"emulation_types", PreviewObjectKind::kEmulationType},
      {"exceptions", PreviewObjectKind::kException},
      {"extensions", PreviewObjectKind::kExtension},
      {"foreign_keys", PreviewObjectKind::kConstraint},
      {"foreign_servers", PreviewObjectKind::kForeignServer},
      {"foreign_tables", PreviewObjectKind::kForeignTable},
      {"functions", PreviewObjectKind::kFunction},
      {"group_mappings", PreviewObjectKind::kGroup},
      {"group_memberships", PreviewObjectKind::kGroup},
      {"groups", PreviewObjectKind::kGroup},
      {"indexes", PreviewObjectKind::kIndex},
      {"index_versions", PreviewObjectKind::kIndex},
      {"jobs", PreviewObjectKind::kJob},
      {"migration_history", PreviewObjectKind::kMigrationHistory},
      {"object_definitions", PreviewObjectKind::kObjectDefinition},
      {"packages", PreviewObjectKind::kPackage},
      {"permissions", PreviewObjectKind::kPermission},
      {"policies", PreviewObjectKind::kPolicy},
      {"prepared_transactions", PreviewObjectKind::kPreparedTransaction},
      {"procedure_params", PreviewObjectKind::kProcedure},
      {"procedures", PreviewObjectKind::kProcedure},
      {"role_memberships", PreviewObjectKind::kRole},
      {"roles", PreviewObjectKind::kRole},
      {"schedules", PreviewObjectKind::kSchedule},
      {"schemas", PreviewObjectKind::kSchema},
      {"security_policy_epoch", PreviewObjectKind::kPolicy},
      {"sequences", PreviewObjectKind::kSequence},
      {"server_registry", PreviewObjectKind::kServerRegistry},
      {"sessions", PreviewObjectKind::kOther},
      {"statistics", PreviewObjectKind::kStatistic},
      {"synonyms", PreviewObjectKind::kSynonym},
      {"tables", PreviewObjectKind::kTable},
      {"tablespaces", PreviewObjectKind::kTablespace},
      {"timezones", PreviewObjectKind::kTimezone},
      {"triggers", PreviewObjectKind::kTrigger},
      {"udr", PreviewObjectKind::kUdr},
      {"udr_engines", PreviewObjectKind::kUdrEngine},
      {"udr_modules", PreviewObjectKind::kUdrModule},
      {"user_mappings", PreviewObjectKind::kUserMapping},
      {"users", PreviewObjectKind::kUser},
      {"views", PreviewObjectKind::kView},
      {"job_types", PreviewObjectKind::kJob},
      {"job_type_params", PreviewObjectKind::kJob},
      {"job_params", PreviewObjectKind::kJob},
      {"job_schedules", PreviewObjectKind::kSchedule},
      {"job_type_policies", PreviewObjectKind::kJob},
      {"package_members", PreviewObjectKind::kPackage},
      {"domain_param_keys", PreviewObjectKind::kDomain},
      {"domain_parameters", PreviewObjectKind::kDomain},
      {"domain_constraints", PreviewObjectKind::kDomain},
      {"domain_security", PreviewObjectKind::kDomain},
      {"domain_validation", PreviewObjectKind::kDomain},
      {"domain_integrity", PreviewObjectKind::kDomain},
  };

  const auto alias_it = kAliasKinds.find(lower);
  if (alias_it != kAliasKinds.end()) {
    return alias_it->second;
  }

  if (lower.find("trigger") != std::string::npos) {
    return PreviewObjectKind::kTrigger;
  }
  if (lower.find("procedure") != std::string::npos || StartsWith(lower, "sp_")) {
    return PreviewObjectKind::kProcedure;
  }
  if (lower.find("function") != std::string::npos || StartsWith(lower, "fn_")) {
    return PreviewObjectKind::kFunction;
  }
  if (lower.find("sequence") != std::string::npos || StartsWith(lower, "seq_")) {
    return PreviewObjectKind::kSequence;
  }
  if (lower.find("view") != std::string::npos || StartsWith(lower, "v_")) {
    return PreviewObjectKind::kView;
  }
  if (lower.find("job") != std::string::npos) {
    return PreviewObjectKind::kJob;
  }
  if (lower.find("schedule") != std::string::npos) {
    return PreviewObjectKind::kSchedule;
  }
  if (lower.find("domain") != std::string::npos) {
    return PreviewObjectKind::kDomain;
  }
  if (lower.find("package") != std::string::npos) {
    return PreviewObjectKind::kPackage;
  }
  if (lower.find("role") != std::string::npos) {
    return PreviewObjectKind::kRole;
  }
  if (lower.find("group") != std::string::npos) {
    return PreviewObjectKind::kGroup;
  }
  if (lower.find("user") != std::string::npos) {
    return PreviewObjectKind::kUser;
  }
  return PreviewObjectKind::kTable;
}

PreviewObjectKind InferSysVirtualObjectKind(const std::string& table_name) {
  const std::string lower = ToLowerCopy(table_name);
  if (lower == "domains") {
    return PreviewObjectKind::kDomain;
  }
  if (lower == "jobs" || lower == "job_runs" || lower == "job_dependencies") {
    return PreviewObjectKind::kJob;
  }
  if (lower == "job_schedules") {
    return PreviewObjectKind::kSchedule;
  }
  return PreviewObjectKind::kTable;
}

std::string SelectSchemaForAlias(const std::string& alias_name,
                                 const std::string& record_type,
                                 const std::unordered_set<std::string>& known_schemas) {
  auto choose_existing = [&](const std::string& preferred, const std::string& fallback) {
    if (known_schemas.contains(preferred)) {
      return preferred;
    }
    return fallback;
  };

  const std::string alias = ToLowerCopy(alias_name);
  const std::string record = ToLowerCopy(record_type);

  if (alias == "users") {
    return choose_existing("root.sys.security.users", "root.sys.security");
  }
  if (alias == "roles") {
    return choose_existing("root.sys.security.roles", "root.sys.security");
  }
  if (alias == "groups") {
    return choose_existing("root.sys.security.groups", "root.sys.security");
  }
  if (alias == "authkeys" || record.find("auth") != std::string::npos) {
    return choose_existing("root.sys.security.auth", "root.sys.security");
  }

  static const std::unordered_set<std::string> kSecurityAliases{
      "authkeys",      "permissions",      "users",         "roles",
      "groups",        "group_mappings",   "group_memberships",
      "role_memberships",                  "security_policy_epoch",
  };
  static const std::unordered_set<std::string> kJobsAliases{
      "jobs",
      "job_runs",
      "job_dependencies",
      "prepared_transactions",
  };
  static const std::unordered_set<std::string> kMonitorAliases{
      "sessions",
      "statistics",
      "audit_log",
      "dormant_transactions",
      "migration_history",
  };
  static const std::unordered_set<std::string> kConfigAliases{
      "charsets",
      "collations",
      "timezones",
      "domains",
  };
  static const std::unordered_set<std::string> kRemoteEmulationAliases{
      "emulated_dbs",
      "emulation_servers",
      "emulation_types",
  };
  static const std::unordered_set<std::string> kRemoteFdwAliases{
      "foreign_servers",
      "foreign_tables",
      "user_mappings",
  };

  if (kSecurityAliases.contains(alias)) {
    return "root.sys.security";
  }
  if (kJobsAliases.contains(alias)) {
    return "root.sys.jobs";
  }
  if (kMonitorAliases.contains(alias)) {
    return "root.sys.monitor";
  }
  if (kConfigAliases.contains(alias)) {
    return "root.sys.config";
  }
  if (kRemoteEmulationAliases.contains(alias)) {
    return "root.remote.emulation";
  }
  if (kRemoteFdwAliases.contains(alias)) {
    return "root.remote.fdw";
  }
  if (alias == "server_registry") {
    return "root.local.instances";
  }

  if (alias == "schemas" || alias == "tables" || alias == "columns" || alias == "indexes" ||
      alias == "constraints" || alias == "foreign_keys" || alias == "views" ||
      alias == "synonyms" || alias == "comments") {
    return "root.sys.information";
  }
  return "root.sys.system";
}

std::string SelectSchemaForSysQueryFunction(const std::string& query_function_name) {
  const std::string fn = ToLowerCopy(query_function_name);
  if (fn.find("job") != std::string::npos) {
    return "root.sys.jobs";
  }

  static const std::vector<std::string> kMonitorTokens{
      "session",   "transaction", "lock",         "statement",   "io",
      "cache",     "performance", "migration",    "replication", "shard",
      "plugin",    "prepared",    "servercapability",
  };
  for (const auto& token : kMonitorTokens) {
    if (fn.find(token) != std::string::npos) {
      return "root.sys.monitor";
    }
  }

  static const std::vector<std::string> kInformationTokens{
      "schema",     "table",   "column",    "index",   "constraint",
      "foreignkey", "primary", "type",      "domain",  "information",
  };
  for (const auto& token : kInformationTokens) {
    if (fn.find(token) != std::string::npos) {
      return "root.sys.information";
    }
  }

  return "root.sys.system";
}

std::string SelectSchemaForSysVirtualTable(const std::string& table_name) {
  static const std::unordered_set<std::string> kJobTables{
      "jobs",
      "job_runs",
      "job_dependencies",
  };
  static const std::unordered_set<std::string> kMonitorTables{
      "sessions",
      "context_variables",
      "transactions",
      "locks",
      "statements",
      "io_stats",
      "cache_stats",
      "buffer_pool_stats",
      "statement_cache",
      "performance",
      "migration_status",
      "migration_audit_summary",
      "replication_channel_status",
      "replication_conflict_queue",
      "replication_cursor_status",
      "shard_status",
      "shard_migrations",
      "plugin",
      "prepared_statement",
  };

  if (kJobTables.contains(table_name)) {
    return "root.sys.jobs";
  }
  if (table_name == "domains") {
    return "root.sys.config";
  }
  if (kMonitorTables.contains(table_name)) {
    return "root.sys.monitor";
  }
  return "root.sys.information";
}

std::vector<std::string> ExtractQuotedPathTokens(const std::string& source) {
  static const std::regex kPathTokenRegex(
      R"re("([A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+){1,})")re");
  std::vector<std::string> tokens;
  for (std::sregex_iterator it(source.begin(), source.end(), kPathTokenRegex), end; it != end;
       ++it) {
    tokens.push_back((*it)[1].str());
  }
  return tokens;
}

void EnsureSchemaPresent(ScratchbirdCatalogPreviewSnapshot* snapshot,
                         const std::string& schema_path) {
  if (!snapshot || schema_path.empty()) {
    return;
  }
  snapshot->schema_paths.push_back(schema_path);
}

void AddPreviewObject(ScratchbirdCatalogPreviewSnapshot* snapshot,
                      const std::string& schema_path, const std::string& name,
                      const PreviewObjectKind kind) {
  if (!snapshot || schema_path.empty() || name.empty()) {
    return;
  }

  EnsureSchemaPresent(snapshot, schema_path);
  snapshot->objects.push_back({schema_path, name, kind});
}

void AddPathTokenHints(ScratchbirdCatalogPreviewSnapshot* snapshot, const std::string& source) {
  if (!snapshot || source.empty()) {
    return;
  }

  const std::string server = snapshot->server_name.empty() ? "localhost" : snapshot->server_name;
  const std::string database =
      snapshot->database_name.empty() ? "default" : snapshot->database_name;

  for (const auto& token_raw : ExtractQuotedPathTokens(source)) {
    const std::string token = ToLowerCopy(token_raw);
    if (token == "users.public") {
      EnsureSchemaPresent(snapshot, "root.users.public");
      continue;
    }
    if (token == "remote.emulation.mysql.localhost.databases") {
      EnsureSchemaPresent(snapshot,
                          "root.remote.emulation.mysql.localhost.databases." + database);
      continue;
    }
    if (token == "remote.emulation.postgresql.localhost.databases") {
      EnsureSchemaPresent(snapshot,
                          "root.remote.emulation.postgresql.localhost.databases." + database);
      continue;
    }
    if (token == "remote.emulation.firebird") {
      EnsureSchemaPresent(snapshot,
                          "root.remote.emulation.firebird." + server + ".databases." + database);
      continue;
    }
    if (StartsWith(token, "root.")) {
      EnsureSchemaPresent(snapshot, token);
      continue;
    }
    if (StartsWith(token, "users.") || StartsWith(token, "sys.") ||
        StartsWith(token, "remote.") || StartsWith(token, "local.") ||
        StartsWith(token, "nosql.")) {
      if (StartsWith(token, "sys.")) {
        const std::vector<std::string> components = SplitPath(token);
        if (components.size() == 2) {
          const std::string& object_name = components[1];
          const std::string schema =
              (object_name == "table_stats" || object_name == "io_stats")
                  ? "root.sys.monitor"
                  : "root.sys.information";
          AddPreviewObject(snapshot, schema, object_name,
                           InferSysVirtualObjectKind(object_name));
          continue;
        }
        if (components.size() >= 3) {
          const std::string schema =
              "root.sys." + JoinPathComponents(components, 1, components.size() - 1);
          AddPreviewObject(snapshot, schema, components.back(),
                           InferObjectKind(components.back()));
          continue;
        }
      }

      const std::string normalized = NormalizeSchemaPath(token);
      if (!normalized.empty()) {
        EnsureSchemaPresent(snapshot, normalized);
      }
    }
  }
}

void AddFallbackTree(ScratchbirdCatalogPreviewSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }
  EnsureSchemaPresent(snapshot, "root");
  EnsureSchemaPresent(snapshot, "root.users");
  EnsureSchemaPresent(snapshot, "root.users.public");
  EnsureSchemaPresent(snapshot, "root.sys");
  EnsureSchemaPresent(snapshot, "root.sys.information");
  AddPreviewObject(snapshot, "root.users.public", "preview_fallback_table",
                   PreviewObjectKind::kTable);
}

void DeduplicateSnapshot(ScratchbirdCatalogPreviewSnapshot* snapshot) {
  if (!snapshot) {
    return;
  }

  std::set<std::string> unique_schemas;
  for (const auto& path : snapshot->schema_paths) {
    if (!path.empty()) {
      unique_schemas.insert(path);
    }
  }
  snapshot->schema_paths.assign(unique_schemas.begin(), unique_schemas.end());

  struct Key {
    std::string schema_path;
    std::string name;
    PreviewObjectKind kind;

    bool operator<(const Key& other) const {
      if (schema_path != other.schema_path) {
        return schema_path < other.schema_path;
      }
      if (name != other.name) {
        return name < other.name;
      }
      return static_cast<int>(kind) < static_cast<int>(other.kind);
    }
  };

  std::set<Key> unique_objects;
  for (const auto& object : snapshot->objects) {
    if (!object.schema_path.empty() && !object.name.empty()) {
      unique_objects.insert({object.schema_path, object.name, object.kind});
    }
  }

  snapshot->objects.clear();
  snapshot->objects.reserve(unique_objects.size());
  for (const auto& key : unique_objects) {
    snapshot->objects.push_back({key.schema_path, key.name, key.kind});
  }
}

}  // namespace

ScratchbirdCatalogPreviewSnapshot LoadScratchbirdCatalogPreview(
    const ScratchbirdRuntimeConfig& runtime_config) {
  return LoadScratchbirdCatalogPreviewFromDir({}, runtime_config);
}

ScratchbirdCatalogPreviewSnapshot LoadScratchbirdCatalogPreviewFromDir(
    const std::string& source_dir, const ScratchbirdRuntimeConfig& runtime_config) {
  ScratchbirdCatalogPreviewSnapshot snapshot;
  snapshot.source_dir = ResolveSourceDirectory(source_dir);
  snapshot.server_name = NormalizeServerName(runtime_config.host);
  snapshot.database_name = NormalizeDatabaseName(runtime_config.database);

  const std::filesystem::path root(snapshot.source_dir);
  const std::string catalog_source =
      ReadTextFile(root / "src" / "core" / "catalog_manager.cpp");
  const std::string sys_catalog_source =
      ReadTextFile(root / "src" / "catalog" / "sys_catalog.cpp");
  const std::string pg_adapter_source =
      ReadTextFile(root / "src" / "protocol" / "adapters" / "postgresql_adapter.cpp");
  const std::string mysql_adapter_source =
      ReadTextFile(root / "src" / "protocol" / "adapters" / "mysql_adapter.cpp");
  const std::string firebird_adapter_source =
      ReadTextFile(root / "src" / "protocol" / "adapters" / "firebird_adapter.cpp");
  const std::string pg_compiler_source =
      ReadTextFile(root / "src" / "sblr" / "postgresql_query_compiler.cpp");
  const std::string fb_compiler_source =
      ReadTextFile(root / "src" / "sblr" / "firebird_query_compiler.cpp");
  const std::string executor_source = ReadTextFile(root / "src" / "sblr" / "executor.cpp");

  const std::vector<std::string> schema_paths =
      ExtractBootstrapSchemaPaths(catalog_source);
  for (const auto& path : schema_paths) {
    EnsureSchemaPresent(&snapshot, path);
  }

  const std::unordered_set<std::string> known_schemas(snapshot.schema_paths.begin(),
                                                      snapshot.schema_paths.end());

  const auto alias_entries = ExtractAliasMapEntries(catalog_source);
  for (const auto& [alias_name, record_type] : alias_entries) {
    const std::string schema_path =
        SelectSchemaForAlias(alias_name, record_type, known_schemas);
    AddPreviewObject(&snapshot, schema_path, alias_name, InferObjectKind(alias_name));
    if (ToLowerCopy(alias_name) == "procedures") {
      AddPreviewObject(&snapshot, schema_path, "functions", PreviewObjectKind::kFunction);
    }
  }

  struct SyntheticPlacement {
    const char* object_name;
    const char* marker;
    PreviewObjectKind kind;
    const char* schema_path;
  };
  static constexpr SyntheticPlacement kSyntheticPlacements[] = {
      {"job_types", "struct JobTypeCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"job_type_params", "struct JobTypeParamCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"job_params", "struct JobParamCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"job_schedules", "struct JobScheduleCatalogRecord", PreviewObjectKind::kSchedule,
       "root.sys.jobs"},
      {"job_type_policies", "struct JobTypePolicyCatalogRecord", PreviewObjectKind::kJob,
       "root.sys.jobs"},
      {"package_members", "struct PackageMemberRecord", PreviewObjectKind::kPackage,
       "root.sys.system"},
      {"domain_param_keys", "struct DomainParamKeyRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_parameters", "struct DomainParameterRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_constraints", "struct DomainConstraintCatalogRecord",
       PreviewObjectKind::kDomain, "root.sys.config"},
      {"domain_security", "struct DomainSecurityCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
      {"domain_validation", "struct DomainValidationCatalogRecord",
       PreviewObjectKind::kDomain, "root.sys.config"},
      {"domain_integrity", "struct DomainIntegrityCatalogRecord", PreviewObjectKind::kDomain,
       "root.sys.config"},
  };
  for (const auto& placement : kSyntheticPlacements) {
    if (catalog_source.find(placement.marker) == std::string::npos) {
      continue;
    }
    AddPreviewObject(&snapshot, placement.schema_path, placement.object_name, placement.kind);
  }

  const auto dispatch_by_table = ExtractSysDispatchQueryFunctions(sys_catalog_source);
  for (const auto& table_name : ExtractSysVirtualTableNames(sys_catalog_source)) {
    const auto dispatch_it = dispatch_by_table.find(table_name);
    const std::string schema_path =
        (dispatch_it != dispatch_by_table.end())
            ? SelectSchemaForSysQueryFunction(dispatch_it->second)
            : SelectSchemaForSysVirtualTable(table_name);
    AddPreviewObject(&snapshot, schema_path, table_name, InferSysVirtualObjectKind(table_name));
  }

  AddPathTokenHints(&snapshot, catalog_source);
  AddPathTokenHints(&snapshot, sys_catalog_source);
  AddPathTokenHints(&snapshot, pg_adapter_source);
  AddPathTokenHints(&snapshot, mysql_adapter_source);
  AddPathTokenHints(&snapshot, firebird_adapter_source);
  AddPathTokenHints(&snapshot, pg_compiler_source);
  AddPathTokenHints(&snapshot, fb_compiler_source);
  AddPathTokenHints(&snapshot, executor_source);

  snapshot.table_metadata = BuildSourcePreviewTableMetadata(
      catalog_source, sys_catalog_source, alias_entries, known_schemas);
  snapshot.object_metadata = BuildSourcePreviewObjectMetadata(
      catalog_source, sys_catalog_source, alias_entries, known_schemas);

  std::string metadata_error;
  const std::vector<PreviewTableMetadata> user_metadata =
      LoadPreviewTableMetadata(DefaultPreviewMetadataPath(), &metadata_error);
  for (const auto& table : user_metadata) {
    UpsertPreviewTableMetadata(&snapshot.table_metadata, table);
  }

  std::string object_metadata_error;
  const std::vector<PreviewObjectMetadata> user_object_metadata =
      LoadPreviewObjectMetadata(DefaultPreviewObjectMetadataPath(),
                                &object_metadata_error);
  for (const auto& object : user_object_metadata) {
    UpsertObjectMetadata(&snapshot.object_metadata, object);
  }

  for (const auto& table : snapshot.table_metadata) {
    if (table.schema_path.empty() || table.table_name.empty()) {
      continue;
    }
    AddPreviewObject(&snapshot, table.schema_path, table.table_name, PreviewObjectKind::kTable);
  }

  DeduplicateSnapshot(&snapshot);

  snapshot.source_driven =
      !schema_paths.empty() && !catalog_source.empty() && !sys_catalog_source.empty();

  if (!snapshot.source_driven && snapshot.schema_paths.empty()) {
    AddFallbackTree(&snapshot);
    DeduplicateSnapshot(&snapshot);
  }

  return snapshot;
}

}  // namespace scratchrobin::backend
