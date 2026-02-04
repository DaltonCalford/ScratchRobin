/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "sequence_manager_frame.h"

#include "diagram_frame.h"
#include "domain_manager_frame.h"
#include "index_designer_frame.h"
#include "job_scheduler_frame.h"
#include "menu_builder.h"
#include "menu_ids.h"
#include "monitoring_frame.h"
#include "result_grid_table.h"
#include "schema_manager_frame.h"
#include "sequence_editor_dialog.h"
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

wxBEGIN_EVENT_TABLE(SequenceManagerFrame, wxFrame)
    EVT_MENU(ID_MENU_NEW_SQL_EDITOR, SequenceManagerFrame::OnNewSqlEditor)
    EVT_MENU(ID_MENU_NEW_DIAGRAM, SequenceManagerFrame::OnNewDiagram)
    EVT_MENU(ID_MENU_MONITORING, SequenceManagerFrame::OnOpenMonitoring)
    EVT_MENU(ID_MENU_USERS_ROLES, SequenceManagerFrame::OnOpenUsersRoles)
    EVT_MENU(ID_MENU_JOB_SCHEDULER, SequenceManagerFrame::OnOpenJobScheduler)
    EVT_MENU(ID_MENU_DOMAIN_MANAGER, SequenceManagerFrame::OnOpenDomainManager)
    EVT_MENU(ID_MENU_SCHEMA_MANAGER, SequenceManagerFrame::OnOpenSchemaManager)
    EVT_MENU(ID_MENU_TABLE_DESIGNER, SequenceManagerFrame::OnOpenTableDesigner)
    EVT_MENU(ID_MENU_INDEX_DESIGNER, SequenceManagerFrame::OnOpenIndexDesigner)
    EVT_BUTTON(kMenuConnect, SequenceManagerFrame::OnConnect)
    EVT_BUTTON(kMenuDisconnect, SequenceManagerFrame::OnDisconnect)
    EVT_BUTTON(kMenuRefresh, SequenceManagerFrame::OnRefresh)
    EVT_BUTTON(kMenuCreate, SequenceManagerFrame::OnCreate)
    EVT_BUTTON(kMenuEdit, SequenceManagerFrame::OnEdit)
    EVT_BUTTON(kMenuDrop, SequenceManagerFrame::OnDrop)
    EVT_CLOSE(SequenceManagerFrame::OnClose)
wxEND_EVENT_TABLE()

SequenceManagerFrame::SequenceManagerFrame(WindowManager* windowManager,
                                           ConnectionManager* connectionManager,
                                           const std::vector<ConnectionProfile>* connections,
                                           const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Sequences", wxDefaultPosition, wxSize(980, 680)),
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

void SequenceManagerFrame::BuildMenu() {
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

void SequenceManagerFrame::BuildLayout() {
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

    auto* splitter = new wxSplitterWindow(this, wxID_ANY);

    auto* listPanel = new wxPanel(splitter, wxID_ANY);
    auto* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(new wxStaticText(listPanel, wxID_ANY, "Sequences"), 0,
                   wxLEFT | wxRIGHT | wxTOP, 8);
    sequences_grid_ = new wxGrid(listPanel, wxID_ANY);
    sequences_grid_->EnableEditing(false);
    sequences_grid_->SetRowLabelSize(40);
    sequences_table_ = new ResultGridTable();
    sequences_grid_->SetTable(sequences_table_, true);
    listSizer->Add(sequences_grid_, 1, wxEXPAND | wxALL, 8);
    listPanel->SetSizer(listSizer);

    auto* detailPanel = new wxPanel(splitter, wxID_ANY);
    auto* detailSizer = new wxBoxSizer(wxVERTICAL);
    detailSizer->Add(new wxStaticText(detailPanel, wxID_ANY, "Details"), 0,
                     wxLEFT | wxRIGHT | wxTOP, 8);
    
    // Sequence current/next values label
    values_label_ = new wxStaticText(detailPanel, wxID_ANY, "Select a sequence to view values");
    values_label_->SetForegroundColour(wxColour(100, 100, 100));
    detailSizer->Add(values_label_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
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

    sequences_grid_->Bind(wxEVT_GRID_SELECT_CELL, &SequenceManagerFrame::OnSequenceSelected, this);
}

void SequenceManagerFrame::PopulateConnections() {
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

const ConnectionProfile* SequenceManagerFrame::GetSelectedProfile() const {
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

bool SequenceManagerFrame::EnsureConnected(const ConnectionProfile& profile) {
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

bool SequenceManagerFrame::IsNativeProfile(const ConnectionProfile& profile) const {
    return NormalizeBackendName(profile.backend) == "native";
}

void SequenceManagerFrame::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    const auto* profile = GetSelectedProfile();
    bool native = profile ? IsNativeProfile(*profile) : false;
    bool busy = pending_queries_ > 0;
    bool has_sequence = !selected_sequence_.empty();

    if (connect_button_) connect_button_->Enable(!connected);
    if (disconnect_button_) disconnect_button_->Enable(connected);
    if (refresh_button_) refresh_button_->Enable(connected && native && !busy);
    if (create_button_) create_button_->Enable(connected && native && !busy);
    if (edit_button_) edit_button_->Enable(connected && native && has_sequence && !busy);
    if (drop_button_) drop_button_->Enable(connected && native && has_sequence && !busy);
}

void SequenceManagerFrame::UpdateStatus(const wxString& status) {
    if (status_text_) {
        status_text_->SetLabel(status);
    }
}

void SequenceManagerFrame::SetMessage(const std::string& message) {
    if (message_text_) {
        message_text_->SetValue(message);
    }
}

void SequenceManagerFrame::RefreshSequences() {
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
        SetMessage("Sequences are available only for ScratchBird connections.");
        return;
    }

    pending_queries_++;
    UpdateControls();
    UpdateStatus("Loading sequences...");
    connection_manager_->ExecuteQueryAsync(
        "SELECT sequence_name, schema_name, data_type, start_value, increment, "
        "min_value, max_value, cycle_option, cache_size "
        "FROM sb_catalog.sb_sequences ORDER BY schema_name, sequence_name",
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                sequences_result_ = result;
                if (sequences_table_) {
                    sequences_table_->Reset(sequences_result_.columns, sequences_result_.rows);
                }
                if (!ok) {
                    SetMessage(error.empty() ? "Failed to load sequences." : error);
                    UpdateStatus("Load failed");
                } else {
                    SetMessage("");
                    UpdateStatus("Sequences updated");
                }
                UpdateControls();
            });
        });
}

void SequenceManagerFrame::RefreshSequenceDetails(const std::string& sequence_name) {
    if (!connection_manager_ || sequence_name.empty()) {
        return;
    }
    std::string sql = "SHOW SEQUENCE '" + EscapeSqlLiteral(sequence_name) + "'";
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, sequence_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, sequence_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                sequence_details_result_ = result;
                if (ok) {
                    if (details_text_) {
                        details_text_->SetValue(FormatDetails(sequence_details_result_));
                    }
                    // Fetch current/next values for this sequence
                    FetchSequenceValues(sequence_name);
                } else if (!error.empty()) {
                    SetMessage(error);
                }
                UpdateControls();
            });
        });
}

