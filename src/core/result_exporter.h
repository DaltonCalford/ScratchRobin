#ifndef SCRATCHROBIN_RESULT_EXPORTER_H
#define SCRATCHROBIN_RESULT_EXPORTER_H

#include <cstddef>
#include <string>

#include "core/query_types.h"

namespace scratchrobin {

struct ExportOptions {
    bool includeHeaders = true;
    size_t maxBinaryBytes = 0;
    bool includeBinarySize = false;
};

bool ExportResultToCsv(const QueryResult& result,
                       const std::string& path,
                       std::string* error,
                       const ExportOptions& options = {});

bool ExportResultToJson(const QueryResult& result,
                        const std::string& path,
                        std::string* error,
                        const ExportOptions& options = {});

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESULT_EXPORTER_H
