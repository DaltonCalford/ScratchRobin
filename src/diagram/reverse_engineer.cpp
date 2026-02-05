/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "reverse_engineer.h"

#include <algorithm>
#include <future>
#include <map>
#include <set>

namespace scratchrobin {
namespace diagram {

ReverseEngineer::ReverseEngineer(ConnectionManager* connection_manager,
                                  const ConnectionProfile* profile)
    : connection_manager_(connection_manager), profile_(profile) {}

bool ReverseEngineer::ImportToDiagram(DiagramModel* model,
                                       const ReverseEngineerOptions& options,
                                       ImportProgressCallback progress) {
    if (!connection_manager_ || !profile_) {
        return false;
    }
    
    // Route to backend-specific importer
    if (profile_->backend == "scratchbird") {
        return ImportScratchBird(model, options, progress);
    } else if (profile_->backend == "postgresql" || profile_->backend == "postgres") {
        return ImportPostgreSQL(model, options, progress);
    } else if (profile_->backend == "mysql" || profile_->backend == "mariadb") {
        return ImportMySQL(model, options, progress);
    } else if (profile_->backend == "firebird") {
        return ImportFirebird(model, options, progress);
    }
    
    // Default to generic SQL approach
    return ImportPostgreSQL(model, options, progress);
}

bool ReverseEngineer::ImportScratchBird(DiagramModel* model,
                                         const ReverseEngineerOptions& options,
                                         ImportProgressCallback progress) {
    // Query tables from sb_catalog
    std::string table_sql = "SELECT schema_name, name FROM sb_catalog.sb_tables";
    if (!options.schema_filter.empty()) {
        table_sql += " WHERE schema_name = '" + options.schema_filter + "'";
    }
    table_sql += " ORDER BY schema_name, name";
    
    QueryResult tables_result;
    if (!ExecuteQuery(table_sql, tables_result)) {
        return false;
    }
    
    int total = tables_result.rows.size();
    int current = 0;
    
    // Import each table
    for (const auto& row : tables_result.rows) {
        if (row.size() < 2) continue;
        
        std::string schema = row[0].isNull ? "" : row[0].text;
        std::string table = row[1].isNull ? "" : row[1].text;
        
        if (progress) {
            progress(table, ++current, total);
        }
        
        // Check if table is in filter
        if (!options.table_filter.empty() &&
            std::find(options.table_filter.begin(), options.table_filter.end(), table) 
            == options.table_filter.end()) {
            continue;
        }
        
        // Get columns
        std::string col_sql = "SELECT column_name, data_type, is_nullable, is_primary_key "
                              "FROM sb_catalog.sb_columns "
                              "WHERE schema_name = '" + schema + "' "
                              "AND table_name = '" + table + "' "
                              "ORDER BY ordinal_position";
        
        QueryResult columns_result;
        if (!ExecuteQuery(col_sql, columns_result)) {
            continue;
        }
        
        // Create node
        DiagramNode node = CreateNodeFromTable(schema, table, columns_result);
        model->AddNode(node);
    }
    
    // Import foreign keys as relationships
    std::string fk_sql = "SELECT tc.table_name, kcu.column_name, "
                        "ccu.table_name AS foreign_table, ccu.column_name AS foreign_column "
                        "FROM sb_catalog.sb_constraints tc "
                        "JOIN sb_catalog.sb_constraint_columns kcu ON tc.name = kcu.constraint_name "
                        "JOIN sb_catalog.sb_constraint_columns ccu ON tc.name = ccu.constraint_name "
                        "WHERE tc.constraint_type = 'FOREIGN KEY'";
    
    if (!options.schema_filter.empty()) {
        fk_sql += " AND tc.schema_name = '" + options.schema_filter + "'";
    }
    
    QueryResult fk_result;
    if (ExecuteQuery(fk_sql, fk_result)) {
        for (const auto& row : fk_result.rows) {
            if (row.size() < 4) continue;
            
            std::string source_table = row[0].isNull ? "" : row[0].text;
            std::string target_table = row[2].isNull ? "" : row[2].text;
            
            // Find nodes
            auto& nodes = model->nodes();
            auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                          [&source_table](const DiagramNode& n) {
                                              return n.name == source_table;
                                          });
            auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                          [&target_table](const DiagramNode& n) {
                                              return n.name == target_table;
                                          });
            
            if (source_it != nodes.end() && target_it != nodes.end()) {
                DiagramEdge edge;
                edge.id = "edge_" + std::to_string(model->NextEdgeIndex());
                edge.source_id = source_it->id;
                edge.target_id = target_it->id;
                edge.target_cardinality = Cardinality::ZeroOrMany;
                model->AddEdge(edge);
            }
        }
    }
    
