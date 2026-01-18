#include "mock_backend.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>

namespace scratchrobin {
namespace {

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
    explicit JsonParser(const std::string& input) : input_(input) {}

    bool Parse(JsonValue* out, std::string* error) {
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

private:
    void SkipWhitespace() {
        while (pos_ < input_.size()) {
            char c = input_[pos_];
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    bool ParseValue(JsonValue* out) {
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

    bool ParseObject(JsonValue* out) {
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

    bool ParseArray(JsonValue* out) {
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

    bool ParseString(std::string* out) {
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

    bool ParseNumber(JsonValue* out) {
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

    bool ParseLiteral(const std::string& literal, JsonValue::Type type, bool bool_value,
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

    bool Consume(char expected) {
        if (pos_ < input_.size() && input_[pos_] == expected) {
            ++pos_;
            return true;
        }
        return false;
    }

    void SetError(const std::string& message) {
        error_ = message;
    }

    const std::string& input_;
    size_t pos_ = 0;
    std::string error_;
};

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

std::string NumberToString(double value) {
    if (std::isfinite(value)) {
        double rounded = std::floor(value);
        if (rounded == value) {
            return std::to_string(static_cast<int64_t>(value));
        }
    }
    std::ostringstream out;
    out << value;
    return out.str();
}

bool HexToBytes(const std::string& text, std::vector<uint8_t>* out, std::string* error) {
    if (!out) {
        return false;
    }
    std::string hex = text;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
        hex = hex.substr(2);
    }
    if (hex.size() % 2 != 0) {
        if (error) {
            *error = "Hex string must have even length";
        }
        return false;
    }
    out->clear();
    out->reserve(hex.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return 10 + (c - 'a');
        }
        if (c >= 'A' && c <= 'F') {
            return 10 + (c - 'A');
        }
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        int high = nibble(hex[i]);
        int low = nibble(hex[i + 1]);
        if (high < 0 || low < 0) {
            if (error) {
                *error = "Invalid hex character";
            }
            return false;
        }
        out->push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return true;
}

std::string NormalizeSql(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    bool in_space = false;
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_space = true;
            continue;
        }
        if (in_space && !out.empty()) {
            out.push_back(' ');
        }
        in_space = false;
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    while (!out.empty() && (out.back() == ';' || out.back() == ' ')) {
        out.pop_back();
    }
    return out;
}

struct FixtureQuery {
    std::string match;
    bool is_regex = false;
    std::regex regex;
    QueryResult result;
    bool has_error = false;
    std::string error_message;
    std::vector<std::string> error_stack;
};

bool ParseStringArray(const JsonValue& value, std::vector<std::string>* out, std::string* error) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Array) {
        if (error) {
            *error = "Expected string array";
        }
        return false;
    }
    out->clear();
    for (const auto& entry : value.array_value) {
        if (entry.type != JsonValue::Type::String) {
            if (error) {
                *error = "Array entry must be a string";
            }
            return false;
        }
        out->push_back(entry.string_value);
    }
    return true;
}

bool ParseMessages(const JsonValue& value, std::vector<QueryMessage>* out, std::string* error) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Array) {
        if (error) {
            *error = "messages must be an array";
        }
        return false;
    }
    out->clear();
    for (const auto& entry : value.array_value) {
        QueryMessage message;
        if (entry.type == JsonValue::Type::String) {
            message.severity = "notice";
            message.message = entry.string_value;
            out->push_back(std::move(message));
            continue;
        }
        if (entry.type != JsonValue::Type::Object) {
            if (error) {
                *error = "message entry must be an object or string";
            }
            return false;
        }
        const JsonValue* severity = FindMember(entry, "severity");
        if (severity && severity->type == JsonValue::Type::String) {
            message.severity = severity->string_value;
        } else {
            message.severity = "notice";
        }
        const JsonValue* text = FindMember(entry, "message");
        if (text && text->type == JsonValue::Type::String) {
            message.message = text->string_value;
        } else {
            if (error) {
                *error = "message.message must be a string";
            }
            return false;
        }
        const JsonValue* detail = FindMember(entry, "detail");
        if (detail && detail->type == JsonValue::Type::String) {
            message.detail = detail->string_value;
        }
        out->push_back(std::move(message));
    }
    return true;
}

bool ParseCellValue(const JsonValue& value, QueryValue* out, std::string* error) {
    if (!out) {
        return false;
    }
    *out = QueryValue{};

    if (value.type == JsonValue::Type::Null) {
        out->isNull = true;
        out->text = "NULL";
        return true;
    }

    if (value.type == JsonValue::Type::Bool) {
        out->isNull = false;
        out->text = value.bool_value ? "true" : "false";
        return true;
    }

    if (value.type == JsonValue::Type::Number) {
        out->isNull = false;
        out->text = NumberToString(value.number_value);
        return true;
    }

    if (value.type == JsonValue::Type::String) {
        out->isNull = false;
        out->text = value.string_value;
        return true;
    }

    if (value.type == JsonValue::Type::Object) {
        const JsonValue* is_null = FindMember(value, "is_null");
        bool null_flag = false;
        if (is_null && GetBoolValue(*is_null, &null_flag) && null_flag) {
            out->isNull = true;
            out->text = "NULL";
            return true;
        }

        const JsonValue* text = FindMember(value, "text");
        if (text && text->type == JsonValue::Type::String) {
            out->text = text->string_value;
        }

        const JsonValue* data_hex = FindMember(value, "data_hex");
        if (data_hex && data_hex->type == JsonValue::Type::String) {
            std::string hex_error;
            if (!HexToBytes(data_hex->string_value, &out->raw, &hex_error)) {
                if (error) {
                    *error = hex_error;
                }
                return false;
            }
            if (out->text.empty() && !out->raw.empty()) {
                out->text = data_hex->string_value;
            }
        }

        out->isNull = false;
        return true;
    }

    if (error) {
        *error = "Unsupported cell value type";
    }
    return false;
}

bool ParseResultObject(const JsonValue& value, QueryResult* out, std::string* error) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Object) {
        if (error) {
            *error = "Result must be an object";
        }
        return false;
    }

    out->columns.clear();
    out->rows.clear();
    out->rowsAffected = 0;
    out->commandTag.clear();
    out->messages.clear();
    out->errorStack.clear();
    out->stats = QueryStats{};

    if (const JsonValue* columns = FindMember(value, "columns")) {
        if (columns->type != JsonValue::Type::Array) {
            if (error) {
                *error = "columns must be an array";
            }
            return false;
        }
        for (const auto& col : columns->array_value) {
            if (col.type != JsonValue::Type::Object) {
                if (error) {
                    *error = "column entry must be an object";
                }
                return false;
            }
            QueryColumn column;
            const JsonValue* name = FindMember(col, "name");
            if (!name || name->type != JsonValue::Type::String) {
                if (error) {
                    *error = "column name must be a string";
                }
                return false;
            }
            column.name = name->string_value;
            const JsonValue* type = FindMember(col, "wire_type");
            if (type && type->type == JsonValue::Type::String) {
                column.type = type->string_value;
            } else {
                column.type = "UNKNOWN";
            }
            out->columns.push_back(std::move(column));
        }
    }

    if (const JsonValue* rows = FindMember(value, "rows")) {
        if (rows->type != JsonValue::Type::Array) {
            if (error) {
                *error = "rows must be an array";
            }
            return false;
        }
        for (const auto& row_value : rows->array_value) {
            if (row_value.type != JsonValue::Type::Array) {
                if (error) {
                    *error = "row must be an array";
                }
                return false;
            }
            std::vector<QueryValue> row;
            row.reserve(row_value.array_value.size());
            for (const auto& cell_value : row_value.array_value) {
                QueryValue cell;
                std::string cell_error;
                if (!ParseCellValue(cell_value, &cell, &cell_error)) {
                    if (error) {
                        *error = cell_error.empty() ? "Invalid cell value" : cell_error;
                    }
                    return false;
                }
                row.push_back(std::move(cell));
            }
            out->rows.push_back(std::move(row));
        }
    }

    if (const JsonValue* rows_affected = FindMember(value, "rows_affected")) {
        int64_t parsed = 0;
        if (GetInt64Value(*rows_affected, &parsed)) {
            out->rowsAffected = parsed;
        }
    }

    if (const JsonValue* command_tag = FindMember(value, "command_tag")) {
        if (command_tag->type == JsonValue::Type::String) {
            out->commandTag = command_tag->string_value;
        }
    }

    if (const JsonValue* messages = FindMember(value, "messages")) {
        if (!ParseMessages(*messages, &out->messages, error)) {
            return false;
        }
    } else if (const JsonValue* notices = FindMember(value, "notices")) {
        if (!ParseMessages(*notices, &out->messages, error)) {
            return false;
        }
    }

    if (const JsonValue* error_stack = FindMember(value, "error_stack")) {
        if (!ParseStringArray(*error_stack, &out->errorStack, error)) {
            return false;
        }
    }

    return true;
}

bool ParseFixtureQueries(const JsonValue& root, std::vector<FixtureQuery>* out,
                         std::string* error) {
    if (!out) {
        return false;
    }
    out->clear();

    if (root.type != JsonValue::Type::Object) {
        if (error) {
            *error = "Fixture root must be an object";
        }
        return false;
    }

    const JsonValue* queries = FindMember(root, "queries");
    if (!queries || queries->type != JsonValue::Type::Array) {
        if (error) {
            *error = "Fixture must contain a 'queries' array";
        }
        return false;
    }

    for (const auto& query_value : queries->array_value) {
        if (query_value.type != JsonValue::Type::Object) {
            if (error) {
                *error = "Query entry must be an object";
            }
            return false;
        }

        FixtureQuery query;
        const JsonValue* match = FindMember(query_value, "match");
        if (!match || match->type != JsonValue::Type::String) {
            if (error) {
                *error = "Query entry missing 'match' string";
            }
            return false;
        }
        query.match = match->string_value;

        const JsonValue* match_type = FindMember(query_value, "match_type");
        if (match_type && match_type->type == JsonValue::Type::String) {
            std::string mode = match_type->string_value;
            std::transform(mode.begin(), mode.end(), mode.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            query.is_regex = (mode == "regex");
        }

        if (query.is_regex) {
            try {
                query.regex = std::regex(query.match, std::regex::icase);
            } catch (const std::regex_error&) {
                if (error) {
                    *error = "Invalid regex in query match";
                }
                return false;
            }
        }

        if (const JsonValue* error_obj = FindMember(query_value, "error")) {
            if (error_obj->type != JsonValue::Type::Object) {
                if (error) {
                    *error = "error must be an object";
                }
                return false;
            }
            const JsonValue* message = FindMember(*error_obj, "message");
            if (message && message->type == JsonValue::Type::String) {
                query.error_message = message->string_value;
                query.has_error = true;
            } else {
                if (error) {
                    *error = "error.message must be a string";
                }
                return false;
            }
            if (const JsonValue* stack = FindMember(*error_obj, "stack")) {
                if (!ParseStringArray(*stack, &query.error_stack, error)) {
                    return false;
                }
            }
        } else if (const JsonValue* result_obj = FindMember(query_value, "result")) {
            if (!ParseResultObject(*result_obj, &query.result, error)) {
                return false;
            }
        } else {
            if (error) {
                *error = "Query entry requires result or error";
            }
            return false;
        }

        out->push_back(std::move(query));
    }

    return true;
}

bool LoadFixtureFile(const std::string& path, std::vector<FixtureQuery>* out,
                     std::string* error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        if (error) {
            *error = "Unable to open fixture file: " + path;
        }
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string contents = buffer.str();

    JsonParser parser(contents);
    JsonValue root;
    std::string parse_error;
    if (!parser.Parse(&root, &parse_error)) {
        if (error) {
            *error = "Fixture parse error: " + parse_error;
        }
        return false;
    }

    return ParseFixtureQueries(root, out, error);
}

class MockBackend : public ConnectionBackend {
public:
    bool Connect(const BackendConfig& config, std::string* error) override {
        if (config.fixturePath.empty()) {
            if (error) {
                *error = "Mock backend requires a fixturePath";
            }
            return false;
        }
        fixture_path_ = config.fixturePath;
        std::vector<FixtureQuery> loaded;
        std::string load_error;
        if (!LoadFixtureFile(fixture_path_, &loaded, &load_error)) {
            if (error) {
                *error = load_error;
            }
            return false;
        }
        queries_ = std::move(loaded);
        connected_ = true;
        return true;
    }

    void Disconnect() override {
        connected_ = false;
    }

    bool IsConnected() const override {
        return connected_;
    }

    bool ExecuteQuery(const std::string& sql, QueryResult* outResult,
                      std::string* error) override {
        if (!connected_) {
            if (error) {
                *error = "Mock backend not connected";
            }
            return false;
        }
        if (!outResult) {
            if (error) {
                *error = "No result buffer provided";
            }
            return false;
        }

        std::string normalized = NormalizeSql(sql);
        for (const auto& query : queries_) {
            if (query.is_regex) {
                if (!std::regex_search(sql, query.regex)) {
                    continue;
                }
            } else {
                if (NormalizeSql(query.match) != normalized) {
                    continue;
                }
            }

            if (query.has_error) {
                if (outResult) {
                    outResult->errorStack = query.error_stack;
                }
                if (error) {
                    *error = query.error_message.empty() ? "Mock error" : query.error_message;
                }
                return false;
            }

            *outResult = query.result;
            return true;
        }

        if (error) {
            *error = "No mock fixture match for query";
        }
        return false;
    }

    bool BeginTransaction(std::string* error) override {
        if (!connected_) {
            if (error) {
                *error = "Mock backend not connected";
            }
            return false;
        }
        return true;
    }

    bool Commit(std::string* error) override {
        if (!connected_) {
            if (error) {
                *error = "Mock backend not connected";
            }
            return false;
        }
        return true;
    }

    bool Rollback(std::string* error) override {
        if (!connected_) {
            if (error) {
                *error = "Mock backend not connected";
            }
            return false;
        }
        return true;
    }

    bool Cancel(std::string* error) override {
        if (!connected_) {
            if (error) {
                *error = "Mock backend not connected";
            }
            return false;
        }
        return true;
    }

    BackendCapabilities Capabilities() const override {
        BackendCapabilities caps;
        caps.supportsCancel = true;
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        return caps;
    }

    std::string BackendName() const override {
        return "mock";
    }

private:
    bool connected_ = false;
    std::string fixture_path_;
    std::vector<FixtureQuery> queries_;
};

} // namespace

std::unique_ptr<ConnectionBackend> CreateMockBackend() {
    return std::make_unique<MockBackend>();
}

} // namespace scratchrobin
