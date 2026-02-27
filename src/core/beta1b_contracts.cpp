#include "core/beta1b_contracts.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "core/sha256.h"

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace scratchrobin::beta1b {

namespace {

using StringMap = std::map<std::string, std::string>;

constexpr std::uint32_t kMagicSrpj = 0x4A505253;
constexpr std::uint16_t kHeaderSize = 44;
constexpr std::uint16_t kTocEntrySize = 40;

std::string Trim(std::string value) {
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c) { return !is_space(static_cast<unsigned char>(c)); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char c) { return !is_space(static_cast<unsigned char>(c)); }).base(),
                value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

std::vector<std::string> Split(const std::string& text, char delimiter) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }
    return out;
}

std::string Join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::ostringstream out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << delimiter;
        }
        out << parts[i];
    }
    return out.str();
}

std::string DefaultIdentityMethodForMode(const std::string& identity_mode) {
    const std::string mode = ToLower(Trim(identity_mode));
    if (mode == "local_password") {
        return "scratchbird.auth.password_compat";
    }
    if (mode == "oidc" || mode == "saml") {
        return "scratchbird.auth.jwt_oidc";
    }
    if (mode == "ldap") {
        return "scratchbird.auth.ldap_bind";
    }
    if (mode == "kerberos") {
        return "scratchbird.auth.kerberos_gssapi";
    }
    if (mode == "ident") {
        return "scratchbird.auth.ident_rfc1413";
    }
    if (mode == "radius") {
        return "scratchbird.auth.radius_pap";
    }
    if (mode == "pam") {
        return "scratchbird.auth.pam_conversation";
    }
    return "";
}

bool HasMethodOverlap(const std::vector<std::string>& required,
                      const std::vector<std::string>& forbidden,
                      std::string& overlap_method) {
    std::set<std::string> normalized_required;
    for (const auto& method : required) {
        const std::string normalized = ToLower(Trim(method));
        if (!normalized.empty()) {
            normalized_required.insert(normalized);
        }
    }
    for (const auto& method : forbidden) {
        const std::string normalized = ToLower(Trim(method));
        if (!normalized.empty() && normalized_required.find(normalized) != normalized_required.end()) {
            overlap_method = normalized;
            return true;
        }
    }
    overlap_method.clear();
    return false;
}

std::string JsonEscape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

bool IsHexLower(std::string_view value) {
    return std::all_of(value.begin(), value.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}

bool IsRfc3339Utc(std::string_view ts) {
    static const std::regex kTs(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$)");
    return std::regex_match(ts.begin(), ts.end(), kTs);
}

bool IsLocalDateTime(std::string_view ts) {
    static const std::regex kTs(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}$)");
    return std::regex_match(ts.begin(), ts.end(), kTs);
}

bool IsTimezoneIana(std::string_view tz) {
    static const std::regex kTz(R"(^(UTC|[A-Za-z_]+(?:/[A-Za-z0-9_+\-]+)+)$)");
    return std::regex_match(tz.begin(), tz.end(), kTz);
}

std::optional<std::time_t> ParseUtc(std::string_view utc) {
    if (!IsRfc3339Utc(utc)) {
        return std::nullopt;
    }
    std::tm tm{};
    if (std::sscanf(std::string(utc).c_str(), "%4d-%2d-%2dT%2d:%2d:%2dZ", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                    &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
        return std::nullopt;
    }
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
#if defined(_WIN32)
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

std::optional<std::time_t> ParseLocalAsUtc(std::string_view local_ts) {
    if (!IsLocalDateTime(local_ts)) {
        return std::nullopt;
    }
    std::tm tm{};
    if (std::sscanf(std::string(local_ts).c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                    &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
        return std::nullopt;
    }
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
#if defined(_WIN32)
    return _mkgmtime(&tm);
#else
    return timegm(&tm);
#endif
}

std::string FormatUtc(std::time_t ts) {
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &ts);
#else
    gmtime_r(&ts, &tm);
#endif
    char buf[64];
    const int n = std::snprintf(buf,
                                sizeof(buf),
                                "%04d-%02d-%02dT%02d:%02d:%02dZ",
                                tm.tm_year + 1900,
                                tm.tm_mon + 1,
                                tm.tm_mday,
                                tm.tm_hour,
                                tm.tm_min,
                                tm.tm_sec);
    if (n <= 0) {
        throw MakeReject("SRB1-R-7102", "failed utc formatting", "reporting", "format_utc");
    }
    if (static_cast<size_t>(n) >= sizeof(buf)) {
        throw MakeReject("SRB1-R-7102", "utc format overflow", "reporting", "format_utc");
    }
    return std::string(buf, static_cast<size_t>(n));
}

std::uint16_t ReadU16(const std::vector<std::uint8_t>& b, size_t off) {
    return static_cast<std::uint16_t>(b[off]) | (static_cast<std::uint16_t>(b[off + 1]) << 8);
}

std::uint32_t ReadU32(const std::vector<std::uint8_t>& b, size_t off) {
    return static_cast<std::uint32_t>(b[off]) |
           (static_cast<std::uint32_t>(b[off + 1]) << 8) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24);
}

std::uint64_t ReadU64(const std::vector<std::uint8_t>& b, size_t off) {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(b[off + static_cast<size_t>(i)]) << (8 * i);
    }
    return value;
}

std::string ToKey(const std::vector<std::string>& parts) {
    return Join(parts, "\x1f");
}

std::string ReadTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw MakeReject("SRB1-R-5402", "failed to read file", "spec_workspace", "read_file", false, path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

JsonValue ParseJsonFile(const std::string& path) {
    std::string text = ReadTextFile(path);
    JsonParser parser(text);
    JsonValue value;
    std::string error;
    if (!parser.Parse(&value, &error)) {
        throw MakeReject("SRB1-R-5402", "json parse failure", "spec_workspace", "parse_json", false, error);
    }
    return value;
}

const JsonValue& RequireMember(const JsonValue& object, const std::string& key, const std::string& code, const std::string& surface,
                               const std::string& op) {
    const JsonValue* m = FindMember(object, key);
    if (m == nullptr) {
        throw MakeReject(code, "missing field: " + key, surface, op);
    }
    return *m;
}

std::string RequireString(const JsonValue& object, const std::string& key, const std::string& code,
                          const std::string& surface, const std::string& op) {
    const JsonValue& v = RequireMember(object, key, code, surface, op);
    std::string out;
    if (!GetStringValue(v, &out) || out.empty()) {
        throw MakeReject(code, "invalid string field: " + key, surface, op);
    }
    return out;
}

std::vector<std::string> RequireStringArray(const JsonValue& object,
                                            const std::string& key,
                                            const std::string& code,
                                            const std::string& surface,
                                            const std::string& op) {
    const JsonValue& v = RequireMember(object, key, code, surface, op);
    if (v.type != JsonValue::Type::Array) {
        throw MakeReject(code, "invalid array field: " + key, surface, op);
    }
    std::vector<std::string> out;
    out.reserve(v.array_value.size());
    for (const JsonValue& item : v.array_value) {
        if (item.type != JsonValue::Type::String || item.string_value.empty()) {
            throw MakeReject(code, "invalid array item in: " + key, surface, op);
        }
        out.push_back(item.string_value);
    }
    return out;
}

void EnsureSortedUnique(std::vector<std::string> values,
                        const std::string& field,
                        const std::string& code,
                        const std::string& surface,
                        const std::string& op) {
    if (values.empty()) {
        return;
    }
    if (!std::is_sorted(values.begin(), values.end())) {
        throw MakeReject(code, field + " must be sorted", surface, op);
    }
    if (std::adjacent_find(values.begin(), values.end()) != values.end()) {
        throw MakeReject(code, field + " must be unique", surface, op);
    }
}

void EnsureOnlyObjectFields(const JsonValue& object,
                            const std::set<std::string>& allowed,
                            const std::string& code,
                            const std::string& surface,
                            const std::string& op) {
    if (object.type != JsonValue::Type::Object) {
        throw MakeReject(code, "object expected", surface, op);
    }
    for (const auto& [key, _] : object.object_value) {
        if (allowed.count(key) == 0U) {
            throw MakeReject(code, "unexpected field: " + key, surface, op);
        }
    }
}

}  // namespace

// -------------------------
// Connection contracts
// -------------------------

std::string SelectBackend(const ConnectionProfile& profile) {
    if (profile.mode == ConnectionMode::Embedded) {
        return "embedded";
    }
    if (profile.mode == ConnectionMode::Ipc) {
        return "ipc";
    }
    std::string backend = ToLower(Trim(profile.backend));
    if (backend == "mock") {
        return "mock";
    }
    if (backend.empty() || backend == "network" || backend == "scratchbird" || backend == "native") {
        return "network";
    }
    if (backend == "postgresql" || backend == "postgres" || backend == "pg") {
        return "postgresql";
    }
    if (backend == "mysql" || backend == "mariadb") {
        return "mysql";
    }
    if (backend == "firebird" || backend == "fb") {
        return "firebird";
    }
    throw MakeReject("SRB1-R-4001", "unknown backend/profile mapping", "connection", "select_backend", false, backend);
}

int ResolvePort(const ConnectionProfile& profile) {
    if (profile.port > 0) {
        return profile.port;
    }
    const std::string backend = SelectBackend(profile);
    if (backend == "postgresql") {
        return 5432;
    }
    if (backend == "mysql") {
        return 3306;
    }
    if (backend == "firebird") {
        return 3050;
    }
    return 3092;
}

std::string ResolveCredential(const ConnectionProfile& profile,
                              const std::map<std::string, std::string>& credential_store,
                              const std::optional<std::string>& inline_secret) {
    if (!profile.credential_id.empty()) {
        auto it = credential_store.find(profile.credential_id);
        if (it == credential_store.end()) {
            throw MakeReject("SRB1-R-4002", "credential resolution failed", "connection", "resolve_credential", false,
                             profile.credential_id);
        }
        if (it->second.empty()) {
            throw MakeReject("SRB1-R-4003", "missing required credential", "connection", "resolve_credential", false,
                             profile.credential_id);
        }
        return it->second;
    }
    if (inline_secret.has_value() && !inline_secret->empty()) {
        return *inline_secret;
    }
    throw MakeReject("SRB1-R-4003", "missing required credential", "connection", "resolve_credential");
}

void EnsureCapability(bool supported, const std::string& backend_name, const std::string& capability_key) {
    if (!supported) {
        throw MakeReject("SRB1-R-4101",
                         "capability not supported by backend",
                         "connection",
                         "capability_gate",
                         false,
                         backend_name + ":" + capability_key);
    }
}

void CancelActive(bool has_active_backend) {
    if (!has_active_backend) {
        throw MakeReject("SRB1-R-4206", "cancellation requested with no active backend", "connection", "cancel");
    }
}

std::string RunCopyIo(const std::string& sql,
                      const std::string& source_kind,
                      const std::string& sink_kind,
                      bool source_open_ok,
                      bool sink_open_ok) {
    if (sql.empty() || ToUpper(sql).find("COPY") == std::string::npos) {
        throw MakeReject("SRB1-R-4202", "COPY SQL missing/invalid", "connection", "copy_io");
    }
    static const std::set<std::string> valid_endpoints = {"file", "clipboard", "stdin", "stdout"};
    if (valid_endpoints.count(source_kind) == 0U || valid_endpoints.count(sink_kind) == 0U) {
        throw MakeReject("SRB1-R-4203", "COPY I/O source or sink open failure", "connection", "copy_io");
    }
    if (!source_open_ok || !sink_open_ok) {
        throw MakeReject("SRB1-R-4203", "COPY I/O source or sink open failure", "connection", "copy_io");
    }
    return "copy-ok";
}

std::string PrepareExecuteClose(bool backend_supports_prepared,
                                const std::string& sql,
                                const std::vector<std::string>& bind_values) {
    if (!backend_supports_prepared) {
        throw MakeReject("SRB1-R-4201", "prepared statement unsupported", "connection", "prepare_execute_close");
    }
    if (sql.empty()) {
        throw MakeReject("SRB1-R-4201", "prepared SQL required", "connection", "prepare_execute_close");
    }
    std::ostringstream out;
    out << "prepared-ok:";
    out << sql.size();
    out << ":";
    out << bind_values.size();
    return out.str();
}

std::string StatusSnapshot(bool backend_supports_status,
                           std::int64_t running_queries,
                           std::int64_t queued_jobs) {
    if (!backend_supports_status) {
        throw MakeReject("SRB1-R-4205", "status snapshot unsupported", "connection", "status_snapshot");
    }
    if (running_queries < 0 || queued_jobs < 0) {
        throw MakeReject("SRB1-R-4205", "invalid status counters", "connection", "status_snapshot");
    }
    std::ostringstream out;
    out << "{\"running_queries\":" << running_queries << ",\"queued_jobs\":" << queued_jobs << "}";
    return out.str();
}

void ValidateTransport(const EnterpriseConnectionProfile& profile) {
    const auto& transport = profile.transport;
    if (transport.mode != "direct" && transport.mode != "ssh_tunnel" && transport.mode != "ssh_jump_chain") {
        throw MakeReject("SRB1-R-4004", "invalid transport mode", "connection", "validate_transport");
    }
    if (transport.tls_mode != "disable" && transport.tls_mode != "prefer" && transport.tls_mode != "required") {
        throw MakeReject("SRB1-R-4004", "invalid tls_mode", "connection", "validate_transport");
    }
    if (transport.connect_timeout_ms < 100U) {
        throw MakeReject("SRB1-R-4004", "invalid connect_timeout_ms", "connection", "validate_transport");
    }

    if (transport.mode != "direct") {
        if (!profile.ssh.has_value()) {
            throw MakeReject("SRB1-R-4004", "missing ssh contract", "connection", "validate_transport");
        }
        const auto& ssh = *profile.ssh;
        if (ssh.target_host.empty() || ssh.target_port <= 0 || ssh.tunnel_user.empty()) {
            throw MakeReject("SRB1-R-4004", "invalid ssh target contract", "connection", "validate_transport");
        }
        if (ssh.auth_method != "password" && ssh.auth_method != "keypair" && ssh.auth_method != "agent") {
            throw MakeReject("SRB1-R-4004", "invalid ssh auth_method", "connection", "validate_transport");
        }
        if ((ssh.auth_method == "password" || ssh.auth_method == "keypair") && ssh.credential_id.empty()) {
            throw MakeReject("SRB1-R-4004", "missing ssh credential_id", "connection", "validate_transport");
        }
    }

    if (transport.mode == "ssh_jump_chain") {
        if (profile.jump_hosts.empty() || profile.jump_hosts.size() > 4) {
            throw MakeReject("SRB1-R-4004", "jump host chain length invalid", "connection", "validate_transport");
        }
        for (size_t i = 0; i < profile.jump_hosts.size(); ++i) {
            const auto& hop = profile.jump_hosts[i];
            if (hop.host.empty() || hop.port <= 0 || hop.user.empty() || hop.auth_method.empty()) {
                throw MakeReject("SRB1-R-4004",
                                 "jump host missing required fields",
                                 "connection",
                                 "validate_transport",
                                 false,
                                 "index=" + std::to_string(i));
            }
            if (hop.auth_method != "password" && hop.auth_method != "keypair" && hop.auth_method != "agent") {
                throw MakeReject("SRB1-R-4004", "jump host invalid auth_method", "connection", "validate_transport",
                                 false, "index=" + std::to_string(i));
            }
            if ((hop.auth_method == "password" || hop.auth_method == "keypair") && hop.credential_id.empty()) {
                throw MakeReject("SRB1-R-4004", "jump host missing credential_id", "connection", "validate_transport",
                                 false, "index=" + std::to_string(i));
            }
        }
    }

    const auto& ident = profile.identity;
    const std::string mode = ToLower(Trim(ident.mode));
    const std::string method_id = ToLower(Trim(ident.auth_method_id));

    if (method_id.empty() && mode != "local_password" && mode != "oidc" && mode != "saml" && mode != "ldap" &&
        mode != "kerberos" && mode != "ident" && mode != "radius" && mode != "pam") {
        throw MakeReject("SRB1-R-4005", "unknown identity mode", "connection", "validate_transport");
    }

    std::string overlap_method;
    if (HasMethodOverlap(ident.auth_required_methods, ident.auth_forbidden_methods, overlap_method)) {
        throw MakeReject("SRB1-R-4005",
                         "invalid auth pinning profile overlap",
                         "connection",
                         "validate_transport",
                         false,
                         overlap_method);
    }

    if (!method_id.empty() && method_id.rfind("scratchbird.auth.", 0) != 0) {
        throw MakeReject("SRB1-R-4005",
                         "identity auth_method_id must use scratchbird.auth.* namespace",
                         "connection",
                         "validate_transport",
                         false,
                         method_id);
    }

    if ((mode == "oidc" || mode == "saml") &&
        method_id.empty() &&
        ident.provider_scope.empty()) {
        throw MakeReject("SRB1-R-4005", "provider_scope required", "connection", "validate_transport");
    }

    if (!mode.empty() && mode != "local_password" && ident.provider_id.empty() &&
        method_id != "scratchbird.auth.proxy_assertion" &&
        method_id != "scratchbird.auth.password_compat") {
        throw MakeReject("SRB1-R-4005", "provider_id required", "connection", "validate_transport");
    }

    if (profile.proxy_assertion_only) {
        if (method_id != "scratchbird.auth.proxy_assertion") {
            throw MakeReject("SRB1-R-4005",
                             "proxy_assertion_only profile requires scratchbird.auth.proxy_assertion",
                             "connection",
                             "validate_transport");
        }
        if (ident.proxy_principal_assertion.empty()) {
            throw MakeReject("SRB1-R-4005",
                             "proxy_assertion_only profile requires proxy principal assertion payload",
                             "connection",
                             "validate_transport");
        }
    }

    if (profile.no_login_direct && mode == "local_password" &&
        method_id != "scratchbird.auth.proxy_assertion") {
        throw MakeReject("SRB1-R-4005",
                         "no_login_direct profile does not allow local_password identity",
                         "connection",
                         "validate_transport");
    }

    if (method_id == "scratchbird.auth.workload_identity" &&
        ident.workload_identity_token.empty()) {
        throw MakeReject("SRB1-R-4005",
                         "workload identity method requires workload_identity_token",
                         "connection",
                         "validate_transport");
    }
    if (method_id == "scratchbird.auth.proxy_assertion" &&
        ident.proxy_principal_assertion.empty()) {
        throw MakeReject("SRB1-R-4005",
                         "proxy assertion method requires proxy_principal_assertion",
                         "connection",
                         "validate_transport");
    }
    if ((method_id == "scratchbird.auth.ident_rfc1413" ||
         method_id == "scratchbird.auth.radius_pap" ||
         method_id == "scratchbird.auth.pam_conversation") &&
        ident.provider_profile.empty()) {
        throw MakeReject("SRB1-R-4005",
                         "provider_profile required for enterprise identity method",
                         "connection",
                         "validate_transport");
    }

    if (profile.secret_provider.has_value()) {
        const auto& sp = *profile.secret_provider;
        if (sp.mode != "app_store" && sp.mode != "keychain" && sp.mode != "libsecret" && sp.mode != "vault") {
            throw MakeReject("SRB1-R-4006", "invalid secret provider mode", "connection", "validate_transport");
        }
        if (sp.mode == "vault" && sp.secret_ref.empty()) {
            throw MakeReject("SRB1-R-4006", "vault secret_ref required", "connection", "validate_transport");
        }
    }
}

