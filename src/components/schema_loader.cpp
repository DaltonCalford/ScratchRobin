#include "schema_loader.h"

namespace scratchrobin {

class SchemaLoader::Impl {};

SchemaLoader::SchemaLoader()
    : impl_(std::make_unique<Impl>()) {
}

SchemaLoader::~SchemaLoader() = default;

std::vector<SchemaInfo> SchemaLoader::loadSchemas(const std::string& connectionId) {
    return {}; // TODO: Implement schema loading
}

std::vector<TableInfo> SchemaLoader::loadTables(const std::string& connectionId, const std::string& schema) {
    return {}; // TODO: Implement table loading
}

std::vector<ColumnInfo> SchemaLoader::loadColumns(const std::string& connectionId, const std::string& schema, const std::string& table) {
    return {}; // TODO: Implement column loading
}

void SchemaLoader::refreshCache(const std::string& connectionId) {
    // TODO: Implement cache refresh
}

} // namespace scratchrobin