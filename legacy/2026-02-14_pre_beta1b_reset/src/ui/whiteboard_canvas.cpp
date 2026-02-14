/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "whiteboard_canvas.h"

#include <cmath>
#include <ctime>
#include <sstream>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/textdlg.h>

namespace scratchrobin {

// ============================================================================
// Type Helpers
// ============================================================================
std::string WhiteboardObjectTypeToString(WhiteboardObjectType type) {
    switch (type) {
        case WhiteboardObjectType::DATABASE: return "Database";
        case WhiteboardObjectType::SCHEMA: return "Schema";
        case WhiteboardObjectType::TABLE: return "Table";
        case WhiteboardObjectType::VIEW: return "View";
        case WhiteboardObjectType::PROCEDURE: return "Procedure";
        case WhiteboardObjectType::FUNCTION: return "Function";
        case WhiteboardObjectType::TRIGGER: return "Trigger";
        case WhiteboardObjectType::INDEX: return "Index";
        case WhiteboardObjectType::DATASTORE: return "Datastore";
        case WhiteboardObjectType::SERVER: return "Server";
        case WhiteboardObjectType::CLUSTER: return "Cluster";
        case WhiteboardObjectType::GENERIC: return "Generic";
        default: return "Unknown";
    }
}

wxColour GetTypeColor(WhiteboardObjectType type) {
    switch (type) {
        case WhiteboardObjectType::DATABASE: return wxColour(100, 149, 237);  // Cornflower blue
        case WhiteboardObjectType::SCHEMA:   return wxColour(144, 238, 144);  // Light green
        case WhiteboardObjectType::TABLE:    return wxColour(255, 218, 185);  // Peach
        case WhiteboardObjectType::VIEW:     return wxColour(221, 160, 221);  // Plum
        case WhiteboardObjectType::PROCEDURE: return wxColour(255, 255, 224); // Light yellow
        case WhiteboardObjectType::FUNCTION: return wxColour(173, 216, 230);  // Light blue
        case WhiteboardObjectType::TRIGGER:  return wxColour(255, 182, 193);  // Light pink
        case WhiteboardObjectType::INDEX:    return wxColour(192, 192, 192);  // Silver
        case WhiteboardObjectType::DATASTORE: return wxColour(210, 180, 140); // Tan
        case WhiteboardObjectType::SERVER:   return wxColour(70, 130, 180);   // Steel blue
        case WhiteboardObjectType::CLUSTER:  return wxColour(60, 179, 113);   // Medium sea green
        case WhiteboardObjectType::GENERIC:  return wxColour(240, 240, 240);  // Light gray
        default: return wxColour(200, 200, 200);
    }
}

// ============================================================================
// TypedObject Implementation
// ============================================================================
TypedObject::TypedObject() : object_type(WhiteboardObjectType::GENERIC) {
    header_color = GetTypeColor(object_type);
    body_color = wxColour(255, 255, 255);
    text_color = wxColour(0, 0, 0);
}

TypedObject::TypedObject(WhiteboardObjectType type, const std::string& name) 
    : object_type(type), name(name) {
    header_color = GetTypeColor(type);
    body_color = wxColour(255, 255, 255);
    text_color = wxColour(0, 0, 0);
    details = GetDefaultDetails();
}

void TypedObject::SetType(WhiteboardObjectType type) {
    object_type = type;
    header_color = GetTypeColor(type);
}

void TypedObject::SetName(const std::string& new_name) {
    name = new_name;
}

void TypedObject::SetDetails(const std::string& new_details) {
    details = new_details;
}

std::string TypedObject::GetDefaultDetails() const {
    switch (object_type) {
        case WhiteboardObjectType::TABLE:
            return "Columns:\n- id (PK)\n- name\n- created_at";
        case WhiteboardObjectType::DATABASE:
            return "Connection:\nHost: localhost\nPort: 5432";
        case WhiteboardObjectType::DATASTORE:
            return "Location:\nPath: /data/storage";
        case WhiteboardObjectType::SERVER:
            return "Spec:\nCPU: 4 cores\nRAM: 16GB";
        default:
            return "";
    }
}

bool TypedObject::IsDatabaseObject() const {
    return object_type == WhiteboardObjectType::TABLE ||
           object_type == WhiteboardObjectType::VIEW ||
           object_type == WhiteboardObjectType::PROCEDURE ||
           object_type == WhiteboardObjectType::FUNCTION ||
           object_type == WhiteboardObjectType::TRIGGER ||
           object_type == WhiteboardObjectType::INDEX;
}

bool TypedObject::IsContainer() const {
    return object_type == WhiteboardObjectType::DATABASE ||
           object_type == WhiteboardObjectType::SCHEMA;
}

// ============================================================================
// Whiteboard Canvas Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(WhiteboardCanvas, wxScrolledCanvas)
    EVT_PAINT(WhiteboardCanvas::OnPaint)
    EVT_MOUSE_EVENTS(WhiteboardCanvas::OnMouseEvent)
    EVT_KEY_DOWN(WhiteboardCanvas::OnKeyEvent)
    EVT_SIZE(WhiteboardCanvas::OnSize)
wxEND_EVENT_TABLE()

WhiteboardCanvas::WhiteboardCanvas(wxWindow* parent, wxWindowID id)
    : wxScrolledCanvas(parent, id, wxDefaultPosition, wxDefaultSize,
                       wxHSCROLL | wxVSCROLL | wxFULL_REPAINT_ON_RESIZE) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(240, 240, 240));
    
    document_ = std::make_unique<WhiteboardDocument>();
    
    SetVirtualSize(document_->page_size);
    SetScrollRate(20, 20);
}

