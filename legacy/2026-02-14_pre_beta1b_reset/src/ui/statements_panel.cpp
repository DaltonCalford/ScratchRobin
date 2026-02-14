/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "statements_panel.h"

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

constexpr int kRefreshButtonId = wxID_HIGHEST + 300;
constexpr int kCancelButtonId = wxID_HIGHEST + 301;
constexpr int kPlanButtonId = wxID_HIGHEST + 302;
constexpr int kDetailsButtonId = wxID_HIGHEST + 303;
constexpr int kAutoRefreshCheckId = wxID_HIGHEST + 304;
constexpr int kIntervalChoiceId = wxID_HIGHEST + 305;
constexpr int kTimerId = wxID_HIGHEST + 306;

// Column mapping for different backends
const std::vector<std::string> kStatementIdCols = {"statement_id", "MON$STATEMENT_ID", "EVENT_ID"};
const std::vector<std::string> kSessionIdCols = {"session_id", "pid", "MON$ATTACHMENT_ID", "THREAD_ID"};
const std::vector<std::string> kUserNameCols = {"user_name", "usename", "USER"};
const std::vector<std::string> kSqlPreviewCols = {"sql_preview", "sql_text", "query", "INFO", "SQL_TEXT", "MON$SQL_TEXT"};
const std::vector<std::string> kStartTimeCols = {"start_time", "TIMESTAMP", "query_start", "xact_start"};
const std::vector<std::string> kElapsedTimeCols = {"elapsed_ms", "elapsed_time", "duration", "TIMER_WAIT", "TIME"};
const std::vector<std::string> kRowsAffectedCols = {"rows_processed", "rows_affected", "rows_returned"};
const std::vector<std::string> kStatusCols = {"state", "status", "MON$STATE"};
const std::vector<std::string> kTransactionIdCols = {"transaction_id", "MON$TRANSACTION_ID"};
const std::vector<std::string> kWaitEventCols = {"wait_event", "wait_event_type", "EVENT_NAME"};
const std::vector<std::string> kWaitResourceCols = {"wait_resource", "LOCK_DATA"};

} // namespace

const std::vector<std::string> StatementsPanel::kColumnNames = {
    "statement_id", "session_id", "user_name", "sql_preview",
    "start_time", "elapsed_time", "rows_affected", "status"
};

const std::vector<std::string> StatementsPanel::kColumnLabels = {
    "Statement ID", "Session", "User", "SQL Preview",
    "Start Time", "Elapsed", "Rows", "Status"
};

wxBEGIN_EVENT_TABLE(StatementsPanel, wxPanel)
    EVT_BUTTON(kRefreshButtonId, StatementsPanel::OnRefresh)
    EVT_BUTTON(kCancelButtonId, StatementsPanel::OnCancelStatement)
    EVT_BUTTON(kPlanButtonId, StatementsPanel::OnShowExecutionPlan)
    EVT_BUTTON(kDetailsButtonId, StatementsPanel::OnShowDetails)
    EVT_CHECKBOX(kAutoRefreshCheckId, StatementsPanel::OnAutoRefreshToggle)
    EVT_CHOICE(kIntervalChoiceId, StatementsPanel::OnIntervalChanged)
    EVT_GRID_SELECT_CELL(StatementsPanel::OnGridSelect)
    EVT_GRID_CELL_LEFT_DCLICK(StatementsPanel::OnGridDoubleClick)
    EVT_TIMER(kTimerId, StatementsPanel::OnTimer)
wxEND_EVENT_TABLE()

StatementsPanel::StatementsPanel(wxWindow* parent,
                                 ConnectionManager* connectionManager)
    : wxPanel(parent, wxID_ANY),
      connection_manager_(connectionManager) {
    BuildLayout();
    UpdateControls();
    UpdateStatus("Ready");
}

