/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATA_VIEW_VALIDATION_H
#define SCRATCHROBIN_DATA_VIEW_VALIDATION_H

#include <string>

namespace scratchrobin {

bool ValidateDataViewJson(const std::string& payload, std::string* error);

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_VIEW_VALIDATION_H