std::string ResolveSecret(const std::optional<std::string>& runtime_override,
                          const std::function<std::optional<std::string>(const SecretProviderContract&)>& provider_fetch,
                          const std::optional<SecretProviderContract>& provider,
                          const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
                          const std::optional<std::string>& credential_id,
                          const std::optional<std::string>& inline_secret,
                          bool allow_inline) {
    if (runtime_override.has_value() && !runtime_override->empty()) {
        return *runtime_override;
    }
    if (provider.has_value()) {
        const auto sec = provider_fetch(*provider);
        if (sec.has_value() && !sec->empty()) {
            return *sec;
        }
    }
    if (credential_id.has_value() && !credential_id->empty()) {
        const auto sec = credential_fetch(*credential_id);
        if (sec.has_value() && !sec->empty()) {
            return *sec;
        }
    }
    if (allow_inline && inline_secret.has_value() && !inline_secret->empty()) {
        return *inline_secret;
    }
    throw MakeReject("SRB1-R-4006", "no usable secret source", "connection", "resolve_secret");
}

std::string RunIdentityHandshake(const IdentityContract& identity,
                                 const std::string& secret,
                                 const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
                                 const std::function<bool(const std::string&, const std::string&)>& directory_bind) {
    const std::string mode = ToLower(Trim(identity.mode));
    std::string method_id = ToLower(Trim(identity.auth_method_id));
    if (method_id.empty()) {
        method_id = DefaultIdentityMethodForMode(mode);
    }

    if (method_id.empty()) {
        throw MakeReject("SRB1-R-4005", "unknown identity mode", "connection", "identity_handshake");
    }

    if (method_id == "scratchbird.auth.password_compat" ||
        method_id == "scratchbird.auth.scram" ||
        method_id == "scratchbird.auth.scram_sha_256" ||
        method_id == "scratchbird.auth.scram_sha_512") {
        if (secret.empty()) {
            throw MakeReject("SRB1-R-4005",
                             "password/scram method requires secret",
                             "connection",
                             "identity_handshake");
        }
        return "local-password-ok";
    }

    if (method_id == "scratchbird.auth.jwt_oidc" ||
        method_id == "scratchbird.auth.oauth_validator") {
        if (!federated_acquire(identity.provider_id, secret)) {
            throw MakeReject("SRB1-R-4005",
                             "federated token acquisition failed",
                             "connection",
                             "identity_handshake");
        }
        return "federated-ok";
    }

    if (method_id == "scratchbird.auth.workload_identity") {
        const std::string token =
            !identity.workload_identity_token.empty() ? identity.workload_identity_token : secret;
        if (token.empty()) {
            throw MakeReject("SRB1-R-4005",
                             "workload identity requires token payload",
                             "connection",
                             "identity_handshake");
        }
        if (!federated_acquire(identity.provider_id, token)) {
            throw MakeReject("SRB1-R-4005",
                             "workload identity token validation failed",
                             "connection",
                             "identity_handshake");
        }
        return "workload-identity-ok";
    }

    if (method_id == "scratchbird.auth.proxy_assertion") {
        const std::string assertion =
            !identity.proxy_principal_assertion.empty() ? identity.proxy_principal_assertion : secret;
        if (assertion.empty()) {
            throw MakeReject("SRB1-R-4005",
                             "proxy assertion payload missing",
                             "connection",
                             "identity_handshake");
        }
        return "proxy-assertion-ok";
    }

    if (method_id == "scratchbird.auth.directory_bind" ||
        method_id == "scratchbird.auth.ldap" ||
        method_id == "scratchbird.auth.kerberos" ||
        method_id == "scratchbird.auth.ldap_bind" ||
        method_id == "scratchbird.auth.kerberos_gssapi" ||
        method_id == "scratchbird.auth.ident_rfc1413" ||
        method_id == "scratchbird.auth.radius_pap" ||
        method_id == "scratchbird.auth.pam_conversation") {
        if (!directory_bind(identity.provider_id, secret)) {
            throw MakeReject("SRB1-R-4005", "directory bind failed", "connection", "identity_handshake");
        }
        return "directory-ok";
    }

    throw MakeReject("SRB1-R-4005",
                     "unsupported method/profile combination",
                     "connection",
                     "identity_handshake",
                     false,
                     method_id);
}

SessionFingerprint ConnectEnterprise(
    const EnterpriseConnectionProfile& profile,
    const std::optional<std::string>& runtime_override,
    const std::function<std::optional<std::string>(const SecretProviderContract&)>& provider_fetch,
    const std::function<std::optional<std::string>(const std::string&)>& credential_fetch,
    const std::function<bool(const std::string&, const std::string&)>& federated_acquire,
    const std::function<bool(const std::string&, const std::string&)>& directory_bind) {
    ValidateTransport(profile);

    std::optional<std::string> credential_id;
    if (profile.ssh.has_value() && !profile.ssh->credential_id.empty()) {
        credential_id = profile.ssh->credential_id;
    }

    const std::string secret = ResolveSecret(runtime_override,
                                             provider_fetch,
                                             profile.secret_provider,
                                             credential_fetch,
                                             credential_id,
                                             profile.inline_secret,
                                             profile.allow_inline_secret);
    (void)RunIdentityHandshake(profile.identity, secret, federated_acquire, directory_bind);

    SessionFingerprint out;
    out.profile_id = profile.profile_id;
    out.transport_mode = profile.transport.mode;
    out.identity_mode = ToLower(Trim(profile.identity.mode));
    out.identity_method_id = ToLower(Trim(profile.identity.auth_method_id));
    out.identity_provider_profile = Trim(profile.identity.provider_profile);
    if (out.identity_method_id.empty()) {
        out.identity_method_id = DefaultIdentityMethodForMode(out.identity_mode);
    }
    out.auth_require_channel_binding = profile.identity.auth_require_channel_binding;
    out.auth_required_methods = profile.identity.auth_required_methods;
    out.auth_forbidden_methods = profile.identity.auth_forbidden_methods;
    out.no_login_direct = profile.no_login_direct;
    out.proxy_assertion_only = profile.proxy_assertion_only;
    if (profile.transport.mode == "direct") {
        out.backend_route = "direct";
    } else if (profile.transport.mode == "ssh_tunnel") {
        out.backend_route = "ssh_tunnel:" + profile.ssh->target_host + ":" + std::to_string(profile.ssh->target_port);
    } else {
        std::vector<std::string> hops;
        hops.reserve(profile.jump_hosts.size());
        for (const auto& hop : profile.jump_hosts) {
            hops.push_back(hop.host + ":" + std::to_string(hop.port));
        }
        out.backend_route = "ssh_jump_chain:" + std::to_string(profile.jump_hosts.size()) + ":" + Join(hops, "->") +
                            "->" + profile.ssh->target_host + ":" + std::to_string(profile.ssh->target_port);
    }
    return out;
}

// -------------------------
// Project/data contracts
// -------------------------

std::uint32_t Crc32(const std::uint8_t* data, std::size_t len) {
    static std::uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1U)) : (c >> 1U);
            }
            table[i] = c;
        }
        init = true;
    }
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

void ValidateHeader(const HeaderV1& header, std::uint64_t actual_size, const std::uint8_t* raw44) {
    if (header.magic != kMagicSrpj) {
        throw MakeReject("SRB1-R-3101", "bad magic", "project", "validate_header");
    }
    if (header.major != 1U || header.header_size != kHeaderSize || header.toc_entry_size != kTocEntrySize) {
        throw MakeReject("SRB1-R-3101", "bad fixed header fields", "project", "validate_header");
    }
    if (header.declared_file_size == 0 || header.declared_file_size != actual_size) {
        throw MakeReject("SRB1-R-3101", "declared file size mismatch", "project", "validate_header");
    }
    if (header.flags != 0 || header.reserved0 != 0) {
        throw MakeReject("SRB1-R-3101", "non-zero reserved/flags", "project", "validate_header");
    }

    std::array<std::uint8_t, 44> temp{};
    for (size_t i = 0; i < temp.size(); ++i) {
        temp[i] = raw44[i];
    }
    temp[40] = 0;
    temp[41] = 0;
    temp[42] = 0;
    temp[43] = 0;
    const std::uint32_t crc = Crc32(temp.data(), temp.size());
    if (crc != header.header_crc32) {
        throw MakeReject("SRB1-R-3101", "header crc mismatch", "project", "validate_header");
    }
}

LoadedProjectBinary LoadProjectBinary(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 44U) {
        throw MakeReject("SRB1-R-3101", "file too small", "project", "load_project_binary");
    }

    HeaderV1 header;
    header.magic = ReadU32(bytes, 0);
    header.major = ReadU16(bytes, 4);
    header.minor = ReadU16(bytes, 6);
    header.header_size = ReadU16(bytes, 8);
    header.toc_entry_size = ReadU16(bytes, 10);
    header.chunk_count = ReadU32(bytes, 12);
    header.toc_offset = ReadU64(bytes, 16);
    header.declared_file_size = ReadU64(bytes, 24);
    header.flags = ReadU32(bytes, 32);
    header.reserved0 = ReadU32(bytes, 36);
    header.header_crc32 = ReadU32(bytes, 40);

    ValidateHeader(header, static_cast<std::uint64_t>(bytes.size()), bytes.data());

    const std::uint64_t toc_bytes = static_cast<std::uint64_t>(header.chunk_count) * kTocEntrySize;
    if (header.toc_offset + toc_bytes > bytes.size()) {
        throw MakeReject("SRB1-R-3101", "toc range out of file", "project", "load_project_binary");
    }

    LoadedProjectBinary out;
    out.header = header;

    std::set<std::string> mandatory = {"PROJ", "OBJS"};

    for (std::uint32_t i = 0; i < header.chunk_count; ++i) {
        size_t off = static_cast<size_t>(header.toc_offset + static_cast<std::uint64_t>(i) * kTocEntrySize);

        TocEntry row;
        row.chunk_id.assign(reinterpret_cast<const char*>(&bytes[off]), 4);
        row.chunk_flags = ReadU32(bytes, off + 4);
        row.data_offset = ReadU64(bytes, off + 8);
        row.data_size = ReadU64(bytes, off + 16);
        row.data_crc32 = ReadU32(bytes, off + 24);
        row.payload_version = ReadU16(bytes, off + 28);
        row.reserved0 = ReadU16(bytes, off + 30);
        row.chunk_ordinal = ReadU32(bytes, off + 32);
        row.reserved1 = ReadU32(bytes, off + 36);

        if (row.chunk_flags != 0 || row.reserved0 != 0 || row.reserved1 != 0 || row.payload_version != 1) {
            throw MakeReject("SRB1-R-3101", "invalid toc row fields", "project", "load_project_binary", false, row.chunk_id);
        }

        const bool in_range = row.data_offset <= bytes.size() && row.data_offset + row.data_size <= bytes.size();
        if (!in_range) {
            if (mandatory.count(row.chunk_id) > 0) {
                throw MakeReject("SRB1-R-3101", "mandatory chunk out of range", "project", "load_project_binary", false,
                                 row.chunk_id);
            }
            continue;
        }

        const auto* payload = bytes.data() + static_cast<size_t>(row.data_offset);
        const std::uint32_t crc = Crc32(payload, static_cast<size_t>(row.data_size));
        if (crc != row.data_crc32) {
            if (mandatory.count(row.chunk_id) > 0) {
                throw MakeReject("SRB1-R-3101", "mandatory chunk crc mismatch", "project", "load_project_binary", false,
                                 row.chunk_id);
            }
            continue;
        }

        out.toc.push_back(row);
        out.loaded_chunks.insert(row.chunk_id);
    }

    for (const auto& required : mandatory) {
        if (out.loaded_chunks.count(required) == 0U) {
            throw MakeReject("SRB1-R-3101", "missing mandatory chunk", "project", "load_project_binary", false, required);
        }
    }

    return out;
}

