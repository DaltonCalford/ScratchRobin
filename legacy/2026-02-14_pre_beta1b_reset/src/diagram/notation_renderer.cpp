/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "diagram/notation_renderer.h"

#include <wx/graphics.h>
#include <wx/font.h>

#include <cmath>

namespace scratchrobin {
namespace diagram {

namespace {

// Constants for rendering
constexpr double kDefaultEntityWidth = 150.0;
constexpr double kDefaultEntityHeight = 100.0;
constexpr double kMinEntityWidth = 80.0;
constexpr double kMinEntityHeight = 50.0;
constexpr double kAttributeHeight = 18.0;
constexpr double kHeaderHeight = 24.0;
constexpr double kCornerRadius = 5.0;
constexpr double kSelectionMargin = 4.0;

// Helper to set text color based on background
wxColor GetContrastColor(const wxColor& bg) {
    int brightness = (bg.Red() * 299 + bg.Green() * 587 + bg.Blue() * 114) / 1000;
    return brightness > 128 ? *wxBLACK : *wxWHITE;
}

// Helper to draw text centered
void DrawCenteredText(wxGraphicsContext* gc, const std::string& text, double x, double y, double maxWidth) {
    wxString wxText(text);
    wxDouble width, height;
    gc->GetTextExtent(wxText, &width, &height);
    
    // Truncate if too long
    if (width > maxWidth && maxWidth > 20) {
        wxString ellipsis = "...";
        wxDouble ellipsisWidth;
        gc->GetTextExtent(ellipsis, &ellipsisWidth, &height);
        
        while (width + ellipsisWidth > maxWidth && wxText.length() > 1) {
            wxText = wxText.Left(wxText.length() - 1);
            gc->GetTextExtent(wxText, &width, &height);
        }
        wxText += ellipsis;
    }
    
    gc->DrawText(wxText, x - width / 2, y - height / 2);
}

// Helper to draw text left-aligned
void DrawLeftText(wxGraphicsContext* gc, const std::string& text, double x, double y, double maxWidth) {
    wxString wxText(text);
    wxDouble width, height;
    gc->GetTextExtent(wxText, &width, &height);
    
    // Truncate if too long
    if (width > maxWidth && maxWidth > 20) {
        wxString ellipsis = "...";
        wxDouble ellipsisWidth;
        gc->GetTextExtent(ellipsis, &ellipsisWidth, &height);
        
        while (width + ellipsisWidth > maxWidth && wxText.length() > 1) {
            wxText = wxText.Left(wxText.length() - 1);
            gc->GetTextExtent(wxText, &width, &height);
        }
        wxText += ellipsis;
    }
    
    gc->DrawText(wxText, x, y);
}

// Calculate entity bounds based on attributes
Rect CalculateEntityBounds(const Entity& entity) {
    double width = std::max(kMinEntityWidth, entity.width);
    double height = std::max(kMinEntityHeight, entity.height);
    
    if (!entity.attributes.empty()) {
        double attrHeight = kHeaderHeight + (entity.attributes.size() * kAttributeHeight) + 8;
        height = std::max(height, attrHeight);
    }
    
    return Rect(entity.x - width / 2, entity.y - height / 2, width, height);
}

// Get point on rectangle border closest to target
Point GetRectBorderPoint(const Rect& rect, const Point& target) {
    double cx = rect.m_x + rect.m_width / 2;
    double cy = rect.m_y + rect.m_height / 2;
    
    double dx = target.m_x - cx;
    double dy = target.m_y - cy;
    
    if (std::abs(dx) < 0.001 && std::abs(dy) < 0.001) {
        return Point(cx + rect.m_width / 2, cy); // Right edge
    }
    
    // Calculate intersection with rectangle edges
    double scaleX = (rect.m_width / 2) / std::abs(dx);
    double scaleY = (rect.m_height / 2) / std::abs(dy);
    double scale = std::min(scaleX, scaleY);
    
    return Point(cx + dx * scale, cy + dy * scale);
}

// Distance from point to line segment
double PointToLineDistance(const Point& p, const Point& lineStart, const Point& lineEnd) {
    double dx = lineEnd.m_x - lineStart.m_x;
    double dy = lineEnd.m_y - lineStart.m_y;
    double len2 = dx * dx + dy * dy;
    
    if (len2 == 0) {
        return std::hypot(p.m_x - lineStart.m_x, p.m_y - lineStart.m_y);
    }
    
    double t = std::max(0.0, std::min(1.0, ((p.m_x - lineStart.m_x) * dx + (p.m_y - lineStart.m_y) * dy) / len2));
    double projX = lineStart.m_x + t * dx;
    double projY = lineStart.m_y + t * dy;
    
    return std::hypot(p.m_x - projX, p.m_y - projY);
}

} // anonymous namespace

// ============================================================================
// Factory Method
// ============================================================================

std::unique_ptr<NotationRenderer> NotationRenderer::Create(NotationType notation) {
    switch (notation) {
        case NotationType::CrowsFoot:
            return std::make_unique<CrowsFootRenderer>();
        case NotationType::IDEF1X:
            return std::make_unique<IDEF1XRenderer>();
        case NotationType::UML:
            return std::make_unique<UMLRenderer>();
        case NotationType::Chen:
            return std::make_unique<ChenRenderer>();
    }
    return std::make_unique<CrowsFootRenderer>();
}

// ============================================================================
// CrowsFootRenderer Implementation
// ============================================================================

void CrowsFootRenderer::DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) {
    Rect bounds = GetEntityBounds(entity);
    
    // Selection highlight
    if (selected) {
        gc->SetBrush(wxBrush(wxColor(200, 220, 255)));
        gc->SetPen(wxPen(wxColor(0, 100, 200), 2));
        gc->DrawRoundedRectangle(bounds.m_x - kSelectionMargin, bounds.m_y - kSelectionMargin,
                                  bounds.m_width + 2 * kSelectionMargin, 
                                  bounds.m_height + 2 * kSelectionMargin, kCornerRadius);
    }
    
    // Entity body
    gc->SetBrush(wxBrush(wxColor(255, 255, 240)));
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
    
    // Header separator
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->StrokeLine(bounds.m_x, bounds.m_y + kHeaderHeight, 
                   bounds.m_x + bounds.m_width, bounds.m_y + kHeaderHeight);
    
    // Entity name
    gc->SetFont(wxFontInfo(10).Bold(), *wxBLACK);
    DrawCenteredText(gc, entity.name, 
                     bounds.m_x + bounds.m_width / 2, 
                     bounds.m_y + kHeaderHeight / 2,
                     bounds.m_width - 10);
    
    // Attributes
    gc->SetFont(wxFontInfo(9), *wxBLACK);
    double attrY = bounds.m_y + kHeaderHeight + 4;
    for (const auto& attr : entity.attributes) {
        std::string attrText = attr.name;
        if (!attr.data_type.empty()) {
            attrText += " : " + attr.data_type;
        }
        
        // Primary key indicator
        if (attr.is_primary) {
            gc->SetFont(wxFontInfo(9).Bold(), *wxBLACK);
            attrText = "# " + attrText;
        } else if (attr.is_foreign) {
            gc->SetFont(wxFontInfo(9).Italic(), *wxBLACK);
            attrText = "* " + attrText;
        } else {
            gc->SetFont(wxFontInfo(9), *wxBLACK);
        }
        
        DrawLeftText(gc, attrText, bounds.m_x + 8, attrY, bounds.m_width - 16);
        attrY += kAttributeHeight;
    }
}

void CrowsFootRenderer::DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                                          const Entity& parent, const Entity& child, bool selected) {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2, 
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    // Draw connection line
    gc->SetPen(wxPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1));
    gc->StrokeLine(start.m_x, start.m_y, end.m_x, end.m_y);
    
    // Calculate angle for symbols
    double angle = std::atan2(end.m_y - start.m_y, end.m_x - start.m_x);
    
    // Draw cardinality symbols at parent end (always "one" for parent)
    DrawCrowsFoot(gc, start, angle + M_PI, CardinalityType::One);
    
    // Draw cardinality symbols at child end
    DrawCrowsFoot(gc, end, angle, ToCardinalityType(rel.target_cardinality));
    
    // Draw relationship label if present
    if (!rel.label.empty()) {
        double midX = (start.m_x + end.m_x) / 2;
        double midY = (start.m_y + end.m_y) / 2;
        
        gc->SetFont(wxFontInfo(8), *wxBLACK);
        wxDouble width, height;
        gc->GetTextExtent(rel.label, &width, &height);
        
        // Background for label
        gc->SetBrush(wxBrush(*wxWHITE));
        gc->SetPen(wxPen(*wxWHITE));
        gc->DrawRectangle(midX - width/2 - 2, midY - height/2 - 2, width + 4, height + 4);
        
        gc->SetFont(wxFontInfo(8), *wxBLACK);
        gc->DrawText(rel.label, midX - width/2, midY - height/2);
    }
}

