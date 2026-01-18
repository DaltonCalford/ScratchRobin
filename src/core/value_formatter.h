#ifndef SCRATCHROBIN_VALUE_FORMATTER_H
#define SCRATCHROBIN_VALUE_FORMATTER_H

#include <cstddef>
#include <string>

#include "core/query_types.h"

namespace scratchrobin {

struct FormatOptions {
    size_t maxBinaryBytes = 64;
    bool includeBinarySize = true;
};

std::string FormatValueForDisplay(const QueryValue& value,
                                  const std::string& type,
                                  const FormatOptions& options = {});

std::string FormatValueForExport(const QueryValue& value,
                                 const std::string& type,
                                 const FormatOptions& options = {});

bool IsBooleanType(const std::string& type);
bool IsNumericType(const std::string& type);
bool IsJsonType(const std::string& type);

} // namespace scratchrobin

#endif // SCRATCHROBIN_VALUE_FORMATTER_H
