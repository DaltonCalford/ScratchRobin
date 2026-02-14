#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace scratchrobin {

std::string Sha256Hex(std::string_view data);
std::string Sha256Hex(const std::vector<std::uint8_t>& data);

}  // namespace scratchrobin
