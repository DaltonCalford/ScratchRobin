#include "connection_manager.h"

namespace scratchrobin {

bool ConnectionManager::Connect(const ConnectionProfile&) {
    connected_ = true;
    return connected_;
}

void ConnectionManager::Disconnect() {
    connected_ = false;
}

bool ConnectionManager::IsConnected() const {
    return connected_;
}

} // namespace scratchrobin
