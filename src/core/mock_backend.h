#ifndef SCRATCHROBIN_MOCK_BACKEND_H
#define SCRATCHROBIN_MOCK_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

std::unique_ptr<ConnectionBackend> CreateMockBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_MOCK_BACKEND_H
