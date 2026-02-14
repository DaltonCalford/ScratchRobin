/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "metadata_model.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include "core/simple_json.h"

namespace scratchrobin {
namespace {

constexpr const char* kDefaultFixturePath = "config/fixtures/default.json";

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = ToLowerCopy(Trim(raw));
    if (value.empty() || value == "network" || value == "scratchbird") {
        return "native";
    }
    if (value == "postgres" || value == "pg") {
        return "postgresql";
    }
    if (value == "mariadb") {
        return "mysql";
    }
    if (value == "fb") {
        return "firebird";
    }
    return value;
}

bool IsExternalBackend(const std::string& backend) {
    return backend == "postgresql" || backend == "mysql" || backend == "firebird";
}

std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '.' || c == '/' || c == '\\') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

std::string LastPathSegment(const std::string& path) {
    auto parts = SplitPath(path);
    if (parts.empty()) {
        return path;
    }
    return parts.back();
}

bool ParseStringArray(const JsonValue& value, std::vector<std::string>* out, std::string* error) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Array) {
        if (error) {
            *error = "Expected string array";
        }
        return false;
    }
    out->clear();
    for (const auto& entry : value.array_value) {
        if (entry.type != JsonValue::Type::String) {
            if (error) {
                *error = "Array entry must be a string";
            }
            return false;
        }
        out->push_back(entry.string_value);
    }
    return true;
}

bool GetRowValue(const std::vector<QueryValue>& row, size_t index, std::string* out) {
    if (!out || index >= row.size()) {
        return false;
    }
    if (row[index].isNull) {
        out->clear();
        return false;
    }
    *out = Trim(row[index].text);
    return !out->empty();
}

bool ParseMetadataNode(const JsonValue& value, MetadataNode* out, std::string* error) {
    if (!out) {
        return false;
    }
    if (value.type != JsonValue::Type::Object) {
        if (error) {
            *error = "Metadata node must be an object";
        }
        return false;
    }

    MetadataNode node;
    if (const JsonValue* label = FindMember(value, "label")) {
        if (label->type == JsonValue::Type::String) {
            node.label = label->string_value;
        }
    }
    if (const JsonValue* path = FindMember(value, "path")) {
        if (path->type == JsonValue::Type::String) {
            node.path = path->string_value;
        }
    }
    if (node.label.empty() && !node.path.empty()) {
        node.label = LastPathSegment(node.path);
    }
    if (node.label.empty()) {
        if (error) {
            *error = "Metadata node missing label";
        }
        return false;
    }

    if (const JsonValue* kind = FindMember(value, "kind")) {
        if (kind->type == JsonValue::Type::String) {
            node.kind = kind->string_value;
        }
    }
    if (const JsonValue* catalog = FindMember(value, "catalog")) {
        if (catalog->type == JsonValue::Type::String) {
            node.catalog = catalog->string_value;
        }
    }
    if (const JsonValue* ddl = FindMember(value, "ddl")) {
        if (ddl->type == JsonValue::Type::String) {
            node.ddl = ddl->string_value;
        }
    }
    if (const JsonValue* deps = FindMember(value, "dependencies")) {
        if (!ParseStringArray(*deps, &node.dependencies, error)) {
            return false;
        }
    }
    if (const JsonValue* children = FindMember(value, "children")) {
        if (children->type != JsonValue::Type::Array) {
            if (error) {
                *error = "children must be an array";
            }
            return false;
        }
        for (const auto& child_value : children->array_value) {
            MetadataNode child;
            if (!ParseMetadataNode(child_value, &child, error)) {
                return false;
            }
            node.children.push_back(std::move(child));
        }
    }

    *out = std::move(node);
    return true;
}

std::string SanitizePathSegment(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') {
            out.push_back(static_cast<char>(ch));
        } else if (ch == '.' || ch == ' ' || ch == '/' || ch == '\\') {
            out.push_back('_');
        }
    }
    return out.empty() ? "item" : out;
}

