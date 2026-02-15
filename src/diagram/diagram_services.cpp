#include "diagram/diagram_services.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <tuple>

#include "core/reject.h"

namespace scratchrobin::diagram {

std::string ToString(DiagramType type) {
    switch (type) {
        case DiagramType::Erd: return "Erd";
        case DiagramType::Silverston: return "Silverston";
        case DiagramType::Whiteboard: return "Whiteboard";
        case DiagramType::MindMap: return "MindMap";
        case DiagramType::DataFlow: return "DataFlow";
        default: return "Erd";
    }
}

DiagramType ParseDiagramType(const std::string& type_name) {
    if (type_name == "Erd") {
        return DiagramType::Erd;
    }
    if (type_name == "Silverston") {
        return DiagramType::Silverston;
    }
    if (type_name == "Whiteboard") {
        return DiagramType::Whiteboard;
    }
    if (type_name == "MindMap") {
        return DiagramType::MindMap;
    }
    if (type_name == "DataFlow") {
        return DiagramType::DataFlow;
    }
    throw MakeReject("SRB1-R-6101", "invalid diagram type", "diagram", "parse_diagram_type", false, type_name);
}

void DiagramService::ValidateDiagramType(DiagramType type) const {
    (void)ToString(type);
}

DiagramSaveResult DiagramService::SaveModel(const std::string& file_path,
                                            DiagramType type,
                                            const beta1b::DiagramDocument& document) const {
    ValidateDiagramType(type);
    const std::string payload = beta1b::SerializeDiagramModel(document);

    const std::filesystem::path out_path(file_path);
    std::error_code ec;
    std::filesystem::create_directories(out_path.parent_path(), ec);
    std::ofstream out(file_path, std::ios::binary);
    if (!out) {
        throw MakeReject("SRB1-R-6101", "failed to open diagram path", "diagram", "save_model", false, file_path);
    }

    const auto escape = [](const std::string& in) {
        std::string out;
        out.reserve(in.size() + 8);
        for (char c : in) {
            if (c == '\\') {
                out += "\\\\";
            } else if (c == '"') {
                out += "\\\"";
            } else {
                out.push_back(c);
            }
        }
        return out;
    };
    out << "{\"diagram_type\":\"" << ToString(type) << "\",\"model_json\":\"" << escape(payload) << "\"}\n";
    out.flush();
    if (!out) {
        throw MakeReject("SRB1-R-6101", "failed to write diagram model", "diagram", "save_model", false, file_path);
    }

    DiagramSaveResult result;
    result.bytes_written = static_cast<std::size_t>(out.tellp());
    result.node_count = document.nodes.size();
    result.edge_count = document.edges.size();
    return result;
}

beta1b::DiagramDocument DiagramService::LoadModel(const std::string& file_path, DiagramType expected_type) const {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        throw MakeReject("SRB1-R-6101", "failed to read diagram model", "diagram", "load_model", false, file_path);
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    JsonParser parser(text);
    JsonValue root;
    std::string err;
    if (!parser.Parse(&root, &err) || root.type != JsonValue::Type::Object) {
        throw MakeReject("SRB1-R-6101", "invalid diagram payload envelope", "diagram", "load_model", false, err);
    }

    std::string type_name;
    const JsonValue* type_value = FindMember(root, "diagram_type");
    if (type_value == nullptr || !GetStringValue(*type_value, &type_name)) {
        throw MakeReject("SRB1-R-6101", "missing diagram_type", "diagram", "load_model");
    }
    if (ParseDiagramType(type_name) != expected_type) {
        throw MakeReject("SRB1-R-6101", "diagram_type mismatch", "diagram", "load_model", false, type_name);
    }

    std::string model_json;
    const JsonValue* model_value = FindMember(root, "model_json");
    if (model_value == nullptr || !GetStringValue(*model_value, &model_json) || model_json.empty()) {
        throw MakeReject("SRB1-R-6101", "missing model payload", "diagram", "load_model");
    }
    return beta1b::ParseDiagramModel(model_json);
}

void DiagramService::ApplyCanvasCommand(const beta1b::DiagramDocument& document,
                                        const std::string& operation,
                                        const std::string& node_id,
                                        const std::string& target_parent_id) const {
    beta1b::ValidateCanvasOperation(document, operation, node_id, target_parent_id);
}

void DiagramService::ValidateTraceRefs(const std::map<std::string, std::vector<std::string>>& trace_refs_by_node,
                                       const std::set<std::string>& resolvable_refs) const {
    for (const auto& [node_id, refs] : trace_refs_by_node) {
        for (const auto& ref : refs) {
            if (resolvable_refs.count(ref) == 0U) {
                throw MakeReject("SRB1-R-6101", "unresolvable trace reference", "diagram", "validate_trace_refs", false,
                                 node_id + ":" + ref);
            }
        }
    }
}

std::vector<std::string> DiagramService::GenerateForwardSql(const std::string& table_name,
                                                            const std::vector<std::string>& logical_types,
                                                            const std::map<std::string, std::string>& type_mapping) const {
    if (table_name.empty()) {
        throw MakeReject("SRB1-R-6301", "table_name required", "diagram", "generate_forward_sql");
    }
    auto physical = beta1b::ForwardEngineerDatatypes(logical_types, type_mapping);
    std::vector<std::string> ddl;
    ddl.reserve(physical.size());
    for (size_t i = 0; i < physical.size(); ++i) {
        ddl.push_back("ALTER TABLE " + table_name + " ADD COLUMN c" + std::to_string(i + 1) + " " + physical[i] + ";");
    }
    return ddl;
}

std::vector<beta1b::SchemaCompareOperation> DiagramService::GenerateMigrationDiffPlan(
    const std::vector<beta1b::SchemaCompareOperation>& operations,
    bool allow_alter_operations) const {
    for (const auto& op : operations) {
        if (!allow_alter_operations && op.operation_type == "alter") {
            throw MakeReject("SRB1-R-6302", "unsupported migration alter operation", "diagram", "generate_migration_diff",
                             false, op.operation_id);
        }
    }
    auto sorted = beta1b::StableSortOps(operations);
    auto rank = [](const std::string& op_type) {
        if (op_type == "add" || op_type == "create") {
            return 0;
        }
        if (op_type == "alter") {
            return 1;
        }
        if (op_type == "drop") {
            return 2;
        }
        return 3;
    };
    std::stable_sort(sorted.begin(), sorted.end(), [&](const auto& a, const auto& b) {
        return std::make_tuple(rank(a.operation_type), a.object_class, a.object_path, a.operation_id) <
               std::make_tuple(rank(b.operation_type), b.object_class, b.object_path, b.operation_id);
    });
    return sorted;
}

std::string DiagramService::ExportDiagram(const beta1b::DiagramDocument& document,
                                          const std::string& format,
                                          const std::string& profile_id) const {
    return beta1b::ExportDiagram(document, format, profile_id);
}

beta1b::DiagramDocument DiagramService::ReverseEngineerModel(DiagramType type,
                                                             const ReverseModelSource& source,
                                                             bool from_fixture) const {
    ValidateDiagramType(type);
    beta1b::DiagramDocument doc;
    doc.diagram_id = source.diagram_id;
    doc.notation = source.notation;
    doc.diagram_type = ToString(type);
    if (doc.diagram_id.empty()) {
        throw MakeReject("SRB1-R-6101", "reverse model missing diagram_id", "diagram", "reverse_engineer_model");
    }
    beta1b::ValidateNotation(doc.notation);

    doc.nodes = source.nodes;
    doc.edges = source.edges;
    for (auto& node : doc.nodes) {
        if (node.name.empty()) {
            node.name = node.node_id;
        }
        if (node.stack_count <= 0) {
            node.stack_count = 1;
        }
        std::sort(node.tags.begin(), node.tags.end());
        node.tags.erase(std::unique(node.tags.begin(), node.tags.end()), node.tags.end());
        std::sort(node.trace_refs.begin(), node.trace_refs.end());
        node.trace_refs.erase(std::unique(node.trace_refs.begin(), node.trace_refs.end()), node.trace_refs.end());
        if (from_fixture && node.trace_refs.empty()) {
            node.trace_refs.push_back("fixture:" + node.node_id);
        }
    }
    for (auto& edge : doc.edges) {
        if (edge.edge_type.empty()) {
            edge.edge_type = edge.relation_type.empty() ? "link" : edge.relation_type;
        }
        if (edge.label.empty()) {
            edge.label = edge.edge_type;
        }
    }

    std::sort(doc.nodes.begin(), doc.nodes.end(), [](const auto& a, const auto& b) {
        return std::tie(a.object_type, a.name, a.node_id) < std::tie(b.object_type, b.name, b.node_id);
    });
    std::sort(doc.edges.begin(), doc.edges.end(), [](const auto& a, const auto& b) {
        return std::tie(a.from_node_id, a.to_node_id, a.edge_type, a.edge_id) <
               std::tie(b.from_node_id, b.to_node_id, b.edge_type, b.edge_id);
    });
    return doc;
}

}  // namespace scratchrobin::diagram
