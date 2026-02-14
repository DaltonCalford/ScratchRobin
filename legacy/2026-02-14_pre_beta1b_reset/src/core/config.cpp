/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
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

std::vector<std::string> SplitCommaList(const std::string& input) {
    std::vector<std::string> out;
    std::string token;
    for (char c : input) {
        if (c == ',') {
            token = Trim(token);
            if (!token.empty()) {
                out.push_back(token);
            }
            token.clear();
        } else {
            token.push_back(c);
        }
    }
    token = Trim(token);
    if (!token.empty()) {
        out.push_back(token);
    }
    return out;
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

bool ParseBool(const std::string& value, bool* out) {
    std::string normalized = Trim(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "true" || normalized == "yes" || normalized == "1") {
        *out = true;
        return true;
    }
    if (normalized == "false" || normalized == "no" || normalized == "0") {
        *out = false;
        return true;
    }
    return false;
}

StatusRequestKind ParseStatusKind(const std::string& input) {
    std::string value = Trim(input);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "connection" || value == "connection_info") {
        return StatusRequestKind::ConnectionInfo;
    }
    if (value == "database" || value == "db" || value == "database_info") {
        return StatusRequestKind::DatabaseInfo;
    }
    if (value == "statistics" || value == "stats") {
        return StatusRequestKind::Statistics;
    }
    return StatusRequestKind::ServerInfo;
}

const char* StatusKindToString(StatusRequestKind kind) {
    switch (kind) {
        case StatusRequestKind::ConnectionInfo: return "connection";
        case StatusRequestKind::DatabaseInfo: return "database";
        case StatusRequestKind::Statistics: return "statistics";
        case StatusRequestKind::ServerInfo:
        default: return "server";
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
        } else if (section == "ui.window.main") {
            if (key == "show_menu") {
                ParseBool(value, &outConfig->chrome.mainWindow.showMenu);
            } else if (key == "show_iconbar") {
                ParseBool(value, &outConfig->chrome.mainWindow.showIconBar);
            } else if (key == "replicate_menu") {
                ParseBool(value, &outConfig->chrome.mainWindow.replicateMenu);
            } else if (key == "replicate_iconbar") {
                ParseBool(value, &outConfig->chrome.mainWindow.replicateIconBar);
            }
        } else if (section == "ui.window.sql_editor") {
            if (key == "show_menu") {
                ParseBool(value, &outConfig->chrome.sqlEditor.showMenu);
            } else if (key == "show_iconbar") {
                ParseBool(value, &outConfig->chrome.sqlEditor.showIconBar);
            } else if (key == "replicate_menu") {
                ParseBool(value, &outConfig->chrome.sqlEditor.replicateMenu);
            } else if (key == "replicate_iconbar") {
                ParseBool(value, &outConfig->chrome.sqlEditor.replicateIconBar);
            }
        } else if (section == "ui.window.monitoring") {
            if (key == "show_menu") {
                ParseBool(value, &outConfig->chrome.monitoring.showMenu);
            } else if (key == "show_iconbar") {
                ParseBool(value, &outConfig->chrome.monitoring.showIconBar);
            } else if (key == "replicate_menu") {
                ParseBool(value, &outConfig->chrome.monitoring.replicateMenu);
            } else if (key == "replicate_iconbar") {
                ParseBool(value, &outConfig->chrome.monitoring.replicateIconBar);
            }
        } else if (section == "ui.window.users_roles") {
            if (key == "show_menu") {
                ParseBool(value, &outConfig->chrome.usersRoles.showMenu);
            } else if (key == "show_iconbar") {
                ParseBool(value, &outConfig->chrome.usersRoles.showIconBar);
            } else if (key == "replicate_menu") {
                ParseBool(value, &outConfig->chrome.usersRoles.replicateMenu);
            } else if (key == "replicate_iconbar") {
                ParseBool(value, &outConfig->chrome.usersRoles.replicateIconBar);
            }
        } else if (section == "ui.window.diagram") {
            if (key == "show_menu") {
                ParseBool(value, &outConfig->chrome.diagram.showMenu);
            } else if (key == "show_iconbar") {
                ParseBool(value, &outConfig->chrome.diagram.showIconBar);
            } else if (key == "replicate_menu") {
                ParseBool(value, &outConfig->chrome.diagram.replicateMenu);
            } else if (key == "replicate_iconbar") {
                ParseBool(value, &outConfig->chrome.diagram.replicateIconBar);
            }
        } else if (section == "editor") {
            if (key == "history_max_items") {
                ParseInt(value, &outConfig->historyMaxItems);
            } else if (key == "row_limit") {
                ParseInt(value, &outConfig->rowLimit);
            } else if (key == "enable_suggestions") {
                ParseBool(value, &outConfig->enableSuggestions);
            }
        } else if (section == "startup") {
            if (key == "enabled") {
                ParseBool(value, &outConfig->startup.enabled);
            } else if (key == "show_progress") {
                ParseBool(value, &outConfig->startup.showProgress);
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
            } else if (key == "stream_window_bytes") {
                int tmp = 0;
                if (ParseInt(value, &tmp) && tmp >= 0) {
                    outConfig->network.streamWindowBytes = static_cast<uint32_t>(tmp);
                }
            } else if (key == "stream_chunk_bytes") {
                int tmp = 0;
                if (ParseInt(value, &tmp) && tmp >= 0) {
                    outConfig->network.streamChunkBytes = static_cast<uint32_t>(tmp);
                }
            }
        } else if (section == "ai") {
            if (key == "provider") {
                ParseString(value, &outConfig->ai.provider);
            } else if (key == "api_endpoint") {
                ParseString(value, &outConfig->ai.api_endpoint);
            } else if (key == "model_name") {
                ParseString(value, &outConfig->ai.model_name);
            } else if (key == "temperature") {
                float temp;
                // Parse float value
                try {
                    temp = std::stof(value);
                    outConfig->ai.temperature = temp;
                } catch (...) {}
            } else if (key == "max_tokens") {
                ParseInt(value, &outConfig->ai.max_tokens);
            } else if (key == "timeout_seconds") {
                ParseInt(value, &outConfig->ai.timeout_seconds);
            } else if (key == "enable_schema_design") {
                ParseBool(value, &outConfig->ai.enable_schema_design);
            } else if (key == "enable_query_optimization") {
                ParseBool(value, &outConfig->ai.enable_query_optimization);
            } else if (key == "enable_code_generation") {
                ParseBool(value, &outConfig->ai.enable_code_generation);
            } else if (key == "enable_documentation") {
                ParseBool(value, &outConfig->ai.enable_documentation);
            }
        }
    }

    return true;
}

