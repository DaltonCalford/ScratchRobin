#include "menu_builder.h"

#include "menu_ids.h"
#include "window_manager.h"

#include <algorithm>

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

    auto* server_menu = new wxMenu();
    AppendItem(server_menu, ID_CONN_SERVER_CREATE, "Create...", false);
    AppendItem(server_menu, ID_CONN_SERVER_CONNECT, "Connect...", false);
    AppendItem(server_menu, ID_CONN_SERVER_DISCONNECT, "Disconnect", false);
    AppendItem(server_menu, ID_CONN_SERVER_DROP, "Drop...", false);
    AppendItem(server_menu, ID_CONN_SERVER_REMOVE, "Remove from list...", false);
    menu->AppendSubMenu(server_menu, "Server");

    auto* cluster_menu = new wxMenu();
    AppendItem(cluster_menu, ID_CONN_CLUSTER_CREATE, "Create...", false);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_CONNECT, "Connect...", false);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_DISCONNECT, "Disconnect", false);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_DROP, "Drop...", false);
    AppendItem(cluster_menu, ID_CONN_CLUSTER_REMOVE, "Remove from list...", false);
    menu->AppendSubMenu(cluster_menu, "Cluster");

    auto* database_menu = new wxMenu();
    AppendItem(database_menu, ID_CONN_DATABASE_CREATE, "Create...", false);
    AppendItem(database_menu, ID_CONN_DATABASE_CONNECT, "Connect...", false);
    AppendItem(database_menu, ID_CONN_DATABASE_DISCONNECT, "Disconnect", false);
    AppendItem(database_menu, ID_CONN_DATABASE_DROP, "Drop...", false);
    menu->AppendSubMenu(database_menu, "Database");

    auto* project_menu = new wxMenu();
    AppendItem(project_menu, ID_CONN_PROJECT_CREATE, "Create...", false);
    AppendItem(project_menu, ID_CONN_PROJECT_CONNECT, "Connect...", false);
    AppendItem(project_menu, ID_CONN_PROJECT_DISCONNECT, "Disconnect", false);
    AppendItem(project_menu, ID_CONN_PROJECT_DROP, "Drop...", false);
    menu->AppendSubMenu(project_menu, "Project");

    auto* diagram_menu = new wxMenu();
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_ERD, "Create ERD", false);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_FLOW, "Create Data Flow", false);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_CREATE_UML, "Create UML", false);
    diagram_menu->AppendSeparator();
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_OPEN, "Open...", false);
    AppendItem(diagram_menu, ID_CONN_DIAGRAM_DROP, "Drop...", false);
    menu->AppendSubMenu(diagram_menu, "Diagram");

    auto* git_menu = new wxMenu();
    AppendItem(git_menu, ID_CONN_GIT_CONFIGURE, "Configure local identity...", false);
    AppendItem(git_menu, ID_CONN_GIT_CONNECT, "Connect to database/cluster Git...", false);
    AppendItem(git_menu, ID_CONN_GIT_OPEN, "Open project repo...", false);
    git_menu->AppendSeparator();
    AppendItem(git_menu, ID_CONN_GIT_STATUS, "Status", false);
    AppendItem(git_menu, ID_CONN_GIT_PULL, "Pull", false);
    AppendItem(git_menu, ID_CONN_GIT_PUSH, "Push", false);
    menu->AppendSubMenu(git_menu, "Git");

    menu->AppendSeparator();
    auto* recent_menu = new wxMenu();
    AppendItem(recent_menu, wxID_ANY, "No recent connections", false);
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
    return menu;
}

wxMenu* BuildObjectsMenu() {
    auto* menu = new wxMenu();
    menu->Append(ID_MENU_SCHEMA_MANAGER, "Schemas");
    menu->Append(ID_MENU_TABLE_DESIGNER, "Tables");
    menu->Append(ID_MENU_INDEX_DESIGNER, "Indexes");
    menu->Append(ID_MENU_DOMAIN_MANAGER, "Domains");
    menu->Append(ID_MENU_JOB_SCHEDULER, "Jobs");
    menu->Append(ID_MENU_USERS_ROLES, "Users & Roles");
    return menu;
}

wxMenu* BuildViewMenu() {
    auto* menu = new wxMenu();
    AppendItem(menu, wxID_ANY, "Toggle Panels", false);
    AppendItem(menu, wxID_ANY, "Refresh", false);
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

} // namespace scratchrobin
