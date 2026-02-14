/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "menu_builder.h"

#include "menu_ids.h"
#include "window_manager.h"

#include <algorithm>

#include <wx/artprov.h>
#include <wx/defs.h>
#include <wx/frame.h>

namespace scratchrobin {
namespace {

wxMenuItem* AppendItem(wxMenu* menu, int id, const wxString& label,
                       bool enabled = true, const wxString& help = wxEmptyString) {
    auto* item = menu->Append(id, label, help);
    item->Enable(enabled);
    return item;
}

wxMenu* BuildConnectionsMenu() {
    auto* menu = new wxMenu();

    // Connection management
    menu->Append(ID_CONN_MANAGE, "&Manage Connections...");
    menu->AppendSeparator();

    auto* server_menu = new wxMenu();
    AppendItem(server_menu, ID_CONN_SERVER_CREATE, "Create...", true);
    AppendItem(server_menu, ID_CONN_SERVER_CONNECT, "Connect...", true);
    AppendItem(server_menu, ID_CONN_SERVER_DISCONNECT, "Disconnect", true);
    AppendItem(server_menu, ID_CONN_SERVER_DROP, "Drop...", true);
    AppendItem(server_menu, ID_CONN_SERVER_REMOVE, "Remove from list...", true);
    menu->AppendSubMenu(server_menu, "Server");

    auto* cluster_menu = new wxMenu();
    AppendItem(cluster_menu, ID_CONN_CLUSTER_CREATE, "Create...", true);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_CONNECT, "Connect...", true);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_DISCONNECT, "Disconnect", true);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_DROP, "Drop...", true);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_REMOVE, "Remove from list...", true);
    menu->AppendSubMenu(cluster_menu, "Cluster");

    auto* database_menu = new wxMenu();
    AppendItem(database_menu, ID_CONN_DATABASE_CREATE, "Create...", true);
    AppendItem(database_menu, ID_CONN_DATABASE_CONNECT, "Connect...", true);
    AppendItem(database_menu, ID_CONN_DATABASE_DISCONNECT, "Disconnect", true);
    AppendItem(database_menu, ID_CONN_DATABASE_DROP, "Drop...", true);
    menu->AppendSubMenu(database_menu, "Database");

    auto* project_menu = new wxMenu();
    AppendItem(project_menu, ID_CONN_PROJECT_CREATE, "Create...", true);
    AppendItem(project_menu, ID_CONN_PROJECT_CONNECT, "Connect...", true);
    AppendItem(project_menu, ID_CONN_PROJECT_DISCONNECT, "Disconnect", true);
    AppendItem(project_menu, ID_CONN_PROJECT_DROP, "Drop...", true);
    menu->AppendSubMenu(project_menu, "Project");

    auto* diagram_menu = new wxMenu();
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_ERD, "Create ERD", true);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_FLOW, "Create Data Flow", true);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_UML, "Create UML", true);
    diagram_menu->AppendSeparator();
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_OPEN, "Open...", true);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_DROP, "Drop...", true);
    menu->AppendSubMenu(diagram_menu, "Diagram");

    auto* git_menu = new wxMenu();
    AppendItem(git_menu, ID_CONN_GIT_CONFIGURE, "Configure local identity...", true);
    AppendItem(git_menu, ID_CONN_GIT_CONNECT, "Connect to database/cluster Git...", true);
    AppendItem(git_menu, ID_CONN_GIT_OPEN, "Open project repo...", true);
    git_menu->AppendSeparator();
    AppendItem(git_menu, ID_CONN_GIT_STATUS, "Status", true);
    AppendItem(git_menu, ID_CONN_GIT_PULL, "Pull", true);
    AppendItem(git_menu, ID_CONN_GIT_PUSH, "Push", true);
    menu->AppendSubMenu(git_menu, "Git");

    menu->AppendSeparator();
    auto* recent_menu = new wxMenu();
    AppendItem(recent_menu, wxID_ANY, "No recent connections", true);
    menu->AppendSubMenu(recent_menu, "Recent / Quick Connections");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "Exit");

    return menu;
}

wxMenu* BuildEditMenu() {
    auto* menu = new wxMenu();
    menu->Append(wxID_CUT, "Cut");
    menu->Append(wxID_COPY, "Copy");
    menu->Append(wxID_PASTE, "Paste");
    menu->AppendSeparator();
    menu->Append(wxID_SELECTALL, "Select All");
    menu->AppendSeparator();
    menu->Append(ID_MENU_PREFERENCES, "Preferences...");
    return menu;
}

