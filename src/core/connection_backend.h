#ifndef SCRATCHROBIN_CONNECTION_BACKEND_H
#define SCRATCHROBIN_CONNECTION_BACKEND_H

#include <memory>
#include <string>

#include "core/query_types.h"

namespace scratchrobin {

struct BackendCapabilities {
    bool supportsCancel = false;
    bool supportsTransactions = true;
    bool supportsPaging = true;
    bool supportsExplain = false;
    bool supportsSblr = false;
    bool supportsDdlExtract = false;
    bool supportsDependencies = false;
    bool supportsUserAdmin = false;
    bool supportsRoleAdmin = false;
    bool supportsGroupAdmin = false;
};

struct BackendConfig {
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string password;
    std::string sslMode;
    std::string fixturePath;
    int connectTimeoutMs = 5000;
    int queryTimeoutMs = 0;
    int readTimeoutMs = 30000;
    int writeTimeoutMs = 30000;
    BackendCapabilities capabilities;
};

class ConnectionBackend {
public:
    virtual ~ConnectionBackend() = default;

    virtual bool Connect(const BackendConfig& config, std::string* error) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;
    virtual bool ExecuteQuery(const std::string& sql, QueryResult* outResult,
                              std::string* error) = 0;
    virtual bool BeginTransaction(std::string* error) = 0;
    virtual bool Commit(std::string* error) = 0;
    virtual bool Rollback(std::string* error) = 0;
    virtual bool Cancel(std::string* error) = 0;
    virtual BackendCapabilities Capabilities() const = 0;
    virtual std::string BackendName() const = 0;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_BACKEND_H
