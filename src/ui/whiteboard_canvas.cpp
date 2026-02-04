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

namespace scratchrobin {

WhiteboardCanvas::WhiteboardCanvas(wxWindow* parent, wxWindowID id)
    : wxScrolledCanvas(parent, id) {
    // Stub implementation
}

WhiteboardCanvas::~WhiteboardCanvas() = default;

void WhiteboardCanvas::SetTool(Tool tool) {}
void WhiteboardCanvas::NewDocument() {}
bool WhiteboardCanvas::LoadDocument(const std::string& path) { return false; }
bool WhiteboardCanvas::SaveDocument(const std::string& path) { return false; }
void WhiteboardCanvas::AddObject(std::unique_ptr<WhiteboardObject> obj) {}
void WhiteboardCanvas::RemoveObject(const std::string& id) {}
void WhiteboardCanvas::ClearSelection() {}
std::vector<WhiteboardObject*> WhiteboardCanvas::GetSelectedObjects() const { return {}; }
void WhiteboardCanvas::ZoomIn() {}
void WhiteboardCanvas::ZoomOut() {}
void WhiteboardCanvas::ResetZoom() {}
void WhiteboardCanvas::FitToWindow() {}
void WhiteboardCanvas::SetZoom(double scale) {}
void WhiteboardCanvas::SetShowGrid(bool show) {}
void WhiteboardCanvas::SetSnapToGrid(bool snap) {}
void WhiteboardCanvas::SetGridSize(int size) {}
void WhiteboardCanvas::ExportAsImage(const std::string& path, const wxString& format) {}
void WhiteboardCanvas::ExportAsSvg(const std::string& path) {}
void WhiteboardCanvas::Print() {}
void WhiteboardCanvas::Cut() {}
void WhiteboardCanvas::Copy() {}
void WhiteboardCanvas::Paste() {}
void WhiteboardCanvas::Delete() {}
bool WhiteboardCanvas::CanPaste() const { return false; }
void WhiteboardCanvas::Undo() {}
void WhiteboardCanvas::Redo() {}
bool WhiteboardCanvas::CanUndo() const { return false; }
bool WhiteboardCanvas::CanRedo() const { return false; }
void WhiteboardCanvas::AutoLayout() {}
void WhiteboardCanvas::ArrangeInGrid(int cols) {}
void WhiteboardCanvas::ArrangeHierarchical() {}
void WhiteboardCanvas::ArrangeCircular() {}

wxBEGIN_EVENT_TABLE(WhiteboardCanvas, wxScrolledCanvas)
wxEND_EVENT_TABLE()

WhiteboardObject::WhiteboardObject(Type t) : type(t) {
    static int id_counter = 0;
    id = "obj_" + std::to_string(++id_counter);
}
void WhiteboardObject::Draw(wxDC& dc) const {}
void WhiteboardObject::DrawSelection(wxDC& dc) const {}
bool WhiteboardObject::HitTest(const wxPoint& pt) const { return false; }
int WhiteboardObject::HitTestResizeHandle(const wxPoint& pt) const { return -1; }
wxPoint WhiteboardObject::GetConnectionPoint(int side) const { return wxPoint(0, 0); }
void WhiteboardObject::ToJson(std::ostream& out) const {}
std::unique_ptr<WhiteboardObject> WhiteboardObject::Clone() const {
    return std::make_unique<WhiteboardObject>(type);
}

TableObject::TableObject() : WhiteboardObject(Type::TABLE) {}
void TableObject::Draw(wxDC& dc) const {}
bool TableObject::HitTest(const wxPoint& pt) const { return false; }

NoteObject::NoteObject() : WhiteboardObject(Type::NOTE) {}
void NoteObject::Draw(wxDC& dc) const {}

WhiteboardConnection::WhiteboardConnection() {}
void WhiteboardConnection::Draw(wxDC& dc, const WhiteboardObject* from, const WhiteboardObject* to) const {}
bool WhiteboardConnection::HitTest(const wxPoint& pt) const { return false; }

WhiteboardDocument::WhiteboardDocument() {}
WhiteboardObject* WhiteboardDocument::FindObject(const std::string& id) { return nullptr; }
WhiteboardConnection* WhiteboardDocument::FindConnection(const std::string& id) { return nullptr; }
void WhiteboardDocument::AddObject(std::unique_ptr<WhiteboardObject> obj) {}
void WhiteboardDocument::RemoveObject(const std::string& id) {}
void WhiteboardDocument::AddConnection(std::unique_ptr<WhiteboardConnection> conn) {}
void WhiteboardDocument::RemoveConnection(const std::string& id) {}
void WhiteboardDocument::SaveToFile(const std::string& path) const {}
std::unique_ptr<WhiteboardDocument> WhiteboardDocument::LoadFromFile(const std::string& path) {
    return std::make_unique<WhiteboardDocument>();
}
void WhiteboardDocument::AutoLayout() {}

WhiteboardPanel::WhiteboardPanel(wxWindow* parent) : wxPanel(parent) {
    canvas_ = new WhiteboardCanvas(this);
}

wxBEGIN_EVENT_TABLE(WhiteboardPanel, wxPanel)
wxEND_EVENT_TABLE()

} // namespace scratchrobin
