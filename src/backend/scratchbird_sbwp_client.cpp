/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "backend/scratchbird_sbwp_client.h"

#include <cstdint>
#include <string>
#include <vector>

#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
#include <array>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "scratchbird/core/error_context.h"
#include "scratchbird/core/firebird_datetime.h"
#include "scratchbird/protocol/adapters/native_adapter.h"
#include "scratchbird/client/connection.h"
#include "scratchbird/server/ipc_server.h"
#endif

namespace scratchrobin::backend {

namespace {

#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
std::string BuildFailureMessage(const std::string& default_message,
                               const scratchbird::core::ErrorContext& context,
                               const std::string& fallback) {
  std::string message = default_message;
  const auto& reason = context.message.empty() ? fallback : context.message;
  if (!reason.empty()) {
    message += ": " + reason;
  }
  if (context.code != scratchbird::core::Status::OK) {
    message += " [status=" + std::to_string(static_cast<int>(context.code));
    if (context.sqlstate && *context.sqlstate != '\0') {
      message += ", sqlstate=" + std::string(context.sqlstate);
    }
    message += "]";
  }
  return message;
}

class EmbeddedProtocolAdapter : public scratchbird::protocol::NativeAdapter {
 public:
  using scratchbird::protocol::NativeAdapter::NativeAdapter;
  using scratchbird::protocol::ProtocolAdapter::executeQuery;
};

QueryResponse StatusFromContext(const std::string& path,
                               const std::string& default_message,
                               const scratchbird::core::ErrorContext& context,
                               const std::string& fallback,
                               const scratchbird::protocol::ResultContext& result_ctx,
                               scratchbird::core::Status status) {
  if (status != scratchbird::core::Status::OK) {
    return QueryResponse{core::Status::Error(BuildFailureMessage(default_message, context, fallback)),
                        {}, path};
  }

  if (result_ctx.has_error) {
    std::string message = result_ctx.error_message.empty()
                              ? default_message
                              : result_ctx.error_message;
    if (!result_ctx.sqlstate.empty() || result_ctx.error_code != 0) {
      message += " [";
      if (!result_ctx.sqlstate.empty()) {
        message += "sqlstate=" + result_ctx.sqlstate;
        if (result_ctx.error_code != 0) {
          message += " ";
        }
      }
      if (result_ctx.error_code != 0) {
        message += "status=" + std::to_string(result_ctx.error_code);
      }
      message += "]";
    }
    return QueryResponse{core::Status::Error(message), {}, path};
  }

  return {core::Status::Ok(), {}, path};
}

std::string HexFromBytes(const std::vector<std::uint8_t>& bytes) {
  static const char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(bytes.size() * 2);
  for (std::uint8_t byte : bytes) {
    out.push_back(kHex[(byte >> 4) & 0x0F]);
    out.push_back(kHex[byte & 0x0F]);
  }
  return out;
}

std::string FormatTimeFromMicros(std::int64_t micros) {
  constexpr std::int64_t kMicrosPerDay = 24LL * 60LL * 60LL * 1000000LL;
  std::int64_t normalized = micros % kMicrosPerDay;
  if (normalized < 0) {
    normalized += kMicrosPerDay;
  }

  const std::int64_t total_seconds = normalized / 1000000;
  const std::int64_t micro_remainder = normalized % 1000000;
  const int hour = static_cast<int>(total_seconds / 3600);
  const int minute = static_cast<int>((total_seconds / 60) % 60);
  const int second = static_cast<int>(total_seconds % 60);

  std::ostringstream out;
  out << std::setfill('0') << std::setw(2) << hour << ":" << std::setw(2) << minute << ":"
      << std::setw(2) << second;
  if (micro_remainder != 0) {
    out << "." << std::setfill('0') << std::setw(6) << micro_remainder;
  }
  return out.str();
}

std::string FormatDateFromDaysSince2000(std::int32_t days_since_2000) {
  const int32_t base_mjd = scratchbird::core::FirebirdDateTime::dateToMJD(2000, 1, 1);
  return scratchbird::core::FirebirdDateTime::formatDate(base_mjd + days_since_2000);
}

std::string FormatTimestampFromMicros(std::int64_t micros) {
  auto floor_div = [](std::int64_t value, std::int64_t divisor) {
    if (divisor == 0) {
      return std::int64_t{0};
    }
    std::int64_t quotient = value / divisor;
    const std::int64_t remainder = value % divisor;
    if ((remainder != 0) && ((remainder < 0) != (divisor < 0))) {
      --quotient;
    }
    return quotient;
  };

  std::int64_t seconds = floor_div(micros, 1000000);
  std::int64_t micro_remainder = micros - seconds * 1000000;
  if (micro_remainder < 0) {
    micro_remainder += 1000000;
    --seconds;
  }

  const std::int64_t days = floor_div(seconds, 86400);
  std::int64_t seconds_of_day = seconds - days * 86400;
  if (seconds_of_day < 0) {
    seconds_of_day += 86400;
  }

  const int hour = static_cast<int>(seconds_of_day / 3600);
  const int minute = static_cast<int>((seconds_of_day / 60) % 60);
  const int second = static_cast<int>(seconds_of_day % 60);
  const std::int32_t mjd = static_cast<std::int32_t>(
      days + scratchbird::core::FirebirdDateTime::UNIX_EPOCH_MJD);

  std::ostringstream out;
  out << scratchbird::core::FirebirdDateTime::formatDate(mjd) << " " << std::setfill('0')
      << std::setw(2) << hour << ":" << std::setw(2) << minute << ":"
      << std::setw(2) << second;
  if (micro_remainder != 0) {
    out << "." << std::setfill('0') << std::setw(6) << micro_remainder;
  }
  return out.str();
}

std::string ColumnValueToString(const scratchbird::protocol::ProtocolCodec::ColumnValue& value,
                               scratchbird::protocol::WireType type) {
  if (value.is_null) {
    return "NULL";
  }

  const auto& bytes = value.data;

  switch (type) {
    case scratchbird::protocol::WireType::BOOLEAN:
      if (bytes.empty()) {
        return "false";
      }
      return bytes[0] ? "true" : "false";

    case scratchbird::protocol::WireType::INT16: {
      if (bytes.size() < sizeof(std::int16_t)) {
        return "0";
      }
      std::int16_t value16;
      std::memcpy(&value16, bytes.data(), sizeof(value16));
      return std::to_string(value16);
    }

    case scratchbird::protocol::WireType::INT32:
    case scratchbird::protocol::WireType::DATE: {
      if (bytes.size() < sizeof(std::int32_t)) {
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      }
      if (type == scratchbird::protocol::WireType::DATE) {
        std::int32_t days;
        std::memcpy(&days, bytes.data(), sizeof(days));
        return FormatDateFromDaysSince2000(days);
      }
      std::int32_t value32;
      std::memcpy(&value32, bytes.data(), sizeof(value32));
      return std::to_string(value32);
    }

    case scratchbird::protocol::WireType::INT64:
    case scratchbird::protocol::WireType::TIME:
    case scratchbird::protocol::WireType::TIMESTAMP:
    case scratchbird::protocol::WireType::TIMESTAMPTZ: {
      if (bytes.size() < sizeof(std::int64_t)) {
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      }
      if (type == scratchbird::protocol::WireType::TIME) {
        std::int64_t micros;
        std::memcpy(&micros, bytes.data(), sizeof(micros));
        return FormatTimeFromMicros(micros);
      }
      if (type == scratchbird::protocol::WireType::TIMESTAMP ||
          type == scratchbird::protocol::WireType::TIMESTAMPTZ) {
        std::int64_t micros;
        std::memcpy(&micros, bytes.data(), sizeof(micros));
        return FormatTimestampFromMicros(micros);
      }
      std::int64_t value64;
      std::memcpy(&value64, bytes.data(), sizeof(value64));
      return std::to_string(value64);
    }

    case scratchbird::protocol::WireType::FLOAT32:
    case scratchbird::protocol::WireType::FLOAT64: {
      if (bytes.size() < sizeof(double)) {
        return "0";
      }
      double value_float;
      std::memcpy(&value_float, bytes.data(), sizeof(value_float));
      std::ostringstream out;
      out.precision(17);
      out << value_float;
      return out.str();
    }

    case scratchbird::protocol::WireType::UUID: {
      if (bytes.size() < 16) {
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      }
      char buffer[37];
      std::snprintf(
          buffer, sizeof(buffer),
          "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
          bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
          bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
      return std::string(buffer);
    }

    case scratchbird::protocol::WireType::BYTEA:
      return HexFromBytes(bytes);

    default:
      return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  }
}

std::string BuildEmbeddedDatabasePath(const ScratchbirdRuntimeConfig& config) {
  std::string database_path = config.database.empty() ? "default" : config.database;
  if (!database_path.empty() &&
      database_path.find_last_of('.') == std::string::npos) {
    database_path.append(".sbdb");
  }
  return database_path;
}

scratchbird::protocol::ProtocolAdapterConfig BuildEmbeddedAdapterConfig(
    const ScratchbirdRuntimeConfig& config) {
  scratchbird::protocol::ProtocolAdapterConfig adapter_config;
  adapter_config.read_timeout_ms = config.read_timeout_ms;
  adapter_config.write_timeout_ms = config.write_timeout_ms;
  adapter_config.idle_timeout_ms = config.auto_start_timeout_ms;
  adapter_config.default_database = config.database.empty() ? "default" : config.database;
  adapter_config.auto_create_db = true;
  adapter_config.database_path = BuildEmbeddedDatabasePath(config);
  return adapter_config;
}

QueryResponse ExecuteEmbeddedQuery(
    EmbeddedProtocolAdapter* adapter,
    const std::string& sql,
    bool collect_rows,
    const std::string& path) {
  if (!adapter) {
    return QueryResponse{core::Status::Error("embedded protocol adapter not initialized"), {}, path};
  }

  scratchbird::protocol::QueryContext query_ctx;
  query_ctx.query = sql;
  query_ctx.statement_name.clear();
  query_ctx.portal_name.clear();
  scratchbird::protocol::ResultContext result_ctx;
  auto status = adapter->executeQuery(query_ctx, result_ctx);
  if (status != scratchbird::core::Status::OK || result_ctx.has_error) {
    return StatusFromContext(
        path, "embedded execution failed", {}
        , {}
        , result_ctx, status);
  }

  QueryResponse response{core::Status::Ok(), {}, path};
  response.result_set.columns.reserve(result_ctx.columns.size());
  for (const auto& column : result_ctx.columns) {
    response.result_set.columns.push_back(column.name);
  }

  if (collect_rows) {
    response.result_set.rows.reserve(result_ctx.rows.size());
    for (const auto& row : result_ctx.rows) {
      std::vector<std::string> output_row;
      output_row.reserve(result_ctx.columns.size());
      for (std::size_t column = 0; column < result_ctx.columns.size(); ++column) {
        if (column < row.size()) {
          output_row.push_back(ColumnValueToString(row[column],
                                                  result_ctx.columns[column].type));
        } else {
          output_row.push_back("NULL");
        }
      }
      response.result_set.rows.push_back(std::move(output_row));
    }
  }

  if (!collect_rows || response.result_set.columns.empty() ||
      response.result_set.rows.empty()) {
    if (response.result_set.columns.empty()) {
      response.result_set.columns = {"command", "rows_affected"};
    }
    const std::string tag = result_ctx.command_tag.empty() ? "OK" : result_ctx.command_tag;
    response.result_set.rows.push_back({tag, std::to_string(result_ctx.rows_affected)});
  }

  return response;
}

scratchbird::client::ConnectionConfig BuildConnectionConfig(
    const ScratchbirdRuntimeConfig& config) {
  scratchbird::client::ConnectionConfig sb_config;
  sb_config.database_name = config.database.empty() ? "default" : config.database;
  sb_config.username = config.user;
  sb_config.password = config.password;
  sb_config.connect_timeout_ms = config.connect_timeout_ms;
  sb_config.read_timeout_ms = config.read_timeout_ms;
  sb_config.write_timeout_ms = config.write_timeout_ms;
  sb_config.query_timeout_ms = config.read_timeout_ms;

  sb_config.auto_start_timeout_ms = config.auto_start_timeout_ms;
  sb_config.server_executable = config.server_executable;

  switch (config.mode) {
    case TransportMode::kManaged:
      sb_config.auto_start_server = true;
      sb_config.ipc_method = scratchbird::server::IPCMethod::AUTO;
      break;
    case TransportMode::kListenerOnly:
      sb_config.auto_start_server = false;
      sb_config.ipc_method = scratchbird::server::IPCMethod::TCP_LOCALHOST;
      sb_config.tcp_port = config.port;
      break;
    case TransportMode::kIpcOnly:
      sb_config.auto_start_server = false;
      sb_config.ipc_method = scratchbird::server::stringToIPCMethod(config.ipc_method);
      sb_config.socket_path = config.socket_path;
      break;
    case TransportMode::kEmbedded:
      sb_config.auto_start_server = false;
      break;
    default:
      break;
  }

  return sb_config;
}
#endif

}  // namespace

struct ScratchbirdSbwpClient::Impl {
#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  std::unique_ptr<scratchbird::client::Connection> connection;
  std::unique_ptr<EmbeddedProtocolAdapter> embedded_adapter;
#endif
};

ScratchbirdSbwpClient::ScratchbirdSbwpClient(ScratchbirdRuntimeConfig config)
    : config_(std::move(config)), impl_(std::make_unique<Impl>()) {}

ScratchbirdSbwpClient::~ScratchbirdSbwpClient() {
  Disconnect();
}

void ScratchbirdSbwpClient::SetConfig(ScratchbirdRuntimeConfig config) {
  const bool same_config =
      config_.mode == config.mode && config_.host == config.host &&
      config_.port == config.port &&
      config_.database == config.database && config_.user == config.user &&
      config_.password == config.password &&
      config_.connect_timeout_ms == config.connect_timeout_ms &&
      config_.read_timeout_ms == config.read_timeout_ms &&
      config_.write_timeout_ms == config.write_timeout_ms &&
      config_.auto_start_timeout_ms == config.auto_start_timeout_ms &&
      config_.server_executable == config.server_executable &&
      config_.socket_path == config.socket_path &&
      config_.ipc_method == config.ipc_method;

  if (same_config) {
    return;
  }

  if (IsConnected()) {
    Disconnect();
  }
  config_ = std::move(config);
}

const ScratchbirdRuntimeConfig& ScratchbirdSbwpClient::GetConfig() const {
  return config_;
}

bool ScratchbirdSbwpClient::IsConnected() const {
#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  if (!impl_) {
    return false;
  }

