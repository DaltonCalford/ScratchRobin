/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MIGRATION_GENERATOR_H
#define SCRATCHROBIN_MIGRATION_GENERATOR_H

#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "simple_json.h"

namespace scratchrobin {

// Forward declarations
class Project;
class ProjectObject;
class DatabaseConnection;
class MetadataSnapshot;

// ============================================================================
// Migration Operation Types
// ============================================================================
enum class MigrationOpType {
    CREATE_TABLE,
    ALTER_TABLE,
    DROP_TABLE,
    CREATE_INDEX,
    DROP_INDEX,
    CREATE_CONSTRAINT,
    DROP_CONSTRAINT,
    CREATE_TRIGGER,
    ALTER_TRIGGER,
    DROP_TRIGGER,
    CREATE_PROCEDURE,
    ALTER_PROCEDURE,
    DROP_PROCEDURE,
    CREATE_VIEW,
    ALTER_VIEW,
    DROP_VIEW,
    CREATE_SEQUENCE,
    ALTER_SEQUENCE,
    DROP_SEQUENCE,
    CREATE_DOMAIN,
    ALTER_DOMAIN,
    DROP_DOMAIN,
    CREATE_SCHEMA,
    DROP_SCHEMA,
    INSERT_DATA,
    UPDATE_DATA,
    DELETE_DATA,
    EXECUTE_SQL,
    COMMENT_ON
};

std::string MigrationOpTypeToString(MigrationOpType type);

// ============================================================================
// Migration Operation
// ============================================================================
struct MigrationOperation {
    MigrationOpType type;
    std::string object_type;  // table, index, procedure, etc.
    std::string schema;
    std::string object_name;
    std::string sql;          // The actual SQL to execute
    std::string rollback_sql; // SQL to reverse this operation
    int execution_order = 0;
    bool can_rollback = true;
    bool is_dangerous = false; // DROP, DELETE, etc.
    std::string description;
    std::vector<std::string> dependencies; // Other operations this depends on
};

// ============================================================================
// Migration Script
// ============================================================================
class MigrationScript {
public:
    std::string id;
    std::string name;
    std::string description;
    std::string version;  // Semantic version
    std::string author;
    std::time_t created_at;
    std::string source_backend;  // firebird, postgres, mysql, etc.
    std::string target_backend;
    
    std::vector<MigrationOperation> operations;
    
    // Pre/post migration scripts
    std::string pre_migration_script;
    std::string post_migration_script;
    
    // Metadata
    std::vector<std::string> tags;
    std::string project_id;
    std::string baseline_version;
    
    // Statistics
    int estimated_duration_seconds = 0;
    size_t estimated_rows_affected = 0;
    bool requires_downtime = false;
    
    MigrationScript();
    
    // Add operations
    void AddOperation(const MigrationOperation& op);
    void AddOperations(const std::vector<MigrationOperation>& ops);
    
    // Generate full SQL
    std::string GenerateForwardSql() const;
    std::string GenerateRollbackSql() const;
    
    // Validation
    bool Validate(std::vector<std::string>& errors) const;
    
    // Analysis
    struct Analysis {
        int create_count = 0;
        int alter_count = 0;
        int drop_count = 0;
        int data_change_count = 0;
        int dangerous_ops = 0;
        bool has_schema_changes = false;
        bool has_data_changes = false;
        std::vector<std::string> affected_tables;
        std::vector<std::string> warnings;
    };
    Analysis Analyze() const;
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<MigrationScript> FromJson(const std::string& json);
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<MigrationScript> LoadFromFile(const std::string& path);
    
    // Export formats
    std::string ExportAsPlainSql() const;
    std::string ExportAsFlywayMigration() const;
    std::string ExportAsLiquibaseChangeSet() const;
};

// ============================================================================
// Migration Generator
// ============================================================================
struct MigrationGeneratorOptions {
    bool generate_rollback = true;
    bool include_comments = true;
    bool use_transactions = true;
    bool validate_constraints = true;
    bool generate_tests = true;
    int max_operations_per_transaction = 100;
    std::string naming_convention = "timestamp";
    std::vector<std::string> skip_objects;  // Objects to ignore
    std::vector<std::string> skip_schemas;
};

class MigrationGenerator {
public:
    using Options = MigrationGeneratorOptions;
    
