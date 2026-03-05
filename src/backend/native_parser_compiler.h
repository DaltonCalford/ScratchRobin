#pragma once

#include <string>

#include "core/query_payload.h"
#include "core/status.h"

namespace scratchrobin::backend {

struct CompileOutput {
  core::Status status;
  core::QueryPayload bytecode;
};

class NativeParserCompiler {
 public:
  CompileOutput CompileSqlToSblr(const std::string& sql) const;
};

}  // namespace scratchrobin::backend
