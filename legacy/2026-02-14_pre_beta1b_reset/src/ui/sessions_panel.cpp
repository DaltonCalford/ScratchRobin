/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "sessions_panel.h"

#include "result_grid_table.h"

#include <algorithm>
#include <cctype>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>

namespace scratchrobin {

namespace {

constexpr int kRefreshButtonId = wxID_HIGHEST + 100;
constexpr int kKillButtonId = wxID_HIGHEST + 101;
constexpr int kDetailsButtonId = wxID_HIGHEST + 102;
constexpr int kAutoRefreshCheckId = wxID_HIGHEST + 103;
constexpr int kIntervalChoiceId = wxID_HIGHEST + 104;
constexpr int kTimerId = wxID_HIGHEST + 105;

// Column mapping for different backends
const std::vector<std::string> kSessionIdCols = {"session_id", "pid", "MON$ATTACHMENT_ID", "ID"};
const std::vector<std::string> kUserNameCols = {"user_name", "usename", "USER", "MON$USER"};
const std::vector<std::string> kDatabaseCols = {"database_name", "datname", "DB", "database"};
const std::vector<std::string> kApplicationCols = {"application_name", "protocol", "COMMAND", "MON$REMOTE_PROTOCOL"};
const std::vector<std::string> kClientAddrCols = {"client_addr", "HOST", "MON$REMOTE_ADDRESS"};
const std::vector<std::string> kLoginTimeCols = {"connected_at", "backend_start", "login_time"};
const std::vector<std::string> kLastActivityCols = {"last_activity_at", "query_start", "xact_start", "last_activity"};
const std::vector<std::string> kStatusCols = {"state", "status", "MON$STATE"};
const std::vector<std::string> kTransactionIdCols = {"transaction_id", "trx_id"};
const std::vector<std::string> kStatementIdCols = {"statement_id", "MON$STATEMENT_ID"};
const std::vector<std::string> kCurrentQueryCols = {"current_query", "query", "INFO", "MON$SQL_TEXT"};
const std::vector<std::string> kWaitEventCols = {"wait_event", "wait_event_type", "STATE"};
const std::vector<std::string> kWaitResourceCols = {"wait_resource", "LOCK_DATA"};

} // namespace

const std::vector<std::string> SessionsPanel::kColumnNames = {
    "session_id", "user_name", "database", "application", 
    "client_addr", "login_time", "last_activity", "status"
};

const std::vector<std::string> SessionsPanel::kColumnLabels = {
    "Session ID", "User", "Database", "Application", 
    "Client Address", "Login Time", "Last Activity", "Status"
};

wxBEGIN_EVENT_TABLE(SessionsPanel, wxPanel)
    EVT_BUTTON(kRefreshButtonId, SessionsPanel::OnRefresh)
    EVT_BUTTON(kKillButtonId, SessionsPanel::OnKillSession)
    EVT_BUTTON(kDetailsButtonId, SessionsPanel::OnShowDetails)
    EVT_CHECKBOX(kAutoRefreshCheckId, SessionsPanel::OnAutoRefreshToggle)
    EVT_CHOICE(kIntervalChoiceId, SessionsPanel::OnIntervalChanged)
    EVT_GRID_SELECT_CELL(SessionsPanel::OnGridSelect)
    EVT_GRID_CELL_LEFT_DCLICK(SessionsPanel::OnGridDoubleClick)
    EVT_TIMER(kTimerId, SessionsPanel::OnTimer)
wxEND_EVENT_TABLE()

SessionsPanel::SessionsPanel(wxWindow* parent,
                             ConnectionManager* connectionManager)
    : wxPanel(parent, wxID_ANY),
      connection_manager_(connectionManager) {
    BuildLayout();
    UpdateControls();
    UpdateStatus("Ready");
}

void SessionsPanel::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar panel
    auto* toolbar = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    refresh_button_ = new wxButton(toolbar, kRefreshButtonId, "Refresh");
    toolbarSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    kill_button_ = new wxButton(toolbar, kKillButtonId, "Kill Session");
    kill_button_->Enable(false);
    toolbarSizer->Add(kill_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    details_button_ = new wxButton(toolbar, kDetailsButtonId, "Details");
    details_button_->Enable(false);
    toolbarSizer->Add(details_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    auto_refresh_check_ = new wxCheckBox(toolbar, kAutoRefreshCheckId, "Auto-refresh");
    toolbarSizer->Add(auto_refresh_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    interval_choice_ = new wxChoice(toolbar, kIntervalChoiceId);
    interval_choice_->Append("5 sec");
    interval_choice_->Append("10 sec");
    interval_choice_->Append("30 sec");
    interval_choice_->Append("1 min");
    interval_choice_->Append("5 min");
    interval_choice_->SetSelection(2);  // Default 30 sec
    interval_choice_->Enable(false);
    toolbarSizer->Add(interval_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->AddStretchSpacer(1);

    count_label_ = new wxStaticText(toolbar, wxID_ANY, "0 sessions");
    toolbarSizer->Add(count_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    status_label_ = new wxStaticText(toolbar, wxID_ANY, "Ready");
    toolbarSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->SetSizer(toolbarSizer);
    rootSizer->Add(toolbar, 0, wxEXPAND | wxALL, 8);

    // Grid panel
    auto* gridPanel = new wxPanel(this, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);

    sessions_grid_ = new wxGrid(gridPanel, wxID_ANY);
    grid_table_ = new ResultGridTable();
    sessions_grid_->SetTable(grid_table_, true);
    sessions_grid_->EnableEditing(false);
    sessions_grid_->SetRowLabelSize(48);
    sessions_grid_->EnableGridLines(true);

    gridSizer->Add(sessions_grid_, 1, wxEXPAND | wxALL, 8);
    gridPanel->SetSizer(gridSizer);
    rootSizer->Add(gridPanel, 1, wxEXPAND);

    SetSizer(rootSizer);

    // Initialize timer
    refresh_timer_ = new wxTimer(this, kTimerId);
}

void SessionsPanel::RefreshData() {
    LoadSessions();
}

void SessionsPanel::LoadSessions() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading sessions...");

    // Query for active sessions
    const std::string query = 
        "SELECT * FROM sys.sessions WHERE status = 'active' "
        "ORDER BY last_activity_at DESC;";

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseSessions(result);
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load sessions" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
            });
        });
}

