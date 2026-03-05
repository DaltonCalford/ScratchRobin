/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "schema_documentation_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_SCHEMA_TREE = wxID_HIGHEST + 1,
  ID_GENERATE,
  ID_EXPORT,
  ID_FORMAT_CHOICE,
};

wxBEGIN_EVENT_TABLE(SchemaDocumentationDialog, wxDialog)
  EVT_BUTTON(ID_GENERATE, SchemaDocumentationDialog::OnGenerate)
  EVT_BUTTON(ID_EXPORT, SchemaDocumentationDialog::OnExport)
  EVT_TREE_SEL_CHANGED(ID_SCHEMA_TREE, SchemaDocumentationDialog::OnTreeSelection)
  EVT_BUTTON(wxID_CLOSE, SchemaDocumentationDialog::OnClose)
wxEND_EVENT_TABLE()

SchemaDocumentationDialog::SchemaDocumentationDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Schema Documentation"), wxDefaultPosition,
               wxSize(900, 650)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

SchemaDocumentationDialog::~SchemaDocumentationDialog() {
  // Cleanup handled by wxWidgets
}

void SchemaDocumentationDialog::CreateControls() {
  // Schema tree
  schema_tree_ = new wxTreeCtrl(this, ID_SCHEMA_TREE);
  
  // HTML view
  html_view_ = new wxHtmlWindow(this, wxID_ANY);
  
  // Format choice
  format_choice_ = new wxChoice(this, ID_FORMAT_CHOICE);
  format_choice_->Append(_("HTML"));
  format_choice_->Append(_("Markdown"));
  format_choice_->Append(_("PDF"));
  format_choice_->SetSelection(0);
  
  // Buttons
  generate_btn_ = new wxButton(this, ID_GENERATE, _("&Generate"));
  export_btn_ = new wxButton(this, ID_EXPORT, _("&Export..."));
  close_btn_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void SchemaDocumentationDialog::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Splitter-like layout: tree on left, html on right
  wxBoxSizer* content_sizer = new wxBoxSizer(wxHORIZONTAL);
  
  // Tree panel
  wxStaticBoxSizer* tree_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Schema Objects"));
  tree_sizer->Add(schema_tree_, 1, wxEXPAND | wxALL, 5);
  content_sizer->Add(tree_sizer, 1, wxEXPAND | wxALL, 5);
  
  // HTML panel
  wxStaticBoxSizer* html_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Documentation Preview"));
  html_sizer->Add(html_view_, 1, wxEXPAND | wxALL, 5);
  content_sizer->Add(html_sizer, 2, wxEXPAND | wxALL, 5);
  
  main_sizer->Add(content_sizer, 1, wxEXPAND);
  
  // Options and buttons
  wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
  
  // Format selection
  bottom_sizer->Add(new wxStaticText(this, wxID_ANY, _("Format:")),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  bottom_sizer->Add(format_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
  bottom_sizer->Add(generate_btn_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  bottom_sizer->Add(export_btn_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  bottom_sizer->AddStretchSpacer(1);
  bottom_sizer->Add(close_btn_, 0, wxALIGN_CENTER_VERTICAL);
  
  main_sizer->Add(bottom_sizer, 0, wxEXPAND | wxALL, 10);
  
  SetSizer(main_sizer);
}

void SchemaDocumentationDialog::BindEvents() {
  // Additional event bindings if needed
}

void SchemaDocumentationDialog::OnGenerate(wxCommandEvent& event) {
  // TODO: Implement
}

void SchemaDocumentationDialog::OnExport(wxCommandEvent& event) {
  // TODO: Implement
}

void SchemaDocumentationDialog::OnTreeSelection(wxTreeEvent& event) {
  // TODO: Implement
}

void SchemaDocumentationDialog::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
