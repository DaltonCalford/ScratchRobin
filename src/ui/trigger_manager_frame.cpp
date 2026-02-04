/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "trigger_manager_frame.h"

#include "core/config.h"
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

constexpr int kMenuConnect = wxID_HIGHEST + 200;
constexpr int kMenuDisconnect = wxID_HIGHEST + 201;
constexpr int kMenuRefresh = wxID_HIGHEST + 202;
constexpr int kMenuCreate = wxID_HIGHEST + 203;
constexpr int kMenuEdit = wxID_HIGHEST + 204;
constexpr int kMenuDrop = wxID_HIGHEST + 205;
constexpr int kMenuEnable = wxID_HIGHEST + 206;
constexpr int kMenuDisable = wxID_HIGHEST + 207;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 208;

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

wxBEGIN_EVENT_TABLE(TriggerManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, TriggerManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, TriggerManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, TriggerManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, TriggerManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, TriggerManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, TriggerManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, TriggerManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, TriggerManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, TriggerManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, TriggerManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, TriggerManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, TriggerManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, TriggerManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, TriggerManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, TriggerManagerFrame::OnDrop)
    EVT_BUTTON(kMenuEnable, TriggerManagerFrame::OnEnable)
    EVT_BUTTON(kMenuDisable, TriggerManagerFrame::OnDisable)
    EVT_CLOSE(TriggerManagerFrame::OnClose)
wxEND_EVENT_TABLE()

TriggerManagerFrame::TriggerManagerFrame(WindowManager* windowManager,
                                         ConnectionManager* connectionManager,
                                         const std::vector<ConnectionProfile>* connections,
                                         const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Triggers", wxDefaultPosition, wxSize(1100, 720)),
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

void TriggerManagerFrame::BuildMenu() {
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

void TriggerManagerFrame::BuildLayout() {
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
    edit_button_ = new wxButton(actionPanel, kMenuEdit, "Edit");
    drop_button_ = new wxButton(actionPanel, kMenuDrop, "Drop");
    enable_button_ = new wxButton(actionPanel, kMenuEnable, "Enable");
    disable_button_ = new wxButton(actionPanel, kMenuDisable, "Disable");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionSizer->Add(enable_button_, 0, wxRIGHT, 6);
    actionSizer->Add(disable_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Triggers"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    triggers_grid_ = new wxGrid(listPanel, wxID_ANY);
    triggers_grid_->EnableEditing(false);
    triggers_grid_->SetRowLabelSize(40);
    triggers_table_ = new ResultGridTable();
    triggers_grid_->SetTable(triggers_table_, true);
    listSizer->Add(triggers_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailsPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailsSizer = new wxBoxSizer(wxVERTICAL);
    auto* notebook = new wxNotebook(detailsPanel, wxID_ANY);

    auto* definitionTab = new wxPanel(notebook, wxID_ANY);
    auto* definitionSizer = new wxBoxSizer(wxVERTICAL);
    definitionSizer->Add(new wxStaticText(definitionTab, wxID_ANY, "Trigger Body:"), 0,
                         wxLEFT | wxRIGHT | wxTOP, 8);
    definition_text_ = new wxTextCtrl(definitionTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                      wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    definitionSizer->Add(definition_text_, 1, wxEXPAND | wxALL, 8);
    definitionTab->SetSizer(definitionSizer);

    auto* timingTab = new wxPanel(notebook, wxID_ANY);
    auto* timingSizer = new wxBoxSizer(wxVERTICAL);
    timingSizer->Add(new wxStaticText(timingTab, wxID_ANY, "Trigger Timing:"), 0,
                     wxLEFT | wxRIGHT | wxTOP, 8);
    timing_text_ = new wxTextCtrl(timingTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    timingSizer->Add(timing_text_, 1, wxEXPAND | wxALL, 8);
    timingTab->SetSizer(timingSizer);

    auto* depsTab = new wxPanel(notebook, wxID_ANY);
    auto* depsSizer = new wxBoxSizer(wxVERTICAL);
    depsSizer->Add(new wxStaticText(depsTab, wxID_ANY, "Dependencies:"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    dependencies_text_ = new wxTextCtrl(depsTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    depsSizer->Add(dependencies_text_, 1, wxEXPAND | wxALL, 8);
    depsTab->SetSizer(depsSizer);

    notebook->AddPage(definitionTab, "Definition");
    notebook->AddPage(timingTab, "Timing");
    notebook->AddPage(depsTab, "Dependencies");

    detailsSizer->Add(notebook, 1, wxEXPAND);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(listPanel, detailsPanel, 480);
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

    triggers_grid_->Bind(wxEVT_GRID_SELECT_CELL, &TriggerManagerFrame::OnTriggerSelected, this);
}

void TriggerManagerFrame::PopulateConnections() {
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

const ConnectionProfile* TriggerManagerFrame::GetSelectedProfile() const {
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

bool TriggerManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool TriggerManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void TriggerManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_trigger = !selected_trigger_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_trigger && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_trigger && !busy);
    if (enable_button_) enable_button_->Enable(connected && native && has_trigger && !busy);
    if (disable_button_) disable_button_->Enable(connected && native && has_trigger && !busy);
}

void TriggerManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void TriggerManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void TriggerManagerFrame::RefreshTriggers() {
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
        SetMessage("Triggers are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading triggers...");

    std::string sql = "SELECT trigger_name, table_name, schema_name, event_manipulation, "
                      "action_timing, is_enabled, trigger_type FROM sb_catalog.sb_triggers "
                      "ORDER BY table_name, trigger_name";

    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                triggers_result_ = result;
                if (triggers_table_) {
                    triggers_table_->Reset(triggers_result_.columns, triggers_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load triggers." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Triggers updated");
                }
                UpdateControls();
            });
        });
}

void TriggerManagerFrame::RefreshTriggerDefinition(const std::string& trigger_name) {
    if (!connection_manager_ || trigger_name.empty()) {
        return;
    }

    std::string sql = "SELECT trigger_body FROM sb_catalog.sb_triggers WHERE trigger_name = '" +
                      EscapeSqlLiteral(trigger_name) + "'";

    pending_queries_++;
    UpdateControls();

    connection_manager_->ExecuteQueryAsync(sql,
        [this, trigger_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, trigger_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok && definition_text_) {
                    if (!result.rows.empty() && !result.rows[0].empty()) {
                        definition_text_->SetValue(result.rows[0][0].text);
                    } else {
                        definition_text_->SetValue("No definition available for trigger: " + trigger_name);
                    }
                } else if (!ok && !error.empty()) {
                    if (definition_text_) {
                        definition_text_->SetValue("Error loading definition: " + error);
                    }
                }
                UpdateControls();
            });
        });
}

void TriggerManagerFrame::RefreshTriggerTiming(const std::string& trigger_name) {
    if (!connection_manager_ || trigger_name.empty()) {
        return;
    }

    std::string sql = "SELECT trigger_name, table_name, schema_name, event_manipulation, "
                      "action_timing, action_orientation, is_enabled, trigger_type "
                      "FROM sb_catalog.sb_triggers WHERE trigger_name = '" +
                      EscapeSqlLiteral(trigger_name) + "'";

    pending_queries_++;
    UpdateControls();

    connection_manager_->ExecuteQueryAsync(sql,
        [this, trigger_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, trigger_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok && timing_text_) {
                    if (!result.rows.empty()) {
                        timing_text_->SetValue(FormatTimingDetails(result));
                    } else {
                        timing_text_->SetValue("No timing information available for trigger: " + trigger_name);
                    }
                } else if (!ok && !error.empty()) {
                    if (timing_text_) {
                        timing_text_->SetValue("Error loading timing: " + error);
                    }
                }
                UpdateControls();
            });
        });
}

