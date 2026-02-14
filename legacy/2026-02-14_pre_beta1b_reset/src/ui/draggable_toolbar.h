/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DRAGGABLE_TOOLBAR_H
#define SCRATCHROBIN_DRAGGABLE_TOOLBAR_H

#include <wx/toolbar.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/dataobj.h>

namespace scratchrobin {

// Forward declarations
class FloatingToolBarFrame;
class DragGhostWindow;
class DropZoneIndicator;
class IconBarHost;

// ============================================================================
// DraggableToolBar - ToolBar that can be dragged to float/dock
// ============================================================================
class DraggableToolBar : public wxToolBar {
public:
    DraggableToolBar(wxWindow* parent, const wxString& name, 
                     wxWindowID id = wxID_ANY,
                     long style = wxTB_HORIZONTAL | wxTB_FLAT);
    ~DraggableToolBar() override;

    // Toolbar identity
    wxString GetBarName() const { return name_; }
    void SetBarName(const wxString& name) { name_ = name; }

    // Drag handling
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeftDClick(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    // Floating state
    void Float(const wxPoint& pos = wxDefaultPosition);
    void Dock(wxWindow* parent = nullptr);
    bool IsFloating() const { return floating_frame_ != nullptr; }
    FloatingToolBarFrame* GetFloatingFrame() const { return floating_frame_; }

    // Orientation
    void SetOrientation(wxOrientation orient);
    wxOrientation GetOrientation() const { return orientation_; }
    void ToggleOrientation();

    // Context menu
    void OnContextMenu(wxContextMenuEvent& event);
    void OnMenuToggleFloat(wxCommandEvent& event);
    void OnMenuToggleOrientation(wxCommandEvent& event);
    void OnMenuCustomize(wxCommandEvent& event);

    // Host reference
    void SetIconBarHost(IconBarHost* host) { host_ = host; }
    IconBarHost* GetIconBarHost() const { return host_; }

    // Save/restore state
    struct State {
        bool is_floating = false;
        wxPoint float_position;
        wxSize float_size;
        wxOrientation orientation = wxHORIZONTAL;
        bool visible = true;
    };
    State GetState() const;
    void RestoreState(const State& state);

protected:
    // Helper methods
    void StartDrag(const wxPoint& pos);
    void EndDrag(bool do_float);
    void CancelDrag();
    void UpdateDragImage(const wxPoint& screenPos);
    wxBitmap CreateDragImage();
    bool IsInDragHandleArea(const wxPoint& pos) const;

private:
    wxString name_;
    wxOrientation orientation_ = wxHORIZONTAL;
    FloatingToolBarFrame* floating_frame_ = nullptr;
    IconBarHost* host_ = nullptr;

    // Drag state
    bool is_dragging_ = false;
    bool drag_initiated_ = false;
    wxPoint drag_start_pos_;
    wxPoint drag_current_pos_;
    wxRect original_rect_;
    
    // Drag visual feedback
    DragGhostWindow* drag_ghost_ = nullptr;
    DropZoneIndicator* drop_indicator_ = nullptr;
    
    // Dock position memory
    wxWindow* dock_parent_ = nullptr;
    int dock_position_ = -1;
    wxPoint last_float_position_;

    // Drag threshold
    static constexpr int DRAG_THRESHOLD = 5;

    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// ToolBarDragData - Data object for toolbar drag operations
// ============================================================================
class ToolBarDragData : public wxDataObjectSimple {
public:
    static const wxDataFormat& GetFormat();
    
    ToolBarDragData();
    explicit ToolBarDragData(const wxString& barName, wxOrientation orient);
    
    // wxDataObjectSimple implementation
    size_t GetDataSize() const override;
    bool GetDataHere(void* buf) const override;
    bool SetData(size_t len, const void* buf) override;
    
    // Accessors
    wxString GetBarName() const { return bar_name_; }
    wxOrientation GetOrientation() const { return orientation_; }

private:
    wxString bar_name_;
    wxOrientation orientation_ = wxHORIZONTAL;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DRAGGABLE_TOOLBAR_H
