/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "result_exporter.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "core/value_formatter.h"

namespace scratchrobin {
namespace {

std::string EscapeCsv(const std::string& input) {
    bool needs_quotes = input.find_first_of(",\"\n\r") != std::string::npos;
    if (!needs_quotes) {
        return input;
    }
    std::string out;
    out.reserve(input.size() + 2);
    out.push_back('"');
    for (char c : input) {
        if (c == '"') {
            out.push_back('"');
            out.push_back('"');
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

std::string EscapeJson(const std::string& input) {
    std::ostringstream out;
    for (char c : input) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::uppercase << std::setw(4)
                        << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c))
                        << std::nouppercase << std::dec;
                } else {
                    out << c;
                }
                break;
        }
    }
    return out.str();
}

std::string Trim(std::string input) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    input.erase(input.begin(), std::find_if(input.begin(), input.end(), not_space));
    input.erase(std::find_if(input.rbegin(), input.rend(), not_space).base(), input.end());
    return input;
}

bool LooksLikeJson(const std::string& input) {
    std::string trimmed = Trim(input);
    if (trimmed.size() < 2) {
        return false;
    }
    char first = trimmed.front();
    char last = trimmed.back();
    return (first == '{' && last == '}') || (first == '[' && last == ']');
}

bool TryParseBool(const std::string& input, bool* out) {
    if (!out) {
        return false;
    }
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "true") {
        *out = true;
        return true;
    }
    if (lower == "false") {
        *out = false;
        return true;
    }
    return false;
}

bool TryParseNumber(const std::string& input) {
    if (input.empty()) {
        return false;
    }
    char* end = nullptr;
    std::strtod(input.c_str(), &end);
    return end && *end == '\0';
}

std::string ToJsonLiteral(const QueryValue& value,
                          const QueryColumn& column,
                          const ExportOptions& options) {
    if (value.isNull) {
        return "null";
    }

    FormatOptions format_options;
    format_options.maxBinaryBytes = options.maxBinaryBytes;
    format_options.includeBinarySize = options.includeBinarySize;

    std::string formatted = FormatValueForExport(value, column.type, format_options);

    if (IsBooleanType(column.type)) {
        bool bool_value = false;
        if (TryParseBool(formatted, &bool_value)) {
            return bool_value ? "true" : "false";
        }
    }

    if (IsNumericType(column.type)) {
        if (TryParseNumber(formatted)) {
            return formatted;
        }
    }

    if (IsJsonType(column.type) && LooksLikeJson(formatted)) {
        return formatted;
    }

    return "\"" + EscapeJson(formatted) + "\"";
}

} // namespace

bool ExportResultToCsv(const QueryResult& result,
                       const std::string& path,
                       std::string* error,
                       const ExportOptions& options) {
    std::ofstream out(path);
    if (!out.is_open()) {
        if (error) {
            *error = "Unable to open export file: " + path;
        }
        return false;
    }

    FormatOptions format_options;
    format_options.maxBinaryBytes = options.maxBinaryBytes;
    format_options.includeBinarySize = options.includeBinarySize;

    if (options.includeHeaders && !result.columns.empty()) {
        for (size_t i = 0; i < result.columns.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            out << EscapeCsv(result.columns[i].name);
        }
        out << '\n';
    }

    for (const auto& row : result.rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            std::string value;
            if (i < result.columns.size()) {
                value = FormatValueForExport(row[i], result.columns[i].type, format_options);
            } else {
                value = FormatValueForExport(row[i], "", format_options);
            }
            out << EscapeCsv(value);
        }
        out << '\n';
    }

    return true;
}

bool ExportResultToJson(const QueryResult& result,
                        const std::string& path,
                        std::string* error,
                        const ExportOptions& options) {
    std::ofstream out(path);
    if (!out.is_open()) {
        if (error) {
            *error = "Unable to open export file: " + path;
        }
        return false;
    }

    out << "{\n";
    out << "  \"columns\": [";
    for (size_t i = 0; i < result.columns.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << "{\"name\": \"" << EscapeJson(result.columns[i].name) << "\", "
            << "\"type\": \"" << EscapeJson(result.columns[i].type) << "\"}";
    }
    out << "],\n";

    out << "  \"rows\": [\n";
    for (size_t r = 0; r < result.rows.size(); ++r) {
        out << "    [";
        const auto& row = result.rows[r];
        for (size_t c = 0; c < row.size(); ++c) {
            if (c > 0) {
                out << ", ";
            }
            if (c < result.columns.size()) {
                out << ToJsonLiteral(row[c], result.columns[c], options);
            } else {
                QueryColumn fallback;
                out << ToJsonLiteral(row[c], fallback, options);
            }
        }
        out << ']';
        if (r + 1 < result.rows.size()) {
            out << ',';
        }
        out << "\n";
    }
    out << "  ],\n";

    int64_t rows_returned = result.stats.rowsReturned;
    if (rows_returned == 0) {
        rows_returned = static_cast<int64_t>(result.rows.size());
    }

    out << "  \"stats\": {";
    out << "\"rows_returned\": " << rows_returned;
    out << ", \"rows_affected\": " << result.rowsAffected;
    out << ", \"elapsed_ms\": " << result.stats.elapsedMs;
    out << "},\n";

    out << "  \"messages\": [";
    for (size_t i = 0; i < result.messages.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << "{\"severity\": \"" << EscapeJson(result.messages[i].severity) << "\", "
            << "\"message\": \"" << EscapeJson(result.messages[i].message) << "\"";
        if (!result.messages[i].detail.empty()) {
            out << ", \"detail\": \"" << EscapeJson(result.messages[i].detail) << "\"";
        }
        out << '}';
    }
    out << "],\n";

    out << "  \"error_stack\": [";
    for (size_t i = 0; i < result.errorStack.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << "\"" << EscapeJson(result.errorStack[i]) << "\"";
    }
    out << "]\n";

    out << "}\n";

    return true;
}

} // namespace scratchrobin
