/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_FLOATING_FRAME_H
#define SCRATCHROBIN_FLOATING_FRAME_H

#include <wx/frame.h>
#include <wx/panel.h>

namespace scratchrobin {

// Forward declaration
class DockableForm;
class MainFrame;

// ============================================================================
// Floating Frame - Hosts dockable forms that have been undocked
// ============================================================================
class FloatingFrame : public wxFrame {
public:
    FloatingFrame(wxWindow* parent, const wxString& title);
    ~FloatingFrame() override;

    // Content management
    void SetContent(DockableForm* form);
    DockableForm* GetContent() const { return content_; }
    void ClearContent();

    // Docking operations
    void DockToMain();
    bool IsDocked() const { return is_docked_; }

    // Restore state before dock
    void RestorePreDockState();

protected:
    // Event handlers
    void OnActivate(wxActivateEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnTitleBarLeftDClick(wxMouseEvent& event);
    void OnTitleBarLeftDown(wxMouseEvent& event);
    void OnTitleBarMouseMove(wxMouseEvent& event);
    void OnTitleBarLeftUp(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

private:
    DockableForm* content_ = nullptr;
    MainFrame* main_frame_ = nullptr;
    bool is_docked_ = false;
    
    // Pre-dock state for restoration
    wxPoint pre_dock_position_;
    wxSize pre_dock_size_;
    bool was_maximized_ = false;
    bool was_iconized_ = false;
    
    // Drag state for Ctrl+Click docking
    bool is_dragging_ = false;
    bool ctrl_pressed_on_down_ = false;
    wxPoint drag_start_pos_;

    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Floating Toolbar Frame - Special frame for hosting floating toolbars
// ============================================================================
class FloatingToolBarFrame : public wxFrame {
public:
    FloatingToolBarFrame(wxWindow* parent, const wxString& title);
    ~FloatingToolBarFrame() override;

    // Toolbar management
    void SetToolBar(wxToolBar* toolbar);
    wxToolBar* GetToolBar() const { return toolbar_; }
    void ClearToolBar();

    // Layout
    void FitToToolBar();
    void RecalculateLayout();

    // Multi-row/column support
    int GetRowCount() const { return rows_; }
    int GetColumnCount() const { return columns_; }

protected:
    void OnResize(wxSizeEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnActivate(wxActivateEvent& event);

private:
    wxToolBar* toolbar_ = nullptr;
    int rows_ = 1;
    int columns_ = 1;
    wxOrientation toolbar_orientation_ = wxHORIZONTAL;
    wxSize min_toolbar_size_;

    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Drag Ghost Window - Visual feedback during toolbar drag
// ============================================================================
class DragGhostWindow : public wxFrame {
public:
    DragGhostWindow(wxWindow* parent, const wxBitmap& bitmap);
    ~DragGhostWindow() override;

    void UpdatePosition(const wxPoint& pos);
    void SetBitmap(const wxBitmap& bitmap);

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxBitmap ghost_bitmap_;
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Drop Zone Indicator - Shows where toolbar will dock
// ============================================================================
class DropZoneIndicator : public wxFrame {
public:
    DropZoneIndicator(wxWindow* parent, const wxRect& rect);
    ~DropZoneIndicator() override;

    void SetTargetRect(const wxRect& rect);

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxRect target_rect_;
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_FLOATING_FRAME_H
