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
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/gauge.h>

namespace scratchrobin::ui {

/**
 * Migration Dialog
 * Dialog for running database migrations
 */
class MigrationDialog : public wxDialog {
 public:
  MigrationDialog(wxWindow* parent);
  ~MigrationDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnRunMigration(wxCommandEvent& event);
  void OnRollback(wxCommandEvent& event);
  void OnRefresh(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxListCtrl* migration_list_{nullptr};
  wxTextCtrl* log_text_{nullptr};
  wxGauge* progress_bar_{nullptr};
  wxButton* run_btn_{nullptr};
  wxButton* rollback_btn_{nullptr};
  wxButton* refresh_btn_{nullptr};
  wxButton* close_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