void SequenceManagerFrame::FetchSequenceValues(const std::string& sequence_name) {
    if (!connection_manager_ || sequence_name.empty()) {
        return;
    }
    
    std::string sql = "SELECT current_value, next_value FROM sb_catalog.sb_sequences "
                     "WHERE sequence_name = '" + EscapeSqlLiteral(sequence_name) + "'";
    
    pending_queries_++;
    UpdateControls();
    connection_manager_->ExecuteQueryAsync(
        sql, [this, sequence_name](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error, sequence_name]() mutable {
                pending_queries_ = std::max(0, pending_queries_ - 1);
                sequence_values_result_ = result;
                if (ok && values_label_) {
                    std::string values_text;
                    if (result.rows.empty()) {
                        values_text = "Current values unavailable";
                    } else {
                        std::string current_val = result.rows[0].size() > 0 && !result.rows[0][0].isNull 
                                                  ? result.rows[0][0].text : "NULL";
                        std::string next_val = result.rows[0].size() > 1 && !result.rows[0][1].isNull 
                                               ? result.rows[0][1].text : "NULL";
                        values_text = "Current: " + current_val + "  |  Next: " + next_val;
                    }
                    values_label_->SetLabel(values_text);
                    values_label_->SetForegroundColour(wxColour(80, 80, 80));
                } else if (!ok && values_label_) {
                    values_label_->SetLabel("Current values unavailable");
                    values_label_->SetForegroundColour(wxColour(150, 150, 150));
                }
                UpdateControls();
            });
        });
}

void SequenceManagerFrame::RunCommand(const std::string& sql, const std::string& success_message) {
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
                RefreshSequences();
                if (!selected_sequence_.empty()) {
                    RefreshSequenceDetails(selected_sequence_);
                }
            });
        });
}

std::string SequenceManagerFrame::GetSelectedSequenceName() const {
    if (!sequences_grid_ || sequences_result_.rows.empty()) {
        return std::string();
    }
    int row = sequences_grid_->GetGridCursorRow();
    if (row < 0 || static_cast<size_t>(row) >= sequences_result_.rows.size()) {
        return std::string();
    }
    std::string value = ExtractValue(sequences_result_, row, 
                                     {"sequence_name", "sequence", "name"});
    if (!value.empty()) {
        return value;
    }
    if (!sequences_result_.rows[row].empty()) {
        return sequences_result_.rows[row][0].text;
    }
    return std::string();
}

