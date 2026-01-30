/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "connection_manager.h"

#include "connection_backend.h"
#include "credentials.h"
#include "firebird_backend.h"
#include "mysql_backend.h"
#include "mock_backend.h"
#include "network_backend.h"
#include "postgres_backend.h"

#include <algorithm>
#include <atomic>
#include <cctype>

namespace scratchrobin {
namespace {

constexpr int kDefaultScratchBirdPort = 3092;
constexpr const char* kDefaultFixturePath = "config/fixtures/default.json";

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

} // namespace

ConnectionManager::ConnectionManager()
    : credential_store_(CreateDefaultCredentialStore()) {}

ConnectionManager::ConnectionManager(std::unique_ptr<CredentialStore> credentialStore)
    : credential_store_(std::move(credentialStore)) {
    if (!credential_store_) {
        credential_store_ = CreateDefaultCredentialStore();
    }
}

ConnectionManager::~ConnectionManager() = default;

void ConnectionManager::SetCredentialStore(std::unique_ptr<CredentialStore> credentialStore) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    credential_store_ = std::move(credentialStore);
    if (!credential_store_) {
        credential_store_ = CreateDefaultCredentialStore();
    }
}

void ConnectionManager::SetNetworkOptions(const NetworkOptions& options) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    network_options_ = options;
}

std::unique_ptr<ConnectionBackend> ConnectionManager::CreateBackendForProfile(
    const ConnectionProfile& profile, std::string* error) {
    std::string backend_name = ToLower(Trim(profile.backend));
    if (backend_name.empty()) {
        backend_name = profile.fixturePath.empty() ? "network" : "mock";
    }

    if (backend_name == "mock") {
        return CreateMockBackend();
    }

    if (backend_name == "network" || backend_name == "scratchbird" || backend_name == "native") {
        auto backend = CreateNetworkBackend();
        if (!backend) {
            if (error) {
                *error = "ScratchBird network backend is not available in this build";
            }
            return nullptr;
        }
        return backend;
    }

    if (backend_name == "postgresql" || backend_name == "postgres" || backend_name == "pg") {
        auto backend = CreatePostgresBackend();
        if (!backend) {
            if (error) {
                *error = "PostgreSQL backend is not available in this build";
            }
            return nullptr;
        }
        return backend;
    }

    if (backend_name == "mysql" || backend_name == "mariadb") {
        auto backend = CreateMySqlBackend();
        if (!backend) {
            if (error) {
                *error = "MySQL backend is not available in this build";
            }
            return nullptr;
        }
        return backend;
    }

    if (backend_name == "firebird" || backend_name == "fb") {
        auto backend = CreateFirebirdBackend();
        if (!backend) {
            if (error) {
                *error = "Firebird backend is not available in this build";
            }
            return nullptr;
        }
        return backend;
    }

    if (error) {
        *error = "Unknown backend: " + profile.backend;
    }
    return nullptr;
}

bool ConnectionManager::Connect(const ConnectionProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    last_error_.clear();

    if (backend_) {
        backend_->Disconnect();
        backend_.reset();
    }

    std::string password;
    std::string error;
    if (credential_store_ && !profile.credentialId.empty()) {
        if (!credential_store_->ResolvePassword(profile.credentialId, &password, &error)) {
            last_error_ = error.empty() ? "Credential lookup failed" : error;
            return false;
        }
    }

    BackendConfig config;
    config.host = profile.host;
    config.port = profile.port;
    if (config.port == 0) {
        std::string backend_name = ToLower(Trim(profile.backend));
        if (backend_name.empty()) {
            backend_name = profile.fixturePath.empty() ? "network" : "mock";
        }
        if (backend_name == "postgresql" || backend_name == "postgres" || backend_name == "pg") {
            config.port = 5432;
        } else if (backend_name == "mysql" || backend_name == "mariadb") {
            config.port = 3306;
        } else if (backend_name == "firebird" || backend_name == "fb") {
            config.port = 3050;
        } else {
            config.port = kDefaultScratchBirdPort;
        }
    }
    config.database = profile.database;
    config.username = profile.username;
    config.password = password;
    config.applicationName = profile.applicationName.empty() ? "scratchrobin" : profile.applicationName;
    config.role = profile.role;
    config.sslMode = profile.sslMode;
    config.sslRootCert = profile.sslRootCert;
    config.sslCert = profile.sslCert;
    config.sslKey = profile.sslKey;
    config.sslPassword = profile.sslPassword;
    config.options = profile.options;
    config.fixturePath = profile.fixturePath.empty() ? kDefaultFixturePath : profile.fixturePath;
    config.connectTimeoutMs = network_options_.connectTimeoutMs;
    config.queryTimeoutMs = network_options_.queryTimeoutMs;
    config.readTimeoutMs = network_options_.readTimeoutMs;
    config.writeTimeoutMs = network_options_.writeTimeoutMs;

    auto backend = CreateBackendForProfile(profile, &last_error_);
    if (!backend) {
        return false;
    }

    if (!backend->Connect(config, &last_error_)) {
        return false;
    }

    backend_ = std::move(backend);
    in_transaction_ = false;
    if (!auto_commit_) {
        if (!BeginTransaction()) {
            return false;
        }
    }
    return true;
}

void ConnectionManager::Disconnect() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (backend_) {
        backend_->Disconnect();
    }
    in_transaction_ = false;
}

bool ConnectionManager::IsConnected() const {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    return backend_ && backend_->IsConnected();
}

