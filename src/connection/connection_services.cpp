#include "connection/connection_services.h"

#include "core/reject.h"

namespace scratchrobin::connection {

namespace {

beta1b::ConnectionMode ToContractMode(runtime::ConnectionMode mode) {
    switch (mode) {
        case runtime::ConnectionMode::Ipc: return beta1b::ConnectionMode::Ipc;
        case runtime::ConnectionMode::Embedded: return beta1b::ConnectionMode::Embedded;
        case runtime::ConnectionMode::Network:
        default: return beta1b::ConnectionMode::Network;
    }
}

}  // namespace

BackendAdapterService::BackendAdapterService()
    : capabilities_by_backend_(DefaultCapabilities()) {}

std::map<std::string, std::set<std::string>> BackendAdapterService::DefaultCapabilities() {
    return {
        {"network", {"transactions", "cancel", "explain", "sblr", "prepared", "copy", "notifications", "status"}},
        {"ipc", {"transactions", "cancel", "prepared", "copy", "notifications", "status"}},
        {"embedded", {"transactions", "cancel", "prepared", "copy", "notifications", "status"}},
        {"postgresql", {"transactions", "cancel", "prepared", "copy", "status"}},
        {"mysql", {"transactions", "cancel", "prepared", "copy", "status"}},
        {"firebird", {"transactions", "cancel", "prepared", "copy", "status"}},
        {"mock", {"copy", "status"}},
    };
}

std::string BackendAdapterService::VersionForBackend(const std::string& backend) {
    if (backend == "postgresql") {
        return "16.0";
    }
    if (backend == "mysql") {
        return "8.0";
    }
    if (backend == "firebird") {
        return "5.0";
    }
    if (backend == "network" || backend == "ipc" || backend == "embedded") {
        return "scratchbird-vnext";
    }
    return "0.0";
}

SessionSnapshot BackendAdapterService::Connect(const runtime::ConnectionProfile& profile) {
    beta1b::ConnectionProfile contract;
    contract.backend = profile.backend;
    contract.mode = ToContractMode(profile.mode);
    contract.host = profile.host;
    contract.port = profile.port;
    contract.ipc_path = profile.ipc_path;
    contract.database = profile.database;
    contract.username = profile.username;
    contract.credential_id = profile.credential_id;

    const std::string backend = beta1b::SelectBackend(contract);
    const int port = beta1b::ResolvePort(contract);

    session_.backend_name = backend;
    session_.server_type = backend;
    session_.server_version = VersionForBackend(backend);
    session_.port = port;
    session_.connected = true;
    active_query_ = false;
    subscribed_channels_.clear();
    notification_queue_.clear();
    return session_;
}

void BackendAdapterService::Disconnect() {
    session_ = SessionSnapshot{};
    active_query_ = false;
    subscribed_channels_.clear();
    notification_queue_.clear();
}

bool BackendAdapterService::IsConnected() const {
    return session_.connected;
}

SessionSnapshot BackendAdapterService::CurrentSession() const {
    return session_;
}

void BackendAdapterService::SetCapabilityMap(const std::map<std::string, std::set<std::string>>& capabilities_by_backend) {
    capabilities_by_backend_ = capabilities_by_backend;
}

bool BackendAdapterService::HasCapability(const std::string& capability_key) const {
    if (!session_.connected) {
        return false;
    }
    auto it = capabilities_by_backend_.find(session_.backend_name);
    if (it == capabilities_by_backend_.end()) {
        return false;
    }
    return it->second.count(capability_key) > 0U;
}

void BackendAdapterService::EnsureCapability(const std::string& capability_key) const {
    if (!session_.connected) {
        throw MakeReject("SRB1-R-4101", "backend not connected", "connection", "ensure_capability");
    }
    beta1b::EnsureCapability(HasCapability(capability_key), session_.backend_name, capability_key);
}

QueryExecutionResult BackendAdapterService::ExecuteQuery(const std::string& sql) {
    if (!session_.connected) {
        throw MakeReject("SRB1-R-4101", "backend not connected", "connection", "execute_query");
    }
    QueryExecutionResult out;
    if (sql.empty()) {
        out.command_tag = "EMPTY";
        out.rows_affected = 0;
        out.messages.push_back("empty sql");
        return out;
    }
    out.command_tag = "EXECUTE";
    out.rows_affected = 1;
    out.messages.push_back("ok");
    return out;
}

std::string BackendAdapterService::ExecuteCopy(const std::string& sql,
                                               const std::string& source_kind,
                                               const std::string& sink_kind,
                                               bool source_open_ok,
                                               bool sink_open_ok) {
    EnsureCapability("copy");
    return beta1b::RunCopyIo(sql, source_kind, sink_kind, source_open_ok, sink_open_ok);
}

std::string BackendAdapterService::ExecutePrepared(const std::string& sql, const std::vector<std::string>& bind_values) {
    EnsureCapability("prepared");
    return beta1b::PrepareExecuteClose(true, sql, bind_values);
}

std::string BackendAdapterService::FetchStatus(std::int64_t running_queries, std::int64_t queued_jobs) {
    EnsureCapability("status");
    return beta1b::StatusSnapshot(true, running_queries, queued_jobs);
}

void BackendAdapterService::Subscribe(const std::string& channel, const std::string& filter) {
    if (!session_.connected) {
        throw MakeReject("SRB1-R-4204", "notification API unsupported", "connection", "subscribe");
    }
    if (!HasCapability("notifications")) {
        throw MakeReject("SRB1-R-4204", "notification API unsupported", "connection", "subscribe");
    }
    subscribed_channels_.insert(channel);
    notification_queue_.push_back({channel, filter});
}

void BackendAdapterService::Unsubscribe(const std::string& channel) {
    if (!session_.connected || !HasCapability("notifications")) {
        throw MakeReject("SRB1-R-4204", "notification API unsupported", "connection", "unsubscribe");
    }
    subscribed_channels_.erase(channel);
}

std::optional<std::pair<std::string, std::string>> BackendAdapterService::FetchNotification() {
    if (!session_.connected || !HasCapability("notifications")) {
        throw MakeReject("SRB1-R-4204", "notification API unsupported", "connection", "fetch_notification");
    }
    if (notification_queue_.empty()) {
        return std::nullopt;
    }
    auto value = notification_queue_.front();
    notification_queue_.erase(notification_queue_.begin());
    if (subscribed_channels_.count(value.first) == 0U) {
        return std::nullopt;
    }
    return value;
}

void BackendAdapterService::MarkActiveQuery(bool active) {
    active_query_ = active;
}

void BackendAdapterService::CancelActiveQuery() {
    beta1b::CancelActive(session_.connected && active_query_);
    active_query_ = false;
}

beta1b::SessionFingerprint BackendAdapterService::ConnectEnterprise(
    const beta1b::EnterpriseConnectionProfile& profile,
    const std::optional<std::string>& runtime_override,
    const std::function<std::optional<std::string>(const beta1b::SecretProviderContract&)>& provider_fetch,
    const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
    const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
    const std::function<bool(const std::string&, const std::string&)>& directory_bind) const {
    return beta1b::ConnectEnterprise(profile,
                                     runtime_override,
                                     provider_fetch,
                                     credential_fetch,
                                     federated_acquire,
                                     directory_bind);
}

void BackendAdapterService::SetCredentialStore(const std::map<std::string, std::string>& credential_store) {
    credential_store_ = credential_store;
}

void BackendAdapterService::SetSecretStore(const std::string& provider_mode,
                                           const std::map<std::string, std::string>& secrets_by_ref) {
    secret_stores_by_mode_[provider_mode] = secrets_by_ref;
}

void BackendAdapterService::SetFederatedIdentityPolicy(const std::string& provider_id,
                                                       const std::set<std::string>& accepted_secrets) {
    federated_identity_policy_[provider_id] = accepted_secrets;
}

void BackendAdapterService::SetDirectoryIdentityPolicy(const std::string& provider_id,
                                                       const std::set<std::string>& accepted_secrets) {
    directory_identity_policy_[provider_id] = accepted_secrets;
}

beta1b::SessionFingerprint BackendAdapterService::ConnectEnterprise(
    const beta1b::EnterpriseConnectionProfile& profile,
    const std::optional<std::string>& runtime_override) const {
    const auto provider_fetch = [&](const beta1b::SecretProviderContract& provider) -> std::optional<std::string> {
        auto mode_it = secret_stores_by_mode_.find(provider.mode);
        if (mode_it == secret_stores_by_mode_.end()) {
            return std::nullopt;
        }
        const std::string ref = provider.secret_ref.empty() ? "__default__" : provider.secret_ref;
        auto secret_it = mode_it->second.find(ref);
        if (secret_it == mode_it->second.end()) {
            return std::nullopt;
        }
        return secret_it->second;
    };
    const auto credential_fetch = [&](const std::string& credential_id) -> std::optional<std::string> {
        auto it = credential_store_.find(credential_id);
        if (it == credential_store_.end()) {
            return std::nullopt;
        }
        return it->second;
    };
    const auto federated_acquire = [&](const std::string& provider_id, const std::string& secret) -> bool {
        auto it = federated_identity_policy_.find(provider_id);
        if (it == federated_identity_policy_.end()) {
            return false;
        }
        return it->second.count(secret) > 0U;
    };
    const auto directory_bind = [&](const std::string& provider_id, const std::string& secret) -> bool {
        auto it = directory_identity_policy_.find(provider_id);
        if (it == directory_identity_policy_.end()) {
            return false;
        }
        return it->second.count(secret) > 0U;
    };

    return beta1b::ConnectEnterprise(profile,
                                     runtime_override,
                                     provider_fetch,
                                     credential_fetch,
                                     federated_acquire,
                                     directory_bind);
}

}  // namespace scratchrobin::connection
