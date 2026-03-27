#pragma once

#include <string>

#include "backend/query_request.h"
#include "backend/query_response.h"

namespace scratchrobin::backend {

/**
 * @brief Server Session Gateway - Final execution stage for SBLR bytecode
 * 
 * The ServerSessionGateway is responsible for executing validated SBLR bytecode
 * on the ScratchBird server. It is the final stage in the native execution pipeline:
 * 
 * SQL → ParserPortRegistry → NativeParserCompiler → ServerSessionGateway
 * 
 * This class:
 * - Validates execution prerequisites (bytecode flag, payload type)
 * - Routes bytecode to the appropriate execution handler
 * - Returns query results or error information
 * 
 * TODO: When ScratchBird client libraries are available, integrate with:
 * - SBWP (ScratchBird Wire Protocol) client
 * - Connection pooling
 * - Transaction management
 */
class ServerSessionGateway {
 public:
  /**
   * @brief Execute a query request containing SBLR bytecode
   * 
   * @param request The query request with bytecode payload
   * @return QueryResponse containing results or error status
   * 
   * Requirements:
   * - request.bytecodeFlag() must be true
   * - request.payload.type must be kSblrBytecode
   * - request.payload.body must not be empty
   */
  QueryResponse Execute(const QueryRequest& request) const;

 private:
  /**
   * @brief Internal method to execute validated bytecode
   * 
   * Dispatches to the appropriate execution handler based on query type.
   * Currently simulates execution until ScratchBird libraries are available.
   * 
   * @param request The validated query request
   * @return QueryResponse with execution results
   */
  QueryResponse ExecuteBytecode(const QueryRequest& request) const;
  
  /**
   * @brief Extract query type from bytecode payload
   * 
   * Uses heuristics to determine the SQL operation type from bytecode.
   * This helps route to the appropriate simulation handler.
   * 
   * @param bytecode The SBLR bytecode payload
   * @return String indicating query type (SELECT, INSERT, UPDATE, etc.)
   */
  std::string ExtractQueryType(const std::string& bytecode) const;
};

}  // namespace scratchrobin::backend
