/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <memory>
#include <vector>

#include <wx/grid.h>
#include <wx/panel.h>

#include "core/result_set.h"

namespace scratchrobin::ui {

/**
 * DataGrid - A wxGrid-based component for displaying database query results
 */
class DataGrid : public wxPanel {
 public:
  DataGrid(wxWindow* parent, wxWindowID id = wxID_ANY);
  ~DataGrid() override;

  // Load data from a result set
  void LoadData(const std::shared_ptr<core::ResultSet>& result_set);

  // Clear all data
  void ClearData();

  // Get the underlying grid control
  wxGrid* GetGrid() const { return grid_; }

  // Get selected row count
  int GetSelectedRowCount() const;

  // Get selected rows
  std::vector<int> GetSelectedRows() const;

  // Copy selected cells to clipboard
  void CopySelectionToClipboard();

  // Export to CSV
  bool ExportToCSV(const wxString& filename);

 private:
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  void OnGridCellRightClick(wxGridEvent& event);
  void OnGridCellDoubleClick(wxGridEvent& event);
  void OnGridSelectionChanged(wxGridEvent& event);
  void OnGridColSort(wxGridEvent& event);
  void OnKeyDown(wxKeyEvent& event);

  wxGrid* grid_{nullptr};
  std::shared_ptr<core::ResultSet> current_result_set_;

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
