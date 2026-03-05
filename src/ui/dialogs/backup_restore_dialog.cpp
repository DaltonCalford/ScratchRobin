/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "backup_restore_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/filedlg.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_BROWSE_SOURCE = wxID_HIGHEST + 1,
  ID_BROWSE_TARGET,
  ID_BACKUP,
  ID_RESTORE,
};

wxBEGIN_EVENT_TABLE(BackupRestoreDialog, wxDialog)
  EVT_BUTTON(ID_BROWSE_SOURCE, BackupRestoreDialog::OnBrowseSource)
  EVT_BUTTON(ID_BROWSE_TARGET, BackupRestoreDialog::OnBrowseTarget)
  EVT_BUTTON(ID_BACKUP, BackupRestoreDialog::OnBackup)
  EVT_BUTTON(ID_RESTORE, BackupRestoreDialog::OnRestore)
  EVT_BUTTON(wxID_CANCEL, BackupRestoreDialog::OnCancel)
wxEND_EVENT_TABLE()

BackupRestoreDialog::BackupRestoreDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Backup / Restore Database"), wxDefaultPosition,
               wxSize(550, 400)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

BackupRestoreDialog::~BackupRestoreDialog() {
  // Cleanup handled by wxWidgets
}

void BackupRestoreDialog::CreateControls() {
  // Operation type
  wxArrayString choices;
  choices.Add(_("Backup"));
  choices.Add(_("Restore"));
  operation_type_ = new wxRadioBox(this, wxID_ANY, _("Operation"),
                                    wxDefaultPosition, wxDefaultSize, choices, 2);
  
  // Source path
  source_path_ = new wxTextCtrl(this, wxID_ANY);
  browse_source_btn_ = new wxButton(this, ID_BROWSE_SOURCE, _("Browse..."));
  
  // Target path
  target_path_ = new wxTextCtrl(this, wxID_ANY);
  browse_target_btn_ = new wxButton(this, ID_BROWSE_TARGET, _("Browse..."));
  
  // Options
  verbose_check_ = new wxCheckBox(this, wxID_ANY, _("Verbose output"));
  metadata_only_ = new wxCheckBox(this, wxID_ANY, _("Metadata only (no data)"));
  
  // Buttons
  backup_btn_ = new wxButton(this, ID_BACKUP, _("&Backup"));
  restore_btn_ = new wxButton(this, ID_RESTORE, _("&Restore"));
  cancel_btn_ = new wxButton(this, wxID_CANCEL, _("&Cancel"));
}

void BackupRestoreDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Operation type
  main_sizer->Add(operation_type_, 0, wxEXPAND | wxALL, 10);
  
  // Source file
  wxBoxSizer* source_sizer = new wxBoxSizer(wxHORIZONTAL);
  source_sizer->Add(new wxStaticText(this, wxID_ANY, _("Source:")),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  source_sizer->Add(source_path_, 1, wxEXPAND);
  source_sizer->Add(browse_source_btn_, 0, wxLEFT, 5);
  main_sizer->Add(source_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Target file
  wxBoxSizer* target_sizer = new wxBoxSizer(wxHORIZONTAL);
  target_sizer->Add(new wxStaticText(this, wxID_ANY, _("Target:")),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  target_sizer->Add(target_path_, 1, wxEXPAND);
  target_sizer->Add(browse_target_btn_, 0, wxLEFT, 5);
  main_sizer->Add(target_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Options
  wxStaticBoxSizer* options_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Options"));
  options_sizer->Add(verbose_check_, 0, wxALL, 5);
  options_sizer->Add(metadata_only_, 0, wxALL, 5);
  main_sizer->Add(options_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(backup_btn_, 0, wxALL, 5);
  btn_sizer->Add(restore_btn_, 0, wxALL, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(cancel_btn_, 0, wxALL, 5);
  
  main_sizer->AddStretchSpacer(1);
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void BackupRestoreDialog::BindEvents() {
  // Additional event bindings if needed
}

void BackupRestoreDialog::OnBackup(wxCommandEvent& event) {
  // TODO: Implement
}

void BackupRestoreDialog::OnRestore(wxCommandEvent& event) {
  // TODO: Implement
}

void BackupRestoreDialog::OnBrowseSource(wxCommandEvent& event) {
  // TODO: Implement
}

void BackupRestoreDialog::OnBrowseTarget(wxCommandEvent& event) {
  // TODO: Implement
}

void BackupRestoreDialog::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

}  // namespace scratchrobin::ui
