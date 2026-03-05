/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "backend/preview_metadata_store.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace scratchrobin::backend {

namespace {

std::string ToLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string MakeTableKey(const std::string& schema_path, const std::string& table_name) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(table_name);
}

std::string EscapeField(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (const char c : value) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '\t':
        out += "\\t";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

std::string UnescapeField(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] != '\\' || i + 1 >= value.size()) {
      out.push_back(value[i]);
      continue;
    }

    ++i;
    switch (value[i]) {
      case '\\':
        out.push_back('\\');
        break;
      case 't':
        out.push_back('\t');
        break;
      case 'n':
        out.push_back('\n');
        break;
      case 'r':
        out.push_back('\r');
        break;
      default:
        out.push_back(value[i]);
        break;
    }
  }
  return out;
}

std::vector<std::string> SplitTab(const std::string& line) {
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (start <= line.size()) {
    const std::size_t pos = line.find('\t', start);
    if (pos == std::string::npos) {
      parts.push_back(line.substr(start));
      break;
    }
    parts.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }
  return parts;
}

std::filesystem::path ResolvePath(const std::string& file_path) {
  if (!file_path.empty()) {
    return std::filesystem::path(file_path);
  }
  return std::filesystem::path(DefaultPreviewMetadataPath());
}

bool ParseBoolField(std::string value) {
  value = ToLowerCopy(value);
  return value == "1" || value == "true" || value == "yes" || value == "y";
}

}  // namespace

std::string DefaultPreviewMetadataPath() {
#ifdef ROBIN_MIGRATE_DEFAULT_PREVIEW_METADATA_PATH
  return ROBIN_MIGRATE_DEFAULT_PREVIEW_METADATA_PATH;
#else
  return "./docs/preview/system_object_metadata.tsv";
#endif
}

