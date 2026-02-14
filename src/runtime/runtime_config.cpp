#include "runtime/runtime_config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>

namespace scratchrobin::runtime {

namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string StripComment(const std::string& value) {
    const auto pos = value.find('#');
    if (pos == std::string::npos) {
        return value;
    }
    return value.substr(0, pos);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool SplitKeyValue(const std::string& line, std::string* out_key, std::string* out_value) {
    const auto pos = line.find('=');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= line.size()) {
        return false;
    }
    *out_key = Trim(line.substr(0, pos));
    *out_value = Trim(line.substr(pos + 1));
    return true;
}

std::optional<std::string> ParseSectionName(const std::string& line) {
    if (line.size() >= 4 && line.rfind("[[", 0) == 0 && line.substr(line.size() - 2) == "]]") {
        return Trim(line.substr(2, line.size() - 4));
    }
    if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
        return Trim(line.substr(1, line.size() - 2));
    }
    return std::nullopt;
}

bool ParseBool(const std::string& value, bool* out) {
    const std::string lowered = ToLower(Trim(value));
    if (lowered == "true" || lowered == "1" || lowered == "yes") {
        *out = true;
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no") {
        *out = false;
        return true;
    }
    return false;
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

bool ParseString(const std::string& value, std::string* out) {
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        *out = value.substr(1, value.size() - 2);
        return true;
    }
    if (!value.empty()) {
        *out = value;
        return true;
    }
    return false;
}

void AddWarning(std::vector<ConfigWarning>* warnings, std::string code, std::string message) {
    if (warnings == nullptr) {
        return;
    }
    warnings->push_back(ConfigWarning{std::move(code), std::move(message)});
}

}  // namespace

ConnectionMode ParseConnectionMode(const std::string& value) {
    const std::string lowered = ToLower(Trim(value));
    if (lowered == "ipc") {
        return ConnectionMode::Ipc;
    }
    if (lowered == "embedded") {
        return ConnectionMode::Embedded;
    }
    return ConnectionMode::Network;
}

std::string ToString(ConnectionMode mode) {
    switch (mode) {
        case ConnectionMode::Ipc: return "ipc";
        case ConnectionMode::Embedded: return "embedded";
        case ConnectionMode::Network:
        default: return "network";
    }
}

std::string ToString(ConfigSource source) {
    switch (source) {
        case ConfigSource::UserConfig: return "user_config";
        case ConfigSource::ExampleFallback: return "example_fallback";
        case ConfigSource::Defaults:
        default: return "defaults";
    }
}

bool ConfigStore::LoadAppConfig(const std::string& path,
                                AppConfig* out_config,
                                std::vector<ConfigWarning>* warnings) const {
    if (out_config == nullptr) {
        return false;
    }
    *out_config = AppConfig{};

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        AddWarning(warnings, "CFG-1001", "app config not found: " + path);
        return false;
    }

    std::string section;
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(StripComment(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            const auto parsed = ParseSectionName(line);
            section = parsed.has_value() ? ToLower(*parsed) : "";
            continue;
        }

        std::string key;
        std::string value;
        if (!SplitKeyValue(line, &key, &value)) {
            AddWarning(warnings, "CFG-1002", "invalid app config line: " + line);
            continue;
        }
        key = ToLower(key);

        if (section == "startup") {
            if (key == "enabled") {
                ParseBool(value, &out_config->startup_enabled);
            } else if (key == "show_progress") {
                ParseBool(value, &out_config->startup_show_progress);
            }
            continue;
        }

        if (section == "network") {
            if (key == "connect_timeout_ms") {
                ParseInt(value, &out_config->connect_timeout_ms);
            } else if (key == "query_timeout_ms") {
                ParseInt(value, &out_config->query_timeout_ms);
            } else if (key == "read_timeout_ms") {
                ParseInt(value, &out_config->read_timeout_ms);
            } else if (key == "write_timeout_ms") {
                ParseInt(value, &out_config->write_timeout_ms);
            }
            continue;
        }

        if (section == "metadata") {
            if (key == "use_fixture") {
                ParseBool(value, &out_config->metadata_use_fixture);
            } else if (key == "fixture_path") {
                ParseString(value, &out_config->metadata_fixture_path);
            }
            continue;
        }

        if (section == "runtime") {
            if (key == "mandatory_backends") {
                ParseBool(value, &out_config->mandatory_backends);
            }
            continue;
        }
    }

    return true;
}