void StatementsPanel::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar panel
    auto* toolbar = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    refresh_button_ = new wxButton(toolbar, kRefreshButtonId, "Refresh");
    toolbarSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    cancel_button_ = new wxButton(toolbar, kCancelButtonId, "Cancel Statement");
    cancel_button_->Enable(false);
    toolbarSizer->Add(cancel_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    plan_button_ = new wxButton(toolbar, kPlanButtonId, "Execution Plan");
    plan_button_->Enable(false);
    toolbarSizer->Add(plan_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

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
    interval_choice_->SetSelection(1);  // Default 10 sec for statements
    interval_choice_->Enable(false);
    toolbarSizer->Add(interval_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->AddStretchSpacer(1);

    count_label_ = new wxStaticText(toolbar, wxID_ANY, "0 statements");
    toolbarSizer->Add(count_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    status_label_ = new wxStaticText(toolbar, wxID_ANY, "Ready");
    toolbarSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->SetSizer(toolbarSizer);
    rootSizer->Add(toolbar, 0, wxEXPAND | wxALL, 8);

    // Grid panel
    auto* gridPanel = new wxPanel(this, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);

    statements_grid_ = new wxGrid(gridPanel, wxID_ANY);
    grid_table_ = new ResultGridTable();
    statements_grid_->SetTable(grid_table_, true);
    statements_grid_->EnableEditing(false);
    statements_grid_->SetRowLabelSize(48);
    statements_grid_->EnableGridLines(true);

    gridSizer->Add(statements_grid_, 1, wxEXPAND | wxALL, 8);
    gridPanel->SetSizer(gridSizer);
    rootSizer->Add(gridPanel, 1, wxEXPAND);

    SetSizer(rootSizer);

    // Initialize timer
    refresh_timer_ = new wxTimer(this, kTimerId);
}

void StatementsPanel::RefreshData() {
    LoadStatements();
}

void StatementsPanel::LoadStatements() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading statements...");

    // Query for running statements
    const std::string query = 
        "SELECT * FROM sys.statements WHERE status = 'running' "
        "ORDER BY start_time DESC;";

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseStatements(result);
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load statements" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
            });
        });
}

