#include "query_parser.h"

namespace scratchrobin {

class QueryParser::Impl {};

QueryParser::QueryParser()
    : impl_(std::make_unique<Impl>()) {
}

QueryParser::~QueryParser() = default;

QueryInfo QueryParser::parseQuery(const std::string& sql) {
    QueryInfo info;
    info.sql = sql;
    info.type = QueryType::UNKNOWN;
    info.state = QueryState::PENDING;
    return info;
}

std::vector<std::string> QueryParser::extractParameters(const std::string& sql) {
    return {}; // TODO: Implement parameter extraction
}

bool QueryParser::validateSyntax(const std::string& sql) {
    return true; // TODO: Implement syntax validation
}

} // namespace scratchrobin