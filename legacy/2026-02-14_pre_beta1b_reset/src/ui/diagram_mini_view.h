/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_DIAGRAM_MINI_VIEW_H
#define SCRATCHROBIN_DIAGRAM_MINI_VIEW_H

#include <memory>
#include <string>

#include <wx/panel.h>
#include <wx/bitmap.h>

namespace scratchrobin {

class DiagramPage;

namespace ui {

/**
 * @brief A thumbnail/mini-view of a diagram for display within another diagram.
 * 
 * Shows a scaled-down representation of a child diagram, with:
 * - Real-time or cached thumbnail rendering
 * - Hover effects
 * - Double-click to open
 * - Drag to reposition within parent
 */
class DiagramMiniView : public wxPanel {
public:
    /**
     * @brief Callback for mini-view events.
     */
    struct Callbacks {
        std::function<void()> onDoubleClick;      // Open the diagram
        std::function<void()> onDelete;           // Remove from parent
        std::function<void(int dx, int dy)> onDrag;  // Move within parent
    };

    DiagramMiniView(wxWindow* parent,
                    DiagramPage* sourcePage,
                    const std::string& diagramId,
                    const wxString& title,
                    const Callbacks& callbacks);
    
    ~DiagramMiniView() override;

    /**
     * @brief Update the thumbnail from the source diagram.
     */
    void RefreshThumbnail();
    
    /**
     * @brief Get the diagram ID this mini-view represents.
     */
    std::string GetDiagramId() const { return diagramId_; }
    
    /**
     * @brief Set the position within the parent diagram.
     */
    void SetDiagramPosition(int x, int y);
    void GetDiagramPosition(int& x, int& y) const;

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMotion(wxMouseEvent& event);
    void OnLeftDClick(wxMouseEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    wxBitmap GenerateThumbnail();
    void DrawPlaceholder(wxDC& dc);

    DiagramPage* sourcePage_ = nullptr;
    std::string diagramId_;
    wxString title_;
    Callbacks callbacks_;
    
    wxBitmap thumbnail_;
    bool thumbnailDirty_ = true;
    bool isHovered_ = false;
    bool isDragging_ = false;
    wxPoint dragStart_;
    wxPoint dragStartPos_;
    
    // Position within parent diagram
    int diagramX_ = 0;
    int diagramY_ = 0;
    
    static constexpr int DEFAULT_WIDTH = 120;
    static constexpr int DEFAULT_HEIGHT = 90;
    static constexpr int BORDER_SIZE = 2;
    static constexpr int TITLE_HEIGHT = 18;

    wxDECLARE_EVENT_TABLE();
};

} // namespace ui
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_MINI_VIEW_H
