/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CONNECTION_BACKEND_H
#define SCRATCHROBIN_CONNECTION_BACKEND_H

#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <string>

#include "core/copy_types.h"
#include "core/notification_types.h"
#include "core/prepared_types.h"
#include "core/query_options.h"
#include "core/query_types.h"
#include "core/status_types.h"

namespace scratchrobin {

struct BackendCapabilities {
    // Core capabilities
    bool supportsCancel = false;
    bool supportsTransactions = true;
    bool supportsPaging = true;
    bool supportsSavepoints = true;
    
    // Query capabilities
    bool supportsExplain = false;
    bool supportsSblr = false;
    bool supportsStreaming = true;
    bool supportsPreparedStatements = false;
    bool supportsStatementCache = false;
    bool supportsCopyIn = false;
    bool supportsCopyOut = false;
    bool supportsCopyBoth = false;
    bool supportsCopyBinary = false;
    bool supportsCopyText = false;
    bool supportsNotifications = false;
    bool supportsStatus = false;
    
    // Schema capabilities
    bool supportsDdlExtract = false;
    bool supportsDependencies = false;
    bool supportsConstraints = true;
    bool supportsIndexes = true;
    
    // Admin capabilities
    bool supportsUserAdmin = false;
    bool supportsRoleAdmin = false;
    bool supportsGroupAdmin = false;
    bool supportsJobScheduler = false;
    
    // Feature availability
    bool supportsDomains = false;
    bool supportsSequences = false;
    bool supportsTriggers = false;
    bool supportsProcedures = false;
    bool supportsViews = true;
    bool supportsTempTables = true;
    
    // Database capabilities
    bool supportsMultipleDatabases = true;
    bool supportsTablespaces = false;
    bool supportsSchemas = true;
    
    // Utility
    bool supportsBackup = false;
    bool supportsImportExport = true;
    
    // Server info (populated on connect)
    std::string serverVersion;
    std::string serverType;
    int majorVersion = 0;
    int minorVersion = 0;
    int patchVersion = 0;
};

struct BackendConfig {
    std::string host;
    int port = 0;
    std::string database;
    std::string username;
    std::string password;
    std::string applicationName;
    std::string role;
    std::string sslMode;
    std::string sslRootCert;
    std::string sslCert;
    std::string sslKey;
    std::string sslPassword;
    std::string options;
    std::string fixturePath;
    int connectTimeoutMs = 5000;
    int queryTimeoutMs = 0;
    int readTimeoutMs = 30000;
    int writeTimeoutMs = 30000;
    uint32_t streamWindowBytes = 0;
    uint32_t streamChunkBytes = 0;
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
    virtual bool ExecuteQuery(const std::string& sql, const QueryOptions& options,
                              QueryResult* outResult, std::string* error) {
        (void)options;
        return ExecuteQuery(sql, outResult, error);
    }
    virtual bool ExecuteCopy(const CopyOptions& options, std::istream* input,
                             std::ostream* output, CopyResult* outResult,
                             std::string* error) = 0;
    virtual bool PrepareStatement(const std::string& sql,
                                  std::shared_ptr<PreparedStatementHandle>* outStatement,
                                  std::string* error) {
        if (error) {
            *error = "Prepared statements not supported";
        }
        (void)sql;
        (void)outStatement;
        return false;
    }
    virtual bool ExecutePrepared(const std::shared_ptr<PreparedStatementHandle>& statement,
                                 const std::vector<PreparedParameter>& params,
                                 QueryResult* outResult,
                                 std::string* error) {
        if (error) {
            *error = "Prepared statements not supported";
        }
        (void)statement;
        (void)params;
        (void)outResult;
        return false;
    }
    virtual void ClosePrepared(const std::shared_ptr<PreparedStatementHandle>& statement) {
        (void)statement;
    }
    virtual bool Subscribe(const std::string& channel, const std::string& filter,
                           std::string* error) {
        (void)channel;
        (void)filter;
        if (error) {
            *error = "Notifications not supported";
        }
        return false;
    }
    virtual bool Unsubscribe(const std::string& channel, std::string* error) {
        (void)channel;
        if (error) {
            *error = "Notifications not supported";
        }
        return false;
    }
    virtual bool FetchNotification(NotificationEvent* outEvent, std::string* error) {
        (void)outEvent;
        if (error) {
            *error = "Notifications not supported";
        }
        return false;
    }
    virtual bool FetchStatus(StatusRequestKind kind, StatusSnapshot* outSnapshot,
                             std::string* error) {
        (void)kind;
        (void)outSnapshot;
        if (error) {
            *error = "Status request not supported";
        }
        return false;
    }
    virtual void SetProgressCallback(std::function<void(uint64_t, uint64_t)> callback) {
        (void)callback;
    }
    virtual bool BeginTransaction(std::string* error) = 0;
    virtual bool Commit(std::string* error) = 0;
    virtual bool Rollback(std::string* error) = 0;
    virtual bool Cancel(std::string* error) = 0;
    virtual BackendCapabilities Capabilities() const = 0;
    virtual std::string BackendName() const = 0;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONNECTION_BACKEND_H
