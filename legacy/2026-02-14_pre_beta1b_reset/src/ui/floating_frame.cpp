/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "floating_frame.h"

#include "dockable_form.h"
#include "main_frame.h"

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/sizer.h>

namespace scratchrobin {

// ============================================================================
// FloatingFrame Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(FloatingFrame, wxFrame)
    EVT_ACTIVATE(FloatingFrame::OnActivate)
    EVT_CLOSE(FloatingFrame::OnClose)
    EVT_LEFT_DCLICK(FloatingFrame::OnTitleBarLeftDClick)
    EVT_LEFT_DOWN(FloatingFrame::OnTitleBarLeftDown)
    EVT_MOTION(FloatingFrame::OnTitleBarMouseMove)
    EVT_LEFT_UP(FloatingFrame::OnTitleBarLeftUp)
    EVT_KEY_DOWN(FloatingFrame::OnKeyDown)
wxEND_EVENT_TABLE()

FloatingFrame::FloatingFrame(wxWindow* parent, const wxString& title)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600),
              wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR),
      main_frame_(dynamic_cast<MainFrame*>(parent)) {
    
    // Set a distinctive appearance for floating frames
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
    
    // Center on parent initially
    if (parent) {
        CenterOnParent();
    }
}

FloatingFrame::~FloatingFrame() {
    // Content is owned by document manager, don't delete it here
    if (content_) {
        content_->SetTabWindow(nullptr);
    }
}

void FloatingFrame::SetContent(DockableForm* form) {
    if (!form) {
        return;
    }
    
    content_ = form;
    
    // Reparent the form to this frame
    form->Reparent(this);
    
    // Create a sizer to fill the frame
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(form, 1, wxEXPAND);
    SetSizer(sizer);
    
    // Update the frame title
    SetTitle(form->GetDocumentTitleWx());
    
    // Refresh layout
    Layout();
}

void FloatingFrame::ClearContent() {
    if (content_) {
        // Remove from sizer without destroying
        auto* sizer = GetSizer();
        if (sizer) {
            sizer->Detach(content_);
        }
        content_ = nullptr;
    }
}

void FloatingFrame::DockToMain() {
    if (!main_frame_ || !content_) {
        return;
    }
    
    // Save current state before docking
    pre_dock_position_ = GetPosition();
    pre_dock_size_ = GetSize();
    was_maximized_ = IsMaximized();
    was_iconized_ = IsIconized();
    
    // Signal that we're docking
    is_docked_ = true;
    
    // Close this floating frame (content will be transferred)
    Close(true);
}

void FloatingFrame::RestorePreDockState() {
    if (was_maximized_) {
        Maximize(true);
    } else if (was_iconized_) {
        Iconize(true);
    }
    
    if (!was_maximized_ && !was_iconized_) {
        SetPosition(pre_dock_position_);
        SetSize(pre_dock_size_);
    }
}

void FloatingFrame::OnActivate(wxActivateEvent& event) {
    if (event.GetActive() && content_) {
        content_->OnActivate();
    } else if (!event.GetActive() && content_) {
        content_->OnDeactivate();
    }
    event.Skip();
}

void FloatingFrame::OnClose(wxCloseEvent& event) {
    if (content_) {
        // Check if content can close
        if (!content_->CanClose()) {
            event.Veto();
            return;
        }
        
        content_->OnClosing();
        
        // If we're not docking, the content needs to go back somewhere
        if (!is_docked_ && main_frame_) {
            // Content will be orphaned - signal to document manager
            // This is handled by the IconBarHost
        }
    }
    
    event.Skip();
}

void FloatingFrame::OnTitleBarLeftDClick(wxMouseEvent& event) {
    // Double-click on title bar docks the window
    DockToMain();
}

