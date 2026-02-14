/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "job_scheduler_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_editor_dialog.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sql_editor_frame.h"
#include "table_designer_frame.h"
#include "users_roles_frame.h"
#include "window_manager.h"
#include "core/project.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
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

constexpr int kMenuConnect = wxID_HIGHEST + 80;
constexpr int kMenuDisconnect = wxID_HIGHEST + 81;
constexpr int kMenuRefresh = wxID_HIGHEST + 82;
constexpr int kMenuCreate = wxID_HIGHEST + 83;
constexpr int kMenuEdit = wxID_HIGHEST + 84;
constexpr int kMenuDrop = wxID_HIGHEST + 85;
constexpr int kMenuRun = wxID_HIGHEST + 86;
constexpr int kMenuCancelRun = wxID_HIGHEST + 87;
constexpr int kMenuGrant = wxID_HIGHEST + 88;
constexpr int kMenuRevoke = wxID_HIGHEST + 89;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 90;

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

wxBEGIN_EVENT_TABLE(JobSchedulerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, JobSchedulerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, JobSchedulerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, JobSchedulerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, JobSchedulerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, JobSchedulerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, JobSchedulerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, JobSchedulerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, JobSchedulerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, JobSchedulerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, JobSchedulerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, JobSchedulerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, JobSchedulerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, JobSchedulerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, JobSchedulerFrame::OnDrop)
    EVT_BUTTON(kMenuRun, JobSchedulerFrame::OnExecute)
    EVT_BUTTON(kMenuCancelRun, JobSchedulerFrame::OnCancelRun)
    EVT_BUTTON(kMenuGrant, JobSchedulerFrame::OnGrant)
    EVT_BUTTON(kMenuRevoke, JobSchedulerFrame::OnRevoke)
    EVT_CLOSE(JobSchedulerFrame::OnClose)
wxEND_EVENT_TABLE()

