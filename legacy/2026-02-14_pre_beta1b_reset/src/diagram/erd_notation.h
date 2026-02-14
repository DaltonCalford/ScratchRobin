/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_ERD_NOTATION_H
#define SCRATCHROBIN_DIAGRAM_ERD_NOTATION_H

#include <string>

namespace scratchrobin {

// Supported ERD notations
enum class ErdNotation {
    CrowsFoot,      // Crow's Foot / IE notation (most common)
    IDEF1X,         // Integration DEFinition for Information Modeling
    UML,            // Unified Modeling Language class diagrams
    Chen            // Chen notation (entities as rectangles, relationships as diamonds)
};

// Convert notation to string
inline std::string ErdNotationToString(ErdNotation notation) {
    switch (notation) {
        case ErdNotation::CrowsFoot: return "crowsfoot";
        case ErdNotation::IDEF1X: return "idef1x";
        case ErdNotation::UML: return "uml";
        case ErdNotation::Chen: return "chen";
        default: return "crowsfoot";
    }
}

// Parse notation from string
inline ErdNotation StringToErdNotation(const std::string& str) {
    if (str == "idef1x") return ErdNotation::IDEF1X;
    if (str == "uml") return ErdNotation::UML;
    if (str == "chen") return ErdNotation::Chen;
    return ErdNotation::CrowsFoot;
}

// Notation capabilities
struct NotationCapabilities {
    bool supports_identifying_relationships;  // Solid vs dashed lines
    bool supports_cardinality;                // 1, 0..1, 0..*, 1..*
    bool supports_optional;                   // Optional vs mandatory
    bool supports_weak_entities;              // Dependent/weak entities
    bool uses_diamonds_for_relationships;     // Chen notation
};

inline NotationCapabilities GetNotationCapabilities(ErdNotation notation) {
    switch (notation) {
        case ErdNotation::CrowsFoot:
            return {true, true, true, false, false};
        case ErdNotation::IDEF1X:
            return {true, true, false, true, false};
        case ErdNotation::UML:
            return {true, true, true, false, false};
        case ErdNotation::Chen:
            return {false, true, false, true, true};
        default:
            return {true, true, true, false, false};
    }
}

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_ERD_NOTATION_H
