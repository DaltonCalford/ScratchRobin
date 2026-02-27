#include "gui/diagram_canvas.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>

#include <wx/dcbuffer.h>
#include <wx/dnd.h>
#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>

#include "core/reject.h"

namespace scratchrobin::gui {

namespace {

wxColour NodeFill(const std::string& object_type) {
    if (object_type == "table") {
        return wxColour(234, 242, 255);
    }
    if (object_type == "view") {
        return wxColour(238, 248, 236);
    }
    if (object_type == "index") {
        return wxColour(250, 244, 227);
    }
    return wxColour(240, 240, 240);
}

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string NodeDisplayName(const beta1b::DiagramNode& node) {
    if (!node.name.empty()) {
        return node.name;
    }
    return node.node_id;
}

wxString ToWx(const std::string& value) {
    return wxString::FromUTF8(value);
}

class CanvasDropTarget final : public wxTextDropTarget {
public:
    explicit CanvasDropTarget(DiagramCanvasPanel* panel) : panel_(panel) {}

    bool OnDropText(wxCoord x, wxCoord y, const wxString& data) override {
        if (panel_ == nullptr) {
            return false;
        }
        const std::string payload = data.ToStdString();
        static const std::string kPrefix = "diagram_item:";
        if (payload.rfind(kPrefix, 0) != 0U) {
            return false;
        }
        std::string error;
        if (!panel_->AddNodeOfTypeAt(payload.substr(kPrefix.size()), wxPoint(x, y), &error)) {
            return false;
        }
        return true;
    }

private:
    DiagramCanvasPanel* panel_ = nullptr;
};

}  // namespace

DiagramCanvasPanel::DiagramCanvasPanel(wxWindow* parent, diagram::DiagramService* diagram_service)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 360)),
      diagram_service_(diagram_service) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 280));

    Bind(wxEVT_PAINT, &DiagramCanvasPanel::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &DiagramCanvasPanel::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &DiagramCanvasPanel::OnLeftUp, this);
    Bind(wxEVT_MOTION, &DiagramCanvasPanel::OnMotion, this);
    Bind(wxEVT_MOUSEWHEEL, &DiagramCanvasPanel::OnMouseWheel, this);
    Bind(wxEVT_CHAR_HOOK, &DiagramCanvasPanel::OnCharHook, this);
    SetDropTarget(new CanvasDropTarget(this));
}

void DiagramCanvasPanel::SetDocument(beta1b::DiagramDocument* document) {
    document_ = document;
    selected_node_id_.clear();
    selected_node_ids_.clear();
    connect_source_node_id_.clear();
    undo_stack_.clear();
    redo_stack_.clear();
    drag_before_document_.reset();
    resize_handle_ = ResizeHandle::None;
    if (document_ != nullptr && document_->grid_size > 0) {
        grid_size_ = document_->grid_size;
    }
    EmitSelection();
    Refresh();
}

void DiagramCanvasPanel::SetStatusSink(std::function<void(const std::string&)> sink) {
    status_sink_ = std::move(sink);
}

void DiagramCanvasPanel::SetMutationSink(std::function<void(const std::string&)> sink) {
    mutation_sink_ = std::move(sink);
}

void DiagramCanvasPanel::SetSelectionSink(std::function<void(const std::string& node_id,
                                                             const std::string& object_type,
                                                             const std::string& icon_slot,
                                                             const std::string& display_mode,
                                                             bool chamfer_notes)> sink) {
    selection_sink_ = std::move(sink);
    EmitSelection();
}

void DiagramCanvasPanel::SetGridVisible(bool visible) {
    show_grid_ = visible;
    Refresh();
}

bool DiagramCanvasPanel::GridVisible() const {
    return show_grid_;
}

void DiagramCanvasPanel::SetSnapToGrid(bool enabled) {
    snap_to_grid_ = enabled;
}

bool DiagramCanvasPanel::SnapToGrid() const {
    return snap_to_grid_;
}

void DiagramCanvasPanel::ZoomIn() {
    zoom_ = std::min(2.5, zoom_ + 0.1);
    Refresh();
    EmitStatus("diagram zoom " + std::to_string(static_cast<int>(zoom_ * 100.0)) + "%");
}

void DiagramCanvasPanel::ZoomOut() {
    zoom_ = std::max(0.4, zoom_ - 0.1);
    Refresh();
    EmitStatus("diagram zoom " + std::to_string(static_cast<int>(zoom_ * 100.0)) + "%");
}

void DiagramCanvasPanel::ZoomReset() {
    zoom_ = 1.0;
    Refresh();
    EmitStatus("diagram zoom reset to 100%");
}

bool DiagramCanvasPanel::NudgeSelectedNode(int dx, int dy, std::string* error) {
    return ApplyMove(dx, dy, error);
}

bool DiagramCanvasPanel::ResizeSelectedNode(int dwidth, int dheight, std::string* error) {
    return ApplyResize(dwidth, dheight, error);
}

bool DiagramCanvasPanel::ConnectSelectedToNext(std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (document_->nodes.size() < 2U) {
        SetError(error, "at least two nodes are required to create a connector");
        return false;
    }
    if (selected_node_id_.empty()) {
        selected_node_id_ = document_->nodes.front().node_id;
    }

    std::size_t selected_index = 0U;
    bool found = false;
    for (std::size_t i = 0; i < document_->nodes.size(); ++i) {
        if (document_->nodes[i].node_id == selected_node_id_) {
            selected_index = i;
            found = true;
            break;
        }
    }
    if (!found) {
        selected_index = 0U;
        selected_node_id_ = document_->nodes[0].node_id;
    }

    const std::size_t target_index = (selected_index + 1U) % document_->nodes.size();
    const std::string target_node_id = document_->nodes[target_index].node_id;
    return ApplyConnect(selected_node_id_, target_node_id, error);
}

bool DiagramCanvasPanel::ReparentSelectedToNext(std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (selected_node_id_.empty()) {
        SetError(error, "select a node to reparent");
        return false;
    }
    if (document_->nodes.size() < 2U) {
        SetError(error, "at least two nodes are required to reparent");
        return false;
    }
    std::size_t selected_index = 0U;
    bool found = false;
    for (std::size_t i = 0; i < document_->nodes.size(); ++i) {
        if (document_->nodes[i].node_id == selected_node_id_) {
            selected_index = i;
            found = true;
            break;
        }
    }
    if (!found) {
        SetError(error, "selected node no longer exists");
        return false;
    }
    const std::string target_parent = document_->nodes[(selected_index + 1U) % document_->nodes.size()].node_id;
    return ApplyReparent(selected_node_id_, target_parent, error);
}

