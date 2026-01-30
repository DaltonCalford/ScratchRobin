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
#include <vector>

#include <wx/cursor.h>
#include <wx/dcbuffer.h>
#include <wx/geometry.h>
#include <wx/utils.h>

namespace scratchrobin {

wxDEFINE_EVENT(EVT_DIAGRAM_SELECTION_CHANGED, wxCommandEvent);

namespace {

constexpr double kMinZoom = 0.1;
constexpr double kMaxZoom = 4.0;
constexpr double kZoomStep = 0.1;
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

void DrawCardinalityMarker(wxDC& dc, const wxPoint& end, const wxPoint2DDouble& direction,
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
    Bind(wxEVT_RIGHT_DOWN, &DiagramCanvas::OnRightDown, this);
    Bind(wxEVT_RIGHT_UP, &DiagramCanvas::OnRightUp, this);
    Bind(wxEVT_MOTION, &DiagramCanvas::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &DiagramCanvas::OnMouseWheel, this);
    Bind(wxEVT_SIZE, &DiagramCanvas::OnSize, this);
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

    model_.AddNode(node);
    Refresh();
}

void DiagramCanvas::AddEdge(const std::string& source_id, const std::string& target_id, const std::string& label) {
    DiagramEdge edge;
    edge.id = DiagramTypeKey(model_.type()) + "_edge_" + std::to_string(model_.NextEdgeIndex());
    edge.source_id = source_id;
    edge.target_id = target_id;
    edge.label = label;
    edge.directed = model_.type() == DiagramType::Silverston;
    edge.identifying = false;
    model_.AddEdge(edge);
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
    DrawNodes(dc);
}

void DiagramCanvas::OnLeftDown(wxMouseEvent& event) {
    SetFocus();
    wxPoint2DDouble world_point_d = ScreenToWorldDouble(event.GetPosition());
    wxPoint world_point = ScreenToWorld(event.GetPosition());
    bool capture = false;

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
            UpdateSelection(node_hit, std::nullopt);
            const auto& node = model_.nodes()[*node_hit];
            dragging_index_ = node_hit;
            drag_offset_.m_x = world_point_d.m_x - node.x;
            drag_offset_.m_y = world_point_d.m_y - node.y;
            capture = true;
        } else {
            auto edge_hit = HitTestEdge(world_point_d);
            UpdateSelection(std::nullopt, edge_hit);
            dragging_index_.reset();
            if (edge_hit.has_value()) {
                EdgeDragEndpoint endpoint = HitTestEdgeEndpoint(*edge_hit, world_point_d);
                if (endpoint != EdgeDragEndpoint::None) {
                    dragging_edge_index_ = edge_hit;
                    edge_drag_endpoint_ = endpoint;
                    edge_drag_point_ = world_point;
                    capture = true;
                }
            }
        }
    }

    if (capture) {
        CaptureMouse();
    }
}

void DiagramCanvas::OnLeftUp(wxMouseEvent&) {
    if (HasCapture()) {
        ReleaseMouse();
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
        node.x = world_point.m_x - drag_offset_.m_x;
        node.y = world_point.m_y - drag_offset_.m_y;
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

    UpdateHoverCursor(ScreenToWorldDouble(event.GetPosition()));
}

void DiagramCanvas::OnMouseWheel(wxMouseEvent& event) {
    int rotation = event.GetWheelRotation();
    if (rotation == 0) {
        return;
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
    for (size_t index = 0; index < model_.edges().size(); ++index) {
        const auto& edge = model_.edges()[index];
        auto source_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.source_id; });
        auto target_it = std::find_if(model_.nodes().begin(), model_.nodes().end(),
                                      [&](const DiagramNode& node) { return node.id == edge.target_id; });
        if (source_it == model_.nodes().end() || target_it == model_.nodes().end()) {
            continue;
        }
        wxColour edge_color = wxColour(190, 190, 190);
        if (selected_edge_index_.has_value() && *selected_edge_index_ == index) {
            edge_color = SelectionColor();
        }
        wxPen pen(edge_color, 2);
        if (model_.type() == DiagramType::Erd && !edge.identifying) {
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
            if (model_.type() == DiagramType::Silverston && is_last) {
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
            DrawCardinalityMarker(dc, path.front(), start_dir, edge.source_cardinality);
            DrawCardinalityMarker(dc, path.back(), end_dir, edge.target_cardinality);
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
        wxRect2DDouble rect = WorldRectForNode(node);
        wxRect draw_rect(static_cast<int>(rect.m_x), static_cast<int>(rect.m_y),
                         static_cast<int>(rect.m_width), static_cast<int>(rect.m_height));

        int stack_layers = std::max(1, std::min(node.stack_count, 3));
        wxColour fill_color = node.ghosted ? GhostColor(base_fill) : base_fill;
        wxColour border_color = node.ghosted ? GhostColor(base_border) : base_border;
        wxBrush node_fill(fill_color);
        wxPen node_border(border_color, 1);
        for (int layer = stack_layers - 1; layer >= 0; --layer) {
            int offset = layer * 4;
            wxRect layer_rect(draw_rect);
            layer_rect.Offset(offset, offset);
            dc.SetBrush(node_fill);
            if (selected_index_.has_value() && *selected_index_ == i && layer == 0) {
                dc.SetPen(selection);
            } else {
                dc.SetPen(node_border);
            }
            dc.DrawRectangle(layer_rect);
        }

        dc.DrawText(node.name, draw_rect.GetX() + 8, draw_rect.GetY() + 6);
        dc.DrawText(node.type, draw_rect.GetX() + 8, draw_rect.GetY() + 26);

        if (model_.type() == DiagramType::Silverston) {
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
                wxString marker;
                if (attr.is_primary) {
                    marker = "PK ";
                } else if (attr.is_foreign) {
                    marker = "FK ";
                }
                wxString line = marker + attr.name + " : " + attr.data_type;
                dc.DrawText(line, draw_rect.GetX() + 8, row_y);
                row_y += 18;
            }
        }

        if (selected_index_.has_value() && *selected_index_ == i) {
            DrawSelectionHandles(dc, node);
        }
    }
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

} // namespace scratchrobin
