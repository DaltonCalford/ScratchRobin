/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "audit_policy_frame.h"

#include "audit_policy_editor_dialog.h"
#include "audit_retention_policy_dialog.h"
#include "core/config.h"
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
#include <wx/bookctrl.h>
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

constexpr int kMenuConnect = wxID_HIGHEST + 260;
constexpr int kMenuDisconnect = wxID_HIGHEST + 261;
constexpr int kMenuRefresh = wxID_HIGHEST + 262;
constexpr int kMenuCreate = wxID_HIGHEST + 263;
constexpr int kMenuEdit = wxID_HIGHEST + 264;
constexpr int kMenuDrop = wxID_HIGHEST + 265;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 266;

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

wxBEGIN_EVENT_TABLE(AuditPolicyFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, AuditPolicyFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, AuditPolicyFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, AuditPolicyFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, AuditPolicyFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, AuditPolicyFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, AuditPolicyFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, AuditPolicyFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, AuditPolicyFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, AuditPolicyFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, AuditPolicyFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, AuditPolicyFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, AuditPolicyFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, AuditPolicyFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, AuditPolicyFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, AuditPolicyFrame::OnDrop)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, AuditPolicyFrame::OnTabChanged)
    EVT_CLOSE(AuditPolicyFrame::OnClose)
wxEND_EVENT_TABLE()

AuditPolicyFrame::AuditPolicyFrame(WindowManager* windowManager,
                                   ConnectionManager* connectionManager,
                                   const std::vector<ConnectionProfile>* connections,
                                   const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Audit Policies", wxDefaultPosition, wxSize(1100, 720)),
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

AuditPolicyFrame::~AuditPolicyFrame() {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
}

void AuditPolicyFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void AuditPolicyFrame::BuildLayout() {
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
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    notebook_ = new wxNotebook(this, wxID_ANY);

    auto* policyPanel = new wxPanel(notebook_, wxID_ANY);
    auto* policySizer = new wxBoxSizer(wxVERTICAL);
    audit_grid_ = new wxGrid(policyPanel, wxID_ANY);
    audit_grid_->EnableEditing(false);
    audit_grid_->SetRowLabelSize(40);
    audit_table_ = new ResultGridTable();
    audit_grid_->SetTable(audit_table_, true);
    audit_grid_->Bind(wxEVT_GRID_SELECT_CELL, &AuditPolicyFrame::OnPolicySelected, this);
    policySizer->Add(audit_grid_, 1, wxEXPAND | wxALL, 8);
    policySizer->Add(new wxStaticText(policyPanel, wxID_ANY, "Policy Details"), 0,
                     wxLEFT | wxRIGHT, 8);
    audit_details_ = new wxTextCtrl(policyPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY);
    policySizer->Add(audit_details_, 0, wxEXPAND | wxALL, 8);
    policyPanel->SetSizer(policySizer);

    auto* retentionPanel = new wxPanel(notebook_, wxID_ANY);
    auto* retentionSizer = new wxBoxSizer(wxVERTICAL);
    retention_grid_ = new wxGrid(retentionPanel, wxID_ANY);
    retention_grid_->EnableEditing(false);
    retention_grid_->SetRowLabelSize(40);
    retention_table_ = new ResultGridTable();
    retention_grid_->SetTable(retention_table_, true);
    retention_grid_->Bind(wxEVT_GRID_SELECT_CELL, &AuditPolicyFrame::OnRetentionSelected, this);
    retentionSizer->Add(retention_grid_, 1, wxEXPAND | wxALL, 8);
    retentionSizer->Add(new wxStaticText(retentionPanel, wxID_ANY, "Retention Details"), 0,
                        wxLEFT | wxRIGHT, 8);
    retention_details_ = new wxTextCtrl(retentionPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY);
    retentionSizer->Add(retention_details_, 0, wxEXPAND | wxALL, 8);
    retentionPanel->SetSizer(retentionSizer);

    notebook_->AddPage(policyPanel, "Audit Policies");
    notebook_->AddPage(retentionPanel, "Retention Policies");
    rootSizer->Add(notebook_, 1, wxEXPAND);

    auto* statusPanel = new wxPanel(this, wxID_ANY);
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    status_label_ = new wxStaticText(statusPanel, wxID_ANY, "Status: Idle");
    message_label_ = new wxStaticText(statusPanel, wxID_ANY, "");
    statusSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    statusSizer->Add(message_label_, 1, wxALIGN_CENTER_VERTICAL);
    statusPanel->SetSizer(statusSizer);
    rootSizer->Add(statusPanel, 0, wxEXPAND | wxALL, 6);

    SetSizer(rootSizer);
}

void AuditPolicyFrame::PopulateConnections() {
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

void AuditPolicyFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool has_selection = false;
    if (notebook_ && notebook_->GetSelection() == kTabRetention) {
        has_selection = !GetSelectedRetentionPolicyId().empty();
    } else {
        has_selection = !GetSelectedAuditPolicyId().empty();
    }
    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected);
    if (create_button_) create_button_->Enable(connected);
    if (edit_button_) edit_button_->Enable(connected && has_selection);
    if (drop_button_) drop_button_->Enable(connected && has_selection);
}

