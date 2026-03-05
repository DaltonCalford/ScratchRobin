#include "backend/session_client.h"

namespace scratchrobin::backend {

void SessionClient::ConfigureRuntime(const ScratchbirdRuntimeConfig& config) const {
  // TODO: Implement runtime configuration
  (void)config;
}

QueryResponse SessionClient::ExecuteSql(const std::uint16_t port,
                                        const std::string& dialect,
                                        const std::string& sql) const {
  return adapter_->SubmitSql(port, dialect, sql);
}

}  // namespace scratchrobin::backend
