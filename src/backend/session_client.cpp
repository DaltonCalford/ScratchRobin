#include "backend/session_client.h"

#include <QDebug>

namespace scratchrobin::backend {

void SessionClient::ConfigureRuntime(const ScratchbirdRuntimeConfig& config) {
  // Store the runtime configuration
  runtime_config_ = config;
  config_applied_ = true;
  
  // Apply configuration to the adapter gateway
  if (adapter_) {
    adapter_->setRuntimeConfig(config);
  }
  
  // Log configuration for debugging
  qDebug() << "SessionClient: Runtime configuration applied:";
  qDebug() << "  Database:" << QString::fromStdString(config.database);
  qDebug() << "  Host:" << QString::fromStdString(config.host);
  qDebug() << "  Port:" << config.port;
  qDebug() << "  Transport mode:" << ToString(config.mode);
  qDebug() << "  Connect timeout:" << config.connect_timeout_ms << "ms";
  qDebug() << "  Query timeout:" << config.read_timeout_ms << "ms";
}

const ScratchbirdRuntimeConfig* SessionClient::GetRuntimeConfig() const {
  return config_applied_ ? &runtime_config_ : nullptr;
}

bool SessionClient::IsRuntimeConfigured() const {
  return config_applied_;
}

QueryResponse SessionClient::ExecuteSql(const std::uint16_t port,
                                        const std::string& dialect,
                                        const std::string& sql) const {
  if (!adapter_) {
    return QueryResponse{
      core::Status::Error("No adapter gateway available"),
      {},
      "session_client::no_adapter"
    };
  }
  
  return adapter_->SubmitSql(port, dialect, sql);
}

}  // namespace scratchrobin::backend
