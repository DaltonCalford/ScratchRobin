/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "io_statistics_panel.h"

#include "result_grid_table.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>

namespace scratchrobin {

namespace {

constexpr int kRefreshButtonId = wxID_HIGHEST + 400;
constexpr int kAutoRefreshCheckId = wxID_HIGHEST + 401;
constexpr int kIntervalChoiceId = wxID_HIGHEST + 402;
constexpr int kTimeRangeChoiceId = wxID_HIGHEST + 403;
constexpr int kExportCSVButtonId = wxID_HIGHEST + 404;
constexpr int kExportJSONButtonId = wxID_HIGHEST + 405;
constexpr int kTimerId = wxID_HIGHEST + 406;

// SQL queries for I/O statistics
const std::string kDatabaseIOQuery =
    "SELECT database_name, reads, writes, "
    "       read_bytes, write_bytes, "
    "       read_time_ms, write_time_ms "
    "FROM sb_catalog.sb_database_io_stats "
    "ORDER BY reads + writes DESC;";

const std::string kTableIOQuery =
    "SELECT schema_name, table_name, "
    "       heap_reads, heap_writes, "
    "       idx_reads, idx_writes "
    "FROM sb_catalog.sb_table_io_stats "
    "ORDER BY heap_reads + heap_writes + idx_reads + idx_writes DESC;";

const std::string kIndexIOQuery =
    "SELECT schema_name, table_name, index_name, "
    "       idx_reads, idx_writes "
    "FROM sb_catalog.sb_index_io_stats "
    "ORDER BY idx_reads + idx_writes DESC;";

// Column mapping for different backends - Database I/O
const std::vector<std::string> kDatabaseNameCols = {
    "database_name", "DATABASE_NAME", "DB_NAME", "datname"
};
const std::vector<std::string> kReadsCols = {
    "reads", "READS", "blk_read_time", "physical_reads"
};
const std::vector<std::string> kWritesCols = {
    "writes", "WRITES", "blk_write_time", "physical_writes"
};
const std::vector<std::string> kReadBytesCols = {
    "read_bytes", "READ_BYTES", "bytes_read"
};
const std::vector<std::string> kWriteBytesCols = {
    "write_bytes", "WRITE_BYTES", "bytes_written"
};
const std::vector<std::string> kReadTimeCols = {
    "read_time_ms", "READ_TIME_MS", "read_time", "blk_read_time"
};
const std::vector<std::string> kWriteTimeCols = {
    "write_time_ms", "WRITE_TIME_MS", "write_time", "blk_write_time"
};

// Column mapping - Table I/O
const std::vector<std::string> kSchemaNameCols = {
    "schema_name", "SCHEMA_NAME", "schemaname", "nspname"
};
const std::vector<std::string> kTableNameCols = {
    "table_name", "TABLE_NAME", "relname", "tablename"
};
const std::vector<std::string> kHeapReadsCols = {
    "heap_reads", "HEAP_READS", "heap_blks_read", "seq_reads"
};
const std::vector<std::string> kHeapWritesCols = {
    "heap_writes", "HEAP_WRITES", "heap_blks_written", "seq_writes"
};
const std::vector<std::string> kIdxReadsCols = {
    "idx_reads", "IDX_READS", "idx_blks_read", "index_reads"
};
const std::vector<std::string> kIdxWritesCols = {
    "idx_writes", "IDX_WRITES", "idx_blks_written", "index_writes"
};
const std::vector<std::string> kToastReadsCols = {
    "toast_reads", "TOAST_READS", "toast_blks_read"
};
const std::vector<std::string> kToastWritesCols = {
    "toast_writes", "TOAST_WRITES", "toast_blks_written"
};

// Column mapping - Index I/O
const std::vector<std::string> kIndexNameCols = {
    "index_name", "INDEX_NAME", "indexrelname", "idx_name"
};

} // namespace

const std::vector<std::string> IOStatisticsPanel::kDatabaseColumnLabels = {
    "Database", "Reads", "Writes", "Read Bytes", "Write Bytes",
    "Read Time (ms)", "Write Time (ms)", "Avg Read (ms)", "Avg Write (ms)"
};

const std::vector<std::string> IOStatisticsPanel::kTableColumnLabels = {
    "Schema", "Table", "Heap Reads", "Heap Writes",
    "Index Reads", "Index Writes", "Toast Reads", "Toast Writes"
};

const std::vector<std::string> IOStatisticsPanel::kIndexColumnLabels = {
    "Schema", "Table", "Index", "Reads", "Writes"
};

wxBEGIN_EVENT_TABLE(IOStatisticsPanel, wxPanel)
    EVT_BUTTON(kRefreshButtonId, IOStatisticsPanel::OnRefresh)
    EVT_CHECKBOX(kAutoRefreshCheckId, IOStatisticsPanel::OnAutoRefreshToggle)
    EVT_CHOICE(kIntervalChoiceId, IOStatisticsPanel::OnIntervalChanged)
    EVT_CHOICE(kTimeRangeChoiceId, IOStatisticsPanel::OnTimeRangeChanged)
    EVT_BUTTON(kExportCSVButtonId, IOStatisticsPanel::OnExportCSV)
    EVT_BUTTON(kExportJSONButtonId, IOStatisticsPanel::OnExportJSON)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, IOStatisticsPanel::OnNotebookPageChanged)
    EVT_GRID_SELECT_CELL(IOStatisticsPanel::OnGridSelect)
    EVT_TIMER(kTimerId, IOStatisticsPanel::OnTimer)