void SessionsPanel::ParseSessions(const QueryResult& result) {
    sessions_.clear();

    // Build column name list
    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    // Parse each row
    for (const auto& row : result.rows) {
        sessions_.push_back(ExtractSessionInfo(row, colNames));
    }

    // Update grid
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> gridRows;

    for (size_t i = 0; i < kColumnNames.size(); ++i) {
        QueryColumn col;
        col.name = kColumnLabels[i];
        col.type = "TEXT";
        columns.push_back(col);
    }

    for (const auto& session : sessions_) {
        std::vector<QueryValue> row;
        row.push_back({false, session.session_id, {}});
        row.push_back({false, session.user_name, {}});
        row.push_back({false, session.database, {}});
        row.push_back({false, session.application, {}});
        row.push_back({false, session.client_addr, {}});
        row.push_back({false, session.login_time, {}});
        row.push_back({false, session.last_activity, {}});
        row.push_back({false, session.status, {}});
        gridRows.push_back(row);
    }

    if (grid_table_) {
        grid_table_->Reset(columns, gridRows);
    }

    // Update count label
    if (count_label_) {
        count_label_->SetLabel(wxString::Format("%zu sessions", sessions_.size()));
    }
}

SessionInfo SessionsPanel::ExtractSessionInfo(const std::vector<QueryValue>& row,
                                               const std::vector<std::string>& colNames) {
    SessionInfo info;
    info.session_id = FindColumnValue(row, colNames, kSessionIdCols);
    info.user_name = FindColumnValue(row, colNames, kUserNameCols);
    info.database = FindColumnValue(row, colNames, kDatabaseCols);
    info.application = FindColumnValue(row, colNames, kApplicationCols);
    info.client_addr = FindColumnValue(row, colNames, kClientAddrCols);
    info.login_time = FindColumnValue(row, colNames, kLoginTimeCols);
    info.last_activity = FindColumnValue(row, colNames, kLastActivityCols);
    info.status = FindColumnValue(row, colNames, kStatusCols);
    info.transaction_id = FindColumnValue(row, colNames, kTransactionIdCols);
    info.statement_id = FindColumnValue(row, colNames, kStatementIdCols);
    info.current_query = FindColumnValue(row, colNames, kCurrentQueryCols);
    info.wait_event = FindColumnValue(row, colNames, kWaitEventCols);
    info.wait_resource = FindColumnValue(row, colNames, kWaitResourceCols);
    return info;
}

std::string SessionsPanel::FindColumnValue(const std::vector<QueryValue>& row,
                                           const std::vector<std::string>& colNames,
                                           const std::vector<std::string>& possibleNames) const {
    int idx = FindColumnIndex(colNames, possibleNames);
    if (idx >= 0 && idx < static_cast<int>(row.size())) {
        return row[idx].isNull ? "" : row[idx].text;
    }
    return "";
}

