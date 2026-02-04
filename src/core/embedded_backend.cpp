/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "embedded_backend.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <fstream>

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
#include "scratchbird/core/status.h"
#include "scratchbird/core/error_context.h"
#include "scratchbird/client/network_client.h"
#include "scratchbird/network/socket_types.h"
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
namespace {

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

std::string BytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    out << "0x";
    for (uint8_t byte : data) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return out.str();
}

// Embedded backend for ScratchBird
// 
// The embedded backend uses the ScratchBird network client with a special
// "embedded" connection mode. When ScratchBird server is running in embedded
// mode, it accepts direct memory-mapped connections without TCP overhead.
//
// For now, this uses localhost TCP as a bridge until full embedded API is ready.
class EmbeddedBackend : public ConnectionBackend {
public:
    EmbeddedBackend() = default;
    
    ~EmbeddedBackend() {
        Disconnect();
    }

    bool Connect(const BackendConfig& config, std::string* error) override {
        scratchbird::client::NetworkClientConfig net_config;
        
        // For embedded mode, we connect to the local embedded server
        // which may use shared memory or localhost
        net_config.host = config.host.empty() ? "localhost" : config.host;
        net_config.port = config.port > 0 ? static_cast<uint16_t>(config.port) : 3092;
        net_config.database = config.database;
        net_config.username = config.username;
        net_config.password = config.password;
        net_config.application_name = config.applicationName.empty() 
            ? "scratchrobin-embedded" : config.applicationName;
        net_config.connect_timeout_ms = static_cast<uint32_t>(config.connectTimeoutMs);
        net_config.read_timeout_ms = static_cast<uint32_t>(config.readTimeoutMs);
        net_config.write_timeout_ms = static_cast<uint32_t>(config.writeTimeoutMs);
        
        // Embedded mode can use PREFER for SSL (often disabled for local)
        net_config.ssl_mode = scratchbird::network::SSLMode::PREFER;
        
        scratchbird::core::ErrorContext ctx;
        auto status = client_.connect(net_config, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.lastError() : ctx.message;
                if (error->find("refused") != std::string::npos ||
                    error->find("No such file") != std::string::npos) {
                    *error += " (Is ScratchBird embedded server running?)";
                }
            }
            return false;
        }
        
        // Populate capabilities
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

        for (const auto& col : results.columns) {
            QueryColumn column;
            column.name = col.name;
            // Map type_oid to type name
            column.type = OidToTypeName(col.type_oid);
            outResult->columns.push_back(std::move(column));
        }

        for (const auto& row : results.rows) {
            std::vector<QueryValue> out_row;
            out_row.reserve(row.size());
            for (size_t i = 0; i < row.size(); ++i) {
                QueryValue cell;
                cell.isNull = row[i].is_null;
                if (!cell.isNull) {
                    cell.value = FormatColumnValue(row[i].data, results.columns[i].type_oid);
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
        return "ScratchBird-Embedded";
    }

private:
    std::string OidToTypeName(uint32_t oid) {
        switch (oid) {
            case 16: return "BOOLEAN";
            case 17: return "BYTEA";
            case 18: return "CHAR";
            case 20: return "INT8";
            case 21: return "INT2";
            case 23: return "INT4";
            case 25: return "TEXT";
            case 114: return "JSON";
            case 142: return "XML";
            case 600: return "POINT";
            case 700: return "FLOAT4";
            case 701: return "FLOAT8";
            case 790: return "MONEY";
            case 829: return "MACADDR";
            case 650: return "CIDR";
            case 869: return "INET";
            case 1042: return "BPCHAR";
            case 1043: return "VARCHAR";
            case 1082: return "DATE";
            case 1083: return "TIME";
            case 1114: return "TIMESTAMP";
            case 1184: return "TIMESTAMPTZ";
            case 1186: return "INTERVAL";
            case 1700: return "NUMERIC";
            case 2950: return "UUID";
            case 3802: return "JSONB";
            case 16384: return "VECTOR";
            default: return "UNKNOWN:" + std::to_string(oid);
        }
    }

    std::string FormatColumnValue(const std::vector<uint8_t>& data, uint32_t type_oid) {
        if (data.empty()) {
            return "";
        }

        // For text-based types, interpret as string
        if (type_oid == 25 || type_oid == 1043 || type_oid == 1042 || 
            type_oid == 114 || type_oid == 3802 || type_oid == 142) {
            return std::string(data.begin(), data.end());
        }

        // For binary/bytea, return hex
        if (type_oid == 17) {
            return BytesToHex(data);
        }

        // For integers
        if (type_oid == 21 && data.size() >= 2) {  // INT2
            int16_t v = 0;
            std::memcpy(&v, data.data(), sizeof(int16_t));
            return std::to_string(v);
        }
        if (type_oid == 23 && data.size() >= 4) {  // INT4
            int32_t v = 0;
            std::memcpy(&v, data.data(), sizeof(int32_t));
            return std::to_string(v);
        }
        if (type_oid == 20 && data.size() >= 8) {  // INT8
            int64_t v = 0;
            std::memcpy(&v, data.data(), sizeof(int64_t));
            return std::to_string(v);
        }

        // For floats
        if (type_oid == 700 && data.size() >= 4) {  // FLOAT4
            float v = 0.0f;
            std::memcpy(&v, data.data(), sizeof(float));
            return std::to_string(v);
        }
        if (type_oid == 701 && data.size() >= 8) {  // FLOAT8
            double v = 0.0;
            std::memcpy(&v, data.data(), sizeof(double));
            return std::to_string(v);
        }

        // For boolean
        if (type_oid == 16) {
            return data[0] ? "true" : "false";
        }

        // Default to hex representation
        return BytesToHex(data);
    }

    scratchbird::client::NetworkClient client_;
    BackendCapabilities capabilities_;
};

} // namespace
#endif // SCRATCHROBIN_USE_SCRATCHBIRD

std::unique_ptr<ConnectionBackend> CreateEmbeddedBackend() {
#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
    return std::make_unique<EmbeddedBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
