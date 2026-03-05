/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

// Include wxWidgets tree control headers in correct order
#include <wx/treectrl.h>
#include <wx/generic/treectlg.h>
#include "ui/window_framework/dockable_window.h"

// Forward declarations for core types
namespace scratchrobin::core {
    struct ServerConfig;
    struct DatabaseEngine;
    struct DatabaseConnection;
}

namespace scratchrobin::ui {

// Project navigator - tree view of servers/databases/objects
// Can be docked left or right
class ProjectNavigator : public DockableWindow {
public:
    ProjectNavigator(wxWindow* parent, const std::string& instance_id = "");
    
    // Tree access
    wxGenericTreeCtrl* getTreeCtrl() { return tree_ctrl_; }
    
    // Populate from config
    void populateFromConfig();
    
    // Selection
    void selectServer(const std::string& server_id);
    void selectDatabase(const std::string& server_id, const std::string& db_name);
    
    // Refresh
    void refreshTree();
    void refreshServer(const std::string& server_id);
    
    // Override from DockableWindow
    std::vector<MenuContribution> getMenuContributions() override;
    void onActivated() override;
    
    // Menu contribution IDs
    static constexpr int MENU_REFRESH = wxID_HIGHEST + 2000;
    static constexpr int MENU_NEW_CONNECTION = wxID_HIGHEST + 2001;
    static constexpr int MENU_DISCONNECT = wxID_HIGHEST + 2002;
    static constexpr int MENU_FILTER = wxID_HIGHEST + 2003;

protected:
    void onWindowCreated() override;
    wxSize getDefaultSize() const override;

private:
    wxGenericTreeCtrl* tree_ctrl_{nullptr};
    
    // Tree item data
    struct TreeItemData : public wxTreeItemData {
        std::string item_type;      // "server", "engine", "database", "folder", "object"
        std::string server_id;
        std::string engine_type;
        std::string database_name;
        std::string object_type;
    };
    
    // Build tree
    void buildTree();
    void addServer(wxTreeItemId parent, const class core::ServerConfig& server);
    void addEngine(wxTreeItemId parent, const class core::DatabaseEngine& engine, 
                   const std::string& server_id);
    void addDatabase(wxTreeItemId parent, const class core::DatabaseConnection& db,
                     const std::string& server_id, const std::string& engine_type);
    
    // Event handlers
    void onTreeSelectionChanged(wxTreeEvent& event);
    void onTreeItemActivated(wxTreeEvent& event);
    void onTreeContextMenu(wxTreeEvent& event);
    void onMenuRefresh(wxCommandEvent& event);
    void onMenuNewConnection(wxCommandEvent& event);
    void onMenuDisconnect(wxCommandEvent& event);
    void onMenuFilter(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

}  // namespace scratchrobin::ui
