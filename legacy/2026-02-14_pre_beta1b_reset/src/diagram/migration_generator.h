/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MIGRATION_GENERATOR_H
#define SCRATCHROBIN_MIGRATION_GENERATOR_H

#include "ui/diagram_model.h"

#include <string>
#include <vector>

namespace scratchrobin {

namespace diagram {

// Migration operation types
enum class MigrationOperationType {
    CreateTable,
    AlterTable,
    DropTable,
    AddColumn,
    DropColumn,
    AlterColumn,
    AddIndex,
    DropIndex,
    AddConstraint,
    DropConstraint
};

// Single migration operation
struct MigrationOperation {
    MigrationOperationType type;
    std::string table_name;
    std::string object_name;  // Column, index, or constraint name
    std::string old_definition;
    std::string new_definition;
    std::string sql;
};

// Migration script generator
class MigrationGenerator {
public:
    // Generate migration operations by comparing diagram with database
    std::vector<MigrationOperation> GenerateMigration(
        const DiagramModel& current_model,
        const DiagramModel& target_model,
        const std::string& dialect = "scratchbird");
    
    // Generate SQL script from operations
    std::string GenerateSqlScript(const std::vector<MigrationOperation>& operations,
                                   const std::string& dialect = "scratchbird");
    
    // Generate upgrade and downgrade scripts
    struct MigrationScripts {
        std::string upgrade_script;
        std::string downgrade_script;
        std::vector<std::string> warnings;
    };
    
    MigrationScripts GenerateFullMigration(
        const DiagramModel& database_model,
        const DiagramModel& diagram_model,
        const std::string& dialect = "scratchbird");

private:
    std::string GenerateCreateTableSql(const DiagramNode& node,
                                        const std::string& dialect);
    std::string GenerateAlterTableSql(const MigrationOperation& op,
                                       const std::string& dialect);
    std::string GenerateDropTableSql(const std::string& table_name,
                                      const std::string& dialect);
    std::string GenerateAddColumnSql(const std::string& table,
                                      const DiagramAttribute& attr,
                                      const std::string& dialect);
    std::string GenerateDropColumnSql(const std::string& table,
                                       const std::string& column,
                                       const std::string& dialect);
    std::string GenerateAlterColumnSql(const std::string& table,
                                        const std::string& column,
                                        const std::string& new_type,
                                        const std::string& dialect);
    
    std::string QuoteIdentifier(const std::string& name,
                                 const std::string& dialect);
    std::string MapDataType(const std::string& type,
                            const std::string& dialect);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_MIGRATION_GENERATOR_H
