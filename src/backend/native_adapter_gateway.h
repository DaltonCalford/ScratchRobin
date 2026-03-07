#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "backend/parser_port_registry.h"
#include "backend/query_response.h"
#include "backend/server_session_gateway.h"
#include "backend/native_parser_compiler.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

/**
 * NativeAdapterGateway - Gateway between SessionClient and server session
 *
 * Routes SQL queries through the native compilation pipeline:
 *   SQL → ParserPortRegistry → NativeParserCompiler → ServerSessionGateway
 */
class NativeAdapterGateway {
 public:
  NativeAdapterGateway(const ParserPortRegistry* registry,
                       const NativeParserCompiler* compiler,
                       const ServerSessionGateway* session)
      : registry_(registry), compiler_(compiler), session_(session) {}

  /**
   * Submit SQL for execution through the native pipeline
   */
  QueryResponse SubmitSql(std::uint16_t port, const std::string& dialect,
                          const std::string& sql) const;
  
  /**
   * Set runtime configuration for query execution
   * @param config Runtime configuration (timeouts, connection settings)
   */
  void setRuntimeConfig(const ScratchbirdRuntimeConfig& config) {
    runtime_config_ = config;
  }
  
  /**
   * Get current runtime configuration
   * @return Optional containing config if set
   */
  std::optional<ScratchbirdRuntimeConfig> runtimeConfig() const {
    return runtime_config_;
  }

 private:
  const ParserPortRegistry* registry_;
  const NativeParserCompiler* compiler_;
  const ServerSessionGateway* session_;
  std::optional<ScratchbirdRuntimeConfig> runtime_config_;
};

}  // namespace scratchrobin::backend
