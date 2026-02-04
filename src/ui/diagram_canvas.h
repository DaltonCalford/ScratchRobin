/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_CANVAS_H
#define SCRATCHROBIN_DIAGRAM_CANVAS_H

#include "diagram/command.h"
#include "diagram/layout_engine.h"
#include "diagram_model.h"

#include <optional>
#include <string>

#include <wx/geometry.h>
#include <wx/panel.h>

namespace scratchrobin {

wxDECLARE_EVENT(EVT_DIAGRAM_SELECTION_CHANGED, wxCommandEvent);

enum class ResizeHandle {
    None,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
    Left
};

enum class EdgeDragEndpoint {
    None,
    Source,
    Target
};

class DiagramCanvas : public wxPanel {
public:
    DiagramCanvas(wxWindow* parent, DiagramType type);

    DiagramType diagram_type() const;
    void SetDiagramType(DiagramType type);
    
    // ERD Notation support (Phase 3.2)
    ErdNotation notation() const;
    void SetNotation(ErdNotation notation);
    
    // Auto-layout (Phase 3.4)
    void ApplyLayout(diagram::LayoutAlgorithm algorithm);
    void ApplyLayout(const diagram::LayoutOptions& options);
    
    void SetTemplateKey(const std::string& key);
    const std::string& template_key() const;
    void SetGridSize(int size);
    int grid_size() const;
    void SetIconSet(const std::string& key);
    const std::string& icon_set() const;
    void SetBorderWidth(int width);
    int border_width() const;
    void SetBorderDashed(bool value);
    bool border_dashed() const;

    DiagramModel& model();
    const DiagramModel& model() const;
    
    // Command manager for undo/redo
    CommandManager& command_manager() { return command_manager_; }
    const CommandManager& command_manager() const { return command_manager_; }
    
    // Undo/redo helpers
    bool CanUndo() const { return command_manager_.CanUndo(); }
    bool CanRedo() const { return command_manager_.CanRedo(); }
    void Undo() { if (command_manager_.Undo()) Refresh(); }
    void Redo() { if (command_manager_.Redo()) Refresh(); }

    const DiagramNode* GetSelectedNode() const;
    DiagramNode* GetSelectedNodeMutable();
    const DiagramEdge* GetSelectedEdge() const;
    DiagramEdge* GetSelectedEdgeMutable();

    void AddNode(const std::string& node_type, const std::string& name);
    void AddEdge(const std::string& source_id, const std::string& target_id, const std::string& label);
    void SelectNextNode();
    void SelectPreviousNode();
    
    // Copy/Paste (Phase 3.3.8)
    void CopySelection();
    void Paste();
    bool CanPaste() const;
    void DeleteSelection();
    
    // Alignment tools (Phase 3.3.9)
    void AlignLeft();
    void AlignRight();
    void AlignTop();
    void AlignBottom();
    void AlignCenterHorizontal();
    void AlignCenterVertical();
    void DistributeHorizontal();
    void DistributeVertical();
    
    // Pin/Unpin nodes (Phase 3.4.6)
    void PinSelectedNode();
    void UnpinSelectedNode();
    void TogglePinSelectedNode();
    bool IsSelectedNodePinned() const;
    std::vector<std::string> GetPinnedNodeIds() const;

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);

    wxPoint2DDouble ScreenToWorldDouble(const wxPoint& point) const;
    wxPoint ScreenToWorld(const wxPoint& point) const;
    wxRect2DDouble WorldRectForNode(const DiagramNode& node) const;
    wxPoint ComputeNodeCenter(const DiagramNode& node) const;
    wxPoint ComputeEdgeAnchor(const DiagramNode& node, const wxPoint& toward) const;
    void DrawGrid(wxDC& dc);
    void DrawEdges(wxDC& dc);
    void DrawNodes(wxDC& dc);
    void DrawSelectionHandles(wxDC& dc, const DiagramNode& node);
    void DrawEdgeHandles(wxDC& dc, const wxPoint& source, const wxPoint& target);

    std::optional<size_t> HitTestNode(const wxPoint2DDouble& world_point) const;
    std::optional<size_t> HitTestEdge(const wxPoint2DDouble& world_point) const;
    ResizeHandle HitTestResizeHandle(const DiagramNode& node, const wxPoint2DDouble& world_point) const;
    EdgeDragEndpoint HitTestEdgeEndpoint(size_t edge_index, const wxPoint2DDouble& world_point) const;
    void UpdateSelection(std::optional<size_t> node_index, std::optional<size_t> edge_index);
    wxPoint2DDouble NextInsertPosition(double width, double height);
    wxCursor CursorForHandle(ResizeHandle handle) const;
    void UpdateHoverCursor(const wxPoint2DDouble& world_point);
    void ApplyResize(const wxPoint2DDouble& world_point);

    DiagramModel model_;
    CommandManager command_manager_;
    std::string template_key_ = "default";
    std::string icon_set_ = "default";
    int border_width_ = 1;
    bool border_dashed_ = false;
    double zoom_ = 1.0;
    wxPoint2DDouble pan_offset_{0.0, 0.0};
    bool show_grid_ = true;
    int grid_size_ = 16;
    std::optional<size_t> selected_index_;
    std::optional<size_t> selected_edge_index_;
    std::optional<size_t> dragging_index_;
    std::optional<size_t> resizing_index_;
    ResizeHandle resize_handle_ = ResizeHandle::None;
    wxRect2DDouble resize_start_rect_{0.0, 0.0, 0.0, 0.0};
    wxPoint2DDouble resize_start_point_{0.0, 0.0};
    std::optional<size_t> dragging_edge_index_;
    EdgeDragEndpoint edge_drag_endpoint_ = EdgeDragEndpoint::None;
    wxPoint edge_drag_point_{0, 0};
    wxPoint2DDouble drag_offset_{0.0, 0.0};
    bool is_panning_ = false;
    wxPoint last_mouse_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_CANVAS_H
