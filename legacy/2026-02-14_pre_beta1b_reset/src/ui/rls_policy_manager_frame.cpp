/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "rls_policy_manager_frame.h"

#include "core/config.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "rls_policy_editor_dialog.h"
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
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

constexpr int kMenuConnect = wxID_HIGHEST + 240;
constexpr int kMenuDisconnect = wxID_HIGHEST + 241;
constexpr int kMenuRefresh = wxID_HIGHEST + 242;
constexpr int kMenuCreate = wxID_HIGHEST + 243;
constexpr int kMenuEdit = wxID_HIGHEST + 244;
constexpr int kMenuDrop = wxID_HIGHEST + 245;
constexpr int kMenuEnable = wxID_HIGHEST + 246;
constexpr int kMenuDisable = wxID_HIGHEST + 247;
constexpr int kConnectionChoiceId = wxID_HIGHEST + 248;

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

wxBEGIN_EVENT_TABLE(RlsPolicyManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, RlsPolicyManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, RlsPolicyManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, RlsPolicyManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, RlsPolicyManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, RlsPolicyManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, RlsPolicyManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, RlsPolicyManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, RlsPolicyManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, RlsPolicyManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, RlsPolicyManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, RlsPolicyManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, RlsPolicyManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, RlsPolicyManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, RlsPolicyManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, RlsPolicyManagerFrame::OnDrop)
    EVT_BUTTON(kMenuEnable, RlsPolicyManagerFrame::OnEnableTable)
    EVT_BUTTON(kMenuDisable, RlsPolicyManagerFrame::OnDisableTable)
    EVT_GRID_SELECT_CELL(RlsPolicyManagerFrame::OnPolicySelected)
    EVT_CLOSE(RlsPolicyManagerFrame::OnClose)
wxEND_EVENT_TABLE()

RlsPolicyManagerFrame::RlsPolicyManagerFrame(WindowManager* windowManager,
                                             ConnectionManager* connectionManager,
                                             const std::vector<ConnectionProfile>* connections,
                                             const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Row-Level Security Policies", wxDefaultPosition, wxSize(1080, 720)),
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

RlsPolicyManagerFrame::~RlsPolicyManagerFrame() {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
}

void RlsPolicyManagerFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void RlsPolicyManagerFrame::BuildLayout() {
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
    enable_button_ = new wxButton(actionPanel, kMenuEnable, "Enable RLS (Table)");
    disable_button_ = new wxButton(actionPanel, kMenuDisable, "Disable RLS (Table)");
    actionSizer->Add(create_button_, 0, wxRIGHT, 6);
    actionSizer->Add(edit_button_, 0, wxRIGHT, 6);
    actionSizer->Add(drop_button_, 0, wxRIGHT, 6);
    actionSizer->Add(enable_button_, 0, wxRIGHT, 6);
    actionSizer->Add(disable_button_, 0, wxRIGHT, 6);
    actionPanel->SetSizer(actionSizer);
    rootSizer->Add(actionPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetSashGravity(0.6);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    policy_grid_ = new wxGrid(listPanel, wxID_ANY);
    policy_grid_->EnableEditing(false);
    policy_grid_->SetRowLabelSize(40);
    policy_table_ = new ResultGridTable();
    policy_grid_->SetTable(policy_table_, true);
    listSizer->Add(policy_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    detailSizer->Add(new wxStaticText(detailPanel, wxID_ANY, "Policy Details"), 0,
                     wxLEFT | wxRIGHT | wxTOP, 8);
    details_text_ = new wxTextCtrl(detailPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY);
    detailSizer->Add(details_text_, 1, wxEXPAND | wxALL, 8);
    detailPanel->SetSizer(detailSizer);

    splitter->SplitVertically(listPanel, detailPanel, 700);
    rootSizer->Add(splitter, 1, wxEXPAND);

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

void RlsPolicyManagerFrame::PopulateConnections() {
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

void RlsPolicyManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool has_policy = !GetSelectedPolicyId().empty();
    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected);
    if (create_button_) create_button_->Enable(connected);
    if (edit_button_) edit_button_->Enable(connected && has_policy);
    if (drop_button_) drop_button_->Enable(connected && has_policy);
    if (enable_button_) enable_button_->Enable(connected && has_policy);
    if (disable_button_) disable_button_->Enable(connected && has_policy);
}

void RlsPolicyManagerFrame::UpdateStatus(const wxString& status) {
    if (status_label_) {
        status_label_->SetLabel("Status: " + status);
    }
}

void RlsPolicyManagerFrame::SetMessage(const std::string& message) {
    if (message_label_) {
        message_label_->SetLabel(message);
    }
}

const ConnectionProfile* RlsPolicyManagerFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int index = connection_choice_->GetSelection();
    if (index < 0 || static_cast<size_t>(index) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(index)];
}

bool RlsPolicyManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    if (connection_manager_->IsConnected()) {
        return true;
    }
    return connection_manager_->Connect(profile);
}

void RlsPolicyManagerFrame::RefreshPolicies() {
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
        SetMessage("RLS policies are currently supported for ScratchBird connections.");
        return;
    }
    UpdateStatus("Loading policies...");
    SetMessage("");

    std::string sql =
        "SELECT policy_id, policy_name, table_id, policy_type, is_enabled, "
        "created_time, modified_time "
        "FROM sb_catalog.sb_policies "
        "ORDER BY policy_name";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load policies." : error);
                    UpdateStatus("Load failed");
                    return;
                }
                policies_result_ = std::move(result);
                if (policy_table_) {
                    policy_table_->Reset(policies_result_.columns, policies_result_.rows);
                }
                UpdateControls();
                UpdateStatus("Policies updated");
            });
        });
}