bool DiagramCanvasPanel::AddNode(std::string* error) {
    const wxSize canvas = GetClientSize();
    const wxPoint drop_point(std::max(40, canvas.x / 3), std::max(48, canvas.y / 3));
    return AddNodeOfTypeAt("", drop_point, error);
}

bool DiagramCanvasPanel::AddNodeOfTypeAt(const std::string& object_type,
                                         const wxPoint& screen_point,
                                         std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    const std::string diagram_type = ToLower(document_->diagram_type);
    std::string normalized_type = ToLower(object_type);
    if (normalized_type.empty()) {
        if (diagram_type == "silverston") {
            normalized_type = "entity";
        } else if (diagram_type == "whiteboard") {
            normalized_type = "note";
        } else if (diagram_type == "mindmap") {
            normalized_type = "topic";
        } else {
            normalized_type = "table";
        }
    }

    const auto icon_for_type = [](const std::string& node_type) {
        if (node_type == "subject_area") {
            return std::string("subject_generic");
        }
        if (node_type == "entity") {
            return std::string("entity_generic");
        }
        if (node_type == "fact") {
            return std::string("fact_measure");
        }
        if (node_type == "dimension") {
            return std::string("dimension_time");
        }
        if (node_type == "lookup") {
            return std::string("lookup_code");
        }
        if (node_type == "hub") {
            return std::string("hub_business_key");
        }
        if (node_type == "link") {
            return std::string("link_association");
        }
        if (node_type == "satellite") {
            return std::string("satellite_context");
        }
        return std::string("");
    };
    beta1b::DiagramDocument before = *document_;
    beta1b::DiagramNode node;
    node.node_id = NextNodeId();
    node.object_type = normalized_type;
    node.name = normalized_type + " " + node.node_id;
    node.width = (diagram_type == "mindmap") ? 230 : 220;
    node.height = (diagram_type == "mindmap") ? 100 : 120;
    if (normalized_type == "note" || normalized_type == "task" || normalized_type == "risk") {
        node.width = 260;
        node.height = 120;
    }
    node.x = ScaleToModel(screen_point.x) - (node.width / 2);
    node.y = ScaleToModel(screen_point.y) - (node.height / 2);
    node.x = std::max(10, node.x);
    node.y = std::max(10, node.y);
    if (snap_to_grid_) {
        node.x = Snap(node.x);
        node.y = Snap(node.y);
    }
    node.logical_datatype = (diagram_type == "erd" || diagram_type.empty()) ? "VARCHAR" : "N/A";
    node.display_mode = "full";
    node.icon_slot = icon_for_type(normalized_type);
    node.stack_count = 1;
    if (diagram_service_ != nullptr) {
        try {
            diagram_service_->ApplyCanvasCommand(*document_, "add_node", node.node_id, "");
        } catch (const RejectError& ex) {
            SetError(error, ex.what());
            return false;
        }
    }
    document_->nodes.push_back(node);
    selected_node_id_ = node.node_id;
    selected_node_ids_.clear();
    selected_node_ids_.insert(node.node_id);
    EmitSelection();
    Refresh();
    if (!PushHistory(before, *document_, "add_node", error)) {
        return false;
    }
    EmitMutation("add_node");
    EmitStatus("added " + normalized_type + " node " + node.node_id);
    return true;
}

bool DiagramCanvasPanel::DeleteSelectedNode(bool destructive, std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (selected_node_id_.empty() && selected_node_ids_.empty()) {
        SetError(error, "select a node to delete");
        return false;
    }
    std::set<std::string> remove_ids = selected_node_ids_;
    if (remove_ids.empty() && !selected_node_id_.empty()) {
        remove_ids.insert(selected_node_id_);
    }

    if (destructive) {
        std::size_t dependent_edges = 0U;
        for (const auto& edge : document_->edges) {
            if (remove_ids.count(edge.from_node_id) > 0U || remove_ids.count(edge.to_node_id) > 0U) {
                ++dependent_edges;
            }
        }
        if (dependent_edges > 0U) {
            const int answer = wxMessageBox(
                ToWx("Project-level delete will remove " + std::to_string(remove_ids.size()) +
                     " node(s) and " + std::to_string(dependent_edges) + " dependent edge(s). Continue?"),
                "Confirm Project Delete",
                wxYES_NO | wxICON_WARNING,
                this);
            if (answer != wxYES) {
                SetError(error, "project-level delete cancelled");
                return false;
            }
        }
    }

    beta1b::DiagramDocument before = *document_;
    for (const auto& node_id : remove_ids) {
        if (diagram_service_ != nullptr) {
            try {
                diagram_service_->ApplyCanvasCommand(*document_, "delete_node", node_id, "");
            } catch (const RejectError& ex) {
                SetError(error, ex.what());
                return false;
            }
        }
    }
    document_->nodes.erase(std::remove_if(document_->nodes.begin(),
                                          document_->nodes.end(),
                                          [&](const beta1b::DiagramNode& node) { return remove_ids.count(node.node_id) > 0U; }),
                           document_->nodes.end());
    document_->edges.erase(std::remove_if(document_->edges.begin(),
                                          document_->edges.end(),
                                          [&](const beta1b::DiagramEdge& edge) {
                                              return remove_ids.count(edge.from_node_id) > 0U ||
                                                     remove_ids.count(edge.to_node_id) > 0U;
                                          }),
                           document_->edges.end());
    selected_node_id_.clear();
    selected_node_ids_.clear();
    connect_source_node_id_.clear();
    EmitSelection();
    Refresh();
    if (!PushHistory(before, *document_, destructive ? "delete_project" : "delete_node", error)) {
        return false;
    }

    const std::string mode = destructive ? "project-level delete" : "diagram-only delete";
    EmitMutation(destructive ? "delete_project" : "delete_node");
    EmitStatus(mode + " completed for " + std::to_string(remove_ids.size()) + " node(s)");
    return true;
}

bool DiagramCanvasPanel::Undo(std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (undo_stack_.empty()) {
        SetError(error, "undo stack empty");
        return false;
    }
    const CanvasHistoryEntry entry = undo_stack_.back();
    undo_stack_.pop_back();
    redo_stack_.push_back(entry);
    *document_ = entry.before;
    selected_node_id_.clear();
    selected_node_ids_.clear();
    connect_source_node_id_.clear();
    EmitSelection();
    Refresh();
    EmitMutation("undo:" + entry.label);
    EmitStatus("undo " + entry.label);
    return true;
}

bool DiagramCanvasPanel::Redo(std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (redo_stack_.empty()) {
        SetError(error, "redo stack empty");
        return false;
    }
    const CanvasHistoryEntry entry = redo_stack_.back();
    redo_stack_.pop_back();
    undo_stack_.push_back(entry);
    *document_ = entry.after;
    selected_node_id_.clear();
    selected_node_ids_.clear();
    connect_source_node_id_.clear();
    EmitSelection();
    Refresh();
    EmitMutation("redo:" + entry.label);
    EmitStatus("redo " + entry.label);
    return true;
}

