/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "value_formatter.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace scratchrobin {
namespace {

std::string NormalizeType(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

bool IsHexChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsPrintableAscii(const std::vector<uint8_t>& data) {
    for (uint8_t byte : data) {
        if (byte == '\t' || byte == '\n' || byte == '\r') {
            continue;
        }
        if (byte < 0x20 || byte > 0x7e) {
            return false;
        }
    }
    return true;
}

std::string BytesToHex(const std::vector<uint8_t>& data, size_t max_bytes, bool* truncated) {
    if (truncated) {
        *truncated = false;
    }
    if (data.empty()) {
        return std::string();
    }

    size_t count = data.size();
    if (max_bytes > 0 && count > max_bytes) {
        count = max_bytes;
        if (truncated) {
            *truncated = true;
        }
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < count; ++i) {
        out << std::setw(2) << static_cast<int>(data[i]);
    }
    return out.str();
}

std::string FormatBinary(const std::vector<uint8_t>& data, const FormatOptions& options) {
    if (data.empty()) {
        return std::string();
    }

    bool truncated = false;
    std::string hex = BytesToHex(data, options.maxBinaryBytes, &truncated);
    if (hex.empty()) {
        return std::string();
    }

    std::string out = "0x" + hex;
    if (truncated) {
        out += "...";
    }
    if (options.includeBinarySize) {
        out += " (" + std::to_string(data.size()) + " bytes)";
    }
    return out;
}

std::string FormatUuidFromRaw(const std::vector<uint8_t>& raw) {
    if (raw.size() != 16) {
        return std::string();
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < raw.size(); ++i) {
        out << std::setw(2) << static_cast<int>(raw[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
            out << '-';
        }
    }
    return out.str();
}

std::string NormalizeUuidText(const std::string& text) {
    std::string trimmed;
    trimmed.reserve(text.size());
    for (char c : text) {
        if (c == '{' || c == '}') {
            continue;
        }
        if (c == '-') {
            continue;
        }
        trimmed.push_back(c);
    }
    if (trimmed.size() != 32) {
        return text;
    }
    for (char c : trimmed) {
        if (!IsHexChar(c)) {
            return text;
        }
    }

    std::string lower = trimmed;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::ostringstream out;
    out << lower.substr(0, 8) << '-' << lower.substr(8, 4) << '-'
        << lower.substr(12, 4) << '-' << lower.substr(16, 4) << '-'
        << lower.substr(20, 12);
    return out.str();
}

std::string FormatVectorPreview(const std::vector<uint8_t>& raw) {
    if (raw.empty()) {
        return std::string();
    }
    if (raw.size() % sizeof(float) != 0) {
        return std::string();
    }

    size_t count = raw.size() / sizeof(float);
    size_t show = std::min<size_t>(count, 8);

    std::ostringstream out;
    out << '[';
    for (size_t i = 0; i < show; ++i) {
        float value = 0.0f;
        std::memcpy(&value, raw.data() + i * sizeof(float), sizeof(float));
        if (i > 0) {
            out << ", ";
        }
        out << std::setprecision(6) << value;
    }
    if (count > show) {
        out << ", ...";
    }
    out << ']';
    return out.str();
}

} // namespace

bool IsBooleanType(const std::string& type) {
    std::string normalized = NormalizeType(type);
    return normalized == "boolean" || normalized == "bool";
}

bool IsNumericType(const std::string& type) {
    std::string normalized = NormalizeType(type);
    return normalized == "int16" || normalized == "int32" || normalized == "int64" ||
           normalized == "float32" || normalized == "float64" || normalized == "decimal" ||
           normalized == "numeric" || normalized == "money";
}

bool IsJsonType(const std::string& type) {
    std::string normalized = NormalizeType(type);
    return normalized == "json" || normalized == "jsonb";
}

std::string FormatValueForDisplay(const QueryValue& value,
                                  const std::string& type,
                                  const FormatOptions& options) {
    if (value.isNull) {
        return "NULL";
    }

    std::string normalized = NormalizeType(type);

    if (normalized == "uuid") {
        if (!value.raw.empty()) {
            std::string formatted = FormatUuidFromRaw(value.raw);
            if (!formatted.empty()) {
                return formatted;
            }
        }
        if (!value.text.empty()) {
            return NormalizeUuidText(value.text);
        }
    }

    if (IsJsonType(normalized)) {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty() && IsPrintableAscii(value.raw)) {
            return std::string(value.raw.begin(), value.raw.end());
        }
        if (!value.raw.empty()) {
            return FormatBinary(value.raw, options);
        }
    }

    if (normalized == "vector") {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty()) {
            std::string preview = FormatVectorPreview(value.raw);
            if (!preview.empty()) {
                return preview;
            }
            return FormatBinary(value.raw, options);
        }
    }

    if (normalized == "geometry") {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty()) {
            return FormatBinary(value.raw, options);
        }
    }

    if (!value.text.empty()) {
        return value.text;
    }

    if (!value.raw.empty()) {
        return FormatBinary(value.raw, options);
    }

    return std::string();
}

std::string FormatValueForExport(const QueryValue& value,
                                 const std::string& type,
                                 const FormatOptions& options) {
    if (value.isNull) {
        return "NULL";
    }

    std::string normalized = NormalizeType(type);

    if (normalized == "uuid") {
        if (!value.raw.empty()) {
            std::string formatted = FormatUuidFromRaw(value.raw);
            if (!formatted.empty()) {
                return formatted;
            }
        }
        if (!value.text.empty()) {
            return NormalizeUuidText(value.text);
        }
    }

    if (IsJsonType(normalized)) {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty() && IsPrintableAscii(value.raw)) {
            return std::string(value.raw.begin(), value.raw.end());
        }
        if (!value.raw.empty()) {
            return FormatBinary(value.raw, options);
        }
    }

    if (normalized == "vector") {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty()) {
            std::string preview = FormatVectorPreview(value.raw);
            if (!preview.empty()) {
                return preview;
            }
            return FormatBinary(value.raw, options);
        }
    }

    if (normalized == "geometry") {
        if (!value.text.empty()) {
            return value.text;
        }
        if (!value.raw.empty()) {
            return FormatBinary(value.raw, options);
        }
    }

    if (!value.text.empty()) {
        return value.text;
    }

    if (!value.raw.empty()) {
        return FormatBinary(value.raw, options);
    }

    return std::string();
}

} // namespace scratchrobin
