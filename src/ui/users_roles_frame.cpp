/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "users_roles_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "icon_bar.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "privilege_editor_dialog.h"
#include "result_grid_table.h"
#include "role_editor_dialog.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "user_editor_dialog.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>

#include <wx/bookctrl.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>

namespace scratchrobin {

namespace {
constexpr int kMenuConnect = wxID_HIGHEST + 40;
constexpr int kMenuDisconnect = wxID_HIGHEST + 41;
constexpr int kMenuRefresh = wxID_HIGHEST + 42;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 43;
constexpr int kNotebookId = wxID_HIGHEST + 44;
constexpr int kCreateUserId = wxID_HIGHEST + 45;
constexpr int kDropUserId = wxID_HIGHEST + 46;
constexpr int kCreateRoleId = wxID_HIGHEST + 47;
constexpr int kDropRoleId = wxID_HIGHEST + 48;
constexpr int kGrantRoleId = wxID_HIGHEST + 49;
constexpr int kRevokeRoleId = wxID_HIGHEST + 50;
constexpr int kGrantMembershipId = wxID_HIGHEST + 51;
constexpr int kRevokeMembershipId = wxID_HIGHEST + 52;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = ToLowerCopy(Trim(raw));
    if (value.empty() || value == "network" || value == "scratchbird") {
        return "native";
    }
    if (value == "postgres" || value == "pg") {
        return "postgresql";
    }
    if (value == "mariadb") {
        return "mysql";
    }
    if (value == "fb") {
        return "firebird";
    }
    return value;
}

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.database.empty()) {
        return profile.database;
    }
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) {
        label += ":" + std::to_string(profile.port);
    }
    return label;
}

bool BuildUsersQuery(const std::string& backend,
                     std::string* out_query,
                     std::string* out_message) {
    if (!out_query) {
        return false;
    }
    out_query->clear();

    if (backend == "native") {
        *out_query =
            "SELECT user_name, is_superuser, default_schema, created_at, last_login_at, "
            "auth_provider, password_state "
            "FROM sys.users ORDER BY user_name;";
        return true;
    }
    if (backend == "postgresql") {
        *out_query =
            "SELECT rolname AS name, rolsuper, rolcreatedb, rolcreaterole, "
            "rolreplication, rolbypassrls, rolvaliduntil "
            "FROM pg_roles WHERE rolcanlogin ORDER BY rolname;";
        return true;
    }
    if (backend == "mysql") {
        *out_query =
            "SELECT user, host FROM mysql.user ORDER BY user, host;";
        return true;
    }
    if (backend == "firebird") {
        *out_query =
            "SELECT DISTINCT RDB$USER FROM RDB$USER_PRIVILEGES "
            "WHERE RDB$USER_TYPE = 8 ORDER BY RDB$USER;";
        return true;
    }

    if (out_message) {
        if (backend == "native") {
            *out_message = "ScratchBird user catalogs are pending on listeners.";
        } else {
            *out_message = "Unsupported backend for user listing: " + backend;
        }
    }
    return false;
}

bool BuildRolesQuery(const std::string& backend,
                     std::string* out_query,
                     std::string* out_message) {
    if (!out_query) {
        return false;
    }
    out_query->clear();

    if (backend == "native") {
        *out_query =
            "SELECT role_name, can_login, is_superuser, is_system_role, "
            "default_schema, created_at "
            "FROM sys.roles ORDER BY role_name;";
        return true;
    }
    if (backend == "postgresql") {
        *out_query =
            "SELECT rolname AS role, rolsuper, rolcreatedb, rolcreaterole, "
            "rolcanlogin, rolreplication, rolbypassrls "
            "FROM pg_roles ORDER BY rolname;";
        return true;
    }
    if (backend == "mysql") {
        *out_query =
            "SELECT user AS role, host FROM mysql.user WHERE is_role = 'Y' "
            "ORDER BY user, host;";
        return true;
    }
    if (backend == "firebird") {
        *out_query =
            "SELECT RDB$ROLE_NAME FROM RDB$ROLES "
            "WHERE RDB$SYSTEM_FLAG = 0 ORDER BY RDB$ROLE_NAME;";
        return true;
    }

    if (out_message) {
        if (backend == "native") {
            *out_message = "ScratchBird role catalogs are pending on listeners.";
        } else {
            *out_message = "Unsupported backend for role listing: " + backend;
        }
    }
    return false;
}

