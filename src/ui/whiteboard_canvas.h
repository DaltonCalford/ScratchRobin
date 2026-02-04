/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_WHITEBOARD_CANVAS_H
#define SCRATCHROBIN_WHITEBOARD_CANVAS_H

#include <wx/wx.h>
#include <ctime>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// Forward declarations
class WhiteboardObject;
class WhiteboardConnection;
class WhiteboardDocument;

// ============================================================================
// Whiteboard Object Types
// ============================================================================
enum class WhiteboardObjectType {
    DATABASE,
    SCHEMA,
    TABLE,
    VIEW,
    PROCEDURE,
    FUNCTION,
    TRIGGER,
    INDEX,
    DATASTORE,
    SERVER,
    CLUSTER,
    GENERIC
};

std::string WhiteboardObjectTypeToString(WhiteboardObjectType type);
wxColour GetTypeColor(WhiteboardObjectType type);

// ============================================================================
// Typed Object - Rectangular shape with name and details
// ============================================================================
class TypedObject {
public:
    WhiteboardObjectType object_type = WhiteboardObjectType::GENERIC;
    std::string name;
    std::string details;  // Free-form text area content
    
    // Visual settings
    wxColour header_color;
    wxColour body_color;
    wxColour text_color;
    int header_height = 24;
    int padding = 8;
    
    // Type-specific metadata
    std::map<std::string, std::string> metadata;
    
    TypedObject();
    explicit TypedObject(WhiteboardObjectType type, const std::string& name = "");
    
    void SetType(WhiteboardObjectType type);
    void SetName(const std::string& new_name);
    void SetDetails(const std::string& new_details);
    
    // Get suggested default details based on type
    std::string GetDefaultDetails() const;
    
    // Type helpers
    bool IsDatabaseObject() const;
    bool IsContainer() const;  // Database, Schema can contain other objects
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static TypedObject FromJson(const std::string& json);
};

// ============================================================================
// Whiteboard Canvas - Interactive diagramming surface
// ============================================================================
class WhiteboardCanvas : public wxScrolledCanvas {
public:
    enum class Tool {
        SELECT,
        PAN,
        RECTANGLE,
        TYPED_OBJECT,  // New tool for creating typed objects
        TEXT,
        LINE,
        ARROW,
        CONNECTOR,
        NOTE,
        IMAGE,
        ERASER
    };
    