void AddNodeByPath(std::vector<MetadataNode>& roots, const std::string& path, MetadataNode node) {
    auto parts = SplitPath(path);
    if (parts.empty()) {
        roots.push_back(std::move(node));
        return;
    }

    std::vector<MetadataNode>* current = &roots;
    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& part = parts[i];
        auto it = std::find_if(current->begin(), current->end(),
                               [&](const MetadataNode& candidate) {
                                   return candidate.label == part;
                               });
        if (it == current->end()) {
            MetadataNode intermediate;
            intermediate.label = part;
            intermediate.kind = (i == 0) ? "catalog" : "path";
            intermediate.catalog = node.catalog;
            current->push_back(std::move(intermediate));
            it = std::prev(current->end());
        }

        if (i + 1 == parts.size()) {
            if (!node.label.empty()) {
                it->label = node.label;
            }
            if (!node.kind.empty()) {
                it->kind = node.kind;
            }
            if (!node.catalog.empty()) {
                it->catalog = node.catalog;
            }
            if (!node.path.empty()) {
                it->path = node.path;
            }
            if (!node.ddl.empty()) {
                it->ddl = node.ddl;
            }
            if (!node.dependencies.empty()) {
                it->dependencies = node.dependencies;
            }
            if (!node.children.empty()) {
                it->children.insert(it->children.end(),
                                    node.children.begin(), node.children.end());
            }
        } else {
            current = &it->children;
        }
    }
}

void AddSchemaNode(std::vector<MetadataNode>& roots,
                   const std::string& catalog,
                   const std::string& schema) {
    if (schema.empty()) {
        return;
    }
    MetadataNode node;
    node.label = schema;
    node.kind = "schema";
    node.catalog = catalog;
    node.path = catalog + "." + schema;
    AddNodeByPath(roots, node.path, std::move(node));
}

void AddTableNode(std::vector<MetadataNode>& roots,
                  const std::string& catalog,
                  const std::string& schema,
                  const std::string& table,
                  const std::string& kind) {
    if (schema.empty() || table.empty()) {
        return;
    }
    MetadataNode node;
    node.label = table;
    node.kind = kind.empty() ? "table" : kind;
    node.catalog = catalog;
    node.path = catalog + "." + schema + "." + table;
    AddNodeByPath(roots, node.path, std::move(node));
}

void AddColumnNode(std::vector<MetadataNode>& roots,
                   const std::string& catalog,
                   const std::string& schema,
                   const std::string& table,
                   const std::string& column) {
    if (schema.empty() || table.empty() || column.empty()) {
        return;
    }
    MetadataNode node;
    node.label = column;
    node.kind = "column";
    node.catalog = catalog;
    node.path = catalog + "." + schema + "." + table + "." + column;
    AddNodeByPath(roots, node.path, std::move(node));
}

void AddErrorNode(std::vector<MetadataNode>& roots,
                  const std::string& catalog,
                  const std::string& label,
                  const std::string& message,
                  int index) {
    std::string key = SanitizePathSegment(label);
    MetadataNode node;
    node.label = label.empty() ? "Error" : "Error: " + label;
    node.kind = "error";
    node.catalog = catalog;
    node.ddl = message;
    node.path = catalog + ".errors." + key + "_" + std::to_string(index);
    AddNodeByPath(roots, node.path, std::move(node));
}

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.database.empty()) {
        return profile.database;
    }
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) {
        label += ":" + std::to_string(profile.port);
    }
    return label;
}

bool ExecuteMetadataQuery(ConnectionManager& manager,
                          const std::string& sql,
                          QueryResult* result,
                          std::string* error) {
    if (!result) {
        if (error) {
            *error = "No result buffer for metadata query";
        }
        return false;
    }
    if (!manager.ExecuteQuery(sql, result)) {
        if (error) {
            *error = manager.LastError().empty() ? "Metadata query failed" : manager.LastError();
        }
        return false;
    }
    return true;
}

