#include "connection_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>

namespace scratchrobin {

// Forward declaration for Connection class
class Connection {
public:
    Connection(const ConnectionInfo& info);
    ~Connection();

    bool connect();
    void disconnect();
    bool isConnected() const;
    const ConnectionInfo& getInfo() const;

private:
    ConnectionInfo info_;
    bool connected_;
};

Connection::Connection(const ConnectionInfo& info)
    : info_(info)
    , connected_(false) {
}

Connection::~Connection() {
    if (connected_) {
        disconnect();
    }
}

bool Connection::connect() {
    try {
        // In a real implementation, this would connect to ScratchBird
        // For now, we'll simulate a connection
        Logger::info("Connecting to database: " + info_.database + " at " + info_.host + ":" + std::to_string(info_.port));

        // Simulate connection time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        connected_ = true;
        Logger::info("Successfully connected to database: " + info_.database);
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to connect to database: " + std::string(e.what()));
        return false;
    }
}

void Connection::disconnect() {
    if (connected_) {
        Logger::info("Disconnecting from database: " + info_.database);
        connected_ = false;
        Logger::info("Disconnected from database: " + info_.database);
    }
}

bool Connection::isConnected() const {
    return connected_;
}

const ConnectionInfo& Connection::getInfo() const {
    return info_;
}

class ConnectionManager::Impl {
public:
    Impl()
        : maxPoolSize_(10)
        , nextConnectionId_(1) {
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Connection>> connections_;
    size_t maxPoolSize_;
    std::atomic<size_t> nextConnectionId_;
    ConnectionCallback connectionCallback_;
    ConnectionCallback disconnectionCallback_;
};

ConnectionManager::ConnectionManager()
    : impl_(std::make_unique<Impl>()) {
    Logger::info("ConnectionManager initialized with max pool size: " + std::to_string(impl_->maxPoolSize_));
}

ConnectionManager::~ConnectionManager() {
    shutdown();
    Logger::info("ConnectionManager destroyed");
}

std::string ConnectionManager::connect(const ConnectionInfo& info) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Check if we're at the pool limit
    if (impl_->connections_.size() >= impl_->maxPoolSize_) {
        Logger::warn("Connection pool is full (size: " + std::to_string(impl_->connections_.size()) + ")");
        return "";
    }

    // Generate unique connection ID
    std::string connectionId;
    do {
        connectionId = "conn_" + std::to_string(impl_->nextConnectionId_++);
    } while (impl_->connections_.find(connectionId) != impl_->connections_.end());

    // Create connection info
    ConnectionInfo connectionInfo = info;
    connectionInfo.id = connectionId;
    connectionInfo.isConnected = false;

    // Create and connect
    auto connection = std::make_shared<Connection>(connectionInfo);
    if (connection->connect()) {
        connectionInfo.isConnected = true;
        impl_->connections_[connectionId] = connection;

        Logger::info("Connection added to pool: " + connectionId);

        // Notify callback
        if (impl_->connectionCallback_) {
            impl_->connectionCallback_(connectionId);
        }

        return connectionId;
    } else {
        Logger::error("Failed to establish connection: " + connectionId);
        return "";
    }
}

bool ConnectionManager::disconnect(const std::string& connectionId) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->connections_.find(connectionId);
    if (it != impl_->connections_.end()) {
        auto connection = it->second;
        connection->disconnect();

        impl_->connections_.erase(it);

        Logger::info("Connection removed from pool: " + connectionId);

        // Notify callback
        if (impl_->disconnectionCallback_) {
            impl_->disconnectionCallback_(connectionId);
        }

        return true;
    }

    Logger::warn("Connection not found: " + connectionId);
    return false;
}

bool ConnectionManager::isConnected(const std::string& connectionId) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->connections_.find(connectionId);
    if (it != impl_->connections_.end()) {
        return it->second->isConnected();
    }

    return false;
}

size_t ConnectionManager::getPoolSize() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->connections_.size();
}

size_t ConnectionManager::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    return std::count_if(impl_->connections_.begin(), impl_->connections_.end(),
        [](const auto& pair) { return pair.second->isConnected(); });
}

void ConnectionManager::setMaxPoolSize(size_t size) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->maxPoolSize_ = size;
    Logger::info("Max connection pool size set to: " + std::to_string(size));
}

size_t ConnectionManager::getMaxPoolSize() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->maxPoolSize_;
}

std::shared_ptr<Connection> ConnectionManager::getConnection(const std::string& connectionId) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->connections_.find(connectionId);
    if (it != impl_->connections_.end()) {
        return it->second;
    }

    return nullptr;
}

std::vector<ConnectionInfo> ConnectionManager::getAllConnections() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::vector<ConnectionInfo> connections;
    connections.reserve(impl_->connections_.size());

    for (const auto& pair : impl_->connections_) {
        connections.push_back(pair.second->getInfo());
    }

    return connections;
}

void ConnectionManager::setConnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->connectionCallback_ = callback;
}

void ConnectionManager::setDisconnectionCallback(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->disconnectionCallback_ = callback;
}

void ConnectionManager::shutdown() {
    Logger::info("ConnectionManager shutdown initiated");

    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Disconnect all connections
    for (auto& pair : impl_->connections_) {
        pair.second->disconnect();
    }

    impl_->connections_.clear();
    Logger::info("ConnectionManager shutdown complete");
}

void ConnectionManager::cleanup() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    // Remove disconnected connections
    auto it = impl_->connections_.begin();
    while (it != impl_->connections_.end()) {
        if (!it->second->isConnected()) {
            it = impl_->connections_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace scratchrobin
