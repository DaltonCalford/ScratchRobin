/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_canvas.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include <wx/cursor.h>
#include <wx/dcbuffer.h>
#include <wx/geometry.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/utils.h>

#include "diagram/command.h"
#include "diagram/mindmap_util.h"
#include "diagram_containment.h"

namespace scratchrobin {

wxDEFINE_EVENT(EVT_DIAGRAM_SELECTION_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_DATA_VIEW_UPDATED, wxCommandEvent);
wxDEFINE_EVENT(EVT_DATA_VIEW_OPEN, wxCommandEvent);

namespace {

constexpr double kMinZoom = 0.1;
constexpr double kMaxZoom = 4.0;
constexpr double kZoomStep = 0.1;
constexpr int kDataViewHeaderHeight = 22;
constexpr int kDataViewRowHeight = 18;
constexpr double kMinErdWidth = 160.0;
constexpr double kMinErdHeight = 120.0;
constexpr double kMinSilverWidth = 120.0;
constexpr double kMinSilverHeight = 100.0;
constexpr int kHandleSize = 8;
constexpr int kHandleHitPadding = 4;
constexpr int kEdgeHandleSize = 8;
constexpr int kEdgeEndpointRadius = 12;

wxColour GridColor() {
    return wxColour(55, 55, 55);
}

wxColour CanvasColor() {
    return wxColour(45, 45, 45);
}

wxColour NodeFillColor() {
    return wxColour(62, 62, 62);
}

wxColour NodeBorderColor() {
    return wxColour(110, 110, 110);
}

wxColour SilverstonFillColor() {
    return wxColour(58, 60, 72);
}

wxColour SilverstonBorderColor() {
    return wxColour(132, 132, 150);
}

wxColour IconFillColor(const std::string& icon_set) {
    if (icon_set == "mono") {
        return wxColour(72, 78, 90);
    }
    if (icon_set == "accent") {
        return wxColour(100, 88, 140);
    }
    return wxColour(80, 88, 110);
}

wxColour SelectionColor() {
    return wxColour(0, 102, 204);
}

wxColour GhostColor(const wxColour& base) {
    return wxColour((base.Red() + 140) / 2,
                    (base.Green() + 140) / 2,
                    (base.Blue() + 140) / 2);
}

// Silverston visual polish colors
wxColour SilverstonAccentColor() {
    return wxColour(140, 180, 220);  // Light blue accent for notes indicator
}

wxColour SilverstonMutedColor() {
    return wxColour(140, 140, 150);  // Muted gray for empty notes
}

// Drag and drop visual feedback colors
wxColour ValidDropColor() {
    return wxColour(76, 175, 80);  // Green - valid drop target
}

wxColour InvalidDropColor() {
    return wxColour(244, 67, 54);  // Red - invalid drop target
}

wxColour DragHighlightColor(bool valid) {
    return valid ? ValidDropColor() : InvalidDropColor();
}

wxColour NoteBackgroundColor() {
    return wxColour(242, 230, 152);  // Yellow sticky note color
}

// Draw a rectangle with chamfered (cut) corner
// chamfer_size: size of the corner cut
// chamfer_corner: 0=top-right, 1=top-left, 2=bottom-left, 3=bottom-right
void DrawChamferedRect(wxDC& dc, const wxRect& rect, int chamfer_size, int chamfer_corner = 0) {
    int x = rect.GetX();
    int y = rect.GetY();
    int w = rect.GetWidth();
    int h = rect.GetHeight();
    int c = chamfer_size;
    
    wxPoint points[6];
    if (chamfer_corner == 0) {  // Top-right (default for Silverston notes)
        points[0] = wxPoint(x, y);
        points[1] = wxPoint(x + w - c, y);
        points[2] = wxPoint(x + w, y + c);
        points[3] = wxPoint(x + w, y + h);
        points[4] = wxPoint(x, y + h);
        points[5] = wxPoint(x, y);
    } else if (chamfer_corner == 1) {  // Top-left
        points[0] = wxPoint(x + c, y);
        points[1] = wxPoint(x + w, y);
        points[2] = wxPoint(x + w, y + h);
        points[3] = wxPoint(x, y + h);
        points[4] = wxPoint(x, y + c);
        points[5] = wxPoint(x + c, y);
    } else if (chamfer_corner == 2) {  // Bottom-left
        points[0] = wxPoint(x, y);
        points[1] = wxPoint(x + w, y);
        points[2] = wxPoint(x + w, y + h);
        points[3] = wxPoint(x + c, y + h);
        points[4] = wxPoint(x, y + h - c);
        points[5] = wxPoint(x, y);
    } else {  // Bottom-right
        points[0] = wxPoint(x, y);
        points[1] = wxPoint(x + w, y);
        points[2] = wxPoint(x + w, y + h - c);
        points[3] = wxPoint(x + w - c, y + h);
        points[4] = wxPoint(x, y + h);
        points[5] = wxPoint(x, y);
    }
    dc.DrawPolygon(6, points);
}

// Draw a simple key icon for PK indicators
void DrawKeyIcon(wxDC& dc, const wxPoint& center, int size) {
    // Draw a simple key shape
    int half = size / 2;
    wxPoint head(center.x - half + 2, center.y - 1);
    
    // Key head (circle)
    dc.DrawCircle(head, size / 3);
    
    // Key shaft
    dc.DrawLine(head.x + 2, head.y, center.x + half, center.y - 1);
    
    // Key teeth
    int teeth_x = center.x + half - 2;
    int teeth_y = center.y - 1;
    dc.DrawLine(teeth_x, teeth_y, teeth_x, teeth_y + 3);
    dc.DrawLine(teeth_x - 2, teeth_y, teeth_x - 2, teeth_y + 2);
}

// Draw a link icon for FK indicators
void DrawLinkIcon(wxDC& dc, const wxPoint& center, int size) {
    // Draw two chain links
    int offset = size / 4;
    wxPoint p1(center.x - offset, center.y);
    wxPoint p2(center.x + offset, center.y);
    
    // First link (oval shape approximated with lines)
    dc.DrawEllipse(p1.x - 3, p1.y - 2, 6, 4);
    // Second link
    dc.DrawEllipse(p2.x - 3, p2.y - 2, 6, 4);
    // Connection line
    dc.DrawLine(p1.x + 3, p1.y, p2.x - 3, p2.y);
}

// Draw Silverston name break: top border with gap for name
// Returns the width of the rendered name area (including notes indicator)
int DrawSilverstonNameBreak(wxDC& dc, const wxRect& rect, const std::string& name, 
                            const std::string& notes, bool interactive_mode,
                            wxColour border_color, wxColour text_color) {
    int x = rect.GetX();
    int y = rect.GetY();
    int w = rect.GetWidth();
    
    // Measure the name text
    wxSize name_size = dc.GetTextExtent(name);
    int name_height = name_size.GetHeight();
    
    // Calculate the name area with padding
    int padding = 6;
    int name_area_width = name_size.GetWidth() + (padding * 2);
    
    // Notes indicator width (if shown)
    int notes_indicator_width = 0;
    if (interactive_mode) {
        wxSize notes_size = dc.GetTextExtent("(...)");
        notes_indicator_width = notes_size.GetWidth() + 4;
        name_area_width += notes_indicator_width;
    }
    
    // Inset from left border
    int inset = 12;
    int name_start = x + inset;
    int name_end = name_start + name_area_width;
    
    // Ensure the gap doesn't exceed the rect width
    if (name_end > x + w - 20) {
        name_end = x + w - 20;
        name_start = name_end - name_area_width;
        if (name_start < x + inset) {
            name_start = x + inset;
        }
    }
    
    // Draw top border with gap
    wxPen border_pen(border_color, 1);
    dc.SetPen(border_pen);
    // Left segment
    dc.DrawLine(x, y, name_start, y);
    // Right segment (leave space for icons on the right)
    int icon_area = 40;  // Space reserved for type icons
    if (name_end < x + w - icon_area) {
        dc.DrawLine(name_end, y, x + w - icon_area + 10, y);
    }
    
    // Draw the name text (centered vertically on the border line)
    int name_text_x = name_start + padding;
    int name_text_y = y - name_height / 2 + 1;  // Center on the border line
    dc.SetTextForeground(text_color);
    dc.DrawText(name, name_text_x, name_text_y);
    
    // Draw notes indicator in Interactive Mode
    if (interactive_mode) {
        wxString notes_text = "(...)";
        wxSize notes_size = dc.GetTextExtent(notes_text);
        int notes_x = name_text_x + name_size.GetWidth() + 4;
        int notes_y = name_text_y;
        
        // Color based on whether notes are present
        bool has_notes = !notes.empty();
        wxColour notes_color = has_notes ? SilverstonAccentColor() : SilverstonMutedColor();
        dc.SetTextForeground(notes_color);
        dc.DrawText(notes_text, notes_x, notes_y);
        
        // TODO: Add hit-testing for notes indicator click to open notes editor
    }
    
    return name_area_width;
}

void DrawArrow(wxDC& dc, const wxPoint& from, const wxPoint& to) {
    dc.DrawLine(from, to);
    wxPoint2DDouble direction(to.x - from.x, to.y - from.y);
    double length = std::sqrt(direction.m_x * direction.m_x + direction.m_y * direction.m_y);
    if (length < 0.01) {
        return;
    }
    direction.m_x /= length;
    direction.m_y /= length;
    wxPoint2DDouble left(-direction.m_y, direction.m_x);
    wxPoint2DDouble right(direction.m_y, -direction.m_x);
    double size = 10.0;
    wxPoint p1(to.x - direction.m_x * size + left.m_x * (size * 0.6),
               to.y - direction.m_y * size + left.m_y * (size * 0.6));
    wxPoint p2(to.x - direction.m_x * size + right.m_x * (size * 0.6),
               to.y - direction.m_y * size + right.m_y * (size * 0.6));
    dc.DrawLine(to, p1);
    dc.DrawLine(to, p2);
}

// Draw cardinality marker for Crow's Foot notation
void DrawCrowsFootMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
                          Cardinality card) {
    wxPoint2DDouble perp(-direction.m_y, direction.m_x);
    double offset = 10.0;
    wxPoint2DDouble base(end.x - direction.m_x * offset, end.y - direction.m_y * offset);

    bool has_zero = card == Cardinality::ZeroOrOne || card == Cardinality::ZeroOrMany;
    bool has_many = card == Cardinality::OneOrMany || card == Cardinality::ZeroOrMany;
    bool has_one = card == Cardinality::One || card == Cardinality::OneOrMany || card == Cardinality::ZeroOrOne;

    if (has_zero) {
        wxPoint center(static_cast<int>(base.m_x - direction.m_x * 6.0),
                       static_cast<int>(base.m_y - direction.m_y * 6.0));
        dc.DrawCircle(center, 3);
    }

    if (has_one) {
        wxPoint line_start(static_cast<int>(base.m_x - perp.m_x * 6.0),
                           static_cast<int>(base.m_y - perp.m_y * 6.0));
        wxPoint line_end(static_cast<int>(base.m_x + perp.m_x * 6.0),
                         static_cast<int>(base.m_y + perp.m_y * 6.0));
        dc.DrawLine(line_start, line_end);
    }

    if (has_many) {
        wxPoint fork_base(static_cast<int>(base.m_x), static_cast<int>(base.m_y));
        wxPoint fork_left(static_cast<int>(base.m_x - direction.m_x * 8.0 - perp.m_x * 6.0),
                          static_cast<int>(base.m_y - direction.m_y * 8.0 - perp.m_y * 6.0));
        wxPoint fork_right(static_cast<int>(base.m_x - direction.m_x * 8.0 + perp.m_x * 6.0),
                           static_cast<int>(base.m_y - direction.m_y * 8.0 + perp.m_y * 6.0));
        dc.DrawLine(fork_base, fork_left);
        dc.DrawLine(fork_base, fork_right);
    }
}

// Draw cardinality marker for IDEF1X notation
void DrawIDEF1XMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
                       Cardinality card) {
    wxPoint2DDouble perp(-direction.m_y, direction.m_x);
    
    bool has_many = card == Cardinality::OneOrMany || card == Cardinality::ZeroOrMany;
    
    // IDEF1X uses 'P' for optional many, 'Z' for mandatory many
    wxString symbol;
    if (has_many) {
        symbol = (card == Cardinality::ZeroOrMany) ? "P" : "Z";
    } else if (card == Cardinality::ZeroOrOne) {
        symbol = "O";
    } else {
        symbol = "|";
    }
    
    wxPoint text_pos(static_cast<int>(end.x - direction.m_x * 15),
                     static_cast<int>(end.y - direction.m_y * 15));
    dc.DrawText(symbol, text_pos.x - 4, text_pos.y - 6);
}

// Draw cardinality marker for UML notation
void DrawUMLMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
                    Cardinality card) {
    wxString symbol;
    switch (card) {
        case Cardinality::One:
            symbol = "1";
            break;
        case Cardinality::ZeroOrOne:
            symbol = "0..1";
            break;
        case Cardinality::OneOrMany:
            symbol = "1..*";
            break;
        case Cardinality::ZeroOrMany:
            symbol = "0..*";
            break;
    }
    
    wxPoint text_pos(static_cast<int>(end.x - direction.m_x * 20),
                     static_cast<int>(end.y - direction.m_y * 20));
    dc.DrawText(symbol, text_pos.x - 8, text_pos.y - 6);
}

// Draw cardinality marker for Chen notation
void DrawChenMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
                     Cardinality card) {
    // Chen notation uses '1' or 'M' at the relationship diamond
    wxString symbol = (card == Cardinality::One || card == Cardinality::ZeroOrOne) ? "1" : "M";
    
    wxPoint text_pos(static_cast<int>(end.x - direction.m_x * 25),
                     static_cast<int>(end.y - direction.m_y * 25));
    dc.DrawText(symbol, text_pos.x - 4, text_pos.y - 6);
}

