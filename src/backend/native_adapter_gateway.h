#pragma once

#include <cstdint>
#include <string>

#include "backend/parser_port_registry.h"
#include "backend/query_response.h"
#include "backend/server_session_gateway.h"
#include "backend/native_parser_compiler.h"

namespace scratchrobin::backend {

class NativeAdapterGateway {
 public:
  NativeAdapterGateway(const ParserPortRegistry* registry,
                       const NativeParserCompiler* compiler,
                       const ServerSessionGateway* session)
      : registry_(registry), compiler_(compiler), session_(session) {}

  QueryResponse SubmitSql(std::uint16_t port, const std::string& dialect,
                          const std::string& sql) const;

 private:
  const ParserPortRegistry* registry_;
  const NativeParserCompiler* compiler_;
  const ServerSessionGateway* session_;
};

}  // namespace scratchrobin::backend
