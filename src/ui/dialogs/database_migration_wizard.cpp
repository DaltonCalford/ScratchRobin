/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "database_migration_wizard.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_PREVIOUS = wxID_HIGHEST + 1,
  ID_NEXT,
  ID_FINISH,
};

wxBEGIN_EVENT_TABLE(DatabaseMigrationWizard, wxDialog)
  EVT_BUTTON(ID_PREVIOUS, DatabaseMigrationWizard::OnPrevious)
  EVT_BUTTON(ID_NEXT, DatabaseMigrationWizard::OnNext)
  EVT_BUTTON(ID_FINISH, DatabaseMigrationWizard::OnFinish)
  EVT_BUTTON(wxID_CANCEL, DatabaseMigrationWizard::OnCancel)
wxEND_EVENT_TABLE()

DatabaseMigrationWizard::DatabaseMigrationWizard(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Database Migration Wizard"), wxDefaultPosition,
               wxSize(700, 500)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Show first page
  ShowPage(0);
  
  // Center on parent
  CentreOnParent();
}

DatabaseMigrationWizard::~DatabaseMigrationWizard() {
  // Cleanup handled by wxWidgets
}

void DatabaseMigrationWizard::CreateControls() {
  // Page container
  page_container_ = new wxPanel(this, wxID_ANY);
  
  // Navigation buttons
  prev_btn_ = new wxButton(this, ID_PREVIOUS, _("< &Back"));
  next_btn_ = new wxButton(this, ID_NEXT, _("&Next >"));
  finish_btn_ = new wxButton(this, ID_FINISH, _("&Finish"));
  cancel_btn_ = new wxButton(this, wxID_CANCEL, _("&Cancel"));
  
  // Initially disable previous and finish
  prev_btn_->Enable(false);
  finish_btn_->Enable(false);
}

void DatabaseMigrationWizard::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Title
  main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Database Migration Wizard")),
                  0, wxALIGN_CENTER | wxALL, 10);
  
  // Page container
  main_sizer->Add(page_container_, 1, wxEXPAND | wxALL, 5);
  
  // Separator line
  main_sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(prev_btn_, 0, wxALL, 5);
  btn_sizer->Add(next_btn_, 0, wxALL, 5);
  btn_sizer->Add(finish_btn_, 0, wxALL, 5);
  btn_sizer->Add(cancel_btn_, 0, wxALL, 5);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void DatabaseMigrationWizard::BindEvents() {
  // Additional event bindings if needed
}

void DatabaseMigrationWizard::ShowPage(int page) {
  // TODO: Implement page switching
  current_page_ = page;
  
  // Update button states
  prev_btn_->Enable(current_page_ > 0);
  next_btn_->Enable(current_page_ < kTotalPages - 1);
  finish_btn_->Enable(current_page_ == kTotalPages - 1);
}

void DatabaseMigrationWizard::OnNext(wxCommandEvent& event) {
  if (current_page_ < kTotalPages - 1) {
    ShowPage(current_page_ + 1);
  }
}

void DatabaseMigrationWizard::OnPrevious(wxCommandEvent& event) {
  if (current_page_ > 0) {
    ShowPage(current_page_ - 1);
  }
}

void DatabaseMigrationWizard::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

void DatabaseMigrationWizard::OnFinish(wxCommandEvent& event) {
  // TODO: Implement migration completion
  EndModal(wxID_OK);
}

}  // namespace scratchrobin::ui