// Main function that dispatches to notation-specific renderers
void DrawCardinalityMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
                           Cardinality card, ErdNotation notation) {
    switch (notation) {
        case ErdNotation::IDEF1X:
            DrawIDEF1XMarker(dc, end, direction, card);
            break;
        case ErdNotation::UML:
            DrawUMLMarker(dc, end, direction, card);
            break;
        case ErdNotation::Chen:
            DrawChenMarker(dc, end, direction, card);
            break;
        case ErdNotation::CrowsFoot:
        default:
            DrawCrowsFootMarker(dc, end, direction, card);
            break;
    }
}

std::vector<wxPoint> BuildOrthogonalPath(const wxPoint& source, const wxPoint& target) {
    std::vector<wxPoint> points;
    points.push_back(source);
    int dx = target.x - source.x;
    int dy = target.y - source.y;

    if (std::abs(dx) < 4 || std::abs(dy) < 4) {
        points.push_back(target);
        return points;
    }

    if (std::abs(dx) >= std::abs(dy)) {
        wxPoint mid(target.x, source.y);
        if (mid != source && mid != target) {
            points.push_back(mid);
        }
    } else {
        wxPoint mid(source.x, target.y);
        if (mid != source && mid != target) {
            points.push_back(mid);
        }
    }
    points.push_back(target);
    return points;
}

struct LabelAnchor {
    wxPoint point;
    wxPoint2DDouble direction;
};

LabelAnchor ComputeLabelAnchor(const std::vector<wxPoint>& points) {
    if (points.size() < 2) {
        return {wxPoint(0, 0), wxPoint2DDouble(1.0, 0.0)};
    }
    double total = 0.0;
    std::vector<double> lengths;
    lengths.reserve(points.size() - 1);
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        double dx = points[i + 1].x - points[i].x;
        double dy = points[i + 1].y - points[i].y;
        double len = std::sqrt(dx * dx + dy * dy);
        lengths.push_back(len);
        total += len;
    }
    double target = total * 0.5;
    double accum = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        double len = lengths[i];
        if (len <= 0.01) {
            continue;
        }
        if (accum + len >= target) {
            double t = (target - accum) / len;
            double x = points[i].x + (points[i + 1].x - points[i].x) * t;
            double y = points[i].y + (points[i + 1].y - points[i].y) * t;
            wxPoint2DDouble direction(points[i + 1].x - points[i].x, points[i + 1].y - points[i].y);
            double norm = std::sqrt(direction.m_x * direction.m_x + direction.m_y * direction.m_y);
            if (norm > 0.01) {
                direction.m_x /= norm;
                direction.m_y /= norm;
            } else {
                direction.m_x = 1.0;
                direction.m_y = 0.0;
            }
            return {wxPoint(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y))), direction};
        }
        accum += len;
    }
    wxPoint2DDouble direction(points.back().x - points.front().x, points.back().y - points.front().y);
    double norm = std::sqrt(direction.m_x * direction.m_x + direction.m_y * direction.m_y);
    if (norm > 0.01) {
        direction.m_x /= norm;
        direction.m_y /= norm;
    } else {
        direction.m_x = 1.0;
        direction.m_y = 0.0;
    }
    return {points.front(), direction};
}

double DistancePointToSegment(const wxPoint2DDouble& point, const wxPoint& a, const wxPoint& b) {
    wxPoint2DDouble ap(point.m_x - a.x, point.m_y - a.y);
    wxPoint2DDouble ab(b.x - a.x, b.y - a.y);
    double ab2 = ab.m_x * ab.m_x + ab.m_y * ab.m_y;
    if (ab2 <= 0.01) {
        return std::sqrt(ap.m_x * ap.m_x + ap.m_y * ap.m_y);
    }
    double t = (ap.m_x * ab.m_x + ap.m_y * ab.m_y) / ab2;
    t = std::clamp(t, 0.0, 1.0);
    double closest_x = a.x + ab.m_x * t;
    double closest_y = a.y + ab.m_y * t;
    double dx = point.m_x - closest_x;
    double dy = point.m_y - closest_y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

DiagramCanvas::DiagramCanvas(wxWindow* parent, DiagramType type)
    : wxPanel(parent, wxID_ANY),
      model_(type) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(CanvasColor());

    Bind(wxEVT_PAINT, &DiagramCanvas::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &DiagramCanvas::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &DiagramCanvas::OnLeftUp, this);
    Bind(wxEVT_LEFT_DCLICK, &DiagramCanvas::OnLeftDClick, this);
    Bind(wxEVT_RIGHT_DOWN, &DiagramCanvas::OnRightDown, this);
    Bind(wxEVT_RIGHT_UP, &DiagramCanvas::OnRightUp, this);
    Bind(wxEVT_MOTION, &DiagramCanvas::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &DiagramCanvas::OnMouseWheel, this);
    Bind(wxEVT_SIZE, &DiagramCanvas::OnSize, this);
    Bind(wxEVT_KEY_DOWN, &DiagramCanvas::OnKeyDown, this);
    Bind(wxEVT_CONTEXT_MENU, &DiagramCanvas::OnContextMenu, this);
    
    // Context menu event bindings
    Bind(wxEVT_MENU, &DiagramCanvas::OnDeleteFromDiagram, this, ID_DELETE_FROM_DIAGRAM);
    Bind(wxEVT_MENU, &DiagramCanvas::OnDeleteFromProject, this, ID_DELETE_FROM_PROJECT);
}

DiagramType DiagramCanvas::diagram_type() const {
    return model_.type();
}

void DiagramCanvas::SetDiagramType(DiagramType type) {
    model_.set_type(type);
    model_.nodes().clear();
    model_.edges().clear();
    selected_index_.reset();
    selected_edge_index_.reset();
    dragging_index_.reset();
    Refresh();
}

double DiagramCanvas::zoom() const {
    return zoom_;
}

wxPoint2DDouble DiagramCanvas::pan_offset() const {
    return pan_offset_;
}

void DiagramCanvas::SetView(double zoom, const wxPoint2DDouble& pan) {
    zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom);
    pan_offset_ = pan;
    Refresh();
}

ErdNotation DiagramCanvas::notation() const {
    return model_.notation();
}

void DiagramCanvas::SetNotation(ErdNotation notation) {
    model_.set_notation(notation);
    Refresh();
}

void DiagramCanvas::ApplyLayout(diagram::LayoutAlgorithm algorithm) {
    diagram::LayoutOptions options;
    options.algorithm = algorithm;
    ApplyLayout(options);
}

void DiagramCanvas::ApplyLayout(const diagram::LayoutOptions& options) {
    auto engine = diagram::LayoutEngine::Create(options.algorithm);
    auto positions = engine->Layout(model_, options);
    
    // Apply positions using commands for undo support
    for (const auto& pos : positions) {
        auto& nodes = model_.nodes();
        auto it = std::find_if(nodes.begin(), nodes.end(),
                               [&pos](const DiagramNode& n) { return n.id == pos.node_id; });
        if (it != nodes.end()) {
            command_manager_.Execute(std::make_unique<MoveNodeCommand>(
                &model_, pos.node_id, it->x, it->y, pos.x, pos.y));
        }
    }
    
    Refresh();
}

void DiagramCanvas::SetTemplateKey(const std::string& key) {
    template_key_ = key;
    Refresh();
}

const std::string& DiagramCanvas::template_key() const {
    return template_key_;
}

void DiagramCanvas::SetGridSize(int size) {
    if (size > 0) {
        grid_size_ = size;
        Refresh();
    }
}

int DiagramCanvas::grid_size() const {
    return grid_size_;
}

void DiagramCanvas::SetIconSet(const std::string& key) {
    icon_set_ = key;
    Refresh();
}

const std::string& DiagramCanvas::icon_set() const {
    return icon_set_;
}

void DiagramCanvas::SetBorderWidth(int width) {
    border_width_ = std::max(1, width);
    Refresh();
}

int DiagramCanvas::border_width() const {
    return border_width_;
}

void DiagramCanvas::SetBorderDashed(bool value) {
    border_dashed_ = value;
    Refresh();
}

void DiagramCanvas::SetDataViews(const std::vector<DataViewPanel>& panels) {
    data_views_ = panels;
    Refresh();
}

const DiagramCanvas::DataViewPanel* DiagramCanvas::HitTestDataView(
    const wxPoint2DDouble& world_point) const {
    for (const auto& panel : data_views_) {
        if (world_point.m_x >= panel.rect.m_x &&
            world_point.m_x <= panel.rect.m_x + panel.rect.m_width &&
            world_point.m_y >= panel.rect.m_y &&
            world_point.m_y <= panel.rect.m_y + panel.rect.m_height) {
            return &panel;
        }
    }
    return nullptr;
}

bool DiagramCanvas::HandleDataViewKey(int key_code) {
    if (focused_data_view_.empty()) {
        return false;
    }
    for (auto& panel : data_views_) {
        if (panel.id != focused_data_view_) continue;
        int col_count = static_cast<int>(panel.columns.size());
        int row_count = static_cast<int>(panel.rows.size());
        if (col_count <= 0 || row_count <= 0) {
            return false;
        }
        if (panel.selected_row < 0) panel.selected_row = 0;
        if (panel.selected_col < 0) panel.selected_col = 0;
        int visible_rows = std::max(1, static_cast<int>(
            (panel.rect.m_height - kDataViewHeaderHeight - 6) / kDataViewRowHeight));

        switch (key_code) {
            case WXK_UP:
                panel.selected_row = std::max(0, panel.selected_row - 1);
                break;
            case WXK_DOWN:
                panel.selected_row = std::min(row_count - 1, panel.selected_row + 1);
                break;
            case WXK_LEFT:
                panel.selected_col = std::max(0, panel.selected_col - 1);
                break;
            case WXK_RIGHT:
                panel.selected_col = std::min(col_count - 1, panel.selected_col + 1);
                break;
            case WXK_PAGEUP:
                panel.selected_row = std::max(0, panel.selected_row - visible_rows);
                break;
            case WXK_PAGEDOWN:
                panel.selected_row = std::min(row_count - 1, panel.selected_row + visible_rows);
                break;
            case WXK_HOME:
                panel.selected_col = 0;
                break;
            case WXK_END:
                panel.selected_col = col_count - 1;
                break;
            default:
                return false;
        }

        if (panel.selected_row < panel.scroll_row) {
            panel.scroll_row = panel.selected_row;
        } else if (panel.selected_row >= panel.scroll_row + visible_rows) {
            panel.scroll_row = panel.selected_row - visible_rows + 1;
        }
        int max_scroll = std::max(0, row_count - visible_rows);
        panel.scroll_row = std::clamp(panel.scroll_row, 0, max_scroll);

        wxCommandEvent evt(EVT_DATA_VIEW_UPDATED);
        evt.SetString(panel.id + "|" +
                      std::to_string(panel.scroll_row) + "|" +
                      std::to_string(panel.selected_row) + "|" +
                      std::to_string(panel.selected_col));
        wxPostEvent(this, evt);
        Refresh();
        return true;
    }
    return false;
}

bool DiagramCanvas::border_dashed() const {
    return border_dashed_;
}

DiagramModel& DiagramCanvas::model() {
    return model_;
}

const DiagramModel& DiagramCanvas::model() const {
    return model_;
}

const DiagramNode* DiagramCanvas::GetSelectedNode() const {
    if (!selected_index_.has_value()) {
        return nullptr;
    }
    const auto& nodes = model_.nodes();
    if (*selected_index_ >= nodes.size()) {
        return nullptr;
    }
    return &nodes[*selected_index_];
}

DiagramNode* DiagramCanvas::GetSelectedNodeMutable() {
    if (!selected_index_.has_value()) {
        return nullptr;
    }
    auto& nodes = model_.nodes();
    if (*selected_index_ >= nodes.size()) {
        return nullptr;
    }
    return &nodes[*selected_index_];
}

const DiagramEdge* DiagramCanvas::GetSelectedEdge() const {
    if (!selected_edge_index_.has_value()) {
        return nullptr;
    }
    const auto& edges = model_.edges();
    if (*selected_edge_index_ >= edges.size()) {
        return nullptr;
    }
    return &edges[*selected_edge_index_];
}

DiagramEdge* DiagramCanvas::GetSelectedEdgeMutable() {
    if (!selected_edge_index_.has_value()) {
        return nullptr;
    }
    auto& edges = model_.edges();
    if (*selected_edge_index_ >= edges.size()) {
        return nullptr;
    }
    return &edges[*selected_edge_index_];
}

void DiagramCanvas::AddNode(const std::string& node_type, const std::string& name) {
    DiagramNode node;
    node.id = DiagramTypeKey(model_.type()) + "_node_" + std::to_string(model_.NextNodeIndex());
    node.type = node_type;
    node.name = name;

    if (model_.type() == DiagramType::Erd) {
        node.width = 220.0;
        node.height = 160.0;
        node.attributes = {
            {"id", "UUID", true, false},
            {"name", "VARCHAR(200)", false, false},
            {"created_at", "TIMESTAMP", false, false}
        };
    } else if (model_.type() == DiagramType::MindMap) {
        node.width = 200.0;
        node.height = 90.0;
    } else if (model_.type() == DiagramType::Whiteboard) {
        node.width = 220.0;
        node.height = 140.0;
        if (node_type == "Table") {
            node.attributes = {
                {"Surrogate_Key", "", true, false, false},
                {"Name", "", false, false, true}
            };
        }
    } else if (model_.type() == DiagramType::DataFlow) {
        node.width = 200.0;
        node.height = 120.0;
    } else {
        node.width = 180.0;
        node.height = 140.0;
        if (node_type == "Cluster" || node_type == "Database") {
            node.stack_count = 2;
        }
        if (node_type == "Network") {
            node.ghosted = true;
        }
    }

    wxPoint2DDouble position = NextInsertPosition(node.width, node.height);
    node.x = position.m_x;
    node.y = position.m_y;

    // Use command pattern for undo/redo support
    auto command = std::make_unique<AddNodeCommand>(&model_, node);
    command_manager_.Execute(std::move(command));
    Refresh();
}