WhiteboardCanvas::~WhiteboardCanvas() = default;

void WhiteboardCanvas::SetTool(Tool tool) {
    current_tool_ = tool;
    SetCursor(tool == Tool::PAN ? wxCURSOR_HAND : wxCURSOR_ARROW);
}

void WhiteboardCanvas::SetObjectTypeForNextCreation(WhiteboardObjectType type) {
    next_object_type_ = type;
}

void WhiteboardCanvas::NewDocument() {
    document_ = std::make_unique<WhiteboardDocument>();
    Refresh();
}

bool WhiteboardCanvas::LoadDocument(const std::string& path) {
    document_ = WhiteboardDocument::LoadFromFile(path);
    if (document_) {
        Refresh();
        return true;
    }
    return false;
}

bool WhiteboardCanvas::SaveDocument(const std::string& path) {
    if (document_) {
        document_->SaveToFile(path);
        return true;
    }
    return false;
}

void WhiteboardCanvas::AddTypedObject(const wxPoint& position, WhiteboardObjectType type, 
                                       const std::string& name) {
    auto obj = std::make_unique<WhiteboardObject>(WhiteboardObject::Type::TYPED_OBJECT);
    obj->MakeTypedObject(type, name);
    
    // Set default size
    obj->bounds = wxRect(position.x - 75, position.y - 50, 150, 100);
    
    AddObject(std::move(obj));
}

void WhiteboardCanvas::AddTypedObject(const wxPoint& position, const TypedObject& template_obj) {
    auto obj = std::make_unique<WhiteboardObject>(WhiteboardObject::Type::TYPED_OBJECT);
    obj->typed_data = std::make_unique<TypedObject>(template_obj);
    obj->typed_data->SetName(template_obj.name);
    obj->typed_data->SetDetails(template_obj.details);
    
    // Calculate size based on content
    int width = 150;
    int height = 100;
    
    // Count lines in details for height estimation
    int lines = 1;
    for (char c : template_obj.details) {
        if (c == '\n') lines++;
    }
    height = std::max(100, 30 + lines * 14 + 16);
    
    obj->bounds = wxRect(position.x - width/2, position.y - height/2, width, height);
    
    AddObject(std::move(obj));
}

void WhiteboardCanvas::AddObject(std::unique_ptr<WhiteboardObject> obj) {
    if (document_) {
        document_->AddObject(std::move(obj));
        Refresh();
    }
}

void WhiteboardCanvas::RemoveObject(const std::string& id) {
    if (document_) {
        document_->RemoveObject(id);
        Refresh();
    }
}

void WhiteboardCanvas::ClearSelection() {
    if (document_) {
        for (auto& obj : document_->objects) {
            obj->selected = false;
        }
        Refresh();
    }
}

std::vector<WhiteboardObject*> WhiteboardCanvas::GetSelectedObjects() const {
    std::vector<WhiteboardObject*> selected;
    if (document_) {
        for (auto& obj : document_->objects) {
            if (obj->selected) {
                selected.push_back(obj.get());
            }
        }
    }
    return selected;
}

WhiteboardObject* WhiteboardCanvas::GetObjectAt(const wxPoint& pt) const {
    if (!document_) return nullptr;
    
    // Search in reverse order (topmost first)
    for (auto it = document_->objects.rbegin(); it != document_->objects.rend(); ++it) {
        if ((*it)->HitTest(pt)) {
            return it->get();
        }
    }
    return nullptr;
}

void WhiteboardCanvas::EditSelectedObjectName() {
    auto selected = GetSelectedObjects();
    if (selected.size() == 1 && selected[0]->IsTypedObject()) {
        selected[0]->StartNameEdit();
    }
}

void WhiteboardCanvas::EditSelectedObjectDetails() {
    auto selected = GetSelectedObjects();
    if (selected.size() == 1 && selected[0]->IsTypedObject()) {
        selected[0]->StartDetailsEdit();
    }
}

void WhiteboardCanvas::ZoomIn() {
    zoom_scale_ *= 1.2;
    if (zoom_scale_ > 5.0) zoom_scale_ = 5.0;
    Refresh();
}

void WhiteboardCanvas::ZoomOut() {
    zoom_scale_ /= 1.2;
    if (zoom_scale_ < 0.25) zoom_scale_ = 0.25;
    Refresh();
}

void WhiteboardCanvas::ResetZoom() {
    zoom_scale_ = 1.0;
    Refresh();
}