std::string DiagramCanvasPanel::SelectedNodeId() const {
    return selected_node_id_;
}

bool DiagramCanvasPanel::ApplySilverstonNodeProfile(const std::string& object_type,
                                                    const std::string& icon_slot,
                                                    const std::string& display_mode,
                                                    bool chamfer_notes,
                                                    std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    beta1b::DiagramNode* node = SelectedNode();
    if (node == nullptr) {
        SetError(error, "select a node in the canvas");
        return false;
    }
    const std::string normalized_type = ToLower(object_type);
    const std::set<std::string> allowed_types = {"subject_area", "entity", "fact", "dimension", "lookup", "hub",
                                                  "link", "satellite", "note", "task", "risk", "topic", "table", "view"};
    if (allowed_types.count(normalized_type) == 0U) {
        SetError(error, "invalid silverston object type");
        return false;
    }
    const std::string normalized_display = ToLower(display_mode);
    const std::set<std::string> allowed_display = {"header_only", "summary", "full"};
    if (allowed_display.count(normalized_display) == 0U) {
        SetError(error, "invalid display mode");
        return false;
    }
    const std::string normalized_icon = ToLower(icon_slot);
    if (normalized_icon.empty()) {
        SetError(error, "icon slot is required");
        return false;
    }

    beta1b::DiagramDocument before = *document_;
    node->object_type = normalized_type;
    node->icon_slot = normalized_icon;
    node->display_mode = normalized_display;
    if (chamfer_notes) {
        if (node->notes.find("[chamfer]") == std::string::npos) {
            if (!node->notes.empty()) {
                node->notes += " ";
            }
            node->notes += "[chamfer]";
        }
    } else {
        const std::string marker = "[chamfer]";
        const auto pos = node->notes.find(marker);
        if (pos != std::string::npos) {
            node->notes.erase(pos, marker.size());
            while (!node->notes.empty() && node->notes.back() == ' ') {
                node->notes.pop_back();
            }
        }
    }
    Refresh();
    EmitSelection();
    if (!PushHistory(before, *document_, "silverston_node_profile", error)) {
        return false;
    }
    EmitMutation("silverston_node_profile");
    EmitStatus("silverston profile applied to " + node->node_id);
    return true;
}

bool DiagramCanvasPanel::ApplySilverstonDiagramPolicy(int grid_size,
                                                      const std::string& alignment_policy,
                                                      const std::string& drop_policy,
                                                      const std::string& resize_policy,
                                                      const std::string& display_profile,
                                                      std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    const std::string normalized_align = ToLower(alignment_policy);
    const std::string normalized_drop = ToLower(drop_policy);
    const std::string normalized_resize = ToLower(resize_policy);
    const std::string normalized_display = ToLower(display_profile);
    const std::set<std::string> allowed_align = {"free", "strict_grid", "column_flow"};
    const std::set<std::string> allowed_drop = {"containment", "containment_strict", "free"};
    const std::set<std::string> allowed_resize = {"free", "snap_step", "fixed_classes"};
    const std::set<std::string> allowed_display = {"standard", "analysis", "catalog"};
    if (grid_size < 4 || grid_size > 256) {
        SetError(error, "grid size must be between 4 and 256");
        return false;
    }
    if (allowed_align.count(normalized_align) == 0U ||
        allowed_drop.count(normalized_drop) == 0U ||
        allowed_resize.count(normalized_resize) == 0U ||
        allowed_display.count(normalized_display) == 0U) {
        SetError(error, "invalid silverston diagram policy");
        return false;
    }

    beta1b::DiagramDocument before = *document_;
    document_->grid_size = grid_size;
    document_->alignment_policy = normalized_align;
    document_->drop_policy = normalized_drop;
    document_->resize_policy = normalized_resize;
    document_->display_profile = normalized_display;
    grid_size_ = grid_size;
    snap_to_grid_ = normalized_align != "free";
    Refresh();
    if (!PushHistory(before, *document_, "silverston_diagram_policy", error)) {
        return false;
    }
    EmitMutation("silverston_diagram_policy");
    EmitStatus("silverston diagram policy applied");
    return true;
}

int DiagramCanvasPanel::GridSize() const {
    return grid_size_;
}

void DiagramCanvasPanel::SetGridSize(int grid_size) {
    if (grid_size > 0) {
        grid_size_ = grid_size;
        if (document_ != nullptr) {
            document_->grid_size = grid_size;
        }
    }
}