void DiagramCanvas::AddEdge(const std::string& source_id, const std::string& target_id, const std::string& label) {
    DiagramEdge edge;
    edge.id = DiagramTypeKey(model_.type()) + "_edge_" + std::to_string(model_.NextEdgeIndex());
    edge.source_id = source_id;
    edge.target_id = target_id;
    edge.label = label;
    if (model_.type() == DiagramType::Silverston) {
        edge.edge_type = "dependency";
    } else if (model_.type() == DiagramType::DataFlow) {
        edge.edge_type = "data_flow";
    } else if (model_.type() == DiagramType::MindMap || model_.type() == DiagramType::Whiteboard) {
        edge.edge_type = "link";
    } else {
        edge.edge_type = "relationship";
    }
    edge.directed = (model_.type() == DiagramType::Silverston ||
                     model_.type() == DiagramType::DataFlow ||
                     model_.type() == DiagramType::MindMap);
    edge.identifying = false;
    
    // Use command pattern for undo/redo support
    auto command = std::make_unique<AddEdgeCommand>(&model_, edge);
    command_manager_.Execute(std::move(command));
    Refresh();
}

void DiagramCanvas::SelectNextNode() {
    if (model_.nodes().empty()) {
        return;
    }
    size_t next = 0;
    if (selected_index_.has_value()) {
        next = (*selected_index_ + 1) % model_.nodes().size();
    }
    UpdateSelection(next, std::nullopt);
}

void DiagramCanvas::SelectPreviousNode() {
    if (model_.nodes().empty()) {
        return;
    }
    size_t prev = model_.nodes().size() - 1;
    if (selected_index_.has_value()) {
        prev = (*selected_index_ + model_.nodes().size() - 1) % model_.nodes().size();
    }
    UpdateSelection(prev, std::nullopt);
}

bool DiagramCanvas::SelectNodeByName(const std::string& name) {
    if (name.empty()) return false;
    const auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].name == name || nodes[i].id == name) {
            UpdateSelection(i, std::nullopt);
            CenterOnNode(nodes[i]);
            Refresh();
            return true;
        }
    }
    return false;
}

void DiagramCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    dc.SetBackground(wxBrush(CanvasColor()));
    dc.Clear();

    dc.SetUserScale(zoom_, zoom_);
    dc.SetDeviceOrigin(static_cast<int>(std::lround(pan_offset_.m_x * zoom_)),
                       static_cast<int>(std::lround(pan_offset_.m_y * zoom_)));

    DrawGrid(dc);
    DrawEdges(dc);
    DrawDataViews(dc);
    DrawNodes(dc);
    
    // Draw drag overlay (drop target highlighting, ghost nodes)
    DrawDragOverlay(dc);
}

void DiagramCanvas::DrawDataViews(wxDC& dc) {
    if (data_views_.empty()) {
        return;
    }
    wxPen border(wxColour(120, 120, 130), 1);
    wxPen warning_border(wxColour(210, 130, 40), 2);
    wxBrush background(wxColour(45, 45, 52));
    wxBrush warning_bg(wxColour(82, 58, 32));
    wxBrush header_bg(wxColour(64, 64, 72));
    wxBrush header_warning_bg(wxColour(110, 70, 30));
    wxPen header_pen(wxColour(90, 90, 100), 1);
    wxPen header_warning_pen(wxColour(160, 100, 30), 1);
    wxBrush column_bg(wxColour(58, 58, 68));
    wxPen grid_pen(wxColour(85, 85, 95), 1);

    for (const auto& panel : data_views_) {
        wxRect draw_rect(static_cast<int>(panel.rect.m_x),
                         static_cast<int>(panel.rect.m_y),
                         static_cast<int>(panel.rect.m_width),
                         static_cast<int>(panel.rect.m_height));
        dc.SetPen(panel.stale ? warning_border : border);
        dc.SetBrush(panel.stale ? warning_bg : background);
        dc.DrawRectangle(draw_rect);

        wxRect header_rect(draw_rect.GetX(), draw_rect.GetY(), draw_rect.GetWidth(), kDataViewHeaderHeight);
        dc.SetPen(panel.stale ? header_warning_pen : header_pen);
        dc.SetBrush(panel.stale ? header_warning_bg : header_bg);
        dc.DrawRectangle(header_rect);
        dc.SetTextForeground(wxColour(230, 230, 235));
        dc.DrawText(panel.name.empty() ? "Data View" : panel.name,
                    header_rect.GetX() + 6, header_rect.GetY() + 3);

        if (panel.stale) {
            wxString badge = "STALE";
            wxSize badge_size = dc.GetTextExtent(badge);
            int badge_w = badge_size.x + 10;
            int badge_h = badge_size.y + 4;
            int badge_x = header_rect.GetRight() - badge_w - 6;
            int badge_y = header_rect.GetY() + 2;
            wxRect badge_rect(badge_x, badge_y, badge_w, badge_h);
            dc.SetBrush(wxBrush(wxColour(230, 170, 60)));
            dc.SetPen(wxPen(wxColour(140, 90, 20), 1));
            dc.DrawRoundedRectangle(badge_rect, 3);
            dc.SetTextForeground(wxColour(40, 30, 20));
            dc.DrawText(badge, badge_rect.GetX() + 5, badge_rect.GetY() + 1);
        }

        wxRect grid_rect(draw_rect.GetX() + 4,
                         header_rect.GetBottom() + 2,
                         draw_rect.GetWidth() - 8,
                         draw_rect.GetHeight() - header_rect.GetHeight() - 6);
        if (grid_rect.GetWidth() <= 20 || grid_rect.GetHeight() <= 20) {
            continue;
        }

        const int row_height = kDataViewRowHeight;
        int col_count = static_cast<int>(panel.columns.size());
        if (col_count <= 0) {
            dc.SetTextForeground(wxColour(200, 200, 200));
            dc.DrawText("No columns", grid_rect.GetX() + 4, grid_rect.GetY() + 4);
            continue;
        }
        int max_rows = (grid_rect.GetHeight() - row_height) / row_height;
        bool needs_scroll = static_cast<int>(panel.rows.size()) > max_rows;
        wxRect content_rect = grid_rect;
        if (needs_scroll) {
            content_rect.SetWidth(grid_rect.GetWidth() - 10);
        }
        int col_width = std::max(60, content_rect.GetWidth() / col_count);
        wxRect header_row(content_rect.GetX(), content_rect.GetY(), content_rect.GetWidth(), row_height);
        dc.SetBrush(column_bg);
        dc.SetPen(grid_pen);
        dc.DrawRectangle(header_row);

        dc.SetTextForeground(wxColour(230, 230, 235));
        for (int c = 0; c < col_count; ++c) {
            int cx = content_rect.GetX() + c * col_width;
            wxRect cell(cx, content_rect.GetY(), col_width, row_height);
            if (panel.id == hover_data_view_ && c == hover_col_) {
                dc.SetBrush(wxBrush(wxColour(70, 70, 90)));
                dc.SetPen(grid_pen);
                dc.DrawRectangle(cell);
            }
            dc.DrawLine(cell.GetRight(), cell.GetTop(), cell.GetRight(), cell.GetBottom());
            std::string label = panel.columns[c];
            if (c < static_cast<int>(panel.column_types.size()) &&
                !panel.column_types[c].empty()) {
                label += " : " + panel.column_types[c];
            }
            dc.DrawText(label, cell.GetX() + 4, cell.GetY() + 2);
        }

        int available_rows = static_cast<int>(panel.rows.size()) - panel.scroll_row;
        if (available_rows < 0) available_rows = 0;
        int row_count = std::min(available_rows, max_rows);
        wxRect clip_rect = content_rect;
        clip_rect.SetY(content_rect.GetY() + row_height);
        clip_rect.SetHeight(content_rect.GetHeight() - row_height);
        dc.DestroyClippingRegion();
        dc.SetClippingRegion(clip_rect);
        dc.SetTextForeground(wxColour(210, 210, 210));
        for (int r = 0; r < row_count; ++r) {
            int data_row = panel.scroll_row + r;
            int y = content_rect.GetY() + row_height + (r * row_height);
            for (int c = 0; c < col_count; ++c) {
                int x = content_rect.GetX() + c * col_width;
                wxRect cell(x, y, col_width, row_height);
                bool hover = (panel.id == hover_data_view_ &&
                              data_row == hover_row_ &&
                              c == hover_col_);
                bool selected = (data_row == panel.selected_row && c == panel.selected_col);
                if (hover || selected) {
                    dc.SetBrush(wxBrush(selected ? wxColour(70, 90, 140) : wxColour(70, 70, 90)));
                    dc.SetPen(grid_pen);
                    dc.DrawRectangle(cell);
                } else {
                    dc.SetPen(grid_pen);
                    dc.DrawRectangle(cell);
                }
                std::string value;
                if (data_row < static_cast<int>(panel.rows.size()) &&
                    c < static_cast<int>(panel.rows[data_row].size())) {
                    value = panel.rows[data_row][c];
                }
                if (!value.empty()) {
                    dc.DrawText(value, cell.GetX() + 4, cell.GetY() + 2);
                }
            }
        }
        dc.DestroyClippingRegion();

        if (needs_scroll) {
            int scroll_area_h = content_rect.GetHeight() - row_height;
            wxRect scroll_rect(content_rect.GetRight() + 2,
                               content_rect.GetY() + row_height,
                               6,
                               scroll_area_h);
            dc.SetPen(wxPen(wxColour(90, 90, 100), 1));
            dc.SetBrush(wxBrush(wxColour(60, 60, 70)));
            dc.DrawRectangle(scroll_rect);
            int max_scroll = std::max(1, static_cast<int>(panel.rows.size()) - max_rows);
            double ratio = static_cast<double>(max_rows) / panel.rows.size();
            int thumb_h = std::max(12, static_cast<int>(scroll_area_h * ratio));
            int thumb_y = scroll_rect.GetY() +
                          static_cast<int>((scroll_area_h - thumb_h) *
                                           (static_cast<double>(panel.scroll_row) / max_scroll));
            wxRect thumb(scroll_rect.GetX() + 1, thumb_y, scroll_rect.GetWidth() - 2, thumb_h);
            dc.SetBrush(wxBrush(wxColour(120, 120, 140)));
            dc.SetPen(wxPen(wxColour(70, 70, 85), 1));
            dc.DrawRectangle(thumb);
        }
    }
}

void DiagramCanvas::OnLeftDown(wxMouseEvent& event) {
    SetFocus();
    wxPoint2DDouble world_point_d = ScreenToWorldDouble(event.GetPosition());
    wxPoint world_point = ScreenToWorld(event.GetPosition());
    bool capture = false;
    
    // Track multi-select modifier
    multi_select_mode_ = event.ControlDown() || event.CmdDown();

    if (auto* panel = const_cast<DataViewPanel*>(HitTestDataView(world_point_d))) {
        wxRect header_rect(static_cast<int>(panel->rect.m_x),
                           static_cast<int>(panel->rect.m_y),
                           static_cast<int>(panel->rect.m_width),
                           kDataViewHeaderHeight);
        if (world_point.y <= header_rect.GetBottom()) {
            wxCommandEvent evt(wxEVT_BUTTON, wxID_REFRESH);
            evt.SetString(panel->id);
            wxPostEvent(this, evt);
            return;
        }
        wxRect grid_rect(header_rect.GetX() + 4,
                         header_rect.GetBottom() + 2,
                         header_rect.GetWidth() - 8,
                         static_cast<int>(panel->rect.m_height) - kDataViewHeaderHeight - 6);
        if (grid_rect.Contains(world_point)) {
            int col_count = static_cast<int>(panel->columns.size());
            int max_rows = (grid_rect.GetHeight() - kDataViewRowHeight) / kDataViewRowHeight;
            bool needs_scroll = static_cast<int>(panel->rows.size()) > max_rows;
            wxRect content_rect = grid_rect;
            if (needs_scroll) {
                content_rect.SetWidth(grid_rect.GetWidth() - 10);
            }
            if (col_count > 0) {
                int col_width = std::max(60, content_rect.GetWidth() / col_count);
                int row_index = (world_point.y - content_rect.GetY() - kDataViewRowHeight) /
                                kDataViewRowHeight;
                if (row_index >= 0) {
                    int col = (world_point.x - content_rect.GetX()) / col_width;
                    int row = panel->scroll_row + row_index;
                    panel->selected_row = row;
                    panel->selected_col = std::clamp(col, 0, col_count - 1);
                    focused_data_view_ = panel->id;
                    wxCommandEvent evt(EVT_DATA_VIEW_UPDATED);
                    evt.SetString(panel->id + "|" +
                                  std::to_string(panel->scroll_row) + "|" +
                                  std::to_string(panel->selected_row) + "|" +
                                  std::to_string(panel->selected_col));
                    wxPostEvent(this, evt);
                    Refresh();
                }
            }
            if (needs_scroll) {
                int scroll_area_h = content_rect.GetHeight() - kDataViewRowHeight;
                wxRect scroll_rect(content_rect.GetRight() + 2,
                                   content_rect.GetY() + kDataViewRowHeight,
                                   6,
                                   scroll_area_h);
                if (scroll_rect.Contains(world_point)) {
                    int max_rows_total = static_cast<int>(panel->rows.size());
                    int max_scroll = std::max(0, max_rows_total - max_rows);
                    double ratio = static_cast<double>(world_point.y - scroll_rect.GetY()) /
                                   std::max(1, scroll_rect.GetHeight());
                    panel->scroll_row = std::clamp(
                        static_cast<int>(ratio * max_scroll), 0, max_scroll);
                    focused_data_view_ = panel->id;
                    wxCommandEvent evt(EVT_DATA_VIEW_UPDATED);
                    evt.SetString(panel->id + "|" +
                                  std::to_string(panel->scroll_row) + "|" +
                                  std::to_string(panel->selected_row) + "|" +
                                  std::to_string(panel->selected_col));
                    wxPostEvent(this, evt);
                    Refresh();
                }
            }
            return;
        }
    }

    if (selected_index_.has_value()) {
        const auto& node = model_.nodes()[*selected_index_];
        ResizeHandle handle = HitTestResizeHandle(node, world_point_d);
        if (handle != ResizeHandle::None) {
            resizing_index_ = selected_index_;
            resize_handle_ = handle;
            resize_start_rect_ = WorldRectForNode(node);
            resize_start_point_ = world_point_d;
            SetCursor(CursorForHandle(handle));
            capture = true;
        }
    }

    if (!capture && selected_edge_index_.has_value()) {
        EdgeDragEndpoint endpoint = HitTestEdgeEndpoint(*selected_edge_index_, world_point_d);
        if (endpoint != EdgeDragEndpoint::None) {
            dragging_edge_index_ = selected_edge_index_;
            edge_drag_endpoint_ = endpoint;
            edge_drag_point_ = world_point;
            capture = true;
        }
    }

    if (!capture) {
        auto node_hit = HitTestNode(world_point_d);
        if (node_hit.has_value()) {
            auto& node = model_.nodes()[*node_hit];
            if (model_.type() == DiagramType::MindMap && HasChildren(node)) {
                wxRect chevron = ChevronRectForNode(node);
                if (chevron.Contains(world_point)) {
                    node.collapsed = !node.collapsed;
                    Refresh();
                    return;
                }
            }
            
            // Handle multi-selection
            if (multi_select_mode_) {
                ToggleNodeSelection(*node_hit);
                if (IsNodeSelected(*node_hit)) {
                    dragging_index_ = node_hit;
                    drag_offset_.m_x = world_point_d.m_x - node.x;
                    drag_offset_.m_y = world_point_d.m_y - node.y;
                    drag_start_pos_ = wxPoint2DDouble(node.x, node.y);
                    capture = true;
                }
            } else {
                // Single selection mode
                if (!IsNodeSelected(*node_hit)) {
                    selected_indices_.clear();
                    selected_indices_.insert(*node_hit);
                    selected_index_ = node_hit;
                    selected_edge_index_.reset();
                }
                dragging_index_ = node_hit;
                drag_offset_.m_x = world_point_d.m_x - node.x;
                drag_offset_.m_y = world_point_d.m_y - node.y;
                drag_start_pos_ = wxPoint2DDouble(node.x, node.y);  // Record start for undo
                capture = true;
                Refresh();
                
                // Notify selection change
                wxCommandEvent event(EVT_DIAGRAM_SELECTION_CHANGED);
                event.SetString("node");
                event.SetInt(static_cast<int>(*node_hit));
                wxPostEvent(GetParent(), event);
            }
        } else {
            auto edge_hit = HitTestEdge(world_point_d);
            if (!multi_select_mode_) {
                selected_indices_.clear();
                selected_index_.reset();
            }
            selected_edge_index_ = edge_hit;
            dragging_index_.reset();
            if (edge_hit.has_value()) {
                EdgeDragEndpoint endpoint = HitTestEdgeEndpoint(*edge_hit, world_point_d);
                if (endpoint != EdgeDragEndpoint::None) {
                    dragging_edge_index_ = edge_hit;
                    edge_drag_endpoint_ = endpoint;
                    edge_drag_point_ = world_point;
                    capture = true;
                }
            } else if (!multi_select_mode_) {
                // Click on empty space - clear selection
                ClearSelection();
            }
            Refresh();
        }
    }

    if (capture) {
        CaptureMouse();
    }
}