void CrowsFootRenderer::DrawCrowsFoot(wxGraphicsContext* gc, const Point& pos, double angle,
                                       CardinalityType cardinality) {
    const double symbolSize = 10.0;
    
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->SetBrush(wxBrush(*wxTRANSPARENT_BRUSH));
    
    // Calculate perpendicular vector
    double perpX = -std::sin(angle);
    double perpY = std::cos(angle);
    
    switch (cardinality) {
        case CardinalityType::One:
            // Single vertical bar
            gc->StrokeLine(pos.m_x + perpX * symbolSize/2, pos.m_y + perpY * symbolSize/2,
                          pos.m_x - perpX * symbolSize/2, pos.m_y - perpY * symbolSize/2);
            break;
            
        case CardinalityType::ZeroOrOne:
            // Circle + vertical bar
            gc->DrawEllipse(pos.m_x - symbolSize/2, pos.m_y - symbolSize/2, symbolSize, symbolSize);
            gc->StrokeLine(pos.m_x + perpX * symbolSize, pos.m_y + perpY * symbolSize,
                          pos.m_x - perpX * symbolSize, pos.m_y - perpY * symbolSize);
            break;
            
        case CardinalityType::OneOrMany:
            // Vertical bar + crow's foot
            gc->StrokeLine(pos.m_x + perpX * symbolSize/2, pos.m_y + perpY * symbolSize/2,
                          pos.m_x - perpX * symbolSize/2, pos.m_y - perpY * symbolSize/2);
            // Crow's foot lines
            gc->StrokeLine(pos.m_x, pos.m_y,
                          pos.m_x + std::cos(angle + 0.5) * symbolSize, 
                          pos.m_y + std::sin(angle + 0.5) * symbolSize);
            gc->StrokeLine(pos.m_x, pos.m_y,
                          pos.m_x + std::cos(angle - 0.5) * symbolSize, 
                          pos.m_y + std::sin(angle - 0.5) * symbolSize);
            break;
            
        case CardinalityType::ZeroOrMany:
            // Circle + crow's foot
            gc->DrawEllipse(pos.m_x - symbolSize/2, pos.m_y - symbolSize/2, symbolSize, symbolSize);
            gc->StrokeLine(pos.m_x, pos.m_y,
                          pos.m_x + std::cos(angle + 0.5) * symbolSize * 1.5, 
                          pos.m_y + std::sin(angle + 0.5) * symbolSize * 1.5);
            gc->StrokeLine(pos.m_x, pos.m_y,
                          pos.m_x + std::cos(angle - 0.5) * symbolSize * 1.5, 
                          pos.m_y + std::sin(angle - 0.5) * symbolSize * 1.5);
            break;
    }
}