  if (config_.mode == TransportMode::kEmbedded) {
    return impl_->embedded_adapter != nullptr;
  }

  return impl_->connection && impl_->connection->isConnected();
#else
  return false;
#endif
}

void ScratchbirdSbwpClient::Disconnect() {
#if defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  if (impl_ && impl_->connection) {
    impl_->connection->disconnect();
  }
  if (impl_) {
    impl_->embedded_adapter.reset();
  }
#endif
}

QueryResponse ScratchbirdSbwpClient::StatusError(const std::string& path,
                                                const std::string& message) const {
  QueryResponse response;
  response.status = core::Status::Error(message);
  response.execution_path = path;
  return response;
}

QueryResponse ScratchbirdSbwpClient::ConnectIfNeeded() {
  constexpr char kPath[] = "native_adapter::sbwp_connect";

#if !defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  return StatusError(kPath, "ScratchBird SBWP integration disabled at build time");
#else
  if (config_.mode == TransportMode::kEmbedded) {
    if (impl_->embedded_adapter) {
      return {core::Status::Ok(), {}, kPath};
    }

    impl_->embedded_adapter =
        std::make_unique<EmbeddedProtocolAdapter>(BuildEmbeddedAdapterConfig(config_));
    if (!impl_->embedded_adapter) {
      return StatusError(kPath, "failed to create embedded protocol adapter");
    }
    return {core::Status::Ok(), {}, kPath};
  }

  if (!impl_->connection) {
    impl_->connection = std::make_unique<scratchbird::client::Connection>();
  }

  if (impl_->connection->isConnected()) {
    return {core::Status::Ok(), {}, kPath};
  }

  if (config_.mode == TransportMode::kPreview) {
    return StatusError(kPath, "Preview mode does not use scratchbird transport");
  }

  const auto connection_config = BuildConnectionConfig(config_);
  scratchbird::core::ErrorContext context;
  const auto status = impl_->connection->connect(connection_config, &context);
  if (status != scratchbird::core::Status::OK) {
    const auto message =
        BuildFailureMessage("failed to connect", context,
                            impl_->connection->getLastError());
    return StatusError(kPath, message);
  }
  return {core::Status::Ok(), {}, kPath};
#endif
}

