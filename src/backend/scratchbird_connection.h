#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

// ScratchBird C API
#if SCRATCHBIRD_CLIENT_AVAILABLE
extern "C" {
#include <scratchbird/client/scratchbird_client.h>
}
#else
// Mock types when library not available
typedef struct sb_connection sb_connection;
typedef struct sb_result sb_result;
typedef struct sb_error {
    int code;
    char message[256];
} sb_error;
#endif

namespace scratchrobin::backend {

struct ConnectionInfo {
    std::string host;
    int port = 3050;
    std::string database;
    std::string username;
    std::string password;
    bool use_ssl = false;
    int timeout_ms = 10000;
};

struct ColumnMeta {
    std::string name;
    int type = 0;
    int nullable = 1;
};

struct QueryResult {
    std::vector<ColumnMeta> columns;
    std::vector<std::vector<std::string>> rows;
    std::string error_message;
    bool success = false;
    int affected_rows = 0;
};

class ScratchbirdConnection {
 public:
    ScratchbirdConnection();
    ~ScratchbirdConnection();

    // Connection management
    bool connect(const ConnectionInfo& info);
    void disconnect();
    bool isConnected() const;
    bool ping();
    
    // Query execution
    QueryResult execute(const std::string& sql);
    QueryResult query(const std::string& sql);
    
    // Metadata queries
    QueryResult getSchemas();
    QueryResult getTables(const std::string& schema = "");
    QueryResult getColumns(const std::string& table, const std::string& schema = "");
    QueryResult getIndexes(const std::string& table, const std::string& schema = "");
    
    // Transactions
    bool beginTransaction();
    bool commit();
    bool rollback();
    
    // Status
    std::string lastError() const;
    
 private:
    sb_connection* conn_ = nullptr;
    sb_error last_error_{};
    bool connected_ = false;
    ConnectionInfo current_info_;
    
    QueryResult resultFromSbResult(sb_result* result);
    void clearError();
};

}  // namespace scratchrobin::backend
