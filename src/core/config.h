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
};

class ConfigStore {
public:
    bool LoadAppConfig(const std::string& path, AppConfig* outConfig);
    bool LoadConnections(const std::string& path, std::vector<ConnectionProfile>* outConnections);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONFIG_H
