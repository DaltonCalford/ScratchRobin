/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "procedure_manager_frame.h"

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

constexpr int kMenuConnect = wxID_HIGHEST + 160;
constexpr int kMenuDisconnect = wxID_HIGHEST + 161;
constexpr int kMenuRefresh = wxID_HIGHEST + 162;
constexpr int kMenuCreateProcedure = wxID_HIGHEST + 163;
constexpr int kMenuCreateFunction = wxID_HIGHEST + 164;
constexpr int kMenuEdit = wxID_HIGHEST + 165;
constexpr int kMenuDrop = wxID_HIGHEST + 166;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 167;
constexpr int kFilterChoiceId = wxID_HIGHEST + 168;

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

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value)) {
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

wxBEGIN_EVENT_TABLE(ProcedureManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, ProcedureManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, ProcedureManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, ProcedureManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, ProcedureManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, ProcedureManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, ProcedureManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, ProcedureManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, ProcedureManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, ProcedureManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, ProcedureManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, ProcedureManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, ProcedureManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreateProcedure, ProcedureManagerFrame::OnCreateProcedure)
    EVT_BUTTON(kMenuCreateFunction, ProcedureManagerFrame::OnCreateFunction)
    EVT_BUTTON(kMenuEdit, ProcedureManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, ProcedureManagerFrame::OnDrop)
    EVT_CHOICE(kFilterChoiceId, ProcedureManagerFrame::OnFilterChanged)
    EVT_CLOSE(ProcedureManagerFrame::OnClose)
wxEND_EVENT_TABLE()

ProcedureManagerFrame::ProcedureManagerFrame(WindowManager* windowManager,
                                             ConnectionManager* connectionManager,
                                             const std::vector<ConnectionProfile>* connections,
                                             const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Procedures & Functions", wxDefaultPosition, wxSize(1100, 720)),
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

void ProcedureManagerFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void ProcedureManagerFrame::BuildLayout() {
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

    // Action panel with filter and buttons
    auto* actionPanel = new wxPanel(this, wxID_ANY);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Filter choice
    actionSizer->Add(new wxStaticText(actionPanel, wxID_ANY, "Filter:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    filter_choice_ = new wxChoice(actionPanel, kFilterChoiceId);
    filter_choice_->Append("Show All");
    filter_choice_->Append("Procedures Only");
    filter_choice_->Append("Functions Only");
    filter_choice_->SetSelection(0);
    actionSizer->Add(filter_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    
    create_procedure_button_ = new wxButton(actionPanel, kMenuCreateProcedure, "Create Procedure");
    create_function_button_ = new wxButton(actionPanel, kMenuCreateFunction, "Create Function");
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Edit");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    actionSizer->Add(create_procedure_button_, 0, wxRIGHT, 6);
    actionSizer->Add(create_function_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionSizer->AddStretchSpacer(1);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Splitter for routines list and details
    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    // Left panel - routines grid
    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Routines"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    routines_grid_ = new wxGrid(listPanel, wxID_ANY);
    routines_grid_->EnableEditing(false);
    routines_grid_->SetRowLabelSize(40);
    routines_table_ = new ResultGridTable();
    routines_grid_->SetTable(routines_table_, true);
    listSizer->Add(routines_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    // Right panel - notebook with tabs
    auto* detailsPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailsSizer = new wxBoxSizer(wxVERTICAL);
    
    notebook_ = new wxNotebook(detailsPanel, wxID_ANY);

    // Definition tab
    auto* definitionTab = new wxPanel(notebook_, wxID_ANY);
    auto* definitionSizer = new wxBoxSizer(wxVERTICAL);
    definitionSizer->Add(new wxStaticText(definitionTab, wxID_ANY, "Routine Definition"), 0,
                        wxLEFT | wxRIGHT | wxTOP, 8);
    definition_text_ = new wxTextCtrl(definitionTab, wxID_ANY, "", wxDefaultPosition,
                                      wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    definitionSizer->Add(definition_text_, 1, wxEXPAND | wxALL, 8);
    definitionTab->SetSizer(definitionSizer);
    notebook_->AddPage(definitionTab, "Definition");

    // Parameters tab
    auto* parametersTab = new wxPanel(notebook_, wxID_ANY);
    auto* parametersSizer = new wxBoxSizer(wxVERTICAL);
    parametersSizer->Add(new wxStaticText(parametersTab, wxID_ANY, "Parameters"), 0,
                        wxLEFT | wxRIGHT | wxTOP, 8);
    parameters_grid_ = new wxGrid(parametersTab, wxID_ANY);
    parameters_grid_->EnableEditing(false);
    parameters_grid_->SetRowLabelSize(40);
    parameters_table_ = new ResultGridTable();
    parameters_grid_->SetTable(parameters_table_, true);
    parametersSizer->Add(parameters_grid_, 1, wxEXPAND | wxALL, 8);
    parametersTab->SetSizer(parametersSizer);
    notebook_->AddPage(parametersTab, "Parameters");

    // Dependencies tab
    auto* dependenciesTab = new wxPanel(notebook_, wxID_ANY);
    auto* dependenciesSizer = new wxBoxSizer(wxVERTICAL);
    dependenciesSizer->Add(new wxStaticText(dependenciesTab, wxID_ANY, "Dependencies"), 0,
                           wxLEFT | wxRIGHT | wxTOP, 8);
    dependencies_text_ = new wxTextCtrl(dependenciesTab, wxID_ANY, "", wxDefaultPosition,
                                        wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    dependenciesSizer->Add(dependencies_text_, 1, wxEXPAND | wxALL, 8);
    dependenciesTab->SetSizer(dependenciesSizer);
    notebook_->AddPage(dependenciesTab, "Dependencies");

    detailsSizer->Add(notebook_, 1, wxEXPAND);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(listPanel, detailsPanel, 480);
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

    routines_grid_->Bind(wxEVT_GRID_SELECT_CELL, &ProcedureManagerFrame::OnRoutineSelected, this);
}

void ProcedureManagerFrame::PopulateConnections() {
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

const ConnectionProfile* ProcedureManagerFrame::GetSelectedProfile() const {
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

bool ProcedureManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool ProcedureManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void ProcedureManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_routine = !selected_routine_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (filter_choice_) filter_choice_->Enable(connected && native && !busy);
    if (create_procedure_button_) create_procedure_button_->Enable(connected && native && !busy);
    if (create_function_button_) create_function_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_routine && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_routine && !busy);
}

void ProcedureManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void ProcedureManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void ProcedureManagerFrame::RefreshRoutines() {
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
        SetMessage("Procedures and functions are available only for ScratchBird connections.");
        return;
    }

    // Build query based on filter
    std::string sql = 
        "SELECT routine_name, routine_type, schema_name, language, "
        "return_type, is_deterministic, security_type, created "
        "FROM sb_catalog.sb_routines ";
    
    int filter = filter_choice_ ? filter_choice_->GetSelection() : 0;
    if (filter == 1) {
        sql += "WHERE routine_type = 'PROCEDURE' ";
    } else if (filter == 2) {
        sql += "WHERE routine_type = 'FUNCTION' ";
    }
    
    sql += "ORDER BY routine_type, schema_name, routine_name";

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading routines...");
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                routines_result_ = result;
                if (routines_table_) {
                    routines_table_->Reset(routines_result_.columns, routines_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load routines." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Routines updated");
                }
                UpdateControls();
            });
        });
}

void ProcedureManagerFrame::RefreshRoutineDefinition(const std::string& routine_name) {
    if (!connection_manager_ || routine_name.empty()) {
        return;
    }
    std::string sql = "SHOW ROUTINE " + QuoteIdentifier(routine_name);
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, routine_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, routine_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok) {
                    if (definition_text_) {
                        definition_text_->SetValue(FormatDefinition(result));
                    }
                    // Also refresh dependencies
                    RefreshDependencies(routine_name);
                } else if (!error.empty()) {
                    SetMessage(error);
                    if (definition_text_) {
                        definition_text_->SetValue("Error loading definition: " + error);
                    }
                }
                UpdateControls();
            });
        });
}