std::vector<PreviewTableMetadata> LoadPreviewTableMetadata(
    const std::string& file_path, std::string* error_out) {
  if (error_out) {
    error_out->clear();
  }

  std::vector<PreviewTableMetadata> tables;
  const std::filesystem::path path = ResolvePath(file_path);
  std::ifstream in(path, std::ios::in | std::ios::binary);
  if (!in.good()) {
    return tables;
  }

  std::unordered_map<std::string, std::size_t> table_index;
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const std::vector<std::string> fields = SplitTab(line);
    if (fields.empty()) {
      continue;
    }

    const std::string record_type = fields[0];
    if (record_type == "TABLE") {
      if (fields.size() < 4) {
        if (error_out) {
          *error_out = "Invalid TABLE record at line " + std::to_string(line_no);
        }
        continue;
      }

      PreviewTableMetadata table;
      table.schema_path = UnescapeField(fields[1]);
      table.table_name = UnescapeField(fields[2]);
      table.description = UnescapeField(fields[3]);
      if (table.schema_path.empty() || table.table_name.empty()) {
        continue;
      }
      const std::string key = MakeTableKey(table.schema_path, table.table_name);
      const auto it = table_index.find(key);
      if (it != table_index.end()) {
        tables[it->second].description = table.description;
      } else {
        table_index.emplace(key, tables.size());
        tables.push_back(std::move(table));
      }
      continue;
    }

    if (record_type == "COLUMN") {
      if (fields.size() < 9) {
        if (error_out) {
          *error_out = "Invalid COLUMN record at line " + std::to_string(line_no);
        }
        continue;
      }

      const std::string schema_path = UnescapeField(fields[1]);
      const std::string table_name = UnescapeField(fields[2]);
      if (schema_path.empty() || table_name.empty()) {
        continue;
      }
      const std::string key = MakeTableKey(schema_path, table_name);
      std::size_t index = 0;
      const auto it = table_index.find(key);
      if (it == table_index.end()) {
        PreviewTableMetadata table;
        table.schema_path = schema_path;
        table.table_name = table_name;
        table_index.emplace(key, tables.size());
        tables.push_back(std::move(table));
        index = tables.size() - 1;
      } else {
        index = it->second;
      }

      PreviewColumnMetadata col;
      col.column_name = UnescapeField(fields[4]);
      col.data_type = UnescapeField(fields[5]);
      col.nullable = ParseBoolField(fields[6]);
      col.default_value = UnescapeField(fields[7]);
      col.notes = UnescapeField(fields[8]);
      if (fields.size() >= 10) {
        col.domain_name = UnescapeField(fields[9]);
      }
      if (fields.size() >= 11) {
        col.is_primary_key = ParseBoolField(fields[10]);
      }
      if (fields.size() >= 12) {
        col.is_foreign_key = ParseBoolField(fields[11]);
      }
      if (fields.size() >= 13) {
        col.references_schema_path = UnescapeField(fields[12]);
      }
      if (fields.size() >= 14) {
        col.references_table_name = UnescapeField(fields[13]);
      }
      if (fields.size() >= 15) {
        col.references_column_name = UnescapeField(fields[14]);
      }

      if (!col.column_name.empty()) {
        tables[index].columns.push_back(std::move(col));
      }
      continue;
    }

    if (record_type == "INDEX") {
      if (fields.size() < 9) {
        if (error_out) {
          *error_out = "Invalid INDEX record at line " + std::to_string(line_no);
        }
        continue;
      }

      const std::string schema_path = UnescapeField(fields[1]);
      const std::string table_name = UnescapeField(fields[2]);
      if (schema_path.empty() || table_name.empty()) {
        continue;
      }
      const std::string key = MakeTableKey(schema_path, table_name);
      std::size_t index = 0;
      const auto it = table_index.find(key);
      if (it == table_index.end()) {
        PreviewTableMetadata table;
        table.schema_path = schema_path;
        table.table_name = table_name;
        table_index.emplace(key, tables.size());
        tables.push_back(std::move(table));
        index = tables.size() - 1;
      } else {
        index = it->second;
      }

      PreviewIndexMetadata idx;
      idx.index_name = UnescapeField(fields[4]);
      idx.unique = ParseBoolField(fields[5]);
      idx.method = UnescapeField(fields[6]);
      idx.target = UnescapeField(fields[7]);
      idx.notes = UnescapeField(fields[8]);
      if (!idx.index_name.empty()) {
        tables[index].indexes.push_back(std::move(idx));
      }
      continue;
    }

    if (record_type == "TRIGGER") {
      if (fields.size() < 8) {
        if (error_out) {
          *error_out = "Invalid TRIGGER record at line " + std::to_string(line_no);
        }
        continue;
      }

      const std::string schema_path = UnescapeField(fields[1]);
      const std::string table_name = UnescapeField(fields[2]);
      if (schema_path.empty() || table_name.empty()) {
        continue;
      }
      const std::string key = MakeTableKey(schema_path, table_name);
      std::size_t index = 0;
      const auto it = table_index.find(key);
      if (it == table_index.end()) {
        PreviewTableMetadata table;
        table.schema_path = schema_path;
        table.table_name = table_name;
        table_index.emplace(key, tables.size());
        tables.push_back(std::move(table));
        index = tables.size() - 1;
      } else {
        index = it->second;
      }

      PreviewTriggerMetadata trig;
      trig.trigger_name = UnescapeField(fields[4]);
      trig.timing = UnescapeField(fields[5]);
      trig.event = UnescapeField(fields[6]);
      trig.notes = UnescapeField(fields[7]);
      if (!trig.trigger_name.empty()) {
        tables[index].triggers.push_back(std::move(trig));
      }
    }
  }

  return tables;
}