void ValidateProjectPayload(const JsonValue& payload) {
    if (payload.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-3002", "project payload must be object", "project", "validate_project_payload");
    }

    const auto require_object = [&](const JsonValue& parent, const std::string& key) -> const JsonValue& {
        const JsonValue* value = FindMember(parent, key);
        if (value == nullptr || value->type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-3002", "missing object field: " + key, "project", "validate_project_payload");
        }
        return *value;
    };
    const auto require_array = [&](const JsonValue& parent, const std::string& key) -> const JsonValue& {
        const JsonValue* value = FindMember(parent, key);
        if (value == nullptr || value->type != JsonValue::Type::Array) {
            throw MakeReject("SRB1-R-3002", "missing array field: " + key, "project", "validate_project_payload");
        }
        return *value;
    };
    const auto require_string = [&](const JsonValue& parent, const std::string& key, bool non_empty) -> std::string {
        const JsonValue* value = FindMember(parent, key);
        std::string out;
        if (value == nullptr || !GetStringValue(*value, &out) || (non_empty && out.empty())) {
            throw MakeReject("SRB1-R-3002", "invalid string field: " + key, "project", "validate_project_payload");
        }
        return out;
    };
    const auto require_bool = [&](const JsonValue& parent, const std::string& key) -> bool {
        const JsonValue* value = FindMember(parent, key);
        bool out = false;
        if (value == nullptr || !GetBoolValue(*value, &out)) {
            throw MakeReject("SRB1-R-3002", "invalid bool field: " + key, "project", "validate_project_payload");
        }
        return out;
    };
    const auto require_int_min = [&](const JsonValue& parent, const std::string& key, int64_t min) -> int64_t {
        const JsonValue* value = FindMember(parent, key);
        int64_t out = 0;
        if (value == nullptr || !GetInt64Value(*value, &out) || out < min) {
            throw MakeReject("SRB1-R-3002", "invalid integer field: " + key, "project", "validate_project_payload");
        }
        return out;
    };
    const auto ensure_only_fields = [&](const JsonValue& object,
                                        const std::set<std::string>& allowed,
                                        const std::string& context) {
        for (const auto& [key, _] : object.object_value) {
            if (allowed.count(key) == 0U) {
                throw MakeReject("SRB1-R-3002", "unexpected field in " + context + ": " + key, "project",
                                 "validate_project_payload");
            }
        }
    };
    const auto is_uuid = [&](const std::string& value) -> bool {
        static const std::regex re(R"(^[0-9a-f]{8}-[0-9a-f]{4}-[1-8][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$)");
        return std::regex_match(value.begin(), value.end(), re);
    };
    const auto is_rel_path = [&](const std::string& value) -> bool {
        return !value.empty() && value.find("..") == std::string::npos &&
               (value[0] != '/' && value.find(':') == std::string::npos);
    };

    ensure_only_fields(payload, {"project"}, "root");
    const JsonValue& project = require_object(payload, "project");
    ensure_only_fields(project,
                       {"project_id", "name", "created_at", "updated_at", "config", "objects", "objects_by_path",
                        "reporting_assets", "reporting_schedules", "data_view_snapshots", "git_sync_state",
                        "audit_log_path"},
                       "project");

    const std::string project_id = require_string(project, "project_id", true);
    if (!is_uuid(project_id)) {
        throw MakeReject("SRB1-R-3002", "invalid project_id", "project", "validate_project_payload");
    }
    (void)require_string(project, "name", true);
    const std::string created_at = require_string(project, "created_at", true);
    const std::string updated_at = require_string(project, "updated_at", true);
    if (!IsRfc3339Utc(created_at) || !IsRfc3339Utc(updated_at)) {
        throw MakeReject("SRB1-R-3002", "invalid project timestamps", "project", "validate_project_payload");
    }
    if (updated_at < created_at) {
        throw MakeReject("SRB1-R-3002", "updated_at earlier than created_at", "project", "validate_project_payload");
    }

    const JsonValue& config = require_object(project, "config");
    ensure_only_fields(config,
                       {"default_environment_id", "active_connection_id", "connections_file_path", "governance",
                        "security_mode", "features"},
                       "project.config");
    (void)require_string(config, "default_environment_id", true);
    const JsonValue* active_connection_id = FindMember(config, "active_connection_id");
    if (active_connection_id == nullptr ||
        !(active_connection_id->type == JsonValue::Type::Null ||
          (active_connection_id->type == JsonValue::Type::String && is_uuid(active_connection_id->string_value)))) {
        throw MakeReject("SRB1-R-3002", "invalid active_connection_id", "project", "validate_project_payload");
    }
    const std::string connections_file_path = require_string(config, "connections_file_path", true);
    if (!is_rel_path(connections_file_path)) {
        throw MakeReject("SRB1-R-3002", "invalid connections_file_path", "project", "validate_project_payload");
    }

    const JsonValue& governance = require_object(config, "governance");
    ensure_only_fields(governance,
                       {"owners", "stewards", "review_min_approvals", "allowed_roles_by_environment", "ai_policy",
                        "audit_policy"},
                       "project.config.governance");
    const JsonValue& owners = require_array(governance, "owners");
    if (owners.array_value.empty()) {
        throw MakeReject("SRB1-R-3002", "governance.owners cannot be empty", "project", "validate_project_payload");
    }
    std::set<std::string> owner_names;
    for (const auto& row : owners.array_value) {
        if (row.type != JsonValue::Type::String || row.string_value.empty() || owner_names.count(row.string_value) > 0U) {
            throw MakeReject("SRB1-R-3002", "invalid governance owner entry", "project", "validate_project_payload");
        }
        owner_names.insert(row.string_value);
    }
    const JsonValue& stewards = require_array(governance, "stewards");
    std::set<std::string> steward_names;
    for (const auto& row : stewards.array_value) {
        if (row.type != JsonValue::Type::String || row.string_value.empty() || steward_names.count(row.string_value) > 0U) {
            throw MakeReject("SRB1-R-3002", "invalid governance steward entry", "project", "validate_project_payload");
        }
        steward_names.insert(row.string_value);
    }
    (void)require_int_min(governance, "review_min_approvals", 1);
    const JsonValue& allowed_roles = require_object(governance, "allowed_roles_by_environment");
    for (const auto& [env_id, role_array] : allowed_roles.object_value) {
        if (env_id.empty() || role_array.type != JsonValue::Type::Array) {
            throw MakeReject("SRB1-R-3002", "invalid allowed_roles_by_environment", "project", "validate_project_payload");
        }
        std::set<std::string> roles;
        for (const auto& role : role_array.array_value) {
            if (role.type != JsonValue::Type::String || role.string_value.empty() || roles.count(role.string_value) > 0U) {
                throw MakeReject("SRB1-R-3002", "invalid role entry", "project", "validate_project_payload");
            }
            roles.insert(role.string_value);
        }
    }
    const JsonValue& ai_policy = require_object(governance, "ai_policy");
    ensure_only_fields(ai_policy, {"enabled", "require_review", "allow_scopes", "deny_scopes"},
                       "project.config.governance.ai_policy");
    (void)require_bool(ai_policy, "enabled");
    (void)require_bool(ai_policy, "require_review");
    for (const auto& key : {"allow_scopes", "deny_scopes"}) {
        const JsonValue& scopes = require_array(ai_policy, key);
        std::set<std::string> seen;
        for (const auto& row : scopes.array_value) {
            if (row.type != JsonValue::Type::String || row.string_value.empty() || seen.count(row.string_value) > 0U) {
                throw MakeReject("SRB1-R-3002", "invalid ai scope entry", "project", "validate_project_payload");
            }
            seen.insert(row.string_value);
        }
    }
    const JsonValue& audit_policy = require_object(governance, "audit_policy");
    ensure_only_fields(audit_policy, {"level", "retention_days", "export_enabled"},
                       "project.config.governance.audit_policy");
    const std::string audit_level = require_string(audit_policy, "level", true);
    if (audit_level != "minimal" && audit_level != "standard" && audit_level != "verbose") {
        throw MakeReject("SRB1-R-3002", "invalid audit level", "project", "validate_project_payload");
    }
    (void)require_int_min(audit_policy, "retention_days", 1);
    (void)require_bool(audit_policy, "export_enabled");

    const std::string security_mode = require_string(config, "security_mode", true);
    if (security_mode != "standard" && security_mode != "hardened") {
        throw MakeReject("SRB1-R-3002", "invalid security_mode", "project", "validate_project_payload");
    }
    const JsonValue& features = require_object(config, "features");
    for (const auto& [name, enabled] : features.object_value) {
        if (name.empty() || enabled.type != JsonValue::Type::Bool) {
            throw MakeReject("SRB1-R-3002", "invalid feature flag", "project", "validate_project_payload");
        }
    }

    static const std::set<std::string> object_kinds = {"schema", "table", "index", "domain", "sequence", "view",
                                                        "trigger", "procedure", "function", "package", "job", "user", "role"};
    static const std::set<std::string> design_states = {"EXTRACTED", "NEW", "MODIFIED", "DELETED", "PENDING",
                                                         "APPROVED", "REJECTED", "IMPLEMENTED", "CONFLICTED"};
    const JsonValue& objects = require_array(project, "objects");
    std::set<std::string> object_ids;
    std::set<std::string> object_paths;
    for (const auto& row : objects.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-3002", "invalid project object row", "project", "validate_project_payload");
        }
        ensure_only_fields(row,
                           {"id", "kind", "name", "path", "schema_name", "design_state", "has_source", "source_snapshot",
                            "current_design", "comments", "change_history", "design_file_path"},
                           "project.objects[]");
        const std::string id = require_string(row, "id", true);
        const std::string kind = require_string(row, "kind", true);
        const std::string name = require_string(row, "name", true);
        const std::string path = require_string(row, "path", true);
        if (!is_uuid(id) || object_ids.count(id) > 0U) {
            throw MakeReject("SRB1-R-3002", "invalid/duplicate project object id", "project", "validate_project_payload");
        }
        if (object_kinds.count(kind) == 0U || name.empty() || !is_rel_path(path) || object_paths.count(path) > 0U) {
            throw MakeReject("SRB1-R-3002", "invalid project object identity", "project", "validate_project_payload");
        }
        object_ids.insert(id);
        object_paths.insert(path);

        const JsonValue* schema_name = FindMember(row, "schema_name");
        if (schema_name == nullptr ||
            !(schema_name->type == JsonValue::Type::Null ||
              (schema_name->type == JsonValue::Type::String))) {
            throw MakeReject("SRB1-R-3002", "invalid schema_name", "project", "validate_project_payload");
        }
        const std::string design_state = require_string(row, "design_state", true);
        if (design_states.count(design_state) == 0U) {
            throw MakeReject("SRB1-R-3002", "invalid design_state", "project", "validate_project_payload");
        }
        const bool has_source = require_bool(row, "has_source");
        const JsonValue* source_snapshot = FindMember(row, "source_snapshot");
        if (source_snapshot == nullptr ||
            !(source_snapshot->type == JsonValue::Type::Null ||
              source_snapshot->type == JsonValue::Type::String)) {
            throw MakeReject("SRB1-R-3002", "invalid source_snapshot", "project", "validate_project_payload");
        }
        if (has_source &&
            !(source_snapshot->type == JsonValue::Type::String && !source_snapshot->string_value.empty())) {
            throw MakeReject("SRB1-R-3002", "has_source requires source_snapshot", "project", "validate_project_payload");
        }
        for (const auto& key : {"current_design", "comments"}) {
            const JsonValue* text = FindMember(row, key);
            if (text == nullptr ||
                !(text->type == JsonValue::Type::Null || text->type == JsonValue::Type::String)) {
                throw MakeReject("SRB1-R-3002", "invalid nullable text field", "project", "validate_project_payload");
            }
        }
        const JsonValue& history = require_array(row, "change_history");
        for (const auto& entry : history.array_value) {
            if (entry.type != JsonValue::Type::Object) {
                throw MakeReject("SRB1-R-3002", "invalid change_history row", "project", "validate_project_payload");
            }
            ensure_only_fields(entry,
                               {"timestamp", "actor", "action", "state_before", "state_after", "note"},
                               "project.objects[].change_history[]");
            const std::string ts = require_string(entry, "timestamp", true);
            const std::string actor = require_string(entry, "actor", true);
            const std::string action = require_string(entry, "action", true);
            const std::string before = require_string(entry, "state_before", true);
            const std::string after = require_string(entry, "state_after", true);
            if (!IsRfc3339Utc(ts) || actor.empty() || action.empty() || design_states.count(before) == 0U ||
                design_states.count(after) == 0U) {
                throw MakeReject("SRB1-R-3002", "invalid change_history fields", "project", "validate_project_payload");
            }
            const JsonValue* note = FindMember(entry, "note");
            if (note == nullptr || !(note->type == JsonValue::Type::Null || note->type == JsonValue::Type::String)) {
                throw MakeReject("SRB1-R-3002", "invalid change_history note", "project", "validate_project_payload");
            }
        }
        const JsonValue* design_file_path = FindMember(row, "design_file_path");
        if (design_file_path == nullptr ||
            !(design_file_path->type == JsonValue::Type::Null ||
              (design_file_path->type == JsonValue::Type::String &&
               (design_file_path->string_value.empty() || is_rel_path(design_file_path->string_value))))) {
            throw MakeReject("SRB1-R-3002", "invalid design_file_path", "project", "validate_project_payload");
        }
    }

    const JsonValue& objects_by_path = require_object(project, "objects_by_path");
    for (const auto& [path, object_id] : objects_by_path.object_value) {
        if (!is_rel_path(path) || object_id.type != JsonValue::Type::String || !is_uuid(object_id.string_value) ||
            object_paths.count(path) == 0U || object_ids.count(object_id.string_value) == 0U) {
            throw MakeReject("SRB1-R-3002", "invalid objects_by_path entry", "project", "validate_project_payload");
        }
    }

    const JsonValue& reporting_assets = require_array(project, "reporting_assets");
    std::set<std::string> reporting_asset_ids;
    static const std::set<std::string> reporting_asset_types = {"Question", "Dashboard", "Model", "Metric", "Segment",
                                                                 "Alert", "Subscription", "Collection", "Timeline"};
    for (const auto& row : reporting_assets.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-3002", "invalid reporting asset row", "project", "validate_project_payload");
        }
        ensure_only_fields(row,
                           {"id", "asset_type", "name", "collection_id", "created_at", "updated_at", "created_by",
                            "updated_by", "payload_json"},
                           "project.reporting_assets[]");
        const std::string id = require_string(row, "id", true);
        const std::string asset_type = require_string(row, "asset_type", true);
        const std::string name = require_string(row, "name", true);
        const std::string created = require_string(row, "created_at", true);
        const std::string updated = require_string(row, "updated_at", true);
        const std::string payload_json = require_string(row, "payload_json", true);
        if (!is_uuid(id) || reporting_asset_ids.count(id) > 0U || reporting_asset_types.count(asset_type) == 0U ||
            name.empty() || payload_json.empty() || !IsRfc3339Utc(created) || !IsRfc3339Utc(updated)) {
            throw MakeReject("SRB1-R-3002", "invalid reporting asset fields", "project", "validate_project_payload");
        }
        reporting_asset_ids.insert(id);
        const JsonValue* collection = FindMember(row, "collection_id");
        if (collection == nullptr ||
            !(collection->type == JsonValue::Type::Null ||
              (collection->type == JsonValue::Type::String && is_uuid(collection->string_value)))) {
            throw MakeReject("SRB1-R-3002", "invalid reporting collection_id", "project", "validate_project_payload");
        }
        for (const auto& key : {"created_by", "updated_by"}) {
            const JsonValue* user = FindMember(row, key);
            if (user == nullptr || !(user->type == JsonValue::Type::Null || user->type == JsonValue::Type::String)) {
                throw MakeReject("SRB1-R-3002", "invalid reporting user field", "project", "validate_project_payload");
            }
        }
    }

    const JsonValue& reporting_schedules = require_array(project, "reporting_schedules");
    for (const auto& row : reporting_schedules.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-3002", "invalid reporting schedule row", "project", "validate_project_payload");
        }
        ensure_only_fields(row,
                           {"id", "asset_id", "schedule_kind", "schedule_spec", "schedule_dtstart_local", "timezone",
                            "schedule_rdates_local", "schedule_exdates_local", "enabled", "next_run_at"},
                           "project.reporting_schedules[]");
        const std::string schedule_id = require_string(row, "id", true);
        const std::string asset_id = require_string(row, "asset_id", true);
        const std::string schedule_kind = require_string(row, "schedule_kind", true);
        ReportingSchedule schedule;
        schedule.schedule_spec = require_string(row, "schedule_spec", true);
        schedule.schedule_dtstart_local = require_string(row, "schedule_dtstart_local", true);
        schedule.timezone = require_string(row, "timezone", true);
        if (!is_uuid(schedule_id) || !is_uuid(asset_id) || reporting_asset_ids.count(asset_id) == 0U ||
            schedule_kind != "RRULE") {
            throw MakeReject("SRB1-R-3002", "invalid reporting schedule identity", "project", "validate_project_payload");
        }
        const JsonValue& rdates = require_array(row, "schedule_rdates_local");
        for (const auto& dt : rdates.array_value) {
            if (dt.type != JsonValue::Type::String || !IsLocalDateTime(dt.string_value)) {
                throw MakeReject("SRB1-R-3002", "invalid schedule_rdates_local", "project", "validate_project_payload");
            }
            schedule.schedule_rdates_local.push_back(dt.string_value);
        }
        const JsonValue& exdates = require_array(row, "schedule_exdates_local");
        for (const auto& dt : exdates.array_value) {
            if (dt.type != JsonValue::Type::String || !IsLocalDateTime(dt.string_value)) {
                throw MakeReject("SRB1-R-3002", "invalid schedule_exdates_local", "project", "validate_project_payload");
            }
            schedule.schedule_exdates_local.push_back(dt.string_value);
        }
        (void)require_bool(row, "enabled");
        const JsonValue* next_run_at = FindMember(row, "next_run_at");
        if (next_run_at == nullptr ||
            !(next_run_at->type == JsonValue::Type::Null ||
              (next_run_at->type == JsonValue::Type::String && IsRfc3339Utc(next_run_at->string_value)))) {
            throw MakeReject("SRB1-R-3002", "invalid schedule next_run_at", "project", "validate_project_payload");
        }
        ValidateAnchorUntil(schedule);
    }

    const JsonValue& snapshots = require_array(project, "data_view_snapshots");
    for (const auto& row : snapshots.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-3002", "invalid data_view_snapshot row", "project", "validate_project_payload");
        }
        ensure_only_fields(row, {"id", "source_ref", "captured_at", "schema_hash", "payload_json"},
                           "project.data_view_snapshots[]");
        const std::string id = require_string(row, "id", true);
        const std::string source_ref = require_string(row, "source_ref", true);
        const std::string captured_at = require_string(row, "captured_at", true);
        const std::string schema_hash = require_string(row, "schema_hash", true);
        const std::string payload_json = require_string(row, "payload_json", true);
        if (!is_uuid(id) || source_ref.empty() || !IsRfc3339Utc(captured_at) || schema_hash.empty() || payload_json.empty()) {
            throw MakeReject("SRB1-R-3002", "invalid data_view_snapshot fields", "project", "validate_project_payload");
        }
    }

    const JsonValue* git_sync = FindMember(project, "git_sync_state");
    if (git_sync == nullptr ||
        !(git_sync->type == JsonValue::Type::Null || git_sync->type == JsonValue::Type::Object)) {
        throw MakeReject("SRB1-R-3002", "invalid git_sync_state", "project", "validate_project_payload");
    }
    if (git_sync->type == JsonValue::Type::Object) {
        ensure_only_fields(*git_sync,
                           {"enabled", "project_repo_head", "project_repo_branch", "database_repo_head",
                            "database_repo_branch", "dirty_files", "last_sync_at", "sync_status"},
                           "project.git_sync_state");
        (void)require_bool(*git_sync, "enabled");
        for (const auto& key : {"project_repo_head", "project_repo_branch", "database_repo_head", "database_repo_branch"}) {
            const JsonValue* value = FindMember(*git_sync, key);
            if (value == nullptr || !(value->type == JsonValue::Type::Null || value->type == JsonValue::Type::String)) {
                throw MakeReject("SRB1-R-3002", "invalid git sync text field", "project", "validate_project_payload");
            }
        }
        const JsonValue& dirty_files = require_array(*git_sync, "dirty_files");
        std::set<std::string> unique_dirty;
        for (const auto& row : dirty_files.array_value) {
            if (row.type != JsonValue::Type::String || !is_rel_path(row.string_value) ||
                unique_dirty.count(row.string_value) > 0U) {
                throw MakeReject("SRB1-R-3002", "invalid dirty_files entry", "project", "validate_project_payload");
            }
            unique_dirty.insert(row.string_value);
        }
        const JsonValue* last_sync_at = FindMember(*git_sync, "last_sync_at");
        if (last_sync_at == nullptr ||
            !(last_sync_at->type == JsonValue::Type::Null ||
              (last_sync_at->type == JsonValue::Type::String && IsRfc3339Utc(last_sync_at->string_value)))) {
            throw MakeReject("SRB1-R-3002", "invalid git_sync_state.last_sync_at", "project", "validate_project_payload");
        }
        const std::string sync_status = require_string(*git_sync, "sync_status", true);
        if (sync_status != "clean" && sync_status != "dirty" && sync_status != "conflicted" && sync_status != "unknown") {
            throw MakeReject("SRB1-R-3002", "invalid git_sync_state.sync_status", "project", "validate_project_payload");
        }
    }

    const std::string audit_log_path = require_string(project, "audit_log_path", true);
    if (!is_rel_path(audit_log_path)) {
        throw MakeReject("SRB1-R-3002", "invalid audit_log_path", "project", "validate_project_payload");
    }
}

void ValidateSpecsetPayload(const JsonValue& payload) {
    if (payload.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-5402", "specset payload must be object", "spec_workspace", "validate_specset_payload");
    }

    const auto require_array = [&](const JsonValue& parent, const std::string& key) -> const JsonValue& {
        const JsonValue* value = FindMember(parent, key);
        if (value == nullptr || value->type != JsonValue::Type::Array) {
            throw MakeReject("SRB1-R-5402", "missing array field: " + key, "spec_workspace", "validate_specset_payload");
        }
        return *value;
    };
    const auto require_string = [&](const JsonValue& parent, const std::string& key, bool non_empty) -> std::string {
        const JsonValue* value = FindMember(parent, key);
        std::string out;
        if (value == nullptr || !GetStringValue(*value, &out) || (non_empty && out.empty())) {
            throw MakeReject("SRB1-R-5402", "invalid string field: " + key, "spec_workspace", "validate_specset_payload");
        }
        return out;
    };
    const auto ensure_only_fields = [&](const JsonValue& object,
                                        const std::set<std::string>& allowed,
                                        const std::string& context) {
        for (const auto& [key, _] : object.object_value) {
            if (allowed.count(key) == 0U) {
                throw MakeReject("SRB1-R-5402", "unexpected field in " + context + ": " + key,
                                 "spec_workspace", "validate_specset_payload");
            }
        }
    };
    const auto is_rel_path = [&](const std::string& value) -> bool {
        return !value.empty() && value.find("..") == std::string::npos &&
               (value[0] != '/' && value.find(':') == std::string::npos);
    };
    const auto is_sha256 = [&](const std::string& value) -> bool {
        static const std::regex re(R"(^[0-9a-f]{64}$)");
        return std::regex_match(value.begin(), value.end(), re);
    };
    const auto is_set_id = [&](const std::string& value) -> bool {
        return value == "sb_v3" || value == "sb_vnext" || value == "sb_beta1";
    };

    ensure_only_fields(payload, {"spec_sets", "spec_files", "coverage_links", "conformance_bindings"}, "root");
    const JsonValue& spec_sets = require_array(payload, "spec_sets");
    const JsonValue& spec_files = require_array(payload, "spec_files");
    const JsonValue& coverage_links = require_array(payload, "coverage_links");
    const JsonValue& conformance_bindings = require_array(payload, "conformance_bindings");

    for (const auto& row : spec_sets.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-5402", "invalid spec_sets row", "spec_workspace", "validate_specset_payload");
        }
        ensure_only_fields(row,
                           {"set_id", "package_manifest_ref", "package_root", "authoritative_inventory_relpath",
                            "version_stamp", "package_hash_sha256", "last_indexed_at", "index_status", "index_error"},
                           "spec_sets[]");
        const std::string set_id = require_string(row, "set_id", true);
        const std::string package_manifest_ref = require_string(row, "package_manifest_ref", true);
        const std::string package_root = require_string(row, "package_root", true);
        const std::string authoritative_inventory_relpath =
            require_string(row, "authoritative_inventory_relpath", true);
        const std::string version_stamp = require_string(row, "version_stamp", true);
        const std::string package_hash_sha256 = require_string(row, "package_hash_sha256", true);
        if (!is_set_id(set_id) || !is_rel_path(package_manifest_ref) || !is_rel_path(package_root) ||
            !is_rel_path(authoritative_inventory_relpath) || version_stamp.empty() || !is_sha256(package_hash_sha256)) {
            throw MakeReject("SRB1-R-5402", "invalid spec_set fields", "spec_workspace", "validate_specset_payload");
        }
        const JsonValue* indexed_at = FindMember(row, "last_indexed_at");
        if (indexed_at == nullptr ||
            !(indexed_at->type == JsonValue::Type::Null ||
              (indexed_at->type == JsonValue::Type::String && IsRfc3339Utc(indexed_at->string_value)))) {
            throw MakeReject("SRB1-R-5402", "invalid last_indexed_at", "spec_workspace", "validate_specset_payload");
        }
        const std::string index_status = require_string(row, "index_status", true);
        if (index_status != "unindexed" && index_status != "indexed" && index_status != "stale" && index_status != "error") {
            throw MakeReject("SRB1-R-5402", "invalid index_status", "spec_workspace", "validate_specset_payload");
        }
        const JsonValue* index_error = FindMember(row, "index_error");
        if (index_error == nullptr ||
            !(index_error->type == JsonValue::Type::Null || index_error->type == JsonValue::Type::String)) {
            throw MakeReject("SRB1-R-5402", "invalid index_error", "spec_workspace", "validate_specset_payload");
        }
    }

    for (const auto& row : spec_files.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-5402", "invalid spec_files row", "spec_workspace", "validate_specset_payload");
        }
        ensure_only_fields(row,
                           {"set_id", "section_id", "relative_path", "is_normative", "file_role", "content_hash",
                            "last_seen_at", "size_bytes"},
                           "spec_files[]");
        const std::string set_id = require_string(row, "set_id", true);
        const std::string section_id = require_string(row, "section_id", true);
        const std::string relative_path = require_string(row, "relative_path", true);
        const std::string file_role = require_string(row, "file_role", true);
        const std::string content_hash = require_string(row, "content_hash", true);
        const std::string last_seen_at = require_string(row, "last_seen_at", true);
        int64_t size_bytes = 0;
        const JsonValue* size_value = FindMember(row, "size_bytes");
        if (size_value == nullptr || !GetInt64Value(*size_value, &size_bytes) || size_bytes < 0) {
            throw MakeReject("SRB1-R-5402", "invalid size_bytes", "spec_workspace", "validate_specset_payload");
        }
        const JsonValue* is_normative = FindMember(row, "is_normative");
        if (is_normative == nullptr || is_normative->type != JsonValue::Type::Bool) {
            throw MakeReject("SRB1-R-5402", "invalid is_normative", "spec_workspace", "validate_specset_payload");
        }
        static const std::set<std::string> file_roles = {"readme", "spec_outline", "decision", "dependencies",
                                                          "test_contract", "contract", "matrix", "registry", "vector", "other"};
        if (!is_set_id(set_id) || section_id.empty() || !is_rel_path(relative_path) || file_roles.count(file_role) == 0U ||
            !is_sha256(content_hash) || !IsRfc3339Utc(last_seen_at)) {
            throw MakeReject("SRB1-R-5402", "invalid spec_files fields", "spec_workspace", "validate_specset_payload");
        }
    }

    static const std::regex spec_ref_re(R"(^(sb_v3|sb_vnext|sb_beta1):.+$)");
    for (const auto& row : coverage_links.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-5402", "invalid coverage_links row", "spec_workspace", "validate_specset_payload");
        }
        ensure_only_fields(row,
                           {"spec_file_ref", "robin_surface_or_service_id", "coverage_class", "coverage_state",
                            "conformance_case_id", "last_updated_at"},
                           "coverage_links[]");
        const std::string spec_file_ref = require_string(row, "spec_file_ref", true);
        const std::string surface_id = require_string(row, "robin_surface_or_service_id", true);
        const std::string coverage_class = require_string(row, "coverage_class", true);
        const std::string coverage_state = require_string(row, "coverage_state", true);
        const std::string last_updated_at = require_string(row, "last_updated_at", true);
        if (!std::regex_match(spec_file_ref.begin(), spec_file_ref.end(), spec_ref_re) || surface_id.empty() ||
            (coverage_class != "design" && coverage_class != "development" && coverage_class != "management") ||
            (coverage_state != "covered" && coverage_state != "partial" && coverage_state != "missing") ||
            !IsRfc3339Utc(last_updated_at)) {
            throw MakeReject("SRB1-R-5402", "invalid coverage_links fields", "spec_workspace", "validate_specset_payload");
        }
        const JsonValue* case_id = FindMember(row, "conformance_case_id");
        if (case_id == nullptr || !(case_id->type == JsonValue::Type::Null || case_id->type == JsonValue::Type::String)) {
            throw MakeReject("SRB1-R-5402", "invalid conformance_case_id", "spec_workspace", "validate_specset_payload");
        }
    }

    for (const auto& row : conformance_bindings.array_value) {
        if (row.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-5402", "invalid conformance_bindings row", "spec_workspace", "validate_specset_payload");
        }
        ensure_only_fields(row, {"binding_id", "spec_file_ref", "case_id", "binding_kind", "notes"},
                           "conformance_bindings[]");
        const std::string binding_id = require_string(row, "binding_id", true);
        const std::string spec_file_ref = require_string(row, "spec_file_ref", true);
        const std::string case_id = require_string(row, "case_id", true);
        const std::string binding_kind = require_string(row, "binding_kind", true);
        if (binding_id.empty() || !std::regex_match(spec_file_ref.begin(), spec_file_ref.end(), spec_ref_re) ||
            case_id.empty() || (binding_kind != "required" && binding_kind != "supporting")) {
            throw MakeReject("SRB1-R-5402", "invalid conformance_bindings fields", "spec_workspace",
                             "validate_specset_payload");
        }
        const JsonValue* notes = FindMember(row, "notes");
        if (notes == nullptr || !(notes->type == JsonValue::Type::Null || notes->type == JsonValue::Type::String)) {
            throw MakeReject("SRB1-R-5402", "invalid conformance binding notes", "spec_workspace", "validate_specset_payload");
        }
    }
}

