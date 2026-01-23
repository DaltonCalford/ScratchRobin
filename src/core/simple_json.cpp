#include "simple_json.h"

#include <cctype>
#include <cmath>
#include <cstdlib>

namespace scratchrobin {

JsonParser::JsonParser(const std::string& input) : input_(input) {}

bool JsonParser::Parse(JsonValue* out, std::string* error) {
    if (!out) {
        if (error) {
            *error = "Null JSON output";
        }
        return false;
    }
    SkipWhitespace();
    if (!ParseValue(out)) {
        if (error) {
            *error = error_;
        }
        return false;
    }
    SkipWhitespace();
    if (pos_ != input_.size()) {
        if (error) {
            *error = "Unexpected trailing JSON";
        }
        return false;
    }
    return true;
}

void JsonParser::SkipWhitespace() {
    while (pos_ < input_.size()) {
        char c = input_[pos_];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            ++pos_;
        } else {
            break;
        }
    }
}

bool JsonParser::ParseValue(JsonValue* out) {
    SkipWhitespace();
    if (pos_ >= input_.size()) {
        SetError("Unexpected end of JSON");
        return false;
    }
    char c = input_[pos_];
    if (c == '{') {
        return ParseObject(out);
    }
    if (c == '[') {
        return ParseArray(out);
    }
    if (c == '"') {
        std::string value;
        if (!ParseString(&value)) {
            return false;
        }
        out->type = JsonValue::Type::String;
        out->string_value = std::move(value);
        return true;
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
        return ParseNumber(out);
    }
    if (c == 't') {
        return ParseLiteral("true", JsonValue::Type::Bool, true, out);
    }
    if (c == 'f') {
        return ParseLiteral("false", JsonValue::Type::Bool, false, out);
    }
    if (c == 'n') {
        return ParseLiteral("null", JsonValue::Type::Null, false, out);
    }
    SetError("Invalid JSON token");
    return false;
}

bool JsonParser::ParseObject(JsonValue* out) {
    if (!Consume('{')) {
        return false;
    }
    SkipWhitespace();
    out->type = JsonValue::Type::Object;
    out->object_value.clear();

    if (Consume('}')) {
        return true;
    }

    while (pos_ < input_.size()) {
        std::string key;
        if (!ParseString(&key)) {
            return false;
        }
        SkipWhitespace();
        if (!Consume(':')) {
            SetError("Expected ':' after object key");
            return false;
        }
        JsonValue value;
        if (!ParseValue(&value)) {
            return false;
        }
        out->object_value.emplace(std::move(key), std::move(value));
        SkipWhitespace();
        if (Consume('}')) {
            return true;
        }
        if (!Consume(',')) {
            SetError("Expected ',' between object entries");
            return false;
        }
        SkipWhitespace();
    }
    SetError("Unterminated JSON object");
    return false;
}

bool JsonParser::ParseArray(JsonValue* out) {
    if (!Consume('[')) {
        return false;
    }
    SkipWhitespace();
    out->type = JsonValue::Type::Array;
    out->array_value.clear();

    if (Consume(']')) {
        return true;
    }

    while (pos_ < input_.size()) {
        JsonValue value;
        if (!ParseValue(&value)) {
            return false;
        }
        out->array_value.push_back(std::move(value));
        SkipWhitespace();
        if (Consume(']')) {
            return true;
        }
        if (!Consume(',')) {
            SetError("Expected ',' between array entries");
            return false;
        }
        SkipWhitespace();
    }
    SetError("Unterminated JSON array");
    return false;
}

bool JsonParser::ParseString(std::string* out) {
    if (!out) {
        SetError("Null string output");
        return false;
    }
    if (!Consume('"')) {
        SetError("Expected '\"' to start string");
        return false;
    }
    out->clear();
    while (pos_ < input_.size()) {
        char c = input_[pos_++];
        if (c == '"') {
            return true;
        }
        if (c == '\\') {
            if (pos_ >= input_.size()) {
                SetError("Unterminated string escape");
                return false;
            }
            char esc = input_[pos_++];
            switch (esc) {
                case '"': out->push_back('"'); break;
                case '\\': out->push_back('\\'); break;
                case '/': out->push_back('/'); break;
                case 'b': out->push_back('\b'); break;
                case 'f': out->push_back('\f'); break;
                case 'n': out->push_back('\n'); break;
                case 'r': out->push_back('\r'); break;
                case 't': out->push_back('\t'); break;
                default:
                    SetError("Unsupported escape sequence");
                    return false;
            }
        } else {
            out->push_back(c);
        }
    }
    SetError("Unterminated JSON string");
    return false;
}

bool JsonParser::ParseNumber(JsonValue* out) {
    const char* start = input_.c_str() + pos_;
    char* end = nullptr;
    double value = std::strtod(start, &end);
    if (end == start) {
        SetError("Invalid JSON number");
        return false;
    }
    pos_ += static_cast<size_t>(end - start);
    out->type = JsonValue::Type::Number;
    out->number_value = value;
    return true;
}

bool JsonParser::ParseLiteral(const std::string& literal, JsonValue::Type type, bool bool_value,
                              JsonValue* out) {
    if (input_.compare(pos_, literal.size(), literal) != 0) {
        SetError("Invalid JSON literal");
        return false;
    }
    pos_ += literal.size();
    out->type = type;
    out->bool_value = bool_value;
    return true;
}

bool JsonParser::Consume(char expected) {
    if (pos_ < input_.size() && input_[pos_] == expected) {
        ++pos_;
        return true;
    }
    return false;
}

void JsonParser::SetError(const std::string& message) {
    error_ = message;
}

const JsonValue* FindMember(const JsonValue& value, const std::string& key) {
    if (value.type != JsonValue::Type::Object) {
        return nullptr;
    }
    auto it = value.object_value.find(key);
    if (it == value.object_value.end()) {
        return nullptr;
    }
    return &it->second;
}

bool GetStringValue(const JsonValue& value, std::string* out) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::String) {
        return false;
    }
    *out = value.string_value;
    return true;
}

bool GetBoolValue(const JsonValue& value, bool* out) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Bool) {
        return false;
    }
    *out = value.bool_value;
    return true;
}

bool GetInt64Value(const JsonValue& value, int64_t* out) {
    if (!out) {
        return false;
    }
    if (value.type == JsonValue::Type::Number) {
        double number = value.number_value;
        if (!std::isfinite(number)) {
            return false;
        }
        double rounded = std::floor(number);
        if (rounded != number) {
            return false;
        }
        *out = static_cast<int64_t>(number);
        return true;
    }
    if (value.type == JsonValue::Type::String) {
        try {
            size_t idx = 0;
            long long parsed = std::stoll(value.string_value, &idx, 10);
            if (idx != value.string_value.size()) {
                return false;
            }
            *out = static_cast<int64_t>(parsed);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

} // namespace scratchrobin