void FloatingFrame::OnTitleBarLeftDown(wxMouseEvent& event) {
    ctrl_pressed_on_down_ = wxGetKeyState(WXK_CONTROL);
    is_dragging_ = true;
    drag_start_pos_ = event.GetPosition();
    CaptureMouse();
}

void FloatingFrame::OnTitleBarMouseMove(wxMouseEvent& event) {
    if (!is_dragging_) {
        event.Skip();
        return;
    }
    
    // Check if Ctrl is still held
    bool ctrl_held = wxGetKeyState(WXK_CONTROL);
    
    if (ctrl_pressed_on_down_ && ctrl_held) {
        // Ctrl+Click drag - dock on release
        // Visual feedback could be shown here
    }
    
    event.Skip();
}

void FloatingFrame::OnTitleBarLeftUp(wxMouseEvent& event) {
    if (is_dragging_) {
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        // If Ctrl was held during the entire drag, dock
        if (ctrl_pressed_on_down_ && wxGetKeyState(WXK_CONTROL)) {
            DockToMain();
        }
        
        is_dragging_ = false;
        ctrl_pressed_on_down_ = false;
    }
    event.Skip();
}

void FloatingFrame::OnKeyDown(wxKeyEvent& event) {
    // Escape key cancels any pending operation
    if (event.GetKeyCode() == WXK_ESCAPE) {
        if (is_dragging_) {
            is_dragging_ = false;
            if (HasCapture()) {
                ReleaseMouse();
            }
        }
    }
    event.Skip();
}

// ============================================================================
// FloatingToolBarFrame Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(FloatingToolBarFrame, wxFrame)
    EVT_SIZE(FloatingToolBarFrame::OnResize)
    EVT_CLOSE(FloatingToolBarFrame::OnClose)
    EVT_ACTIVATE(FloatingToolBarFrame::OnActivate)
wxEND_EVENT_TABLE()

FloatingToolBarFrame::FloatingToolBarFrame(wxWindow* parent, const wxString& title)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE | wxFRAME_TOOL_WINDOW | wxFRAME_NO_TASKBAR) {
    
    // Small title bar for toolbar frame
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
}

FloatingToolBarFrame::~FloatingToolBarFrame() {
    // Toolbar is owned by IconBarHost, don't delete it here
    toolbar_ = nullptr;
}

void FloatingToolBarFrame::SetToolBar(wxToolBar* toolbar) {
    if (!toolbar) {
        return;
    }
    
    toolbar_ = toolbar;
    toolbar_orientation_ = toolbar->GetWindowStyle() & wxTB_VERTICAL 
                           ? wxVERTICAL : wxHORIZONTAL;
    
    // Reparent the toolbar to this frame
    toolbar->Reparent(this);
    
    // Create layout
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar, 0, wxEXPAND);
    SetSizer(sizer);
    
    // Fit to toolbar
    FitToToolBar();
}

void FloatingToolBarFrame::ClearToolBar() {
    if (toolbar_) {
        auto* sizer = GetSizer();
        if (sizer) {
            sizer->Detach(toolbar_);
        }
        toolbar_ = nullptr;
    }
}

void FloatingToolBarFrame::FitToToolBar() {
    if (!toolbar_) {
        return;
    }
    
    wxSize toolbarSize = toolbar_->GetBestSize();
    
    // Add some padding for the frame border
    wxSize frameSize = toolbarSize;
    frameSize.x += 8;  // Border padding
    frameSize.y += 30; // Title bar + border
    
    SetClientSize(toolbarSize);
    SetMinSize(frameSize);
    
    min_toolbar_size_ = toolbarSize;
}

