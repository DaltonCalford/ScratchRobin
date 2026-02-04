/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_IPC_BACKEND_H
#define SCRATCHROBIN_IPC_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

/**
 * @brief Create an IPC backend for local socket-based database access
 * 
 * The IPC backend uses Unix domain sockets (Linux/macOS) or named pipes
 * (Windows) for local communication with the ScratchBird server.
 * This provides:
 * - Lower latency than TCP for local connections
 * - No network stack overhead
 * - File-system based access control
 * - Automatic socket path resolution
 * 
 * @return std::unique_ptr<ConnectionBackend> or nullptr if not available
 */
std::unique_ptr<ConnectionBackend> CreateIpcBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_IPC_BACKEND_H
