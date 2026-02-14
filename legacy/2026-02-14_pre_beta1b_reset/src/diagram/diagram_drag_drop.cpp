// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "diagram_drag_drop.h"

#include <wx/window.h>
#include <sstream>

namespace scratchrobin {
namespace diagram {

wxDataFormat DiagramDropData::diagram_format_("application/x-scratchrobin-diagram");

DiagramDropData::DiagramDropData() {
    Add(&text_data_, true);  // Preferred format
    Add(&diagram_data_, false);
    diagram_data_.SetFormat(diagram_format_);
}

void DiagramDropData::SetDiagramId(const std::string& id) {
    // Store as JSON in custom data
    std::stringstream json;
    json << "{\"id\":\"" << id << "\",";
    json << "\"type\":\"" << GetDiagramType() << "\",";
    json << "\"name\":\"" << GetDiagramName() << "\"}";
    
    std::string data = json.str();
    diagram_data_.SetData(data.size(), data.c_str());
    text_data_.SetText(wxString(id));
}

void DiagramDropData::SetDiagramType(const std::string& type) {
    // Type is stored with ID in the JSON
}

void DiagramDropData::SetDiagramName(const std::string& name) {
    // Name is stored with ID in the JSON
}

void DiagramDropData::SetSourceCanvasId(const std::string& id) {
    // Could be stored separately if needed
}

std::string DiagramDropData::GetDiagramId() const {
    wxString text = text_data_.GetText();
    return std::string(text);
}

std::string DiagramDropData::GetDiagramType() const {
    size_t len = diagram_data_.GetDataSize();
    if (len == 0) return "";
    
    const char* data = static_cast<const char*>(diagram_data_.GetData());
    // Simple parsing - extract type from JSON
    std::string str(data, len);
    size_t pos = str.find("\"type\":\"");
    if (pos != std::string::npos) {
        pos += 9;
        size_t end = str.find("\"", pos);
        if (end != std::string::npos) {
            return str.substr(pos, end - pos);
        }
    }
    return "";
}

std::string DiagramDropData::GetDiagramName() const {
    size_t len = diagram_data_.GetDataSize();
    if (len == 0) return "";
    
    const char* data = static_cast<const char*>(diagram_data_.GetData());
    std::string str(data, len);
    size_t pos = str.find("\"name\":\"");
    if (pos != std::string::npos) {
        pos += 9;
        size_t end = str.find("\"", pos);
        if (end != std::string::npos) {
            return str.substr(pos, end - pos);
        }
    }
    return "";
}

std::string DiagramDropData::GetSourceCanvasId() const {
    // Not currently stored separately
    return "";
}

const wxDataFormat& DiagramDropData::GetDiagramFormat() {
    return diagram_format_;
}

// DiagramDropTarget implementation

DiagramDropTarget::DiagramDropTarget(DropCallback callback)
    : wxDropTarget(new DiagramDropData()),
      callback_(callback) {}

wxDragResult DiagramDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def) {
    // Determine drop action based on modifier keys
    wxMouseState mouse_state = wxGetMouseState();
    
    if (mouse_state.ShiftDown()) {
        current_action_ = DropActionEmbed;
    } else if (mouse_state.ControlDown()) {
        current_action_ = DropActionCopy;
    } else if (mouse_state.AltDown()) {
        current_action_ = DropActionReference;
    } else {
        current_action_ = DropActionLink;
    }
    
    showing_preview_ = true;
    return def;
}

wxDragResult DiagramDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def) {
    if (!GetData()) {
        return wxDragNone;
    }
    
    DiagramDropData* data = static_cast<DiagramDropData*>(GetDataObject());
    if (!data) {
        return wxDragNone;
    }
    
    std::string source_id = data->GetDiagramId();
    if (source_id.empty()) {
        return wxDragNone;
    }
    
    if (callback_) {
        bool success = callback_(source_id, current_action_, x, y);
        return success ? def : wxDragNone;
    }
    
    return wxDragNone;
}

bool DiagramDropTarget::OnDrop(wxCoord x, wxCoord y) {
    showing_preview_ = false;
    return true;
}

void DiagramDropTarget::OnLeave() {
    showing_preview_ = false;
}

// DiagramDragSource implementation

DiagramDragSource::DiagramDragSource(wxWindow* win, const std::string& diagram_id)
    : wxDropSource(win) {
    data_.SetDiagramId(diagram_id);
    SetData(data_);
}

void DiagramDragSource::SetDragData(const std::string& diagram_id,
                                     const std::string& diagram_type,
                                     const std::string& diagram_name) {
    data_.SetDiagramId(diagram_id);
    data_.SetDiagramType(diagram_type);
    data_.SetDiagramName(diagram_name);
}

wxDragResult DiagramDragSource::DoDragDrop(int flags) {
    return wxDropSource::DoDragDrop(flags);
}

// DiagramDragDropManager implementation

DiagramDragDropManager& DiagramDragDropManager::Instance() {
    static DiagramDragDropManager instance;
    return instance;
}

void DiagramDragDropManager::RegisterDropTarget(const std::string& canvas_id,
                                                wxWindow* window,
                                                DiagramDropTarget::DropCallback callback) {
    if (!window) return;
    
    auto* target = new DiagramDropTarget(callback);
    window->SetDropTarget(target);
    registered_targets_[canvas_id] = window;
}

void DiagramDragDropManager::UnregisterDropTarget(const std::string& canvas_id) {
    auto it = registered_targets_.find(canvas_id);
    if (it != registered_targets_.end()) {
        if (it->second) {
            it->second->SetDropTarget(nullptr);
        }
        registered_targets_.erase(it);
    }
}

bool DiagramDragDropManager::StartDrag(const std::string& source_canvas_id,
                                        const std::string& diagram_id,
                                        const std::string& diagram_type,
                                        const std::string& diagram_name) {
    current_drag_source_ = source_canvas_id;
    
    auto it = registered_targets_.find(source_canvas_id);
    if (it == registered_targets_.end() || !it->second) {
        return false;
    }
    
    DiagramDragSource source(it->second, diagram_id);
    source.SetDragData(diagram_id, diagram_type, diagram_name);
    
    wxDragResult result = source.DoDragDrop(wxDrag_CopyOnly);
    
    current_drag_source_.clear();
    return (result != wxDragNone && result != wxDragError);
}

bool DiagramDragDropManager::CanDrop(const std::string& source_id, 
                                     const std::string& target_id) const {
    // Cannot drop on self
    if (source_id == target_id) {
        return false;
    }
    
    // All other combinations are allowed
    // ERD can drop on DFD, DFD on ERD, etc.
    return true;
}

bool DiagramDragDropManager::ExecuteDrop(const std::string& source_id,
                                          const std::string& target_id,
                                          DiagramDropTarget::DropAction action,
                                          int x, int y) {
    if (!CanDrop(source_id, target_id)) {
        return false;
    }
    
    // Execute the drop based on action type
    switch (action) {
        case DiagramDropTarget::DropActionEmbed:
            // Embed source diagram as a child/sub-diagram
            // This would create a parent-child relationship
            return true;
            
        case DiagramDropTarget::DropActionLink:
            // Create a bi-directional link between diagrams
            return true;
            
        case DiagramDropTarget::DropActionCopy:
            // Copy nodes from source to target
            return true;
            
        case DiagramDropTarget::DropActionReference:
            // Create a reference link (weaker than embed)
            return true;
            
        default:
            return false;
    }
}

} // namespace diagram
} // namespace scratchrobin