void WriteAuditRequired(const std::string& audit_path, const std::string& event_json_line) {
    std::ofstream out(audit_path, std::ios::binary | std::ios::app);
    if (!out) {
        throw MakeReject("SRB1-R-3201", "audit write failure: open", "governance", "write_audit_required", false, audit_path);
    }
    out << event_json_line;
    out << '\n';
    out.flush();
    if (!out) {
        throw MakeReject("SRB1-R-3201", "audit write failure: flush", "governance", "write_audit_required", false, audit_path);
    }
#if defined(__unix__) || defined(__APPLE__)
    const int fd = ::open(audit_path.c_str(), O_RDONLY);
    if (fd >= 0) {
        if (::fsync(fd) != 0) {
            ::close(fd);
            throw MakeReject("SRB1-R-3201", "audit write failure: fsync", "governance", "write_audit_required", false,
                             audit_path);
        }
        ::close(fd);
    }
#endif
}

// -------------------------
// Governance contracts
// -------------------------

void ValidateBlockerRows(const std::vector<BlockerRow>& rows) {
    static const std::set<std::string> severities = {"P0", "P1", "P2"};
    static const std::set<std::string> statuses = {"open", "mitigated", "waived", "closed"};
    static const std::set<std::string> source_types = {"reject_code", "conformance_case", "manual"};
    static const std::regex blk_id(R"(^BLK-[0-9]{4}$)");

    for (const auto& row : rows) {
        if (!std::regex_match(row.blocker_id, blk_id)) {
            throw MakeReject("SRB1-R-5407", "invalid blocker id", "governance", "validate_blockers");
        }
        if (severities.count(row.severity) == 0 || statuses.count(row.status) == 0 || source_types.count(row.source_type) == 0) {
            throw MakeReject("SRB1-R-5407", "invalid blocker enum value", "governance", "validate_blockers");
        }
        if (!IsRfc3339Utc(row.opened_at) || !IsRfc3339Utc(row.updated_at) || row.owner.empty() || row.summary.empty()) {
            throw MakeReject("SRB1-R-5407", "invalid blocker row fields", "governance", "validate_blockers");
        }
        if (row.status == "waived") {
            if (row.source_type != "manual") {
                throw MakeReject("SRB1-R-5407", "waived requires manual source", "governance", "validate_blockers");
            }
            if (row.summary.find("ga-only") == std::string::npos && row.summary.find("preview-only") == std::string::npos) {
                throw MakeReject("SRB1-R-5407", "waived requires profile scope in summary", "governance", "validate_blockers");
            }
        }
    }
}

void ValidateRejectCodeReferences(const std::set<std::string>& referenced_codes,
                                  const std::set<std::string>& registry_codes) {
    for (const auto& code : referenced_codes) {
        if (!IsValidRejectCodeFormat(code)) {
            throw MakeReject("SRB1-R-5407", "invalid reject code format", "governance", "validate_reject_registry", false, code);
        }
        if (registry_codes.count(code) == 0) {
            throw MakeReject("SRB1-R-5407", "unregistered reject code", "governance", "validate_reject_registry", false, code);
        }
    }
}

void EnforceGovernanceGate(bool allowed,
                           const std::function<void()>& apply_action,
                           const std::function<void(const std::string&)>& audit_write) {
    if (!allowed) {
        audit_write("denied");
        throw MakeReject("SRB1-R-3202", "governance policy denied action", "governance", "enforce_governance_gate");
    }
    apply_action();
}

// -------------------------
// UI workflow contracts
// -------------------------

std::vector<std::string> SortedSuggestions(const std::vector<SuggestionCandidate>& candidates,
                                           const std::string& prefix,
                                           const std::function<int(std::string_view, std::string_view)>& fuzzy_distance) {
    if (candidates.empty()) {
        throw MakeReject("SRB1-R-5102", "assist context unavailable", "ui", "sorted_suggestions");
    }
    struct Ranked {
        int context_weight = 0;
        int exact = 1;
        int fuzzy = 0;
        std::string token;
    };

    std::vector<Ranked> ranked;
    ranked.reserve(candidates.size());
    for (const auto& c : candidates) {
        if (c.token.empty()) {
            throw MakeReject("SRB1-R-5102", "invalid candidate token", "ui", "sorted_suggestions");
        }
        Ranked r;
        r.context_weight = c.context_weight;
        r.exact = c.token.rfind(prefix, 0) == 0 ? 0 : 1;
        r.fuzzy = fuzzy_distance(c.token, prefix);
        r.token = c.token;
        ranked.push_back(std::move(r));
    }

    std::sort(ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b) {
        return std::tie(a.context_weight, a.exact, a.fuzzy, a.token) < std::tie(b.context_weight, b.exact, b.fuzzy, b.token);
    });

    std::vector<std::string> out;
    out.reserve(ranked.size());
    for (const auto& r : ranked) {
        out.push_back(r.token);
    }
    return out;
}

std::string SnippetInsertExact(const Snippet& snippet) {
    if (snippet.snippet_id.empty() || snippet.name.empty() || snippet.body.empty()) {
        throw MakeReject("SRB1-R-5103", "snippet missing required fields", "ui", "snippet_insert_exact");
    }
    if (snippet.scope != "global" && snippet.scope != "project" && snippet.scope != "connection") {
        throw MakeReject("SRB1-R-5103", "invalid snippet scope", "ui", "snippet_insert_exact");
    }
    return snippet.body;
}

std::vector<QueryHistoryRow> PruneHistory(const std::vector<QueryHistoryRow>& rows,
                                          const std::string& cutoff_utc,
                                          std::size_t max_rows) {
    if (!IsRfc3339Utc(cutoff_utc)) {
        throw MakeReject("SRB1-R-5104", "invalid cutoff timestamp", "ui", "prune_history");
    }
    std::vector<QueryHistoryRow> filtered;
    filtered.reserve(rows.size());
    for (const auto& row : rows) {
        if (!IsRfc3339Utc(row.started_at_utc)) {
            throw MakeReject("SRB1-R-5104", "invalid history row timestamp", "ui", "prune_history", false, row.query_id);
        }
        if (row.started_at_utc >= cutoff_utc) {
            filtered.push_back(row);
        }
    }
    std::sort(filtered.begin(), filtered.end(), [](const QueryHistoryRow& a, const QueryHistoryRow& b) {
        return std::tie(a.started_at_utc, a.query_id) < std::tie(b.started_at_utc, b.query_id);
    });
    if (filtered.size() > max_rows) {
        filtered.erase(filtered.begin(), filtered.end() - static_cast<std::ptrdiff_t>(max_rows));
    }
    return filtered;
}

std::string ExportHistoryCsv(const std::vector<QueryHistoryRow>& rows) {
    auto sorted = rows;
    std::sort(sorted.begin(), sorted.end(), [](const QueryHistoryRow& a, const QueryHistoryRow& b) {
        return std::tie(a.started_at_utc, a.query_id) > std::tie(b.started_at_utc, b.query_id);
    });

    auto csv_escape = [](std::string s) {
        if (s.find(',') != std::string::npos || s.find('"') != std::string::npos) {
            std::string out = "\"";
            for (char c : s) {
                if (c == '"') {
                    out += "\"\"";
                } else {
                    out.push_back(c);
                }
            }
            out += '"';
            return out;
        }
        return s;
    };

    std::ostringstream out;
    out << "query_id,profile_id,started_at_utc,duration_ms,status,error_code,sql_hash\n";
    for (const auto& row : sorted) {
        out << csv_escape(row.query_id) << ',' << csv_escape(row.profile_id) << ',' << csv_escape(row.started_at_utc) << ','
            << row.duration_ms << ',' << csv_escape(row.status) << ',' << csv_escape(row.error_code) << ','
            << csv_escape(row.sql_hash) << '\n';
    }
    return out.str();
}

std::vector<SchemaCompareOperation> StableSortOps(const std::vector<SchemaCompareOperation>& ops) {
    static const std::map<std::string, int> kClassOrder = {{"schema", 0},    {"domain", 1},  {"table", 2},  {"column", 3},
                                                            {"constraint", 4}, {"index", 5},   {"view", 6},   {"routine", 7},
                                                            {"trigger", 8},    {"sequence", 9}, {"grant", 10}};
    static const std::map<std::string, int> kOpOrder = {{"add", 0}, {"alter", 1}, {"rename", 2}, {"drop", 3}};

    auto sorted = ops;
    for (const auto& op : sorted) {
        if (kClassOrder.count(op.object_class) == 0 || kOpOrder.count(op.operation_type) == 0 || op.object_path.empty() ||
            op.operation_id.empty()) {
            throw MakeReject("SRB1-R-5105", "invalid schema compare operation", "ui", "stable_sort_ops");
        }
    }

    std::sort(sorted.begin(), sorted.end(), [&](const SchemaCompareOperation& a, const SchemaCompareOperation& b) {
        return std::tie(kClassOrder.at(a.object_class), a.object_path, kOpOrder.at(a.operation_type), a.operation_id) <
               std::tie(kClassOrder.at(b.object_class), b.object_path, kOpOrder.at(b.operation_type), b.operation_id);
    });
    return sorted;
}

DataCompareResult RunDataCompareKeyed(const std::vector<DataCompareRow>& left,
                                      const std::vector<DataCompareRow>& right) {
    std::map<std::string, DataCompareRow> lmap;
    std::map<std::string, DataCompareRow> rmap;

    auto add_rows = [](const std::vector<DataCompareRow>& in, std::map<std::string, DataCompareRow>* out, const std::string& side) {
        for (const auto& row : in) {
            if (row.key.empty()) {
                throw MakeReject("SRB1-R-5105", "empty compare key", "ui", "run_data_compare_keyed", false, side);
            }
            const std::string key = ToKey(row.key);
            if (out->count(key) > 0U) {
                throw MakeReject("SRB1-R-5105", "duplicate compare key", "ui", "run_data_compare_keyed", false, key);
            }
            (*out)[key] = row;
        }
    };

    add_rows(left, &lmap, "left");
    add_rows(right, &rmap, "right");

    DataCompareResult result;
    std::set<std::string> all_keys;
    for (const auto& [k, _] : lmap) {
        all_keys.insert(k);
    }
    for (const auto& [k, _] : rmap) {
        all_keys.insert(k);
    }

    for (const auto& k : all_keys) {
        auto lit = lmap.find(k);
        auto rit = rmap.find(k);
        if (lit == lmap.end()) {
            result.only_right.push_back(rit->second);
            continue;
        }
        if (rit == rmap.end()) {
            result.only_left.push_back(lit->second);
            continue;
        }
        if (lit->second.payload.type == rit->second.payload.type && lit->second.payload.string_value == rit->second.payload.string_value) {
            result.equal.push_back({lit->second, rit->second});
        } else {
            result.different.push_back({lit->second, rit->second});
        }
    }
    return result;
}

std::string GenerateMigrationScript(const std::vector<SchemaCompareOperation>& ops,
                                    const std::string& compare_timestamp_utc,
                                    const std::string& left_source,
                                    const std::string& right_source) {
    if (!IsRfc3339Utc(compare_timestamp_utc) || left_source.empty() || right_source.empty()) {
        throw MakeReject("SRB1-R-5106", "invalid migration metadata", "ui", "generate_migration_script");
    }

    auto sorted = StableSortOps(ops);

    std::ostringstream body;
    for (const auto& op : sorted) {
        if (op.ddl_statement.empty()) {
            throw MakeReject("SRB1-R-5106", "missing ddl_statement", "ui", "generate_migration_script", false, op.operation_id);
        }
        body << op.ddl_statement;
        if (op.ddl_statement.back() != ';') {
            body << ';';
        }
        body << '\n';
    }

    std::string body_str = body.str();
    std::string hash = Sha256Hex(body_str);

    std::ostringstream out;
    out << "-- ScratchRobin Migration Script\n";
    out << "-- compare_timestamp_utc: " << compare_timestamp_utc << "\n";
    out << "-- left_source: " << left_source << "\n";
    out << "-- right_source: " << right_source << "\n";
    out << "-- operation_count: " << sorted.size() << "\n";
    out << "-- script_hash_sha256: " << hash << "\n";
    out << body_str;
    return out.str();
}

std::map<int, std::vector<PlanNode>> OrderPlanNodes(const std::vector<PlanNode>& nodes) {
    std::map<int, std::vector<PlanNode>> grouped;
    for (const auto& node : nodes) {
        if (node.operator_name.empty()) {
            throw MakeReject("SRB1-R-5107", "invalid plan node", "ui", "order_plan_nodes");
        }
        grouped[node.parent_id].push_back(node);
    }
    for (auto& [_, siblings] : grouped) {
        std::sort(siblings.begin(), siblings.end(), [](const PlanNode& a, const PlanNode& b) {
            return std::tie(a.estimated_cost, a.node_id) < std::tie(b.estimated_cost, b.node_id);
        });
    }
    return grouped;
}

BuilderApplyResult ApplyBuilderGraph(bool has_unsupported_construct,
                                     bool strict_builder,
                                     const std::string& emitted_sql,
                                     bool canonical_equivalent) {
    if (has_unsupported_construct) {
        if (strict_builder) {
            throw MakeReject("SRB1-R-5108", "unsupported construct in strict mode", "ui", "apply_builder_graph");
        }
        return BuilderApplyResult{"read_only", ""};
    }
    if (emitted_sql.empty() || !canonical_equivalent) {
        throw MakeReject("SRB1-R-5108", "round-trip mismatch", "ui", "apply_builder_graph");
    }
    return BuilderApplyResult{"editable", emitted_sql};
}

std::vector<std::pair<std::string, std::string>> BuildToolsMenu() {
    return {
        {"Spec Workspace", "open_spec_workspace"},
        {"Reporting", "open_reporting"},
        {"Data Masking", "open_data_masking"},
    };
}

std::string CoverageBadge(const std::string& design, const std::string& development, const std::string& management) {
    const std::set<std::string> states = {design, development, management};
    if (states == std::set<std::string>{"covered"}) {
        return "covered";
    }
    if (states.count("missing") > 0U) {
        return "missing";
    }
    return "partial";
}

void ApplySecurityPolicyAction(bool has_permission,
                               const std::string& permission_key,
                               const std::function<void()>& action) {
    if (!has_permission) {
        throw MakeReject("SRB1-R-8301", "permission denied: " + permission_key, "ui", "security_policy_action");
    }
    action();
}

void ValidateEmbeddedDetachedExclusivity(
    const std::map<std::string, SurfaceVisibilityState>& visibility_by_surface) {
    if (visibility_by_surface.empty()) {
        throw MakeReject("SRB1-R-5101", "no surface visibility states provided", "ui", "validate_window_exclusivity");
    }
    for (const auto& [surface_id, state] : visibility_by_surface) {
        if (surface_id.empty()) {
            throw MakeReject("SRB1-R-5101", "surface id is required", "ui", "validate_window_exclusivity");
        }
        if (state.embedded_visible && state.detached_visible) {
            throw MakeReject("SRB1-R-5101",
                             "embedded and detached surface visibility conflict",
                             "ui",
                             "validate_window_exclusivity",
                             false,
                             surface_id);
        }
    }
}

SurfaceVisibilityState ApplyDockingRule(bool detached_visible,
                                        bool dock_action_requested,
                                        double overlap_ratio) {
    if (overlap_ratio < 0.0 || overlap_ratio > 1.0) {
        throw MakeReject("SRB1-R-5101", "invalid overlap ratio", "ui", "apply_docking_rule");
    }

    if (dock_action_requested || (detached_visible && overlap_ratio >= 0.70)) {
        return SurfaceVisibilityState{.embedded_visible = true, .detached_visible = false};
    }

    if (detached_visible) {
        return SurfaceVisibilityState{.embedded_visible = false, .detached_visible = true};
    }

    return SurfaceVisibilityState{.embedded_visible = false, .detached_visible = false};
}

void ValidateUiWorkflowState(const std::string& workflow_id,
                             bool capability_ready,
                             bool state_ready) {
    if (workflow_id.empty() || !capability_ready || !state_ready) {
        throw MakeReject("SRB1-R-5101", "workflow unavailable/invalid state", "ui", "validate_workflow_state", false,
                         workflow_id);
    }
}