void WhiteboardCanvas::FitToWindow() {
    if (!document_ || document_->objects.empty()) return;
    
    wxRect bounds = document_->objects[0]->GetBounds();
    for (const auto& obj : document_->objects) {
        bounds = bounds.Union(obj->GetBounds());
    }
    
    wxSize client = GetClientSize();
    double scale_x = (double)client.x / (bounds.width + 40);
    double scale_y = (double)client.y / (bounds.height + 40);
    zoom_scale_ = std::min(scale_x, scale_y);
    if (zoom_scale_ < 0.25) zoom_scale_ = 0.25;
    if (zoom_scale_ > 5.0) zoom_scale_ = 5.0;
    
    Refresh();
}

void WhiteboardCanvas::SetZoom(double scale) {
    zoom_scale_ = std::max(0.25, std::min(5.0, scale));
    Refresh();
}

void WhiteboardCanvas::SetShowGrid(bool show) {
    show_grid_ = show;
    Refresh();
}

void WhiteboardCanvas::SetSnapToGrid(bool snap) {
    snap_to_grid_ = snap;
}

void WhiteboardCanvas::SetGridSize(int size) {
    grid_size_ = size;
    Refresh();
}

void WhiteboardCanvas::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    DoPrepareDC(dc);
    
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    
    // Apply zoom
    dc.SetUserScale(zoom_scale_, zoom_scale_);
    
    // Draw grid
    if (show_grid_) {
        DrawGrid(dc);
    }
    
    // Draw connections
    DrawConnections(dc);
    
    // Draw objects
    DrawObjects(dc);
    
    // Draw selection rectangle
    if (state_ == InteractionState::SELECTING) {
        DrawSelectionRect(dc);
    }
    
    // Draw rubber band for connections/drawing
    if (state_ == InteractionState::CONNECTING || state_ == InteractionState::DRAWING) {
        DrawRubberBand(dc);
    }
}

void WhiteboardCanvas::DrawGrid(wxDC& dc) {
    dc.SetPen(wxPen(wxColour(220, 220, 220), 1));
    
    wxSize virtual_size = GetVirtualSize();
    
    for (int x = 0; x < virtual_size.x; x += grid_size_) {
        dc.DrawLine(x, 0, x, virtual_size.y);
    }
    for (int y = 0; y < virtual_size.y; y += grid_size_) {
        dc.DrawLine(0, y, virtual_size.x, y);
    }
}

void WhiteboardCanvas::DrawObjects(wxDC& dc) {
    if (!document_) return;
    
    // Draw non-selected objects first
    for (const auto& obj : document_->objects) {
        if (!obj->selected) {
            obj->Draw(dc);
        }
    }
    
    // Draw selected objects on top
    for (const auto& obj : document_->objects) {
        if (obj->selected) {
            obj->Draw(dc);
            obj->DrawSelection(dc);
        }
    }
}

void WhiteboardCanvas::DrawConnections(wxDC& dc) {
    if (!document_) return;
    
    for (const auto& conn : document_->connections) {
        WhiteboardObject* from = document_->FindObject(conn->from_object_id);
        WhiteboardObject* to = document_->FindObject(conn->to_object_id);
        if (from && to) {
            conn->Draw(dc, from, to);
        }
    }
}

void WhiteboardCanvas::DrawSelectionRect(wxDC& dc) {
    dc.SetBrush(wxBrush(wxColour(0, 120, 215, 50)));
    dc.SetPen(wxPen(wxColour(0, 120, 215), 1, wxPENSTYLE_DOT));
    dc.DrawRectangle(selection_rect_);
}

void WhiteboardCanvas::DrawRubberBand(wxDC& dc) {
    if (rubber_band_points_.size() < 2) return;
    
    dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_DOT));
    dc.DrawLines(rubber_band_points_.size(), rubber_band_points_.data());
}

void WhiteboardCanvas::OnMouseEvent(wxMouseEvent& event) {
    wxPoint pt = ScreenToCanvas(event.GetPosition());
    
    if (event.ButtonDown()) {
        last_mouse_pos_ = pt;
        
        switch (current_tool_) {
            case Tool::SELECT:
                HandleSelectTool(event);
                break;
            case Tool::PAN:
                state_ = InteractionState::PANNING;
                drag_start_ = pt;
                break;
            case Tool::TYPED_OBJECT:
                HandleTypedObjectTool(pt);
                break;
            case Tool::CONNECTOR:
                HandleConnectorTool(event);
                break;
            default:
                break;
        }
        
        CaptureMouse();
        
    } else if (event.Dragging()) {
        if (state_ == InteractionState::PANNING) {
            int dx = pt.x - drag_start_.x;
            int dy = pt.y - drag_start_.y;
            Scroll(-dx / 20, -dy / 20);
        } else if (state_ == InteractionState::SELECTING) {
            UpdateSelection(pt);
        } else if (state_ == InteractionState::DRAGGING) {
            UpdateDrag(pt);
        } else if (state_ == InteractionState::RESIZING) {
            UpdateResize(pt);
        } else if (state_ == InteractionState::CONNECTING) {
            UpdateConnection(pt);
        }
        
    } else if (event.ButtonUp()) {
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        switch (state_) {
            case InteractionState::SELECTING:
                EndSelection();
                break;
            case InteractionState::DRAGGING:
                EndDrag();
                break;
            case InteractionState::RESIZING:
                EndResize();
                break;
            case InteractionState::CONNECTING:
                EndConnection(pt);
                break;
            default:
                state_ = InteractionState::IDLE;
                break;
        }
    } else if (event.LeftDClick()) {
        // Double-click to edit
        WhiteboardObject* obj = GetObjectAt(pt);
        if (obj && obj->IsTypedObject()) {
            if (obj->HitTestHeader(pt)) {
                obj->StartNameEdit();
            } else if (obj->HitTestDetailsArea(pt)) {
                obj->StartDetailsEdit();
            }
        }
    }
}

