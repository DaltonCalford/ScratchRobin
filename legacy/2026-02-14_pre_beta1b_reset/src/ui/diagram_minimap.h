/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_MINIMAP_H
#define SCRATCHROBIN_DIAGRAM_MINIMAP_H

#include <wx/panel.h>

namespace scratchrobin {

class DiagramCanvas;
class DiagramModel;

// Minimap/Navigation panel for diagram overview
class DiagramMinimap : public wxPanel {
public:
    DiagramMinimap(wxWindow* parent, DiagramCanvas* canvas);

    // Update the minimap view
    void UpdateView();
    
    // Set the canvas to monitor
    void SetCanvas(DiagramCanvas* canvas);

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    
    // Calculate viewport rectangle in minimap coordinates
    wxRect CalculateViewportRect() const;
    
    // Convert minimap point to canvas world coordinates
    wxPoint MinimapToCanvas(int x, int y) const;
    
    // Get the bounds of all nodes in the diagram
    void GetDiagramBounds(double& minX, double& minY, double& maxX, double& maxY) const;

    DiagramCanvas* canvas_ = nullptr;
    bool is_dragging_ = false;
    wxPoint drag_start_;
    
    // Cached bounds
    double min_x_ = 0, min_y_ = 0, max_x_ = 0, max_y_ = 0;
    double scale_ = 1.0;
    int offset_x_ = 0, offset_y_ = 0;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_MINIMAP_H
