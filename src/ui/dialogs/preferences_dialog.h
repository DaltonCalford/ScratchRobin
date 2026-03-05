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

namespace scratchrobin::ui {

/**
 * Preferences Dialog
 * Application settings and preferences
 */
class PreferencesDialog : public wxDialog {
 public:
  explicit PreferencesDialog(wxWindow* parent);
  ~PreferencesDialog() override;

 private:
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  void OnOK(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);
  void OnApply(wxCommandEvent& event);

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
