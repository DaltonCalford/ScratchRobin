#include "backend/native_parser_compiler.h"

#include <chrono>

// ScratchBird SBLR v3 Compiler integration (when available)
#if defined(SCRATCHROBIN_WITH_SBLR_COMPILER)
#include <scratchbird/sblr/query_compiler_v3.h>
#include <scratchbird/sblr/v3_container.h>
#endif

namespace scratchrobin::backend {

CompileOutput NativeParserCompiler::CompileSqlToSblr(const std::string& sql) const {
  if (sql.empty()) {
    return {core::Status::Error("SQL input is empty"), {}};
  }

#if defined(SCRATCHROBIN_WITH_SBLR_COMPILER)
  // REAL IMPLEMENTATION: Use ScratchBird's v3 QueryCompiler
  scratchbird::sblr::QueryCompilerV3 compiler;
  compiler.setStatsEnabled(true);
  
  auto result = compiler.compile(sql);
  
  if (!result.success()) {
    std::string error_msg = "SBLR compilation failed:";
    for (const auto& err : result.errors()) {
      error_msg += "\n  - " + err;
    }
    return {core::Status::Error(error_msg), {}};
  }
  
  // Convert bytecode vector to string payload
  const auto& bytecode = result.bytecode();
  std::string payload_body(bytecode.begin(), bytecode.end());
  
  core::QueryPayload payload;
  payload.type = core::QueryPayloadType::kSblrBytecode;
  payload.body = payload_body;
  
  // Add compilation metadata
  const auto& stats = result.stats();
  payload.metadata["bytecode_size"] = std::to_string(stats.bytecode_size);
  payload.metadata["parser_time_us"] = std::to_string(stats.parser_time.count());
  payload.metadata["compiler_version"] = "v3";
  
  return {core::Status::Ok(), payload};
#else
  // FALLBACK: Structured placeholder implementation
  // This maintains the same interface but doesn't require full ScratchBird libs
  
  auto start = std::chrono::steady_clock::now();
  
  // Create a structured "pseudo-bytecode" that includes metadata
  // Format: SBv1|<sql_length>|<sql>|<timestamp>
  std::string pseudo_bytecode = "SBv1|";
  pseudo_bytecode += std::to_string(sql.size()) + "|";
  pseudo_bytecode += sql;
  pseudo_bytecode += "|";
  pseudo_bytecode += std::to_string(
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
  
  auto end = std::chrono::steady_clock::now();
  auto parser_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  core::QueryPayload payload;
  payload.type = core::QueryPayloadType::kSblrBytecode;
  payload.body = pseudo_bytecode;
  
  // Add metadata (same as real implementation)
  payload.metadata["bytecode_size"] = std::to_string(pseudo_bytecode.size());
  payload.metadata["parser_time_us"] = std::to_string(parser_time.count());
  payload.metadata["compiler_version"] = "placeholder_v1";
  payload.metadata["note"] = "Full SBLR compiler not linked - using placeholder";
  
  return {core::Status::Ok(), payload};
#endif
}

CompileOutput NativeParserCompiler::CompileSqlToSblrWithTrace(
    const std::string& sql,
    std::string* trace_output) const {
  
  if (trace_output) {
#if defined(SCRATCHROBIN_WITH_SBLR_COMPILER)
    // REAL: Use trace mode to get diagnostic information
    scratchbird::sblr::QueryCompilerV3 compiler;
    auto trace_result = compiler.compileTrace(sql);
    
    if (trace_result.success()) {
      const auto& digest = trace_result.digest();
      std::ostringstream trace;
      trace << "=== SBLR Compilation Trace ===\n";
      trace << "SQL Hash:      " << digest.sql_hash << "\n";
      trace << "AST Hash:      " << digest.ast_hash << "\n";
      trace << "SBLR Hash:     " << digest.sblr_hash << "\n";
      trace << "Root Opcode:   " << digest.root_opcode_symbol << "\n";
      trace << "Normalized:    " << digest.normalized_sql << "\n";
      if (!trace_result.warnings().empty()) {
        trace << "\nWarnings:\n";
        for (const auto& warn : trace_result.warnings()) {
          trace << "  - " << warn << "\n";
        }
      }
      *trace_output = trace.str();
    } else {
      std::string errors;
      for (const auto& err : trace_result.errors()) {
        errors += "  - " + err + "\n";
      }
      *trace_output = "Trace failed:\n" + errors;
    }
#else
    // FALLBACK: Simple trace info
    std::ostringstream trace;
    trace << "=== SBLR Compilation Trace (Placeholder Mode) ===\n";
    trace << "Note: Full SBLR compiler not linked\n";
    trace << "SQL Length:    " << sql.size() << " bytes\n";
    trace << "SQL Preview:   " << sql.substr(0, 50) << "...\n";
    trace << "\nTo enable full compilation, build with:\n";
    trace << "  -DSCRATCHROBIN_WITH_SBLR_COMPILER=ON\n";
    trace << "and link against ScratchBird libraries.\n";
    *trace_output = trace.str();
#endif
  }
  
  // Perform actual compilation (or fallback)
  return CompileSqlToSblr(sql);
}

}  // namespace scratchrobin::backend