int SessionsPanel::FindColumnIndex(const std::vector<std::string>& colNames,
                                   const std::vector<std::string>& possibleNames) const {
    for (const auto& name : possibleNames) {
        auto it = std::find_if(colNames.begin(), colNames.end(),
            [&name](const std::string& col) {
                if (col.size() != name.size()) return false;
                return std::equal(col.begin(), col.end(), name.begin(),
                    [](char a, char b) {
                        return std::tolower(static_cast<unsigned char>(a)) ==
                               std::tolower(static_cast<unsigned char>(b));
                    });
            });
        if (it != colNames.end()) {
            return static_cast<int>(std::distance(colNames.begin(), it));
        }
    }
    return -1;
}

void SessionsPanel::SetAutoRefresh(bool enable, int intervalSeconds) {
    if (!auto_refresh_check_ || !interval_choice_ || !refresh_timer_) {
        return;
    }

    auto_refresh_check_->SetValue(enable);
    interval_choice_->Enable(enable);

    if (enable) {
        // Set interval
        int intervalMs = intervalSeconds * 1000;
        refresh_timer_->Start(intervalMs);
    } else {
        refresh_timer_->Stop();
    }
}

bool SessionsPanel::IsAutoRefreshEnabled() const {
    return auto_refresh_check_ && auto_refresh_check_->IsChecked();
}

void SessionsPanel::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool has_selection = selected_row_ >= 0 && selected_row_ < static_cast<int>(sessions_.size());

    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
    if (kill_button_) {
        kill_button_->Enable(connected && has_selection && !query_running_);
    }
    if (details_button_) {
        details_button_->Enable(has_selection);
    }
}

void SessionsPanel::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void SessionsPanel::OnRefresh(wxCommandEvent&) {
    RefreshData();
}

void SessionsPanel::OnKillSession(wxCommandEvent&) {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(sessions_.size())) {
        return;
    }

    const auto& session = sessions_[selected_row_];
    wxString msg = wxString::Format(
        "Are you sure you want to kill session %s (user: %s)?\n\n"
        "This will terminate all active transactions and statements for this session.",
        session.session_id, session.user_name);

    int result = wxMessageBox(msg, "Confirm Kill Session",
                              wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }

    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }

    // Execute kill session command
    std::string sql = "CALL sys.kill_session('" + session.session_id + "');";
    connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Session killed");
                    RefreshData();  // Refresh to show updated list
                } else {
                    UpdateStatus("Kill failed");
                    wxMessageBox(error.empty() ? "Failed to kill session" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void SessionsPanel::OnShowDetails(wxCommandEvent&) {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(sessions_.size())) {
        return;
    }

    const auto& session = sessions_[selected_row_];
    wxString details = wxString::Format(
        "Session Details:\n\n"
        "Session ID: %s\n"
        "User: %s\n"
        "Database: %s\n"
        "Application: %s\n"
        "Client Address: %s\n"
        "Login Time: %s\n"
        "Last Activity: %s\n"
        "Status: %s\n"
        "Transaction ID: %s\n"
        "Statement ID: %s\n"
        "Wait Event: %s\n"
        "Wait Resource: %s\n\n"
        "Current Query:\n%s",
        session.session_id, session.user_name, session.database,
        session.application, session.client_addr, session.login_time,
        session.last_activity, session.status, session.transaction_id,
        session.statement_id, session.wait_event, session.wait_resource,
        session.current_query.empty() ? "(none)" : session.current_query);

    wxMessageBox(details, "Session Details", wxOK | wxICON_INFORMATION);
}

void SessionsPanel::OnAutoRefreshToggle(wxCommandEvent&) {
    bool enable = auto_refresh_check_->IsChecked();
    interval_choice_->Enable(enable);

    if (enable) {
        int intervals[] = {5, 10, 30, 60, 300};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 5) {
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    } else {
        refresh_timer_->Stop();
    }
}

void SessionsPanel::OnIntervalChanged(wxCommandEvent&) {
    if (auto_refresh_check_->IsChecked()) {
        int intervals[] = {5, 10, 30, 60, 300};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 5) {
            refresh_timer_->Stop();
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    }
}

void SessionsPanel::OnGridSelect(wxGridEvent& event) {
    selected_row_ = event.GetRow();
    UpdateControls();
}

void SessionsPanel::OnGridDoubleClick(wxGridEvent& event) {
    selected_row_ = event.GetRow();
    UpdateControls();
    // Show details on double-click
    wxCommandEvent dummy;
    OnShowDetails(dummy);
}

void SessionsPanel::OnTimer(wxTimerEvent&) {
    if (!query_running_) {
        LoadSessions();
    }
}

} // namespace scratchrobin
