#include <cassert>

#include "backend/native_adapter_gateway.h"
#include "backend/native_parser_compiler.h"
#include "backend/parser_port_registry.h"
#include "backend/session_client.h"
#include "backend/server_session_gateway.h"

int main() {
  scratchrobin::backend::ParserPortRegistry registry;
  const auto register_status = registry.Register(4044, "scratchbird-native");
  assert(register_status.ok);

  scratchrobin::backend::NativeParserCompiler compiler;
  scratchrobin::backend::ServerSessionGateway session;
  scratchrobin::backend::NativeAdapterGateway adapter(&registry, &compiler, &session);
  scratchrobin::backend::SessionClient client(&adapter);

  {
    const auto ok = client.ExecuteSql(4044, "scratchbird-native", "select 1");
    assert(ok.status.ok);
    assert(ok.execution_path == "server_session::execute_validated_sblr");
  }

  {
    const auto wrong_dialect = client.ExecuteSql(4044, "legacy-firebird", "select 1");
    assert(!wrong_dialect.status.ok);
  }

  {
    const auto missing_port = client.ExecuteSql(5050, "scratchbird-native", "select 1");
    assert(!missing_port.status.ok);
  }

  return 0;
}
