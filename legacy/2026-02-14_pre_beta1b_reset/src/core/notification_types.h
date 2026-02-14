/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_NOTIFICATION_TYPES_H
#define SCRATCHROBIN_NOTIFICATION_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace scratchrobin {

struct NotificationEvent {
    uint32_t processId = 0;
    std::string channel;
    std::vector<uint8_t> payload;
    uint8_t changeType = 0;
    uint64_t rowId = 0;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_NOTIFICATION_TYPES_H