    // Apply auto-layout if requested
    if (options.auto_layout) {
        LayoutOptions layout_options;
        layout_options.algorithm = options.layout_algorithm;
        auto engine = LayoutEngine::Create(options.layout_algorithm);
        auto positions = engine->Layout(*model, layout_options);
        
        for (const auto& node_pos : positions) {
            auto& nodes = model->nodes();
            auto it = std::find_if(nodes.begin(), nodes.end(),
                                   [&node_pos](const DiagramNode& n) { return n.id == node_pos.node_id; });
            if (it != nodes.end()) {
                it->x = node_pos.x;
                it->y = node_pos.y;
            }
        }
    }
    
    return true;
}

bool ReverseEngineer::ImportPostgreSQL(DiagramModel* model,
                                        const ReverseEngineerOptions& options,
                                        ImportProgressCallback progress) {
    // PostgreSQL-specific import queries
    std::string table_sql = "SELECT schemaname, tablename FROM pg_tables "
                           "WHERE schemaname NOT IN ('pg_catalog', 'information_schema')";
    if (!options.schema_filter.empty()) {
        table_sql += " AND schemaname = '" + options.schema_filter + "'";
    }
    
    QueryResult tables_result;
    if (!ExecuteQuery(table_sql, tables_result)) {
        return false;
    }
    
    int total = tables_result.rows.size();
    int current = 0;
    
    for (const auto& row : tables_result.rows) {
        if (row.size() < 2) continue;
        
        std::string schema = row[0].isNull ? "" : row[0].text;
        std::string table = row[1].isNull ? "" : row[1].text;
        
        if (progress) {
            progress(table, ++current, total);
        }
        
        // Get columns
        std::string col_sql = "SELECT column_name, data_type, is_nullable "
                              "FROM information_schema.columns "
                              "WHERE table_schema = '" + schema + "' "
                              "AND table_name = '" + table + "' "
                              "ORDER BY ordinal_position";
        
        QueryResult columns_result;
        if (!ExecuteQuery(col_sql, columns_result)) {
            continue;
        }
        
        DiagramNode node = CreateNodeFromTable(schema, table, columns_result);
        model->AddNode(node);
    }
    
    // Import foreign keys
    std::string fk_sql = "SELECT kcu.table_name, kcu.column_name, "
                        "ccu.table_name AS foreign_table_name, ccu.column_name AS foreign_column_name "
                        "FROM information_schema.table_constraints AS tc "
                        "JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name "
                        "JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name "
                        "WHERE constraint_type = 'FOREIGN KEY'";
    
    QueryResult fk_result;
    if (ExecuteQuery(fk_sql, fk_result)) {
        for (const auto& row : fk_result.rows) {
            if (row.size() < 4) continue;
            
            std::string source_table = row[0].isNull ? "" : row[0].text;
            std::string target_table = row[2].isNull ? "" : row[2].text;
            
            auto& nodes = model->nodes();
            auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                          [&source_table](const DiagramNode& n) {
                                              return n.name == source_table;
                                          });
            auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                          [&target_table](const DiagramNode& n) {
                                              return n.name == target_table;
                                          });
            
            if (source_it != nodes.end() && target_it != nodes.end()) {
                DiagramEdge edge;
                edge.id = "edge_" + std::to_string(model->NextEdgeIndex());
                edge.source_id = source_it->id;
                edge.target_id = target_it->id;
                edge.target_cardinality = Cardinality::ZeroOrMany;
                model->AddEdge(edge);
            }
        }
    }
    
    if (options.auto_layout) {
        LayoutOptions layout_options;
        layout_options.algorithm = options.layout_algorithm;
        auto engine = LayoutEngine::Create(options.layout_algorithm);
        auto positions = engine->Layout(*model, layout_options);
        
        for (const auto& node_pos : positions) {
            auto& nodes = model->nodes();
            auto it = std::find_if(nodes.begin(), nodes.end(),
                                   [&node_pos](const DiagramNode& n) { return n.id == node_pos.node_id; });
            if (it != nodes.end()) {
                it->x = node_pos.x;
                it->y = node_pos.y;
            }
        }
    }
    
    return true;
}

