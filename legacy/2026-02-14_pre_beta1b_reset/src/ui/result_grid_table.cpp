/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "result_grid_table.h"

#include <wx/grid.h>

#include "core/value_formatter.h"

namespace scratchrobin {

int ResultGridTable::GetNumberRows() {
    return static_cast<int>(rows_.size());
}

int ResultGridTable::GetNumberCols() {
    return static_cast<int>(column_labels_.size());
}

wxString ResultGridTable::GetValue(int row, int col) {
    if (row < 0 || col < 0) {
        return wxString();
    }
    if (static_cast<size_t>(row) >= rows_.size()) {
        return wxString();
    }
    if (static_cast<size_t>(col) >= rows_[row].size()) {
        return wxString();
    }
    return wxString(rows_[row][col]);
}

bool ResultGridTable::IsEmptyCell(int row, int col) {
    if (row < 0 || col < 0) {
        return true;
    }
    if (static_cast<size_t>(row) >= rows_.size()) {
        return true;
    }
    if (static_cast<size_t>(col) >= rows_[row].size()) {
        return true;
    }
    return rows_[row][col].empty();
}

wxString ResultGridTable::GetColLabelValue(int col) {
    if (col < 0) {
        return wxString();
    }
    if (static_cast<size_t>(col) >= column_labels_.size()) {
        return wxString();
    }
    return wxString(column_labels_[col]);
}

void ResultGridTable::Reset(const std::vector<QueryColumn>& columns,
                            const std::vector<std::vector<QueryValue>>& rows) {
    int old_rows = GetNumberRows();
    int old_cols = GetNumberCols();

    column_labels_.clear();
    column_types_.clear();
    if (!columns.empty()) {
        column_labels_.reserve(columns.size());
        column_types_.reserve(columns.size());
        for (const auto& col : columns) {
            column_labels_.push_back(col.name);
            column_types_.push_back(col.type);
        }
    } else if (!rows.empty()) {
        column_labels_.reserve(rows.front().size());
        column_types_.reserve(rows.front().size());
        for (size_t i = 0; i < rows.front().size(); ++i) {
            column_labels_.push_back("col" + std::to_string(i + 1));
            column_types_.push_back("UNKNOWN");
        }
    }

    rows_.clear();
    rows_.reserve(rows.size());
    FormatOptions format_options;
    for (const auto& row : rows) {
        std::vector<std::string> out_row;
        out_row.reserve(row.size());
        for (size_t i = 0; i < row.size(); ++i) {
            const std::string& type = (i < column_types_.size()) ? column_types_[i] : std::string();
            out_row.push_back(FormatValueForDisplay(row[i], type, format_options));
        }
        rows_.push_back(std::move(out_row));
    }

    NotifyViewReset(old_rows, old_cols, GetNumberRows(), GetNumberCols());
}

void ResultGridTable::Clear() {
    int old_rows = GetNumberRows();
    int old_cols = GetNumberCols();
    column_labels_.clear();
    column_types_.clear();
    rows_.clear();
    NotifyViewReset(old_rows, old_cols, 0, 0);
}

void ResultGridTable::AppendRows(const std::vector<std::vector<QueryValue>>& rows) {
    int old_rows = GetNumberRows();
    FormatOptions format_options;
    for (const auto& row : rows) {
        std::vector<std::string> out_row;
        out_row.reserve(row.size());
        for (size_t i = 0; i < row.size(); ++i) {
            const std::string& type = (i < column_types_.size()) ? column_types_[i] : std::string();
            out_row.push_back(FormatValueForDisplay(row[i], type, format_options));
        }
        rows_.push_back(std::move(out_row));
    }
    int new_rows = GetNumberRows();
    if (auto* view = GetView()) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, new_rows - old_rows);
        view->ProcessTableMessage(msg);
    }
}

void ResultGridTable::NotifyViewReset(int old_rows,
                                      int old_cols,
                                      int new_rows,
                                      int new_cols) {
    auto* view = GetView();
    if (!view) {
        return;
    }

    view->BeginBatch();
    if (old_rows > 0) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, old_rows);
        view->ProcessTableMessage(msg);
    }
    if (old_cols > 0) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED, 0, old_cols);
        view->ProcessTableMessage(msg);
    }
    if (new_cols > 0) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED, new_cols);
        view->ProcessTableMessage(msg);
    }
    if (new_rows > 0) {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, new_rows);
        view->ProcessTableMessage(msg);
    }
    view->EndBatch();
    view->ForceRefresh();
}

} // namespace scratchrobin
