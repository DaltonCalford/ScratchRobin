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

std::string BytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    out << "0x";
    for (uint8_t byte : data) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return out.str();
}

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
        if (error) {
            *error = "Cancel not supported by network backend";
        }
        return false;
    }

    BackendCapabilities Capabilities() const override {
        BackendCapabilities caps;
        caps.supportsCancel = false;
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsExplain = true;
        caps.supportsSblr = true;
        caps.supportsDdlExtract = true;
        caps.supportsDependencies = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
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
