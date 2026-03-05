/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/project_navigator.h"
#include "ui/window_framework/window_types.h"
#include "core/app_config.h"

namespace scratchrobin::ui {

BEGIN_EVENT_TABLE(ProjectNavigator, DockableWindow)
    EVT_TREE_SEL_CHANGED(wxID_ANY, ProjectNavigator::onTreeSelectionChanged)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, ProjectNavigator::onTreeItemActivated)
    EVT_TREE_ITEM_MENU(wxID_ANY, ProjectNavigator::onTreeContextMenu)
END_EVENT_TABLE()

ProjectNavigator::ProjectNavigator(wxWindow* parent, const std::string& instance_id)
    : DockableWindow(parent, WindowTypes::kProjectNavigator, instance_id) {
}

void ProjectNavigator::onWindowCreated() {
    // Create tree control
    long style = wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_HIDE_ROOT | 
                 wxTR_DEFAULT_STYLE | wxBORDER_THEME;
    tree_ctrl_ = new wxGenericTreeCtrl(this, wxID_ANY, wxDefaultPosition, 
                                       wxDefaultSize, style);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(tree_ctrl_, 1, wxEXPAND);
    SetSizer(sizer);
    
    // Populate tree
    buildTree();
}

wxSize ProjectNavigator::getDefaultSize() const {
    return wxSize(250, 600);
}

void ProjectNavigator::buildTree() {
    tree_ctrl_->DeleteAllItems();
    
    wxTreeItemId root = tree_ctrl_->AddRoot("Servers");
    
    // Add servers from config
    core::AppConfig& config = core::AppConfig::get();
    for (const auto& server : config.getServers()) {
        if (server.is_active) {
            addServer(root, server);
        }
    }
    
    tree_ctrl_->Expand(root);
}

void ProjectNavigator::addServer(wxTreeItemId parent, 
                                  const core::ServerConfig& server) {
    wxString label = wxString::FromUTF8(server.display_name.c_str());
    wxTreeItemId server_item = tree_ctrl_->AppendItem(parent, label);
    
    // Store server info
    TreeItemData* data = new TreeItemData();
    data->item_type = "server";
    data->server_id = server.server_id;
    tree_ctrl_->SetItemData(server_item, data);
    
    // Add engines
    for (const auto& engine : server.engines) {
        addEngine(server_item, engine, server.server_id);
    }
}

void ProjectNavigator::addEngine(wxTreeItemId parent,
                                  const core::DatabaseEngine& engine,
                                  const std::string& server_id) {
    wxString label = wxString::FromUTF8(dbEngineTypeToString(engine.engine_type).c_str());
    wxTreeItemId engine_item = tree_ctrl_->AppendItem(parent, label);
    
    TreeItemData* data = new TreeItemData();
    data->item_type = "engine";
    data->server_id = server_id;
    data->engine_type = dbEngineTypeToString(engine.engine_type);
    tree_ctrl_->SetItemData(engine_item, data);
    
    // Add databases
    for (const auto& db : engine.databases) {
        addDatabase(engine_item, db, server_id, data->engine_type);
    }
}

void ProjectNavigator::addDatabase(wxTreeItemId parent,
                                    const core::DatabaseConnection& db,
                                    const std::string& server_id,
                                    const std::string& engine_type) {
    wxString label = wxString::FromUTF8(db.database_name.c_str());
    wxTreeItemId db_item = tree_ctrl_->AppendItem(parent, label);
    
    TreeItemData* data = new TreeItemData();
    data->item_type = "database";
    data->server_id = server_id;
    data->engine_type = engine_type;
    data->database_name = db.database_name;
    tree_ctrl_->SetItemData(db_item, data);
}

std::vector<MenuContribution> ProjectNavigator::getMenuContributions() {
    std::vector<MenuContribution> menus;
    
    // View menu contributions
    MenuContribution refresh;
    refresh.menu_name = "View";
    refresh.item_label = "&Refresh Tree";
    refresh.item_help = "Refresh the project navigator tree";
    refresh.item_id = MENU_REFRESH;
    refresh.callback = [this]() { this->refreshTree(); };
    refresh.accelerator = "F5";
    menus.push_back(refresh);
    
    MenuContribution filter;
    filter.menu_name = "View";
    filter.item_label = "&Filter...";
    filter.item_help = "Filter tree items";
    filter.item_id = MENU_FILTER;
    menus.push_back(filter);
    
    // Database menu contributions
    MenuContribution newConn;
    newConn.menu_name = "Database";
    newConn.item_label = "&New Connection...";
    newConn.item_help = "Create a new database connection";
    newConn.item_id = MENU_NEW_CONNECTION;
    menus.push_back(newConn);
    
    MenuContribution disconnect;
    disconnect.menu_name = "Database";
    disconnect.item_label = "&Disconnect";
    disconnect.item_help = "Disconnect from selected server";
    disconnect.item_id = MENU_DISCONNECT;
    menus.push_back(disconnect);
    
    return menus;
}

void ProjectNavigator::onActivated() {
    DockableWindow::onActivated();
    // Refresh on activation
    refreshTree();
}

void ProjectNavigator::onTreeSelectionChanged(wxTreeEvent& event) {
    // Handle selection change
    event.Skip();
}

void ProjectNavigator::onTreeItemActivated(wxTreeEvent& event) {
    // Handle double-click/activation
    event.Skip();
}

void ProjectNavigator::onTreeContextMenu(wxTreeEvent& event) {
    // Show context menu
    event.Skip();
}

void ProjectNavigator::onMenuRefresh(wxCommandEvent& event) {
    refreshTree();
}

void ProjectNavigator::onMenuNewConnection(wxCommandEvent& event) {
    // Show new connection dialog
}

void ProjectNavigator::onMenuDisconnect(wxCommandEvent& event) {
    // Disconnect from selected server
}

void ProjectNavigator::onMenuFilter(wxCommandEvent& event) {
    // Show filter dialog
}

void ProjectNavigator::populateFromConfig() {
    buildTree();
}

void ProjectNavigator::selectServer(const std::string& server_id) {
    // Find and select server in tree
}

void ProjectNavigator::selectDatabase(const std::string& server_id, 
                                       const std::string& db_name) {
    // Find and select database in tree
}

void ProjectNavigator::refreshTree() {
    buildTree();
}

void ProjectNavigator::refreshServer(const std::string& server_id) {
    // Refresh specific server node
    refreshTree();
}

}  // namespace scratchrobin::ui
