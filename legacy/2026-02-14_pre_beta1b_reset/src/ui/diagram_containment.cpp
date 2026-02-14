/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_containment.h"

#include <algorithm>
#include <cctype>

namespace scratchrobin {

namespace {

// Convert string to lowercase for case-insensitive comparison
std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace

DiagramNodeType StringToDiagramNodeType(const std::string& type) {
    std::string lower = ToLower(type);
    if (lower == "schema") return DiagramNodeType::Schema;
    if (lower == "table") return DiagramNodeType::Table;
    if (lower == "view") return DiagramNodeType::View;
    if (lower == "column") return DiagramNodeType::Column;
    if (lower == "index") return DiagramNodeType::Index;
    if (lower == "trigger") return DiagramNodeType::Trigger;
    if (lower == "procedure" || lower == "storedprocedure") return DiagramNodeType::Procedure;
    if (lower == "function" || lower == "userdefinedfunction") return DiagramNodeType::Function;
    if (lower == "database") return DiagramNodeType::Database;
    if (lower == "cluster") return DiagramNodeType::Cluster;
    if (lower == "network") return DiagramNodeType::Network;
    if (lower == "note") return DiagramNodeType::Note;
    if (lower == "sketch") return DiagramNodeType::Sketch;
    if (lower == "process") return DiagramNodeType::Process;
    if (lower == "datastore" || lower == "data store") return DiagramNodeType::DataStore;
    return DiagramNodeType::Generic;
}

std::string DiagramNodeTypeToString(DiagramNodeType type) {
    switch (type) {
        case DiagramNodeType::Schema: return "Schema";
        case DiagramNodeType::Table: return "Table";
        case DiagramNodeType::View: return "View";
        case DiagramNodeType::Column: return "Column";
        case DiagramNodeType::Index: return "Index";
        case DiagramNodeType::Trigger: return "Trigger";
        case DiagramNodeType::Procedure: return "Procedure";
        case DiagramNodeType::Function: return "Function";
        case DiagramNodeType::Database: return "Database";
        case DiagramNodeType::Cluster: return "Cluster";
        case DiagramNodeType::Network: return "Network";
        case DiagramNodeType::Note: return "Note";
        case DiagramNodeType::Sketch: return "Sketch";
        case DiagramNodeType::Process: return "Process";
        case DiagramNodeType::DataStore: return "DataStore";
        case DiagramNodeType::Generic: return "Generic";
        default: return "Unknown";
    }
}

bool CanAcceptChild(DiagramNodeType parent_type, DiagramNodeType child_type) {
    // A node cannot be its own parent
    if (parent_type == child_type && parent_type != DiagramNodeType::Generic) {
        // Some types like Generic might allow same-type nesting, others don't
    }

    switch (parent_type) {
        case DiagramNodeType::Schema:
            // Schema can contain: Tables, Views, Procedures, Functions, Triggers
            return child_type == DiagramNodeType::Table ||
                   child_type == DiagramNodeType::View ||
                   child_type == DiagramNodeType::Procedure ||
                   child_type == DiagramNodeType::Function ||
                   child_type == DiagramNodeType::Trigger;

        case DiagramNodeType::Table:
            // Table can contain: Columns, Indexes, Triggers
            return child_type == DiagramNodeType::Column ||
                   child_type == DiagramNodeType::Index ||
                   child_type == DiagramNodeType::Trigger;

        case DiagramNodeType::Database:
            // Database can contain: Schemas, Tables (at top level), Views
            return child_type == DiagramNodeType::Schema ||
                   child_type == DiagramNodeType::Table ||
                   child_type == DiagramNodeType::View ||
                   child_type == DiagramNodeType::Procedure ||
                   child_type == DiagramNodeType::Function;

        case DiagramNodeType::Cluster:
            // Cluster can contain: Databases, Servers, Networks
            return child_type == DiagramNodeType::Database ||
                   child_type == DiagramNodeType::Network;

        case DiagramNodeType::View:
            // Views typically don't contain other objects
            return false;

        case DiagramNodeType::Procedure:
        case DiagramNodeType::Function:
            // Procedures and Functions don't contain diagram children
            return false;

        case DiagramNodeType::Column:
        case DiagramNodeType::Index:
        case DiagramNodeType::Trigger:
            // These are leaf nodes
            return false;

        case DiagramNodeType::Process:
            // Process can contain subprocesses
            return child_type == DiagramNodeType::Process ||
                   child_type == DiagramNodeType::DataStore;

        case DiagramNodeType::DataStore:
            // Data stores are typically leaf nodes
            return false;

        case DiagramNodeType::Note:
        case DiagramNodeType::Sketch:
            // Notes and sketches can contain anything (whiteboard behavior)
            return true;

        case DiagramNodeType::Generic:
        case DiagramNodeType::Network:
        default:
            // Generic and Network can contain other generic items
            return child_type == DiagramNodeType::Generic ||
                   child_type == DiagramNodeType::Network;
    }
}

bool CanAcceptChild(const std::string& parent_type, const std::string& child_type) {
    return CanAcceptChild(StringToDiagramNodeType(parent_type), 
                          StringToDiagramNodeType(child_type));
}

bool IsContainerType(DiagramNodeType type) {
    switch (type) {
        case DiagramNodeType::Schema:
        case DiagramNodeType::Table:
        case DiagramNodeType::Database:
        case DiagramNodeType::Cluster:
        case DiagramNodeType::Process:
        case DiagramNodeType::Note:
        case DiagramNodeType::Sketch:
        case DiagramNodeType::Generic:
            return true;
        default:
            return false;
    }
}

bool IsContainerType(const std::string& type) {
    return IsContainerType(StringToDiagramNodeType(type));
}

std::vector<DiagramNodeType> GetValidChildTypes(DiagramNodeType parent_type) {
    std::vector<DiagramNodeType> valid_types;
    
    // Check all possible child types
    DiagramNodeType all_types[] = {
        DiagramNodeType::Schema,
        DiagramNodeType::Table,
        DiagramNodeType::View,
        DiagramNodeType::Column,
        DiagramNodeType::Index,
        DiagramNodeType::Trigger,
        DiagramNodeType::Procedure,
        DiagramNodeType::Function,
        DiagramNodeType::Database,
        DiagramNodeType::Cluster,
        DiagramNodeType::Process,
        DiagramNodeType::DataStore,
        DiagramNodeType::Generic
    };
    
    for (auto type : all_types) {
        if (CanAcceptChild(parent_type, type)) {
            valid_types.push_back(type);
        }
    }
    
    return valid_types;
}

std::vector<std::string> GetValidChildTypes(const std::string& parent_type) {
    auto types = GetValidChildTypes(StringToDiagramNodeType(parent_type));
    std::vector<std::string> result;
    for (auto type : types) {
        result.push_back(DiagramNodeTypeToString(type));
    }
    return result;
}

} // namespace scratchrobin
