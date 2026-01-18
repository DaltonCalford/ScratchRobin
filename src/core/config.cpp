#include "config.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace scratchrobin {
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string StripComment(const std::string& value) {
    auto pos = value.find('#');
    if (pos == std::string::npos) {
        return value;
    }
    return value.substr(0, pos);
}

bool SplitKeyValue(const std::string& line, std::string* outKey, std::string* outValue) {
    auto pos = line.find('=');
    if (pos == std::string::npos) {
        return false;
    }
    *outKey = Trim(line.substr(0, pos));
    *outValue = Trim(line.substr(pos + 1));
    return true;
}

bool ParseInt(const std::string& value, int* out) {
    try {
        size_t idx = 0;
        int parsed = std::stoi(value, &idx, 10);
        if (idx != value.size()) {
            return false;
        }
        *out = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

std::string UnescapeTomlString(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            char next = value[i + 1];
            if (next == '\\' || next == '"') {
                out.push_back(next);
                ++i;
                continue;
            }
        }
        out.push_back(value[i]);
    }
    return out;
}

bool ParseString(const std::string& value, std::string* out) {
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
        return false;
    }
    std::string raw = value.substr(1, value.size() - 2);
    *out = UnescapeTomlString(raw);
    return true;
}

std::string ParseSectionName(const std::string& line) {
    if (line.size() >= 4 && line.rfind("[[", 0) == 0 && line.substr(line.size() - 2) == "]]") {
        return Trim(line.substr(2, line.size() - 4));
    }
    if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
        return Trim(line.substr(1, line.size() - 2));
    }
    return {};
}

} // namespace

bool ConfigStore::LoadAppConfig(const std::string& path, AppConfig* outConfig) {
    if (!outConfig) {
        return false;
    }
    *outConfig = AppConfig{};

    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(StripComment(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            section = ParseSectionName(line);
            continue;
        }

        std::string key;
        std::string value;
        if (!SplitKeyValue(line, &key, &value)) {
            continue;
        }

        if (section == "ui") {
            if (key == "theme") {
                ParseString(value, &outConfig->theme);
            } else if (key == "font_family") {
                ParseString(value, &outConfig->fontFamily);
            } else if (key == "font_size") {
                ParseInt(value, &outConfig->fontSize);
            }
        } else if (section == "network") {
            if (key == "connect_timeout_ms") {
                ParseInt(value, &outConfig->network.connectTimeoutMs);
            } else if (key == "query_timeout_ms") {
                ParseInt(value, &outConfig->network.queryTimeoutMs);
            } else if (key == "read_timeout_ms") {
                ParseInt(value, &outConfig->network.readTimeoutMs);
            } else if (key == "write_timeout_ms") {
                ParseInt(value, &outConfig->network.writeTimeoutMs);
            }
        }
    }

    return true;
}

bool ConfigStore::LoadConnections(const std::string& path, std::vector<ConnectionProfile>* outConnections) {
    if (!outConnections) {
        return false;
    }
    outConnections->clear();

    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    std::string line;
    std::string section;
    ConnectionProfile current;
    bool in_connection = false;

    auto flush_current = [&]() {
        if (in_connection) {
            outConnections->push_back(current);
            current = ConnectionProfile{};
        }
    };

    while (std::getline(input, line)) {
        line = Trim(StripComment(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            section = ParseSectionName(line);
            if (section == "connection") {
                flush_current();
                in_connection = true;
            } else {
                in_connection = false;
            }
            continue;
        }

        if (!in_connection) {
            continue;
        }

        std::string key;
        std::string value;
        if (!SplitKeyValue(line, &key, &value)) {
            continue;
        }

        if (key == "name") {
            ParseString(value, &current.name);
        } else if (key == "host") {
            ParseString(value, &current.host);
        } else if (key == "port") {
            ParseInt(value, &current.port);
        } else if (key == "database") {
            ParseString(value, &current.database);
        } else if (key == "username") {
            ParseString(value, &current.username);
        } else if (key == "credential_id") {
            ParseString(value, &current.credentialId);
        } else if (key == "ssl_mode") {
            ParseString(value, &current.sslMode);
        } else if (key == "backend") {
            ParseString(value, &current.backend);
        } else if (key == "fixture_path" || key == "fixture") {
            ParseString(value, &current.fixturePath);
        }
    }

    flush_current();
    return true;
}

} // namespace scratchrobin
