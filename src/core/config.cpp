#include "config.h"

namespace scratchrobin {

bool ConfigStore::LoadAppConfig(const std::string&, AppConfig* outConfig) {
    if (!outConfig) {
        return false;
    }
    *outConfig = AppConfig{};
    return true;
}

bool ConfigStore::LoadConnections(const std::string&, std::vector<ConnectionProfile>* outConnections) {
    if (!outConnections) {
        return false;
    }
    outConnections->clear();
    return true;
}

} // namespace scratchrobin