bool ConfigStore::LoadConnections(const std::string& path,
                                  std::vector<ConnectionProfile>* out_connections,
                                  std::vector<ConfigWarning>* warnings) const {
    if (out_connections == nullptr) {
        return false;
    }
    out_connections->clear();

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        AddWarning(warnings, "CFG-1101", "connections config not found: " + path);
        return false;
    }

    std::string section;
    bool in_connection = false;
    ConnectionProfile current;

    auto flush = [&]() {
        if (!in_connection) {
            return;
        }
        if (current.name.empty() || current.backend.empty() || current.database.empty() || current.username.empty()) {
            AddWarning(warnings, "CFG-1102", "skipped connection with missing required fields");
        } else if (current.mode == ConnectionMode::Ipc && current.ipc_path.empty()) {
            AddWarning(warnings, "CFG-1103", "skipped ipc connection missing ipc_path: " + current.name);
        } else {
            out_connections->push_back(current);
        }
        current = ConnectionProfile{};
    };

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(StripComment(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            const auto parsed = ParseSectionName(line);
            section = parsed.has_value() ? ToLower(*parsed) : "";
            if (section == "connection") {
                flush();
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
            AddWarning(warnings, "CFG-1104", "invalid connection line: " + line);
            continue;
        }
        key = ToLower(key);

        if (key == "name") {
            ParseString(value, &current.name);
        } else if (key == "backend") {
            ParseString(value, &current.backend);
        } else if (key == "mode") {
            std::string parsed_mode;
            if (ParseString(value, &parsed_mode)) {
                current.mode = ParseConnectionMode(parsed_mode);
            }
        } else if (key == "host") {
            ParseString(value, &current.host);
        } else if (key == "port") {
            ParseInt(value, &current.port);
        } else if (key == "ipc_path") {
            ParseString(value, &current.ipc_path);
        } else if (key == "database") {
            ParseString(value, &current.database);
        } else if (key == "username") {
            ParseString(value, &current.username);
        } else if (key == "credential_id") {
            ParseString(value, &current.credential_id);
        }
    }

    flush();

    if (out_connections->empty()) {
        AddWarning(warnings, "CFG-1105", "no valid connection profiles loaded from " + path);
    }
    return true;
}

ConfigBundle ConfigStore::LoadWithFallback(const std::string& app_path,
                                           const std::string& app_example_path,
                                           const std::string& connections_path,
                                           const std::string& connections_example_path) const {
    ConfigBundle bundle;

    AppConfig app;
    std::vector<ConnectionProfile> profiles;
    const bool app_ok = LoadAppConfig(app_path, &app, &bundle.warnings);
    const bool con_ok = LoadConnections(connections_path, &profiles, &bundle.warnings);
    if (app_ok && con_ok) {
        bundle.app = app;
        bundle.connections = std::move(profiles);
        bundle.source = ConfigSource::UserConfig;
        return bundle;
    }

    AppConfig app_example;
    std::vector<ConnectionProfile> profiles_example;
    const bool app_example_ok = LoadAppConfig(app_example_path, &app_example, &bundle.warnings);
    const bool con_example_ok = LoadConnections(connections_example_path, &profiles_example, &bundle.warnings);
    if (app_example_ok && con_example_ok) {
        bundle.app = app_example;
        bundle.connections = std::move(profiles_example);
        bundle.source = ConfigSource::ExampleFallback;
        return bundle;
    }

    bundle.app = AppConfig{};
    bundle.connections.clear();
    bundle.source = ConfigSource::Defaults;
    AddWarning(&bundle.warnings, "CFG-1999", "using runtime defaults due to config load failures");
    return bundle;
}

}  // namespace scratchrobin::runtime