bool ReverseEngineer::ImportMySQL(DiagramModel* model,
                                   const ReverseEngineerOptions& options,
                                   ImportProgressCallback progress) {
    // MySQL-specific queries (similar structure to PostgreSQL)
    std::string table_sql = "SELECT TABLE_SCHEMA, TABLE_NAME FROM INFORMATION_SCHEMA.TABLES "
                           "WHERE TABLE_TYPE = 'BASE TABLE'";
    if (!options.schema_filter.empty()) {
        table_sql += " AND TABLE_SCHEMA = '" + options.schema_filter + "'";
    } else {
        table_sql += " AND TABLE_SCHEMA NOT IN ('information_schema', 'mysql', 'performance_schema', 'sys')";
    }
    
    QueryResult tables_result;
    if (!ExecuteQuery(table_sql, tables_result)) {
        return false;
    }
    
    int total = tables_result.rows.size();
    int current = 0;
    
    for (const auto& row : tables_result.rows) {
        if (row.size() < 2) continue;
        
        std::string schema = row[0].isNull ? "" : row[0].text;
        std::string table = row[1].isNull ? "" : row[1].text;
        
        if (progress) {
            progress(table, ++current, total);
        }
        
        std::string col_sql = "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE "
                              "FROM INFORMATION_SCHEMA.COLUMNS "
                              "WHERE TABLE_SCHEMA = '" + schema + "' "
                              "AND TABLE_NAME = '" + table + "' "
                              "ORDER BY ORDINAL_POSITION";
        
        QueryResult columns_result;
        if (!ExecuteQuery(col_sql, columns_result)) {
            continue;
        }
        
        DiagramNode node = CreateNodeFromTable(schema, table, columns_result);
        model->AddNode(node);
    }
    
    return true;
}

