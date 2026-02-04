/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_minimap.h"

#include "diagram_canvas.h"
#include "diagram_model.h"

#include <algorithm>
#include <cmath>

#include <wx/dcclient.h>
#include <wx/dcbuffer.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(DiagramMinimap, wxPanel)
    EVT_PAINT(DiagramMinimap::OnPaint)
    EVT_SIZE(DiagramMinimap::OnSize)
    EVT_LEFT_DOWN(DiagramMinimap::OnLeftDown)
    EVT_MOTION(DiagramMinimap::OnMotion)
    EVT_LEFT_UP(DiagramMinimap::OnLeftUp)
wxEND_EVENT_TABLE()

DiagramMinimap::DiagramMinimap(wxWindow* parent, DiagramCanvas* canvas)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(200, 150)),
      canvas_(canvas) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void DiagramMinimap::SetCanvas(DiagramCanvas* canvas) {
    canvas_ = canvas;
    UpdateView();
}

void DiagramMinimap::UpdateView() {
    if (!canvas_) return;
    
    // Get diagram bounds
    GetDiagramBounds(min_x_, min_y_, max_x_, max_y_);
    
    // Add padding
    double padding = 50.0;
    min_x_ -= padding;
    min_y_ -= padding;
    max_x_ += padding;
    max_y_ += padding;
    
    // Calculate scale to fit diagram in minimap
    wxSize size = GetClientSize();
    double diagramWidth = max_x_ - min_x_;
    double diagramHeight = max_y_ - min_y_;
    
    if (diagramWidth > 0 && diagramHeight > 0) {
        double scaleX = (size.GetWidth() - 10) / diagramWidth;
        double scaleY = (size.GetHeight() - 10) / diagramHeight;
        scale_ = std::min(scaleX, scaleY);
    }
    
    // Center the diagram
    double scaledWidth = diagramWidth * scale_;
    double scaledHeight = diagramHeight * scale_;
    offset_x_ = static_cast<int>((size.GetWidth() - scaledWidth) / 2 - min_x_ * scale_);
    offset_y_ = static_cast<int>((size.GetHeight() - scaledHeight) / 2 - min_y_ * scale_);
    
    Refresh();
}

void DiagramMinimap::GetDiagramBounds(double& minX, double& minY, double& maxX, double& maxY) const {
    minX = minY = 0;
    maxX = maxY = 100;  // Default size
    
    if (!canvas_) return;
    
    const auto& nodes = canvas_->model().nodes();
    if (nodes.empty()) return;
    
    minX = maxX = nodes[0].x;
    minY = maxY = nodes[0].y;
    
    for (const auto& node : nodes) {
        minX = std::min(minX, node.x);
        minY = std::min(minY, node.y);
        maxX = std::max(maxX, node.x + node.width);
        maxY = std::max(maxY, node.y + node.height);
    }
}

wxRect DiagramMinimap::CalculateViewportRect() const {
    if (!canvas_) return wxRect();
    
    // Get canvas viewport in world coordinates
    // This is a simplified version - canvas doesn't expose viewport directly
    // We'll estimate based on canvas size and pan offset
    wxSize canvasSize = canvas_->GetClientSize();
    
    // Estimate viewport (this is simplified)
    double viewX = -offset_x_ / scale_;  // Approximate
    double viewY = -offset_y_ / scale_;
    double viewW = canvasSize.GetWidth() / scale_;
    double viewH = canvasSize.GetHeight() / scale_;
    
    // Convert to minimap coordinates
    int mx = static_cast<int>(viewX * scale_) + offset_x_;
    int my = static_cast<int>(viewY * scale_) + offset_y_;
    int mw = static_cast<int>(viewW * scale_);
    int mh = static_cast<int>(viewH * scale_);
    
    return wxRect(mx, my, mw, mh);
}

wxPoint DiagramMinimap::MinimapToCanvas(int x, int y) const {
    double worldX = (x - offset_x_) / scale_;
    double worldY = (y - offset_y_) / scale_;
    return wxPoint(static_cast<int>(worldX), static_cast<int>(worldY));
}

void DiagramMinimap::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    
    wxSize size = GetClientSize();
    
    // Background
    dc.SetBrush(wxBrush(wxColour(45, 45, 45)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
    
    if (!canvas_) return;
    
    const auto& model = canvas_->model();
    const auto& nodes = model.nodes();
    
    // Draw grid
    dc.SetPen(wxPen(wxColour(55, 55, 55), 1));
    for (int x = 0; x < size.GetWidth(); x += 20) {
        dc.DrawLine(x, 0, x, size.GetHeight());
    }
    for (int y = 0; y < size.GetHeight(); y += 20) {
        dc.DrawLine(0, y, size.GetWidth(), y);
    }
    
    // Draw edges
    dc.SetPen(wxPen(wxColour(150, 150, 150), 1));
    for (const auto& edge : model.edges()) {
        auto sourceIt = std::find_if(nodes.begin(), nodes.end(),
            [&edge](const DiagramNode& n) { return n.id == edge.source_id; });
        auto targetIt = std::find_if(nodes.begin(), nodes.end(),
            [&edge](const DiagramNode& n) { return n.id == edge.target_id; });
        
        if (sourceIt != nodes.end() && targetIt != nodes.end()) {
            int x1 = static_cast<int>(sourceIt->x * scale_) + offset_x_;
            int y1 = static_cast<int>(sourceIt->y * scale_) + offset_y_;
            int x2 = static_cast<int>(targetIt->x * scale_) + offset_x_;
            int y2 = static_cast<int>(targetIt->y * scale_) + offset_y_;
            dc.DrawLine(x1, y1, x2, y2);
        }
    }
    
    // Draw nodes
    dc.SetBrush(wxBrush(wxColour(80, 88, 110)));
    dc.SetPen(wxPen(wxColour(160, 160, 180), 1));
    for (const auto& node : nodes) {
        int x = static_cast<int>(node.x * scale_) + offset_x_;
        int y = static_cast<int>(node.y * scale_) + offset_y_;
        int w = static_cast<int>(std::max(4.0, node.width * scale_));
        int h = static_cast<int>(std::max(3.0, node.height * scale_));
        dc.DrawRectangle(x, y, w, h);
    }
    
    // Draw viewport rectangle
    wxRect viewport = CalculateViewportRect();
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(0, 150, 255), 2));
    dc.DrawRectangle(viewport);
    
    // Draw border
    dc.SetPen(wxPen(wxColour(100, 100, 100), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
}

void DiagramMinimap::OnSize(wxSizeEvent& event) {
    UpdateView();
    event.Skip();
}

void DiagramMinimap::OnLeftDown(wxMouseEvent& event) {
    is_dragging_ = true;
    drag_start_ = event.GetPosition();
    CaptureMouse();
    
    // Jump to clicked position
    wxPoint worldPos = MinimapToCanvas(drag_start_.x, drag_start_.y);
    // Center canvas on this point (would need to add pan control to canvas)
}

void DiagramMinimap::OnMotion(wxMouseEvent& event) {
    if (!is_dragging_) return;
    
    wxPoint pos = event.GetPosition();
    wxPoint worldPos = MinimapToCanvas(pos.x, pos.y);
    
    // Pan canvas to follow drag
    // This would require adding pan control methods to DiagramCanvas
}

void DiagramMinimap::OnLeftUp(wxMouseEvent&) {
    if (is_dragging_) {
        is_dragging_ = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
}

} // namespace scratchrobin
