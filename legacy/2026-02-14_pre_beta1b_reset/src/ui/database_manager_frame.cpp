/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "database_manager_frame.h"

#include "database_editor_dialog.h"
#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/textdlg.h>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 1200;
constexpr int kMenuDisconnect = wxID_HIGHEST + 1201;
constexpr int kMenuRefresh = wxID_HIGHEST + 1202;
constexpr int kMenuCreate = wxID_HIGHEST + 1203;
constexpr int kMenuDrop = wxID_HIGHEST + 1204;
constexpr int kMenuClone = wxID_HIGHEST + 1205;
constexpr int kMenuProperties = wxID_HIGHEST + 1206;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 1207;

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

std::string EscapeSqlLiteral(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '\'') {
            out.push_back('\'');
        }
        out.push_back(ch);
    }
    return out;
}

bool IsSimpleIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    if (!(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_')) {
        return false;
    }
    for (char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value) || IsQuotedIdentifier(value)) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out.push_back('"');
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
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

} // namespace

wxBEGIN_EVENT_TABLE(DatabaseManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, DatabaseManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, DatabaseManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, DatabaseManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, DatabaseManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, DatabaseManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, DatabaseManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, DatabaseManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, DatabaseManagerFrame::OnOpenIndexDesigner)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, DatabaseManagerFrame::OnOpenDomainManager)
    EVT_BUTTON(kMenuConnect, DatabaseManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, DatabaseManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, DatabaseManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, DatabaseManagerFrame::OnCreate)
    EVT_BUTTON(kMenuDrop, DatabaseManagerFrame::OnDrop)
    EVT_BUTTON(kMenuClone, DatabaseManagerFrame::OnClone)
    EVT_BUTTON(kMenuProperties, DatabaseManagerFrame::OnProperties)
    EVT_CLOSE(DatabaseManagerFrame::OnClose)
wxEND_EVENT_TABLE()

