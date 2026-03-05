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

namespace scratchrobin::backend {

struct PreviewColumnMetadata {
  std::string column_name;
  std::string data_type;
  bool nullable{true};
  std::string default_value;
  std::string notes;
  std::string domain_name;
  bool is_primary_key{false};
  bool is_foreign_key{false};
  std::string references_schema_path;
  std::string references_table_name;
  std::string references_column_name;
};

struct PreviewIndexMetadata {
  std::string index_name;
  bool unique{false};
  std::string method;
  std::string target;
  std::string notes;
};

struct PreviewTriggerMetadata {
  std::string trigger_name;
  std::string timing;
  std::string event;
  std::string notes;
};

struct PreviewTableMetadata {
  std::string schema_path;
  std::string table_name;
  std::string description;
  std::vector<PreviewColumnMetadata> columns;
  std::vector<PreviewIndexMetadata> indexes;
  std::vector<PreviewTriggerMetadata> triggers;
};

std::string DefaultPreviewMetadataPath();

std::vector<PreviewTableMetadata> LoadPreviewTableMetadata(
    const std::string& file_path, std::string* error_out = nullptr);

bool SavePreviewTableMetadata(const std::string& file_path,
                              const std::vector<PreviewTableMetadata>& tables,
                              std::string* error_out = nullptr);

void UpsertPreviewTableMetadata(std::vector<PreviewTableMetadata>* tables,
                                const PreviewTableMetadata& metadata);

bool RemovePreviewTableMetadata(std::vector<PreviewTableMetadata>* tables,
                                const std::string& schema_path,
                                const std::string& table_name);

}  // namespace scratchrobin::backend
