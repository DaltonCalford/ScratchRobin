/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CONNECTION_MANAGER_H
#define SCRATCHROBIN_CONNECTION_MANAGER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/connection_backend.h"
#include "core/copy_types.h"
#include "core/notification_types.h"
#include "core/prepared_types.h"
#include "core/query_options.h"
#include "core/job_queue.h"
#include "core/query_types.h"
#include "core/status_types.h"

namespace scratchrobin {

class CredentialStore;
class ConnectionBackend;

enum class ConnectionMode {
    Network,    // TCP/IP to remote server (default)
    Ipc,        // Unix domain socket / named pipes
    Embedded    // In-process, direct engine access
};

struct ConnectionProfile {
    std::string name;
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string credentialId;
    std::string applicationName;
    std::string role;
    std::string sslMode;
    std::string sslRootCert;
    std::string sslCert;
    std::string sslKey;
    std::string sslPassword;
    std::string options;
    std::string backend;
    std::string fixturePath;
    ConnectionMode mode = ConnectionMode::Network;  // Connection mode
    std::string ipcPath;                            // Custom IPC socket path
    bool statusAutoPollEnabled = false;
    int statusPollIntervalMs = 2000;
    StatusRequestKind statusDefaultKind = StatusRequestKind::ServerInfo;
    std::vector<std::string> statusCategoryOrder;
    std::string statusCategoryFilter;
    bool statusDiffEnabled = false;
    bool statusDiffIgnoreUnchanged = true;
    bool statusDiffIgnoreEmpty = true;
};

struct NetworkOptions {
    int connectTimeoutMs = 5000;
    int queryTimeoutMs = 0;
    int readTimeoutMs = 30000;
    int writeTimeoutMs = 30000;
    uint32_t streamWindowBytes = 65536;
    uint32_t streamChunkBytes = 16384;
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
    bool ExecuteQuery(const std::string& sql, const QueryOptions& options, QueryResult* outResult);
    bool ExecuteStatement(const std::string& sql, int64_t* rowsAffected);
    bool ExecuteCopy(const CopyOptions& options, CopyResult* outResult);
    bool PrepareStatement(const std::string& sql, PreparedStatementHandlePtr* outStatement);
    bool ExecutePrepared(const PreparedStatementHandlePtr& statement,
                         const std::vector<PreparedParameter>& params,
                         QueryResult* outResult);
    void ClosePrepared(const PreparedStatementHandlePtr& statement);
    bool Subscribe(const std::string& channel, const std::string& filter);
    bool Unsubscribe(const std::string& channel);
    bool FetchNotification(NotificationEvent* outEvent);
    bool FetchStatus(StatusRequestKind kind, StatusSnapshot* outSnapshot);
    void SetProgressCallback(std::function<void(uint64_t, uint64_t)> callback);
    bool BeginTransaction();
    bool Commit();
    bool Rollback();
    const std::string& LastError() const;

    void SetCredentialStore(std::unique_ptr<CredentialStore> credentialStore);
    void SetNetworkOptions(const NetworkOptions& options);
    NetworkOptions GetNetworkOptions() const;
    void SetAutoCommit(bool enabled);
    bool IsAutoCommit() const { return auto_commit_; }
    bool IsInTransaction() const { return in_transaction_; }
    bool CancelActive();
    BackendCapabilities Capabilities() const;

    using SimpleCallback = std::function<void(bool, const std::string&)>;
    using QueryCallback = std::function<void(bool, QueryResult, const std::string&)>;
    using NotificationCallback = std::function<void(bool, NotificationEvent, const std::string&)>;
    using StatusCallback = std::function<void(bool, StatusSnapshot, const std::string&)>;

    JobHandle ConnectAsync(const ConnectionProfile& profile, SimpleCallback callback);
    JobHandle ExecuteQueryAsync(const std::string& sql, QueryCallback callback);
    JobHandle ExecuteQueryAsync(const std::string& sql, const QueryOptions& options,
                                QueryCallback callback);
    JobHandle FetchNotificationAsync(NotificationCallback callback);
    JobHandle FetchStatusAsync(StatusRequestKind kind, StatusCallback callback);
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