wxMenu* BuildObjectsMenu() {
    auto* menu = new wxMenu();
    menu->Append(ID_MENU_SCHEMA_MANAGER, "Schemas");
    menu->Append(ID_MENU_TABLE_DESIGNER, "Tables");
    menu->Append(ID_MENU_INDEX_DESIGNER, "Indexes");
    menu->Append(ID_MENU_DOMAIN_MANAGER, "Domains");
    menu->Append(ID_MENU_SEQUENCE_MANAGER, "Sequences");
    menu->Append(ID_MENU_VIEW_MANAGER, "Views");
    menu->Append(ID_MENU_TRIGGER_MANAGER, "Triggers");
    menu->Append(ID_MENU_PROCEDURE_MANAGER, "Procedures & Functions");
    menu->Append(ID_MENU_PACKAGE_MANAGER, "Packages");
    menu->AppendSeparator();
    menu->Append(ID_MENU_JOB_SCHEDULER, "Jobs");
    menu->Append(ID_MENU_USERS_ROLES, "Users & Roles");
    menu->Append(ID_MENU_RLS_POLICY_MANAGER, "Row-Level Security Policies");
    menu->Append(ID_MENU_AUDIT_POLICY, "Audit Policies");
    menu->Append(ID_MENU_PASSWORD_POLICY, "Password Policy");
    menu->Append(ID_MENU_LOCKOUT_POLICY, "Lockout Policy");
    menu->Append(ID_MENU_ROLE_SWITCH_POLICY, "Role Switch Policy");
    menu->Append(ID_MENU_POLICY_EPOCH_VIEWER, "Policy Epoch Viewer");
    menu->Append(ID_MENU_AUDIT_LOG_VIEWER, "Audit Log Viewer");
    return menu;
}

wxMenu* BuildAdminMenu() {
    auto* menu = new wxMenu();
    
    // Backup & Restore submenu
    auto* backup_menu = new wxMenu();
    backup_menu->Append(ID_MENU_BACKUP, "Backup Database...");
    backup_menu->Append(ID_MENU_RESTORE, "Restore Database...");
    backup_menu->AppendSeparator();
    backup_menu->Append(ID_MENU_BACKUP_HISTORY, "Backup History...");
    backup_menu->Append(ID_MENU_BACKUP_SCHEDULE, "Backup Schedule...");
    menu->AppendSubMenu(backup_menu, "Backup & Restore");
    
    menu->AppendSeparator();
    menu->Append(ID_MENU_STORAGE_MANAGER, "Storage Management...");
    menu->Append(ID_MENU_DATABASE_MANAGER, "Database Management...");
    return menu;
}

wxMenu* BuildToolsMenu() {
    auto* menu = new wxMenu();
    
    // Beta placeholder items - these open stub windows
    auto* cluster_item = AppendItem(menu, ID_MENU_CLUSTER_MANAGER, 
        "Cluster Manager...", true, 
        "High-availability cluster management (Beta Preview)");
    cluster_item->SetBitmap(wxArtProvider::GetBitmap(wxART_TIP, wxART_MENU, wxSize(16, 16)));
    
    auto* repl_item = AppendItem(menu, ID_MENU_REPLICATION_MANAGER, 
        "Replication Manager...", true,
        "Replication monitoring and management (Beta Preview)");
    repl_item->SetBitmap(wxArtProvider::GetBitmap(wxART_TIP, wxART_MENU, wxSize(16, 16)));
    
    auto* etl_item = AppendItem(menu, ID_MENU_ETL_MANAGER, 
        "ETL Manager...", true,
        "Extract, Transform, Load workflows (Beta Preview)");
    etl_item->SetBitmap(wxArtProvider::GetBitmap(wxART_TIP, wxART_MENU, wxSize(16, 16)));
    
    menu->AppendSeparator();
    
    auto* reporting_item = AppendItem(menu, ID_MENU_REPORTING,
        "Reporting & Analytics...", true,
        "Query builder, dashboards, and alerting (ScratchBird)");
    reporting_item->SetBitmap(wxArtProvider::GetBitmap(wxART_REPORT_VIEW, wxART_MENU, wxSize(16, 16)));
    
    auto* git_item = AppendItem(menu, ID_MENU_GIT_INTEGRATION, 
        "Git Integration...", true,
        "Version control for database schema (Beta Preview)");
    git_item->SetBitmap(wxArtProvider::GetBitmap(wxART_TIP, wxART_MENU, wxSize(16, 16)));
    
    menu->AppendSeparator();
    
    auto* beta_info = AppendItem(menu, wxID_ANY, 
        "About Beta Features...", true,
        "Learn about upcoming Beta features");
    beta_info->SetBitmap(wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MENU, wxSize(16, 16)));
    
    return menu;
}

wxMenu* BuildViewMenu() {
    auto* menu = new wxMenu();
    menu->AppendCheckItem(ID_MENU_TOGGLE_NAVIGATOR, "Navigator\tF8");
    menu->AppendSeparator();
    AppendItem(menu, wxID_ANY, "Toggle Panels", false);
    AppendItem(menu, wxID_ANY, "Refresh", false);
    menu->AppendSeparator();
    AppendItem(menu, ID_MENU_STATUS_MONITOR, "Status Monitor...", false);
    menu->AppendSeparator();
    AppendItem(menu, ID_MENU_CUSTOMIZE_TOOLBARS, "Customize Toolbars...", false);
    return menu;
}

void ClearMenu(wxMenu* menu) {
    if (!menu) {
        return;
    }
    while (menu->GetMenuItemCount() > 0) {
        auto* item = menu->FindItemByPosition(0);
        if (!item) {
            break;
        }
        menu->Destroy(item);
    }
}

