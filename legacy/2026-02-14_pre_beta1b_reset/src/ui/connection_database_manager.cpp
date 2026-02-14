/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "connection_database_manager.h"

#include "connection_editor_dialog.h"
#include "database_editor_dialog.h"
#include "menu_builder.h"
#include "result_grid_table.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/grid.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/textdlg.h>

namespace scratchrobin {

namespace {

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

// Control IDs
constexpr int kIdConnectionList = wxID_HIGHEST + 300;
constexpr int kIdNewConnection = wxID_HIGHEST + 301;
constexpr int kIdEditConnection = wxID_HIGHEST + 302;
constexpr int kIdDuplicateConnection = wxID_HIGHEST + 303;
constexpr int kIdDeleteConnection = wxID_HIGHEST + 304;
constexpr int kIdConnect = wxID_HIGHEST + 305;
constexpr int kIdDisconnect = wxID_HIGHEST + 306;
constexpr int kIdRefreshDatabases = wxID_HIGHEST + 307;
constexpr int kIdCreateDatabase = wxID_HIGHEST + 308;
constexpr int kIdDropDatabase = wxID_HIGHEST + 309;
constexpr int kIdCloneDatabase = wxID_HIGHEST + 310;
constexpr int kIdDatabaseProperties = wxID_HIGHEST + 311;

} // namespace

wxBEGIN_EVENT_TABLE(ConnectionDatabaseManager, wxFrame)
    EVT_BUTTON(kIdNewConnection, ConnectionDatabaseManager::OnNewConnection)
    EVT_BUTTON(kIdEditConnection, ConnectionDatabaseManager::OnEditConnection)
    EVT_BUTTON(kIdDuplicateConnection, ConnectionDatabaseManager::OnDuplicateConnection)
    EVT_BUTTON(kIdDeleteConnection, ConnectionDatabaseManager::OnDeleteConnection)
    EVT_LISTBOX(kIdConnectionList, ConnectionDatabaseManager::OnConnectionSelected)
    EVT_LISTBOX_DCLICK(kIdConnectionList, ConnectionDatabaseManager::OnConnectionActivated)
    EVT_BUTTON(kIdConnect, ConnectionDatabaseManager::OnConnect)
    EVT_BUTTON(kIdDisconnect, ConnectionDatabaseManager::OnDisconnect)
    EVT_BUTTON(kIdRefreshDatabases, ConnectionDatabaseManager::OnRefreshDatabases)
    EVT_BUTTON(kIdCreateDatabase, ConnectionDatabaseManager::OnCreateDatabase)
    EVT_BUTTON(kIdDropDatabase, ConnectionDatabaseManager::OnDropDatabase)
    EVT_BUTTON(kIdCloneDatabase, ConnectionDatabaseManager::OnCloneDatabase)
    EVT_BUTTON(kIdDatabaseProperties, ConnectionDatabaseManager::OnDatabaseProperties)
    EVT_GRID_SELECT_CELL(ConnectionDatabaseManager::OnDatabaseSelected)
    EVT_CLOSE(ConnectionDatabaseManager::OnClose)
wxEND_EVENT_TABLE()

ConnectionDatabaseManager::ConnectionDatabaseManager(
    wxWindow* parent,
    WindowManager* windowManager,
    ConnectionManager* connectionManager,
    std::vector<ConnectionProfile>* connections,
    const AppConfig* appConfig)
    : wxFrame(parent, wxID_ANY, "Connection & Database Manager",
              wxDefaultPosition, wxSize(1200, 700)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig) {
    
    BuildMenu();
    BuildLayout();
    RefreshConnectionList();
    UpdateConnectionButtonStates();
    UpdateDatabaseButtonStates();
    
    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void ConnectionDatabaseManager::BuildMenu() {
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void ConnectionDatabaseManager::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // Main splitter: connections on left, databases on right
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetMinimumPaneSize(300);
    
    // === LEFT PANEL: Connections ===
    auto* leftPanel = new wxPanel(splitter, wxID_ANY);
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    // Connection list
    leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "Connections"), 0, wxALL, 8);
    connection_list_ = new wxListBox(leftPanel, kIdConnectionList);
    leftSizer->Add(connection_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Connection buttons
    auto* connButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    conn_new_button_ = new wxButton(leftPanel, kIdNewConnection, "&New...");
    conn_edit_button_ = new wxButton(leftPanel, kIdEditConnection, "&Edit...");
    conn_duplicate_button_ = new wxButton(leftPanel, kIdDuplicateConnection, "&Duplicate");
    conn_delete_button_ = new wxButton(leftPanel, kIdDeleteConnection, "&Delete");
    connButtonSizer->Add(conn_new_button_, 0, wxRIGHT, 4);
    connButtonSizer->Add(conn_edit_button_, 0, wxRIGHT, 4);
    connButtonSizer->Add(conn_duplicate_button_, 0, wxRIGHT, 4);
    connButtonSizer->Add(conn_delete_button_, 0, wxRIGHT, 4);
    leftSizer->Add(connButtonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Connection status and connect buttons
    auto* connActionSizer = new wxBoxSizer(wxHORIZONTAL);
    conn_status_label_ = new wxStaticText(leftPanel, wxID_ANY, "Not connected");
    conn_status_label_->SetForegroundColour(wxColour(128, 128, 128));
    connActionSizer->Add(conn_status_label_, 1, wxALIGN_CENTER_VERTICAL);
    conn_connect_button_ = new wxButton(leftPanel, kIdConnect, "&Connect");
    conn_disconnect_button_ = new wxButton(leftPanel, kIdDisconnect, "&Disconnect");
    conn_disconnect_button_->Enable(false);
    connActionSizer->Add(conn_connect_button_, 0, wxRIGHT, 4);
    connActionSizer->Add(conn_disconnect_button_, 0);
    leftSizer->Add(connActionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    leftPanel->SetSizer(leftSizer);
    
    // === RIGHT PANEL: Databases ===
    auto* rightPanel = new wxPanel(splitter, wxID_ANY);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    // Database toolbar
    auto* dbToolSizer = new wxBoxSizer(wxHORIZONTAL);
    dbToolSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "Databases"), 1, wxALIGN_CENTER_VERTICAL);
    db_refresh_button_ = new wxButton(rightPanel, kIdRefreshDatabases, "&Refresh");
    db_create_button_ = new wxButton(rightPanel, kIdCreateDatabase, "&Create...");
    db_drop_button_ = new wxButton(rightPanel, kIdDropDatabase, "&Drop...");
    db_clone_button_ = new wxButton(rightPanel, kIdCloneDatabase, "C&lone...");
    db_properties_button_ = new wxButton(rightPanel, kIdDatabaseProperties, "&Properties...");
    dbToolSizer->Add(db_refresh_button_, 0, wxRIGHT, 4);
    dbToolSizer->Add(db_create_button_, 0, wxRIGHT, 4);
    dbToolSizer->Add(db_drop_button_, 0, wxRIGHT, 4);
    dbToolSizer->Add(db_clone_button_, 0, wxRIGHT, 4);
    dbToolSizer->Add(db_properties_button_, 0);
    rightSizer->Add(dbToolSizer, 0, wxEXPAND | wxALL, 8);
    
    // Database grid
    databases_grid_ = new wxGrid(rightPanel, wxID_ANY);
    databases_grid_->EnableEditing(false);
    databases_grid_->SetRowLabelSize(40);
    databases_table_ = new ResultGridTable();
    databases_grid_->SetTable(databases_table_, true);
    rightSizer->Add(databases_grid_, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Details panel
    rightSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "Details"), 0, wxLEFT | wxRIGHT, 8);
    details_text_ = new wxTextCtrl(rightPanel, wxID_ANY, "",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    rightSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    
    // Status bar
    auto* statusSizer = new wxBoxSizer(wxVERTICAL);
    status_text_ = new wxStaticText(rightPanel, wxID_ANY, "Ready");
    statusSizer->Add(status_text_, 0, wxLEFT | wxRIGHT | wxTOP, 4);
    message_text_ = new wxTextCtrl(rightPanel, wxID_ANY, "",
                                   wxDefaultPosition, wxSize(-1, 60),
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    statusSizer->Add(message_text_, 0, wxEXPAND | wxALL, 4);
    rightSizer->Add(statusSizer, 0, wxEXPAND);
    
    rightPanel->SetSizer(rightSizer);
    
    // Split the panels
    splitter->SplitVertically(leftPanel, rightPanel, 350);
    rootSizer->Add(splitter, 1, wxEXPAND);
    
    SetSizer(rootSizer);
    
    // Initial state
    UpdateDatabaseButtonStates();
}

// ============================================================================
// Connection Management
// ============================================================================

void ConnectionDatabaseManager::RefreshConnectionList() {
    if (!connection_list_ || !connections_) return;
    
    connection_list_->Clear();
    for (const auto& conn : *connections_) {
        wxString label = conn.name.empty() ? "(Unnamed)" : wxString::FromUTF8(conn.name);
        
        // Add mode indicator
        std::string mode_str = "Network";
        if (conn.mode == ConnectionMode::Embedded) mode_str = "Embedded";
        else if (conn.mode == ConnectionMode::Ipc) mode_str = "IPC";
        
        label += wxString::Format(" [%s", mode_str);
        
        // Add backend indicator
        std::string backend = conn.backend;
        if (backend.empty()) backend = "native";
        if (backend != mode_str) {
            label += wxString::Format("/%s", backend);
        }
        label += "]";
        
        // Add host/database indicator
        if (!conn.database.empty()) {
            label += wxString::Format(" - %s", conn.database);
        } else if (!conn.host.empty()) {
            label += wxString::Format(" - %s", conn.host);
            if (conn.port > 0) {
                label += wxString::Format(":%d", conn.port);
            }
        }
        
        connection_list_->Append(label);
    }
}

void ConnectionDatabaseManager::UpdateConnectionButtonStates() {
    bool hasSelection = connection_list_ && connection_list_->GetSelection() != wxNOT_FOUND;
    int selection = hasSelection ? connection_list_->GetSelection() : -1;
    
    if (conn_edit_button_) conn_edit_button_->Enable(hasSelection);
    if (conn_duplicate_button_) conn_duplicate_button_->Enable(hasSelection);
    if (conn_delete_button_) conn_delete_button_->Enable(hasSelection);
    if (conn_connect_button_) conn_connect_button_->Enable(hasSelection);
    
    // Connect/Disconnect based on connection state
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    if (conn_connect_button_) conn_connect_button_->Enable(hasSelection && !connected);
    if (conn_disconnect_button_) conn_disconnect_button_->Enable(connected);
    
    // Update status label
    if (conn_status_label_) {
        if (connected) {
            conn_status_label_->SetLabel("Connected");
            conn_status_label_->SetForegroundColour(wxColour(0, 128, 0));
        } else {
            conn_status_label_->SetLabel("Not connected");
            conn_status_label_->SetForegroundColour(wxColour(128, 128, 128));
        }
    }
}

const ConnectionProfile* ConnectionDatabaseManager::GetSelectedConnection() const {
    if (!connections_ || !connection_list_) return nullptr;
    int selection = connection_list_->GetSelection();
    if (selection == wxNOT_FOUND || selection < 0 || 
        static_cast<size_t>(selection) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[selection];
}

bool ConnectionDatabaseManager::IsNativeProfile(const ConnectionProfile& profile) const {
    std::string backend = NormalizeBackendName(profile.backend);
    if (backend == "native") return true;
    if (profile.mode == ConnectionMode::Embedded || 
        profile.mode == ConnectionMode::Ipc) {
        return true;
    }
    return false;
}

void ConnectionDatabaseManager::OnConnectionSelected(wxCommandEvent& event) {
    UpdateConnectionButtonStates();
    // Auto-disconnect when switching connections
    if (connection_manager_ && connection_manager_->IsConnected()) {
        connection_manager_->Disconnect();
        RefreshDatabaseList();
        UpdateDatabaseButtonStates();
    }
}

void ConnectionDatabaseManager::OnConnectionActivated(wxCommandEvent& event) {
    // Double-click to connect
    OnConnect(event);
}

void ConnectionDatabaseManager::OnNewConnection(wxCommandEvent& event) {
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Create);
    if (dialog.ShowModal() == wxID_OK && dialog.ValidateForm()) {
        connections_->push_back(dialog.GetProfile());
        RefreshConnectionList();
        connection_list_->SetSelection(connection_list_->GetCount() - 1);
        UpdateConnectionButtonStates();
    }
}

void ConnectionDatabaseManager::OnEditConnection(wxCommandEvent& event) {
    int selection = connection_list_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Edit, 
                                  &(*connections_)[selection]);
    if (dialog.ShowModal() == wxID_OK && dialog.ValidateForm()) {
        (*connections_)[selection] = dialog.GetProfile();
        RefreshConnectionList();
        connection_list_->SetSelection(selection);
    }
}

void ConnectionDatabaseManager::OnDuplicateConnection(wxCommandEvent& event) {
    int selection = connection_list_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    ConnectionEditorDialog dialog(this, ConnectionEditorMode::Duplicate,
                                  &(*connections_)[selection]);
    if (dialog.ShowModal() == wxID_OK && dialog.ValidateForm()) {
        connections_->push_back(dialog.GetProfile());
        RefreshConnectionList();
        connection_list_->SetSelection(connection_list_->GetCount() - 1);
        UpdateConnectionButtonStates();
    }
}

void ConnectionDatabaseManager::OnDeleteConnection(wxCommandEvent& event) {
    int selection = connection_list_->GetSelection();
    if (selection == wxNOT_FOUND || !connections_) return;
    
    wxString name = connection_list_->GetString(selection);
    wxString msg = wxString::Format("Delete connection '%s'?", name.BeforeFirst('[').Trim());
    
    if (wxMessageBox(msg, "Confirm Delete", wxYES_NO | wxICON_QUESTION, this) == wxYES) {
        connections_->erase(connections_->begin() + selection);
        RefreshConnectionList();
        UpdateConnectionButtonStates();
        
        // Disconnect if we deleted the active connection
        if (connection_manager_) {
            connection_manager_->Disconnect();
        }
        RefreshDatabaseList();
        UpdateDatabaseButtonStates();
    }
}

// ============================================================================
// Database Operations
// ============================================================================

void ConnectionDatabaseManager::OnConnect(wxCommandEvent& event) {
    const auto* profile = GetSelectedConnection();
    if (!profile || !connection_manager_) return;
    
    UpdateStatus("Connecting...");
    
    if (!connection_manager_->Connect(*profile)) {
        SetMessage(connection_manager_->LastError());
        UpdateStatus("Connection failed");
    } else {
        SetMessage("");
        UpdateStatus("Connected");
        RefreshDatabaseList();
    }
    
    UpdateConnectionButtonStates();
    UpdateDatabaseButtonStates();
}

void ConnectionDatabaseManager::OnDisconnect(wxCommandEvent& event) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
    }
    RefreshDatabaseList();
    UpdateConnectionButtonStates();
    UpdateDatabaseButtonStates();
    UpdateStatus("Disconnected");
}

