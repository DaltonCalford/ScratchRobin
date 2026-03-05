/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "connection_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>

#include "backend/session_client.h"

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_TEST_CONNECTION = wxID_HIGHEST + 1,
  ID_BROWSE_DATABASE,
  ID_SAVE_PROFILE,
  ID_LOAD_PROFILE,
};

wxBEGIN_EVENT_TABLE(ConnectionDialog, wxDialog)
  EVT_BUTTON(ID_TEST_CONNECTION, ConnectionDialog::OnTestConnection)
  EVT_BUTTON(ID_BROWSE_DATABASE, ConnectionDialog::OnBrowseDatabase)
  EVT_BUTTON(ID_SAVE_PROFILE, ConnectionDialog::OnSaveProfile)
  EVT_BUTTON(ID_LOAD_PROFILE, ConnectionDialog::OnLoadProfile)
  EVT_BUTTON(wxID_OK, ConnectionDialog::OnConnect)
  EVT_BUTTON(wxID_CANCEL, ConnectionDialog::OnCancel)
wxEND_EVENT_TABLE()

ConnectionDialog::ConnectionDialog(wxWindow* parent,
                                   scratchrobin::backend::SessionClient* session_client)
    : wxDialog(parent, wxID_ANY, _("Connect to Database"), wxDefaultPosition,
               wxSize(450, 400)),
      session_client_(session_client) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

ConnectionDialog::~ConnectionDialog() {
  // Cleanup handled by wxWidgets
}

void ConnectionDialog::CreateControls() {
  // Host
  host_ctrl_ = new wxTextCtrl(this, wxID_ANY, "localhost");
  
  // Port
  port_ctrl_ = new wxTextCtrl(this, wxID_ANY, "2055");
  
  // Database
  database_ctrl_ = new wxTextCtrl(this, wxID_ANY);
  
  // Username
  username_ctrl_ = new wxTextCtrl(this, wxID_ANY);
  
  // Password
  password_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_PASSWORD);
  
  // Charset
  charset_ctrl_ = new wxChoice(this, wxID_ANY);
  charset_ctrl_->Append("UTF8");
  charset_ctrl_->Append("ASCII");
  charset_ctrl_->Append("ISO8859_1");
  charset_ctrl_->SetSelection(0);
  
  // Role
  role_ctrl_ = new wxTextCtrl(this, wxID_ANY);
  
  // Options
  compression_check_ = new wxCheckBox(this, wxID_ANY, _("Use compression"));
  compression_check_->SetValue(true);
  
  ssl_check_ = new wxCheckBox(this, wxID_ANY, _("Use SSL/TLS"));
  ssl_check_->SetValue(false);
  
  // Saved profiles
  saved_profiles_ctrl_ = new wxChoice(this, wxID_ANY);
  saved_profiles_ctrl_->Append(_("-- Select saved profile --"));
  saved_profiles_ctrl_->SetSelection(0);
}

void ConnectionDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Connection settings box
  wxStaticBoxSizer* conn_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Connection Settings"));
  
  // Host/Port row
  wxFlexGridSizer* host_port_sizer = new wxFlexGridSizer(2, 4, 5, 5);
  host_port_sizer->AddGrowableCol(1, 2);
  host_port_sizer->AddGrowableCol(3, 1);
  host_port_sizer->Add(new wxStaticText(this, wxID_ANY, _("Host:")), 0, wxALIGN_CENTER_VERTICAL);
  host_port_sizer->Add(host_ctrl_, 1, wxEXPAND);
  host_port_sizer->Add(new wxStaticText(this, wxID_ANY, _("Port:")), 0, wxALIGN_CENTER_VERTICAL);
  host_port_sizer->Add(port_ctrl_, 1, wxEXPAND);
  conn_sizer->Add(host_port_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Database row
  wxBoxSizer* db_sizer = new wxBoxSizer(wxHORIZONTAL);
  db_sizer->Add(new wxStaticText(this, wxID_ANY, _("Database:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  db_sizer->Add(database_ctrl_, 1, wxEXPAND);
  db_sizer->Add(new wxButton(this, ID_BROWSE_DATABASE, _("Browse...")), 0, wxLEFT, 5);
  conn_sizer->Add(db_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Authentication box
  wxStaticBoxSizer* auth_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Authentication"));
  
  wxFlexGridSizer* auth_grid = new wxFlexGridSizer(2, 2, 5, 5);
  auth_grid->AddGrowableCol(1, 1);
  auth_grid->Add(new wxStaticText(this, wxID_ANY, _("Username:")), 0, wxALIGN_CENTER_VERTICAL);
  auth_grid->Add(username_ctrl_, 1, wxEXPAND);
  auth_grid->Add(new wxStaticText(this, wxID_ANY, _("Password:")), 0, wxALIGN_CENTER_VERTICAL);
  auth_grid->Add(password_ctrl_, 1, wxEXPAND);
  auth_grid->Add(new wxStaticText(this, wxID_ANY, _("Charset:")), 0, wxALIGN_CENTER_VERTICAL);
  auth_grid->Add(charset_ctrl_, 1, wxEXPAND);
  auth_grid->Add(new wxStaticText(this, wxID_ANY, _("Role:")), 0, wxALIGN_CENTER_VERTICAL);
  auth_grid->Add(role_ctrl_, 1, wxEXPAND);
  auth_sizer->Add(auth_grid, 0, wxEXPAND | wxALL, 5);
  
  // Options box
  wxStaticBoxSizer* options_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Options"));
  options_sizer->Add(compression_check_, 0, wxALL, 5);
  options_sizer->Add(ssl_check_, 0, wxALL, 5);
  
  // Profiles row
  wxBoxSizer* profile_sizer = new wxBoxSizer(wxHORIZONTAL);
  profile_sizer->Add(new wxStaticText(this, wxID_ANY, _("Saved Profiles:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  profile_sizer->Add(saved_profiles_ctrl_, 1, wxEXPAND);
  profile_sizer->Add(new wxButton(this, ID_SAVE_PROFILE, _("Save")), 0, wxLEFT, 5);
  profile_sizer->Add(new wxButton(this, ID_LOAD_PROFILE, _("Load")), 0, wxLEFT, 5);
  
  // Button sizer
  wxStdDialogButtonSizer* btn_sizer = new wxStdDialogButtonSizer();
  btn_sizer->AddButton(new wxButton(this, ID_TEST_CONNECTION, _("&Test")));
  btn_sizer->AddButton(new wxButton(this, wxID_OK, _("&Connect")));
  btn_sizer->AddButton(new wxButton(this, wxID_CANCEL, _("&Cancel")));
  btn_sizer->Realize();
  
  // Assemble main sizer
  main_sizer->Add(conn_sizer, 0, wxEXPAND | wxALL, 5);
  main_sizer->Add(auth_sizer, 0, wxEXPAND | wxALL, 5);
  main_sizer->Add(options_sizer, 0, wxEXPAND | wxALL, 5);
  main_sizer->Add(profile_sizer, 0, wxEXPAND | wxALL, 5);
  main_sizer->AddStretchSpacer(1);
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void ConnectionDialog::BindEvents() {
  // Additional event bindings if needed
}

ConnectionConfig ConnectionDialog::GetConnectionConfig() const {
  ConnectionConfig cfg;
  cfg.host = host_ctrl_->GetValue().ToStdString();
  cfg.port = wxAtoi(port_ctrl_->GetValue());
  cfg.database = database_ctrl_->GetValue().ToStdString();
  cfg.username = username_ctrl_->GetValue().ToStdString();
  cfg.password = password_ctrl_->GetValue().ToStdString();
  cfg.charset = charset_ctrl_->GetStringSelection().ToStdString();
  cfg.role = role_ctrl_->GetValue().ToStdString();
  cfg.use_compression = compression_check_->IsChecked();
  cfg.use_ssl = ssl_check_->IsChecked();
  return cfg;
}

void ConnectionDialog::OnTestConnection(wxCommandEvent& event) {
  // TODO: Implement connection test
  wxMessageBox(_("Connection test not yet implemented"), _("Test Connection"),
               wxOK | wxICON_INFORMATION);
}

void ConnectionDialog::OnConnect(wxCommandEvent& event) {
  // Validate required fields
  if (database_ctrl_->GetValue().IsEmpty()) {
    wxMessageBox(_("Database name is required"), _("Validation Error"),
                 wxOK | wxICON_WARNING);
    database_ctrl_->SetFocus();
    return;
  }
  
  // Store config and close with OK
  config_ = GetConnectionConfig();
  EndModal(wxID_OK);
}

void ConnectionDialog::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

void ConnectionDialog::OnBrowseDatabase(wxCommandEvent& event) {
  wxFileDialog dlg(this, _("Select Database"), "", "",
                   "Database files (*.fdb;*.sdb)|*.fdb;*.sdb|All files (*.*)|*.*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (dlg.ShowModal() == wxID_OK) {
    database_ctrl_->SetValue(dlg.GetPath());
  }
}

void ConnectionDialog::OnSaveProfile(wxCommandEvent& event) {
  // TODO: Implement profile saving
  wxMessageBox(_("Profile saving not yet implemented"), _("Save Profile"),
               wxOK | wxICON_INFORMATION);
}

void ConnectionDialog::OnLoadProfile(wxCommandEvent& event) {
  // TODO: Implement profile loading
  wxMessageBox(_("Profile loading not yet implemented"), _("Load Profile"),
               wxOK | wxICON_INFORMATION);
}

}  // namespace scratchrobin::ui