void WhiteboardCanvas::HandleSelectTool(wxMouseEvent& event) {
    wxPoint pt = ScreenToCanvas(event.GetPosition());
    
    // Check if clicking on an object
    WhiteboardObject* hit = GetObjectAt(pt);
    
    if (hit) {
        if (!event.ControlDown()) {
            // Clear other selections unless Ctrl is held
            for (auto& obj : document_->objects) {
                obj->selected = false;
            }
        }
        hit->selected = !hit->selected;
        
        state_ = InteractionState::DRAGGING;
        drag_start_ = pt;
    } else {
        // Start selection rectangle
        state_ = InteractionState::SELECTING;
        drag_start_ = pt;
        selection_rect_ = wxRect(pt, wxSize(0, 0));
    }
    
    Refresh();
}

void WhiteboardCanvas::HandleTypedObjectTool(const wxPoint& pt) {
    // Show dialog to select type and name
    ObjectTypeDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        AddTypedObject(pt, dlg.GetSelectedType(), dlg.GetObjectName());
    }
    
    // Reset to select tool after creating object
    current_tool_ = Tool::SELECT;
}

void WhiteboardCanvas::HandleConnectorTool(wxMouseEvent& event) {
    StartConnection(ScreenToCanvas(event.GetPosition()));
}

wxPoint WhiteboardCanvas::ScreenToCanvas(const wxPoint& pt) const {
    int x, y;
    CalcUnscrolledPosition(pt.x, pt.y, &x, &y);
    return wxPoint((int)(x / zoom_scale_), (int)(y / zoom_scale_));
}

wxPoint WhiteboardCanvas::CanvasToScreen(const wxPoint& pt) const {
    int x = (int)(pt.x * zoom_scale_);
    int y = (int)(pt.y * zoom_scale_);
    int sx, sy;
    CalcScrolledPosition(x, y, &sx, &sy);
    return wxPoint(sx, sy);
}

wxPoint WhiteboardCanvas::SnapToGrid(const wxPoint& pt) const {
    if (!snap_to_grid_) return pt;
    
    int x = ((pt.x + grid_size_ / 2) / grid_size_) * grid_size_;
    int y = ((pt.y + grid_size_ / 2) / grid_size_) * grid_size_;
    return wxPoint(x, y);
}

void WhiteboardCanvas::StartSelection(const wxPoint& pt) {
    drag_start_ = pt;
    selection_rect_ = wxRect(pt, wxSize(0, 0));
}

void WhiteboardCanvas::UpdateSelection(const wxPoint& pt) {
    selection_rect_ = wxRect(drag_start_, pt);
    // Normalize the rect
    if (selection_rect_.width < 0) {
        selection_rect_.x += selection_rect_.width;
        selection_rect_.width = -selection_rect_.width;
    }
    if (selection_rect_.height < 0) {
        selection_rect_.y += selection_rect_.height;
        selection_rect_.height = -selection_rect_.height;
    }
    Refresh();
}

void WhiteboardCanvas::EndSelection() {
    // Select objects in rectangle
    for (auto& obj : document_->objects) {
        if (selection_rect_.Intersects(obj->GetBounds())) {
            obj->selected = true;
        }
    }
    
    state_ = InteractionState::IDLE;
    selection_rect_ = wxRect();
    Refresh();
}

void WhiteboardCanvas::StartDrag(const wxPoint& pt) {
    drag_start_ = pt;
    state_ = InteractionState::DRAGGING;
}

void WhiteboardCanvas::UpdateDrag(const wxPoint& pt) {
    int dx = pt.x - last_mouse_pos_.x;
    int dy = pt.y - last_mouse_pos_.y;
    
    for (auto& obj : document_->objects) {
        if (obj->selected && !obj->locked) {
            wxRect bounds = obj->GetBounds();
            bounds.x += dx;
            bounds.y += dy;
            
            if (snap_to_grid_) {
                bounds.x = ((bounds.x + grid_size_ / 2) / grid_size_) * grid_size_;
                bounds.y = ((bounds.y + grid_size_ / 2) / grid_size_) * grid_size_;
            }
            
            obj->SetBounds(bounds);
        }
    }
    
    last_mouse_pos_ = pt;
    Refresh();
}

void WhiteboardCanvas::EndDrag() {
    state_ = InteractionState::IDLE;
}

void WhiteboardCanvas::StartResize(const wxPoint& pt, int handle) {
    resize_handle_ = handle;
    state_ = InteractionState::RESIZING;
}

