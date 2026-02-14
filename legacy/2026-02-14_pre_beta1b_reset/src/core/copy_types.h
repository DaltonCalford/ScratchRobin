/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_COPY_TYPES_H
#define SCRATCHROBIN_COPY_TYPES_H

#include <cstdint>
#include <string>

namespace scratchrobin {

enum class CopyMode {
    In,
    Out,
    Both
};

enum class CopyDataSource {
    None,
    File,
    Clipboard,
    Stream
};

struct CopyOptions {
    CopyMode mode = CopyMode::Out;
    CopyDataSource inputSource = CopyDataSource::None;
    CopyDataSource outputSource = CopyDataSource::None;

    std::string sql;
    std::string inputPath;
    std::string outputPath;
    std::string clipboardPayload;

    bool preferBinary = true;
    bool includeHeaders = true;

    uint32_t copyWindowBytes = 0;
    uint32_t copyChunkBytes = 0;
};

struct CopyResult {
    int64_t rowsProcessed = 0;
    double elapsedMs = 0.0;
    std::string commandTag;
    std::string message;
    std::string outputPayload;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_COPY_TYPES_H