bool LoadPostgresMetadata(ConnectionManager& manager,
                          const std::string& catalog,
                          MetadataSnapshot* snapshot,
                          std::string* error) {
    if (!snapshot) {
        if (error) {
            *error = "Missing metadata snapshot";
        }
        return false;
    }

    QueryResult result;
    if (!ExecuteMetadataQuery(manager,
                              "SELECT nspname FROM pg_namespace ORDER BY nspname;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        if (!GetRowValue(row, 0, &schema)) {
            continue;
        }
        AddSchemaNode(snapshot->roots, catalog, schema);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT table_schema, table_name, table_type "
                              "FROM information_schema.tables "
                              "ORDER BY table_schema, table_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string type;
        if (!GetRowValue(row, 0, &schema) || !GetRowValue(row, 1, &table)) {
            continue;
        }
        GetRowValue(row, 2, &type);
        std::string type_lower = ToLowerCopy(type);
        std::string kind = type_lower.find("view") != std::string::npos ? "view" : "table";
        AddTableNode(snapshot->roots, catalog, schema, table, kind);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT table_schema, table_name, column_name, data_type "
                              "FROM information_schema.columns "
                              "ORDER BY table_schema, table_name, ordinal_position;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string column;
        if (!GetRowValue(row, 0, &schema) ||
            !GetRowValue(row, 1, &table) ||
            !GetRowValue(row, 2, &column)) {
            continue;
        }
        AddColumnNode(snapshot->roots, catalog, schema, table, column);
    }

    return true;
}

bool LoadMySqlMetadata(ConnectionManager& manager,
                       const std::string& catalog,
                       MetadataSnapshot* snapshot,
                       std::string* error) {
    if (!snapshot) {
        if (error) {
            *error = "Missing metadata snapshot";
        }
        return false;
    }

    QueryResult result;
    if (!ExecuteMetadataQuery(manager,
                              "SELECT schema_name FROM information_schema.schemata "
                              "ORDER BY schema_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        if (!GetRowValue(row, 0, &schema)) {
            continue;
        }
        AddSchemaNode(snapshot->roots, catalog, schema);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT table_schema, table_name, table_type "
                              "FROM information_schema.tables "
                              "ORDER BY table_schema, table_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string type;
        if (!GetRowValue(row, 0, &schema) || !GetRowValue(row, 1, &table)) {
            continue;
        }
        GetRowValue(row, 2, &type);
        std::string type_lower = ToLowerCopy(type);
        std::string kind = type_lower.find("view") != std::string::npos ? "view" : "table";
        AddTableNode(snapshot->roots, catalog, schema, table, kind);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT table_schema, table_name, column_name, column_type "
                              "FROM information_schema.columns "
                              "ORDER BY table_schema, table_name, ordinal_position;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string column;
        if (!GetRowValue(row, 0, &schema) ||
            !GetRowValue(row, 1, &table) ||
            !GetRowValue(row, 2, &column)) {
            continue;
        }
        AddColumnNode(snapshot->roots, catalog, schema, table, column);
    }

    return true;
}