void DiagramCanvas::OnLeftUp(wxMouseEvent& event) {
    if (HasCapture()) {
        ReleaseMouse();
    }
    
    // Handle drag and drop operations
    if (dragging_index_.has_value()) {
        auto& node = model_.nodes()[*dragging_index_];
        
        if (current_drag_operation_ == DragOperation::Reparent && drag_target_index_.has_value()) {
            // Execute reparenting operation
            ExecuteReparent(*dragging_index_, *drag_target_index_, 
                           wxPoint2DDouble(node.x, node.y));
        } else if (drag_start_pos_.m_x != node.x || drag_start_pos_.m_y != node.y) {
            // Simple move - only create command if position actually changed
            auto command = std::make_unique<MoveNodeCommand>(
                &model_, node.id, 
                drag_start_pos_.m_x, drag_start_pos_.m_y,
                node.x, node.y);
            command_manager_.Execute(std::move(command));
        }
        
        // Reset drag state
        ClearDragTarget();
        current_drag_operation_ = DragOperation::None;
    }
    
    // Handle external drag drop (from tree)
    if (external_drag_active_) {
        wxPoint2DDouble world_point = ScreenToWorldDouble(event.GetPosition());
        ExecuteMultiExternalDrop(world_point);
        CancelExternalDrag();
    }
    
    dragging_index_.reset();
    resizing_index_.reset();
    resize_handle_ = ResizeHandle::None;
    if (dragging_edge_index_.has_value()) {
        wxPoint2DDouble world_point_d = ScreenToWorldDouble(ScreenToClient(wxGetMousePosition()));
        auto node_hit = HitTestNode(world_point_d);
        if (node_hit.has_value()) {
            auto& edge = model_.edges()[*dragging_edge_index_];
            const auto& node = model_.nodes()[*node_hit];
            if (edge_drag_endpoint_ == EdgeDragEndpoint::Source && node.id != edge.target_id) {
                edge.source_id = node.id;
            } else if (edge_drag_endpoint_ == EdgeDragEndpoint::Target && node.id != edge.source_id) {
                edge.target_id = node.id;
            }
        }
        dragging_edge_index_.reset();
        edge_drag_endpoint_ = EdgeDragEndpoint::None;
        Refresh();
    }
}

void DiagramCanvas::OnLeftDClick(wxMouseEvent& event) {
    if (model_.type() != DiagramType::MindMap) {
        wxPoint2DDouble world_point_d = ScreenToWorldDouble(event.GetPosition());
        if (const auto* panel = HitTestDataView(world_point_d)) {
            wxCommandEvent evt(EVT_DATA_VIEW_OPEN);
            evt.SetString(panel->id);
            wxPostEvent(this, evt);
            return;
        }
        return;
    }
    wxPoint2DDouble world_point_d = ScreenToWorldDouble(event.GetPosition());
    auto node_hit = HitTestNode(world_point_d);
    if (node_hit.has_value()) {
        auto& node = model_.nodes()[*node_hit];
        node.collapsed = !node.collapsed;
        Refresh();
    }
}

void DiagramCanvas::OnRightDown(wxMouseEvent& event) {
    is_panning_ = true;
    last_mouse_ = event.GetPosition();
    CaptureMouse();
}

void DiagramCanvas::OnRightUp(wxMouseEvent&) {
    if (HasCapture()) {
        ReleaseMouse();
    }
    is_panning_ = false;
}

void DiagramCanvas::OnMotion(wxMouseEvent& event) {
    if (resizing_index_.has_value() && event.Dragging() && event.LeftIsDown()) {
        ApplyResize(ScreenToWorldDouble(event.GetPosition()));
        return;
    }

    if (dragging_edge_index_.has_value() && event.Dragging() && event.LeftIsDown()) {
        edge_drag_point_ = ScreenToWorld(event.GetPosition());
        Refresh();
        return;
    }

    if (dragging_index_.has_value() && event.Dragging() && event.LeftIsDown()) {
        wxPoint2DDouble world_point = ScreenToWorldDouble(event.GetPosition());
        auto& node = model_.nodes()[*dragging_index_];
        
        // Check if we're in reparenting mode (Ctrl key pressed during drag)
        if (event.ControlDown()) {
            current_drag_operation_ = DragOperation::Reparent;
            // Update position with snap-to-grid
            wxPoint2DDouble new_pos = SnapToGrid(world_point - drag_offset_);
            node.x = new_pos.m_x;
            node.y = new_pos.m_y;
            drag_current_pos_ = world_point;
            // Update drag target for visual feedback
            UpdateDragTarget(world_point);
        } else {
            current_drag_operation_ = DragOperation::Move;
            ClearDragTarget();
            node.x = world_point.m_x - drag_offset_.m_x;
            node.y = world_point.m_y - drag_offset_.m_y;
        }
        
        UpdateDragCursor();
        Refresh();
        return;
    }
    
    // Handle external drag (from tree)
    if (external_drag_active_) {
        wxPoint2DDouble world_point = ScreenToWorldDouble(event.GetPosition());
        drag_current_pos_ = world_point;
        UpdateDragTarget(world_point);
        UpdateDragCursor();
        Refresh();
        return;
    }

    if (is_panning_ && event.Dragging()) {
        wxPoint current = event.GetPosition();
        wxPoint delta = current - last_mouse_;
        pan_offset_.m_x += delta.x / zoom_;
        pan_offset_.m_y += delta.y / zoom_;
        last_mouse_ = current;
        Refresh();
        return;
    }

    auto* panel = const_cast<DataViewPanel*>(HitTestDataView(ScreenToWorldDouble(event.GetPosition())));
    if (panel) {
        wxPoint world_point = ScreenToWorld(event.GetPosition());
        wxRect header_rect(static_cast<int>(panel->rect.m_x),
                           static_cast<int>(panel->rect.m_y),
                           static_cast<int>(panel->rect.m_width),
                           kDataViewHeaderHeight);
        if (world_point.y > header_rect.GetBottom()) {
            wxRect grid_rect(header_rect.GetX() + 4,
                             header_rect.GetBottom() + 2,
                             header_rect.GetWidth() - 8,
                             static_cast<int>(panel->rect.m_height) - kDataViewHeaderHeight - 6);
            if (grid_rect.Contains(world_point)) {
                int col_count = static_cast<int>(panel->columns.size());
                if (col_count > 0) {
                    int max_rows = (grid_rect.GetHeight() - kDataViewRowHeight) / kDataViewRowHeight;
                    bool needs_scroll = static_cast<int>(panel->rows.size()) > max_rows;
                    wxRect content_rect = grid_rect;
                    if (needs_scroll) {
                        content_rect.SetWidth(grid_rect.GetWidth() - 10);
                    }
                    int col_width = std::max(60, content_rect.GetWidth() / col_count);
                    int col = (world_point.x - grid_rect.GetX()) / col_width;
                    int row_index = (world_point.y - content_rect.GetY() - kDataViewRowHeight) / kDataViewRowHeight;
                    if (row_index >= 0) {
                        hover_data_view_ = panel->id;
                        hover_row_ = panel->scroll_row + row_index;
                        hover_col_ = std::clamp(col, 0, col_count - 1);
                        Refresh();
                        return;
                    }
                }
            }
        }
    }
    if (!hover_data_view_.empty()) {
        hover_data_view_.clear();
        hover_row_ = -1;
        hover_col_ = -1;
        Refresh();
    }

    UpdateHoverCursor(ScreenToWorldDouble(event.GetPosition()));
}

void DiagramCanvas::OnMouseWheel(wxMouseEvent& event) {
    int rotation = event.GetWheelRotation();
    if (rotation == 0) {
        return;
    }
    wxPoint2DDouble world = ScreenToWorldDouble(event.GetPosition());
    if (auto* panel = const_cast<DataViewPanel*>(HitTestDataView(world))) {
        int col_count = static_cast<int>(panel->columns.size());
        if (col_count > 0) {
            int max_rows = static_cast<int>(panel->rows.size());
            int visible_rows = std::max(1, static_cast<int>(
                (panel->rect.m_height - kDataViewHeaderHeight - 6) / kDataViewRowHeight));
            int max_scroll = std::max(0, max_rows - visible_rows);
            int delta = rotation > 0 ? -1 : 1;
            panel->scroll_row = std::clamp(panel->scroll_row + delta, 0, max_scroll);
            wxCommandEvent evt(EVT_DATA_VIEW_UPDATED);
            evt.SetString(panel->id + "|" +
                          std::to_string(panel->scroll_row) + "|" +
                          std::to_string(panel->selected_row) + "|" +
                          std::to_string(panel->selected_col));
            wxPostEvent(this, evt);
            Refresh();
            return;
        }
    }
    double step = rotation > 0 ? kZoomStep : -kZoomStep;
    double new_zoom = std::clamp(zoom_ + step, kMinZoom, kMaxZoom);
    if (std::abs(new_zoom - zoom_) < 0.001) {
        return;
    }

    wxPoint mouse_pos = event.GetPosition();
    wxPoint world_before = ScreenToWorld(mouse_pos);
    zoom_ = new_zoom;
    wxPoint world_after = ScreenToWorld(mouse_pos);
    pan_offset_.m_x += (world_after.x - world_before.x);
    pan_offset_.m_y += (world_after.y - world_before.y);
    Refresh();
}

void DiagramCanvas::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

wxPoint2DDouble DiagramCanvas::ScreenToWorldDouble(const wxPoint& point) const {
    double x = (point.x / zoom_) - pan_offset_.m_x;
    double y = (point.y / zoom_) - pan_offset_.m_y;
    return wxPoint2DDouble(x, y);
}

wxPoint DiagramCanvas::ScreenToWorld(const wxPoint& point) const {
    wxPoint2DDouble world = ScreenToWorldDouble(point);
    return wxPoint(static_cast<int>(std::lround(world.m_x)),
                   static_cast<int>(std::lround(world.m_y)));
}

wxRect2DDouble DiagramCanvas::WorldRectForNode(const DiagramNode& node) const {
    return wxRect2DDouble(node.x, node.y, node.width, node.height);
}

wxPoint DiagramCanvas::ComputeNodeCenter(const DiagramNode& node) const {
    return wxPoint(static_cast<int>(node.x + node.width / 2.0),
                   static_cast<int>(node.y + node.height / 2.0));
}

wxPoint DiagramCanvas::ComputeEdgeAnchor(const DiagramNode& node, const wxPoint& toward) const {
    wxPoint center = ComputeNodeCenter(node);
    double dx = toward.x - center.x;
    double dy = toward.y - center.y;
    if (std::abs(dx) < 0.001 && std::abs(dy) < 0.001) {
        return center;
    }
    double half_w = node.width / 2.0;
    double half_h = node.height / 2.0;
    double abs_dx = std::abs(dx);
    double abs_dy = std::abs(dy);
    double scale = 1.0;
    if (abs_dx * half_h > abs_dy * half_w) {
        scale = half_w / abs_dx;
    } else {
        scale = half_h / abs_dy;
    }
    return wxPoint(static_cast<int>(center.x + dx * scale),
                   static_cast<int>(center.y + dy * scale));
}