void StatementsPanel::ParseStatements(const QueryResult& result) {
    statements_.clear();

    // Build column name list
    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    // Parse each row
    for (const auto& row : result.rows) {
        statements_.push_back(ExtractStatementInfo(row, colNames));
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

    for (const auto& stmt : statements_) {
        std::vector<QueryValue> row;
        row.push_back({false, stmt.statement_id, {}});
        row.push_back({false, stmt.session_id, {}});
        row.push_back({false, stmt.user_name, {}});
        row.push_back({false, stmt.sql_preview, {}});
        row.push_back({false, stmt.start_time, {}});
        row.push_back({false, stmt.elapsed_time, {}});
        row.push_back({false, stmt.rows_affected, {}});
        row.push_back({false, stmt.status, {}});
        gridRows.push_back(row);
    }

    if (grid_table_) {
        grid_table_->Reset(columns, gridRows);
    }

    // Update count label
    if (count_label_) {
        count_label_->SetLabel(wxString::Format("%zu statements", statements_.size()));
    }
}

StatementInfo StatementsPanel::ExtractStatementInfo(const std::vector<QueryValue>& row,
                                                    const std::vector<std::string>& colNames) {
    StatementInfo info;
    info.statement_id = FindColumnValue(row, colNames, kStatementIdCols);
    info.session_id = FindColumnValue(row, colNames, kSessionIdCols);
    info.user_name = FindColumnValue(row, colNames, kUserNameCols);
    info.full_sql = FindColumnValue(row, colNames, kSqlPreviewCols);
    info.sql_preview = TruncateSql(info.full_sql, 80);
    info.start_time = FindColumnValue(row, colNames, kStartTimeCols);
    info.elapsed_time = FindColumnValue(row, colNames, kElapsedTimeCols);
    info.rows_affected = FindColumnValue(row, colNames, kRowsAffectedCols);
    info.status = FindColumnValue(row, colNames, kStatusCols);
    info.transaction_id = FindColumnValue(row, colNames, kTransactionIdCols);
    info.wait_event = FindColumnValue(row, colNames, kWaitEventCols);
    info.wait_resource = FindColumnValue(row, colNames, kWaitResourceCols);
    return info;
}

std::string StatementsPanel::FindColumnValue(const std::vector<QueryValue>& row,
                                             const std::vector<std::string>& colNames,
                                             const std::vector<std::string>& possibleNames) const {
    int idx = FindColumnIndex(colNames, possibleNames);
    if (idx >= 0 && idx < static_cast<int>(row.size())) {
        return row[idx].isNull ? "" : row[idx].text;
    }
    return "";
}

int StatementsPanel::FindColumnIndex(const std::vector<std::string>& colNames,
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

std::string StatementsPanel::TruncateSql(const std::string& sql, size_t maxLength) const {
    if (sql.length() <= maxLength) {
        return sql;
    }
    return sql.substr(0, maxLength - 3) + "...";
}

void StatementsPanel::SetAutoRefresh(bool enable, int intervalSeconds) {
    if (!auto_refresh_check_ || !interval_choice_ || !refresh_timer_) {
        return;
    }

    auto_refresh_check_->SetValue(enable);
    interval_choice_->Enable(enable);

    if (enable) {
        int intervalMs = intervalSeconds * 1000;
        refresh_timer_->Start(intervalMs);
    } else {
        refresh_timer_->Stop();
    }
}

bool StatementsPanel::IsAutoRefreshEnabled() const {
    return auto_refresh_check_ && auto_refresh_check_->IsChecked();
}

void StatementsPanel::CancelSelectedStatement() {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(statements_.size())) {
        return;
    }

    const auto& stmt = statements_[selected_row_];
    wxString msg = wxString::Format(
        "Are you sure you want to cancel statement %s?\n\n"
        "Session: %s\n"
        "User: %s\n"
        "SQL: %s",
        stmt.statement_id, stmt.session_id, stmt.user_name, stmt.sql_preview);

    int result = wxMessageBox(msg, "Confirm Cancel Statement",
                              wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }

    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }

    // Execute cancel statement command
    std::string sql = "CALL sys.cancel_statement('" + stmt.statement_id + "');";
    cancel_job_ = connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Statement cancelled");
                    RefreshData();  // Refresh to show updated list
                } else {
                    UpdateStatus("Cancel failed");
                    wxMessageBox(error.empty() ? "Failed to cancel statement" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void StatementsPanel::ShowExecutionPlan() {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(statements_.size())) {
        return;
    }

    const auto& stmt = statements_[selected_row_];
    if (stmt.full_sql.empty()) {
        wxMessageBox("No SQL statement available for execution plan.",
                     "Execution Plan", wxOK | wxICON_INFORMATION);
        return;
    }

    // Execute EXPLAIN to get execution plan
    std::string explainSql = "EXPLAIN " + stmt.full_sql;
    connection_manager_->ExecuteQueryAsync(explainSql,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                if (ok) {
                    // Format execution plan
                    std::string plan;
                    for (const auto& row : result.rows) {
                        for (const auto& col : row) {
                            if (!col.isNull) {
                                plan += col.text + "\n";
                            }
                        }
                    }
                    if (plan.empty()) {
                        plan = "Execution plan not available.";
                    }
                    wxMessageBox(wxString::FromUTF8(plan), "Execution Plan",
                                 wxOK | wxICON_INFORMATION);
                } else {
                    wxMessageBox(error.empty() ? "Failed to get execution plan" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void StatementsPanel::ShowStatementDetails() {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(statements_.size())) {
        return;
    }

    const auto& stmt = statements_[selected_row_];
    wxString details = wxString::Format(
        "Statement Details:\n\n"
        "Statement ID: %s\n"
        "Session ID: %s\n"
        "User: %s\n"
        "Transaction ID: %s\n"
        "Start Time: %s\n"
        "Elapsed Time: %s\n"
        "Rows Affected: %s\n"
        "Status: %s\n"
        "Wait Event: %s\n"
        "Wait Resource: %s\n\n"
        "Full SQL:\n%s",
        stmt.statement_id, stmt.session_id, stmt.user_name,
        stmt.transaction_id, stmt.start_time, stmt.elapsed_time,
        stmt.rows_affected, stmt.status, stmt.wait_event,
        stmt.wait_resource,
        stmt.full_sql.empty() ? "(none)" : stmt.full_sql);

    wxMessageBox(details, "Statement Details", wxOK | wxICON_INFORMATION);
}

void StatementsPanel::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool has_selection = selected_row_ >= 0 && selected_row_ < static_cast<int>(statements_.size());

    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
    if (cancel_button_) {
        cancel_button_->Enable(connected && has_selection && !query_running_);
    }
    if (plan_button_) {
        plan_button_->Enable(connected && has_selection);
    }
    if (details_button_) {
        details_button_->Enable(has_selection);
    }
}

void StatementsPanel::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void StatementsPanel::OnRefresh(wxCommandEvent&) {
    RefreshData();
}

void StatementsPanel::OnCancelStatement(wxCommandEvent&) {
    CancelSelectedStatement();
}

void StatementsPanel::OnShowExecutionPlan(wxCommandEvent&) {
    ShowExecutionPlan();
}

void StatementsPanel::OnShowDetails(wxCommandEvent&) {
    ShowStatementDetails();
}

void StatementsPanel::OnAutoRefreshToggle(wxCommandEvent&) {
    bool enable = auto_refresh_check_->IsChecked();
    interval_choice_->Enable(enable);

    if (enable) {
        int intervals[] = {5, 10, 30, 60};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 4) {
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    } else {
        refresh_timer_->Stop();
    }
}

void StatementsPanel::OnIntervalChanged(wxCommandEvent&) {
    if (auto_refresh_check_->IsChecked()) {
        int intervals[] = {5, 10, 30, 60};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 4) {
            refresh_timer_->Stop();
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    }
}

void StatementsPanel::OnGridSelect(wxGridEvent& event) {
    selected_row_ = event.GetRow();
    UpdateControls();
}

void StatementsPanel::OnGridDoubleClick(wxGridEvent& event) {
    selected_row_ = event.GetRow();
    UpdateControls();
    // Show details on double-click
    ShowStatementDetails();
}

void StatementsPanel::OnTimer(wxTimerEvent&) {
    if (!query_running_) {
        LoadStatements();
    }
}

} // namespace scratchrobin
