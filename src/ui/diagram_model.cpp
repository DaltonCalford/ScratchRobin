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

ErdNotation DiagramModel::notation() const {
    return notation_;
}

void DiagramModel::set_notation(ErdNotation notation) {
    notation_ = notation;
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
        case DiagramType::Whiteboard:
            return "Whiteboard";
        case DiagramType::MindMap:
            return "Mind Map";
        case DiagramType::DataFlow:
            return "DFD";
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
        case DiagramType::Whiteboard:
            return "whiteboard";
        case DiagramType::MindMap:
            return "mindmap";
        case DiagramType::DataFlow:
            return "dfd";
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

// ERD Notation support (Phase 3.2)
std::string ErdNotationToString(ErdNotation notation) {
    switch (notation) {
        case ErdNotation::CrowsFoot:
            return "crowsfoot";
        case ErdNotation::IDEF1X:
            return "idef1x";
        case ErdNotation::UML:
            return "uml";
        case ErdNotation::Chen:
            return "chen";
        default:
            return "crowsfoot";
    }
}

ErdNotation StringToErdNotation(const std::string& str) {
    if (str == "idef1x") return ErdNotation::IDEF1X;
    if (str == "uml") return ErdNotation::UML;
    if (str == "chen") return ErdNotation::Chen;
    return ErdNotation::CrowsFoot;
}

std::string ErdNotationLabel(ErdNotation notation) {
    switch (notation) {
        case ErdNotation::CrowsFoot:
            return "Crow's Foot";
        case ErdNotation::IDEF1X:
            return "IDEF1X";
        case ErdNotation::UML:
            return "UML Class";
        case ErdNotation::Chen:
            return "Chen";
        default:
            return "Unknown";
    }
}

} // namespace scratchrobin
