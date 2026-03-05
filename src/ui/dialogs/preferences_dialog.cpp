/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "preferences_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/stattext.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
  EVT_BUTTON(wxID_OK, PreferencesDialog::OnOK)
  EVT_BUTTON(wxID_CANCEL, PreferencesDialog::OnCancel)
  EVT_BUTTON(wxID_APPLY, PreferencesDialog::OnApply)
wxEND_EVENT_TABLE()

PreferencesDialog::PreferencesDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Preferences"), wxDefaultPosition,
               wxSize(600, 450)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  CentreOnParent();
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::CreateControls() {
  // Create notebook for preference categories
  // Categories will be added here
}

void PreferencesDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Placeholder text
  main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Preferences dialog - To be implemented")),
                  1, wxALIGN_CENTER | wxALL, 20);
  
  // Button sizer
  wxStdDialogButtonSizer* btn_sizer = new wxStdDialogButtonSizer();
  btn_sizer->AddButton(new wxButton(this, wxID_OK, _("&OK")));
  btn_sizer->AddButton(new wxButton(this, wxID_CANCEL, _("&Cancel")));
  btn_sizer->AddButton(new wxButton(this, wxID_APPLY, _("&Apply")));
  btn_sizer->Realize();
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  SetSizer(main_sizer);
}

void PreferencesDialog::BindEvents() {
  // Additional event bindings
}

void PreferencesDialog::OnOK(wxCommandEvent& event) {
  // TODO: Save preferences
  EndModal(wxID_OK);
}

void PreferencesDialog::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

void PreferencesDialog::OnApply(wxCommandEvent& event) {
  // TODO: Apply preferences without closing
}

}  // namespace scratchrobin::ui