DatabaseManagerFrame::DatabaseManagerFrame(WindowManager* windowManager,
                                           ConnectionManager* connectionManager,
                                           const std::vector<ConnectionProfile>* connections,
                                           const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Databases", wxDefaultPosition, wxSize(1100, 720)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    BuildMenu();
    BuildLayout();
    PopulateConnections();
    UpdateControls();

    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void DatabaseManagerFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void DatabaseManagerFrame::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* topPanel = new wxPanel(this, wxID_ANY);
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(new wxStaticText(topPanel, wxID_ANY, "Connection:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connection_choice_ = new wxChoice(topPanel, kConnectionChoiceId);
    topSizer->Add(connection_choice_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connect_button_ = new wxButton(topPanel, kMenuConnect, "Connect");
    topSizer->Add(connect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    disconnect_button_ = new wxButton(topPanel, kMenuDisconnect, "Disconnect");
    topSizer->Add(disconnect_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    refresh_button_ = new wxButton(topPanel, kMenuRefresh, "Refresh");
    topSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL);
    topPanel->SetSizer(topSizer);
    rootSizer->Add(topPanel, 0, wxEXPAND | wxALL, 8);

    auto* actionPanel = new wxPanel(this, wxID_ANY);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    create_button_ = new wxButton(actionPanel, kMenuCreate, "Create");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    clone_button_ = new wxButton(actionPanel, kMenuClone, "Clone");
    properties_button_ = new wxButton(actionPanel, kMenuProperties, "Properties");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionSizer->Add(clone_button_, 0, wxRIGHT, 6);
    actionSizer->Add(properties_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Databases"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    databases_grid_ = new wxGrid(listPanel, wxID_ANY);
    databases_grid_->EnableEditing(false);
    databases_grid_->SetRowLabelSize(40);
    databases_table_ = new ResultGridTable();
    databases_grid_->SetTable(databases_table_, true);
    listSizer->Add(databases_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    detailSizer->Add(new wxStaticText(detailPanel, wxID_ANY, "Details"), 0,
                     wxLEFT | wxRIGHT | wxTOP, 8);
    
    activity_label_ = new wxStaticText(detailPanel, wxID_ANY, "Select a database to view details");
    activity_label_->SetForegroundColour(wxColour(100, 100, 100));
    detailSizer->Add(activity_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    details_text_ = new wxTextCtrl(detailPanel, wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    detailSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    detailPanel->SetSizer(detailSizer);

    splitter->SplitVertically(listPanel, detailPanel, 500);
    rootSizer->Add(splitter, 1, wxEXPAND);

    auto* statusPanel = new wxPanel(this, wxID_ANY);
    auto* statusSizer = new wxBoxSizer(wxVERTICAL);
    status_text_ = new wxStaticText(statusPanel, wxID_ANY, "Ready");
    statusSizer->Add(status_text_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    message_text_ = new wxTextCtrl(statusPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    message_text_->SetMinSize(wxSize(-1, 70));
    statusSizer->Add(message_text_, 0, wxEXPAND | wxALL, 8);
    statusPanel->SetSizer(statusSizer);
    rootSizer->Add(statusPanel, 0, wxEXPAND);

    SetSizer(rootSizer);

    databases_grid_->Bind(wxEVT_GRID_SELECT_CELL, &DatabaseManagerFrame::OnDatabaseSelected, this);
}

void DatabaseManagerFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    active_profile_index_ = -1;
    if (!connections_ || connections_->empty()) {
        connection_choice_->Append("No connections configured");
        connection_choice_->SetSelection(0);
        connection_choice_->Enable(false);
        return;
    }
    connection_choice_->Enable(true);
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    connection_choice_->SetSelection(0);
}

const ConnectionProfile* DatabaseManagerFrame::GetSelectedProfile() const {
    if (!connections_ || connections_->empty() || !connection_choice_) {
        return nullptr;
    }
    int selection = connection_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return nullptr;
    }
    if (selection < 0 || static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(selection)];
}

bool DatabaseManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    int selection = connection_choice_ ? connection_choice_->GetSelection() : -1;
    bool profile_changed = selection != active_profile_index_;

    if (!connection_manager_->IsConnected() || profile_changed) {
        connection_manager_->Disconnect();
        if (!connection_manager_->Connect(profile)) {
            active_profile_index_ = -1;
            return false;
        }
        active_profile_index_ = selection;
    }
    return true;
}

bool DatabaseManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    // Native/ScratchBird backends include: network, scratchbird, embedded, ipc
    std::string backend = NormalizeBackendName(profile.backend);
    if (backend == "native") {
        return true;
    }
    // Also check connection mode - embedded and ipc are native modes
    if (profile.mode == ConnectionMode::Embedded || profile.mode == ConnectionMode::Ipc) {
        return true;
    }
    return false;
}

void DatabaseManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_db = !selected_database_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_db && !busy);
    if (clone_button_) clone_button_->Enable(connected && native && has_db && !busy);
    if (properties_button_) properties_button_->Enable(connected && native && has_db && !busy);
}

void DatabaseManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void DatabaseManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void DatabaseManagerFrame::RefreshDatabases() {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    if (!IsNativeProfile(*profile)) {
        SetMessage("Database management is available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading databases...");
    
    std::string sql = 
        "SELECT d.datname as database_name, "
        "       pg_get_userbyid(d.datdba) as owner, "
        "       pg_encoding_to_char(d.encoding) as encoding, "
        "       d.datcollate as collation, "
        "       d.datctype as character_class, "
        "       d.datcreated::timestamp::text as created, "
        "       pg_size_pretty(pg_database_size(d.datname)) as size, "
        "       CASE WHEN d.datallowconn THEN 'Online' ELSE 'No Connections' END as status, "
        "       (SELECT count(*) FROM pg_stat_activity WHERE datname = d.datname) as connection_count "
        "FROM pg_database d "
        "WHERE d.datistemplate = false "
        "ORDER BY d.datname";
    
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                databases_result_ = result;
                if (databases_table_) {
                    databases_table_->Reset(databases_result_.columns, databases_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load databases." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Databases updated");
                }
                UpdateControls();
            });
        });
}

void DatabaseManagerFrame::RefreshDatabaseDetails(const std::string& db_name) {
    if (!connection_manager_ || db_name.empty()) {
        return;
    }
    
    std::string sql = 
        "SELECT d.datname as database_name, "
        "       pg_get_userbyid(d.datdba) as owner, "
        "       pg_encoding_to_char(d.encoding) as encoding, "
        "       d.datcollate as collation, "
        "       d.datctype as character_class, "
        "       d.datistemplate as is_template, "
        "       d.datallowconn as allow_connections, "
        "       d.datconnlimit as connection_limit, "
        "       d.datcreated::timestamp::text as created, "
        "       pg_size_pretty(pg_database_size(d.datname)) as size, "
        "       t.spcname as tablespace "
        "FROM pg_database d "
        "LEFT JOIN pg_tablespace t ON d.dattablespace = t.oid "
        "WHERE d.datname = '" + EscapeSqlLiteral(db_name) + "'";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, db_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, db_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                database_details_result_ = result;
                if (ok) {
                    if (details_text_) {
                        details_text_->SetValue(FormatDetails(database_details_result_));
                    }
                    FetchDatabaseActivity(db_name);
                } else if (!error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void DatabaseManagerFrame::FetchDatabaseActivity(const std::string& db_name) {
    if (!connection_manager_ || db_name.empty()) {
        return;
    }
    
    std::string sql = 
        "SELECT count(*) as active_connections, "
        "       count(*) FILTER (WHERE state = 'active') as active_queries, "
        "       count(*) FILTER (WHERE state = 'idle') as idle_connections, "
        "       max(now() - backend_start) as oldest_connection, "
        "       max(now() - xact_start) FILTER (WHERE xact_start IS NOT NULL) as longest_transaction "
        "FROM pg_stat_activity "
        "WHERE datname = '" + EscapeSqlLiteral(db_name) + "'";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, db_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, db_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                database_activity_result_ = result;
                if (ok && activity_label_) {
                    activity_label_->SetLabel(FormatActivity(result));
                    activity_label_->SetForegroundColour(wxColour(80, 80, 80));
                } else if (!ok && !error.empty()) {
                    activity_label_->SetLabel("Activity info unavailable");
                    activity_label_->SetForegroundColour(wxColour(150, 150, 150));
                }
                UpdateControls();
            });
        });
}

void DatabaseManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
    if (!connection_manager_) {
        return;
    }
    pending_queries_++;
    UpdateControls();
    UpdateStatus("Running...");
    connection_manager_->ExecuteQueryAsync(
        sql, [this, success_message](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, success_message]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok) {
                    UpdateStatus(success_message);
                    SetMessage("");
                } else {
                    UpdateStatus("Command failed");
                    SetMessage(error.empty() ? "Command failed." : error);
                }
                UpdateControls();
                RefreshDatabases();
                if (!selected_database_.empty()) {
                    RefreshDatabaseDetails(selected_database_);
                }
            });
        });
}

std::string DatabaseManagerFrame::GetSelectedDatabaseName() const {
    if (!databases_grid_ || databases_result_.rows.empty()) {
        return std::string();
    }
    int row = databases_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= databases_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(databases_result_, row, {"database_name", "datname", "name"});
    if (!value.empty()) {
        return value;
    }
    if (!databases_result_.rows[row].empty()) {
        return databases_result_.rows[row][0].text;
    }
    return std::string();
}

int DatabaseManagerFrame::FindColumnIndex(const QueryResult& result,
                                          const std::vector<std::string>& names) const {
    for (size_t i = 0; i < result.columns.size(); ++i) {
        std::string column = ToLowerCopy(result.columns[i].name);
        for (const auto& name : names) {
            if (column == name) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

std::string DatabaseManagerFrame::ExtractValue(const QueryResult& result,
                                               int row,
                                               const std::vector<std::string>& names) const {
    int index = FindColumnIndex(result, names);
    if (index < 0 || row < 0 || static_cast<size_t>(row) >= result.rows.size()) {
        return std::string();
    }
    if (static_cast<size_t>(index) >= result.rows[row].size()) {
        return std::string();
    }
    return result.rows[row][static_cast<size_t>(index)].text;
}

std::string DatabaseManagerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No database details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

std::string DatabaseManagerFrame::FormatActivity(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No activity data available";
    }
    const auto& row = result.rows[0];
    std::string connections = row.size() > 0 && !row[0].isNull ? row[0].text : "0";
    std::string active = row.size() > 1 && !row[1].isNull ? row[1].text : "0";
    std::string idle = row.size() > 2 && !row[2].isNull ? row[2].text : "0";
    
    return "Active connections: " + connections + " (Active queries: " + active + ", Idle: " + idle + ")";
}

void DatabaseManagerFrame::OnConnect(wxCommandEvent&) {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    UpdateStatus("Connected");
    UpdateControls();
    RefreshDatabases();
}

void DatabaseManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void DatabaseManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshDatabases();
}

void DatabaseManagerFrame::OnDatabaseSelected(wxGridEvent& event) {
    selected_database_ = GetSelectedDatabaseName();
    if (!selected_database_.empty()) {
        RefreshDatabaseDetails(selected_database_);
    }
    UpdateControls();
    event.Skip();
}

void DatabaseManagerFrame::OnCreate(wxCommandEvent&) {
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create database statement is empty.");
        return;
    }
    RunCommand(sql, "Database created");
}

void DatabaseManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_database_.empty()) {
        return;
    }
    
    // Strong confirmation dialog
    wxTextEntryDialog confirm_dialog(this, 
        "WARNING: Dropping a database is irreversible!\n\n"
        "Type the database name '" + selected_database_ + "' to confirm:",
        "Confirm Database Drop", "", wxOK | wxCANCEL);
    
    if (confirm_dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string typed_name = Trim(confirm_dialog.GetValue().ToStdString());
    if (typed_name != selected_database_) {
        SetMessage("Database name does not match. Drop cancelled.");
        return;
    }
    
    std::string sql = "DROP DATABASE " + QuoteIdentifier(selected_database_) + ";";
    RunCommand(sql, "Database dropped");
}

void DatabaseManagerFrame::OnClone(wxCommandEvent&) {
    if (selected_database_.empty()) {
        return;
    }
    
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Clone);
    dialog.SetSourceDatabase(selected_database_);
    
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Clone database statement is empty.");
        return;
    }
    
    RunCommand(sql, "Database cloned");
}

void DatabaseManagerFrame::OnProperties(wxCommandEvent&) {
    if (selected_database_.empty()) {
        return;
    }
    
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Properties);
    dialog.SetSourceDatabase(selected_database_);
    
    // Load and set current properties
    if (!database_details_result_.rows.empty()) {
        dialog.LoadProperties(database_details_result_);
    }
    
    dialog.ShowModal();
}

void DatabaseManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void DatabaseManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void DatabaseManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                         app_config_);
    monitor->Show(true);
}

void DatabaseManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void DatabaseManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void DatabaseManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void DatabaseManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void DatabaseManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void DatabaseManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void DatabaseManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
