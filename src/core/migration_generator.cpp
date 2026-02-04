/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "migration_generator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <chrono>

#include "project.h"
#include "simple_json.h"

namespace scratchrobin {

// ============================================================================
// Migration Operation Type
// ============================================================================
std::string MigrationOpTypeToString(MigrationOpType type) {
    switch (type) {
        case MigrationOpType::CREATE_TABLE: return "CREATE_TABLE";
        case MigrationOpType::ALTER_TABLE: return "ALTER_TABLE";
        case MigrationOpType::DROP_TABLE: return "DROP_TABLE";
        case MigrationOpType::CREATE_INDEX: return "CREATE_INDEX";
        case MigrationOpType::DROP_INDEX: return "DROP_INDEX";
        case MigrationOpType::CREATE_CONSTRAINT: return "CREATE_CONSTRAINT";
        case MigrationOpType::DROP_CONSTRAINT: return "DROP_CONSTRAINT";
        case MigrationOpType::CREATE_TRIGGER: return "CREATE_TRIGGER";
        case MigrationOpType::ALTER_TRIGGER: return "ALTER_TRIGGER";
        case MigrationOpType::DROP_TRIGGER: return "DROP_TRIGGER";
        case MigrationOpType::CREATE_PROCEDURE: return "CREATE_PROCEDURE";
        case MigrationOpType::ALTER_PROCEDURE: return "ALTER_PROCEDURE";
        case MigrationOpType::DROP_PROCEDURE: return "DROP_PROCEDURE";
        case MigrationOpType::CREATE_VIEW: return "CREATE_VIEW";
        case MigrationOpType::ALTER_VIEW: return "ALTER_VIEW";
        case MigrationOpType::DROP_VIEW: return "DROP_VIEW";
        case MigrationOpType::CREATE_SEQUENCE: return "CREATE_SEQUENCE";
        case MigrationOpType::ALTER_SEQUENCE: return "ALTER_SEQUENCE";
        case MigrationOpType::DROP_SEQUENCE: return "DROP_SEQUENCE";
        case MigrationOpType::CREATE_DOMAIN: return "CREATE_DOMAIN";
        case MigrationOpType::ALTER_DOMAIN: return "ALTER_DOMAIN";
        case MigrationOpType::DROP_DOMAIN: return "DROP_DOMAIN";
        case MigrationOpType::CREATE_SCHEMA: return "CREATE_SCHEMA";
        case MigrationOpType::DROP_SCHEMA: return "DROP_SCHEMA";
        case MigrationOpType::INSERT_DATA: return "INSERT_DATA";
        case MigrationOpType::UPDATE_DATA: return "UPDATE_DATA";
        case MigrationOpType::DELETE_DATA: return "DELETE_DATA";
        case MigrationOpType::EXECUTE_SQL: return "EXECUTE_SQL";
        case MigrationOpType::COMMENT_ON: return "COMMENT_ON";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Migration Script Implementation
// ============================================================================
MigrationScript::MigrationScript() {
    id = "migration_" + std::to_string(std::time(nullptr));
}

void MigrationScript::AddOperation(const MigrationOperation& op) {
    operations.push_back(op);
    std::sort(operations.begin(), operations.end(),
              [](const auto& a, const auto& b) { return a.execution_order < b.execution_order; });
}

void MigrationScript::AddOperations(const std::vector<MigrationOperation>& ops) {
    for (const auto& op : ops) {
        AddOperation(op);
    }
}

std::string MigrationScript::GenerateForwardSql() const {
    std::ostringstream oss;
    
    if (!pre_migration_script.empty()) {
        oss << "-- Pre-migration script\n";
        oss << pre_migration_script << "\n\n";
    }
    
    oss << "-- Start transaction\n";
    oss << "START TRANSACTION;\n\n";
    
    for (const auto& op : operations) {
        if (!op.description.empty()) {
            oss << "-- " << op.description << "\n";
        }
        oss << op.sql << ";\n\n";
    }
    
    oss << "-- Commit transaction\n";
    oss << "COMMIT;\n\n";
    
    if (!post_migration_script.empty()) {
        oss << "-- Post-migration script\n";
        oss << post_migration_script << "\n";
    }
    
    return oss.str();
}

std::string MigrationScript::GenerateRollbackSql() const {
    std::ostringstream oss;
    
    oss << "-- Rollback transaction (if not committed)\n";
    oss << "ROLLBACK;\n\n";
    
    // Generate rollback operations in reverse order
    for (auto it = operations.rbegin(); it != operations.rend(); ++it) {
        if (!it->can_rollback) continue;
        if (!it->rollback_sql.empty()) {
            oss << it->rollback_sql << ";\n\n";
        }
    }
    
    return oss.str();
}

bool MigrationScript::Validate(std::vector<std::string>& errors) const {
    bool valid = true;
    
    if (operations.empty()) {
        errors.push_back("Migration script has no operations");
        valid = false;
    }
    
    for (const auto& op : operations) {
        if (op.sql.empty()) {
            errors.push_back("Operation for " + op.object_name + " has empty SQL");
            valid = false;
        }
    }
    
    return valid;
}

MigrationScript::Analysis MigrationScript::Analyze() const {
    Analysis analysis;
    
    for (const auto& op : operations) {
        switch (op.type) {
            case MigrationOpType::CREATE_TABLE:
            case MigrationOpType::CREATE_INDEX:
            case MigrationOpType::CREATE_CONSTRAINT:
            case MigrationOpType::CREATE_TRIGGER:
            case MigrationOpType::CREATE_PROCEDURE:
            case MigrationOpType::CREATE_VIEW:
            case MigrationOpType::CREATE_SEQUENCE:
            case MigrationOpType::CREATE_DOMAIN:
            case MigrationOpType::CREATE_SCHEMA:
                analysis.create_count++;
                analysis.has_schema_changes = true;
                break;
                
            case MigrationOpType::ALTER_TABLE:
            case MigrationOpType::ALTER_TRIGGER:
            case MigrationOpType::ALTER_PROCEDURE:
            case MigrationOpType::ALTER_VIEW:
            case MigrationOpType::ALTER_SEQUENCE:
            case MigrationOpType::ALTER_DOMAIN:
                analysis.alter_count++;
                analysis.has_schema_changes = true;
                break;
                
            case MigrationOpType::DROP_TABLE:
            case MigrationOpType::DROP_INDEX:
            case MigrationOpType::DROP_CONSTRAINT:
            case MigrationOpType::DROP_TRIGGER:
            case MigrationOpType::DROP_PROCEDURE:
            case MigrationOpType::DROP_VIEW:
            case MigrationOpType::DROP_SEQUENCE:
            case MigrationOpType::DROP_DOMAIN:
            case MigrationOpType::DROP_SCHEMA:
                analysis.drop_count++;
                analysis.dangerous_ops++;
                analysis.has_schema_changes = true;
                break;
                
            case MigrationOpType::INSERT_DATA:
            case MigrationOpType::UPDATE_DATA:
            case MigrationOpType::DELETE_DATA:
                analysis.data_change_count++;
                analysis.has_data_changes = true;
                if (op.type == MigrationOpType::DELETE_DATA) {
                    analysis.dangerous_ops++;
                }
                break;
                
            default:
                break;
        }
        
        if (!op.schema.empty() && !op.object_name.empty()) {
            std::string full_name = op.schema + "." + op.object_name;
            if (std::find(analysis.affected_tables.begin(), analysis.affected_tables.end(), 
                         full_name) == analysis.affected_tables.end()) {
                analysis.affected_tables.push_back(full_name);
            }
        }
        
        if (op.is_dangerous) {
            analysis.warnings.push_back("Dangerous operation: " + op.description);
        }
    }
    
    return analysis;
}

// ============================================================================
// Migration Generator Implementation
// ============================================================================
MigrationGenerator::MigrationGenerator(const Options& options)
    : options_(options) {}

std::unique_ptr<MigrationScript> MigrationGenerator::GenerateFromProject(
    const Project& project,
    const std::string& from_version,
    const std::string& to_version) {
    
    auto script = std::make_unique<MigrationScript>();
    script->name = "Migration from " + from_version + " to " + to_version;
    script->description = "Auto-generated migration";
    script->baseline_version = from_version;
    script->version = to_version;
    
    next_order_ = 1;
    
    return script;
}

std::unique_ptr<MigrationScript> MigrationGenerator::GenerateFromSchemaComparison(
    const MetadataSnapshot& source,
    const MetadataSnapshot& target) {
    
    auto script = std::make_unique<MigrationScript>();
    script->name = "Schema migration";
    
    // Compare tables
    // Compare indexes
    // Compare procedures
    // etc.
    
    return script;
}

std::vector<MigrationOperation> MigrationGenerator::GenerateTableOperations(
    const ProjectObject& old_obj,
    const ProjectObject& new_obj) {
    
    std::vector<MigrationOperation> ops;
    
    // Stub implementation - would generate operations based on object state changes
    // For now, just return empty vector
    
    return ops;
}

std::vector<MigrationOperation> MigrationGenerator::DiffTableStructure(
    const std::string& schema,
    const std::string& table,
    const JsonValue& old_cols,
    const JsonValue& new_cols) {
    
    std::vector<MigrationOperation> ops;
    
    // Compare columns and generate ADD COLUMN, ALTER COLUMN, DROP COLUMN
    // This is a simplified implementation
    
    return ops;
}

std::string MigrationGenerator::EscapeIdentifier(const std::string& name) {
    // Simple escaping - wrap in quotes
    return "\"" + name + "\"";
}

std::string MigrationGenerator::EscapeStringLiteral(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == '\'') escaped += "''";
        else escaped += c;
    }
    return "'" + escaped + "'";
}

// ============================================================================
// Migration Template Implementation
// ============================================================================
std::string MigrationTemplate::TableCreationTemplate(const std::string& backend) {
    if (backend == "firebird") {
        return R"(
CREATE TABLE {schema}.{table} (
    -- Primary key column
    ID INTEGER NOT NULL PRIMARY KEY,
    
    -- Add your columns here
    
    -- Audit columns
    CREATED_AT TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UPDATED_AT TIMESTAMP
);

-- Generator for auto-increment
CREATE GENERATOR {table}_GEN;

-- Trigger for auto-increment
CREATE TRIGGER {table}_BI FOR {table}
ACTIVE BEFORE INSERT POSITION 0
AS
BEGIN
    IF (NEW.ID IS NULL) THEN
        NEW.ID = GEN_ID({table}_GEN, 1);
END;

-- Update trigger
CREATE TRIGGER {table}_BU FOR {table}
ACTIVE BEFORE UPDATE POSITION 0
AS
BEGIN
    NEW.UPDATED_AT = CURRENT_TIMESTAMP;
END;
)";
    } else if (backend == "postgres") {
        return R"(
CREATE TABLE {schema}.{table} (
    id SERIAL PRIMARY KEY,
    -- Add your columns here
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP
);

-- Update trigger function
CREATE OR REPLACE FUNCTION update_updated_at()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Update trigger
CREATE TRIGGER {table}_updated_at
    BEFORE UPDATE ON {schema}.{table}
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at();
)";
    } else if (backend == "mysql") {
        return R"(
CREATE TABLE {schema}.{table} (
    id INT AUTO_INCREMENT PRIMARY KEY,
    -- Add your columns here
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;
)";
    }
    return "";
}

