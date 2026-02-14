/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "view_manager_frame.h"

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
constexpr int kConnectionChoiceId = wxID_HIGHEST + 136;

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

std::string FormatViewPath(const std::string& schema, const std::string& view) {
    std::string result;
    if (!schema.empty()) {
        result = QuoteIdentifier(schema) + ".";
    }
    result += QuoteIdentifier(view);
    return result;
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

wxBEGIN_EVENT_TABLE(ViewManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, ViewManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, ViewManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, ViewManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, ViewManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, ViewManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, ViewManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, ViewManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, ViewManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, ViewManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, ViewManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, ViewManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, ViewManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, ViewManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, ViewManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, ViewManagerFrame::OnDrop)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, ViewManagerFrame::OnNotebookPageChanged)
    EVT_CLOSE(ViewManagerFrame::OnClose)
wxEND_EVENT_TABLE()

ViewManagerFrame::ViewManagerFrame(WindowManager* windowManager,
                                   ConnectionManager* connectionManager,
                                   const std::vector<ConnectionProfile>* connections,
                                   const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Views", wxDefaultPosition, wxSize(1000, 700)),
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

void ViewManagerFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void ViewManagerFrame::BuildLayout() {
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

    // Toolbar with action buttons
    auto* actionPanel = new wxPanel(this, wxID_ANY);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    create_button_ = new wxButton(actionPanel, kMenuCreate, "Create");
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Edit");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Main splitter for views list and notebook
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    // Left panel - views grid
    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Views"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    views_grid_ = new wxGrid(listPanel, wxID_ANY);
    views_grid_->EnableEditing(false);
    views_grid_->SetRowLabelSize(40);
    views_table_ = new ResultGridTable();
    views_grid_->SetTable(views_table_, true);
    listSizer->Add(views_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    // Right panel - notebook with tabs
    auto* rightPanel = new wxPanel(splitter, wxID_ANY);
    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "View Details"), 0,
                    wxLEFT | wxRIGHT | wxTOP, 8);
    
    notebook_ = new wxNotebook(rightPanel, wxID_ANY);
    
    // Definition tab
    auto* defPanel = new wxPanel(notebook_, wxID_ANY);
    auto* defSizer = new wxBoxSizer(wxVERTICAL);
    definition_text_ = new wxTextCtrl(defPanel, wxID_ANY, "", wxDefaultPosition,
                                      wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    defSizer->Add(definition_text_, 1, wxEXPAND | wxALL, 8);
    defPanel->SetSizer(defSizer);
    notebook_->AddPage(defPanel, "Definition");
    
    // Columns tab
    auto* colPanel = new wxPanel(notebook_, wxID_ANY);
    auto* colSizer = new wxBoxSizer(wxVERTICAL);
    columns_grid_ = new wxGrid(colPanel, wxID_ANY);
    columns_grid_->EnableEditing(false);
    columns_grid_->SetRowLabelSize(40);
    columns_table_ = new ResultGridTable();
    columns_grid_->SetTable(columns_table_, true);
    colSizer->Add(columns_grid_, 1, wxEXPAND | wxALL, 8);
    colPanel->SetSizer(colSizer);
    notebook_->AddPage(colPanel, "Columns");
    
    // Dependencies tab
    auto* depPanel = new wxPanel(notebook_, wxID_ANY);
    auto* depSizer = new wxBoxSizer(wxVERTICAL);
    dependencies_grid_ = new wxGrid(depPanel, wxID_ANY);
    dependencies_grid_->EnableEditing(false);
    dependencies_grid_->SetRowLabelSize(40);
    dependencies_table_ = new ResultGridTable();
    dependencies_grid_->SetTable(dependencies_table_, true);
    depSizer->Add(dependencies_grid_, 1, wxEXPAND | wxALL, 8);
    depPanel->SetSizer(depSizer);
    notebook_->AddPage(depPanel, "Dependencies");
    
    rightSizer->Add(notebook_, 1, wxEXPAND | wxALL, 8);
    rightPanel->SetSizer(rightSizer);

    splitter->SplitVertically(listPanel, rightPanel, 400);
    rootSizer->Add(splitter, 1, wxEXPAND);

    // Status panel at bottom
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

    views_grid_->Bind(wxEVT_GRID_SELECT_CELL, &ViewManagerFrame::OnViewSelected, this);
}

void ViewManagerFrame::PopulateConnections() {
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

const ConnectionProfile* ViewManagerFrame::GetSelectedProfile() const {
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

bool ViewManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool ViewManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void ViewManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_view = !selected_view_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_view && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_view && !busy);
}

void ViewManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void ViewManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void ViewManagerFrame::RefreshViews() {
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
        SetMessage("Views manager is available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading views...");
    connection_manager_->ExecuteQueryAsync(
        "SELECT view_name, schema_name, view_type, is_updatable, check_option "
        "FROM sb_catalog.sb_views ORDER BY schema_name, view_name",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                views_result_ = result;
                if (views_table_) {
                    views_table_->Reset(views_result_.columns, views_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load views." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Views updated");
                    // Clear detail views
                    if (definition_text_) definition_text_->Clear();
                    if (columns_table_) columns_table_->Reset({}, {});
                    if (dependencies_table_) dependencies_table_->Reset({}, {});
                    selected_view_.clear();
                    selected_schema_.clear();
                }
                UpdateControls();
            });
        });
}

