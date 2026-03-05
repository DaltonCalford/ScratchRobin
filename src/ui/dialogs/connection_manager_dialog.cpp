/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "connection_manager_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_CONNECTION_LIST = wxID_HIGHEST + 1,
  ID_ADD,
  ID_EDIT,
  ID_DELETE,
  ID_CONNECT,
};

wxBEGIN_EVENT_TABLE(ConnectionManagerDialog, wxDialog)
  EVT_BUTTON(ID_ADD, ConnectionManagerDialog::OnAdd)
  EVT_BUTTON(ID_EDIT, ConnectionManagerDialog::OnEdit)
  EVT_BUTTON(ID_DELETE, ConnectionManagerDialog::OnDelete)
  EVT_BUTTON(ID_CONNECT, ConnectionManagerDialog::OnConnect)
  EVT_BUTTON(wxID_CLOSE, ConnectionManagerDialog::OnClose)
wxEND_EVENT_TABLE()

ConnectionManagerDialog::ConnectionManagerDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Connection Manager"), wxDefaultPosition,
               wxSize(600, 450)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

ConnectionManagerDialog::~ConnectionManagerDialog() {
  // Cleanup handled by wxWidgets
}

void ConnectionManagerDialog::CreateControls() {
  // Connection list
  connection_list_ = new wxListCtrl(this, ID_CONNECTION_LIST, wxDefaultPosition,
                                     wxDefaultSize, wxLC_REPORT);
  
  // Buttons
  add_btn_ = new wxButton(this, ID_ADD, _("&Add..."));
  edit_btn_ = new wxButton(this, ID_EDIT, _("&Edit..."));
  delete_btn_ = new wxButton(this, ID_DELETE, _("&Delete"));
  connect_btn_ = new wxButton(this, ID_CONNECT, _("&Connect"));
  close_btn_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void ConnectionManagerDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Info text
  main_sizer->Add(new wxStaticText(this, wxID_ANY,
                                   _("Manage your saved database connections:")),
                  0, wxALL, 10);
  
  // Connection list and buttons
  wxBoxSizer* content_sizer = new wxBoxSizer(wxHORIZONTAL);
  content_sizer->Add(connection_list_, 1, wxEXPAND);
  
  // Button column
  wxBoxSizer* btn_col_sizer = new wxBoxSizer(wxVERTICAL);
  btn_col_sizer->Add(add_btn_, 0, wxEXPAND | wxALL, 2);
  btn_col_sizer->Add(edit_btn_, 0, wxEXPAND | wxALL, 2);
  btn_col_sizer->Add(delete_btn_, 0, wxEXPAND | wxALL, 2);
  btn_col_sizer->AddSpacer(20);
  btn_col_sizer->Add(connect_btn_, 0, wxEXPAND | wxALL, 2);
  content_sizer->Add(btn_col_sizer, 0, wxLEFT, 10);
  
  main_sizer->Add(content_sizer, 1, wxEXPAND | wxALL, 5);
  
  // Close button
  wxStdDialogButtonSizer* btn_sizer = new wxStdDialogButtonSizer();
  btn_sizer->AddButton(close_btn_);
  btn_sizer->Realize();
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void ConnectionManagerDialog::BindEvents() {
  // Additional event bindings if needed
}

void ConnectionManagerDialog::OnAdd(wxCommandEvent& event) {
  // TODO: Implement
}

void ConnectionManagerDialog::OnEdit(wxCommandEvent& event) {
  // TODO: Implement
}

void ConnectionManagerDialog::OnDelete(wxCommandEvent& event) {
  // TODO: Implement
}

void ConnectionManagerDialog::OnConnect(wxCommandEvent& event) {
  // TODO: Implement
}

void ConnectionManagerDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
