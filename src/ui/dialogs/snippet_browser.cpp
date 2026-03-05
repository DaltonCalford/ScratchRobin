/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "snippet_browser.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

// Control IDs
enum {
  ID_SEARCH_CTRL = wxID_HIGHEST + 1,
  ID_SNIPPET_TREE,
  ID_SNIPPET_EDITOR,
  ID_INSERT,
  ID_NEW_SNIPPET,
  ID_EDIT_SNIPPET,
  ID_DELETE_SNIPPET,
};

wxBEGIN_EVENT_TABLE(SnippetBrowser, wxDialog)
  EVT_BUTTON(ID_INSERT, SnippetBrowser::OnInsert)
  EVT_BUTTON(ID_NEW_SNIPPET, SnippetBrowser::OnNewSnippet)
  EVT_BUTTON(ID_EDIT_SNIPPET, SnippetBrowser::OnEditSnippet)
  EVT_BUTTON(ID_DELETE_SNIPPET, SnippetBrowser::OnDeleteSnippet)
  EVT_TEXT_ENTER(ID_SEARCH_CTRL, SnippetBrowser::OnSearch)
  EVT_TREE_SEL_CHANGED(ID_SNIPPET_TREE, SnippetBrowser::OnTreeSelection)
  EVT_BUTTON(wxID_CLOSE, SnippetBrowser::OnClose)
wxEND_EVENT_TABLE()

SnippetBrowser::SnippetBrowser(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("SQL Snippet Browser"), wxDefaultPosition,
               wxSize(800, 600)) {
  CreateControls();
  SetupLayout();
  BindEvents();
  
  // Center on parent
  CentreOnParent();
}

SnippetBrowser::~SnippetBrowser() {
  // Cleanup handled by wxWidgets
}

void SnippetBrowser::CreateControls() {
  // Search control
  search_ctrl_ = new wxTextCtrl(this, ID_SEARCH_CTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
  search_ctrl_->SetHint(_("Search snippets..."));

  
  // Snippet tree
  snippet_tree_ = new wxTreeCtrl(this, ID_SNIPPET_TREE);
  
  // Snippet editor (read-only for preview)
  snippet_editor_ = new wxStyledTextCtrl(this, ID_SNIPPET_EDITOR);
  snippet_editor_->SetReadOnly(true);
  
  // Buttons
  insert_btn_ = new wxButton(this, ID_INSERT, _("&Insert"));
  new_btn_ = new wxButton(this, ID_NEW_SNIPPET, _("&New..."));
  edit_btn_ = new wxButton(this, ID_EDIT_SNIPPET, _("&Edit..."));
  delete_btn_ = new wxButton(this, ID_DELETE_SNIPPET, _("&Delete"));
  close_btn_ = new wxButton(this, wxID_CLOSE, _("&Close"));
}

void SnippetBrowser::SetupLayout() {
  wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
  
  // Search bar
  wxBoxSizer* search_sizer = new wxBoxSizer(wxHORIZONTAL);
  search_sizer->Add(new wxStaticText(this, wxID_ANY, _("Search:")),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
  search_sizer->Add(search_ctrl_, 1, wxEXPAND);
  main_sizer->Add(search_sizer, 0, wxEXPAND | wxALL, 5);
  
  // Content area: tree on left, editor on right
  wxBoxSizer* content_sizer = new wxBoxSizer(wxHORIZONTAL);
  
  // Tree panel
  wxStaticBoxSizer* tree_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Snippets"));
  tree_sizer->Add(snippet_tree_, 1, wxEXPAND | wxALL, 5);
  content_sizer->Add(tree_sizer, 1, wxEXPAND | wxALL, 5);
  
  // Editor panel
  wxStaticBoxSizer* editor_sizer = new wxStaticBoxSizer(
      wxVERTICAL, this, _("Preview"));
  editor_sizer->Add(snippet_editor_, 1, wxEXPAND | wxALL, 5);
  content_sizer->Add(editor_sizer, 2, wxEXPAND | wxALL, 5);
  
  main_sizer->Add(content_sizer, 1, wxEXPAND);
  
  // Button sizer
  wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
  btn_sizer->Add(insert_btn_, 0, wxALL, 5);
  btn_sizer->Add(new_btn_, 0, wxALL, 5);
  btn_sizer->Add(edit_btn_, 0, wxALL, 5);
  btn_sizer->Add(delete_btn_, 0, wxALL, 5);
  btn_sizer->AddStretchSpacer(1);
  btn_sizer->Add(close_btn_, 0, wxALL, 5);
  
  main_sizer->Add(btn_sizer, 0, wxEXPAND | wxALL, 5);
  
  SetSizer(main_sizer);
}

void SnippetBrowser::BindEvents() {
  // Additional event bindings if needed
}

wxString SnippetBrowser::GetSelectedSnippet() const {
  return snippet_editor_->GetText();
}

void SnippetBrowser::OnInsert(wxCommandEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnNewSnippet(wxCommandEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnEditSnippet(wxCommandEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnDeleteSnippet(wxCommandEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnSearch(wxCommandEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnTreeSelection(wxTreeEvent& event) {
  // TODO: Implement
}

void SnippetBrowser::OnClose(wxCommandEvent& event) {
  EndModal(wxID_CLOSE);
}

}  // namespace scratchrobin::ui