bool ConnectionManager::ExecuteQuery(const std::string& sql, QueryResult* outResult) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (!outResult) {
        last_error_ = "No result buffer provided";
        return false;
    }

    outResult->columns.clear();
    outResult->rows.clear();
    outResult->rowsAffected = 0;
    outResult->commandTag.clear();
    outResult->messages.clear();
    outResult->errorStack.clear();
    outResult->stats = QueryStats{};

    if (!IsConnected()) {
        last_error_ = "Not connected";
        return false;
    }

    if (!auto_commit_ && !in_transaction_) {
        if (!BeginTransaction()) {
            return false;
        }
    }

    if (!backend_->ExecuteQuery(sql, outResult, &last_error_)) {
        return false;
    }

    outResult->stats.rowsReturned = static_cast<int64_t>(outResult->rows.size());
    return true;
}

bool ConnectionManager::ExecuteStatement(const std::string& sql, int64_t* rowsAffected) {
    QueryResult result;
    if (!ExecuteQuery(sql, &result)) {
        return false;
    }
    if (rowsAffected) {
        *rowsAffected = result.rowsAffected;
    }
    return true;
}

bool ConnectionManager::BeginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (!IsConnected()) {
        last_error_ = "Not connected";
        return false;
    }

    if (!backend_->BeginTransaction(&last_error_)) {
        return false;
    }

    in_transaction_ = true;
    return true;
}

bool ConnectionManager::Commit() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (!IsConnected()) {
        last_error_ = "Not connected";
        return false;
    }

    if (!backend_->Commit(&last_error_)) {
        return false;
    }

    in_transaction_ = false;
    if (!auto_commit_) {
        return BeginTransaction();
    }
    return true;
}

bool ConnectionManager::Rollback() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (!IsConnected()) {
        last_error_ = "Not connected";
        return false;
    }

    if (!backend_->Rollback(&last_error_)) {
        return false;
    }

    in_transaction_ = false;
    if (!auto_commit_) {
        return BeginTransaction();
    }
    return true;
}

void ConnectionManager::SetAutoCommit(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    auto_commit_ = enabled;
    if (!IsConnected()) {
        return;
    }
    if (auto_commit_) {
        if (in_transaction_) {
            Commit();
        }
    } else if (!in_transaction_) {
        BeginTransaction();
    }
}

bool ConnectionManager::CancelActive() {
    auto backend = std::atomic_load_explicit(&backend_, std::memory_order_acquire);
    if (!backend) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex_);
        last_error_ = "Not connected";
        return false;
    }
    std::string error;
    bool ok = backend->Cancel(&error);
    if (!ok || !error.empty()) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex_);
        last_error_ = error.empty() ? "Cancel failed" : error;
    }
    return ok;
}

BackendCapabilities ConnectionManager::Capabilities() const {
    std::lock_guard<std::recursive_mutex> lock(state_mutex_);
    if (!backend_) {
        return BackendCapabilities{};
    }
    return backend_->Capabilities();
}

JobHandle ConnectionManager::ConnectAsync(const ConnectionProfile& profile,
                                          SimpleCallback callback) {
    JobHandle handle = job_queue_.Submit([this, profile, callback](JobHandle& job) {
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        bool ok = Connect(profile);
        std::string error = ok ? std::string() : LastError();
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        if (callback) {
            callback(ok, error);
        }
    });
    handle.SetCancelCallback([this]() { CancelActive(); });
    return handle;
}

JobHandle ConnectionManager::ExecuteQueryAsync(const std::string& sql,
                                               QueryCallback callback) {
    JobHandle handle = job_queue_.Submit([this, sql, callback](JobHandle& job) {
        QueryResult result;
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, QueryResult{}, "Canceled");
            }
            return;
        }
        bool ok = ExecuteQuery(sql, &result);
        std::string error = ok ? std::string() : LastError();
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, QueryResult{}, "Canceled");
            }
            return;
        }
        if (callback) {
            callback(ok, result, error);
        }
    });
    handle.SetCancelCallback([this]() { CancelActive(); });
    return handle;
}

JobHandle ConnectionManager::BeginTransactionAsync(SimpleCallback callback) {
    JobHandle handle = job_queue_.Submit([this, callback](JobHandle& job) {
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        bool ok = BeginTransaction();
        std::string error = ok ? std::string() : LastError();
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        if (callback) {
            callback(ok, error);
        }
    });
    handle.SetCancelCallback([this]() { CancelActive(); });
    return handle;
}

JobHandle ConnectionManager::CommitAsync(SimpleCallback callback) {
    JobHandle handle = job_queue_.Submit([this, callback](JobHandle& job) {
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        bool ok = Commit();
        std::string error = ok ? std::string() : LastError();
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        if (callback) {
            callback(ok, error);
        }
    });
    handle.SetCancelCallback([this]() { CancelActive(); });
    return handle;
}

JobHandle ConnectionManager::RollbackAsync(SimpleCallback callback) {
    JobHandle handle = job_queue_.Submit([this, callback](JobHandle& job) {
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        bool ok = Rollback();
        std::string error = ok ? std::string() : LastError();
        if (job.IsCanceled()) {
            if (callback) {
                callback(false, "Canceled");
            }
            return;
        }
        if (callback) {
            callback(ok, error);
        }
    });
    handle.SetCancelCallback([this]() { CancelActive(); });
    return handle;
}

const std::string& ConnectionManager::LastError() const {
    return last_error_;
}

} // namespace scratchrobin
