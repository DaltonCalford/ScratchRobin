#include "firebird_backend.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef SCRATCHROBIN_USE_FIREBIRD
#include <ibase.h>
#endif

namespace scratchrobin {

#ifdef SCRATCHROBIN_USE_FIREBIRD
namespace {

std::string TrimRight(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
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

std::string FormatScaledInteger(int64_t value, short scale) {
    if (scale >= 0) {
        return std::to_string(value);
    }
    bool negative = value < 0;
    uint64_t abs_value = static_cast<uint64_t>(negative ? -value : value);
    std::string digits = std::to_string(abs_value);
    int scale_abs = -scale;
    if (scale_abs >= static_cast<int>(digits.size())) {
        digits.insert(0, static_cast<size_t>(scale_abs - digits.size() + 1), '0');
    }
    size_t point = digits.size() - static_cast<size_t>(scale_abs);
    std::string out = digits.substr(0, point) + "." + digits.substr(point);
    if (negative) {
        out.insert(out.begin(), '-');
    }
    return out;
}

std::string FormatDate(const ISC_DATE& date) {
    std::tm tm{};
    isc_decode_sql_date(&date, &tm);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)
        << "-" << std::setw(2) << (tm.tm_mon + 1)
        << "-" << std::setw(2) << tm.tm_mday;
    return out.str();
}

std::string FormatTime(const ISC_TIME& time) {
    std::tm tm{};
    isc_decode_sql_time(&time, &tm);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << tm.tm_hour
        << ":" << std::setw(2) << tm.tm_min
        << ":" << std::setw(2) << tm.tm_sec;
    return out.str();
}

std::string FormatTimestamp(const ISC_TIMESTAMP& ts) {
    std::tm tm{};
    isc_decode_timestamp(&ts, &tm);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)
        << "-" << std::setw(2) << (tm.tm_mon + 1)
        << "-" << std::setw(2) << tm.tm_mday
        << " " << std::setw(2) << tm.tm_hour
        << ":" << std::setw(2) << tm.tm_min
        << ":" << std::setw(2) << tm.tm_sec;
    return out.str();
}

std::string BuildCommandTag(const std::string& sql, int64_t rows_affected) {
    std::string trimmed = sql;
    trimmed.erase(trimmed.begin(),
                  std::find_if(trimmed.begin(), trimmed.end(),
                               [](unsigned char ch) { return !std::isspace(ch); }));
    trimmed = TrimRight(trimmed);
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

std::vector<char> BuildDpb(const BackendConfig& config) {
    std::vector<char> dpb;
    dpb.push_back(isc_dpb_version1);
    if (!config.username.empty()) {
        dpb.push_back(isc_dpb_user_name);
        unsigned char len = static_cast<unsigned char>(config.username.size());
        dpb.push_back(static_cast<char>(len));
        dpb.insert(dpb.end(), config.username.begin(), config.username.end());
    }
    if (!config.password.empty()) {
        dpb.push_back(isc_dpb_password);
        unsigned char len = static_cast<unsigned char>(config.password.size());
        dpb.push_back(static_cast<char>(len));
        dpb.insert(dpb.end(), config.password.begin(), config.password.end());
    }
    return dpb;
}

std::string BuildDatabasePath(const BackendConfig& config) {
    if (config.host.empty()) {
        return config.database;
    }
    std::ostringstream out;
    out << config.host;
    if (config.port > 0) {
        out << "/" << config.port;
    }
    out << ":" << config.database;
    return out.str();
}

std::string CollectStatus(ISC_STATUS* status) {
    std::ostringstream out;
    char buffer[512];
#ifdef fb_interpret
    const ISC_STATUS* ptr = status;
    while (fb_interpret(buffer, sizeof(buffer), &ptr) > 0) {
        if (!out.str().empty()) {
            out << "\n";
        }
        out << buffer;
    }
#else
    ISC_STATUS* ptr = status;
    while (isc_interprete(buffer, &ptr)) {
        if (!out.str().empty()) {
            out << "\n";
        }
        out << buffer;
    }
#endif
    std::string msg = out.str();
    return msg.empty() ? "Firebird error" : msg;
}

size_t BufferSizeForType(short sqltype, short sqllen) {
    switch (sqltype) {
        case SQL_TEXT:
            return static_cast<size_t>(sqllen);
        case SQL_VARYING:
            return sizeof(short) + static_cast<size_t>(sqllen);
        case SQL_SHORT:
            return sizeof(short);
        case SQL_LONG:
            return sizeof(long);
        case SQL_INT64:
            return sizeof(ISC_INT64);
        case SQL_FLOAT:
            return sizeof(float);
        case SQL_DOUBLE:
            return sizeof(double);
        case SQL_TYPE_DATE:
            return sizeof(ISC_DATE);
        case SQL_TYPE_TIME:
            return sizeof(ISC_TIME);
        case SQL_TIMESTAMP:
            return sizeof(ISC_TIMESTAMP);
        case SQL_BLOB:
        case SQL_ARRAY:
            return sizeof(ISC_QUAD);
#ifdef SQL_BOOLEAN
        case SQL_BOOLEAN:
#ifdef ISC_BOOLEAN
            return sizeof(ISC_BOOLEAN);
#else
            return sizeof(unsigned char);
#endif
#endif
        default:
            return static_cast<size_t>(sqllen > 0 ? sqllen : 1);
    }
}

int64_t ParseVaxInt(const unsigned char* data, size_t len) {
    int64_t value = 0;
    for (size_t i = 0; i < len; ++i) {
        value |= static_cast<int64_t>(data[i]) << (8 * i);
    }
    return value;
}

int64_t ExtractRowsAffected(isc_stmt_handle stmt, ISC_STATUS* status) {
    static const unsigned char items[] = { isc_info_sql_records, isc_info_end };
    unsigned char buffer[64];
    if (isc_dsql_sql_info(status, &stmt, sizeof(items),
                          reinterpret_cast<const ISC_SCHAR*>(items),
                          sizeof(buffer), reinterpret_cast<char*>(buffer)) != 0) {
        return 0;
    }
    const unsigned char* ptr = buffer;
    if (*ptr != isc_info_sql_records) {
        return 0;
    }
    ++ptr;
    unsigned short len = ptr[0] | (ptr[1] << 8);
    ptr += 2;
    const unsigned char* end = ptr + len;
    int64_t total = 0;
    while (ptr < end) {
        unsigned char item = *ptr++;
        unsigned short item_len = ptr[0] | (ptr[1] << 8);
        ptr += 2;
        int64_t value = ParseVaxInt(ptr, item_len);
        ptr += item_len;
        if (item == isc_info_req_insert_count ||
            item == isc_info_req_update_count ||
            item == isc_info_req_delete_count) {
            total += value;
        }
    }
    return total;
}

class SqlDaHolder {
public:
    explicit SqlDaHolder(int count) {
        data_.reset(reinterpret_cast<XSQLDA*>(std::malloc(XSQLDA_LENGTH(count))));
        if (data_) {
            std::memset(data_.get(), 0, XSQLDA_LENGTH(count));
            data_->version = SQLDA_VERSION1;
            data_->sqln = count;
        }
    }

    XSQLDA* get() const { return data_.get(); }

private:
    struct FreeDeleter {
        void operator()(XSQLDA* ptr) const { std::free(ptr); }
    };
    std::unique_ptr<XSQLDA, FreeDeleter> data_;
};

} // namespace

class FirebirdBackend : public ConnectionBackend {
public:
    ~FirebirdBackend() override {
        Disconnect();
    }

