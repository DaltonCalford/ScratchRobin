/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "migration_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_MIGRATION_LIST = wxID_HIGHEST + 1,
  ID_RUN_MIGRATION,
  ID_ROLLBACK,
  ID_REFRESH,
};

wxBEGIN_EVENT_TABLE(MigrationDialog, wxDialog)
  EVT_BUTTON(ID_RUN_MIGRATION, MigrationDialog::OnRunMigration)
  EVT_BUTTON(ID_ROLLBACK, MigrationDialog::OnRollback)
  EVT_BUTTON(ID_REFRESH, MigrationDialog::OnRefresh)
  EVT_BUTTON(wxID_CLOSE, MigrationDialog::OnClose)
wxEND_EVENT_TABLE()

MigrationDialog::MigrationDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Database Migration"), wxDefaultPosition,
               wxSize(700, 550)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

MigrationDialog::~MigrationDialog() {
  // Cleanup handled by wxWidgets
}

void MigrationDialog::CreateControls() {
  // Migration list
  migration_list_ = new wxListCtrl(this, ID_MIGRATION_LIST, wxDefaultPosition,
                                    wxDefaultSize, wxLC_REPORT);
  
  // Log text
  log_text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_READONLY);
  
  // Progress bar
  progress_bar_ = new wxGauge(this, wxID_ANY, 100);
  
  // Buttons
  run_btn_ = new wxButton(this, ID_RUN_MIGRATION, _("&Run Migration"));
  rollback_btn_ = new wxButton(this, ID_ROLLBACK, _("&Rollback"));
  refresh_btn_ = new wxButton(this, ID_REFRESH, _("&Refresh"));
  close_btn_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void MigrationDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Migration list
  wxStaticBoxSizer* list_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Available Migrations"));
  list_sizer->Add(migration_list_, 1, wxEXPAND | wxALL, 5);
  main_sizer->Add(list_sizer, 1, wxEXPAND | wxALL, 5);
  
  // Progress bar
  wxBoxSizer* progress_sizer = new wxBoxSizer(wxHORIZONTAL);
  progress_sizer->Add(new wxStaticText(this, wxID_ANY, _("Progress:")),
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  progress_sizer->Add(progress_bar_, 1, wxEXPAND);
  main_sizer->Add(progress_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Log area
  wxStaticBoxSizer* log_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Log"));
  log_sizer->Add(log_text_, 1, wxEXPAND | wxALL, 5);
  main_sizer->Add(log_sizer, 1, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(run_btn_, 0, wxALL, 5);
  btn_sizer->Add(rollback_btn_, 0, wxALL, 5);
  btn_sizer->Add(refresh_btn_, 0, wxALL, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(close_btn_, 0, wxALL, 5);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void MigrationDialog::BindEvents() {
  // Additional event bindings if needed
}

void MigrationDialog::OnRunMigration(wxCommandEvent& event) {
  // TODO: Implement
}

void MigrationDialog::OnRollback(wxCommandEvent& event) {
  // TODO: Implement
}

void MigrationDialog::OnRefresh(wxCommandEvent& event) {
  // TODO: Implement
}

void MigrationDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