void ConnectionDatabaseManager::UpdateDatabaseButtonStates() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedConnection();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_db = !selected_database_.empty();
    
    if (db_refresh_button_) db_refresh_button_->Enable(connected && !busy);
    if (db_create_button_) db_create_button_->Enable(connected && native && !busy);
    if (db_drop_button_) db_drop_button_->Enable(connected && native && has_db && !busy);
    if (db_clone_button_) db_clone_button_->Enable(connected && native && has_db && !busy);
    if (db_properties_button_) db_properties_button_->Enable(connected && native && has_db && !busy);
}

void ConnectionDatabaseManager::RefreshDatabaseList() {
    if (!databases_grid_ || !connection_manager_) return;
    
    databases_table_->Clear();
    databases_grid_->ForceRefresh();
    selected_database_.clear();
    
    if (!connection_manager_->IsConnected()) {
        // Not connected - grid is empty
        databases_grid_->ForceRefresh();
        return;
    }
    
    // Query databases - different SQL for different backends
    const auto* profile = GetSelectedConnection();
    std::string sql;
    if (profile && NormalizeBackendName(profile->backend) == "postgresql") {
        sql = "SELECT datname as name, pg_size_pretty(pg_database_size(datname)) as size, "
              "pg_encoding_to_char(encoding) as encoding FROM pg_database WHERE datistemplate = false;";
    } else {
        // ScratchBird native query
        sql = "SELECT name, path, size FROM system.databases;";
    }
    
    QueryResult result;
    if (!connection_manager_->ExecuteQuery(sql, &result)) {
        // Error - show in message area
        SetMessage(connection_manager_->LastError());
    } else {
        databases_result_ = result;
        databases_table_->Reset(result.columns, result.rows);
        
        // Resize columns
        if (!result.columns.empty()) {
            for (size_t i = 0; i < result.columns.size() && i < 4; ++i) {
                databases_grid_->SetColSize(static_cast<int>(i), 150);
            }
        }
    }
    
    databases_grid_->ForceRefresh();
}