void ViewManagerFrame::RefreshViewDefinition(const std::string& view_name) {
    if (!connection_manager_ || view_name.empty()) {
        return;
    }
    std::string sql = "SELECT view_definition FROM sb_catalog.sb_views "
                     "WHERE view_name = '" + EscapeSqlLiteral(view_name) + "'";
    if (!selected_schema_.empty()) {
        sql += " AND schema_name = '" + EscapeSqlLiteral(selected_schema_) + "'";
    }
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, view_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, view_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                definition_result_ = result;
                if (ok && definition_text_) {
                    definition_text_->SetValue(FormatDefinition(definition_result_));
                } else if (!ok && !error.empty()) {
                    SetMessage("Failed to load definition: " + error);
                }
                UpdateControls();
            });
        });
}

void ViewManagerFrame::RefreshViewColumns(const std::string& view_name) {
    if (!connection_manager_ || view_name.empty()) {
        return;
    }
    std::string sql = "SELECT column_name, data_type, is_nullable "
                     "FROM sb_catalog.sb_view_columns "
                     "WHERE view_name = '" + EscapeSqlLiteral(view_name) + "'";
    if (!selected_schema_.empty()) {
        sql += " AND schema_name = '" + EscapeSqlLiteral(selected_schema_) + "'";
    }
    sql += " ORDER BY ordinal_position";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, view_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, view_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                columns_result_ = result;
                if (columns_table_) {
                    columns_table_->Reset(columns_result_.columns, columns_result_.rows);
                }
                if (!ok && !error.empty()) {
                    SetMessage("Failed to load columns: " + error);
                }
                UpdateControls();
            });
        });
}

void ViewManagerFrame::RefreshViewDependencies(const std::string& view_name) {
    if (!connection_manager_ || view_name.empty()) {
        return;
    }
    // Query dependencies from catalog - shows tables and views this view depends on
    std::string sql = "SELECT DISTINCT dep.referenced_schema, dep.referenced_name, dep.referenced_type "
                     "FROM sb_catalog.sb_dependencies dep "
                     "WHERE dep.dependent_name = '" + EscapeSqlLiteral(view_name) + "'";
    if (!selected_schema_.empty()) {
        sql += " AND dep.dependent_schema = '" + EscapeSqlLiteral(selected_schema_) + "'";
    }
    sql += " AND dep.referenced_type IN ('TABLE', 'VIEW') "
           "ORDER BY dep.referenced_schema, dep.referenced_name";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, view_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, view_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                dependencies_result_ = result;
                if (dependencies_table_) {
                    dependencies_table_->Reset(dependencies_result_.columns, dependencies_result_.rows);
                }
                if (!ok && !error.empty()) {
                    SetMessage("Failed to load dependencies: " + error);
                }
                UpdateControls();
            });
        });
}

void ViewManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshViews();
                if (!selected_view_.empty()) {
                    RefreshViewDefinition(selected_view_);
                    RefreshViewColumns(selected_view_);
                    RefreshViewDependencies(selected_view_);
                }
            });
        });
}

std::string ViewManagerFrame::GetSelectedViewName() const {
    if (!views_grid_ || views_result_.rows.empty()) {
        return std::string();
    }
    int row = views_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= views_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(views_result_, row, {"view_name", "view", "name"});
    if (!value.empty()) {
        return value;
    }
    if (!views_result_.rows[row].empty()) {
        return views_result_.rows[row][0].text;
    }
    return std::string();
}

std::string ViewManagerFrame::GetSelectedSchemaName() const {
    if (!views_grid_ || views_result_.rows.empty()) {
        return std::string();
    }
    int row = views_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= views_result_.rows.size()) {
        return std::string();
    }
    return ExtractValue(views_result_, row, {"schema_name", "schema"});
}

int ViewManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string ViewManagerFrame::ExtractValue(const QueryResult& result,
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

std::string ViewManagerFrame::FormatDefinition(const QueryResult& result) const {
    if (result.rows.empty()) {
        // Try to build a definition from information_schema
        return "-- No definition found in catalog";
    }
    std::string definition = ExtractValue(result, 0, {"view_definition", "definition", "text"});
    if (!definition.empty()) {
        return definition;
    }
    std::ostringstream out;
    for (size_t i = 0; i < result.columns.size(); ++i) {
        out << "-- " << result.columns[i].name << ": ";
        if (!result.rows[0].empty() && i < result.rows[0].size()) {
            out << result.rows[0][i].text;
        }
        out << "\n";
    }
    return out.str();
}

std::string ViewManagerFrame::FormatDependencies(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No dependencies found.";
    }
    std::ostringstream out;
    out << "Objects referenced by this view:\n\n";
    for (const auto& row : result.rows) {
        for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
            out << result.columns[i].name << ": " << row[i].text << "\n";
        }
        out << "\n";
    }
    return out.str();
}

void ViewManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshViews();
}

void ViewManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void ViewManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshViews();
}

void ViewManagerFrame::OnViewSelected(wxGridEvent& event) {
    selected_view_ = GetSelectedViewName();
    selected_schema_ = GetSelectedSchemaName();
    if (!selected_view_.empty()) {
        RefreshViewDefinition(selected_view_);
        RefreshViewColumns(selected_view_);
        RefreshViewDependencies(selected_view_);
    }
    UpdateControls();
    event.Skip();
}

void ViewManagerFrame::OnCreate(wxCommandEvent&) {
    // Open SQL editor with CREATE VIEW template
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
    // Could pre-populate with a CREATE VIEW template
}

void ViewManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_view_.empty()) {
        return;
    }
    // Open SQL editor with ALTER VIEW or show definition for editing
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
    // Could pre-populate with current definition
}

void ViewManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_view_.empty()) {
        return;
    }
    std::string sql = "DROP VIEW " + FormatViewPath(selected_schema_, selected_view_) + ";";
    RunCommand(sql, "View dropped");
}

void ViewManagerFrame::OnNotebookPageChanged(wxNotebookEvent& event) {
    event.Skip();
}

void ViewManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void ViewManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void ViewManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                         app_config_);
    monitor->Show(true);
}

void ViewManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void ViewManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void ViewManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    // This is the View Manager, no need to open itself
    // Could navigate to DomainManagerFrame if needed
}

void ViewManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void ViewManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void ViewManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void ViewManagerFrame::OnOpenSequenceManager(wxCommandEvent&) {
    // Sequence manager not yet implemented - placeholder
    SetMessage("Sequence Manager not yet implemented.");
}

void ViewManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