Rect CrowsFootRenderer::GetEntityBounds(const Entity& entity) const {
    return CalculateEntityBounds(entity);
}

Point CrowsFootRenderer::GetConnectionPoint(const Entity& entity, const Point& target) const {
    Rect bounds = GetEntityBounds(entity);
    return GetRectBorderPoint(bounds, target);
}

bool CrowsFootRenderer::HitTestRelationship(const Relationship& rel, const Point& test_point,
                                             const Entity& parent, const Entity& child) const {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    return PointToLineDistance(test_point, start, end) < 5.0;
}

// ============================================================================
// IDEF1XRenderer Implementation
// ============================================================================

void IDEF1XRenderer::DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) {
    Rect bounds = GetEntityBounds(entity);
    
    // IDEF1X uses rounded corners for dependent entities
    bool isDependent = !entity.parent_id.empty();
    
    // Selection highlight
    if (selected) {
        gc->SetBrush(wxBrush(wxColor(200, 220, 255)));
        gc->SetPen(wxPen(wxColor(0, 100, 200), 2));
        if (isDependent) {
            gc->DrawRoundedRectangle(bounds.m_x - kSelectionMargin, bounds.m_y - kSelectionMargin,
                                      bounds.m_width + 2 * kSelectionMargin,
                                      bounds.m_height + 2 * kSelectionMargin, 8);
        } else {
            gc->DrawRectangle(bounds.m_x - kSelectionMargin, bounds.m_y - kSelectionMargin,
                             bounds.m_width + 2 * kSelectionMargin,
                             bounds.m_height + 2 * kSelectionMargin);
        }
    }
    
    // Entity body
    gc->SetBrush(wxBrush(wxColor(255, 255, 255)));
    gc->SetPen(wxPen(wxColor(0, 0, 0), 2));
    
    if (isDependent) {
        // Dependent entities have rounded corners
        gc->DrawRoundedRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height, 8);
    } else {
        // Independent entities have square corners
        gc->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
    }
    
    // Header area (shaded for independent entities)
    if (!isDependent) {
        gc->SetBrush(wxBrush(wxColor(220, 220, 220)));
        gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
        gc->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, kHeaderHeight);
    }
    
    // Header separator
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->StrokeLine(bounds.m_x, bounds.m_y + kHeaderHeight,
                   bounds.m_x + bounds.m_width, bounds.m_y + kHeaderHeight);
    
    // Entity name
    gc->SetFont(wxFontInfo(10).Bold(), *wxBLACK);
    DrawCenteredText(gc, entity.name,
                     bounds.m_x + bounds.m_width / 2,
                     bounds.m_y + kHeaderHeight / 2,
                     bounds.m_width - 10);
    
    // Primary key attributes (in header area)
    gc->SetFont(wxFontInfo(9).Bold(), *wxBLACK);
    double attrY = bounds.m_y + kHeaderHeight + 4;
    
    for (const auto& attr : entity.attributes) {
        if (!attr.is_primary) continue;
        
        std::string attrText = attr.name;
        if (!attr.data_type.empty()) {
            attrText += " : " + attr.data_type;
        }
        
        DrawLeftText(gc, attrText, bounds.m_x + 8, attrY, bounds.m_width - 16);
        attrY += kAttributeHeight;
    }
    
    // Non-key attributes
    gc->SetFont(wxFontInfo(9), *wxBLACK);
    for (const auto& attr : entity.attributes) {
        if (attr.is_primary) continue;
        
        std::string attrText = attr.name;
        if (!attr.data_type.empty()) {
            attrText += " : " + attr.data_type;
        }
        
        DrawLeftText(gc, attrText, bounds.m_x + 8, attrY, bounds.m_width - 16);
        attrY += kAttributeHeight;
    }
}

