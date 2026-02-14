/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_NOTATION_RENDERER_H
#define SCRATCHROBIN_DIAGRAM_NOTATION_RENDERER_H

#include "diagram/diagram_types.h"

#include <wx/wx.h>
#include <wx/graphics.h>

#include <memory>

namespace scratchrobin {
namespace diagram {

// Abstract base class for notation renderers
class NotationRenderer {
public:
    virtual ~NotationRenderer() = default;
    
    // Get the notation type this renderer handles
    virtual NotationType GetNotationType() const = 0;
    
    // Entity rendering
    virtual void DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) = 0;
    
    // Relationship rendering
    virtual void DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                                   const Entity& parent, const Entity& child,
                                   bool selected) = 0;
    
    // Get entity bounds (accounting for notation-specific decorations)
    virtual Rect GetEntityBounds(const Entity& entity) const = 0;
    
    // Get connection point on entity border
    virtual Point GetConnectionPoint(const Entity& entity, const Point& target) const = 0;
    
    // Hit test for relationship
    virtual bool HitTestRelationship(const Relationship& rel, const Point& test_point,
                                      const Entity& parent, const Entity& child) const = 0;
    
    // Factory method to create appropriate renderer
    static std::unique_ptr<NotationRenderer> Create(NotationType notation);
};

// Crow's Foot notation renderer
class CrowsFootRenderer : public NotationRenderer {
public:
    NotationType GetNotationType() const override { return NotationType::CrowsFoot; }
    
    void DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) override;
    void DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                          const Entity& parent, const Entity& child, bool selected) override;
    Rect GetEntityBounds(const Entity& entity) const override;
    Point GetConnectionPoint(const Entity& entity, const Point& target) const override;
    bool HitTestRelationship(const Relationship& rel, const Point& test_point,
                             const Entity& parent, const Entity& child) const override;

private:
    void DrawCrowsFoot(wxGraphicsContext* gc, const Point& pos, double angle,
                       CardinalityType cardinality);
    void DrawCardinalitySymbol(wxGraphicsContext* gc, const Point& pos, double angle,
                               CardinalityType cardinality, bool is_parent);
};

// IDEF1X notation renderer
class IDEF1XRenderer : public NotationRenderer {
public:
    NotationType GetNotationType() const override { return NotationType::IDEF1X; }
    
    void DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) override;
    void DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                          const Entity& parent, const Entity& child, bool selected) override;
    Rect GetEntityBounds(const Entity& entity) const override;
    Point GetConnectionPoint(const Entity& entity, const Point& target) const override;
    bool HitTestRelationship(const Relationship& rel, const Point& test_point,
                             const Entity& parent, const Entity& child) const override;

private:
    void DrawIDEF1XConnection(wxGraphicsContext* gc, const Point& pos, double angle,
                              CardinalityType cardinality, bool is_identifying);
};

// UML notation renderer
class UMLRenderer : public NotationRenderer {
public:
    NotationType GetNotationType() const override { return NotationType::UML; }
    
    void DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) override;
    void DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                          const Entity& parent, const Entity& child, bool selected) override;
    Rect GetEntityBounds(const Entity& entity) const override;
    Point GetConnectionPoint(const Entity& entity, const Point& target) const override;
    bool HitTestRelationship(const Relationship& rel, const Point& test_point,
                             const Entity& parent, const Entity& child) const override;

private:
    void DrawMultiplicity(wxGraphicsContext* gc, const Point& pos, double angle,
                          CardinalityType cardinality);
};

// Chen notation renderer
class ChenRenderer : public NotationRenderer {
public:
    NotationType GetNotationType() const override { return NotationType::Chen; }
    
    void DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) override;
    void DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                          const Entity& parent, const Entity& child, bool selected) override;
    Rect GetEntityBounds(const Entity& entity) const override;
    Point GetConnectionPoint(const Entity& entity, const Point& target) const override;
    bool HitTestRelationship(const Relationship& rel, const Point& test_point,
                             const Entity& parent, const Entity& child) const override;

private:
    void DrawDiamond(wxGraphicsContext* gc, const Point& center, double width, double height);
    void DrawAttributeOval(wxGraphicsContext* gc, const Point& center, const std::string& name);
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_NOTATION_RENDERER_H
