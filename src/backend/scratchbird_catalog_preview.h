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

#include "backend/preview_object_metadata_store.h"
#include "backend/preview_metadata_store.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

enum class PreviewObjectKind {
  kTable = 0,
  kView = 1,
  kProcedure = 2,
  kFunction = 3,
  kSequence = 4,
  kTrigger = 5,
  kDomain = 6,
  kPackage = 7,
  kJob = 8,
  kSchedule = 9,
  kUser = 10,
  kRole = 11,
  kGroup = 12,
  kSchema = 13,
  kColumn = 14,
  kIndex = 15,
  kConstraint = 16,
  kCompositeType = 17,
  kTablespace = 18,
  kDatabase = 19,
  kEmulationType = 20,
  kEmulationServer = 21,
  kEmulatedDatabase = 22,
  kCharset = 23,
  kCollation = 24,
  kTimezone = 25,
  kUdr = 26,
  kUdrEngine = 27,
  kUdrModule = 28,
  kException = 29,
  kComment = 30,
  kDependency = 31,
  kPermission = 32,
  kStatistic = 33,
  kExtension = 34,
  kForeignServer = 35,
  kForeignTable = 36,
  kUserMapping = 37,
  kServerRegistry = 38,
  kCluster = 39,
  kSynonym = 40,
  kPolicy = 41,
  kObjectDefinition = 42,
  kPreparedTransaction = 43,
  kAuditLog = 44,
  kMigrationHistory = 45,
  kOther = 46,
};

struct PreviewObjectPlacement {
  std::string schema_path;
  std::string name;
  PreviewObjectKind kind{PreviewObjectKind::kOther};
};

struct ScratchbirdCatalogPreviewSnapshot {
  std::string source_dir;
  std::string server_name;
  std::string database_name;
  bool source_driven{false};
  std::vector<std::string> schema_paths;
  std::vector<PreviewObjectPlacement> objects;
  std::vector<PreviewTableMetadata> table_metadata;
  std::vector<PreviewObjectMetadata> object_metadata;
};

ScratchbirdCatalogPreviewSnapshot LoadScratchbirdCatalogPreview(
    const ScratchbirdRuntimeConfig& runtime_config);

ScratchbirdCatalogPreviewSnapshot LoadScratchbirdCatalogPreviewFromDir(
    const std::string& source_dir, const ScratchbirdRuntimeConfig& runtime_config);

}  // namespace scratchrobin::backend
