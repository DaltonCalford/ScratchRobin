#pragma once

#include "backend/query_request.h"
#include "backend/query_response.h"

namespace scratchrobin::backend {

class ServerSessionGateway {
 public:
  QueryResponse Execute(const QueryRequest& request) const;
};

}  // namespace scratchrobin::backend