void TriggerManagerFrame::RefreshTriggerDependencies(const std::string& trigger_name) {
    if (!connection_manager_ || trigger_name.empty()) {
        return;
    }

    std::string table_name = GetSelectedTableName();
    std::string schema_name;

    // Get schema name from the selected trigger row
    if (!triggers_result_.rows.empty()) {
        int row = triggers_grid_->GetGridCursorRow();
        if (row >= 0 && static_cast<size_t>(row) < triggers_result_.rows.size()) {
            schema_name = ExtractValue(triggers_result_, row, {"schema_name", "schema"});
        }
    }

    // Query dependencies: look for objects referenced by this trigger
    // This is a simplified query - in a real implementation, you might parse the trigger body
    // or have a dedicated dependencies catalog table
    std::string sql = "SELECT 'Table: " + EscapeSqlLiteral(table_name) + "' as dependency_type, "
                      "'" + EscapeSqlLiteral(table_name) + "' as object_name "
                      "UNION ALL "
                      "SELECT 'Schema' as dependency_type, '" +
                      EscapeSqlLiteral(schema_name) + "' as object_name "
                      "WHERE '" + EscapeSqlLiteral(schema_name) + "' <> ''";

    pending_queries_++;
    UpdateControls();

    connection_manager_->ExecuteQueryAsync(sql,
        [this, trigger_name, table_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, trigger_name, table_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                trigger_deps_result_ = result;
                if (ok && dependencies_text_) {
                    std::ostringstream out;
                    out << "Trigger: " << trigger_name << "\n";
                    out << "On Table: " << table_name << "\n\n";
                    out << "Dependencies:\n";
                    out << "────────────────────────────────────────────────\n";
                    if (result.rows.empty()) {
                        out << "  (No dependencies tracked for this trigger)\n";
                    } else {
                        for (const auto& row : result.rows) {
                            if (row.size() >= 2) {
                                out << "  • " << row[0].text << ": " << row[1].text << "\n";
                            }
                        }
                    }
                    dependencies_text_->SetValue(out.str());
                } else if (!ok && !error.empty()) {
                    if (dependencies_text_) {
                        dependencies_text_->SetValue("Unable to load dependencies information.\n"
                                                     "This feature requires ScratchBird catalog tables.");
                    }
                }
                UpdateControls();
            });
        });
}

void TriggerManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
    if (!connection_manager_) {
        return;
    }
    pending_queries_++;
    UpdateControls();
    UpdateStatus("Running...");
    connection_manager_->ExecuteQueryAsync(sql,
        [this, success_message](bool ok, QueryResult result, const std::string& error) {
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
                RefreshTriggers();
                if (!selected_trigger_.empty()) {
                    RefreshTriggerDefinition(selected_trigger_);
                    RefreshTriggerTiming(selected_trigger_);
                    RefreshTriggerDependencies(selected_trigger_);
                }
            });
        });
}

std::string TriggerManagerFrame::GetSelectedTriggerName() const {
    if (!triggers_grid_ || triggers_result_.rows.empty()) {
        return std::string();
    }
    int row = triggers_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= triggers_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(triggers_result_, row, {"trigger_name", "trigger", "name"});
    if (!value.empty()) {
        return value;
    }
    if (!triggers_result_.rows[row].empty()) {
        return triggers_result_.rows[row][0].text;
    }
    return std::string();
}

std::string TriggerManagerFrame::GetSelectedTableName() const {
    if (!triggers_grid_ || triggers_result_.rows.empty()) {
        return std::string();
    }
    int row = triggers_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= triggers_result_.rows.size()) {
        return std::string();
    }
    return ExtractValue(triggers_result_, row, {"table_name", "table"});
}

int TriggerManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string TriggerManagerFrame::ExtractValue(const QueryResult& result,
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

std::string TriggerManagerFrame::FormatTimingDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No timing details available.";
    }
    std::ostringstream out;
    const auto& row = result.rows[0];
    
    out << "╔══════════════════════════════════════════════════════════════╗\n";
    out << "║                    TRIGGER TIMING DETAILS                    ║\n";
    out << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Map column names to values
    auto get_value = [&](const std::string& col_name) -> std::string {
        for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
            if (ToLowerCopy(result.columns[i].name) == col_name) {
                return row[i].isNull ? "(null)" : row[i].text;
            }
        }
        return "(unknown)";
    };
    
    out << "Trigger Name:     " << get_value("trigger_name") << "\n";
    out << "Table:            " << get_value("table_name") << "\n";
    out << "Schema:           " << get_value("schema_name") << "\n\n";
    
    out << "Timing:\n";
    out << "────────────────────────────────────────────────────────────────\n";
    out << "  Action Timing:  " << get_value("action_timing") << "\n";
    out << "  Event:          " << get_value("event_manipulation") << "\n";
    out << "  Orientation:    " << get_value("action_orientation") << "\n\n";
    
    out << "Status:\n";
    out << "────────────────────────────────────────────────────────────────\n";
    std::string is_enabled = get_value("is_enabled");
    out << "  Enabled:        " << is_enabled << "\n";
    out << "  Type:           " << get_value("trigger_type") << "\n";
    
    return out.str();
}

void TriggerManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshTriggers();
}

void TriggerManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void TriggerManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshTriggers();
}

void TriggerManagerFrame::OnTriggerSelected(wxGridEvent& event) {
    selected_trigger_ = GetSelectedTriggerName();
    if (!selected_trigger_.empty()) {
        RefreshTriggerDefinition(selected_trigger_);
        RefreshTriggerTiming(selected_trigger_);
        RefreshTriggerDependencies(selected_trigger_);
    }
    UpdateControls();
    event.Skip();
}

void TriggerManagerFrame::OnCreate(wxCommandEvent&) {
    SetMessage("Create trigger: Use SQL Editor to create triggers.");
}

void TriggerManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_trigger_.empty()) {
        return;
    }
    SetMessage("Edit trigger: Use SQL Editor to modify triggers.");
}

void TriggerManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_trigger_.empty()) {
        return;
    }
    std::string table_name = GetSelectedTableName();
    std::string sql = "DROP TRIGGER " + QuoteIdentifier(selected_trigger_);
    if (!table_name.empty()) {
        sql += " ON " + QuoteIdentifier(table_name);
    }
    sql += ";";
    RunCommand(sql, "Trigger dropped");
}

void TriggerManagerFrame::OnEnable(wxCommandEvent&) {
    if (selected_trigger_.empty()) {
        return;
    }
    std::string sql = "ALTER TRIGGER " + QuoteIdentifier(selected_trigger_) + " ENABLE;";
    RunCommand(sql, "Trigger enabled");
}

void TriggerManagerFrame::OnDisable(wxCommandEvent&) {
    if (selected_trigger_.empty()) {
        return;
    }
    std::string sql = "ALTER TRIGGER " + QuoteIdentifier(selected_trigger_) + " DISABLE;";
    RunCommand(sql, "Trigger disabled");
}

void TriggerManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void TriggerManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void TriggerManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                        app_config_);
    monitor->Show(true);
}

void TriggerManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void TriggerManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void TriggerManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void TriggerManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void TriggerManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void TriggerManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void TriggerManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
