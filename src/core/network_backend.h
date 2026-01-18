#ifndef SCRATCHROBIN_NETWORK_BACKEND_H
#define SCRATCHROBIN_NETWORK_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

std::unique_ptr<ConnectionBackend> CreateNetworkBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_NETWORK_BACKEND_H
