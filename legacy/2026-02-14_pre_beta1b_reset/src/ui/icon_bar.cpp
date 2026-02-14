/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "icon_bar.h"

#include "draggable_toolbar.h"
#include "icon_bar_host.h"
#include "menu_ids.h"
#include "core/resource_paths.h"

#include <wx/artprov.h>
#include <wx/frame.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/stdpaths.h>

namespace scratchrobin {
namespace {

wxBitmap LoadIconBitmap(const wxString& name, int size, const wxArtID& fallback) {
    // Try the resource paths utility first (supports AppImage/portable)
    std::string path = ResourcePaths::GetIconPath(name.ToStdString(), size);
    if (wxFileName::FileExists(path)) {
        wxImage image;
        if (image.LoadFile(path, wxBITMAP_TYPE_PNG)) {
            return wxBitmap(image);
        }
    }
    
    // Fallback to local assets directory
    wxString filename = wxString::Format("assets/icons/%s@%d.png", name, size);
    if (wxFileName::FileExists(filename)) {
        wxImage image;
        if (image.LoadFile(filename, wxBITMAP_TYPE_PNG)) {
            return wxBitmap(image);
        }
    }
    return wxArtProvider::GetBitmap(fallback, wxART_TOOLBAR, wxSize(size, size));
}

void AddTool(wxToolBar* toolbar, int id, const wxString& label,
             const wxString& icon_name, int size, const wxArtID& fallback) {
    wxBitmap bitmap = LoadIconBitmap(icon_name, size, fallback);
    toolbar->AddTool(id, label, bitmap, label);
}

} // namespace

wxToolBar* BuildIconBar(wxFrame* frame, IconBarType type, int iconSize) {
    if (!frame) {
        return nullptr;
    }

    auto* toolbar = frame->CreateToolBar(wxTB_HORIZONTAL | wxTB_TEXT | wxTB_FLAT);
    toolbar->SetToolBitmapSize(wxSize(iconSize, iconSize));

    switch (type) {
        case IconBarType::Main:
            AddTool(toolbar, ID_MENU_NEW_SQL_EDITOR, "SQL", "sql", iconSize, wxART_NEW);
            AddTool(toolbar, ID_MENU_NEW_DIAGRAM, "Diagram", "diagram", iconSize, wxART_PASTE);
            AddTool(toolbar, ID_MENU_REPORTING, "Reporting", "reporting", iconSize, wxART_REPORT_VIEW);
            toolbar->AddSeparator();
            AddTool(toolbar, ID_MENU_MONITORING, "Monitor", "monitor", iconSize, wxART_TIP);
            AddTool(toolbar, ID_MENU_STATUS_MONITOR, "Status", "status", iconSize, wxART_INFORMATION);
            AddTool(toolbar, ID_MENU_USERS_ROLES, "Users", "users", iconSize, wxART_HELP_BOOK);
            break;
        case IconBarType::SqlEditor:
            AddTool(toolbar, ID_SQL_RUN, "Run", "run", iconSize, wxART_EXECUTABLE_FILE);
            AddTool(toolbar, ID_SQL_CANCEL, "Cancel", "cancel", iconSize, wxART_CROSS_MARK);
            toolbar->AddSeparator();
            AddTool(toolbar, ID_SQL_EXPORT_CSV, "Export CSV", "export_csv", iconSize, wxART_FILE_SAVE);
            AddTool(toolbar, ID_SQL_EXPORT_JSON, "Export JSON", "export_json", iconSize, wxART_FILE_SAVE_AS);
            break;
        case IconBarType::Monitoring:
            AddTool(toolbar, wxID_REFRESH, "Refresh", "refresh", iconSize, wxART_REDO);
            break;
        case IconBarType::UsersRoles:
            AddTool(toolbar, wxID_REFRESH, "Refresh", "refresh", iconSize, wxART_REDO);
            break;
        case IconBarType::Diagram:
            AddTool(toolbar, ID_MENU_NEW_DIAGRAM, "New", "diagram", iconSize, wxART_NEW);
            AddTool(toolbar, wxID_SAVE, "Save", "save", iconSize, wxART_FILE_SAVE);
            break;
    }

    toolbar->Realize();
    return toolbar;
}

wxString GetIconBarTypeName(IconBarType type) {
    switch (type) {
        case IconBarType::Main:
            return "Main";
        case IconBarType::SqlEditor:
            return "SqlEditor";
        case IconBarType::Monitoring:
            return "Monitoring";
        case IconBarType::UsersRoles:
            return "UsersRoles";
        case IconBarType::Diagram:
            return "Diagram";
    }
    return "Unknown";
}

DraggableToolBar* BuildDraggableIconBar(wxFrame* frame, IconBarHost* host,
                                        IconBarType type, int iconSize) {
    if (!frame) {
        return nullptr;
    }

    wxString name = GetIconBarTypeName(type);
    
    // Create draggable toolbar with appropriate style
    long style = wxTB_HORIZONTAL | wxTB_FLAT;
    if (type == IconBarType::Main) {
        style |= wxTB_TEXT;  // Show text labels on main toolbar
    }
    
    auto* toolbar = new DraggableToolBar(frame, name, wxID_ANY, style);
    toolbar->SetToolBitmapSize(wxSize(iconSize, iconSize));
    toolbar->SetIconBarHost(host);

    switch (type) {
        case IconBarType::Main:
            AddTool(toolbar, ID_MENU_NEW_SQL_EDITOR, "SQL", "sql", iconSize, wxART_NEW);
            AddTool(toolbar, ID_MENU_NEW_DIAGRAM, "Diagram", "diagram", iconSize, wxART_PASTE);
            AddTool(toolbar, ID_MENU_REPORTING, "Reporting", "reporting", iconSize, wxART_REPORT_VIEW);
            toolbar->AddSeparator();
            AddTool(toolbar, ID_MENU_MONITORING, "Monitor", "monitor", iconSize, wxART_TIP);
            AddTool(toolbar, ID_MENU_STATUS_MONITOR, "Status", "status", iconSize, wxART_INFORMATION);
            AddTool(toolbar, ID_MENU_USERS_ROLES, "Users", "users", iconSize, wxART_HELP_BOOK);
            break;
        case IconBarType::SqlEditor:
            AddTool(toolbar, ID_SQL_RUN, "Run", "run", iconSize, wxART_EXECUTABLE_FILE);
            AddTool(toolbar, ID_SQL_CANCEL, "Cancel", "cancel", iconSize, wxART_CROSS_MARK);
            toolbar->AddSeparator();
            AddTool(toolbar, ID_SQL_EXPORT_CSV, "Export CSV", "export_csv", iconSize, wxART_FILE_SAVE);
            AddTool(toolbar, ID_SQL_EXPORT_JSON, "Export JSON", "export_json", iconSize, wxART_FILE_SAVE_AS);
            break;
        case IconBarType::Monitoring:
            AddTool(toolbar, wxID_REFRESH, "Refresh", "refresh", iconSize, wxART_REDO);
            break;
        case IconBarType::UsersRoles:
            AddTool(toolbar, wxID_REFRESH, "Refresh", "refresh", iconSize, wxART_REDO);
            break;
        case IconBarType::Diagram:
            AddTool(toolbar, ID_MENU_NEW_DIAGRAM, "New", "diagram", iconSize, wxART_NEW);
            AddTool(toolbar, wxID_SAVE, "Save", "save", iconSize, wxART_FILE_SAVE);
            break;
    }

    toolbar->Realize();
    
    // Register with host
    if (host) {
        ToolBarInfo info;
        info.name = name;
        info.label = name + " Toolbar";
        info.visible = true;
        info.can_float = true;
        host->RegisterToolBar(toolbar, info);
    }
    
    return toolbar;
}

} // namespace scratchrobin