bool ConfigStore::SaveAppConfig(const std::string& path, const AppConfig& config) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    
    out << "# ScratchRobin Configuration\n\n";
    
    // UI section
    out << "[ui]\n";
    out << "theme = \"" << config.theme << "\"\n";
    out << "font_family = \"" << config.fontFamily << "\"\n";
    out << "font_size = " << config.fontSize << "\n\n";
    
    // Editor section
    out << "[editor]\n";
    out << "history_max_items = " << config.historyMaxItems << "\n";
    out << "row_limit = " << config.rowLimit << "\n";
    out << "enable_suggestions = " << (config.enableSuggestions ? "true" : "false") << "\n\n";
    
    // Network section
    out << "[network]\n";
    out << "connect_timeout_ms = " << config.network.connectTimeoutMs << "\n";
    out << "query_timeout_ms = " << config.network.queryTimeoutMs << "\n";
    out << "read_timeout_ms = " << config.network.readTimeoutMs << "\n";
    out << "write_timeout_ms = " << config.network.writeTimeoutMs << "\n";
    out << "stream_window_bytes = " << config.network.streamWindowBytes << "\n";
    out << "stream_chunk_bytes = " << config.network.streamChunkBytes << "\n\n";

    // AI section
    out << "[ai]\n";
    out << "provider = \"" << config.ai.provider << "\"\n";
    out << "api_endpoint = \"" << config.ai.api_endpoint << "\"\n";
    out << "model_name = \"" << config.ai.model_name << "\"\n";
    out << "temperature = " << config.ai.temperature << "\n";
    out << "max_tokens = " << config.ai.max_tokens << "\n";
    out << "timeout_seconds = " << config.ai.timeout_seconds << "\n";
    out << "enable_schema_design = " << (config.ai.enable_schema_design ? "true" : "false") << "\n";
    out << "enable_query_optimization = " << (config.ai.enable_query_optimization ? "true" : "false") << "\n";
    out << "enable_code_generation = " << (config.ai.enable_code_generation ? "true" : "false") << "\n";
    out << "enable_documentation = " << (config.ai.enable_documentation ? "true" : "false") << "\n\n";
    
    return true;
}