void WhiteboardCanvas::UpdateResize(const wxPoint& pt) {
    // Stub implementation
}

void WhiteboardCanvas::EndResize() {
    state_ = InteractionState::IDLE;
    resize_handle_ = -1;
}

void WhiteboardCanvas::StartConnection(const wxPoint& pt) {
    state_ = InteractionState::CONNECTING;
    rubber_band_points_.clear();
    rubber_band_points_.push_back(pt);
}

void WhiteboardCanvas::UpdateConnection(const wxPoint& pt) {
    if (rubber_band_points_.size() >= 2) {
        rubber_band_points_[1] = pt;
    } else {
        rubber_band_points_.push_back(pt);
    }
    Refresh();
}

void WhiteboardCanvas::EndConnection(const wxPoint& pt) {
    // Find objects at start and end points
    WhiteboardObject* from = nullptr;
    WhiteboardObject* to = nullptr;
    
    for (auto& obj : document_->objects) {
        if (obj->HitTest(rubber_band_points_[0])) {
            from = obj.get();
        }
        if (obj->HitTest(pt)) {
            to = obj.get();
        }
    }
    
    if (from && to && from != to) {
        auto conn = std::make_unique<WhiteboardConnection>();
        conn->from_object_id = from->id;
        conn->to_object_id = to->id;
        document_->AddConnection(std::move(conn));
    }
    
    state_ = InteractionState::IDLE;
    rubber_band_points_.clear();
    Refresh();
}

void WhiteboardCanvas::OnKeyEvent(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_DELETE) {
        auto selected = GetSelectedObjects();
        for (auto* obj : selected) {
            RemoveObject(obj->id);
        }
    } else if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) {
        EditSelectedObjectDetails();
    } else if (event.GetKeyCode() == WXK_F2) {
        EditSelectedObjectName();
    } else {
        event.Skip();
    }
}

void WhiteboardCanvas::OnSize(wxSizeEvent& event) {
    event.Skip();
}

void WhiteboardCanvas::OnScroll(wxScrollWinEvent& event) {
    event.Skip();
}

void WhiteboardCanvas::AutoLayout() {
    if (document_) {
        document_->AutoLayout();
        Refresh();
    }
}

void WhiteboardCanvas::ArrangeInGrid(int cols) {
    if (!document_) return;
    
    int x = 50, y = 50;
    int col = 0;
    int max_height = 0;
    
    for (auto& obj : document_->objects) {
        wxRect bounds = obj->GetBounds();
        bounds.x = x;
        bounds.y = y;
        obj->SetBounds(bounds);
        
        x += bounds.width + 50;
        max_height = std::max(max_height, bounds.height);
        
        if (++col >= cols) {
            col = 0;
            x = 50;
            y += max_height + 50;
            max_height = 0;
        }
    }
    
    Refresh();
}

void WhiteboardCanvas::ArrangeHierarchical() {
    // Simple hierarchical layout
    AutoLayout();
}

void WhiteboardCanvas::ExportAsImage(const std::string& path, const wxString& format) {
    if (!document_) return;
    
    wxSize size = document_->page_size;
    wxBitmap bitmap(size);
    wxMemoryDC dc(bitmap);
    
    dc.SetBackground(wxBrush(document_->background_color));
    dc.Clear();
    
    DrawGrid(dc);
    DrawConnections(dc);
    DrawObjects(dc);
    
    dc.SelectObject(wxNullBitmap);
    
    wxImage image = bitmap.ConvertToImage();
    image.SaveFile(path, format == "png" ? wxBITMAP_TYPE_PNG : wxBITMAP_TYPE_JPEG);
}

void WhiteboardCanvas::ExportAsSvg(const std::string& path) {
    // Would implement SVG export
}

void WhiteboardCanvas::Print() {
    // Would implement printing
}

// ============================================================================
// Whiteboard Object Implementation
// ============================================================================
WhiteboardObject::WhiteboardObject(Type t) : type(t) {
    static int id_counter = 0;
    id = "obj_" + std::to_string(++id_counter);
}

void WhiteboardObject::MakeTypedObject(WhiteboardObjectType obj_type, const std::string& name) {
    type = Type::TYPED_OBJECT;
    typed_data = std::make_unique<TypedObject>(obj_type, name);
}

void WhiteboardObject::Draw(wxDC& dc) const {
    if (IsTypedObject()) {
        DrawTypedObject(dc);
    } else {
        DrawBasicRectangle(dc);
    }
}

