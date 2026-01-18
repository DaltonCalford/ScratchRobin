#ifndef SCRATCHROBIN_CONFIG_H
#define SCRATCHROBIN_CONFIG_H

#include <string>
#include <vector>

#include "connection_manager.h"

namespace scratchrobin {

struct AppConfig {
    std::string theme = "system";
    std::string fontFamily = "default";
    int fontSize = 11;
    int historyMaxItems = 2000;
    NetworkOptions network;
};

class ConfigStore {
public:
    bool LoadAppConfig(const std::string& path, AppConfig* outConfig);
    bool LoadConnections(const std::string& path, std::vector<ConnectionProfile>* outConnections);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONFIG_H
