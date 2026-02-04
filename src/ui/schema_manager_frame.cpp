/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "schema_manager_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_editor_dialog.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/choicdlg.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 140;
constexpr int kMenuDisconnect = wxID_HIGHEST + 141;
constexpr int kMenuRefresh = wxID_HIGHEST + 142;
constexpr int kMenuCreate = wxID_HIGHEST + 143;
constexpr int kMenuEdit = wxID_HIGHEST + 144;
constexpr int kMenuDrop = wxID_HIGHEST + 145;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 146;

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

std::string EscapeString(const std::string& value) {
    std::string out;
    for (char ch : value) {
        if (ch == '\'') {
            out += "''";
        } else if (ch == '\\') {
            out += "\\\\";
        } else {
            out.push_back(ch);
        }
    }
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

wxBEGIN_EVENT_TABLE(SchemaManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, SchemaManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, SchemaManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, SchemaManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, SchemaManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, SchemaManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, SchemaManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, SchemaManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, SchemaManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, SchemaManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, SchemaManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, SchemaManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, SchemaManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, SchemaManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, SchemaManagerFrame::OnDrop)
    EVT_CLOSE(SchemaManagerFrame::OnClose)
wxEND_EVENT_TABLE()

SchemaManagerFrame::SchemaManagerFrame(WindowManager* windowManager,
                                       ConnectionManager* connectionManager,
                                       const std::vector<ConnectionProfile>* connections,
                                       const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Schemas", wxDefaultPosition, wxSize(980, 680)),
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

void SchemaManagerFrame::BuildMenu() {
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

void SchemaManagerFrame::BuildLayout() {
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
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Alter");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Schemas"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    schemas_grid_ = new wxGrid(listPanel, wxID_ANY);
    schemas_grid_->EnableEditing(false);
    schemas_grid_->SetRowLabelSize(40);
    schemas_table_ = new ResultGridTable();
    schemas_grid_->SetTable(schemas_table_, true);
    listSizer->Add(schemas_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    detailSizer->Add(new wxStaticText(detailPanel, wxID_ANY, "Details"), 0,
                     wxLEFT | wxRIGHT | wxTOP, 8);
    
    // Object counts label
    object_counts_label_ = new wxStaticText(detailPanel, wxID_ANY, "Select a schema to view object counts");
    object_counts_label_->SetForegroundColour(wxColour(100, 100, 100));
    detailSizer->Add(object_counts_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    details_text_ = new wxTextCtrl(detailPanel, wxID_ANY, "", wxDefaultPosition,
                                   wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    detailSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    detailPanel->SetSizer(detailSizer);

    splitter->SplitVertically(listPanel, detailPanel, 380);
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

    schemas_grid_->Bind(wxEVT_GRID_SELECT_CELL, &SchemaManagerFrame::OnSchemaSelected, this);
}

void SchemaManagerFrame::PopulateConnections() {
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

const ConnectionProfile* SchemaManagerFrame::GetSelectedProfile() const {
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

bool SchemaManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool SchemaManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void SchemaManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_schema = !selected_schema_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_schema && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_schema && !busy);
}

void SchemaManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void SchemaManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void SchemaManagerFrame::RefreshSchemas() {
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
        SetMessage("Schemas are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading schemas...");
    connection_manager_->ExecuteQueryAsync(
        "SHOW SCHEMAS",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                schemas_result_ = result;
                if (schemas_table_) {
                    schemas_table_->Reset(schemas_result_.columns, schemas_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load schemas." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Schemas updated");
                }
                UpdateControls();
            });
        });
}

void SchemaManagerFrame::RefreshSchemaDetails(const std::string& schema_name) {
    if (!connection_manager_ || schema_name.empty()) {
        return;
    }
    std::string sql = "SHOW SCHEMA " + QuoteIdentifier(schema_name);
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, schema_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, schema_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                schema_details_result_ = result;
                if (ok) {
                    if (details_text_) {
                        details_text_->SetValue(FormatDetails(schema_details_result_));
                    }
                    // Fetch object counts for this schema
                    FetchObjectCounts(schema_name);
                } else if (!error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void SchemaManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshSchemas();
                if (!selected_schema_.empty()) {
                    RefreshSchemaDetails(selected_schema_);
                }
            });
        });
}

