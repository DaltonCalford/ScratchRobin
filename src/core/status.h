#pragma once

#include <string>
#include <utility>

namespace scratchrobin::core {

struct Status {
  bool ok{false};
  std::string message;

  static Status Ok() {
    return {true, "ok"};
  }

  static Status Error(std::string message_value) {
    return {false, std::move(message_value)};
  }
};

}  // namespace scratchrobin::core
