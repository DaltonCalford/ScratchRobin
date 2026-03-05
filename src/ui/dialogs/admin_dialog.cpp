/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "admin_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_ADMIN_LIST = wxID_HIGHEST + 1,
};

wxBEGIN_EVENT_TABLE(AdminDialog, wxDialog)
  EVT_BUTTON(wxID_CLOSE, AdminDialog::OnClose)
wxEND_EVENT_TABLE()

AdminDialog::AdminDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Database Administration"), wxDefaultPosition,
               wxSize(600, 400)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

AdminDialog::~AdminDialog() {
  // Cleanup handled by wxWidgets
}

void AdminDialog::CreateControls() {
  // Admin list
  admin_list_ = new wxListCtrl(this, ID_ADMIN_LIST, wxDefaultPosition,
                                wxDefaultSize, wxLC_REPORT);
  
  // Close button
  close_button_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void AdminDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Info text
  main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Administration Tasks")),
                  0, wxALL, 10);
  
  // Admin list
  main_sizer->Add(admin_list_, 1, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxStdDialogButtonSizer* btn_sizer = new wxStdDialogButtonSizer();
  btn_sizer->AddButton(close_button_);
  btn_sizer->Realize();
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void AdminDialog::BindEvents() {
  // Additional event bindings if needed
}

void AdminDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
