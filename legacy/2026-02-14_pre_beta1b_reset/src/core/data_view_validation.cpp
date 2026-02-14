/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "data_view_validation.h"

#include "core/simple_json.h"

namespace scratchrobin {

bool ValidateDataViewJson(const std::string& payload, std::string* error) {
    JsonParser parser(payload);
    JsonValue root;
    std::string parse_error;
    if (!parser.Parse(&root, &parse_error)) {
        if (error) {
            *error = parse_error;
        }
        return false;
    }
    if (root.type != JsonValue::Type::Object) {
        if (error) {
            *error = "Root must be a JSON object.";
        }
        return false;
    }
    if (error) {
        error->clear();
    }
    return true;
}

} // namespace scratchrobin
