/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_MODEL_H
#define SCRATCHROBIN_DIAGRAM_MODEL_H

#include <string>
#include <vector>

namespace scratchrobin {

enum class DiagramType {
    Erd,
    Silverston,
    Whiteboard,
    MindMap,
    DataFlow
};

// Supported ERD notations (Phase 3.2)
enum class ErdNotation {
    CrowsFoot,      // Crow's Foot / IE notation (most common)
    IDEF1X,         // Integration DEFinition for Information Modeling  
    UML,            // Unified Modeling Language class diagrams
    Chen            // Chen notation (entities as rectangles, relationships as diamonds)
};

// Convert notation to string
std::string ErdNotationToString(ErdNotation notation);

// Parse notation from string  
ErdNotation StringToErdNotation(const std::string& str);

// Get notation display label
std::string ErdNotationLabel(ErdNotation notation);

enum class Cardinality {
    One,
    ZeroOrOne,
    OneOrMany,
    ZeroOrMany
};

struct DiagramAttribute {
    std::string name;
    std::string data_type;
    bool is_primary = false;
    bool is_foreign = false;
    bool is_nullable = true;  // Default nullable for new attributes
};

struct DiagramNode {
    std::string id;
    std::string name;
    std::string type;
    std::string parent_id;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    int stack_count = 1;
    bool ghosted = false;
    bool pinned = false;  // Pinned nodes are excluded from auto-layout
    std::vector<DiagramAttribute> attributes;
    std::vector<std::string> tags;
    std::vector<std::string> trace_refs;  // Links to ERD/metadata objects (DFD traceability)
};

struct DiagramEdge {
    std::string id;
    std::string source_id;
    std::string target_id;
    std::string label;
    std::string edge_type;
    bool directed = true;
    bool identifying = false;
    Cardinality source_cardinality = Cardinality::One;
    Cardinality target_cardinality = Cardinality::OneOrMany;
    int label_offset = 1;
};

class DiagramModel {
public:
    explicit DiagramModel(DiagramType type);

    DiagramType type() const;
    void set_type(DiagramType type);

    // ERD Notation (Phase 3.2)
    ErdNotation notation() const;
    void set_notation(ErdNotation notation);

    DiagramNode& AddNode(const DiagramNode& node);
    DiagramEdge& AddEdge(const DiagramEdge& edge);

    std::vector<DiagramNode>& nodes();
    const std::vector<DiagramNode>& nodes() const;

    std::vector<DiagramEdge>& edges();
    const std::vector<DiagramEdge>& edges() const;

    int NextNodeIndex();
    int NextEdgeIndex();

private:
    DiagramType type_;
    ErdNotation notation_ = ErdNotation::CrowsFoot;  // Default notation
    int next_node_index_ = 1;
    int next_edge_index_ = 1;
    std::vector<DiagramNode> nodes_;
    std::vector<DiagramEdge> edges_;
};

std::string DiagramTypeLabel(DiagramType type);
std::string DiagramTypeKey(DiagramType type);
DiagramType StringToDiagramType(const std::string& value);
std::string CardinalityLabel(Cardinality value);
Cardinality CardinalityFromString(const std::string& value);

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_MODEL_H
