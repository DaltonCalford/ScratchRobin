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
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/notebook.h>

namespace scratchrobin::ui {

/**
 * Query Profiler Dialog
 * Dialog for profiling SQL queries and analyzing performance
 */
class QueryProfilerDialog : public wxDialog {
 public:
  QueryProfilerDialog(wxWindow* parent);
  ~QueryProfilerDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnRunProfile(wxCommandEvent& event);
  void OnClear(wxCommandEvent& event);
  void OnExport(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxTextCtrl* query_text_{nullptr};
  wxNotebook* results_notebook_{nullptr};
  wxListCtrl* timing_list_{nullptr};
  wxListCtrl* plan_list_{nullptr};
  wxTextCtrl* stats_text_{nullptr};
  wxButton* run_btn_{nullptr};
  wxButton* clear_btn_{nullptr};
  wxButton* export_btn_{nullptr};
  wxButton* close_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
