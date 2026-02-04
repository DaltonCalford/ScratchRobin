/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CONFIG_H
#define SCRATCHROBIN_CONFIG_H

#include <string>
#include <vector>

#include "connection_manager.h"

namespace scratchrobin {

struct WindowChromeConfig {
    bool showMenu = true;
    bool showIconBar = true;
    bool replicateMenu = true;
    bool replicateIconBar = false;
};

struct UiChromeConfig {
    WindowChromeConfig mainWindow;
    WindowChromeConfig sqlEditor;
    WindowChromeConfig monitoring;
    WindowChromeConfig usersRoles;
    WindowChromeConfig diagram;
};

struct StartupConfig {
    bool enabled = true;
    bool showProgress = true;
};

// AI Provider Configuration
struct AiConfig {
    std::string provider = "openai";  // openai, anthropic, ollama, gemini
    std::string api_key;
    std::string api_endpoint;
    std::string model_name;
    float temperature = 0.3f;
    int max_tokens = 4096;
    int timeout_seconds = 60;
    bool enable_schema_design = true;
    bool enable_query_optimization = true;
    bool enable_code_generation = true;
    bool enable_documentation = true;
};

struct AppConfig {
    std::string theme = "system";
    std::string fontFamily = "default";
    int fontSize = 11;
    int historyMaxItems = 2000;
    int rowLimit = 200;
    bool enableSuggestions = true;
    UiChromeConfig chrome;
    StartupConfig startup;
    NetworkOptions network;
    AiConfig ai;
};

class ConfigStore {
public:
    bool LoadAppConfig(const std::string& path, AppConfig* outConfig);
    bool SaveAppConfig(const std::string& path, const AppConfig& config);
    bool LoadConnections(const std::string& path, std::vector<ConnectionProfile>* outConnections);
    
    // AI Configuration helpers
    bool LoadAiConfig(const std::string& path, AiConfig* outConfig);
    bool SaveAiConfig(const std::string& path, const AiConfig& config);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONFIG_H
