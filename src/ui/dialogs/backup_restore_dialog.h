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
#include <wx/button.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>

namespace scratchrobin::ui {

/**
 * Backup/Restore Dialog
 * Dialog for database backup and restore operations
 */
class BackupRestoreDialog : public wxDialog {
 public:
  BackupRestoreDialog(wxWindow* parent);
  ~BackupRestoreDialog() override;

 private:
  // UI Setup
  void CreateControls();
  void SetupLayout();
  void BindEvents();

  // Event handlers
  void OnBackup(wxCommandEvent& event);
  void OnRestore(wxCommandEvent& event);
  void OnBrowseSource(wxCommandEvent& event);
  void OnBrowseTarget(wxCommandEvent& event);
  void OnCancel(wxCommandEvent& event);

  // UI Components
  wxTextCtrl* source_path_{nullptr};
  wxTextCtrl* target_path_{nullptr};
  wxRadioBox* operation_type_{nullptr};
  wxCheckBox* verbose_check_{nullptr};
  wxCheckBox* metadata_only_{nullptr};
  wxButton* browse_source_btn_{nullptr};
  wxButton* browse_target_btn_{nullptr};
  wxButton* backup_btn_{nullptr};
  wxButton* restore_btn_{nullptr};
  wxButton* cancel_btn_{nullptr};

  wxDECLARE_EVENT_TABLE();
};

}  // namespace scratchrobin::ui