bool LoadFirebirdMetadata(ConnectionManager& manager,
                          const std::string& catalog,
                          MetadataSnapshot* snapshot,
                          std::string* error) {
    if (!snapshot) {
        if (error) {
            *error = "Missing metadata snapshot";
        }
        return false;
    }

    const std::string schema = "public";
    AddSchemaNode(snapshot->roots, catalog, schema);

    QueryResult result;
    if (!ExecuteMetadataQuery(manager,
                              "SELECT rdb$relation_name, rdb$view_blr "
                              "FROM rdb$relations "
                              "WHERE rdb$system_flag = 0 "
                              "ORDER BY rdb$relation_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string relation;
        if (!GetRowValue(row, 0, &relation)) {
            continue;
        }
        bool is_view = false;
        if (row.size() > 1 && !row[1].isNull) {
            is_view = true;
        }
        AddTableNode(snapshot->roots, catalog, schema, relation, is_view ? "view" : "table");
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT rf.rdb$relation_name, rf.rdb$field_name "
                              "FROM rdb$relation_fields rf "
                              "JOIN rdb$relations r ON rf.rdb$relation_name = r.rdb$relation_name "
                              "WHERE r.rdb$system_flag = 0 "
                              "ORDER BY rf.rdb$relation_name, rf.rdb$field_position;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string relation;
        std::string column;
        if (!GetRowValue(row, 0, &relation) || !GetRowValue(row, 1, &column)) {
            continue;
        }
        AddColumnNode(snapshot->roots, catalog, schema, relation, column);
    }

    return true;
}

bool LoadScratchBirdMetadata(ConnectionManager& manager,
                             const std::string& catalog,
                             MetadataSnapshot* snapshot,
                             std::string* error) {
    if (!snapshot) {
        if (error) {
            *error = "Missing metadata snapshot";
        }
        return false;
    }

    QueryResult result;
    if (!ExecuteMetadataQuery(manager,
                              "SELECT schema_name "
                              "FROM sys.schemas "
                              "WHERE is_valid = 1 "
                              "ORDER BY schema_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        if (!GetRowValue(row, 0, &schema)) {
            continue;
        }
        AddSchemaNode(snapshot->roots, catalog, schema);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT s.schema_name, t.table_name, t.table_type "
                              "FROM sys.tables t "
                              "JOIN sys.schemas s ON t.schema_id = s.schema_id "
                              "WHERE t.is_valid = 1 "
                              "ORDER BY s.schema_name, t.table_name;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string type;
        if (!GetRowValue(row, 0, &schema) || !GetRowValue(row, 1, &table)) {
            continue;
        }
        GetRowValue(row, 2, &type);
        std::string type_lower = ToLowerCopy(type);
        std::string kind = type_lower.find("view") != std::string::npos ? "view" : "table";
        AddTableNode(snapshot->roots, catalog, schema, table, kind);
    }

    if (!ExecuteMetadataQuery(manager,
                              "SELECT s.schema_name, t.table_name, c.column_name, c.ordinal_position "
                              "FROM sys.columns c "
                              "JOIN sys.tables t ON c.table_id = t.table_id "
                              "JOIN sys.schemas s ON t.schema_id = s.schema_id "
                              "WHERE c.is_valid = 1 "
                              "ORDER BY s.schema_name, t.table_name, c.ordinal_position;",
                              &result, error)) {
        return false;
    }
    for (const auto& row : result.rows) {
        std::string schema;
        std::string table;
        std::string column;
        if (!GetRowValue(row, 0, &schema) ||
            !GetRowValue(row, 1, &table) ||
            !GetRowValue(row, 2, &column)) {
            continue;
        }
        AddColumnNode(snapshot->roots, catalog, schema, table, column);
    }

    return true;
}

void AppendConnectionsRoot(MetadataSnapshot* snapshot,
                           const std::vector<ConnectionProfile>& profiles) {
    if (!snapshot || profiles.empty()) {
        return;
    }

    MetadataNode root;
    root.label = "Connections";
    root.kind = "root";

    for (const auto& profile : profiles) {
        MetadataNode entry;
        entry.label = profile.name.empty() ? profile.database : profile.name;
        entry.kind = "connection";
        entry.catalog = NormalizeBackendName(profile.backend);
        if (!profile.host.empty() || profile.port != 0) {
            std::string host_label = "Host: " + (profile.host.empty() ? "localhost" : profile.host);
            if (profile.port != 0) {
                host_label += ":" + std::to_string(profile.port);
            }
            entry.children.push_back(MetadataNode{host_label, "host", entry.catalog, "", "", {}, {}});
        }
        if (!profile.database.empty()) {
            entry.children.push_back(MetadataNode{"Database: " + profile.database, "database",
                                                  entry.catalog, "", "", {}, {}});
        }
        if (!profile.username.empty()) {
            entry.children.push_back(MetadataNode{"User: " + profile.username, "user",
                                                  entry.catalog, "", "", {}, {}});
        }
        root.children.push_back(std::move(entry));
    }

    snapshot->roots.insert(snapshot->roots.begin(), std::move(root));
}

} // namespace

void MetadataModel::AddObserver(MetadataObserver* observer) {
    if (!observer) {
        return;
    }
    if (std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) {
        return;
    }
    observers_.push_back(observer);
}

void MetadataModel::RemoveObserver(MetadataObserver* observer) {
    if (!observer) {
        return;
    }
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
}

