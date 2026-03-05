/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "query_profiler_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_QUERY_TEXT = wxID_HIGHEST + 1,
  ID_RUN_PROFILE,
  ID_CLEAR,
  ID_EXPORT,
};

wxBEGIN_EVENT_TABLE(QueryProfilerDialog, wxDialog)
  EVT_BUTTON(ID_RUN_PROFILE, QueryProfilerDialog::OnRunProfile)
  EVT_BUTTON(ID_CLEAR, QueryProfilerDialog::OnClear)
  EVT_BUTTON(ID_EXPORT, QueryProfilerDialog::OnExport)
  EVT_BUTTON(wxID_CLOSE, QueryProfilerDialog::OnClose)
wxEND_EVENT_TABLE()

QueryProfilerDialog::QueryProfilerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Query Profiler"), wxDefaultPosition,
               wxSize(800, 600)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

QueryProfilerDialog::~QueryProfilerDialog() {
  // Cleanup handled by wxWidgets
}

void QueryProfilerDialog::CreateControls() {
  // Query text
  query_text_ = new wxTextCtrl(this, ID_QUERY_TEXT, wxEmptyString,
                                wxDefaultPosition, wxSize(-1, 100),
                                wxTE_MULTILINE);
  
  // Results notebook
  results_notebook_ = new wxNotebook(this, wxID_ANY);
  
  // Timing list
  timing_list_ = new wxListCtrl(results_notebook_, wxID_ANY,
                                 wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
  
  // Plan list
  plan_list_ = new wxListCtrl(results_notebook_, wxID_ANY,
                              wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
  
  // Stats text
  stats_text_ = new wxTextCtrl(results_notebook_, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY);
  
  // Add pages to notebook
  results_notebook_->AddPage(timing_list_, _("Timing"));
  results_notebook_->AddPage(plan_list_, _("Query Plan"));
  results_notebook_->AddPage(stats_text_, _("Statistics"));
  
  // Buttons
  run_btn_ = new wxButton(this, ID_RUN_PROFILE, _("&Run Profile"));
  clear_btn_ = new wxButton(this, ID_CLEAR, _("&Clear"));
  export_btn_ = new wxButton(this, ID_EXPORT, _("&Export..."));
  close_btn_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void QueryProfilerDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Query section
  wxStaticBoxSizer* query_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("SQL Query"));
  query_sizer->Add(query_text_, 0, wxEXPAND | wxALL, 5);
  main_sizer->Add(query_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Results notebook
  main_sizer->Add(results_notebook_, 1, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(run_btn_, 0, wxALL, 5);
  btn_sizer->Add(clear_btn_, 0, wxALL, 5);
  btn_sizer->Add(export_btn_, 0, wxALL, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(close_btn_, 0, wxALL, 5);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void QueryProfilerDialog::BindEvents() {
  // Additional event bindings if needed
}

void QueryProfilerDialog::OnRunProfile(wxCommandEvent& event) {
  // TODO: Implement
}

void QueryProfilerDialog::OnClear(wxCommandEvent& event) {
  // TODO: Implement
}

void QueryProfilerDialog::OnExport(wxCommandEvent& event) {
  // TODO: Implement
}

void QueryProfilerDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
