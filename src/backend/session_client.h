#pragma once

#include <cstdint>
#include <string>

#include "backend/native_adapter_gateway.h"
#include "backend/query_response.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

/**
 * SessionClient - High-level client for database sessions
 *
 * Manages runtime configuration and delegates query execution
 * to the NativeAdapterGateway.
 */
class SessionClient {
 public:
  explicit SessionClient(NativeAdapterGateway* adapter) 
    : adapter_(adapter), config_applied_(false) {}

  /**
   * Configure runtime settings for the session
   * @param config Runtime configuration (connection, timeouts, etc.)
   */
  void ConfigureRuntime(const ScratchbirdRuntimeConfig& config);
  
  /**
   * Get the current runtime configuration
   * @return Pointer to config, or nullptr if not configured
   */
  const ScratchbirdRuntimeConfig* GetRuntimeConfig() const;
  
  /**
   * Check if runtime has been configured
   * @return true if ConfigureRuntime has been called
   */
  bool IsRuntimeConfigured() const;
  
  /**
   * Execute SQL through the native adapter gateway
   * @param port Parser port (4044 for scratchbird-native)
   * @param dialect SQL dialect identifier
   * @param sql The SQL statement to execute
   * @return Query response with results or error
   */
  QueryResponse ExecuteSql(std::uint16_t port, const std::string& dialect,
                           const std::string& sql) const;

 private:
  NativeAdapterGateway* adapter_;
  ScratchbirdRuntimeConfig runtime_config_;
  bool config_applied_;
};

}  // namespace scratchrobin::backend