void DiagramCanvas::DrawGrid(wxDC& dc) {
    if (!show_grid_) {
        return;
    }
    wxSize size = GetClientSize();
    double width = size.x / zoom_;
    double height = size.y / zoom_;
    double left = -pan_offset_.m_x;
    double top = -pan_offset_.m_y;
    double right = left + width;
    double bottom = top + height;

    double start_x = std::floor(left / grid_size_) * grid_size_;
    double start_y = std::floor(top / grid_size_) * grid_size_;

    dc.SetPen(wxPen(GridColor(), 1));
    for (double x = start_x; x <= right; x += grid_size_) {
        dc.DrawLine(wxPoint(static_cast<int>(x), static_cast<int>(top)),
                    wxPoint(static_cast<int>(x), static_cast<int>(bottom)));
    }
    for (double y = start_y; y <= bottom; y += grid_size_) {
        dc.DrawLine(wxPoint(static_cast<int>(left), static_cast<int>(y)),
                    wxPoint(static_cast<int>(right), static_cast<int>(y)));
    }
}

void DiagramCanvas::DrawEdges(wxDC& dc) {
    if (model_.edges().empty()) {
        return;
    }
    if (model_.type() == DiagramType::MindMap) {
        // Render mind map hierarchy connections based on parent_id
        for (const auto& node : model_.nodes()) {
            if (node.parent_id.empty() || !IsNodeVisible(node)) {
                continue;
            }
            auto parent_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                          [&](const DiagramNode& n) { return n.id == node.parent_id; });
            if (parent_it == model_.nodes().end() || !IsNodeVisible(*parent_it)) {
                continue;
            }
            wxPoint start = ComputeNodeCenter(*parent_it);
            wxPoint end = ComputeNodeCenter(node);
            wxPoint ctrl((start.x + end.x) / 2, start.y);
            wxPoint points[3] = {start, ctrl, end};
            dc.SetPen(wxPen(wxColour(180, 180, 200), 2));
            dc.DrawSpline(3, points);
            DrawArrow(dc, ctrl, end);
        }
    }
    for (size_t index = 0; index < model_.edges().size(); ++index) {
        const auto& edge = model_.edges()[index];
        auto source_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.source_id; });
        auto target_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.target_id; });
        if (source_it == model_.nodes().end() || target_it == model_.nodes().end()) {
            continue;
        }
        if (!IsNodeVisible(*source_it) || !IsNodeVisible(*target_it)) {
            continue;
        }
        wxColour edge_color = wxColour(190, 190, 190);
        if (selected_edge_index_.has_value() && *selected_edge_index_ == index) {
            edge_color = SelectionColor();
        }
        wxPen pen(edge_color, 2);
        
        // IDEF1X line styles per Silverston spec:
        // - Identifying relationships: solid lines
        // - Non-identifying relationships: dashed lines
        bool is_idef1x = (model_.type() == DiagramType::Erd && model_.notation() == ErdNotation::IDEF1X);
        bool use_dashed = false;
        
        if (model_.type() == DiagramType::Erd && !edge.identifying) {
            use_dashed = true;
        }
        // For IDEF1X, also check cardinality for optionality (optional = dashed)
        if (is_idef1x) {
            // In IDEF1X: optional relationships use dashed lines
            // This is handled by the identifying flag in our model
            // Non-identifying = dashed, Identifying = solid
            use_dashed = !edge.identifying;
        }
        
        if (use_dashed) {
            pen.SetStyle(wxPENSTYLE_SHORT_DASH);
        }
        dc.SetPen(pen);
        wxPoint source_center = ComputeNodeCenter(*source_it);
        wxPoint target_center = ComputeNodeCenter(*target_it);
        wxPoint source = ComputeEdgeAnchor(*source_it, target_center);
        wxPoint target = ComputeEdgeAnchor(*target_it, source_center);
        if (dragging_edge_index_.has_value() && *dragging_edge_index_ == index) {
            if (edge_drag_endpoint_ == EdgeDragEndpoint::Source) {
                source = edge_drag_point_;
                target = ComputeEdgeAnchor(*target_it, source);
            } else if (edge_drag_endpoint_ == EdgeDragEndpoint::Target) {
                target = edge_drag_point_;
                source = ComputeEdgeAnchor(*source_it, target);
            }
        }

        std::vector<wxPoint> path = BuildOrthogonalPath(source, target);
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            bool is_last = (i + 1 == path.size() - 1);
            if ((model_.type() == DiagramType::Silverston ||
                 model_.type() == DiagramType::DataFlow ||
                 model_.type() == DiagramType::MindMap) && is_last) {
                DrawArrow(dc, path[i], path[i + 1]);
            } else {
                dc.DrawLine(path[i], path[i + 1]);
            }
        }

        if (model_.type() == DiagramType::Erd && path.size() >= 2) {
            wxPoint2DDouble start_dir(path[1].x - path[0].x, path[1].y - path[0].y);
            double start_len = std::sqrt(start_dir.m_x * start_dir.m_x + start_dir.m_y * start_dir.m_y);
            if (start_len > 0.01) {
                start_dir.m_x /= start_len;
                start_dir.m_y /= start_len;
            }
            wxPoint2DDouble end_dir(path[path.size() - 2].x - path.back().x,
                                    path[path.size() - 2].y - path.back().y);
            double end_len = std::sqrt(end_dir.m_x * end_dir.m_x + end_dir.m_y * end_dir.m_y);
            if (end_len > 0.01) {
                end_dir.m_x /= end_len;
                end_dir.m_y /= end_len;
            }
            DrawCardinalityMarker(dc, path.front(), start_dir, edge.source_cardinality, model_.notation());
            DrawCardinalityMarker(dc, path.back(), end_dir, edge.target_cardinality, model_.notation());
            
            // Chen notation: draw diamond for relationship
            if (model_.notation() == ErdNotation::Chen) {
                wxPoint mid((path.front().x + path.back().x) / 2,
                           (path.front().y + path.back().y) / 2);
                wxPoint diamond[4] = {
                    wxPoint(mid.x, mid.y - 15),
                    wxPoint(mid.x + 20, mid.y),
                    wxPoint(mid.x, mid.y + 15),
                    wxPoint(mid.x - 20, mid.y)
                };
                dc.SetBrush(wxBrush(wxColour(230, 230, 255)));
                dc.DrawPolygon(4, diamond);
                
                // Draw relationship name in diamond
                if (!edge.label.empty()) {
                    wxSize text_size = dc.GetTextExtent(edge.label);
                    dc.DrawText(edge.label, mid.x - text_size.x / 2, mid.y - text_size.y / 2);
                }
            }
        }

        if (selected_edge_index_.has_value() && *selected_edge_index_ == index) {
            DrawEdgeHandles(dc, path.front(), path.back());
        }

        if (!edge.label.empty()) {
            LabelAnchor anchor = ComputeLabelAnchor(path);
            wxPoint2DDouble perp(-anchor.direction.m_y, anchor.direction.m_x);
            double offset = 8.0 * static_cast<double>(edge.label_offset);
            wxPoint label_pos(static_cast<int>(anchor.point.x + perp.m_x * offset),
                              static_cast<int>(anchor.point.y + perp.m_y * offset));
            wxSize text_size = dc.GetTextExtent(edge.label);
            wxRect label_rect(label_pos.x - text_size.x / 2 - 4,
                              label_pos.y - text_size.y / 2 - 2,
                              text_size.x + 8,
                              text_size.y + 4);
            dc.SetBrush(wxBrush(wxColour(50, 50, 50)));
            dc.SetPen(wxPen(wxColour(80, 80, 80)));
            dc.DrawRectangle(label_rect);
            dc.DrawText(edge.label, label_rect.GetX() + 4, label_rect.GetY() + 2);
        }
    }
}

void DiagramCanvas::DrawNodes(wxDC& dc) {
    wxColour base_fill = model_.type() == DiagramType::Silverston ? SilverstonFillColor() : NodeFillColor();
    wxColour base_border = model_.type() == DiagramType::Silverston ? SilverstonBorderColor() : NodeBorderColor();
    if (model_.type() == DiagramType::Silverston) {
        if (template_key_ == "organization") {
            base_fill = wxColour(58, 74, 68);
            base_border = wxColour(120, 150, 132);
        } else if (template_key_ == "infrastructure") {
            base_fill = wxColour(54, 62, 84);
            base_border = wxColour(120, 130, 160);
        } else if (template_key_ == "network") {
            base_fill = wxColour(64, 58, 86);
            base_border = wxColour(140, 120, 168);
        }
    }
    wxBrush fill(base_fill);
    int border_width = model_.type() == DiagramType::Silverston ? border_width_ : 1;
    wxPen border(base_border, border_width);
    if (model_.type() == DiagramType::Silverston && border_dashed_) {
        border.SetStyle(wxPENSTYLE_SHORT_DASH);
    }
    wxPen selection(SelectionColor(), 2);
    wxColour text_color(235, 235, 235);
    dc.SetTextForeground(text_color);

    const auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        if (!IsNodeVisible(node)) {
            continue;
        }
        wxRect2DDouble rect = WorldRectForNode(node);
        wxRect draw_rect(static_cast<int>(rect.m_x), static_cast<int>(rect.m_y),
                         static_cast<int>(rect.m_width), static_cast<int>(rect.m_height));

        int stack_layers = std::max(1, std::min(node.stack_count, 3));
        wxColour fill_color = node.ghosted ? GhostColor(base_fill) : base_fill;
        wxColour border_color = node.ghosted ? GhostColor(base_border) : base_border;
        wxBrush node_fill(fill_color);
        wxPen node_border(border_color, 1);
        
        // Determine if this is a Note node (for chamfered corners)
        bool is_note_node = (model_.type() == DiagramType::Whiteboard && node.type == "Note") ||
                            (model_.type() == DiagramType::Silverston && node.type == "Note");
        
        // Check if this node is selected (multi-selection support)
        bool is_selected = selected_indices_.count(i) > 0;
        
        for (int layer = stack_layers - 1; layer >= 0; --layer) {
            int offset = layer * 4;
            wxRect layer_rect(draw_rect);
            layer_rect.Offset(offset, offset);
            dc.SetBrush(node_fill);
            if (is_selected && layer == 0) {
                dc.SetPen(selection);
            } else {
                dc.SetPen(node_border);
            }

            if (model_.type() == DiagramType::MindMap) {
                dc.DrawEllipse(layer_rect);
            } else if (model_.type() == DiagramType::DataFlow) {
                if (node.type == "Process") {
                    dc.DrawRoundedRectangle(layer_rect, 12);
                } else if (node.type == "Data Store") {
                    dc.DrawRectangle(layer_rect);
                    wxRect inner = layer_rect;
                    inner.Deflate(6, 0);
                    dc.DrawLine(inner.GetLeft(), inner.GetTop(), inner.GetLeft(), inner.GetBottom());
                    dc.DrawLine(inner.GetRight(), inner.GetTop(), inner.GetRight(), inner.GetBottom());
                } else {
                    dc.DrawRectangle(layer_rect);
                }
            } else if (is_note_node) {
                // Silverston spec: Notes have chamfered top-right corner (12px)
                dc.SetBrush(wxBrush(NoteBackgroundColor()));
                DrawChamferedRect(dc, layer_rect, 12, 0);  // 0 = top-right corner
            } else if (model_.type() == DiagramType::Whiteboard) {
                if (node.type == "Sketch") {
                    wxPen dashed(node_border);
                    dashed.SetStyle(wxPENSTYLE_SHORT_DASH);
                    dc.SetPen(dashed);
                    dc.DrawRectangle(layer_rect);
                } else {
                    dc.DrawRectangle(layer_rect);
                }
            } else if (model_.type() == DiagramType::Silverston) {
                // Silverston objects: draw full border first, then name break will redraw top
                dc.DrawRectangle(layer_rect);
            } else {
                dc.DrawRectangle(layer_rect);
            }
        }

        // Name rendering based on diagram type
        if (model_.type() == DiagramType::Silverston) {
            // Silverston: Name break with top border gap -{ NAME (...) }-
            // TODO: Interactive Mode detection - for now assume interactive
            bool interactive_mode = true;
            DrawSilverstonNameBreak(dc, draw_rect, node.name, node.notes, 
                                    interactive_mode, border_color, text_color);
            // Type is shown in the icon area, not as text below name
        } else if (model_.type() == DiagramType::Erd && !node.attributes.empty()) {
            // ERD: Standard name at top
            dc.DrawText(node.name, draw_rect.GetX() + 8, draw_rect.GetY() + 6);
        } else {
            // Default: Name and type
            dc.DrawText(node.name, draw_rect.GetX() + 8, draw_rect.GetY() + 6);
            if (model_.type() != DiagramType::MindMap) {
                dc.DrawText(node.type, draw_rect.GetX() + 8, draw_rect.GetY() + 26);
            }
        }

        if (model_.type() == DiagramType::MindMap && HasChildren(node)) {
            wxRect chevron = ChevronRectForNode(node);
            wxPoint p1;
            wxPoint p2;
            wxPoint p3;
            if (node.collapsed) {
                p1 = wxPoint(chevron.GetX() + 3, chevron.GetY() + 2);
                p2 = wxPoint(chevron.GetX() + chevron.GetWidth() - 3,
                             chevron.GetY() + chevron.GetHeight() / 2);
                p3 = wxPoint(chevron.GetX() + 3,
                             chevron.GetY() + chevron.GetHeight() - 2);
            } else {
                p1 = wxPoint(chevron.GetX() + 2, chevron.GetY() + 3);
                p2 = wxPoint(chevron.GetX() + chevron.GetWidth() - 2,
                             chevron.GetY() + 3);
                p3 = wxPoint(chevron.GetX() + chevron.GetWidth() / 2,
                             chevron.GetY() + chevron.GetHeight() - 2);
            }
            wxPoint points[3] = {p1, p2, p3};
            dc.SetBrush(wxBrush(wxColour(210, 210, 220)));
            dc.SetPen(wxPen(wxColour(80, 80, 95)));
            dc.DrawPolygon(3, points);

            int descendants = CountDescendants(node.id);
            if (descendants > 0) {
                wxString count = wxString::Format("%d", descendants);
                wxSize count_size = dc.GetTextExtent(count);
                int count_x = chevron.GetX() - count_size.x - 4;
                int count_y = chevron.GetY() + (chevron.GetHeight() - count_size.y) / 2;
                dc.DrawText(count, count_x, count_y);
            }
        }

        if (model_.type() == DiagramType::Silverston) {
            // Silverston: Type icon in top-right corner
            wxRect icon_rect(draw_rect.GetX() + draw_rect.GetWidth() - 28,
                             draw_rect.GetY() - 6, 20, 20);
            dc.SetBrush(wxBrush(IconFillColor(icon_set_)));
            dc.SetPen(wxPen(wxColour(160, 160, 180), 1));
            dc.DrawRectangle(icon_rect);
            if (!node.type.empty()) {
                wxString letter = wxString(node.type.substr(0, 1));
                letter.MakeUpper();
                wxSize text_size = dc.GetTextExtent(letter);
                int text_x = icon_rect.GetX() + (icon_rect.GetWidth() - text_size.x) / 2;
                int text_y = icon_rect.GetY() + (icon_rect.GetHeight() - text_size.y) / 2;
                dc.DrawText(letter, text_x, text_y);
            }
        }

        if (model_.type() == DiagramType::Erd && !node.attributes.empty()) {
            int row_y = draw_rect.GetY() + 48;
            for (const auto& attr : node.attributes) {
                wxString line;
                int icon_offset = 0;
                
                // Draw PK/FK icons instead of text markers
                if (attr.is_primary) {
                    wxPoint key_center(draw_rect.GetX() + 14, row_y + 8);
                    dc.SetPen(wxPen(wxColour(180, 180, 100), 2));
                    dc.SetBrush(wxBrush(wxColour(200, 200, 120)));
                    DrawKeyIcon(dc, key_center, 10);
                    icon_offset = 18;
                } else if (attr.is_foreign) {
                    wxPoint link_center(draw_rect.GetX() + 14, row_y + 8);
                    dc.SetPen(wxPen(wxColour(120, 160, 200), 1));
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                    DrawLinkIcon(dc, link_center, 10);
                    icon_offset = 18;
                }
                
                line = attr.name + " : " + attr.data_type;
                dc.SetTextForeground(text_color);
                dc.DrawText(line, draw_rect.GetX() + 8 + icon_offset, row_y);
                row_y += 18;
            }
        }

        // Draw selection handles for all selected nodes
        // Only draw handles on the primary selection if multiple are selected
        if (is_selected) {
            if (selected_indices_.size() == 1 || selected_index_ == i) {
                DrawSelectionHandles(dc, node);
            }
        }
    }
}

