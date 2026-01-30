/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_RESULT_EXPORTER_H
#define SCRATCHROBIN_RESULT_EXPORTER_H

#include <cstddef>
#include <string>

#include "core/query_types.h"

namespace scratchrobin {

struct ExportOptions {
    bool includeHeaders = true;
    size_t maxBinaryBytes = 0;
    bool includeBinarySize = false;
};

bool ExportResultToCsv(const QueryResult& result,
                       const std::string& path,
                       std::string* error,
                       const ExportOptions& options = {});

bool ExportResultToJson(const QueryResult& result,
                        const std::string& path,
                        std::string* error,
                        const ExportOptions& options = {});

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESULT_EXPORTER_H