std::string MigrationTemplate::IndexCreationTemplate(const std::string& backend) {
    if (backend == "firebird") {
        return "CREATE {unique} INDEX {index_name} ON {table} ({columns});";
    } else if (backend == "postgres") {
        return "CREATE {unique} INDEX {index_name} ON {schema}.{table} USING {method} ({columns});";
    } else if (backend == "mysql") {
        return "CREATE {unique} INDEX {index_name} ON {table} ({columns});";
    }
    return "";
}

std::vector<std::string> MigrationTemplate::GetDefaultChecks(const std::string& backend) {
    return {
        "Check for missing primary keys",
        "Verify index on foreign key columns",
        "Ensure proper column constraints",
        "Review default values",
        "Check for proper naming conventions"
    };
}

// ============================================================================
// Migration Validator Implementation
// ============================================================================
MigrationValidator::ValidationResult MigrationValidator::Validate(
    const MigrationScript& script) {
    
    ValidationResult result;
    
    if (script.operations.empty()) {
        result.errors.push_back("Migration has no operations");
        result.valid = false;
    }
    
    auto analysis = script.Analyze();
    if (analysis.dangerous_ops > 0) {
        result.warnings.push_back("Migration contains " + 
                                  std::to_string(analysis.dangerous_ops) + 
                                  " potentially dangerous operations");
    }
    
    return result;
}