wxRect DiagramCanvas::ChevronRectForNode(const DiagramNode& node) const {
    wxRect2DDouble rect = WorldRectForNode(node);
    int size = 12;
    int x = static_cast<int>(rect.m_x + rect.m_width - size - 6);
    int y = static_cast<int>(rect.m_y + 6);
    return wxRect(x, y, size, size);
}

bool DiagramCanvas::HasChildren(const DiagramNode& node) const {
    return MindMapHasChildren(model_, node.id);
}

int DiagramCanvas::CountDescendants(const std::string& node_id) const {
    return MindMapCountDescendants(model_, node_id);
}

void DiagramCanvas::CenterOnNode(const DiagramNode& node) {
    wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0) return;
    double center_x = node.x + node.width / 2.0;
    double center_y = node.y + node.height / 2.0;
    pan_offset_.m_x = (size.x / (2.0 * zoom_)) - center_x;
    pan_offset_.m_y = (size.y / (2.0 * zoom_)) - center_y;
}

bool DiagramCanvas::IsNodeVisible(const DiagramNode& node) const {
    if (model_.type() != DiagramType::MindMap) {
        return true;
    }
    if (node.parent_id.empty()) {
        return true;
    }
    const DiagramNode* current = &node;
    while (!current->parent_id.empty()) {
        auto parent_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& n) { return n.id == current->parent_id; });
        if (parent_it == model_.nodes().end()) {
            return true;
        }
        if (parent_it->collapsed) {
            return false;
        }
        current = &(*parent_it);
    }
    return true;
}

void DiagramCanvas::DrawSelectionHandles(wxDC& dc, const DiagramNode& node) {
    wxRect2DDouble rect = WorldRectForNode(node);
    double x = rect.m_x;
    double y = rect.m_y;
    double w = rect.m_width;
    double h = rect.m_height;
    const int handle = kHandleSize;
    const double half = handle / 2.0;

    std::vector<wxPoint> points = {
        {static_cast<int>(x), static_cast<int>(y)},
        {static_cast<int>(x + w / 2.0), static_cast<int>(y)},
        {static_cast<int>(x + w), static_cast<int>(y)},
        {static_cast<int>(x), static_cast<int>(y + h / 2.0)},
        {static_cast<int>(x + w), static_cast<int>(y + h / 2.0)},
        {static_cast<int>(x), static_cast<int>(y + h)},
        {static_cast<int>(x + w / 2.0), static_cast<int>(y + h)},
        {static_cast<int>(x + w), static_cast<int>(y + h)}
    };

    dc.SetBrush(wxBrush(SelectionColor()));
    dc.SetPen(wxPen(SelectionColor()));
    for (const auto& point : points) {
        dc.DrawRectangle(point.x - static_cast<int>(half),
                         point.y - static_cast<int>(half),
                         handle, handle);
    }
}

void DiagramCanvas::DrawEdgeHandles(wxDC& dc, const wxPoint& source, const wxPoint& target) {
    const int handle = kEdgeHandleSize;
    const double half = handle / 2.0;
    dc.SetBrush(wxBrush(SelectionColor()));
    dc.SetPen(wxPen(SelectionColor()));
    dc.DrawRectangle(source.x - static_cast<int>(half),
                     source.y - static_cast<int>(half),
                     handle, handle);
    dc.DrawRectangle(target.x - static_cast<int>(half),
                     target.y - static_cast<int>(half),
                     handle, handle);
}

std::optional<size_t> DiagramCanvas::HitTestNode(const wxPoint2DDouble& world_point) const {
    const auto& nodes = model_.nodes();
    for (size_t i = nodes.size(); i-- > 0;) {
        const auto& node = nodes[i];
        if (!IsNodeVisible(node)) {
            continue;
        }
        if (world_point.m_x >= node.x && world_point.m_x <= node.x + node.width &&
            world_point.m_y >= node.y && world_point.m_y <= node.y + node.height) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> DiagramCanvas::HitTestEdge(const wxPoint2DDouble& world_point) const {
    const auto& edges = model_.edges();
    for (size_t i = edges.size(); i-- > 0;) {
        const auto& edge = edges[i];
        auto source_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.source_id; });
        auto target_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.target_id; });
        if (source_it == model_.nodes().end() || target_it == model_.nodes().end()) {
            continue;
        }
        if (!IsNodeVisible(*source_it) || !IsNodeVisible(*target_it)) {
            continue;
        }
        wxPoint source_center = ComputeNodeCenter(*source_it);
        wxPoint target_center = ComputeNodeCenter(*target_it);
        wxPoint source = ComputeEdgeAnchor(*source_it, target_center);
        wxPoint target = ComputeEdgeAnchor(*target_it, source_center);
        std::vector<wxPoint> path = BuildOrthogonalPath(source, target);
        for (size_t seg = 0; seg + 1 < path.size(); ++seg) {
            double distance = DistancePointToSegment(world_point, path[seg], path[seg + 1]);
            if (distance <= 8.0) {
                return i;
            }
        }
    }
    return std::nullopt;
}

ResizeHandle DiagramCanvas::HitTestResizeHandle(const DiagramNode& node,
                                                const wxPoint2DDouble& world_point) const {
    double x = node.x;
    double y = node.y;
    double w = node.width;
    double h = node.height;
    double half = (kHandleSize + kHandleHitPadding) / 2.0;

    struct HandlePoint {
        ResizeHandle handle;
        double hx;
        double hy;
    };
    std::vector<HandlePoint> handles = {
        {ResizeHandle::TopLeft, x, y},
        {ResizeHandle::Top, x + w / 2.0, y},
        {ResizeHandle::TopRight, x + w, y},
        {ResizeHandle::Right, x + w, y + h / 2.0},
        {ResizeHandle::BottomRight, x + w, y + h},
        {ResizeHandle::Bottom, x + w / 2.0, y + h},
        {ResizeHandle::BottomLeft, x, y + h},
        {ResizeHandle::Left, x, y + h / 2.0}
    };
    for (const auto& handle : handles) {
        if (std::abs(world_point.m_x - handle.hx) <= half &&
            std::abs(world_point.m_y - handle.hy) <= half) {
            return handle.handle;
        }
    }
    return ResizeHandle::None;
}

EdgeDragEndpoint DiagramCanvas::HitTestEdgeEndpoint(size_t edge_index,
                                                    const wxPoint2DDouble& world_point) const {
    if (edge_index >= model_.edges().size()) {
        return EdgeDragEndpoint::None;
    }
    const auto& edge = model_.edges()[edge_index];
    auto source_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                  [&](const DiagramNode& node) { return node.id == edge.source_id; });
    auto target_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                  [&](const DiagramNode& node) { return node.id == edge.target_id; });
    if (source_it == model_.nodes().end() || target_it == model_.nodes().end()) {
        return EdgeDragEndpoint::None;
    }
    wxPoint source_center = ComputeNodeCenter(*source_it);
    wxPoint target_center = ComputeNodeCenter(*target_it);
    wxPoint source = ComputeEdgeAnchor(*source_it, target_center);
    wxPoint target = ComputeEdgeAnchor(*target_it, source_center);
    double source_dist = std::sqrt(std::pow(world_point.m_x - source.x, 2) +
                                   std::pow(world_point.m_y - source.y, 2));
    double target_dist = std::sqrt(std::pow(world_point.m_x - target.x, 2) +
                                   std::pow(world_point.m_y - target.y, 2));
    if (source_dist <= kEdgeEndpointRadius && source_dist <= target_dist) {
        return EdgeDragEndpoint::Source;
    }
    if (target_dist <= kEdgeEndpointRadius) {
        return EdgeDragEndpoint::Target;
    }
    return EdgeDragEndpoint::None;
}

void DiagramCanvas::UpdateSelection(std::optional<size_t> node_index, std::optional<size_t> edge_index) {
    if (selected_index_ == node_index && selected_edge_index_ == edge_index) {
        return;
    }
    selected_index_ = node_index;
    selected_edge_index_ = edge_index;
    wxCommandEvent event(EVT_DIAGRAM_SELECTION_CHANGED);
    if (selected_index_.has_value()) {
        event.SetString("node");
        event.SetInt(static_cast<int>(*selected_index_));
    } else if (selected_edge_index_.has_value()) {
        event.SetString("edge");
        event.SetInt(static_cast<int>(*selected_edge_index_));
    } else {
        event.SetString("none");
        event.SetInt(-1);
    }
    wxPostEvent(GetParent(), event);
    Refresh();
}

wxPoint2DDouble DiagramCanvas::NextInsertPosition(double width, double height) {
    int index = static_cast<int>(model_.nodes().size());
    int columns = 3;
    int column = index % columns;
    int row = index / columns;
    double gap = 40.0;
    return wxPoint2DDouble(40.0 + column * (width + gap),
                           40.0 + row * (height + gap));
}

wxCursor DiagramCanvas::CursorForHandle(ResizeHandle handle) const {
    switch (handle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::BottomRight:
            return wxCursor(wxCURSOR_SIZENWSE);
        case ResizeHandle::TopRight:
        case ResizeHandle::BottomLeft:
            return wxCursor(wxCURSOR_SIZENESW);
        case ResizeHandle::Top:
        case ResizeHandle::Bottom:
            return wxCursor(wxCURSOR_SIZENS);
        case ResizeHandle::Left:
        case ResizeHandle::Right:
            return wxCursor(wxCURSOR_SIZEWE);
        default:
            return wxCursor(wxCURSOR_ARROW);
    }
}

void DiagramCanvas::UpdateHoverCursor(const wxPoint2DDouble& world_point) {
    if (selected_index_.has_value()) {
        const auto& node = model_.nodes()[*selected_index_];
        ResizeHandle handle = HitTestResizeHandle(node, world_point);
        if (handle != ResizeHandle::None) {
            SetCursor(CursorForHandle(handle));
            return;
        }
    }
    if (selected_edge_index_.has_value()) {
        EdgeDragEndpoint endpoint = HitTestEdgeEndpoint(*selected_edge_index_, world_point);
        if (endpoint != EdgeDragEndpoint::None) {
            SetCursor(wxCursor(wxCURSOR_CROSS));
            return;
        }
    }
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void DiagramCanvas::ApplyResize(const wxPoint2DDouble& world_point) {
    if (!resizing_index_.has_value()) {
        return;
    }
    auto& node = model_.nodes()[*resizing_index_];
    double min_w = model_.type() == DiagramType::Erd ? kMinErdWidth : kMinSilverWidth;
    double min_h = model_.type() == DiagramType::Erd ? kMinErdHeight : kMinSilverHeight;
    double dx = world_point.m_x - resize_start_point_.m_x;
    double dy = world_point.m_y - resize_start_point_.m_y;
    double x = resize_start_rect_.m_x;
    double y = resize_start_rect_.m_y;
    double w = resize_start_rect_.m_width;
    double h = resize_start_rect_.m_height;

    switch (resize_handle_) {
        case ResizeHandle::TopLeft:
            x += dx;
            y += dy;
            w -= dx;
            h -= dy;
            break;
        case ResizeHandle::Top:
            y += dy;
            h -= dy;
            break;
        case ResizeHandle::TopRight:
            y += dy;
            w += dx;
            h -= dy;
            break;
        case ResizeHandle::Right:
            w += dx;
            break;
        case ResizeHandle::BottomRight:
            w += dx;
            h += dy;
            break;
        case ResizeHandle::Bottom:
            h += dy;
            break;
        case ResizeHandle::BottomLeft:
            x += dx;
            w -= dx;
            h += dy;
            break;
        case ResizeHandle::Left:
            x += dx;
            w -= dx;
            break;
        default:
            break;
    }

    if (w < min_w) {
        if (resize_handle_ == ResizeHandle::TopLeft || resize_handle_ == ResizeHandle::Left ||
            resize_handle_ == ResizeHandle::BottomLeft) {
            x -= (min_w - w);
        }
        w = min_w;
    }
    if (h < min_h) {
        if (resize_handle_ == ResizeHandle::TopLeft || resize_handle_ == ResizeHandle::Top ||
            resize_handle_ == ResizeHandle::TopRight) {
            y -= (min_h - h);
        }
        h = min_h;
    }

    node.x = x;
    node.y = y;
    node.width = w;
    node.height = h;
    Refresh();
}

// ============================================================================
// Multi-Selection Support
// ============================================================================

bool DiagramCanvas::IsNodeSelected(size_t index) const {
    return selected_indices_.count(index) > 0;
}

bool DiagramCanvas::IsNodeSelected(const std::string& node_id) const {
    const auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].id == node_id) {
            return selected_indices_.count(i) > 0;
        }
    }
    return false;
}