void ConnectionDatabaseManager::OnRefreshDatabases(wxCommandEvent& event) {
    RefreshDatabaseList();
    UpdateDatabaseButtonStates();
}

void ConnectionDatabaseManager::OnDatabaseSelected(wxGridEvent& event) {
    selected_database_ = GetSelectedDatabaseName();
    
    // Load details
    if (!selected_database_.empty() && connection_manager_ && details_text_) {
        // Simple details query
        std::string sql = "SELECT * FROM system.databases WHERE name = '" + selected_database_ + "';";
        QueryResult result;
        if (connection_manager_->ExecuteQuery(sql, &result) && !result.rows.empty()) {
            std::ostringstream details;
            for (size_t i = 0; i < result.columns.size() && i < result.rows[0].size(); ++i) {
                details << result.columns[i].name << ": " 
                        << (result.rows[0][i].isNull ? "NULL" : result.rows[0][i].text) << "\n";
            }
            details_text_->SetValue(details.str());
        }
    }
    
    UpdateDatabaseButtonStates();
    event.Skip();
}

std::string ConnectionDatabaseManager::GetSelectedDatabaseName() const {
    if (!databases_grid_ || databases_result_.rows.empty()) return "";
    
    int row = databases_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= databases_result_.rows.size()) return "";
    
    // Try common column names
    std::vector<std::string> name_cols = {"name", "datname", "database_name"};
    for (const auto& col : name_cols) {
        int idx = FindColumnIndex(databases_result_, {col});
        if (idx >= 0 && static_cast<size_t>(idx) < databases_result_.rows[row].size()) {
            return databases_result_.rows[row][idx].text;
        }
    }
    
    // Fallback to first column
    if (!databases_result_.rows[row].empty()) {
        return databases_result_.rows[row][0].text;
    }
    return "";
}