void IDEF1XRenderer::DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                                       const Entity& parent, const Entity& child, bool selected) {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    // IDEF1X uses solid lines for identifying, dashed for non-identifying
    if (rel.identifying) {
        gc->SetPen(wxPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1));
    } else {
        wxPen dashedPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1);
        dashedPen.SetStyle(wxPENSTYLE_SHORT_DASH);
        gc->SetPen(dashedPen);
    }
    
    gc->StrokeLine(start.m_x, start.m_y, end.m_x, end.m_y);
    
    // Draw cardinality dots
    double angle = std::atan2(end.m_y - start.m_y, end.m_x - start.m_x);
    
    // Parent end - always "P" (one) in IDEF1X, shown as a dot
    gc->SetBrush(wxBrush(wxColor(0, 0, 0)));
    gc->DrawEllipse(start.m_x - 3, start.m_y - 3, 6, 6);
    
    // Child end - cardinality
    CardinalityType card = ToCardinalityType(rel.target_cardinality);
    if (card == CardinalityType::ZeroOrOne || card == CardinalityType::ZeroOrMany) {
        // Open diamond for optional
        gc->SetBrush(wxBrush(*wxWHITE));
        gc->SetPen(wxPen(wxColor(0, 0, 0)));
        
        wxPoint2DDouble diamond[4];
        double diamondSize = 6;
        diamond[0] = wxPoint2DDouble(end.m_x + std::cos(angle) * diamondSize, 
                                      end.m_y + std::sin(angle) * diamondSize);
        diamond[1] = wxPoint2DDouble(end.m_x + std::cos(angle + M_PI/2) * diamondSize,
                                      end.m_y + std::sin(angle + M_PI/2) * diamondSize);
        diamond[2] = wxPoint2DDouble(end.m_x - std::cos(angle) * diamondSize,
                                      end.m_y - std::sin(angle) * diamondSize);
        diamond[3] = wxPoint2DDouble(end.m_x + std::cos(angle - M_PI/2) * diamondSize,
                                      end.m_y + std::sin(angle - M_PI/2) * diamondSize);
        
        gc->DrawLines(4, diamond);
    } else {
        // Solid diamond for mandatory
        gc->SetBrush(wxBrush(wxColor(0, 0, 0)));
        gc->SetPen(wxPen(wxColor(0, 0, 0)));
        
        wxPoint2DDouble diamond[4];
        double diamondSize = 6;
        diamond[0] = wxPoint2DDouble(end.m_x + std::cos(angle) * diamondSize,
                                      end.m_y + std::sin(angle) * diamondSize);
        diamond[1] = wxPoint2DDouble(end.m_x + std::cos(angle + M_PI/2) * diamondSize,
                                      end.m_y + std::sin(angle + M_PI/2) * diamondSize);
        diamond[2] = wxPoint2DDouble(end.m_x - std::cos(angle) * diamondSize,
                                      end.m_y - std::sin(angle) * diamondSize);
        diamond[3] = wxPoint2DDouble(end.m_x + std::cos(angle - M_PI/2) * diamondSize,
                                      end.m_y + std::sin(angle - M_PI/2) * diamondSize);
        
        gc->DrawLines(4, diamond);
    }
}

