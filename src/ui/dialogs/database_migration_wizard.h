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
#include <wx/wizard.h>
#include <wx/listctrl.h>
#include <wx/combobox.h>
#include <wx/button.h>

namespace scratchrobin::ui {

/**
 * Database Migration Wizard
 * Wizard-style dialog for database migration operations
 */
class DatabaseMigrationWizard : public wxDialog {
 public:
  DatabaseMigrationWizard(wxWindow* parent);
  ~DatabaseMigrationWizard() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnNext(wxCommandEvent& event);
  void OnPrevious(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);
  void OnFinish(wxCommandEvent& event);

  // Wizard pages
  void ShowPage(int page);

  // UI Components
  wxPanel* page_container_{nullptr};
  wxButton* prev_btn_{nullptr};
  wxButton* next_btn_{nullptr};
  wxButton* finish_btn_{nullptr};
  wxButton* cancel_btn_{nullptr};

  // Current page
  int current_page_{0};
  static constexpr int kTotalPages = 4;

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