int ConnectionDatabaseManager::FindColumnIndex(const QueryResult& result,
                                                const std::vector<std::string>& names) const {
    for (size_t i = 0; i < result.columns.size(); ++i) {
        std::string col = ToLowerCopy(result.columns[i].name);
        for (const auto& name : names) {
            if (col == ToLowerCopy(name)) return static_cast<int>(i);
        }
    }
    return -1;
}

void ConnectionDatabaseManager::OnCreateDatabase(wxCommandEvent& event) {
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) return;
    
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create database statement is empty.");
        return;
    }
    RunCommand(sql, "Database created");
}

void ConnectionDatabaseManager::OnDropDatabase(wxCommandEvent& event) {
    if (selected_database_.empty()) return;
    
    wxTextEntryDialog confirm(this,
        wxString::Format("WARNING: Dropping database '%s' is irreversible!\n\n"
                        "Type the database name to confirm:", selected_database_),
        "Confirm Database Drop", "", wxOK | wxCANCEL);
    
    if (confirm.ShowModal() != wxID_OK) return;
    
    if (confirm.GetValue().ToStdString() != selected_database_) {
        SetMessage("Database name does not match. Drop cancelled.");
        return;
    }
    
    std::string sql = "DROP DATABASE \"" + selected_database_ + "\";";
    RunCommand(sql, "Database dropped");
}

