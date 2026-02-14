/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "table_statistics_panel.h"

#include "result_grid_table.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

namespace scratchrobin {

namespace {

// Control IDs
constexpr int kRefreshButtonId = wxID_HIGHEST + 400;
constexpr int kExportButtonId = wxID_HIGHEST + 401;
constexpr int kAnalyzeButtonId = wxID_HIGHEST + 402;
constexpr int kVacuumButtonId = wxID_HIGHEST + 403;
constexpr int kAutoRefreshCheckId = wxID_HIGHEST + 404;
constexpr int kIntervalChoiceId = wxID_HIGHEST + 405;
constexpr int kSortChoiceId = wxID_HIGHEST + 406;
constexpr int kSchemaFilterId = wxID_HIGHEST + 407;
constexpr int kTableFilterId = wxID_HIGHEST + 408;
constexpr int kSizeThresholdId = wxID_HIGHEST + 409;
constexpr int kTimerId = wxID_HIGHEST + 410;

// Column mapping for different backends
const std::vector<std::string> kSchemaNameCols = {"schema_name", "schemaname", "nspname", "OWNER"};
const std::vector<std::string> kTableNameCols = {"table_name", "relname", "tablename", "TABLE_NAME"};
const std::vector<std::string> kRowCountCols = {"row_count", "n_live_tup", "reltuples", "NUM_ROWS"};
const std::vector<std::string> kTableSizeCols = {"table_size", "pg_table_size", "data_length"};
const std::vector<std::string> kIndexSizeCols = {"index_size", "pg_indexes_size", "index_length"};
const std::vector<std::string> kTotalSizeCols = {"total_size", "pg_total_relation_size", "total_length"};
const std::vector<std::string> kSeqScanCols = {"seq_scans", "seq_scan", "table_scans"};
const std::vector<std::string> kIdxScanCols = {"idx_scans", "idx_scan", "index_scans"};
const std::vector<std::string> kTupInsCols = {"n_tup_ins", "tup_inserted", "rows_inserted"};
const std::vector<std::string> kTupUpdCols = {"n_tup_upd", "tup_updated", "rows_updated"};
const std::vector<std::string> kTupDelCols = {"n_tup_del", "tup_deleted", "rows_deleted"};
const std::vector<std::string> kLastVacuumCols = {"last_vacuum", "last_vacuum_date", "last_compact"};
const std::vector<std::string> kLastAnalyzeCols = {"last_analyze", "last_analyze_date", "last_statistics_update"};

// Parse size string to bytes
long long ParseSizeString(const std::string& sizeStr) {
    if (sizeStr.empty()) return 0;
    
    std::string numStr;
    std::string unitStr;
    
    // Separate number and unit
    for (char c : sizeStr) {
        if (std::isdigit(c) || c == '.' || c == ' ') {
            if (c != ' ') numStr += c;
        } else {
            unitStr += static_cast<char>(std::tolower(c));
        }
    }
    
    double value = 0;
    try {
        value = std::stod(numStr);
    } catch (...) {
        return 0;
    }
    
    // Convert to bytes
    if (unitStr.find("tb") != std::string::npos || unitStr.find("tib") != std::string::npos) {
        return static_cast<long long>(value * 1024LL * 1024 * 1024 * 1024);
    } else if (unitStr.find("gb") != std::string::npos || unitStr.find("gib") != std::string::npos) {
        return static_cast<long long>(value * 1024LL * 1024 * 1024);
    } else if (unitStr.find("mb") != std::string::npos || unitStr.find("mib") != std::string::npos) {
        return static_cast<long long>(value * 1024LL * 1024);
    } else if (unitStr.find("kb") != std::string::npos || unitStr.find("kib") != std::string::npos) {
        return static_cast<long long>(value * 1024);
    } else if (unitStr.find("b") != std::string::npos) {
        return static_cast<long long>(value);
    }
    
    return static_cast<long long>(value);
}

// Parse count string to number
long long ParseCountString(const std::string& countStr) {
    if (countStr.empty()) return 0;
    
    try {
        // Remove commas and spaces
        std::string cleanStr;
        for (char c : countStr) {
            if (std::isdigit(c) || c == '-' || c == '+') {
                cleanStr += c;
            }
        }
        return std::stoll(cleanStr);
    } catch (...) {
        return 0;
    }
}

// Format bytes to human readable
std::string FormatBytes(long long bytes) {
    if (bytes < 0) return "N/A";
    if (bytes == 0) return "0 B";
    
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unitIndex = 0;
    double value = static_cast<double>(bytes);
    
    while (value >= 1024.0 && unitIndex < 5) {
        value /= 1024.0;
        unitIndex++;
    }
    
    char buf[64];
    if (value < 10) {
        std::snprintf(buf, sizeof(buf), "%.2f %s", value, units[unitIndex]);
    } else if (value < 100) {
        std::snprintf(buf, sizeof(buf), "%.1f %s", value, units[unitIndex]);
    } else {
        std::snprintf(buf, sizeof(buf), "%.0f %s", value, units[unitIndex]);
    }
    return std::string(buf);
}

// Format number with commas
std::string FormatNumber(long long num) {
    if (num < 0) return "N/A";
    if (num == 0) return "0";
    
    std::string result;
    int count = 0;
    bool negative = num < 0;
    num = std::llabs(num);
    
    do {
        if (count > 0 && count % 3 == 0) {
            result = ',' + result;
        }
        result = static_cast<char>('0' + (num % 10)) + result;
        num /= 10;
        count++;
    } while (num > 0);
    
    return negative ? "-" + result : result;
}

} // anonymous namespace

const std::vector<std::string> TableStatisticsPanel::kColumnNames = {
    "schema_name", "table_name", "row_count", "table_size",
    "index_size", "total_size", "seq_scans", "idx_scans",
    "n_tup_ins", "n_tup_upd", "n_tup_del", "last_vacuum", "last_analyze"
};

const std::vector<std::string> TableStatisticsPanel::kColumnLabels = {
    "Schema", "Table", "Rows", "Table Size",
    "Index Size", "Total Size", "Seq Scans", "Idx Scans",
    "Inserts", "Updates", "Deletes", "Last Vacuum", "Last Analyze"
};

wxBEGIN_EVENT_TABLE(TableStatisticsPanel, wxPanel)
    EVT_BUTTON(kRefreshButtonId, TableStatisticsPanel::OnRefresh)
    EVT_BUTTON(kExportButtonId, TableStatisticsPanel::OnExport)
    EVT_BUTTON(kAnalyzeButtonId, TableStatisticsPanel::OnAnalyze)
    EVT_BUTTON(kVacuumButtonId, TableStatisticsPanel::OnVacuum)
    EVT_CHECKBOX(kAutoRefreshCheckId, TableStatisticsPanel::OnAutoRefreshToggle)
    EVT_CHOICE(kIntervalChoiceId, TableStatisticsPanel::OnIntervalChanged)
    EVT_CHOICE(kSortChoiceId, TableStatisticsPanel::OnSortChanged)
    EVT_TEXT(kSchemaFilterId, TableStatisticsPanel::OnFilterChanged)
    EVT_TEXT(kTableFilterId, TableStatisticsPanel::OnFilterChanged)
    EVT_TEXT(kSizeThresholdId, TableStatisticsPanel::OnFilterChanged)
    EVT_GRID_SELECT_CELL(TableStatisticsPanel::OnGridSelect)
    EVT_TIMER(kTimerId, TableStatisticsPanel::OnTimer)
wxEND_EVENT_TABLE()

TableStatisticsPanel::TableStatisticsPanel(wxWindow* parent,
                                           ConnectionManager* connectionManager)
    : wxPanel(parent, wxID_ANY),
      connection_manager_(connectionManager) {
    BuildLayout();
    UpdateControls();
    UpdateStatus("Ready");
}

TableStatisticsPanel::~TableStatisticsPanel() {
    if (refresh_timer_) {
        refresh_timer_->Stop();
    }
}

void TableStatisticsPanel::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // === Filter Panel ===
    auto* filterPanel = new wxPanel(this, wxID_ANY);
    filterPanel->SetBackgroundColour(wxColour(250, 250, 250));
    auto* filterSizer = new wxBoxSizer(wxHORIZONTAL);