void MetadataModel::LoadStub() {
    MetadataSnapshot snapshot;
    MetadataNode root;
    root.label = "Connections";
    root.kind = "root";
    root.catalog = "native";

    MetadataNode local;
    local.label = "Local ScratchBird";
    local.kind = "connection";
    local.catalog = "native";
    local.children.push_back(MetadataNode{"Host: 127.0.0.1:3050", "host", "native", "", "", {}, {}});
    local.children.push_back(MetadataNode{"Database: /data/scratchbird/demo.sdb", "database", "native",
                                          "", "", {}, {}});

    MetadataNode schema;
    schema.label = "Schema: public";
    schema.kind = "schema";
    schema.catalog = "native";

    MetadataNode table;
    table.label = "Table: demo";
    table.kind = "table";
    table.catalog = "native";
    table.path = "native.public.demo";
    table.ddl = "CREATE TABLE public.demo (\n"
                "    id BIGINT PRIMARY KEY,\n"
                "    name VARCHAR(64) NOT NULL,\n"
                "    created_at TIMESTAMPTZ DEFAULT now()\n"
                ");";
    table.dependencies = {
        "Index: demo_pkey",
        "Sequence: demo_id_seq"
    };
    schema.children.push_back(std::move(table));
    local.children.push_back(std::move(schema));
    root.children.push_back(std::move(local));

    snapshot.roots.push_back(std::move(root));
    snapshot_ = std::move(snapshot);
    last_error_.clear();
    NotifyObservers();
}

void MetadataModel::UpdateConnections(const std::vector<ConnectionProfile>& profiles) {
    profiles_ = profiles;
    MetadataSnapshot snapshot;
    AppendConnectionsRoot(&snapshot, profiles_);
    snapshot_ = std::move(snapshot);
    last_error_.clear();
    NotifyObservers();
}

const MetadataSnapshot& MetadataModel::GetSnapshot() const {
    return snapshot_;
}

void MetadataModel::SetFixturePath(const std::string& path) {
    fixture_path_ = Trim(path);
}

bool MetadataModel::LoadFromFixture(const std::string& path, std::string* error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        if (error) {
            *error = "Unable to open metadata fixture: " + path;
        }
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string contents = buffer.str();

    JsonParser parser(contents);
    JsonValue root;
    std::string parse_error;
    if (!parser.Parse(&root, &parse_error)) {
        if (error) {
            *error = "Metadata fixture parse error: " + parse_error;
        }
        return false;
    }

    const JsonValue* metadata = FindMember(root, "metadata");
    const JsonValue* nodes = nullptr;
    if (metadata) {
        if (metadata->type == JsonValue::Type::Array) {
            nodes = metadata;
        } else if (metadata->type == JsonValue::Type::Object) {
            nodes = FindMember(*metadata, "nodes");
        }
    } else {
        nodes = FindMember(root, "nodes");
    }

    if (!nodes || nodes->type != JsonValue::Type::Array) {
        if (error) {
            *error = "Metadata fixture missing metadata nodes";
        }
        return false;
    }

    MetadataSnapshot snapshot;
    for (const auto& node_value : nodes->array_value) {
        MetadataNode node;
        std::string node_error;
        if (!ParseMetadataNode(node_value, &node, &node_error)) {
            if (error) {
                *error = node_error.empty() ? "Invalid metadata node" : node_error;
            }
            return false;
        }
        snapshot.nodes.push_back(node);
        if (!node.path.empty()) {
            AddNodeByPath(snapshot.roots, node.path, std::move(node));
        } else {
            snapshot.roots.push_back(std::move(node));
        }
    }

    AppendConnectionsRoot(&snapshot, profiles_);
    snapshot_ = std::move(snapshot);
    last_error_.clear();
    NotifyObservers();
    return true;
}

void MetadataModel::Refresh() {
    std::string error;
    if (!fixture_path_.empty()) {
        if (LoadFromFixture(fixture_path_, &error)) {
            return;
        }
    }

    if (LoadFromConnections(&error)) {
        return;
    }

    std::string fallback_path = kDefaultFixturePath;
    if (LoadFromFixture(fallback_path, &error)) {
        return;
    }

    LoadFallback(error.empty() ? "Metadata refresh failed" : error);
}

