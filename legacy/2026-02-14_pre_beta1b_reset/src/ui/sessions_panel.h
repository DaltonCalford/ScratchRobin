/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SESSIONS_PANEL_H
#define SCRATCHROBIN_SESSIONS_PANEL_H

#include <string>
#include <vector>

#include <wx/panel.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxButton;
class wxCheckBox;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxStaticText;
class wxTimer;
class wxTimerEvent;

namespace scratchrobin {

class ResultGridTable;

// Data structure for session information
struct SessionInfo {
    std::string session_id;
    std::string user_name;
    std::string database;
    std::string application;
    std::string client_addr;
    std::string login_time;
    std::string last_activity;
    std::string status;
    std::string transaction_id;
    std::string statement_id;
    std::string current_query;
    std::string wait_event;
    std::string wait_resource;
};

// Panel for monitoring active database sessions
class SessionsPanel : public wxPanel {
public:
    SessionsPanel(wxWindow* parent,
                  ConnectionManager* connectionManager);

    void RefreshData();
    void SetAutoRefresh(bool enable, int intervalSeconds = 30);
    bool IsAutoRefreshEnabled() const;

private:
    void BuildLayout();
    void UpdateControls();
    void UpdateStatus(const std::string& message);

    void LoadSessions();
    void ParseSessions(const QueryResult& result);
    SessionInfo ExtractSessionInfo(const std::vector<QueryValue>& row,
                                   const std::vector<std::string>& colNames);
    std::string FindColumnValue(const std::vector<QueryValue>& row,
                                const std::vector<std::string>& colNames,
                                const std::vector<std::string>& possibleNames) const;
    int FindColumnIndex(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& possibleNames) const;

    void OnRefresh(wxCommandEvent& event);
    void OnKillSession(wxCommandEvent& event);
    void OnShowDetails(wxCommandEvent& event);
    void OnAutoRefreshToggle(wxCommandEvent& event);
    void OnIntervalChanged(wxCommandEvent& event);
    void OnGridSelect(wxGridEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnGridDoubleClick(wxGridEvent& event);

    ConnectionManager* connection_manager_ = nullptr;

    // UI Controls
    wxGrid* sessions_grid_ = nullptr;
    ResultGridTable* grid_table_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* kill_button_ = nullptr;
    wxButton* details_button_ = nullptr;
    wxCheckBox* auto_refresh_check_ = nullptr;
    wxChoice* interval_choice_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* count_label_ = nullptr;

    // Timer for auto-refresh
    wxTimer* refresh_timer_ = nullptr;

    // Data
    std::vector<SessionInfo> sessions_;
    int selected_row_ = -1;
    bool query_running_ = false;
    JobHandle query_job_;

    // Column definitions for the grid
    static const std::vector<std::string> kColumnNames;
    static const std::vector<std::string> kColumnLabels;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SESSIONS_PANEL_H
