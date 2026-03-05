#pragma once

#include <cstdint>
#include <string>

#include "backend/native_adapter_gateway.h"
#include "backend/query_response.h"
#include "backend/scratchbird_runtime_config.h"

namespace scratchrobin::backend {

class SessionClient {
 public:
  explicit SessionClient(NativeAdapterGateway* adapter) : adapter_(adapter) {}

  void ConfigureRuntime(const ScratchbirdRuntimeConfig& config) const;
  
  QueryResponse ExecuteSql(std::uint16_t port, const std::string& dialect,
                           const std::string& sql) const;

 private:
  NativeAdapterGateway* adapter_;
};

}  // namespace scratchrobin::backend
