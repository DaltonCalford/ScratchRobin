/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_TYPES_H
#define SCRATCHROBIN_DIAGRAM_TYPES_H

#include "../ui/diagram_model.h"

#include <wx/geometry.h>

namespace scratchrobin {
namespace diagram {

// Type aliases for notation renderer interface
using Entity = DiagramNode;
using Relationship = DiagramEdge;
using Rect = wxRect2DDouble;
using Point = wxPoint2DDouble;

// Cardinality type for notation rendering
enum class CardinalityType {
    One,           // Exactly one
    ZeroOrOne,     // Zero or one (optional)
    OneOrMany,     // One or many
    ZeroOrMany     // Zero or many
};

// Convert model Cardinality to renderer CardinalityType
inline CardinalityType ToCardinalityType(Cardinality c) {
    switch (c) {
        case Cardinality::One: return CardinalityType::One;
        case Cardinality::ZeroOrOne: return CardinalityType::ZeroOrOne;
        case Cardinality::OneOrMany: return CardinalityType::OneOrMany;
        case Cardinality::ZeroOrMany: return CardinalityType::ZeroOrMany;
    }
    return CardinalityType::One;
}

// Notation type enum (matches ErdNotation but in diagram namespace)
enum class NotationType {
    CrowsFoot,
    IDEF1X,
    UML,
    Chen
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_TYPES_H