std::string ResolveIconSlot(const std::string& slot,
                            const std::map<std::string, std::string>& icon_map,
                            const std::string& fallback_icon) {
    if (slot.empty() || fallback_icon.empty()) {
        throw MakeReject("SRB1-R-5101", "invalid icon slot contract", "ui", "resolve_icon_slot");
    }
    auto it = icon_map.find(slot);
    if (it == icon_map.end() || it->second.empty()) {
        return fallback_icon;
    }
    return it->second;
}

std::string BuildSpecWorkspaceSummary(const std::map<std::string, int>& gap_counts) {
    for (const std::string key : {"design", "development", "management"}) {
        auto it = gap_counts.find(key);
        if (it == gap_counts.end() || it->second < 0) {
            throw MakeReject("SRB1-R-5405", "invalid support dashboard query/export request", "spec_workspace",
                             "build_summary");
        }
    }
    std::ostringstream out;
    out << "{\"design\":" << gap_counts.at("design")
        << ",\"development\":" << gap_counts.at("development")
        << ",\"management\":" << gap_counts.at("management")
        << ",\"total\":" << (gap_counts.at("design") + gap_counts.at("development") + gap_counts.at("management")) << "}";
    return out.str();
}

// -------------------------
// Diagram contracts
// -------------------------

std::vector<std::string> PaletteTokensForDiagramType(const std::string& diagram_type) {
    const std::string canonical = ToLower(Trim(diagram_type));
    if (canonical == "erd") {
        return {"table", "view", "index", "domain", "note", "relation"};
    }
    if (canonical == "silverston") {
        return {"subject_area", "entity", "fact", "dimension", "lookup", "hub", "link", "satellite"};
    }
    if (canonical == "whiteboard") {
        return {"note", "task", "risk", "decision", "milestone"};
    }
    if (canonical == "mindmap" || canonical == "mind map") {
        return {"topic", "branch", "idea", "question", "action"};
    }
    throw MakeReject("SRB1-R-6201", "unsupported diagram type for palette", "diagram", "palette_tokens", false, diagram_type);
}

void ValidatePaletteModeExclusivity(bool docked_visible, bool floating_visible) {
    if (docked_visible && floating_visible) {
        throw MakeReject("SRB1-R-6201", "palette docked/floating mode conflict", "diagram", "validate_palette_mode");
    }
}

DiagramNode BuildNodeFromPaletteToken(const std::string& diagram_type,
                                      const std::string& token,
                                      int x,
                                      int y,
                                      int width,
                                      int height) {
    const auto allowed_tokens = PaletteTokensForDiagramType(diagram_type);
    const std::string canonical_token = ToLower(Trim(token));
    if (canonical_token.empty() ||
        std::find(allowed_tokens.begin(), allowed_tokens.end(), canonical_token) == allowed_tokens.end()) {
        throw MakeReject("SRB1-R-6201", "invalid palette token payload", "diagram", "build_node_from_palette_token",
                         false, token);
    }
    if (width <= 0 || height <= 0) {
        throw MakeReject("SRB1-R-6201", "invalid palette drop geometry", "diagram", "build_node_from_palette_token");
    }

    DiagramNode node;
    node.node_id = canonical_token + ":" + std::to_string(x) + ":" + std::to_string(y);
    node.object_type = canonical_token;
    node.parent_node_id = "";
    node.x = x;
    node.y = y;
    node.width = width;
    node.height = height;
    return node;
}

void ValidateNotation(const std::string& notation) {
    static const std::set<std::string> allowed = {"crowsfoot", "idef1x", "uml", "chen"};
    const std::string canonical = ToLower(Trim(notation));
    if (allowed.count(canonical) == 0U) {
        throw MakeReject("SRB1-R-6101", "invalid/unresolvable diagram notation", "diagram", "validate_notation", false,
                         notation);
    }
}

void ValidateCanvasOperation(const DiagramDocument& doc,
                             const std::string& operation,
                             const std::string& node_id,
                             const std::string& target_parent_id) {
    ValidateNotation(doc.notation);
    const std::set<std::string> ops = {
        "drag", "resize", "connect", "reparent", "add_node", "remove_node", "add_edge", "remove_edge", "delete_node",
        "delete_project"};
    if (ops.count(operation) == 0U) {
        throw MakeReject("SRB1-R-6201", "invalid diagram operation", "diagram", "canvas_operation");
    }

    const auto node_it = std::find_if(doc.nodes.begin(), doc.nodes.end(), [&](const DiagramNode& n) { return n.node_id == node_id; });
    const bool requires_node = operation != "add_node" && operation != "delete_project";
    if (requires_node && node_id.empty()) {
        throw MakeReject("SRB1-R-6201", "missing node id", "diagram", "canvas_operation");
    }
    if (requires_node && node_it == doc.nodes.end()) {
        throw MakeReject("SRB1-R-6201", "node not found", "diagram", "canvas_operation", false, node_id);
    }

    if (operation == "connect" || operation == "add_edge" || operation == "remove_edge") {
        if (target_parent_id.empty()) {
            throw MakeReject("SRB1-R-6201", "missing edge target node", "diagram", "canvas_operation");
        }
        const auto target_it =
            std::find_if(doc.nodes.begin(), doc.nodes.end(), [&](const DiagramNode& n) { return n.node_id == target_parent_id; });
        if (target_it == doc.nodes.end()) {
            throw MakeReject("SRB1-R-6201", "target node not found", "diagram", "canvas_operation", false, target_parent_id);
        }
        if (target_parent_id == node_id) {
            throw MakeReject("SRB1-R-6201", "self edge not allowed", "diagram", "canvas_operation", false, node_id);
        }
    }

    if (operation == "reparent") {
        if (node_id.empty()) {
            throw MakeReject("SRB1-R-6201", "missing node id", "diagram", "canvas_operation");
        }
        if (!target_parent_id.empty()) {
            const auto target_it =
                std::find_if(doc.nodes.begin(), doc.nodes.end(), [&](const DiagramNode& n) { return n.node_id == target_parent_id; });
            if (target_it == doc.nodes.end()) {
                throw MakeReject("SRB1-R-6201", "target parent not found", "diagram", "canvas_operation", false, target_parent_id);
            }
            if (target_parent_id == node_id) {
                throw MakeReject("SRB1-R-6201", "self-parenting not allowed", "diagram", "canvas_operation", false, node_id);
            }
            std::map<std::string, std::string> parent_by_node;
            for (const auto& n : doc.nodes) {
                parent_by_node[n.node_id] = n.parent_node_id;
            }
            std::set<std::string> visited;
            std::string cursor = target_parent_id;
            while (!cursor.empty()) {
                if (!visited.insert(cursor).second) {
                    throw MakeReject("SRB1-R-6201", "parent cycle detected", "diagram", "canvas_operation", false, cursor);
                }
                if (cursor == node_id) {
                    throw MakeReject("SRB1-R-6201", "invalid parent-child cycle", "diagram", "canvas_operation", false,
                                     node_id + "->" + target_parent_id);
                }
                auto parent_it = parent_by_node.find(cursor);
                if (parent_it == parent_by_node.end()) {
                    break;
                }
                cursor = parent_it->second;
            }
        }
    }
}

std::string SerializeDiagramModel(const DiagramDocument& doc) {
    ValidateNotation(doc.notation);
    if (doc.diagram_id.empty()) {
        throw MakeReject("SRB1-R-6101", "diagram_id required", "diagram", "serialize_model");
    }
    const auto emit_string_array = [](std::ostringstream* out, const std::vector<std::string>& items) {
        *out << "[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) {
                *out << ",";
            }
            *out << "\"" << JsonEscape(items[i]) << "\"";
        }
        *out << "]";
    };

    std::ostringstream out;
    const int grid_size = doc.grid_size <= 0 ? 20 : doc.grid_size;
    const std::string alignment_policy = doc.alignment_policy.empty() ? "free" : ToLower(Trim(doc.alignment_policy));
    const std::string drop_policy = doc.drop_policy.empty() ? "containment" : ToLower(Trim(doc.drop_policy));
    const std::string resize_policy = doc.resize_policy.empty() ? "free" : ToLower(Trim(doc.resize_policy));
    const std::string display_profile = doc.display_profile.empty() ? "standard" : ToLower(Trim(doc.display_profile));

    out << "{\"diagram_id\":\"" << JsonEscape(doc.diagram_id)
        << "\",\"notation\":\"" << JsonEscape(ToLower(Trim(doc.notation)))
        << "\",\"diagram_type\":\"" << JsonEscape(doc.diagram_type.empty() ? "Erd" : doc.diagram_type)
        << "\",\"grid_size\":" << grid_size
        << ",\"alignment_policy\":\"" << JsonEscape(alignment_policy)
        << "\",\"drop_policy\":\"" << JsonEscape(drop_policy)
        << "\",\"resize_policy\":\"" << JsonEscape(resize_policy)
        << "\",\"display_profile\":\"" << JsonEscape(display_profile)
        << "\",\"nodes\":[";
    for (size_t i = 0; i < doc.nodes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& n = doc.nodes[i];
        if (n.node_id.empty() || n.object_type.empty()) {
            throw MakeReject("SRB1-R-6101", "invalid diagram node", "diagram", "serialize_model");
        }
        if (n.stack_count <= 0) {
            throw MakeReject("SRB1-R-6101", "stack_count must be >= 1", "diagram", "serialize_model");
        }
        std::vector<std::string> attributes = n.attributes;
        std::vector<std::string> tags = n.tags;
        std::vector<std::string> trace_refs = n.trace_refs;
        std::sort(tags.begin(), tags.end());
        tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
        std::sort(trace_refs.begin(), trace_refs.end());
        trace_refs.erase(std::unique(trace_refs.begin(), trace_refs.end()), trace_refs.end());

        out << "{\"node_id\":\"" << JsonEscape(n.node_id)
            << "\",\"name\":\"" << JsonEscape(n.name.empty() ? n.node_id : n.name)
            << "\",\"object_type\":\"" << JsonEscape(n.object_type)
            << "\",\"parent_node_id\":\"" << JsonEscape(n.parent_node_id)
            << "\",\"x\":" << n.x
            << ",\"y\":" << n.y
            << ",\"width\":" << n.width
            << ",\"height\":" << n.height
            << ",\"logical_datatype\":\"" << JsonEscape(n.logical_datatype)
            << "\",\"notes\":\"" << JsonEscape(n.notes)
            << "\",\"icon_slot\":\"" << JsonEscape(n.icon_slot)
            << "\",\"display_mode\":\"" << JsonEscape(n.display_mode.empty() ? "full" : ToLower(Trim(n.display_mode)))
            << "\",\"collapsed\":" << (n.collapsed ? "true" : "false")
            << ",\"pinned\":" << (n.pinned ? "true" : "false")
            << ",\"ghosted\":" << (n.ghosted ? "true" : "false")
            << ",\"stack_count\":" << n.stack_count
            << ",\"attributes\":";
        emit_string_array(&out, attributes);
        out << ",\"tags\":";
        emit_string_array(&out, tags);
        out << ",\"trace_refs\":";
        emit_string_array(&out, trace_refs);
        out << "}";
    }
    out << "],\"edges\":[";
    for (size_t i = 0; i < doc.edges.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& e = doc.edges[i];
        if (e.edge_id.empty() || e.from_node_id.empty() || e.to_node_id.empty()) {
            throw MakeReject("SRB1-R-6101", "invalid diagram edge", "diagram", "serialize_model");
        }
        out << "{\"edge_id\":\"" << JsonEscape(e.edge_id)
            << "\",\"from_node_id\":\"" << JsonEscape(e.from_node_id)
            << "\",\"to_node_id\":\"" << JsonEscape(e.to_node_id)
            << "\",\"relation_type\":\"" << JsonEscape(e.relation_type)
            << "\",\"label\":\"" << JsonEscape(e.label)
            << "\",\"edge_type\":\"" << JsonEscape(e.edge_type)
            << "\",\"directed\":" << (e.directed ? "true" : "false")
            << ",\"identifying\":" << (e.identifying ? "true" : "false")
            << ",\"source_cardinality\":\"" << JsonEscape(e.source_cardinality)
            << "\",\"target_cardinality\":\"" << JsonEscape(e.target_cardinality) << "\"}";
    }
    out << "]}";
    return out.str();
}

DiagramDocument ParseDiagramModel(const std::string& payload_json) {
    JsonParser parser(payload_json);
    JsonValue root;
    std::string err;
    if (!parser.Parse(&root, &err) || root.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-6101", "invalid diagram payload", "diagram", "parse_model", false, err);
    }
    DiagramDocument doc;
    doc.diagram_id = RequireString(root, "diagram_id", "SRB1-R-6101", "diagram", "parse_model");
    doc.notation = RequireString(root, "notation", "SRB1-R-6101", "diagram", "parse_model");
    doc.diagram_type = "Erd";
    const JsonValue* type_v = FindMember(root, "diagram_type");
    if (type_v != nullptr && type_v->type == JsonValue::Type::String && !type_v->string_value.empty()) {
        doc.diagram_type = type_v->string_value;
    }
    doc.grid_size = 20;
    doc.alignment_policy = "free";
    doc.drop_policy = "containment";
    doc.resize_policy = "free";
    doc.display_profile = "standard";
    const JsonValue* grid_size_v = FindMember(root, "grid_size");
    if (grid_size_v != nullptr) {
        int64_t parsed = 0;
        if (!GetInt64Value(*grid_size_v, &parsed)) {
            throw MakeReject("SRB1-R-6101", "invalid numeric field: grid_size", "diagram", "parse_model");
        }
        doc.grid_size = std::max<int>(1, static_cast<int>(parsed));
    }
    const JsonValue* align_v = FindMember(root, "alignment_policy");
    if (align_v != nullptr && align_v->type == JsonValue::Type::String && !align_v->string_value.empty()) {
        doc.alignment_policy = align_v->string_value;
    }
    const JsonValue* drop_v = FindMember(root, "drop_policy");
    if (drop_v != nullptr && drop_v->type == JsonValue::Type::String && !drop_v->string_value.empty()) {
        doc.drop_policy = drop_v->string_value;
    }
    const JsonValue* resize_v = FindMember(root, "resize_policy");
    if (resize_v != nullptr && resize_v->type == JsonValue::Type::String && !resize_v->string_value.empty()) {
        doc.resize_policy = resize_v->string_value;
    }
    const JsonValue* display_v = FindMember(root, "display_profile");
    if (display_v != nullptr && display_v->type == JsonValue::Type::String && !display_v->string_value.empty()) {
        doc.display_profile = display_v->string_value;
    }
    ValidateNotation(doc.notation);

    const auto read_int = [](const JsonValue& obj, const std::string& key, int fallback) {
        const JsonValue* v = FindMember(obj, key);
        if (v == nullptr) {
            return fallback;
        }
        int64_t parsed = 0;
        if (!GetInt64Value(*v, &parsed)) {
            throw MakeReject("SRB1-R-6101", "invalid numeric field: " + key, "diagram", "parse_model");
        }
        return static_cast<int>(parsed);
    };
    const auto read_bool = [](const JsonValue& obj, const std::string& key, bool fallback) {
        const JsonValue* v = FindMember(obj, key);
        if (v == nullptr) {
            return fallback;
        }
        bool parsed = false;
        if (!GetBoolValue(*v, &parsed)) {
            throw MakeReject("SRB1-R-6101", "invalid bool field: " + key, "diagram", "parse_model");
        }
        return parsed;
    };
    const auto read_string = [](const JsonValue& obj, const std::string& key) {
        const JsonValue* v = FindMember(obj, key);
        if (v == nullptr || v->type != JsonValue::Type::String) {
            return std::string();
        }
        return v->string_value;
    };
    const auto read_string_array = [](const JsonValue& obj, const std::string& key) {
        const JsonValue* v = FindMember(obj, key);
        if (v == nullptr) {
            return std::vector<std::string>{};
        }
        if (v->type != JsonValue::Type::Array) {
            throw MakeReject("SRB1-R-6101", "invalid array field: " + key, "diagram", "parse_model");
        }
        std::vector<std::string> out;
        out.reserve(v->array_value.size());
        for (const auto& item : v->array_value) {
            if (item.type != JsonValue::Type::String) {
                throw MakeReject("SRB1-R-6101", "invalid array item: " + key, "diagram", "parse_model");
            }
            out.push_back(item.string_value);
        }
        return out;
    };

    const JsonValue& nodes = RequireMember(root, "nodes", "SRB1-R-6101", "diagram", "parse_model");
    if (nodes.type != JsonValue::Type::Array) {
        throw MakeReject("SRB1-R-6101", "nodes must be array", "diagram", "parse_model");
    }
    for (const auto& v : nodes.array_value) {
        if (v.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-6101", "invalid node row", "diagram", "parse_model");
        }
        DiagramNode n;
        n.node_id = RequireString(v, "node_id", "SRB1-R-6101", "diagram", "parse_model");
        n.object_type = RequireString(v, "object_type", "SRB1-R-6101", "diagram", "parse_model");
        n.parent_node_id = read_string(v, "parent_node_id");
        n.x = read_int(v, "x", 0);
        n.y = read_int(v, "y", 0);
        n.width = read_int(v, "width", 100);
        n.height = read_int(v, "height", 50);
        n.logical_datatype = read_string(v, "logical_datatype");
        n.name = read_string(v, "name");
        n.attributes = read_string_array(v, "attributes");
        n.notes = read_string(v, "notes");
        n.tags = read_string_array(v, "tags");
        n.trace_refs = read_string_array(v, "trace_refs");
        n.icon_slot = read_string(v, "icon_slot");
        n.display_mode = read_string(v, "display_mode");
        n.collapsed = read_bool(v, "collapsed", false);
        n.pinned = read_bool(v, "pinned", false);
        n.ghosted = read_bool(v, "ghosted", false);
        n.stack_count = std::max(1, read_int(v, "stack_count", 1));
        doc.nodes.push_back(std::move(n));
    }

    const JsonValue& edges = RequireMember(root, "edges", "SRB1-R-6101", "diagram", "parse_model");
    if (edges.type != JsonValue::Type::Array) {
        throw MakeReject("SRB1-R-6101", "edges must be array", "diagram", "parse_model");
    }
    for (const auto& v : edges.array_value) {
        if (v.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-6101", "invalid edge row", "diagram", "parse_model");
        }
        DiagramEdge e;
        e.edge_id = RequireString(v, "edge_id", "SRB1-R-6101", "diagram", "parse_model");
        e.from_node_id = RequireString(v, "from_node_id", "SRB1-R-6101", "diagram", "parse_model");
        e.to_node_id = RequireString(v, "to_node_id", "SRB1-R-6101", "diagram", "parse_model");
        e.relation_type = read_string(v, "relation_type");
        e.label = read_string(v, "label");
        e.edge_type = read_string(v, "edge_type");
        e.directed = read_bool(v, "directed", true);
        e.identifying = read_bool(v, "identifying", false);
        e.source_cardinality = read_string(v, "source_cardinality");
        e.target_cardinality = read_string(v, "target_cardinality");
        doc.edges.push_back(std::move(e));
    }
    return doc;
}

std::vector<std::string> ForwardEngineerDatatypes(const std::vector<std::string>& logical_types,
                                                  const std::map<std::string, std::string>& mapping) {
    std::vector<std::string> out;
    out.reserve(logical_types.size());
    for (const auto& lt : logical_types) {
        auto it = mapping.find(lt);
        if (it == mapping.end() || it->second.empty()) {
            throw MakeReject("SRB1-R-6301", "unmappable datatype", "diagram", "forward_engineer", false, lt);
        }
        out.push_back(it->second);
    }
    return out;
}