bool BuildMembershipsQuery(const std::string& backend,
                           std::string* out_query,
                           std::string* out_message) {
    if (!out_query) {
        return false;
    }
    out_query->clear();

    if (backend == "native") {
        *out_query =
            "SELECT role_name, member_name, admin_option, is_default, granted_at "
            "FROM sys.role_members ORDER BY role_name, member_name;";
        return true;
    }
    if (backend == "postgresql") {
        *out_query =
            "SELECT r.rolname AS role, m.rolname AS member, a.admin_option "
            "FROM pg_auth_members a "
            "JOIN pg_roles r ON r.oid = a.roleid "
            "JOIN pg_roles m ON m.oid = a.member "
            "ORDER BY r.rolname, m.rolname;";
        return true;
    }
    if (backend == "mysql") {
        *out_query =
            "SELECT FROM_USER AS role, "
            "CONCAT(TO_USER, '@', TO_HOST) AS member, "
            "WITH_ADMIN_OPTION AS admin_option "
            "FROM mysql.role_edges "
            "ORDER BY FROM_USER, TO_USER;";
        return true;
    }
    if (backend == "firebird") {
        *out_query =
            "SELECT TRIM(RDB$RELATION_NAME) AS role, "
            "TRIM(RDB$USER) AS member, "
            "RDB$GRANT_OPTION AS admin_option "
            "FROM RDB$USER_PRIVILEGES "
            "WHERE RDB$OBJECT_TYPE = 13 "
            "ORDER BY RDB$RELATION_NAME, RDB$USER;";
        return true;
    }

    if (out_message) {
        if (backend == "native") {
            *out_message = "ScratchBird group catalogs are pending on listeners.";
        } else {
            *out_message = "Unsupported backend for memberships: " + backend;
        }
    }
    return false;
}

std::string BuildCreateUserTemplate(const std::string& backend) {
    if (backend == "native") {
        return "CREATE USER user_name WITH PASSWORD 'password';";
    }
    if (backend == "postgresql") {
        return "CREATE ROLE user_name WITH LOGIN PASSWORD 'password';";
    }
    if (backend == "mysql") {
        return "CREATE USER 'user'@'host' IDENTIFIED BY 'password';";
    }
    if (backend == "firebird") {
        return "CREATE USER user_name PASSWORD 'password';";
    }
    return "-- User creation not supported for this backend.";
}

std::string BuildDropUserTemplate(const std::string& backend, const std::string& name) {
    std::string target = name.empty() ? "user_name" : name;
    if (backend == "native") {
        return "DROP USER " + target + ";";
    }
    if (backend == "postgresql") {
        return "DROP ROLE " + target + ";";
    }
    if (backend == "mysql") {
        return "DROP USER '" + target + "'@'host';";
    }
    if (backend == "firebird") {
        return "DROP USER " + target + ";";
    }
    return "-- User drop not supported for this backend.";
}

std::string BuildCreateRoleTemplate(const std::string& backend) {
    if (backend == "native") {
        return "CREATE ROLE role_name NOLOGIN;";
    }
    if (backend == "postgresql") {
        return "CREATE ROLE role_name;";
    }
    if (backend == "mysql") {
        return "CREATE ROLE 'role'@'host';";
    }
    if (backend == "firebird") {
        return "CREATE ROLE role_name;";
    }
    return "-- Role creation not supported for this backend.";
}

std::string BuildDropRoleTemplate(const std::string& backend, const std::string& name) {
    std::string target = name.empty() ? "role_name" : name;
    if (backend == "native") {
        return "DROP ROLE " + target + ";";
    }
    if (backend == "postgresql") {
        return "DROP ROLE " + target + ";";
    }
    if (backend == "mysql") {
        return "DROP ROLE '" + target + "'@'host';";
    }
    if (backend == "firebird") {
        return "DROP ROLE " + target + ";";
    }
    return "-- Role drop not supported for this backend.";
}

std::string BuildGrantRoleTemplate(const std::string& backend, const std::string& role) {
    std::string target = role.empty() ? "role_name" : role;
    if (backend == "native") {
        return "GRANT " + target + " TO user_name;";
    }
    if (backend == "postgresql") {
        return "GRANT " + target + " TO user_name;";
    }
    if (backend == "mysql") {
        return "GRANT " + target + " TO 'user'@'host';";
    }
    if (backend == "firebird") {
        return "GRANT " + target + " TO user_name;";
    }
    return "-- Role grant not supported for this backend.";
}

