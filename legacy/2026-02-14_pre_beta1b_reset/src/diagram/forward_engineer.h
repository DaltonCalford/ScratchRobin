/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_FORWARD_ENGINEER_H
#define SCRATCHROBIN_DIAGRAM_FORWARD_ENGINEER_H

#include "../ui/diagram_model.h"

#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {
namespace diagram {

// DDL generation options
struct ForwardEngineerOptions {
    bool create_if_not_exists = true;
    bool drop_existing = false;
    bool include_indexes = true;
    bool include_constraints = true;
    bool include_comments = false;
    std::string schema_name = "public";
    bool use_domains = true;        // Use ScratchBird domains
};

// Data type mapping for different backends
struct DataTypeMapping {
    std::string scratchbird_type;
    std::string postgres_type;
    std::string mysql_type;
    std::string firebird_type;
};

// DDL generator base class
class DdlGenerator {
public:
    virtual ~DdlGenerator() = default;
    
    // Generate DDL for entire diagram
    virtual std::string GenerateDdl(const DiagramModel& model,
                                     const ForwardEngineerOptions& options) = 0;
    
    // Generate DDL for single table
    virtual std::string GenerateTableDdl(const DiagramNode& node,
                                          const ForwardEngineerOptions& options) = 0;
    
    // Generate DDL for foreign keys
    virtual std::string GenerateForeignKeyDdl(const DiagramEdge& edge,
                                               const DiagramModel& model,
                                               const ForwardEngineerOptions& options) = 0;
    
    // Factory method - backend is a string: "scratchbird", "postgresql", "mysql", "firebird"
    static std::unique_ptr<DdlGenerator> Create(const std::string& backend);
};

// ScratchBird DDL generator
class ScratchBirdDdlGenerator : public DdlGenerator {
public:
    std::string GenerateDdl(const DiagramModel& model,
                            const ForwardEngineerOptions& options) override;
    std::string GenerateTableDdl(const DiagramNode& node,
                                  const ForwardEngineerOptions& options) override;
    std::string GenerateForeignKeyDdl(const DiagramEdge& edge,
                                       const DiagramModel& model,
                                       const ForwardEngineerOptions& options) override;

private:
    std::string QuoteIdentifier(const std::string& name);
    std::string GenerateColumnDdl(const DiagramAttribute& attr);
public:
    std::string MapDataType(const std::string& type);
};

// PostgreSQL DDL generator
class PostgreSqlDdlGenerator : public DdlGenerator {
public:
    std::string GenerateDdl(const DiagramModel& model,
                            const ForwardEngineerOptions& options) override;
    std::string GenerateTableDdl(const DiagramNode& node,
                                  const ForwardEngineerOptions& options) override;
    std::string GenerateForeignKeyDdl(const DiagramEdge& edge,
                                       const DiagramModel& model,
                                       const ForwardEngineerOptions& options) override;

private:
    std::string QuoteIdentifier(const std::string& name);
    std::string GenerateColumnDdl(const DiagramAttribute& attr);
public:
    std::string MapDataType(const std::string& type);
};

// MySQL DDL generator
class MySqlDdlGenerator : public DdlGenerator {
public:
    std::string GenerateDdl(const DiagramModel& model,
                            const ForwardEngineerOptions& options) override;
    std::string GenerateTableDdl(const DiagramNode& node,
                                  const ForwardEngineerOptions& options) override;
    std::string GenerateForeignKeyDdl(const DiagramEdge& edge,
                                       const DiagramModel& model,
                                       const ForwardEngineerOptions& options) override;

private:
    std::string QuoteIdentifier(const std::string& name);
    std::string GenerateColumnDdl(const DiagramAttribute& attr);
public:
    std::string MapDataType(const std::string& type);
};

// Firebird DDL generator
class FirebirdDdlGenerator : public DdlGenerator {
public:
    std::string GenerateDdl(const DiagramModel& model,
                            const ForwardEngineerOptions& options) override;
    std::string GenerateTableDdl(const DiagramNode& node,
                                  const ForwardEngineerOptions& options) override;
    std::string GenerateForeignKeyDdl(const DiagramEdge& edge,
                                       const DiagramModel& model,
                                       const ForwardEngineerOptions& options) override;

private:
    std::string QuoteIdentifier(const std::string& name);
    std::string GenerateColumnDdl(const DiagramAttribute& attr);
public:
    std::string MapDataType(const std::string& type);
};

// Data type mapping table
class DataTypeMapper {
public:
    static std::string Map(const std::string& source_type, const std::string& target_backend);
    static std::vector<DataTypeMapping> GetMappings();
};

// DDL preview dialog helper
class DdlPreview {
public:
    // Generate DDL with preview metadata
    struct PreviewResult {
        std::string ddl;
        int table_count = 0;
        int index_count = 0;
        int constraint_count = 0;
        std::vector<std::string> warnings;
    };
    
    static PreviewResult GeneratePreview(const DiagramModel& model,
                                          const std::string& backend,
                                          const ForwardEngineerOptions& options);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_FORWARD_ENGINEER_H
