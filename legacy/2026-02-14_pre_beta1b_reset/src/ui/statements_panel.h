/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_STATEMENTS_PANEL_H
#define SCRATCHROBIN_STATEMENTS_PANEL_H

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

// Data structure for statement information
struct StatementInfo {
    std::string statement_id;
    std::string session_id;
    std::string user_name;
    std::string sql_preview;
    std::string full_sql;
    std::string start_time;
    std::string elapsed_time;
    std::string rows_affected;
    std::string status;
    std::string transaction_id;
    std::string wait_event;
    std::string wait_resource;
};

// Panel for monitoring running SQL statements
class StatementsPanel : public wxPanel {
public:
    StatementsPanel(wxWindow* parent,
                    ConnectionManager* connectionManager);

    void RefreshData();
    void SetAutoRefresh(bool enable, int intervalSeconds = 10);
    bool IsAutoRefreshEnabled() const;

private:
    void BuildLayout();
    void UpdateControls();
    void UpdateStatus(const std::string& message);

    void LoadStatements();
    void ParseStatements(const QueryResult& result);
    StatementInfo ExtractStatementInfo(const std::vector<QueryValue>& row,
                                       const std::vector<std::string>& colNames);
    std::string FindColumnValue(const std::vector<QueryValue>& row,
                                const std::vector<std::string>& colNames,
                                const std::vector<std::string>& possibleNames) const;
    int FindColumnIndex(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& possibleNames) const;
    std::string TruncateSql(const std::string& sql, size_t maxLength = 80) const;

    void CancelSelectedStatement();
    void ShowExecutionPlan();
    void ShowStatementDetails();

    void OnRefresh(wxCommandEvent& event);
    void OnCancelStatement(wxCommandEvent& event);
    void OnShowExecutionPlan(wxCommandEvent& event);
    void OnShowDetails(wxCommandEvent& event);
    void OnAutoRefreshToggle(wxCommandEvent& event);
    void OnIntervalChanged(wxCommandEvent& event);
    void OnGridSelect(wxGridEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnGridDoubleClick(wxGridEvent& event);

    ConnectionManager* connection_manager_ = nullptr;

    // UI Controls
    wxGrid* statements_grid_ = nullptr;
    ResultGridTable* grid_table_ = nullptr;
    wxButton* refresh_button_ = nullptr;
    wxButton* cancel_button_ = nullptr;
    wxButton* plan_button_ = nullptr;
    wxButton* details_button_ = nullptr;
    wxCheckBox* auto_refresh_check_ = nullptr;
    wxChoice* interval_choice_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* count_label_ = nullptr;

    // Timer for auto-refresh
    wxTimer* refresh_timer_ = nullptr;

    // Data
    std::vector<StatementInfo> statements_;
    int selected_row_ = -1;
    bool query_running_ = false;
    JobHandle query_job_;
    JobHandle cancel_job_;

    // Column definitions for the grid
    static const std::vector<std::string> kColumnNames;
    static const std::vector<std::string> kColumnLabels;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STATEMENTS_PANEL_H
