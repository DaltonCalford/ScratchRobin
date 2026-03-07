#include "backend/scratchbird_connection.h"

#include <sstream>
#include <cstring>

namespace scratchrobin::backend {

ScratchbirdConnection::ScratchbirdConnection() = default;

ScratchbirdConnection::~ScratchbirdConnection() {
    disconnect();
}

void ScratchbirdConnection::clearError() {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    last_error_.code = SB_OK;
#else
    last_error_.code = 0;
#endif
    last_error_.message[0] = '\0';
}

bool ScratchbirdConnection::connect(const ConnectionInfo& info) {
    disconnect();
    clearError();
    
#if SCRATCHBIRD_CLIENT_AVAILABLE
    // Build connection string: "host=localhost port=3050 dbname=test user=... password=..."
    std::ostringstream conn_str;
    conn_str << "host=" << info.host;
    conn_str << " port=" << info.port;
    if (!info.database.empty()) {
        conn_str << " dbname=" << info.database;
    }
    if (!info.username.empty()) {
        conn_str << " user=" << info.username;
    }
    if (!info.password.empty()) {
        conn_str << " password=" << info.password;
    }
    if (info.use_ssl) {
        conn_str << " sslmode=require";
    }
    conn_str << " connect_timeout=" << (info.timeout_ms / 1000);
    
    conn_ = sb_connect(conn_str.str().c_str(), &last_error_);
    if (!conn_) {
        return false;
    }
    
    connected_ = true;
    current_info_ = info;
    return true;
#else
    // Mock implementation for development
    connected_ = true;
    current_info_ = info;
    return true;
#endif
}

void ScratchbirdConnection::disconnect() {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    if (conn_) {
        sb_disconnect(conn_);
        conn_ = nullptr;
    }
#endif
    connected_ = false;
}

bool ScratchbirdConnection::isConnected() const {
    return connected_;
}

bool ScratchbirdConnection::ping() {
    if (!connected_) {
        return false;
    }
    
#if SCRATCHBIRD_CLIENT_AVAILABLE
    clearError();
    return sb_ping(conn_, &last_error_) == 0;
#else
    return true;
#endif
}

QueryResult ScratchbirdConnection::execute(const std::string& sql) {
    QueryResult result;
    
    if (!connected_) {
        result.error_message = "Not connected to database";
        return result;
    }
    
    clearError();
    
#if SCRATCHBIRD_CLIENT_AVAILABLE
    sb_result* sb_res = sb_execute(conn_, sql.c_str(), &last_error_);
    if (!sb_res) {
        result.error_message = last_error_.message;
        return result;
    }
    
    result = resultFromSbResult(sb_res);
    sb_result_free(sb_res);
#else
    // Mock implementation - return sample data for SELECT statements
    if (sql.find("SELECT") != std::string::npos || sql.find("select") != std::string::npos) {
        result.success = true;
        result.columns = {
            {"ID", 3, 0},
            {"Name", 11, 1},
            {"Email", 11, 1},
            {"Created", 32, 1}
        };
        result.rows = {
            {"1", "John Doe", "john@example.com", "2024-01-15"},
            {"2", "Jane Smith", "jane@example.com", "2024-02-20"},
            {"3", "Bob Johnson", "bob@example.com", "2024-03-10"},
            {"4", "Alice Brown", "alice@example.com", "2024-04-05"},
            {"5", "Charlie Wilson", "charlie@example.com", "2024-05-12"}
        };
    } else {
        result.success = true;
        result.affected_rows = 1;
    }
#endif
    
    return result;
}

QueryResult ScratchbirdConnection::query(const std::string& sql) {
    return execute(sql);
}

QueryResult ScratchbirdConnection::getSchemas() {
    // Use the inline query from scratchbird_client.h
    return query(sb_metadata_schemas_query());
}

QueryResult ScratchbirdConnection::getTables(const std::string& schema) {
    (void)schema;  // Schema filtering not yet implemented in C API
    return query(sb_metadata_tables_query());
}

QueryResult ScratchbirdConnection::getColumns(const std::string& table, const std::string& schema) {
    (void)table;
    (void)schema;
    return query(sb_metadata_columns_query());
}

QueryResult ScratchbirdConnection::getIndexes(const std::string& table, const std::string& schema) {
    (void)table;
    (void)schema;
    return query(sb_metadata_indexes_query());
}

bool ScratchbirdConnection::beginTransaction() {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    if (!connected_) return false;
    clearError();
    return sb_tx_begin(conn_, &last_error_) == 0;
#else
    return true;
#endif
}

bool ScratchbirdConnection::commit() {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    if (!connected_) return false;
    clearError();
    return sb_tx_commit(conn_, &last_error_) == 0;
#else
    return true;
#endif
}

bool ScratchbirdConnection::rollback() {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    if (!connected_) return false;
    clearError();
    return sb_tx_rollback(conn_, &last_error_) == 0;
#else
    return true;
#endif
}

std::string ScratchbirdConnection::lastError() const {
#if SCRATCHBIRD_CLIENT_AVAILABLE
    return last_error_.message;
#else
    return "";
#endif
}

#if SCRATCHBIRD_CLIENT_AVAILABLE
QueryResult ScratchbirdConnection::resultFromSbResult(sb_result* result) {
    QueryResult qr;
    if (!result) {
        qr.error_message = "Null result";
        return qr;
    }
    
    // Get column count and metadata
    int col_count = sb_column_count(result);
    for (int i = 0; i < col_count; ++i) {
        sb_column_meta meta;
        if (sb_get_column_meta(result, i, &meta) == 0) {
            ColumnMeta cm;
            cm.name = meta.name ? meta.name : "";
            cm.type = meta.type;
            cm.nullable = meta.nullable;
            qr.columns.push_back(cm);
        }
    }
    
    // Fetch rows
    sb_error err;
    sb_row row;
    while (sb_fetch(result, &row, &err) == 0) {
        std::vector<std::string> row_data;
        for (int i = 0; i < col_count; ++i) {
            size_t len = 0;
            const char* val = sb_get_string(&row, i, &len);
            if (val) {
                row_data.emplace_back(val, len);
            } else {
                row_data.push_back("NULL");
            }
        }
        qr.rows.push_back(row_data);
    }
    
    qr.success = true;
    return qr;
}
#else
QueryResult ScratchbirdConnection::resultFromSbResult(void* result) {
    (void)result;
    return QueryResult{};
}
#endif

}  // namespace scratchrobin::backend
