#pragma once

#include <map>
#include <string>

namespace scratchrobin::core {

enum class QueryPayloadType {
  kSblrBytecode,
  kSqlText,
};

/**
 * Query payload for transmission through the execution pipeline
 */
struct QueryPayload {
  QueryPayloadType type{QueryPayloadType::kSqlText};
  std::string body;
  
  // Optional metadata for debugging/telemetry (e.g., compilation stats)
  std::map<std::string, std::string> metadata;
};

}  // namespace scratchrobin::core