    bool Connect(const BackendConfig& config, std::string* error) override {
        Disconnect();
        ISC_STATUS status[20] = {};
        std::string db = BuildDatabasePath(config);
        std::vector<char> dpb = BuildDpb(config);

        if (isc_attach_database(status,
                                static_cast<short>(db.size()),
                                const_cast<char*>(db.c_str()),
                                &db_,
                                static_cast<short>(dpb.size()),
                                dpb.empty() ? nullptr : dpb.data()) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            db_ = 0;
            return false;
        }
        return true;
    }

    void Disconnect() override {
        ISC_STATUS status[20] = {};
        if (tr_) {
            isc_rollback_transaction(status, &tr_);
            tr_ = 0;
        }
        if (db_) {
            isc_detach_database(status, &db_);
            db_ = 0;
        }
    }

    bool IsConnected() const override {
        return db_ != 0;
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

        bool local_transaction = false;
        if (!tr_) {
            if (!StartTransaction(error)) {
                return false;
            }
            local_transaction = true;
        }

        ISC_STATUS status[20] = {};
        isc_stmt_handle stmt = 0;
        if (isc_dsql_allocate_statement(status, &db_, &stmt) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            if (local_transaction) {
                RollbackTransaction(nullptr);
            }
            return false;
        }

        if (isc_dsql_prepare(status, &tr_, &stmt, 0,
                             const_cast<char*>(sql.c_str()),
                             SQL_DIALECT_V6, nullptr) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            isc_dsql_free_statement(status, &stmt, DSQL_drop);
            if (local_transaction) {
                RollbackTransaction(nullptr);
            }
            return false;
        }

        SqlDaHolder da(1);
        if (!da.get()) {
            if (error) {
                *error = "Failed to allocate SQLDA";
            }
            isc_dsql_free_statement(status, &stmt, DSQL_drop);
            if (local_transaction) {
                RollbackTransaction(nullptr);
            }
            return false;
        }

        if (isc_dsql_describe(status, &stmt, SQL_DIALECT_V6, da.get()) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            isc_dsql_free_statement(status, &stmt, DSQL_drop);
            if (local_transaction) {
                RollbackTransaction(nullptr);
            }
            return false;
        }

        if (da.get()->sqld > da.get()->sqln) {
            SqlDaHolder bigger(da.get()->sqld);
            if (!bigger.get()) {
                if (error) {
                    *error = "Failed to allocate SQLDA";
                }
                isc_dsql_free_statement(status, &stmt, DSQL_drop);
                if (local_transaction) {
                    RollbackTransaction(nullptr);
                }
                return false;
            }
            if (isc_dsql_describe(status, &stmt, SQL_DIALECT_V6, bigger.get()) != 0) {
                if (error) {
                    *error = CollectStatus(status);
                }
                isc_dsql_free_statement(status, &stmt, DSQL_drop);
                if (local_transaction) {
                    RollbackTransaction(nullptr);
                }
                return false;
            }
            da = std::move(bigger);
        }

        std::vector<std::unique_ptr<char[]>> buffers;
        std::vector<std::unique_ptr<short>> indicators;
        buffers.reserve(static_cast<size_t>(da.get()->sqld));
        indicators.reserve(static_cast<size_t>(da.get()->sqld));
        for (int i = 0; i < da.get()->sqld; ++i) {
            auto& var = da.get()->sqlvar[i];
            short sqltype = var.sqltype & ~1;
            size_t buffer_size = BufferSizeForType(sqltype, var.sqllen);
            auto buffer = std::make_unique<char[]>(buffer_size);
            auto indicator = std::make_unique<short>(0);
            var.sqldata = buffer.get();
            var.sqlind = indicator.get();
            buffers.push_back(std::move(buffer));
            indicators.push_back(std::move(indicator));
        }

        if (isc_dsql_execute(status, &tr_, &stmt, SQL_DIALECT_V6, nullptr) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            isc_dsql_free_statement(status, &stmt, DSQL_drop);
            if (local_transaction) {
                RollbackTransaction(nullptr);
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

        for (int i = 0; i < da.get()->sqld; ++i) {
            QueryColumn column;
            auto& var = da.get()->sqlvar[i];
            if (var.aliasname_length > 0) {
                column.name.assign(var.aliasname, var.aliasname_length);
            } else if (var.sqlname_length > 0) {
                column.name.assign(var.sqlname, var.sqlname_length);
            } else {
                column.name = "COL" + std::to_string(i + 1);
            }

            short sqltype = var.sqltype & ~1;
            switch (sqltype) {
                case SQL_TEXT: column.type = "CHAR"; break;
                case SQL_VARYING: column.type = "VARCHAR"; break;
                case SQL_SHORT:
                case SQL_LONG:
                case SQL_INT64: column.type = "INT"; break;
                case SQL_FLOAT: column.type = "FLOAT32"; break;
                case SQL_DOUBLE: column.type = "FLOAT64"; break;
                case SQL_TYPE_DATE: column.type = "DATE"; break;
                case SQL_TYPE_TIME: column.type = "TIME"; break;
                case SQL_TIMESTAMP: column.type = "TIMESTAMP"; break;
#ifdef SQL_BOOLEAN
                case SQL_BOOLEAN: column.type = "BOOLEAN"; break;
#endif
                case SQL_BLOB: column.type = "BLOB"; break;
                case SQL_ARRAY: column.type = "ARRAY"; break;
                default: column.type = "UNKNOWN"; break;
            }
            outResult->columns.push_back(std::move(column));
        }

        if (da.get()->sqld == 0) {
            outResult->rowsAffected = ExtractRowsAffected(stmt, status);
            outResult->commandTag = BuildCommandTag(sql, outResult->rowsAffected);
            isc_dsql_free_statement(status, &stmt, DSQL_drop);
            if (local_transaction) {
                CommitTransaction(nullptr);
            }
            return true;
        }

        while (isc_dsql_fetch(status, &stmt, SQL_DIALECT_V6, da.get()) == 0) {
            std::vector<QueryValue> out_row;
            out_row.reserve(static_cast<size_t>(da.get()->sqld));
            for (int i = 0; i < da.get()->sqld; ++i) {
                auto& var = da.get()->sqlvar[i];
                QueryValue cell;
                if (var.sqlind && *var.sqlind < 0) {
                    cell.isNull = true;
                    cell.text = "NULL";
                    out_row.push_back(std::move(cell));
                    continue;
                }

                short sqltype = var.sqltype & ~1;
                cell.isNull = false;
                switch (sqltype) {
                    case SQL_TEXT: {
                        std::string text(var.sqldata, var.sqldata + var.sqllen);
                        cell.text = TrimRight(text);
                        break;
                    }
                    case SQL_VARYING: {
                        auto* varying = reinterpret_cast<PARAMVARY*>(var.sqldata);
                        short len = varying->vary_length;
                        cell.text.assign(reinterpret_cast<char*>(varying->vary_string),
                                         reinterpret_cast<char*>(varying->vary_string) + len);
                        break;
                    }
                    case SQL_SHORT: {
                        short value = 0;
                        std::memcpy(&value, var.sqldata, sizeof(short));
                        if (var.sqlscale < 0) {
                            cell.text = FormatScaledInteger(value, var.sqlscale);
                        } else {
                            cell.text = std::to_string(value);
                        }
                        break;
                    }
                    case SQL_LONG: {
                        int32_t value = 0;
                        std::memcpy(&value, var.sqldata, sizeof(int32_t));
                        if (var.sqlscale < 0) {
                            cell.text = FormatScaledInteger(value, var.sqlscale);
                        } else {
                            cell.text = std::to_string(value);
                        }
                        break;
                    }
                    case SQL_INT64: {
                        ISC_INT64 value = 0;
                        std::memcpy(&value, var.sqldata, sizeof(ISC_INT64));
                        if (var.sqlscale < 0) {
                            cell.text = FormatScaledInteger(value, var.sqlscale);
                        } else {
                            cell.text = std::to_string(value);
                        }
                        break;
                    }
                    case SQL_FLOAT: {
                        float value = 0.0f;
                        std::memcpy(&value, var.sqldata, sizeof(float));
                        cell.text = std::to_string(value);
                        break;
                    }
                    case SQL_DOUBLE: {
                        double value = 0.0;
                        std::memcpy(&value, var.sqldata, sizeof(double));
                        cell.text = std::to_string(value);
                        break;
                    }
                    case SQL_TYPE_DATE: {
                        auto* date = reinterpret_cast<ISC_DATE*>(var.sqldata);
                        cell.text = FormatDate(*date);
                        break;
                    }
                    case SQL_TYPE_TIME: {
                        auto* time = reinterpret_cast<ISC_TIME*>(var.sqldata);
                        cell.text = FormatTime(*time);
                        break;
                    }
                    case SQL_TIMESTAMP: {
                        auto* ts = reinterpret_cast<ISC_TIMESTAMP*>(var.sqldata);
                        cell.text = FormatTimestamp(*ts);
                        break;
                    }
#ifdef SQL_BOOLEAN
                    case SQL_BOOLEAN: {
#ifdef ISC_BOOLEAN
                        ISC_BOOLEAN value = 0;
                        std::memcpy(&value, var.sqldata, sizeof(ISC_BOOLEAN));
                        cell.text = value ? "true" : "false";
#else
                        unsigned char value = 0;
                        std::memcpy(&value, var.sqldata, sizeof(unsigned char));
                        cell.text = value ? "true" : "false";
#endif
                        break;
                    }
#endif
                    case SQL_BLOB: {
                        cell.text = "<blob>";
                        break;
                    }
                    case SQL_ARRAY: {
                        cell.text = "<array>";
                        break;
                    }
                    default: {
                        cell.text = BytesToHex(reinterpret_cast<unsigned char*>(var.sqldata),
                                               BufferSizeForType(sqltype, var.sqllen));
                        break;
                    }
                }
                out_row.push_back(std::move(cell));
            }
            outResult->rows.push_back(std::move(out_row));
        }

        outResult->rowsAffected = ExtractRowsAffected(stmt, status);
        outResult->commandTag = BuildCommandTag(sql, outResult->rowsAffected);

        isc_dsql_free_statement(status, &stmt, DSQL_drop);
        if (local_transaction) {
            CommitTransaction(nullptr);
        }
        return true;
    }

    bool BeginTransaction(std::string* error) override {
        if (tr_) {
            return true;
        }
        return StartTransaction(error);
    }

    bool Commit(std::string* error) override {
        return CommitTransaction(error);
    }

    bool Rollback(std::string* error) override {
        return RollbackTransaction(error);
    }

    bool Cancel(std::string* error) override {
        if (error) {
            *error = "Cancel not supported for Firebird backend";
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
        return "firebird";
    }

private:
    bool StartTransaction(std::string* error) {
        ISC_STATUS status[20] = {};
        if (isc_start_transaction(status, &tr_, 1, &db_, 0, nullptr) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            tr_ = 0;
            return false;
        }
        return true;
    }

    bool CommitTransaction(std::string* error) {
        if (!tr_) {
            return true;
        }
        ISC_STATUS status[20] = {};
        if (isc_commit_transaction(status, &tr_) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            tr_ = 0;
            return false;
        }
        tr_ = 0;
        return true;
    }

    bool RollbackTransaction(std::string* error) {
        if (!tr_) {
            return true;
        }
        ISC_STATUS status[20] = {};
        if (isc_rollback_transaction(status, &tr_) != 0) {
            if (error) {
                *error = CollectStatus(status);
            }
            tr_ = 0;
            return false;
        }
        tr_ = 0;
        return true;
    }

    isc_db_handle db_ = 0;
    isc_tr_handle tr_ = 0;
};

#endif

std::unique_ptr<ConnectionBackend> CreateFirebirdBackend() {
#ifdef SCRATCHROBIN_USE_FIREBIRD
    return std::make_unique<FirebirdBackend>();
#else
    return nullptr;
#endif
}

} // namespace scratchrobin
