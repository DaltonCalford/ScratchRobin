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

#include <functional>
#include <memory>

#include <wx/panel.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>

namespace scratchrobin::ui {

class DataGrid;

/**
 * DataGridPaginated - A paginated data grid with navigation controls
 */
class DataGridPaginated : public wxPanel {
 public:
  // Callback for fetching a page of data
  using FetchPageCallback = std::function<bool(int page_number, int page_size)>;

  DataGridPaginated(wxWindow* parent, wxWindowID id = wxID_ANY);
  ~DataGridPaginated() override;

  // Set the callback for fetching pages
  void SetFetchPageCallback(FetchPageCallback callback);

  // Navigate to a specific page
  bool GoToPage(int page_number);

  // Navigate to next/previous page
  bool GoToNextPage();
  bool GoToPreviousPage();

  // Get current page info
  int GetCurrentPage() const { return current_page_; }
  int GetTotalPages() const { return total_pages_; }
  int GetPageSize() const { return page_size_; }

  // Set page size (rows per page)
  void SetPageSize(int size);

  // Set total row count (needed for pagination)
  void SetTotalRowCount(int64_t count);

  // Refresh current page
  bool RefreshCurrentPage();

  // Get the underlying DataGrid
  DataGrid* GetDataGrid() const;

 private:
  void CreateControls();
  void SetupLayout();
  void BindEvents();
  void UpdatePaginationControls();

  void OnFirstPage(wxCommandEvent& event);
  void OnPreviousPage(wxCommandEvent& event);
  void OnNextPage(wxCommandEvent& event);
  void OnLastPage(wxCommandEvent& event);
  void OnPageSizeChanged(wxCommandEvent& event);
  void OnGoToPage(wxCommandEvent& event);

  DataGrid* data_grid_{nullptr};
  FetchPageCallback fetch_callback_;

  int current_page_{1};
  int total_pages_{1};
  int page_size_{100};
  int64_t total_row_count_{0};

  // UI Controls
  wxButton* btn_first_{nullptr};
  wxButton* btn_prev_{nullptr};
  wxButton* btn_next_{nullptr};
  wxButton* btn_last_{nullptr};
  wxTextCtrl* txt_page_{nullptr};
  wxStaticText* lbl_page_info_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
