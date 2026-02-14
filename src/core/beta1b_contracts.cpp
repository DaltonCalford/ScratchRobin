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
        }
    }

    const auto& ident = profile.identity;
    if (ident.mode != "local_password" && ident.mode != "oidc" && ident.mode != "saml" && ident.mode != "ldap" &&
        ident.mode != "kerberos") {
        throw MakeReject("SRB1-R-4005", "unknown identity mode", "connection", "validate_transport");
    }
    if (ident.mode != "local_password" && ident.provider_id.empty()) {
        throw MakeReject("SRB1-R-4005", "provider_id required", "connection", "validate_transport");
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
    if (identity.mode == "local_password") {
        if (secret.empty()) {
            throw MakeReject("SRB1-R-4005", "local_password requires secret", "connection", "identity_handshake");
        }
        return "local-password-ok";
    }
    if (identity.mode == "oidc" || identity.mode == "saml") {
        if (!federated_acquire(identity.provider_id, secret)) {
            throw MakeReject("SRB1-R-4005", "federated token acquisition failed", "connection", "identity_handshake");
        }
        return "federated-ok";
    }
    if (identity.mode == "ldap" || identity.mode == "kerberos") {
        if (!directory_bind(identity.provider_id, secret)) {
            throw MakeReject("SRB1-R-4005", "directory bind failed", "connection", "identity_handshake");
        }
        return "directory-ok";
    }
    throw MakeReject("SRB1-R-4005", "unknown identity mode", "connection", "identity_handshake");
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
    out.identity_mode = profile.identity.mode;
    out.backend_route = profile.transport.mode == "direct" ? "direct" : "ssh";
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
    const JsonValue* project = FindMember(payload, "project");
    if (project == nullptr || project->type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-3002", "missing project object", "project", "validate_project_payload");
    }

    const std::vector<std::string> required = {
        "project_id", "name", "created_at", "updated_at", "config", "objects", "objects_by_path", "reporting_assets",
        "reporting_schedules", "data_view_snapshots", "git_sync_state", "audit_log_path"};
    for (const auto& field : required) {
        if (FindMember(*project, field) == nullptr) {
            throw MakeReject("SRB1-R-3002", "missing project field: " + field, "project", "validate_project_payload");
        }
    }

    std::string ts;
    if (!GetStringValue(*FindMember(*project, "created_at"), &ts) || !IsRfc3339Utc(ts)) {
        throw MakeReject("SRB1-R-3002", "invalid created_at", "project", "validate_project_payload");
    }
}

