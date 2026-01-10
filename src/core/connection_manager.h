#ifndef SCRATCHROBIN_CONNECTION_MANAGER_H
#define SCRATCHROBIN_CONNECTION_MANAGER_H

#include <string>

namespace scratchrobin {

struct ConnectionProfile {
    std::string name;
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string credentialId;
};

class ConnectionManager {
public:
    ConnectionManager() = default;

    bool Connect(const ConnectionProfile& profile);
    void Disconnect();
    bool IsConnected() const;

private:
    bool connected_ = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_MANAGER_H
