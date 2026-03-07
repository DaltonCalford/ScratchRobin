/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "backend/scratchbird_sbwp_client.h"

#include <QDebug>

#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
// Only include the driver client header - avoid conflicting headers
#include <scratchbird/client/connection.h>
#endif

namespace scratchrobin::backend {

#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
// Helper to convert driver ResultSet to QueryResponse
static QueryResponse ConvertResultSetToResponse(
    scratchbird::client::ResultSet& rs,
    const std::string& execution_path) {
  
  QueryResponse response;
  response.status = core::Status::Ok();
  response.execution_path = execution_path;
  
  // Get column names
  const auto& columns = rs.getColumns();
  for (const auto& col : columns) {
    response.result_set.columns.push_back(col.name);
  }
  
  // Get rows
  while (rs.next()) {
    std::vector<std::string> row;
    for (size_t i = 0; i < columns.size(); ++i) {
      if (rs.isNull(i)) {
        row.push_back("NULL");
      } else {
        row.push_back(rs.getString(i));
      }
    }
    response.result_set.rows.push_back(std::move(row));
  }
  
  return response;
}

static std::string StatusToString(scratchbird::core::Status status) {
  switch (status) {
    case scratchbird::core::Status::OK: return "OK";
    case scratchbird::core::Status::CANCELLED: return "CANCELLED";
    case scratchbird::core::Status::LOCK_TIMEOUT: return "TIMEOUT";
    case scratchbird::core::Status::CONNECTION_FAILURE: return "CONNECTION_FAILURE";
    case scratchbird::core::Status::INVALID_PASSWORD: return "INVALID_PASSWORD";
    case scratchbird::core::Status::INVALID_AUTHORIZATION: return "INVALID_AUTHORIZATION";
    default: 
      // For any non-OK status, return the numeric code
      if (status != scratchbird::core::Status::OK) {
        return "ERROR_" + std::to_string(static_cast<uint32_t>(status));
      }
      return "UNKNOWN";
  }
}
#endif

struct ScratchbirdSbwpClient::Impl {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  std::unique_ptr<scratchbird::client::Connection> connection;
  scratchbird::client::ConnectionConfig sb_config;
  bool connected = false;
#endif
};

ScratchbirdSbwpClient::ScratchbirdSbwpClient(ScratchbirdRuntimeConfig config)
    : config_(std::move(config)), impl_(std::make_unique<Impl>()) {
}

ScratchbirdSbwpClient::~ScratchbirdSbwpClient() = default;

void ScratchbirdSbwpClient::SetConfig(ScratchbirdRuntimeConfig config) {
  config_ = std::move(config);
}

const ScratchbirdRuntimeConfig& ScratchbirdSbwpClient::GetConfig() const {
  return config_;
}

bool ScratchbirdSbwpClient::IsConnected() const {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  return impl_->connection && impl_->connection->isConnected();
#else
  return false;
#endif
}

void ScratchbirdSbwpClient::Disconnect() {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  if (impl_->connection) {
    impl_->connection->disconnect();
    impl_->connected = false;
    qDebug() << "SBWP client disconnected";
  }
#endif
}

QueryResponse ScratchbirdSbwpClient::ConnectIfNeeded() {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  if (impl_->connection && impl_->connection->isConnected()) {
    return QueryResponse{core::Status::Ok(), {}, "sbwp::already_connected"};
  }
  
  // Build connection config
  impl_->sb_config.database_name = config_.database.empty() ? "default" : config_.database;
  impl_->sb_config.username = config_.user;
  impl_->sb_config.password = config_.password;
  impl_->sb_config.connect_timeout_ms = config_.connect_timeout_ms;
  impl_->sb_config.read_timeout_ms = config_.read_timeout_ms;
  impl_->sb_config.write_timeout_ms = config_.write_timeout_ms;
  impl_->sb_config.query_timeout_ms = config_.read_timeout_ms;
  impl_->sb_config.auto_commit = true;
  
  // Map transport mode
  switch (config_.mode) {
    case TransportMode::kManaged:
      impl_->sb_config.transport_mode = "managed";
      break;
    case TransportMode::kListenerOnly:
      impl_->sb_config.transport_mode = "inet_listener";
      impl_->sb_config.host = config_.host.empty() ? "127.0.0.1" : config_.host;
      impl_->sb_config.tcp_port = config_.port;
      break;
    case TransportMode::kIpcOnly:
    case TransportMode::kEmbedded:
    default:
      // Fall back to TCP localhost
      impl_->sb_config.transport_mode = "inet_listener";
      impl_->sb_config.host = "127.0.0.1";
      impl_->sb_config.tcp_port = config_.port;
      break;
  }
  
  // Create connection
  impl_->connection = std::make_unique<scratchbird::client::Connection>();
  
  scratchbird::core::ErrorContext ctx;
  auto status = impl_->connection->connect(impl_->sb_config, &ctx);
  
  if (status != scratchbird::core::Status::OK) {
    std::string error = "Failed to connect to ScratchBird: ";
    error += ctx.message.empty() ? StatusToString(status) : ctx.message;
    qWarning() << "SBWP connection failed:" << QString::fromStdString(error);
    return QueryResponse{core::Status::Error(error), {}, "sbwp::connect_failed"};
  }
  
  impl_->connected = true;
  qDebug() << "SBWP client connected to" << QString::fromStdString(config_.database);
  return QueryResponse{core::Status::Ok(), {}, "sbwp::connected"};
#else
  return QueryResponse{
    core::Status::Error("SBWP support not compiled in"),
    {},
    "sbwp::not_compiled"
  };
#endif
}

QueryResponse ScratchbirdSbwpClient::ExecuteSql(const std::string& sql) {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  // Ensure connection
  auto connect_response = ConnectIfNeeded();
  if (!connect_response.status.ok && connect_response.execution_path != "sbwp::already_connected") {
    return connect_response;
  }
  
  scratchbird::client::ResultSet rs;
  scratchbird::core::ErrorContext ctx;
  
  auto status = impl_->connection->executeQuery(sql, &rs, &ctx);
  
  if (status != scratchbird::core::Status::OK) {
    std::string error = "Query failed: ";
    error += ctx.message.empty() ? StatusToString(status) : ctx.message;
    return QueryResponse{core::Status::Error(error), {}, "sbwp::query_failed"};
  }
  
  return ConvertResultSetToResponse(rs, "sbwp::execute_sql");
#else
  (void)sql;
  return QueryResponse{
    core::Status::Error("SBWP support not compiled in"),
    {},
    "sbwp::not_compiled"
  };
#endif
}

QueryResponse ScratchbirdSbwpClient::BeginTransaction() {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  auto connect_response = ConnectIfNeeded();
  if (!connect_response.status.ok && connect_response.execution_path != "sbwp::already_connected") {
    return connect_response;
  }
  
  scratchbird::core::ErrorContext ctx;
  auto status = impl_->connection->beginTransaction(&ctx);
  
  if (status != scratchbird::core::Status::OK) {
    std::string error = ctx.message.empty() ? StatusToString(status) : ctx.message;
    return QueryResponse{core::Status::Error(error), {}, "sbwp::begin_failed"};
  }
  
  QueryResponse response;
  response.status = core::Status::Ok();
  response.execution_path = "sbwp::begin_transaction";
  response.result_set.columns = {"status"};
  response.result_set.rows = {{"transaction started"}};
  return response;
#else
  return QueryResponse{
    core::Status::Error("SBWP support not compiled in"),
    {},
    "sbwp::not_compiled"
  };
#endif
}

QueryResponse ScratchbirdSbwpClient::CommitTransaction() {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  if (!impl_->connection || !impl_->connection->isConnected()) {
    return QueryResponse{
      core::Status::Error("Not connected"),
      {},
      "sbwp::not_connected"
    };
  }
  
  scratchbird::core::ErrorContext ctx;
  auto status = impl_->connection->commit(&ctx);
  
  if (status != scratchbird::core::Status::OK) {
    std::string error = ctx.message.empty() ? StatusToString(status) : ctx.message;
    return QueryResponse{core::Status::Error(error), {}, "sbwp::commit_failed"};
  }
  
  QueryResponse response;
  response.status = core::Status::Ok();
  response.execution_path = "sbwp::commit_transaction";
  response.result_set.columns = {"status"};
  response.result_set.rows = {{"committed"}};
  return response;
#else
  return QueryResponse{
    core::Status::Error("SBWP support not compiled in"),
    {},
    "sbwp::not_compiled"
  };
#endif
}

QueryResponse ScratchbirdSbwpClient::RollbackTransaction() {
#if defined(SCRATCHROBIN_WITH_SCRATCHBIRD_SBWP)
  if (!impl_->connection || !impl_->connection->isConnected()) {
    return QueryResponse{
      core::Status::Error("Not connected"),
      {},
      "sbwp::not_connected"
    };
  }
  
  scratchbird::core::ErrorContext ctx;
  auto status = impl_->connection->rollback(&ctx);
  
  if (status != scratchbird::core::Status::OK) {
    std::string error = ctx.message.empty() ? StatusToString(status) : ctx.message;
    return QueryResponse{core::Status::Error(error), {}, "sbwp::rollback_failed"};
  }
  
  QueryResponse response;
  response.status = core::Status::Ok();
  response.execution_path = "sbwp::rollback_transaction";
  response.result_set.columns = {"status"};
  response.result_set.rows = {{"rolled back"}};
  return response;
#else
  return QueryResponse{
    core::Status::Error("SBWP support not compiled in"),
    {},
    "sbwp::not_compiled"
  };
#endif
}

}  // namespace scratchrobin::backend
