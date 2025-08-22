#include "string_utils.h"

#include <sstream>
#include <regex>

namespace scratchrobin::utils {

// Trimming functions
std::string trim(const std::string& str) {
    return trimLeft(trimRight(str));
}

std::string trimLeft(const std::string& str) {
    auto it = std::find_if_not(str.begin(), str.end(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    });
    return std::string(it, str.end());
}

std::string trimRight(const std::string& str) {
    auto it = std::find_if_not(str.rbegin(), str.rend(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    });
    return std::string(str.begin(), it.base());
}

// Case conversion
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](char c) { return std::tolower(static_cast<unsigned char>(c)); });
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
    return result;
}

// Splitting and joining
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.push_back(str.substr(start));
    return result;
}

std::string join(const std::vector<std::string>& parts, const std::string& separator) {
    if (parts.empty()) {
        return "";
    }

    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += separator + parts[i];
    }

    return result;
}

std::string join(const std::vector<std::string>& parts, char separator) {
    return join(parts, std::string(1, separator));
}

// Prefix and suffix checking
bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() &&
           str.compare(0, prefix.length(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.length() >= suffix.length() &&
           str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// String replacement
std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return str;
    }

    std::string result = str;
    size_t pos = 0;

    if ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
    }

    return result;
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return str;
    }

    std::string result = str;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

// Empty and blank checking
bool isEmpty(const std::string& str) {
    return str.empty();
}

bool isBlank(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    });
}

// Case formatting
std::string capitalize(const std::string& str) {
    if (str.empty()) {
        return str;
    }

    std::string result = str;
    result[0] = std::toupper(static_cast<unsigned char>(result[0]));

    for (size_t i = 1; i < result.length(); ++i) {
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    }

    return result;
}

std::string titleCase(const std::string& str) {
    std::string result = str;
    bool capitalizeNext = true;

    for (size_t i = 0; i < result.length(); ++i) {
        if (std::isspace(static_cast<unsigned char>(result[i]))) {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result[i] = std::toupper(static_cast<unsigned char>(result[i]));
            capitalizeNext = false;
        } else {
            result[i] = std::tolower(static_cast<unsigned char>(result[i]));
        }
    }

    return result;
}

// String reversal
std::string reverse(const std::string& str) {
    return std::string(str.rbegin(), str.rend());
}

// Padding
std::string leftPad(const std::string& str, size_t length, char padChar) {
    if (str.length() >= length) {
        return str;
    }

    return std::string(length - str.length(), padChar) + str;
}

std::string rightPad(const std::string& str, size_t length, char padChar) {
    if (str.length() >= length) {
        return str;
    }

    return str + std::string(length - str.length(), padChar);
}

// Truncation
std::string truncate(const std::string& str, size_t length, const std::string& suffix) {
    if (str.length() <= length) {
        return str;
    }

    if (length <= suffix.length()) {
        return suffix.substr(0, length);
    }

    return str.substr(0, length - suffix.length()) + suffix;
}

// String searching
bool contains(const std::string& str, const std::string& substring) {
    return str.find(substring) != std::string::npos;
}

bool containsIgnoreCase(const std::string& str, const std::string& substring) {
    return contains(toLower(str), toLower(substring));
}

size_t countOccurrences(const std::string& str, const std::string& substring) {
    if (substring.empty()) {
        return 0;
    }

    size_t count = 0;
    size_t pos = 0;

    while ((pos = str.find(substring, pos)) != std::string::npos) {
        ++count;
        pos += substring.length();
    }

    return count;
}

// String removal
std::string remove(const std::string& str, const std::string& toRemove) {
    return replace(str, toRemove, "");
}

std::string removeAll(const std::string& str, const std::string& toRemove) {
    return replaceAll(str, toRemove, "");
}

// Escape and unescape
std::string escape(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 2);

    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '\"': result += "\\\""; break;
            case '\'': result += "\\\'"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            default:
                if (c < 32 || c > 126) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }

    return result;
}

std::string unescape(const std::string& str) {
    std::string result;
    result.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; break;
                case '\"': result += '\"'; break;
                case '\'': result += '\''; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'x':
                    if (i + 3 < str.length()) {
                        char buf[3] = {str[i + 2], str[i + 3], '\0'};
                        char c = static_cast<char>(std::strtol(buf, nullptr, 16));
                        result += c;
                        i += 2;
                    }
                    break;
                default:
                    result += str[i + 1];
                    break;
            }
            ++i;
        } else {
            result += str[i];
        }
    }

    return result;
}

// Character type checking
bool isNumeric(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
}

bool isAlpha(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) {
        return std::isalpha(static_cast<unsigned char>(c));
    });
}

bool isAlphaNumeric(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c));
    });
}

} // namespace scratchrobin::utils
