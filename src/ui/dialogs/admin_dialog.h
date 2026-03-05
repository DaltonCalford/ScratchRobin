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
#include <wx/button.h>

namespace scratchrobin::ui {

/**
 * Admin Dialog
 * Dialog for database administration tasks
 */
class AdminDialog : public wxDialog {
 public:
  AdminDialog(wxWindow* parent);
  ~AdminDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxListCtrl* admin_list_{nullptr};
  wxButton* close_button_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
