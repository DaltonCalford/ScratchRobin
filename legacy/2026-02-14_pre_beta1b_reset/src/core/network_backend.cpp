/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "network_backend.h"

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

scratchbird::network::SSLMode ParseSslMode(const std::string& mode) {
    std::string normalized = ToLower(mode);
    if (normalized == "disable" || normalized == "disabled" || normalized == "off") {
        return scratchbird::network::SSLMode::DISABLED;
    }
    if (normalized == "allow") {
        return scratchbird::network::SSLMode::ALLOW;
    }
    if (normalized == "prefer") {
        return scratchbird::network::SSLMode::PREFER;
    }
    if (normalized == "verify_ca") {
        return scratchbird::network::SSLMode::VERIFY_CA;
    }
    if (normalized == "verify_full") {
        return scratchbird::network::SSLMode::VERIFY_FULL;
    }
    if (normalized == "require" || normalized.empty()) {
        return scratchbird::network::SSLMode::REQUIRE;
    }
    return scratchbird::network::SSLMode::REQUIRE;
}

std::string WireTypeToString(scratchbird::protocol::WireType type) {
    using scratchbird::protocol::WireType;
    switch (type) {
        case WireType::NULL_TYPE: return "NULL";
        case WireType::BOOLEAN: return "BOOL";
        case WireType::INT16: return "INT16";
        case WireType::INT32: return "INT32";
        case WireType::INT64: return "INT64";
        case WireType::FLOAT32: return "FLOAT32";
        case WireType::FLOAT64: return "FLOAT64";
        case WireType::VARCHAR: return "VARCHAR";
        case WireType::CHAR: return "CHAR";
        case WireType::BYTEA: return "BYTEA";
        case WireType::DATE: return "DATE";
        case WireType::TIME: return "TIME";
        case WireType::TIMESTAMP: return "TIMESTAMP";
        case WireType::TIMESTAMPTZ: return "TIMESTAMPTZ";
        case WireType::UUID: return "UUID";
        case WireType::DECIMAL: return "DECIMAL";
        case WireType::INTERVAL: return "INTERVAL";
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

class NetworkBackend : public ConnectionBackend {
public:
    bool Connect(const BackendConfig& config, std::string* error) override {
        scratchbird::client::ConnectionConfig net_config;
        net_config.database_name = config.database;
        net_config.username = config.username;
        net_config.password = config.password;
        net_config.connect_timeout_ms = static_cast<uint32_t>(config.connectTimeoutMs);
        net_config.read_timeout_ms = static_cast<uint32_t>(config.readTimeoutMs);
        net_config.write_timeout_ms = static_cast<uint32_t>(config.writeTimeoutMs);
        if (config.streamWindowBytes > 0) {
            net_config.copy_window_bytes = config.streamWindowBytes;
        }
        if (config.streamChunkBytes > 0) {
            net_config.copy_chunk_bytes = config.streamChunkBytes;
        }

        scratchbird::core::ErrorContext ctx;
        auto status = client_.connect(net_config, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
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
            if (error) {
                *error = "No result buffer provided";
            }
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
        size_t column_count = results.getColumnCount();
        for (size_t i = 0; i < column_count; ++i) {
            QueryColumn column;
            column.name = results.getColumnName(i);
            column.type = WireTypeToString(results.getColumnType(i));
            outResult->columns.push_back(std::move(column));
        }

        // Iterate through rows using next()
        while (results.next()) {
            std::vector<QueryValue> out_row;
            out_row.reserve(column_count);
            for (size_t i = 0; i < column_count; ++i) {
                QueryValue cell;
                cell.isNull = results.isNull(i);
                if (!cell.isNull) {
                    // Get raw data
                    size_t length = 0;
                    const uint8_t* raw = results.getRaw(i, &length);
                    if (raw && length > 0) {
                        cell.raw.assign(raw, raw + length);
                    }
                    // Get string representation based on type
                    cell.text = results.getString(i);
                } else {
                    cell.text = "NULL";
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
            if (error) {
                *error = "No result buffer provided";
            }
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

        size_t column_count = results.getColumnCount();
        for (size_t i = 0; i < column_count; ++i) {
            QueryColumn column;
            column.name = results.getColumnName(i);
            column.type = WireTypeToString(results.getColumnType(i));
            outResult->columns.push_back(std::move(column));
        }

        while (results.next()) {
            std::vector<QueryValue> out_row;
            out_row.reserve(column_count);
            for (size_t i = 0; i < column_count; ++i) {
                QueryValue cell;
                cell.isNull = results.isNull(i);
                if (!cell.isNull) {
                    size_t length = 0;
                    const uint8_t* raw = results.getRaw(i, &length);
                    if (raw && length > 0) {
                        cell.raw.assign(raw, raw + length);
                    }
                    cell.text = results.getString(i);
                } else {
                    cell.text = "NULL";
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

    bool FetchStatus(StatusRequestKind kind, StatusSnapshot* outSnapshot,
                     std::string* error) override {
        if (!outSnapshot) {
            if (error) {
                *error = "No status buffer provided";
            }
            return false;
        }
        scratchbird::client::Connection::StatusResponse response;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.requestStatus(ToStatusRequestType(kind), &response, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
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

    bool PrepareStatement(const std::string& sql,
                          std::shared_ptr<PreparedStatementHandle>* outStatement,
                          std::string* error) override {
        if (!outStatement) {
            if (error) {
                *error = "No prepared statement handle provided";
            }
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
        if (!statement) {
            if (error) {
                *error = "Invalid prepared statement";
            }
            return false;
        }
        auto handle = std::dynamic_pointer_cast<ScratchbirdPreparedStatement>(statement);
        if (!handle) {
            if (error) {
                *error = "Prepared statement type mismatch";
            }
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

        size_t column_count = results.getColumnCount();
        for (size_t i = 0; i < column_count; ++i) {
            QueryColumn column;
            column.name = results.getColumnName(i);
            column.type = WireTypeToString(results.getColumnType(i));
            outResult->columns.push_back(std::move(column));
        }

        while (results.next()) {
            std::vector<QueryValue> out_row;
            out_row.reserve(column_count);
            for (size_t i = 0; i < column_count; ++i) {
                QueryValue cell;
                cell.isNull = results.isNull(i);
                if (!cell.isNull) {
                    size_t length = 0;
                    const uint8_t* raw = results.getRaw(i, &length);
                    if (raw && length > 0) {
                        cell.raw.assign(raw, raw + length);
                    }
                    cell.text = results.getString(i);
                } else {
                    cell.text = "NULL";
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
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    bool Unsubscribe(const std::string& channel, std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.unsubscribe(channel, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    bool FetchNotification(NotificationEvent* outEvent, std::string* error) override {
        if (!outEvent) {
            if (error) {
                *error = "No notification buffer provided";
            }
            return false;
        }
        scratchbird::client::Connection::Notification note;
        scratchbird::core::ErrorContext ctx;
        auto status = client_.receiveNotification(&note, &ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        outEvent->processId = note.processId;
        outEvent->channel = note.channel;
        outEvent->payload = note.payload;
        outEvent->changeType = note.changeType;
        outEvent->rowId = note.rowId;
        return true;
    }

    void SetProgressCallback(std::function<void(uint64_t, uint64_t)> callback) override {
        client_.setProgressCallback(std::move(callback));
    }

    bool BeginTransaction(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.beginTransaction(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    bool Commit(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.commit(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    bool Rollback(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.rollback(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    bool Cancel(std::string* error) override {
        scratchbird::core::ErrorContext ctx;
        auto status = client_.cancelQuery(&ctx);
        if (status != scratchbird::core::Status::OK) {
            if (error) {
                *error = ctx.message.empty() ? client_.getLastError() : ctx.message;
            }
            return false;
        }
        return true;
    }

    BackendCapabilities Capabilities() const override {
        BackendCapabilities caps;
        caps.supportsCancel = true;
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsExplain = true;
        caps.supportsSblr = true;
        caps.supportsDdlExtract = true;
        caps.supportsDependencies = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        caps.supportsPreparedStatements = true;
        caps.supportsStatementCache = true;
        caps.supportsCopyIn = true;
        caps.supportsCopyOut = true;
        caps.supportsCopyBoth = true;
        caps.supportsCopyBinary = true;
        caps.supportsCopyText = true;
        caps.supportsNotifications = true;
        caps.supportsStatus = true;
        return caps;
    }

    std::string BackendName() const override {
        return "network";
    }

private:
    scratchbird::client::Connection client_;
};

} // namespace
#endif

std::unique_ptr<ConnectionBackend> CreateNetworkBackend() {
#ifdef SCRATCHROBIN_USE_SCRATCHBIRD
    return std::make_unique<NetworkBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
