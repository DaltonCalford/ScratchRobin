// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include <wx/dnd.h>
#include <wx/window.h>
#include <functional>
#include <map>
#include <string>

namespace scratchrobin {
namespace diagram {

/**
 * @brief Drag-and-drop data for diagram entities
 */
class DiagramDropData : public wxDataObjectComposite {
public:
    DiagramDropData();
    
    void SetDiagramId(const std::string& id);
    void SetDiagramType(const std::string& type);
    void SetDiagramName(const std::string& name);
    void SetSourceCanvasId(const std::string& id);
    
    std::string GetDiagramId() const;
    std::string GetDiagramType() const;
    std::string GetDiagramName() const;
    std::string GetSourceCanvasId() const;
    
    static const wxDataFormat& GetDiagramFormat();
    
private:
    wxTextDataObject text_data_;
    wxCustomDataObject diagram_data_;
    
    static wxDataFormat diagram_format_;
};

/**
 * @brief Drop target for accepting diagram drops
 */
class DiagramDropTarget : public wxDropTarget {
public:
    enum DropAction {
        DropActionNone = 0,
        DropActionEmbed = 1,      // Embed source diagram as child
        DropActionLink = 2,       // Create link between diagrams
        DropActionCopy = 3,       // Copy nodes from source
        DropActionReference = 4   // Create reference (non-owned link)
    };
    
    using DropCallback = std::function<bool(const std::string& source_id, 
                                            DropAction action,
                                            int x, int y)>;
    
    explicit DiagramDropTarget(DropCallback callback);
    
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def) override;
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) override;
    bool OnDrop(wxCoord x, wxCoord y) override;
    void OnLeave() override;
    
private:
    DropCallback callback_;
    DropAction current_action_ = DropActionNone;
    bool showing_preview_ = false;
};

/**
 * @brief Drag source for initiating diagram drags
 */
class DiagramDragSource : public wxDropSource {
public:
    DiagramDragSource(wxWindow* win, const std::string& diagram_id);
    
    void SetDragData(const std::string& diagram_id,
                     const std::string& diagram_type,
                     const std::string& diagram_name);
    
    wxDragResult DoDragDrop(int flags = wxDrag_CopyOnly) override;
    
private:
    DiagramDropData data_;
};

/**
 * @brief Handles cross-diagram drag and drop operations
 */
class DiagramDragDropManager {
public:
    static DiagramDragDropManager& Instance();
    
    // Register a canvas as a drop target
    void RegisterDropTarget(const std::string& canvas_id, wxWindow* window, 
                            DiagramDropTarget::DropCallback callback);
    void UnregisterDropTarget(const std::string& canvas_id);
    
    // Start a drag operation
    bool StartDrag(const std::string& source_canvas_id,
                   const std::string& diagram_id,
                   const std::string& diagram_type,
                   const std::string& diagram_name);
    
    // Check if a drop is valid
    bool CanDrop(const std::string& source_id, const std::string& target_id) const;
    
    // Execute a drop operation
    bool ExecuteDrop(const std::string& source_id, 
                     const std::string& target_id,
                     DiagramDropTarget::DropAction action,
                     int x = 0, int y = 0);
    
private:
    DiagramDragDropManager() = default;
    
    std::map<std::string, wxWindow*> registered_targets_;
    std::string current_drag_source_;
};

} // namespace diagram
} // namespace scratchrobin
