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
#include "scratchbird/client/connection.h"
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
        case WireType::VARCHAR: return "VARCHAR";
        // TEXT type not available, VARCHAR used instead
        case WireType::BYTEA: return "BYTEA";
        case WireType::DATE: return "DATE";
        case WireType::TIME: return "TIME";
        case WireType::TIMESTAMP: return "TIMESTAMP";
        case WireType::UUID: return "UUID";
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

std::string FormatColumnValue(scratchbird::client::ResultSet* rs, int columnIndex,
                              scratchbird::protocol::WireType type) {
    if (rs->isNull(columnIndex)) {
        return "NULL";
    }

    switch (type) {
        case scratchbird::protocol::WireType::BOOLEAN:
            return rs->getBool(columnIndex) ? "true" : "false";
        case scratchbird::protocol::WireType::INT16:
            return std::to_string(rs->getInt16(columnIndex));
        case scratchbird::protocol::WireType::INT32:
            return std::to_string(rs->getInt32(columnIndex));
        case scratchbird::protocol::WireType::INT64:
            return std::to_string(rs->getInt64(columnIndex));
        case scratchbird::protocol::WireType::FLOAT32:
            return std::to_string(rs->getFloat(columnIndex));
        case scratchbird::protocol::WireType::FLOAT64:
            return std::to_string(rs->getDouble(columnIndex));
        case scratchbird::protocol::WireType::VARCHAR:
            return rs->getString(columnIndex);
        case scratchbird::protocol::WireType::BYTEA: {
            auto data = rs->getBytes(columnIndex);
            return BytesToHex(data);
        }
        case scratchbird::protocol::WireType::DATE:
            return std::to_string(rs->getDate(columnIndex));
        case scratchbird::protocol::WireType::TIME:
            return std::to_string(rs->getTime(columnIndex));
        case scratchbird::protocol::WireType::TIMESTAMP:
            return std::to_string(rs->getTimestamp(columnIndex));
        case scratchbird::protocol::WireType::UUID: {
            return rs->getUUID(columnIndex);
        }
        case scratchbird::protocol::WireType::NULL_TYPE:
            return "NULL";
    }
    return "<unknown>";
}

class IpcBackend : public ConnectionBackend {
public:
    bool Connect(const BackendConfig& config, std::string* error) override {
        scratchbird::client::ConnectionConfig conn_config;
        
        // Resolve socket path
        std::string socket_path = ResolveSocketPath(config.host, config.database);
        conn_config.database_name = config.database;
        conn_config.username = config.username;
        conn_config.password = config.password;
        // Application name not in new API
        conn_config.connect_timeout_ms = static_cast<uint32_t>(config.connectTimeoutMs);
        conn_config.read_timeout_ms = static_cast<uint32_t>(config.readTimeoutMs);
        conn_config.write_timeout_ms = static_cast<uint32_t>(config.writeTimeoutMs);
        
        // IPC uses REQUIRE by default (still uses TLS over Unix sockets if available)
        // SSL mode not in new ConnectionConfig API
        
        scratchbird::core::ErrorContext ctx;
        auto status = client_.connect(conn_config, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
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

        scratchbird::client::ResultSet rs;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.executeQuery(sql, &rs, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }

        outResult->columns.clear();
        outResult->rows.clear();
        outResult->rowsAffected = rs.getRowsAffected();
        outResult->commandTag = rs.getCommandTag();

        int columnCount = static_cast<int>(rs.getColumnCount());
        std::vector<scratchbird::protocol::WireType> types;
        types.reserve(columnCount);

        for (int i = 0; i < columnCount; ++i) {
            QueryColumn column;
            column.name = rs.getColumnName(i);
            column.type = WireTypeToString(rs.getColumnType(i));
            outResult->columns.push_back(std::move(column));
            types.push_back(rs.getColumnType(i));
        }

        while (rs.next()) {
            std::vector<QueryValue> out_row;
            out_row.reserve(columnCount);
            for (int i = 0; i < columnCount; ++i) {
                QueryValue cell;
                cell.isNull = rs.isNull(i);
                if (!cell.isNull) {
                    cell.text = FormatColumnValue(&rs, i, types[i]);
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
        // Query cancellation not supported in current API
        if (error) *error = "Query cancellation not supported";
        return false;
    }

    BackendCapabilities Capabilities() const override {
        return capabilities_;
    }

    std::string BackendName() const override {
        return "ScratchBird-IPC";
    }

private:
    scratchbird::client::Connection client_;
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
