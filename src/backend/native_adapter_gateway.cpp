#include "backend/native_adapter_gateway.h"

#include "backend/query_request.h"

namespace scratchrobin::backend {

QueryResponse NativeAdapterGateway::SubmitSql(const std::uint16_t port,
                                              const std::string& dialect,
                                              const std::string& sql) const {
  const core::Status port_status = registry_->Validate(port, dialect);
  if (!port_status.ok) {
    return {port_status, {}, "native_adapter::validate_parser_port"};
  }

  // Compile with runtime config if available
  CompileOutput compile_output;
  if (runtime_config_.has_value()) {
    compile_output = compiler_->CompileSqlToSblr(sql, runtime_config_.value());
  } else {
    compile_output = compiler_->CompileSqlToSblr(sql);
  }
  
  if (!compile_output.status.ok) {
    return {compile_output.status, {}, "native_adapter::compile_sql_to_sblr"};
  }

  QueryRequest request;
  request.payload = compile_output.bytecode;
  request.setBytecodeFlag(true);
  request.port = port;
  request.dialect = dialect;

  return session_->Execute(request);
}

}  // namespace scratchrobin::backend
