/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/components/data_grid_paginated.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "ui/components/data_grid.h"

namespace scratchrobin::ui {

enum {
  ID_FIRST_PAGE = wxID_HIGHEST + 1,
  ID_PREV_PAGE,
  ID_NEXT_PAGE,
  ID_LAST_PAGE,
  ID_PAGE_SIZE,
  ID_GO_TO_PAGE,
};

wxBEGIN_EVENT_TABLE(DataGridPaginated, wxPanel)
  EVT_BUTTON(ID_FIRST_PAGE, DataGridPaginated::OnFirstPage)
  EVT_BUTTON(ID_PREV_PAGE, DataGridPaginated::OnPreviousPage)
  EVT_BUTTON(ID_NEXT_PAGE, DataGridPaginated::OnNextPage)
  EVT_BUTTON(ID_LAST_PAGE, DataGridPaginated::OnLastPage)
  EVT_BUTTON(ID_GO_TO_PAGE, DataGridPaginated::OnGoToPage)
  EVT_TEXT_ENTER(ID_PAGE_SIZE, DataGridPaginated::OnPageSizeChanged)
wxEND_EVENT_TABLE()

DataGridPaginated::DataGridPaginated(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
  CreateControls();
  SetupLayout();
}

DataGridPaginated::~DataGridPaginated() {
  // Cleanup handled by wxWidgets
}

void DataGridPaginated::CreateControls() {
  data_grid_ = new DataGrid(this);
  
  btn_first_ = new wxButton(this, ID_FIRST_PAGE, _("<<"));
  btn_prev_ = new wxButton(this, ID_PREV_PAGE, _("<"));
  btn_next_ = new wxButton(this, ID_NEXT_PAGE, _(">"));
  btn_last_ = new wxButton(this, ID_LAST_PAGE, _(">>"));
  
  txt_page_ = new wxTextCtrl(this, ID_PAGE_SIZE, _("1"),
                             wxDefaultPosition, wxSize(50, -1));
  
  lbl_page_info_ = new wxStaticText(this, wxID_ANY, _("Page 1 of 1"));
  
  UpdatePaginationControls();
}

void DataGridPaginated::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Data grid
  main_sizer->Add(data_grid_, 1, wxEXPAND | wxALL, 5);
  
  // Pagination controls
  wxBoxSizer* pagination_sizer = new wxBoxSizer(wxHORIZONTAL);
  
  pagination_sizer->Add(btn_first_, 0, wxALL, 2);
  pagination_sizer->Add(btn_prev_, 0, wxALL, 2);
  pagination_sizer->Add(lbl_page_info_, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
  pagination_sizer->Add(txt_page_, 0, wxALL, 2);
  pagination_sizer->Add(new wxButton(this, ID_GO_TO_PAGE, _("Go")), 0, wxALL, 2);
  pagination_sizer->Add(btn_next_, 0, wxALL, 2);
  pagination_sizer->Add(btn_last_, 0, wxALL, 2);
  
  pagination_sizer->AddStretchSpacer(1);
  
  main_sizer->Add(pagination_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void DataGridPaginated::BindEvents() {
  // Additional event bindings if needed
}

void DataGridPaginated::SetFetchPageCallback(FetchPageCallback callback) {
  fetch_callback_ = callback;
}

bool DataGridPaginated::GoToPage(int page_number) {
  if (page_number < 1 || page_number > total_pages_) {
    return false;
  }
  
  current_page_ = page_number;
  
  if (fetch_callback_) {
    if (!fetch_callback_(current_page_, page_size_)) {
      return false;
    }
  }
  
  UpdatePaginationControls();
  return true;
}

bool DataGridPaginated::GoToNextPage() {
  return GoToPage(current_page_ + 1);
}

bool DataGridPaginated::GoToPreviousPage() {
  return GoToPage(current_page_ - 1);
}

void DataGridPaginated::SetPageSize(int size) {
  if (size > 0) {
    page_size_ = size;
    if (total_row_count_ > 0) {
      total_pages_ = static_cast<int>((total_row_count_ + page_size_ - 1) / page_size_);
    }
    UpdatePaginationControls();
  }
}

void DataGridPaginated::SetTotalRowCount(int64_t count) {
  total_row_count_ = count;
  if (total_row_count_ > 0) {
    total_pages_ = static_cast<int>((total_row_count_ + page_size_ - 1) / page_size_);
  } else {
    total_pages_ = 1;
  }
  UpdatePaginationControls();
}

bool DataGridPaginated::RefreshCurrentPage() {
  return GoToPage(current_page_);
}

DataGrid* DataGridPaginated::GetDataGrid() const {
  return data_grid_;
}

void DataGridPaginated::UpdatePaginationControls() {
  lbl_page_info_->SetLabel(wxString::Format(_("Page %d of %d"), 
                                           current_page_, total_pages_));
  
  btn_first_->Enable(current_page_ > 1);
  btn_prev_->Enable(current_page_ > 1);
  btn_next_->Enable(current_page_ < total_pages_);
  btn_last_->Enable(current_page_ < total_pages_);
  
  txt_page_->SetValue(wxString::Format("%d", current_page_));
}

void DataGridPaginated::OnFirstPage(wxCommandEvent& event) {
  GoToPage(1);
}

void DataGridPaginated::OnPreviousPage(wxCommandEvent& event) {
  GoToPreviousPage();
}

void DataGridPaginated::OnNextPage(wxCommandEvent& event) {
  GoToNextPage();
}

void DataGridPaginated::OnLastPage(wxCommandEvent& event) {
  GoToPage(total_pages_);
}

void DataGridPaginated::OnPageSizeChanged(wxCommandEvent& event) {
  long new_size;
  if (txt_page_->GetValue().ToLong(&new_size) && new_size > 0) {
    SetPageSize(static_cast<int>(new_size));
    GoToPage(1);
  }
}

void DataGridPaginated::OnGoToPage(wxCommandEvent& event) {
  long page;
  if (txt_page_->GetValue().ToLong(&page)) {
    GoToPage(static_cast<int>(page));
  }
}

}  // namespace scratchrobin::ui