bool ReverseEngineer::ImportFirebird(DiagramModel* model,
                                      const ReverseEngineerOptions& options,
                                      ImportProgressCallback progress) {
    // Firebird-specific queries
    std::string table_sql = "SELECT rdb$relation_name FROM rdb$relations "
                           "WHERE rdb$view_blr IS NULL AND rdb$relation_name NOT STARTING WITH 'RDB$'";
    
    QueryResult tables_result;
    if (!ExecuteQuery(table_sql, tables_result)) {
        return false;
    }
    
    int total = tables_result.rows.size();
    int current = 0;
    
    for (const auto& row : tables_result.rows) {
        if (row.empty()) continue;
        
        std::string table = row[0].isNull ? "" : row[0].text;
        // Trim whitespace from Firebird identifiers
        table.erase(0, table.find_first_not_of(" \t\r\n"));
        table.erase(table.find_last_not_of(" \t\r\n") + 1);
        
        if (progress) {
            progress(table, ++current, total);
        }
        
        std::string col_sql = "SELECT rdb$field_name, rdb$field_type "
                              "FROM rdb$relation_fields "
                              "WHERE rdb$relation_name = '" + table + "' "
                              "ORDER BY rdb$field_position";
        
        QueryResult columns_result;
        if (!ExecuteQuery(col_sql, columns_result)) {
            continue;
        }
        
        DiagramNode node;
        node.id = "node_" + std::to_string(model->NextNodeIndex());
        node.name = table;
        node.type = "table";
        
        for (const auto& col_row : columns_result.rows) {
            if (col_row.empty()) continue;
            
            DiagramAttribute attr;
            attr.name = col_row[0].isNull ? "" : col_row[0].text;
            // Trim
            attr.name.erase(0, attr.name.find_first_not_of(" \t\r\n"));
            attr.name.erase(attr.name.find_last_not_of(" \t\r\n") + 1);
            
            attr.data_type = col_row.size() > 1 && !col_row[1].isNull ? col_row[1].text : "UNKNOWN";
            node.attributes.push_back(attr);
        }
        
        model->AddNode(node);
    }
    
    return true;
}

bool ReverseEngineer::ExecuteQuery(const std::string& sql, QueryResult& result) {
    if (!connection_manager_) {
        return false;
    }
    
    std::promise<bool> promise;
    auto future = promise.get_future();
    
    connection_manager_->ExecuteQueryAsync(sql, 
        [&promise, &result](bool ok, QueryResult res, const std::string& error) {
            result = std::move(res);
            promise.set_value(ok);
        });
    
    return future.get();
}

DiagramNode ReverseEngineer::CreateNodeFromTable(const std::string& schema,
                                                  const std::string& table,
                                                  const QueryResult& columns) {
    DiagramNode node;
    node.id = "node_" + table;
    node.name = table;
    node.type = "table";
    
    for (const auto& row : columns.rows) {
        if (row.empty()) continue;
        
        DiagramAttribute attr;
        attr.name = row[0].isNull ? "" : row[0].text;
        attr.data_type = row.size() > 1 && !row[1].isNull ? row[1].text : "UNKNOWN";
        
        if (row.size() > 2 && !row[2].isNull) {
            attr.is_primary = (row[2].text == "YES" || row[2].text == "true");
        }
        
        node.attributes.push_back(attr);
    }
    
    return node;
}

// Helper function to execute query synchronously using async API
static bool ExecuteQuerySync(ConnectionManager* cm, const std::string& sql, QueryResult& result) {
    if (!cm) return false;
    
    std::promise<bool> promise;
    auto future = promise.get_future();
    
    cm->ExecuteQueryAsync(sql, [&promise, &result](bool ok, QueryResult res, const std::string&) {
        result = std::move(res);
        promise.set_value(ok);
    });
    
    return future.get();
}

