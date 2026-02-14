/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_LOCKS_PANEL_H
#define SCRATCHROBIN_LOCKS_PANEL_H

#include <string>
#include <vector>

#include <wx/panel.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxButton;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxStaticText;

namespace scratchrobin {

class ResultGridTable;

// Filter types for lock display
enum class LockFilter {
    All,
    BlockingOnly,
    WaitingOnly
};

// Data structure for lock information
struct LockInfo {
    std::string lock_id;
    std::string object_type;
    std::string object_name;
    std::string lock_mode;
    std::string session_id;
    std::string user_name;
    std::string granted_time;
    std::string wait_time;
    std::string lock_state;
    std::string transaction_id;
    std::string relation_name;
    bool is_granted = false;
    bool is_waiting = false;
};

// Panel for monitoring database locks
class LocksPanel : public wxPanel {
public:
    LocksPanel(wxWindow* parent,
               ConnectionManager* connectionManager);

    void RefreshData();
    void SetFilter(LockFilter filter);
    LockFilter GetFilter() const;

private:
    void BuildLayout();
    void UpdateControls();
    void UpdateStatus(const std::string& message);

    void LoadLocks();
    void ParseLocks(const QueryResult& result);
    LockInfo ExtractLockInfo(const std::vector<QueryValue>& row,
                             const std::vector<std::string>& colNames);
    std::string FindColumnValue(const std::vector<QueryValue>& row,
                                const std::vector<std::string>& colNames,
                                const std::vector<std::string>& possibleNames) const;
    int FindColumnIndex(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& possibleNames) const;

    void ApplyFilter();
    bool MatchesFilter(const LockInfo& lock) const;

    void BuildDependencyGraph();
    std::string GenerateDependencyDotGraph() const;

    void OnRefresh(wxCommandEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnShowDependencyGraph(wxCommandEvent& event);
    void OnGridSelect(wxGridEvent& event);

    ConnectionManager* connection_manager_ = nullptr;

    // UI Controls
    wxGrid* locks_grid_ = nullptr;
    ResultGridTable* grid_table_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* graph_button_ = nullptr;
    wxChoice* filter_choice_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* count_label_ = nullptr;

    // Data
    std::vector<LockInfo> all_locks_;
    std::vector<LockInfo> filtered_locks_;
    LockFilter current_filter_ = LockFilter::All;
    int selected_row_ = -1;
    bool query_running_ = false;
    JobHandle query_job_;

    // Column definitions for the grid
    static const std::vector<std::string> kColumnNames;
    static const std::vector<std::string> kColumnLabels;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LOCKS_PANEL_H
