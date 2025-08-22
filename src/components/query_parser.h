#ifndef SCRATCHROBIN_QUERY_PARSER_H
#define SCRATCHROBIN_QUERY_PARSER_H

#include <memory>
#include <string>
#include <vector>
#include "types/query_types.h"

namespace scratchrobin {

class QueryParser {
public:
    QueryParser();
    ~QueryParser();

    QueryInfo parseQuery(const std::string& sql);
    std::vector<std::string> extractParameters(const std::string& sql);
    bool validateSyntax(const std::string& sql);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_QUERY_PARSER_H