std::string ExportDiagram(const DiagramDocument& doc, const std::string& format, const std::string& profile_id) {
    ValidateNotation(doc.notation);
    if (format != "svg" && format != "png" && format != "pdf") {
        throw MakeReject("SRB1-R-6303", "unsupported export format", "diagram", "export_diagram", false, format);
    }
    if (format == "pdf" && profile_id != "full" && profile_id != "preview") {
        throw MakeReject("SRB1-R-6303", "pdf export not enabled for profile", "diagram", "export_diagram", false, profile_id);
    }
    std::ostringstream out;
    out << "diagram-export:" << format << ":" << doc.diagram_id << ":" << doc.nodes.size() << ":" << doc.edges.size();
    return out.str();
}

// -------------------------
// Reporting contracts
// -------------------------

std::vector<ReportingAsset> CanonicalArtifactOrder(const std::vector<ReportingAsset>& rows) {
    auto out = rows;
    std::sort(out.begin(), out.end(), [](const ReportingAsset& a, const ReportingAsset& b) {
        return std::tie(a.asset_type, a.collection_id, a.id) < std::tie(b.asset_type, b.collection_id, b.id);
    });
    return out;
}

std::string CanonicalizeRRule(const std::map<std::string, std::string>& key_values) {
    static const std::set<std::string> allowed = {"FREQ",      "INTERVAL",  "COUNT",    "UNTIL",   "BYSECOND",
                                                  "BYMINUTE",  "BYHOUR",    "BYDAY",    "BYMONTHDAY", "BYYEARDAY",
                                                  "BYWEEKNO",  "BYMONTH",   "BYSETPOS", "WKST"};
    static const std::set<std::string> allowed_freq = {"SECONDLY", "MINUTELY", "HOURLY", "DAILY", "WEEKLY", "MONTHLY", "YEARLY"};
    static const std::set<std::string> weekday_tokens = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};

    auto parse_int = [&](const std::string& text, int min, int max, const std::string& key) {
        try {
            size_t idx = 0;
            int v = std::stoi(text, &idx, 10);
            if (idx != text.size() || v < min || v > max) {
                throw MakeReject("SRB1-R-7101", "invalid numeric value for " + key, "reporting", "canonicalize_rrule");
            }
            return v;
        } catch (const RejectError&) {
            throw;
        } catch (...) {
            throw MakeReject("SRB1-R-7101", "invalid numeric value for " + key, "reporting", "canonicalize_rrule");
        }
    };
    auto normalize_list = [&](const std::string& key, const std::string& value) -> std::string {
        std::vector<std::string> tokens = Split(value, ',');
        if (tokens.empty()) {
            throw MakeReject("SRB1-R-7101", "empty list for " + key, "reporting", "canonicalize_rrule");
        }
        std::vector<std::string> normalized;
        normalized.reserve(tokens.size());
        std::set<std::string> unique;
        for (auto& token : tokens) {
            token = Trim(token);
            if (token.empty()) {
                throw MakeReject("SRB1-R-7101", "empty token in " + key, "reporting", "canonicalize_rrule");
            }
            if (key == "BYSECOND") {
                token = std::to_string(parse_int(token, 0, 59, key));
            } else if (key == "BYMINUTE") {
                token = std::to_string(parse_int(token, 0, 59, key));
            } else if (key == "BYHOUR") {
                token = std::to_string(parse_int(token, 0, 23, key));
            } else if (key == "BYMONTH") {
                token = std::to_string(parse_int(token, 1, 12, key));
            } else if (key == "BYMONTHDAY") {
                int v = parse_int(token, -31, 31, key);
                if (v == 0) {
                    throw MakeReject("SRB1-R-7101", "BYMONTHDAY cannot contain zero", "reporting", "canonicalize_rrule");
                }
                token = std::to_string(v);
            } else if (key == "BYSETPOS") {
                int v = parse_int(token, -366, 366, key);
                if (v == 0) {
                    throw MakeReject("SRB1-R-7101", "BYSETPOS cannot contain zero", "reporting", "canonicalize_rrule");
                }
                token = std::to_string(v);
            } else if (key == "BYDAY") {
                std::string upper = ToUpper(token);
                if (weekday_tokens.count(upper) == 0U) {
                    static const std::regex ordinal_re(R"(^([+-]?[1-5])?(MO|TU|WE|TH|FR|SA|SU)$)");
                    if (!std::regex_match(upper.begin(), upper.end(), ordinal_re)) {
                        throw MakeReject("SRB1-R-7101", "invalid BYDAY token", "reporting", "canonicalize_rrule", false, token);
                    }
                }
                token = upper;
            } else if (key == "WKST") {
                std::string upper = ToUpper(token);
                if (weekday_tokens.count(upper) == 0U) {
                    throw MakeReject("SRB1-R-7101", "invalid WKST token", "reporting", "canonicalize_rrule", false, token);
                }
                token = upper;
            }
            if (unique.insert(token).second) {
                normalized.push_back(token);
            }
        }
        std::sort(normalized.begin(), normalized.end());
        return Join(normalized, ",");
    };

    if (key_values.count("FREQ") == 0U) {
        throw MakeReject("SRB1-R-7101", "FREQ required", "reporting", "canonicalize_rrule");
    }

    std::vector<std::string> keys;
    keys.reserve(key_values.size());
    for (const auto& [k, v] : key_values) {
        if (k != ToUpper(k) || allowed.count(k) == 0U || v.empty()) {
            throw MakeReject("SRB1-R-7101", "unsupported key/value " + k, "reporting", "canonicalize_rrule");
        }
        if (v.find(' ') != std::string::npos) {
            throw MakeReject("SRB1-R-7101", "spaces not allowed in rrule values", "reporting", "canonicalize_rrule");
        }
        if (k == "FREQ") {
            const std::string freq = ToUpper(v);
            if (allowed_freq.count(freq) == 0U) {
                throw MakeReject("SRB1-R-7101", "invalid FREQ", "reporting", "canonicalize_rrule");
            }
        } else if (k == "INTERVAL") {
            (void)parse_int(v, 1, 1000000, k);
        } else if (k == "COUNT") {
            (void)parse_int(v, 1, 1000000, k);
        } else if (k == "UNTIL") {
            if (!IsRfc3339Utc(v)) {
                throw MakeReject("SRB1-R-7101", "invalid UNTIL", "reporting", "canonicalize_rrule");
            }
        } else if (k == "BYSECOND" || k == "BYMINUTE" || k == "BYHOUR" || k == "BYDAY" || k == "BYMONTHDAY" ||
                   k == "BYMONTH" || k == "BYSETPOS" || k == "WKST") {
            (void)normalize_list(k, v);
        }
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());

    std::vector<std::string> pairs;
    pairs.reserve(keys.size());
    for (const auto& k : keys) {
        if (k == "BYSECOND" || k == "BYMINUTE" || k == "BYHOUR" || k == "BYDAY" || k == "BYMONTHDAY" ||
            k == "BYMONTH" || k == "BYSETPOS" || k == "WKST") {
            pairs.push_back(k + "=" + normalize_list(k, key_values.at(k)));
        } else if (k == "FREQ") {
            pairs.push_back(k + "=" + ToUpper(key_values.at(k)));
        } else {
            pairs.push_back(k + "=" + key_values.at(k));
        }
    }
    return Join(pairs, ";");
}

void ValidateAnchorUntil(const ReportingSchedule& schedule) {
    if (!IsTimezoneIana(schedule.timezone)) {
        throw MakeReject("SRB1-R-7102", "invalid timezone", "reporting", "validate_anchor_until");
    }
    const auto anchor = ParseLocalAsUtc(schedule.schedule_dtstart_local);
    if (!anchor.has_value()) {
        throw MakeReject("SRB1-R-7102", "invalid local anchor datetime", "reporting", "validate_anchor_until");
    }

    std::map<std::string, std::string> kv;
    for (const auto& pair : Split(schedule.schedule_spec, ';')) {
        auto pos = pair.find('=');
        if (pos == std::string::npos) {
            throw MakeReject("SRB1-R-7101", "invalid schedule specification", "reporting", "validate_anchor_until");
        }
        kv[pair.substr(0, pos)] = pair.substr(pos + 1);
    }

    (void)CanonicalizeRRule(kv);
    auto it = kv.find("UNTIL");
    if (it != kv.end()) {
        const auto until = ParseUtc(it->second);
        if (!until.has_value()) {
            throw MakeReject("SRB1-R-7101", "invalid UNTIL", "reporting", "validate_anchor_until");
        }
        if (*until < *anchor) {
            throw MakeReject("SRB1-R-7104", "UNTIL earlier than anchor", "reporting", "validate_anchor_until");
        }
    }
}

std::vector<std::string> ExpandRRuleBounded(const ReportingSchedule& schedule,
                                            const std::string& now_utc,
                                            std::size_t max_candidates) {
    ValidateAnchorUntil(schedule);

    const auto now = ParseUtc(now_utc);
    if (!now.has_value()) {
        throw MakeReject("SRB1-R-7102", "invalid now_utc", "reporting", "expand_rrule_bounded");
    }

    std::map<std::string, std::string> kv;
    for (const auto& pair : Split(schedule.schedule_spec, ';')) {
        auto pos = pair.find('=');
        if (pos == std::string::npos || pos == 0 || pos + 1 >= pair.size()) {
            throw MakeReject("SRB1-R-7101", "invalid schedule_spec token", "reporting", "expand_rrule_bounded", false,
                             pair);
        }
        kv[pair.substr(0, pos)] = pair.substr(pos + 1);
    }

    const std::string canonical = CanonicalizeRRule(kv);
    (void)canonical;

    const auto anchor = ParseLocalAsUtc(schedule.schedule_dtstart_local);
    if (!anchor.has_value()) {
        throw MakeReject("SRB1-R-7102", "invalid anchor", "reporting", "expand_rrule_bounded");
    }

    auto parse_int_list = [&](const std::string& key, int min, int max, bool disallow_zero = false) {
        std::vector<int> out;
        if (kv.count(key) == 0U) {
            return out;
        }
        for (const auto& token : Split(kv[key], ',')) {
            try {
                size_t idx = 0;
                int value = std::stoi(token, &idx, 10);
                if (idx != token.size() || value < min || value > max || (disallow_zero && value == 0)) {
                    throw MakeReject("SRB1-R-7101", "invalid " + key + " value", "reporting", "expand_rrule_bounded",
                                     false, token);
                }
                out.push_back(value);
            } catch (const RejectError&) {
                throw;
            } catch (...) {
                throw MakeReject("SRB1-R-7101", "invalid " + key + " value", "reporting", "expand_rrule_bounded",
                                 false, token);
            }
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    };
    auto parse_weekday_list = [&](const std::string& key) {
        std::set<int> weekdays;
        if (kv.count(key) == 0U) {
            return weekdays;
        }
        static const std::map<std::string, int> weekday_map = {
            {"SU", 0}, {"MO", 1}, {"TU", 2}, {"WE", 3}, {"TH", 4}, {"FR", 5}, {"SA", 6}};
        static const std::regex ordinal_re(R"(^([+-]?[1-5])?(MO|TU|WE|TH|FR|SA|SU)$)");
        for (auto token : Split(kv[key], ',')) {
            token = ToUpper(Trim(token));
            std::smatch match;
            if (!std::regex_match(token, match, ordinal_re)) {
                throw MakeReject("SRB1-R-7101", "invalid weekday token", "reporting", "expand_rrule_bounded", false, token);
            }
            weekdays.insert(weekday_map.at(match[2].str()));
        }
        return weekdays;
    };
    auto days_in_month = [](int year, int month_1_to_12) -> int {
        static const std::array<int, 12> base = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int days = base[month_1_to_12 - 1];
        if (month_1_to_12 == 2) {
            const bool leap = ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
            if (leap) {
                days = 29;
            }
        }
        return days;
    };
    auto to_tm = [](std::time_t t) {
        std::tm tm{};
#if defined(_WIN32)
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        return tm;
    };
    auto to_time_t = [](std::tm tm) -> std::time_t {
#if defined(_WIN32)
        return _mkgmtime(&tm);
#else
        return timegm(&tm);
#endif
    };
    auto add_months = [&](std::time_t base, int months) -> std::time_t {
        std::tm tm = to_tm(base);
        const int original_day = tm.tm_mday;
        int total_month = tm.tm_mon + months;
        int year_adjust = total_month / 12;
        if (total_month < 0 && (total_month % 12) != 0) {
            year_adjust -= 1;
        }
        tm.tm_year += year_adjust;
        total_month -= year_adjust * 12;
        tm.tm_mon = total_month;
        tm.tm_mday = std::min(original_day, days_in_month(tm.tm_year + 1900, tm.tm_mon + 1));
        return to_time_t(tm);
    };
    auto add_years = [&](std::time_t base, int years) -> std::time_t {
        std::tm tm = to_tm(base);
        tm.tm_year += years;
        tm.tm_mday = std::min(tm.tm_mday, days_in_month(tm.tm_year + 1900, tm.tm_mon + 1));
        return to_time_t(tm);
    };

    int interval = 1;
    if (kv.count("INTERVAL") > 0U) {
        interval = std::max(1, std::stoi(kv["INTERVAL"]));
    }

    int count_limit = -1;
    if (kv.count("COUNT") > 0U) {
        count_limit = std::max(0, std::stoi(kv["COUNT"]));
    }

    std::optional<std::time_t> until;
    if (kv.count("UNTIL") > 0U) {
        until = ParseUtc(kv["UNTIL"]);
        if (!until.has_value()) {
            throw MakeReject("SRB1-R-7101", "invalid UNTIL", "reporting", "expand_rrule_bounded");
        }
    }

    const std::string freq = kv["FREQ"];
    if (freq != "SECONDLY" && freq != "MINUTELY" && freq != "HOURLY" && freq != "DAILY" && freq != "WEEKLY" &&
        freq != "MONTHLY" && freq != "YEARLY") {
        throw MakeReject("SRB1-R-7101", "invalid FREQ", "reporting", "expand_rrule_bounded");
    }

    const std::vector<int> by_second = parse_int_list("BYSECOND", 0, 59);
    const std::vector<int> by_minute = parse_int_list("BYMINUTE", 0, 59);
    const std::vector<int> by_hour = parse_int_list("BYHOUR", 0, 23);
    const std::vector<int> by_month = parse_int_list("BYMONTH", 1, 12);
    const std::vector<int> by_monthday = parse_int_list("BYMONTHDAY", -31, 31, true);
    const std::vector<int> by_setpos = parse_int_list("BYSETPOS", -366, 366, true);
    const std::set<int> by_weekday = parse_weekday_list("BYDAY");

    std::set<std::string> out;
    std::time_t period_cursor = *anchor;
    std::size_t period_count = 0;
    int emitted = 0;

    while (period_count < max_candidates * 8U) {
        ++period_count;
        if (until.has_value() && period_cursor > *until) {
            break;
        }

        std::vector<std::time_t> period_candidates = {period_cursor};
        if (freq == "WEEKLY" && !by_weekday.empty()) {
            period_candidates.clear();
            std::tm base = to_tm(period_cursor);
            const int weekday = base.tm_wday;
            std::time_t week_start = period_cursor - static_cast<std::time_t>(weekday * 86400);
            for (int wd : by_weekday) {
                period_candidates.push_back(week_start + static_cast<std::time_t>(wd * 86400));
            }
        } else if ((freq == "MONTHLY" || freq == "YEARLY") && (!by_monthday.empty() || !by_weekday.empty() || !by_month.empty())) {
            period_candidates.clear();
            std::tm base = to_tm(period_cursor);
            std::vector<int> months;
            if (freq == "YEARLY" && !by_month.empty()) {
                months = by_month;
            } else {
                months = {base.tm_mon + 1};
            }
            for (int month : months) {
                int year = base.tm_year + 1900;
                int max_day = days_in_month(year, month);
                std::set<int> day_candidates;
                if (!by_monthday.empty()) {
                    for (int md : by_monthday) {
                        int day = md > 0 ? md : (max_day + md + 1);
                        if (day >= 1 && day <= max_day) {
                            day_candidates.insert(day);
                        }
                    }
                } else {
                    day_candidates.insert(base.tm_mday);
                }
                if (!by_weekday.empty()) {
                    std::set<int> filtered;
                    for (int day = 1; day <= max_day; ++day) {
                        std::tm tm{};
                        tm.tm_year = year - 1900;
                        tm.tm_mon = month - 1;
                        tm.tm_mday = day;
                        tm.tm_hour = base.tm_hour;
                        tm.tm_min = base.tm_min;
                        tm.tm_sec = base.tm_sec;
                        std::time_t t = to_time_t(tm);
                        if (by_weekday.count(to_tm(t).tm_wday) > 0U) {
                            if (by_monthday.empty() || day_candidates.count(day) > 0U) {
                                filtered.insert(day);
                            }
                        }
                    }
                    day_candidates = std::move(filtered);
                }
                for (int day : day_candidates) {
                    std::tm tm{};
                    tm.tm_year = year - 1900;
                    tm.tm_mon = month - 1;
                    tm.tm_mday = day;
                    tm.tm_hour = base.tm_hour;
                    tm.tm_min = base.tm_min;
                    tm.tm_sec = base.tm_sec;
                    period_candidates.push_back(to_time_t(tm));
                }
            }
        }

        std::vector<std::time_t> expanded;
        for (std::time_t base_ts : period_candidates) {
            std::vector<int> hours = by_hour.empty() ? std::vector<int>{to_tm(base_ts).tm_hour} : by_hour;
            std::vector<int> minutes = by_minute.empty() ? std::vector<int>{to_tm(base_ts).tm_min} : by_minute;
            std::vector<int> seconds = by_second.empty() ? std::vector<int>{to_tm(base_ts).tm_sec} : by_second;
            std::tm base_tm = to_tm(base_ts);
            for (int h : hours) {
                for (int m : minutes) {
                    for (int s : seconds) {
                        std::tm tm = base_tm;
                        tm.tm_hour = h;
                        tm.tm_min = m;
                        tm.tm_sec = s;
                        std::time_t candidate = to_time_t(tm);
                        std::tm ctm = to_tm(candidate);
                        const int month_1 = ctm.tm_mon + 1;
                        const int day = ctm.tm_mday;
                        const int days_this_month = days_in_month(ctm.tm_year + 1900, month_1);
                        bool pass = true;
                        if (!by_month.empty() &&
                            std::find(by_month.begin(), by_month.end(), month_1) == by_month.end()) {
                            pass = false;
                        }
                        if (pass && !by_monthday.empty()) {
                            bool monthday_match = false;
                            for (int md : by_monthday) {
                                if (md > 0 && day == md) {
                                    monthday_match = true;
                                    break;
                                }
                                if (md < 0 && day == (days_this_month + md + 1)) {
                                    monthday_match = true;
                                    break;
                                }
                            }
                            pass = monthday_match;
                        }
                        if (pass && !by_weekday.empty() && by_weekday.count(ctm.tm_wday) == 0U) {
                            pass = false;
                        }
                        if (pass) {
                            expanded.push_back(candidate);
                        }
                    }
                }
            }
        }

        std::sort(expanded.begin(), expanded.end());
        expanded.erase(std::unique(expanded.begin(), expanded.end()), expanded.end());

        std::vector<std::time_t> selected;
        if (!by_setpos.empty()) {
            for (int pos : by_setpos) {
                int idx = pos > 0 ? pos - 1 : static_cast<int>(expanded.size()) + pos;
                if (idx >= 0 && idx < static_cast<int>(expanded.size())) {
                    selected.push_back(expanded[static_cast<size_t>(idx)]);
                }
            }
            std::sort(selected.begin(), selected.end());
            selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
        } else {
            selected = expanded;
        }

        for (std::time_t candidate : selected) {
            if (until.has_value() && candidate > *until) {
                continue;
            }
            if (candidate > *now) {
                out.insert(FormatUtc(candidate));
                ++emitted;
                if (count_limit >= 0 && emitted >= count_limit) {
                    break;
                }
            }
        }
        if (count_limit >= 0 && emitted >= count_limit) {
            break;
        }

        if (freq == "SECONDLY") {
            period_cursor += interval;
        } else if (freq == "MINUTELY") {
            period_cursor += static_cast<std::time_t>(60LL * interval);
        } else if (freq == "HOURLY") {
            period_cursor += static_cast<std::time_t>(3600LL * interval);
        } else if (freq == "DAILY") {
            period_cursor += static_cast<std::time_t>(86400LL * interval);
        } else if (freq == "WEEKLY") {
            period_cursor += static_cast<std::time_t>(7LL * 86400LL * interval);
        } else if (freq == "MONTHLY") {
            period_cursor = add_months(period_cursor, interval);
        } else if (freq == "YEARLY") {
            period_cursor = add_years(period_cursor, interval);
        }
    }

    if (period_count >= max_candidates * 8U && out.empty()) {
        throw MakeReject("SRB1-R-7103", "candidate cap exceeded", "reporting", "expand_rrule_bounded");
    }

    for (const auto& local : schedule.schedule_rdates_local) {
        const auto ts = ParseLocalAsUtc(local);
        if (!ts.has_value()) {
            throw MakeReject("SRB1-R-7102", "invalid rdate local datetime", "reporting", "expand_rrule_bounded");
        }
        if (*ts > *now) {
            out.insert(FormatUtc(*ts));
        }
    }

    for (const auto& local : schedule.schedule_exdates_local) {
        const auto ts = ParseLocalAsUtc(local);
        if (!ts.has_value()) {
            throw MakeReject("SRB1-R-7102", "invalid exdate local datetime", "reporting", "expand_rrule_bounded");
        }
        out.erase(FormatUtc(*ts));
    }

    return std::vector<std::string>(out.begin(), out.end());
}

std::string NextRun(const ReportingSchedule& schedule, const std::string& now_utc) {
    const auto cands = ExpandRRuleBounded(schedule, now_utc);
    if (cands.empty()) {
        throw MakeReject("SRB1-R-7104", "no next run candidate", "reporting", "next_run");
    }
    return cands.front();
}

std::string RunQuestion(bool question_exists,
                        const std::string& normalized_sql,
                        const std::function<std::string(const std::string&)>& exec,
                        const std::function<bool(const std::string&)>& persist_result_fn) {
    if (!question_exists) {
        throw MakeReject("SRB1-R-7001", "question not found", "reporting", "run_question");
    }
    const auto started = std::chrono::steady_clock::now();
    const std::string result = exec(normalized_sql);
    const auto finished = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
    const bool is_embedded_json = !result.empty() && (result.front() == '{' || result.front() == '[');
    std::ostringstream payload;
    payload << "{\"success\":true,\"query_result\":";
    if (is_embedded_json) {
        payload << result;
    } else {
        payload << "{\"message\":\"" << JsonEscape(result) << "\"}";
    }
    payload << ",\"timing\":{\"elapsed_ms\":" << elapsed_ms
            << "},\"cache\":{\"hit\":false,\"cache_key\":\"\",\"ttl_seconds\":0},\"error\":{\"code\":\"\",\"message\":\"\"}}";
    const std::string full_result = payload.str();
    if (!persist_result_fn(full_result)) {
        throw MakeReject("SRB1-R-7002", "result storage failure", "reporting", "run_question");
    }
    return full_result;
}

std::string RunDashboardRuntime(const std::string& dashboard_id,
                                const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                                bool cache_hit) {
    if (dashboard_id.empty()) {
        throw MakeReject("SRB1-R-7001", "dashboard id missing", "reporting", "run_dashboard_runtime");
    }
    auto sorted = widget_statuses;
    std::sort(sorted.begin(), sorted.end());

    const auto now_ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const std::string now_utc = FormatUtc(now_ts);

    std::ostringstream out;
    out << "{\"dashboard_id\":\"" << JsonEscape(dashboard_id)
        << "\",\"executed_at_utc\":\"" << now_utc
        << "\",\"widgets\":[";
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        int row_count = 0;
        if (sorted[i].second == "ok") {
            row_count = 1;
        }
        const auto split_pos = sorted[i].second.find(':');
        if (split_pos != std::string::npos) {
            try {
                row_count = std::max(0, std::stoi(sorted[i].second.substr(split_pos + 1)));
            } catch (...) {
                row_count = 0;
            }
        }
        out << "{\"widget_id\":\"" << JsonEscape(sorted[i].first) << "\",\"status\":\"" << JsonEscape(sorted[i].second)
            << "\",\"row_count\":" << row_count
            << ",\"dataset_key\":\"" << JsonEscape("dataset:" + sorted[i].first) << "\"}";
    }
    out << "],\"cache\":{\"hit\":" << (cache_hit ? "true" : "false")
        << ",\"cache_key\":\"dash:" << JsonEscape(dashboard_id) << "\"}}";
    return out.str();
}