QueryResponse ScratchbirdSbwpClient::ExecuteRemoteSql(const std::string& sql,
                                                     bool collect_rows,
                                                     const std::string& path) {
  QueryResponse response = ConnectIfNeeded();
  if (!response.status.ok) {
    response.execution_path = path;
    return response;
  }

#if !defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  return StatusError(path, "ScratchBird SBWP integration disabled at build time");
#else
  if (config_.mode == TransportMode::kEmbedded) {
    return ExecuteEmbeddedQuery(impl_->embedded_adapter.get(), sql, collect_rows,
                               path);
  }

  scratchbird::client::ResultSet results;
  scratchbird::core::ErrorContext context;
  const auto status = impl_->connection->executeQuery(sql, &results, &context);
  if (status != scratchbird::core::Status::OK) {
    const auto message =
        BuildFailureMessage("server execution failed", context,
                            impl_->connection->getLastError());
    return StatusError(path, message);
  }

  const std::size_t column_count = results.getColumnCount();
  response.result_set.columns.reserve(column_count);
  for (std::size_t i = 0; i < column_count; ++i) {
    response.result_set.columns.push_back(results.getColumnName(i));
  }

  if (collect_rows) {
    while (results.next()) {
      std::vector<std::string> row;
      row.reserve(column_count);
      for (std::size_t column = 0; column < column_count; ++column) {
        if (results.isNull(column)) {
          row.push_back("NULL");
          continue;
        }
        row.push_back(results.getString(column));
      }
      response.result_set.rows.push_back(std::move(row));
    }
  } else if (response.result_set.columns.empty()) {
    response.result_set.columns = {"command", "rows_affected"};
    response.result_set.rows.push_back(
        {results.getCommandTag(), std::to_string(results.getRowsAffected())});
  } else if (response.result_set.rows.empty()) {
    const std::string command = results.getCommandTag().empty() ? "OK" : results.getCommandTag();
    response.result_set.rows.push_back({command, std::to_string(results.getRowsAffected())});
  }

  response.status = core::Status::Ok();
  response.execution_path = path;
  return response;
#endif
}

