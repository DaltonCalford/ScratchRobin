/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "mysql_backend.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#ifdef SCRATCHROBIN_USE_MYSQL
#if defined(__has_include)
#  if __has_include(<mysql/mysql.h>)
#    include <mysql/mysql.h>
#  elif __has_include(<mysql.h>)
#    include <mysql.h>
#  else
#    error "MySQL client headers not found"
#  endif
#else
#  include <mysql.h>
#endif
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_MYSQL
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string BytesToHex(const unsigned char* data, size_t size) {
    std::ostringstream out;
    out << "0x";
    for (size_t i = 0; i < size; ++i) {
        out << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return out.str();
}

std::string MySqlTypeToString(enum_field_types type) {
    switch (type) {
        case MYSQL_TYPE_TINY: return "INT8";
        case MYSQL_TYPE_SHORT: return "INT16";
        case MYSQL_TYPE_LONG: return "INT32";
        case MYSQL_TYPE_LONGLONG: return "INT64";
        case MYSQL_TYPE_FLOAT: return "FLOAT32";
        case MYSQL_TYPE_DOUBLE: return "FLOAT64";
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL: return "DECIMAL";
        case MYSQL_TYPE_STRING: return "CHAR";
        case MYSQL_TYPE_VAR_STRING: return "VARCHAR";
        case MYSQL_TYPE_VARCHAR: return "VARCHAR";
        case MYSQL_TYPE_DATE: return "DATE";
        case MYSQL_TYPE_TIME: return "TIME";
        case MYSQL_TYPE_DATETIME: return "DATETIME";
        case MYSQL_TYPE_TIMESTAMP: return "TIMESTAMP";
        case MYSQL_TYPE_YEAR: return "YEAR";
        case MYSQL_TYPE_JSON: return "JSON";
        case MYSQL_TYPE_BIT: return "BIT";
        case MYSQL_TYPE_ENUM: return "ENUM";
        case MYSQL_TYPE_SET: return "SET";
        case MYSQL_TYPE_GEOMETRY: return "GEOMETRY";
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB: return "BLOB";
        default:
            break;
    }
    return "UNKNOWN";
}

bool IsBinaryType(enum_field_types type) {
    switch (type) {
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_GEOMETRY:
            return true;
        default:
            return false;
    }
}

std::string BuildCommandTag(const std::string& sql, int64_t rows_affected) {
    std::string trimmed = Trim(sql);
    if (trimmed.empty()) {
        return std::to_string(rows_affected);
    }
    std::string lower = ToLower(trimmed);
    size_t space = lower.find(' ');
    std::string keyword = space == std::string::npos ? lower : lower.substr(0, space);
    std::string tag = keyword;
    if (rows_affected > 0) {
        tag += " " + std::to_string(rows_affected);
    }
    return tag;
}

} // namespace

class MySqlBackend : public ConnectionBackend {
public:
    ~MySqlBackend() override {
        Disconnect();
    }

    bool Connect(const BackendConfig& config, std::string* error) override {
        Disconnect();
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            if (error) {
                *error = "mysql_init failed";
            }
            return false;
        }

        unsigned int timeout = config.connectTimeoutMs > 0
            ? static_cast<unsigned int>((config.connectTimeoutMs + 999) / 1000)
            : 0;
        if (timeout > 0) {
            mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
        }

        unsigned int read_timeout = config.readTimeoutMs > 0
            ? static_cast<unsigned int>((config.readTimeoutMs + 999) / 1000)
            : 0;
        if (read_timeout > 0) {
            mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &read_timeout);
        }

        unsigned int write_timeout = config.writeTimeoutMs > 0
            ? static_cast<unsigned int>((config.writeTimeoutMs + 999) / 1000)
            : 0;
        if (write_timeout > 0) {
            mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &write_timeout);
        }

#ifdef MYSQL_OPT_SSL_MODE
        if (!config.sslMode.empty()) {
            std::string mode = ToLower(config.sslMode);
            enum mysql_ssl_mode ssl_mode = SSL_MODE_PREFERRED;
            if (mode == "disable" || mode == "disabled" || mode == "off") {
                ssl_mode = SSL_MODE_DISABLED;
            } else if (mode == "require") {
                ssl_mode = SSL_MODE_REQUIRED;
            } else if (mode == "verify_ca") {
                ssl_mode = SSL_MODE_VERIFY_CA;
            } else if (mode == "verify_full") {
                ssl_mode = SSL_MODE_VERIFY_IDENTITY;
            } else if (mode == "prefer") {
                ssl_mode = SSL_MODE_PREFERRED;
            }
            mysql_options(conn, MYSQL_OPT_SSL_MODE, &ssl_mode);
        }
#endif
#ifdef MYSQL_OPT_SSL_CA
        if (!config.sslRootCert.empty()) {
            mysql_options(conn, MYSQL_OPT_SSL_CA, config.sslRootCert.c_str());
        }
#endif
#ifdef MYSQL_OPT_SSL_CERT
        if (!config.sslCert.empty()) {
            mysql_options(conn, MYSQL_OPT_SSL_CERT, config.sslCert.c_str());
        }
