#ifndef SCRATCHROBIN_MYSQL_BACKEND_H
#define SCRATCHROBIN_MYSQL_BACKEND_H

#include <memory>

#include "core/connection_backend.h"

namespace scratchrobin {

std::unique_ptr<ConnectionBackend> CreateMySqlBackend();

} // namespace scratchrobin

#endif // SCRATCHROBIN_MYSQL_BACKEND_H
