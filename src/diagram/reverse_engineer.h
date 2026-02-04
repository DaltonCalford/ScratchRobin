/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_REVERSE_ENGINEER_H
#define SCRATCHROBIN_DIAGRAM_REVERSE_ENGINEER_H

#include "layout_engine.h"
#include "../ui/diagram_model.h"
#include "../core/connection_manager.h"

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {
namespace diagram {

// Import options for reverse engineering
struct ReverseEngineerOptions {
    std::string schema_filter;              // Schema to import (empty = all)
    std::vector<std::string> table_filter;  // Specific tables to import
    bool include_indexes = false;           // Include index information
    bool include_constraints = true;        // Include constraints
    bool include_comments = true;           // Include table/column comments
    bool auto_layout = true;                // Apply auto-layout after import
    LayoutAlgorithm layout_algorithm = LayoutAlgorithm::Sugiyama;
};

// Progress callback
using ImportProgressCallback = std::function<void(const std::string& table_name, int current, int total)>;

// Reverse engineer database schema into diagram
class ReverseEngineer {
public:
    ReverseEngineer(ConnectionManager* connection_manager,
                    const ConnectionProfile* profile);
    
    // Import schema to diagram model
    bool ImportToDiagram(DiagramModel* model,
                         const ReverseEngineerOptions& options,
                         ImportProgressCallback progress = nullptr);
    
    // Get available schemas
    std::vector<std::string> GetSchemas();
    
    // Get tables in a schema
    std::vector<std::string> GetTables(const std::string& schema);

private:
    ConnectionManager* connection_manager_;
    const ConnectionProfile* profile_;
    
    // Import methods for different backends
    bool ImportScratchBird(DiagramModel* model,
                           const ReverseEngineerOptions& options,
                           ImportProgressCallback progress);
    bool ImportPostgreSQL(DiagramModel* model,
                          const ReverseEngineerOptions& options,
                          ImportProgressCallback progress);
    bool ImportMySQL(DiagramModel* model,
                     const ReverseEngineerOptions& options,
                     ImportProgressCallback progress);
    bool ImportFirebird(DiagramModel* model,
                        const ReverseEngineerOptions& options,
                        ImportProgressCallback progress);
    
    // Helper methods
    bool ExecuteQuery(const std::string& sql, QueryResult& result);
    DiagramNode CreateNodeFromTable(const std::string& schema,
                                     const std::string& table,
                                     const QueryResult& columns);
    DiagramEdge CreateEdgeFromForeignKey(const std::string& source_table,
                                          const std::string& target_table,
                                          const QueryResult& fk_info);
};

// Schema comparison for incremental updates
struct SchemaDifference {
    enum class ChangeType {
        Added,
        Removed,
        Modified
    };
    
    ChangeType type;
    std::string object_type;  // "table", "column", "index", etc.
    std::string object_name;
    std::string details;
};

class SchemaComparator {
public:
    // Compare current diagram with database schema
    std::vector<SchemaDifference> Compare(const DiagramModel& model,
                                          ConnectionManager* connection_manager,
                                          const ConnectionProfile* profile);
    
    // Apply differences to diagram
    bool ApplyDifferences(DiagramModel* model,
                          const std::vector<SchemaDifference>& differences);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_REVERSE_ENGINEER_H
