/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <cstdint>
#include <string>

namespace scratchrobin::backend {

enum class TransportMode {
  kPreview = 0,
  kManaged = 1,
  kListenerOnly = 2,
  kIpcOnly = 3,
  kEmbedded = 4,
};

inline const char* ToString(TransportMode mode) {
  switch (mode) {
    case TransportMode::kPreview:
      return "preview";
    case TransportMode::kManaged:
      return "managed";
    case TransportMode::kListenerOnly:
      return "listener-only";
    case TransportMode::kIpcOnly:
      return "ipc-only";
    case TransportMode::kEmbedded:
      return "embedded";
    default:
      return "unknown";
  }
}

struct ScratchbirdRuntimeConfig {
  TransportMode mode{TransportMode::kPreview};
  std::string host{"127.0.0.1"};
  std::uint16_t port{4044};
  std::string database{"default"};
  std::string user{};
  std::string password{};
  std::uint32_t connect_timeout_ms{5000};
  std::uint32_t read_timeout_ms{30000};
  std::uint32_t write_timeout_ms{30000};
  std::uint32_t auto_start_timeout_ms{10000};
  std::string server_executable{};
  std::string socket_path{};
  std::string ipc_method{"AUTO"};

  bool operator==(const ScratchbirdRuntimeConfig& other) const {
    return mode == other.mode && host == other.host && port == other.port &&
           database == other.database && user == other.user &&
           password == other.password &&
           connect_timeout_ms == other.connect_timeout_ms &&
           read_timeout_ms == other.read_timeout_ms &&
           write_timeout_ms == other.write_timeout_ms &&
           auto_start_timeout_ms == other.auto_start_timeout_ms &&
           server_executable == other.server_executable &&
           socket_path == other.socket_path &&
           ipc_method == other.ipc_method;
  }

  bool operator!=(const ScratchbirdRuntimeConfig& other) const {
    return !(*this == other);
  }
};

}  // namespace scratchrobin::backend
