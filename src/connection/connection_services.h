#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/beta1b_contracts.h"
#include "runtime/runtime_config.h"

namespace scratchrobin::connection {

struct SessionSnapshot {
    std::string backend_name;
    std::string server_type;
    std::string server_version;
    int port = 0;
    bool connected = false;
};

struct QueryExecutionResult {
    std::string command_tag;
    std::int64_t rows_affected = 0;
    std::vector<std::string> messages;
};

class BackendAdapterService {
public:
    BackendAdapterService();

    SessionSnapshot Connect(const runtime::ConnectionProfile& profile);
    void Disconnect();
    bool IsConnected() const;
    SessionSnapshot CurrentSession() const;

    void SetCapabilityMap(const std::map<std::string, std::set<std::string>>& capabilities_by_backend);
    bool HasCapability(const std::string& capability_key) const;
    void EnsureCapability(const std::string& capability_key) const;

    QueryExecutionResult ExecuteQuery(const std::string& sql);
    std::string ExecuteCopy(const std::string& sql,
                            const std::string& source_kind,
                            const std::string& sink_kind,
                            bool source_open_ok,
                            bool sink_open_ok);
    std::string ExecutePrepared(const std::string& sql, const std::vector<std::string>& bind_values);
    std::string FetchStatus(std::int64_t running_queries, std::int64_t queued_jobs);

    void Subscribe(const std::string& channel, const std::string& filter);
    void Unsubscribe(const std::string& channel);
    std::optional<std::pair<std::string, std::string>> FetchNotification();

    void MarkActiveQuery(bool active);
    void CancelActiveQuery();

    beta1b::SessionFingerprint ConnectEnterprise(
        const beta1b::EnterpriseConnectionProfile& profile,
        const std::optional<std::string>& runtime_override,
        const std::function<std::optional<std::string>(const beta1b::SecretProviderContract&)>& provider_fetch,
        const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
        const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
        const std::function<bool(const std::string&, const std::string&)>& directory_bind) const;

private:
    static std::map<std::string, std::set<std::string>> DefaultCapabilities();
    static std::string VersionForBackend(const std::string& backend);

    std::map<std::string, std::set<std::string>> capabilities_by_backend_;
    SessionSnapshot session_;
    std::set<std::string> subscribed_channels_;
    std::vector<std::pair<std::string, std::string>> notification_queue_;
    bool active_query_ = false;
};

}  // namespace scratchrobin::connection

