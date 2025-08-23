#include "metadata/schema_collector.h"
#include "core/connection_manager.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

namespace scratchrobin {

class SchemaCollector::Impl {
public:
    Impl(std::shared_ptr<IConnection> connection)
        : connection_(connection) {}

    DatabaseType detectDatabaseType() {
        try {
            auto result = connection_->executeQuery("SELECT VERSION()");
            if (result.isSuccess() && !result.value().empty()) {
                std::string version = result.value()[0].at("version");
                if (version.find("ScratchBird") != std::string::npos) {
                    return DatabaseType::SCRATCHBIRD;
                } else if (version.find("PostgreSQL") != std::string::npos) {
                    return DatabaseType::POSTGRESQL;
                } else if (version.find("MySQL") != std::string::npos) {
                    return DatabaseType::MYSQL;
                } else if (version.find("SQL Server") != std::string::npos) {
                    return DatabaseType::SQLSERVER;
                } else if (version.find("Oracle") != std::string::npos) {
                    return DatabaseType::ORACLE;
                }
            }
        } catch (...) {
            // Fallback to ScratchBird if version detection fails
        }
        return DatabaseType::SCRATCHBIRD;
    }

    std::string getDatabaseVersionString() {
        try {
            auto result = connection_->executeQuery("SELECT VERSION()");
            if (result.isSuccess() && !result.value().empty()) {
                return result.value()[0].at("version");
            }
        } catch (...) {
            // Return unknown if version query fails
        }
        return "Unknown";
    }

    Result<SchemaCollectionResult> collectScratchBirdSchema(const SchemaCollectionOptions& options) {
        SchemaCollectionResult result;
        result.collectedAt = std::chrono::system_clock::now();
        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            // Collect schemas
            if (shouldCollectType(SchemaObjectType::SCHEMA, options)) {
                auto schemas = collectSchemas(options);
                result.objects.insert(result.objects.end(), schemas.begin(), schemas.end());
            }

            // Collect tables
            if (shouldCollectType(SchemaObjectType::TABLE, options)) {
                auto tables = collectTables(options);
                result.objects.insert(result.objects.end(), tables.begin(), tables.end());
            }

            // Collect views
            if (shouldCollectType(SchemaObjectType::VIEW, options)) {
                auto views = collectViews(options);
                result.objects.insert(result.objects.end(), views.begin(), views.end());
            }

            // Collect columns
            if (shouldCollectType(SchemaObjectType::COLUMN, options)) {
                auto columns = collectColumns(options);
                result.objects.insert(result.objects.end(), columns.begin(), columns.end());
            }

            // Collect indexes
            if (shouldCollectType(SchemaObjectType::INDEX, options)) {
                auto indexes = collectIndexes(options);
                result.objects.insert(result.objects.end(), indexes.begin(), indexes.end());
            }

            // Collect constraints
            if (shouldCollectType(SchemaObjectType::CONSTRAINT, options)) {
                auto constraints = collectConstraints(options);
                result.objects.insert(result.objects.end(), constraints.begin(), constraints.end());
            }

            // Apply filtering and pagination
            applyFilteringAndPagination(result, options);

            // Resolve dependencies if requested
            if (options.followDependencies) {
                resolveObjectDependencies(result.objects);
            }



        } catch (const std::exception& e) {
            result.errors.push_back(std::string("Schema collection failed: ") + e.what());

        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.collectionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        result.totalCount = result.objects.size();

        return Result<SchemaCollectionResult>::success(result);
    }

    Result<SchemaCollectionResult> collectPostgreSQLSchema(const SchemaCollectionOptions& options) {
        // PostgreSQL-specific schema collection implementation
        // This would contain PostgreSQL-specific queries and logic
        return collectScratchBirdSchema(options); // Fallback for now
    }

    Result<SchemaCollectionResult> collectMySQLSchema(const SchemaCollectionOptions& options) {
        // MySQL-specific schema collection implementation
        return collectScratchBirdSchema(options); // Fallback for now
    }

