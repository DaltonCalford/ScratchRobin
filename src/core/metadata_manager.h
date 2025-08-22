#ifndef SCRATCHROBIN_METADATA_MANAGER_H
#define SCRATCHROBIN_METADATA_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace scratchrobin {

class ConnectionManager;

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
    std::string type; // TABLE, VIEW, etc.
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

struct IndexInfo {
    std::string schema;
    std::string table;
    std::string name;
    std::string type; // BTREE, HASH, etc.
    bool isUnique;
    std::vector<std::string> columns;
    int64_t sizeBytes;
};

struct ConstraintInfo {
    std::string schema;
    std::string table;
    std::string name;
    std::string type; // PRIMARY KEY, FOREIGN KEY, UNIQUE, CHECK
    std::vector<std::string> columns;
    std::string definition;
};

class MetadataManager {
public:
    explicit MetadataManager(ConnectionManager* connectionManager);
    ~MetadataManager();

    // Schema operations
    std::vector<SchemaInfo> getSchemas(const std::string& connectionId);
    bool createSchema(const std::string& connectionId, const std::string& schemaName, const std::string& description = "");
    bool dropSchema(const std::string& connectionId, const std::string& schemaName);

    // Table operations
    std::vector<TableInfo> getTables(const std::string& connectionId, const std::string& schema = "");
    TableInfo getTableInfo(const std::string& connectionId, const std::string& schema, const std::string& table);
    bool createTable(const std::string& connectionId, const std::string& schema, const std::string& table, const std::vector<ColumnInfo>& columns);
    bool dropTable(const std::string& connectionId, const std::string& schema, const std::string& table);

    // Column operations
    std::vector<ColumnInfo> getColumns(const std::string& connectionId, const std::string& schema, const std::string& table);
    bool addColumn(const std::string& connectionId, const std::string& schema, const std::string& table, const ColumnInfo& column);
    bool dropColumn(const std::string& connectionId, const std::string& schema, const std::string& table, const std::string& column);
    bool alterColumn(const std::string& connectionId, const std::string& schema, const std::string& table, const ColumnInfo& column);

    // Index operations
    std::vector<IndexInfo> getIndexes(const std::string& connectionId, const std::string& schema = "", const std::string& table = "");
    bool createIndex(const std::string& connectionId, const IndexInfo& index);
    bool dropIndex(const std::string& connectionId, const std::string& schema, const std::string& index);

    // Constraint operations
    std::vector<ConstraintInfo> getConstraints(const std::string& connectionId, const std::string& schema = "", const std::string& table = "");
    bool addConstraint(const std::string& connectionId, const ConstraintInfo& constraint);
    bool dropConstraint(const std::string& connectionId, const std::string& schema, const std::string& table, const std::string& constraint);

    // Cache management
    void clearCache(const std::string& connectionId = "");
    void refreshCache(const std::string& connectionId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy and assignment
    MetadataManager(const MetadataManager&) = delete;
    MetadataManager& operator=(const MetadataManager&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_METADATA_MANAGER_H