std::string SchemaManagerFrame::GetSelectedSchemaName() const {
    if (!schemas_grid_ || schemas_result_.rows.empty()) {
        return std::string();
    }
    int row = schemas_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= schemas_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(schemas_result_, row, {"schema", "schema_name", "schema name"});
    if (!value.empty()) {
        return value;
    }
    if (!schemas_result_.rows[row].empty()) {
        return schemas_result_.rows[row][0].text;
    }
    return std::string();
}

int SchemaManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string SchemaManagerFrame::ExtractValue(const QueryResult& result,
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

std::string SchemaManagerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No schema details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void SchemaManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshSchemas();
}

void SchemaManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void SchemaManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshSchemas();
}

void SchemaManagerFrame::OnSchemaSelected(wxGridEvent& event) {
    selected_schema_ = GetSelectedSchemaName();
    if (!selected_schema_.empty()) {
        RefreshSchemaDetails(selected_schema_);
    }
    UpdateControls();
    event.Skip();
}

void SchemaManagerFrame::OnCreate(wxCommandEvent&) {
    SchemaEditorDialog dialog(this, SchemaEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create schema statement is empty.");
        return;
    }
    RunCommand(sql, "Schema created");
}

void SchemaManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_schema_.empty()) {
        return;
    }
    SchemaEditorDialog dialog(this, SchemaEditorMode::Alter);
    dialog.SetSchemaName(selected_schema_);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Alter schema statement is empty.");
        return;
    }
    RunCommand(sql, "Schema altered");
}

void SchemaManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_schema_.empty()) {
        return;
    }
    wxArrayString choices;
    choices.Add("Drop (default)");
    choices.Add("Drop (cascade)");
    choices.Add("Drop (restrict)");
    wxSingleChoiceDialog dialog(this, "Drop schema option", "Drop Schema", choices);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = "DROP SCHEMA " + QuoteIdentifier(selected_schema_);
    if (dialog.GetSelection() == 1) {
        sql += " CASCADE";
    } else if (dialog.GetSelection() == 2) {
        sql += " RESTRICT";
    }
    sql += ";";
    RunCommand(sql, "Schema dropped");
}

void SchemaManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void SchemaManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void SchemaManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                        app_config_);
    monitor->Show(true);
}

void SchemaManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void SchemaManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void SchemaManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void SchemaManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void SchemaManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void SchemaManagerFrame::FetchObjectCounts(const std::string& schema_name) {
    if (!connection_manager_ || schema_name.empty()) {
        return;
    }
    
    // Query object counts from catalog tables (native profiles only)
    std::string sql = "SELECT\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_tables WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as table_count,\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_views WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as view_count,\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_procedures WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as procedure_count,\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_functions WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as function_count,\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_domains WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as domain_count,\n"
                     "  (SELECT COUNT(*) FROM sb_catalog.sb_sequences WHERE schema_name = '" + 
                     EscapeString(schema_name) + "') as sequence_count;";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok && object_counts_label_) {
                    std::string counts_text;
                    if (!result.rows.empty() && result.rows[0].size() >= 6) {
                        auto& row = result.rows[0];
                        auto get_value = [](const QueryValue& val) -> std::string {
                            return val.isNull ? "0" : val.text;
                        };
                        auto table_count = get_value(row[0]);
                        auto view_count = get_value(row[1]);
                        auto proc_count = get_value(row[2]);
                        auto func_count = get_value(row[3]);
                        auto domain_count = get_value(row[4]);
                        auto seq_count = get_value(row[5]);
                        
                        counts_text = "Objects: ";
                        bool first = true;
                        auto add_count = [&](const std::string& name, const std::string& val) {
                            if (val == "0" || val.empty()) return;
                            if (!first) counts_text += ", ";
                            first = false;
                            counts_text += val + " " + name;
                            if (val != "1") counts_text += "s";
                        };
                        add_count("table", table_count);
                        add_count("view", view_count);
                        add_count("procedure", proc_count);
                        add_count("function", func_count);
                        add_count("domain", domain_count);
                        add_count("sequence", seq_count);
                        
                        if (first) {
                            counts_text = "No objects in this schema";
                        }
                    } else {
                        counts_text = "Object counts unavailable";
                    }
                    object_counts_label_->SetLabel(counts_text);
                } else if (!ok && !error.empty()) {
                    // Silently ignore errors - catalog tables may not exist
                    object_counts_label_->SetLabel("Object counts unavailable");
                }
                UpdateControls();
            });
        });
}

void SchemaManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