    Result<SchemaCollectionResult> collectSQLServerSchema(const SchemaCollectionOptions& options) {
        // SQL Server-specific schema collection implementation
        return collectScratchBirdSchema(options); // Fallback for now
    }

    Result<SchemaCollectionResult> collectOracleSchema(const SchemaCollectionOptions& options) {
        // Oracle-specific schema collection implementation
        return collectScratchBirdSchema(options); // Fallback for now
    }

    Result<SchemaCollectionResult> collectSQLiteSchema(const SchemaCollectionOptions& options) {
        // SQLite-specific schema collection implementation
        return collectScratchBirdSchema(options); // Fallback for now
    }

private:
    std::shared_ptr<IConnection> connection_;

    bool shouldCollectType(SchemaObjectType type, const SchemaCollectionOptions& options) {
        if (!options.includedTypes.empty()) {
            return std::find(options.includedTypes.begin(), options.includedTypes.end(), type) != options.includedTypes.end();
        }
        if (!options.excludedTypes.empty()) {
            return std::find(options.excludedTypes.begin(), options.excludedTypes.end(), type) == options.excludedTypes.end();
        }
        return true;
    }

    std::vector<SchemaObject> collectSchemas(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> schemas;

        try {
            std::string query = R"(
                SELECT schema_name, schema_owner
                FROM information_schema.schemata
                WHERE schema_name NOT IN ('information_schema', 'pg_catalog')
                ORDER BY schema_name
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject schema;
                    schema.name = row.at("schema_name");
                    schema.schema = row.at("schema_name"); // Schema name is the same
                    schema.database = connection_->getDatabaseName();
                    schema.type = SchemaObjectType::SCHEMA;
                    schema.owner = row.at("schema_owner");
                    schema.createdAt = std::chrono::system_clock::now(); // Approximate
                    schema.isSystemObject = false;
                    schema.properties["owner"] = row.at("schema_owner");

                    if (shouldIncludeSchema(schema.schema, options)) {
                        schemas.push_back(schema);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting schemas: " << e.what() << std::endl;
        }

        return schemas;
    }

    std::vector<SchemaObject> collectTables(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> tables;

        try {
            std::string query = R"(
                SELECT
                    schemaname,
                    tablename,
                    tableowner,
                    tablespace,
                    hasindexes,
                    hasrules,
                    hastriggers,
                    rowsecurity
                FROM pg_tables
                WHERE schemaname NOT IN ('information_schema', 'pg_catalog')
                ORDER BY schemaname, tablename
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject table;
                    table.name = row.at("tablename");
                    table.schema = row.at("schemaname");
                    table.database = connection_->getDatabaseName();
                    table.type = SchemaObjectType::TABLE;
                    table.owner = row.at("tableowner");
                    table.createdAt = std::chrono::system_clock::now(); // Approximate
                    table.isSystemObject = false;
                    table.properties["tablespace"] = row.at("tablespace");
                    table.properties["hasindexes"] = row.at("hasindexes");
                    table.properties["hasrules"] = row.at("hasrules");
                    table.properties["hastriggers"] = row.at("hastriggers");
                    table.properties["rowsecurity"] = row.at("rowsecurity");

                    if (shouldIncludeSchema(table.schema, options)) {
                        tables.push_back(table);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting tables: " << e.what() << std::endl;
        }

        return tables;
    }

    std::vector<SchemaObject> collectViews(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> views;

        try {
            std::string query = R"(
                SELECT
                    schemaname,
                    viewname,
                    viewowner,
                    definition
                FROM pg_views
                WHERE schemaname NOT IN ('information_schema', 'pg_catalog')
                ORDER BY schemaname, viewname
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject view;
                    view.name = row.at("viewname");
                    view.schema = row.at("schemaname");
                    view.database = connection_->getDatabaseName();
                    view.type = SchemaObjectType::VIEW;
                    view.owner = row.at("viewowner");
                    view.createdAt = std::chrono::system_clock::now(); // Approximate
                    view.isSystemObject = false;
                    view.properties["definition"] = row.at("definition");

                    if (shouldIncludeSchema(view.schema, options)) {
                        views.push_back(view);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting views: " << e.what() << std::endl;
        }

        return views;
    }

    std::vector<SchemaObject> collectColumns(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> columns;

        try {
            std::string query = R"(
                SELECT
                    table_schema,
                    table_name,
                    column_name,
                    ordinal_position,
                    column_default,
                    is_nullable,
                    data_type,
                    character_maximum_length,
                    character_octet_length,
                    numeric_precision,
                    numeric_precision_radix,
                    numeric_scale,
                    datetime_precision,
                    interval_type,
                    interval_precision,
                    character_set_name,
                    collation_name,
                    domain_name,
                    udt_name,
                    is_identity,
                    identity_generation,
                    identity_start,
                    identity_increment,
                    identity_maximum,
                    identity_minimum,
                    identity_cycle,
                    is_generated,
                    generation_expression,
                    is_updatable
                FROM information_schema.columns
                WHERE table_schema NOT IN ('information_schema', 'pg_catalog')
                ORDER BY table_schema, table_name, ordinal_position
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject column;
                    column.name = row.at("column_name");
                    column.schema = row.at("table_schema");
                    column.database = connection_->getDatabaseName();
                    column.type = SchemaObjectType::COLUMN;
                    column.createdAt = std::chrono::system_clock::now(); // Approximate
                    column.isSystemObject = false;

                    // Set column properties
                    column.properties["table_name"] = row.at("table_name");
                    column.properties["ordinal_position"] = row.at("ordinal_position");
                    column.properties["column_default"] = row.at("column_default");
                    column.properties["is_nullable"] = row.at("is_nullable");
                    column.properties["data_type"] = row.at("data_type");
                    column.properties["character_maximum_length"] = row.at("character_maximum_length");
                    column.properties["character_octet_length"] = row.at("character_octet_length");
                    column.properties["numeric_precision"] = row.at("numeric_precision");
                    column.properties["numeric_precision_radix"] = row.at("numeric_precision_radix");
                    column.properties["numeric_scale"] = row.at("numeric_scale");
                    column.properties["datetime_precision"] = row.at("datetime_precision");
                    column.properties["interval_type"] = row.at("interval_type");
                    column.properties["interval_precision"] = row.at("interval_precision");
                    column.properties["character_set_name"] = row.at("character_set_name");
                    column.properties["collation_name"] = row.at("collation_name");
                    column.properties["domain_name"] = row.at("domain_name");
                    column.properties["udt_name"] = row.at("udt_name");
                    column.properties["is_identity"] = row.at("is_identity");
                    column.properties["identity_generation"] = row.at("identity_generation");
                    column.properties["identity_start"] = row.at("identity_start");
                    column.properties["identity_increment"] = row.at("identity_increment");
                    column.properties["identity_maximum"] = row.at("identity_maximum");
                    column.properties["identity_minimum"] = row.at("identity_minimum");
                    column.properties["identity_cycle"] = row.at("identity_cycle");
                    column.properties["is_generated"] = row.at("is_generated");
                    column.properties["generation_expression"] = row.at("generation_expression");
                    column.properties["is_updatable"] = row.at("is_updatable");

                    if (shouldIncludeSchema(column.schema, options)) {
                        columns.push_back(column);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting columns: " << e.what() << std::endl;
        }

        return columns;
    }

    std::vector<SchemaObject> collectIndexes(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> indexes;

        try {
            std::string query = R"(
                SELECT
                    schemaname,
                    tablename,
                    indexname,
                    tablespace,
                    indexdef
                FROM pg_indexes
                WHERE schemaname NOT IN ('information_schema', 'pg_catalog')
                ORDER BY schemaname, tablename, indexname
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject index;
                    index.name = row.at("indexname");
                    index.schema = row.at("schemaname");
                    index.database = connection_->getDatabaseName();
                    index.type = SchemaObjectType::INDEX;
                    index.createdAt = std::chrono::system_clock::now(); // Approximate
                    index.isSystemObject = false;
                    index.properties["table_name"] = row.at("tablename");
                    index.properties["tablespace"] = row.at("tablespace");
                    index.properties["indexdef"] = row.at("indexdef");

                    if (shouldIncludeSchema(index.schema, options)) {
                        indexes.push_back(index);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting indexes: " << e.what() << std::endl;
        }

        return indexes;
    }

    std::vector<SchemaObject> collectConstraints(const SchemaCollectionOptions& options) {
        std::vector<SchemaObject> constraints;

        try {
            std::string query = R"(
                SELECT
                    n.nspname as schema_name,
                    c.conname as constraint_name,
                    c.contype as constraint_type,
                    r.relname as table_name,
                    c.condeferrable as is_deferrable,
                    c.condeferred as is_deferred,
                    pg_get_constraintdef(c.oid) as constraint_def
                FROM pg_constraint c
                JOIN pg_namespace n ON n.oid = c.connamespace
                JOIN pg_class r ON r.oid = c.conrelid
                WHERE n.nspname NOT IN ('information_schema', 'pg_catalog')
                ORDER BY n.nspname, r.relname, c.conname
            )";

            auto result = connection_->executeQuery(query);
            if (result.isSuccess()) {
                for (const auto& row : result.value()) {
                    SchemaObject constraint;
                    constraint.name = row.at("constraint_name");
                    constraint.schema = row.at("constraint_schema");
                    constraint.database = connection_->getDatabaseName();
                    constraint.type = SchemaObjectType::CONSTRAINT;
                    constraint.createdAt = std::chrono::system_clock::now(); // Approximate
                    constraint.isSystemObject = false;
                    constraint.properties["constraint_type"] = row.at("constraint_type");
                    constraint.properties["table_name"] = row.at("table_name");
                    constraint.properties["is_deferrable"] = row.at("is_deferrable");
                    constraint.properties["is_deferred"] = row.at("is_deferred");
                    constraint.properties["constraint_def"] = row.at("constraint_def");

                    if (shouldIncludeSchema(constraint.schema, options)) {
                        constraints.push_back(constraint);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Log error but continue
            std::cerr << "Error collecting constraints: " << e.what() << std::endl;
        }

        return constraints;
    }

    bool shouldIncludeSchema(const std::string& schema, const SchemaCollectionOptions& options) {
        if (!options.includedSchemas.empty()) {
            return std::find(options.includedSchemas.begin(), options.includedSchemas.end(), schema) != options.includedSchemas.end();
        }
        if (!options.excludedSchemas.empty()) {
            return std::find(options.excludedSchemas.begin(), options.excludedSchemas.end(), schema) == options.excludedSchemas.end();
        }
        return true;
    }

    void applyFilteringAndPagination(SchemaCollectionResult& result, const SchemaCollectionOptions& options) {
        // Apply filtering
        if (!options.filterPattern.empty()) {
            std::vector<SchemaObject> filtered;
            std::regex pattern(options.filterPattern,
                             options.caseSensitive ? std::regex::ECMAScript : std::regex::icase);

            std::copy_if(result.objects.begin(), result.objects.end(), std::back_inserter(filtered),
                        [&](const SchemaObject& obj) {
                            return std::regex_search(obj.name, pattern) ||
                                   std::regex_search(obj.schema, pattern);
                        });

            result.objects = filtered;
        }

        // Apply pagination
        if (options.pageSize > 0) {
            int startIndex = options.pageNumber * options.pageSize;
            int endIndex = startIndex + options.pageSize;

            if (startIndex < static_cast<int>(result.objects.size())) {
                if (endIndex >= static_cast<int>(result.objects.size())) {
                    endIndex = result.objects.size();
                    result.hasMorePages = false;
                } else {
                    result.hasMorePages = true;
                }

                std::vector<SchemaObject> paginated(
                    result.objects.begin() + startIndex,
                    result.objects.begin() + endIndex);
                result.objects = paginated;
            } else {
                result.objects.clear();
                result.hasMorePages = false;
            }

            result.pageCount = (result.totalCount + options.pageSize - 1) / options.pageSize;
        }
    }

    void resolveObjectDependencies(std::vector<SchemaObject>& objects) {
        // Build a map of objects for quick lookup
        std::unordered_map<std::string, SchemaObject*> objectMap;
        for (auto& obj : objects) {
            std::string key = obj.schema + "." + obj.name + "." +
                            std::to_string(static_cast<int>(obj.type));
            objectMap[key] = &obj;
        }

        // Resolve dependencies (simplified implementation)
        // In a real implementation, this would query the database for actual dependencies
        for (auto& obj : objects) {
            switch (obj.type) {
                case SchemaObjectType::COLUMN:
                    // Columns depend on their tables
                    if (obj.properties.count("table_name")) {
                        std::string tableKey = obj.schema + "." + obj.properties["table_name"] + "." +
                                             std::to_string(static_cast<int>(SchemaObjectType::TABLE));
                        if (objectMap.count(tableKey)) {
                            obj.dependencies.push_back(obj.properties["table_name"]);
                        }
                    }
                    break;

                case SchemaObjectType::INDEX:
                    // Indexes depend on their tables
                    if (obj.properties.count("table_name")) {
                        std::string tableKey = obj.schema + "." + obj.properties["table_name"] + "." +
                                             std::to_string(static_cast<int>(SchemaObjectType::TABLE));
                        if (objectMap.count(tableKey)) {
                            obj.dependencies.push_back(obj.properties["table_name"]);
                        }
                    }
                    break;

                case SchemaObjectType::CONSTRAINT:
                    // Constraints depend on their tables
                    if (obj.properties.count("table_name")) {
                        std::string tableKey = obj.schema + "." + obj.properties["table_name"] + "." +
                                             std::to_string(static_cast<int>(SchemaObjectType::TABLE));
                        if (objectMap.count(tableKey)) {
                            obj.dependencies.push_back(obj.properties["table_name"]);
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }
};

// SchemaCollector implementation

SchemaCollector::SchemaCollector(std::shared_ptr<IConnection> connection)
    : impl_(std::make_unique<Impl>(connection)) {
    connection_ = connection;
    databaseType_ = impl_->detectDatabaseType();
    databaseVersion_ = impl_->getDatabaseVersionString();
}

SchemaCollector::~SchemaCollector() = default;

DatabaseType SchemaCollector::getDatabaseType() const {
    return databaseType_;
}

std::string SchemaCollector::getDatabaseVersion() const {
    return databaseVersion_;
}

Result<SchemaCollectionResult> SchemaCollector::collectSchema(
    const SchemaCollectionOptions& options) {

    switch (databaseType_) {
        case DatabaseType::SCRATCHBIRD:
            return impl_->collectScratchBirdSchema(options);
        case DatabaseType::POSTGRESQL:
            return impl_->collectPostgreSQLSchema(options);
        case DatabaseType::MYSQL:
            return impl_->collectMySQLSchema(options);
        case DatabaseType::SQLSERVER:
            return impl_->collectSQLServerSchema(options);
        case DatabaseType::ORACLE:
            return impl_->collectOracleSchema(options);
        case DatabaseType::SQLITE:
            return impl_->collectSQLiteSchema(options);
        default:
            return impl_->collectScratchBirdSchema(options);
    }
}

Result<SchemaObject> SchemaCollector::getObjectDetails(
    const std::string& schema, const std::string& name, SchemaObjectType type) {

    // This is a simplified implementation
    // In a real implementation, this would query the database for specific object details
    auto result = collectSchema();
    if (!result.isSuccess()) {
        return Result<SchemaObject>::error(result.error().message);
    }

    for (const auto& obj : result.value().objects) {
        if (obj.schema == schema && obj.name == name && obj.type == type) {
            return Result<SchemaObject>::success(obj);
        }
    }

            return Result<SchemaObject>::error("Object not found: " + schema + "." + name);
}

Result<std::vector<SchemaObject>> SchemaCollector::getObjectDependencies(
    const std::string& schema, const std::string& name, SchemaObjectType type) {

    auto objResult = getObjectDetails(schema, name, type);
    if (!objResult.isSuccess()) {
        return Result<std::vector<SchemaObject>>::error(objResult.error().message);
    }

    std::vector<SchemaObject> dependencies;
    // In a real implementation, this would resolve actual dependencies
    // For now, return empty vector
    return Result<std::vector<SchemaObject>>::success(dependencies);
}

Result<std::vector<SchemaObject>> SchemaCollector::getObjectDependents(
    const std::string& schema, const std::string& name, SchemaObjectType type) {

    auto objResult = getObjectDetails(schema, name, type);
    if (!objResult.isSuccess()) {
        return Result<std::vector<SchemaObject>>::error(objResult.error().message);
    }

    std::vector<SchemaObject> dependents;
    // In a real implementation, this would resolve actual dependents
    // For now, return empty vector
    return Result<std::vector<SchemaObject>>::success(dependents);
}

Result<bool> SchemaCollector::objectExists(
    const std::string& schema, const std::string& name, SchemaObjectType type) {

    auto result = getObjectDetails(schema, name, type);
    return Result<bool>::success(result.isSuccess());
}

Result<std::chrono::system_clock::time_point> SchemaCollector::getObjectLastModified(
    const std::string& schema, const std::string& name, SchemaObjectType type) {

    auto result = getObjectDetails(schema, name, type);
    if (!result.isSuccess()) {
        return Result<std::chrono::system_clock::time_point>::error(result.error().message);
    }

    return Result<std::chrono::system_clock::time_point>::success(result.value().modifiedAt);
}

Result<void> SchemaCollector::refreshSchemaCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    schemaCache_.clear();
    lastCacheRefresh_ = std::chrono::system_clock::now();
    return Result<void>::success();
}

Result<SchemaCollectionResult> SchemaCollector::getCachedSchema(
    const SchemaCollectionOptions& options) {

    std::string cacheKey = generateCacheKey(options);

    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = schemaCache_.find(cacheKey);
    if (it != schemaCache_.end() && isCacheValid(options)) {
        return Result<SchemaCollectionResult>::success(it->second);
    }

    // Cache miss - collect fresh data
    auto result = collectSchema(options);
    if (result.isSuccess()) {
        updateCache(result.value(), options);
    }

    return result;
}

void SchemaCollector::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void SchemaCollector::setObjectFilter(ObjectFilter filter) {
    objectFilter_ = filter;
}

std::string SchemaCollector::generateCacheKey(const SchemaCollectionOptions& options) {
    std::stringstream ss;
    ss << options.pageSize << "_" << options.pageNumber << "_"
       << options.filterPattern << "_" << options.caseSensitive << "_"
       << options.includeSystemObjects << "_" << options.includeTemporaryObjects;

    for (const auto& schema : options.includedSchemas) {
        ss << "_" << schema;
    }

    for (const auto& schema : options.excludedSchemas) {
        ss << "_x" << schema;
    }

    for (const auto& type : options.includedTypes) {
        ss << "_t" << static_cast<int>(type);
    }

    return ss.str();
}

bool SchemaCollector::isCacheValid(const SchemaCollectionOptions& options) {
    // Simple cache validity check - cache is valid for 5 minutes
    auto now = std::chrono::system_clock::now();
    auto cacheAge = now - lastCacheRefresh_;
    return cacheAge < std::chrono::minutes(5);
}

void SchemaCollector::updateCache(const SchemaCollectionResult& result, const SchemaCollectionOptions& options) {
    std::string cacheKey = generateCacheKey(options);
    schemaCache_[cacheKey] = result;
}

void SchemaCollector::cleanupExpiredCache() {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(30); // Remove cache entries older than 30 minutes

    std::lock_guard<std::mutex> lock(cacheMutex_);
    for (auto it = schemaCache_.begin(); it != schemaCache_.end();) {
        if (it->second.collectedAt < cutoff) {
            it = schemaCache_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace scratchrobin