QueryResponse ScratchbirdSbwpClient::ExecuteSql(const std::string& sql) {
  return ExecuteRemoteSql(sql, true, "native_adapter::sbwp_query");
}

QueryResponse ScratchbirdSbwpClient::BeginTransaction() {
  QueryResponse response = ConnectIfNeeded();
  if (!response.status.ok) {
    return response;
  }

#if !defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  return StatusError("native_adapter::sbwp_txn_begin",
                     "ScratchBird SBWP integration disabled at build time");
#else
  if (config_.mode == TransportMode::kEmbedded) {
    return ExecuteEmbeddedQuery(impl_->embedded_adapter.get(), "BEGIN", false,
                               "native_adapter::sbwp_txn_begin");
  }

  scratchbird::core::ErrorContext context;
  const auto status = impl_->connection->beginTransaction(&context);
  if (status != scratchbird::core::Status::OK) {
    const auto message =
        BuildFailureMessage("failed to begin transaction", context,
                            impl_->connection->getLastError());
    return StatusError("native_adapter::sbwp_txn_begin", message);
  }

  return QueryResponse{core::Status::Ok(), {}, "native_adapter::sbwp_txn_begin"};
#endif
}

QueryResponse ScratchbirdSbwpClient::CommitTransaction() {
  QueryResponse response = ConnectIfNeeded();
  if (!response.status.ok) {
    return response;
  }

#if !defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  return StatusError("native_adapter::sbwp_txn_commit",
                     "ScratchBird SBWP integration disabled at build time");
