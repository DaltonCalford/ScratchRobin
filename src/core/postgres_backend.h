#ifndef SCRATCHROBIN_POSTGRES_BACKEND_H
#define SCRATCHROBIN_POSTGRES_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

std::unique_ptr<ConnectionBackend> CreatePostgresBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_POSTGRES_BACKEND_H