std::string BuildRevokeRoleTemplate(const std::string& backend, const std::string& role) {
    std::string target = role.empty() ? "role_name" : role;
    if (backend == "native") {
        return "REVOKE " + target + " FROM user_name;";
    }
    if (backend == "postgresql") {
        return "REVOKE " + target + " FROM user_name;";
    }
    if (backend == "mysql") {
        return "REVOKE " + target + " FROM 'user'@'host';";
    }
    if (backend == "firebird") {
        return "REVOKE " + target + " FROM user_name;";
    }
    return "-- Role revoke not supported for this backend.";
}

std::string BuildGrantMembershipTemplate(const std::string& backend,
                                         const std::string& role,
                                         const std::string& member) {
    std::string role_name = role.empty() ? "role_name" : role;
    std::string member_name = member.empty() ? "user_name" : member;
    if (backend == "native") {
        return "GRANT " + role_name + " TO " + member_name + ";";
    }
    if (backend == "postgresql") {
        return "GRANT " + role_name + " TO " + member_name + ";";
    }
    if (backend == "mysql") {
        return "GRANT " + role_name + " TO " + member_name + ";";
    }
    if (backend == "firebird") {
        return "GRANT " + role_name + " TO " + member_name + ";";
    }
    return "-- Membership grant not supported for this backend.";
}

std::string BuildRevokeMembershipTemplate(const std::string& backend,
                                          const std::string& role,
                                          const std::string& member) {
    std::string role_name = role.empty() ? "role_name" : role;
    std::string member_name = member.empty() ? "user_name" : member;
    if (backend == "native") {
        return "REVOKE " + role_name + " FROM " + member_name + ";";
    }
    if (backend == "postgresql") {
        return "REVOKE " + role_name + " FROM " + member_name + ";";
    }
    if (backend == "mysql") {
        return "REVOKE " + role_name + " FROM " + member_name + ";";
    }
    if (backend == "firebird") {
        return "REVOKE " + role_name + " FROM " + member_name + ";";
    }
    return "-- Membership revoke not supported for this backend.";
}

} // namespace

wxBEGIN_EVENT_TABLE(UsersRolesFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, UsersRolesFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, UsersRolesFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, UsersRolesFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, UsersRolesFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, UsersRolesFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, UsersRolesFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, UsersRolesFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, UsersRolesFrame::OnOpenIndexDesigner)
    EVT_MENU(wxID_REFRESH, UsersRolesFrame::OnRefresh)
    EVT_BUTTON(kMenuConnect, UsersRolesFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, UsersRolesFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, UsersRolesFrame::OnRefresh)
    EVT_CHOICE(kConnectionChoiceId, UsersRolesFrame::OnConnectionChanged)
    EVT_NOTEBOOK_PAGE_CHANGED(kNotebookId, UsersRolesFrame::OnTabChanged)
    EVT_BUTTON(kCreateUserId, UsersRolesFrame::OnCreateUser)
    EVT_BUTTON(kDropUserId, UsersRolesFrame::OnDropUser)
    EVT_BUTTON(kCreateRoleId, UsersRolesFrame::OnCreateRole)
    EVT_BUTTON(kDropRoleId, UsersRolesFrame::OnDropRole)
    EVT_BUTTON(kGrantRoleId, UsersRolesFrame::OnGrantRole)
    EVT_BUTTON(kRevokeRoleId, UsersRolesFrame::OnRevokeRole)
    EVT_BUTTON(kGrantMembershipId, UsersRolesFrame::OnGrantMembership)
    EVT_BUTTON(kRevokeMembershipId, UsersRolesFrame::OnRevokeMembership)
    EVT_CLOSE(UsersRolesFrame::OnClose)
wxEND_EVENT_TABLE()

UsersRolesFrame::UsersRolesFrame(WindowManager* windowManager,
                                 ConnectionManager* connectionManager,
                                 const std::vector<ConnectionProfile>* connections,
                                 const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Users & Roles", wxDefaultPosition, wxSize(1000, 720)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    if (app_config_ && app_config_->chrome.usersRoles.showIconBar) {
        IconBarType type = app_config_->chrome.usersRoles.replicateIconBar
                               ? IconBarType::Main
                               : IconBarType::UsersRoles;
        BuildIconBar(this, type, 24);
    }
    BuildLayout();
    PopulateConnections();
    UpdateControls();
    UpdateStatus("Ready");

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void UsersRolesFrame::BuildMenu() {
    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.usersRoles;
    }
    if (!chrome.showMenu) {
        return;
    }
    MenuBuildOptions options;
    options.includeConnections = chrome.replicateMenu;
    auto* menu_bar = BuildMenuBar(options, window_manager_, this);
    SetMenuBar(menu_bar);
}