void DiagramCanvasPanel::OnPaint(wxPaintEvent& event) {
    (void)event;
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    dc.Clear();

    const wxSize size = GetClientSize();
    if (show_grid_) {
        dc.SetPen(wxPen(wxColour(236, 236, 236), 1));
        const int scaled_grid = std::max(10, static_cast<int>(grid_size_ * zoom_));
        for (int x = 0; x < size.x; x += scaled_grid) {
            dc.DrawLine(x, 0, x, size.y);
        }
        for (int y = 0; y < size.y; y += scaled_grid) {
            dc.DrawLine(0, y, size.x, y);
        }
    }

    if (document_ == nullptr) {
        dc.SetTextForeground(wxColour(120, 120, 120));
        dc.DrawText("No active diagram. Use Open Link to load a model.", 12, 12);
        return;
    }

    std::map<std::string, const beta1b::DiagramNode*> index;
    for (const auto& node : document_->nodes) {
        index[node.node_id] = &node;
    }

    const std::string notation = ToLower(document_->notation);
    const bool is_silverston = ToLower(document_->diagram_type) == "silverston";
    const bool is_mind_map = ToLower(document_->diagram_type) == "mindmap";
    const bool is_whiteboard = ToLower(document_->diagram_type) == "whiteboard";
    std::map<std::string, int> child_count_by_parent;
    for (const auto& node : document_->nodes) {
        if (!node.parent_node_id.empty()) {
            ++child_count_by_parent[node.parent_node_id];
        }
    }

    std::string mode_label = "ERD";
    if (is_silverston) {
        mode_label = "Silverston";
    } else if (is_whiteboard) {
        mode_label = "Whiteboard";
    } else if (is_mind_map) {
        mode_label = "MindMap";
    }
    dc.SetTextForeground(wxColour(72, 72, 72));
    std::string policy_line = "Mode: " + mode_label + "  Notation: " + ToUpper(document_->notation);
    if (is_silverston) {
        policy_line += "  Grid=" + std::to_string(grid_size_);
        policy_line += "  Align=" + (document_->alignment_policy.empty() ? "free" : document_->alignment_policy);
        policy_line += "  Drop=" + (document_->drop_policy.empty() ? "containment" : document_->drop_policy);
        policy_line += "  Resize=" + (document_->resize_policy.empty() ? "free" : document_->resize_policy);
        policy_line += "  Display=" + (document_->display_profile.empty() ? "standard" : document_->display_profile);
    }
    dc.DrawText(policy_line, 10, 8);

    for (const auto& edge : document_->edges) {
        const auto from_it = index.find(edge.from_node_id);
        const auto to_it = index.find(edge.to_node_id);
        if (from_it == index.end() || to_it == index.end()) {
            continue;
        }
        const wxPoint from_center = NodeCenter(*from_it->second);
        const wxPoint to_center = NodeCenter(*to_it->second);
        if (is_mind_map) {
            dc.SetPen(wxPen(wxColour(46, 123, 81), 3));
            const int mid_x = (from_center.x + to_center.x) / 2;
            dc.DrawLine(from_center.x, from_center.y, mid_x, from_center.y);
            dc.DrawLine(mid_x, from_center.y, to_center.x, to_center.y);
        } else if (is_whiteboard) {
            dc.SetPen(wxPen(wxColour(130, 106, 67), 2, wxPENSTYLE_DOT));
            dc.DrawLine(from_center, to_center);
        } else if (is_silverston) {
            dc.SetPen(wxPen(wxColour(56, 91, 145), 2, wxPENSTYLE_SHORT_DASH));
            dc.DrawLine(from_center, to_center);
        } else {
            dc.SetPen(wxPen(wxColour(90, 90, 90), notation == "chen" ? 3 : 2));
            dc.DrawLine(from_center, to_center);
        }
        if (edge.directed && !is_mind_map) {
            const wxPoint arrow(to_center.x - 8, to_center.y - 4);
            dc.DrawLine(arrow, to_center);
            dc.DrawLine(wxPoint(to_center.x - 8, to_center.y + 4), to_center);
        }

        if (!is_mind_map) {
            const int label_x = (from_center.x + to_center.x) / 2;
            const int label_y = (from_center.y + to_center.y) / 2;
            dc.SetTextForeground(wxColour(60, 60, 60));
            const std::string label = !edge.label.empty() ? edge.label :
                                      (!edge.edge_type.empty() ? edge.edge_type :
                                       (edge.relation_type.empty() ? "link" : edge.relation_type));
            dc.DrawText(label, label_x + 4, label_y + 2);
        }
    }

    for (const auto& node : document_->nodes) {
        const wxRect rect = ScreenRectForNode(node);
        const bool selected = IsSelected(node.node_id);

        wxColour fill = NodeFill(node.object_type);
        if (is_silverston) {
            fill = wxColour(217, 229, 247);
        } else if (is_whiteboard) {
            if (node.object_type == "risk") {
                fill = wxColour(255, 222, 222);
            } else if (node.object_type == "task") {
                fill = wxColour(255, 245, 194);
            } else {
                fill = wxColour(255, 251, 221);
            }
        } else if (is_mind_map) {
            fill = node.parent_node_id.empty() ? wxColour(209, 232, 255) : wxColour(224, 245, 226);
        }
        if (node.ghosted) {
            fill = wxColour(225, 225, 225);
        }
        wxColour border = selected ? wxColour(38, 107, 255) : wxColour(68, 68, 68);
        int pen_width = selected ? 3 : 1;
        if (is_silverston) {
            border = selected ? wxColour(43, 82, 140) : wxColour(56, 91, 145);
            pen_width = selected ? 2 : 1;
        } else if (is_whiteboard) {
            border = selected ? wxColour(145, 110, 49) : wxColour(153, 127, 78);
            pen_width = selected ? 2 : 1;
        } else if (is_mind_map) {
            border = selected ? wxColour(31, 110, 77) : wxColour(39, 131, 93);
            pen_width = selected ? 3 : 2;
        } else if (notation == "uml") {
            pen_width = selected ? 2 : 1;
        }

        if (is_whiteboard) {
            dc.SetPen(wxPen(wxColour(210, 210, 200), 1));
            dc.SetBrush(wxBrush(wxColour(225, 225, 225)));
            dc.DrawRectangle(rect.x + 4, rect.y + 4, rect.width, rect.height);
        }
        dc.SetBrush(wxBrush(fill));
        dc.SetPen(wxPen(border, pen_width));
        if (is_mind_map) {
            dc.DrawEllipse(rect);
        } else if (is_whiteboard) {
            dc.DrawRectangle(rect);
        } else {
            dc.DrawRoundedRectangle(rect, is_silverston ? 2 : 6);
        }
        if (selected) {
            const int hs = 7;
            const int half = hs / 2;
            const int left = rect.x;
            const int right = rect.x + rect.width;
            const int top = rect.y;
            const int bottom = rect.y + rect.height;
            const int cx = rect.x + (rect.width / 2);
            const int cy = rect.y + (rect.height / 2);
            dc.SetPen(wxPen(wxColour(38, 107, 255), 1));
            dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
            const auto draw_handle = [&](int x, int y) {
                dc.DrawRectangle(x - half, y - half, hs, hs);
            };
            draw_handle(left, top);
            draw_handle(cx, top);
            draw_handle(right, top);
            draw_handle(left, cy);
            draw_handle(right, cy);
            draw_handle(left, bottom);
            draw_handle(cx, bottom);
            draw_handle(right, bottom);
        }
        if (is_silverston) {
            dc.SetPen(wxPen(wxColour(56, 91, 145), 2));
            dc.DrawLine(rect.x, rect.y + 24, rect.x + rect.width, rect.y + 24);
        }

        dc.SetTextForeground(wxColour(24, 24, 24));
        std::string header;
        if (is_mind_map) {
            header = NodeDisplayName(node);
        } else if (is_whiteboard) {
            header = ToUpper(node.object_type) + " | " + NodeDisplayName(node);
        } else if (is_silverston) {
            header = NodeDisplayName(node) + " :: " + ToUpper(node.object_type);
        } else {
            header = ToUpper(node.object_type) + " " + NodeDisplayName(node);
        }
        dc.DrawText(header, rect.x + 8, rect.y + 6);
        dc.SetTextForeground(wxColour(70, 70, 70));
        const std::string details = node.logical_datatype.empty() ? "datatype: n/a" : ("datatype: " + node.logical_datatype);
        const std::string node_display_mode = node.display_mode.empty() ? "full" : ToLower(node.display_mode);
        if (node_display_mode != "header_only") {
            dc.DrawText(details, rect.x + 8, rect.y + 28);
        }
        if (is_mind_map) {
            const int children = child_count_by_parent.count(node.node_id) > 0U ? child_count_by_parent[node.node_id] : 0;
            dc.DrawText("children: " + std::to_string(children), rect.x + 8, rect.y + 46);
            const std::string mm = node.collapsed ? ("collapsed (" + std::to_string(node.stack_count) + ")")
                                                  : ("expanded (" + std::to_string(node.stack_count) + ")");
            dc.DrawText(mm, rect.x + 8, rect.y + 64);
        } else if (is_whiteboard) {
            std::string notes = node.notes;
            if (notes.size() > 42U) {
                notes = notes.substr(0, 39) + "...";
            }
            dc.DrawText(notes.empty() ? "notes: none" : ("notes: " + notes), rect.x + 8, rect.y + 46);
        } else if (is_silverston) {
            if (!node.icon_slot.empty()) {
                dc.DrawText("icon:" + node.icon_slot, rect.x + rect.width - 96, rect.y + 6);
            }
            if (!node.notes.empty()) {
                dc.DrawText("note*", rect.x + rect.width - 40, rect.y + 6);
            }
            if (node_display_mode == "summary" || node_display_mode == "full") {
                dc.DrawText("display:" + node_display_mode, rect.x + 8, rect.y + 46);
            }
            if (node_display_mode == "full" && !node.attributes.empty()) {
                const std::string attr = node.attributes.front();
                dc.DrawText("attr:" + attr.substr(0, std::min<std::size_t>(26, attr.size())), rect.x + 8, rect.y + 64);
            }
            if (node.stack_count > 1) {
                dc.DrawText("stack x" + std::to_string(node.stack_count), rect.x + 8, rect.y + 82);
            }
        }
    }

    if (!connect_source_node_id_.empty()) {
        dc.SetTextForeground(wxColour(0, 80, 170));
        dc.DrawText("Connect source: " + connect_source_node_id_ + " (Ctrl+Click target node)", 10, size.y - 24);
    } else {
        dc.SetTextForeground(wxColour(100, 100, 100));
        dc.DrawText("Click=select/drag  Border-drag=resize  Shift+Click=multi-select  Ctrl+Click=connect  Del/Shift+Del delete",
                    10, size.y - 24);
    }
}

