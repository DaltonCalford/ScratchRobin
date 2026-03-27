#include "backend/server_session_gateway.h"

#include <cstring>
#include <sstream>

namespace scratchrobin::backend {

/**
 * @brief Execute SBLR bytecode on the ScratchBird server
 * 
 * This is the final execution stage in the native pipeline:
 * SQL → ParserPortRegistry → NativeParserCompiler → ServerSessionGateway
 * 
 * The request contains validated SBLR bytecode that is ready for execution.
 * In production, this would connect to the ScratchBird server via SBWP
 * and execute the bytecode directly.
 */
QueryResponse ServerSessionGateway::Execute(const QueryRequest& request) const {
  // Validate bytecode flag is set
  if (!request.bytecodeFlag()) {
    return {
        core::Status::Error("QUERY rejected: QueryFlags::BYTECODE is required"),
        {},
        "server_session::reject_missing_bytecode_flag",
    };
  }

  // Validate payload type is bytecode (not raw SQL)
  if (request.payload.type != core::QueryPayloadType::kSblrBytecode) {
    return {
        core::Status::Error("QUERY rejected: SQL text cannot execute in server_session"),
        {},
        "server_session::reject_sql_text",
    };
  }

  // Validate bytecode is not empty
  if (request.payload.body.empty()) {
    return {
        core::Status::Error("QUERY rejected: Empty bytecode payload"),
        {},
        "server_session::reject_empty_bytecode",
    };
  }

  // Execute the bytecode
  return ExecuteBytecode(request);
}

/**
 * @brief Internal method to execute validated SBLR bytecode
 * 
 * TODO: When ScratchBird libraries are available, this will:
 * 1. Connect to ScratchBird server via SBWP (ScratchBird Wire Protocol)
 * 2. Send the bytecode payload
 * 3. Execute on the server
 * 4. Return actual results
 * 
 * For now, we simulate execution with realistic result generation based on
 * the bytecode content to allow testing of the full pipeline.
 */
QueryResponse ServerSessionGateway::ExecuteBytecode(const QueryRequest& request) const {
  QueryResponse response;
  
  // Extract query type from bytecode metadata if available
  std::string query_type = ExtractQueryType(request.payload.body);
  
  if (query_type == "SELECT") {
    // Simulate SELECT execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"id", "name", "value"};
    response.result_set.rows = {
        {"1", "Test Row 1", "100"},
        {"2", "Test Row 2", "200"},
        {"3", "Test Row 3", "300"},
    };
  } else if (query_type == "INSERT") {
    // Simulate INSERT execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"affected_rows", "last_insert_id"};
    response.result_set.rows = {{"1", "42"}};
  } else if (query_type == "UPDATE") {
    // Simulate UPDATE execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"affected_rows"};
    response.result_set.rows = {{"5"}};
  } else if (query_type == "DELETE") {
    // Simulate DELETE execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"affected_rows"};
    response.result_set.rows = {{"3"}};
  } else if (query_type == "CREATE" || query_type == "DROP" || query_type == "ALTER") {
    // Simulate DDL execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"command", "status"};
    response.result_set.rows = {{query_type, "OK"}};
  } else {
    // Generic execution
    response.status = core::Status::Ok();
    response.execution_path = "server_session::execute_validated_sblr";
    response.result_set.columns = {"status", "message"};
    response.result_set.rows = {{"ok", "Bytecode executed successfully"}};
  }
  
  return response;
}

/**
 * @brief Extract query type from bytecode payload
 * 
 * This is a heuristic to determine the query type from bytecode.
 * In production, the bytecode would contain metadata about the operation.
 */
std::string ServerSessionGateway::ExtractQueryType(const std::string& bytecode) const {
  // Check for query type hints in bytecode metadata section
  // This is a simplified version - real implementation would parse bytecode
  
  if (bytecode.size() >= 4) {
    // Check first bytes for magic number or query type indicator
    const char* data = bytecode.data();
    
    // Look for SQL keyword hints in the bytecode (for testing)
    std::string upper_bytecode;
    for (size_t i = 0; i < std::min(bytecode.size(), size_t(100)); ++i) {
      upper_bytecode += static_cast<char>(std::toupper(data[i]));
    }
    
    if (upper_bytecode.find("SELECT") != std::string::npos) return "SELECT";
    if (upper_bytecode.find("INSERT") != std::string::npos) return "INSERT";
    if (upper_bytecode.find("UPDATE") != std::string::npos) return "UPDATE";
    if (upper_bytecode.find("DELETE") != std::string::npos) return "DELETE";
    if (upper_bytecode.find("CREATE") != std::string::npos) return "CREATE";
    if (upper_bytecode.find("DROP") != std::string::npos) return "DROP";
    if (upper_bytecode.find("ALTER") != std::string::npos) return "ALTER";
  }
  
  return "UNKNOWN";
}

}  // namespace scratchrobin::backend
