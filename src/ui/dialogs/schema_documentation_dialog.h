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
#include <wx/html/htmlwin.h>
#include <wx/button.h>
#include <wx/choice.h>

namespace scratchrobin::ui {

/**
 * Schema Documentation Dialog
 * Dialog for viewing and generating schema documentation
 */
class SchemaDocumentationDialog : public wxDialog {
 public:
  SchemaDocumentationDialog(wxWindow* parent);
  ~SchemaDocumentationDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnGenerate(wxCommandEvent& event);
  void OnExport(wxCommandEvent& event);
  void OnTreeSelection(wxTreeEvent& event);
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxTreeCtrl* schema_tree_{nullptr};
  wxHtmlWindow* html_view_{nullptr};
  wxChoice* format_choice_{nullptr};
  wxButton* generate_btn_{nullptr};
  wxButton* export_btn_{nullptr};
  wxButton* close_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