#endif
#ifdef MYSQL_OPT_SSL_KEY
        if (!config.sslKey.empty()) {
            mysql_options(conn, MYSQL_OPT_SSL_KEY, config.sslKey.c_str());
        }
#endif

        const char* host = config.host.empty() ? nullptr : config.host.c_str();
        const char* user = config.username.empty() ? nullptr : config.username.c_str();
        const char* pass = config.password.empty() ? nullptr : config.password.c_str();
        const char* db = config.database.empty() ? nullptr : config.database.c_str();
        unsigned int port = config.port > 0 ? static_cast<unsigned int>(config.port) : 0;

        if (!mysql_real_connect(conn, host, user, pass, db, port, nullptr, 0)) {
            if (error) {
                *error = mysql_error(conn);
            }
            mysql_close(conn);
            return false;
        }

        conn_ = conn;
        return true;
    }

    void Disconnect() override {
        if (conn_) {
            mysql_close(conn_);
            conn_ = nullptr;
        }
    }

    bool IsConnected() const override {
        return conn_ != nullptr;
    }

    bool ExecuteQuery(const std::string& sql, QueryResult* outResult,
                      std::string* error) override {
        if (!outResult) {
            if (error) {
                *error = "No result buffer provided";
            }
            return false;
        }
        if (!IsConnected()) {
            if (error) {
                *error = "Not connected";
            }
            return false;
        }

        if (mysql_real_query(conn_, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0) {
            if (error) {
                *error = mysql_error(conn_);
            }
            return false;
        }

        MYSQL_RES* result = mysql_store_result(conn_);
        if (!result) {
            if (mysql_field_count(conn_) != 0) {
                if (error) {
                    *error = mysql_error(conn_);
                }
                return false;
            }
        }

        outResult->columns.clear();
        outResult->rows.clear();
        outResult->messages.clear();
        outResult->errorStack.clear();
        outResult->stats = QueryStats{};
        outResult->rowsAffected = static_cast<int64_t>(mysql_affected_rows(conn_));
        outResult->commandTag = BuildCommandTag(sql, outResult->rowsAffected);

        if (!result) {
            return true;
        }

        int field_count = mysql_num_fields(result);
        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        for (int i = 0; i < field_count; ++i) {
            QueryColumn column;
            column.name = fields[i].name ? fields[i].name : "";
            column.type = MySqlTypeToString(fields[i].type);
            outResult->columns.push_back(std::move(column));
        }

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != nullptr) {
            unsigned long* lengths = mysql_fetch_lengths(result);
            std::vector<QueryValue> out_row;
            out_row.reserve(static_cast<size_t>(field_count));
            for (int col = 0; col < field_count; ++col) {
                QueryValue cell;
                if (!row[col]) {
                    cell.isNull = true;
                    cell.text = "NULL";
                    out_row.push_back(std::move(cell));
                    continue;
                }
                cell.isNull = false;
                unsigned long len = lengths ? lengths[col] : 0;
                if (IsBinaryType(fields[col].type)) {
                    cell.raw.assign(reinterpret_cast<unsigned char*>(row[col]),
                                    reinterpret_cast<unsigned char*>(row[col]) + len);
                    cell.text = BytesToHex(reinterpret_cast<unsigned char*>(row[col]), len);
                } else {
                    cell.text.assign(row[col], row[col] + len);
                }
                out_row.push_back(std::move(cell));
            }
            outResult->rows.push_back(std::move(out_row));
        }

        mysql_free_result(result);
        return true;
    }

    bool BeginTransaction(std::string* error) override {
        return ExecuteSimpleCommand("BEGIN", error);
    }

    bool Commit(std::string* error) override {
        return ExecuteSimpleCommand("COMMIT", error);
    }

    bool Rollback(std::string* error) override {
        return ExecuteSimpleCommand("ROLLBACK", error);
    }

    bool Cancel(std::string* error) override {
        if (error) {
            *error = "Cancel not supported for MySQL backend";
        }
        return false;
    }

    BackendCapabilities Capabilities() const override {
        BackendCapabilities caps;
        caps.supportsCancel = false;
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        return caps;
    }

    std::string BackendName() const override {
        return "mysql";
    }

private:
    bool ExecuteSimpleCommand(const char* sql, std::string* error) {
        if (!IsConnected()) {
            if (error) {
                *error = "Not connected";
            }
            return false;
        }
        if (mysql_real_query(conn_, sql, static_cast<unsigned long>(std::strlen(sql))) != 0) {
            if (error) {
                *error = mysql_error(conn_);
            }
            return false;
        }
        MYSQL_RES* result = mysql_store_result(conn_);
        if (result) {
            mysql_free_result(result);
        }
        return true;
    }

    MYSQL* conn_ = nullptr;
};

#endif

std::unique_ptr<ConnectionBackend> CreateMySqlBackend() {
#ifdef SCRATCHROBIN_USE_MYSQL
    return std::make_unique<MySqlBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
