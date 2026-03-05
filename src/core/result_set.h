#pragma once

#include <string>
#include <vector>

namespace scratchrobin::core {

struct ResultSet {
  std::vector<std::string> columns;
  std::vector<std::vector<std::string>> rows;
};

}  // namespace scratchrobin::core
