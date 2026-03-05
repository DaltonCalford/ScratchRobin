/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/docking_overlay.h"
#include <wx/dcclient.h>
#include <wx/dcgraph.h>

namespace scratchrobin::ui {

// DockingOverlay implementation
DockingOverlay::DockingOverlay(wxWindow* parent)
    : parent_(parent) {
}

DockingOverlay::~DockingOverlay() {
}

void DockingOverlay::show(const wxRect& target_rect, DockCapability allowed_zones) {
    target_rect_ = target_rect;
    allowed_zones_ = allowed_zones;
    is_visible_ = true;
    calculateZones();
}

void DockingOverlay::hide() {
    is_visible_ = false;
}

void DockingOverlay::setHoverZone(int zone) {
    if (hover_zone_ != zone) {
        hover_zone_ = zone;
        // Redraw
    }
}

int DockingOverlay::hitTest(const wxPoint& pos) const {
    if (left_zone_.Contains(pos)) return wxLEFT;
    if (right_zone_.Contains(pos)) return wxRIGHT;
    if (top_zone_.Contains(pos)) return wxTOP;
    if (bottom_zone_.Contains(pos)) return wxBOTTOM;
    if (center_zone_.Contains(pos)) return -1;  // Center/tab
    return -1;
}

void DockingOverlay::calculateZones() {
    int zone_width = target_rect_.width / 5;
    int zone_height = target_rect_.height / 5;
    
    // Center zone (for tabbing)
    center_zone_ = wxRect(
        target_rect_.x + zone_width,
        target_rect_.y + zone_height,
        target_rect_.width - 2 * zone_width,
        target_rect_.height - 2 * zone_height
    );
    
    // Edge zones
    left_zone_ = wxRect(target_rect_.x, target_rect_.y, zone_width, target_rect_.height);
    right_zone_ = wxRect(target_rect_.x + target_rect_.width - zone_width, 
                         target_rect_.y, zone_width, target_rect_.height);
    top_zone_ = wxRect(target_rect_.x, target_rect_.y, target_rect_.width, zone_height);
    bottom_zone_ = wxRect(target_rect_.x, target_rect_.y + target_rect_.height - zone_height,
                          target_rect_.width, zone_height);
}

void DockingOverlay::paint(wxDC& dc) {
    if (!is_visible_) return;
    
    // Draw semi-transparent overlay
    dc.SetBrush(wxBrush(wxColour(0, 0, 255, 50)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    
    // Draw active zone highlighted
    wxRect active_rect;
    switch (hover_zone_) {
        case wxLEFT: active_rect = left_zone_; break;
        case wxRIGHT: active_rect = right_zone_; break;
        case wxTOP: active_rect = top_zone_; break;
        case wxBOTTOM: active_rect = bottom_zone_; break;
        case wxCENTER: active_rect = center_zone_; break;
        default: break;
    }
    
    if (!active_rect.IsEmpty()) {
        dc.SetBrush(wxBrush(wxColour(0, 120, 255, 100)));
        dc.DrawRectangle(active_rect);
    }
    
    // Draw zone indicators
    dc.SetPen(wxPen(wxColour(255, 255, 255, 150), 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    
    if (hasCapability(allowed_zones_, DockCapability::kLeft)) {
        dc.DrawRectangle(left_zone_);
    }
    if (hasCapability(allowed_zones_, DockCapability::kRight)) {
        dc.DrawRectangle(right_zone_);
    }
    if (hasCapability(allowed_zones_, DockCapability::kTop)) {
        dc.DrawRectangle(top_zone_);
    }
    if (hasCapability(allowed_zones_, DockCapability::kBottom)) {
        dc.DrawRectangle(bottom_zone_);
    }
}

void DockingOverlay::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(parent_);
    paint(dc);
}



// DropZoneWindow implementation
DropZoneWindow::DropZoneWindow(wxWindow* parent)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
               wxTRANSPARENT_WINDOW) {
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
}

void DropZoneWindow::showZones(const wxRect& rect, DockCapability zones) {
    target_rect_ = rect;
    allowed_zones_ = zones;
    Show();
    Refresh();
}

void DropZoneWindow::setActiveZone(int zone) {
    active_zone_ = zone;
    Refresh();
}

void DropZoneWindow::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    // Clear background
    dc.Clear();
    
    // Draw zones
    drawZone(dc, wxRect(0, 0, GetSize().x / 3, GetSize().y), wxLEFT, 
             active_zone_ == wxLEFT);
}

void DropZoneWindow::drawZone(wxDC& dc, const wxRect& rect, int zone, bool active) {
    if (active) {
        dc.SetBrush(wxBrush(wxColour(0, 120, 255, 150)));
    } else {
        dc.SetBrush(wxBrush(wxColour(200, 200, 200, 100)));
    }
    dc.SetPen(wxPen(wxColour(255, 255, 255), 2));
    dc.DrawRectangle(rect);
}

BEGIN_EVENT_TABLE(DropZoneWindow, wxWindow)
    EVT_PAINT(DropZoneWindow::onPaint)
END_EVENT_TABLE()

// DragFeedbackWindow implementation
DragFeedbackWindow::DragFeedbackWindow(wxWindow* parent, const wxBitmap& content_preview)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, content_preview.GetSize(),
               wxTRANSPARENT_WINDOW)
    , preview_(content_preview) {
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
}

void DragFeedbackWindow::setPosition(const wxPoint& pos) {
    SetPosition(pos);
}

void DragFeedbackWindow::setOpacity(int opacity) {
    opacity_ = opacity;
    Refresh();
}

void DragFeedbackWindow::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    if (preview_.IsOk()) {
        // Draw semi-transparent preview
        dc.DrawBitmap(preview_, 0, 0, true);
        
        // Draw border
        dc.SetPen(wxPen(wxColour(0, 120, 255, opacity_), 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(GetClientRect());
    }
}

BEGIN_EVENT_TABLE(DragFeedbackWindow, wxWindow)
    EVT_PAINT(DragFeedbackWindow::onPaint)
END_EVENT_TABLE()

}  // namespace scratchrobin::ui