wxEND_EVENT_TABLE()

IOStatisticsPanel::IOStatisticsPanel(wxWindow* parent,
                                     ConnectionManager* connectionManager)
    : wxPanel(parent, wxID_ANY),
      connection_manager_(connectionManager) {
    BuildLayout();
    UpdateControls();
    UpdateStatus("Ready");
}

void IOStatisticsPanel::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar panel
    auto* toolbar = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    refresh_button_ = new wxButton(toolbar, kRefreshButtonId, "Refresh");
    toolbarSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto_refresh_check_ = new wxCheckBox(toolbar, kAutoRefreshCheckId, "Auto-refresh");
    toolbarSizer->Add(auto_refresh_check_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    interval_choice_ = new wxChoice(toolbar, kIntervalChoiceId);
    interval_choice_->Append("30 sec");
    interval_choice_->Append("1 min");
    interval_choice_->Append("5 min");
    interval_choice_->Append("15 min");
    interval_choice_->SetSelection(1);  // Default 1 min
    interval_choice_->Enable(false);
    toolbarSizer->Add(interval_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->Add(new wxStaticText(toolbar, wxID_ANY, "Time Range:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    time_range_choice_ = new wxChoice(toolbar, kTimeRangeChoiceId);
    time_range_choice_->Append("Last Hour");
    time_range_choice_->Append("Last 24 Hours");
    time_range_choice_->Append("Last 7 Days");
    time_range_choice_->Append("Custom");
    time_range_choice_->SetSelection(0);
    toolbarSizer->Add(time_range_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    export_csv_button_ = new wxButton(toolbar, kExportCSVButtonId, "Export CSV");
    toolbarSizer->Add(export_csv_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    export_json_button_ = new wxButton(toolbar, kExportJSONButtonId, "Export JSON");
    toolbarSizer->Add(export_json_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    toolbarSizer->AddStretchSpacer(1);

    status_label_ = new wxStaticText(toolbar, wxID_ANY, "Ready");
    toolbarSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->SetSizer(toolbarSizer);
    rootSizer->Add(toolbar, 0, wxEXPAND | wxALL, 8);

    // Notebook with tabs
    notebook_ = new wxNotebook(this, wxID_ANY);

    // Database I/O tab
    database_tab_ = new wxPanel(notebook_, wxID_ANY);
    auto* dbSizer = new wxBoxSizer(wxVERTICAL);

    database_grid_ = new wxGrid(database_tab_, wxID_ANY);
    database_table_ = new ResultGridTable();
    database_grid_->SetTable(database_table_, true);
    database_grid_->EnableEditing(false);
    database_grid_->SetRowLabelSize(48);
    database_grid_->EnableGridLines(true);
    dbSizer->Add(database_grid_, 1, wxEXPAND | wxALL, 8);

    // Charts placeholder panel
    auto* chartsPanel = new wxPanel(database_tab_, wxID_ANY);
    chartsPanel->SetBackgroundColour(wxColour(240, 240, 240));
    auto* chartsSizer = new wxBoxSizer(wxVERTICAL);
    chartsSizer->Add(new wxStaticText(chartsPanel, wxID_ANY,
        "Charts area - Future enhancement for I/O visualization"),
        1, wxALIGN_CENTER);
    chartsPanel->SetSizer(chartsSizer);
    chartsPanel->SetMinSize(wxSize(-1, 120));
    dbSizer->Add(chartsPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    database_summary_label_ = new wxStaticText(database_tab_, wxID_ANY,
                                                "Total: 0 databases");
    dbSizer->Add(database_summary_label_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    database_tab_->SetSizer(dbSizer);
    notebook_->AddPage(database_tab_, "Database I/O");

    // Table I/O tab
    table_tab_ = new wxPanel(notebook_, wxID_ANY);
    auto* tableSizer = new wxBoxSizer(wxVERTICAL);

    table_grid_ = new wxGrid(table_tab_, wxID_ANY);
    table_table_ = new ResultGridTable();
    table_grid_->SetTable(table_table_, true);
    table_grid_->EnableEditing(false);
    table_grid_->SetRowLabelSize(48);
    table_grid_->EnableGridLines(true);
    tableSizer->Add(table_grid_, 1, wxEXPAND | wxALL, 8);

    table_summary_label_ = new wxStaticText(table_tab_, wxID_ANY,
                                            "Total: 0 tables");
    tableSizer->Add(table_summary_label_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    table_tab_->SetSizer(tableSizer);
    notebook_->AddPage(table_tab_, "Table I/O");

    // Index I/O tab
    index_tab_ = new wxPanel(notebook_, wxID_ANY);
    auto* indexSizer = new wxBoxSizer(wxVERTICAL);

    index_grid_ = new wxGrid(index_tab_, wxID_ANY);
    index_table_ = new ResultGridTable();
    index_grid_->SetTable(index_table_, true);
    index_grid_->EnableEditing(false);
    index_grid_->SetRowLabelSize(48);
    index_grid_->EnableGridLines(true);
    indexSizer->Add(index_grid_, 1, wxEXPAND | wxALL, 8);

    index_summary_label_ = new wxStaticText(index_tab_, wxID_ANY,
                                            "Total: 0 indexes");
    indexSizer->Add(index_summary_label_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    index_tab_->SetSizer(indexSizer);
    notebook_->AddPage(index_tab_, "Index I/O");

    rootSizer->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Summary panel at bottom
    auto* summaryPanel = new wxPanel(this, wxID_ANY);
    summaryPanel->SetBackgroundColour(wxColour(230, 230, 230));
    auto* summarySizer = new wxBoxSizer(wxHORIZONTAL);

    summary_label_ = new wxStaticText(summaryPanel, wxID_ANY,
        "Summary: No data loaded");
    summarySizer->Add(summary_label_, 1, wxEXPAND | wxALL, 8);

    summaryPanel->SetSizer(summarySizer);
    rootSizer->Add(summaryPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    SetSizer(rootSizer);

    // Initialize timer
    refresh_timer_ = new wxTimer(this, kTimerId);
}

void IOStatisticsPanel::RefreshData() {
    switch (current_tab_) {
        case 0:
            LoadDatabaseIO();
            break;
        case 1:
            LoadTableIO();
            break;
        case 2:
            LoadIndexIO();
            break;
    }
}

void IOStatisticsPanel::LoadDatabaseIO() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading database I/O statistics...");

    query_job_ = connection_manager_->ExecuteQueryAsync(kDatabaseIOQuery,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseDatabaseIO(result);
                    UpdateStatus("Database I/O updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load database I/O" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
                UpdateSummary();
            });
        });
}

void IOStatisticsPanel::LoadTableIO() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading table I/O statistics...");

    query_job_ = connection_manager_->ExecuteQueryAsync(kTableIOQuery,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseTableIO(result);
                    UpdateStatus("Table I/O updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load table I/O" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
                UpdateSummary();
            });
        });
}

void IOStatisticsPanel::LoadIndexIO() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading index I/O statistics...");

    query_job_ = connection_manager_->ExecuteQueryAsync(kIndexIOQuery,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseIndexIO(result);
                    UpdateStatus("Index I/O updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load index I/O" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
                UpdateSummary();
            });
        });
}

void IOStatisticsPanel::ParseDatabaseIO(const QueryResult& result) {
    database_io_stats_.clear();

    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    for (const auto& row : result.rows) {
        DatabaseIOStats stats;
        stats.database_name = FindColumnValue(row, colNames, kDatabaseNameCols);
        stats.reads = FindColumnValue(row, colNames, kReadsCols);
        stats.writes = FindColumnValue(row, colNames, kWritesCols);
        stats.read_bytes = FindColumnValue(row, colNames, kReadBytesCols);
        stats.write_bytes = FindColumnValue(row, colNames, kWriteBytesCols);
        stats.read_time_ms = FindColumnValue(row, colNames, kReadTimeCols);
        stats.write_time_ms = FindColumnValue(row, colNames, kWriteTimeCols);
        stats.avg_read_time_ms = CalculateAverage(stats.read_time_ms, stats.reads);
        stats.avg_write_time_ms = CalculateAverage(stats.write_time_ms, stats.writes);
        database_io_stats_.push_back(stats);
    }

    // Update grid
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> gridRows;

    for (const auto& label : kDatabaseColumnLabels) {
        QueryColumn col;
        col.name = label;
        col.type = "TEXT";
        columns.push_back(col);
    }

    for (const auto& stats : database_io_stats_) {
        std::vector<QueryValue> row;
        row.push_back({false, stats.database_name, {}});
        row.push_back({false, stats.reads, {}});
        row.push_back({false, stats.writes, {}});
        row.push_back({false, FormatBytes(stats.read_bytes), {}});
        row.push_back({false, FormatBytes(stats.write_bytes), {}});
        row.push_back({false, stats.read_time_ms, {}});
        row.push_back({false, stats.write_time_ms, {}});
        row.push_back({false, stats.avg_read_time_ms, {}});
        row.push_back({false, stats.avg_write_time_ms, {}});
        gridRows.push_back(row);
    }

    if (database_table_) {
        database_table_->Reset(columns, gridRows);
    }

    if (database_summary_label_) {
        database_summary_label_->SetLabel(
            wxString::Format("Total: %zu databases", database_io_stats_.size()));
    }
}

void IOStatisticsPanel::ParseTableIO(const QueryResult& result) {
    table_io_stats_.clear();

    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    for (const auto& row : result.rows) {
        TableIOStats stats;
        stats.schema_name = FindColumnValue(row, colNames, kSchemaNameCols);
        stats.table_name = FindColumnValue(row, colNames, kTableNameCols);
        stats.heap_reads = FindColumnValue(row, colNames, kHeapReadsCols);
        stats.heap_writes = FindColumnValue(row, colNames, kHeapWritesCols);
        stats.idx_reads = FindColumnValue(row, colNames, kIdxReadsCols);
        stats.idx_writes = FindColumnValue(row, colNames, kIdxWritesCols);
        stats.toast_reads = FindColumnValue(row, colNames, kToastReadsCols);
        stats.toast_writes = FindColumnValue(row, colNames, kToastWritesCols);
        table_io_stats_.push_back(stats);
    }

    // Update grid
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> gridRows;

    for (const auto& label : kTableColumnLabels) {
        QueryColumn col;
        col.name = label;
        col.type = "TEXT";
        columns.push_back(col);
    }

    for (const auto& stats : table_io_stats_) {
        std::vector<QueryValue> row;
        row.push_back({false, stats.schema_name, {}});
        row.push_back({false, stats.table_name, {}});
        row.push_back({false, stats.heap_reads, {}});
        row.push_back({false, stats.heap_writes, {}});
        row.push_back({false, stats.idx_reads, {}});
        row.push_back({false, stats.idx_writes, {}});
        row.push_back({false, stats.toast_reads, {}});
        row.push_back({false, stats.toast_writes, {}});
        gridRows.push_back(row);
    }

    if (table_table_) {
        table_table_->Reset(columns, gridRows);
    }

    if (table_summary_label_) {
        table_summary_label_->SetLabel(
            wxString::Format("Total: %zu tables", table_io_stats_.size()));
    }
}

void IOStatisticsPanel::ParseIndexIO(const QueryResult& result) {
    index_io_stats_.clear();

    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    for (const auto& row : result.rows) {
        IndexIOStats stats;
        stats.schema_name = FindColumnValue(row, colNames, kSchemaNameCols);
        stats.table_name = FindColumnValue(row, colNames, kTableNameCols);
        stats.index_name = FindColumnValue(row, colNames, kIndexNameCols);
        stats.idx_reads = FindColumnValue(row, colNames, kIdxReadsCols);
        stats.idx_writes = FindColumnValue(row, colNames, kIdxWritesCols);
        index_io_stats_.push_back(stats);
    }

    // Update grid
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> gridRows;

    for (const auto& label : kIndexColumnLabels) {
        QueryColumn col;
        col.name = label;
        col.type = "TEXT";
        columns.push_back(col);
    }

    for (const auto& stats : index_io_stats_) {
        std::vector<QueryValue> row;
        row.push_back({false, stats.schema_name, {}});
        row.push_back({false, stats.table_name, {}});
        row.push_back({false, stats.index_name, {}});
        row.push_back({false, stats.idx_reads, {}});
        row.push_back({false, stats.idx_writes, {}});
        gridRows.push_back(row);
    }

    if (index_table_) {
        index_table_->Reset(columns, gridRows);
    }

    if (index_summary_label_) {
        index_summary_label_->SetLabel(
            wxString::Format("Total: %zu indexes", index_io_stats_.size()));
    }
}

std::string IOStatisticsPanel::FindColumnValue(
    const std::vector<QueryValue>& row,
    const std::vector<std::string>& colNames,
    const std::vector<std::string>& possibleNames) const {
    int idx = FindColumnIndex(colNames, possibleNames);
    if (idx >= 0 && idx < static_cast<int>(row.size())) {
        return row[idx].isNull ? "" : row[idx].text;
    }
    return "";
}

int IOStatisticsPanel::FindColumnIndex(
    const std::vector<std::string>& colNames,
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

std::string IOStatisticsPanel::FormatBytes(const std::string& bytes) const {
    if (bytes.empty()) return "0 B";

    try {
        double value = std::stod(bytes);
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;

        while (value >= 1024.0 && unitIndex < 4) {
            value /= 1024.0;
            unitIndex++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value << " " << units[unitIndex];
        return oss.str();
    } catch (...) {
        return bytes;
    }
}

std::string IOStatisticsPanel::CalculateAverage(const std::string& total_time,
                                                 const std::string& count) const {
    if (total_time.empty() || count.empty()) return "0.00";

    try {
        double total = std::stod(total_time);
        double cnt = std::stod(count);
        if (cnt == 0) return "0.00";

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (total / cnt);
        return oss.str();
    } catch (...) {
        return "0.00";
    }
}

void IOStatisticsPanel::SetAutoRefresh(bool enable, int intervalSeconds) {
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

bool IOStatisticsPanel::IsAutoRefreshEnabled() const {
    return auto_refresh_check_ && auto_refresh_check_->IsChecked();
}

void IOStatisticsPanel::SetTimeRange(TimeRange range) {
    current_time_range_ = range;
    if (time_range_choice_) {
        switch (range) {
            case TimeRange::LastHour:
                time_range_choice_->SetSelection(0);
                break;
            case TimeRange::Last24Hours:
                time_range_choice_->SetSelection(1);
                break;
            case TimeRange::Last7Days:
                time_range_choice_->SetSelection(2);
                break;
            case TimeRange::Custom:
                time_range_choice_->SetSelection(3);
                break;
        }
    }
}

TimeRange IOStatisticsPanel::GetTimeRange() const {
    return current_time_range_;
}

void IOStatisticsPanel::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();

    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
    if (export_csv_button_) {
        export_csv_button_->Enable(connected);
    }
    if (export_json_button_) {
        export_json_button_->Enable(connected);
    }
}

void IOStatisticsPanel::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void IOStatisticsPanel::UpdateSummary() {
    if (!summary_label_) return;

    // Calculate totals across all tabs
    long long totalReads = 0;
    long long totalWrites = 0;

    for (const auto& stats : database_io_stats_) {
        try {
            totalReads += std::stoll(stats.reads.empty() ? "0" : stats.reads);
            totalWrites += std::stoll(stats.writes.empty() ? "0" : stats.writes);
        } catch (...) {}
    }

    wxString summary = wxString::Format(
        "Summary: %zu databases, %zu tables, %zu indexes | "
        "Total I/O Operations: %s reads, %s writes",
        database_io_stats_.size(),
        table_io_stats_.size(),
        index_io_stats_.size(),
        wxString::Format("%lld", totalReads),
        wxString::Format("%lld", totalWrites));

    summary_label_->SetLabel(summary);
}

std::string IOStatisticsPanel::GetCurrentTabName() const {
    switch (current_tab_) {
        case 0: return "database_io";
        case 1: return "table_io";
        case 2: return "index_io";
        default: return "unknown";
    }
}

void IOStatisticsPanel::ExportToCSV() {
    wxString defaultName = wxString::Format("%s_%s.csv",
        GetCurrentTabName(),
        wxDateTime::Now().Format("%Y%m%d_%H%M%S"));

    wxFileDialog dialog(this, "Export to CSV", "", defaultName,
                        "CSV files (*.csv)|*.csv",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    std::ofstream file(dialog.GetPath().ToUTF8());
    if (!file.is_open()) {
        wxMessageBox("Failed to create file", "Export Error", wxOK | wxICON_ERROR);
        return;
    }

    // Write header
    switch (current_tab_) {
        case 0:
            file << "Database,Reads,Writes,Read Bytes,Write Bytes,"
                 << "Read Time (ms),Write Time (ms),Avg Read (ms),Avg Write (ms)\n";
            for (const auto& stats : database_io_stats_) {
                file << stats.database_name << ","
                     << stats.reads << ","
                     << stats.writes << ","
                     << stats.read_bytes << ","
                     << stats.write_bytes << ","
                     << stats.read_time_ms << ","
                     << stats.write_time_ms << ","
                     << stats.avg_read_time_ms << ","
                     << stats.avg_write_time_ms << "\n";
            }
            break;
        case 1:
            file << "Schema,Table,Heap Reads,Heap Writes,"
                 << "Index Reads,Index Writes,Toast Reads,Toast Writes\n";
            for (const auto& stats : table_io_stats_) {
                file << stats.schema_name << ","
                     << stats.table_name << ","
                     << stats.heap_reads << ","
                     << stats.heap_writes << ","
                     << stats.idx_reads << ","
                     << stats.idx_writes << ","
                     << stats.toast_reads << ","
                     << stats.toast_writes << "\n";
            }
            break;
        case 2:
            file << "Schema,Table,Index,Reads,Writes\n";
            for (const auto& stats : index_io_stats_) {
                file << stats.schema_name << ","
                     << stats.table_name << ","
                     << stats.index_name << ","
                     << stats.idx_reads << ","
                     << stats.idx_writes << "\n";
            }
            break;
    }

    file.close();
    UpdateStatus("Exported to CSV");
}

void IOStatisticsPanel::ExportToJSON() {
    wxString defaultName = wxString::Format("%s_%s.json",
        GetCurrentTabName(),
        wxDateTime::Now().Format("%Y%m%d_%H%M%S"));

    wxFileDialog dialog(this, "Export to JSON", "", defaultName,
                        "JSON files (*.json)|*.json",
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK) {
        return;
    }

    std::ofstream file(dialog.GetPath().ToUTF8());
    if (!file.is_open()) {
        wxMessageBox("Failed to create file", "Export Error", wxOK | wxICON_ERROR);
        return;
    }

    file << "{\n";
    file << "  \"export_time\": \"" << wxDateTime::Now().FormatISOCombined().ToUTF8() << "\",\n";
    file << "  \"data_type\": \"" << GetCurrentTabName() << "\",\n";
    file << "  \"records\": [\n";

    switch (current_tab_) {
        case 0:
            for (size_t i = 0; i < database_io_stats_.size(); ++i) {
                const auto& stats = database_io_stats_[i];
                file << "    {\n"
                     << "      \"database_name\": \"" << stats.database_name << "\",\n"
                     << "      \"reads\": " << (stats.reads.empty() ? "0" : stats.reads) << ",\n"
                     << "      \"writes\": " << (stats.writes.empty() ? "0" : stats.writes) << ",\n"
                     << "      \"read_bytes\": " << (stats.read_bytes.empty() ? "0" : stats.read_bytes) << ",\n"
                     << "      \"write_bytes\": " << (stats.write_bytes.empty() ? "0" : stats.write_bytes) << ",\n"
                     << "      \"read_time_ms\": " << (stats.read_time_ms.empty() ? "0" : stats.read_time_ms) << ",\n"
                     << "      \"write_time_ms\": " << (stats.write_time_ms.empty() ? "0" : stats.write_time_ms) << "\n"
                     << "    }";
                if (i < database_io_stats_.size() - 1) file << ",";
                file << "\n";
            }
            break;
        case 1:
            for (size_t i = 0; i < table_io_stats_.size(); ++i) {
                const auto& stats = table_io_stats_[i];
                file << "    {\n"
                     << "      \"schema_name\": \"" << stats.schema_name << "\",\n"
                     << "      \"table_name\": \"" << stats.table_name << "\",\n"
                     << "      \"heap_reads\": " << (stats.heap_reads.empty() ? "0" : stats.heap_reads) << ",\n"
                     << "      \"heap_writes\": " << (stats.heap_writes.empty() ? "0" : stats.heap_writes) << ",\n"
                     << "      \"idx_reads\": " << (stats.idx_reads.empty() ? "0" : stats.idx_reads) << ",\n"
                     << "      \"idx_writes\": " << (stats.idx_writes.empty() ? "0" : stats.idx_writes) << "\n"
                     << "    }";
                if (i < table_io_stats_.size() - 1) file << ",";
                file << "\n";
            }
            break;
        case 2:
            for (size_t i = 0; i < index_io_stats_.size(); ++i) {
                const auto& stats = index_io_stats_[i];
                file << "    {\n"
                     << "      \"schema_name\": \"" << stats.schema_name << "\",\n"
                     << "      \"table_name\": \"" << stats.table_name << "\",\n"
                     << "      \"index_name\": \"" << stats.index_name << "\",\n"
                     << "      \"idx_reads\": " << (stats.idx_reads.empty() ? "0" : stats.idx_reads) << ",\n"
                     << "      \"idx_writes\": " << (stats.idx_writes.empty() ? "0" : stats.idx_writes) << "\n"
                     << "    }";
                if (i < index_io_stats_.size() - 1) file << ",";
                file << "\n";
            }
            break;
    }

    file << "  ]\n";
    file << "}\n";
    file.close();

    UpdateStatus("Exported to JSON");
}

void IOStatisticsPanel::OnRefresh(wxCommandEvent&) {
    RefreshData();
}

void IOStatisticsPanel::OnAutoRefreshToggle(wxCommandEvent&) {
    bool enable = auto_refresh_check_->IsChecked();
    interval_choice_->Enable(enable);

    if (enable) {
        int intervals[] = {30, 60, 300, 900};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 4) {
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    } else {
        refresh_timer_->Stop();
    }
}

void IOStatisticsPanel::OnIntervalChanged(wxCommandEvent&) {
    if (auto_refresh_check_->IsChecked()) {
        int intervals[] = {30, 60, 300, 900};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 4) {
            refresh_timer_->Stop();
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    }
}

void IOStatisticsPanel::OnTimeRangeChanged(wxCommandEvent&) {
    int selection = time_range_choice_->GetSelection();
    switch (selection) {
        case 0:
            current_time_range_ = TimeRange::LastHour;
            break;
        case 1:
            current_time_range_ = TimeRange::Last24Hours;
            break;
        case 2:
            current_time_range_ = TimeRange::Last7Days;
            break;
        case 3:
            current_time_range_ = TimeRange::Custom;
            // TODO: Show custom date range dialog
            break;
    }
    RefreshData();
}

void IOStatisticsPanel::OnExportCSV(wxCommandEvent&) {
    ExportToCSV();
}

void IOStatisticsPanel::OnExportJSON(wxCommandEvent&) {
    ExportToJSON();
}

void IOStatisticsPanel::OnNotebookPageChanged(wxBookCtrlEvent& event) {
    current_tab_ = event.GetSelection();
    RefreshData();
}

void IOStatisticsPanel::OnGridSelect(wxGridEvent&) {
    // Selection handling if needed
}

void IOStatisticsPanel::OnTimer(wxTimerEvent&) {
    if (!query_running_) {
        RefreshData();
    }
}

} // namespace scratchrobin