#else
  if (config_.mode == TransportMode::kEmbedded) {
    return ExecuteEmbeddedQuery(impl_->embedded_adapter.get(), "COMMIT", false,
                               "native_adapter::sbwp_txn_commit");
  }

  scratchbird::core::ErrorContext context;
  const auto status = impl_->connection->commit(&context);
  if (status != scratchbird::core::Status::OK) {
    const auto message =
        BuildFailureMessage("failed to commit transaction", context,
                            impl_->connection->getLastError());
    return StatusError("native_adapter::sbwp_txn_commit", message);
  }

  return QueryResponse{core::Status::Ok(), {}, "native_adapter::sbwp_txn_commit"};
#endif
}

QueryResponse ScratchbirdSbwpClient::RollbackTransaction() {
  QueryResponse response = ConnectIfNeeded();
  if (!response.status.ok) {
    return response;
  }

#if !defined(ROBIN_MIGRATE_WITH_SCRATCHBIRD_SBWP)
  return StatusError("native_adapter::sbwp_txn_rollback",
                     "ScratchBird SBWP integration disabled at build time");
#else
  if (config_.mode == TransportMode::kEmbedded) {
    return ExecuteEmbeddedQuery(impl_->embedded_adapter.get(), "ROLLBACK", false,
                               "native_adapter::sbwp_txn_rollback");
  }

  scratchbird::core::ErrorContext context;
  const auto status = impl_->connection->rollback(&context);
  if (status != scratchbird::core::Status::OK) {
    const auto message =
        BuildFailureMessage("failed to rollback transaction", context,
                            impl_->connection->getLastError());
    return StatusError("native_adapter::sbwp_txn_rollback", message);
  }

  return QueryResponse{core::Status::Ok(), {}, "native_adapter::sbwp_txn_rollback"};
#endif
}

}  // namespace scratchrobin::backend