void MetadataModel::LoadFallback(const std::string& message) {
    MetadataSnapshot snapshot;
    MetadataNode root;
    root.label = "Metadata";
    root.kind = "root";
    MetadataNode error_node;
    error_node.label = message;
    error_node.kind = "error";
    root.children.push_back(std::move(error_node));
    snapshot.roots.push_back(std::move(root));
    AppendConnectionsRoot(&snapshot, profiles_);
    snapshot_ = std::move(snapshot);
    last_error_ = message;
    NotifyObservers();
}

void MetadataModel::NotifyObservers() {
    for (auto* observer : observers_) {
        if (observer) {
            observer->OnMetadataUpdated(snapshot_);
        }
    }
}

bool MetadataModel::LoadFromConnections(std::string* error) {
    MetadataSnapshot snapshot;
    bool loaded_any = false;
    bool attempted_any = false;
    std::string aggregated_error;
    int error_index = 0;

    for (const auto& profile : profiles_) {
        std::string backend = NormalizeBackendName(profile.backend);
        if (backend == "mock") {
            continue;
        }
        if (!IsExternalBackend(backend) && backend != "native") {
            continue;
        }

        attempted_any = true;
        ConnectionManager manager;
        if (!manager.Connect(profile)) {
            std::string message = manager.LastError().empty()
                                      ? "Connection failed"
                                      : manager.LastError();
            AddErrorNode(snapshot.roots, backend, ProfileLabel(profile), message, ++error_index);
            if (!aggregated_error.empty()) {
                aggregated_error += "\n";
            }
            aggregated_error += backend + ": " + message;
            continue;
        }

        std::string load_error;
        bool ok = false;
        if (backend == "native") {
            ok = LoadScratchBirdMetadata(manager, backend, &snapshot, &load_error);
        } else if (backend == "postgresql") {
            ok = LoadPostgresMetadata(manager, backend, &snapshot, &load_error);
        } else if (backend == "mysql") {
            ok = LoadMySqlMetadata(manager, backend, &snapshot, &load_error);
        } else if (backend == "firebird") {
            ok = LoadFirebirdMetadata(manager, backend, &snapshot, &load_error);
        }

        manager.Disconnect();

        if (!ok) {
            std::string message = load_error.empty() ? "Metadata query failed" : load_error;
            AddErrorNode(snapshot.roots, backend, ProfileLabel(profile), message, ++error_index);
            if (!aggregated_error.empty()) {
                aggregated_error += "\n";
            }
            aggregated_error += backend + ": " + message;
            continue;
        }
        loaded_any = true;
    }

    if (!attempted_any) {
        if (error) {
            *error = "No live metadata sources configured";
        }
        return false;
    }

    AppendConnectionsRoot(&snapshot, profiles_);
    snapshot_ = std::move(snapshot);
    last_error_.clear();
    NotifyObservers();
    return true;
}

void MetadataModel::UpdateNode(const MetadataNode& node) {
    // Update or add the node
    bool found = false;
    for (auto& existing : snapshot_.nodes) {
        if (existing.id == node.id) {
            existing = node;
            found = true;
            break;
        }
    }
    if (!found) {
        snapshot_.nodes.push_back(node);
    }
    snapshot_.timestamp = std::chrono::system_clock::now();
    NotifyObservers();
}

void MetadataModel::RemoveNode(int id) {
    snapshot_.nodes.erase(
        std::remove_if(snapshot_.nodes.begin(), snapshot_.nodes.end(),
            [id](const MetadataNode& node) { return node.id == id; }),
        snapshot_.nodes.end()
    );
    snapshot_.timestamp = std::chrono::system_clock::now();
    NotifyObservers();
}

std::optional<MetadataNode> MetadataModel::FindNodeByPath(const std::string& path) const {
    for (const auto& node : snapshot_.nodes) {
        if (node.path == path) {
            return node;
        }
    }
    return std::nullopt;
}

std::vector<MetadataNode> MetadataModel::FindNodesByType(MetadataType type) const {
    std::vector<MetadataNode> result;
    for (const auto& node : snapshot_.nodes) {
        if (node.type == type) {
            result.push_back(node);
        }
    }
    return result;
}

void MetadataModel::Clear() {
    snapshot_.nodes.clear();
    snapshot_.roots.clear();
    snapshot_.timestamp = std::chrono::system_clock::now();
    NotifyObservers();
}

} // namespace scratchrobin
