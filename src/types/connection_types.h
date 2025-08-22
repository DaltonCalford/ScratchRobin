#ifndef SCRATCHROBIN_CONNECTION_TYPES_H
#define SCRATCHROBIN_CONNECTION_TYPES_H

#include <string>
#include <chrono>

namespace scratchrobin {

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    ERROR
};

struct ConnectionStats {
    std::chrono::system_clock::time_point connectedAt;
    std::chrono::system_clock::time_point lastActivity;
    size_t queriesExecuted;
    size_t bytesSent;
    size_t bytesReceived;
    double averageResponseTime; // in milliseconds
};

struct ConnectionOptions {
    bool autoReconnect = true;
    int reconnectAttempts = 3;
    std::chrono::milliseconds reconnectDelay = std::chrono::milliseconds(1000);
    std::chrono::milliseconds connectionTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds queryTimeout = std::chrono::milliseconds(300000);
    bool sslEnabled = false;
    std::string sslCertFile;
    std::string sslKeyFile;
    std::string sslCAFile;
    bool sslVerifyPeer = true;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_TYPES_H