void UsersRolesFrame::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, kMenuConnect, "Connect");
    disconnect_button_ = new wxButton(topPanel, kMenuDisconnect, "Disconnect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    refresh_button_ = new wxButton(topPanel, kMenuRefresh, "Refresh");
    topSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    topSizer->AddStretchSpacer(1);
    status_label_ = new wxStaticText(topPanel, wxID_ANY, "Ready");
    topSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    topPanel->SetSizer(topSizer);
    root->Add(topPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

    notebook_ = new wxNotebook(this, kNotebookId);

    auto* usersPage = new wxPanel(notebook_, wxID_ANY);
    auto* usersSizer = new wxBoxSizer(wxVERTICAL);
    auto* usersButtons = new wxBoxSizer(wxHORIZONTAL);
    create_user_button_ = new wxButton(usersPage, kCreateUserId, "Create User");
    auto* alter_user_btn = new wxButton(usersPage, wxID_ANY, "Alter User");
    drop_user_button_ = new wxButton(usersPage, kDropUserId, "Drop User");
    usersButtons->Add(create_user_button_, 0, wxRIGHT, 6);
    usersButtons->Add(alter_user_btn, 0, wxRIGHT, 6);
    usersButtons->Add(drop_user_button_, 0, wxRIGHT, 6);
    usersButtons->AddStretchSpacer(1);
    usersSizer->Add(usersButtons, 0, wxEXPAND | wxALL, 6);
    alter_user_btn->Bind(wxEVT_BUTTON, &UsersRolesFrame::OnAlterUser, this);

    users_grid_ = new wxGrid(usersPage, wxID_ANY);
    users_table_ = new ResultGridTable();
    users_grid_->SetTable(users_table_, true);
    users_grid_->EnableEditing(false);
    users_grid_->SetRowLabelSize(64);
    usersSizer->Add(users_grid_, 1, wxEXPAND | wxALL, 8);
    usersPage->SetSizer(usersSizer);

    auto* rolesPage = new wxPanel(notebook_, wxID_ANY);
    auto* rolesSizer = new wxBoxSizer(wxVERTICAL);
    auto* rolesButtons = new wxBoxSizer(wxHORIZONTAL);
    create_role_button_ = new wxButton(rolesPage, kCreateRoleId, "Create Role");
    auto* alter_role_btn = new wxButton(rolesPage, wxID_ANY, "Alter Role");
    drop_role_button_ = new wxButton(rolesPage, kDropRoleId, "Drop Role");
    grant_role_button_ = new wxButton(rolesPage, kGrantRoleId, "Grant Role");
    revoke_role_button_ = new wxButton(rolesPage, kRevokeRoleId, "Revoke Role");
    rolesButtons->Add(create_role_button_, 0, wxRIGHT, 6);
    rolesButtons->Add(alter_role_btn, 0, wxRIGHT, 6);
    rolesButtons->Add(drop_role_button_, 0, wxRIGHT, 6);
    rolesButtons->Add(grant_role_button_, 0, wxRIGHT, 6);
    rolesButtons->Add(revoke_role_button_, 0, wxRIGHT, 6);
    rolesButtons->AddStretchSpacer(1);
    rolesSizer->Add(rolesButtons, 0, wxEXPAND | wxALL, 6);
    alter_role_btn->Bind(wxEVT_BUTTON, &UsersRolesFrame::OnAlterRole, this);
    
    // Privileges section
    auto* privButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* grant_priv_btn = new wxButton(rolesPage, wxID_ANY, "Grant Privileges");
    auto* revoke_priv_btn = new wxButton(rolesPage, wxID_ANY, "Revoke Privileges");
    privButtons->Add(grant_priv_btn, 0, wxRIGHT, 6);
    privButtons->Add(revoke_priv_btn, 0, wxRIGHT, 6);
    privButtons->AddStretchSpacer(1);
    rolesSizer->Add(privButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    grant_priv_btn->Bind(wxEVT_BUTTON, &UsersRolesFrame::OnGrantPrivileges, this);
    revoke_priv_btn->Bind(wxEVT_BUTTON, &UsersRolesFrame::OnRevokePrivileges, this);

    roles_grid_ = new wxGrid(rolesPage, wxID_ANY);
    roles_table_ = new ResultGridTable();
    roles_grid_->SetTable(roles_table_, true);
    roles_grid_->EnableEditing(false);
    roles_grid_->SetRowLabelSize(64);
    rolesSizer->Add(roles_grid_, 1, wxEXPAND | wxALL, 8);
    rolesPage->SetSizer(rolesSizer);

    users_tab_index_ = notebook_->GetPageCount();
    notebook_->AddPage(usersPage, "Users");
    roles_tab_index_ = notebook_->GetPageCount();
    notebook_->AddPage(rolesPage, "Roles");

    auto* membershipsPage = new wxPanel(notebook_, wxID_ANY);
    auto* membershipsSizer = new wxBoxSizer(wxVERTICAL);
    auto* membershipsButtons = new wxBoxSizer(wxHORIZONTAL);
    grant_membership_button_ = new wxButton(membershipsPage, kGrantMembershipId, "Grant Membership");
    revoke_membership_button_ = new wxButton(membershipsPage, kRevokeMembershipId, "Revoke Membership");
    membershipsButtons->Add(grant_membership_button_, 0, wxRIGHT, 6);
    membershipsButtons->Add(revoke_membership_button_, 0, wxRIGHT, 6);
    membershipsButtons->AddStretchSpacer(1);
    membershipsSizer->Add(membershipsButtons, 0, wxEXPAND | wxALL, 6);

    memberships_grid_ = new wxGrid(membershipsPage, wxID_ANY);
    memberships_table_ = new ResultGridTable();
    memberships_grid_->SetTable(memberships_table_, true);
    memberships_grid_->EnableEditing(false);
    memberships_grid_->SetRowLabelSize(64);
    membershipsSizer->Add(memberships_grid_, 1, wxEXPAND | wxALL, 8);
    membershipsPage->SetSizer(membershipsSizer);

    memberships_tab_index_ = notebook_->GetPageCount();
    notebook_->AddPage(membershipsPage, "Memberships");

    root->Add(notebook_, 1, wxEXPAND | wxALL, 4);

    auto* messagePanel = new wxPanel(this, wxID_ANY);
    auto* messageSizer = new wxBoxSizer(wxVERTICAL);
    message_log_ = new wxTextCtrl(messagePanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    messageSizer->Add(message_log_, 1, wxEXPAND | wxALL, 8);
    messagePanel->SetSizer(messageSizer);
    root->Add(messagePanel, 0, wxEXPAND);

    SetSizer(root);
}

void UsersRolesFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    if (!connections_) {
        return;
    }
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    if (!connections_->empty()) {
        connection_choice_->SetSelection(0);
    }
}

const ConnectionProfile* UsersRolesFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int selection = connection_choice_->GetSelection();
    if (selection < 0 || static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(selection)];
}

void UsersRolesFrame::UpdateControls() {
    bool has_connections = connections_ && !connections_->empty();
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    BackendCapabilities caps = connection_manager_ ? connection_manager_->Capabilities()
                                                   : BackendCapabilities{};

    if (connection_choice_) {
        connection_choice_->Enable(has_connections && !connect_running_ && !query_running_);
    }
    if (connect_button_) {
        connect_button_->Enable(has_connections && !connected && !connect_running_ && !query_running_);
    }
    if (disconnect_button_) {
        disconnect_button_->Enable(connected && !connect_running_ && !query_running_);
    }
    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }

    bool allow_user_admin = connected && caps.supportsUserAdmin && !query_running_;
    if (create_user_button_) {
        create_user_button_->Enable(allow_user_admin);
    }
    if (drop_user_button_) {
        drop_user_button_->Enable(allow_user_admin);
    }

    bool allow_role_admin = connected && caps.supportsRoleAdmin && !query_running_;
    if (create_role_button_) {
        create_role_button_->Enable(allow_role_admin);
    }
    if (drop_role_button_) {
        drop_role_button_->Enable(allow_role_admin);
    }
    if (grant_role_button_) {
        grant_role_button_->Enable(allow_role_admin);
    }
    if (revoke_role_button_) {
        revoke_role_button_->Enable(allow_role_admin);
    }

    bool allow_group_admin = connected && caps.supportsGroupAdmin && !query_running_;
    if (grant_membership_button_) {
        grant_membership_button_->Enable(allow_group_admin);
    }
    if (revoke_membership_button_) {
        revoke_membership_button_->Enable(allow_group_admin);
    }
}