Rect IDEF1XRenderer::GetEntityBounds(const Entity& entity) const {
    return CalculateEntityBounds(entity);
}

Point IDEF1XRenderer::GetConnectionPoint(const Entity& entity, const Point& target) const {
    Rect bounds = GetEntityBounds(entity);
    return GetRectBorderPoint(bounds, target);
}

bool IDEF1XRenderer::HitTestRelationship(const Relationship& rel, const Point& test_point,
                                          const Entity& parent, const Entity& child) const {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    return PointToLineDistance(test_point, start, end) < 5.0;
}

// ============================================================================
// UMLRenderer Implementation
// ============================================================================

void UMLRenderer::DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) {
    Rect bounds = GetEntityBounds(entity);
    
    // UML uses three compartments: name, attributes, operations
    // For ERD, we use name and attributes
    
    // Selection highlight
    if (selected) {
        gc->SetBrush(wxBrush(wxColor(200, 220, 255)));
        gc->SetPen(wxPen(wxColor(0, 100, 200), 2));
        gc->DrawRectangle(bounds.m_x - kSelectionMargin, bounds.m_y - kSelectionMargin,
                         bounds.m_width + 2 * kSelectionMargin,
                         bounds.m_height + 2 * kSelectionMargin);
    }
    
    // Entity body (white background)
    gc->SetBrush(wxBrush(wxColor(255, 255, 255)));
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
    
    // Name compartment (bold, centered)
    gc->SetFont(wxFontInfo(10).Bold(), *wxBLACK);
    DrawCenteredText(gc, entity.name,
                     bounds.m_x + bounds.m_width / 2,
                     bounds.m_y + kHeaderHeight / 2,
                     bounds.m_width - 10);
    
    // First separator
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->StrokeLine(bounds.m_x, bounds.m_y + kHeaderHeight,
                   bounds.m_x + bounds.m_width, bounds.m_y + kHeaderHeight);
    
    // Attributes compartment
    gc->SetFont(wxFontInfo(9), *wxBLACK);
    double attrY = bounds.m_y + kHeaderHeight + 4;
    
    for (const auto& attr : entity.attributes) {
        std::string attrText = "+ " + attr.name;
        if (!attr.data_type.empty()) {
            attrText += " : " + attr.data_type;
        }
        
        if (attr.is_primary) {
            gc->SetFont(wxFontInfo(9).Bold().Underlined(), *wxBLACK);
        } else if (attr.is_foreign) {
            gc->SetFont(wxFontInfo(9).Italic(), *wxBLACK);
        } else {
            gc->SetFont(wxFontInfo(9), *wxBLACK);
        }
        
        DrawLeftText(gc, attrText, bounds.m_x + 8, attrY, bounds.m_width - 16);
        attrY += kAttributeHeight;
    }
    
    // Operations compartment (if any operations defined)
    // For ERD, we can leave this empty or show calculated fields
}

