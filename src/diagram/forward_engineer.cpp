/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "forward_engineer.h"

#include <algorithm>
#include <sstream>

namespace scratchrobin {
namespace diagram {

// Factory method
std::unique_ptr<DdlGenerator> DdlGenerator::Create(const std::string& backend) {
    if (backend == "postgresql" || backend == "postgres") {
        return std::make_unique<PostgreSqlDdlGenerator>();
    } else if (backend == "mysql" || backend == "mariadb") {
        return std::make_unique<MySqlDdlGenerator>();
    } else if (backend == "firebird") {
        return std::make_unique<FirebirdDdlGenerator>();
    } else if (backend == "scratchbird") {
        return std::make_unique<ScratchBirdDdlGenerator>();
    }
    return std::make_unique<ScratchBirdDdlGenerator>();
}

// ==================== ScratchBird DDL Generator ====================

std::string ScratchBirdDdlGenerator::GenerateDdl(const DiagramModel& model,
                                                  const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    ddl << "-- Generated DDL for ScratchBird\n";
    ddl << "-- Diagram: ERD\n\n";
    
    // Create schema
    ddl << "CREATE SCHEMA IF NOT EXISTS " << QuoteIdentifier(options.schema_name) << ";\n\n";
    
    // Create tables
    for (const auto& node : model.nodes()) {
        ddl << GenerateTableDdl(node, options) << "\n";
    }
    
    // Create foreign keys
    if (options.include_constraints) {
        ddl << "\n-- Foreign Key Constraints\n";
        for (const auto& edge : model.edges()) {
            ddl << GenerateForeignKeyDdl(edge, model, options) << "\n";
        }
    }
    
    return ddl.str();
}

std::string ScratchBirdDdlGenerator::GenerateTableDdl(const DiagramNode& node,
                                                       const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    if (options.drop_existing) {
        ddl << "DROP TABLE IF EXISTS " << QuoteIdentifier(node.name) << ";\n";
    }
    
    ddl << "CREATE TABLE ";
    if (options.create_if_not_exists) {
        ddl << "IF NOT EXISTS ";
    }
    ddl << QuoteIdentifier(node.name) << " (\n";
    
    // Columns
    bool first = true;
    std::vector<std::string> pk_columns;
    
    for (const auto& attr : node.attributes) {
        if (!first) ddl << ",\n";
        first = false;
        
        ddl << "    " << QuoteIdentifier(attr.name) << " ";
        ddl << MapDataType(attr.data_type);
        
        if (!attr.is_nullable) {
            ddl << " NOT NULL";
        }
        
        if (attr.is_primary) {
            pk_columns.push_back(attr.name);
        }
    }
    
    // Primary key
    if (!pk_columns.empty() && options.include_constraints) {
        ddl << ",\n    PRIMARY KEY (";
        for (size_t i = 0; i < pk_columns.size(); ++i) {
            if (i > 0) ddl << ", ";
            ddl << QuoteIdentifier(pk_columns[i]);
        }
        ddl << ")";
    }
    
    ddl << "\n);\n";
    
    // Comments
    if (options.include_comments && !node.name.empty()) {
        ddl << "COMMENT ON TABLE " << QuoteIdentifier(node.name) 
            << " IS 'Generated from ERD';\n";
    }
    
    return ddl.str();
}

std::string ScratchBirdDdlGenerator::GenerateForeignKeyDdl(const DiagramEdge& edge,
                                                            const DiagramModel& model,
                                                            const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    // Find source and target nodes
    auto& nodes = model.nodes();
    auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
    auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
    
    if (source_it == nodes.end() || target_it == nodes.end()) {
        return "";
    }
    
    std::string fk_name = "fk_" + source_it->name + "_" + target_it->name;
    
    ddl << "ALTER TABLE " << QuoteIdentifier(target_it->name) << "\n";
    ddl << "    ADD CONSTRAINT " << QuoteIdentifier(fk_name) << "\n";
    ddl << "    FOREIGN KEY (" << QuoteIdentifier(source_it->name + "_id") << ")\n";
    ddl << "    REFERENCES " << QuoteIdentifier(source_it->name) << " (id);\n";
    
    return ddl.str();
}

std::string ScratchBirdDdlGenerator::QuoteIdentifier(const std::string& name) {
    return "\"" + name + "\"";
}

std::string ScratchBirdDdlGenerator::MapDataType(const std::string& type) {
    // ScratchBird types are already in the correct format
    return type.empty() ? "TEXT" : type;
}

std::string ScratchBirdDdlGenerator::GenerateColumnDdl(const DiagramAttribute& attr) {
    std::stringstream ddl;
    ddl << QuoteIdentifier(attr.name) << " " << MapDataType(attr.data_type);
    if (!attr.is_nullable) {
        ddl << " NOT NULL";
    }
    return ddl.str();
}

// ==================== PostgreSQL DDL Generator ====================

std::string PostgreSqlDdlGenerator::GenerateDdl(const DiagramModel& model,
                                                 const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    ddl << "-- Generated DDL for PostgreSQL\n\n";
    
    ddl << "CREATE SCHEMA IF NOT EXISTS " << QuoteIdentifier(options.schema_name) << ";\n\n";
    
    for (const auto& node : model.nodes()) {
        ddl << GenerateTableDdl(node, options) << "\n";
    }
    
    if (options.include_constraints) {
        ddl << "\n-- Foreign Key Constraints\n";
        for (const auto& edge : model.edges()) {
            ddl << GenerateForeignKeyDdl(edge, model, options) << "\n";
        }
    }
    
    return ddl.str();
}

std::string PostgreSqlDdlGenerator::GenerateTableDdl(const DiagramNode& node,
                                                      const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    if (options.drop_existing) {
        ddl << "DROP TABLE IF EXISTS " << QuoteIdentifier(options.schema_name) << "." 
            << QuoteIdentifier(node.name) << " CASCADE;\n";
    }
    
    ddl << "CREATE TABLE " << QuoteIdentifier(options.schema_name) << "." 
        << QuoteIdentifier(node.name) << " (\n";
    
    bool first = true;
    std::vector<std::string> pk_columns;
    
    for (const auto& attr : node.attributes) {
        if (!first) ddl << ",\n";
        first = false;
        
        ddl << "    " << QuoteIdentifier(attr.name) << " ";
        ddl << MapDataType(attr.data_type);
        
        if (!attr.is_nullable) {
            ddl << " NOT NULL";
        }
        
        if (attr.is_primary) {
            pk_columns.push_back(attr.name);
        }
    }
    
    if (!pk_columns.empty() && options.include_constraints) {
        ddl << ",\n    PRIMARY KEY (";
        for (size_t i = 0; i < pk_columns.size(); ++i) {
            if (i > 0) ddl << ", ";
            ddl << QuoteIdentifier(pk_columns[i]);
        }
        ddl << ")";
    }
    
    ddl << "\n);\n";
    
    return ddl.str();
}

std::string PostgreSqlDdlGenerator::GenerateForeignKeyDdl(const DiagramEdge& edge,
                                                           const DiagramModel& model,
                                                           const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    auto& nodes = model.nodes();
    auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
    auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
    
    if (source_it == nodes.end() || target_it == nodes.end()) {
        return "";
    }
    
    std::string fk_name = "fk_" + source_it->name + "_" + target_it->name;
    
    ddl << "ALTER TABLE " << QuoteIdentifier(options.schema_name) << "." 
        << QuoteIdentifier(target_it->name) << "\n";
    ddl << "    ADD CONSTRAINT " << QuoteIdentifier(fk_name) << "\n";
    ddl << "    FOREIGN KEY (" << QuoteIdentifier(source_it->name + "_id") << ")\n";
    ddl << "    REFERENCES " << QuoteIdentifier(options.schema_name) << "." 
        << QuoteIdentifier(source_it->name) << " (id);\n";
    
    return ddl.str();
}

std::string PostgreSqlDdlGenerator::QuoteIdentifier(const std::string& name) {
    return "\"" + name + "\"";
}

std::string PostgreSqlDdlGenerator::MapDataType(const std::string& type) {
    // Map generic types to PostgreSQL
    if (type == "INT" || type == "INTEGER") return "INTEGER";
    if (type == "BIGINT") return "BIGINT";
    if (type == "STRING" || type == "TEXT") return "TEXT";
    if (type == "VARCHAR") return "VARCHAR(255)";
    if (type == "BOOL" || type == "BOOLEAN") return "BOOLEAN";
    if (type == "FLOAT" || type == "REAL") return "REAL";
    if (type == "DOUBLE") return "DOUBLE PRECISION";
    if (type == "DECIMAL" || type == "NUMERIC") return "NUMERIC(18,2)";
    if (type == "DATE") return "DATE";
    if (type == "DATETIME" || type == "TIMESTAMP") return "TIMESTAMP";
    if (type == "UUID") return "UUID";
    if (type == "JSON") return "JSONB";
    if (type == "BLOB" || type == "BYTEA") return "BYTEA";
    return type.empty() ? "TEXT" : type;
}

std::string PostgreSqlDdlGenerator::GenerateColumnDdl(const DiagramAttribute& attr) {
    std::stringstream ddl;
    ddl << QuoteIdentifier(attr.name) << " " << MapDataType(attr.data_type);
    if (!attr.is_nullable) {
        ddl << " NOT NULL";
    }
    return ddl.str();
}

// ==================== MySQL DDL Generator ====================

std::string MySqlDdlGenerator::GenerateDdl(const DiagramModel& model,
                                            const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    ddl << "-- Generated DDL for MySQL\n\n";
    ddl << "USE " << QuoteIdentifier(options.schema_name) << ";\n\n";
    
    for (const auto& node : model.nodes()) {
        ddl << GenerateTableDdl(node, options) << "\n";
    }
    
    if (options.include_constraints) {
        ddl << "\n-- Foreign Key Constraints\n";
        for (const auto& edge : model.edges()) {
            ddl << GenerateForeignKeyDdl(edge, model, options) << "\n";
        }
    }
    
    return ddl.str();
}

std::string MySqlDdlGenerator::GenerateTableDdl(const DiagramNode& node,
                                                 const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    if (options.drop_existing) {
        ddl << "DROP TABLE IF EXISTS " << QuoteIdentifier(node.name) << ";\n";
    }
    
    ddl << "CREATE TABLE " << QuoteIdentifier(node.name) << " (\n";
    
    bool first = true;
    std::vector<std::string> pk_columns;
    
    for (const auto& attr : node.attributes) {
        if (!first) ddl << ",\n";
        first = false;
        
        ddl << "    " << QuoteIdentifier(attr.name) << " ";
        ddl << MapDataType(attr.data_type);
        
        if (!attr.is_nullable) {
            ddl << " NOT NULL";
        }
        
        if (attr.is_primary) {
            pk_columns.push_back(attr.name);
        }
    }
    
    if (!pk_columns.empty() && options.include_constraints) {
        ddl << ",\n    PRIMARY KEY (";
        for (size_t i = 0; i < pk_columns.size(); ++i) {
            if (i > 0) ddl << ", ";
            ddl << QuoteIdentifier(pk_columns[i]);
        }
        ddl << ")";
    }
    
    ddl << "\n) ENGINE=InnoDB;\n";
    
    return ddl.str();
}

std::string MySqlDdlGenerator::GenerateForeignKeyDdl(const DiagramEdge& edge,
                                                      const DiagramModel& model,
                                                      const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    auto& nodes = model.nodes();
    auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
    auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
    
    if (source_it == nodes.end() || target_it == nodes.end()) {
        return "";
    }
    
    std::string fk_name = "fk_" + source_it->name + "_" + target_it->name;
    
    ddl << "ALTER TABLE " << QuoteIdentifier(target_it->name) << "\n";
    ddl << "    ADD CONSTRAINT " << QuoteIdentifier(fk_name) << "\n";
    ddl << "    FOREIGN KEY (" << QuoteIdentifier(source_it->name + "_id") << ")\n";
    ddl << "    REFERENCES " << QuoteIdentifier(source_it->name) << " (id);\n";
    
    return ddl.str();
}

std::string MySqlDdlGenerator::QuoteIdentifier(const std::string& name) {
    return "`" + name + "`";
}

std::string MySqlDdlGenerator::MapDataType(const std::string& type) {
    if (type == "INT" || type == "INTEGER") return "INT";
    if (type == "BIGINT") return "BIGINT";
    if (type == "STRING" || type == "TEXT") return "TEXT";
    if (type == "VARCHAR") return "VARCHAR(255)";
    if (type == "BOOL" || type == "BOOLEAN") return "BOOLEAN";
    if (type == "FLOAT" || type == "REAL") return "FLOAT";
    if (type == "DOUBLE") return "DOUBLE";
    if (type == "DECIMAL" || type == "NUMERIC") return "DECIMAL(18,2)";
    if (type == "DATE") return "DATE";
    if (type == "DATETIME" || type == "TIMESTAMP") return "DATETIME";
    if (type == "UUID") return "CHAR(36)";
    if (type == "JSON") return "JSON";
    if (type == "BLOB") return "BLOB";
    return type.empty() ? "TEXT" : type;
}

std::string MySqlDdlGenerator::GenerateColumnDdl(const DiagramAttribute& attr) {
    std::stringstream ddl;
    ddl << QuoteIdentifier(attr.name) << " " << MapDataType(attr.data_type);
    if (!attr.is_nullable) {
        ddl << " NOT NULL";
    }
    return ddl.str();
}

// ==================== Firebird DDL Generator ====================

std::string FirebirdDdlGenerator::GenerateDdl(const DiagramModel& model,
                                               const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    ddl << "-- Generated DDL for Firebird\n\n";
    
    for (const auto& node : model.nodes()) {
        ddl << GenerateTableDdl(node, options) << "\n";
    }
    
    if (options.include_constraints) {
        ddl << "\n-- Foreign Key Constraints\n";
        for (const auto& edge : model.edges()) {
            ddl << GenerateForeignKeyDdl(edge, model, options) << "\n";
        }
    }
    
    return ddl.str();
}

std::string FirebirdDdlGenerator::GenerateTableDdl(const DiagramNode& node,
                                                    const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    if (options.drop_existing) {
        ddl << "DROP TABLE " << QuoteIdentifier(node.name) << ";\n";
    }
    
    ddl << "CREATE TABLE " << QuoteIdentifier(node.name) << " (\n";
    
    bool first = true;
    std::vector<std::string> pk_columns;
    
    for (const auto& attr : node.attributes) {
        if (!first) ddl << ",\n";
        first = false;
        
        ddl << "    " << QuoteIdentifier(attr.name) << " ";
        ddl << MapDataType(attr.data_type);
        
        if (!attr.is_nullable) {
            ddl << " NOT NULL";
        }
        
        if (attr.is_primary) {
            pk_columns.push_back(attr.name);
        }
    }
    
    if (!pk_columns.empty() && options.include_constraints) {
        ddl << ",\n    PRIMARY KEY (";
        for (size_t i = 0; i < pk_columns.size(); ++i) {
            if (i > 0) ddl << ", ";
            ddl << QuoteIdentifier(pk_columns[i]);
        }
        ddl << ")";
    }
    
    ddl << "\n);\n";
    
    return ddl.str();
}

std::string FirebirdDdlGenerator::GenerateForeignKeyDdl(const DiagramEdge& edge,
                                                         const DiagramModel& model,
                                                         const ForwardEngineerOptions& options) {
    std::stringstream ddl;
    
    auto& nodes = model.nodes();
    auto source_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
    auto target_it = std::find_if(nodes.begin(), nodes.end(),
                                  [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
    
    if (source_it == nodes.end() || target_it == nodes.end()) {
        return "";
    }
    
    std::string fk_name = "FK_" + source_it->name + "_" + target_it->name;
    
    ddl << "ALTER TABLE " << QuoteIdentifier(target_it->name) << "\n";
    ddl << "    ADD CONSTRAINT " << QuoteIdentifier(fk_name) << "\n";
    ddl << "    FOREIGN KEY (" << QuoteIdentifier(source_it->name + "_ID") << ")\n";
    ddl << "    REFERENCES " << QuoteIdentifier(source_it->name) << " (ID);\n";
    
    return ddl.str();
}

std::string FirebirdDdlGenerator::QuoteIdentifier(const std::string& name) {
    return "\"" + name + "\"";
}

std::string FirebirdDdlGenerator::MapDataType(const std::string& type) {
    if (type == "INT" || type == "INTEGER") return "INTEGER";
    if (type == "BIGINT") return "BIGINT";
    if (type == "STRING" || type == "TEXT") return "BLOB SUB_TYPE TEXT";
    if (type == "VARCHAR") return "VARCHAR(255)";
    if (type == "BOOL" || type == "BOOLEAN") return "BOOLEAN";
    if (type == "FLOAT" || type == "REAL") return "FLOAT";
    if (type == "DOUBLE") return "DOUBLE PRECISION";
    if (type == "DECIMAL" || type == "NUMERIC") return "DECIMAL(18,2)";
    if (type == "DATE") return "DATE";
    if (type == "DATETIME" || type == "TIMESTAMP") return "TIMESTAMP";
    if (type == "UUID") return "CHAR(36)";
    if (type == "BLOB") return "BLOB";
    return type.empty() ? "VARCHAR(255)" : type;
}

std::string FirebirdDdlGenerator::GenerateColumnDdl(const DiagramAttribute& attr) {
    std::stringstream ddl;
    ddl << QuoteIdentifier(attr.name) << " " << MapDataType(attr.data_type);
    if (!attr.is_nullable) {
        ddl << " NOT NULL";
    }
    return ddl.str();
}

// ==================== Data Type Mapper ====================

std::string DataTypeMapper::Map(const std::string& source_type, const std::string& target_backend) {
    if (target_backend == "postgresql" || target_backend == "postgres") {
        return PostgreSqlDdlGenerator().MapDataType(source_type);
    } else if (target_backend == "mysql" || target_backend == "mariadb") {
        return MySqlDdlGenerator().MapDataType(source_type);
    } else if (target_backend == "firebird") {
        return FirebirdDdlGenerator().MapDataType(source_type);
    }
    return ScratchBirdDdlGenerator().MapDataType(source_type);
}

std::vector<DataTypeMapping> DataTypeMapper::GetMappings() {
    return {
        {"INT", "INTEGER", "INT", "INTEGER"},
        {"BIGINT", "BIGINT", "BIGINT", "BIGINT"},
        {"STRING", "TEXT", "TEXT", "BLOB SUB_TYPE TEXT"},
        {"VARCHAR", "VARCHAR(255)", "VARCHAR(255)", "VARCHAR(255)"},
        {"BOOL", "BOOLEAN", "BOOLEAN", "BOOLEAN"},
        {"FLOAT", "REAL", "FLOAT", "FLOAT"},
        {"DOUBLE", "DOUBLE PRECISION", "DOUBLE", "DOUBLE PRECISION"},
        {"DECIMAL", "NUMERIC(18,2)", "DECIMAL(18,2)", "DECIMAL(18,2)"},
        {"DATE", "DATE", "DATE", "DATE"},
        {"DATETIME", "TIMESTAMP", "DATETIME", "TIMESTAMP"},
        {"UUID", "UUID", "CHAR(36)", "CHAR(36)"},
        {"JSON", "JSONB", "JSON", "BLOB SUB_TYPE TEXT"},
        {"BLOB", "BYTEA", "BLOB", "BLOB"}
    };
}

// ==================== DDL Preview ====================

DdlPreview::PreviewResult DdlPreview::GeneratePreview(const DiagramModel& model,
                                                       const std::string& backend,
                                                       const ForwardEngineerOptions& options) {
    PreviewResult result;
    
    auto generator = DdlGenerator::Create(backend);
    result.ddl = generator->GenerateDdl(model, options);
    
    // Count objects
    result.table_count = model.nodes().size();
    result.constraint_count = model.edges().size();
    
    // Simple index count (primary keys)
    for (const auto& node : model.nodes()) {
        for (const auto& attr : node.attributes) {
            if (attr.is_primary) {
                result.index_count++;
            }
        }
    }
    
    // Check for potential issues
    for (const auto& node : model.nodes()) {
        if (node.attributes.empty()) {
            result.warnings.push_back("Table '" + node.name + "' has no columns");
        }
    }
    
    return result;
}

} // namespace diagram
} // namespace scratchrobin
