#include "backend/server_session_gateway.h"

namespace scratchrobin::backend {

QueryResponse ServerSessionGateway::Execute(const QueryRequest& request) const {
  if (!request.bytecode_flag) {
    return {
        core::Status::Error("QUERY rejected: QueryFlags::BYTECODE is required"),
        {},
        "server_session::reject_missing_bytecode_flag",
    };
  }

  if (request.payload.type != core::QueryPayloadType::kSblrBytecode) {
    return {
        core::Status::Error("QUERY rejected: SQL text cannot execute in server_session"),
        {},
        "server_session::reject_sql_text",
    };
  }

  QueryResponse response;
  response.status = core::Status::Ok();
  response.execution_path = "server_session::execute_validated_sblr";
  response.result_set.columns = {"status", "notes"};
  response.result_set.rows = {{"ok", "simulated sblr execution"}};
  return response;
}

}  // namespace scratchrobin::backend
