/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SIMPLE_JSON_H
#define SCRATCHROBIN_SIMPLE_JSON_H

#include <map>
#include <string>
#include <vector>

namespace scratchrobin {

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    Type type = Type::Null;
    bool bool_value = false;
    double number_value = 0.0;
    std::string string_value;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;
};

class JsonParser {
public:
    explicit JsonParser(const std::string& input);

    bool Parse(JsonValue* out, std::string* error);

private:
    void SkipWhitespace();
    bool ParseValue(JsonValue* out);
    bool ParseObject(JsonValue* out);
    bool ParseArray(JsonValue* out);
    bool ParseString(std::string* out);
    bool ParseNumber(JsonValue* out);
    bool ParseLiteral(const std::string& literal, JsonValue::Type type, bool bool_value,
                      JsonValue* out);
    bool Consume(char expected);
    void SetError(const std::string& message);

    const std::string& input_;
    size_t pos_ = 0;
    std::string error_;
};

const JsonValue* FindMember(const JsonValue& value, const std::string& key);

bool GetStringValue(const JsonValue& value, std::string* out);
bool GetBoolValue(const JsonValue& value, bool* out);
bool GetInt64Value(const JsonValue& value, int64_t* out);

} // namespace scratchrobin

#endif // SCRATCHROBIN_SIMPLE_JSON_H