    WhiteboardCanvas(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~WhiteboardCanvas();
    
    // Tool management
    void SetTool(Tool tool);
    Tool GetTool() const { return current_tool_; }
    void SetObjectTypeForNextCreation(WhiteboardObjectType type);
    
    // Document operations
    void NewDocument();
    bool LoadDocument(const std::string& path);
    bool SaveDocument(const std::string& path);
    
    // Typed object operations
    void AddTypedObject(const wxPoint& position, WhiteboardObjectType type, 
                        const std::string& name = "");
    void AddTypedObject(const wxPoint& position, const TypedObject& template_obj);
    
    // Object operations
    void AddObject(std::unique_ptr<WhiteboardObject> obj);
    void RemoveObject(const std::string& id);
    void SelectObject(const std::string& id);
    void ClearSelection();
    std::vector<WhiteboardObject*> GetSelectedObjects() const;
    WhiteboardObject* GetObjectAt(const wxPoint& pt) const;
    
    // View operations
    void ZoomIn();
    void ZoomOut();
    void ResetZoom();
    void FitToWindow();
    void SetZoom(double scale);
    double GetZoom() const { return zoom_scale_; }
    
    // Grid and snapping
    void SetShowGrid(bool show);
    bool GetShowGrid() const { return show_grid_; }
    void SetSnapToGrid(bool snap);
    bool GetSnapToGrid() const { return snap_to_grid_; }
    void SetGridSize(int size);
    
    // Export
    void ExportAsImage(const std::string& path, const wxString& format = "png");
    void ExportAsSvg(const std::string& path);
    void Print();
    
    // Editing
    void EditSelectedObjectName();
    void EditSelectedObjectDetails();
    
    // Layout algorithms
    void AutoLayout();
    void ArrangeInGrid(int cols);
    void ArrangeHierarchical();
    
    // Get document
    WhiteboardDocument* GetDocument() { return document_.get(); }
    
private:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnKeyEvent(wxKeyEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnScroll(wxScrollWinEvent& event);
    
    // Drawing
    void DrawGrid(wxDC& dc);
    void DrawObjects(wxDC& dc);
    void DrawConnections(wxDC& dc);
    void DrawSelectionRect(wxDC& dc);
    void DrawRubberBand(wxDC& dc);
    
    // Coordinate conversion
    wxPoint ScreenToCanvas(const wxPoint& pt) const;
    wxPoint CanvasToScreen(const wxPoint& pt) const;
    wxPoint SnapToGrid(const wxPoint& pt) const;
    
    // Selection handling
    void StartSelection(const wxPoint& pt);
    void UpdateSelection(const wxPoint& pt);
    void EndSelection();
    void SelectInRect(const wxRect& rect);
    
    // Drag handling
    void StartDrag(const wxPoint& pt);
    void UpdateDrag(const wxPoint& pt);
    void EndDrag();
    
    // Resize handling
    void StartResize(const wxPoint& pt, int handle);
    void UpdateResize(const wxPoint& pt);
    void EndResize();
    
    // Connection handling
    void StartConnection(const wxPoint& pt);
    void UpdateConnection(const wxPoint& pt);
    void EndConnection(const wxPoint& pt);
    
    // Tool handling
    void HandleSelectTool(wxMouseEvent& event);
    void HandleTypedObjectTool(const wxPoint& pt);
    void HandlePanTool(wxMouseEvent& event);
    void HandleConnectorTool(wxMouseEvent& event);
    
    // State
    Tool current_tool_ = Tool::SELECT;
    WhiteboardObjectType next_object_type_ = WhiteboardObjectType::GENERIC;
    double zoom_scale_ = 1.0;
    bool show_grid_ = true;
    bool snap_to_grid_ = true;
    int grid_size_ = 20;
    
    // Document
    std::unique_ptr<WhiteboardDocument> document_;
    
    // Interaction state
    enum class InteractionState {
        IDLE,
        SELECTING,
        DRAGGING,
        RESIZING,
        CONNECTING,
        PANNING,
        DRAWING
    };
    InteractionState state_ = InteractionState::IDLE;
    
    wxPoint drag_start_;
    wxPoint last_mouse_pos_;
    wxRect selection_rect_;
    std::vector<wxPoint> rubber_band_points_;
    int resize_handle_ = -1;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Whiteboard Object Base Class
// ============================================================================
class WhiteboardObject {
public:
    enum class Type {
        BASIC_RECTANGLE,
        TYPED_OBJECT,    // New type with header and details area
        ELLIPSE,
        TEXT,
        IMAGE,
        NOTE,
        CUSTOM
    };
    
    std::string id;
    Type type;
    wxRect bounds;
    bool selected = false;
    bool locked = false;
    
    // Typed object data (only used when type == TYPED_OBJECT)
    std::unique_ptr<TypedObject> typed_data;
    
    // Basic appearance (fallback when not a typed object)
    wxColour fill_color = wxColour(255, 255, 255);
    wxColour border_color = wxColour(0, 0, 0);
    wxColour text_color = wxColour(0, 0, 0);
    int border_width = 1;
    int corner_radius = 0;
    
    // Basic text (fallback when not a typed object)
    std::string text;
    wxFont font = wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    int text_alignment = wxALIGN_CENTER;
    
    WhiteboardObject(Type t);
    virtual ~WhiteboardObject() = default;
    
    // Typed object helpers
    bool IsTypedObject() const { return type == Type::TYPED_OBJECT && typed_data != nullptr; }
    TypedObject* GetTypedObject() { return typed_data.get(); }
    const TypedObject* GetTypedObject() const { return typed_data.get(); }
    void MakeTypedObject(WhiteboardObjectType obj_type, const std::string& name = "");
    
    // Drawing
    virtual void Draw(wxDC& dc) const;
    virtual void DrawSelection(wxDC& dc) const;
    virtual wxRect GetBounds() const { return bounds; }
    virtual void SetBounds(const wxRect& rect) { bounds = rect; }
    
    // Typed object drawing
    void DrawTypedObject(wxDC& dc) const;
    void DrawBasicRectangle(wxDC& dc) const;
    
    // Hit testing
    virtual bool HitTest(const wxPoint& pt) const;
    virtual bool HitTestHeader(const wxPoint& pt) const;  // For typed objects
    virtual bool HitTestDetailsArea(const wxPoint& pt) const;  // For typed objects
    virtual int HitTestResizeHandle(const wxPoint& pt) const;
    virtual wxPoint GetConnectionPoint(int side) const;
    
    // Editing
    virtual void StartNameEdit();
    virtual void StartDetailsEdit();
    
    // Serialization
    virtual void ToJson(std::ostream& out) const;
    static std::unique_ptr<WhiteboardObject> FromJson(const std::string& json);
    
    // Cloning
    virtual std::unique_ptr<WhiteboardObject> Clone() const;
    
protected:
    void DrawResizeHandles(wxDC& dc) const;
};

// ============================================================================
// Whiteboard Connection
// ============================================================================
class WhiteboardConnection {
public:
    enum class Type {
        STRAIGHT,
        ORTHOGONAL,
        CURVED,
        ARROW
    };
    
    enum class Style {
        SOLID,
        DASHED,
        DOTTED
    };
    
    std::string id;
    Type type = Type::STRAIGHT;
    Style style = Style::SOLID;
    
    std::string from_object_id;
    std::string to_object_id;
    int from_port = 0;
    int to_port = 0;
    
    wxColour color = wxColour(0, 0, 0);
    int width = 2;
    
    std::string label;
    std::string cardinality_from;
    std::string cardinality_to;
    
    std::vector<wxPoint> waypoints;
    
    WhiteboardConnection();
    
    void Draw(wxDC& dc, const WhiteboardObject* from, const WhiteboardObject* to) const;
    bool HitTest(const wxPoint& pt) const;
    
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<WhiteboardConnection> FromJson(const std::string& json);
};

// ============================================================================
// Whiteboard Document
// ============================================================================
class WhiteboardDocument {
public:
    std::string id;
    std::string name;
    std::string description;
    
    std::vector<std::unique_ptr<WhiteboardObject>> objects;
    std::vector<std::unique_ptr<WhiteboardConnection>> connections;
    
    // Page settings
    wxSize page_size = wxSize(2000, 1500);
    wxColour background_color = wxColour(255, 255, 255);
    
    WhiteboardDocument();
    
    // Object management
    WhiteboardObject* FindObject(const std::string& id);
    WhiteboardConnection* FindConnection(const std::string& id);
    
    void AddObject(std::unique_ptr<WhiteboardObject> obj);
    void RemoveObject(const std::string& id);
    
    void AddConnection(std::unique_ptr<WhiteboardConnection> conn);
    void RemoveConnection(const std::string& id);
    
    // Typed object helpers
    std::vector<WhiteboardObject*> GetObjectsByType(WhiteboardObjectType type) const;
    std::vector<WhiteboardObject*> GetChildObjects(const std::string& parent_id) const;
    
    // Serialization
    void ToJson(std::ostream& out) const;
    static std::unique_ptr<WhiteboardDocument> FromJson(const std::string& json);
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<WhiteboardDocument> LoadFromFile(const std::string& path);
    
    // Export
    void ExportAsSvg(const std::string& path) const;
    void ExportAsPng(const std::string& path, int width, int height) const;
    
    // Layout
    void AutoLayout();
};

// ============================================================================
// Whiteboard Panel
// ============================================================================
class WhiteboardPanel : public wxPanel {
public:
    WhiteboardPanel(wxWindow* parent);
    
    WhiteboardCanvas* GetCanvas() { return canvas_; }
    
    // Toolbar helpers
    void AddObjectTypeTool(WhiteboardObjectType type, const wxString& label, 
                           const wxBitmap& bitmap);
    
private:
    void BuildToolbar();
    void BuildLayout();
    
    WhiteboardCanvas* canvas_ = nullptr;
    wxToolBar* toolbar_ = nullptr;
    wxComboBox* zoom_combo_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Object Type Dialog - For selecting object type when creating
// ============================================================================
class ObjectTypeDialog : public wxDialog {
public:
    ObjectTypeDialog(wxWindow* parent);
    
    WhiteboardObjectType GetSelectedType() const { return selected_type_; }
    std::string GetObjectName() const { return object_name_; }
    
private:
    void BuildLayout();
    void OnTypeSelect(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    
    WhiteboardObjectType selected_type_ = WhiteboardObjectType::GENERIC;
    std::string object_name_;
    wxChoice* type_choice_ = nullptr;
    wxTextCtrl* name_ctrl_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Object Edit Dialog - For editing name and details
// ============================================================================
class ObjectEditDialog : public wxDialog {
public:
    ObjectEditDialog(wxWindow* parent, TypedObject* object);
    
private:
    void BuildLayout();
    void OnOK(wxCommandEvent& event);
    
    TypedObject* object_;
    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* details_ctrl_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WHITEBOARD_CANVAS_H