void UsersRolesFrame::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void UsersRolesFrame::SetMessage(const std::string& message) {
    if (message_log_) {
        message_log_->SetValue(wxString::FromUTF8(message));
    }
}

void UsersRolesFrame::OnConnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        UpdateStatus("No connection profile selected");
        return;
    }
    if (connect_running_) {
        return;
    }
    connect_running_ = true;
    UpdateControls();
    UpdateStatus("Connecting...");
    SetMessage(std::string());

    connect_job_ = connection_manager_->ConnectAsync(*profile,
        [this](bool ok, const std::string& error) {
            CallAfter([this, ok, error]() {
                connect_running_ = false;
                if (ok) {
                    UpdateStatus("Connected");
                } else {
                    UpdateStatus("Connect failed");
                    SetMessage(error.empty() ? "Connect failed" : error);
                }
                UpdateControls();
            });
        });
}

void UsersRolesFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void UsersRolesFrame::OnNewDiagram(wxCommandEvent&) {
    if (window_manager_) {
        if (auto* host = dynamic_cast<DiagramFrame*>(window_manager_->GetDiagramHost())) {
            host->AddDiagramTab();
            host->Raise();
            host->Show(true);
            return;
        }
    }
    auto* diagram = new DiagramFrame(window_manager_, app_config_);
    diagram->Show(true);
}

void UsersRolesFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                         app_config_);
    monitor->Show(true);
}

void UsersRolesFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void UsersRolesFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void UsersRolesFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void UsersRolesFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void UsersRolesFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void UsersRolesFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void UsersRolesFrame::OnRefresh(wxCommandEvent&) {
    RefreshActiveTab();
}

void UsersRolesFrame::RefreshActiveTab() {
    if (!connection_manager_) {
        return;
    }
    if (!connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string query;
    std::string warning;
    int selection = notebook_ ? notebook_->GetSelection() : users_tab_index_;
    bool ok = false;
    if (selection == users_tab_index_) {
        ok = BuildUsersQuery(backend, &query, &warning);
    } else if (selection == roles_tab_index_) {
        ok = BuildRolesQuery(backend, &query, &warning);
    } else if (selection == memberships_tab_index_) {
        ok = BuildMembershipsQuery(backend, &query, &warning);
    }

    if (!ok) {
        UpdateStatus("Unsupported");
        SetMessage(warning);
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Running...");
    SetMessage(std::string());

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this, selection](bool ok_result, QueryResult result, const std::string& error) {
            CallAfter([this, selection, ok_result, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (selection == users_tab_index_ && users_table_) {
                    users_table_->Reset(result.columns, result.rows);
                } else if (selection == roles_tab_index_ && roles_table_) {
                    roles_table_->Reset(result.columns, result.rows);
                } else if (selection == memberships_tab_index_ && memberships_table_) {
                    memberships_table_->Reset(result.columns, result.rows);
                }
                if (ok_result) {
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    SetMessage(error.empty() ? "Query failed" : error);
                }
                UpdateControls();
            });
        });
}

void UsersRolesFrame::OnConnectionChanged(wxCommandEvent&) {
    UpdateControls();
}

void UsersRolesFrame::OnTabChanged(wxBookCtrlEvent&) {
    UpdateControls();
}

std::string UsersRolesFrame::SelectedGridValue(wxGrid* grid) const {
    return SelectedGridValueAt(grid, 0);
}

std::string UsersRolesFrame::SelectedGridValueAt(wxGrid* grid, int column) const {
    if (!grid) {
        return {};
    }
    wxArrayInt selected = grid->GetSelectedRows();
    int row = selected.IsEmpty() ? grid->GetGridCursorRow() : selected[0];
    if (row < 0 || column < 0) {
        return {};
    }
    if (column >= grid->GetNumberCols()) {
        return {};
    }
    wxString value = grid->GetCellValue(row, column);
    return value.ToStdString();
}

