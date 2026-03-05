#pragma once

#include <string>

#include "core/result_set.h"
#include "core/status.h"

namespace scratchrobin::backend {

struct QueryResponse {
  core::Status status;
  core::ResultSet result_set;
  std::string execution_path;
};

}  // namespace scratchrobin::backend