void PersistResult(const std::string& key,
                   const std::string& result_payload,
                   std::map<std::string, std::string>* storage) {
    if (storage == nullptr || key.empty()) {
        throw MakeReject("SRB1-R-7002", "storage metadata path incomplete", "reporting", "persist_result");
    }
    (*storage)[key] = result_payload;
}

std::string ExportReportingRepository(const std::vector<ReportingAsset>& assets) {
    static const std::set<std::string> kAllowedTypes = {"Question", "Dashboard", "Model", "Metric", "Segment",
                                                        "Alert", "Subscription", "Collection", "Timeline",
                                                        // Backward-compatible lowercase aliases from older fixtures/tests.
                                                        "question", "dashboard", "model", "metric", "segment", "alert",
                                                        "subscription", "collection", "timeline"};
    auto canonical = CanonicalArtifactOrder(assets);
    std::ostringstream out;
    out << "{\"assets\":[";
    for (size_t i = 0; i < canonical.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& a = canonical[i];
        if (a.id.empty() || a.asset_type.empty() || a.name.empty()) {
            throw MakeReject("SRB1-R-7003", "report artifact import/export fidelity failure", "reporting", "export_repository");
        }
        if (kAllowedTypes.count(a.asset_type) == 0U) {
            throw MakeReject("SRB1-R-7003", "unknown reporting artifact type", "reporting", "export_repository", false,
                             a.asset_type);
        }
        out << "{\"id\":\"" << JsonEscape(a.id)
            << "\",\"asset_type\":\"" << JsonEscape(a.asset_type)
            << "\",\"name\":\"" << JsonEscape(a.name)
            << "\",\"payload_json\":\"" << JsonEscape(a.payload_json)
            << "\",\"collection_id\":\"" << JsonEscape(a.collection_id)
            << "\",\"created_at_utc\":\"" << JsonEscape(a.created_at_utc)
            << "\",\"updated_at_utc\":\"" << JsonEscape(a.updated_at_utc)
            << "\",\"created_by\":\"" << JsonEscape(a.created_by)
            << "\",\"updated_by\":\"" << JsonEscape(a.updated_by) << "\"}";
    }
    out << "]}";
    return out.str();
}

std::vector<ReportingAsset> ImportReportingRepository(const std::string& payload_json) {
    static const std::set<std::string> kAllowedTypes = {"Question", "Dashboard", "Model", "Metric", "Segment",
                                                        "Alert", "Subscription", "Collection", "Timeline",
                                                        "question", "dashboard", "model", "metric", "segment", "alert",
                                                        "subscription", "collection", "timeline"};
    JsonParser parser(payload_json);
    JsonValue root;
    std::string err;
    if (!parser.Parse(&root, &err) || root.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-7003", "report artifact import/export fidelity failure", "reporting", "import_repository",
                         false, err);
    }
    const JsonValue& arr = RequireMember(root, "assets", "SRB1-R-7003", "reporting", "import_repository");
    if (arr.type != JsonValue::Type::Array) {
        throw MakeReject("SRB1-R-7003", "assets must be array", "reporting", "import_repository");
    }

    std::vector<ReportingAsset> out;
    out.reserve(arr.array_value.size());
    for (const auto& v : arr.array_value) {
        if (v.type != JsonValue::Type::Object) {
            throw MakeReject("SRB1-R-7003", "invalid asset row", "reporting", "import_repository");
        }
        ReportingAsset a;
        a.id = RequireString(v, "id", "SRB1-R-7003", "reporting", "import_repository");
        a.asset_type = RequireString(v, "asset_type", "SRB1-R-7003", "reporting", "import_repository");
        a.name = RequireString(v, "name", "SRB1-R-7003", "reporting", "import_repository");
        if (kAllowedTypes.count(a.asset_type) == 0U) {
            throw MakeReject("SRB1-R-7003", "unknown reporting artifact type", "reporting", "import_repository", false,
                             a.asset_type);
        }
        const JsonValue& payload = RequireMember(v, "payload_json", "SRB1-R-7003", "reporting", "import_repository");
        if (payload.type != JsonValue::Type::String) {
            throw MakeReject("SRB1-R-7003", "invalid payload_json", "reporting", "import_repository");
        }
        a.payload_json = payload.string_value;
        const JsonValue* collection_v = FindMember(v, "collection_id");
        if (collection_v != nullptr && collection_v->type == JsonValue::Type::String) {
            a.collection_id = collection_v->string_value;
        }
        const JsonValue* created_at_v = FindMember(v, "created_at_utc");
        if (created_at_v != nullptr && created_at_v->type == JsonValue::Type::String) {
            a.created_at_utc = created_at_v->string_value;
        }
        const JsonValue* updated_at_v = FindMember(v, "updated_at_utc");
        if (updated_at_v != nullptr && updated_at_v->type == JsonValue::Type::String) {
            a.updated_at_utc = updated_at_v->string_value;
        }
        const JsonValue* created_by_v = FindMember(v, "created_by");
        if (created_by_v != nullptr && created_by_v->type == JsonValue::Type::String) {
            a.created_by = created_by_v->string_value;
        }
        const JsonValue* updated_by_v = FindMember(v, "updated_by");
        if (updated_by_v != nullptr && updated_by_v->type == JsonValue::Type::String) {
            a.updated_by = updated_by_v->string_value;
        }
        out.push_back(std::move(a));
    }
    return CanonicalArtifactOrder(out);
}

std::vector<ActivityRow> RunActivityWindowQuery(const std::vector<ActivityRow>& source,
                                                const std::string& window,
                                                const std::set<std::string>& allowed_metrics) {
    static const std::set<std::string> kWindows = {"5m", "15m", "1h", "24h"};
    if (kWindows.count(window) == 0U) {
        throw MakeReject("SRB1-R-7202", "unsupported window", "reporting", "run_activity_window_query");
    }
    if (allowed_metrics.empty()) {
        throw MakeReject("SRB1-R-7202", "no metrics requested", "reporting", "run_activity_window_query");
    }

    std::vector<ActivityRow> out;
    for (const auto& row : source) {
        if (allowed_metrics.count(row.metric_key) > 0U) {
            if (!IsRfc3339Utc(row.timestamp_utc)) {
                throw MakeReject("SRB1-R-7203", "invalid activity timestamp", "reporting", "run_activity_window_query");
            }
            out.push_back(row);
        }
    }

    std::sort(out.begin(), out.end(), [](const ActivityRow& a, const ActivityRow& b) {
        return std::tie(a.timestamp_utc, a.metric_key) < std::tie(b.timestamp_utc, b.metric_key);
    });
    return out;
}

std::string ExportActivity(const std::vector<ActivityRow>& rows, const std::string& fmt) {
    auto ordered = rows;
    std::sort(ordered.begin(), ordered.end(), [](const ActivityRow& a, const ActivityRow& b) {
        return std::tie(a.timestamp_utc, a.metric_key) < std::tie(b.timestamp_utc, b.metric_key);
    });

    if (fmt == "csv") {
        std::ostringstream out;
        out << "timestamp_utc,metric_key,value\n";
        for (const auto& row : ordered) {
            out << row.timestamp_utc << ',' << row.metric_key << ',' << row.value << '\n';
        }
        return out.str();
    }
    if (fmt == "json") {
        std::ostringstream out;
        out << '[';
        for (size_t i = 0; i < ordered.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            out << "{\"metric_key\":\"" << JsonEscape(ordered[i].metric_key) << "\",\"timestamp_utc\":\""
                << JsonEscape(ordered[i].timestamp_utc) << "\",\"value\":" << ordered[i].value << '}';
        }
        out << ']';
        return out.str();
    }
    throw MakeReject("SRB1-R-7202", "unsupported export format", "reporting", "export_activity");
}

// -------------------------
// Advanced contracts
// -------------------------

std::string RunCdcEvent(const std::string& event_payload,
                        int max_attempts,
                        int /*backoff_ms*/,
                        const std::function<bool(const std::string&)>& publish,
                        const std::function<void(const std::string&)>& dead_letter) {
    int attempts = 0;
    while (attempts < max_attempts) {
        ++attempts;
        if (publish(event_payload)) {
            return "published";
        }
    }
    dead_letter(event_payload);
    throw MakeReject("SRB1-R-7004", "cdc publish failed after retries", "advanced", "run_cdc_event");
}

std::vector<std::map<std::string, std::string>> PreviewMask(
    const std::vector<std::map<std::string, std::string>>& rows,
    const std::map<std::string, std::string>& rules) {
    if (rules.empty()) {
        throw MakeReject("SRB1-R-7005", "masking profile missing", "advanced", "preview_mask");
    }

    auto out = rows;
    for (auto& row : out) {
        for (const auto& [field, method] : rules) {
            auto it = row.find(field);
            if (it == row.end()) {
                continue;
            }
            if (method == "redact") {
                it->second = "***";
            } else if (method == "hash") {
                it->second = Sha256Hex(it->second);
            } else if (method == "prefix_mask") {
                if (it->second.size() > 2U) {
                    it->second = it->second.substr(0, 2) + std::string(it->second.size() - 2, '*');
                } else {
                    it->second = std::string(it->second.size(), '*');
                }
            } else {
                throw MakeReject("SRB1-R-7005", "unsupported masking method", "advanced", "preview_mask", false, method);
            }
        }
    }
    return out;
}

void CheckReviewQuorum(int approved_count, int min_reviewers) {
    if (min_reviewers < 1 || approved_count < min_reviewers) {
        throw MakeReject("SRB1-R-7301", "insufficient approvals", "advanced", "check_review_quorum");
    }
}

void RequireChangeAdvisory(const std::string& action_id, const std::string& advisory_state) {
    if (advisory_state != "Approved") {
        throw MakeReject("SRB1-R-7305", "action " + action_id + " requires approved advisory", "advanced", "require_change_advisory");
    }
}

void ValidateExtension(bool signature_ok, bool compatibility_ok) {
    if (!signature_ok || !compatibility_ok) {
        throw MakeReject("SRB1-R-7303", "extension signature/compatibility invalid", "advanced", "validate_extension");
    }
}

void EnforceExtensionAllowlist(const std::set<std::string>& requested_capabilities,
                               const std::set<std::string>& allowlist) {
    for (const auto& cap : requested_capabilities) {
        if (allowlist.count(cap) == 0U) {
            throw MakeReject("SRB1-R-7304", "extension capability not allowed", "advanced", "enforce_extension_allowlist", false,
                             cap);
        }
    }
}

std::pair<std::vector<std::string>, int> BuildLineage(
    const std::vector<std::string>& node_ids,
    const std::vector<std::pair<std::string, std::optional<std::string>>>& edges) {
    std::vector<std::string> sorted_nodes = node_ids;
    std::sort(sorted_nodes.begin(), sorted_nodes.end());

    int unresolved = 0;
    for (const auto& edge : edges) {
        if (!edge.second.has_value()) {
            ++unresolved;
        }
    }
    return {sorted_nodes, unresolved};
}

std::map<std::string, std::optional<std::string>> RegisterOptionalSurfaces(const std::string& profile_id) {
    const std::array<std::pair<const char*, const char*>, 5> surfaces = {
        std::pair{"ClusterManagerFrame", "SRB1-R-7008"},
        std::pair{"ReplicationManagerFrame", "SRB1-R-7009"},
        std::pair{"EtlManagerFrame", "SRB1-R-7010"},
        std::pair{"DockerManagerPanel", "SRB1-R-7011"},
        std::pair{"TestRunnerPanel", "SRB1-R-7012"},
    };

    std::map<std::string, std::optional<std::string>> out;
    for (const auto& [id, reject] : surfaces) {
        if (profile_id == "preview") {
            out[id] = std::nullopt;
        } else {
            out[id] = std::string(reject);
        }
    }
    return out;
}

void ValidateAiProviderConfig(const std::string& provider_id,
                              bool async_enabled,
                              const std::string& endpoint_or_model,
                              const std::optional<std::string>& credential) {
    static const std::set<std::string> providers = {"openai", "ollama", "anthropic", "local_mock"};
    if (!async_enabled || providers.count(provider_id) == 0U || endpoint_or_model.empty() ||
        !credential.has_value() || credential->empty()) {
        throw MakeReject("SRB1-R-7006", "AI provider configuration invalid", "advanced", "validate_ai_provider");
    }
}

void ValidateIssueTrackerConfig(const std::string& provider_id,
                                const std::string& project_or_repo,
                                const std::optional<std::string>& credential) {
    static const std::set<std::string> providers = {"github", "gitlab", "jira"};
    if (providers.count(provider_id) == 0U || project_or_repo.empty() || !credential.has_value() || credential->empty()) {
        throw MakeReject("SRB1-R-7007", "issue tracker integration invalid configuration/path", "advanced",
                         "validate_issue_tracker");
    }
}

void ValidateGitSyncState(bool branch_selected,
                          bool remote_reachable,
                          bool conflicts_resolved) {
    if (!branch_selected || !remote_reachable || !conflicts_resolved) {
        throw MakeReject("SRB1-R-8201", "git sync conflict unresolved", "advanced", "validate_git_sync");
    }
}

// -------------------------
// Packaging + spec support
// -------------------------

std::string CanonicalBuildHash(const std::string& full_commit_id) {
    std::string normalized = ToLower(Trim(full_commit_id));
    if (!IsHexLower(normalized) || (normalized.size() != 40U && normalized.size() != 64U)) {
        throw MakeReject("SRB1-R-9002", "invalid commit id format", "packaging", "canonical_build_hash");
    }
    return Sha256Hex(normalized);
}

void ValidateSurfaceRegistry(const JsonValue& manifest, const std::set<std::string>& surface_registry) {
    const JsonValue& surfaces = RequireMember(manifest, "surfaces", "SRB1-R-9002", "packaging", "validate_surface_registry");
    if (surfaces.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid surfaces object", "packaging", "validate_surface_registry");
    }

    std::set<std::string> seen;
    for (const std::string& group : {"enabled", "disabled", "preview_only"}) {
        const auto values = RequireStringArray(surfaces, group, "SRB1-R-9002", "packaging", "validate_surface_registry");
        for (const auto& v : values) {
            if (surface_registry.count(v) == 0U) {
                throw MakeReject("SRB1-R-9002", "unknown surface id " + v, "packaging", "validate_surface_registry");
            }
            if (seen.count(v) > 0U) {
                throw MakeReject("SRB1-R-9002", "surface id duplicated across groups: " + v, "packaging", "validate_surface_registry");
            }
            seen.insert(v);
        }
    }

    std::string profile_id = RequireString(manifest, "profile_id", "SRB1-R-9002", "packaging", "validate_surface_registry");
    if (profile_id == "ga") {
        const auto preview_only = RequireStringArray(surfaces, "preview_only", "SRB1-R-9002", "packaging", "validate_surface_registry");
        if (!preview_only.empty()) {
            throw MakeReject("SRB1-R-9001", "ga profile cannot contain preview-only surfaces", "packaging", "validate_surface_registry");
        }
    }
}

