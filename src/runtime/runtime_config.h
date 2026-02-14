#pragma once

#include <string>
#include <vector>

namespace scratchrobin::runtime {

enum class ConnectionMode {
    Network,
    Ipc,
    Embedded,
};

struct ConnectionProfile {
    std::string name;
    std::string backend;
    ConnectionMode mode = ConnectionMode::Network;
    std::string host;
    int port = 0;
    std::string ipc_path;
    std::string database;
    std::string username;
    std::string credential_id;
};

struct AppConfig {
    bool startup_enabled = true;
    bool startup_show_progress = true;

    int connect_timeout_ms = 5000;
    int query_timeout_ms = 0;
    int read_timeout_ms = 30000;
    int write_timeout_ms = 30000;

    bool metadata_use_fixture = false;
    std::string metadata_fixture_path;

    bool mandatory_backends = false;
};

struct ConfigWarning {
    std::string code;
    std::string message;
};

enum class ConfigSource {
    UserConfig,
    ExampleFallback,
    Defaults,
};

struct ConfigBundle {
    AppConfig app;
    std::vector<ConnectionProfile> connections;
    ConfigSource source = ConfigSource::Defaults;
    std::vector<ConfigWarning> warnings;
};

class ConfigStore {
public:
    bool LoadAppConfig(const std::string& path,
                       AppConfig* out_config,
                       std::vector<ConfigWarning>* warnings) const;
    bool LoadConnections(const std::string& path,
                         std::vector<ConnectionProfile>* out_connections,
                         std::vector<ConfigWarning>* warnings) const;

    ConfigBundle LoadWithFallback(const std::string& app_path,
                                  const std::string& app_example_path,
                                  const std::string& connections_path,
                                  const std::string& connections_example_path) const;
};

ConnectionMode ParseConnectionMode(const std::string& value);
std::string ToString(ConnectionMode mode);
std::string ToString(ConfigSource source);

}  // namespace scratchrobin::runtime

