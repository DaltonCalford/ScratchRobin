/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_CONTAINMENT_H
#define SCRATCHROBIN_DIAGRAM_CONTAINMENT_H

#include <string>
#include <vector>

namespace scratchrobin {

// Supported node types in the diagram system
enum class DiagramNodeType {
    Unknown,
    Schema,
    Table,
    View,
    Column,
    Index,
    Trigger,
    Procedure,
    Function,
    Database,
    Cluster,
    Network,
    Generic,
    Note,
    Sketch,
    Process,
    DataStore
};

// Convert string type to enum
DiagramNodeType StringToDiagramNodeType(const std::string& type);

// Convert enum to string
std::string DiagramNodeTypeToString(DiagramNodeType type);

// Check if a parent type can accept a child type
bool CanAcceptChild(DiagramNodeType parent_type, DiagramNodeType child_type);

// Check if a parent type can accept a child type (string versions)
bool CanAcceptChild(const std::string& parent_type, const std::string& child_type);

// Check if a node type is a container (can have children)
bool IsContainerType(DiagramNodeType type);
bool IsContainerType(const std::string& type);

// Get valid child types for a given parent type
std::vector<DiagramNodeType> GetValidChildTypes(DiagramNodeType parent_type);
std::vector<std::string> GetValidChildTypes(const std::string& parent_type);

// Drag operation type
enum class DragOperation {
    None,
    Move,           // Simple move on canvas
    Reparent,       // Changing parent of node
    AddFromTree     // Adding new node from tree
};

// Drop target info during drag
struct DropTargetInfo {
    std::string node_id;
    std::string node_name;
    std::string node_type;
    bool is_valid = false;
    bool is_container = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_CONTAINMENT_H
