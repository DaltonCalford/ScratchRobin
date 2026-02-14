/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_EMBEDDED_BACKEND_H
#define SCRATCHROBIN_EMBEDDED_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

/**
 * @brief Create an embedded backend for direct in-process database access
 * 
 * The embedded backend links directly to the ScratchBird engine, bypassing
 * network/IPC layers entirely. This provides:
 * - Zero-copy result transfer via shared memory
 * - No socket overhead
 * - Direct engine API access
 * - Ideal for local development and embedded deployments
 * 
 * @return std::unique_ptr<ConnectionBackend> or nullptr if not available
 */
std::unique_ptr<ConnectionBackend> CreateEmbeddedBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_EMBEDDED_BACKEND_H