void DiagramCanvas::ToggleNodeSelection(size_t index) {
    if (selected_indices_.count(index) > 0) {
        selected_indices_.erase(index);
        if (selected_index_ == index) {
            selected_index_.reset();
        }
    } else {
        selected_indices_.insert(index);
        selected_index_ = index;
    }
    Refresh();
}

void DiagramCanvas::SelectNode(size_t index, bool add_to_selection) {
    if (!add_to_selection) {
        selected_indices_.clear();
        selected_index_ = index;
        selected_edge_index_.reset();
    }
    if (index < model_.nodes().size()) {
        selected_indices_.insert(index);
        selected_index_ = index;
    }
    Refresh();
}

void DiagramCanvas::ClearSelection() {
    selected_indices_.clear();
    selected_index_.reset();
    selected_edge_index_.reset();
    Refresh();
}

std::vector<size_t> DiagramCanvas::GetSelectedNodeIndices() const {
    return std::vector<size_t>(selected_indices_.begin(), selected_indices_.end());
}

std::vector<std::string> DiagramCanvas::GetSelectedNodeIds() const {
    std::vector<std::string> ids;
    const auto& nodes = model_.nodes();
    for (size_t idx : selected_indices_) {
        if (idx < nodes.size()) {
            ids.push_back(nodes[idx].id);
        }
    }
    return ids;
}

std::vector<DiagramCanvas::DependencyInfo> DiagramCanvas::GetDependenciesForNodes(
    const std::vector<std::string>& node_ids) const {
    std::vector<DependencyInfo> dependencies;
    
    // Query the model for edges that connect to nodes outside the deletion set
    // These represent foreign key dependencies or view references
    for (const auto& edge : model_.edges()) {
        bool source_being_deleted = std::find(node_ids.begin(), node_ids.end(), edge.source_id) != node_ids.end();
        bool target_being_deleted = std::find(node_ids.begin(), node_ids.end(), edge.target_id) != node_ids.end();
        
        if (source_being_deleted && !target_being_deleted) {
            // Source is being deleted but target depends on it
            DependencyInfo dep;
            dep.object_id = edge.source_id;
            dep.dependency_type = "relationship";
            dep.dependent_object = edge.target_id;
            dependencies.push_back(dep);
        } else if (target_being_deleted && !source_being_deleted) {
            // Target is being deleted but source depends on it
            DependencyInfo dep;
            dep.object_id = edge.target_id;
            dep.dependency_type = "relationship";
            dep.dependent_object = edge.source_id;
            dependencies.push_back(dep);
        }
    }
    
    return dependencies;
}

// ============================================================================
// Keyboard Handling
// ============================================================================

void DiagramCanvas::OnKeyDown(wxKeyEvent& event) {
    int key_code = event.GetKeyCode();
    bool shift_down = event.ShiftDown();
    bool ctrl_down = event.ControlDown() || event.CmdDown();
    
    // Update multi-select mode
    multi_select_mode_ = ctrl_down;
    
    switch (key_code) {
        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            if (shift_down) {
                // Shift+Delete = delete from project
                DeleteSelectionFromProject();
            } else {
                // Delete = delete from diagram only
                DeleteSelection();
            }
            break;
            
        case 'A':
            if (ctrl_down) {
                // Ctrl+A = select all
                selected_indices_.clear();
                for (size_t i = 0; i < model_.nodes().size(); ++i) {
                    selected_indices_.insert(i);
                }
                if (!model_.nodes().empty()) {
                    selected_index_ = 0;
                }
                Refresh();
            } else {
                event.Skip();
            }
            break;
            
        case WXK_ESCAPE:
            ClearSelection();
            break;
            
        default:
            event.Skip();
            break;
    }
}

void DiagramCanvas::OnContextMenu(wxContextMenuEvent& event) {
    wxPoint pos = ScreenToClient(event.GetPosition());
    if (pos.x < 0 || pos.y < 0) {
        pos = wxPoint(GetClientSize().x / 2, GetClientSize().y / 2);
    }
    ShowDiagramContextMenu(pos);
}

void DiagramCanvas::ShowDiagramContextMenu(const wxPoint& pos) {
    wxMenu menu;
    
    bool has_selection = !selected_indices_.empty() || selected_edge_index_.has_value();
    bool has_node_selection = !selected_indices_.empty();
    
    // Delete options
    if (has_selection) {
        if (GetSelectionCount() == 1) {
            menu.Append(ID_DELETE_FROM_DIAGRAM, "Delete from Diagram\tDel", 
                       "Remove from diagram (keeps project object)");
            menu.Append(ID_DELETE_FROM_PROJECT, "Delete from Project\tShift+Del", 
                       "Permanently delete from project");
        } else {
            menu.Append(ID_DELETE_FROM_DIAGRAM, 
                       wxString::Format("Delete %zu from Diagram\tDel", GetSelectionCount()), 
                       "Remove from diagram (keeps project objects)");
            menu.Append(ID_DELETE_FROM_PROJECT, 
                       wxString::Format("Delete %zu from Project\tShift+Del", GetSelectionCount()), 
                       "Permanently delete from project");
        }
        menu.AppendSeparator();
    }
    
    // Copy/Paste
    menu.Append(wxID_COPY, "Copy\tCtrl+C");
    menu.Append(wxID_PASTE, "Paste\tCtrl+V");
    menu.Enable(wxID_COPY, has_selection);
    menu.Enable(wxID_PASTE, CanPaste());
    
    if (has_node_selection) {
        menu.AppendSeparator();
        
        // Alignment submenu
        wxMenu* alignMenu = new wxMenu();
        alignMenu->Append(ID_ALIGN_LEFT, "Align Left");
        alignMenu->Append(ID_ALIGN_RIGHT, "Align Right");
        alignMenu->Append(ID_ALIGN_TOP, "Align Top");
        alignMenu->Append(ID_ALIGN_BOTTOM, "Align Bottom");
        menu.AppendSubMenu(alignMenu, "Align");
        
        // Pin options
        menu.AppendSeparator();
        if (GetSelectionCount() == 1 && selected_index_.has_value()) {
            if (IsSelectedNodePinned()) {
                menu.Append(ID_UNPIN_NODE, "Unpin Node");
            } else {
                menu.Append(ID_PIN_NODE, "Pin Node");
            }
        }
    }
    
    PopupMenu(&menu, pos);
}

void DiagramCanvas::OnDeleteFromDiagram(wxCommandEvent& event) {
    DeleteSelection();
}

void DiagramCanvas::OnDeleteFromProject(wxCommandEvent& event) {
    DeleteSelectionFromProject();
}

// ============================================================================
// Copy/Paste (Phase 3.3.8)
// ============================================================================

static DiagramNode s_copiedNode;  // Simple single-node clipboard
static bool s_hasClipboard = false;

void DiagramCanvas::CopySelection() {
    if (!selected_index_.has_value()) return;
    
    const auto& nodes = model_.nodes();
    size_t idx = *selected_index_;
    if (idx >= nodes.size()) return;
    
    s_copiedNode = nodes[idx];
    // Clear ID so paste creates a new unique node
    s_copiedNode.id.clear();
    s_hasClipboard = true;
}

void DiagramCanvas::Paste() {
    if (!s_hasClipboard) return;
    
    // Create a new node based on the copied one
    DiagramNode newNode = s_copiedNode;
    // Generate new ID
    int idx = model_.NextNodeIndex();
    newNode.id = "node_" + std::to_string(idx);
    // Offset position slightly
    newNode.x += 20;
    newNode.y += 20;
    
    // Add to model
    model_.AddNode(newNode);
    
    // Select the new node (it will be the last one)
    selected_index_ = model_.nodes().size() - 1;
    
    Refresh();
    
    // Notify selection changed
    wxCommandEvent evt(EVT_DIAGRAM_SELECTION_CHANGED);
    evt.SetInt(static_cast<int>(*selected_index_));
    wxPostEvent(this, evt);
}

bool DiagramCanvas::CanPaste() const {
    return s_hasClipboard;
}

bool DiagramCanvas::CanDeleteSelection() const {
    return !selected_indices_.empty() || selected_edge_index_.has_value();
}

void DiagramCanvas::DeleteSelection() {
    // Handle edge deletion first
    if (selected_edge_index_.has_value()) {
        size_t edge_idx = *selected_edge_index_;
        auto& edges = model_.edges();
        if (edge_idx < edges.size()) {
            auto command = std::make_unique<DeleteEdgeCommand>(&model_, edges[edge_idx].id);
            command_manager_.Execute(std::move(command));
        }
        selected_edge_index_.reset();
        Refresh();
        return;
    }
    
    // Handle node deletion
    if (selected_indices_.empty()) return;
    
    // Get all selected node IDs
    std::vector<std::string> node_ids = GetSelectedNodeIds();
    if (node_ids.empty()) return;
    
    // Use multi-delete command for undo/redo support
    auto command = std::make_unique<DeleteMultipleNodesCommand>(&model_, node_ids);
    command_manager_.Execute(std::move(command));
    
    selected_indices_.clear();
    selected_index_.reset();
    Refresh();
}

void DiagramCanvas::DeleteSelectionFromProject() {
    if (selected_indices_.empty() && !selected_edge_index_.has_value()) return;
    
    // Get selected node IDs
    std::vector<std::string> node_ids = GetSelectedNodeIds();
    if (node_ids.empty()) return;
    
    // Check for dependencies
    auto dependencies = GetDependenciesForNodes(node_ids);
    
    // Build confirmation message
    wxString message;
    wxString title;
    
    if (node_ids.size() == 1) {
        const auto& node = model_.nodes()[*selected_index_];
        title = "Delete from Project";
        message = wxString::Format(
            "Are you sure you want to permanently delete '%s' from the project?\n\n",
            node.name);
    } else {
        title = "Delete Multiple Objects from Project";
        message = wxString::Format(
            "Are you sure you want to permanently delete %zu objects from the project?\n\n",
            node_ids.size());
    }
    
    message += "This action cannot be undone.\n";
    
    if (!dependencies.empty()) {
        message += "\nWARNING: This will also affect related objects:\n";
        // Limit displayed dependencies to avoid huge dialogs
        size_t display_count = std::min(dependencies.size(), size_t(10));
        for (size_t i = 0; i < display_count; ++i) {
            message += wxString::Format("  - %s depends on this\n", dependencies[i].dependent_object);
        }
        if (dependencies.size() > 10) {
            message += wxString::Format("  ... and %zu more\n", dependencies.size() - 10);
        }
    }
    
    // Show confirmation dialog
    wxMessageDialog dialog(this, message, title, 
                          wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    dialog.SetYesNoLabels("Delete", "Cancel");
    
    if (dialog.ShowModal() != wxID_YES) {
        return;
    }
    
    // Create the project delete command with a callback
    // The callback would normally interface with the database layer
    auto delete_callback = [](const std::vector<std::string>& ids, std::string* error) -> bool {
        // In a real implementation, this would:
        // 1. Check foreign key constraints
        // 2. Drop dependent views/triggers if cascade is enabled
        // 3. Execute DROP statements in the database
        // 4. Update project metadata
        
        // For now, simulate success
        // TODO: Integrate with actual database layer
        (void)ids;  // Suppress unused parameter warning
        (void)error;
        return true;
    };
    
    auto command = std::make_unique<ProjectDeleteCommand>(&model_, node_ids, delete_callback);
    command_manager_.Execute(std::move(command));
    
    // Clear selection after deletion
    selected_indices_.clear();
    selected_index_.reset();
    selected_edge_index_.reset();
    
    Refresh();
}

// ============================================================================
// Alignment Tools (Phase 3.3.9)
// ============================================================================

void DiagramCanvas::AlignLeft() {
    if (!selected_index_.has_value()) return;
    
    // Find leftmost position among selected nodes (for multi-select, we'd use all selected)
    double minX = model_.nodes()[*selected_index_].x;
    
    // Align all nodes to this position
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        // In multi-select mode, we'd check if node is selected
        nodes[i].x = minX;
    }
    Refresh();
}

void DiagramCanvas::AlignRight() {
    if (!selected_index_.has_value()) return;
    
    const auto& node = model_.nodes()[*selected_index_];
    double rightEdge = node.x + node.width;
    
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        nodes[i].x = rightEdge - nodes[i].width;
    }
    Refresh();
}

void DiagramCanvas::AlignTop() {
    if (!selected_index_.has_value()) return;
    
    double minY = model_.nodes()[*selected_index_].y;
    
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        nodes[i].y = minY;
    }
    Refresh();
}

void DiagramCanvas::AlignBottom() {
    if (!selected_index_.has_value()) return;
    
    const auto& node = model_.nodes()[*selected_index_];
    double bottomEdge = node.y + node.height;
    
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        nodes[i].y = bottomEdge - nodes[i].height;
    }
    Refresh();
}

void DiagramCanvas::AlignCenterHorizontal() {
    if (!selected_index_.has_value()) return;
    
    const auto& node = model_.nodes()[*selected_index_];
    double centerX = node.x + node.width / 2;
    
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        nodes[i].x = centerX - nodes[i].width / 2;
    }
    Refresh();
}

void DiagramCanvas::AlignCenterVertical() {
    if (!selected_index_.has_value()) return;
    
    const auto& node = model_.nodes()[*selected_index_];
    double centerY = node.y + node.height / 2;
    
    auto& nodes = model_.nodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i == *selected_index_) continue;
        nodes[i].y = centerY - nodes[i].height / 2;
    }
    Refresh();
}

