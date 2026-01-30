/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_STATEMENT_SPLITTER_H
#define SCRATCHROBIN_STATEMENT_SPLITTER_H

#include <string>
#include <vector>

namespace scratchrobin {

class StatementSplitter {
public:
    struct SplitResult {
        std::vector<std::string> statements;
        std::string delimiter = ";";
    };

    SplitResult Split(const std::string& input) const;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STATEMENT_SPLITTER_H