    MigrationGenerator(const Options& options = Options());
    
    // Generate from project changes
    std::unique_ptr<MigrationScript> GenerateFromProject(
        const Project& project,
        const std::string& from_version,
        const std::string& to_version);
    
    // Generate from schema comparison
    std::unique_ptr<MigrationScript> GenerateFromSchemaComparison(
        const MetadataSnapshot& source,
        const MetadataSnapshot& target);
    
    // Generate from database connection (diff with project)
    std::unique_ptr<MigrationScript> GenerateFromConnection(
        const Project& project,
        DatabaseConnection* connection);
    
    // Generate for specific object changes
    std::vector<MigrationOperation> GenerateTableOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    std::vector<MigrationOperation> GenerateIndexOperations(
        const ProjectObject& obj,
        bool is_create);
    
    std::vector<MigrationOperation> GenerateProcedureOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    std::vector<MigrationOperation> GenerateTriggerOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    // Utility operations
    std::optional<MigrationOperation> GenerateSetComment(
        const std::string& object_type,
        const std::string& schema,
        const std::string& object_name,
        const std::string& comment);
    
    std::optional<MigrationOperation> GenerateGrantPermissions(
        const std::string& object_type,
        const std::string& schema,
        const std::string& object_name,
        const std::string& grantee,
        const std::vector<std::string>& permissions);
    
private:
    Options options_;
    
    // Backend-specific generators
    std::vector<MigrationOperation> GenerateFirebirdOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    std::vector<MigrationOperation> GeneratePostgresOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    std::vector<MigrationOperation> GenerateMySqlOperations(
        const ProjectObject& old_obj,
        const ProjectObject& new_obj);
    
    // Helper methods
    std::vector<MigrationOperation> DiffTableStructure(
        const std::string& schema,
        const std::string& table,
        const JsonValue& old_cols,
        const JsonValue& new_cols);
    
    std::string EscapeIdentifier(const std::string& name);
    std::string EscapeStringLiteral(const std::string& value);
    std::string FormatDefaultValue(const std::string& type, const std::string& value);
    
    int next_order_ = 1;
    int GetNextOrder() { return next_order_++; }
};

// ============================================================================
// Migration Template
// ============================================================================
class MigrationTemplate {
public:
    static std::string TableCreationTemplate(const std::string& backend);
    static std::string IndexCreationTemplate(const std::string& backend);
    static std::string ProcedureTemplate(const std::string& backend);
    static std::string TriggerTemplate(const std::string& backend);
    static std::string AlterTableTemplate(const std::string& backend);
    
    static std::vector<std::string> GetDefaultChecks(const std::string& backend);
    static std::string GetBestPracticeComment(const std::string& backend, 
                                               const std::string& object_type);
};

// ============================================================================
// Migration Validator
// ============================================================================
class MigrationValidator {
public:
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
    };
    
    static ValidationResult Validate(const MigrationScript& script);
    static ValidationResult ValidateOperation(const MigrationOperation& op);
    
    static bool CheckSyntax(const std::string& sql, const std::string& backend);
    static bool CheckPermissions(const MigrationScript& script, DatabaseConnection* conn);
    static bool CheckDependencies(const MigrationScript& script);
};

// ============================================================================
// Migration Preview
// ============================================================================
class MigrationPreview {
public:
    struct ChangePreview {
        std::string object_name;
        std::string change_type;  // CREATE, ALTER, DROP
        std::string old_definition;
        std::string new_definition;
        std::string affected_sql;
        bool has_data_loss = false;
        std::vector<std::string> warnings;
    };
    
    static std::vector<ChangePreview> PreviewChanges(
        const MigrationScript& script,
        DatabaseConnection* connection);
    
    static bool HasDataLoss(const MigrationOperation& op);
    static std::vector<std::string> EstimateImpact(
        const MigrationScript& script,
        DatabaseConnection* connection);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MIGRATION_GENERATOR_H
