/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "query_progress_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/gauge.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(QueryProgressDialog, wxDialog)
  EVT_BUTTON(wxID_CANCEL, QueryProgressDialog::OnCancel)
wxEND_EVENT_TABLE()

QueryProgressDialog::QueryProgressDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition,
               wxSize(400, 150), wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX) {
  CreateControls();
  SetupLayout();
  BindEvents();
  CentreOnParent();
}

QueryProgressDialog::~QueryProgressDialog() = default;

void QueryProgressDialog::CreateControls() {
  message_label_ = new wxStaticText(this, wxID_ANY, _("Executing query..."));
  progress_bar_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition,
                              wxDefaultSize, wxGA_HORIZONTAL | wxGA_SMOOTH);
  cancel_btn_ = new wxButton(this, wxID_CANCEL, _("&Cancel"));
}

void QueryProgressDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  main_sizer->Add(message_label_, 0, wxEXPAND | wxALL, 10);
  main_sizer->Add(progress_bar_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
  
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(cancel_btn_, 0, wxALL, 5);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND);
  
  SetSizer(main_sizer);
}

void QueryProgressDialog::BindEvents() {
  // Additional event bindings if needed
}

void QueryProgressDialog::SetProgress(int percent, const wxString& message) {
  progress_bar_->SetValue(percent);
  if (!message.IsEmpty()) {
    message_label_->SetLabel(message);
  }
  wxYield();
}

void QueryProgressDialog::SetIndeterminate() {
  // Pulse the progress bar for indeterminate mode
  progress_bar_->Pulse();
  wxYield();
}

bool QueryProgressDialog::WasCancelled() const {
  return was_cancelled_;
}

void QueryProgressDialog::Complete(const wxString& message) {
  is_complete_ = true;
  progress_bar_->SetValue(100);
  if (!message.IsEmpty()) {
    message_label_->SetLabel(message);
  }
  cancel_btn_->SetLabel(_("&Close"));
  cancel_btn_->SetId(wxID_CLOSE);
}

void QueryProgressDialog::OnCancel(wxCommandEvent& event) {
  if (is_complete_) {
    EndModal(wxID_CLOSE);
  } else {
    was_cancelled_ = true;
    message_label_->SetLabel(_("Cancelling..."));
  }
}

void QueryProgressDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
