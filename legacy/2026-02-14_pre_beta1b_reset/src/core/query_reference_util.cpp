/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "query_reference_util.h"

#include <algorithm>
#include <cctype>

namespace scratchrobin {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

} // namespace

namespace {

bool IsIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::vector<std::string> TokenizeQuery(const std::string& query) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < query.size()) {
        char c = query[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        if (c == '.') {
            tokens.emplace_back(".");
            ++i;
            continue;
        }
        if (c == '"' || c == '`' || c == '[') {
            char closer = (c == '[') ? ']' : c;
            std::string value;
            ++i;
            while (i < query.size()) {
                char ch = query[i];
                if (ch == closer) {
                    if (closer == '"' && i + 1 < query.size() && query[i + 1] == '"') {
                        value.push_back('"');
                        i += 2;
                        continue;
                    }
                    ++i;
                    break;
                }
                value.push_back(ch);
                ++i;
            }
            tokens.push_back(ToLower(value));
            continue;
        }
        if (IsIdentifierChar(c)) {
            std::string value;
            while (i < query.size()) {
                char ch = query[i];
                if (IsIdentifierChar(ch)) {
                    value.push_back(ch);
                    ++i;
                } else {
                    break;
                }
            }
            tokens.push_back(ToLower(value));
            continue;
        }
        ++i;
    }
    return tokens;
}

bool IsStopToken(const std::string& tok) {
    return tok == "where" || tok == "group" || tok == "order" ||
           tok == "having" || tok == "limit" || tok == "union" ||
           tok == "intersect" || tok == "except";
}

bool IsJoinModifier(const std::string& tok) {
    return tok == "inner" || tok == "left" || tok == "right" ||
           tok == "full" || tok == "cross" || tok == "outer";
}

} // namespace

QueryReferenceResult ExtractQueryReferences(const std::string& query) {
    QueryReferenceResult result;
    if (query.empty()) {
        return result;
    }

    auto tokens = TokenizeQuery(query);
    if (tokens.empty()) {
        return result;
    }

    result.parsed = true;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& tok = tokens[i];
        if (tok == "from" || tok == "join") {
            size_t j = i + 1;
            while (j < tokens.size()) {
                const std::string& candidate = tokens[j];
                if (candidate == "on" || IsStopToken(candidate)) {
                    break;
                }
                if (IsJoinModifier(candidate)) {
                    ++j;
                    continue;
                }
                if (candidate == "select") {
                    break;
                }
                if (candidate == ".") {
                    ++j;
                    continue;
                }
                std::string identifier = candidate;
                size_t k = j + 1;
                int parts = 1;
                while (k + 1 < tokens.size() && tokens[k] == "." && !IsStopToken(tokens[k + 1])) {
                    if (parts >= 3 || tokens[k + 1] == "on" || tokens[k + 1] == "select") {
                        break;
                    }
                    identifier += "." + tokens[k + 1];
                    k += 2;
                    parts++;
                }
                result.identifiers.push_back(identifier);
                j = k;
                break;
            }
        }
    }
    return result;
}

bool QueryReferencesObject(const std::string& query,
                           const std::string& schema,
                           const std::string& name) {
    if (query.empty()) {
        return false;
    }
    auto refs = ExtractQueryReferences(query);
    if (!refs.parsed) {
        return false;
    }
    if (refs.identifiers.empty()) {
        return false;
    }
    std::string schema_l = ToLower(schema);
    std::string name_l = ToLower(name);
    std::string full = schema_l.empty() ? name_l : (schema_l + "." + name_l);
    for (const auto& ident : refs.identifiers) {
        if (ident.empty()) {
            continue;
        }
        if (ident == full) {
            return true;
        }
        bool has_dot = ident.find('.') != std::string::npos;
        if (!has_dot) {
            if (ident == name_l) {
                return true;
            }
            continue;
        }
        // Handle schema-qualified (or catalog.schema) identifiers.
        std::vector<std::string> parts;
        std::string current;
        for (char c : ident) {
            if (c == '.') {
                parts.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }
        if (parts.size() >= 2) {
            const std::string& tail = parts.back();
            const std::string& schema_part = parts[parts.size() - 2];
            if (tail == name_l) {
                if (schema_l.empty()) {
                    return true;
                }
                if (schema_part == schema_l) {
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace scratchrobin
