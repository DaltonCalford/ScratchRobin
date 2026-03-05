/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "backend/preview_object_metadata_store.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
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

std::string MakeObjectKey(const std::string& schema_path, const std::string& object_name,
                          const std::string& object_kind) {
  return ToLowerCopy(schema_path) + "|" + ToLowerCopy(object_name) + "|" +
         ToLowerCopy(object_kind);
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
  return std::filesystem::path(DefaultPreviewObjectMetadataPath());
}

}  // namespace

std::string DefaultPreviewObjectMetadataPath() {
#ifdef ROBIN_MIGRATE_DEFAULT_PREVIEW_OBJECT_METADATA_PATH
  return ROBIN_MIGRATE_DEFAULT_PREVIEW_OBJECT_METADATA_PATH;
#else
  return "./docs/preview/system_sql_object_metadata.tsv";
#endif
}

std::vector<PreviewObjectMetadata> LoadPreviewObjectMetadata(
    const std::string& file_path, std::string* error_out) {
  if (error_out) {
    error_out->clear();
  }

  std::vector<PreviewObjectMetadata> objects;
  const std::filesystem::path path = ResolvePath(file_path);
  std::ifstream in(path, std::ios::in | std::ios::binary);
  if (!in.good()) {
    return objects;
  }

  std::unordered_map<std::string, std::size_t> object_index;
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
    if (record_type == "OBJECT") {
      if (fields.size() < 5) {
        if (error_out) {
          *error_out = "Invalid OBJECT record at line " + std::to_string(line_no);
        }
        continue;
      }

      PreviewObjectMetadata object;
      object.schema_path = UnescapeField(fields[1]);
      object.object_name = UnescapeField(fields[2]);
      object.object_kind = UnescapeField(fields[3]);
      object.description = UnescapeField(fields[4]);
      if (object.schema_path.empty() || object.object_name.empty() ||
          object.object_kind.empty()) {
        continue;
      }

      const std::string key =
          MakeObjectKey(object.schema_path, object.object_name, object.object_kind);
      const auto it = object_index.find(key);
      if (it != object_index.end()) {
        objects[it->second].description = object.description;
      } else {
        object_index.emplace(key, objects.size());
        objects.push_back(std::move(object));
      }
      continue;
    }

    if (record_type == "PROPERTY") {
      if (fields.size() < 8) {
        if (error_out) {
          *error_out = "Invalid PROPERTY record at line " + std::to_string(line_no);
        }
        continue;
      }

      const std::string schema_path = UnescapeField(fields[1]);
      const std::string object_name = UnescapeField(fields[2]);
      const std::string object_kind = UnescapeField(fields[3]);
      if (schema_path.empty() || object_name.empty() || object_kind.empty()) {
        continue;
      }

      const std::string key = MakeObjectKey(schema_path, object_name, object_kind);
      std::size_t index = 0;
      const auto it = object_index.find(key);
      if (it == object_index.end()) {
        PreviewObjectMetadata object;
        object.schema_path = schema_path;
        object.object_name = object_name;
        object.object_kind = object_kind;
        object_index.emplace(key, objects.size());
        objects.push_back(std::move(object));
        index = objects.size() - 1;
      } else {
        index = it->second;
      }

      PreviewObjectPropertyMetadata property;
      property.property_name = UnescapeField(fields[5]);
      property.property_type = UnescapeField(fields[6]);
      if (fields.size() >= 9) {
        property.property_value = UnescapeField(fields[7]);
        property.notes = UnescapeField(fields[8]);
      } else {
        property.notes = UnescapeField(fields[7]);
      }
      if (!property.property_name.empty()) {
        objects[index].properties.push_back(std::move(property));
      }
    }
  }

  return objects;
}

bool SavePreviewObjectMetadata(const std::string& file_path,
                               const std::vector<PreviewObjectMetadata>& objects,
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
      *error_out = "Failed creating object metadata directory: " + ec.message();
      return false;
    }
  }

  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  if (!out.good()) {
    if (error_out) {
      *error_out = "Failed opening object metadata file for write: " + path.string();
    }
    return false;
  }

  std::vector<PreviewObjectMetadata> sorted = objects;
  std::sort(sorted.begin(), sorted.end(),
            [](const PreviewObjectMetadata& lhs, const PreviewObjectMetadata& rhs) {
              if (lhs.schema_path != rhs.schema_path) {
                return lhs.schema_path < rhs.schema_path;
              }
              if (lhs.object_name != rhs.object_name) {
                return lhs.object_name < rhs.object_name;
              }
              return lhs.object_kind < rhs.object_kind;
            });

  out << "# robin-migrate sql object metadata v1\n";
  out << "# OBJECT\\t<schema_path>\\t<object_name>\\t<object_kind>\\t<description>\n";
  out << "# PROPERTY\\t<schema_path>\\t<object_name>\\t<object_kind>\\t<ordinal>"
         "\\t<property_name>\\t<property_type>\\t<property_value>\\t<notes>\n";

  for (const auto& object : sorted) {
    if (object.schema_path.empty() || object.object_name.empty() ||
        object.object_kind.empty()) {
      continue;
    }

    out << "OBJECT"
        << '\t' << EscapeField(object.schema_path)
        << '\t' << EscapeField(object.object_name)
        << '\t' << EscapeField(object.object_kind)
        << '\t' << EscapeField(object.description) << '\n';

    for (std::size_t i = 0; i < object.properties.size(); ++i) {
      const auto& property = object.properties[i];
      if (property.property_name.empty()) {
        continue;
      }
      out << "PROPERTY"
          << '\t' << EscapeField(object.schema_path)
          << '\t' << EscapeField(object.object_name)
          << '\t' << EscapeField(object.object_kind)
          << '\t' << (i + 1)
          << '\t' << EscapeField(property.property_name)
          << '\t' << EscapeField(property.property_type)
          << '\t' << EscapeField(property.property_value)
          << '\t' << EscapeField(property.notes) << '\n';
    }
  }

  if (!out.good()) {
    if (error_out) {
      *error_out = "Failed writing object metadata file: " + path.string();
    }
    return false;
  }

  return true;
}

void UpsertPreviewObjectMetadata(std::vector<PreviewObjectMetadata>* objects,
                                 const PreviewObjectMetadata& metadata) {
  if (!objects || metadata.schema_path.empty() || metadata.object_name.empty() ||
      metadata.object_kind.empty()) {
    return;
  }

  const std::string key =
      MakeObjectKey(metadata.schema_path, metadata.object_name, metadata.object_kind);
  for (auto& existing : *objects) {
    if (MakeObjectKey(existing.schema_path, existing.object_name, existing.object_kind) ==
        key) {
      existing = metadata;
      return;
    }
  }
  objects->push_back(metadata);
}

bool RemovePreviewObjectMetadata(std::vector<PreviewObjectMetadata>* objects,
                                 const std::string& schema_path,
                                 const std::string& object_name,
                                 const std::string& object_kind) {
  if (!objects) {
    return false;
  }

  const std::string key = MakeObjectKey(schema_path, object_name, object_kind);
  const auto it = std::remove_if(objects->begin(), objects->end(),
                                 [&](const PreviewObjectMetadata& item) {
                                   return MakeObjectKey(item.schema_path, item.object_name,
                                                        item.object_kind) == key;
                                 });
  if (it == objects->end()) {
    return false;
  }
  objects->erase(it, objects->end());
  return true;
}

}  // namespace scratchrobin::backend
