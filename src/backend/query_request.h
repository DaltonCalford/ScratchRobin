#pragma once

#include <cstdint>
#include <string>

#include "core/query_payload.h"

namespace scratchrobin::backend {

struct QueryRequest {
  core::QueryPayload payload;
  bool bytecode_flag{false};
  std::uint16_t server_port{0};
  std::string dialect;
};

}  // namespace scratchrobin::backend