void ConnectionDatabaseManager::OnCloneDatabase(wxCommandEvent& event) {
    if (selected_database_.empty()) return;
    
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Clone);
    dialog.SetSourceDatabase(selected_database_);
    
    if (dialog.ShowModal() != wxID_OK) return;
    
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Clone database statement is empty.");
        return;
    }
    RunCommand(sql, "Database cloned");
}

void ConnectionDatabaseManager::OnDatabaseProperties(wxCommandEvent& event) {
    if (selected_database_.empty()) return;
    
    // Query properties
    std::string sql = "SELECT * FROM system.databases WHERE name = '" + selected_database_ + "';";
    QueryResult result;
    if (!connection_manager_->ExecuteQuery(sql, &result)) {
        SetMessage(connection_manager_->LastError());
        return;
    }
    
    DatabaseEditorDialog dialog(this, DatabaseEditorMode::Properties);
    dialog.LoadProperties(result);
    dialog.ShowModal();
}

void ConnectionDatabaseManager::RunCommand(const std::string& sql, 
                                           const std::string& success_message) {
    if (!connection_manager_) return;
    
    pending_queries_++;
    UpdateDatabaseButtonStates();
    UpdateStatus("Running...");
    
    connection_manager_->ExecuteQueryAsync(
        sql, [this, success_message](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result, error, success_message]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok) {
                    UpdateStatus(success_message);
                    SetMessage("");
                } else {
                    UpdateStatus("Command failed");
                    SetMessage(error.empty() ? "Command failed." : error);
                }
                UpdateDatabaseButtonStates();
                RefreshDatabaseList();
            });
        });
}

void ConnectionDatabaseManager::UpdateStatus(const wxString& status) {
    if (status_text_) status_text_->SetLabel(status);
}

void ConnectionDatabaseManager::SetMessage(const std::string& message) {
    if (message_text_) message_text_->SetValue(message);
}

void ConnectionDatabaseManager::OnClose(wxCloseEvent& event) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    event.Skip();
}

} // namespace scratchrobin