MigrationValidator::ValidationResult MigrationValidator::ValidateOperation(
    const MigrationOperation& op) {
    
    ValidationResult result;
    
    if (op.sql.empty()) {
        result.errors.push_back("Empty SQL for operation on " + op.object_name);
        result.valid = false;
    }
    
    if (op.is_dangerous) {
        result.warnings.push_back("Dangerous operation: " + op.description);
    }
    
    return result;
}

bool MigrationValidator::CheckSyntax(const std::string& sql, const std::string& backend) {
    // Would use backend-specific parser to validate SQL
    return true;
}

// ============================================================================
// Migration Preview Implementation
// ============================================================================
std::vector<MigrationPreview::ChangePreview> MigrationPreview::PreviewChanges(
    const MigrationScript& script,
    DatabaseConnection* connection) {
    
    std::vector<ChangePreview> previews;
    
    for (const auto& op : script.operations) {
        ChangePreview preview;
        preview.object_name = op.object_name;
        preview.change_type = MigrationOpTypeToString(op.type);
        preview.affected_sql = op.sql;
        preview.has_data_loss = HasDataLoss(op);
        previews.push_back(preview);
    }
    
    return previews;
}

bool MigrationPreview::HasDataLoss(const MigrationOperation& op) {
    return op.type == MigrationOpType::DROP_TABLE ||
           op.type == MigrationOpType::DELETE_DATA ||
           (op.type == MigrationOpType::ALTER_TABLE && op.sql.find("DROP") != std::string::npos);
}

} // namespace scratchrobin