void WhiteboardObject::DrawTypedObject(wxDC& dc) const {
    if (!typed_data) return;
    
    const TypedObject& data = *typed_data;
    
    // Draw body (background)
    dc.SetBrush(wxBrush(data.body_color));
    dc.SetPen(wxPen(data.header_color, 2));
    dc.DrawRectangle(bounds);
    
    // Draw header
    wxRect header = bounds;
    header.height = data.header_height;
    dc.SetBrush(wxBrush(data.header_color));
    dc.SetPen(wxPen(data.header_color, 1));
    dc.DrawRectangle(header);
    
    // Draw header text (type + name)
    dc.SetTextForeground(data.text_color);
    dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    wxString headerText = WhiteboardObjectTypeToString(data.object_type);
    if (!data.name.empty()) {
        headerText += ": " + data.name;
    }
    dc.DrawLabel(headerText, header.Deflate(4, 2), wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    
    // Draw separator line
    dc.SetPen(wxPen(data.header_color, 1));
    dc.DrawLine(bounds.x, header.GetBottom(), bounds.GetRight(), header.GetBottom());
    
    // Draw details area
    wxRect details_area = bounds;
    details_area.y += data.header_height + 2;
    details_area.height -= data.header_height + 4;
    details_area = details_area.Deflate(data.padding, data.padding/2);
    
    dc.SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(data.text_color);
    
    // Draw details text (handle multiline)
    wxString details = data.details;
    int line_height = dc.GetCharHeight();
    int y = details_area.y;
    size_t start = 0;
    while (start < details.length() && y + line_height <= details_area.GetBottom()) {
        size_t end = details.find('\n', start);
        if (end == wxString::npos) end = details.length();
        wxString line = details.substr(start, end - start);
        dc.DrawText(line, details_area.x, y);
        y += line_height;
        start = end + 1;
    }
}

void WhiteboardObject::DrawBasicRectangle(wxDC& dc) const {
    dc.SetBrush(wxBrush(fill_color));
    dc.SetPen(wxPen(border_color, border_width));
    
    if (corner_radius > 0) {
        dc.DrawRoundedRectangle(bounds, corner_radius);
    } else {
        dc.DrawRectangle(bounds);
    }
    
    // Draw text
    if (!text.empty()) {
        dc.SetTextForeground(text_color);
        dc.SetFont(font);
        dc.DrawLabel(text, bounds, text_alignment);
    }
}

void WhiteboardObject::DrawSelection(wxDC& dc) const {
    // Draw resize handles
    const int handle_size = 6;
    dc.SetBrush(wxBrush(wxColour(0, 120, 215)));
    dc.SetPen(wxPen(wxColour(255, 255, 255), 1));
    
    wxPoint corners[] = {
        bounds.GetTopLeft(),
        wxPoint(bounds.x + bounds.width / 2, bounds.y),
        bounds.GetTopRight(),
        wxPoint(bounds.x + bounds.width, bounds.y + bounds.height / 2),
        bounds.GetBottomRight(),
        wxPoint(bounds.x + bounds.width / 2, bounds.y + bounds.height),
        bounds.GetBottomLeft(),
        wxPoint(bounds.x, bounds.y + bounds.height / 2)
    };
    
    for (const auto& pt : corners) {
        dc.DrawRectangle(pt.x - handle_size / 2, pt.y - handle_size / 2, 
                         handle_size, handle_size);
    }
}

bool WhiteboardObject::HitTest(const wxPoint& pt) const {
    return bounds.Contains(pt);
}

bool WhiteboardObject::HitTestHeader(const wxPoint& pt) const {
    if (!IsTypedObject() || !typed_data) return false;
    wxRect header = bounds;
    header.height = typed_data->header_height;
    return header.Contains(pt);
}

bool WhiteboardObject::HitTestDetailsArea(const wxPoint& pt) const {
    if (!IsTypedObject() || !typed_data) return false;
    return bounds.Contains(pt) && !HitTestHeader(pt);
}

int WhiteboardObject::HitTestResizeHandle(const wxPoint& pt) const {
    const int handle_size = 8;
    wxPoint corners[] = {
        bounds.GetTopLeft(),
        wxPoint(bounds.x + bounds.width / 2, bounds.y),
        bounds.GetTopRight(),
        wxPoint(bounds.x + bounds.width, bounds.y + bounds.height / 2),
        bounds.GetBottomRight(),
        wxPoint(bounds.x + bounds.width / 2, bounds.y + bounds.height),
        bounds.GetBottomLeft(),
        wxPoint(bounds.x, bounds.y + bounds.height / 2)
    };
    
    for (int i = 0; i < 8; ++i) {
        wxRect handle(corners[i].x - handle_size / 2, corners[i].y - handle_size / 2,
                      handle_size, handle_size);
        if (handle.Contains(pt)) {
            return i;
        }
    }
    
    return -1;
}

wxPoint WhiteboardObject::GetConnectionPoint(int side) const {
    switch (side % 4) {
        case 0: return wxPoint(bounds.x + bounds.width / 2, bounds.y);
        case 1: return wxPoint(bounds.x + bounds.width, bounds.y + bounds.height / 2);
        case 2: return wxPoint(bounds.x + bounds.width / 2, bounds.y + bounds.height);
        case 3: return wxPoint(bounds.x, bounds.y + bounds.height / 2);
    }
    return wxPoint(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
}

void WhiteboardObject::StartNameEdit() {
    if (!typed_data) return;
    
    wxTextEntryDialog dlg(nullptr, "Edit object name:", "Edit Name", typed_data->name);
    if (dlg.ShowModal() == wxID_OK) {
        typed_data->SetName(dlg.GetValue().ToStdString());
    }
}

void WhiteboardObject::StartDetailsEdit() {
    if (!typed_data) return;
    
    ObjectEditDialog dlg(nullptr, typed_data.get());
    dlg.ShowModal();
}

std::unique_ptr<WhiteboardObject> WhiteboardObject::Clone() const {
    auto clone = std::make_unique<WhiteboardObject>(type);
    clone->bounds = bounds;
    clone->fill_color = fill_color;
    clone->border_color = border_color;
    clone->text_color = text_color;
    clone->border_width = border_width;
    clone->corner_radius = corner_radius;
    clone->text = text;
    clone->font = font;
    clone->text_alignment = text_alignment;
    if (typed_data) {
        clone->typed_data = std::make_unique<TypedObject>(*typed_data);
    }
    return clone;
}

void WhiteboardObject::ToJson(std::ostream& out) const {
    out << "{\"id\":\"" << id << "\",\"type\":" << static_cast<int>(type) << "}";
}

// ============================================================================
// Whiteboard Connection Implementation
// ============================================================================
WhiteboardConnection::WhiteboardConnection() {
    static int id_counter = 0;
    id = "conn_" + std::to_string(++id_counter);
}

void WhiteboardConnection::Draw(wxDC& dc, const WhiteboardObject* from, 
                                 const WhiteboardObject* to) const {
    wxPoint start = from->GetConnectionPoint(from_port);
    wxPoint end = to->GetConnectionPoint(to_port);
    
    dc.SetPen(wxPen(color, width, 
                    style == Style::DASHED ? wxPENSTYLE_SHORT_DASH :
                    style == Style::DOTTED ? wxPENSTYLE_DOT :
                    wxPENSTYLE_SOLID));
    
    if (waypoints.empty()) {
        dc.DrawLine(start, end);
    } else {
        wxPoint prev = start;
        for (const auto& wp : waypoints) {
            dc.DrawLine(prev, wp);
            prev = wp;
        }
        dc.DrawLine(prev, end);
    }
    
    // Draw arrowhead
    if (type == Type::ARROW) {
        double angle = atan2(end.y - start.y, end.x - start.x);
        int arrow_len = 10;
        
        wxPoint arrow1(end.x - arrow_len * cos(angle - M_PI / 6),
                       end.y - arrow_len * sin(angle - M_PI / 6));
        wxPoint arrow2(end.x - arrow_len * cos(angle + M_PI / 6),
                       end.y - arrow_len * sin(angle + M_PI / 6));
        
        dc.DrawLine(end, arrow1);
        dc.DrawLine(end, arrow2);
    }
    
    // Draw label
    if (!label.empty()) {
        wxPoint mid((start.x + end.x) / 2, (start.y + end.y) / 2);
        dc.DrawText(label, mid);
    }
}

bool WhiteboardConnection::HitTest(const wxPoint& pt) const {
    // Simplified hit test
    return false;
}

// ============================================================================
// Whiteboard Document Implementation
// ============================================================================
WhiteboardDocument::WhiteboardDocument() {
    static int id_counter = 0;
    id = "doc_" + std::to_string(++id_counter);
}

WhiteboardObject* WhiteboardDocument::FindObject(const std::string& id) {
    for (auto& obj : objects) {
        if (obj->id == id) return obj.get();
    }
    return nullptr;
}

WhiteboardConnection* WhiteboardDocument::FindConnection(const std::string& id) {
    for (auto& conn : connections) {
        if (conn->id == id) return conn.get();
    }
    return nullptr;
}

void WhiteboardDocument::AddObject(std::unique_ptr<WhiteboardObject> obj) {
    objects.push_back(std::move(obj));
}

void WhiteboardDocument::RemoveObject(const std::string& id) {
    // Remove associated connections first
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
            [&id](const auto& conn) {
                return conn->from_object_id == id || conn->to_object_id == id;
            }),
        connections.end());
    
    // Remove object
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [&id](const auto& obj) { return obj->id == id; }),
        objects.end());
}

