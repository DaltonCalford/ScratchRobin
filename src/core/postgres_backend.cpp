#include "postgres_backend.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <sstream>

#ifdef SCRATCHROBIN_USE_LIBPQ
#include <libpq-fe.h>
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_LIBPQ
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
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

std::string BuildConnInfo(const BackendConfig& config) {
    std::ostringstream out;
    if (!config.host.empty()) {
        out << "host=" << config.host << " ";
    }
    if (config.port > 0) {
        out << "port=" << config.port << " ";
    }
    if (!config.database.empty()) {
        out << "dbname=" << config.database << " ";
    }
    if (!config.username.empty()) {
        out << "user=" << config.username << " ";
    }
    if (!config.password.empty()) {
        out << "password=" << config.password << " ";
    }
    if (!config.sslMode.empty()) {
        out << "sslmode=" << config.sslMode << " ";
    }
    if (config.connectTimeoutMs > 0) {
        out << "connect_timeout=" << (config.connectTimeoutMs / 1000) << " ";
    }
    out << "application_name=scratchrobin";
    return Trim(out.str());
}

std::string OidToTypeName(unsigned int oid) {
    switch (oid) {
        case 16: return "BOOLEAN";
        case 17: return "BYTEA";
        case 18: return "CHAR";
        case 19: return "NAME";
        case 20: return "INT8";
        case 21: return "INT2";
        case 22: return "INT2VECTOR";
        case 23: return "INT4";
        case 24: return "REGPROC";
        case 25: return "TEXT";
        case 26: return "OID";
        case 114: return "JSON";
        case 142: return "XML";
        case 700: return "FLOAT4";
        case 701: return "FLOAT8";
        case 790: return "MONEY";
        case 1042: return "CHAR";
        case 1043: return "VARCHAR";
        case 1082: return "DATE";
        case 1083: return "TIME";
        case 1114: return "TIMESTAMP";
        case 1184: return "TIMESTAMPTZ";
        case 1186: return "INTERVAL";
        case 1266: return "TIMETZ";
        case 1700: return "NUMERIC";
        case 2950: return "UUID";
        case 3802: return "JSONB";
        default:
            break;
    }
    return "OID:" + std::to_string(oid);
}

class ResultHolder {
public:
    explicit ResultHolder(PGresult* result) : result_(result) {}
    ~ResultHolder() { if (result_) { PQclear(result_); } }
    PGresult* get() const { return result_; }
    PGresult* release() {
        PGresult* tmp = result_;
        result_ = nullptr;
        return tmp;
    }

private:
    PGresult* result_ = nullptr;
};

} // namespace

class PostgresBackend : public ConnectionBackend {
public:
    ~PostgresBackend() override {
        Disconnect();
    }

