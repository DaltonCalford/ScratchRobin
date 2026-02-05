/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "diagram_serialization.h"

#include <fstream>
#include <sstream>

#include "core/simple_json.h"

namespace scratchrobin {
namespace diagram {

namespace {

void WriteStringArray(std::ostream& out, const std::vector<std::string>& values) {
    out << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        out << "\"" << values[i] << "\"";
        if (i + 1 < values.size()) out << ", ";
    }
    out << "]";
}

std::vector<std::string> ReadStringArray(const JsonValue& value) {
    std::vector<std::string> result;
    if (value.type != JsonValue::Type::Array) return result;
    for (const auto& item : value.array_value) {
        if (item.type == JsonValue::Type::String) {
            result.push_back(item.string_value);
        }
    }
    return result;
}

} // namespace

std::string DiagramSerializer::ToJson(const DiagramModel& model, const DiagramDocument& doc) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"diagram_id\": \"" << doc.diagram_id << "\",\n";
    out << "  \"name\": \"" << doc.name << "\",\n";
    out << "  \"diagram_type\": \"" << DiagramTypeKey(model.type()) << "\",\n";
    out << "  \"notation\": \"" << ErdNotationToString(model.notation()) << "\",\n";
    out << "  \"view\": {\n";
    out << "    \"zoom\": " << doc.zoom << ",\n";
    out << "    \"pan_x\": " << doc.pan_x << ",\n";
    out << "    \"pan_y\": " << doc.pan_y << "\n";
    out << "  },\n";
    out << "  \"nodes\": [\n";
    for (size_t i = 0; i < model.nodes().size(); ++i) {
        const auto& node = model.nodes()[i];
        out << "    {\n";
        out << "      \"id\": \"" << node.id << "\",\n";
        out << "      \"name\": \"" << node.name << "\",\n";
        out << "      \"type\": \"" << node.type << "\",\n";
        out << "      \"parent_id\": \"" << node.parent_id << "\",\n";
        out << "      \"x\": " << node.x << ",\n";
        out << "      \"y\": " << node.y << ",\n";
        out << "      \"width\": " << node.width << ",\n";
        out << "      \"height\": " << node.height << ",\n";
        out << "      \"stack_count\": " << node.stack_count << ",\n";
        out << "      \"ghosted\": " << (node.ghosted ? "true" : "false") << ",\n";
        out << "      \"pinned\": " << (node.pinned ? "true" : "false") << ",\n";
        out << "      \"tags\": ";
        WriteStringArray(out, node.tags);
        out << ",\n";
        out << "      \"trace_refs\": ";
        WriteStringArray(out, node.trace_refs);
        out << ",\n";
        out << "      \"attributes\": [\n";
        for (size_t a = 0; a < node.attributes.size(); ++a) {
            const auto& attr = node.attributes[a];
            out << "        {\n";
            out << "          \"name\": \"" << attr.name << "\",\n";
            out << "          \"data_type\": \"" << attr.data_type << "\",\n";
            out << "          \"is_primary\": " << (attr.is_primary ? "true" : "false") << ",\n";
            out << "          \"is_foreign\": " << (attr.is_foreign ? "true" : "false") << ",\n";
            out << "          \"is_nullable\": " << (attr.is_nullable ? "true" : "false") << "\n";
            out << "        }";
            if (a + 1 < node.attributes.size()) out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i + 1 < model.nodes().size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"edges\": [\n";
    for (size_t i = 0; i < model.edges().size(); ++i) {
        const auto& edge = model.edges()[i];
        out << "    {\n";
        out << "      \"id\": \"" << edge.id << "\",\n";
        out << "      \"source_id\": \"" << edge.source_id << "\",\n";
        out << "      \"target_id\": \"" << edge.target_id << "\",\n";
        out << "      \"label\": \"" << edge.label << "\",\n";
        out << "      \"edge_type\": \"" << edge.edge_type << "\",\n";
        out << "      \"directed\": " << (edge.directed ? "true" : "false") << ",\n";
        out << "      \"identifying\": " << (edge.identifying ? "true" : "false") << ",\n";
        out << "      \"source_cardinality\": \"" << CardinalityLabel(edge.source_cardinality) << "\",\n";
        out << "      \"target_cardinality\": \"" << CardinalityLabel(edge.target_cardinality) << "\",\n";
        out << "      \"label_offset\": " << edge.label_offset << "\n";
        out << "    }";
        if (i + 1 < model.edges().size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

bool DiagramSerializer::FromJson(const std::string& json, DiagramModel* model,
                                 DiagramDocument* doc, std::string* error) {
    if (!model || !doc) return false;
    JsonParser parser(json);
    JsonValue root;
    std::string parse_error;
    if (!parser.Parse(&root, &parse_error)) {
        if (error) *error = parse_error;
        return false;
    }

    if (auto value = FindMember(root, "diagram_id")) {
        doc->diagram_id = value->string_value;
    }
    if (auto value = FindMember(root, "name")) {
        doc->name = value->string_value;
    }
    if (auto value = FindMember(root, "diagram_type")) {
        model->set_type(StringToDiagramType(value->string_value));
    }
    if (auto value = FindMember(root, "notation")) {
        model->set_notation(StringToErdNotation(value->string_value));
    }
    if (auto view = FindMember(root, "view")) {
        if (auto zoom = FindMember(*view, "zoom")) doc->zoom = zoom->number_value;
        if (auto pan_x = FindMember(*view, "pan_x")) doc->pan_x = pan_x->number_value;
        if (auto pan_y = FindMember(*view, "pan_y")) doc->pan_y = pan_y->number_value;
    }

    model->nodes().clear();
    model->edges().clear();

    if (auto nodes = FindMember(root, "nodes")) {
        if (nodes->type == JsonValue::Type::Array) {
            for (const auto& node_val : nodes->array_value) {
                DiagramNode node;
                if (auto val = FindMember(node_val, "id")) node.id = val->string_value;
                if (auto val = FindMember(node_val, "name")) node.name = val->string_value;
                if (auto val = FindMember(node_val, "type")) node.type = val->string_value;
                if (auto val = FindMember(node_val, "parent_id")) node.parent_id = val->string_value;
                if (auto val = FindMember(node_val, "x")) node.x = val->number_value;
                if (auto val = FindMember(node_val, "y")) node.y = val->number_value;
                if (auto val = FindMember(node_val, "width")) node.width = val->number_value;
                if (auto val = FindMember(node_val, "height")) node.height = val->number_value;
                if (auto val = FindMember(node_val, "stack_count")) node.stack_count = static_cast<int>(val->number_value);
                if (auto val = FindMember(node_val, "ghosted")) node.ghosted = val->bool_value;
                if (auto val = FindMember(node_val, "pinned")) node.pinned = val->bool_value;
                if (auto val = FindMember(node_val, "tags")) node.tags = ReadStringArray(*val);
                if (auto val = FindMember(node_val, "trace_refs")) node.trace_refs = ReadStringArray(*val);

                if (auto attrs = FindMember(node_val, "attributes")) {
                    if (attrs->type == JsonValue::Type::Array) {
                        for (const auto& attr_val : attrs->array_value) {
                            DiagramAttribute attr;
                            if (auto v = FindMember(attr_val, "name")) attr.name = v->string_value;
                            if (auto v = FindMember(attr_val, "data_type")) attr.data_type = v->string_value;
                            if (auto v = FindMember(attr_val, "is_primary")) attr.is_primary = v->bool_value;
                            if (auto v = FindMember(attr_val, "is_foreign")) attr.is_foreign = v->bool_value;
                            if (auto v = FindMember(attr_val, "is_nullable")) attr.is_nullable = v->bool_value;
                            node.attributes.push_back(attr);
                        }
                    }
                }

                model->AddNode(node);
            }
        }
    }

    if (auto edges = FindMember(root, "edges")) {
        if (edges->type == JsonValue::Type::Array) {
            for (const auto& edge_val : edges->array_value) {
                DiagramEdge edge;
                if (auto val = FindMember(edge_val, "id")) edge.id = val->string_value;
                if (auto val = FindMember(edge_val, "source_id")) edge.source_id = val->string_value;
                if (auto val = FindMember(edge_val, "target_id")) edge.target_id = val->string_value;
                if (auto val = FindMember(edge_val, "label")) edge.label = val->string_value;
                if (auto val = FindMember(edge_val, "edge_type")) edge.edge_type = val->string_value;
                if (auto val = FindMember(edge_val, "directed")) edge.directed = val->bool_value;
                if (auto val = FindMember(edge_val, "identifying")) edge.identifying = val->bool_value;
                if (auto val = FindMember(edge_val, "source_cardinality")) {
                    edge.source_cardinality = CardinalityFromString(val->string_value);
                }
                if (auto val = FindMember(edge_val, "target_cardinality")) {
                    edge.target_cardinality = CardinalityFromString(val->string_value);
                }
                if (auto val = FindMember(edge_val, "label_offset")) edge.label_offset = static_cast<int>(val->number_value);
                model->AddEdge(edge);
            }
        }
    }

    return true;
}

bool DiagramSerializer::SaveToFile(const DiagramModel& model, const DiagramDocument& doc,
                                   const std::string& path, std::string* error) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        if (error) *error = "Failed to open diagram file";
        return false;
    }
    out << ToJson(model, doc);
    return true;
}

bool DiagramSerializer::LoadFromFile(DiagramModel* model, DiagramDocument* doc,
                                     const std::string& path, std::string* error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error) *error = "Failed to open diagram file";
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return FromJson(buffer.str(), model, doc, error);
}

} // namespace diagram
} // namespace scratchrobin
