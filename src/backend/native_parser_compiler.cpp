#include "backend/native_parser_compiler.h"

namespace scratchrobin::backend {

CompileOutput NativeParserCompiler::CompileSqlToSblr(const std::string& sql) const {
  if (sql.empty()) {
    return {core::Status::Error("SQL input is empty"), {}};
  }

  core::QueryPayload payload;
  payload.type = core::QueryPayloadType::kSblrBytecode;
  payload.body = "SBLR:" + sql;
  return {core::Status::Ok(), payload};
}

}  // namespace scratchrobin::backend