void ProcedureManagerFrame::RefreshParameters(const std::string& routine_name) {
    if (!connection_manager_ || routine_name.empty()) {
        return;
    }
    std::string sql = 
        "SELECT parameter_name, data_type, parameter_mode, ordinal_position "
        "FROM sb_catalog.sb_parameters "
        "WHERE routine_name = '" + EscapeSqlLiteral(routine_name) + "' "
        "ORDER BY ordinal_position";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                parameters_result_ = result;
                if (parameters_table_) {
                    parameters_table_->Reset(parameters_result_.columns, parameters_result_.rows);
                }
                if (!ok && !error.empty()) {
                    // Parameters catalog may not exist - clear grid but don't show error
                    if (parameters_table_) {
                        parameters_table_->Clear();
                    }
                }
                UpdateControls();
            });
        });
}

void ProcedureManagerFrame::RefreshDependencies(const std::string& routine_name) {
    if (!connection_manager_ || routine_name.empty() || !dependencies_text_) {
        return;
    }

    // Query objects that this routine depends on
    std::string depends_sql = 
        "SELECT object_name, object_type "
        "FROM sb_catalog.sb_routine_dependencies "
        "WHERE routine_name = '" + EscapeSqlLiteral(routine_name) + "' "
        "ORDER BY object_type, object_name";
    
    // Query routines that depend on this routine
    std::string dependents_sql = 
        "SELECT routine_name, routine_type "
        "FROM sb_catalog.sb_routine_dependencies "
        "WHERE object_name = '" + EscapeSqlLiteral(routine_name) + "' "
        "ORDER BY routine_type, routine_name";
    
    pending_queries_++;
    UpdateControls();
    
    connection_manager_->ExecuteQueryAsync(
        depends_sql, [this, routine_name, dependents_sql](bool ok1, QueryResult depends_result, 
                                                          const std::string& error1) mutable {
            CallAfter([this, ok1, depends_result = std::move(depends_result), error1, 
                       routine_name, dependents_sql]() mutable {
                if (!ok1) {
                    // Catalog table may not exist
                    dependencies_text_->SetValue("Dependency information not available.\n"
                                                "This feature requires ScratchBird catalog tables.");
                    pending_queries_ = std::max(0, pending_queries_ - 1);
                    UpdateControls();
                    return;
                }
                
                connection_manager_->ExecuteQueryAsync(
                    dependents_sql, [this, depends_result](bool ok2, QueryResult dependents_result,
                                                           const std::string& error2) mutable {
                        CallAfter([this, ok2, depends_result = std::move(depends_result),
                                   dependents_result = std::move(dependents_result), error2]() mutable {
                            pending_queries_ = std::max(0, pending_queries_ - 1);
                            
                            std::string text;
                            text += "╔══════════════════════════════════════════════════════════════╗\n";
                            text += "║              ROUTINE DEPENDENCIES                            ║\n";
                            text += "╚══════════════════════════════════════════════════════════════╝\n\n";
                            
                            // Objects this routine depends on
                            text += "📋 DEPENDS ON:\n";
                            text += "────────────────────────────────────────────────────────────────\n";
                            if (depends_result.rows.empty()) {
                                text += "   (none - no external dependencies)\n";
                            } else {
                                for (const auto& row : depends_result.rows) {
                                    if (row.size() >= 2) {
                                        std::string obj_name = row[0].isNull ? "" : row[0].text;
                                        std::string obj_type = row[1].isNull ? "" : row[1].text;
                                        text += "   ↳ " + obj_type + ": " + obj_name + "\n";
                                    }
                                }
                            }
                            text += "\n";
                            
                            // Routines that depend on this routine
                            text += "📎 DEPENDED ON BY:\n";
                            text += "────────────────────────────────────────────────────────────────\n";
                            if (!ok2 || dependents_result.rows.empty()) {
                                text += "   (none - no routines depend on this)\n";
                            } else {
                                for (const auto& row : dependents_result.rows) {
                                    if (row.size() >= 2) {
                                        std::string rtn_name = row[0].isNull ? "" : row[0].text;
                                        std::string rtn_type = row[1].isNull ? "" : row[1].text;
                                        text += "   ↱ " + rtn_type + ": " + rtn_name + "\n";
                                    }
                                }
                            }
                            text += "\n";
                            
                            dependencies_text_->SetValue(text);
                            UpdateControls();
                        });
                    });
            });
        });
}

void ProcedureManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshRoutines();
                if (!selected_routine_.empty()) {
                    RefreshRoutineDefinition(selected_routine_);
                    RefreshParameters(selected_routine_);
                }
            });
        });
}

std::string ProcedureManagerFrame::GetSelectedRoutineName() const {
    if (!routines_grid_ || routines_result_.rows.empty()) {
        return std::string();
    }
    int row = routines_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= routines_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(routines_result_, row, 
                                     {"routine_name", "routine name", "name", "routine"});
    if (!value.empty()) {
        return value;
    }
    if (!routines_result_.rows[row].empty()) {
        return routines_result_.rows[row][0].text;
    }
    return std::string();
}

std::string ProcedureManagerFrame::GetSelectedRoutineType() const {
    if (!routines_grid_ || routines_result_.rows.empty()) {
        return std::string();
    }
    int row = routines_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= routines_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(routines_result_, row, 
                                     {"routine_type", "routine type", "type"});
    if (!value.empty()) {
        return value;
    }
    // Default based on name or first column
    if (routines_result_.rows[row].size() > 1) {
        return routines_result_.rows[row][1].text;
    }
    return "PROCEDURE";
}

int ProcedureManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string ProcedureManagerFrame::ExtractValue(const QueryResult& result,
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

std::string ProcedureManagerFrame::FormatDefinition(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No routine definition returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        // Skip metadata columns, show routine body if available
        std::string col_name = ToLowerCopy(result.columns[i].name);
        if (col_name == "routine_body" || col_name == "body" || col_name == "source" ||
            col_name == "routine_source" || col_name == "definition") {
            out << row[i].text;
            return out.str();
        }
    }
    // Fallback: format all columns
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void ProcedureManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshRoutines();
}

void ProcedureManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
    
    // Clear displays
    if (routines_table_) routines_table_->Clear();
    if (parameters_table_) parameters_table_->Clear();
    if (definition_text_) definition_text_->Clear();
    if (dependencies_text_) dependencies_text_->Clear();
    selected_routine_.clear();
    selected_routine_type_.clear();
}

void ProcedureManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshRoutines();
}

void ProcedureManagerFrame::OnRoutineSelected(wxGridEvent& event) {
    selected_routine_ = GetSelectedRoutineName();
    selected_routine_type_ = GetSelectedRoutineType();
    if (!selected_routine_.empty()) {
        RefreshRoutineDefinition(selected_routine_);
        RefreshParameters(selected_routine_);
    }
    UpdateControls();
    event.Skip();
}

void ProcedureManagerFrame::OnFilterChanged(wxCommandEvent&) {
    RefreshRoutines();
}

void ProcedureManagerFrame::OnCreateProcedure(wxCommandEvent&) {
    // Open SQL editor with CREATE PROCEDURE template
    if (!window_manager_) {
        return;
    }
    std::string sql = 
        "CREATE PROCEDURE schema_name.procedure_name (\n"
        "    param1 datatype,\n"
        "    param2 datatype\n"
        ")\n"
        "LANGUAGE SQL\n"
        "AS $$\n"
        "BEGIN\n"
        "    -- Procedure body here\n"
        "END;\n"
        "$$;";
    
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->LoadStatement(sql);
    editor->Show(true);
}

void ProcedureManagerFrame::OnCreateFunction(wxCommandEvent&) {
    // Open SQL editor with CREATE FUNCTION template
    if (!window_manager_) {
        return;
    }
    std::string sql = 
        "CREATE FUNCTION schema_name.function_name (\n"
        "    param1 datatype\n"
        ")\n"
        "RETURNS return_datatype\n"
        "LANGUAGE SQL\n"
        "DETERMINISTIC\n"
        "AS $$\n"
        "BEGIN\n"
        "    -- Function body here\n"
        "    RETURN value;\n"
        "END;\n"
        "$$;";
    
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->LoadStatement(sql);
    editor->Show(true);
}

void ProcedureManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_routine_.empty()) {
        return;
    }
    // Open SQL editor with ALTER statement
    if (!window_manager_) {
        return;
    }
    
    std::string sql;
    if (selected_routine_type_ == "FUNCTION") {
        sql = "ALTER FUNCTION " + QuoteIdentifier(selected_routine_) + " ...";
    } else {
        sql = "ALTER PROCEDURE " + QuoteIdentifier(selected_routine_) + " ...";
    }
    
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->LoadStatement(sql);
    editor->Show(true);
}

void ProcedureManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_routine_.empty()) {
        return;
    }
    
    std::string routine_type = selected_routine_type_;
    if (routine_type.empty()) {
        routine_type = "ROUTINE";
    }
    
    std::string sql;
    if (ToLowerCopy(routine_type) == "function") {
        sql = "DROP FUNCTION " + QuoteIdentifier(selected_routine_) + ";";
    } else {
        sql = "DROP PROCEDURE " + QuoteIdentifier(selected_routine_) + ";";
    }
    
    RunCommand(sql, routine_type + " dropped");
}

void ProcedureManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void ProcedureManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void ProcedureManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                        app_config_);
    monitor->Show(true);
}

void ProcedureManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void ProcedureManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void ProcedureManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void ProcedureManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void ProcedureManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void ProcedureManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void ProcedureManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
