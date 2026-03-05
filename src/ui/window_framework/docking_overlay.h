/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/overlay.h>
#include <wx/dcclient.h>
#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// Forward declarations
class DockableWindow;
class WindowManager;

// Drop zone indicator - visual feedback during drag
class DockingOverlay {
public:
    DockingOverlay(wxWindow* parent);
    ~DockingOverlay();
    
    // Show/hide overlay
    void show(const wxRect& target_rect, DockCapability allowed_zones);
    void hide();
    bool isVisible() const { return is_visible_; }
    
    // Update hover state
    void setHoverZone(int zone);
    int getHoverZone() const { return hover_zone_; }
    
    // Hit test - which zone is at this point?
    int hitTest(const wxPoint& pos) const;

private:
    wxWindow* parent_{nullptr};
    wxOverlay overlay_;
    bool is_visible_{false};
    wxRect target_rect_;
    DockCapability allowed_zones_;
    int hover_zone_{-1};  // wxLEFT, wxRIGHT, wxTOP, wxBOTTOM, or -1 for center
    
    // Zone rectangles
    wxRect left_zone_;
    wxRect right_zone_;
    wxRect top_zone_;
    wxRect bottom_zone_;
    wxRect center_zone_;
    
    void calculateZones();
    void paint(wxDC& dc);
    void onPaint(wxPaintEvent& event);
    
};

// Drop zone indicator window - transparent overlay
class DropZoneWindow : public wxWindow {
public:
    DropZoneWindow(wxWindow* parent);
    
    void showZones(const wxRect& rect, DockCapability zones);
    void setActiveZone(int zone);
    
private:
    DockCapability allowed_zones_{};
    int active_zone_{-1};
    wxRect target_rect_;
    
    void onPaint(wxPaintEvent& event);
    void drawZone(wxDC& dc, const wxRect& rect, int zone, bool active);
    
    DECLARE_EVENT_TABLE()
};

// Drag feedback window - follows cursor during drag
class DragFeedbackWindow : public wxWindow {
public:
    DragFeedbackWindow(wxWindow* parent, const wxBitmap& content_preview);
    
    void setPosition(const wxPoint& pos);
    void setOpacity(int opacity);  // 0-255
    
private:
    wxBitmap preview_;
    int opacity_{200};
    
    void onPaint(wxPaintEvent& event);
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