    bool Connect(const BackendConfig& config, std::string* error) override {
        Disconnect();
        std::string conninfo = BuildConnInfo(config);
        PGconn* conn = PQconnectdb(conninfo.c_str());
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            if (error) {
                *error = conn ? PQerrorMessage(conn) : "libpq connection failed";
            }
            if (conn) {
                PQfinish(conn);
            }
            return false;
        }
        conn_ = conn;
        RefreshCancelHandle();
        return true;
    }

    void Disconnect() override {
        {
            std::lock_guard<std::mutex> lock(cancel_mutex_);
            if (cancel_) {
                PQfreeCancel(cancel_);
                cancel_ = nullptr;
            }
        }
        if (conn_) {
            PQfinish(conn_);
            conn_ = nullptr;
        }
    }

    bool IsConnected() const override {
        return conn_ && PQstatus(conn_) == CONNECTION_OK;
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

        ResultHolder result(PQexec(conn_, sql.c_str()));
        if (!result.get()) {
            if (error) {
                *error = "Query failed";
            }
            return false;
        }

        ExecStatusType status = PQresultStatus(result.get());
        if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
            if (error) {
                *error = PQresultErrorMessage(result.get());
            }
            return false;
        }

        outResult->columns.clear();
        outResult->rows.clear();
        outResult->messages.clear();
        outResult->errorStack.clear();
        outResult->stats = QueryStats{};
        outResult->rowsAffected = 0;
        outResult->commandTag.clear();

        int field_count = PQnfields(result.get());
        for (int i = 0; i < field_count; ++i) {
            QueryColumn column;
            column.name = PQfname(result.get(), i) ? PQfname(result.get(), i) : "";
            unsigned int oid = PQftype(result.get(), i);
            column.type = OidToTypeName(oid);
            outResult->columns.push_back(std::move(column));
        }

        int row_count = PQntuples(result.get());
        outResult->rows.reserve(static_cast<size_t>(row_count));
        for (int row = 0; row < row_count; ++row) {
            std::vector<QueryValue> out_row;
            out_row.reserve(static_cast<size_t>(field_count));
            for (int col = 0; col < field_count; ++col) {
                QueryValue cell;
                if (PQgetisnull(result.get(), row, col)) {
                    cell.isNull = true;
                    cell.text = "NULL";
                    out_row.push_back(std::move(cell));
                    continue;
                }
                cell.isNull = false;
                char* value = PQgetvalue(result.get(), row, col);
                if (!value) {
                    cell.text = "";
                    out_row.push_back(std::move(cell));
                    continue;
                }
                unsigned int oid = PQftype(result.get(), col);
                if (oid == 17) { // bytea
                    size_t len = 0;
                    unsigned char* bytes = PQunescapeBytea(
                        reinterpret_cast<unsigned char*>(value), &len);
                    if (bytes && len > 0) {
                        cell.raw.assign(bytes, bytes + len);
                        cell.text = BytesToHex(bytes, len);
                    } else {
                        cell.text = value;
                    }
                    if (bytes) {
                        PQfreemem(bytes);
                    }
                } else {
                    cell.text = value;
                }
                out_row.push_back(std::move(cell));
            }
            outResult->rows.push_back(std::move(out_row));
        }

        const char* tag = PQcmdStatus(result.get());
        if (tag) {
            outResult->commandTag = tag;
        }

        if (status == PGRES_COMMAND_OK) {
            const char* tuple_str = PQcmdTuples(result.get());
            if (tuple_str && *tuple_str) {
                outResult->rowsAffected = std::strtoll(tuple_str, nullptr, 10);
            }
        }

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
        PGcancel* cancel_handle = nullptr;
        {
            std::lock_guard<std::mutex> lock(cancel_mutex_);
            cancel_handle = cancel_;
        }
        if (!cancel_handle) {
            if (error) {
                *error = "Cancel handle not available";
            }
            return false;
        }
        char errbuf[256];
        errbuf[0] = '\0';
        if (PQcancel(cancel_handle, errbuf, sizeof(errbuf)) == 0) {
            if (error) {
                *error = errbuf[0] ? errbuf : "Cancel request failed";
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
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        return caps;
    }

    std::string BackendName() const override {
        return "postgresql";
    }

private:
    void RefreshCancelHandle() {
        std::lock_guard<std::mutex> lock(cancel_mutex_);
        if (cancel_) {
            PQfreeCancel(cancel_);
            cancel_ = nullptr;
        }
        if (conn_) {
            cancel_ = PQgetCancel(conn_);
        }
    }

    bool ExecuteSimpleCommand(const char* sql, std::string* error) {
        if (!IsConnected()) {
            if (error) {
                *error = "Not connected";
            }
            return false;
        }
        ResultHolder result(PQexec(conn_, sql));
        if (!result.get() || PQresultStatus(result.get()) != PGRES_COMMAND_OK) {
            if (error) {
                *error = result.get() ? PQresultErrorMessage(result.get()) : "Command failed";
            }
            return false;
        }
        return true;
    }

    PGconn* conn_ = nullptr;
    PGcancel* cancel_ = nullptr;
    mutable std::mutex cancel_mutex_;
};

#endif

std::unique_ptr<ConnectionBackend> CreatePostgresBackend() {
#ifdef SCRATCHROBIN_USE_LIBPQ
    return std::make_unique<PostgresBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