// SchemaComparator implementation
std::vector<SchemaDifference> SchemaComparator::Compare(
    const DiagramModel& model,
    ConnectionManager* connection_manager,
    const ConnectionProfile* profile) {
    
    std::vector<SchemaDifference> differences;
    
    if (!connection_manager || !profile) {
        return differences;
    }
    
    // Build a set of tables in the diagram model
    std::map<std::string, const DiagramNode*> model_tables;
    for (const auto& node : model.nodes()) {
        model_tables[node.name] = &node;
    }
    
    // Query database tables based on backend
    std::string tables_sql;
    if (profile->backend == "postgresql" || profile->backend == "postgres") {
        tables_sql = "SELECT table_schema, table_name FROM information_schema.tables "
                     "WHERE table_type = 'BASE TABLE' AND table_schema NOT IN ('pg_catalog', 'information_schema')";
    } else if (profile->backend == "mysql" || profile->backend == "mariadb") {
        tables_sql = "SELECT table_schema, table_name FROM information_schema.tables "
                     "WHERE table_type = 'BASE TABLE' AND table_schema NOT IN ('mysql', 'information_schema', 'performance_schema', 'sys')";
    } else {
        tables_sql = "SELECT schema_name, name FROM sb_catalog.sb_tables";
    }
    
    QueryResult db_tables;
    if (ExecuteQuerySync(connection_manager, tables_sql, db_tables)) {
        std::set<std::string> db_table_names;
        
        for (size_t i = 1; i < db_tables.rows.size(); ++i) {
            const auto& row = db_tables.rows[i];
            std::string schema = row.size() > 0 && !row[0].isNull ? row[0].text : "";
            std::string table = row.size() > 1 && !row[1].isNull ? row[1].text : "";
            std::string full_name = schema.empty() ? table : schema + "." + table;
            db_table_names.insert(full_name);
            
            // Check if table exists in model
            auto it = model_tables.find(full_name);
            if (it == model_tables.end()) {
                // Table exists in DB but not in model
                SchemaDifference diff;
                diff.type = SchemaDifference::ChangeType::Added;
                diff.object_type = "table";
                diff.object_name = full_name;
                diff.details = "Table exists in database but not in diagram";
                differences.push_back(diff);
            } else {
                // Table exists in both - compare columns
                const DiagramNode* node = it->second;
                
                // Query columns for this table
                std::string cols_sql;
                if (profile->backend == "postgresql" || profile->backend == "postgres") {
                    cols_sql = "SELECT column_name, data_type FROM information_schema.columns "
                               "WHERE table_schema = '" + schema + "' AND table_name = '" + table + "'";
                } else if (profile->backend == "mysql" || profile->backend == "mariadb") {
                    cols_sql = "SELECT column_name, data_type FROM information_schema.columns "
                               "WHERE table_schema = '" + schema + "' AND table_name = '" + table + "'";
                } else {
                    cols_sql = "SELECT name, data_type FROM sb_catalog.sb_columns "
                               "WHERE schema_name = '" + schema + "' AND table_name = '" + table + "'";
                }
                
                QueryResult db_columns;
                if (ExecuteQuerySync(connection_manager, cols_sql, db_columns)) {
                    std::map<std::string, std::string> db_cols;
                    for (size_t j = 1; j < db_columns.rows.size(); ++j) {
                        const auto& col_row = db_columns.rows[j];
                        std::string col_name = col_row.size() > 0 && !col_row[0].isNull ? col_row[0].text : "";
                        std::string col_type = col_row.size() > 1 && !col_row[1].isNull ? col_row[1].text : "";
                        db_cols[col_name] = col_type;
                    }
                    
                    // Compare columns
                    for (const auto& attr : node->attributes) {
                        auto col_it = db_cols.find(attr.name);
                        if (col_it == db_cols.end()) {
                            // Column in model but not in DB
                            SchemaDifference diff;
                            diff.type = SchemaDifference::ChangeType::Removed;
                            diff.object_type = "column";
                            diff.object_name = full_name + "." + attr.name;
                            diff.details = "Column exists in diagram but not in database";
                            differences.push_back(diff);
                        } else if (col_it->second != attr.data_type) {
                            // Column type differs
                            SchemaDifference diff;
                            diff.type = SchemaDifference::ChangeType::Modified;
                            diff.object_type = "column";
                            diff.object_name = full_name + "." + attr.name;
                            diff.details = "Column type differs: diagram=" + attr.data_type + ", database=" + col_it->second;
                            differences.push_back(diff);
                        }
                    }
                    
                    // Check for columns in DB but not in model
                    std::set<std::string> model_col_names;
                    for (const auto& attr : node->attributes) {
                        model_col_names.insert(attr.name);
                    }
                    for (const auto& [col_name, col_type] : db_cols) {
                        if (model_col_names.find(col_name) == model_col_names.end()) {
                            SchemaDifference diff;
                            diff.type = SchemaDifference::ChangeType::Added;
                            diff.object_type = "column";
                            diff.object_name = full_name + "." + col_name;
                            diff.details = "Column exists in database but not in diagram: " + col_type;
                            differences.push_back(diff);
                        }
                    }
                }
            }
        }
        
        // Check for tables in model but not in DB
        for (const auto& [name, node] : model_tables) {
            if (db_table_names.find(name) == db_table_names.end()) {
                SchemaDifference diff;
                diff.type = SchemaDifference::ChangeType::Removed;
                diff.object_type = "table";
                diff.object_name = name;
                diff.details = "Table exists in diagram but not in database";
                differences.push_back(diff);
            }
        }
    }
    
    return differences;
}

