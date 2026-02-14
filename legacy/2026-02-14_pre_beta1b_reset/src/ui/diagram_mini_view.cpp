/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "diagram_mini_view.h"

#include "diagram_page.h"

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>

namespace scratchrobin {
namespace ui {

wxBEGIN_EVENT_TABLE(DiagramMiniView, wxPanel)
    EVT_PAINT(DiagramMiniView::OnPaint)
    EVT_ENTER_WINDOW(DiagramMiniView::OnMouseEnter)
    EVT_LEAVE_WINDOW(DiagramMiniView::OnMouseLeave)
    EVT_LEFT_DOWN(DiagramMiniView::OnLeftDown)
    EVT_LEFT_UP(DiagramMiniView::OnLeftUp)
    EVT_MOTION(DiagramMiniView::OnMotion)
    EVT_LEFT_DCLICK(DiagramMiniView::OnLeftDClick)
    EVT_ERASE_BACKGROUND(DiagramMiniView::OnEraseBackground)
wxEND_EVENT_TABLE()

DiagramMiniView::DiagramMiniView(wxWindow* parent,
                                  DiagramPage* sourcePage,
                                  const std::string& diagramId,
                                  const wxString& title,
                                  const Callbacks& callbacks)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, 
              wxSize(DEFAULT_WIDTH, DEFAULT_HEIGHT)),
      sourcePage_(sourcePage),
      diagramId_(diagramId),
      title_(title),
      callbacks_(callbacks) {
    
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    SetCursor(wxCursor(wxCURSOR_HAND));
}

DiagramMiniView::~DiagramMiniView() = default;

void DiagramMiniView::RefreshThumbnail() {
    thumbnailDirty_ = true;
    Refresh();
}

void DiagramMiniView::SetDiagramPosition(int x, int y) {
    diagramX_ = x;
    diagramY_ = y;
}

void DiagramMiniView::GetDiagramPosition(int& x, int& y) const {
    x = diagramX_;
    y = diagramY_;
}

wxBitmap DiagramMiniView::GenerateThumbnail() {
    if (!sourcePage_) {
        return wxBitmap();
    }
    
    // Create bitmap for thumbnail
    wxBitmap thumb(DEFAULT_WIDTH - 2 * BORDER_SIZE, 
                   DEFAULT_HEIGHT - TITLE_HEIGHT - 2 * BORDER_SIZE);
    
    wxMemoryDC dc(thumb);
    
    // Fill background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    // Draw simplified diagram representation
    // In a full implementation, this would render the actual diagram scaled down
    // For now, draw a placeholder with some visual interest
    
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        // Draw some rectangles representing tables
        gc->SetPen(wxPen(wxColour(100, 100, 100), 1));
        gc->SetBrush(wxBrush(wxColour(200, 220, 255)));
        
        // Table 1
        gc->DrawRectangle(5, 5, 40, 25);
        
        // Table 2
        gc->DrawRectangle(60, 10, 35, 20);
        
        // Table 3
        gc->DrawRectangle(30, 40, 45, 25);
        
        // Connection lines
        gc->SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_DOT));
        gc->StrokeLine(45, 17, 60, 20);
        gc->StrokeLine(52, 30, 52, 40);
        
        delete gc;
    }
    
    dc.SelectObject(wxNullBitmap);
    
    return thumb;
}

void DiagramMiniView::DrawPlaceholder(wxDC& dc) {
    wxRect clientRect = GetClientRect();
    
    // Title bar area
    wxRect titleRect(clientRect.x, clientRect.y, 
                     clientRect.width, TITLE_HEIGHT);
    
    // Draw title background
    dc.SetBrush(wxBrush(wxColour(230, 230, 230)));
    dc.SetPen(wxPen(wxColour(180, 180, 180)));
    dc.DrawRectangle(titleRect);
    
    // Draw title text
    dc.SetTextForeground(wxColour(50, 50, 50));
    wxFont titleFont = dc.GetFont();
    titleFont.SetPointSize(8);
    dc.SetFont(titleFont);
    
    wxString displayTitle = title_;
    if (displayTitle.Length() > 15) {
        displayTitle = displayTitle.Left(12) + "...";
    }
    dc.DrawText(displayTitle, titleRect.x + 4, titleRect.y + 2);
    
    // Content area
    wxRect contentRect(clientRect.x + BORDER_SIZE,
                       clientRect.y + TITLE_HEIGHT,
                       clientRect.width - 2 * BORDER_SIZE,
                       clientRect.height - TITLE_HEIGHT - BORDER_SIZE);
    
    // Draw border
    dc.SetPen(wxPen(isHovered_ ? wxColour(0, 120, 215) : wxColour(180, 180, 180), 
                    isHovered_ ? 2 : 1));
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawRectangle(contentRect);
    
    // Draw thumbnail or placeholder
    if (thumbnailDirty_ || !thumbnail_.IsOk()) {
        thumbnail_ = GenerateThumbnail();
        thumbnailDirty_ = false;
    }
    
    if (thumbnail_.IsOk()) {
        dc.DrawBitmap(thumbnail_, contentRect.x, contentRect.y);
    } else {
        // Draw simple placeholder
        dc.SetTextForeground(wxColour(150, 150, 150));
        dc.DrawLabel("Diagram", contentRect, wxALIGN_CENTER);
    }
    
    // Draw resize handle if hovered
    if (isHovered_) {
        dc.SetPen(wxPen(wxColour(0, 120, 215), 1));
        int handleX = clientRect.GetRight() - 8;
        int handleY = clientRect.GetBottom() - 8;
        dc.DrawLine(handleX, clientRect.GetBottom() - 3, 
                    clientRect.GetRight() - 3, handleY);
        dc.DrawLine(handleX + 3, clientRect.GetBottom() - 3, 
                    clientRect.GetRight() - 3, handleY + 3);
    }
}

void DiagramMiniView::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    DrawPlaceholder(dc);
}

void DiagramMiniView::OnEraseBackground(wxEraseEvent& event) {
    // Prevent flicker by handling background erase
}

void DiagramMiniView::OnMouseEnter(wxMouseEvent& event) {
    isHovered_ = true;
    Refresh();
}

void DiagramMiniView::OnMouseLeave(wxMouseEvent& event) {
    isHovered_ = false;
    isDragging_ = false;
    Refresh();
}

void DiagramMiniView::OnLeftDown(wxMouseEvent& event) {
    SetFocus();
    isDragging_ = true;
    dragStart_ = event.GetPosition();
    dragStartPos_ = wxPoint(diagramX_, diagramY_);
    CaptureMouse();
}

void DiagramMiniView::OnLeftUp(wxMouseEvent& event) {
    if (isDragging_) {
        isDragging_ = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
}

void DiagramMiniView::OnMotion(wxMouseEvent& event) {
    if (isDragging_ && event.Dragging()) {
        wxPoint pos = event.GetPosition();
        int dx = pos.x - dragStart_.x;
        int dy = pos.y - dragStart_.y;
        
        // Update position
        diagramX_ = dragStartPos_.x + dx;
        diagramY_ = dragStartPos_.y + dy;
        
        // Notify parent
        if (callbacks_.onDrag) {
            callbacks_.onDrag(dx, dy);
        }
        
        Refresh();
    }
}

void DiagramMiniView::OnLeftDClick(wxMouseEvent& event) {
    if (callbacks_.onDoubleClick) {
        callbacks_.onDoubleClick();
    }
}

} // namespace ui
} // namespace scratchrobin
