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
#include <chrono>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
#include "scratchbird/core/status.h"
#include "scratchbird/core/error_context.h"
#include "scratchbird/client/connection.h"
#include "scratchbird/network/socket_types.h"
#include "scratchbird/protocol/wire_protocol.h"
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
namespace {
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

struct ScratchbirdPreparedStatement : PreparedStatementHandle {
    scratchbird::client::PreparedStatement stmt;
};

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
        net_config.auto_start_server = true;  // Auto-start embedded server
        
        // Try to find sb_server in common build locations
        std::vector<std::string> search_paths = {
            "./scratchbird/src/sb_server",                                    // From build dir
            "./build/scratchbird/src/sb_server",                             // From project root
            "../scratchbird/src/sb_server",                                  // Relative to binary
            "/home/dcalford/CliWork/ScratchRobin/build/scratchbird/src/sb_server"  // Absolute path
        };
        for (const auto& path : search_paths) {
            std::ifstream f(path);
            if (f.good()) {
                net_config.server_executable = path;
                std::cerr << "[EmbeddedBackend] Found server at: " << path << std::endl;
                break;
            }
        }
        
        if (config.streamWindowBytes > 0) {
            net_config.copy_window_bytes = config.streamWindowBytes;
        }
        if (config.streamChunkBytes > 0) {
            net_config.copy_chunk_bytes = config.streamChunkBytes;
        }
        
        std::cerr << "[EmbeddedBackend] Connecting to database: " << config.database << std::endl;
        std::cerr << "[EmbeddedBackend] Port: " << net_config.tcp_port << std::endl;
        std::cerr << "[EmbeddedBackend] Auto-start: " << (net_config.auto_start_server ? "true" : "false") << std::endl;
        std::cerr << "[EmbeddedBackend] Server path: " << (net_config.server_executable.empty() ? "(will search PATH)" : net_config.server_executable) << std::endl;
        
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

    bool ExecuteQuery(const std::string& sql, const QueryOptions& options,
                      QueryResult* outResult, std::string* error) override {
        if (!outResult) {
            if (error) *error = "No result buffer provided";
            return false;
        }

        scratchbird::client::ResultSet results;
        scratchbird::core::ErrorContext ctx;
        uint8_t flags = 0;
        if (options.streaming) {
            flags |= static_cast<uint8_t>(scratchbird::protocol::QueryFlags::STREAMING);
        }
        auto status = client_.executeQuery(sql, &results, flags, &ctx);
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

        const auto& columns = results.getColumns();
        for (const auto& col : columns) {
            QueryColumn column;
            column.name = col.name;
            column.type = WireTypeToTypeName(col.type);
            outResult->columns.push_back(std::move(column));
        }

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

    bool ExecuteCopy(const CopyOptions& options, std::istream* input,
                     std::ostream* output, CopyResult* outResult,
                     std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto start = std::chrono::steady_clock::now();

        client_.setCopyInputStream(input);
        client_.setCopyOutputStream(output);

        scratchbird::client::ResultSet results;
        auto status = client_.executeQuery(options.sql, &results, &ctx);

        client_.setCopyInputStream(nullptr);
        client_.setCopyOutputStream(nullptr);

        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }

        if (outResult) {
            outResult->rowsProcessed = results.getRowsAffected();
            outResult->commandTag = results.getCommandTag();
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

        scratchbird::client::ResultSet results;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.executeQuery(handle->stmt, &results, &ctx);
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

        const auto& columns = results.getColumns();
        for (const auto& col : columns) {
            QueryColumn column;
            column.name = col.name;
            column.type = WireTypeToTypeName(col.type);
            outResult->columns.push_back(std::move(column));
        }

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
