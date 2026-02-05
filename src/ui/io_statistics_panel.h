/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_IO_STATISTICS_PANEL_H
#define SCRATCHROBIN_IO_STATISTICS_PANEL_H

#include <string>
#include <vector>

#include <wx/panel.h>
#include <wx/datetime.h>

#include "core/connection_manager.h"
#include "core/query_types.h"

class wxBookCtrlEvent;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxGrid;
class wxGridEvent;
class wxNotebook;
class wxStaticText;
class wxTimer;
class wxTimerEvent;

namespace scratchrobin {

class ResultGridTable;

// Time range options for statistics
enum class TimeRange {
    LastHour,
    Last24Hours,
    Last7Days,
    Custom
};

// Data structure for database I/O statistics
struct DatabaseIOStats {
    std::string database_name;
    std::string reads;
    std::string writes;
    std::string read_bytes;
    std::string write_bytes;
    std::string read_time_ms;
    std::string write_time_ms;
    std::string avg_read_time_ms;
    std::string avg_write_time_ms;
};

// Data structure for table I/O statistics
struct TableIOStats {
    std::string schema_name;
    std::string table_name;
    std::string heap_reads;
    std::string heap_writes;
    std::string idx_reads;
    std::string idx_writes;
    std::string toast_reads;
    std::string toast_writes;
};

// Data structure for index I/O statistics
struct IndexIOStats {
    std::string schema_name;
    std::string table_name;
    std::string index_name;
    std::string idx_reads;
    std::string idx_writes;
};

// Panel for monitoring I/O statistics with notebook tabs
class IOStatisticsPanel : public wxPanel {
public:
    IOStatisticsPanel(wxWindow* parent,
                      ConnectionManager* connectionManager);

    void RefreshData();
    void SetAutoRefresh(bool enable, int intervalSeconds = 60);
    bool IsAutoRefreshEnabled() const;
    void SetTimeRange(TimeRange range);
    TimeRange GetTimeRange() const;

private:
    void BuildLayout();
    void UpdateControls();
    void UpdateStatus(const std::string& message);
    void UpdateSummary();

    // Data loading methods
    void LoadDatabaseIO();
    void LoadTableIO();
    void LoadIndexIO();

    // Parsing methods
    void ParseDatabaseIO(const QueryResult& result);
    void ParseTableIO(const QueryResult& result);
    void ParseIndexIO(const QueryResult& result);

    // Helper methods
    std::string FindColumnValue(const std::vector<QueryValue>& row,
                                const std::vector<std::string>& colNames,
                                const std::vector<std::string>& possibleNames) const;
    int FindColumnIndex(const std::vector<std::string>& colNames,
                        const std::vector<std::string>& possibleNames) const;
    std::string FormatBytes(const std::string& bytes) const;
    std::string CalculateAverage(const std::string& total_time,
                                  const std::string& count) const;

    // Export methods
    void ExportToCSV();
    void ExportToJSON();
    std::string GetCurrentTabName() const;

    // Custom date range dialog
    bool ShowCustomDateRangeDialog();

    // Event handlers
    void OnRefresh(wxCommandEvent& event);
    void OnAutoRefreshToggle(wxCommandEvent& event);
    void OnIntervalChanged(wxCommandEvent& event);
    void OnTimeRangeChanged(wxCommandEvent& event);
    void OnExportCSV(wxCommandEvent& event);
    void OnExportJSON(wxCommandEvent& event);
    void OnNotebookPageChanged(wxBookCtrlEvent& event);
    void OnGridSelect(wxGridEvent& event);
    void OnTimer(wxTimerEvent& event);

    ConnectionManager* connection_manager_ = nullptr;

    // UI Controls - Toolbar
    wxButton* refresh_button_ = nullptr;
    wxCheckBox* auto_refresh_check_ = nullptr;
    wxChoice* interval_choice_ = nullptr;
    wxChoice* time_range_choice_ = nullptr;
    wxButton* export_csv_button_ = nullptr;
    wxButton* export_json_button_ = nullptr;
    wxStaticText* status_label_ = nullptr;

    // UI Controls - Notebook and tabs
    wxNotebook* notebook_ = nullptr;

    // Database I/O tab controls
    wxPanel* database_tab_ = nullptr;
    wxGrid* database_grid_ = nullptr;
    ResultGridTable* database_table_ = nullptr;
    wxStaticText* database_summary_label_ = nullptr;

    // Table I/O tab controls
    wxPanel* table_tab_ = nullptr;
    wxGrid* table_grid_ = nullptr;
    ResultGridTable* table_table_ = nullptr;
    wxStaticText* table_summary_label_ = nullptr;

    // Index I/O tab controls
    wxPanel* index_tab_ = nullptr;
    wxGrid* index_grid_ = nullptr;
    ResultGridTable* index_table_ = nullptr;
    wxStaticText* index_summary_label_ = nullptr;

    // Summary panel at bottom
    wxStaticText* summary_label_ = nullptr;

    // Timer for auto-refresh
    wxTimer* refresh_timer_ = nullptr;

    // Data
    std::vector<DatabaseIOStats> database_io_stats_;
    std::vector<TableIOStats> table_io_stats_;
    std::vector<IndexIOStats> index_io_stats_;
    TimeRange current_time_range_ = TimeRange::LastHour;
    wxDateTime custom_start_date_;
    wxDateTime custom_end_date_;
    int current_tab_ = 0;  // 0=Database, 1=Table, 2=Index
    bool query_running_ = false;
    JobHandle query_job_;

    // Column definitions for grids
    static const std::vector<std::string> kDatabaseColumnLabels;
    static const std::vector<std::string> kTableColumnLabels;
    static const std::vector<std::string> kIndexColumnLabels;

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_IO_STATISTICS_PANEL_H
