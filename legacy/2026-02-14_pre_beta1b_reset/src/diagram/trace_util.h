/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TRACE_UTIL_H
#define SCRATCHROBIN_TRACE_UTIL_H

#include <string>

namespace scratchrobin {

struct TraceTarget {
    std::string diagram_path;
    std::string node_name;
};

TraceTarget ParseTraceRef(const std::string& ref);

} // namespace scratchrobin

#endif // SCRATCHROBIN_TRACE_UTIL_H
