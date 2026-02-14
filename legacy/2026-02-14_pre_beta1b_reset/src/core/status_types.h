/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_STATUS_TYPES_H
#define SCRATCHROBIN_STATUS_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace scratchrobin {

enum class StatusRequestKind : uint8_t {
    ServerInfo,
    ConnectionInfo,
    DatabaseInfo,
    Statistics
};

inline const char* StatusRequestKindToString(StatusRequestKind kind) {
    switch (kind) {
        case StatusRequestKind::ServerInfo: return "Server Info";
        case StatusRequestKind::ConnectionInfo: return "Connection Info";
        case StatusRequestKind::DatabaseInfo: return "Database Info";
        case StatusRequestKind::Statistics: return "Statistics";
    }
    return "Unknown";
}

struct StatusEntry {
    std::string key;
    std::string value;
};

struct StatusSnapshot {
    StatusRequestKind kind = StatusRequestKind::ServerInfo;
    std::vector<StatusEntry> entries;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STATUS_TYPES_H
