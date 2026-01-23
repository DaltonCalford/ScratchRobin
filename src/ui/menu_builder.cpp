#include "menu_builder.h"

#include "menu_ids.h"

#include <wx/defs.h>

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

wxMenu* BuildViewMenu() {
    auto* menu = new wxMenu();
    AppendItem(menu, wxID_ANY, "Toggle Panels", false);
    AppendItem(menu, wxID_ANY, "Refresh", false);
    return menu;
}

wxMenu* BuildWindowMenu() {
    auto* menu = new wxMenu();
    menu->Append(ID_MENU_NEW_SQL_EDITOR, "New SQL Editor");
    menu->Append(ID_MENU_NEW_DIAGRAM, "New Diagram");
    menu->AppendSeparator();
    menu->Append(ID_MENU_MONITORING, "Monitoring");
    menu->Append(ID_MENU_USERS_ROLES, "Users & Roles");
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

wxMenuBar* BuildMenuBar(const MenuBuildOptions& options) {
    auto* menu_bar = new wxMenuBar();
    if (options.includeConnections) {
        menu_bar->Append(BuildConnectionsMenu(), "Connections");
    }
    if (options.includeEdit) {
        menu_bar->Append(BuildEditMenu(), "Edit");
    }
    if (options.includeView) {
        menu_bar->Append(BuildViewMenu(), "View");
    }
    if (options.includeWindow) {
        menu_bar->Append(BuildWindowMenu(), "Window");
    }
    if (options.includeHelp) {
        menu_bar->Append(BuildHelpMenu(), "Help");
    }
    return menu_bar;
}

} // namespace scratchrobin