void AuditPolicyFrame::UpdateStatus(const wxString& status) {
    if (status_label_) {
        status_label_->SetLabel("Status: " + status);
    }
}

void AuditPolicyFrame::SetMessage(const std::string& message) {
    if (message_label_) {
        message_label_->SetLabel(message);
    }
}

const ConnectionProfile* AuditPolicyFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int index = connection_choice_->GetSelection();
    if (index < 0 || static_cast<size_t>(index) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(index)];
}

bool AuditPolicyFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    if (connection_manager_->IsConnected()) {
        return true;
    }
    return connection_manager_->Connect(profile);
}

void AuditPolicyFrame::RefreshPolicies() {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    if (NormalizeBackendName(profile->backend) != "native") {
        SetMessage("Audit policies are currently supported for ScratchBird connections.");
        return;
    }
    UpdateStatus("Loading audit policies...");

    std::string sql =
        "SELECT policy_uuid, scope_type, scope_uuid, category, event_code, min_severity, "
        "audit_select, audit_insert, audit_update, audit_delete, audit_condition, is_enabled, created_at "
        "FROM sys.audit_policies "
        "ORDER BY scope_type, category, event_code";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load audit policies." : error);
                    UpdateStatus("Load failed");
                    return;
                }
                audit_result_ = std::move(result);
                if (audit_table_) {
                    audit_table_->Reset(audit_result_.columns, audit_result_.rows);
                }
                UpdateControls();
                UpdateStatus("Audit policies updated");
            });
        });
}

void AuditPolicyFrame::RefreshRetentionPolicies() {
    const auto* profile = GetSelectedProfile();
    if (!profile) {
        SetMessage("Select a connection profile first.");
        return;
    }
    if (!EnsureConnected(*profile)) {
        SetMessage(connection_manager_ ? connection_manager_->LastError() : "Connection failed.");
        return;
    }
    if (NormalizeBackendName(profile->backend) != "native") {
        SetMessage("Audit retention policies are currently supported for ScratchBird connections.");
        return;
    }
    UpdateStatus("Loading retention policies...");

    std::string sql =
        "SELECT policy_id, category, severity_min, retention_period, archive_after, delete_after, "
        "storage_class, is_active "
        "FROM sys.audit_retention_policy "
        "ORDER BY category, severity_min";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load retention policies." : error);
                    UpdateStatus("Load failed");
                    return;
                }
                retention_result_ = std::move(result);
                if (retention_table_) {
                    retention_table_->Reset(retention_result_.columns, retention_result_.rows);
                }
                UpdateControls();
                UpdateStatus("Retention policies updated");
            });
        });
}

void AuditPolicyFrame::RefreshActiveTab() {
    if (notebook_ && notebook_->GetSelection() == kTabRetention) {
        RefreshRetentionPolicies();
    } else {
        RefreshPolicies();
    }
}

std::string AuditPolicyFrame::GetSelectedAuditPolicyId() const {
    if (!audit_grid_ || audit_result_.rows.empty()) {
        return std::string();
    }
    int row = audit_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= audit_result_.rows.size()) {
        return std::string();
    }
    return ExtractValue(audit_result_, row, {"policy_uuid", "policy_id", "id"});
}

std::string AuditPolicyFrame::GetSelectedRetentionPolicyId() const {
    if (!retention_grid_ || retention_result_.rows.empty()) {
        return std::string();
    }
    int row = retention_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= retention_result_.rows.size()) {
        return std::string();
    }
    return ExtractValue(retention_result_, row, {"policy_id", "id"});
}

int AuditPolicyFrame::FindColumnIndex(const QueryResult& result,
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

std::string AuditPolicyFrame::ExtractValue(const QueryResult& result,
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

std::string AuditPolicyFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void AuditPolicyFrame::RunCommand(const std::string& sql, const std::string& success_message) {
    if (!connection_manager_) {
        SetMessage("Not connected.");
        return;
    }
    UpdateStatus("Running...");
    connection_manager_->ExecuteQueryAsync(
        sql, [this, success_message](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, success_message]() mutable {
                if (ok) {
                    UpdateStatus(success_message);
                    SetMessage("");
                    RefreshActiveTab();
                } else {
                    UpdateStatus("Command failed");
                    SetMessage(error.empty() ? "Command failed." : error);
                }
            });
        });
}

void AuditPolicyFrame::OnConnect(wxCommandEvent&) {
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
    RefreshActiveTab();
}

void AuditPolicyFrame::OnDisconnect(wxCommandEvent&) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
    }
    UpdateStatus("Disconnected");
    UpdateControls();
}

void AuditPolicyFrame::OnRefresh(wxCommandEvent&) {
    RefreshActiveTab();
}