void DiagramCanvasPanel::OnLeftDown(wxMouseEvent& event) {
    SetFocus();
    if (document_ == nullptr) {
        event.Skip();
        return;
    }

    beta1b::DiagramNode* node = HitTestNode(event.GetPosition());
    if (event.ControlDown() && node != nullptr) {
        selected_node_id_ = node->node_id;
        selected_node_ids_.insert(node->node_id);
        EmitSelection();
        if (connect_source_node_id_.empty()) {
            connect_source_node_id_ = node->node_id;
            EmitStatus("connect source selected " + node->node_id);
        } else if (connect_source_node_id_ == node->node_id) {
            connect_source_node_id_.clear();
            EmitStatus("connect mode cancelled");
        } else {
            std::string error;
            if (!ApplyConnect(connect_source_node_id_, node->node_id, &error)) {
                EmitStatus(error);
            }
            connect_source_node_id_.clear();
        }
        Refresh();
        return;
    }

    if (event.ShiftDown() && node != nullptr) {
        ToggleSelection(node->node_id);
        EmitSelection();
        Refresh();
        return;
    }

    if (node == nullptr) {
        selected_node_id_.clear();
        selected_node_ids_.clear();
        connect_source_node_id_.clear();
        EmitSelection();
        SetCursor(wxCursor(wxCURSOR_ARROW));
        Refresh();
        return;
    }

    const ResizeHandle resize_handle = ResizeHandleForPoint(*node, event.GetPosition());
    selected_node_id_ = node->node_id;
    if (resize_handle != ResizeHandle::None) {
        // Resize gesture always targets one node.
        selected_node_ids_.clear();
        selected_node_ids_.insert(node->node_id);
    } else {
        if (!event.ShiftDown()) {
            selected_node_ids_.clear();
        }
        selected_node_ids_.insert(node->node_id);
    }
    EmitSelection();
    connect_source_node_id_.clear();
    dragging_ = true;
    resizing_ = resize_handle != ResizeHandle::None;
    resize_handle_ = resize_handle;
    drag_anchor_ = event.GetPosition();
    drag_origin_x_ = node->x;
    drag_origin_y_ = node->y;
    drag_origin_width_ = node->width;
    drag_origin_height_ = node->height;
    drag_before_document_ = *document_;
    SetCursor(resizing_ ? CursorForResizeHandle(resize_handle_) : wxCursor(wxCURSOR_ARROW));
    CaptureMouse();
    Refresh();
}

