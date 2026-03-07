#pragma once

#include <sstream>
#include <string>

#include "core/query_payload.h"
#include "core/status.h"

namespace scratchrobin::backend {

struct CompileOutput {
  core::Status status;
  core::QueryPayload bytecode;
};

/**
 * Native SQL to SBLR Bytecode Compiler
 *
 * Integrates ScratchBird's QueryCompilerV3 to perform real SQL parsing
 * and SBLR bytecode generation. Replaces the previous placeholder
 * implementation that just prefixed SQL with "SBLR:".
 */
class NativeParserCompiler {
 public:
  /**
   * Compile SQL to SBLR bytecode
   * @param sql The SQL statement to compile
   * @return CompileOutput with status and bytecode payload
   */
  CompileOutput CompileSqlToSblr(const std::string& sql) const;
  
  /**
   * Compile with trace/diagnostic output
   * @param sql The SQL statement to compile
   * @param trace_output Optional output for trace information (hashes, etc.)
   * @return CompileOutput with status and bytecode payload
   */
  CompileOutput CompileSqlToSblrWithTrace(const std::string& sql,
                                          std::string* trace_output) const;
};

}  // namespace scratchrobin::backend
