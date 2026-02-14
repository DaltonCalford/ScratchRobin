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
#include "diagram_containment.h"
#include "diagram_model.h"

#include <optional>
#include <set>
#include <string>

#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/panel.h>

namespace scratchrobin {

wxDECLARE_EVENT(EVT_DIAGRAM_SELECTION_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(EVT_DATA_VIEW_UPDATED, wxCommandEvent);
wxDECLARE_EVENT(EVT_DATA_VIEW_OPEN, wxCommandEvent);

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
    double zoom() const;
    wxPoint2DDouble pan_offset() const;
    void SetView(double zoom, const wxPoint2DDouble& pan);
    
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

    struct DataViewPanel {
        std::string id;
        std::string name;
        wxRect2DDouble rect;
        bool stale = false;
        std::vector<std::string> columns;
        std::vector<std::string> column_types;
        std::vector<std::vector<std::string>> rows;
        int scroll_row = 0;
        int selected_row = -1;
        int selected_col = -1;
    };
    void SetDataViews(const std::vector<DataViewPanel>& panels);
    const DataViewPanel* HitTestDataView(const wxPoint2DDouble& world_point) const;
    bool HandleDataViewKey(int key_code);

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
    
    // Multi-selection support
    bool IsNodeSelected(size_t index) const;
    bool IsNodeSelected(const std::string& node_id) const;
    void ToggleNodeSelection(size_t index);
    void SelectNode(size_t index, bool add_to_selection = false);
    void ClearSelection();
    std::vector<size_t> GetSelectedNodeIndices() const;
    std::vector<std::string> GetSelectedNodeIds() const;
    size_t GetSelectionCount() const { return selected_indices_.size(); }

    void AddNode(const std::string& node_type, const std::string& name);
    void AddEdge(const std::string& source_id, const std::string& target_id, const std::string& label);
    void SelectNextNode();
    void SelectPreviousNode();
    bool SelectNodeByName(const std::string& name);
    
    // Copy/Paste (Phase 3.3.8)
    void CopySelection();
    void Paste();
    bool CanPaste() const;
    
    // Deletion (Phase 3.3.x)
    void DeleteSelection();  // Delete from diagram only (undoable)
    void DeleteSelectionFromProject();  // Delete from project (permanent, with confirmation)
    bool CanDeleteSelection() const;
    
    // Dependency checking for project deletion
    struct DependencyInfo {
        std::string object_id;
        std::string object_name;
        std::string object_type;
        std::string dependent_object;
        std::string dependency_type;  // "foreign_key", "view", "trigger", etc.
    };
    std::vector<DependencyInfo> GetDependenciesForNodes(const std::vector<std::string>& node_ids) const;
    
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
    void OnLeftDClick(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
    void ShowDiagramContextMenu(const wxPoint& pos);
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
    void DrawDataViews(wxDC& dc);
    bool IsNodeVisible(const DiagramNode& node) const;
    void DrawSelectionHandles(wxDC& dc, const DiagramNode& node);
    void DrawEdgeHandles(wxDC& dc, const wxPoint& source, const wxPoint& target);
    wxRect ChevronRectForNode(const DiagramNode& node) const;
    bool HasChildren(const DiagramNode& node) const;
    int CountDescendants(const std::string& node_id) const;
    void CenterOnNode(const DiagramNode& node);

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
    std::optional<size_t> selected_index_;  // Primary selection (for single-select operations)
    std::optional<size_t> selected_edge_index_;
    std::set<size_t> selected_indices_;  // Multi-selection support
    bool multi_select_mode_ = false;  // Ctrl/Cmd key state
    std::optional<size_t> dragging_index_;
    std::optional<size_t> resizing_index_;
    ResizeHandle resize_handle_ = ResizeHandle::None;
    wxRect2DDouble resize_start_rect_{0.0, 0.0, 0.0, 0.0};
    wxPoint2DDouble resize_start_point_{0.0, 0.0};
    std::optional<size_t> dragging_edge_index_;
    EdgeDragEndpoint edge_drag_endpoint_ = EdgeDragEndpoint::None;
    wxPoint edge_drag_point_{0, 0};
    wxPoint2DDouble drag_offset_{0.0, 0.0};
    wxPoint2DDouble drag_start_pos_{0.0, 0.0};  // Starting position for move undo/redo
    bool is_panning_ = false;
    wxPoint last_mouse_;
    std::vector<DataViewPanel> data_views_;
    std::string hover_data_view_;
    int hover_row_ = -1;
    int hover_col_ = -1;
    std::string focused_data_view_;
    
    // Context menu handling
    void OnDeleteFromDiagram(wxCommandEvent& event);
    void OnDeleteFromProject(wxCommandEvent& event);
    
    enum ContextMenuIds {
        ID_DELETE_FROM_DIAGRAM = 1000,
        ID_DELETE_FROM_PROJECT,
        ID_SEPARATOR_1,
        ID_COPY,
        ID_PASTE,
        ID_SEPARATOR_2,
        ID_ALIGN_LEFT,
        ID_ALIGN_RIGHT,
        ID_ALIGN_TOP,
        ID_ALIGN_BOTTOM,
        ID_SEPARATOR_3,
        ID_PIN_NODE,
        ID_UNPIN_NODE
    };
    
    // ============================================================================
    // Drag and Drop Support (Parent/Child Containment)
    // ============================================================================
public:
    // External drag support - called when dragging from tree to diagram
    void BeginExternalDrag(const std::string& node_type, const std::string& node_name);
    void BeginExternalDragMultiple(const std::vector<std::pair<std::string, std::string>>& items);
    bool IsInExternalDrag() const { return external_drag_active_; }
    void CancelExternalDrag();
    
    // Containment checking
    bool CanDropOnNode(size_t dragged_index, size_t target_index) const;
    bool CanDropOnNode(const std::string& dragged_type, size_t target_index) const;
    
private:
    // Drag state
    DragOperation current_drag_operation_ = DragOperation::None;
    std::optional<size_t> drag_target_index_;  // Node being hovered over during drag
    bool drag_target_valid_ = false;
    wxPoint2DDouble drag_current_pos_;  // Current drag position in world coords
    
    // External drag state (from tree)
    bool external_drag_active_ = false;
    std::vector<std::pair<std::string, std::string>> external_drag_items_;  // type, name pairs
    
    // Drag visual feedback
    void DrawDragOverlay(wxDC& dc);
    void DrawDropTargetHighlight(wxDC& dc);
    void DrawGhostNode(wxDC& dc, const wxPoint2DDouble& pos, const std::string& node_type);
    
    // Drag handling helpers
    void UpdateDragTarget(const wxPoint2DDouble& world_point);
    void ClearDragTarget();
    bool IsValidDropTarget(size_t target_index) const;
    bool WouldCreateCircularReference(const std::string& node_id, const std::string& potential_parent_id) const;
    
    // Snap to grid
    wxPoint2DDouble SnapToGrid(const wxPoint2DDouble& pos) const;
    
    // Execute drop operation
    void ExecuteDrop(const wxPoint2DDouble& world_point);
    void ExecuteReparent(size_t node_index, size_t new_parent_index, const wxPoint2DDouble& new_pos);
    void ExecuteExternalDrop(const wxPoint2DDouble& world_point, const std::string& node_type, 
                              const std::string& node_name);
    void ExecuteMultiExternalDrop(const wxPoint2DDouble& world_point);
    
    // Cursor management during drag
    void UpdateDragCursor();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_CANVAS_H