void DiagramCanvasPanel::OnLeftUp(wxMouseEvent& event) {
    (void)event;
    if (!dragging_) {
        return;
    }
    if (HasCapture()) {
        ReleaseMouse();
    }

    dragging_ = false;
    if (document_ == nullptr || selected_node_id_.empty()) {
        resizing_ = false;
        resize_handle_ = ResizeHandle::None;
        SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    beta1b::DiagramNode* node = SelectedNode();
    if (node == nullptr) {
        resizing_ = false;
        resize_handle_ = ResizeHandle::None;
        SetCursor(wxCursor(wxCURSOR_ARROW));
        return;
    }

    try {
        if (diagram_service_ != nullptr) {
            diagram_service_->ApplyCanvasCommand(*document_,
                                                 resizing_ ? "resize" : "drag",
                                                 node->node_id,
                                                 node->parent_node_id);
        }
        std::string history_error;
        if (drag_before_document_.has_value()) {
            (void)PushHistory(*drag_before_document_, *document_, resizing_ ? "resize" : "drag", &history_error);
        }
        drag_before_document_.reset();
        EmitStatus((resizing_ ? "resized node " : "moved node ") + node->node_id);
        EmitMutation(resizing_ ? "resize" : "drag");
    } catch (const RejectError& ex) {
        if (drag_before_document_.has_value()) {
            *document_ = *drag_before_document_;
        } else {
            node->x = drag_origin_x_;
            node->y = drag_origin_y_;
            node->width = drag_origin_width_;
            node->height = drag_origin_height_;
        }
        drag_before_document_.reset();
        EmitStatus(ex.what());
    }
    resizing_ = false;
    resize_handle_ = ResizeHandle::None;
    SetCursor(wxCursor(wxCURSOR_ARROW));
    Refresh();
}

void DiagramCanvasPanel::OnMotion(wxMouseEvent& event) {
    if (!dragging_ || document_ == nullptr || selected_node_id_.empty() || !event.Dragging() || !event.LeftIsDown()) {
        if (document_ != nullptr) {
            beta1b::DiagramNode* hover_node = HitTestNode(event.GetPosition());
            if (hover_node != nullptr) {
                const ResizeHandle hover = ResizeHandleForPoint(*hover_node, event.GetPosition());
                SetCursor(CursorForResizeHandle(hover));
            } else {
                SetCursor(wxCursor(wxCURSOR_ARROW));
            }
        }
        event.Skip();
        return;
    }

    beta1b::DiagramNode* node = SelectedNode();
    if (node == nullptr) {
        return;
    }

    const int delta_x = ScaleToModel(event.GetPosition().x - drag_anchor_.x);
    const int delta_y = ScaleToModel(event.GetPosition().y - drag_anchor_.y);
    if (resizing_) {
        int new_x = drag_origin_x_;
        int new_y = drag_origin_y_;
        int new_width = drag_origin_width_;
        int new_height = drag_origin_height_;
        const bool adjust_left =
            resize_handle_ == ResizeHandle::W || resize_handle_ == ResizeHandle::NW || resize_handle_ == ResizeHandle::SW;
        const bool adjust_right =
            resize_handle_ == ResizeHandle::E || resize_handle_ == ResizeHandle::NE || resize_handle_ == ResizeHandle::SE;
        const bool adjust_top =
            resize_handle_ == ResizeHandle::N || resize_handle_ == ResizeHandle::NE || resize_handle_ == ResizeHandle::NW;
        const bool adjust_bottom =
            resize_handle_ == ResizeHandle::S || resize_handle_ == ResizeHandle::SE || resize_handle_ == ResizeHandle::SW;

        int right = drag_origin_x_ + drag_origin_width_;
        int bottom = drag_origin_y_ + drag_origin_height_;
        if (adjust_left) {
            new_x = drag_origin_x_ + delta_x;
        }
        if (adjust_right) {
            right = drag_origin_x_ + drag_origin_width_ + delta_x;
        }
        if (adjust_top) {
            new_y = drag_origin_y_ + delta_y;
        }
        if (adjust_bottom) {
            bottom = drag_origin_y_ + drag_origin_height_ + delta_y;
        }

        if (snap_to_grid_) {
            if (adjust_left) {
                new_x = Snap(new_x);
            }
            if (adjust_right) {
                right = Snap(right);
            }
            if (adjust_top) {
                new_y = Snap(new_y);
            }
            if (adjust_bottom) {
                bottom = Snap(bottom);
            }
        }

        new_width = right - new_x;
        new_height = bottom - new_y;
        const int min_w = 60;
        const int min_h = 40;
        if (new_width < min_w) {
            if (adjust_left && !adjust_right) {
                new_x = right - min_w;
            }
            new_width = min_w;
        }
        if (new_height < min_h) {
            if (adjust_top && !adjust_bottom) {
                new_y = bottom - min_h;
            }
            new_height = min_h;
        }

        node->x = new_x;
        node->y = new_y;
        node->width = new_width;
        node->height = new_height;
        if (snap_to_grid_) {
            node->x = Snap(node->x);
            node->y = Snap(node->y);
        }
    } else {
        for (auto& candidate : document_->nodes) {
            if (selected_node_ids_.empty()) {
                if (candidate.node_id != node->node_id) {
                    continue;
                }
            } else if (selected_node_ids_.count(candidate.node_id) == 0U) {
                continue;
            }
            if (candidate.node_id == node->node_id) {
                candidate.x = drag_origin_x_ + delta_x;
                candidate.y = drag_origin_y_ + delta_y;
            } else {
                // Preserve relative offsets when dragging a multi-selection.
                if (drag_before_document_.has_value()) {
                    const auto before_it =
                        std::find_if(drag_before_document_->nodes.begin(),
                                     drag_before_document_->nodes.end(),
                                     [&](const beta1b::DiagramNode& n) { return n.node_id == candidate.node_id; });
                    if (before_it != drag_before_document_->nodes.end()) {
                        candidate.x = before_it->x + delta_x;
                        candidate.y = before_it->y + delta_y;
                    }
                }
            }
            if (snap_to_grid_) {
                candidate.x = Snap(candidate.x);
                candidate.y = Snap(candidate.y);
            }
        }
    }
    Refresh();
}

void DiagramCanvasPanel::OnMouseWheel(wxMouseEvent& event) {
    if (event.GetWheelRotation() > 0) {
        ZoomIn();
    } else if (event.GetWheelRotation() < 0) {
        ZoomOut();
    }
}

void DiagramCanvasPanel::OnCharHook(wxKeyEvent& event) {
    std::string error;
    switch (event.GetKeyCode()) {
        case WXK_DELETE:
            if (!DeleteSelectedNode(event.ShiftDown(), &error)) {
                EmitStatus(error);
            }
            return;
        case WXK_LEFT:
            if (!NudgeSelectedNode(-10, 0, &error)) {
                EmitStatus(error);
            }
            return;
        case WXK_RIGHT:
            if (!NudgeSelectedNode(10, 0, &error)) {
                EmitStatus(error);
            }
            return;
        case WXK_UP:
            if (!NudgeSelectedNode(0, -10, &error)) {
                EmitStatus(error);
            }
            return;
        case WXK_DOWN:
            if (!NudgeSelectedNode(0, 10, &error)) {
                EmitStatus(error);
            }
            return;
        case WXK_RETURN:
        case WXK_NUMPAD_ENTER: {
            const auto* node = SelectedNode();
            if (node == nullptr || node->trace_refs.empty()) {
                EmitStatus("no trace reference on selected node");
                return;
            }
            EmitStatus("open trace ref " + node->trace_refs.front());
            EmitMutation("trace_open");
            return;
        }
        default: break;
    }

    if (event.ControlDown() && (event.GetKeyCode() == 'Z' || event.GetKeyCode() == 'z')) {
        if (!Undo(&error)) {
            EmitStatus(error);
        }
        return;
    }
    if (event.ControlDown() && (event.GetKeyCode() == 'Y' || event.GetKeyCode() == 'y')) {
        if (!Redo(&error)) {
            EmitStatus(error);
        }
        return;
    }
    if (event.ControlDown() && (event.GetKeyCode() == 'N' || event.GetKeyCode() == 'n')) {
        if (!AddNode(&error)) {
            EmitStatus(error);
        }
        return;
    }
    if (event.ControlDown() && (event.GetKeyCode() == 'R' || event.GetKeyCode() == 'r')) {
        if (!ReparentSelectedToNext(&error)) {
            EmitStatus(error);
        }
        return;
    }

    if (event.GetKeyCode() == '+' || event.GetKeyCode() == '=') {
        ZoomIn();
        return;
    }
    if (event.GetKeyCode() == '-') {
        ZoomOut();
        return;
    }
    if (event.GetKeyCode() == '0' && event.ControlDown()) {
        ZoomReset();
        return;
    }

    event.Skip();
}

beta1b::DiagramNode* DiagramCanvasPanel::SelectedNode() {
    return FindNode(selected_node_id_);
}

const beta1b::DiagramNode* DiagramCanvasPanel::FindNode(const std::string& node_id) const {
    if (document_ == nullptr) {
        return nullptr;
    }
    for (const auto& node : document_->nodes) {
        if (node.node_id == node_id) {
            return &node;
        }
    }
    return nullptr;
}

beta1b::DiagramNode* DiagramCanvasPanel::FindNode(const std::string& node_id) {
    if (document_ == nullptr) {
        return nullptr;
    }
    for (auto& node : document_->nodes) {
        if (node.node_id == node_id) {
            return &node;
        }
    }
    return nullptr;
}

beta1b::DiagramNode* DiagramCanvasPanel::HitTestNode(const wxPoint& screen_point) {
    if (document_ == nullptr) {
        return nullptr;
    }
    // Search in reverse so the most recently added node is treated as top-most.
    for (std::size_t i = document_->nodes.size(); i > 0U; --i) {
        auto& node = document_->nodes[i - 1U];
        if (ScreenRectForNode(node).Contains(screen_point)) {
            return &node;
        }
    }
    return nullptr;
}

bool DiagramCanvasPanel::IsSelected(const std::string& node_id) const {
    if (node_id.empty()) {
        return false;
    }
    if (selected_node_ids_.count(node_id) > 0U) {
        return true;
    }
    return selected_node_id_ == node_id;
}

void DiagramCanvasPanel::ToggleSelection(const std::string& node_id) {
    if (node_id.empty()) {
        return;
    }
    auto it = selected_node_ids_.find(node_id);
    if (it == selected_node_ids_.end()) {
        selected_node_ids_.insert(node_id);
        selected_node_id_ = node_id;
        EmitSelection();
        EmitStatus("selected " + node_id);
        return;
    }
    selected_node_ids_.erase(it);
    if (selected_node_id_ == node_id) {
        selected_node_id_ = selected_node_ids_.empty() ? std::string() : *selected_node_ids_.begin();
    }
    EmitSelection();
    EmitStatus("deselected " + node_id);
}

bool DiagramCanvasPanel::ApplyMove(int dx, int dy, std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    beta1b::DiagramNode* node = SelectedNode();
    if (node == nullptr) {
        SetError(error, "select a node to move");
        return false;
    }
    beta1b::DiagramDocument before = *document_;
    const std::set<std::string> move_ids = selected_node_ids_.empty() ? std::set<std::string>{node->node_id} : selected_node_ids_;
    try {
        for (const auto& id : move_ids) {
            const auto* target = FindNode(id);
            if (target == nullptr) {
                continue;
            }
            if (diagram_service_ != nullptr) {
                diagram_service_->ApplyCanvasCommand(*document_, "drag", id, target->parent_node_id);
            }
        }
        for (auto& target : document_->nodes) {
            if (move_ids.count(target.node_id) == 0U) {
                continue;
            }
            target.x += dx;
            target.y += dy;
            if (snap_to_grid_) {
                target.x = Snap(target.x);
                target.y = Snap(target.y);
            }
        }
        Refresh();
        if (!PushHistory(before, *document_, "move_node", error)) {
            return false;
        }
        EmitMutation("move_node");
        EmitStatus("moved " + std::to_string(move_ids.size()) + " node(s)");
        return true;
    } catch (const RejectError& ex) {
        SetError(error, ex.what());
        return false;
    }
}

bool DiagramCanvasPanel::ApplyResize(int dwidth, int dheight, std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    beta1b::DiagramNode* node = SelectedNode();
    if (node == nullptr) {
        SetError(error, "select a node to resize");
        return false;
    }
    beta1b::DiagramDocument before = *document_;
    try {
        if (diagram_service_ != nullptr) {
            diagram_service_->ApplyCanvasCommand(*document_, "resize", node->node_id, node->parent_node_id);
        }
        node->width = std::max(60, node->width + dwidth);
        node->height = std::max(40, node->height + dheight);
        if (snap_to_grid_) {
            node->width = std::max(60, Snap(node->width));
            node->height = std::max(40, Snap(node->height));
        }
        Refresh();
        if (!PushHistory(before, *document_, "resize_node", error)) {
            return false;
        }
        EmitMutation("resize_node");
        EmitStatus("resized node " + node->node_id);
        return true;
    } catch (const RejectError& ex) {
        SetError(error, ex.what());
        return false;
    }
}

bool DiagramCanvasPanel::ApplyConnect(const std::string& source_node_id,
                                      const std::string& target_node_id,
                                      std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    if (source_node_id.empty() || target_node_id.empty() || source_node_id == target_node_id) {
        SetError(error, "connector requires distinct source/target");
        return false;
    }
    if (FindNode(source_node_id) == nullptr || FindNode(target_node_id) == nullptr) {
        SetError(error, "connector target node not found");
        return false;
    }
    for (const auto& edge : document_->edges) {
        if (edge.from_node_id == source_node_id && edge.to_node_id == target_node_id) {
            SetError(error, "connector already exists");
            return false;
        }
    }

    beta1b::DiagramDocument before = *document_;
    try {
        if (diagram_service_ != nullptr) {
            diagram_service_->ApplyCanvasCommand(*document_, "connect", source_node_id, target_node_id);
        }
        beta1b::DiagramEdge edge;
        edge.edge_id = NextEdgeId();
        edge.from_node_id = source_node_id;
        edge.to_node_id = target_node_id;
        edge.relation_type = "link";
        edge.edge_type = "link";
        edge.label = "link";
        edge.source_cardinality = "1";
        edge.target_cardinality = "N";
        edge.directed = true;
        document_->edges.push_back(std::move(edge));
        Refresh();
        if (!PushHistory(before, *document_, "add_edge", error)) {
            return false;
        }
        EmitMutation("add_edge");
        EmitStatus("created connector " + source_node_id + " -> " + target_node_id);
        return true;
    } catch (const RejectError& ex) {
        SetError(error, ex.what());
        return false;
    }
}

bool DiagramCanvasPanel::ApplyReparent(const std::string& node_id, const std::string& new_parent_id, std::string* error) {
    if (document_ == nullptr) {
        SetError(error, "no active diagram loaded");
        return false;
    }
    beta1b::DiagramNode* node = FindNode(node_id);
    if (node == nullptr) {
        SetError(error, "selected node no longer exists");
        return false;
    }
    beta1b::DiagramDocument before = *document_;
    try {
        if (diagram_service_ != nullptr) {
            diagram_service_->ApplyCanvasCommand(*document_, "reparent", node_id, new_parent_id);
        }
        node->parent_node_id = new_parent_id;
        Refresh();
        if (!PushHistory(before, *document_, "reparent", error)) {
            return false;
        }
        EmitMutation("reparent");
        EmitStatus("reparented node " + node_id + " -> " + (new_parent_id.empty() ? "root" : new_parent_id));
        return true;
    } catch (const RejectError& ex) {
        SetError(error, ex.what());
        return false;
    }
}

bool DiagramCanvasPanel::PushHistory(const beta1b::DiagramDocument& before,
                                     const beta1b::DiagramDocument& after,
                                     const std::string& label,
                                     std::string* error) {
    try {
        if (beta1b::SerializeDiagramModel(before) == beta1b::SerializeDiagramModel(after)) {
            return true;
        }
    } catch (const RejectError& ex) {
        SetError(error, ex.what());
        return false;
    }
    redo_stack_.clear();
    undo_stack_.push_back(CanvasHistoryEntry{before, after, label});
    if (undo_stack_.size() > max_history_entries_) {
        undo_stack_.erase(undo_stack_.begin());
    }
    return true;
}

DiagramCanvasPanel::ResizeHandle DiagramCanvasPanel::ResizeHandleForPoint(const beta1b::DiagramNode& node,
                                                                           const wxPoint& screen_point) const {
    const wxRect rect = ScreenRectForNode(node);
    if (!rect.Contains(screen_point)) {
        return ResizeHandle::None;
    }
    const int tol = std::max(5, static_cast<int>(std::lround(6.0 * zoom_)));
    const int left = rect.x;
    const int top = rect.y;
    const int right = rect.x + rect.width;
    const int bottom = rect.y + rect.height;
    const bool near_left = std::abs(screen_point.x - left) <= tol;
    const bool near_right = std::abs(screen_point.x - right) <= tol;
    const bool near_top = std::abs(screen_point.y - top) <= tol;
    const bool near_bottom = std::abs(screen_point.y - bottom) <= tol;

    if (near_top && near_left) {
        return ResizeHandle::NW;
    }
    if (near_top && near_right) {
        return ResizeHandle::NE;
    }
    if (near_bottom && near_left) {
        return ResizeHandle::SW;
    }
    if (near_bottom && near_right) {
        return ResizeHandle::SE;
    }
    if (near_top) {
        return ResizeHandle::N;
    }
    if (near_bottom) {
        return ResizeHandle::S;
    }
    if (near_left) {
        return ResizeHandle::W;
    }
    if (near_right) {
        return ResizeHandle::E;
    }
    return ResizeHandle::None;
}

wxCursor DiagramCanvasPanel::CursorForResizeHandle(ResizeHandle handle) const {
    switch (handle) {
        case ResizeHandle::N:
        case ResizeHandle::S:
            return wxCursor(wxCURSOR_SIZENS);
        case ResizeHandle::E:
        case ResizeHandle::W:
            return wxCursor(wxCURSOR_SIZEWE);
        case ResizeHandle::NE:
        case ResizeHandle::SW:
            return wxCursor(wxCURSOR_SIZENESW);
        case ResizeHandle::NW:
        case ResizeHandle::SE:
            return wxCursor(wxCURSOR_SIZENWSE);
        case ResizeHandle::None:
        default:
            return wxCursor(wxCURSOR_ARROW);
    }
}

wxRect DiagramCanvasPanel::ScreenRectForNode(const beta1b::DiagramNode& node) const {
    return wxRect(static_cast<int>(std::lround(static_cast<double>(node.x) * zoom_)),
                  static_cast<int>(std::lround(static_cast<double>(node.y) * zoom_)),
                  std::max(30, static_cast<int>(std::lround(static_cast<double>(node.width) * zoom_))),
                  std::max(24, static_cast<int>(std::lround(static_cast<double>(node.height) * zoom_))));
}

wxPoint DiagramCanvasPanel::NodeCenter(const beta1b::DiagramNode& node) const {
    const wxRect rect = ScreenRectForNode(node);
    return wxPoint(rect.x + (rect.width / 2), rect.y + (rect.height / 2));
}

int DiagramCanvasPanel::Snap(int value) const {
    if (grid_size_ <= 0) {
        return value;
    }
    const int rem = value % grid_size_;
    if (rem == 0) {
        return value;
    }
    if (rem >= grid_size_ / 2) {
        return value + (grid_size_ - rem);
    }
    return value - rem;
}

int DiagramCanvasPanel::ScaleToModel(int value) const {
    if (zoom_ <= 0.0) {
        return value;
    }
    return static_cast<int>(std::lround(static_cast<double>(value) / zoom_));
}

std::string DiagramCanvasPanel::NextNodeId() const {
    std::set<std::string> used;
    if (document_ != nullptr) {
        for (const auto& node : document_->nodes) {
            used.insert(node.node_id);
        }
    }
    std::size_t ordinal = used.size() + 1U;
    while (true) {
        const std::string candidate = "n" + std::to_string(ordinal);
        if (used.count(candidate) == 0U) {
            return candidate;
        }
        ++ordinal;
    }
}

std::string DiagramCanvasPanel::NextEdgeId() const {
    std::set<std::string> used;
    if (document_ != nullptr) {
        for (const auto& edge : document_->edges) {
            used.insert(edge.edge_id);
        }
    }
    std::size_t ordinal = used.size() + 1U;
    while (true) {
        const std::string candidate = "e" + std::to_string(ordinal);
        if (used.count(candidate) == 0U) {
            return candidate;
        }
        ++ordinal;
    }
}

void DiagramCanvasPanel::EmitMutation(const std::string& mutation_kind) const {
    if (mutation_sink_) {
        mutation_sink_(mutation_kind);
    }
}

void DiagramCanvasPanel::EmitSelection() const {
    if (!selection_sink_) {
        return;
    }
    const beta1b::DiagramNode* node = FindNode(selected_node_id_);
    if (node == nullptr) {
        selection_sink_("", "", "", "", false);
        return;
    }
    const bool chamfer_notes = node->notes.find("[chamfer]") != std::string::npos;
    selection_sink_(node->node_id,
                    node->object_type,
                    node->icon_slot,
                    node->display_mode,
                    chamfer_notes);
}

void DiagramCanvasPanel::EmitStatus(const std::string& message) const {
    if (status_sink_) {
        status_sink_(message);
    }
}

void DiagramCanvasPanel::SetError(std::string* error, const std::string& message) const {
    if (error != nullptr) {
        *error = message;
    }
}

}  // namespace scratchrobin::gui
