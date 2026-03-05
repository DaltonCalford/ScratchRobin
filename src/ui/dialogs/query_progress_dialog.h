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
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace scratchrobin::ui {

/**
 * Query Progress Dialog
 * Shows progress of long-running queries with cancel option
 */
class QueryProgressDialog : public wxDialog {
 public:
  QueryProgressDialog(wxWindow* parent, const wxString& title = _("Query Progress"));
  ~QueryProgressDialog() override;

  // Update progress
  void SetProgress(int percent, const wxString& message = wxEmptyString);
  void SetIndeterminate();
  
  // Check if user cancelled
  bool WasCancelled() const;
  
  // Complete the operation
  void Complete(const wxString& message = wxEmptyString);

 private:
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  void OnCancel(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);

  wxGauge* progress_bar_{nullptr};
  wxStaticText* message_label_{nullptr};
  wxButton* cancel_btn_{nullptr};
  
  bool was_cancelled_{false};
  bool is_complete_{false};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
