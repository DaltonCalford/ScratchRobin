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

struct PreviewObjectPropertyMetadata {
  std::string property_name;
  std::string property_type;
  std::string property_value;
  std::string notes;
};

struct PreviewObjectMetadata {
  std::string schema_path;
  std::string object_name;
  std::string object_kind;
  std::string description;
  std::vector<PreviewObjectPropertyMetadata> properties;
};

std::string DefaultPreviewObjectMetadataPath();

std::vector<PreviewObjectMetadata> LoadPreviewObjectMetadata(
    const std::string& file_path, std::string* error_out = nullptr);

bool SavePreviewObjectMetadata(const std::string& file_path,
                               const std::vector<PreviewObjectMetadata>& objects,
                               std::string* error_out = nullptr);

void UpsertPreviewObjectMetadata(std::vector<PreviewObjectMetadata>* objects,
                                 const PreviewObjectMetadata& metadata);

bool RemovePreviewObjectMetadata(std::vector<PreviewObjectMetadata>* objects,
                                 const std::string& schema_path,
                                 const std::string& object_name,
                                 const std::string& object_kind);

}  // namespace scratchrobin::backend
