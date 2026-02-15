#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "core/beta1b_contracts.h"

namespace scratchrobin::diagram {

enum class DiagramType {
    Erd,
    Silverston,
    Whiteboard,
    MindMap,
    DataFlow,
};

std::string ToString(DiagramType type);
DiagramType ParseDiagramType(const std::string& type_name);

struct DiagramSaveResult {
    std::size_t bytes_written = 0;
    std::size_t node_count = 0;
    std::size_t edge_count = 0;
};

struct ReverseModelSource {
    std::string diagram_id;
    std::string notation;
    std::vector<beta1b::DiagramNode> nodes;
    std::vector<beta1b::DiagramEdge> edges;
};

class DiagramService {
public:
    void ValidateDiagramType(DiagramType type) const;

    DiagramSaveResult SaveModel(const std::string& file_path,
                                DiagramType type,
                                const beta1b::DiagramDocument& document) const;
    beta1b::DiagramDocument LoadModel(const std::string& file_path, DiagramType expected_type) const;

    void ApplyCanvasCommand(const beta1b::DiagramDocument& document,
                            const std::string& operation,
                            const std::string& node_id,
                            const std::string& target_parent_id) const;

    void ValidateTraceRefs(const std::map<std::string, std::vector<std::string>>& trace_refs_by_node,
                           const std::set<std::string>& resolvable_refs) const;

    std::vector<std::string> GenerateForwardSql(const std::string& table_name,
                                                const std::vector<std::string>& logical_types,
                                                const std::map<std::string, std::string>& type_mapping) const;

    std::vector<beta1b::SchemaCompareOperation> GenerateMigrationDiffPlan(
        const std::vector<beta1b::SchemaCompareOperation>& operations,
        bool allow_alter_operations) const;

    std::string ExportDiagram(const beta1b::DiagramDocument& document,
                              const std::string& format,
                              const std::string& profile_id) const;

    beta1b::DiagramDocument ReverseEngineerModel(DiagramType type,
                                                 const ReverseModelSource& source,
                                                 bool from_fixture) const;
};

}  // namespace scratchrobin::diagram