bool SavePreviewTableMetadata(const std::string& file_path,
                              const std::vector<PreviewTableMetadata>& tables,
                              std::string* error_out) {
  if (error_out) {
    error_out->clear();
  }

  const std::filesystem::path path = ResolvePath(file_path);
  const std::filesystem::path dir = path.parent_path();
  if (!dir.empty()) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec && error_out) {
      *error_out = "Failed creating preview metadata directory: " + ec.message();
      return false;
    }
  }

  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.good()) {
    if (error_out) {
      *error_out = "Failed opening preview metadata file for write: " + path.string();
    }
    return false;
  }

  std::vector<PreviewTableMetadata> sorted = tables;
  std::sort(sorted.begin(), sorted.end(),
            [](const PreviewTableMetadata& lhs, const PreviewTableMetadata& rhs) {
              if (lhs.schema_path != rhs.schema_path) {
                return lhs.schema_path < rhs.schema_path;
              }
              return lhs.table_name < rhs.table_name;
            });

  out << "# robin-migrate preview metadata v2\n";
  out << "# TABLE\\t<schema_path>\\t<table_name>\\t<description>\n";
  out << "# COLUMN\\t<schema_path>\\t<table_name>\\t<ordinal>\\t<column_name>\\t<data_type>"
         "\\t<nullable:1|0>\\t<default_value>\\t<notes>\\t<domain_name>\\t<pk:1|0>"
         "\\t<fk:1|0>\\t<ref_schema>\\t<ref_table>\\t<ref_column>\n";
  out << "# INDEX\\t<schema_path>\\t<table_name>\\t<ordinal>\\t<index_name>"
         "\\t<unique:1|0>\\t<method>\\t<target>\\t<notes>\n";
  out << "# TRIGGER\\t<schema_path>\\t<table_name>\\t<ordinal>\\t<trigger_name>"
         "\\t<timing>\\t<event>\\t<notes>\n";

  for (const auto& table : sorted) {
    if (table.schema_path.empty() || table.table_name.empty()) {
      continue;
    }

    out << "TABLE"
        << '\t' << EscapeField(table.schema_path)
        << '\t' << EscapeField(table.table_name)
        << '\t' << EscapeField(table.description) << '\n';

    for (std::size_t i = 0; i < table.columns.size(); ++i) {
      const auto& col = table.columns[i];
      if (col.column_name.empty()) {
        continue;
      }
      out << "COLUMN"
          << '\t' << EscapeField(table.schema_path)
          << '\t' << EscapeField(table.table_name)
          << '\t' << (i + 1)
          << '\t' << EscapeField(col.column_name)
          << '\t' << EscapeField(col.data_type)
          << '\t' << (col.nullable ? "1" : "0")
          << '\t' << EscapeField(col.default_value)
          << '\t' << EscapeField(col.notes)
          << '\t' << EscapeField(col.domain_name)
          << '\t' << (col.is_primary_key ? "1" : "0")
          << '\t' << (col.is_foreign_key ? "1" : "0")
          << '\t' << EscapeField(col.references_schema_path)
          << '\t' << EscapeField(col.references_table_name)
          << '\t' << EscapeField(col.references_column_name) << '\n';
    }

    for (std::size_t i = 0; i < table.indexes.size(); ++i) {
      const auto& idx = table.indexes[i];
      if (idx.index_name.empty()) {
        continue;
      }
      out << "INDEX"
          << '\t' << EscapeField(table.schema_path)
          << '\t' << EscapeField(table.table_name)
          << '\t' << (i + 1)
          << '\t' << EscapeField(idx.index_name)
          << '\t' << (idx.unique ? "1" : "0")
          << '\t' << EscapeField(idx.method)
          << '\t' << EscapeField(idx.target)
          << '\t' << EscapeField(idx.notes) << '\n';
    }

    for (std::size_t i = 0; i < table.triggers.size(); ++i) {
      const auto& trigger = table.triggers[i];
      if (trigger.trigger_name.empty()) {
        continue;
      }
      out << "TRIGGER"
          << '\t' << EscapeField(table.schema_path)
          << '\t' << EscapeField(table.table_name)
          << '\t' << (i + 1)
          << '\t' << EscapeField(trigger.trigger_name)
          << '\t' << EscapeField(trigger.timing)
          << '\t' << EscapeField(trigger.event)
          << '\t' << EscapeField(trigger.notes) << '\n';
    }
  }

  if (!out.good()) {
    if (error_out) {
      *error_out = "Failed writing preview metadata file: " + path.string();
    }
    return false;
  }

  return true;
}

void UpsertPreviewTableMetadata(std::vector<PreviewTableMetadata>* tables,
                                const PreviewTableMetadata& metadata) {
  if (!tables || metadata.schema_path.empty() || metadata.table_name.empty()) {
    return;
  }
  const std::string key = MakeTableKey(metadata.schema_path, metadata.table_name);
  for (auto& existing : *tables) {
    if (MakeTableKey(existing.schema_path, existing.table_name) == key) {
      existing = metadata;
      return;
    }
  }
  tables->push_back(metadata);
}

bool RemovePreviewTableMetadata(std::vector<PreviewTableMetadata>* tables,
                                const std::string& schema_path,
                                const std::string& table_name) {
  if (!tables) {
    return false;
  }
  const std::string key = MakeTableKey(schema_path, table_name);
  const auto it = std::remove_if(tables->begin(), tables->end(),
                                 [&](const PreviewTableMetadata& item) {
                                   return MakeTableKey(item.schema_path, item.table_name) ==
                                          key;
                                 });
  if (it == tables->end()) {
    return false;
  }
  tables->erase(it, tables->end());
  return true;
}

}  // namespace scratchrobin::backend
