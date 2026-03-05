#pragma once

#include <string>

namespace scratchrobin::core {

enum class QueryPayloadType {
  kSblrBytecode,
  kSqlText,
};

struct QueryPayload {
  QueryPayloadType type{QueryPayloadType::kSqlText};
  std::string body;
};

}  // namespace scratchrobin::core
