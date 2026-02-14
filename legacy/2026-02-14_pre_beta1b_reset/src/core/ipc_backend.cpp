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
#include <chrono>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
#include "scratchbird/client/connection.h"
#include "scratchbird/core/error_context.h"
#include "scratchbird/network/socket_types.h"
#include "scratchbird/protocol/sbwp_protocol.h"
#include "scratchbird/protocol/wire_protocol.h"
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

scratchbird::protocol::StatusRequestType ToStatusRequestType(StatusRequestKind kind) {
    using scratchbird::protocol::StatusRequestType;
    switch (kind) {
        case StatusRequestKind::ServerInfo: return StatusRequestType::SERVER_INFO;
        case StatusRequestKind::ConnectionInfo: return StatusRequestType::CONNECTION_INFO;
        case StatusRequestKind::DatabaseInfo: return StatusRequestType::DATABASE_INFO;
        case StatusRequestKind::Statistics: return StatusRequestType::STATISTICS;
    }
    return StatusRequestType::SERVER_INFO;
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

struct ScratchbirdPreparedStatement : PreparedStatementHandle {
    scratchbird::client::PreparedStatement stmt;
};

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
        if (config.streamWindowBytes > 0) {
            conn_config.copy_window_bytes = config.streamWindowBytes;
        }
        if (config.streamChunkBytes > 0) {
            conn_config.copy_chunk_bytes = config.streamChunkBytes;
        }
        
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
        capabilities_.supportsPreparedStatements = true;
        capabilities_.supportsStatementCache = true;
        capabilities_.supportsCopyIn = true;
        capabilities_.supportsCopyOut = true;
        capabilities_.supportsCopyBoth = true;
        capabilities_.supportsCopyBinary = true;
        capabilities_.supportsCopyText = true;
        capabilities_.supportsNotifications = true;
        capabilities_.supportsStatus = true;
        
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

    bool ExecuteQuery(const std::string& sql, const QueryOptions& options,
                      QueryResult* outResult, std::string* error) override {
        if (!outResult) {
            if (error) *error = "No result buffer provided";
            return false;
        }

        scratchbird::client::ResultSet rs;
        scratchbird::core::ErrorContext ctx;
        uint8_t flags = 0;
        if (options.streaming) {
            flags |= static_cast<uint8_t>(scratchbird::protocol::QueryFlags::STREAMING);
        }
        auto status = client_.executeQuery(sql, &rs, flags, &ctx);
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

    bool ExecuteCopy(const CopyOptions& options, std::istream* input,
                     std::ostream* output, CopyResult* outResult,
                     std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto start = std::chrono::steady_clock::now();

        client_.setCopyInputStream(input);
        client_.setCopyOutputStream(output);

        scratchbird::client::ResultSet rs;
        auto status = client_.executeQuery(options.sql, &rs, &ctx);

        client_.setCopyInputStream(nullptr);
        client_.setCopyOutputStream(nullptr);

        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }

        if (outResult) {
            outResult->rowsProcessed = rs.getRowsAffected();
            outResult->commandTag = rs.getCommandTag();
            outResult->elapsedMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
        }

        return true;
    }

    bool PrepareStatement(const std::string& sql,
                          std::shared_ptr<PreparedStatementHandle>* outStatement,
                          std::string* error) override {
        if (!outStatement) {
            if (error) *error = "No prepared statement handle provided";
            return false;
        }
        scratchbird::client::PreparedStatement stmt;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.prepare(sql, &stmt, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        auto handle = std::make_shared<ScratchbirdPreparedStatement>();
        handle->stmt = std::move(stmt);
        handle->sql = sql;
        handle->parameterCount = handle->stmt.getParameterCount();
        *outStatement = handle;
        return true;
    }

    bool ExecutePrepared(const std::shared_ptr<PreparedStatementHandle>& statement,
                         const std::vector<PreparedParameter>& params,
                         QueryResult* outResult,
                         std::string* error) override {
        auto handle = std::dynamic_pointer_cast<ScratchbirdPreparedStatement>(statement);
        if (!handle) {
            if (error) *error = "Prepared statement type mismatch";
            return false;
        }

        handle->stmt.clearParameters();
        for (size_t i = 0; i < params.size(); ++i) {
            const auto& param = params[i];
            size_t index = i + 1;
            switch (param.type) {
                case PreparedParamType::Null:
                    handle->stmt.setNull(index);
                    break;
                case PreparedParamType::Bool:
                    handle->stmt.setBool(index, param.boolValue);
                    break;
                case PreparedParamType::Int64:
                    handle->stmt.setInt64(index, param.intValue);
                    break;
                case PreparedParamType::Double:
                    handle->stmt.setDouble(index, param.doubleValue);
                    break;
                case PreparedParamType::String:
                    handle->stmt.setString(index, param.stringValue);
                    break;
                case PreparedParamType::Bytes:
                    handle->stmt.setBytes(index, param.bytesValue);
                    break;
            }
        }

        scratchbird::client::ResultSet rs;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.executeQuery(handle->stmt, &rs, &ctx);
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

        handle->stmt.clearParameters();
        return true;
    }

    void ClosePrepared(const std::shared_ptr<PreparedStatementHandle>& statement) override {
        auto handle = std::dynamic_pointer_cast<ScratchbirdPreparedStatement>(statement);
        if (handle) {
            client_.closeStatement(handle->stmt);
        }
    }

    bool Subscribe(const std::string& channel, const std::string& filter,
                   std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.subscribe(channel, filter, 0, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            return false;
        }
        return true;
    }

    bool Unsubscribe(const std::string& channel, std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.unsubscribe(channel, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            return false;
        }
        return true;
    }

    bool FetchNotification(NotificationEvent* outEvent, std::string* error) override {
        if (!outEvent) {
            if (error) *error = "No notification buffer provided";
            return false;
        }
        scratchbird::client::Connection::Notification note;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.receiveNotification(&note, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            return false;
        }
        outEvent->processId = note.processId;
        outEvent->channel = note.channel;
        outEvent->payload = note.payload;
        outEvent->changeType = note.changeType;
        outEvent->rowId = note.rowId;
        return true;
    }

    bool FetchStatus(StatusRequestKind kind, StatusSnapshot* outSnapshot,
                     std::string* error) override {
        if (!outSnapshot) {
            if (error) *error = "No status buffer provided";
            return false;
        }
        scratchbird::client::Connection::StatusResponse response;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.requestStatus(ToStatusRequestType(kind), &response, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            return false;
        }
        outSnapshot->kind = kind;
        outSnapshot->entries.clear();
        outSnapshot->entries.reserve(response.entries.size());
        for (const auto& entry : response.entries) {
            StatusEntry out_entry;
            out_entry.key = entry.key;
            out_entry.value = entry.value;
            outSnapshot->entries.push_back(std::move(out_entry));
        }
        return true;
    }

    void SetProgressCallback(std::function<void(uint64_t, uint64_t)> callback) override {
        client_.setProgressCallback(std::move(callback));
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
        auto status = client_.cancelQuery(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
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
