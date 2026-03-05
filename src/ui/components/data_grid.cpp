/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/data_grid.h"

#include <wx/clipbrd.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(DataGrid, wxPanel)
  EVT_GRID_CELL_RIGHT_CLICK(DataGrid::OnGridCellRightClick)
  EVT_GRID_CELL_LEFT_DCLICK(DataGrid::OnGridCellDoubleClick)
  EVT_GRID_SELECT_CELL(DataGrid::OnGridSelectionChanged)
  EVT_GRID_COL_SORT(DataGrid::OnGridColSort)
  EVT_KEY_DOWN(DataGrid::OnKeyDown)
wxEND_EVENT_TABLE()

DataGrid::DataGrid(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
  CreateControls();
  SetupLayout();
  BindEvents();
}

DataGrid::~DataGrid() {
  // Cleanup handled by wxWidgets
}

void DataGrid::CreateControls() {
  grid_ = new wxGrid(this, wxID_ANY);
  grid_->CreateGrid(0, 0);
  grid_->EnableEditing(false);
  grid_->SetSelectionMode(wxGrid::wxGridSelectCells);
}

void DataGrid::SetupLayout() {
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(grid_, 1, wxEXPAND | wxALL, 0);
  SetSizer(sizer);
}

void DataGrid::BindEvents() {
  // Additional event bindings if needed
}

void DataGrid::LoadData(const std::shared_ptr<core::ResultSet>& result_set) {
  current_result_set_ = result_set;
  
  if (!result_set) {
    ClearData();
    return;
  }

  // Clear existing data
  grid_->ClearGrid();
  
  if (grid_->GetNumberRows() > 0) {
    grid_->DeleteRows(0, grid_->GetNumberRows());
  }
  if (grid_->GetNumberCols() > 0) {
    grid_->DeleteCols(0, grid_->GetNumberCols());
  }

  // TODO: Populate grid with result_set data
  // This would iterate through the result set and add rows/columns
}

void DataGrid::ClearData() {
  current_result_set_.reset();
  grid_->ClearGrid();
  
  if (grid_->GetNumberRows() > 0) {
    grid_->DeleteRows(0, grid_->GetNumberRows());
  }
  if (grid_->GetNumberCols() > 0) {
    grid_->DeleteCols(0, grid_->GetNumberCols());
  }
}

int DataGrid::GetSelectedRowCount() const {
  return grid_->GetSelectedRows().GetCount();
}

std::vector<int> DataGrid::GetSelectedRows() const {
  std::vector<int> rows;
  const wxArrayInt selected = grid_->GetSelectedRows();
  for (size_t i = 0; i < selected.GetCount(); ++i) {
    rows.push_back(selected[i]);
  }
  return rows;
}

void DataGrid::CopySelectionToClipboard() {
  // TODO: Implement copy to clipboard
  wxMessageBox(_("Copy to clipboard not yet implemented"),
               _("Copy"), wxOK | wxICON_INFORMATION);
}

bool DataGrid::ExportToCSV(const wxString& filename) {
  // TODO: Implement CSV export
  wxMessageBox(_("CSV export not yet implemented"),
               _("Export"), wxOK | wxICON_INFORMATION);
  return false;
}

void DataGrid::OnGridCellRightClick(wxGridEvent& event) {
  // TODO: Show context menu
  event.Skip();
}

void DataGrid::OnGridCellDoubleClick(wxGridEvent& event) {
  // TODO: Handle cell double-click (e.g., open editor)
  event.Skip();
}

void DataGrid::OnGridSelectionChanged(wxGridEvent& event) {
  // TODO: Handle selection change
  event.Skip();
}

void DataGrid::OnGridColSort(wxGridEvent& event) {
  // TODO: Handle column sorting
  event.Skip();
}

void DataGrid::OnKeyDown(wxKeyEvent& event) {
  // TODO: Handle keyboard shortcuts (Ctrl+C, etc.)
  event.Skip();
}

}  // namespace scratchrobin::ui
