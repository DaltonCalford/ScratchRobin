/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "storage_manager_frame.h"

#include "diagram_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "tablespace_editor_dialog.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 130;
constexpr int kMenuDisconnect = wxID_HIGHEST + 131;
constexpr int kMenuRefresh = wxID_HIGHEST + 132;
constexpr int kMenuCreate = wxID_HIGHEST + 133;
constexpr int kMenuEdit = wxID_HIGHEST + 134;
constexpr int kMenuDrop = wxID_HIGHEST + 135;
constexpr int kMenuRelocate = wxID_HIGHEST + 136;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 137;

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

wxBEGIN_EVENT_TABLE(StorageManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, StorageManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, StorageManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, StorageManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, StorageManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, StorageManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, StorageManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, StorageManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, StorageManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, StorageManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, StorageManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, StorageManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, StorageManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, StorageManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, StorageManagerFrame::OnDrop)
    EVT_BUTTON(kMenuRelocate, StorageManagerFrame::OnRelocate)
    EVT_CLOSE(StorageManagerFrame::OnClose)
wxEND_EVENT_TABLE()

StorageManagerFrame::StorageManagerFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Storage Manager", wxDefaultPosition, wxSize(1100, 750)),
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

void StorageManagerFrame::BuildMenu() {
    WindowChromeConfig chrome;
    if (app_config_) {
        chrome = app_config_->chrome.monitoring;
    }
    if (!chrome.showMenu) {
        return;
    }

    MenuBuildOptions options;
    options.includeConnections = chrome.replicateMenu;
    options.includeEdit = true;
    options.includeView = true;
    options.includeWindow = true;
    options.includeHelp = true;
    auto* menu_bar = BuildMenuBar(options, window_manager_, this);
    SetMenuBar(menu_bar);
}

