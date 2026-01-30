/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_RESULT_GRID_TABLE_H
#define SCRATCHROBIN_RESULT_GRID_TABLE_H

#include <string>
#include <vector>

#include <wx/grid.h>

#include "core/query_types.h"

namespace scratchrobin {

class ResultGridTable : public wxGridTableBase {
public:
    ResultGridTable() = default;

    int GetNumberRows() override;
    int GetNumberCols() override;
    wxString GetValue(int row, int col) override;
    void SetValue(int, int, const wxString&) override {}
    bool IsEmptyCell(int row, int col) override;
    wxString GetColLabelValue(int col) override;

    void Reset(const std::vector<QueryColumn>& columns,
               const std::vector<std::vector<QueryValue>>& rows);
    void Clear();
    void AppendRows(const std::vector<std::vector<QueryValue>>& rows);

private:
    void NotifyViewReset(int old_rows, int old_cols, int new_rows, int new_cols);

    std::vector<std::string> column_labels_;
    std::vector<std::string> column_types_;
    std::vector<std::vector<std::string>> rows_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESULT_GRID_TABLE_H