void PopulateWindowMenu(wxMenu* menu,
                        WindowManager* window_manager,
                        wxFrame* current_frame) {
    ClearMenu(menu);
    if (!menu) {
        return;
    }
    if (!window_manager) {
        AppendItem(menu, wxID_ANY, "No windows", false);
        return;
    }
    auto windows = window_manager->GetWindows();
    if (windows.empty()) {
        AppendItem(menu, wxID_ANY, "No windows", false);
        return;
    }
    std::sort(windows.begin(), windows.end(),
              [](const wxFrame* a, const wxFrame* b) {
                  return a && b ? a->GetTitle().CmpNoCase(b->GetTitle()) < 0 : a < b;
              });
    for (auto* frame : windows) {
        if (!frame) {
            continue;
        }
        int id = wxWindow::NewControlId();
        auto* item = menu->AppendRadioItem(id, frame->GetTitle());
        if (frame == current_frame) {
            item->Check(true);
        }
        if (current_frame) {
            current_frame->Bind(wxEVT_MENU,
                [frame](wxCommandEvent&) {
                    if (!frame) {
                        return;
                    }
                    frame->Show(true);
                    frame->Raise();
                    frame->SetFocus();
                },
                id);
        }
    }
}

wxMenu* BuildWindowMenu(WindowManager* window_manager, wxFrame* current_frame) {
    auto* menu = new wxMenu();
    
    // Auto-Size Mode submenu
    auto* auto_size_menu = new wxMenu();
    auto_size_menu->AppendRadioItem(ID_MENU_AUTO_SIZE_COMPACT, "Compact (menu only)");
    auto_size_menu->AppendRadioItem(ID_MENU_AUTO_SIZE_ADAPTIVE, "Adaptive (grow/shrink)");
    auto_size_menu->AppendRadioItem(ID_MENU_AUTO_SIZE_FIXED, "Fixed (manual)");
    auto_size_menu->AppendRadioItem(ID_MENU_AUTO_SIZE_FULLSCREEN, "Fullscreen");
    menu->AppendSubMenu(auto_size_menu, "Auto-Size Mode");
    
    menu->AppendSeparator();
    menu->Append(ID_MENU_REMEMBER_SIZE, "Remember Current Size");
    menu->Append(ID_MENU_RESET_LAYOUT, "Reset to Default Layout");
    
    menu->AppendSeparator();
    
    // Window list (populated dynamically)
    PopulateWindowMenu(menu, window_manager, current_frame);
    
    return menu;
}

wxMenu* BuildHelpMenu() {
    auto* menu = new wxMenu();
    menu->Append(ID_MENU_HELP_WINDOW, "Help for this window");
    menu->Append(ID_MENU_HELP_COMMAND, "Help for selected command");
    menu->Append(ID_MENU_HELP_LANGUAGE, "Language guide");
    return menu;
}

} // namespace

wxMenuBar* BuildMenuBar(const MenuBuildOptions& options,
                        WindowManager* window_manager,
                        wxFrame* current_frame) {
    auto* menu_bar = new wxMenuBar();
    if (options.includeConnections) {
        menu_bar->Append(BuildConnectionsMenu(), "Connections");
    }
    if (options.includeObjects) {
        menu_bar->Append(BuildObjectsMenu(), "Objects");
    }
    if (options.includeEdit) {
        menu_bar->Append(BuildEditMenu(), "Edit");
    }
    if (options.includeView) {
        menu_bar->Append(BuildViewMenu(), "View");
    }
    if (options.includeAdmin) {
        menu_bar->Append(BuildAdminMenu(), "Admin");
    }
    if (options.includeTools) {
        menu_bar->Append(BuildToolsMenu(), "Tools");
    }
    if (options.includeWindow) {
        auto* window_menu = BuildWindowMenu(window_manager, current_frame);
        menu_bar->Append(window_menu, "Window");
        if (current_frame) {
            current_frame->Bind(wxEVT_MENU_OPEN,
                [window_menu, window_manager, current_frame](wxMenuEvent& event) {
                    if (event.GetMenu() == window_menu) {
                        PopulateWindowMenu(window_menu, window_manager, current_frame);
                    }
                    event.Skip();
                });
        }
    }
    if (options.includeHelp) {
        menu_bar->Append(BuildHelpMenu(), "Help");
    }
    return menu_bar;
}

wxMenuBar* BuildMenuBar(const MenuBuildOptions& options) {
    return BuildMenuBar(options, nullptr, nullptr);
}

wxMenuBar* BuildMinimalMenuBar(wxFrame* current_frame) {
    auto* menu_bar = new wxMenuBar();
    
    // File menu with just Close
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, "&Close\tCtrl+W", "Close this window");
    file_menu->AppendSeparator();
    file_menu->Append(wxID_EXIT, "E&xit\tCtrl+Q", "Exit application");
    menu_bar->Append(file_menu, "&File");
    
    // Help menu
    auto* help_menu = new wxMenu();
    help_menu->Append(wxID_HELP, "&Documentation\tF1");
    help_menu->Append(wxID_ABOUT, "&About...");
    menu_bar->Append(help_menu, "&Help");
    
    return menu_bar;
}

} // namespace scratchrobin
