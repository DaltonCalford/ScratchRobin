#pragma once

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <wx/panel.h>

#include "core/beta1b_contracts.h"
#include "diagram/diagram_services.h"

namespace scratchrobin::gui {

class DiagramCanvasPanel final : public wxPanel {
public:
    DiagramCanvasPanel(wxWindow* parent, diagram::DiagramService* diagram_service);

    void SetDocument(beta1b::DiagramDocument* document);
    void SetStatusSink(std::function<void(const std::string&)> sink);
    void SetMutationSink(std::function<void(const std::string&)> sink);
    void SetSelectionSink(std::function<void(const std::string& node_id,
                                             const std::string& object_type,
                                             const std::string& icon_slot,
                                             const std::string& display_mode,
                                             bool chamfer_notes)> sink);

    void SetGridVisible(bool visible);
    bool GridVisible() const;
    void SetSnapToGrid(bool enabled);
    bool SnapToGrid() const;

    void ZoomIn();
    void ZoomOut();
    void ZoomReset();

    bool NudgeSelectedNode(int dx, int dy, std::string* error = nullptr);
    bool ResizeSelectedNode(int dwidth, int dheight, std::string* error = nullptr);
    bool ConnectSelectedToNext(std::string* error = nullptr);
    bool ReparentSelectedToNext(std::string* error = nullptr);
    bool AddNode(std::string* error = nullptr);
    bool DeleteSelectedNode(bool destructive, std::string* error = nullptr);
    bool Undo(std::string* error = nullptr);
    bool Redo(std::string* error = nullptr);
    std::string SelectedNodeId() const;
    bool ApplySilverstonNodeProfile(const std::string& object_type,
                                    const std::string& icon_slot,
                                    const std::string& display_mode,
                                    bool chamfer_notes,
                                    std::string* error = nullptr);
    bool ApplySilverstonDiagramPolicy(int grid_size,
                                      const std::string& alignment_policy,
                                      const std::string& drop_policy,
                                      const std::string& resize_policy,
                                      const std::string& display_profile,
                                      std::string* error = nullptr);
    int GridSize() const;
    void SetGridSize(int grid_size);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnCharHook(wxKeyEvent& event);

    beta1b::DiagramNode* SelectedNode();
    const beta1b::DiagramNode* FindNode(const std::string& node_id) const;
    beta1b::DiagramNode* FindNode(const std::string& node_id);
    beta1b::DiagramNode* HitTestNode(const wxPoint& screen_point);
    bool IsSelected(const std::string& node_id) const;
    void ToggleSelection(const std::string& node_id);

    bool ApplyMove(int dx, int dy, std::string* error);
    bool ApplyResize(int dwidth, int dheight, std::string* error);
    bool ApplyConnect(const std::string& source_node_id, const std::string& target_node_id, std::string* error);
    bool ApplyReparent(const std::string& node_id, const std::string& new_parent_id, std::string* error);
    bool PushHistory(const beta1b::DiagramDocument& before,
                     const beta1b::DiagramDocument& after,
                     const std::string& label,
                     std::string* error);

    wxRect ScreenRectForNode(const beta1b::DiagramNode& node) const;
    wxPoint NodeCenter(const beta1b::DiagramNode& node) const;
    int Snap(int value) const;
    int ScaleToModel(int value) const;
    std::string NextNodeId() const;
    std::string NextEdgeId() const;
    void EmitStatus(const std::string& message) const;
    void EmitMutation(const std::string& mutation_kind) const;
    void EmitSelection() const;
    void SetError(std::string* error, const std::string& message) const;

    diagram::DiagramService* diagram_service_ = nullptr;
    beta1b::DiagramDocument* document_ = nullptr;
    std::function<void(const std::string&)> status_sink_;
    std::function<void(const std::string&)> mutation_sink_;
    std::function<void(const std::string& node_id,
                       const std::string& object_type,
                       const std::string& icon_slot,
                       const std::string& display_mode,
                       bool chamfer_notes)> selection_sink_;

    std::string selected_node_id_;
    std::set<std::string> selected_node_ids_;
    std::string connect_source_node_id_;

    struct CanvasHistoryEntry {
        beta1b::DiagramDocument before;
        beta1b::DiagramDocument after;
        std::string label;
    };
    std::vector<CanvasHistoryEntry> undo_stack_;
    std::vector<CanvasHistoryEntry> redo_stack_;
    std::size_t max_history_entries_ = 128;

    bool dragging_ = false;
    bool resizing_ = false;
    wxPoint drag_anchor_;
    int drag_origin_x_ = 0;
    int drag_origin_y_ = 0;
    int drag_origin_width_ = 0;
    int drag_origin_height_ = 0;
    std::optional<beta1b::DiagramDocument> drag_before_document_;

    bool show_grid_ = true;
    bool snap_to_grid_ = false;
    int grid_size_ = 20;
    double zoom_ = 1.0;
};

}  // namespace scratchrobin::gui
