/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "draggable_toolbar.h"

#include "floating_frame.h"
#include "icon_bar_host.h"

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/sizer.h>

namespace scratchrobin {

// ============================================================================
// DraggableToolBar Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(DraggableToolBar, wxToolBar)
    EVT_LEFT_DOWN(DraggableToolBar::OnLeftDown)
    EVT_MOTION(DraggableToolBar::OnMouseMove)
    EVT_LEFT_UP(DraggableToolBar::OnLeftUp)
    EVT_LEFT_DCLICK(DraggableToolBar::OnLeftDClick)
    EVT_KEY_DOWN(DraggableToolBar::OnKeyDown)
    EVT_CONTEXT_MENU(DraggableToolBar::OnContextMenu)
wxEND_EVENT_TABLE()

DraggableToolBar::DraggableToolBar(wxWindow* parent, const wxString& name,
                                   wxWindowID id, long style)
    : wxToolBar(parent, id, wxDefaultPosition, wxDefaultSize, style),
      name_(name),
      orientation_((style & wxTB_VERTICAL) ? wxVERTICAL : wxHORIZONTAL),
      dock_parent_(parent) {
    
    // Enable drag image support
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

DraggableToolBar::~DraggableToolBar() {
    // Clean up drag visuals
    if (drag_ghost_) {
        drag_ghost_->Destroy();
        drag_ghost_ = nullptr;
    }
    if (drop_indicator_) {
        drop_indicator_->Destroy();
        drop_indicator_ = nullptr;
    }
    
    // Note: floating_frame_ is managed by IconBarHost
}

void DraggableToolBar::OnLeftDown(wxMouseEvent& event) {
    // Check if clicking in drag handle area (left edge or gripper)
    if (IsInDragHandleArea(event.GetPosition())) {
        drag_start_pos_ = event.GetPosition();
        is_dragging_ = true;
        drag_initiated_ = false;
        original_rect_ = GetScreenRect();
        
        // Capture mouse for drag tracking
        CaptureMouse();
    } else {
        event.Skip();
    }
}

void DraggableToolBar::OnMouseMove(wxMouseEvent& event) {
    if (!is_dragging_) {
        event.Skip();
        return;
    }
    
    wxPoint currentPos = event.GetPosition();
    
    // Check if we've moved past the drag threshold
    if (!drag_initiated_) {
        int dx = std::abs(currentPos.x - drag_start_pos_.x);
        int dy = std::abs(currentPos.y - drag_start_pos_.y);
        
        if (dx > DRAG_THRESHOLD || dy > DRAG_THRESHOLD) {
            drag_initiated_ = true;
            StartDrag(currentPos);
        }
    }
    
    if (drag_initiated_) {
        // Update drag position
        wxPoint screenPos = ClientToScreen(currentPos);
        drag_current_pos_ = screenPos;
        UpdateDragImage(screenPos);
        
        // Check for potential drop zones
        if (host_) {
            // Update drop indicator
            // This would be implemented by IconBarHost
        }
    }
}

void DraggableToolBar::OnLeftUp(wxMouseEvent& event) {
    if (is_dragging_) {
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        if (drag_initiated_) {
            // Determine if we should float or dock
            bool shouldFloat = true;
            
            if (host_) {
                // Check if we're over a valid drop zone
                // shouldFloat = !host_->IsOverDropZone(event.GetPosition());
            }
            
            EndDrag(shouldFloat);
        }
        
        is_dragging_ = false;
        drag_initiated_ = false;
    } else {
        event.Skip();
    }
}

void DraggableToolBar::OnLeftDClick(wxMouseEvent& event) {
    // Double-click toggles floating state
    if (IsFloating()) {
        Dock();
    } else {
        Float();
    }
}

void DraggableToolBar::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_ESCAPE && is_dragging_) {
        CancelDrag();
    }
    event.Skip();
}

void DraggableToolBar::OnContextMenu(wxContextMenuEvent& event) {
    wxPoint pos = event.GetPosition();
    if (pos == wxDefaultPosition) {
        pos = wxPoint(0, 0);
    } else {
        pos = ScreenToClient(pos);
    }
    
    wxMenu menu;
    
    if (IsFloating()) {
        menu.Append(wxID_ANY, "&Dock", "Dock the toolbar");
    } else {
        menu.Append(wxID_ANY, "&Float", "Float the toolbar");
    }
    
    menu.AppendSeparator();
    menu.Append(wxID_ANY, 
                orientation_ == wxHORIZONTAL ? "&Vertical" : "&Horizontal",
                "Change orientation");
    
    menu.AppendSeparator();
    menu.Append(wxID_ANY, "&Customize...", "Customize toolbar");
    
    PopupMenu(&menu, pos);
}

