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
#include "scratchbird/client/connection.h"
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
        scratchbird::client::ConnectionConfig net_config;
        
        // For embedded mode, we connect to the local embedded server
        // which may use shared memory or localhost
        net_config.database_name = config.database;
        net_config.username = config.username;
        net_config.password = config.password;
        net_config.connect_timeout_ms = static_cast<uint32_t>(config.connectTimeoutMs);
        net_config.read_timeout_ms = static_cast<uint32_t>(config.readTimeoutMs);
        net_config.write_timeout_ms = static_cast<uint32_t>(config.writeTimeoutMs);
        net_config.tcp_port = config.port > 0 ? static_cast<uint16_t>(config.port) : 3092;
        
        scratchbird::core::ErrorContext ctx;
        auto status = client_.connect(net_config, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
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

        scratchbird::client::ResultSet results;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.executeQuery(sql, &results, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }

        outResult->columns.clear();
        outResult->rows.clear();
        outResult->rowsAffected = results.getRowsAffected();
        outResult->commandTag = results.getCommandTag();

        // Get column metadata
        const auto& columns = results.getColumns();
        for (const auto& col : columns) {
            QueryColumn column;
            column.name = col.name;
            // Map wire type to type name
            column.type = WireTypeToTypeName(col.type);
            outResult->columns.push_back(std::move(column));
        }

        // Iterate through results using next()
        while (results.next()) {
            std::vector<QueryValue> out_row;
            out_row.reserve(columns.size());
            for (size_t i = 0; i < columns.size(); ++i) {
                QueryValue cell;
                cell.isNull = results.isNull(i);
                if (!cell.isNull) {
                    cell.text = GetColumnValue(results, i, columns[i].type);
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
        // TODO: Query cancellation not yet implemented in ScratchBird client
        if (error) *error = "Query cancellation not supported";
        return false;
    }

    BackendCapabilities Capabilities() const override {
        return capabilities_;
    }

    std::string BackendName() const override {
        return "ScratchBird-Embedded";
    }

private:
    std::string WireTypeToTypeName(scratchbird::protocol::WireType type) {
        using scratchbird::protocol::WireType;
        switch (type) {
            case WireType::BOOLEAN: return "BOOLEAN";
            case WireType::INT16: return "INT2";
            case WireType::INT32: return "INT4";
            case WireType::INT64: return "INT8";
            case WireType::FLOAT32: return "FLOAT4";
            case WireType::FLOAT64: return "FLOAT8";
            case WireType::VARCHAR: return "TEXT";
            case WireType::BYTEA: return "BYTEA";
            case WireType::TIMESTAMP: return "TIMESTAMP";
            case WireType::TIMESTAMPTZ: return "TIMESTAMPTZ";
            case WireType::DATE: return "DATE";
            case WireType::TIME: return "TIME";
            case WireType::INTERVAL: return "INTERVAL";
            case WireType::DECIMAL: return "NUMERIC";
            case WireType::UUID: return "UUID";
            case WireType::JSON: return "JSON";
            case WireType::JSONB: return "JSONB";
            case WireType::ARRAY: return "ARRAY";
            case WireType::NULL_TYPE: return "NULL";
            default: return "UNKNOWN";
        }
    }

    std::string GetColumnValue(scratchbird::client::ResultSet& results, size_t column,
                               scratchbird::protocol::WireType type) {
        using scratchbird::protocol::WireType;
        
        if (results.isNull(column)) {
            return "";
        }

        switch (type) {
            case WireType::BOOLEAN:
                return results.getBool(column) ? "true" : "false";
            case WireType::INT16:
                return std::to_string(results.getInt16(column));
            case WireType::INT32:
                return std::to_string(results.getInt32(column));
            case WireType::INT64:
                return std::to_string(results.getInt64(column));
            case WireType::FLOAT32:
                return std::to_string(results.getFloat(column));
            case WireType::FLOAT64:
                return std::to_string(results.getDouble(column));
            case WireType::VARCHAR:
            case WireType::JSON:
            case WireType::JSONB:
                return results.getString(column);
            case WireType::BYTEA:
                return BytesToHex(results.getBytes(column));
            case WireType::UUID:
                return results.getUUID(column);
            case WireType::TIMESTAMP:
            case WireType::TIMESTAMPTZ:
                return std::to_string(results.getTimestamp(column));
            case WireType::DATE:
                return std::to_string(results.getDate(column));
            case WireType::TIME:
                return std::to_string(results.getTime(column));
            default:
                // For unknown types, try to get as string
                return results.getString(column);
        }
    }

    scratchbird::client::Connection client_;
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
