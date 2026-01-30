/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_model.h"

namespace scratchrobin {

DiagramModel::DiagramModel(DiagramType type)
    : type_(type) {}

DiagramType DiagramModel::type() const {
    return type_;
}

void DiagramModel::set_type(DiagramType type) {
    type_ = type;
}

DiagramNode& DiagramModel::AddNode(const DiagramNode& node) {
    nodes_.push_back(node);
    return nodes_.back();
}

DiagramEdge& DiagramModel::AddEdge(const DiagramEdge& edge) {
    edges_.push_back(edge);
    return edges_.back();
}

std::vector<DiagramNode>& DiagramModel::nodes() {
    return nodes_;
}

const std::vector<DiagramNode>& DiagramModel::nodes() const {
    return nodes_;
}

std::vector<DiagramEdge>& DiagramModel::edges() {
    return edges_;
}

const std::vector<DiagramEdge>& DiagramModel::edges() const {
    return edges_;
}

int DiagramModel::NextNodeIndex() {
    return next_node_index_++;
}

int DiagramModel::NextEdgeIndex() {
    return next_edge_index_++;
}

std::string DiagramTypeLabel(DiagramType type) {
    switch (type) {
        case DiagramType::Erd:
            return "ERD";
        case DiagramType::Silverston:
            return "Silverston";
        default:
            return "Diagram";
    }
}

std::string DiagramTypeKey(DiagramType type) {
    switch (type) {
        case DiagramType::Erd:
            return "erd";
        case DiagramType::Silverston:
            return "silverston";
        default:
            return "diagram";
    }
}

std::string CardinalityLabel(Cardinality value) {
    switch (value) {
        case Cardinality::One:
            return "1";
        case Cardinality::ZeroOrOne:
            return "0..1";
        case Cardinality::OneOrMany:
            return "1..N";
        case Cardinality::ZeroOrMany:
            return "0..N";
        default:
            return "?";
    }
}

} // namespace scratchrobin