void StorageManagerFrame::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Top panel with connection selector
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

    // Toolbar panel
    auto* toolbarPanel = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);
    create_button_ = new wxButton(toolbarPanel, kMenuCreate, "Create");
    edit_button_ = new wxButton(toolbarPanel, kMenuEdit, "Edit");
    drop_button_ = new wxButton(toolbarPanel, kMenuDrop, "Drop");
    relocate_button_ = new wxButton(toolbarPanel, kMenuRelocate, "Relocate");
    toolbarSizer->Add(create_button_, 0, wxRIGHT, 6);
    toolbarSizer->Add(edit_button_, 0, wxRIGHT, 6);
    toolbarSizer->Add(drop_button_, 0, wxRIGHT, 6);
    toolbarSizer->Add(relocate_button_, 0);
    toolbarPanel->SetSizer(toolbarSizer);
    rootSizer->Add(toolbarPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Main splitter with tablespace list and notebook
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    // Left panel - Tablespaces grid
    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Tablespaces"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    tablespaces_grid_ = new wxGrid(listPanel, wxID_ANY);
    tablespaces_grid_->EnableEditing(false);
    tablespaces_grid_->SetRowLabelSize(40);
    tablespaces_table_ = new ResultGridTable();
    tablespaces_grid_->SetTable(tablespaces_table_, true);
    listSizer->Add(tablespaces_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    // Right panel - Notebook with tabs
    auto* rightPanel = new wxPanel(splitter, wxID_ANY);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(rightPanel, wxID_ANY);
    
    // Objects tab
    auto* objectsPanel = new wxPanel(notebook_, wxID_ANY);
    auto* objectsSizer = new wxBoxSizer(wxVERTICAL);
    objects_label_ = new wxStaticText(objectsPanel, wxID_ANY, "Select a tablespace to view objects");
    objects_label_->SetForegroundColour(wxColour(100, 100, 100));
    objectsSizer->Add(objects_label_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    objects_grid_ = new wxGrid(objectsPanel, wxID_ANY);
    objects_grid_->EnableEditing(false);
    objects_grid_->SetRowLabelSize(40);
    objects_table_ = new ResultGridTable();
    objects_grid_->SetTable(objects_table_, true);
    objectsSizer->Add(objects_grid_, 1, wxEXPAND | wxALL, 8);
    objectsPanel->SetSizer(objectsSizer);
    notebook_->AddPage(objectsPanel, "Objects");
    
    // Statistics tab
    auto* statsPanel = new wxPanel(notebook_, wxID_ANY);
    auto* statsSizer = new wxBoxSizer(wxVERTICAL);
    stats_label_ = new wxStaticText(statsPanel, wxID_ANY, "Select a tablespace to view statistics");
    stats_label_->SetForegroundColour(wxColour(100, 100, 100));
    statsSizer->Add(stats_label_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    stats_text_ = new wxTextCtrl(statsPanel, wxID_ANY, "", wxDefaultPosition,
                                 wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    statsSizer->Add(stats_text_, 1, wxEXPAND | wxALL, 8);
    statsPanel->SetSizer(statsSizer);
    notebook_->AddPage(statsPanel, "Statistics");
    
    rightSizer->Add(notebook_, 1, wxEXPAND);
    rightPanel->SetSizer(rightSizer);

    splitter->SplitVertically(listPanel, rightPanel, 450);
    rootSizer->Add(splitter, 1, wxEXPAND);

    // Status panel
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

    // Bind events
    tablespaces_grid_->Bind(wxEVT_GRID_SELECT_CELL, &StorageManagerFrame::OnTablespaceSelected, this);
}

void StorageManagerFrame::PopulateConnections() {
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

const ConnectionProfile* StorageManagerFrame::GetSelectedProfile() const {
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

bool StorageManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool StorageManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void StorageManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_tablespace = !selected_tablespace_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_tablespace && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_tablespace && !busy);
    if (relocate_button_) relocate_button_->Enable(connected && native && has_tablespace && !busy);
}

void StorageManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void StorageManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void StorageManagerFrame::RefreshTablespaces() {
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
        SetMessage("Tablespaces are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading tablespaces...");
    connection_manager_->ExecuteQueryAsync(
        "SELECT tablespace_name, owner, location, size, used_space, free_space, percent_used "
        "FROM sb_catalog.sb_tablespaces ORDER BY tablespace_name",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                tablespaces_result_ = result;
                if (tablespaces_table_) {
                    tablespaces_table_->Reset(tablespaces_result_.columns, tablespaces_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load tablespaces." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Tablespaces updated");
                }
                UpdateControls();
            });
        });
}

void StorageManagerFrame::RefreshTablespaceObjects(const std::string& tablespace_name) {
    if (!connection_manager_ || tablespace_name.empty()) {
        return;
    }

    std::string sql = "SELECT object_name, object_type, schema_name "
                     "FROM sb_catalog.sb_objects "
                     "WHERE tablespace_name = '" + EscapeSqlLiteral(tablespace_name) + "' "
                     "ORDER BY object_type, object_name";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, tablespace_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, tablespace_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                objects_result_ = result;
                if (objects_table_) {
                    objects_table_->Reset(objects_result_.columns, objects_result_.rows);
                }
                if (ok && objects_label_) {
                    size_t count = result.rows.size();
                    if (count == 0) {
                        objects_label_->SetLabel("No objects in tablespace '" + tablespace_name + "'");
                    } else {
                        objects_label_->SetLabel("Objects in '" + tablespace_name + "' (" +
                                                 std::to_string(count) + ")");
                    }
                    objects_label_->SetForegroundColour(wxColour(80, 80, 80));
                } else if (!ok && !error.empty() && objects_label_) {
                    objects_label_->SetLabel("Objects info unavailable");
                    objects_label_->SetForegroundColour(wxColour(150, 150, 150));
                }
                UpdateControls();
            });
        });
}

