/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "locks_panel.h"

#include "result_grid_table.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>

namespace scratchrobin {

namespace {

constexpr int kRefreshButtonId = wxID_HIGHEST + 200;
constexpr int kGraphButtonId = wxID_HIGHEST + 201;
constexpr int kFilterChoiceId = wxID_HIGHEST + 202;

// Column mapping for different backends
const std::vector<std::string> kLockIdCols = {"lock_id", "LOCK_ID", "MON$LOCK_ID"};
const std::vector<std::string> kObjectTypeCols = {"lock_type", "object_type", "LOCK_TYPE", "MON$LOCK_TYPE"};
const std::vector<std::string> kObjectNameCols = {"object_name", "relation_name", "relname", "LOCK_DATA", "MON$OBJECT_NAME"};
const std::vector<std::string> kLockModeCols = {"lock_mode", "mode", "LOCK_MODE", "MON$LOCK_MODE"};
const std::vector<std::string> kSessionIdCols = {"session_id", "pid", "MON$ATTACHMENT_ID"};
const std::vector<std::string> kUserNameCols = {"user_name", "usename", "MON$USER"};
const std::vector<std::string> kGrantedTimeCols = {"granted_at", "granted_time", "wait_start"};
const std::vector<std::string> kWaitTimeCols = {"wait_duration", "wait_time", "wait_start"};
const std::vector<std::string> kLockStateCols = {"lock_state", "granted", "LOCK_STATUS", "MON$LOCK_STATE"};
const std::vector<std::string> kTransactionIdCols = {"transaction_id", "MON$TRANSACTION_ID"};
const std::vector<std::string> kRelationNameCols = {"relation_name", "relname", "nspname"};

} // namespace

const std::vector<std::string> LocksPanel::kColumnNames = {
    "lock_id", "object_type", "object_name", "lock_mode",
    "session_id", "user_name", "granted_time", "wait_time"
};

const std::vector<std::string> LocksPanel::kColumnLabels = {
    "Lock ID", "Object Type", "Object Name", "Lock Mode",
    "Session", "User", "Granted Time", "Wait Time"
};

wxBEGIN_EVENT_TABLE(LocksPanel, wxPanel)
    EVT_BUTTON(kRefreshButtonId, LocksPanel::OnRefresh)
    EVT_BUTTON(kGraphButtonId, LocksPanel::OnShowDependencyGraph)
    EVT_CHOICE(kFilterChoiceId, LocksPanel::OnFilterChanged)
    EVT_GRID_SELECT_CELL(LocksPanel::OnGridSelect)
wxEND_EVENT_TABLE()

LocksPanel::LocksPanel(wxWindow* parent,
                       ConnectionManager* connectionManager)
    : wxPanel(parent, wxID_ANY),
      connection_manager_(connectionManager) {
    BuildLayout();
    UpdateControls();
    UpdateStatus("Ready");
}

void LocksPanel::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar panel
    auto* toolbar = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    refresh_button_ = new wxButton(toolbar, kRefreshButtonId, "Refresh");
    toolbarSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    graph_button_ = new wxButton(toolbar, kGraphButtonId, "Show Dependency Graph");
    toolbarSizer->Add(graph_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->Add(new wxStaticText(toolbar, wxID_ANY, "Filter:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    filter_choice_ = new wxChoice(toolbar, kFilterChoiceId);
    filter_choice_->Append("All Locks");
    filter_choice_->Append("Blocking Only");
    filter_choice_->Append("Waiting Only");
    filter_choice_->SetSelection(0);
    toolbarSizer->Add(filter_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->AddStretchSpacer(1);

    count_label_ = new wxStaticText(toolbar, wxID_ANY, "0 locks");
    toolbarSizer->Add(count_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    status_label_ = new wxStaticText(toolbar, wxID_ANY, "Ready");
    toolbarSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->SetSizer(toolbarSizer);
    rootSizer->Add(toolbar, 0, wxEXPAND | wxALL, 8);

    // Grid panel
    auto* gridPanel = new wxPanel(this, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);

    locks_grid_ = new wxGrid(gridPanel, wxID_ANY);
    grid_table_ = new ResultGridTable();
    locks_grid_->SetTable(grid_table_, true);
    locks_grid_->EnableEditing(false);
    locks_grid_->SetRowLabelSize(48);
    locks_grid_->EnableGridLines(true);

    gridSizer->Add(locks_grid_, 1, wxEXPAND | wxALL, 8);
    gridPanel->SetSizer(gridSizer);
    rootSizer->Add(gridPanel, 1, wxEXPAND);

    SetSizer(rootSizer);
}

void LocksPanel::RefreshData() {
    LoadLocks();
}

void LocksPanel::LoadLocks() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading locks...");

    // Query for current locks
    const std::string query = "SELECT * FROM sys.locks ORDER BY lock_id;";

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseLocks(result);
                    ApplyFilter();
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load locks" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
            });
        });
}