void DraggableToolBar::OnMenuToggleFloat(wxCommandEvent& /*event*/) {
    if (IsFloating()) {
        Dock();
    } else {
        Float();
    }
}

void DraggableToolBar::OnMenuToggleOrientation(wxCommandEvent& /*event*/) {
    ToggleOrientation();
}

void DraggableToolBar::OnMenuCustomize(wxCommandEvent& /*event*/) {
    // Would open toolbar customization dialog
}

void DraggableToolBar::Float(const wxPoint& pos) {
    if (IsFloating()) {
        return;
    }
    
    // Remember dock position
    dock_parent_ = GetParent();
    
    // Create floating frame
    floating_frame_ = new FloatingToolBarFrame(nullptr, name_);
    
    // Reparent to floating frame
    Reparent(floating_frame_);
    
    // Calculate position
    wxPoint floatPos = pos;
    if (floatPos == wxDefaultPosition) {
        if (last_float_position_ != wxDefaultPosition) {
            floatPos = last_float_position_;
        } else {
            // Position near the original location
            floatPos = GetScreenPosition() + wxPoint(50, 50);
        }
    }
    
    floating_frame_->SetToolBar(this);
    floating_frame_->SetPosition(floatPos);
    floating_frame_->Show(true);
    
    // Notify host
    if (host_) {
        host_->OnToolBarFloated(this);
    }
    
    Realize();
}

void DraggableToolBar::Dock(wxWindow* parent) {
    if (!IsFloating()) {
        return;
    }
    
    // Remember float position for next time
    if (floating_frame_) {
        last_float_position_ = floating_frame_->GetPosition();
    }
    
    // Determine parent to dock to
    wxWindow* dockTo = parent ? parent : dock_parent_;
    if (!dockTo) {
        return;
    }
    
    // Clear from floating frame
    if (floating_frame_) {
        floating_frame_->ClearToolBar();
        floating_frame_->Destroy();
        floating_frame_ = nullptr;
    }
    
    // Reparent back to dock
    Reparent(dockTo);
    
    // Notify host
    if (host_) {
        host_->OnToolBarDocked(this);
    }
    
    Realize();
}

void DraggableToolBar::SetOrientation(wxOrientation orient) {
    if (orientation_ == orient) {
        return;
    }
    
    orientation_ = orient;
    
    // Update window style
    long style = GetWindowStyle();
    style &= ~(wxTB_HORIZONTAL | wxTB_VERTICAL);
    style |= (orient == wxVERTICAL) ? wxTB_VERTICAL : wxTB_HORIZONTAL;
    SetWindowStyle(style);
    
    // Refresh
    Realize();
    Refresh();
}

void DraggableToolBar::ToggleOrientation() {
    SetOrientation(orientation_ == wxHORIZONTAL ? wxVERTICAL : wxHORIZONTAL);
}

void DraggableToolBar::StartDrag(const wxPoint& pos) {
    // Create drag ghost image
    wxBitmap dragImage = CreateDragImage();
    
    if (dragImage.IsOk()) {
        drag_ghost_ = new DragGhostWindow(wxGetTopLevelParent(this), dragImage);
        drag_ghost_->Show();
    }
    
    // Hide the actual toolbar temporarily
    if (!IsFloating()) {
        Hide();
    }
}

void DraggableToolBar::EndDrag(bool do_float) {
    // Clean up drag visuals
    if (drag_ghost_) {
        drag_ghost_->Destroy();
        drag_ghost_ = nullptr;
    }
    if (drop_indicator_) {
        drop_indicator_->Destroy();
        drop_indicator_ = nullptr;
    }
    
    if (do_float && !IsFloating()) {
        // Float at current position
        Float(drag_current_pos_);
    } else if (!do_float && IsFloating()) {
        // Dock back
        Dock();
    } else {
        // Show the toolbar again
        Show();
    }
}

void DraggableToolBar::CancelDrag() {
    if (drag_ghost_) {
        drag_ghost_->Destroy();
        drag_ghost_ = nullptr;
    }
    if (drop_indicator_) {
        drop_indicator_->Destroy();
        drop_indicator_ = nullptr;
    }
    
    // Show the toolbar again
    Show();
    
    is_dragging_ = false;
    drag_initiated_ = false;
    
    if (HasCapture()) {
        ReleaseMouse();
    }
}