void WhiteboardDocument::AddConnection(std::unique_ptr<WhiteboardConnection> conn) {
    connections.push_back(std::move(conn));
}

void WhiteboardDocument::RemoveConnection(const std::string& id) {
    connections.erase(
        std::remove_if(connections.begin(), connections.end(),
            [&id](const auto& conn) { return conn->id == id; }),
        connections.end());
}

std::vector<WhiteboardObject*> WhiteboardDocument::GetObjectsByType(WhiteboardObjectType type) const {
    std::vector<WhiteboardObject*> result;
    for (const auto& obj : objects) {
        if (obj->IsTypedObject() && obj->typed_data && obj->typed_data->object_type == type) {
            result.push_back(const_cast<WhiteboardObject*>(obj.get()));
        }
    }
    return result;
}

void WhiteboardDocument::SaveToFile(const std::string& path) const {
    // Would implement serialization
}

std::unique_ptr<WhiteboardDocument> WhiteboardDocument::LoadFromFile(
    const std::string& path) {
    // Would implement deserialization
    return std::make_unique<WhiteboardDocument>();
}

void WhiteboardDocument::AutoLayout() {
    // Simple grid layout
    int x = 50, y = 50;
    int col = 0;
    const int cols = 4;
    const int spacing = 50;
    
    for (auto& obj : objects) {
        wxRect bounds = obj->GetBounds();
        bounds.x = x;
        bounds.y = y;
        obj->SetBounds(bounds);
        
        x += bounds.width + spacing;
        if (++col >= cols) {
            col = 0;
            x = 50;
            y += bounds.height + spacing;
        }
    }
}

