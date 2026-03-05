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
 * Connection Manager Dialog
 * Dialog for managing saved database connections
 */
class ConnectionManagerDialog : public wxDialog {
 public:
  ConnectionManagerDialog(wxWindow* parent);
  ~ConnectionManagerDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnAdd(wxCommandEvent& event);
  void OnEdit(wxCommandEvent& event);
  void OnDelete(wxCommandEvent& event);
  void OnConnect(wxCommandEvent& event);
  void OnClose(wxCommandEvent& event);

  // UI Components
  wxListCtrl* connection_list_{nullptr};
  wxButton* add_btn_{nullptr};
  wxButton* edit_btn_{nullptr};
  wxButton* delete_btn_{nullptr};
  wxButton* connect_btn_{nullptr};
  wxButton* close_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
