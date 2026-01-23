#ifndef SCRATCHROBIN_CONNECTION_MANAGER_H
#define SCRATCHROBIN_CONNECTION_MANAGER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/connection_backend.h"
#include "core/job_queue.h"
#include "core/query_types.h"

namespace scratchrobin {

class CredentialStore;
class ConnectionBackend;

struct ConnectionProfile {
    std::string name;
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string credentialId;
    std::string sslMode;
    std::string backend;
    std::string fixturePath;
};

struct NetworkOptions {
    int connectTimeoutMs = 5000;
    int queryTimeoutMs = 0;
    int readTimeoutMs = 30000;
    int writeTimeoutMs = 30000;
};

class ConnectionManager {
public:
    ConnectionManager();
    explicit ConnectionManager(std::unique_ptr<CredentialStore> credentialStore);
    ~ConnectionManager();

    bool Connect(const ConnectionProfile& profile);
    void Disconnect();
    bool IsConnected() const;
    bool ExecuteQuery(const std::string& sql, QueryResult* outResult);
    bool ExecuteStatement(const std::string& sql, int64_t* rowsAffected);
    bool BeginTransaction();
    bool Commit();
    bool Rollback();
    const std::string& LastError() const;

    void SetCredentialStore(std::unique_ptr<CredentialStore> credentialStore);
    void SetNetworkOptions(const NetworkOptions& options);
    void SetAutoCommit(bool enabled);
    bool IsAutoCommit() const { return auto_commit_; }
    bool IsInTransaction() const { return in_transaction_; }
    bool CancelActive();
    BackendCapabilities Capabilities() const;

    using SimpleCallback = std::function<void(bool, const std::string&)>;
    using QueryCallback = std::function<void(bool, QueryResult, const std::string&)>;

    JobHandle ConnectAsync(const ConnectionProfile& profile, SimpleCallback callback);
    JobHandle ExecuteQueryAsync(const std::string& sql, QueryCallback callback);
    JobHandle BeginTransactionAsync(SimpleCallback callback);
    JobHandle CommitAsync(SimpleCallback callback);
    JobHandle RollbackAsync(SimpleCallback callback);

private:
    std::unique_ptr<ConnectionBackend> CreateBackendForProfile(const ConnectionProfile& profile,
                                                               std::string* error);

    std::string last_error_;
    NetworkOptions network_options_;
    std::unique_ptr<CredentialStore> credential_store_;
    std::shared_ptr<ConnectionBackend> backend_;
    JobQueue job_queue_;
    mutable std::recursive_mutex state_mutex_;
    bool auto_commit_ = true;
    bool in_transaction_ = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_MANAGER_H