void RlsPolicyManagerFrame::RefreshPolicyDetails(const std::string& policy_id) {
    if (!connection_manager_ || policy_id.empty()) {
        return;
    }
    std::string sql = "SELECT policy_id, policy_name, table_id, policy_type, roles_oid, "
                      "using_expr_oid, with_check_expr_oid, is_enabled, created_time, modified_time "
                      "FROM sb_catalog.sb_policies "
                      "WHERE policy_id = '" + EscapeSqlLiteral(policy_id) + "'";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (!ok) {
                    details_text_->SetValue(error.empty() ? "Failed to load policy details." : error);
                    return;
                }
                policy_details_result_ = std::move(result);
                details_text_->SetValue(FormatDetails(policy_details_result_));
            });
        });
}

std::string RlsPolicyManagerFrame::GetSelectedPolicyId() const {
    if (!policy_grid_ || policies_result_.rows.empty()) {
        return std::string();
    }
    int row = policy_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= policies_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(policies_result_, row, {"policy_id", "id"});
    if (!value.empty()) {
        return value;
    }
    if (!policies_result_.rows[row].empty()) {
        return policies_result_.rows[row][0].text;
    }
    return std::string();
}

std::string RlsPolicyManagerFrame::GetSelectedPolicyName() const {
    if (!policy_grid_ || policies_result_.rows.empty()) {
        return std::string();
    }
    int row = policy_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= policies_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(policies_result_, row, {"policy_name", "name"});
    if (!value.empty()) {
        return value;
    }
    return std::string();
}

std::string RlsPolicyManagerFrame::GetSelectedTableName() const {
    if (!policy_grid_ || policies_result_.rows.empty()) {
        return std::string();
    }
    int row = policy_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= policies_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(policies_result_, row, {"table_name", "table"});
    if (!value.empty()) {
        return value;
    }
    return ExtractValue(policies_result_, row, {"table_id"});
}

int RlsPolicyManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string RlsPolicyManagerFrame::ExtractValue(const QueryResult& result,
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

std::string RlsPolicyManagerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No policy details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void RlsPolicyManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                    RefreshPolicies();
                } else {
                    UpdateStatus("Command failed");
                    SetMessage(error.empty() ? "Command failed." : error);
                }
            });
        });
}

void RlsPolicyManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshPolicies();
}

void RlsPolicyManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (connection_manager_) {
        connection_manager_->Disconnect();
    }
    UpdateStatus("Disconnected");
    UpdateControls();
}

void RlsPolicyManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshPolicies();
}

void RlsPolicyManagerFrame::OnCreate(wxCommandEvent&) {
    RlsPolicyEditorDialog dialog(this, RlsPolicyEditorDialog::Mode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.GetStatement();
    if (sql.empty()) {
        SetMessage("Create policy statement is empty.");
        return;
    }
    RunCommand(sql, "Policy created");
}

void RlsPolicyManagerFrame::OnEdit(wxCommandEvent&) {
    std::string policy_name = GetSelectedPolicyName();
    std::string table_name = GetSelectedTableName();
    if (policy_name.empty()) {
        SetMessage("Select a policy first.");
        return;
    }
    RlsPolicyEditorDialog dialog(this, RlsPolicyEditorDialog::Mode::Edit);
    dialog.SetPolicyName(policy_name);
    if (!table_name.empty()) {
        dialog.SetTableName(table_name);
    }
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.GetStatement();
    if (sql.empty()) {
        SetMessage("Edit policy statement is empty.");
        return;
    }
    RunCommand(sql, "Policy updated");
}

void RlsPolicyManagerFrame::OnDrop(wxCommandEvent&) {
    std::string policy_name = GetSelectedPolicyName();
    std::string table_name = GetSelectedTableName();
    if (policy_name.empty()) {
        SetMessage("Select a policy first.");
        return;
    }
    std::string sql = "DROP POLICY " + QuoteIdentifier(policy_name) +
                      " ON " + QuoteIdentifier(table_name.empty() ? "table_name" : table_name) + ";";
    RunCommand(sql, "Policy dropped");
}

void RlsPolicyManagerFrame::OnEnableTable(wxCommandEvent&) {
    std::string table_name = GetSelectedTableName();
    if (table_name.empty()) {
        SetMessage("Select a policy with a table name.");
        return;
    }
    std::string sql = "ALTER TABLE " + QuoteIdentifier(table_name) +
                      " ENABLE ROW LEVEL SECURITY;";
    RunCommand(sql, "Row-level security enabled");
}

void RlsPolicyManagerFrame::OnDisableTable(wxCommandEvent&) {
    std::string table_name = GetSelectedTableName();
    if (table_name.empty()) {
        SetMessage("Select a policy with a table name.");
        return;
    }
    std::string sql = "ALTER TABLE " + QuoteIdentifier(table_name) +
                      " DISABLE ROW LEVEL SECURITY;";
    RunCommand(sql, "Row-level security disabled");
}

void RlsPolicyManagerFrame::OnPolicySelected(wxGridEvent& event) {
    event.Skip();
    std::string policy_id = GetSelectedPolicyId();
    UpdateControls();
    if (!policy_id.empty()) {
        RefreshPolicyDetails(policy_id);
    }
}

void RlsPolicyManagerFrame::OnClose(wxCloseEvent& event) {
    Destroy();
    event.Skip(false);
}

void RlsPolicyManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void RlsPolicyManagerFrame::OnNewDiagram(wxCommandEvent&) {
    SetMessage("Diagram creation from this view is not yet wired.");
}

void RlsPolicyManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    auto* frame = new MonitoringFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    auto* frame = new UsersRolesFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    auto* frame = new JobSchedulerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    auto* frame = new SchemaManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    auto* frame = new DomainManagerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    auto* frame = new TableDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

void RlsPolicyManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    auto* frame = new IndexDesignerFrame(window_manager_, connection_manager_, connections_, app_config_);
    frame->Show(true);
}

} // namespace scratchrobin
