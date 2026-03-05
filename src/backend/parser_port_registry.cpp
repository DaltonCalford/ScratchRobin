#include "backend/parser_port_registry.h"

namespace scratchrobin::backend {

core::Status ParserPortRegistry::Register(const std::uint16_t port,
                                          std::string dialect) {
  if (dialect.empty()) {
    return core::Status::Error("dialect is required for parser registration");
  }

  map_[port] = std::move(dialect);
  return core::Status::Ok();
}

core::Status ParserPortRegistry::Validate(const std::uint16_t port,
                                          const std::string& dialect) const {
  const auto it = map_.find(port);
  if (it == map_.end()) {
    return core::Status::Error(
        "port has no parser mapping; parser auto-detect is not allowed");
  }

  if (it->second != dialect) {
    return core::Status::Error(
        "dialect mismatch for configured port; one parser per port enforced");
  }

  return core::Status::Ok();
}

}  // namespace scratchrobin::backend
