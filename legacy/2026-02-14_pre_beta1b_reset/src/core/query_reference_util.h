/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#ifndef SCRATCHROBIN_QUERY_REFERENCE_UTIL_H
#define SCRATCHROBIN_QUERY_REFERENCE_UTIL_H

#include <string>
#include <vector>

namespace scratchrobin {

struct QueryReferenceResult {
    bool parsed = false;
    std::vector<std::string> identifiers;
};

QueryReferenceResult ExtractQueryReferences(const std::string& query);
bool QueryReferencesObject(const std::string& query,
                           const std::string& schema,
                           const std::string& name);

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_REFERENCE_UTIL_H