void UMLRenderer::DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                                    const Entity& parent, const Entity& child, bool selected) {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    // UML uses different line styles for different relationship types
    // Association: solid line
    // Aggregation: hollow diamond on parent end
    // Composition: filled diamond on parent end
    
    gc->SetPen(wxPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1));
    gc->StrokeLine(start.m_x, start.m_y, end.m_x, end.m_y);
    
    // Calculate angle
    double angle = std::atan2(end.m_y - start.m_y, end.m_x - start.m_x);
    
    // Draw multiplicity labels
    gc->SetFont(wxFontInfo(8), *wxBLACK);
    
    // Parent multiplicity (always "1" for parent in ERD context)
    std::string parentMult = "1";
    wxDouble pw, ph;
    gc->GetTextExtent(parentMult, &pw, &ph);
    double parentLabelX = start.m_x + std::cos(angle + M_PI/4) * 15;
    double parentLabelY = start.m_y + std::sin(angle + M_PI/4) * 15;
    gc->DrawText(parentMult, parentLabelX - pw/2, parentLabelY - ph/2);
    
    // Child multiplicity
    std::string childMult;
    switch (rel.target_cardinality) {
        case Cardinality::One: childMult = "1"; break;
        case Cardinality::ZeroOrOne: childMult = "0..1"; break;
        case Cardinality::OneOrMany: childMult = "1..*"; break;
        case Cardinality::ZeroOrMany: childMult = "0..*"; break;
    }
    wxDouble cw, ch;
    gc->GetTextExtent(childMult, &cw, &ch);
    double childLabelX = end.m_x + std::cos(angle - M_PI/4 * 3) * 15;
    double childLabelY = end.m_y + std::sin(angle - M_PI/4 * 3) * 15;
    gc->DrawText(childMult, childLabelX - cw/2, childLabelY - ch/2);
    
    // Relationship label
    if (!rel.label.empty()) {
        double midX = (start.m_x + end.m_x) / 2;
        double midY = (start.m_y + end.m_y) / 2;
        
        wxDouble lw, lh;
        gc->GetTextExtent(rel.label, &lw, &lh);
        
        gc->SetBrush(wxBrush(*wxWHITE));
        gc->SetPen(wxPen(*wxWHITE));
        gc->DrawRectangle(midX - lw/2 - 2, midY - lh/2 - 2, lw + 4, lh + 4);
        
        gc->SetFont(wxFontInfo(8), *wxBLACK);
        gc->DrawText(rel.label, midX - lw/2, midY - lh/2);
    }
}

Rect UMLRenderer::GetEntityBounds(const Entity& entity) const {
    return CalculateEntityBounds(entity);
}

Point UMLRenderer::GetConnectionPoint(const Entity& entity, const Point& target) const {
    Rect bounds = GetEntityBounds(entity);
    return GetRectBorderPoint(bounds, target);
}

bool UMLRenderer::HitTestRelationship(const Relationship& rel, const Point& test_point,
                                       const Entity& parent, const Entity& child) const {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    return PointToLineDistance(test_point, start, end) < 5.0;
}

// ============================================================================
// ChenRenderer Implementation
// ============================================================================