void ValidateSpecsetPayload(const JsonValue& payload) {
    if (payload.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-5402", "specset payload must be object", "spec_workspace", "validate_specset_payload");
    }
    for (const auto& key : {"spec_sets", "spec_files", "coverage_links", "conformance_bindings"}) {
        const JsonValue* v = FindMember(payload, key);
        if (v == nullptr || v->type != JsonValue::Type::Array) {
            throw MakeReject("SRB1-R-5402", "missing array field: " + std::string(key), "spec_workspace", "validate_specset_payload");
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

void ValidateNotation(const std::string& notation) {
    static const std::set<std::string> allowed = {"crowsfoot", "idef1x", "uml", "chen"};
    if (allowed.count(notation) == 0U) {
        throw MakeReject("SRB1-R-6101", "invalid/unresolvable diagram notation", "diagram", "validate_notation", false,
                         notation);
    }
}

void ValidateCanvasOperation(const DiagramDocument& doc,
                             const std::string& operation,
                             const std::string& node_id,
                             const std::string& target_parent_id) {
    (void)target_parent_id;
    ValidateNotation(doc.notation);
    const std::set<std::string> ops = {"drag", "resize", "connect"};
    if (ops.count(operation) == 0U) {
        throw MakeReject("SRB1-R-6201", "invalid diagram operation", "diagram", "canvas_operation");
    }
    if (node_id.empty()) {
        throw MakeReject("SRB1-R-6201", "missing node id", "diagram", "canvas_operation");
    }
    bool found = false;
    for (const auto& n : doc.nodes) {
        if (n.node_id == node_id) {
            found = true;
            break;
        }
    }
    if (!found) {
        throw MakeReject("SRB1-R-6201", "node not found", "diagram", "canvas_operation", false, node_id);
    }
}

std::string SerializeDiagramModel(const DiagramDocument& doc) {
    ValidateNotation(doc.notation);
    if (doc.diagram_id.empty()) {
        throw MakeReject("SRB1-R-6101", "diagram_id required", "diagram", "serialize_model");
    }
    std::ostringstream out;
    out << "{\"diagram_id\":\"" << JsonEscape(doc.diagram_id)
        << "\",\"notation\":\"" << JsonEscape(doc.notation) << "\",\"nodes\":[";
    for (size_t i = 0; i < doc.nodes.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        const auto& n = doc.nodes[i];
        if (n.node_id.empty() || n.object_type.empty()) {
            throw MakeReject("SRB1-R-6101", "invalid diagram node", "diagram", "serialize_model");
        }
        out << "{\"node_id\":\"" << JsonEscape(n.node_id)
            << "\",\"object_type\":\"" << JsonEscape(n.object_type)
            << "\",\"parent_node_id\":\"" << JsonEscape(n.parent_node_id)
            << "\",\"x\":" << n.x
            << ",\"y\":" << n.y
            << ",\"width\":" << n.width
            << ",\"height\":" << n.height
            << ",\"logical_datatype\":\"" << JsonEscape(n.logical_datatype) << "\"}";
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
            << "\",\"relation_type\":\"" << JsonEscape(e.relation_type) << "\"}";
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
    ValidateNotation(doc.notation);

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
        n.parent_node_id = RequireString(v, "parent_node_id", "SRB1-R-6101", "diagram", "parse_model");
        std::string dtype;
        const JsonValue* dtv = FindMember(v, "logical_datatype");
        if (dtv != nullptr && GetStringValue(*dtv, &dtype)) {
            n.logical_datatype = dtype;
        }
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
        std::string rel;
        const JsonValue* rv = FindMember(v, "relation_type");
        if (rv != nullptr && GetStringValue(*rv, &rel)) {
            e.relation_type = rel;
        }
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
        return std::tie(a.asset_type, a.id) < std::tie(b.asset_type, b.id);
    });
    return out;
}

std::string CanonicalizeRRule(const std::map<std::string, std::string>& key_values) {
    static const std::set<std::string> allowed = {"FREQ",      "INTERVAL",  "COUNT",    "UNTIL",   "BYSECOND",
                                                  "BYMINUTE",  "BYHOUR",    "BYDAY",    "BYMONTHDAY", "BYYEARDAY",
                                                  "BYWEEKNO",  "BYMONTH",   "BYSETPOS", "WKST"};

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
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());

    std::vector<std::string> pairs;
    pairs.reserve(keys.size());
    for (const auto& k : keys) {
        pairs.push_back(k + "=" + key_values.at(k));
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

    std::int64_t step_seconds = 0;
    const std::string freq = kv["FREQ"];
    if (freq == "SECONDLY") {
        step_seconds = interval;
    } else if (freq == "MINUTELY") {
        step_seconds = 60LL * interval;
    } else if (freq == "HOURLY") {
        step_seconds = 3600LL * interval;
    } else if (freq == "DAILY") {
        step_seconds = 86400LL * interval;
    } else if (freq == "WEEKLY") {
        step_seconds = 7LL * 86400LL * interval;
    } else if (freq == "MONTHLY") {
        step_seconds = 30LL * 86400LL * interval;
    } else if (freq == "YEARLY") {
        step_seconds = 365LL * 86400LL * interval;
    } else {
        throw MakeReject("SRB1-R-7101", "invalid FREQ", "reporting", "expand_rrule_bounded");
    }

    if (step_seconds <= 0) {
        throw MakeReject("SRB1-R-7101", "invalid interval", "reporting", "expand_rrule_bounded");
    }

    std::set<std::string> out;
    std::time_t t = *anchor;
    std::size_t eval_count = 0;
    int emitted = 0;

    while (eval_count < max_candidates) {
        ++eval_count;
        if (until.has_value() && t > *until) {
            break;
        }
        if (t > *now) {
            out.insert(FormatUtc(t));
            ++emitted;
        }
        if (count_limit >= 0 && emitted >= count_limit) {
            break;
        }
        t += step_seconds;
    }

    if (eval_count >= max_candidates && out.empty()) {
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
    const std::string result = exec(normalized_sql);
    if (!persist_result_fn(result)) {
        throw MakeReject("SRB1-R-7002", "result storage failure", "reporting", "run_question");
    }
    return result;
}

std::string RunDashboardRuntime(const std::string& dashboard_id,
                                const std::vector<std::pair<std::string, std::string>>& widget_statuses,
                                bool cache_hit) {
    if (dashboard_id.empty()) {
        throw MakeReject("SRB1-R-7001", "dashboard id missing", "reporting", "run_dashboard_runtime");
    }
    auto sorted = widget_statuses;
    std::sort(sorted.begin(), sorted.end());

    std::ostringstream out;
    out << "{\"dashboard_id\":\"" << JsonEscape(dashboard_id) << "\",\"widgets\":[";
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << "{\"widget_id\":\"" << JsonEscape(sorted[i].first) << "\",\"status\":\"" << JsonEscape(sorted[i].second)
            << "\"}";
    }
    out << "],\"cache\":{\"hit\":" << (cache_hit ? "true" : "false") << "}}";
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
        out << "{\"id\":\"" << JsonEscape(a.id)
            << "\",\"asset_type\":\"" << JsonEscape(a.asset_type)
            << "\",\"name\":\"" << JsonEscape(a.name)
            << "\",\"payload_json\":\"" << JsonEscape(a.payload_json) << "\"}";
    }
    out << "]}";
    return out.str();
}

std::vector<ReportingAsset> ImportReportingRepository(const std::string& payload_json) {
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
        const JsonValue& payload = RequireMember(v, "payload_json", "SRB1-R-7003", "reporting", "import_repository");
        if (payload.type != JsonValue::Type::String) {
            throw MakeReject("SRB1-R-7003", "invalid payload_json", "reporting", "import_repository");
        }
        a.payload_json = payload.string_value;
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

    std::string build_hash = RequireString(manifest, "build_hash", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (build_hash.size() != 64U || !IsHexLower(build_hash)) {
        throw MakeReject("SRB1-R-9002", "invalid build_hash", "packaging", "validate_profile_manifest");
    }

    std::string ts = RequireString(manifest, "build_timestamp_utc", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (!IsRfc3339Utc(ts)) {
        throw MakeReject("SRB1-R-9002", "invalid build_timestamp_utc", "packaging", "validate_profile_manifest");
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
    (void)RequireString(security_defaults, "security_mode", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    (void)RequireString(security_defaults, "credential_store_policy", "SRB1-R-9002", "packaging", "validate_profile_manifest");

    const JsonValue& artifacts = RequireMember(manifest, "artifacts", "SRB1-R-9002", "packaging", "validate_profile_manifest");
    if (artifacts.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-9002", "invalid artifacts", "packaging", "validate_profile_manifest");
    }
    for (const auto& key : {"license_path", "attribution_path", "help_root_path", "config_template_path", "connections_template_path"}) {
        std::string v = RequireString(artifacts, key, "SRB1-R-9002", "packaging", "validate_profile_manifest");
        if (v.find("..") != std::string::npos || (!v.empty() && v[0] == '/')) {
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
    if (out.package_root.find("..") != std::string::npos || out.authoritative_inventory_relpath.find("..") != std::string::npos) {
        throw MakeReject("SRB1-R-5402", "path traversal in manifest", "spec_workspace", "load_specset_manifest");
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
            rows.push_back(rel);
        }
    }

    if (rows.empty()) {
        throw MakeReject("SRB1-R-5402", "inventory parse failure", "spec_workspace", "parse_inventory", false, inventory_path);
    }
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
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
    for (const auto& rel : rel_files) {
        const std::filesystem::path abs = package_root / rel;
        if (!std::filesystem::exists(abs)) {
            throw MakeReject("SRB1-R-5402", "missing normative file", "spec_workspace", "load_specset_package", false, rel);
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
    for (const auto& id : binding_case_ids) {
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