void DraggableToolBar::UpdateDragImage(const wxPoint& screenPos) {
    if (drag_ghost_) {
        wxPoint ghostPos = screenPos - wxPoint(drag_start_pos_.x, drag_start_pos_.y);
        drag_ghost_->UpdatePosition(ghostPos);
    }
}

wxBitmap DraggableToolBar::CreateDragImage() {
    wxSize size = GetSize();
    wxBitmap bitmap(size.x, size.y);
    
    wxMemoryDC dc(bitmap);
    dc.SetBackground(wxBrush(wxColour(200, 200, 200, 128)));
    dc.Clear();
    
    // Draw a representation of the toolbar
    dc.SetPen(wxPen(wxColour(100, 100, 100)));
    dc.SetBrush(wxBrush(wxColour(220, 220, 220, 180)));
    dc.DrawRectangle(0, 0, size.x, size.y);
    
    // Draw tool icons (simplified)
    dc.SetBrush(wxBrush(wxColour(150, 150, 150)));
    int toolCount = GetToolsCount();
    for (int i = 0; i < std::min(toolCount, 5); ++i) {
        int x = (orientation_ == wxHORIZONTAL) ? 5 + i * 30 : 5;
        int y = (orientation_ == wxHORIZONTAL) ? 5 : 5 + i * 30;
        dc.DrawRectangle(x, y, 24, 24);
    }
    
    dc.SelectObject(wxNullBitmap);
    
    return bitmap;
}

bool DraggableToolBar::IsInDragHandleArea(const wxPoint& pos) const {
    // Check if position is in the left edge area (drag handle)
    // The drag handle is typically the leftmost 8-12 pixels
    return pos.x < 12 && pos.y > 0 && pos.y < GetSize().y;
}

DraggableToolBar::State DraggableToolBar::GetState() const {
    State state;
    state.is_floating = IsFloating();
    state.orientation = orientation_;
    state.visible = IsShown();
    
    if (IsFloating() && floating_frame_) {
        state.float_position = floating_frame_->GetPosition();
        state.float_size = floating_frame_->GetSize();
    } else {
        state.float_position = last_float_position_;
    }
    
    return state;
}

void DraggableToolBar::RestoreState(const State& state) {
    if (state.orientation != orientation_) {
        SetOrientation(state.orientation);
    }
    
    if (state.is_floating && !IsFloating()) {
        Float(state.float_position);
        if (floating_frame_ && state.float_size != wxDefaultSize) {
            floating_frame_->SetSize(state.float_size);
        }
    } else if (!state.is_floating && IsFloating()) {
        Dock();
    }
    
    Show(state.visible);
}

// ============================================================================
// ToolBarDragData Implementation
// ============================================================================
const wxDataFormat& ToolBarDragData::GetFormat() {
    static wxDataFormat format("application/x-scratchrobin-toolbar");
    return format;
}

ToolBarDragData::ToolBarDragData()
    : wxDataObjectSimple(GetFormat()) {}

ToolBarDragData::ToolBarDragData(const wxString& barName, wxOrientation orient)
    : wxDataObjectSimple(GetFormat()),
      bar_name_(barName),
      orientation_(orient) {}

size_t ToolBarDragData::GetDataSize() const {
    // Format: name_length|name|orientation
    return sizeof(int) + bar_name_.Len() + sizeof(int);
}

bool ToolBarDragData::GetDataHere(void* buf) const {
    char* ptr = static_cast<char*>(buf);
    
    // Write name length and name
    int nameLen = static_cast<int>(bar_name_.Len());
    memcpy(ptr, &nameLen, sizeof(int));
    ptr += sizeof(int);
    memcpy(ptr, bar_name_.c_str(), nameLen);
    ptr += nameLen;
    
    // Write orientation
    int orient = static_cast<int>(orientation_);
    memcpy(ptr, &orient, sizeof(int));
    
    return true;
}

bool ToolBarDragData::SetData(size_t len, const void* buf) {
    const char* ptr = static_cast<const char*>(buf);
    const char* end = ptr + len;
    
    // Read name length
    if (ptr + sizeof(int) > end) return false;
    int nameLen;
    memcpy(&nameLen, ptr, sizeof(int));
    ptr += sizeof(int);
    
    // Read name
    if (ptr + nameLen > end) return false;
    bar_name_ = wxString(ptr, nameLen);
    ptr += nameLen;
    
    // Read orientation
    if (ptr + sizeof(int) > end) return false;
    int orient;
    memcpy(&orient, ptr, sizeof(int));
    orientation_ = static_cast<wxOrientation>(orient);
    
    return true;
}

} // namespace scratchrobin
