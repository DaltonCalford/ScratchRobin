#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/status.h"

namespace scratchrobin::backend {

class ParserPortRegistry {
 public:
  core::Status Register(std::uint16_t port, std::string dialect);
  core::Status Validate(std::uint16_t port, const std::string& dialect) const;

 private:
  std::unordered_map<std::uint16_t, std::string> map_;
};

}  // namespace scratchrobin::backend
