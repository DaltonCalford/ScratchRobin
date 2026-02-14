/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "trace_util.h"

namespace scratchrobin {

TraceTarget ParseTraceRef(const std::string& ref) {
    TraceTarget target;
    if (ref.empty()) {
        return target;
    }
    std::string value = ref;
    auto hash_pos = value.find('#');
    if (hash_pos != std::string::npos) {
        target.diagram_path = value.substr(0, hash_pos);
        value = value.substr(hash_pos + 1);
    }
    auto colon_pos = value.find(':');
    if (colon_pos != std::string::npos) {
        std::string prefix = value.substr(0, colon_pos);
        if (prefix == "diagram" || prefix == "erd") {
            value = value.substr(colon_pos + 1);
        }
    }
    target.node_name = value;
    return target;
}

} // namespace scratchrobin