void ChenRenderer::DrawEntity(wxGraphicsContext* gc, const Entity& entity, bool selected) {
    Rect bounds = GetEntityBounds(entity);
    
    // Chen notation uses rectangles for entities
    // Weak entities have double borders
    bool isWeak = !entity.parent_id.empty();
    
    // Selection highlight
    if (selected) {
        gc->SetBrush(wxBrush(wxColor(200, 220, 255)));
        gc->SetPen(wxPen(wxColor(0, 100, 200), 2));
        gc->DrawRectangle(bounds.m_x - kSelectionMargin - 2, bounds.m_y - kSelectionMargin - 2,
                         bounds.m_width + 2 * kSelectionMargin + 4,
                         bounds.m_height + 2 * kSelectionMargin + 4);
    }
    
    // Entity body (light yellow/cream for regular entities)
    gc->SetBrush(wxBrush(wxColor(255, 255, 220)));
    gc->SetPen(wxPen(wxColor(0, 0, 0), isWeak ? 2 : 1));
    gc->DrawRectangle(bounds.m_x, bounds.m_y, bounds.m_width, bounds.m_height);
    
    // Double border for weak entities
    if (isWeak) {
        gc->DrawRectangle(bounds.m_x + 3, bounds.m_y + 3, 
                         bounds.m_width - 6, bounds.m_height - 6);
    }
    
    // Entity name (centered, bold)
    gc->SetFont(wxFontInfo(10).Bold(), *wxBLACK);
    DrawCenteredText(gc, entity.name,
                     bounds.m_x + bounds.m_width / 2,
                     bounds.m_y + kHeaderHeight / 2,
                     bounds.m_width - 10);
    
    // Separator
    gc->SetPen(wxPen(wxColor(0, 0, 0), 1));
    gc->StrokeLine(bounds.m_x, bounds.m_y + kHeaderHeight,
                   bounds.m_x + bounds.m_width, bounds.m_y + kHeaderHeight);
    
    // In Chen notation, attributes are shown as ovals connected to entities
    // For this implementation, we show them inside for simplicity
    gc->SetFont(wxFontInfo(9), *wxBLACK);
    double attrY = bounds.m_y + kHeaderHeight + 4;
    
    for (const auto& attr : entity.attributes) {
        std::string attrText = attr.name;
        
        if (attr.is_primary) {
            gc->SetFont(wxFontInfo(9).Bold().Underlined(), *wxBLACK);
            attrText = "* " + attrText;
        } else if (attr.is_foreign) {
            gc->SetFont(wxFontInfo(9), *wxBLACK);
            attrText = "o " + attrText;
        } else {
            gc->SetFont(wxFontInfo(9), *wxBLACK);
            attrText = "o " + attrText;
        }
        
        DrawLeftText(gc, attrText, bounds.m_x + 8, attrY, bounds.m_width - 16);
        attrY += kAttributeHeight;
    }
}

void ChenRenderer::DrawRelationship(wxGraphicsContext* gc, const Relationship& rel,
                                     const Entity& parent, const Entity& child, bool selected) {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    // Chen notation uses diamonds for relationships
    double midX = (start.m_x + end.m_x) / 2;
    double midY = (start.m_y + end.m_y) / 2;
    
    // Draw diamond
    double diamondWidth = 30;
    double diamondHeight = 20;
    
    wxPoint2DDouble diamond[4];
    diamond[0] = wxPoint2DDouble(midX, midY - diamondHeight);        // Top
    diamond[1] = wxPoint2DDouble(midX + diamondWidth, midY);         // Right
    diamond[2] = wxPoint2DDouble(midX, midY + diamondHeight);        // Bottom
    diamond[3] = wxPoint2DDouble(midX - diamondWidth, midY);         // Left
    
    gc->SetBrush(wxBrush(wxColor(220, 220, 255)));
    gc->SetPen(wxPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1));
    gc->DrawLines(4, diamond);
    gc->StrokeLine(diamond[3].m_x, diamond[3].m_y, diamond[0].m_x, diamond[0].m_y); // Close the polygon
    
    // Relationship label inside diamond
    if (!rel.label.empty()) {
        gc->SetFont(wxFontInfo(8), *wxBLACK);
        wxDouble lw, lh;
        gc->GetTextExtent(rel.label, &lw, &lh);
        gc->DrawText(rel.label, midX - lw/2, midY - lh/2);
    }
    
    // Draw lines from entities to diamond
    gc->SetPen(wxPen(selected ? wxColor(0, 100, 200) : wxColor(0, 0, 0), selected ? 2 : 1));
    gc->StrokeLine(start.m_x, start.m_y, diamond[3].m_x, diamond[3].m_y);
    gc->StrokeLine(diamond[1].m_x, diamond[1].m_y, end.m_x, end.m_y);
    
    // Draw cardinality labels
    gc->SetFont(wxFontInfo(8), *wxBLACK);
    
    // Parent cardinality (always 1 for parent)
    std::string parentCard = "1";
    wxDouble pw, ph;
    gc->GetTextExtent(parentCard, &pw, &ph);
    double parentLabelX = (start.m_x + diamond[3].m_x) / 2;
    double parentLabelY = (start.m_y + diamond[3].m_y) / 2;
    gc->DrawText(parentCard, parentLabelX - pw/2, parentLabelY - ph/2);
    
    // Child cardinality
    std::string childCard;
    switch (rel.target_cardinality) {
        case Cardinality::One: childCard = "1"; break;
        case Cardinality::ZeroOrOne: childCard = "1"; break; // Chen uses 1 for optional too
        case Cardinality::OneOrMany: childCard = "M"; break;
        case Cardinality::ZeroOrMany: childCard = "N"; break;
    }
    wxDouble cw, ch;
    gc->GetTextExtent(childCard, &cw, &ch);
    double childLabelX = (diamond[1].m_x + end.m_x) / 2;
    double childLabelY = (diamond[1].m_y + end.m_y) / 2;
    gc->DrawText(childCard, childLabelX - cw/2, childLabelY - ch/2);
}

