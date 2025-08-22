#ifndef SCRATCHROBIN_SCHEMA_LOADER_H
#define SCRATCHROBIN_SCHEMA_LOADER_H

#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// Forward declarations for metadata structures
struct SchemaInfo {
    std::string name;
    std::string owner;
    std::string description;
    std::string created;
    std::string modified;
};

struct TableInfo {
    std::string schema;
    std::string name;
    std::string type;
    std::string owner;
    int64_t rowCount;
    std::string created;
    std::string modified;
    std::string description;
};

struct ColumnInfo {
    std::string schema;
    std::string table;
    std::string name;
    std::string type;
    int size;
    int precision;
    int scale;
    bool isNullable;
    std::string defaultValue;
    std::string description;
    bool isPrimaryKey;
    bool isForeignKey;
};

class SchemaLoader {
public:
    SchemaLoader();
    ~SchemaLoader();

    std::vector<SchemaInfo> loadSchemas(const std::string& connectionId);
    std::vector<TableInfo> loadTables(const std::string& connectionId, const std::string& schema);
    std::vector<ColumnInfo> loadColumns(const std::string& connectionId, const std::string& schema, const std::string& table);
    void refreshCache(const std::string& connectionId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SCHEMA_LOADER_H