void DiagramCanvas::DistributeHorizontal() {
    auto& nodes = model_.nodes();
    if (nodes.size() < 3) return;
    
    // Sort by x position
    std::vector<size_t> indices(nodes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&nodes](size_t a, size_t b) {
        return nodes[a].x < nodes[b].x;
    });
    
    double minX = nodes[indices.front()].x;
    double maxX = nodes[indices.back()].x + nodes[indices.back()].width;
    double totalWidth = maxX - minX;
    double spacing = totalWidth / (nodes.size() - 1);
    
    for (size_t i = 1; i < indices.size() - 1; ++i) {
        nodes[indices[i]].x = minX + spacing * i - nodes[indices[i]].width / 2;
    }
    Refresh();
}

void DiagramCanvas::DistributeVertical() {
    auto& nodes = model_.nodes();
    if (nodes.size() < 3) return;
    
    // Sort by y position
    std::vector<size_t> indices(nodes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&nodes](size_t a, size_t b) {
        return nodes[a].y < nodes[b].y;
    });
    
    double minY = nodes[indices.front()].y;
    double maxY = nodes[indices.back()].y + nodes[indices.back()].height;
    double totalHeight = maxY - minY;
    double spacing = totalHeight / (nodes.size() - 1);
    
    for (size_t i = 1; i < indices.size() - 1; ++i) {
        nodes[indices[i]].y = minY + spacing * i - nodes[indices[i]].height / 2;
    }
    Refresh();
}

// ============================================================================
// Pin/Unpin Nodes (Phase 3.4.6)
// ============================================================================

void DiagramCanvas::PinSelectedNode() {
    if (!selected_index_.has_value()) return;
    
    auto& nodes = model_.nodes();
    if (*selected_index_ < nodes.size()) {
        nodes[*selected_index_].pinned = true;
        Refresh();
    }
}

void DiagramCanvas::UnpinSelectedNode() {
    if (!selected_index_.has_value()) return;
    
    auto& nodes = model_.nodes();
    if (*selected_index_ < nodes.size()) {
        nodes[*selected_index_].pinned = false;
        Refresh();
    }
}

void DiagramCanvas::TogglePinSelectedNode() {
    if (!selected_index_.has_value()) return;
    
    auto& nodes = model_.nodes();
    if (*selected_index_ < nodes.size()) {
        nodes[*selected_index_].pinned = !nodes[*selected_index_].pinned;
        Refresh();
    }
}

bool DiagramCanvas::IsSelectedNodePinned() const {
    if (!selected_index_.has_value()) return false;
    
    const auto& nodes = model_.nodes();
    if (*selected_index_ < nodes.size()) {
        return nodes[*selected_index_].pinned;
    }
    return false;
}

std::vector<std::string> DiagramCanvas::GetPinnedNodeIds() const {
    std::vector<std::string> pinnedIds;
    for (const auto& node : model_.nodes()) {
        if (node.pinned) {
            pinnedIds.push_back(node.id);
        }
    }
    return pinnedIds;
}

// ============================================================================
// Drag and Drop Support (Parent/Child Containment)
// ============================================================================

void DiagramCanvas::BeginExternalDrag(const std::string& node_type, const std::string& node_name) {
    external_drag_active_ = true;
    external_drag_items_.clear();
    external_drag_items_.push_back({node_type, node_name});
    current_drag_operation_ = DragOperation::AddFromTree;
    SetCursor(wxCursor(wxCURSOR_CROSS));
}

void DiagramCanvas::BeginExternalDragMultiple(const std::vector<std::pair<std::string, std::string>>& items) {
    external_drag_active_ = true;
    external_drag_items_ = items;
    current_drag_operation_ = DragOperation::AddFromTree;
    SetCursor(wxCursor(wxCURSOR_CROSS));
}

void DiagramCanvas::CancelExternalDrag() {
    external_drag_active_ = false;
    external_drag_items_.clear();
    current_drag_operation_ = DragOperation::None;
    ClearDragTarget();
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
}

bool DiagramCanvas::CanDropOnNode(size_t dragged_index, size_t target_index) const {
    if (dragged_index == target_index) return false;
    
    const auto& nodes = model_.nodes();
    if (dragged_index >= nodes.size() || target_index >= nodes.size()) return false;
    
    const auto& dragged = nodes[dragged_index];
    const auto& target = nodes[target_index];
    
    // Check if target is a container type
    if (!IsContainerType(target.type)) return false;
    
    // Check containment rules
    if (!CanAcceptChild(target.type, dragged.type)) return false;
    
    // Check for circular reference
    if (WouldCreateCircularReference(dragged.id, target.id)) return false;
    
    return true;
}

bool DiagramCanvas::CanDropOnNode(const std::string& dragged_type, size_t target_index) const {
    const auto& nodes = model_.nodes();
    if (target_index >= nodes.size()) return false;
    
    const auto& target = nodes[target_index];
    
    // Check if target is a container type
    if (!IsContainerType(target.type)) return false;
    
    // Check containment rules
    return CanAcceptChild(target.type, dragged_type);
}

bool DiagramCanvas::WouldCreateCircularReference(const std::string& node_id, 
                                                  const std::string& potential_parent_id) const {
    // Check if making potential_parent_id the parent of node_id would create a cycle
    // This happens if potential_parent_id is already a descendant of node_id
    
    const auto& nodes = model_.nodes();
    
    // Walk up from potential_parent_id
    std::string current_id = potential_parent_id;
    while (!current_id.empty()) {
        if (current_id == node_id) {
            return true;  // Found cycle
        }
        
        // Find parent of current
        auto it = std::find_if(nodes.begin(), nodes.end(),
                               [&current_id](const DiagramNode& n) { return n.id == current_id; });
        if (it == nodes.end()) break;
        current_id = it->parent_id;
    }
    
    return false;
}

void DiagramCanvas::UpdateDragTarget(const wxPoint2DDouble& world_point) {
    // Find the node under the cursor
    auto hit = HitTestNode(world_point);
    
    if (hit.has_value()) {
        size_t target_idx = *hit;
        bool is_valid = false;
        
        if (dragging_index_.has_value()) {
            // Internal drag - check if valid reparent
            is_valid = CanDropOnNode(*dragging_index_, target_idx);
        } else if (external_drag_active_ && !external_drag_items_.empty()) {
            // External drag - check if valid for the first item
            is_valid = CanDropOnNode(external_drag_items_[0].first, target_idx);
        }
        
        if (is_valid || IsContainerType(model_.nodes()[target_idx].type)) {
            drag_target_index_ = target_idx;
            drag_target_valid_ = is_valid;
        } else {
            ClearDragTarget();
        }
    } else {
        ClearDragTarget();
    }
}

void DiagramCanvas::ClearDragTarget() {
    drag_target_index_.reset();
    drag_target_valid_ = false;
}

bool DiagramCanvas::IsValidDropTarget(size_t target_index) const {
    if (!drag_target_index_.has_value()) return false;
    return *drag_target_index_ == target_index && drag_target_valid_;
}

wxPoint2DDouble DiagramCanvas::SnapToGrid(const wxPoint2DDouble& pos) const {
    if (grid_size_ <= 0) return pos;
    
    double snapped_x = std::round(pos.m_x / grid_size_) * grid_size_;
    double snapped_y = std::round(pos.m_y / grid_size_) * grid_size_;
    return wxPoint2DDouble(snapped_x, snapped_y);
}

void DiagramCanvas::DrawDragOverlay(wxDC& dc) {
    // Draw drop target highlight
    if (drag_target_index_.has_value()) {
        DrawDropTargetHighlight(dc);
    }
    
    // Draw ghost of dragged node(s) for external drag
    if (external_drag_active_ && !external_drag_items_.empty()) {
        wxPoint2DDouble ghost_pos = drag_current_pos_;
        for (size_t i = 0; i < external_drag_items_.size() && i < 5; ++i) {
            // Offset multiple items
            wxPoint2DDouble offset_pos = ghost_pos + wxPoint2DDouble(i * 20, i * 20);
            DrawGhostNode(dc, offset_pos, external_drag_items_[i].first);
        }
        
        // If more than 5 items, show count
        if (external_drag_items_.size() > 5) {
            dc.SetTextForeground(wxColour(255, 255, 255));
            wxString count = wxString::Format("+%zu more", external_drag_items_.size() - 5);
            dc.DrawText(count, 
                       static_cast<int>(ghost_pos.m_x + 100),
                       static_cast<int>(ghost_pos.m_y + 100));
        }
    }
}

void DiagramCanvas::DrawDropTargetHighlight(wxDC& dc) {
    if (!drag_target_index_.has_value()) return;
    
    size_t idx = *drag_target_index_;
    const auto& nodes = model_.nodes();
    if (idx >= nodes.size()) return;
    
    const auto& node = nodes[idx];
    wxRect2DDouble rect = WorldRectForNode(node);
    
    wxColour highlight_color = drag_target_valid_ ? ValidDropColor() : InvalidDropColor();
    
    // Draw highlight border
    wxPen highlight_pen(highlight_color, 3);
    dc.SetPen(highlight_pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    
    wxRect draw_rect(static_cast<int>(rect.m_x - 3),
                     static_cast<int>(rect.m_y - 3),
                     static_cast<int>(rect.m_width + 6),
                     static_cast<int>(rect.m_height + 6));
    dc.DrawRectangle(draw_rect);
    
    // Draw semi-transparent fill
    wxColour fill_color(highlight_color.Red(), highlight_color.Green(), 
                       highlight_color.Blue(), 50);
    dc.SetBrush(wxBrush(fill_color));
    dc.DrawRectangle(draw_rect);
    
    // Draw indicator text
    dc.SetTextForeground(highlight_color);
    wxString indicator = drag_target_valid_ ? "✓ Drop here" : "✗ Invalid";
    dc.DrawText(indicator, 
               static_cast<int>(rect.m_x),
               static_cast<int>(rect.m_y - 20));
}

void DiagramCanvas::DrawGhostNode(wxDC& dc, const wxPoint2DDouble& pos, const std::string& node_type) {
    // Semi-transparent ghost representation
    double width = 160.0;
    double height = 100.0;
    
    wxRect ghost_rect(static_cast<int>(pos.m_x - width/2),
                      static_cast<int>(pos.m_y - height/2),
                      static_cast<int>(width),
                      static_cast<int>(height));
    
    // Ghost fill
    wxColour ghost_fill(100, 100, 100, 100);
    dc.SetBrush(wxBrush(ghost_fill));
    dc.SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_SHORT_DASH));
    dc.DrawRectangle(ghost_rect);
    
    // Type label
    dc.SetTextForeground(wxColour(200, 200, 200));
    dc.DrawText(node_type, ghost_rect.GetX() + 8, ghost_rect.GetY() + 8);
}

void DiagramCanvas::ExecuteDrop(const wxPoint2DDouble& world_point) {
    if (!drag_target_index_.has_value()) return;
    
    size_t target_idx = *drag_target_index_;
    if (!drag_target_valid_) return;
    
    if (dragging_index_.has_value()) {
        ExecuteReparent(*dragging_index_, target_idx, world_point);
    }
}

void DiagramCanvas::ExecuteReparent(size_t node_index, size_t new_parent_index, 
                                     const wxPoint2DDouble& new_pos) {
    auto& nodes = model_.nodes();
    if (node_index >= nodes.size() || new_parent_index >= nodes.size()) return;
    
    auto& node = nodes[node_index];
    auto& new_parent = nodes[new_parent_index];
    
    std::string old_parent_id = node.parent_id;
    std::string new_parent_id = new_parent.id;
    
    // Create and execute the reparent command
    auto command = std::make_unique<ReparentNodeCommand>(
        &model_, node.id, old_parent_id, new_parent_id,
        node.x, node.y, new_pos.m_x, new_pos.m_y);
    
    command_manager_.Execute(std::move(command));
    
    Refresh();
}

void DiagramCanvas::ExecuteExternalDrop(const wxPoint2DDouble& world_point, 
                                         const std::string& node_type, 
                                         const std::string& node_name) {
    // Check if dropping on a valid parent
    std::string parent_id;
    
    if (drag_target_index_.has_value() && drag_target_valid_) {
        parent_id = model_.nodes()[*drag_target_index_].id;
    }
    
    // Create the new node
    DiagramNode node;
    node.id = DiagramTypeKey(model_.type()) + "_node_" + std::to_string(model_.NextNodeIndex());
    node.type = node_type;
    node.name = node_name;
    node.parent_id = parent_id;
    
    // Set default dimensions based on type
    if (node_type == "Table") {
        node.width = 220.0;
        node.height = 160.0;
    } else if (node_type == "Schema" || node_type == "Database") {
        node.width = 240.0;
        node.height = 180.0;
    } else {
        node.width = 180.0;
        node.height = 120.0;
    }
    
    // Set position (snap to grid)
    wxPoint2DDouble pos = SnapToGrid(world_point - wxPoint2DDouble(node.width/2, node.height/2));
    node.x = pos.m_x;
    node.y = pos.m_y;
    
    // Add to model
    auto command = std::make_unique<AddNodeCommand>(&model_, node);
    command_manager_.Execute(std::move(command));
    
    Refresh();
}

void DiagramCanvas::ExecuteMultiExternalDrop(const wxPoint2DDouble& world_point) {
    if (external_drag_items_.empty()) return;
    
    // Create a compound command for multiple drops
    auto compound = std::make_unique<CompoundCommand>();
    
    wxPoint2DDouble current_pos = world_point;
    
    for (const auto& item : external_drag_items_) {
        ExecuteExternalDrop(current_pos, item.first, item.second);
        // Offset next item
        current_pos = current_pos + wxPoint2DDouble(20, 20);
    }
    
    Refresh();
}

void DiagramCanvas::UpdateDragCursor() {
    if (drag_target_index_.has_value()) {
        if (drag_target_valid_) {
            SetCursor(wxCursor(wxCURSOR_HAND));
        } else {
            SetCursor(wxCursor(wxCURSOR_NO_ENTRY));
        }
    } else {
        SetCursor(wxCursor(wxCURSOR_CROSS));
    }
}

} // namespace scratchrobin