bool SchemaComparator::ApplyDifferences(DiagramModel* model,
                                        const std::vector<SchemaDifference>& differences) {
    if (!model) {
        return false;
    }
    
    for (const auto& diff : differences) {
        switch (diff.type) {
            case SchemaDifference::ChangeType::Added:
                if (diff.object_type == "table") {
                    // Add table to diagram (create a placeholder node)
                    DiagramNode new_node;
                    new_node.name = diff.object_name;
                    new_node.type = "entity";
                    new_node.x = 100.0;  // Default position
                    new_node.y = 100.0;
                    model->AddNode(new_node);
                } else if (diff.object_type == "column") {
                    // Add column to existing table
                    size_t dot_pos = diff.object_name.rfind('.');
                    if (dot_pos != std::string::npos) {
                        std::string table_name = diff.object_name.substr(0, dot_pos);
                        std::string col_name = diff.object_name.substr(dot_pos + 1);
                        
                        for (auto& node : model->nodes()) {
                            if (node.name == table_name) {
                                DiagramAttribute attr;
                                attr.name = col_name;
                                attr.data_type = "UNKNOWN";
                                node.attributes.push_back(attr);
                                break;
                            }
                        }
                    }
                }
                break;
                
            case SchemaDifference::ChangeType::Removed:
                if (diff.object_type == "table") {
                    // Remove table from diagram
                    for (auto it = model->nodes().begin(); it != model->nodes().end(); ++it) {
                        if (it->name == diff.object_name) {
                            model->nodes().erase(it);
                            break;
                        }
                    }
                } else if (diff.object_type == "column") {
                    // Remove column from table
                    size_t dot_pos = diff.object_name.rfind('.');
                    if (dot_pos != std::string::npos) {
                        std::string table_name = diff.object_name.substr(0, dot_pos);
                        std::string col_name = diff.object_name.substr(dot_pos + 1);
                        
                        for (auto& node : model->nodes()) {
                            if (node.name == table_name) {
                                auto& attrs = node.attributes;
                                attrs.erase(std::remove_if(attrs.begin(), attrs.end(),
                                    [&col_name](const DiagramAttribute& a) { return a.name == col_name; }),
                                    attrs.end());
                                break;
                            }
                        }
                    }
                }
                break;
                
            case SchemaDifference::ChangeType::Modified:
                // For modified columns, update the type
                if (diff.object_type == "column") {
                    // Parse the details to get new type
                    size_t db_pos = diff.details.find("database=");
                    if (db_pos != std::string::npos) {
                        std::string new_type = diff.details.substr(db_pos + 9);
                        
                        size_t dot_pos = diff.object_name.rfind('.');
                        if (dot_pos != std::string::npos) {
                            std::string table_name = diff.object_name.substr(0, dot_pos);
                            std::string col_name = diff.object_name.substr(dot_pos + 1);
                            
                            for (auto& node : model->nodes()) {
                                if (node.name == table_name) {
                                    for (auto& attr : node.attributes) {
                                        if (attr.name == col_name) {
                                            attr.data_type = new_type;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
        }
    }
    
    return true;
}

} // namespace diagram
} // namespace scratchrobin