void AuditPolicyFrame::OnCreate(wxCommandEvent&) {
    if (notebook_ && notebook_->GetSelection() == kTabRetention) {
        AuditRetentionPolicyDialog dialog(this, AuditRetentionPolicyDialog::Mode::Create);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }
        std::string sql = dialog.GetStatement();
        if (sql.empty()) {
            SetMessage("Create retention policy statement is empty.");
            return;
        }
        RunCommand(sql, "Retention policy created");
        return;
    }
    AuditPolicyEditorDialog dialog(this, AuditPolicyEditorDialog::Mode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.GetStatement();
    if (sql.empty()) {
        SetMessage("Create audit policy statement is empty.");
        return;
    }
    RunCommand(sql, "Audit policy created");
}

void AuditPolicyFrame::OnEdit(wxCommandEvent&) {
    if (notebook_ && notebook_->GetSelection() == kTabRetention) {
        std::string policy_id = GetSelectedRetentionPolicyId();
        if (policy_id.empty()) {
            SetMessage("Select a retention policy first.");
            return;
        }
        AuditRetentionPolicyDialog dialog(this, AuditRetentionPolicyDialog::Mode::Edit);
        dialog.SetPolicyId(policy_id);
        if (dialog.ShowModal() != wxID_OK) {
            return;
        }
        std::string sql = dialog.GetStatement();
        if (sql.empty()) {
            SetMessage("Edit retention policy statement is empty.");
            return;
        }
        RunCommand(sql, "Retention policy updated");
        return;
    }
    std::string policy_id = GetSelectedAuditPolicyId();
    if (policy_id.empty()) {
        SetMessage("Select an audit policy first.");
        return;
    }
    AuditPolicyEditorDialog dialog(this, AuditPolicyEditorDialog::Mode::Edit);
    dialog.SetPolicyId(policy_id);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.GetStatement();
    if (sql.empty()) {
        SetMessage("Edit audit policy statement is empty.");
        return;
    }
    RunCommand(sql, "Audit policy updated");
}

void AuditPolicyFrame::OnDrop(wxCommandEvent&) {
    if (notebook_ && notebook_->GetSelection() == kTabRetention) {
        std::string policy_id = GetSelectedRetentionPolicyId();
        if (policy_id.empty()) {
            SetMessage("Select a retention policy first.");
            return;
        }
        std::string sql = "DELETE FROM sys.audit_retention_policy WHERE policy_id = '" +
                          EscapeSqlLiteral(policy_id) + "';";
        RunCommand(sql, "Retention policy deleted");
        return;
    }
    std::string policy_id = GetSelectedAuditPolicyId();
    if (policy_id.empty()) {
        SetMessage("Select an audit policy first.");
        return;
    }
    std::string sql = "DELETE FROM sys.audit_policies WHERE policy_uuid = '" +
                      EscapeSqlLiteral(policy_id) + "';";
    RunCommand(sql, "Audit policy deleted");
}

void AuditPolicyFrame::OnTabChanged(wxBookCtrlEvent&) {
    UpdateControls();
}

void AuditPolicyFrame::OnPolicySelected(wxGridEvent& event) {
    if (!audit_details_) {
        return;
    }
    int row = audit_grid_ ? audit_grid_->GetGridCursorRow() : -1;
    if (row >= 0 && static_cast<size_t>(row) < audit_result_.rows.size()) {
        QueryResult single;
        single.columns = audit_result_.columns;
        single.rows.push_back(audit_result_.rows[static_cast<size_t>(row)]);
        audit_details_->SetValue(FormatDetails(single));
    }
    UpdateControls();
}

void AuditPolicyFrame::OnRetentionSelected(wxGridEvent& event) {
    if (!retention_details_) {
        return;
    }
    int row = retention_grid_ ? retention_grid_->GetGridCursorRow() : -1;
    if (row >= 0 && static_cast<size_t>(row) < retention_result_.rows.size()) {
        QueryResult single;
        single.columns = retention_result_.columns;
        single.rows.push_back(retention_result_.rows[static_cast<size_t>(row)]);
        retention_details_->SetValue(FormatDetails(single));
    }
    UpdateControls();
}

void AuditPolicyFrame::OnClose(wxCloseEvent& event) {
    Destroy();
    event.Skip(false);
}

void AuditPolicyFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void AuditPolicyFrame::OnNewDiagram(wxCommandEvent&) {
    SetMessage("Diagram creation from this view is not yet wired.");
}

void AuditPolicyFrame::OnOpenMonitoring(wxCommandEvent&) {
    auto* frame = new MonitoringFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenUsersRoles(wxCommandEvent&) {
    auto* frame = new UsersRolesFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenJobScheduler(wxCommandEvent&) {
    auto* frame = new JobSchedulerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenSchemaManager(wxCommandEvent&) {
    auto* frame = new SchemaManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenDomainManager(wxCommandEvent&) {
    auto* frame = new DomainManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenTableDesigner(wxCommandEvent&) {
    auto* frame = new TableDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void AuditPolicyFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    auto* frame = new IndexDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

} // namespace scratchrobin