void ChenRenderer::DrawDiamond(wxGraphicsContext* gc, const Point& center, 
                                double width, double height) {
    wxPoint2DDouble diamond[4];
    diamond[0] = wxPoint2DDouble(center.m_x, center.m_y - height);
    diamond[1] = wxPoint2DDouble(center.m_x + width, center.m_y);
    diamond[2] = wxPoint2DDouble(center.m_x, center.m_y + height);
    diamond[3] = wxPoint2DDouble(center.m_x - width, center.m_y);
    
    gc->StrokeLine(diamond[0].m_x, diamond[0].m_y, diamond[1].m_x, diamond[1].m_y);
    gc->StrokeLine(diamond[1].m_x, diamond[1].m_y, diamond[2].m_x, diamond[2].m_y);
    gc->StrokeLine(diamond[2].m_x, diamond[2].m_y, diamond[3].m_x, diamond[3].m_y);
    gc->StrokeLine(diamond[3].m_x, diamond[3].m_y, diamond[0].m_x, diamond[0].m_y);
}

void ChenRenderer::DrawAttributeOval(wxGraphicsContext* gc, const Point& center, 
                                      const std::string& name) {
    // Not used in current implementation, but would draw ovals for attributes
}

Rect ChenRenderer::GetEntityBounds(const Entity& entity) const {
    return CalculateEntityBounds(entity);
}

Point ChenRenderer::GetConnectionPoint(const Entity& entity, const Point& target) const {
    Rect bounds = GetEntityBounds(entity);
    return GetRectBorderPoint(bounds, target);
}

bool ChenRenderer::HitTestRelationship(const Relationship& rel, const Point& test_point,
                                        const Entity& parent, const Entity& child) const {
    Rect parentBounds = GetEntityBounds(parent);
    Rect childBounds = GetEntityBounds(child);
    
    Point parentCenter(parentBounds.m_x + parentBounds.m_width / 2,
                       parentBounds.m_y + parentBounds.m_height / 2);
    Point childCenter(childBounds.m_x + childBounds.m_width / 2,
                      childBounds.m_y + childBounds.m_height / 2);
    
    Point start = GetConnectionPoint(parent, childCenter);
    Point end = GetConnectionPoint(child, parentCenter);
    
    double midX = (start.m_x + end.m_x) / 2;
    double midY = (start.m_y + end.m_y) / 2;
    
    // Hit test against diamond
    double diamondWidth = 30;
    double diamondHeight = 20;
    
    // Simple bounding box test for diamond
    if (test_point.m_x >= midX - diamondWidth && test_point.m_x <= midX + diamondWidth &&
        test_point.m_y >= midY - diamondHeight && test_point.m_y <= midY + diamondHeight) {
        return true;
    }
    
    // Hit test against lines
    return PointToLineDistance(test_point, start, Point(midX - diamondWidth, midY)) < 5.0 ||
           PointToLineDistance(test_point, Point(midX + diamondWidth, midY), end) < 5.0;
}

} // namespace diagram
} // namespace scratchrobin
