/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "json_util.h"

#include <sstream>

#include "simple_json.h"

namespace scratchrobin {
namespace {

void AppendEscapedString(std::string& out, const std::string& value) {
    out.push_back('"');
    for (char c : value) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    out.push_back('"');
}

void AppendJsonValue(std::string& out, const JsonValue& value) {
    switch (value.type) {
        case JsonValue::Type::Null:
            out += "null";
            break;
        case JsonValue::Type::Bool:
            out += (value.bool_value ? "true" : "false");
            break;
        case JsonValue::Type::Number: {
            std::ostringstream oss;
            oss << value.number_value;
            out += oss.str();
            break;
        }
        case JsonValue::Type::String:
            AppendEscapedString(out, value.string_value);
            break;
        case JsonValue::Type::Array: {
            out.push_back('[');
            for (size_t i = 0; i < value.array_value.size(); ++i) {
                if (i > 0) out.push_back(',');
                AppendJsonValue(out, value.array_value[i]);
            }
            out.push_back(']');
            break;
        }
        case JsonValue::Type::Object: {
            out.push_back('{');
            bool first = true;
            for (const auto& pair : value.object_value) {
                if (!first) out.push_back(',');
                first = false;
                AppendEscapedString(out, pair.first);
                out.push_back(':');
                AppendJsonValue(out, pair.second);
            }
            out.push_back('}');
            break;
        }
    }
}

bool UpdateJsonObjectFieldInternal(const std::string& json,
                                   const std::string& key,
                                   const JsonValue& new_value,
                                   std::string* out_json) {
    if (!out_json) return false;
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    if (!parser.Parse(&root, &error)) {
        return false;
    }
    if (root.type != JsonValue::Type::Object) {
        return false;
    }
    root.object_value[key] = new_value;
    std::string result;
    AppendJsonValue(result, root);
    *out_json = result;
    return true;
}

} // namespace

bool UpdateJsonObjectBoolField(const std::string& json, const std::string& key,
                               bool value, std::string* out_json) {
    JsonValue val;
    val.type = JsonValue::Type::Bool;
    val.bool_value = value;
    return UpdateJsonObjectFieldInternal(json, key, val, out_json);
}

bool UpdateJsonObjectStringField(const std::string& json, const std::string& key,
                                 const std::string& value, std::string* out_json) {
    JsonValue val;
    val.type = JsonValue::Type::String;
    val.string_value = value;
    return UpdateJsonObjectFieldInternal(json, key, val, out_json);
}

bool UpdateJsonObjectNumberField(const std::string& json, const std::string& key,
                                 double value, std::string* out_json) {
    JsonValue val;
    val.type = JsonValue::Type::Number;
    val.number_value = value;
    return UpdateJsonObjectFieldInternal(json, key, val, out_json);
}

bool UpdateJsonObjectStringArrayField(const std::string& json, const std::string& key,
                                      const std::vector<std::string>& values,
                                      std::string* out_json) {
    JsonValue val;
    val.type = JsonValue::Type::Array;
    for (const auto& entry : values) {
        JsonValue item;
        item.type = JsonValue::Type::String;
        item.string_value = entry;
        val.array_value.push_back(item);
    }
    return UpdateJsonObjectFieldInternal(json, key, val, out_json);
}

} // namespace scratchrobin
