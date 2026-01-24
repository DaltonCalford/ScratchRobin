#ifndef SCRATCHROBIN_DIAGRAM_CANVAS_H
#define SCRATCHROBIN_DIAGRAM_CANVAS_H

#include "diagram_model.h"

#include <optional>
#include <string>

#include <wx/geometry.h>
#include <wx/panel.h>

namespace scratchrobin {

wxDECLARE_EVENT(EVT_DIAGRAM_SELECTION_CHANGED, wxCommandEvent);

class DiagramCanvas : public wxPanel {
public:
    DiagramCanvas(wxWindow* parent, DiagramType type);

    DiagramType diagram_type() const;
    void SetDiagramType(DiagramType type);
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

    const DiagramNode* GetSelectedNode() const;
    DiagramNode* GetSelectedNodeMutable();
    const DiagramEdge* GetSelectedEdge() const;
    DiagramEdge* GetSelectedEdgeMutable();

    void AddNode(const std::string& node_type, const std::string& name);
    void AddEdge(const std::string& source_id, const std::string& target_id, const std::string& label);
    void SelectNextNode();
    void SelectPreviousNode();

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);

    wxPoint ScreenToWorld(const wxPoint& point) const;
    wxRect2DDouble WorldRectForNode(const DiagramNode& node) const;
    void DrawGrid(wxDC& dc);
    void DrawEdges(wxDC& dc);
    void DrawNodes(wxDC& dc);
    void DrawSelectionHandles(wxDC& dc, const DiagramNode& node);

    std::optional<size_t> HitTestNode(const wxPoint& world_point) const;
    std::optional<size_t> HitTestEdge(const wxPoint& world_point) const;
    void UpdateSelection(std::optional<size_t> node_index, std::optional<size_t> edge_index);
    wxPoint2DDouble NextInsertPosition(double width, double height);

    DiagramModel model_;
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
    wxPoint2DDouble drag_offset_{0.0, 0.0};
    bool is_panning_ = false;
    wxPoint last_mouse_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_CANVAS_H
