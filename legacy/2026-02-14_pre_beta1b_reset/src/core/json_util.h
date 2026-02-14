/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_JSON_UTIL_H
#define SCRATCHROBIN_JSON_UTIL_H

#include <string>
#include <vector>

namespace scratchrobin {

bool UpdateJsonObjectBoolField(const std::string& json, const std::string& key,
                               bool value, std::string* out_json);
bool UpdateJsonObjectStringField(const std::string& json, const std::string& key,
                                 const std::string& value, std::string* out_json);
bool UpdateJsonObjectNumberField(const std::string& json, const std::string& key,
                                 double value, std::string* out_json);
bool UpdateJsonObjectStringArrayField(const std::string& json, const std::string& key,
                                      const std::vector<std::string>& values,
                                      std::string* out_json);

} // namespace scratchrobin

#endif // SCRATCHROBIN_JSON_UTIL_H