void LocksPanel::ParseLocks(const QueryResult& result) {
    all_locks_.clear();

    // Build column name list
    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    // Parse each row
    for (const auto& row : result.rows) {
        all_locks_.push_back(ExtractLockInfo(row, colNames));
    }
}

LockInfo LocksPanel::ExtractLockInfo(const std::vector<QueryValue>& row,
                                     const std::vector<std::string>& colNames) {
    LockInfo info;
    info.lock_id = FindColumnValue(row, colNames, kLockIdCols);
    info.object_type = FindColumnValue(row, colNames, kObjectTypeCols);
    info.object_name = FindColumnValue(row, colNames, kObjectNameCols);
    info.lock_mode = FindColumnValue(row, colNames, kLockModeCols);
    info.session_id = FindColumnValue(row, colNames, kSessionIdCols);
    info.user_name = FindColumnValue(row, colNames, kUserNameCols);
    info.granted_time = FindColumnValue(row, colNames, kGrantedTimeCols);
    info.wait_time = FindColumnValue(row, colNames, kWaitTimeCols);
    info.lock_state = FindColumnValue(row, colNames, kLockStateCols);
    info.transaction_id = FindColumnValue(row, colNames, kTransactionIdCols);
    info.relation_name = FindColumnValue(row, colNames, kRelationNameCols);

    // Determine if granted/waiting
    std::string state = info.lock_state;
    std::transform(state.begin(), state.end(), state.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (state == "granted" || state == "true" || state == "1" || state == "t") {
        info.is_granted = true;
        info.is_waiting = false;
    } else if (state == "waiting" || state == "false" || state == "0" || state == "f") {
        info.is_granted = false;
        info.is_waiting = true;
    } else {
        // Check wait_time to determine waiting status
        info.is_waiting = !info.wait_time.empty() && info.wait_time != "0";
        info.is_granted = !info.is_waiting;
    }

    return info;
}

std::string LocksPanel::FindColumnValue(const std::vector<QueryValue>& row,
                                        const std::vector<std::string>& colNames,
                                        const std::vector<std::string>& possibleNames) const {
    int idx = FindColumnIndex(colNames, possibleNames);
    if (idx >= 0 && idx < static_cast<int>(row.size())) {
        return row[idx].isNull ? "" : row[idx].text;
    }
    return "";
}

int LocksPanel::FindColumnIndex(const std::vector<std::string>& colNames,
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

void LocksPanel::SetFilter(LockFilter filter) {
    current_filter_ = filter;
    if (filter_choice_) {
        switch (filter) {
            case LockFilter::All:
                filter_choice_->SetSelection(0);
                break;
            case LockFilter::BlockingOnly:
                filter_choice_->SetSelection(1);
                break;
            case LockFilter::WaitingOnly:
                filter_choice_->SetSelection(2);
                break;
        }
    }
    ApplyFilter();
}

LockFilter LocksPanel::GetFilter() const {
    return current_filter_;
}

void LocksPanel::ApplyFilter() {
    filtered_locks_.clear();

    for (const auto& lock : all_locks_) {
        if (MatchesFilter(lock)) {
            filtered_locks_.push_back(lock);
        }
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

    for (const auto& lock : filtered_locks_) {
        std::vector<QueryValue> row;
        row.push_back({false, lock.lock_id, {}});
        row.push_back({false, lock.object_type, {}});
        row.push_back({false, lock.object_name.empty() ? lock.relation_name : lock.object_name, {}});
        row.push_back({false, lock.lock_mode, {}});
        row.push_back({false, lock.session_id, {}});
        row.push_back({false, lock.user_name, {}});
        row.push_back({false, lock.granted_time, {}});
        row.push_back({false, lock.wait_time, {}});
        gridRows.push_back(row);
    }

    if (grid_table_) {
        grid_table_->Reset(columns, gridRows);
    }

    // Update count label
    wxString filterText;
    switch (current_filter_) {
        case LockFilter::All:
            filterText = "total";
            break;
        case LockFilter::BlockingOnly:
            filterText = "blocking";
            break;
        case LockFilter::WaitingOnly:
            filterText = "waiting";
            break;
    }
    if (count_label_) {
        count_label_->SetLabel(wxString::Format("%zu %s locks (%zu total)",
                                                filtered_locks_.size(),
                                                filterText,
                                                all_locks_.size()));
    }
}

bool LocksPanel::MatchesFilter(const LockInfo& lock) const {
    switch (current_filter_) {
        case LockFilter::All:
            return true;
        case LockFilter::BlockingOnly:
            // A lock is blocking if it's granted and there's another session waiting
            // For simplicity, we consider granted locks as potentially blocking
            return lock.is_granted;
        case LockFilter::WaitingOnly:
            return lock.is_waiting;
    }
    return true;
}

void LocksPanel::BuildDependencyGraph() {
    // This would typically open a dialog with a visual graph
    // For now, show the DOT representation in a message box
    wxString dotGraph = wxString::FromUTF8(GenerateDependencyDotGraph());

    wxMessageBox(dotGraph, "Lock Dependency Graph (DOT Format)",
                 wxOK | wxICON_INFORMATION);
}

std::string LocksPanel::GenerateDependencyDotGraph() const {
    std::ostringstream dot;
    dot << "digraph LockDependencies {\n";
    dot << "  rankdir=TB;\n";
    dot << "  node [shape=box, style=filled, fillcolor=lightblue];\n\n";

    // Add nodes for each unique session
    std::set<std::string> sessions;
    for (const auto& lock : filtered_locks_) {
        sessions.insert(lock.session_id);
    }

    for (const auto& session : sessions) {
        dot << "  \"session_" << session << "\" [label=\"Session " << session << "\"];\n";
    }
    dot << "\n";

    // Add nodes for each lock
    for (const auto& lock : filtered_locks_) {
        std::string color = lock.is_waiting ? "orange" : "lightgreen";
        dot << "  \"lock_" << lock.lock_id << "\" [label=\"" 
            << lock.object_type << "\\n" << lock.lock_mode 
            << "\", fillcolor=" << color << "];\n";
    }
    dot << "\n";

    // Add edges from sessions to locks
    for (const auto& lock : filtered_locks_) {
        dot << "  \"session_" << lock.session_id << "\" -> \"lock_" 
            << lock.lock_id << "\";\n";
    }
    dot << "\n";

    // Add edges for lock dependencies (simplified)
    // In a real implementation, you'd analyze which locks block which

    dot << "}\n";
    return dot.str();
}

void LocksPanel::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();

    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
    if (graph_button_) {
        graph_button_->Enable(!filtered_locks_.empty());
    }
}

void LocksPanel::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void LocksPanel::OnRefresh(wxCommandEvent&) {
    RefreshData();
}

void LocksPanel::OnFilterChanged(wxCommandEvent&) {
    int selection = filter_choice_->GetSelection();
    switch (selection) {
        case 0:
            current_filter_ = LockFilter::All;
            break;
        case 1:
            current_filter_ = LockFilter::BlockingOnly;
            break;
        case 2:
            current_filter_ = LockFilter::WaitingOnly;
            break;
    }
    ApplyFilter();
}

void LocksPanel::OnShowDependencyGraph(wxCommandEvent&) {
    BuildDependencyGraph();
}

void LocksPanel::OnGridSelect(wxGridEvent& event) {
    selected_row_ = event.GetRow();
}

} // namespace scratchrobin