// ============================================================================
// Whiteboard Panel Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(WhiteboardPanel, wxPanel)
wxEND_EVENT_TABLE()

WhiteboardPanel::WhiteboardPanel(wxWindow* parent) : wxPanel(parent) {
    canvas_ = new WhiteboardCanvas(this);
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(canvas_, 1, wxEXPAND);
    SetSizer(sizer);
}

// ============================================================================
// Object Type Dialog Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(ObjectTypeDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ObjectTypeDialog::OnOK)
wxEND_EVENT_TABLE()

ObjectTypeDialog::ObjectTypeDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Select Object Type", wxDefaultPosition, wxSize(300, 200)) {
    BuildLayout();
}

void ObjectTypeDialog::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Type selection
    auto* type_sizer = new wxBoxSizer(wxHORIZONTAL);
    type_sizer->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    
    type_choice_ = new wxChoice(this, wxID_ANY);
    type_choice_->Append("Database");
    type_choice_->Append("Schema");
    type_choice_->Append("Table");
    type_choice_->Append("View");
    type_choice_->Append("Procedure");
    type_choice_->Append("Function");
    type_choice_->Append("Trigger");
    type_choice_->Append("Index");
    type_choice_->Append("Datastore");
    type_choice_->Append("Server");
    type_choice_->Append("Cluster");
    type_choice_->Append("Generic");
    type_choice_->SetSelection(11);  // Generic default
    
    type_sizer->Add(type_choice_, 1, wxEXPAND | wxALL, 4);
    main_sizer->Add(type_sizer, 0, wxEXPAND);
    
    // Name input
    auto* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    name_sizer->Add(name_ctrl_, 1, wxEXPAND | wxALL, 4);
    main_sizer->Add(name_sizer, 0, wxEXPAND);
    
    // Buttons
    main_sizer->Add(new wxButton(this, wxID_OK, "Create"), 0, wxALIGN_CENTER | wxALL, 8);
    
    SetSizer(main_sizer);
}

void ObjectTypeDialog::OnOK(wxCommandEvent&) {
    int sel = type_choice_->GetSelection();
    switch (sel) {
        case 0: selected_type_ = WhiteboardObjectType::DATABASE; break;
        case 1: selected_type_ = WhiteboardObjectType::SCHEMA; break;
        case 2: selected_type_ = WhiteboardObjectType::TABLE; break;
        case 3: selected_type_ = WhiteboardObjectType::VIEW; break;
        case 4: selected_type_ = WhiteboardObjectType::PROCEDURE; break;
        case 5: selected_type_ = WhiteboardObjectType::FUNCTION; break;
        case 6: selected_type_ = WhiteboardObjectType::TRIGGER; break;
        case 7: selected_type_ = WhiteboardObjectType::INDEX; break;
        case 8: selected_type_ = WhiteboardObjectType::DATASTORE; break;
        case 9: selected_type_ = WhiteboardObjectType::SERVER; break;
        case 10: selected_type_ = WhiteboardObjectType::CLUSTER; break;
        default: selected_type_ = WhiteboardObjectType::GENERIC; break;
    }
    object_name_ = name_ctrl_->GetValue().ToStdString();
    EndModal(wxID_OK);
}

// ============================================================================
// Object Edit Dialog Implementation
// ============================================================================
wxBEGIN_EVENT_TABLE(ObjectEditDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ObjectEditDialog::OnOK)
wxEND_EVENT_TABLE()

ObjectEditDialog::ObjectEditDialog(wxWindow* parent, TypedObject* object)
    : wxDialog(parent, wxID_ANY, "Edit Object", wxDefaultPosition, wxSize(400, 400))
    , object_(object) {
    BuildLayout();
}

void ObjectEditDialog::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Name
    auto* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY, object_->name);
    name_sizer->Add(name_ctrl_, 1, wxEXPAND | wxALL, 4);
    main_sizer->Add(name_sizer, 0, wxEXPAND);
    
    // Details label
    main_sizer->Add(new wxStaticText(this, wxID_ANY, "Details:"), 0, wxALL, 4);
    
    // Details text area
    details_ctrl_ = new wxTextCtrl(this, wxID_ANY, object_->details, 
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_DONTWRAP);
    main_sizer->Add(details_ctrl_, 1, wxEXPAND | wxALL, 4);
    
    // Buttons
    auto* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
    btn_sizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 4);
    btn_sizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 4);
    main_sizer->Add(btn_sizer, 0, wxALIGN_CENTER);
    
    SetSizer(main_sizer);
}

void ObjectEditDialog::OnOK(wxCommandEvent&) {
    object_->SetName(name_ctrl_->GetValue().ToStdString());
    object_->SetDetails(details_ctrl_->GetValue().ToStdString());
    EndModal(wxID_OK);
}

} // namespace scratchrobin
