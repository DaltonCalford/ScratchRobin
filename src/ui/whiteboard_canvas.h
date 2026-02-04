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
// Whiteboard Canvas - Interactive diagramming surface
// ============================================================================
class WhiteboardCanvas : public wxScrolledCanvas {
public:
    enum class Tool {
        SELECT,
        PAN,
        RECTANGLE,
        ELLIPSE,
        TEXT,
        LINE,
        ARROW,
        CONNECTOR,
        TABLE,
        NOTE,
        IMAGE,
        ERASER
    };
    
    WhiteboardCanvas(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~WhiteboardCanvas();
    
    // Tool management
    void SetTool(Tool tool);
    Tool GetTool() const { return current_tool_; }
    
    // Document operations
    void NewDocument();
    bool LoadDocument(const std::string& path);
    bool SaveDocument(const std::string& path);
    void LoadFromProject(const class Project& project);
    
    // Object operations
    void AddObject(std::unique_ptr<WhiteboardObject> obj);
    void RemoveObject(const std::string& id);
    void SelectObject(const std::string& id);
    void ClearSelection();
    std::vector<WhiteboardObject*> GetSelectedObjects() const;
    
    // Connection operations
    void AddConnection(std::unique_ptr<WhiteboardConnection> conn);
    void RemoveConnection(const std::string& id);
    
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
    
    // Clipboard
    void Cut();
    void Copy();
    void Paste();
    void Delete();
    bool CanPaste() const;
    
    // Undo/Redo
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
    // Layout algorithms
    void AutoLayout();
    void ArrangeInGrid(int cols);
    void ArrangeHierarchical();
    void ArrangeCircular();
    
    // Data model integration
    void SynchronizeFromDatabase();
    void ApplyToDatabase();
    
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
    wxRect ScreenToCanvas(const wxRect& rect) const;
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
    void HandleDrawTool(wxMouseEvent& event);
    void HandlePanTool(wxMouseEvent& event);
    void HandleConnectorTool(wxMouseEvent& event);
    void HandleTextTool(const wxPoint& pt);
    void HandleTableTool(const wxPoint& pt);
    
    // State
    Tool current_tool_ = Tool::SELECT;
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
    
    // Canvas offset for scrolling
    wxPoint canvas_offset_;
    
    wxDECLARE_EVENT_TABLE();
};

// ============================================================================
// Whiteboard Object Base Class
// ============================================================================
class WhiteboardObject {
public:
    enum class Type {
        RECTANGLE,
        ELLIPSE,
        TEXT,
        IMAGE,
        TABLE,
        NOTE,
        CUSTOM
    };
    
    std::string id;
    Type type;
    wxRect bounds;
    bool selected = false;
    bool locked = false;
    
    // Appearance
    wxColour fill_color = wxColour(255, 255, 255);
    wxColour border_color = wxColour(0, 0, 0);
    wxColour text_color = wxColour(0, 0, 0);
    int border_width = 1;
    int corner_radius = 0;
    
    // Text
    std::string text;
    wxFont font = wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    int text_alignment = wxALIGN_CENTER;
    
    // Metadata
    std::string linked_object_id;  // Link to database object
    std::string linked_object_type;
    std::map<std::string, std::string> properties;
    
    WhiteboardObject(Type t);
    virtual ~WhiteboardObject() = default;
    
    // Drawing
    virtual void Draw(wxDC& dc) const;
    virtual void DrawSelection(wxDC& dc) const;
    virtual wxRect GetBounds() const { return bounds; }
    virtual void SetBounds(const wxRect& rect) { bounds = rect; }
    
    // Hit testing
    virtual bool HitTest(const wxPoint& pt) const;
    virtual int HitTestResizeHandle(const wxPoint& pt) const;
    virtual wxPoint GetConnectionPoint(int side) const;
    
    // Serialization
    virtual void ToJson(std::ostream& out) const;
    static std::unique_ptr<WhiteboardObject> FromJson(const std::string& json);
    
    // Cloning
    virtual std::unique_ptr<WhiteboardObject> Clone() const;
    
protected:
    void DrawResizeHandles(wxDC& dc) const;
};

// ============================================================================
// Table Object
// ============================================================================
class TableObject : public WhiteboardObject {
public:
    struct Column {
        std::string name;
        std::string type;
        bool is_pk = false;
        bool is_fk = false;
        bool is_nullable = true;
    };
    
    std::string table_name;
    std::string schema;
    std::vector<Column> columns;
    
    TableObject();
    
    void AddColumn(const Column& col);
    void RemoveColumn(const std::string& name);
    
    void Draw(wxDC& dc) const override;
    bool HitTest(const wxPoint& pt) const override;
    
    static std::unique_ptr<TableObject> FromDatabaseTable(
        const std::string& schema, 
        const std::string& table,
        const std::vector<std::pair<std::string, std::string>>& columns);
};

// ============================================================================
// Note Object
// ============================================================================
class NoteObject : public WhiteboardObject {
public:
    NoteObject();
    
    void Draw(wxDC& dc) const override;
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
    
    // For relationships
    std::string label;
    std::string cardinality_from;  // 1, 0..1, 0..*, 1..*
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
    
private:
    void BuildToolbar();
    void BuildLayout();
    
    WhiteboardCanvas* canvas_ = nullptr;
    wxToolBar* toolbar_ = nullptr;
    wxComboBox* zoom_combo_ = nullptr;
    
    void OnToolSelect(wxCommandEvent& event);
    void OnZoomChange(wxCommandEvent& event);
    void OnShowGrid(wxCommandEvent& event);
    void OnSnapToGrid(wxCommandEvent& event);
    void OnAutoLayout(wxCommandEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WHITEBOARD_CANVAS_H
