/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "data_import_export_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_FORMAT_CHOICE = wxID_HIGHEST + 1,
  ID_TABLE_CHOICE,
  ID_BROWSE_SOURCE,
  ID_BROWSE_TARGET,
  ID_IMPORT,
  ID_EXPORT,
};

wxBEGIN_EVENT_TABLE(DataImportExportDialog, wxDialog)
  EVT_BUTTON(ID_BROWSE_SOURCE, DataImportExportDialog::OnBrowseSource)
  EVT_BUTTON(ID_BROWSE_TARGET, DataImportExportDialog::OnBrowseTarget)
  EVT_BUTTON(ID_IMPORT, DataImportExportDialog::OnImport)
  EVT_BUTTON(ID_EXPORT, DataImportExportDialog::OnExport)
  EVT_BUTTON(wxID_CANCEL, DataImportExportDialog::OnCancel)
wxEND_EVENT_TABLE()

DataImportExportDialog::DataImportExportDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Data Import / Export"), wxDefaultPosition,
               wxSize(550, 450)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

DataImportExportDialog::~DataImportExportDialog() {
  // Cleanup handled by wxWidgets
}

void DataImportExportDialog::CreateControls() {
  // Format choice
  format_choice_ = new wxChoice(this, ID_FORMAT_CHOICE);
  format_choice_->Append(_("CSV"));
  format_choice_->Append(_("JSON"));
  format_choice_->Append(_("XML"));
  format_choice_->Append(_("SQL INSERT"));
  format_choice_->SetSelection(0);
  
  // Table choice
  table_choice_ = new wxChoice(this, ID_TABLE_CHOICE);
  table_choice_->Append(_("-- Select table --"));
  table_choice_->SetSelection(0);
  
  // Source path
  source_path_ = new wxTextCtrl(this, wxID_ANY);
  browse_source_btn_ = new wxButton(this, ID_BROWSE_SOURCE, _("Browse..."));
  
  // Target path
  target_path_ = new wxTextCtrl(this, wxID_ANY);
  browse_target_btn_ = new wxButton(this, ID_BROWSE_TARGET, _("Browse..."));
  
  // Options
  header_check_ = new wxCheckBox(this, wxID_ANY, _("Include header row"));
  overwrite_check_ = new wxCheckBox(this, wxID_ANY, _("Overwrite existing"));
  
  // Buttons
  import_btn_ = new wxButton(this, ID_IMPORT, _("&Import"));
  export_btn_ = new wxButton(this, ID_EXPORT, _("&Export"));
  cancel_btn_ = new wxButton(this, wxID_CANCEL, _("&Cancel"));
}

void DataImportExportDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Format selection
  wxBoxSizer* format_sizer = new wxBoxSizer(wxHORIZONTAL);
  format_sizer->Add(new wxStaticText(this, wxID_ANY, _("Format:")),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  format_sizer->Add(format_choice_, 1, wxEXPAND);
  main_sizer->Add(format_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Table selection
  wxBoxSizer* table_sizer = new wxBoxSizer(wxHORIZONTAL);
  table_sizer->Add(new wxStaticText(this, wxID_ANY, _("Table:")),
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  table_sizer->Add(table_choice_, 1, wxEXPAND);
  main_sizer->Add(table_sizer, 0, wxEXPAND | wxALL, 5);
  
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
  options_sizer->Add(header_check_, 0, wxALL, 5);
  options_sizer->Add(overwrite_check_, 0, wxALL, 5);
  main_sizer->Add(options_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(import_btn_, 0, wxALL, 5);
  btn_sizer->Add(export_btn_, 0, wxALL, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(cancel_btn_, 0, wxALL, 5);
  
  main_sizer->AddStretchSpacer(1);
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void DataImportExportDialog::BindEvents() {
  // Additional event bindings if needed
}

void DataImportExportDialog::OnImport(wxCommandEvent& event) {
  // TODO: Implement
}

void DataImportExportDialog::OnExport(wxCommandEvent& event) {
  // TODO: Implement
}

void DataImportExportDialog::OnBrowseSource(wxCommandEvent& event) {
  // TODO: Implement
}

void DataImportExportDialog::OnBrowseTarget(wxCommandEvent& event) {
  // TODO: Implement
}

void DataImportExportDialog::OnCancel(wxCommandEvent& event) {
  EndModal(wxID_CANCEL);
}

}  // namespace scratchrobin::ui