void FloatingToolBarFrame::RecalculateLayout() {
    if (!toolbar_) {
        return;
    }
    
    wxSize clientSize = GetClientSize();
    wxSize toolSize = toolbar_->GetToolSize();
    int toolCount = toolbar_->GetToolsCount();
    
    if (toolCount == 0) {
        return;
    }
    
    // Calculate how many tools fit per row/column
    int toolsPerRow = 1;
    int toolsPerCol = 1;
    
    if (toolbar_orientation_ == wxHORIZONTAL) {
        toolsPerRow = std::max(1, clientSize.x / (toolSize.x + 8));
        toolsPerCol = (toolCount + toolsPerRow - 1) / toolsPerRow;
        rows_ = toolsPerCol;
        columns_ = toolsPerRow;
    } else {
        toolsPerCol = std::max(1, clientSize.y / (toolSize.y + 8));
        toolsPerRow = (toolCount + toolsPerCol - 1) / toolsPerCol;
        rows_ = toolsPerCol;
        columns_ = toolsPerRow;
    }
    
    // Refresh the toolbar layout
    toolbar_->Refresh();
}

void FloatingToolBarFrame::OnResize(wxSizeEvent& event) {
    RecalculateLayout();
    event.Skip();
}

void FloatingToolBarFrame::OnClose(wxCloseEvent& event) {
    // When closing, we need to dock the toolbar back
    // This is handled by IconBarHost
    event.Skip();
}

void FloatingToolBarFrame::OnActivate(wxActivateEvent& event) {
    // Could highlight the toolbar when active
    event.Skip();
}

// ============================================================================
// DragGhostWindow Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(DragGhostWindow, wxFrame)
    EVT_PAINT(DragGhostWindow::OnPaint)
wxEND_EVENT_TABLE()

DragGhostWindow::DragGhostWindow(wxWindow* parent, const wxBitmap& bitmap)
    : wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, bitmap.GetSize(),
              wxFRAME_NO_TASKBAR | wxFRAME_SHAPED | wxNO_BORDER),
      ghost_bitmap_(bitmap) {
    
    SetTransparent(180);  // Semi-transparent
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
}

DragGhostWindow::~DragGhostWindow() = default;

void DragGhostWindow::UpdatePosition(const wxPoint& pos) {
    SetPosition(pos);
}

void DragGhostWindow::SetBitmap(const wxBitmap& bitmap) {
    ghost_bitmap_ = bitmap;
    SetSize(bitmap.GetSize());
    Refresh();
}

void DragGhostWindow::OnPaint(wxPaintEvent& /*event*/) {
    wxPaintDC dc(this);
    
    if (ghost_bitmap_.IsOk()) {
        dc.DrawBitmap(ghost_bitmap_, 0, 0, true);
    }
}

// ============================================================================
// DropZoneIndicator Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(DropZoneIndicator, wxFrame)
    EVT_PAINT(DropZoneIndicator::OnPaint)
wxEND_EVENT_TABLE()

DropZoneIndicator::DropZoneIndicator(wxWindow* parent, const wxRect& rect)
    : wxFrame(parent, wxID_ANY, wxEmptyString, rect.GetPosition(), rect.GetSize(),
              wxFRAME_NO_TASKBAR | wxFRAME_SHAPED | wxNO_BORDER),
      target_rect_(wxSize(rect.GetWidth(), rect.GetHeight())) {
    
    SetTransparent(100);
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
}

DropZoneIndicator::~DropZoneIndicator() = default;

void DropZoneIndicator::SetTargetRect(const wxRect& rect) {
    SetPosition(rect.GetPosition());
    SetSize(rect.GetSize());
    target_rect_ = wxSize(rect.GetWidth(), rect.GetHeight());
    Refresh();
}

void DropZoneIndicator::OnPaint(wxPaintEvent& /*event*/) {
    wxPaintDC dc(this);
    
    wxSize size = GetClientSize();
    
    // Draw a blue dashed outline
    dc.SetBrush(wxBrush(wxColour(100, 150, 255, 50)));
    dc.SetPen(wxPen(wxColour(100, 150, 255), 2, wxPENSTYLE_SHORT_DASH));
    dc.DrawRectangle(0, 0, size.x, size.y);
}

} // namespace scratchrobin