JobSchedulerFrame::JobSchedulerFrame(WindowManager* windowManager,
                                     ConnectionManager* connectionManager,
                                     const std::vector<ConnectionProfile>* connections,
                                     const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Job Scheduler", wxDefaultPosition, wxSize(1100, 720)),
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

void JobSchedulerFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void JobSchedulerFrame::BuildLayout() {
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
    run_button_ = new wxButton(actionPanel, kMenuRun, "Run");
    cancel_button_ = new wxButton(actionPanel, kMenuCancelRun, "Cancel Run");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionSizer->Add(run_button_, 0, wxRIGHT, 6);
    actionSizer->Add(cancel_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* jobsPanel = new wxPanel(splitter, wxID_ANY);
    auto* jobsSizer = new wxBoxSizer(wxVERTICAL);
    jobsSizer->Add(new wxStaticText(jobsPanel, wxID_ANY, "Jobs"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    jobs_grid_ = new wxGrid(jobsPanel, wxID_ANY);
    jobs_grid_->EnableEditing(false);
    jobs_grid_->SetRowLabelSize(40);
    jobs_table_ = new ResultGridTable();
    jobs_grid_->SetTable(jobs_table_, true);
    jobsSizer->Add(jobs_grid_, 1, wxEXPAND | wxALL, 8);
    jobsPanel->SetSizer(jobsSizer);

    auto* detailsPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailsSizer = new wxBoxSizer(wxVERTICAL);
    auto* notebook = new wxNotebook(detailsPanel, wxID_ANY);

    auto* detailTab = new wxPanel(notebook, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    details_text_ = new wxTextCtrl(detailTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    detailSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    detailTab->SetSizer(detailSizer);

    auto* runsTab = new wxPanel(notebook, wxID_ANY);
    auto* runsSizer = new wxBoxSizer(wxVERTICAL);
    runs_grid_ = new wxGrid(runsTab, wxID_ANY);
    runs_grid_->EnableEditing(false);
    runs_grid_->SetRowLabelSize(40);
    runs_table_ = new ResultGridTable();
    runs_grid_->SetTable(runs_table_, true);
    runsSizer->Add(runs_grid_, 1, wxEXPAND | wxALL, 8);
    runsTab->SetSizer(runsSizer);

    auto* grantsTab = new wxPanel(notebook, wxID_ANY);
    auto* grantsSizer = new wxBoxSizer(wxVERTICAL);
    auto* grantBar = new wxBoxSizer(wxHORIZONTAL);
    grantBar->Add(new wxStaticText(grantsTab, wxID_ANY, "Principal:"), 0,
                  wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    grant_principal_ctrl_ = new wxTextCtrl(grantsTab, wxID_ANY);
    grantBar->Add(grant_principal_ctrl_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    grant_button_ = new wxButton(grantsTab, kMenuGrant, "Grant EXECUTE");
    revoke_button_ = new wxButton(grantsTab, kMenuRevoke, "Revoke EXECUTE");
    grantBar->Add(grant_button_, 0, wxRIGHT, 6);
    grantBar->Add(revoke_button_, 0, wxRIGHT, 6);
    grantsSizer->Add(grantBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    grants_grid_ = new wxGrid(grantsTab, wxID_ANY);
    grants_grid_->EnableEditing(false);
    grants_grid_->SetRowLabelSize(40);
    grants_table_ = new ResultGridTable();
    grants_grid_->SetTable(grants_table_, true);
    grantsSizer->Add(grants_grid_, 1, wxEXPAND | wxALL, 8);
    grantsTab->SetSizer(grantsSizer);

    // Dependencies tab
    auto* depsTab = new wxPanel(notebook, wxID_ANY);
    auto* depsSizer = new wxBoxSizer(wxVERTICAL);
    deps_text_ = new wxTextCtrl(depsTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    depsSizer->Add(deps_text_, 1, wxEXPAND | wxALL, 8);
    depsTab->SetSizer(depsSizer);

    // Scheduler Configuration tab
    auto* configTab = new wxPanel(notebook, wxID_ANY);
    auto* configSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* configTopBar = new wxBoxSizer(wxHORIZONTAL);
    config_refresh_btn_ = new wxButton(configTab, wxID_ANY, "Refresh Config");
    config_save_btn_ = new wxButton(configTab, wxID_ANY, "Save Config");
    configTopBar->Add(config_refresh_btn_, 0, wxRIGHT, 6);
    configTopBar->Add(config_save_btn_, 0, wxRIGHT, 6);
    configSizer->Add(configTopBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    
    // Config form - using vertical sizer with horizontal rows
    auto* configForm = new wxBoxSizer(wxVERTICAL);
    
    auto* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(new wxStaticText(configTab, wxID_ANY, "Enabled:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    config_enabled_chk_ = new wxCheckBox(configTab, wxID_ANY, "");
    row1->Add(config_enabled_chk_, 0);
    configForm->Add(row1, 0, wxEXPAND | wxBOTTOM, 4);
    
    auto* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(new wxStaticText(configTab, wxID_ANY, "Max Concurrent Jobs:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    config_max_concurrent_ctrl_ = new wxTextCtrl(configTab, wxID_ANY, "", wxDefaultPosition, wxSize(80, -1));
    row2->Add(config_max_concurrent_ctrl_, 0);
    configForm->Add(row2, 0, wxEXPAND | wxBOTTOM, 4);
    
    auto* row3 = new wxBoxSizer(wxHORIZONTAL);
    row3->Add(new wxStaticText(configTab, wxID_ANY, "Poll Interval (seconds):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    config_poll_interval_ctrl_ = new wxTextCtrl(configTab, wxID_ANY, "", wxDefaultPosition, wxSize(80, -1));
    row3->Add(config_poll_interval_ctrl_, 0);
    configForm->Add(row3, 0, wxEXPAND | wxBOTTOM, 4);
    
    auto* row4 = new wxBoxSizer(wxHORIZONTAL);
    row4->Add(new wxStaticText(configTab, wxID_ANY, "Default Timezone:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    config_timezone_choice_ = new wxChoice(configTab, wxID_ANY);
    config_timezone_choice_->Append("UTC");
    config_timezone_choice_->Append("Local");
    config_timezone_choice_->Append("America/New_York");
    config_timezone_choice_->Append("America/Chicago");
    config_timezone_choice_->Append("America/Denver");
    config_timezone_choice_->Append("America/Los_Angeles");
    config_timezone_choice_->Append("Europe/London");
    config_timezone_choice_->Append("Europe/Paris");
    config_timezone_choice_->Append("Europe/Berlin");
    config_timezone_choice_->Append("Asia/Tokyo");
    config_timezone_choice_->Append("Asia/Shanghai");
    config_timezone_choice_->Append("Australia/Sydney");
    config_timezone_choice_->SetSelection(0);
    row4->Add(config_timezone_choice_, 1);
    configForm->Add(row4, 0, wxEXPAND | wxBOTTOM, 4);
    
    configSizer->Add(configForm, 0, wxEXPAND | wxALL, 8);
    
    // Config text view (raw view of configuration)
    config_text_ctrl_ = new wxTextCtrl(configTab, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    configSizer->Add(new wxStaticText(configTab, wxID_ANY, "Raw Configuration:"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    configSizer->Add(config_text_ctrl_, 1, wxEXPAND | wxALL, 8);
    
    configTab->SetSizer(configSizer);

    notebook->AddPage(detailTab, "Details");
    notebook->AddPage(runsTab, "Runs");
    notebook->AddPage(grantsTab, "Privileges");
    notebook->AddPage(depsTab, "Dependencies");
    notebook->AddPage(configTab, "Configuration");

    detailsSizer->Add(notebook, 1, wxEXPAND);
    detailsPanel->SetSizer(detailsSizer);

    splitter->SplitVertically(jobsPanel, detailsPanel, 420);
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

    jobs_grid_->Bind(wxEVT_GRID_SELECT_CELL, &JobSchedulerFrame::OnJobSelected, this);
    runs_grid_->Bind(wxEVT_GRID_SELECT_CELL, &JobSchedulerFrame::OnRunSelected, this);
    
    // Config tab bindings
    config_refresh_btn_->Bind(wxEVT_BUTTON, &JobSchedulerFrame::OnConfigRefresh, this);
    config_save_btn_->Bind(wxEVT_BUTTON, &JobSchedulerFrame::OnConfigSave, this);
    config_enabled_chk_->Bind(wxEVT_CHECKBOX, &JobSchedulerFrame::OnConfigEnable, this);
}

void JobSchedulerFrame::PopulateConnections() {
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

const ConnectionProfile* JobSchedulerFrame::GetSelectedProfile() const {
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

bool JobSchedulerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool JobSchedulerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void JobSchedulerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_job = !selected_job_.empty();
    bool has_run = !GetSelectedRunId().empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_job && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_job && !busy);
    if (run_button_) run_button_->Enable(connected && native && has_job && !busy);
    if (cancel_button_) cancel_button_->Enable(connected && native && has_run && !busy);
    if (grant_button_) grant_button_->Enable(connected && native && has_job && !busy);
    if (revoke_button_) revoke_button_->Enable(connected && native && has_job && !busy);
}

void JobSchedulerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void JobSchedulerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void JobSchedulerFrame::RefreshJobs() {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_->LastError());
        return;
    }
    if (!IsNativeProfile(*profile)) {
        SetMessage("Job scheduler is available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading jobs...");
    connection_manager_->ExecuteQueryAsync("SHOW JOBS",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                jobs_result_ = result;
                if (jobs_table_) {
                    jobs_table_->Reset(jobs_result_.columns, jobs_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load jobs." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Jobs updated");
                }
                UpdateControls();
            });
        });
}

void JobSchedulerFrame::RefreshJobDetails(const std::string& job_name) {
    if (!connection_manager_ || job_name.empty()) {
        return;
    }
    std::string sql = "SHOW JOB '" + EscapeSqlLiteral(job_name) + "'";
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(sql,
        [this, job_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, job_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                job_details_result_ = result;
                if (ok) {
                    if (details_text_) {
                        details_text_->SetValue(FormatJobDetails(job_details_result_));
                    }
                    // Refresh dependencies visualization
                    RefreshJobDependencies(job_name);
                } else if (!error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void JobSchedulerFrame::RefreshJobRuns(const std::string& job_name) {
    if (!connection_manager_ || job_name.empty()) {
        return;
    }
    std::string sql = "SHOW JOB RUNS FOR '" + EscapeSqlLiteral(job_name) + "'";
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                runs_result_ = result;
                if (runs_table_) {
                    runs_table_->Reset(runs_result_.columns, runs_result_.rows);
                }
                if (!ok && !error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void JobSchedulerFrame::RefreshJobGrants(const std::string& job_name) {
    if (!connection_manager_ || job_name.empty()) {
        return;
    }
    std::string sql = "SHOW GRANTS FOR " + QuoteIdentifier(job_name);
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                grants_result_ = result;
                if (grants_table_) {
                    grants_table_->Reset(grants_result_.columns, grants_result_.rows);
                }
                if (!ok && !error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void JobSchedulerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshJobs();
                if (!selected_job_.empty()) {
                    RefreshJobRuns(selected_job_);
                    RefreshJobDetails(selected_job_);
                    RefreshJobGrants(selected_job_);
                }
            });
        });
}

std::string JobSchedulerFrame::GetSelectedJobName() const {
    if (!jobs_grid_ || jobs_result_.rows.empty()) {
        return std::string();
    }
    int row = jobs_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= jobs_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(jobs_result_, row, {"job_name", "job name", "name", "job"});
    if (!value.empty()) {
        return value;
    }
    if (!jobs_result_.rows[row].empty()) {
        return jobs_result_.rows[row][0].text;
    }
    return std::string();
}

std::string JobSchedulerFrame::GetSelectedRunId() const {
    if (!runs_grid_ || runs_result_.rows.empty()) {
        return std::string();
    }
    int row = runs_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= runs_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(
        runs_result_, row,
        {"job_run_id", "job run id", "run_id", "run_uuid", "job_run_uuid"});
    if (!value.empty()) {
        return value;
    }
    if (!runs_result_.rows[row].empty()) {
        return runs_result_.rows[row][0].text;
    }
    return std::string();
}

int JobSchedulerFrame::FindColumnIndex(const QueryResult& result,
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

std::string JobSchedulerFrame::ExtractValue(const QueryResult& result,
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

std::string JobSchedulerFrame::FormatJobDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No job details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void JobSchedulerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshJobs();
}

void JobSchedulerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void JobSchedulerFrame::OnRefresh(wxCommandEvent&) {
    RefreshJobs();
}

void JobSchedulerFrame::OnJobSelected(wxGridEvent& event) {
    selected_job_ = GetSelectedJobName();
    if (!selected_job_.empty()) {
        RefreshJobDetails(selected_job_);
        RefreshJobRuns(selected_job_);
        RefreshJobGrants(selected_job_);
    }
    UpdateControls();
    event.Skip();
}

void JobSchedulerFrame::OnRunSelected(wxGridEvent& event) {
    UpdateControls();
    event.Skip();
}

void JobSchedulerFrame::OnCreate(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "schedule";
        context.role = "operator";
        std::string reason;
        if (!project->ScheduleReportingAction("schedule", "job:create", context, &reason)) {
            SetMessage("Governance blocked schedule create: " + reason);
            return;
        }
    }
    JobEditorDialog dialog(this, JobEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create job statement is empty.");
        return;
    }
    RunCommand(sql, "Job created");
}

void JobSchedulerFrame::OnEdit(wxCommandEvent&) {
    if (selected_job_.empty()) {
        return;
    }
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "schedule";
        context.role = "operator";
        std::string reason;
        if (!project->ScheduleReportingAction("schedule", selected_job_, context, &reason)) {
            SetMessage("Governance blocked schedule edit: " + reason);
            return;
        }
    }
    JobEditorDialog dialog(this, JobEditorMode::Edit);
    dialog.SetJobName(selected_job_);
    if (!job_details_result_.rows.empty()) {
        dialog.SetDescription(ExtractValue(job_details_result_, 0, {"description"}));
        dialog.SetState(ExtractValue(job_details_result_, 0, {"state"}));
        dialog.SetRunAs(ExtractValue(job_details_result_, 0, {"run_as_role", "run_as_role_name"}));
        dialog.SetTimeoutSeconds(ExtractValue(job_details_result_, 0, {"timeout_seconds"}));
        dialog.SetMaxRetries(ExtractValue(job_details_result_, 0, {"max_retries"}));
        dialog.SetRetryBackoffSeconds(ExtractValue(job_details_result_, 0, {"retry_backoff_seconds"}));
        dialog.SetOnCompletion(ExtractValue(job_details_result_, 0, {"on_completion", "on completion"}));
        dialog.SetDependsOn(ExtractValue(job_details_result_, 0, {"depends_on", "depends on"}));
        dialog.SetJobClass(ExtractValue(job_details_result_, 0, {"job_class", "class"}));
        dialog.SetPartition(
            ExtractValue(job_details_result_, 0, {"partition_strategy", "partition"}),
            ExtractValue(job_details_result_, 0, {"partition_expression", "partition value"}));

        std::string job_type = ExtractValue(job_details_result_, 0, {"job_type", "job type"});
        dialog.SetJobType(job_type);
        std::string job_body;
        if (job_type == "PROCEDURE") {
            job_body = ExtractValue(job_details_result_, 0, {"procedure_name", "procedure"});
        } else if (job_type == "EXTERNAL") {
            job_body = ExtractValue(job_details_result_, 0, {"external_command", "external command"});
        } else {
            job_body = ExtractValue(job_details_result_, 0, {"job_sql", "job sql"});
        }
        if (!job_body.empty()) {
            dialog.SetJobBody(job_body);
        }

        std::string schedule_kind = ExtractValue(
            job_details_result_, 0,
            {"schedule_kind", "schedule"});
        dialog.SetScheduleKind(schedule_kind);
        if (schedule_kind == "CRON") {
            dialog.SetScheduleValue(ExtractValue(job_details_result_, 0, {"cron_expression", "cron"}));
        } else if (schedule_kind == "AT") {
            dialog.SetScheduleValue(ExtractValue(job_details_result_, 0, {"at_timestamp", "starts_at", "starts at"}));
        } else if (schedule_kind == "EVERY") {
            dialog.SetScheduleValue(ExtractValue(job_details_result_, 0, {"interval_seconds", "interval seconds"}));
            dialog.SetScheduleStarts(ExtractValue(job_details_result_, 0, {"starts_at", "starts at"}));
            dialog.SetScheduleEnds(ExtractValue(job_details_result_, 0, {"ends_at", "ends at"}));
        }
    }
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Alter job statement is empty.");
        return;
    }
    RunCommand(sql, "Job updated");
}

void JobSchedulerFrame::OnDrop(wxCommandEvent&) {
    if (selected_job_.empty()) {
        return;
    }
    wxArrayString choices;
    choices.Add("Drop (keep history)");
    choices.Add("Drop (delete history)");
    wxSingleChoiceDialog dialog(this, "Drop job history option", "Drop Job", choices);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    bool keep_history = dialog.GetSelection() == 0;
    std::string sql = "DROP JOB " + QuoteIdentifier(selected_job_);
    if (keep_history) {
        sql += " KEEP HISTORY";
    }
    sql += ";";
    RunCommand(sql, "Job dropped");
}

void JobSchedulerFrame::OnExecute(wxCommandEvent&) {
    if (selected_job_.empty()) {
        return;
    }
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "run";
        context.role = "operator";
        std::string reason;
        if (!project->ScheduleReportingAction("execute", selected_job_, context, &reason)) {
            SetMessage("Governance blocked job execution: " + reason);
            return;
        }
    }
    std::string sql = "EXECUTE JOB " + QuoteIdentifier(selected_job_) + ";";
    RunCommand(sql, "Job executed");
}

void JobSchedulerFrame::OnCancelRun(wxCommandEvent&) {
    std::string run_id = GetSelectedRunId();
    if (run_id.empty()) {
        return;
    }
    std::string sql = "CANCEL JOB RUN '" + EscapeSqlLiteral(run_id) + "';";
    RunCommand(sql, "Run cancelled");
}

void JobSchedulerFrame::OnGrant(wxCommandEvent&) {
    if (selected_job_.empty()) {
        return;
    }
    std::string principal = Trim(grant_principal_ctrl_ ? grant_principal_ctrl_->GetValue().ToStdString() : "");
    if (principal.empty()) {
        SetMessage("Enter a principal to grant EXECUTE.");
        return;
    }
    std::string principal_sql = ToLowerCopy(principal) == "public" ? "PUBLIC" : QuoteIdentifier(principal);
    std::string sql = "GRANT EXECUTE ON JOB " + QuoteIdentifier(selected_job_) + " TO " + principal_sql + ";";
    RunCommand(sql, "Grant applied");
}

void JobSchedulerFrame::OnRevoke(wxCommandEvent&) {
    if (selected_job_.empty()) {
        return;
    }
    std::string principal = Trim(grant_principal_ctrl_ ? grant_principal_ctrl_->GetValue().ToStdString() : "");
    if (principal.empty()) {
        SetMessage("Enter a principal to revoke EXECUTE.");
        return;
    }
    std::string principal_sql = ToLowerCopy(principal) == "public" ? "PUBLIC" : QuoteIdentifier(principal);
    std::string sql = "REVOKE EXECUTE ON JOB " + QuoteIdentifier(selected_job_) + " FROM " + principal_sql + ";";
    RunCommand(sql, "Grant revoked");
}

void JobSchedulerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void JobSchedulerFrame::RefreshJobDependencies(const std::string& job_name) {
    if (!connection_manager_ || job_name.empty()) {
        return;
    }
    
    // Query job prerequisites (jobs this job depends on)
    std::string prereq_sql = "SELECT prerequisite_job_name FROM sb_catalog.sb_job_prerequisites\n"
                            "WHERE job_name = '" + EscapeSqlLiteral(job_name) + "'\n"
                            "ORDER BY prerequisite_job_name;";
    
    // Query job dependents (jobs that depend on this job)
    std::string dep_sql = "SELECT job_name FROM sb_catalog.sb_job_prerequisites\n"
                         "WHERE prerequisite_job_name = '" + EscapeSqlLiteral(job_name) + "'\n"
                         "ORDER BY job_name;";
    
    pending_queries_++;
    UpdateControls();
    
    // First query: prerequisites
    connection_manager_->ExecuteQueryAsync(prereq_sql,
        [this, job_name, dep_sql](bool ok1, QueryResult prereq_result, const std::string& error1) {
            CallAfter([this, ok1, prereq_result = std::move(prereq_result), error1, job_name, dep_sql]() mutable {
                if (!ok1) {
                    // Catalog table may not exist - show friendly message
                    if (deps_text_) {
                        deps_text_->SetValue("Job dependencies information is not available.\n"
                                            "This feature requires ScratchBird catalog tables.");
                    }
                    pending_queries_ = std::max(0, pending_queries_ - 1);
                    UpdateControls();
                    return;
                }
                
                // Second query: dependents
                connection_manager_->ExecuteQueryAsync(dep_sql,
                    [this, prereq_result](bool ok2, QueryResult dep_result, const std::string& error2) mutable {
                        CallAfter([this, ok2, prereq_result = std::move(prereq_result), 
                                   dep_result = std::move(dep_result), error2]() mutable {
                            pending_queries_ = std::max(0, pending_queries_ - 1);
                            if (ok2 && deps_text_) {
                                deps_text_->SetValue(BuildDependenciesText(prereq_result, dep_result));
                            } else if (deps_text_) {
                                deps_text_->SetValue("Unable to load job dependencies.");
                            }
                            UpdateControls();
                        });
                    });
            });
        });
}

std::string JobSchedulerFrame::BuildDependenciesText(const QueryResult& prerequisites,
                                                      const QueryResult& dependents) {
    std::string text;
    
    // Header
    text += "╔══════════════════════════════════════════════════════════════╗\n";
    text += "║                    JOB DEPENDENCIES                          ║\n";
    text += "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Prerequisites section (jobs this job depends on)
    text += "📋 PREREQUISITES (This job waits for):\n";
    text += "────────────────────────────────────────────────────────────────\n";
    if (prerequisites.rows.empty()) {
        text += "   (none - this job has no prerequisites)\n";
    } else {
        for (const auto& row : prerequisites.rows) {
            if (!row.empty() && !row[0].isNull) {
                text += "   ↳ " + row[0].text + "\n";
            }
        }
    }
    text += "\n";
    
    // Dependents section (jobs that depend on this job)
    text += "📎 DEPENDENTS (Jobs waiting for this):\n";
    text += "────────────────────────────────────────────────────────────────\n";
    if (dependents.rows.empty()) {
        text += "   (none - no jobs depend on this job)\n";
    } else {
        for (const auto& row : dependents.rows) {
            if (!row.empty() && !row[0].isNull) {
                text += "   ↱ " + row[0].text + "\n";
            }
        }
    }
    text += "\n";
    
    // Summary
    size_t prereq_count = prerequisites.rows.size();
    size_t dep_count = dependents.rows.size();
    text += "────────────────────────────────────────────────────────────────\n";
    text += "Summary: " + std::to_string(prereq_count) + " prerequisite" + 
            (prereq_count == 1 ? "" : "s") + ", " +
            std::to_string(dep_count) + " dependent" + (dep_count == 1 ? "" : "s") + "\n";
    
    return text;
}

void JobSchedulerFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_, app_config_, nullptr);
    editor->Show(true);
}

void JobSchedulerFrame::OnNewDiagram(wxCommandEvent&) {
    if (auto* host = dynamic_cast<DiagramFrame*>(window_manager_->GetDiagramHost())) {
        host->AddDiagramTab();
        host->Raise();
        return;
    }
    auto* diagram = new DiagramFrame(window_manager_, app_config_);
    diagram->Show(true);
}

void JobSchedulerFrame::OnOpenMonitoring(wxCommandEvent&) {
    auto* monitoring = new MonitoringFrame(window_manager_, connection_manager_, connections_, app_config_);
    monitoring->Show(true);
}

void JobSchedulerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_, app_config_);
    users->Show(true);
}

void JobSchedulerFrame::OnOpenDomainManager(wxCommandEvent&) {
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    domains->Show(true);
}

void JobSchedulerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    schemas->Show(true);
}

void JobSchedulerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    tables->Show(true);
}

void JobSchedulerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    indexes->Show(true);
}

void JobSchedulerFrame::RefreshSchedulerConfig() {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile || !IsNativeProfile(*profile)) {
        if (config_text_ctrl_) {
            config_text_ctrl_->SetValue("Scheduler configuration is only available for ScratchBird connections.");
        }
        return;
    }
    
    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading scheduler config...");
    
    // Query scheduler configuration from system catalog
    std::string sql = "SELECT config_key, config_value FROM sb_catalog.sb_scheduler_config ORDER BY config_key;";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                if (ok) {
                    // Parse config values and update UI
                    std::string raw_text;
                    bool enabled = false;
                    int max_concurrent = 4;
                    int poll_interval = 30;
                    std::string timezone = "UTC";
                    
                    for (const auto& row : result.rows) {
                        if (row.size() >= 2) {
                            std::string key = row[0].isNull ? "" : row[0].text;
                            std::string value = row[1].isNull ? "" : row[1].text;
                            raw_text += key + " = " + value + "\n";
                            
                            if (key == "enabled") {
                                enabled = (value == "true" || value == "1" || value == "yes");
                            } else if (key == "max_concurrent_jobs") {
                                max_concurrent = std::stoi(value);
                            } else if (key == "poll_interval_seconds") {
                                poll_interval = std::stoi(value);
                            } else if (key == "default_timezone") {
                                timezone = value;
                            }
                        }
                    }
                    
                    // Update UI controls
                    if (config_enabled_chk_) {
                        config_enabled_chk_->SetValue(enabled);
                    }
                    if (config_max_concurrent_ctrl_) {
                        config_max_concurrent_ctrl_->SetValue(std::to_string(max_concurrent));
                    }
                    if (config_poll_interval_ctrl_) {
                        config_poll_interval_ctrl_->SetValue(std::to_string(poll_interval));
                    }
                    if (config_timezone_choice_) {
                        int tz_sel = config_timezone_choice_->FindString(timezone);
                        if (tz_sel != wxNOT_FOUND) {
                            config_timezone_choice_->SetSelection(tz_sel);
                        }
                    }
                    if (config_text_ctrl_) {
                        if (raw_text.empty()) {
                            config_text_ctrl_->SetValue("No scheduler configuration found.");
                        } else {
                            config_text_ctrl_->SetValue(raw_text);
                        }
                    }
                    UpdateStatus("Config loaded");
                } else {
                    if (config_text_ctrl_) {
                        config_text_ctrl_->SetValue("Failed to load scheduler configuration:\n" + 
                                                    (error.empty() ? "Unknown error" : error));
                    }
                    UpdateStatus("Config load failed");
                }
                UpdateControls();
            });
        });
}

void JobSchedulerFrame::SaveSchedulerConfig() {
    if (!connection_manager_) {
        return;
    }
    const auto* profile = GetSelectedProfile();
    if (!profile || !IsNativeProfile(*profile)) {
        SetMessage("Scheduler configuration is only available for ScratchBird connections.");
        return;
    }
    
    // Build SQL to update config
    bool enabled = config_enabled_chk_ ? config_enabled_chk_->GetValue() : false;
    std::string max_concurrent = config_max_concurrent_ctrl_ ? 
                                 config_max_concurrent_ctrl_->GetValue().ToStdString() : "4";
    std::string poll_interval = config_poll_interval_ctrl_ ? 
                                config_poll_interval_ctrl_->GetValue().ToStdString() : "30";
    std::string timezone = config_timezone_choice_ ? 
                          config_timezone_choice_->GetStringSelection().ToStdString() : "UTC";
    
    std::string sql = "UPDATE sb_catalog.sb_scheduler_config SET config_value = CASE config_key "
                     "WHEN 'enabled' THEN '" + std::string(enabled ? "true" : "false") + "' "
                     "WHEN 'max_concurrent_jobs' THEN '" + EscapeSqlLiteral(max_concurrent) + "' "
                     "WHEN 'poll_interval_seconds' THEN '" + EscapeSqlLiteral(poll_interval) + "' "
                     "WHEN 'default_timezone' THEN '" + EscapeSqlLiteral(timezone) + "' "
                     "ELSE config_value END "
                     "WHERE config_key IN ('enabled', 'max_concurrent_jobs', 'poll_interval_seconds', 'default_timezone');";
    
    RunCommand(sql, "Configuration saved");
}

void JobSchedulerFrame::OnConfigRefresh(wxCommandEvent&) {
    RefreshSchedulerConfig();
}

void JobSchedulerFrame::OnConfigSave(wxCommandEvent&) {
    SaveSchedulerConfig();
}

void JobSchedulerFrame::OnConfigEnable(wxCommandEvent&) {
    // Auto-save when enable checkbox is toggled (optional UX improvement)
    // For now, just mark that user needs to click Save
    if (config_text_ctrl_) {
        config_text_ctrl_->SetValue(config_text_ctrl_->GetValue() + 
                                    "\n[Note: Click 'Save Config' to apply changes]");
    }
}

} // namespace scratchrobin
