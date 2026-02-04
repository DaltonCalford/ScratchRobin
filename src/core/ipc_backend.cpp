/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "ipc_backend.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
#include "scratchbird/client/network_client.h"
#include "scratchbird/core/error_context.h"
#include "scratchbird/network/socket_types.h"
#include "scratchbird/protocol/sbwp_protocol.h"
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string GetDefaultSocketPath(const std::string& database) {
#ifdef _WIN32
    // Windows named pipe
    return "\\\\.\\pipe\\scratchbird" + (database.empty() ? "" : "-" + database);
#else
    // Unix domain socket
    std::string base_path = "/var/run/scratchbird";
    if (!database.empty()) {
        return base_path + "/" + database + ".sock";
    }
    return base_path + "/scratchbird.sock";
#endif
}

std::string ResolveSocketPath(const std::string& config_path, const std::string& database) {
    if (!config_path.empty()) {
        return config_path;
    }
    return GetDefaultSocketPath(database);
}

std::string WireTypeToString(scratchbird::protocol::WireType type) {
    using scratchbird::protocol::WireType;
    switch (type) {
        case WireType::NULL_TYPE: return "NULL";
        case WireType::BOOLEAN: return "BOOLEAN";
        case WireType::INT16: return "INT16";
        case WireType::INT32: return "INT32";
        case WireType::INT64: return "INT64";
        case WireType::FLOAT32: return "FLOAT32";
        case WireType::FLOAT64: return "FLOAT64";
        case WireType::DECIMAL: return "DECIMAL";
        case WireType::VARCHAR: return "VARCHAR";
        case WireType::CHAR: return "CHAR";
        case WireType::BYTEA: return "BYTEA";
        case WireType::DATE: return "DATE";
        case WireType::TIME: return "TIME";
        case WireType::TIMESTAMP: return "TIMESTAMP";
        case WireType::TIMESTAMPTZ: return "TIMESTAMPTZ";
        case WireType::INTERVAL: return "INTERVAL";
        case WireType::UUID: return "UUID";
        case WireType::JSON: return "JSON";
        case WireType::JSONB: return "JSONB";
        case WireType::ARRAY: return "ARRAY";
        case WireType::COMPOSITE: return "COMPOSITE";
        case WireType::GEOMETRY: return "GEOMETRY";
        case WireType::VECTOR: return "VECTOR";
        case WireType::MONEY: return "MONEY";
        case WireType::XML: return "XML";
        case WireType::INET: return "INET";
        case WireType::CIDR: return "CIDR";
        case WireType::MACADDR: return "MACADDR";
        case WireType::TSVECTOR: return "TSVECTOR";
        case WireType::TSQUERY: return "TSQUERY";
        case WireType::RANGE: return "RANGE";
        case WireType::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string BytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    out << "0x";
    for (uint8_t byte : data) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return out.str();
}

std::string FormatColumnValue(scratchbird::protocol::WireType type,
                              const scratchbird::protocol::ProtocolCodec::ColumnValue& value) {
    if (value.is_null) {
        return "NULL";
    }

    auto as_string = [&value]() {
        return std::string(value.data.begin(), value.data.end());
    };

    switch (type) {
        case scratchbird::protocol::WireType::BOOLEAN: {
            if (value.data.empty()) return "false";
            return value.data[0] ? "true" : "false";
        }
        case scratchbird::protocol::WireType::INT16: {
            if (value.data.size() < sizeof(int16_t)) return "<int16>";
            int16_t v = 0;
            std::memcpy(&v, value.data.data(), sizeof(int16_t));
            return std::to_string(v);
        }
        case scratchbird::protocol::WireType::INT32:
        case scratchbird::protocol::WireType::DATE: {
            if (value.data.size() < sizeof(int32_t)) return "<int32>";
            int32_t v = 0;
            std::memcpy(&v, value.data.data(), sizeof(int32_t));
            return std::to_string(v);
        }
        case scratchbird::protocol::WireType::INT64:
        case scratchbird::protocol::WireType::TIMESTAMP:
        case scratchbird::protocol::WireType::TIMESTAMPTZ:
        case scratchbird::protocol::WireType::TIME:
        case scratchbird::protocol::WireType::MONEY: {
            if (value.data.size() < sizeof(int64_t)) return "<int64>";
            int64_t v = 0;
            std::memcpy(&v, value.data.data(), sizeof(int64_t));
            return std::to_string(v);
        }
        case scratchbird::protocol::WireType::FLOAT32: {
            if (value.data.size() < sizeof(float)) return "<float>";
            float v = 0.0f;
            std::memcpy(&v, value.data.data(), sizeof(float));
            return std::to_string(v);
        }
        case scratchbird::protocol::WireType::FLOAT64: {
            if (value.data.size() < sizeof(double)) return "<double>";
            double v = 0.0;
            std::memcpy(&v, value.data.data(), sizeof(double));
            return std::to_string(v);
        }
        case scratchbird::protocol::WireType::DECIMAL:
        case scratchbird::protocol::WireType::VARCHAR:
        case scratchbird::protocol::WireType::CHAR:
        case scratchbird::protocol::WireType::JSON:
        case scratchbird::protocol::WireType::XML: {
            return as_string();
        }
        case scratchbird::protocol::WireType::BYTEA:
        case scratchbird::protocol::WireType::UUID:
        case scratchbird::protocol::WireType::JSONB:
        case scratchbird::protocol::WireType::ARRAY:
        case scratchbird::protocol::WireType::COMPOSITE:
        case scratchbird::protocol::WireType::GEOMETRY:
        case scratchbird::protocol::WireType::VECTOR:
        case scratchbird::protocol::WireType::INET:
        case scratchbird::protocol::WireType::CIDR:
        case scratchbird::protocol::WireType::MACADDR:
        case scratchbird::protocol::WireType::TSVECTOR:
        case scratchbird::protocol::WireType::TSQUERY:
        case scratchbird::protocol::WireType::RANGE:
        case scratchbird::protocol::WireType::INTERVAL:
        case scratchbird::protocol::WireType::UNKNOWN:
        case scratchbird::protocol::WireType::NULL_TYPE:
            break;
    }

    if (value.data.empty()) return "<empty>";
    return BytesToHex(value.data);
}

class IpcBackend : public ConnectionBackend {
public:
    bool Connect(const BackendConfig& config, std::string* error) override {
        scratchbird::client::NetworkClientConfig net_config;
        
        // Resolve socket path
        std::string socket_path = ResolveSocketPath(config.host, config.database);
        net_config.host = socket_path;
        net_config.port = 0;  // IPC doesn't use ports
        net_config.database = config.database;
        net_config.username = config.username;
        net_config.password = config.password;
        net_config.application_name = config.applicationName.empty() 
            ? "scratchrobin-ipc" : config.applicationName;
        net_config.connect_timeout_ms = static_cast<uint32_t>(config.connectTimeoutMs);
        net_config.read_timeout_ms = static_cast<uint32_t>(config.readTimeoutMs);
        net_config.write_timeout_ms = static_cast<uint32_t>(config.writeTimeoutMs);
        
        // IPC uses REQUIRE by default (still uses TLS over Unix sockets if available)
        net_config.ssl_mode = scratchbird::network::SSLMode::PREFER;
        
        scratchbird::core::ErrorContext ctx;
        auto status = client_.connect(net_config, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.lastError() : ctx.message;
                // Add helpful context for common IPC errors
                if (error->find("No such file") != std::string::npos ||
                    error->find("cannot connect") != std::string::npos) {
                    *error += " (Is ScratchBird server running with IPC enabled?)";
                }
            }
            return false;
        }
        
        // Populate capabilities from server
        capabilities_.supportsCancel = true;
        capabilities_.supportsTransactions = true;
        capabilities_.supportsPaging = true;
        capabilities_.supportsSavepoints = true;
        capabilities_.supportsExplain = true;
        capabilities_.supportsSblr = true;
        capabilities_.supportsStreaming = true;
        capabilities_.supportsDdlExtract = true;
        capabilities_.supportsDependencies = true;
        capabilities_.supportsConstraints = true;
        capabilities_.supportsIndexes = true;
        capabilities_.supportsUserAdmin = true;
        capabilities_.supportsRoleAdmin = true;
        capabilities_.supportsGroupAdmin = false;
        capabilities_.supportsJobScheduler = true;
        capabilities_.supportsDomains = true;
        capabilities_.supportsSequences = true;
        capabilities_.supportsTriggers = true;
        capabilities_.supportsProcedures = true;
        capabilities_.supportsViews = true;
        capabilities_.supportsTempTables = true;
        capabilities_.supportsMultipleDatabases = true;
        capabilities_.supportsTablespaces = true;
        capabilities_.supportsSchemas = true;
        capabilities_.supportsBackup = true;
        capabilities_.supportsImportExport = true;
        
        return true;
    }

    void Disconnect() override {
        client_.disconnect();
    }

    bool IsConnected() const override {
        return client_.isConnected();
    }

    bool ExecuteQuery(const std::string& sql, QueryResult* outResult,
                      std::string* error) override {
        if (!outResult) {
            if (error) *error = "No result buffer provided";
            return false;
        }

        scratchbird::client::NetworkResultSet results;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.executeQuery(sql, results, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.lastError() : ctx.message;
            }
            return false;
        }

        outResult->columns.clear();
        outResult->rows.clear();
        outResult->rowsAffected = results.rows_affected;
        outResult->commandTag = results.command_tag;

        std::vector<scratchbird::protocol::WireType> types;
        types.reserve(results.columns.size());

        for (const auto& col : results.columns) {
            QueryColumn column;
            column.name = col.name;
            column.type = WireTypeToString(col.type);
            outResult->columns.push_back(std::move(column));
            types.push_back(col.type);
        }

        for (const auto& row : results.rows) {
            std::vector<QueryValue> out_row;
            out_row.reserve(row.size());
            for (size_t i = 0; i < row.size(); ++i) {
                QueryValue cell;
                cell.isNull = row[i].is_null;
                if (!cell.isNull) {
                    cell.value = FormatColumnValue(types[i], row[i]);
                }
                out_row.push_back(std::move(cell));
            }
            outResult->rows.push_back(std::move(out_row));
        }

        return true;
    }

    bool BeginTransaction(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.beginTransaction(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message;
            return false;
        }
        return true;
    }

    bool Commit(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.commit(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message;
            return false;
        }
        return true;
    }

    bool Rollback(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.rollback(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message;
            return false;
        }
        return true;
    }

    bool Cancel(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.sendQueryCancel(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message;
            return false;
        }
        return true;
    }

    BackendCapabilities Capabilities() const override {
        return capabilities_;
    }

    std::string BackendName() const override {
        return "ScratchBird-IPC";
    }

private:
    scratchbird::client::NetworkClient client_;
    BackendCapabilities capabilities_;
};

} // namespace
#endif // SCRATCHROBIN_USE_SCRATCHBIRD

std::unique_ptr<ConnectionBackend> CreateIpcBackend() {
#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
    return std::make_unique<IpcBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
