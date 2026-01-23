#ifndef SCRATCHROBIN_FIREBIRD_BACKEND_H
#define SCRATCHROBIN_FIREBIRD_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

std::unique_ptr<ConnectionBackend> CreateFirebirdBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_FIREBIRD_BACKEND_H