void UsersRolesFrame::OpenSqlTemplate(const std::string& sql) {
    if (!window_manager_ || !connection_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_, nullptr, nullptr);
    editor->LoadStatement(sql);
    editor->Show(true);
}

void UsersRolesFrame::OnCreateUser(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    UserEditorDialog dialog(this, UserEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnAlterUser(wxCommandEvent&) {
    std::string name = SelectedGridValue(users_grid_);
    if (name.empty()) {
        SetMessage("Select a user to alter.");
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    UserEditorDialog dialog(this, UserEditorMode::Alter);
    dialog.SetUserName(name);
    // TODO: Populate other fields from user details query
    
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnDropUser(wxCommandEvent&) {
    std::string name = SelectedGridValue(users_grid_);
    if (name.empty()) {
        SetMessage("Select a user to drop.");
        return;
    }
    
    wxString msg = wxString::Format("Are you sure you want to drop user '%s'?", name);
    int result = wxMessageBox(msg, "Confirm Drop User", wxYES_NO | wxICON_QUESTION, this);
    if (result != wxYES) {
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string sql = BuildDropUserTemplate(backend, name);
    OpenSqlTemplate(sql);
}

void UsersRolesFrame::OnCreateRole(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    RoleEditorDialog dialog(this, RoleEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnAlterRole(wxCommandEvent&) {
    std::string name = SelectedGridValue(roles_grid_);
    if (name.empty()) {
        SetMessage("Select a role to alter.");
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    RoleEditorDialog dialog(this, RoleEditorMode::Alter);
    dialog.SetRoleName(name);
    // TODO: Populate other fields from role details query
    
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnDropRole(wxCommandEvent&) {
    std::string name = SelectedGridValue(roles_grid_);
    if (name.empty()) {
        SetMessage("Select a role to drop.");
        return;
    }
    
    wxString msg = wxString::Format("Are you sure you want to drop role '%s'?", name);
    int result = wxMessageBox(msg, "Confirm Drop Role", wxYES_NO | wxICON_QUESTION, this);
    if (result != wxYES) {
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string sql = BuildDropRoleTemplate(backend, name);
    OpenSqlTemplate(sql);
}

void UsersRolesFrame::OnGrantRole(wxCommandEvent&) {
    std::string role = SelectedGridValue(roles_grid_);
    if (role.empty()) {
        SetMessage("Select a role to grant.");
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    wxTextEntryDialog dialog(this, "Enter username to grant role to:", "Grant Role", "");
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string user = dialog.GetValue().ToStdString();
    if (user.empty()) {
        SetMessage("Username is required.");
        return;
    }
    
    std::string sql = "GRANT " + role + " TO " + user + ";";
    OpenSqlTemplate(sql);
}

void UsersRolesFrame::OnRevokeRole(wxCommandEvent&) {
    std::string role = SelectedGridValue(roles_grid_);
    if (role.empty()) {
        SetMessage("Select a role to revoke.");
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    
    wxTextEntryDialog dialog(this, "Enter username to revoke role from:", "Revoke Role", "");
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string user = dialog.GetValue().ToStdString();
    if (user.empty()) {
        SetMessage("Username is required.");
        return;
    }
    
    std::string sql = "REVOKE " + role + " FROM " + user + ";";
    OpenSqlTemplate(sql);
}

void UsersRolesFrame::OnGrantPrivileges(wxCommandEvent&) {
    PrivilegeEditorDialog dialog(this, PrivilegeOperation::Grant);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnRevokePrivileges(wxCommandEvent&) {
    PrivilegeEditorDialog dialog(this, PrivilegeOperation::Revoke);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string sql = dialog.BuildSql(backend);
    if (!sql.empty()) {
        OpenSqlTemplate(sql);
    }
}

void UsersRolesFrame::OnGrantMembership(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string role = SelectedGridValueAt(memberships_grid_, 0);
    std::string member = SelectedGridValueAt(memberships_grid_, 1);
    OpenSqlTemplate(BuildGrantMembershipTemplate(backend, role, member));
}

void UsersRolesFrame::OnRevokeMembership(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    std::string backend = profile ? NormalizeBackendName(profile->backend) : "native";
    std::string role = SelectedGridValueAt(memberships_grid_, 0);
    std::string member = SelectedGridValueAt(memberships_grid_, 1);
    OpenSqlTemplate(BuildRevokeMembershipTemplate(backend, role, member));
}

void UsersRolesFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