    // Schema filter
    filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Schema:"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    schema_filter_ = new wxTextCtrl(filterPanel, kSchemaFilterId, "", 
                                    wxDefaultPosition, wxSize(120, -1));
    filterSizer->Add(schema_filter_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    // Table name filter
    filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Table:"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    table_filter_ = new wxTextCtrl(filterPanel, kTableFilterId, "", 
                                   wxDefaultPosition, wxSize(150, -1));
    filterSizer->Add(table_filter_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    // Size threshold filter
    filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Min Size (MB):"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    size_threshold_filter_ = new wxTextCtrl(filterPanel, kSizeThresholdId, "0", 
                                            wxDefaultPosition, wxSize(80, -1));
    filterSizer->Add(size_threshold_filter_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    // Sort choice
    filterSizer->Add(new wxStaticText(filterPanel, wxID_ANY, "Sort by:"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    sort_choice_ = new wxChoice(filterPanel, kSortChoiceId);
    sort_choice_->Append("Size (largest first)");
    sort_choice_->Append("Size (smallest first)");
    sort_choice_->Append("Row Count (highest first)");
    sort_choice_->Append("Row Count (lowest first)");
    sort_choice_->Append("Scan Count (highest first)");
    sort_choice_->Append("Scan Count (lowest first)");
    sort_choice_->Append("Last Modified");
    sort_choice_->SetSelection(0);
    filterSizer->Add(sort_choice_, 0, wxALIGN_CENTER_VERTICAL);

    filterPanel->SetSizer(filterSizer);
    rootSizer->Add(filterPanel, 0, wxEXPAND | wxALL, 8);

    // === Toolbar Panel ===
    auto* toolbar = new wxPanel(this, wxID_ANY);
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    refresh_button_ = new wxButton(toolbar, kRefreshButtonId, "Refresh");
    toolbarSizer->Add(refresh_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    export_button_ = new wxButton(toolbar, kExportButtonId, "Export");
    toolbarSizer->Add(export_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    analyze_button_ = new wxButton(toolbar, kAnalyzeButtonId, "Analyze Selected");
    toolbarSizer->Add(analyze_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    vacuum_button_ = new wxButton(toolbar, kVacuumButtonId, "Vacuum Selected");
    toolbarSizer->Add(vacuum_button_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

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

    toolbarSizer->AddStretchSpacer(1);

    count_label_ = new wxStaticText(toolbar, wxID_ANY, "0 tables");
    toolbarSizer->Add(count_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    status_label_ = new wxStaticText(toolbar, wxID_ANY, "Ready");
    toolbarSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->SetSizer(toolbarSizer);
    rootSizer->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // === Grid Panel ===
    auto* gridPanel = new wxPanel(this, wxID_ANY);
    auto* gridSizer = new wxBoxSizer(wxVERTICAL);

    statistics_grid_ = new wxGrid(gridPanel, wxID_ANY);
    grid_table_ = new ResultGridTable();
    statistics_grid_->SetTable(grid_table_, true);
    statistics_grid_->EnableEditing(false);
    statistics_grid_->SetRowLabelSize(48);
    statistics_grid_->EnableGridLines(true);
    statistics_grid_->SetDefaultColSize(100);
    statistics_grid_->SetColSize(1, 150);  // Table name wider
    statistics_grid_->SetColSize(2, 80);   // Row count
    statistics_grid_->SetColSize(3, 100);  // Table size
    statistics_grid_->SetColSize(4, 100);  // Index size
    statistics_grid_->SetColSize(5, 100);  // Total size

    gridSizer->Add(statistics_grid_, 1, wxEXPAND | wxALL, 8);
    gridPanel->SetSizer(gridSizer);
    rootSizer->Add(gridPanel, 1, wxEXPAND);

    // === Summary Panel ===
    auto* summaryPanel = new wxPanel(this, wxID_ANY);
    summaryPanel->SetBackgroundColour(wxColour(240, 248, 255));  // Light blue
    auto* summarySizer = new wxBoxSizer(wxHORIZONTAL);

    summarySizer->Add(new wxStaticText(summaryPanel, wxID_ANY, "Summary:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    summarySizer->AddSpacer(16);

    summarySizer->Add(new wxStaticText(summaryPanel, wxID_ANY, "Total Tables:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    total_tables_label_ = new wxStaticText(summaryPanel, wxID_ANY, "0");
    summarySizer->Add(total_tables_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    summarySizer->Add(new wxStaticText(summaryPanel, wxID_ANY, "Total Size:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    total_size_label_ = new wxStaticText(summaryPanel, wxID_ANY, "0 B");
    summarySizer->Add(total_size_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    summarySizer->Add(new wxStaticText(summaryPanel, wxID_ANY, "Total Rows:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    total_rows_label_ = new wxStaticText(summaryPanel, wxID_ANY, "0");
    summarySizer->Add(total_rows_label_, 0, wxALIGN_CENTER_VERTICAL);

    summarySizer->AddStretchSpacer(1);

    summaryPanel->SetSizer(summarySizer);
    rootSizer->Add(summaryPanel, 0, wxEXPAND | wxALL, 8);

    SetSizer(rootSizer);

    // Initialize timer
    refresh_timer_ = new wxTimer(this, kTimerId);
}

void TableStatisticsPanel::RefreshData() {
    LoadStatistics();
}

void TableStatisticsPanel::LoadStatistics() {
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        UpdateStatus("Not connected");
        return;
    }
    if (query_running_) {
        return;
    }

    query_running_ = true;
    UpdateControls();
    UpdateStatus("Loading table statistics...");

    // Query for table statistics from sb_catalog.sb_table_statistics
    const std::string query = 
        "SELECT schema_name, table_name, row_count, "
        "       pg_size_pretty(table_size) as table_size, "
        "       pg_size_pretty(index_size) as index_size, "
        "       pg_size_pretty(total_size) as total_size, "
        "       seq_scans, idx_scans, "
        "       n_tup_ins, n_tup_upd, n_tup_del, "
        "       last_vacuum, last_analyze, "
        "       table_size as table_size_raw, "
        "       index_size as index_size_raw, "
        "       total_size as total_size_raw "
        "FROM sb_catalog.sb_table_statistics "
        "ORDER BY total_size DESC;";

    query_job_ = connection_manager_->ExecuteQueryAsync(query,
        [this](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, ok, result = std::move(result), error]() mutable {
                query_running_ = false;
                if (ok) {
                    ParseStatistics(result);
                    ApplyFilters();
                    UpdateStatus("Updated");
                } else {
                    UpdateStatus("Query failed");
                    wxMessageBox(error.empty() ? "Failed to load table statistics" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
                UpdateControls();
            });
        });
}

void TableStatisticsPanel::ParseStatistics(const QueryResult& result) {
    all_statistics_.clear();

    // Build column name list
    std::vector<std::string> colNames;
    for (const auto& col : result.columns) {
        colNames.push_back(col.name);
    }

    // Parse each row
    for (const auto& row : result.rows) {
        all_statistics_.push_back(ExtractStatisticsInfo(row, colNames));
    }
}

TableStatisticsInfo TableStatisticsPanel::ExtractStatisticsInfo(
        const std::vector<QueryValue>& row,
        const std::vector<std::string>& colNames) {
    TableStatisticsInfo info;
    info.schema_name = FindColumnValue(row, colNames, kSchemaNameCols);
    info.table_name = FindColumnValue(row, colNames, kTableNameCols);
    info.row_count = FindColumnValue(row, colNames, kRowCountCols);
    info.table_size = FindColumnValue(row, colNames, kTableSizeCols);
    info.index_size = FindColumnValue(row, colNames, kIndexSizeCols);
    info.total_size = FindColumnValue(row, colNames, kTotalSizeCols);
    info.seq_scans = FindColumnValue(row, colNames, kSeqScanCols);
    info.idx_scans = FindColumnValue(row, colNames, kIdxScanCols);
    info.n_tup_ins = FindColumnValue(row, colNames, kTupInsCols);
    info.n_tup_upd = FindColumnValue(row, colNames, kTupUpdCols);
    info.n_tup_del = FindColumnValue(row, colNames, kTupDelCols);
    info.last_vacuum = FindColumnValue(row, colNames, kLastVacuumCols);
    info.last_analyze = FindColumnValue(row, colNames, kLastAnalyzeCols);

    // Parse raw values for sorting/filtering
    info.row_count_raw = ParseCount(info.row_count);
    info.total_size_raw = ParseSizeToBytes(info.total_size);
    info.scan_count_raw = ParseCount(info.seq_scans) + ParseCount(info.idx_scans);

    return info;
}

std::string TableStatisticsPanel::FindColumnValue(
        const std::vector<QueryValue>& row,
        const std::vector<std::string>& colNames,
        const std::vector<std::string>& possibleNames) const {
    int idx = FindColumnIndex(colNames, possibleNames);
    if (idx >= 0 && idx < static_cast<int>(row.size())) {
        return row[idx].isNull ? "" : row[idx].text;
    }
    return "";
}

int TableStatisticsPanel::FindColumnIndex(
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

long long TableStatisticsPanel::ParseSizeToBytes(const std::string& sizeStr) const {
    return ParseSizeString(sizeStr);
}

long long TableStatisticsPanel::ParseCount(const std::string& countStr) const {
    return ParseCountString(countStr);
}

void TableStatisticsPanel::ApplyFilters() {
    filtered_statistics_.clear();

    for (const auto& info : all_statistics_) {
        if (MatchesFilters(info)) {
            filtered_statistics_.push_back(info);
        }
    }

    ApplySort();
    UpdateSummary();
    UpdateGrid();
}

bool TableStatisticsPanel::MatchesFilters(const TableStatisticsInfo& info) const {
    // Schema filter
    if (schema_filter_) {
        wxString schemaFilter = schema_filter_->GetValue().Lower();
        if (!schemaFilter.IsEmpty()) {
            wxString schema = wxString::FromUTF8(info.schema_name).Lower();
            if (!schema.Contains(schemaFilter)) {
                return false;
            }
        }
    }

    // Table name filter (with wildcard support)
    if (table_filter_) {
        wxString tableFilter = table_filter_->GetValue().Lower();
        if (!tableFilter.IsEmpty()) {
            wxString tableName = wxString::FromUTF8(info.table_name).Lower();
            
            // Simple wildcard matching (* and ?)
            if (tableFilter.Contains('*') || tableFilter.Contains('?')) {
                wxString pattern = tableFilter;
                pattern.Replace("*", ".*");
                pattern.Replace("?", ".");
                if (!tableName.Matches(tableFilter)) {
                    return false;
                }
            } else {
                if (!tableName.Contains(tableFilter)) {
                    return false;
                }
            }
        }
    }

    // Size threshold filter
    if (size_threshold_filter_) {
        wxString thresholdStr = size_threshold_filter_->GetValue();
        if (!thresholdStr.IsEmpty()) {
            long thresholdMB = 0;
            if (thresholdStr.ToLong(&thresholdMB)) {
                long long thresholdBytes = thresholdMB * 1024 * 1024;
                if (info.total_size_raw < thresholdBytes) {
                    return false;
                }
            }
        }
    }

    return true;
}

void TableStatisticsPanel::ApplySort() {
    std::sort(filtered_statistics_.begin(), filtered_statistics_.end(),
        [this](const TableStatisticsInfo& a, const TableStatisticsInfo& b) {
            return CompareStatistics(a, b);
        });
}

bool TableStatisticsPanel::CompareStatistics(const TableStatisticsInfo& a,
                                              const TableStatisticsInfo& b) const {
    switch (current_sort_) {
        case TableSortOrder::BySizeDesc:
            return a.total_size_raw > b.total_size_raw;
        case TableSortOrder::BySizeAsc:
            return a.total_size_raw < b.total_size_raw;
        case TableSortOrder::ByRowCountDesc:
            return a.row_count_raw > b.row_count_raw;
        case TableSortOrder::ByRowCountAsc:
            return a.row_count_raw < b.row_count_raw;
        case TableSortOrder::ByScanCountDesc:
            return a.scan_count_raw > b.scan_count_raw;
        case TableSortOrder::ByScanCountAsc:
            return a.scan_count_raw < b.scan_count_raw;
        case TableSortOrder::ByModificationTimeDesc:
            return a.last_analyze > b.last_analyze;
    }
    return false;
}

void TableStatisticsPanel::UpdateGrid() {
    std::vector<QueryColumn> columns;
    std::vector<std::vector<QueryValue>> gridRows;

    for (size_t i = 0; i < kColumnNames.size(); ++i) {
        QueryColumn col;
        col.name = kColumnLabels[i];
        col.type = "TEXT";
        columns.push_back(col);
    }

    for (const auto& info : filtered_statistics_) {
        std::vector<QueryValue> row;
        row.push_back({false, info.schema_name, {}});
        row.push_back({false, info.table_name, {}});
        row.push_back({false, info.row_count, {}});
        row.push_back({false, info.table_size, {}});
        row.push_back({false, info.index_size, {}});
        row.push_back({false, info.total_size, {}});
        row.push_back({false, info.seq_scans, {}});
        row.push_back({false, info.idx_scans, {}});
        row.push_back({false, info.n_tup_ins, {}});
        row.push_back({false, info.n_tup_upd, {}});
        row.push_back({false, info.n_tup_del, {}});
        row.push_back({false, info.last_vacuum, {}});
        row.push_back({false, info.last_analyze, {}});
        gridRows.push_back(row);
    }

    if (grid_table_) {
        grid_table_->Reset(columns, gridRows);
    }

    // Update count label
    if (count_label_) {
        count_label_->SetLabel(wxString::Format("%zu of %zu tables", 
                                                filtered_statistics_.size(),
                                                all_statistics_.size()));
    }
}

void TableStatisticsPanel::UpdateSummary() {
    long long totalSize = 0;
    long long totalRows = 0;

    for (const auto& info : filtered_statistics_) {
        totalSize += info.total_size_raw;
        totalRows += info.row_count_raw;
    }

    if (total_tables_label_) {
        total_tables_label_->SetLabel(wxString::Format("%zu", filtered_statistics_.size()));
    }
    if (total_size_label_) {
        total_size_label_->SetLabel(wxString::FromUTF8(FormatBytes(totalSize)));
    }
    if (total_rows_label_) {
        total_rows_label_->SetLabel(wxString::FromUTF8(FormatNumber(totalRows)));
    }
}

void TableStatisticsPanel::ExportToFile() {
    if (filtered_statistics_.empty()) {
        wxMessageBox("No data to export.", "Export", wxOK | wxICON_INFORMATION);
        return;
    }

    wxFileDialog saveDialog(this, "Export Table Statistics", "", 
                            "table_statistics.csv",
                            "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveDialog.ShowModal() != wxID_OK) {
        return;
    }

    std::ofstream file(saveDialog.GetPath().ToStdString());
    if (!file.is_open()) {
        wxMessageBox("Failed to open file for writing.", "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Write header
    for (size_t i = 0; i < kColumnLabels.size(); ++i) {
        if (i > 0) file << ",";
        file << "\"" << kColumnLabels[i] << "\"";
    }
    file << "\n";

    // Write data
    for (const auto& info : filtered_statistics_) {
        file << "\"" << info.schema_name << "\","
             << "\"" << info.table_name << "\","
             << "\"" << info.row_count << "\","
             << "\"" << info.table_size << "\","
             << "\"" << info.index_size << "\","
             << "\"" << info.total_size << "\","
             << "\"" << info.seq_scans << "\","
             << "\"" << info.idx_scans << "\","
             << "\"" << info.n_tup_ins << "\","
             << "\"" << info.n_tup_upd << "\","
             << "\"" << info.n_tup_del << "\","
             << "\"" << info.last_vacuum << "\","
             << "\"" << info.last_analyze << "\"\n";
    }

    file.close();
    UpdateStatus("Export completed");
}

void TableStatisticsPanel::AnalyzeSelected() {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(filtered_statistics_.size())) {
        return;
    }

    const auto& info = filtered_statistics_[selected_row_];
    wxString msg = wxString::Format(
        "Are you sure you want to analyze table %s.%s?\n\n"
        "This will update the statistics for query optimization.",
        info.schema_name, info.table_name);

    int result = wxMessageBox(msg, "Confirm Analyze",
                              wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }

    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }

    std::string sql = "ANALYZE " + info.schema_name + "." + info.table_name + ";";
    maintenance_job_ = connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Analyze completed");
                    RefreshData();
                } else {
                    UpdateStatus("Analyze failed");
                    wxMessageBox(error.empty() ? "Failed to analyze table" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void TableStatisticsPanel::VacuumSelected() {
    if (selected_row_ < 0 || selected_row_ >= static_cast<int>(filtered_statistics_.size())) {
        return;
    }

    const auto& info = filtered_statistics_[selected_row_];
    wxString msg = wxString::Format(
        "Are you sure you want to vacuum table %s.%s?\n\n"
        "This will reclaim storage and update statistics. "
        "The table may be locked during the operation.",
        info.schema_name, info.table_name);

    int result = wxMessageBox(msg, "Confirm Vacuum",
                              wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }

    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }

    std::string sql = "VACUUM ANALYZE " + info.schema_name + "." + info.table_name + ";";
    maintenance_job_ = connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Vacuum completed");
                    RefreshData();
                } else {
                    UpdateStatus("Vacuum failed");
                    wxMessageBox(error.empty() ? "Failed to vacuum table" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void TableStatisticsPanel::AnalyzeAll() {
    if (filtered_statistics_.empty()) {
        wxMessageBox("No tables to analyze.",
                     "No Tables", wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxString msg = wxString::Format(
        "Are you sure you want to analyze all %zu visible tables?\n\n"
        "This will update statistics for query optimization.",
        filtered_statistics_.size());
    
    int result = wxMessageBox(msg, "Confirm Analyze All",
                              wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }
    
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }
    
    // Build SQL to analyze all visible tables
    std::string sql;
    for (const auto& info : filtered_statistics_) {
        if (!sql.empty()) {
            sql += "\n";
        }
        sql += "ANALYZE " + info.schema_name + "." + info.table_name + ";";
    }
    
    UpdateStatus("Analyzing all tables...");
    maintenance_job_ = connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Analyze all completed");
                    RefreshData();
                } else {
                    UpdateStatus("Analyze all failed");
                    wxMessageBox(error.empty() ? "Failed to analyze tables" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void TableStatisticsPanel::VacuumAll() {
    if (filtered_statistics_.empty()) {
        wxMessageBox("No tables to vacuum.",
                     "No Tables", wxOK | wxICON_INFORMATION);
        return;
    }
    
    wxString msg = wxString::Format(
        "Are you sure you want to vacuum all %zu visible tables?\n\n"
        "This will reclaim storage and update statistics. "
        "Tables may be locked during the operation.",
        filtered_statistics_.size());
    
    int result = wxMessageBox(msg, "Confirm Vacuum All",
                              wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);
    if (result != wxYES) {
        return;
    }
    
    if (!connection_manager_ || !connection_manager_->IsConnected()) {
        return;
    }
    
    // Build SQL to vacuum all visible tables
    std::string sql;
    for (const auto& info : filtered_statistics_) {
        if (!sql.empty()) {
            sql += "\n";
        }
        sql += "VACUUM ANALYZE " + info.schema_name + "." + info.table_name + ";";
    }
    
    UpdateStatus("Vacuuming all tables...");
    maintenance_job_ = connection_manager_->ExecuteQueryAsync(sql,
        [this](bool ok, QueryResult, const std::string& error) {
            CallAfter([this, ok, error]() {
                if (ok) {
                    UpdateStatus("Vacuum all completed");
                    RefreshData();
                } else {
                    UpdateStatus("Vacuum all failed");
                    wxMessageBox(error.empty() ? "Failed to vacuum tables" : error,
                                 "Error", wxOK | wxICON_ERROR);
                }
            });
        });
}

void TableStatisticsPanel::SetAutoRefresh(bool enable, int intervalSeconds) {
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

bool TableStatisticsPanel::IsAutoRefreshEnabled() const {
    return auto_refresh_check_ && auto_refresh_check_->IsChecked();
}

void TableStatisticsPanel::UpdateControls() {
    bool connected = connection_manager_ && connection_manager_->IsConnected();
    bool has_selection = selected_row_ >= 0 && 
                         selected_row_ < static_cast<int>(filtered_statistics_.size());

    if (refresh_button_) {
        refresh_button_->Enable(connected && !query_running_);
    }
    if (export_button_) {
        export_button_->Enable(!filtered_statistics_.empty());
    }
    if (analyze_button_) {
        analyze_button_->Enable(connected && has_selection && !query_running_);
    }
    if (vacuum_button_) {
        vacuum_button_->Enable(connected && has_selection && !query_running_);
    }
}

void TableStatisticsPanel::UpdateStatus(const std::string& message) {
    if (status_label_) {
        status_label_->SetLabel(wxString::FromUTF8(message));
    }
}

void TableStatisticsPanel::OnRefresh(wxCommandEvent&) {
    RefreshData();
}

void TableStatisticsPanel::OnExport(wxCommandEvent&) {
    ExportToFile();
}

void TableStatisticsPanel::OnAnalyze(wxCommandEvent&) {
    AnalyzeSelected();
}

void TableStatisticsPanel::OnVacuum(wxCommandEvent&) {
    VacuumSelected();
}

void TableStatisticsPanel::OnAutoRefreshToggle(wxCommandEvent&) {
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

void TableStatisticsPanel::OnIntervalChanged(wxCommandEvent&) {
    if (auto_refresh_check_->IsChecked()) {
        int intervals[] = {30, 60, 300, 900};  // seconds
        int idx = interval_choice_->GetSelection();
        if (idx >= 0 && idx < 4) {
            refresh_timer_->Stop();
            refresh_timer_->Start(intervals[idx] * 1000);
        }
    }
}

void TableStatisticsPanel::OnSortChanged(wxCommandEvent&) {
    int selection = sort_choice_->GetSelection();
    switch (selection) {
        case 0:
            current_sort_ = TableSortOrder::BySizeDesc;
            break;
        case 1:
            current_sort_ = TableSortOrder::BySizeAsc;
            break;
        case 2:
            current_sort_ = TableSortOrder::ByRowCountDesc;
            break;
        case 3:
            current_sort_ = TableSortOrder::ByRowCountAsc;
            break;
        case 4:
            current_sort_ = TableSortOrder::ByScanCountDesc;
            break;
        case 5:
            current_sort_ = TableSortOrder::ByScanCountAsc;
            break;
        case 6:
            current_sort_ = TableSortOrder::ByModificationTimeDesc;
            break;
    }
    ApplySort();
    UpdateGrid();
}

void TableStatisticsPanel::OnFilterChanged(wxCommandEvent&) {
    ApplyFilters();
}

void TableStatisticsPanel::OnGridSelect(wxGridEvent& event) {
    selected_row_ = event.GetRow();
    UpdateControls();
}

void TableStatisticsPanel::OnTimer(wxTimerEvent&) {
    if (!query_running_) {
        LoadStatistics();
    }
}

} // namespace scratchrobin
