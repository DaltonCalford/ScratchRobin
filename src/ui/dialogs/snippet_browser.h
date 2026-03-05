/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <wx/dialog.h>
#include <wx/treectrl.h>
#include <wx/stc/stc.h>
#include <wx/button.h>
#include <wx/textctrl.h>

namespace scratchrobin::ui {

/**
 * Snippet Browser Dialog
 * Dialog for browsing and managing SQL code snippets
 */
class SnippetBrowser : public wxDialog {
 public:
  SnippetBrowser(wxWindow* parent);
  ~SnippetBrowser() override;

  // Get the selected snippet content
  wxString GetSelectedSnippet() const;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnInsert(wxCommandEvent& event);
  void OnNewSnippet(wxCommandEvent& event);
  void OnEditSnippet(wxCommandEvent& event);
  void OnDeleteSnippet(wxCommandEvent& event);
  void OnSearch(wxCommandEvent& event);
  void OnTreeSelection(wxTreeEvent& event);
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxTextCtrl* search_ctrl_{nullptr};
  wxTreeCtrl* snippet_tree_{nullptr};
  wxStyledTextCtrl* snippet_editor_{nullptr};
  wxButton* insert_btn_{nullptr};
  wxButton* new_btn_{nullptr};
  wxButton* edit_btn_{nullptr};
  wxButton* delete_btn_{nullptr};
  wxButton* close_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
