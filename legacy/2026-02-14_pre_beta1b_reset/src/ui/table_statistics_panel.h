/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TABLE_STATISTICS_PANEL_H
#define SCRATCHROBIN_TABLE_STATISTICS_PANEL_H

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
class wxTextCtrl;
class wxTimer;
class wxTimerEvent;

namespace scratchrobin {

class ResultGridTable;

// Sort options for table statistics
enum class TableSortOrder {
    BySizeDesc,
    BySizeAsc,
    ByRowCountDesc,
    ByRowCountAsc,
    ByScanCountDesc,
    ByScanCountAsc,
    ByModificationTimeDesc
};

// Data structure for table statistics
struct TableStatisticsInfo {
    std::string schema_name;
    std::string table_name;
    std::string row_count;
    std::string table_size;
    std::string index_size;
    std::string total_size;
    std::string seq_scans;
    std::string idx_scans;
    std::string n_tup_ins;
    std::string n_tup_upd;
    std::string n_tup_del;
    std::string last_vacuum;
    std::string last_analyze;
    // Raw values for sorting/filtering
    long long row_count_raw = 0;
    long long total_size_raw = 0;
    long long scan_count_raw = 0;
};

// Panel for displaying table statistics
class TableStatisticsPanel : public wxPanel {
public:
    TableStatisticsPanel(wxWindow* parent,
                         ConnectionManager* connectionManager);
    ~TableStatisticsPanel();

    void RefreshData();
    void SetAutoRefresh(bool enable, int intervalSeconds = 60);
    bool IsAutoRefreshEnabled() const;

private:
    void BuildLayout();
    void UpdateControls();
    void UpdateStatus(const std::string& message);
    void UpdateSummary();
    void UpdateGrid();

    void LoadStatistics();
    void ParseStatistics(const QueryResult& result);
    TableStatisticsInfo ExtractStatisticsInfo(const std::vector<QueryValue>& row,
                                              const std::vector<std::string>& colNames);
    std::string FindColumnValue(const std::vector<QueryValue>& row,
                                const std::vector<std::string>& colNames,
                                const std::vector<std::string>& possibleNames) const;
    int FindColumnIndex(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& possibleNames) const;
    long long ParseSizeToBytes(const std::string& sizeStr) const;
    long long ParseCount(const std::string& countStr) const;

    void ApplyFilters();
    bool MatchesFilters(const TableStatisticsInfo& info) const;
    void ApplySort();
    bool CompareStatistics(const TableStatisticsInfo& a, 
                           const TableStatisticsInfo& b) const;

    void ExportToFile();
    void AnalyzeSelected();
    void VacuumSelected();
    void AnalyzeAll();
    void VacuumAll();

    void OnRefresh(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnAnalyze(wxCommandEvent& event);
    void OnVacuum(wxCommandEvent& event);
    void OnAutoRefreshToggle(wxCommandEvent& event);
    void OnIntervalChanged(wxCommandEvent& event);
    void OnSortChanged(wxCommandEvent& event);
    void OnFilterChanged(wxCommandEvent& event);
    void OnGridSelect(wxGridEvent& event);
    void OnTimer(wxTimerEvent& event);

    ConnectionManager* connection_manager_ = nullptr;

    // UI Controls - Filter panel
    wxTextCtrl* schema_filter_ = nullptr;
    wxTextCtrl* table_filter_ = nullptr;
    wxTextCtrl* size_threshold_filter_ = nullptr;
    wxChoice* sort_choice_ = nullptr;
    
    // UI Controls - Toolbar
    wxButton* refresh_button_ = nullptr;
    wxButton* export_button_ = nullptr;
    wxButton* analyze_button_ = nullptr;
    wxButton* vacuum_button_ = nullptr;
    wxCheckBox* auto_refresh_check_ = nullptr;
    wxChoice* interval_choice_ = nullptr;
    
    // UI Controls - Summary panel
    wxStaticText* total_tables_label_ = nullptr;
    wxStaticText* total_size_label_ = nullptr;
    wxStaticText* total_rows_label_ = nullptr;
    
    // UI Controls - Grid and status
    wxGrid* statistics_grid_ = nullptr;
    ResultGridTable* grid_table_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* count_label_ = nullptr;

    // Timer for auto-refresh
    wxTimer* refresh_timer_ = nullptr;

    // Data
    std::vector<TableStatisticsInfo> all_statistics_;
    std::vector<TableStatisticsInfo> filtered_statistics_;
    TableSortOrder current_sort_ = TableSortOrder::BySizeDesc;
    int selected_row_ = -1;
    bool query_running_ = false;
    JobHandle query_job_;
    JobHandle maintenance_job_;

    // Column definitions for the grid
    static const std::vector<std::string> kColumnNames;
    static const std::vector<std::string> kColumnLabels;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TABLE_STATISTICS_PANEL_H
