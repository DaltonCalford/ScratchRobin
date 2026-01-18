#include "statement_splitter.h"

#include <algorithm>
#include <cctype>

namespace scratchrobin {
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool StartsWithInsensitive(const std::string& value, const std::string& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    return ToLower(value.substr(0, prefix.size())) == ToLower(prefix);
}

std::vector<std::string> SplitTokens(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : line) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

bool ParseDelimiterDirective(const std::string& line, std::string* outDelimiter) {
    std::string trimmed = Trim(line);
    if (trimmed.empty()) {
        return false;
    }

    if (StartsWithInsensitive(trimmed, "delimiter")) {
        auto tokens = SplitTokens(trimmed);
        if (tokens.size() >= 2) {
            *outDelimiter = tokens[1];
            return true;
        }
        return false;
    }

    if (StartsWithInsensitive(trimmed, "set term")) {
        auto tokens = SplitTokens(trimmed);
        if (tokens.size() >= 3) {
            *outDelimiter = tokens[2];
            return true;
        }
        return false;
    }

    return false;
}

} // namespace

StatementSplitter::SplitResult StatementSplitter::Split(const std::string& input) const {
    SplitResult result;

    std::string current;
    size_t line_start = 0;
    bool in_single = false;
    bool in_double = false;
    bool in_line_comment = false;
    bool in_block_comment = false;

    auto flush_statement = [&](std::string statement) {
        statement = Trim(statement);
        if (!statement.empty()) {
            result.statements.push_back(std::move(statement));
        }
    };

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        char next = (i + 1 < input.size()) ? input[i + 1] : '\0';

        if (in_line_comment) {
            current.push_back(c);
            if (c == '\n') {
                in_line_comment = false;
            }
            continue;
        }

        if (in_block_comment) {
            current.push_back(c);
            if (c == '*' && next == '/') {
                current.push_back(next);
                ++i;
                in_block_comment = false;
            }
            continue;
        }

        if (!in_single && !in_double) {
            if (c == '-' && next == '-') {
                current.push_back(c);
                current.push_back(next);
                ++i;
                in_line_comment = true;
                continue;
            }
            if (c == '/' && next == '*') {
                current.push_back(c);
                current.push_back(next);
                ++i;
                in_block_comment = true;
                continue;
            }
        }

        current.push_back(c);

        if (c == '\'' && !in_double) {
            if (next == '\'') {
                current.push_back(next);
                ++i;
            } else {
                in_single = !in_single;
            }
        } else if (c == '"' && !in_single) {
            if (next == '"') {
                current.push_back(next);
                ++i;
            } else {
                in_double = !in_double;
            }
        }

        if (!in_single && !in_double) {
            if (c == '\n') {
                std::string line = current.substr(line_start);
                std::string new_delim;
                if (ParseDelimiterDirective(line, &new_delim)) {
                    current.erase(line_start);
                    result.delimiter = new_delim;
                }
                line_start = current.size();
            }

            if (!result.delimiter.empty() &&
                current.size() >= result.delimiter.size() &&
                current.compare(current.size() - result.delimiter.size(),
                                result.delimiter.size(),
                                result.delimiter) == 0) {
                std::string statement = current.substr(0, current.size() - result.delimiter.size());
                flush_statement(statement);
                current.clear();
                line_start = 0;
            }
        }
    }

    if (!current.empty()) {
        std::string line = current.substr(line_start);
        std::string new_delim;
        if (ParseDelimiterDirective(line, &new_delim)) {
            current.erase(line_start);
            result.delimiter = new_delim;
        }
    }

    flush_statement(current);
    return result;
}

} // namespace scratchrobin