bool ConfigStore::LoadAiConfig(const std::string& path, AiConfig* outConfig) {
    if (!outConfig) {
        return false;
    }
    *outConfig = AiConfig{};
    
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(StripComment(line));
        if (line.empty()) continue;
        
        std::string key;
        std::string value;
        if (!SplitKeyValue(line, &key, &value)) continue;
        
        if (key == "provider") {
            ParseString(value, &outConfig->provider);
        } else if (key == "api_key") {
            ParseString(value, &outConfig->api_key);
        } else if (key == "api_endpoint") {
            ParseString(value, &outConfig->api_endpoint);
        } else if (key == "model_name") {
            ParseString(value, &outConfig->model_name);
        } else if (key == "temperature") {
            try {
                outConfig->temperature = std::stof(value);
            } catch (...) {}
        } else if (key == "max_tokens") {
            ParseInt(value, &outConfig->max_tokens);
        } else if (key == "timeout_seconds") {
            ParseInt(value, &outConfig->timeout_seconds);
        } else if (key == "enable_schema_design") {
            ParseBool(value, &outConfig->enable_schema_design);
        } else if (key == "enable_query_optimization") {
            ParseBool(value, &outConfig->enable_query_optimization);
        } else if (key == "enable_code_generation") {
            ParseBool(value, &outConfig->enable_code_generation);
        } else if (key == "enable_documentation") {
            ParseBool(value, &outConfig->enable_documentation);
        }
    }
    
    return true;
}

bool ConfigStore::SaveAiConfig(const std::string& path, const AiConfig& config) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    
    out << "# AI Provider Configuration\n\n";
    out << "provider = \"" << config.provider << "\"\n";
    out << "api_key = \"" << config.api_key << "\"\n";
    out << "api_endpoint = \"" << config.api_endpoint << "\"\n";
    out << "model_name = \"" << config.model_name << "\"\n";
    out << "temperature = " << config.temperature << "\n";
    out << "max_tokens = " << config.max_tokens << "\n";
    out << "timeout_seconds = " << config.timeout_seconds << "\n";
    out << "enable_schema_design = " << (config.enable_schema_design ? "true" : "false") << "\n";
    out << "enable_query_optimization = " << (config.enable_query_optimization ? "true" : "false") << "\n";
    out << "enable_code_generation = " << (config.enable_code_generation ? "true" : "false") << "\n";
    out << "enable_documentation = " << (config.enable_documentation ? "true" : "false") << "\n";
    
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
        } else if (key == "application_name") {
            ParseString(value, &current.applicationName);
        } else if (key == "role") {
            ParseString(value, &current.role);
        } else if (key == "ssl_mode") {
            ParseString(value, &current.sslMode);
        } else if (key == "ssl_root_cert") {
            ParseString(value, &current.sslRootCert);
        } else if (key == "ssl_cert") {
            ParseString(value, &current.sslCert);
        } else if (key == "ssl_key") {
            ParseString(value, &current.sslKey);
        } else if (key == "ssl_password") {
            ParseString(value, &current.sslPassword);
        } else if (key == "options") {
            ParseString(value, &current.options);
        } else if (key == "backend") {
            ParseString(value, &current.backend);
        } else if (key == "fixture_path" || key == "fixture") {
            ParseString(value, &current.fixturePath);
        } else if (key == "ipc_path") {
            ParseString(value, &current.ipcPath);
        } else if (key == "status_auto_poll") {
            ParseBool(value, &current.statusAutoPollEnabled);
        } else if (key == "status_poll_interval_ms") {
            ParseInt(value, &current.statusPollIntervalMs);
        } else if (key == "status_request_default") {
            std::string tmp;
            if (ParseString(value, &tmp)) {
                current.statusDefaultKind = ParseStatusKind(tmp);
            }
        } else if (key == "status_category_order") {
            std::string tmp;
            if (ParseString(value, &tmp)) {
                current.statusCategoryOrder = SplitCommaList(tmp);
            }
        } else if (key == "status_category_filter") {
            ParseString(value, &current.statusCategoryFilter);
        } else if (key == "status_diff_enabled") {
            ParseBool(value, &current.statusDiffEnabled);
        } else if (key == "status_diff_ignore_unchanged") {
            ParseBool(value, &current.statusDiffIgnoreUnchanged);
        } else if (key == "status_diff_ignore_empty") {
            ParseBool(value, &current.statusDiffIgnoreEmpty);
        }
    }

    flush_current();
    return true;
}