void ValidatePackageArtifacts(const std::set<std::string>& packaged_paths) {
    static const std::set<std::string> required = {
        "LICENSE",
        "README.md",
        "docs/installation_guide/README.md",
        "docs/developers_guide/README.md",
    };
    for (const auto& path : required) {
        if (packaged_paths.count(path) == 0U) {
            throw MakeReject("SRB1-R-9003", "missing mandatory license/documentation artifacts", "packaging",
                             "validate_package_artifacts", false, path);
        }
    }
}

PackageValidationResult ValidateProfileManifest(const JsonValue& manifest,
                                                const std::set<std::string>& surface_registry,
                                                const std::set<std::string>& backend_enum) {
    if (manifest.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "manifest must be object", "packaging", "validate_profile_manifest");
    }
    EnsureOnlyObjectFields(
        manifest,
        {"manifest_version", "profile_id", "build_version", "build_hash", "build_timestamp_utc",
         "platform", "enabled_backends", "surfaces", "security_defaults", "artifacts"},
        "SRB1-R-9002",
        "packaging",
        "validate_profile_manifest");

    const std::string manifest_version =
        RequireString(manifest, "manifest_version", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (manifest_version != "1.0.0") {
        throw MakeReject("SRB1-R-9002", "unsupported manifest_version", "packaging", "validate_profile_manifest");
    }

    std::string profile_id = RequireString(manifest, "profile_id", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    static const std::set<std::string> profiles = {"full", "no_scratchbird", "minimal_ui", "ci_strict", "preview", "ga"};
    if (profiles.count(profile_id) == 0U) {
        throw MakeReject("SRB1-R-9002", "invalid profile_id", "packaging", "validate_profile_manifest");
    }

    (void)RequireString(manifest, "build_version", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    std::string build_hash = RequireString(manifest, "build_hash", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (build_hash.size() != 64U || !IsHexLower(build_hash)) {
        throw MakeReject("SRB1-R-9002", "invalid build_hash", "packaging", "validate_profile_manifest");
    }

    std::string ts = RequireString(manifest, "build_timestamp_utc", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (!IsRfc3339Utc(ts)) {
        throw MakeReject("SRB1-R-9002", "invalid build_timestamp_utc", "packaging", "validate_profile_manifest");
    }
    static const std::set<std::string> platforms = {"linux", "windows", "macos"};
    const std::string platform =
        RequireString(manifest, "platform", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (platforms.count(platform) == 0U) {
        throw MakeReject("SRB1-R-9002", "invalid platform", "packaging", "validate_profile_manifest");
    }

    const auto backends = RequireStringArray(manifest, "enabled_backends", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (backends.empty()) {
        throw MakeReject("SRB1-R-9002", "enabled_backends must be non-empty", "packaging", "validate_profile_manifest");
    }
    for (const auto& b : backends) {
        if (backend_enum.count(b) == 0U) {
            throw MakeReject("SRB1-R-9002", "unknown backend id", "packaging", "validate_profile_manifest", false, b);
        }
    }
    EnsureSortedUnique(backends, "enabled_backends", "SRB1-R-9002", "packaging", "validate_profile_manifest");

    const JsonValue& security_defaults =
        RequireMember(manifest, "security_defaults", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (security_defaults.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid security_defaults", "packaging", "validate_profile_manifest");
    }
    EnsureOnlyObjectFields(
        security_defaults,
        {"security_mode", "credential_store_policy", "audit_enabled_default", "tls_required_default"},
        "SRB1-R-9002",
        "packaging",
        "validate_profile_manifest");
    const std::string security_mode =
        RequireString(security_defaults, "security_mode", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (security_mode != "standard" && security_mode != "hardened") {
        throw MakeReject("SRB1-R-9002", "invalid security_mode", "packaging", "validate_profile_manifest");
    }
    const std::string credential_store_policy =
        RequireString(security_defaults, "credential_store_policy", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (credential_store_policy != "required" && credential_store_policy != "preferred" &&
        credential_store_policy != "fallback_file") {
        throw MakeReject("SRB1-R-9002", "invalid credential_store_policy", "packaging", "validate_profile_manifest");
    }
    bool audit_enabled = false;
    bool tls_required = false;
    if (!GetBoolValue(RequireMember(security_defaults, "audit_enabled_default", "SRB1-R-9002", "packaging",
                                    "validate_profile_manifest"), &audit_enabled)) {
        throw MakeReject("SRB1-R-9002", "invalid audit_enabled_default", "packaging", "validate_profile_manifest");
    }
    if (!GetBoolValue(RequireMember(security_defaults, "tls_required_default", "SRB1-R-9002", "packaging",
                                    "validate_profile_manifest"), &tls_required)) {
        throw MakeReject("SRB1-R-9002", "invalid tls_required_default", "packaging", "validate_profile_manifest");
    }
    (void)audit_enabled;
    (void)tls_required;

    const JsonValue& artifacts = RequireMember(manifest, "artifacts", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (artifacts.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid artifacts", "packaging", "validate_profile_manifest");
    }
    EnsureOnlyObjectFields(
        artifacts,
        {"license_path", "attribution_path", "help_root_path", "config_template_path", "connections_template_path"},
        "SRB1-R-9002",
        "packaging",
        "validate_profile_manifest");
    for (const auto& key : {"license_path", "attribution_path", "help_root_path", "config_template_path", "connections_template_path"}) {
        std::string v = RequireString(artifacts, key, "SRB1-R-9002", "packaging", "validate_profile_manifest");
        if (v.find("..") != std::string::npos || (!v.empty() && v[0] == '/') || v.find(':') != std::string::npos) {
            throw MakeReject("SRB1-R-9002", "invalid artifact path", "packaging", "validate_profile_manifest", false, v);
        }
    }

    ValidateSurfaceRegistry(manifest, surface_registry);

    return PackageValidationResult{true, profile_id};
}

std::vector<std::string> DiscoverSpecsets(const std::string& spec_root) {
    const std::vector<std::string> required = {
        spec_root + "/resources/specset_packages/sb_v3_specset_manifest.example.json",
        spec_root + "/resources/specset_packages/sb_vnext_specset_manifest.example.json",
        spec_root + "/resources/specset_packages/sb_beta1_specset_manifest.example.json",
    };

    for (const auto& path : required) {
        if (!std::filesystem::exists(path)) {
            throw MakeReject("SRB1-R-5401", "missing manifest", "spec_workspace", "discover_specsets", false, path);
        }
    }
    return required;
}

SpecSetManifest LoadSpecsetManifest(const std::string& manifest_path) {
    JsonValue json = ParseJsonFile(manifest_path);
    if (json.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-5402", "manifest must be object", "spec_workspace", "load_specset_manifest");
    }
    EnsureOnlyObjectFields(
        json,
        {"set_id", "package_root", "authoritative_inventory_relpath", "version_stamp", "package_hash_sha256"},
        "SRB1-R-5402",
        "spec_workspace",
        "load_specset_manifest");

    SpecSetManifest out;
    out.set_id = RequireString(json, "set_id", "SRB1-R-5402", "spec_workspace", "load_specset_manifest");
    out.package_root = RequireString(json, "package_root", "SRB1-R-5402", "spec_workspace", "load_specset_manifest");
    out.authoritative_inventory_relpath =
        RequireString(json, "authoritative_inventory_relpath", "SRB1-R-5402", "spec_workspace", "load_specset_manifest");
    out.version_stamp = RequireString(json, "version_stamp", "SRB1-R-5402", "spec_workspace", "load_specset_manifest");
    out.package_hash_sha256 =
        RequireString(json, "package_hash_sha256", "SRB1-R-5402", "spec_workspace", "load_specset_manifest");

    if (out.set_id != "sb_v3" && out.set_id != "sb_vnext" && out.set_id != "sb_beta1") {
        throw MakeReject("SRB1-R-5401", "unsupported set id", "spec_workspace", "load_specset_manifest", false, out.set_id);
    }
    if (out.package_root.find("..") != std::string::npos || out.authoritative_inventory_relpath.find("..") != std::string::npos ||
        out.package_root.find(':') != std::string::npos || out.authoritative_inventory_relpath.find(':') != std::string::npos ||
        (!out.package_root.empty() && out.package_root[0] == '/') ||
        (!out.authoritative_inventory_relpath.empty() && out.authoritative_inventory_relpath[0] == '/')) {
        throw MakeReject("SRB1-R-5402", "path traversal in manifest", "spec_workspace", "load_specset_manifest");
    }
    if (out.package_hash_sha256.size() != 64U || !IsHexLower(ToLower(out.package_hash_sha256))) {
        throw MakeReject("SRB1-R-5402", "invalid package_hash_sha256", "spec_workspace", "load_specset_manifest");
    }
    return out;
}

std::vector<std::string> ParseAuthoritativeInventory(const std::string& inventory_path) {
    std::string text = ReadTextFile(inventory_path);
    std::stringstream ss(text);
    std::string line;
    std::vector<std::string> rows;

    while (std::getline(ss, line)) {
        const auto first = line.find('`');
        if (first == std::string::npos) {
            continue;
        }
        const auto second = line.find('`', first + 1);
        if (second == std::string::npos || second <= first + 1) {
            continue;
        }
        std::string rel = line.substr(first + 1, second - first - 1);
        if (!rel.empty()) {
            if (rel.find("..") != std::string::npos || rel.find(':') != std::string::npos ||
                (!rel.empty() && rel[0] == '/')) {
                throw MakeReject("SRB1-R-5402", "invalid inventory relative path", "spec_workspace",
                                 "parse_inventory", false, rel);
            }
            rows.push_back(rel);
        }
    }

    if (rows.empty()) {
        throw MakeReject("SRB1-R-5402", "inventory parse failure", "spec_workspace", "parse_inventory", false, inventory_path);
    }
    std::sort(rows.begin(), rows.end());
    auto unique_it = std::unique(rows.begin(), rows.end());
    if (unique_it != rows.end()) {
        throw MakeReject("SRB1-R-5402", "duplicate authoritative inventory entries", "spec_workspace",
                         "parse_inventory", false, inventory_path);
    }
    return rows;
}

std::vector<SpecFileRow> LoadSpecsetPackage(const std::string& manifest_path) {
    const SpecSetManifest manifest = LoadSpecsetManifest(manifest_path);
    const std::filesystem::path manifest_dir = std::filesystem::path(manifest_path).parent_path();
    const std::filesystem::path package_root = manifest_dir / manifest.package_root;
    const std::filesystem::path inventory_path = package_root / manifest.authoritative_inventory_relpath;

    if (!std::filesystem::exists(inventory_path)) {
        throw MakeReject("SRB1-R-5402", "inventory missing", "spec_workspace", "load_specset_package", false,
                         inventory_path.string());
    }

    const auto rel_files = ParseAuthoritativeInventory(inventory_path.string());
    std::vector<SpecFileRow> out;
    out.reserve(rel_files.size());
    const std::filesystem::path package_root_abs = std::filesystem::weakly_canonical(package_root);
    for (const auto& rel : rel_files) {
        const std::filesystem::path abs = package_root / rel;
        if (!std::filesystem::exists(abs)) {
            throw MakeReject("SRB1-R-5402", "missing normative file", "spec_workspace", "load_specset_package", false, rel);
        }
        const std::filesystem::path abs_canonical = std::filesystem::weakly_canonical(abs);
        const std::string abs_canonical_s = abs_canonical.generic_string();
        const std::string package_root_s = package_root_abs.generic_string();
        if (abs_canonical_s.rfind(package_root_s, 0) != 0U) {
            throw MakeReject("SRB1-R-5402", "normative path escaped package root", "spec_workspace",
                             "load_specset_package", false, rel);
        }
        std::ifstream in(abs, std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        SpecFileRow row;
        row.set_id = manifest.set_id;
        row.relative_path = rel;
        row.is_normative = true;
        row.content_hash = Sha256Hex(bytes);
        row.size_bytes = static_cast<std::uint64_t>(bytes.size());
        out.push_back(std::move(row));
    }
    std::sort(out.begin(), out.end(), [](const SpecFileRow& a, const SpecFileRow& b) {
        return std::tie(a.set_id, a.relative_path) < std::tie(b.set_id, b.relative_path);
    });
    return out;
}

void AssertSupportComplete(const std::vector<SpecFileRow>& spec_files,
                           const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links,
                           const std::string& coverage_class) {
    std::set<std::string> required;
    for (const auto& row : spec_files) {
        if (row.is_normative) {
            required.insert(row.set_id + ":" + row.relative_path);
        }
    }

    std::set<std::string> covered;
    for (const auto& [spec_file_ref, cls, state] : coverage_links) {
        if (cls == coverage_class && state == "covered") {
            covered.insert(spec_file_ref);
        }
    }

    std::vector<std::string> missing;
    std::set_difference(required.begin(), required.end(), covered.begin(), covered.end(), std::back_inserter(missing));
    if (!missing.empty()) {
        throw MakeReject("SRB1-R-5403",
                         "missing " + coverage_class + " coverage: " + std::to_string(missing.size()) + " files",
                         "spec_workspace",
                         "assert_support_complete");
    }
}

void ValidateBindings(const std::vector<std::string>& binding_case_ids,
                      const std::set<std::string>& conformance_case_ids) {
    if (conformance_case_ids.empty()) {
        throw MakeReject("SRB1-R-5404", "conformance case registry empty", "spec_workspace", "validate_bindings");
    }
    static const std::regex case_id_re(R"(^[A-Z0-9][A-Z0-9\-]*$)");
    std::set<std::string> seen;
    for (const auto& id : binding_case_ids) {
        if (id.empty() || !std::regex_match(id.begin(), id.end(), case_id_re)) {
            throw MakeReject("SRB1-R-5404", "invalid case id format: " + id, "spec_workspace", "validate_bindings");
        }
        if (!seen.insert(id).second) {
            throw MakeReject("SRB1-R-5404", "duplicate case id binding: " + id, "spec_workspace", "validate_bindings");
        }
        if (conformance_case_ids.count(id) == 0U) {
            throw MakeReject("SRB1-R-5404", "unknown case id: " + id, "spec_workspace", "validate_bindings");
        }
    }
}

std::map<std::string, int> AggregateSupport(const std::vector<std::tuple<std::string, std::string, std::string>>& coverage_links) {
    std::map<std::string, int> counts;
    for (const auto& [spec_file_ref, coverage_class, coverage_state] : coverage_links) {
        (void)spec_file_ref;
        counts[coverage_class + ":" + coverage_state] += 1;
    }
    return counts;
}

std::string ExportWorkPackage(const std::string& set_id,
                              const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& gaps,
                              const std::string& generated_at_utc) {
    if (set_id.empty() || !IsRfc3339Utc(generated_at_utc)) {
        throw MakeReject("SRB1-R-5406", "invalid work package header", "spec_workspace", "export_work_package");
    }

    auto sorted_gaps = gaps;
    std::sort(sorted_gaps.begin(), sorted_gaps.end(), [](const auto& a, const auto& b) {
        return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b));
    });

    std::ostringstream out;
    out << "{\"export_version\":\"1.0.0\",\"generated_at_utc\":\"" << generated_at_utc << "\",\"set_id\":\""
        << JsonEscape(set_id) << "\",\"gaps\":[";

    for (size_t i = 0; i < sorted_gaps.size(); ++i) {
        const auto& [spec_file_ref, coverage_class, required_case_ids] = sorted_gaps[i];
        if (i > 0) {
            out << ',';
        }
        auto case_ids = required_case_ids;
        std::sort(case_ids.begin(), case_ids.end());

        out << "{\"spec_file_ref\":\"" << JsonEscape(spec_file_ref) << "\",\"coverage_class\":\""
            << JsonEscape(coverage_class) << "\",\"coverage_state\":\"missing\",\"required_case_ids\":[";
        for (size_t j = 0; j < case_ids.size(); ++j) {
            if (j > 0) {
                out << ',';
            }
            out << "\"" << JsonEscape(case_ids[j]) << "\"";
        }
        out << "]}";
    }

    out << "]}";
    return out.str();
}

void ValidateAlphaMirrorPresence(const std::string& mirror_root,
                                 const std::vector<AlphaMirrorEntry>& entries) {
    for (const auto& e : entries) {
        if (e.relative_path.empty()) {
            throw MakeReject("SRB1-R-5501", "required alpha deep-pack mirror file missing", "alpha_preservation",
                             "validate_mirror_presence");
        }
        const auto path = std::filesystem::path(mirror_root) / e.relative_path;
        if (!std::filesystem::exists(path)) {
            throw MakeReject("SRB1-R-5501", "required alpha deep-pack mirror file missing", "alpha_preservation",
                             "validate_mirror_presence", false, e.relative_path);
        }
    }
}

void ValidateAlphaMirrorHashes(const std::string& mirror_root,
                               const std::vector<AlphaMirrorEntry>& entries) {
    for (const auto& e : entries) {
        const auto path = std::filesystem::path(mirror_root) / e.relative_path;
        if (!std::filesystem::exists(path)) {
            throw MakeReject("SRB1-R-5502", "alpha deep-pack mirror hash/size mismatch", "alpha_preservation",
                             "validate_mirror_hashes", false, e.relative_path);
        }
        std::ifstream in(path, std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (static_cast<std::uint64_t>(bytes.size()) != e.expected_size || Sha256Hex(bytes) != ToLower(e.expected_sha256)) {
            throw MakeReject("SRB1-R-5502", "alpha deep-pack mirror hash/size mismatch", "alpha_preservation",
                             "validate_mirror_hashes", false, e.relative_path);
        }
    }
}

void ValidateSilverstonContinuity(const std::set<std::string>& present_artifacts,
                                  const std::set<std::string>& required_artifacts) {
    for (const auto& required : required_artifacts) {
        if (present_artifacts.count(required) == 0U) {
            throw MakeReject("SRB1-R-5503", "mandatory Silverston/ERD continuity artifact missing", "alpha_preservation",
                             "validate_silverston_continuity", false, required);
        }
    }
}

void ValidateAlphaInventoryMapping(const std::set<std::string>& required_element_ids,
                                   const std::map<std::string, std::string>& file_to_element_id) {
    std::set<std::string> seen;
    for (const auto& [file_path, element_id] : file_to_element_id) {
        if (file_path.empty() || element_id.empty()) {
            throw MakeReject("SRB1-R-5504", "alpha deep-pack element inventory mapping incomplete/invalid",
                             "alpha_preservation", "validate_inventory_mapping");
        }
        seen.insert(element_id);
    }
    for (const auto& required : required_element_ids) {
        if (seen.count(required) == 0U) {
            throw MakeReject("SRB1-R-5504", "alpha deep-pack element inventory mapping incomplete/invalid",
                             "alpha_preservation", "validate_inventory_mapping", false, required);
        }
    }
}

void ValidateAlphaExtractionGate(bool extraction_passed, bool continuity_passed, bool deep_contract_passed) {
    if (!extraction_passed || !continuity_passed || !deep_contract_passed) {
        throw MakeReject("SRB1-R-5505", "alpha deep-pack extraction/continuity conformance gate failure",
                         "alpha_preservation", "validate_extraction_gate");
    }
}

}  // namespace scratchrobin::beta1b
