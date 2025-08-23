#ifndef SCRATCHROBIN_CONNECTION_MANAGER_H
#define SCRATCHROBIN_CONNECTION_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "types/result.h"

namespace scratchrobin {

struct ConnectionInfo {
    std::string id;
    std::string name;
    std::string host;
    int port;
    std::string database;
    std::string user;
    std::string connectionString;
    bool isConnected;
    std::string lastError;
};

class IConnection {
public:
    virtual ~IConnection() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual Result<std::vector<std::unordered_map<std::string, std::string>>> executeQuery(const std::string& query) = 0;
    virtual std::string getDatabaseName() const = 0;
    virtual const ConnectionInfo& getInfo() const = 0;
};

class Connection;

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    // Connection management
    std::string connect(const ConnectionInfo& info);
    bool disconnect(const std::string& connectionId);
    bool isConnected(const std::string& connectionId) const;

    // Connection pool management
    size_t getPoolSize() const;
    size_t getActiveConnections() const;
    void setMaxPoolSize(size_t size);
    size_t getMaxPoolSize() const;

    // Connection retrieval
    std::shared_ptr<Connection> getConnection(const std::string& connectionId);
    std::vector<ConnectionInfo> getAllConnections() const;

    // Event callbacks
    using ConnectionCallback = std::function<void(const std::string&)>;
    void setConnectionCallback(ConnectionCallback callback);
    void setDisconnectionCallback(ConnectionCallback callback);

    // Utility methods
    void shutdown();
    void cleanup();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy and assignment
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_MANAGER_H