int SequenceManagerFrame::FindColumnIndex(const QueryResult& result,
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

std::string SequenceManagerFrame::ExtractValue(const QueryResult& result,
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

std::string SequenceManagerFrame::FormatDetails(const QueryResult& result) const {
    if (result.rows.empty()) {
        return "No sequence details returned.";
    }
    std::ostringstream out;
    const auto& row = result.rows.front();
    for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
        out << result.columns[i].name << ": " << row[i].text << "\n";
    }
    return out.str();
}

void SequenceManagerFrame::OnConnect(wxCommandEvent&) {
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
    RefreshSequences();
}

void SequenceManagerFrame::OnDisconnect(wxCommandEvent&) {
    if (!connection_manager_) {
        return;
    }
    connection_manager_->Disconnect();
    UpdateStatus("Disconnected");
    UpdateControls();
}

void SequenceManagerFrame::OnRefresh(wxCommandEvent&) {
    RefreshSequences();
}

void SequenceManagerFrame::OnSequenceSelected(wxGridEvent& event) {
    selected_sequence_ = GetSelectedSequenceName();
    if (!selected_sequence_.empty()) {
        RefreshSequenceDetails(selected_sequence_);
    }
    UpdateControls();
    event.Skip();
}

void SequenceManagerFrame::OnCreate(wxCommandEvent&) {
    SequenceEditorDialog dialog(this, SequenceEditorMode::Create);
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Create sequence statement is empty.");
        return;
    }
    RunCommand(sql, "Sequence created");
}

void SequenceManagerFrame::OnEdit(wxCommandEvent&) {
    if (selected_sequence_.empty()) {
        return;
    }
    SequenceEditorDialog dialog(this, SequenceEditorMode::Edit);
    dialog.SetSequenceName(selected_sequence_);
    if (!sequence_details_result_.rows.empty()) {
        dialog.SetDataType(ExtractValue(sequence_details_result_, 0, {"data_type"}));
        // Parse numeric values from strings
        auto parse_int64 = [](const std::string& s) -> int64_t {
            try { return static_cast<int64_t>(std::stoll(s)); } catch (...) { return 0; }
        };
        auto parse_int = [](const std::string& s) -> int {
            try { return std::stoi(s); } catch (...) { return 0; }
        };
        auto parse_bool = [](const std::string& s) -> bool {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });
            return lower == "true" || lower == "yes" || lower == "1" || lower == "on" || lower == "cycle";
        };
        dialog.SetStartValue(parse_int64(ExtractValue(sequence_details_result_, 0, {"start_value"})));
        dialog.SetIncrementBy(parse_int64(ExtractValue(sequence_details_result_, 0, {"increment"})));
        dialog.SetMinValue(parse_int64(ExtractValue(sequence_details_result_, 0, {"min_value"})));
        dialog.SetMaxValue(parse_int64(ExtractValue(sequence_details_result_, 0, {"max_value"})));
        dialog.SetCycle(parse_bool(ExtractValue(sequence_details_result_, 0, {"cycle_option", "cycle"})));
        dialog.SetCacheSize(parse_int(ExtractValue(sequence_details_result_, 0, {"cache_size", "cache"})));
    }
    if (dialog.ShowModal() != wxID_OK) {
        return;
    }
    std::string sql = dialog.BuildSql();
    if (sql.empty()) {
        SetMessage("Alter sequence statement is empty.");
        return;
    }
    RunCommand(sql, "Sequence altered");
}

void SequenceManagerFrame::OnDrop(wxCommandEvent&) {
    if (selected_sequence_.empty()) {
        return;
    }
    std::string sql = "DROP SEQUENCE " + QuoteIdentifier(selected_sequence_) + ";";
    RunCommand(sql, "Sequence dropped");
}

void SequenceManagerFrame::OnNewSqlEditor(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* editor = new SqlEditorFrame(window_manager_, connection_manager_, connections_,
                                      app_config_, nullptr);
    editor->Show(true);
}

void SequenceManagerFrame::OnNewDiagram(wxCommandEvent&) {
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

void SequenceManagerFrame::OnOpenMonitoring(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* monitor = new MonitoringFrame(window_manager_, connection_manager_, connections_,
                                         app_config_);
    monitor->Show(true);
}

void SequenceManagerFrame::OnOpenUsersRoles(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* users = new UsersRolesFrame(window_manager_, connection_manager_, connections_,
                                      app_config_);
    users->Show(true);
}

void SequenceManagerFrame::OnOpenJobScheduler(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* scheduler = new JobSchedulerFrame(window_manager_, connection_manager_, connections_,
                                            app_config_);
    scheduler->Show(true);
}

void SequenceManagerFrame::OnOpenDomainManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* domains = new DomainManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    domains->Show(true);
}

void SequenceManagerFrame::OnOpenSchemaManager(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* schemas = new SchemaManagerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    schemas->Show(true);
}

void SequenceManagerFrame::OnOpenTableDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* tables = new TableDesignerFrame(window_manager_, connection_manager_, connections_,
                                          app_config_);
    tables->Show(true);
}

void SequenceManagerFrame::OnOpenIndexDesigner(wxCommandEvent&) {
    if (!window_manager_) {
        return;
    }
    auto* indexes = new IndexDesignerFrame(window_manager_, connection_manager_, connections_,
                                           app_config_);
    indexes->Show(true);
}

void SequenceManagerFrame::OnClose(wxCloseEvent&) {
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

} // namespace scratchrobin