bool ConfigStore::SaveConnections(const std::string& path,
                                  const std::vector<ConnectionProfile>& connections) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    out << "# ScratchRobin connections\n\n";
    for (const auto& conn : connections) {
        out << "[[connection]]\n";
        out << "name = \"" << conn.name << "\"\n";
        out << "host = \"" << conn.host << "\"\n";
        out << "port = " << conn.port << "\n";
        out << "database = \"" << conn.database << "\"\n";
        out << "username = \"" << conn.username << "\"\n";
        out << "credential_id = \"" << conn.credentialId << "\"\n";
        out << "application_name = \"" << conn.applicationName << "\"\n";
        if (!conn.role.empty()) {
            out << "role = \"" << conn.role << "\"\n";
        }
        if (!conn.sslMode.empty()) {
            out << "ssl_mode = \"" << conn.sslMode << "\"\n";
        }
        if (!conn.sslRootCert.empty()) {
            out << "ssl_root_cert = \"" << conn.sslRootCert << "\"\n";
        }
        if (!conn.sslCert.empty()) {
            out << "ssl_cert = \"" << conn.sslCert << "\"\n";
        }
        if (!conn.sslKey.empty()) {
            out << "ssl_key = \"" << conn.sslKey << "\"\n";
        }
        if (!conn.sslPassword.empty()) {
            out << "ssl_password = \"" << conn.sslPassword << "\"\n";
        }
        if (!conn.options.empty()) {
            out << "options = \"" << conn.options << "\"\n";
        }
        if (!conn.backend.empty()) {
            out << "backend = \"" << conn.backend << "\"\n";
        }
        if (!conn.fixturePath.empty()) {
            out << "fixture_path = \"" << conn.fixturePath << "\"\n";
        }
        if (!conn.ipcPath.empty()) {
            out << "ipc_path = \"" << conn.ipcPath << "\"\n";
        }
        out << "status_auto_poll = " << (conn.statusAutoPollEnabled ? "true" : "false") << "\n";
        out << "status_poll_interval_ms = " << conn.statusPollIntervalMs << "\n";
        out << "status_request_default = \"" << StatusKindToString(conn.statusDefaultKind) << "\"\n";
        if (!conn.statusCategoryOrder.empty()) {
            std::string joined;
            for (size_t i = 0; i < conn.statusCategoryOrder.size(); ++i) {
                if (i > 0) {
                    joined += ", ";
                }
                joined += conn.statusCategoryOrder[i];
            }
            out << "status_category_order = \"" << joined << "\"\n";
        }
        if (!conn.statusCategoryFilter.empty()) {
            out << "status_category_filter = \"" << conn.statusCategoryFilter << "\"\n";
        }
        out << "status_diff_enabled = " << (conn.statusDiffEnabled ? "true" : "false") << "\n";
        out << "status_diff_ignore_unchanged = "
            << (conn.statusDiffIgnoreUnchanged ? "true" : "false") << "\n";
        out << "status_diff_ignore_empty = " << (conn.statusDiffIgnoreEmpty ? "true" : "false") << "\n";
        out << "\n";
    }
    return true;
}

} // namespace scratchrobin