void StorageManagerFrame::RefreshTablespaceStats(const std::string& tablespace_name) {
    if (!connection_manager_ || tablespace_name.empty()) {
        return;
    }

    // Query for detailed statistics about the tablespace
    std::string sql = "SELECT tablespace_name, owner, location, size, used_space, free_space, "
                     "percent_used, total_extents, used_extents, free_extents "
                     "FROM sb_catalog.sb_tablespaces "
                     "WHERE tablespace_name = '" + EscapeSqlLiteral(tablespace_name) + "'";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, tablespace_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, tablespace_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                stats_result_ = result;
                if (ok && stats_text_) {
                    stats_text_->SetValue(FormatStats(result));
                    if (stats_label_) {
                        stats_label_->SetLabel("Statistics for '" + tablespace_name + "'");
                        stats_label_->SetForegroundColour(wxColour(80, 80, 80));
                    }
                } else if (!ok && !error.empty() && stats_label_) {
                    stats_label_->SetLabel("Statistics unavailable");
                    stats_label_->SetForegroundColour(wxColour(150, 150, 150));
                    stats_text_->SetValue("");
                }
                UpdateControls();
            });
        });
}

void StorageManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshTablespaces();
                if (!selected_tablespace_.empty()) {
                    RefreshTablespaceObjects(selected_tablespace_);
                    RefreshTablespaceStats(selected_tablespace_);
                }
            });
        });
}

std::string StorageManagerFrame::GetSelectedTablespaceName() const {
    if (!tablespaces_grid_ || tablespaces_result_.rows.empty()) {
        return std::string();
    }
    int row = tablespaces_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= tablespaces_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(tablespaces_result_, row, 
                                     {"tablespace_name", "name", "tablespace"});
    if (!value.empty()) {
        return value;
    }
    if (!tablespaces_result_.rows[row].empty()) {
        return tablespaces_result_.rows[row][0].text;
    }
    return std::string();
}

int StorageManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string StorageManagerFrame::ExtractValue(const QueryResult& result,
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

std::string StorageManagerFrame::FormatStats(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No statistics available.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void StorageManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshTablespaces();
}

void StorageManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void StorageManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshTablespaces();
}

void StorageManagerFrame::OnTablespaceSelected(wxGridEvent& event) {
    selected_tablespace_ = GetSelectedTablespaceName();
    if (!selected_tablespace_.empty()) {
        RefreshTablespaceObjects(selected_tablespace_);
        RefreshTablespaceStats(selected_tablespace_);
    }
    UpdateControls();
    event.Skip();
}

void StorageManagerFrame::OnCreate(wxCommandEvent&) {
    TablespaceEditorDialog dialog(this, TablespaceEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create tablespace statement is empty.");
        return;
    }
    RunCommand(sql, "Tablespace created");
}

void StorageManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_tablespace_.empty()) {
        return;
    }
    TablespaceEditorDialog dialog(this, TablespaceEditorMode::Edit);
    dialog.SetTablespaceName(selected_tablespace_);
    
    // Pre-populate with current values if available
    if (!stats_result_.rows.empty()) {
        // Extract owner, location, size from stats result
        std::string owner = ExtractValue(stats_result_, 0, {"owner"});
        std::string location = ExtractValue(stats_result_, 0, {"location"});
        std::string size = ExtractValue(stats_result_, 0, {"size"});
        if (!owner.empty()) dialog.SetOwner(owner);
        if (!location.empty()) dialog.SetLocation(location);
        if (!size.empty()) dialog.SetSize(size);
    }
    
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Alter tablespace statement is empty.");
        return;
    }
    RunCommand(sql, "Tablespace altered");
}

void StorageManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_tablespace_.empty()) {
        return;
    }
    std::string sql = "DROP TABLESPACE " + QuoteIdentifier(selected_tablespace_) + ";";
    RunCommand(sql, "Tablespace dropped");
}

void StorageManagerFrame::OnRelocate(wxCommandEvent&) {
    if (selected_tablespace_.empty()) {
        return;
    }
    // Relocate dialog would be implemented here
    // For now, show a message about the feature
    SetMessage("Relocate feature: Move objects between tablespaces. "
               "Select objects to move from '" + selected_tablespace_ + "' to another tablespace.");
}

void StorageManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void StorageManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void StorageManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                         app_config_);
    monitor->Show(true);
}

void StorageManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void StorageManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void StorageManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void StorageManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void StorageManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void StorageManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
